#include "vsf.h"
#include "app_hw_cfg.h"

#include "../led_module/led.h"

#define LED_EVT_CARRY					(VSFSM_EVT_USER_LOCAL + 0)

struct vsfapp_t
{
	struct vsf_module_t app_mod;
	
	struct vsfsm_t sm;
	struct vsftimer_timer_t timer;
	
	// Application
	uint8_t led_num;
	struct led_ifs_t *led_ifs;
	struct app_led_t
	{
		struct led_t led_mod;
		
		struct vsfsm_pt_t pt;
		struct vsfsm_t *sm_carry;
		
		struct vsfsm_t sm;
		struct vsfapp_t *app;
	} led[24];
};

static vsf_err_t app_led_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct app_led_t *led = (struct app_led_t *)pt->user_data;
	struct vsfapp_t *app = led->app;
	
	vsfsm_pt_begin(pt);
	
	while (1)
	{
		vsfsm_pt_wfe(pt, LED_EVT_CARRY);
		
		if (app->led_ifs->is_on(&led->led_mod))
		{
			app->led_ifs->off(&led->led_mod);
			if (led->sm_carry != NULL)
			{
				vsfsm_post_evt(led->sm_carry, LED_EVT_CARRY);
			}
		}
		else
		{
			app->led_ifs->on(&led->led_mod);
		}
	}
	
	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}

static struct vsfsm_state_t *
app_evt_handler(struct vsfsm_t *sm, vsfsm_evt_t evt)
{
	struct vsfapp_t *app = (struct vsfapp_t *)sm->user_data;
	struct app_led_t *led;
	uint8_t i;
	
	switch (evt)
	{
	case VSFSM_EVT_INIT:
		// Application
		for (i = 0; i < app->led_num; i++)
		{
			led = &app->led[i];
			app->led_ifs->init(&led->led_mod);
			
			led->pt.thread = app_led_thread;
			led->pt.user_data = led;
			led->sm_carry = (i < (app->led_num - 1)) ?
								&app->led[i + 1].sm : NULL;
			led->app = app;
			vsfsm_pt_init(&led->sm, &led->pt);
		}
		
		app->timer.interval = 1;
		app->timer.evt = LED_EVT_CARRY;
		app->timer.sm = sm;
		vsftimer_register(&app->timer);
		break;
	case LED_EVT_CARRY:
		vsfsm_post_evt(&app->led[0].sm, LED_EVT_CARRY);
		break;
	}
	return NULL;
}

void app_on_load(struct vsf_module_t *me, struct vsf_module_t *new)
{
	struct vsfapp_t *app = container_of(me, struct vsfapp_t, app_mod);
	
	if (!strcmp(new->name, "led"))
	{
		app->led_ifs = new->ifs;
		app->sm.init_state.evt_handler = app_evt_handler;
		app->sm.user_data = app;
		vsfsm_init(&app->sm);
	}
}

vsf_err_t __iar_program_start(struct app_hwcfg_t const *hwcfg)
{
	int i;
	struct vsfapp_t *app = vsf_bufmgr_malloc(sizeof(struct vsfapp_t));
	struct vsf_module_t *led_mod;
	
	if (NULL == app)
	{
		return VSFERR_NOT_ENOUGH_RESOURCES;
	}
	
	memset(app, 0, sizeof(*app));
	app->led_num = dimof(hwcfg->led);
	for (i = 0; i < app->led_num; i++)
	{
		app->led[i].led_mod.hw = &hwcfg->led[i];
	}
	
	// get led_module
	led_mod = vsf_module_get("led");
	if (NULL == led_mod)
	{
		app->app_mod.name = "app";
		app->app_mod.callback.on_load = app_on_load;
		return VSFERR_NONE;
	}
	app_on_load(&app->app_mod, led_mod);
	return VSFERR_NONE;
}
