#include "vsf.h"
#include "app_hw_cfg.h"

struct vsfapp_t
{
	struct app_hwcfg_t hwcfg;
	
	uint8_t bufmgr_buffer[64 * 1024];
} static app =
{
	{
		APP_BOARD_NAME,				// char *board;
		{
			{
				2,					// uint8_t port;
				13,					// uint8_t pin;
			},						// struct usb_pullup;
		},							// struct usbd;
		{
			{3, 4, 3, 6},
			{4, 1, 4, 0},
			{3, 5, 3, 3},
			{3, 9, 3, 10},
			{4, 15, 3, 8},
			{4, 13, 4, 14},
			{4, 11, 4, 12},
			{4, 9, 4, 10},
			{4, 7, 4, 8},
			{3, 0, 3, 1},
			{3, 14, 3, 15},
			{6, 13, 6, 14},
			{4, 6, 4, 2},
			{4, 4, 4, 5},
			{3, 13, 4, 3},
			{3, 11, 3, 12},
			{6, 4, 6, 5},
			{6, 2, 6, 3},
			{6, 0, 6, 1},
			{5, 14, 5, 15},
			{5, 12, 6, 13},
			{5, 4, 5, 5},
			{5, 2, 5, 3},
			{5, 0, 5, 1},
		},							// struct led_t led[24];
	},								// struct app_hwcfg_t hwcfg;
};

// tickclk interrupt, simply call vsftimer_callback_int
static void app_tickclk_callback_int(void *param)
{
	vsftimer_callback_int();
}

int main(void)
{
	int (*app_main)(struct app_hwcfg_t *hwcfg) = NULL;
	
	vsf_leave_critical();
	
	// system initialize
	vsf_bufmgr_init(app.bufmgr_buffer, sizeof(app.bufmgr_buffer));
	core_interfaces.core.init(NULL);
	core_interfaces.tickclk.init();
	core_interfaces.tickclk.start();
	vsftimer_init();
	core_interfaces.tickclk.set_callback(app_tickclk_callback_int, NULL);
	
	// load and call application
	// TODO: try to load app_main address
	if (app_main != NULL)
	{
		app_main(&app.hwcfg);
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
