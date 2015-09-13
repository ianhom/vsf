#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "compiler.h"
#include "app_cfg.h"
#include "app_type.h"

#include "interfaces.h"
#include "framework/vsfsm/vsfsm.h"
#include "framework/vsftimer/vsftimer.h"
#include "tool/buffer/buffer.h"

#include "vsfusb.h"
#include "core/hcd/ohci/vsfohci.h"
#include "class/host/HUB/vsfusbh_HUB.h"

static struct vsfusbh_t usbh =
{
	&ohci_drv,
	NULL,
};

static void heap_init(void)
{
	static uint32_t project_global_heap[PROJECT_GLOBAL_HEAP_SIZE / 4];
	vsf_bufmgr_init((uint8_t *)project_global_heap, PROJECT_GLOBAL_HEAP_SIZE);
}

static void board_init(void)
{
	interfaces->core.init(NULL);
	interfaces->tickclk.init();
	interfaces->tickclk.start();
}

static void tickclk_callback_int(void *param)
{
	vsftimer_callback_int();
}

static void framework_init(void)
{
	vsftimer_init();
	interfaces->tickclk.set_callback(tickclk_callback_int, NULL);
}

int main(void)
{
	vsf_leave_critical();
	heap_init();
	board_init();
	framework_init();	
	
	vsfusbh_init(&usbh);
	vsfusbh_register_driver(&usbh, &vsfusbh_hub_drv);
	
	while (1)
	{
		vsfsm_poll();
		
		vsf_enter_critical();
		if (!vsfsm_get_event_pending())
		{
			// sleep, will also enable interrupt
			//core_interfaces.core.sleep(SLEEP_WFI);
			vsf_leave_critical();
		}
		else
		{
			vsf_leave_critical();
		}
	}
}

