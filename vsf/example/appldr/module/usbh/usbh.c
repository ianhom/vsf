#include "vsf.h"
#include "app_hw_cfg.h"

void *vsfusbh_hub_init(struct vsfusbh_t *, struct vsfusbh_device_t *);
void vsfusbh_hub_free(struct vsfusbh_device_t *);
vsf_err_t vsfusbh_hub_match(struct vsfusbh_device_t *);

vsf_err_t vsfohci_init_thread(struct vsfsm_pt_t *, vsfsm_evt_t);
vsf_err_t vsfohci_fini(void *);
vsf_err_t vsfohci_suspend(void *);
vsf_err_t vsfohci_resume(void *);
vsf_err_t vsfohci_alloc_device(void *, struct vsfusbh_device_t *);
vsf_err_t vsfohci_free_device(void *, struct vsfusbh_device_t *);
vsf_err_t vsfohci_submit_urb(void *, struct vsfusbh_urb_t *);
vsf_err_t vsfohci_unlink_urb(void *, struct vsfusbh_urb_t *, void *);
vsf_err_t vsfohci_relink_urb(void *, struct vsfusbh_urb_t *);
vsf_err_t vsfohci_rh_control(void *, struct vsfusbh_urb_t *);

// dummy main, make compiler happy
int main(void)
{
	return 0;
}

ROOTFUNC vsf_err_t module_exit(struct vsf_module_t *module)
{
	struct vsf_usbh_api_t *api = vsf.stack.usb.host;
	memset(api, 0, sizeof(*api));
	module->ifs = NULL;
	return VSFERR_NONE;
}

ROOTFUNC vsf_err_t __iar_program_start(struct vsf_module_t *module,
							struct app_hwcfg_t const *hwcfg)
{
	struct vsf_usbh_api_t *api = vsf.stack.usb.host;

	// check board and api version
	if (strcmp(hwcfg->board, APP_BOARD_NAME) ||
		(api_ver != VSF_API_VERSION))
	{
		return VSFERR_NOT_SUPPORT;
	}

	memset(api, 0, sizeof(*api));
	api->init = vsfusbh_init;
	api->fini = vsfusbh_fini;
	api->register_driver = vsfusbh_register_driver;
	api->submit_urb = vsfusbh_submit_urb;

	api->hcd.ohci.driver.init_thread = vsfohci_init_thread;
	api->hcd.ohci.driver.fini = vsfohci_fini;
	api->hcd.ohci.driver.suspend = vsfohci_suspend;
	api->hcd.ohci.driver.resume = vsfohci_resume;
	api->hcd.ohci.driver.alloc_device = vsfohci_alloc_device;
	api->hcd.ohci.driver.free_device = vsfohci_free_device;
	api->hcd.ohci.driver.submit_urb = vsfohci_submit_urb;
	api->hcd.ohci.driver.unlink_urb = vsfohci_unlink_urb;
	api->hcd.ohci.driver.relink_urb = vsfohci_relink_urb;
	api->hcd.ohci.driver.rh_control = vsfohci_rh_control;

	api->classes.hub.driver.init = vsfusbh_hub_init;
	api->classes.hub.driver.free = vsfusbh_hub_free;
	api->classes.hub.driver.match = vsfusbh_hub_match;

	module->ifs = (void *)api;
	return VSFERR_NONE;
}
