#include "vsf.h"
#include "app_hw_cfg.h"

#include "led.h"

struct vsfapp_t
{
	struct vsf_module_t module;
	
	struct
	{
		struct vsf_t const *vsf;
	} sys;
	
	struct led_ifs_t ifs;
};

static vsf_err_t led_on(struct led_t *led)
{
	// TODO: how to get vsf pointer for system calls?
}

static vsf_err_t led_off(struct led_t *led)
{
	// TODO: how to get vsf pointer for system calls?
}

static bool led_is_on(struct led_t *led)
{
	return led->on;
}

vsf_err_t __iar_program_start(struct vsf_t const *vsf, struct app_hwcfg_t const *hwcfg)
{
	struct vsfapp_t *app = vsf->buffer.bufmgr.malloc(sizeof(struct vsfapp_t));
	
	if (NULL == app)
	{
		return VSFERR_NOT_ENOUGH_RESOURCES;
	}
	
	memset(app, 0, sizeof(*app));
	app->sys.vsf = vsf;
	
	app->ifs.on = led_on;
	app->ifs.off = led_off;
	app->ifs.is_on = led_is_on;
	app->module.ifs = &app->ifs;
	app->module.name = "led";
	return vsf->framework.module.load(&app->module);
}
