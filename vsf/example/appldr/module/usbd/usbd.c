#include "vsf.h"
#include "app_hw_cfg.h"

extern const struct vsfusbd_setup_filter_t vsfusbd_CDCControl_class_setup[3];
extern const struct vsfusbd_setup_filter_t vsfusbd_CDCACMControl_class_setup[5];
extern const struct vsfusbd_setup_filter_t vsfusbd_HID_class_setup[7];

static void usbd_protocol_free(struct vsfusbd_class_protocol_t *protocol)
{
	if (protocol != NULL)
	{
		if (protocol->desc_filter != NULL)
		{
			vsf_bufmgr_free(protocol->desc_filter);
		}
		if (protocol->req_filter != NULL)
		{
			vsf_bufmgr_free(protocol->req_filter);
		}
#ifdef VSFCFG_MODULE_ALLOC_RAM
		vsf_bufmgr_free(protocol);
#endif
	}
}

static vsf_err_t
usbd_protocol_clone(uint32_t base,
		struct vsfusbd_class_protocol_t **dst,
		const struct vsfusbd_class_protocol_t *src,
		const struct vsfusbd_desc_filter_t *desc_filter, uint32_t desc_num,
		const struct vsfusbd_setup_filter_t *req_filter, uint32_t req_num)
{
	uint32_t i;

#ifdef VSFCFG_MODULE_ALLOC_RAM
	*dst = vsf_bufmgr_malloc(sizeof(struct vsfusbd_class_protocol_t));
	if (NULL == dst)
	{
		goto fail_malloc_protocol;
	}
#endif
	memcpy(*dst, src, sizeof(struct vsfusbd_class_protocol_t));

	if ((desc_filter != NULL) && (desc_num > 0))
	{
		(*dst)->desc_filter =
			vsf_bufmgr_malloc(sizeof(struct vsfusbd_desc_filter_t) * desc_num);
		 if (NULL == (*dst)->desc_filter)
		 {
			 goto fail_malloc_desc;
		 }
	}

	if ((req_filter != NULL) && (req_num > 0))
	{
		(*dst)->req_filter =
			vsf_bufmgr_malloc(sizeof(struct vsfusbd_setup_filter_t) * req_num);
		 if (NULL == (*dst)->desc_filter)
		 {
			 goto fail_malloc_req;
		 }
	}

	if ((*dst)->desc_filter != NULL)
	{
		(*dst)->get_desc =
			(vsf_err_t (*)(struct vsfusbd_device_t*, uint8_t, uint8_t, uint16_t,
							struct vsf_buffer_t*))
				((uint32_t)(*dst)->get_desc + base);
	}
	if ((*dst)->get_request_filter != NULL)
	{
		(*dst)->get_request_filter =
			(struct vsfusbd_setup_filter_t* (*)(struct vsfusbd_device_t*))
				((uint32_t)(*dst)->get_request_filter + base);
	}
	if ((*dst)->init != NULL)
	{
		(*dst)->init =
			(vsf_err_t (*)(uint8_t, struct vsfusbd_device_t*))
				((uint32_t)(*dst)->init + base);
	}
	if ((*dst)->fini != NULL)
	{
		(*dst)->fini =
			(vsf_err_t (*)(uint8_t, struct vsfusbd_device_t*))
				((uint32_t)(*dst)->fini + base);
	}

	memcpy((*dst)->desc_filter, desc_filter,
				sizeof(struct vsfusbd_desc_filter_t) * desc_num);
	for (i = 0; i < desc_num; i++)
	{
		if ((*dst)->desc_filter[i].buffer.buffer != NULL)
		{
			(*dst)->desc_filter[i].buffer.buffer += base;
		}
		if ((*dst)->desc_filter[i].read != NULL)
		{
			(*dst)->desc_filter[i].read =
				(vsf_err_t (*)(struct vsfusbd_device_t*, struct vsf_buffer_t*))
					((uint32_t)(*dst)->desc_filter[i].read + base);
		}
	}

	memcpy((*dst)->req_filter, req_filter,
				sizeof(struct vsfusbd_setup_filter_t) * req_num);
	for (i = 0; i < req_num; i++)
	{
		if ((*dst)->req_filter[i].prepare != NULL)
		{
			(*dst)->req_filter[i].prepare =
				(vsf_err_t (*)(struct vsfusbd_device_t*, struct vsf_buffer_t*,
								uint8_t* (*)(void*)))
					((uint32_t)(*dst)->req_filter[i].prepare + base);
		}
		if ((*dst)->req_filter[i].process != NULL)
		{
			(*dst)->req_filter[i].process =
				(vsf_err_t (*)(struct vsfusbd_device_t*, struct vsf_buffer_t*))
					((uint32_t)(*dst)->req_filter[i].process + base);
		}
	}
	return VSFERR_NONE;

fail_malloc_req:
	vsf_bufmgr_free((*dst)->desc_filter);
fail_malloc_desc:
	vsf_bufmgr_free(*dst);
	return VSFERR_NOT_ENOUGH_RESOURCES;
}

// dummy main, make compiler happy
int main(void)
{
	return 0;
}

ROOTFUNC vsf_err_t __iar_program_start(struct vsf_module_t *module,
							struct app_hwcfg_t const *hwcfg)
{
	struct vsf_usbd_api_t *api = vsf.stack.usb.device;

	// check board and api version
	if (strcmp(hwcfg->board, APP_BOARD_NAME) ||
		(api_ver != VSF_API_VERSION))
	{
		return VSFERR_NOT_SUPPORT;
	}

	memset(api, 0, sizeof(*api));
	api->init = vsfusbd_device_init;
	api->fini = vsfusbd_device_fini;
	api->ep_send = vsfusbd_ep_send_nb;
	api->ep_recv = vsfusbd_ep_receive_nb;
	api->set_IN_handler = vsfusbd_set_IN_handler;
	api->set_OUT_handler = vsfusbd_set_OUT_handler;

	usbd_protocol_clone((uint32_t)module->code_buff,
			&api->classes.hid.protocol, &vsfusbd_HID_class,
			NULL, 0, vsfusbd_HID_class_setup, dimof(vsfusbd_HID_class_setup));
	usbd_protocol_clone((uint32_t)module->code_buff,
			&api->classes.cdc.control_protocol, &vsfusbd_CDCControl_class,
			NULL, 0, vsfusbd_CDCControl_class_setup,
			dimof(vsfusbd_CDCControl_class_setup));
	usbd_protocol_clone((uint32_t)module->code_buff,
			&api->classes.cdc.data_protocol, &vsfusbd_CDCData_class,
			NULL, 0, NULL, 0);
	usbd_protocol_clone((uint32_t)module->code_buff,
			&api->classes.cdcacm.control_protocol, &vsfusbd_CDCACMControl_class,
			NULL, 0, vsfusbd_CDCACMControl_class_setup,
			dimof(vsfusbd_CDCACMControl_class_setup));
	usbd_protocol_clone((uint32_t)module->code_buff,
			&api->classes.cdcacm.data_protocol, &vsfusbd_CDCACMData_class,
			NULL, 0, NULL, 0);

	module->ifs = (void *)api;
	return VSFERR_NONE;
}

// for shell module, module_exit is just a place holder
ROOTFUNC vsf_err_t module_exit(struct vsf_module_t *module)
{
	struct vsf_usbd_api_t *api = vsf.stack.usb.device;
	usbd_protocol_free(api->classes.hid.protocol);
	usbd_protocol_free(api->classes.cdc.control_protocol);
	usbd_protocol_free(api->classes.cdc.data_protocol);
	usbd_protocol_free(api->classes.cdcacm.control_protocol);
	usbd_protocol_free(api->classes.cdcacm.data_protocol);
	memset(api, 0, sizeof(*api));
	module->ifs = NULL;
	return VSFERR_NONE;
}
