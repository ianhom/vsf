#include "vsf.h"
#include "app_hw_cfg.h"

#include "led.h"

struct vsfapp_t
{
	struct vsf_module_t module;
	struct led_ifs_t ifs;
};

static vsf_err_t led_init(struct led_t *led)
{
	struct led_hw_t const *hwled = led->hw;
	
	led->on = false;
	core_interfaces.gpio.init(hwled->lport);
	core_interfaces.gpio.clear(hwled->lport, 1 << hwled->lpin);
	core_interfaces.gpio.config_pin(hwled->lport, hwled->lpin,
									core_interfaces.gpio.constants.OUTPP);
	core_interfaces.gpio.init(hwled->hport);
	core_interfaces.gpio.clear(hwled->hport, 1 << hwled->hpin);
	core_interfaces.gpio.config_pin(hwled->hport, hwled->hpin,
									core_interfaces.gpio.constants.OUTPP);
	return VSFERR_NONE;
}

static vsf_err_t led_on(struct led_t *led)
{
	return core_interfaces.gpio.set(led->hw->hport, 1 << led->hw->hpin);
}

static vsf_err_t led_off(struct led_t *led)
{
	return core_interfaces.gpio.clear(led->hw->hport, 1 << led->hw->hpin);
}

static bool led_is_on(struct led_t *led)
{
	return led->on;
}

vsf_err_t __iar_program_start(struct app_hwcfg_t const *hwcfg)
{
	struct vsfapp_t *app;
	
	// check board and api version
	if (strcmp(hwcfg->board, APP_BOARD_NAME) ||
		(vsf_api_ver != VSF_API_VERSION))
	{
		return VSFERR_NOT_SUPPORT;
	}
	
	app = vsf_bufmgr_malloc(sizeof(struct vsfapp_t));
	if (NULL == app)
	{
		return VSFERR_NOT_ENOUGH_RESOURCES;
	}
	
	memset(app, 0, sizeof(*app));
	
	app->ifs.init = led_init;
	app->ifs.on = led_on;
	app->ifs.off = led_off;
	app->ifs.is_on = led_is_on;
	app->module.ifs = &app->ifs;
	app->module.name = "led";
	return vsf_module_load(&app->module);
}
