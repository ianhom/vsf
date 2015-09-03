#include "app_cfg.h"
#include "app_type.h"

#include "vsfusbh.h"
#include "class_driver\HUB\vsfusbh_hub.h"

#define USB_MAX_DEVICE				127

#define vsfusbh_check_ret_value()	\
	do {\
		if (err != VSFERR_NONE)\
		{\
			vsfusbh_free_buffer(usbh);\
			return err;\
		}\
	} while (0)

#define vsfusbh_check_urb_status()	\
	do {\
		if (dev->status != VSFERR_NONE)\
		{\
			vsfusbh_free_buffer(usbh);\
			return usbh->vsfurb.status;\
		}\
	} while (0)

struct vsfusbh_class_drv_list
{
	const struct vsfusbh_class_drv_t *drv;
	struct sllist list;
};

static uint32_t ffz(uint32_t x)
{
	uint32_t i;
	for (i = 0; i < 32; i++)
	{
		if (0 == ((1 << i) & x))
			return i + 1;
	}
	return 0;
}

static uint16_t find_first_clear_bit(uint32_t *data, uint32_t bit_size)
{
	uint32_t temp;
	uint16_t imax, i;

	imax = (bit_size + 31) >> 5;
	for (i = 0; i < imax; i++)
	{
		temp = data[i];
		if ((temp & 0xffffffff) != 0xffffffff)
		{
			temp = (i << 5) + ffz(temp);
			return temp <= bit_size ? temp : 0;
		}
	}
	return 0;
}

static void set_bit(uint32_t *data, uint32_t bit)
{
	while (bit > 32)
	{
		data++;
		bit -= 32;
	}
	*data |= (1 << (bit - 1));
}

static void clear_bit(uint32_t *data, uint32_t bit)
{
	while (bit > 32)
	{
		data++;
		bit -= 32;
	}
	*data &= ~(1 << (bit - 1));
}

struct vsfusbh_device_t *vsfusbh_alloc_device(struct vsfusbh_t *usbh)
{
	vsf_err_t err;
	struct vsfusbh_device_t *dev;

	dev = vsf_bufmgr_malloc(sizeof(struct vsfusbh_device_t));
	if (NULL == dev)
		return NULL;
	memset(dev, 0, sizeof(struct vsfusbh_device_t));

	err = usbh->hcd->alloc_device(usbh->hcd_data, dev);
	if (err != VSFERR_NONE)
	{
		vsf_bufmgr_free(dev);
		return NULL;
	}

	dev->devnum = find_first_clear_bit(usbh->device_bitmap, USB_MAX_DEVICE);
	if (dev->devnum == 0)
	{
		vsf_bufmgr_free(dev);
		return NULL;
	}
	set_bit(usbh->device_bitmap, dev->devnum);
	return dev;
}

void vsfusbh_free_device(struct vsfusbh_t *usbh, struct vsfusbh_device_t *dev)
{
	clear_bit(usbh->device_bitmap, dev->devnum);

	usbh->hcd->free_device(usbh->hcd_data, dev);

	if (dev->config != NULL)
	{
		vsf_bufmgr_free(dev->config);
		dev->config = NULL;
	}
	vsf_bufmgr_free(dev);
}

vsf_err_t vsfusbh_add_device(struct vsfusbh_t *usbh, struct vsfusbh_device_t *dev)
{
	struct sllist *list = &usbh->drv_list;
	const struct vsfusbh_class_drv_t *drv;
	struct vsfusbh_class_drv_list *drv_list;
	struct vsfusbh_class_data_t *cdata;

	while (list->next)
	{
		list = list->next;
		drv_list = sllist_get_container(list, struct vsfusbh_class_drv_list, list);
		drv = drv_list->drv;
		if (VSFERR_NONE == drv->match(dev))
		{
			cdata = (struct vsfusbh_class_data_t *)drv->init(usbh, dev);
			if (NULL != cdata)
			{
				cdata->drv = drv;
				sllist_append(&usbh->dev_list, &cdata->list);
				return VSFERR_NONE;
			}
			return VSFERR_FAIL;
		}
	}
	return VSFERR_FAIL;
}

void vsfusbh_remove_device(struct vsfusbh_t *usbh, struct vsfusbh_device_t *dev)
{
	struct sllist *list = &usbh->dev_list;
	struct vsfusbh_class_data_t *cdata;
	struct sllist *next;
	int i;

	while (list->next)
	{
		next = list->next;
		cdata = sllist_get_container(next, struct vsfusbh_class_data_t, list);
		if (cdata->dev == dev)
		{
			for (i = 0; i < dev->maxchild; i++)
			{
				if (dev->children[i])
					vsfusbh_remove_device(usbh, dev->children[i]);
			}
			sllist_delete_next(list);
			cdata->drv->free(dev);
			vsfusbh_free_device(usbh, dev);
			break;
		}
		list = next;
	}
}


vsf_err_t vsfusbh_submit_urb(struct vsfusbh_t *usbh, struct vsfusbh_urb_t *vsfurb)
{
	return usbh->hcd->submit_urb(usbh->hcd_data, vsfurb);
}

vsf_err_t vsfusbh_unlink_urb(struct vsfusbh_t *usbh, struct vsfusbh_urb_t *vsfurb)
{
	// TODO
	return usbh->hcd->unlink_urb(usbh->hcd_data, vsfurb, NULL);
}

vsf_err_t vsfusbh_relink_urb(struct vsfusbh_t *usbh, struct vsfusbh_urb_t *vsfurb)
{
	return usbh->hcd->relink_urb(usbh->hcd_data, vsfurb);
}

vsf_err_t vsfusbh_control_msg(struct vsfusbh_t *usbh, struct vsfusbh_urb_t *vsfurb,
	uint8_t requesttype, uint8_t request, uint16_t value, uint16_t index)
{
	vsfurb->timeout = 200;

	vsfurb->setup_packet.requesttype = requesttype;
	vsfurb->setup_packet.request = request;
	vsfurb->setup_packet.value = value;
	vsfurb->setup_packet.index = index;
	vsfurb->setup_packet.length = vsfurb->transfer_length;

	return vsfusbh_submit_urb(usbh, vsfurb);
}


static void vsfusbh_set_maxpacket_ep(struct vsfusbh_device_t *dev)
{
	int i, b;

	for (i = 0; i < dev->actconfig->bNumInterfaces; i++)
	{
		struct usb_interface_t *ifp = dev->actconfig->interface + i;
		struct usb_interface_descriptor_t *as =
			ifp->interface_desc + ifp->act_altsetting;
		struct usb_endpoint_descriptor_t *ep = as->ep_desc;
		int e;

		for (e = 0; e < as->bNumEndpoints; e++)
		{
			b = ep[e].bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;
			if ((ep[e].bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
				USB_ENDPOINT_XFER_CONTROL)		/* Control => bidirectional */
			{
				dev->epmaxpacketout[b] = ep[e].wMaxPacketSize;
				dev->epmaxpacketin[b] = ep[e].wMaxPacketSize;
			}
			else if (usb_endpoint_out(ep[e].bEndpointAddress))
			{
				if (ep[e].wMaxPacketSize > dev->epmaxpacketout[b])
					dev->epmaxpacketout[b] = ep[e].wMaxPacketSize;
			}
			else
			{
				if (ep[e].wMaxPacketSize > dev->epmaxpacketin[b])
					dev->epmaxpacketin[b] = ep[e].wMaxPacketSize;
			}
		}
	}
}


vsf_err_t vsfusbh_set_address(struct vsfusbh_t *usbh,
struct vsfusbh_urb_t *vsfurb)
{
	vsfurb->pipe = usb_snddefctrl(vsfurb->vsfdev);
	return vsfusbh_control_msg(usbh, vsfurb, USB_DIR_OUT, USB_REQ_SET_ADDRESS,
		vsfurb->vsfdev->devnum, 0);
}
vsf_err_t vsfusbh_get_descriptor(struct vsfusbh_t *usbh,
struct vsfusbh_urb_t *vsfurb, uint8_t type, uint8_t index)
{
	vsfurb->pipe = usb_rcvctrlpipe(vsfurb->vsfdev, 0);
	return vsfusbh_control_msg(usbh, vsfurb, USB_DIR_IN, USB_REQ_GET_DESCRIPTOR,
		(type << 8) + index, index);
}
vsf_err_t vsfusbh_set_configuration(struct vsfusbh_t *usbh,
struct vsfusbh_urb_t *vsfurb, uint8_t configuration)
{
	vsfurb->pipe = usb_sndctrlpipe(vsfurb->vsfdev, 0);
	return vsfusbh_control_msg(usbh, vsfurb, USB_DIR_OUT,
		USB_REQ_SET_CONFIGURATION, configuration, 0);
}
vsf_err_t vsfusbh_set_interface(struct vsfusbh_t *usbh,
struct vsfusbh_urb_t *vsfurb, uint16_t interface, uint16_t alternate)
{
	vsfurb->pipe = usb_sndctrlpipe(vsfurb->vsfdev, 0);
	return vsfusbh_control_msg(usbh, vsfurb, USB_RECIP_INTERFACE,
		USB_REQ_SET_INTERFACE, alternate, interface);
}


static int parse_endpoint(struct usb_endpoint_descriptor_t *endpoint,
	unsigned char *buffer, int size)
{
	struct usb_descriptor_header_t *header;
	int parsed = 0, numskipped;

	header = (struct usb_descriptor_header_t *)buffer;

	/* Everything should be fine being passed into here, but we sanity */
	/* check JIC */
	if (header->bLength > size)
	{
		return -1;
	}

	if (header->bDescriptorType != USB_DT_ENDPOINT)
	{
		return parsed;
	}

	if (header->bLength == USB_DT_ENDPOINT_AUDIO_SIZE)
		memcpy(endpoint, buffer, USB_DT_ENDPOINT_AUDIO_SIZE);
	else
		memcpy(endpoint, buffer, USB_DT_ENDPOINT_SIZE);

	buffer += header->bLength;
	size -= header->bLength;
	parsed += header->bLength;

	/* Skip over the rest of the Class Specific or Vendor Specific */
	/* descriptors */
	numskipped = 0;
	while (size >= sizeof(struct usb_descriptor_header_t))
	{
		header = (struct usb_descriptor_header_t *)buffer;

		if (header->bLength < 2)
		{
			return -1;
		}

		/* If we find another "proper" descriptor then we're done */
		if ((header->bDescriptorType == USB_DT_ENDPOINT) ||
			(header->bDescriptorType == USB_DT_INTERFACE) ||
			(header->bDescriptorType == USB_DT_CONFIG) ||
			(header->bDescriptorType == USB_DT_DEVICE))
			break;

		numskipped++;

		buffer += header->bLength;
		size -= header->bLength;
		parsed += header->bLength;
	}

	return parsed;
}

static int parse_interface(struct usb_interface_t *interface, unsigned char *buffer, int size)
{
	int i, numskipped, retval, parsed = 0;
	struct usb_descriptor_header_t *header;
	struct usb_interface_descriptor_t *ifp;

	interface->act_altsetting = 0;
	interface->num_altsetting = 0;
	interface->max_altsetting = USB_ALTSETTINGALLOC;

	interface->interface_desc = vsf_bufmgr_malloc\
		(sizeof(struct usb_interface_descriptor_t) * interface->max_altsetting);

	if (!interface->interface_desc)
	{
		return -1;
	}

	while (size > 0)
	{
		if (interface->num_altsetting >= interface->max_altsetting)
		{
			void *ptr;
			int oldmas;

			oldmas = interface->max_altsetting;
			interface->max_altsetting += USB_ALTSETTINGALLOC;
			if (interface->max_altsetting > USB_MAXALTSETTING)
			{
				return -1;
			}

			ptr = interface->interface_desc;
			interface->interface_desc = vsf_bufmgr_malloc\
				(sizeof(struct usb_interface_descriptor_t) *
			interface->max_altsetting);
			if (!interface->interface_desc)
			{
				interface->interface_desc = ptr;
				return -1;
			}
			memcpy(interface->interface_desc, ptr,
				sizeof(struct usb_interface_descriptor_t) * oldmas);
			vsf_bufmgr_free(ptr);
		}

		ifp = interface->interface_desc + interface->num_altsetting;
		interface->num_altsetting++;

		memcpy(ifp, buffer, USB_DT_INTERFACE_SIZE);

		/* Skip over the interface */
		buffer += ifp->bLength;
		parsed += ifp->bLength;
		size -= ifp->bLength;

		numskipped = 0;

		/* Skip over any interface, class or vendor descriptors */
		while (size >= sizeof(struct usb_descriptor_header_t))
		{
			header = (struct usb_descriptor_header_t *)buffer;

			if (header->bLength < 2)
			{
				return -1;
			}

			/* If we find another "proper" descriptor then we're done */
			if ((header->bDescriptorType == USB_DT_INTERFACE) ||
				(header->bDescriptorType == USB_DT_ENDPOINT) ||
				(header->bDescriptorType == USB_DT_CONFIG) ||
				(header->bDescriptorType == USB_DT_DEVICE))
				break;

			numskipped++;

			buffer += header->bLength;
			parsed += header->bLength;
			size -= header->bLength;
		}

		/* Did we hit an unexpected descriptor? */
		header = (struct usb_descriptor_header_t *)buffer;
		if ((size >= sizeof(struct usb_descriptor_header_t)) &&
			((header->bDescriptorType == USB_DT_CONFIG) ||
			(header->bDescriptorType == USB_DT_DEVICE)))
			return parsed;

		if (ifp->bNumEndpoints > USB_MAXENDPOINTS)
		{
			return -1;
		}

		if (ifp->bNumEndpoints != 0)
		{
			ifp->ep_desc = vsf_bufmgr_malloc(ifp->bNumEndpoints *
				sizeof(struct usb_endpoint_descriptor_t));
			if (!ifp->ep_desc)
			{
				return -1;
			}

			memset(ifp->ep_desc, 0, ifp->bNumEndpoints *
				sizeof(struct usb_endpoint_descriptor_t));

			for (i = 0; i < ifp->bNumEndpoints; i++)
			{
				header = (struct usb_descriptor_header_t *)buffer;

				if (header->bLength > size)
				{
					return -1;
				}

				retval = parse_endpoint(ifp->ep_desc + i, buffer, size);
				if (retval < 0)
					return retval;

				buffer += retval;
				parsed += retval;
				size -= retval;
			}
		}

		/* We check to see if it's an alternate to this one */
		ifp = (struct usb_interface_descriptor_t *)buffer;
		if (size < USB_DT_INTERFACE_SIZE ||
			ifp->bDescriptorType != USB_DT_INTERFACE ||
			!ifp->bAlternateSetting)
			return parsed;
	}

	return parsed;
}

static vsf_err_t parse_configuration(struct usb_config_descriptor_t *config,
	uint8_t *buffer)
{
	int i, retval, size;
	struct usb_descriptor_header_t *header;

	memcpy(config, buffer, USB_DT_CONFIG_SIZE);
	size = config->wTotalLength;

	if (config->bNumInterfaces > USB_MAXINTERFACES)
		return VSFERR_FAIL;

	config->interface = vsf_bufmgr_malloc(sizeof(struct usb_interface_t) *
		config->bNumInterfaces);
	if (config->interface == NULL)
		return VSFERR_NOT_ENOUGH_RESOURCES;
	memset(config->interface, 0, sizeof(struct usb_interface_t) *
		config->bNumInterfaces);

	buffer += config->bLength;
	size -= config->bLength;

	for (i = 0; i < config->bNumInterfaces; i++)
	{
		int numskipped;

		/* Skip over the rest of the Class Specific or Vendor */
		/* Specific descriptors */
		numskipped = 0;
		while (size >= sizeof(struct usb_descriptor_header_t))
		{
			header = (struct usb_descriptor_header_t *)buffer;

			if ((header->bLength > size) || (header->bLength < 2))
			{
				return VSFERR_FAIL;
			}

			/* If we find another "proper" descriptor then we're done */
			if ((header->bDescriptorType == USB_DT_ENDPOINT) ||
				(header->bDescriptorType == USB_DT_INTERFACE) ||
				(header->bDescriptorType == USB_DT_CONFIG) ||
				(header->bDescriptorType == USB_DT_DEVICE))
				break;

			numskipped++;
			buffer += header->bLength;
			size -= header->bLength;
		}

		retval = parse_interface(config->interface + i, buffer, size);
		if (retval < 0)
			return VSFERR_FAIL;

		buffer += retval;
		size -= retval;
	}

	return VSFERR_NONE;
}


vsf_err_t vsfusbh_probe_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	vsf_err_t err;
	uint32_t len;
	struct vsfusbh_t *usbh = (struct vsfusbh_t *)pt->user_data;
	struct vsfusbh_urb_t *probe_urb = &usbh->probe_urb;
	struct vsfusbh_device_t *dev = usbh->new_dev;

	vsfsm_pt_begin(pt);

	dev->devnum_temp = dev->devnum;
	dev->devnum = 0;
	dev->epmaxpacketin[0] = 8;
	dev->epmaxpacketout[0] = 8;

	probe_urb->vsfdev = dev;
	probe_urb->sm = &usbh->sm;
	probe_urb->timeout = DEFAULT_TIMEOUT;

	// get 8 bytes device descriptor
	probe_urb->transfer_buffer = &dev->descriptor;
	probe_urb->transfer_length = 8;
	err = vsfusbh_get_descriptor(usbh, probe_urb, USB_DT_DEVICE, 0);
	if (err != VSFERR_NONE)
		return err;
	vsfsm_pt_wfe(pt, VSFSM_EVT_URB_COMPLETE);
	if (probe_urb->status != USB_OK)
		return VSFERR_FAIL;
	dev->epmaxpacketin[0] = dev->descriptor.bMaxPacketSize0;
	dev->epmaxpacketout[0] = dev->descriptor.bMaxPacketSize0;

	// set address
	dev->devnum = dev->devnum_temp;
	probe_urb->transfer_buffer = NULL;
	probe_urb->transfer_length = 0;
	err = vsfusbh_set_address(usbh, probe_urb);
	if (err != VSFERR_NONE)
		return err;
	vsfsm_pt_wfe(pt, VSFSM_EVT_URB_COMPLETE);
	if (probe_urb->status != USB_OK)
		return VSFERR_FAIL;

	vsfsm_pt_delay(pt, 10);

	// get full device descriptor
	probe_urb->transfer_buffer = &dev->descriptor;
	probe_urb->transfer_length = sizeof(dev->descriptor);
	err = vsfusbh_get_descriptor(usbh, probe_urb, USB_DT_DEVICE, 0);
	if (err != VSFERR_NONE)
		return err;
	vsfsm_pt_wfe(pt, VSFSM_EVT_URB_COMPLETE);
	if (probe_urb->status != USB_OK)
		return VSFERR_FAIL;

	// get 9 bytes configuration
	probe_urb->transfer_buffer = vsf_bufmgr_malloc(9);
	if (probe_urb->transfer_buffer == NULL)
		return VSFERR_FAIL;
	memset(probe_urb->transfer_buffer, 0, 9);
	probe_urb->transfer_length = 9;
	err = vsfusbh_get_descriptor(usbh, &usbh->probe_urb, USB_DT_CONFIG, 0);
	if (err != VSFERR_NONE)
	{
		vsf_bufmgr_free(probe_urb->transfer_buffer);
		return err;
	}
	vsfsm_pt_wfe(pt, VSFSM_EVT_URB_COMPLETE);
	if (probe_urb->status != USB_OK)
	{
		vsf_bufmgr_free(probe_urb->transfer_buffer);
		return VSFERR_FAIL;
	}
	// get wTotalLength
	len = GET_U16_LSBFIRST(&((uint8_t *)(probe_urb->transfer_buffer))[2]);
	vsf_bufmgr_free(probe_urb->transfer_buffer);

	// NOTE: only probe first configuration
	// if need probe all configuration,
	// need malloc(sizeof(config) * bNumConfigurations)
	dev->config = vsf_bufmgr_malloc_aligned\
		(sizeof(struct usb_config_descriptor_t), 4);
	if (dev->config == NULL)
	{
		return VSFERR_FAIL;
	}
	memset(dev->config, 0, sizeof(struct usb_config_descriptor_t));

	// get index 0 configuration
	probe_urb->transfer_buffer = vsf_bufmgr_malloc(len);
	if (probe_urb->transfer_buffer == NULL)
	{
		vsf_bufmgr_free(dev->config);
		dev->config = NULL;
		return VSFERR_FAIL;
	}
	memset(probe_urb->transfer_buffer, 0, len);
	probe_urb->transfer_length = len;
	err = vsfusbh_get_descriptor(usbh, &usbh->probe_urb, USB_DT_CONFIG, 0);
	if (err != VSFERR_NONE)
	{
		vsf_bufmgr_free(probe_urb->transfer_buffer);
		vsf_bufmgr_free(dev->config);
		dev->config = NULL;
		return err;
	}
	vsfsm_pt_wfe(pt, VSFSM_EVT_URB_COMPLETE);
	if (probe_urb->status != USB_OK)
	{
		vsf_bufmgr_free(probe_urb->transfer_buffer);
		vsf_bufmgr_free(dev->config);
		dev->config = NULL;
		return VSFERR_FAIL;
	}
	// get wTotalLength
	len = GET_U16_LSBFIRST(&((uint8_t *)(probe_urb->transfer_buffer))[2]);
	if (probe_urb->actual_length != len)
	{
		vsf_bufmgr_free(probe_urb->transfer_buffer);
		vsf_bufmgr_free(dev->config);
		dev->config = NULL;
		return VSFERR_FAIL;
	}
	err = parse_configuration(dev->config, probe_urb->transfer_buffer);
	vsf_bufmgr_free(probe_urb->transfer_buffer);
	if (err != VSFERR_NONE)
	{
		vsf_bufmgr_free(dev->config);
		dev->config = NULL;
		return err;
	}

	// set the default configuration
	probe_urb->transfer_buffer = NULL;
	probe_urb->transfer_length = 0;
	err = vsfusbh_set_configuration(usbh, probe_urb,
		dev->config->bConfigurationValue);
	if (err != VSFERR_NONE)
		return err;
	vsfsm_pt_wfe(pt, VSFSM_EVT_URB_COMPLETE);
	if (probe_urb->status != USB_OK)
		return VSFERR_FAIL;

	dev->actconfig = dev->config;
	vsfusbh_set_maxpacket_ep(dev);

	vsfsm_pt_end(pt);

	return VSFERR_NONE;
}

static struct vsfsm_state_t *vsfusbh_probe_evt_handler(struct vsfsm_t *sm,
	vsfsm_evt_t evt)
{
	vsf_err_t err;
	struct vsfusbh_t *usbh = (struct vsfusbh_t *)sm->user_data;

	switch (evt)
	{
	case VSFSM_EVT_INIT:
		usbh->dev_probe_pt.thread = vsfusbh_probe_thread;
		usbh->dev_probe_pt.user_data = usbh;
		usbh->dev_probe_pt.state = 0;
		usbh->dev_probe_pt.sm = sm;
	default:
		err = usbh->dev_probe_pt.thread(&usbh->dev_probe_pt, evt);
		if (VSFERR_NONE == err)
		{
			err = vsfusbh_add_device(usbh, usbh->new_dev);
			if (err < 0)
			{
				vsfusbh_free_device(usbh, usbh->new_dev);
			}
			usbh->new_dev = NULL;
		}
		else if (err < 0)
		{
			vsfusbh_free_device(usbh, usbh->new_dev);
			usbh->new_dev = NULL;
		}
		break;
	}
	return NULL;
}

static struct vsfsm_state_t *vsfusbh_init_evt_handler(struct vsfsm_t *sm,
	vsfsm_evt_t evt)
{
	vsf_err_t err;
	struct vsfusbh_t *usbh = (struct vsfusbh_t *)sm->user_data;

	switch (evt)
	{
	case VSFSM_EVT_ENTER:
		break;
	case VSFSM_EVT_INIT:
		usbh->dev_probe_pt.thread = NULL;
		sllist_init_node(usbh->drv_list);
		sllist_init_node(usbh->dev_list);
		usbh->hcd_init_pt.thread = usbh->hcd->init_thread;
		usbh->hcd_init_pt.user_data = usbh;
		usbh->hcd_init_pt.state = 0;
		usbh->hcd_init_pt.sm = sm;
	default:
		err = usbh->hcd_init_pt.thread(&usbh->hcd_init_pt, evt);
		if (VSFERR_NONE == err)
		{
			sm->init_state.evt_handler = vsfusbh_probe_evt_handler;
			usbh->rh_dev = vsfusbh_alloc_device(usbh);
			if (NULL == usbh->rh_dev)
			{
				// error
				usbh->hcd_init_pt.thread = NULL;
			}
			else
			{
				usbh->new_dev = usbh->rh_dev;
				vsfsm_post_evt_pending(&usbh->sm, VSFSM_EVT_INIT);
			}
		}
		else if (err < 0)
		{
			// error
			usbh->hcd_init_pt.thread = NULL;
		}
		break;
	}
	return NULL;
}

vsf_err_t vsfusbh_init(struct vsfusbh_t *usbh)
{
	if (NULL == usbh->hcd)
	{
		return VSFERR_INVALID_PARAMETER;
	}

	memset(&usbh->sm, 0, sizeof(usbh->sm));
	usbh->device_bitmap[0] = 0;
	usbh->device_bitmap[1] = 0;
	usbh->device_bitmap[2] = 0;
	usbh->device_bitmap[3] = 0;
	usbh->sm.init_state.evt_handler = vsfusbh_init_evt_handler;
	usbh->sm.user_data = (void*)usbh;
	return vsfsm_init(&usbh->sm);
}

vsf_err_t vsfusbh_fini(struct vsfusbh_t *usbh)
{
	// TODO
	return VSFERR_NONE;
}

vsf_err_t vsfusbh_register_driver(struct vsfusbh_t *usbh,
	const struct vsfusbh_class_drv_t *drv)
{
	struct vsfusbh_class_drv_list *drv_list;

	drv_list = (struct vsfusbh_class_drv_list *)\
		vsf_bufmgr_malloc(sizeof(struct vsfusbh_class_drv_list));
	if (drv_list == NULL)
	{
		return VSFERR_FAIL;
	}
	memset(drv_list, 0, sizeof(struct vsfusbh_class_drv_list));
	drv_list->drv = drv;
	sllist_append(&usbh->drv_list, &drv_list->list);
	return VSFERR_NONE;
}
