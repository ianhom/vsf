/***************************************************************************
 *   Copyright (C) 2009 - 2010 by Simon Qian <SimonQian@SimonQian.com>     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "app_cfg.h"
#include "app_type.h"

#include "interfaces.h"
#include "component/buffer/buffer.h"

#include "stack/usb/core/vsfusbh.h"

struct vsfusbh_hub_t
{
	struct vsfsm_t sm;
	struct vsfsm_pt_t drv_init_pt;
	struct vsfsm_pt_t drv_scan_pt;
	struct vsfsm_pt_t drv_connect_pt;
	struct vsfsm_pt_t drv_reset_pt;

	struct vsfusbh_t *usbh;
	struct vsfusbh_device_t *dev;

	struct vsfusbh_urb_t vsfurb;

	struct usb_hub_descriptor_t hub_desc;
	struct usb_hub_status_t hub_status;
	struct usb_port_status_t hub_portsts;

	int16_t counter;
	int16_t retry;
	int16_t inited;
};

static vsf_err_t hub_set_port_feature(struct vsfusbh_t *usbh,
		struct vsfusbh_urb_t *vsfurb, uint16_t port, uint16_t feature)
{
	vsfurb->pipe = usb_sndctrlpipe(vsfurb->vsfdev, 0);
	return vsfusbh_control_msg(usbh, vsfurb, USB_RT_PORT, USB_REQ_SET_FEATURE,
		feature, port);
}
static vsf_err_t hub_get_port_status(struct vsfusbh_t *usbh,
		struct vsfusbh_urb_t *vsfurb, uint16_t port)
{
	vsfurb->pipe = usb_rcvctrlpipe(vsfurb->vsfdev, 0);
	return vsfusbh_control_msg(usbh, vsfurb, USB_DIR_IN | USB_RT_PORT,
		USB_REQ_GET_STATUS, 0, port);
}
static vsf_err_t hub_clear_port_feature(struct vsfusbh_t *usbh,
		struct vsfusbh_urb_t *vsfurb, int port, int feature)
{
	vsfurb->pipe = usb_sndctrlpipe(vsfurb->vsfdev, 0);
	return vsfusbh_control_msg(usbh, vsfurb, USB_RT_PORT, USB_REQ_CLEAR_FEATURE,
		feature, port);
}
static vsf_err_t hub_get_status(struct vsfusbh_t *usbh,
		struct vsfusbh_urb_t *vsfurb)
{
	vsfurb->pipe = usb_rcvctrlpipe(vsfurb->vsfdev, 0);
	return vsfusbh_control_msg(usbh, vsfurb, USB_DIR_IN | USB_RT_HUB,
		USB_REQ_GET_STATUS, 0, 0);
}


static vsf_err_t hub_reset_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfusbh_hub_t *hdata = (struct vsfusbh_hub_t *)pt->user_data;
	struct vsfusbh_urb_t *vsfurb = &hdata->vsfurb;
	vsf_err_t err;

	vsfsm_pt_begin(pt);

	hdata->retry = 0;

	do
	{
		/* send command to reset port */
		vsfurb->transfer_buffer = NULL;
		vsfurb->transfer_length = 0;
		err = hub_set_port_feature(hdata->usbh, vsfurb, hdata->counter,
				USB_PORT_FEAT_RESET);
		if (err != VSFERR_NONE)
			return err;
		vsfsm_pt_wfe(pt, VSFSM_EVT_URB_COMPLETE);
		if (vsfurb->status != URB_OK)
			return VSFERR_FAIL;

		/* delay 100ms after port reset*/
		vsfsm_pt_delay(pt, 100);
		
		// clear reset
		vsfurb->transfer_buffer = NULL;
		vsfurb->transfer_length = 0;
		err = hub_clear_port_feature(hdata->usbh, vsfurb,
				hdata->counter, USB_PORT_FEAT_C_RESET);
		if (err != VSFERR_NONE)
			return err;
		vsfsm_pt_wfe(pt, VSFSM_EVT_URB_COMPLETE);
		if (vsfurb->status != URB_OK)
			return VSFERR_FAIL;
		
		/* delay 100ms after port reset*/
		vsfsm_pt_delay(pt, 50);
		
		/* get port status for check */
		vsfurb->transfer_buffer = &hdata->hub_portsts;
		vsfurb->transfer_length = sizeof(hdata->hub_portsts);
		err = hub_get_port_status(hdata->usbh, vsfurb, hdata->counter);
		if (err != VSFERR_NONE)
			return err;
		vsfsm_pt_wfe(pt, VSFSM_EVT_URB_COMPLETE);
		if (vsfurb->status != URB_OK)
			return VSFERR_FAIL;

		/* check port status after reset */
		if (hdata->hub_portsts.wPortStatus & USB_PORT_STAT_ENABLE)
		{
			return VSFERR_NONE;
		}
		else
		{
			/* delay 200ms for next reset*/
			vsfsm_pt_delay(pt, 200);
		}

	} while (hdata->retry++ <= 3);

	vsfsm_pt_end(pt);
	return VSFERR_FAIL;
}

static vsf_err_t hub_connect_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	vsf_err_t err;
	struct vsfusbh_hub_t *hdata = (struct vsfusbh_hub_t *)pt->user_data;
	struct vsfusbh_urb_t *vsfurb = &hdata->vsfurb;
	struct vsfusbh_device_t *usb;

	vsfsm_pt_begin(pt);

	/* clear the cnnection change state */
	vsfurb->transfer_buffer = NULL;
	vsfurb->transfer_length = 0;
	err = hub_clear_port_feature(hdata->usbh, vsfurb, hdata->counter,
		USB_PORT_FEAT_C_CONNECTION);
	if (err != VSFERR_NONE)
		return err;
	vsfsm_pt_wfe(pt, VSFSM_EVT_URB_COMPLETE);
	if (vsfurb->status != URB_OK)
		return VSFERR_FAIL;

	usb = hdata->dev->children[hdata->counter - 1];
	if (usb)
	{
		vsfusbh_remove_device(hdata->usbh, usb);
		hdata->dev->children[hdata->counter - 1] = NULL;
	}

	if (!(hdata->hub_portsts.wPortStatus & USB_PORT_STAT_CONNECTION))
	{
		if (hdata->hub_portsts.wPortStatus & USB_PORT_STAT_ENABLE)
		{
			vsfurb->transfer_buffer = NULL;
			vsfurb->transfer_length = 0;
			err = hub_clear_port_feature(hdata->usbh, vsfurb,
				hdata->counter, USB_PORT_FEAT_ENABLE);
			if (err != VSFERR_NONE)
				return err;
			vsfsm_pt_wfe(pt, VSFSM_EVT_URB_COMPLETE);
			if (vsfurb->status != URB_OK)
				return VSFERR_FAIL;
		}

		return VSFERR_NONE;
	}

	if (hdata->hub_portsts.wPortStatus & USB_PORT_STAT_LOW_SPEED)
	{
		vsfsm_pt_delay(pt, 200);
	}

	vsfsm_pt_wfpt(pt, &hdata->drv_reset_pt);

	vsfsm_pt_delay(pt, 200);

	// wait for new_dev free
	while (hdata->usbh->new_dev != NULL)
	{
		vsfsm_pt_delay(pt, 200);
	}

	usb = vsfusbh_alloc_device(hdata->usbh);
	if (NULL == usb)
		return VSFERR_FAIL;

	if (hdata->hub_portsts.wPortStatus & USB_PORT_STAT_LOW_SPEED)
		usb->speed = USB_SPEED_LOW;
	else if (hdata->hub_portsts.wPortStatus & USB_PORT_STAT_HIGH_SPEED)
		usb->speed = USB_SPEED_HIGH;
	else
		usb->speed = USB_SPEED_FULL;

	hdata->dev->children[hdata->counter - 1] = usb;
	usb->parent = hdata->dev;

	hdata->usbh->new_dev = usb;
	vsfsm_post_evt_pending(&hdata->usbh->sm, VSFSM_EVT_INIT);

	vsfsm_pt_end(pt);

	return VSFERR_NONE;
}

static vsf_err_t hub_scan_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	vsf_err_t err;
	struct vsfusbh_hub_t *hdata = (struct vsfusbh_hub_t *)pt->user_data;
	struct vsfusbh_urb_t *vsfurb = &hdata->vsfurb;

	vsfsm_pt_begin(pt);

	do
	{
		hdata->counter = 1;
		do
		{
			// get port status
			vsfurb->transfer_buffer = &hdata->hub_portsts;
			vsfurb->transfer_length = sizeof(hdata->hub_portsts);
			err = hub_get_port_status(hdata->usbh, vsfurb, hdata->counter);
			if (err != VSFERR_NONE)
				return err;
			vsfsm_pt_wfe(pt, VSFSM_EVT_URB_COMPLETE);
			if (vsfurb->status != URB_OK)
				return VSFERR_FAIL;

			if (hdata->hub_portsts.wPortChange & USB_PORT_STAT_C_CONNECTION)
			{
				// try to connect
				vsfsm_pt_wfpt(pt, &hdata->drv_connect_pt);
			}
			else if (hdata->hub_portsts.wPortChange & USB_PORT_STAT_C_ENABLE)
			{
				vsfurb->transfer_buffer = NULL;
				vsfurb->transfer_length = 0;
				err = hub_clear_port_feature(hdata->usbh, vsfurb,
					hdata->counter, USB_PORT_FEAT_C_ENABLE);
				if (err != VSFERR_NONE)
					return err;
				vsfsm_pt_wfe(pt, VSFSM_EVT_URB_COMPLETE);
				if (vsfurb->status != URB_OK)
					return VSFERR_FAIL;

				hdata->hub_portsts.wPortChange &= ~USB_PORT_STAT_C_ENABLE;
			}

			if (hdata->hub_portsts.wPortChange & USB_PORT_STAT_C_SUSPEND)
			{
				vsfurb->transfer_buffer = NULL;
				vsfurb->transfer_length = 0;
				err = hub_clear_port_feature(hdata->usbh, vsfurb,
					hdata->counter, USB_PORT_FEAT_C_SUSPEND);
				if (err != VSFERR_NONE)
					return err;
				vsfsm_pt_wfe(pt, VSFSM_EVT_URB_COMPLETE);
				if (vsfurb->status != URB_OK)
					return VSFERR_FAIL;

				hdata->hub_portsts.wPortChange &= ~USB_PORT_STAT_C_SUSPEND;
			}
			if (hdata->hub_portsts.wPortChange & USB_PORT_STAT_C_OVERCURRENT)
			{
				vsfurb->transfer_buffer = NULL;
				vsfurb->transfer_length = 0;
				err = hub_clear_port_feature(hdata->usbh, vsfurb,
					hdata->counter, USB_PORT_FEAT_C_OVER_CURRENT);
				if (err != VSFERR_NONE)
					return err;
				vsfsm_pt_wfe(pt, VSFSM_EVT_URB_COMPLETE);
				if (vsfurb->status != URB_OK)
					return VSFERR_FAIL;

				hdata->hub_portsts.wPortChange &= ~USB_PORT_FEAT_C_OVER_CURRENT;

				// TODO : power every port
			}
			if (hdata->hub_portsts.wPortChange & USB_PORT_STAT_C_RESET)
			{
				vsfurb->transfer_buffer = NULL;
				vsfurb->transfer_length = 0;
				err = hub_clear_port_feature(hdata->usbh, vsfurb,
					hdata->counter, USB_PORT_FEAT_C_RESET);
				if (err != VSFERR_NONE)
					return err;
				vsfsm_pt_wfe(pt, VSFSM_EVT_URB_COMPLETE);
				if (vsfurb->status != URB_OK)
					return VSFERR_FAIL;

				hdata->hub_portsts.wPortChange &= ~USB_PORT_FEAT_C_RESET;
			}
		} while (hdata->counter++ < hdata->dev->maxchild);

		// TODO : poll hub status		

		vsfsm_pt_delay(pt, 500);
	} while (1);
	vsfsm_pt_end(pt);

	return VSFERR_NONE;
}

static vsf_err_t hub_class_check(struct vsfusbh_hub_t *hdata)
{
	struct vsfusbh_device_t *dev = hdata->dev;
	struct usb_interface_desc_t *iface;
	struct usb_endpoint_descriptor_t *ep;

	if (dev == NULL)
		return VSFERR_FAIL;

	iface = dev->config->interface->interface_desc;
	ep = iface->ep_desc;
	if ((iface->bInterfaceClass != USB_CLASS_HUB) ||
		(iface->bInterfaceSubClass > 1) ||
		(iface->bNumEndpoints != 1) ||
		(!(ep->bEndpointAddress & USB_DIR_IN)) ||
		((ep->bmAttributes & USB_ENDPOINT_XFER_INT) != USB_ENDPOINT_XFER_INT))
		return VSFERR_FAIL;

	return VSFERR_NONE;
}

static vsf_err_t hub_get_descriptor(struct vsfusbh_t *usbh,
struct vsfusbh_urb_t *vsfurb)
{
	vsfurb->pipe = usb_rcvctrlpipe(vsfurb->vsfdev, 0);
	return vsfusbh_control_msg(usbh, vsfurb, USB_DIR_IN | USB_RT_HUB,
		USB_REQ_GET_DESCRIPTOR, (USB_DT_HUB << 8), 0);
}

static vsf_err_t hub_init_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	vsf_err_t err;
	struct vsfusbh_hub_t *hdata = (struct vsfusbh_hub_t *)pt->user_data;
	struct vsfusbh_urb_t *vsfurb = &hdata->vsfurb;

	vsfsm_pt_begin(pt);

	vsfurb->sm = &hdata->sm;

	// init hub
	err = hub_class_check(hdata);
	if (err != VSFERR_NONE)
		return err;

	vsfurb->transfer_buffer = &hdata->hub_desc;
	vsfurb->transfer_length = 4;
	err = hub_get_descriptor(hdata->usbh, vsfurb);
	if (err != VSFERR_NONE)
		return err;
	vsfsm_pt_wfe(pt, VSFSM_EVT_URB_COMPLETE);
	if (vsfurb->status != URB_OK)
		return VSFERR_FAIL;

	if (hdata->hub_desc.bDescLength > sizeof(hdata->hub_desc))
	{
		return VSFERR_FAIL;
	}

	vsfurb->transfer_buffer = &hdata->hub_desc;
	vsfurb->transfer_length = hdata->hub_desc.bDescLength;
	err = hub_get_descriptor(hdata->usbh, vsfurb);
	if (err != VSFERR_NONE)
		return err;
	vsfsm_pt_wfe(pt, VSFSM_EVT_URB_COMPLETE);
	if (vsfurb->status != URB_OK)
		return VSFERR_FAIL;

	hdata->dev->maxchild = min(hdata->hub_desc.bNbrPorts, USB_MAXCHILDREN);

	vsfurb->transfer_buffer = &hdata->hub_status;
	vsfurb->transfer_length = sizeof(hdata->hub_status);
	err = hub_get_status(hdata->usbh, vsfurb);
	if (err != VSFERR_NONE)
		return err;
	vsfsm_pt_wfe(pt, VSFSM_EVT_URB_COMPLETE);
	if (vsfurb->status != URB_OK)
		return VSFERR_FAIL;

	hdata->counter = 0;

	do
	{
		hdata->counter++;
		hdata->vsfurb.transfer_buffer = NULL;
		hdata->vsfurb.transfer_length = 0;
		err = hub_set_port_feature(hdata->usbh, &hdata->vsfurb, hdata->counter,
			USB_PORT_FEAT_POWER);
		if (err != VSFERR_NONE)
			return err;
		vsfsm_pt_wfe(pt, VSFSM_EVT_URB_COMPLETE);
		if (vsfurb->status != URB_OK)
			return VSFERR_FAIL;
		vsfsm_pt_delay(pt, hdata->hub_desc.bPwrOn2PwrGood * 2);
	} while (hdata->counter < hdata->dev->maxchild);

	vsfsm_pt_end(pt);

	return VSFERR_NONE;
}

static struct vsfsm_state_t *vsfusbh_hub_evt_handler_init(struct vsfsm_t *sm,
	vsfsm_evt_t evt)
{
	vsf_err_t err;
	struct vsfusbh_hub_t *hdata = (struct vsfusbh_hub_t *)sm->user_data;

	switch (evt)
	{
	case VSFSM_EVT_INIT:
		hdata->drv_init_pt.thread = hub_init_thread;
		hdata->drv_init_pt.user_data = hdata;
		hdata->drv_init_pt.sm = sm;
		hdata->drv_init_pt.state = 0;
		hdata->drv_scan_pt.thread = hub_scan_thread;
		hdata->drv_scan_pt.user_data = hdata;
		hdata->drv_scan_pt.sm = sm;
		hdata->drv_scan_pt.state = 0;
		hdata->drv_connect_pt.thread = hub_connect_thread;
		hdata->drv_connect_pt.user_data = hdata;
		hdata->drv_connect_pt.sm = &hdata->sm;
		hdata->drv_connect_pt.state = 0;
		hdata->drv_reset_pt.thread = hub_reset_thread;
		hdata->drv_reset_pt.user_data = hdata;
		hdata->drv_reset_pt.sm = &hdata->sm;
		hdata->drv_reset_pt.state = 0;
		hdata->inited = 0;

	case VSFSM_EVT_URB_COMPLETE:
	case VSFSM_EVT_DELAY_DONE:
		if (hdata->inited == 0)
		{
			err = hdata->drv_init_pt.thread(&hdata->drv_init_pt, evt);
			if (err == 0)
			{
				hdata->inited = 1;
				vsfsm_post_evt_pending(sm, VSFSM_EVT_DELAY_DONE);
			}
			if (err < 0)
			{
				vsfusbh_free_device(hdata->usbh, hdata->dev);
			}
		}
		else
		{
			err = hdata->drv_scan_pt.thread(&hdata->drv_scan_pt, evt);
			if (err < 0)
			{
				hdata->drv_scan_pt.state = 0;
				vsfsm_post_evt_pending(sm, VSFSM_EVT_DELAY_DONE);
			}
		}
		break;
	default:
		break;
	}
	return NULL;
}

void *vsfusbh_hub_init(struct vsfusbh_t *usbh, struct vsfusbh_device_t *dev)
{
	struct vsfusbh_class_data_t *cdata;
	struct vsfusbh_hub_t *hdata;

	cdata = vsf_bufmgr_malloc(sizeof(struct vsfusbh_class_data_t));
	if (NULL == cdata)
		return NULL;

	hdata = vsf_bufmgr_malloc(sizeof(struct vsfusbh_hub_t));
	if (NULL == hdata)
	{
		vsf_bufmgr_free(cdata);
		return NULL;
	}

	memset(cdata, 0, sizeof(struct vsfusbh_class_data_t));
	memset(hdata, 0, sizeof(struct vsfusbh_hub_t));

	cdata->dev = dev;
	hdata->dev = dev;
	hdata->vsfurb.vsfdev = dev;
	hdata->vsfurb.timeout = 200; 	/* default timeout 200ms */
	hdata->usbh = usbh;
	cdata->param = hdata;

	hdata->sm.init_state.evt_handler = vsfusbh_hub_evt_handler_init;
	hdata->sm.user_data = (void*)hdata;
	vsfsm_init(&hdata->sm);

	return cdata;
}

static void vsfusbh_hub_free(struct vsfusbh_device_t *dev)
{
	struct vsfusbh_class_data_t *cdata = (struct vsfusbh_class_data_t *)(dev->priv);

	vsf_bufmgr_free(cdata->param);
	vsf_bufmgr_free(cdata);
}

static vsf_err_t vsfusbh_hub_match(struct vsfusbh_device_t *dev)
{
	if (dev->descriptor.bDeviceClass == USB_CLASS_HUB)
		return VSFERR_NONE;

	return VSFERR_FAIL;
}

const struct vsfusbh_class_drv_t vsfusbh_hub_drv =
{
	vsfusbh_hub_init,
	vsfusbh_hub_free,
	vsfusbh_hub_match,
};
