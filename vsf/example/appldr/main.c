#include "vsf.h"
#include "app_hw_cfg.h"

struct vsfapp_t
{
	struct app_hwcfg_t hwcfg;

	struct vsfsm_evtq_t pendsvq;
	struct vsfsm_evtq_t mainq;

	// private
	struct vsfsm_evtq_element_t pendsvq_ele[16];
	struct vsfsm_evtq_element_t mainq_ele[16];
	uint8_t bufmgr_buffer[8 * 1024];
} static app =
{
	{
		APP_BOARD_NAME,				// char *board;
		{
			KEY_PORT, KEY_PIN
		},							// struct key;
		{
			{
				USB_PULLUP_PORT,	// uint8_t port;
				USB_PULLUP_PIN,		// uint8_t pin;
			},						// struct usb_pullup;
		},							// struct usbd;
		{
			LED24
		},							// struct led_t led[24];
		{
			BCM_PORT_TYPE,

			BCM_PORT,
			BCM_FREQ,

			BCM_RST_PORT,
			BCM_RST_PIN,
			BCM_WAKEUP_PORT,
			BCM_WAKEUP_PIN,
			BCM_MODE_PORT,
			BCM_MODE_PIN,

			{
				BCM_SPI_CS_PORT,
				BCM_SPI_CS_PIN,
				BCM_EINT_PORT,
				BCM_EINT_PIN,
				BCM_EINT,
			},

			BCM_PWRCTRL_PORT,
			BCM_PWRCTRL_PIN,
		},							// struct bcm_wifi_port;
	},								// struct app_hwcfg_t hwcfg;
	{
		.size = dimof(app.pendsvq_ele),
		.queue = app.pendsvq_ele,
	},								// struct vsfsm_evtq_t pendsvq;
	{
		.size = dimof(app.mainq_ele),
		.queue = app.mainq_ele,
	},								// struct vsfsm_evtq_t mainq;
};

// tickclk interrupt, simply call vsftimer_callback_int
static void app_tickclk_callback_int(void *param)
{
	vsftimer_callback_int();
}

static void app_on_pendsv(void *param)
{
	struct vsfsm_evtq_t *evtq_cur = param, *evtq_old = vsfsm_evtq_get();

	vsfsm_evtq_set(evtq_cur);
	if (vsfsm_get_event_pending())
		vsfsm_poll();
	vsfsm_evtq_set(evtq_old);
}

int main(void)
{
	uint32_t app_main_ptr;

	vsf_enter_critical();
	vsfsm_evtq_init(&app.pendsvq);
	vsfsm_evtq_init(&app.mainq);

	vsfsm_evtq_set(&app.pendsvq);

	// system initialize
	vsf_bufmgr_init(app.bufmgr_buffer, sizeof(app.bufmgr_buffer));
	core_interfaces.core.init(NULL);
	core_interfaces.tickclk.init();
	core_interfaces.tickclk.start();
	vsftimer_init();
	core_interfaces.tickclk.set_callback(app_tickclk_callback_int, NULL);

	if (app.hwcfg.key.port != IFS_DUMMY_PORT)
	{
		gpio_init(app.hwcfg.key.port);
		gpio_config_pin(app.hwcfg.key.port, app.hwcfg.key.port, GPIO_INPU);
	}
	if ((IFS_DUMMY_PORT == app.hwcfg.key.port) ||
		core_interfaces.gpio.get(app.hwcfg.key.port, 1 << app.hwcfg.key.pin))
	{
		// key release
		// load and call application
		app_main_ptr = *(uint32_t *)APP_MAIN_ADDR;
		if (app_main_ptr < 128 * 1024)
		{
			app_main_ptr += APP_MAIN_ADDR;
			((int (*)(struct app_hwcfg_t *hwcfg))app_main_ptr)(&app.hwcfg);
		}
		else
		{
			goto bootlaoder_init;
		}
	}
	else
	{
		// run bootloader
	bootlaoder_init:

	}

	core_interfaces.core.pendsv(app_on_pendsv, &app.pendsvq);
	vsf_leave_critical();
	vsfsm_evtq_set(&app.mainq);

	while (1)
	{
		vsfsm_poll();

		vsf_enter_critical();
		if (!vsfsm_get_event_pending())
		{
			// sleep, will also enable interrupt
			core_interfaces.core.sleep(SLEEP_WFI);
		}
		else
		{
			vsf_leave_critical();
		}
	}
}
