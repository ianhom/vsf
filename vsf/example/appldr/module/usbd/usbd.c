#include "vsf.h"
#include "app_hw_cfg.h"

// dummy main, make compiler happy
int main(void)
{
	return 0;
}

vsf_err_t vsfusbd_HID_get_desc(struct vsfusbd_device_t *, uint8_t, uint8_t,
							uint16_t, struct vsf_buffer_t *);
vsf_err_t vsfusbd_HID_class_init(uint8_t, struct vsfusbd_device_t *);

vsf_err_t vsfusbd_CDCData_class_init(uint8_t, struct vsfusbd_device_t *);
vsf_err_t vsfusbd_CDCACMData_class_init(uint8_t, struct vsfusbd_device_t *);
struct vsfusbd_setup_filter_t *
vsfusbd_CDCACMControl_get_request_filter(struct vsfusbd_device_t *);

vsf_err_t vsfusbd_stdreq_get_device_status_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_stdreq_get_interface_status_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_stdreq_get_endpoint_status_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_stdreq_clear_device_feature_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_stdreq_clear_interface_feature_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_stdreq_clear_endpoint_feature_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_stdreq_set_device_feature_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_stdreq_set_interface_feature_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_stdreq_set_endpoint_feature_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_stdreq_set_address_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_stdreq_set_address_process(
		struct vsfusbd_device_t *device, struct vsf_buffer_t *buffer);
vsf_err_t vsfusbd_stdreq_get_device_descriptor_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_stdreq_get_interface_descriptor_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_stdreq_get_configuration_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_stdreq_set_configuration_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_stdreq_set_configuration_process(
		struct vsfusbd_device_t *device, struct vsf_buffer_t *buffer);
vsf_err_t vsfusbd_stdreq_get_interface_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_stdreq_set_interface_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));

vsf_err_t vsfusbd_HID_GetReport_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_HID_GetIdle_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_HID_GetProtocol_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_HID_SetReport_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_HID_SetReport_process(
		struct vsfusbd_device_t *, struct vsf_buffer_t *);
vsf_err_t vsfusbd_HID_SetIdle_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_HID_SetProtocol_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));

vsf_err_t vsfusbd_CDCControl_SendEncapsulatedCommand_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_CDCControl_SendEncapsulatedCommand_process(
		struct vsfusbd_device_t *, struct vsf_buffer_t *);
vsf_err_t vsfusbd_CDCControl_GetEncapsulatedResponse_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));

vsf_err_t vsfusbd_CDCACMControl_GetLineCoding_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_CDCACMControl_SetLineCoding_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_CDCACMControl_SetLineCoding_process(
		struct vsfusbd_device_t *, struct vsf_buffer_t *);
vsf_err_t vsfusbd_CDCACMControl_SetControlLineState_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));
vsf_err_t vsfusbd_CDCACMControl_SendBreak_prepare(
		struct vsfusbd_device_t *, struct vsf_buffer_t *, uint8_t* (*)(void *));

ROOTFUNC vsf_err_t module_exit(struct vsf_module_t *module)
{
	struct vsf_usbd_api_t *api = vsf.stack.usb.device;
	vsf_bufmgr_free(api->stdreq_filter);
	vsf_bufmgr_free(api->classes.hid.protocol.req_filter);
	vsf_bufmgr_free(api->classes.cdc.control_protocol.req_filter);
	vsf_bufmgr_free(api->classes.cdcacm.control_protocol.req_filter);
	memset(api, 0, sizeof(*api));
	module->ifs = NULL;
	return VSFERR_NONE;
}

#define usbd_req_init(req, t, r, pre, pro)\
	do {\
		(req)->type = (t);\
		(req)->request = (r);\
		(req)->prepare = (pre);\
		(req)->process = (pro);\
		(req)++;\
	} while (0)
#define usbd_stdreq_init(req, t, r, pre, pro)\
	usbd_req_init((req), (t) | USB_TYPE_STANDARD, (r), (pre), (pro))
#define usbd_classreq_init(req, t, r, pre, pro)\
	usbd_req_init((req), (t) | USB_TYPE_CLASS, (r), (pre), (pro))

ROOTFUNC vsf_err_t __iar_program_start(struct vsf_module_t *module,
							struct app_hwcfg_t const *hwcfg)
{
	struct vsf_usbd_api_t *api = vsf.stack.usb.device;
	struct vsfusbd_setup_filter_t *req;

	// check board and api version
	if (strcmp(hwcfg->board, APP_BOARD_NAME) ||
		(api_ver != VSF_API_VERSION))
	{
		return VSFERR_NOT_SUPPORT;
	}

	memset(api, 0, sizeof(*api));

	api->stdreq_filter = (struct vsfusbd_setup_filter_t *)
			vsf_bufmgr_malloc(sizeof(struct vsfusbd_setup_filter_t) * 18);
	if (NULL == api->stdreq_filter)
	{
		goto fail_malloc_std;
	}
	api->classes.hid.protocol.req_filter = (struct vsfusbd_setup_filter_t *)
			vsf_bufmgr_malloc(sizeof(struct vsfusbd_setup_filter_t) * 7);
	if (NULL == api->classes.hid.protocol.req_filter)
	{
		goto fail_malloc_hid;
	}
	api->classes.cdc.control_protocol.req_filter =
		(struct vsfusbd_setup_filter_t *)
			vsf_bufmgr_malloc(sizeof(struct vsfusbd_setup_filter_t) * 6);
	if (NULL == api->classes.cdc.control_protocol.req_filter)
	{
		goto fail_malloc_cdc;
	}
	api->classes.cdcacm.control_protocol.req_filter =
		(struct vsfusbd_setup_filter_t *)
			vsf_bufmgr_malloc(sizeof(struct vsfusbd_setup_filter_t) * 5);
	if (NULL == api->classes.cdcacm.control_protocol.req_filter)
	{
		goto fail_malloc_cdcacm;
	}

	api->init = vsfusbd_device_init;
	api->fini = vsfusbd_device_fini;
	api->ep_send = vsfusbd_ep_send_nb;
	api->ep_recv = vsfusbd_ep_receive_nb;
	api->set_IN_handler = vsfusbd_set_IN_handler;
	api->set_OUT_handler = vsfusbd_set_OUT_handler;
	req = api->stdreq_filter;
	usbd_stdreq_init(req, USB_DIR_IN | USB_RECIP_DEVICE, USB_REQ_GET_STATUS,
			vsfusbd_stdreq_get_device_status_prepare, NULL);
	usbd_stdreq_init(req, USB_DIR_IN | USB_RECIP_INTERFACE, USB_REQ_GET_STATUS,
			vsfusbd_stdreq_get_interface_status_prepare, NULL);
	usbd_stdreq_init(req, USB_DIR_IN | USB_RECIP_ENDPOINT, USB_REQ_GET_STATUS,
			vsfusbd_stdreq_get_endpoint_status_prepare, NULL);
	usbd_stdreq_init(req, USB_DIR_OUT | USB_RECIP_DEVICE, USB_REQ_CLEAR_FEATURE,
			vsfusbd_stdreq_clear_device_feature_prepare, NULL);
	usbd_stdreq_init(req, USB_DIR_OUT | USB_RECIP_INTERFACE,
			USB_REQ_CLEAR_FEATURE,
			vsfusbd_stdreq_clear_interface_feature_prepare, NULL);
	usbd_stdreq_init(req, USB_DIR_OUT | USB_RECIP_ENDPOINT,
			USB_REQ_CLEAR_FEATURE,
			vsfusbd_stdreq_clear_endpoint_feature_prepare, NULL);
	usbd_stdreq_init(req, USB_DIR_OUT | USB_RECIP_DEVICE, USB_REQ_SET_FEATURE,
			vsfusbd_stdreq_set_device_feature_prepare, NULL);
	usbd_stdreq_init(req, USB_DIR_OUT | USB_RECIP_INTERFACE,
			USB_REQ_SET_FEATURE,
			vsfusbd_stdreq_set_interface_feature_prepare, NULL);
	usbd_stdreq_init(req, USB_DIR_OUT | USB_RECIP_ENDPOINT, USB_REQ_SET_FEATURE,
			vsfusbd_stdreq_set_endpoint_feature_prepare, NULL);
	usbd_stdreq_init(req, USB_DIR_OUT | USB_RECIP_DEVICE, USB_REQ_SET_ADDRESS,
			vsfusbd_stdreq_set_address_prepare,
			vsfusbd_stdreq_set_address_process);
	usbd_stdreq_init(req, USB_DIR_IN | USB_RECIP_DEVICE, USB_REQ_GET_DESCRIPTOR,
			vsfusbd_stdreq_get_device_descriptor_prepare, NULL);
	usbd_stdreq_init(req, USB_DIR_IN | USB_RECIP_INTERFACE,
			USB_REQ_GET_DESCRIPTOR,
			vsfusbd_stdreq_get_interface_descriptor_prepare, NULL);
	usbd_stdreq_init(req, USB_DIR_IN | USB_RECIP_DEVICE,
			USB_REQ_GET_CONFIGURATION,
			vsfusbd_stdreq_get_configuration_prepare, NULL);
	usbd_stdreq_init(req, USB_DIR_OUT | USB_RECIP_DEVICE,
			USB_REQ_SET_CONFIGURATION,
			vsfusbd_stdreq_set_configuration_prepare,
			vsfusbd_stdreq_set_configuration_process);
	usbd_stdreq_init(req, USB_DIR_IN | USB_RECIP_INTERFACE,
			USB_REQ_GET_INTERFACE, vsfusbd_stdreq_get_interface_prepare, NULL);
	usbd_stdreq_init(req, USB_DIR_OUT | USB_RECIP_INTERFACE,
			USB_REQ_SET_INTERFACE, vsfusbd_stdreq_set_interface_prepare, NULL);
	usbd_req_init(req, 0, 0, NULL, NULL);

	api->classes.hid.protocol.get_desc = vsfusbd_HID_get_desc;
	api->classes.hid.protocol.init = vsfusbd_HID_class_init;
	req = api->classes.hid.protocol.req_filter;
	usbd_classreq_init(req, USB_DIR_IN | USB_RECIP_INTERFACE,
			USB_HIDREQ_GET_REPORT, vsfusbd_HID_GetReport_prepare, NULL);
	usbd_classreq_init(req, USB_DIR_IN | USB_RECIP_INTERFACE,
			USB_HIDREQ_GET_IDLE, vsfusbd_HID_GetIdle_prepare, NULL);
	usbd_classreq_init(req, USB_DIR_IN | USB_RECIP_INTERFACE,
			USB_HIDREQ_GET_PROTOCOL, vsfusbd_HID_GetProtocol_prepare, NULL);
	usbd_classreq_init(req, USB_DIR_OUT | USB_RECIP_INTERFACE,
			USB_HIDREQ_SET_REPORT, vsfusbd_HID_SetReport_prepare,
			vsfusbd_HID_SetReport_process);
	usbd_classreq_init(req, USB_DIR_OUT | USB_RECIP_INTERFACE,
			USB_HIDREQ_SET_IDLE, vsfusbd_HID_SetIdle_prepare, NULL);
	usbd_classreq_init(req, USB_DIR_OUT | USB_RECIP_INTERFACE,
			USB_HIDREQ_SET_PROTOCOL, vsfusbd_HID_SetProtocol_prepare, NULL);
	usbd_req_init(req, 0, 0, NULL, NULL);

	api->classes.cdc.data_protocol.init = vsfusbd_CDCData_class_init;
	req = api->classes.cdc.control_protocol.req_filter;
	usbd_classreq_init(req, USB_DIR_OUT | USB_RECIP_INTERFACE,
			USB_CDCREQ_SEND_ENCAPSULATED_COMMAND,
			vsfusbd_CDCControl_SendEncapsulatedCommand_prepare,
			vsfusbd_CDCControl_SendEncapsulatedCommand_process);
	usbd_classreq_init(req, USB_DIR_IN | USB_RECIP_INTERFACE,
			USB_CDCREQ_GET_ENCAPSULATED_RESPONSE,
			vsfusbd_CDCControl_GetEncapsulatedResponse_prepare, NULL);
	usbd_req_init(req, 0, 0, NULL, NULL);

	api->classes.cdcacm.control_protocol.get_request_filter =
								vsfusbd_CDCACMControl_get_request_filter;
	api->classes.cdcacm.data_protocol.init = vsfusbd_CDCACMData_class_init;
	req = api->classes.cdcacm.control_protocol.req_filter;
	usbd_classreq_init(req, USB_DIR_IN | USB_RECIP_INTERFACE,
			USB_CDCACMREQ_GET_LINE_CODING,
			vsfusbd_CDCACMControl_GetLineCoding_prepare, NULL);
	usbd_classreq_init(req, USB_DIR_OUT | USB_RECIP_INTERFACE,
			USB_CDCACMREQ_SET_LINE_CODING,
			vsfusbd_CDCACMControl_SetLineCoding_prepare,
			vsfusbd_CDCACMControl_SetLineCoding_process);
	usbd_classreq_init(req, USB_DIR_OUT | USB_RECIP_INTERFACE,
			USB_CDCACMREQ_SET_CONTROL_LINE_STATE,
			vsfusbd_CDCACMControl_SetControlLineState_prepare, NULL);
	usbd_classreq_init(req, USB_DIR_OUT | USB_RECIP_INTERFACE,
			USB_CDCACMREQ_SEND_BREAK,
			vsfusbd_CDCACMControl_SendBreak_prepare, NULL);
	usbd_req_init(req, 0, 0, NULL, NULL);

	module->ifs = (void *)api;
	return VSFERR_NONE;

fail_malloc_cdcacm:
	vsf_bufmgr_free(api->classes.cdcacm.control_protocol.req_filter);
	api->classes.cdcacm.control_protocol.req_filter = NULL;
fail_malloc_cdc:
	vsf_bufmgr_free(api->classes.cdc.control_protocol.req_filter);
	api->classes.cdc.control_protocol.req_filter = NULL;
fail_malloc_hid:
	vsf_bufmgr_free(api->stdreq_filter);
	api->stdreq_filter = NULL;
fail_malloc_std:
	return VSFERR_NOT_ENOUGH_RESOURCES;
}
