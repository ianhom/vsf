#include "app_cfg.h"
#include "app_type.h"

#include "interfaces.h"
#include "framework/vsfsm/vsfsm.h"
#include "framework/vsftimer/vsftimer.h"
#include "framework/vsfshell/vsfshell.h"

#include "tool/buffer/buffer.h"
#include "dal/usart_stream/usart_stream.h"

#include "stack/usb/vsfusb.h"
#include "stack/usb/class/host/HUB/vsfusbh_HUB.h"
#include "stack/usb/core/dwc_otg/vsfdwcotg.h"

struct vsfsm_evtq_element_t mainq_ele[16];
struct vsfsm_evtq_t mainq = 
{
	.size = dimof(mainq_ele),
	.queue = mainq_ele,
};

#define USART_STREAM_BUFFER_SIZE	64
static uint8_t tx_buf[USART_STREAM_BUFFER_SIZE];
static uint8_t rx_buf[USART_STREAM_BUFFER_SIZE];

struct vsfapp_t
{
	struct usart_stream_info_t usart_stream_info;
	struct vsfshell_t shell;
} static app = 
{
	{
		2,
		USART_STOPBITS_1 | USART_PARITY_NONE | USART_DATALEN_8,
		0,
		115200,
		{
			{
				tx_buf,
				USART_STREAM_BUFFER_SIZE,
			},
			0,
			0,			
		},
		{
			{
				rx_buf,
				USART_STREAM_BUFFER_SIZE,
			},
			0,
			0,			
		},
		{
			&app.usart_stream_info.tx_fifo,
			&fifo_stream_op,
		},
		{
			&app.usart_stream_info.rx_fifo,
			&fifo_stream_op,
		},
	},
	{
		&app.usart_stream_info.stream_tx,
		&app.usart_stream_info.stream_rx,
	},
};

static void heap_init(void)
{
	static uint32_t project_global_heap[PROJECT_GLOBAL_HEAP_SIZE / 4];
	vsf_bufmgr_init((uint8_t *)project_global_heap, PROJECT_GLOBAL_HEAP_SIZE);
}

static void board_init(void)
{
	vsfhal_core_init(NULL);
	vsfhal_tickclk_init();
	vsfhal_tickclk_start();
}

static void tickclk_callback_int(void *param)
{
	vsftimer_callback_int();
}

static void framework_init(void)
{
	vsftimer_init();
	vsfhal_tickclk_set_callback(tickclk_callback_int, NULL);
}

int main(void)
{
	vsf_leave_critical();
	vsfsm_evtq_init(&mainq);
	vsfsm_evtq_set(&mainq);
	
	// system initialize
	heap_init();
	board_init();
		
	framework_init();
		
	//vsfusbh_user_init();
	//vsfusbh_init(&usbh);
	//vsfusbh_register_driver(&usbh, &vsfusbh_hub_drv);

	usart_stream_init(&app.usart_stream_info);
	vsfshell_init(&app.shell);
	
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

