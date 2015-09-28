#include "vsf.h"
#include "app_hw_cfg.h"

struct vsfapp_t
{
	struct app_hwcfg_t hwcfg;
	
	uint8_t bufmgr_buffer[32 * 1024];
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
			BCM_PORT,
			{{
				BCM_SPI_PORT,
				BCM_SPI_CS_PORT,
				BCM_SPI_CS_PIN,
				BCM_SPI_FREQ,
			}},
			BCM_RST_PORT,
			BCM_RST_PIN,
			BCM_EINT_PORT,
			BCM_EINT_PIN,
			BCM_EINT,
			
			BCM_PWRCTRL_PORT,
			BCM_PWRCTRL_PIN,
		},							// struct bcm_wifi_port;
	},								// struct app_hwcfg_t hwcfg;
};

// tickclk interrupt, simply call vsftimer_callback_int
static void app_tickclk_callback_int(void *param)
{
	vsftimer_callback_int();
}

int main(void)
{
	uint32_t app_main_ptr;
	
	vsf_leave_critical();
	
	// system initialize
	vsf_bufmgr_init(app.bufmgr_buffer, sizeof(app.bufmgr_buffer));
	core_interfaces.core.init(NULL);
	core_interfaces.tickclk.init();
	core_interfaces.tickclk.start();
	vsftimer_init();
	core_interfaces.tickclk.set_callback(app_tickclk_callback_int, NULL);
	
	core_interfaces.gpio.init(app.hwcfg.key.port);
	core_interfaces.gpio.config_pin(app.hwcfg.key.port, app.hwcfg.key.port, GPIO_INPU);
	if (core_interfaces.gpio.get(app.hwcfg.key.port, 1 << app.hwcfg.key.pin))
	{
		// key release
		// load and call application
		app_main_ptr = *(uint32_t *)APP_MAIN_ADDR;
		if (app_main_ptr != 0xFFFFFFFF)
		{
			app_main_ptr += APP_MAIN_ADDR;
			((int (*)(struct app_hwcfg_t *hwcfg))app_main_ptr)(&app.hwcfg);
		}
	}
	else
	{
		// run bootloader
	}
	
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
