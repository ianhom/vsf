#include "vsf.h"
#include "app_hw_cfg.h"

// dummy main, make compiler happy
int main(void)
{
	return 0;
}

vsf_err_t vsfusbd_HID_get_desc(struct vsfusbd_device_t*, uint8_t, uint8_t,
							uint16_t, struct vsf_buffer_t*);
vsf_err_t vsfusbd_HID_class_init(uint8_t, struct vsfusbd_device_t*);
vsf_err_t vsfusbd_HID_request_prepare(struct vsfusbd_device_t*);
vsf_err_t vsfusbd_HID_request_process(struct vsfusbd_device_t*);

vsf_err_t vsfusbd_CDCData_class_init(uint8_t, struct vsfusbd_device_t*);
vsf_err_t vsfusbd_CDCControl_request_prepare(struct vsfusbd_device_t*);
vsf_err_t vsfusbd_CDCControl_request_process(struct vsfusbd_device_t*);
vsf_err_t vsfusbd_CDCACMData_class_init(uint8_t, struct vsfusbd_device_t*);
vsf_err_t vsfusbd_CDCACMControl_request_prepare(struct vsfusbd_device_t*);
vsf_err_t vsfusbd_CDCACMControl_request_process(struct vsfusbd_device_t*);

vsf_err_t vsfusbd_MSCBOT_class_init(uint8_t, struct vsfusbd_device_t*);
vsf_err_t vsfusbd_MSCBOT_request_prepare(struct vsfusbd_device_t*);

ROOTFUNC vsf_err_t module_exit(struct vsf_module_t *module)
{
	struct vsf_usbd_api_t *api = vsf.stack.usb.device;
	memset(api, 0, sizeof(*api));
	module->ifs = NULL;
	return VSFERR_NONE;
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
	api->ep_send = vsfusbd_ep_send;
	api->ep_recv = vsfusbd_ep_recv;
	api->set_IN_handler = vsfusbd_set_IN_handler;
	api->set_OUT_handler = vsfusbd_set_OUT_handler;

	api->classes.hid.protocol.get_desc = vsfusbd_HID_get_desc;
	api->classes.hid.protocol.init = vsfusbd_HID_class_init;
	api->classes.hid.protocol.request_prepare = vsfusbd_HID_request_prepare;
	api->classes.hid.protocol.request_process = vsfusbd_HID_request_process;

	api->classes.cdc.data_protocol.init = vsfusbd_CDCData_class_init;
	api->classes.cdc.control_protocol.request_prepare = vsfusbd_CDCControl_request_prepare;
	api->classes.cdc.control_protocol.request_process = vsfusbd_CDCControl_request_process;

	api->classes.cdcacm.data_protocol.init = vsfusbd_CDCACMData_class_init;
	api->classes.cdcacm.control_protocol.request_prepare = vsfusbd_CDCACMControl_request_prepare;
	api->classes.cdcacm.control_protocol.request_process = vsfusbd_CDCACMControl_request_process;

	api->classes.mscbot.protocol.init = vsfusbd_MSCBOT_class_init;
	api->classes.mscbot.protocol.request_prepare = vsfusbd_MSCBOT_request_prepare;

	module->ifs = (void *)api;
	return VSFERR_NONE;
}
