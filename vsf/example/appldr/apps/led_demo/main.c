#include "vsf.h"
#include "app_hw_cfg.h"

#define LED_EVT_CARRY					(VSFSM_EVT_USER_LOCAL + 0)

struct vsfapp_t
{
	struct
	{
		struct vsf_t const *vsf;
	} sys;
	
	struct vsfsm_t sm;
	struct vsftimer_timer_t timer;
	
	// Application
	uint8_t led_num;
	struct app_led_t
	{
		struct led_t const *hw;
		bool on;
		
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
	struct interfaces_info_t const *ifs = app->sys.vsf->ifs;
	struct vsf_framework_t const *framework = &app->sys.vsf->framework;
	
	vsfsm_pt_begin(pt);
	
	while (1)
	{
		vsfsm_pt_wfe(pt, LED_EVT_CARRY);
		
		if (led->on)
		{
			ifs->gpio.clear(led->hw->hport, 1 << led->hw->hpin);
			if (led->sm_carry != NULL)
			{
				framework->post_evt(led->sm_carry, LED_EVT_CARRY);
			}
		}
		else
		{
			ifs->gpio.set(led->hw->hport, 1 << led->hw->hpin);
		}
		led->on = !led->on;
	}
	
	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}

static struct vsfsm_state_t *
app_evt_handler(struct vsfsm_t *sm, vsfsm_evt_t evt)
{
	struct vsfapp_t *app = (struct vsfapp_t *)sm->user_data;
	struct interfaces_info_t const *ifs = app->sys.vsf->ifs;
	struct vsf_framework_t const *framework = &app->sys.vsf->framework;
	struct app_led_t *led;
	struct led_t const *hwled;
	uint8_t i;
	
	switch (evt)
	{
	case VSFSM_EVT_INIT:
		// Application
		for (i = 0; i < app->led_num; i++)
		{
			led = &app->led[i];
			hwled = led->hw;
			
			led->on = false;
			led->pt.thread = app_led_thread;
			led->pt.user_data = led;
			led->sm_carry = (i < (app->led_num - 1)) ? &app->led[i + 1].sm : NULL;
			led->app = app;
			
			ifs->gpio.init(hwled->lport);
			ifs->gpio.clear(hwled->lport, 1 << hwled->lpin);
			ifs->gpio.config_pin(hwled->lport, hwled->lpin, ifs->gpio.constants.OUTPP);
			ifs->gpio.init(hwled->hport);
			ifs->gpio.clear(hwled->hport, 1 << hwled->hpin);
			ifs->gpio.config_pin(hwled->hport, hwled->hpin, ifs->gpio.constants.OUTPP);
			
			framework->pt_init(&led->sm, &led->pt);
		}
		
		app->timer.interval = 1;
		app->timer.evt = LED_EVT_CARRY;
		app->timer.sm = sm;
		framework->timer_register(&app->timer);
		break;
	case LED_EVT_CARRY:
		framework->post_evt(&app->led[0].sm, LED_EVT_CARRY);
		break;
	}
	return NULL;
}

vsf_err_t app_main(struct vsf_t const *vsf, struct app_hwcfg_t const *hwcfg)
{
	int i;
	struct vsfapp_t *app = vsf->buffer.bufmgr.malloc(sizeof(struct vsfapp_t));
	
	if (NULL == app)
	{
		return VSFERR_NOT_ENOUGH_RESOURCES;
	}
	
	memset(app, 0, sizeof(*app));
	app->sys.vsf = vsf;
	app->led_num = dimof(hwcfg->led);
	for (i = 0; i < app->led_num; i++)
	{
		app->led[i].hw = &hwcfg->led[i];
	}
	
	app->sm.init_state.evt_handler = app_evt_handler;
	app->sm.user_data = app;
	vsf->framework.sm_init(&app->sm);
	return VSFERR_NONE;
}
