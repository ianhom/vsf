#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "compiler.h"
#include "app_cfg.h"
#include "app_type.h"

#include "interfaces.h"
#include "framework/vsfsm/vsfsm.h"
#include "framework/vsftimer/vsftimer.h"
#include "framework/vsfshell/vsfshell.h"

#include "component/stream/stream.h"

#include "stack/usb/core/vsfusbd.h"
#include "stack/usb/class/device/CDC/vsfusbd_CDCACM.h"

#define APPCFG_VSFTIMER_NUM				16
#define APPCFG_VSFSM_PENDSVQ_LEN		16
#define APPCFG_VSFSM_MAINQ_LEN			16

// USB descriptors
static const uint8_t USB_DeviceDescriptor[] =
{
	0x12,	// bLength = 18
	USB_DT_DEVICE,	// USB_DESC_TYPE_DEVICE
	0x00,
	0x02,	// bcdUSB
	0xEF,	// device class: IAD
	0x02,	// device sub class
	0x01,	// device protocol
	0x40,	// max packet size
	0x83,
	0x04,	// vendor
	0x3A,
	0xA0,	// product
	0x00,
	0x02,	// bcdDevice
	1,		// manu facturer
	2,		// product
	3,		// serial number
	0x01	// number of configuration
};

static const uint8_t USB_ConfigDescriptor[] =
{
	// Configuation Descriptor
	0x09,	// bLength: Configuation Descriptor size
	USB_DT_CONFIG,
			// bDescriptorType: Configuration
	75,		// wTotalLength:no of returned bytes*
	0x00,
	0x02,	// bNumInterfaces: 1 interface
	0x01,	// bConfigurationValue: Configuration value
	0x00,	// iConfiguration: Index of string descriptor describing the configuration
	0x80,	// bmAttributes: bus powered
	0x64,	// MaxPower 200 mA

	// IAD
	0x08,	// bLength: IAD Descriptor size
	USB_DT_INTERFACE_ASSOCIATION,
			// bDescriptorType: IAD
	0,		// bFirstInterface
	2,		// bInterfaceCount
	0x02,	// bFunctionClass
	0x02,	// bFunctionSubClass
	0x01,	// bFunctionProtocol
	0x04,	// iFunction

	// Interface Descriptor for CDC
	0x09,	// bLength: Interface Descriptor size
	USB_DT_INTERFACE,
			// bDescriptorType: Interface
	0,		// bInterfaceNumber: Number of Interface
	0x00,	// bAlternateSetting: Alternate setting
	0x01,	// bNumEndpoints: One endpoints used
	0x02,	// bInterfaceClass: Communication Interface Class
	0x02,	// bInterfaceSubClass: Abstract Control Model
	0x01,	// bInterfaceProtocol: Common AT commands
	0x04,	// iInterface:

	// Header Functional Descriptor
	0x05,	// bLength: Endpoint Descriptor size
	0x24,	// bDescriptorType: CS_INTERFACE
	0x00,	// bDescriptorSubtype: Header Func Desc
	0x10,	// bcdCDC: spec release number
	0x01,

	// Call Managment Functional Descriptor
	0x05,	// bFunctionLength
	0x24,	// bDescriptorType: CS_INTERFACE
	0x01,	// bDescriptorSubtype: Call Management Func Desc
	0x00,	// bmCapabilities: D0+D1
	0x01,	// bDataInterface: 1

	// ACM Functional Descriptor
	0x04,	// bFunctionLength
	0x24,	// bDescriptorType: CS_INTERFACE
	0x02,	// bDescriptorSubtype: Abstract Control Management desc
	0x02,	// bmCapabilities

	// Union Functional Descriptor
	0x05,	// bFunctionLength
	0x24,	// bDescriptorType: CS_INTERFACE
	0x06,	// bDescriptorSubtype: Union func desc
	0,		// bMasterInterface: Communication class interface
	1,		// bSlaveInterface0: Data Class Interface

	// Endpoint 2 Descriptor
	0x07,	// bLength: Endpoint Descriptor size
	USB_DT_ENDPOINT,
			// bDescriptorType: Endpoint
	0x82,	// bEndpointAddress: (IN2)
	0x03,	// bmAttributes: Interrupt
	8,		// wMaxPacketSize:
	0x00,
	0xFF,	// bInterval:

	// Data class interface descriptor
	0x09,	// bLength: Endpoint Descriptor size
	USB_DT_INTERFACE,
			// bDescriptorType: Interface
	1,		// bInterfaceNumber: Number of Interface
	0x00,	// bAlternateSetting: Alternate setting
	0x02,	// bNumEndpoints: Two endpoints used
	0x0A,	// bInterfaceClass: CDC
	0x00,	// bInterfaceSubClass:
	0x00,	// bInterfaceProtocol:
	0x00,	// iInterface:

	// Endpoint 3 Descriptor
	0x07,	// bLength: Endpoint Descriptor size
	USB_DT_ENDPOINT,
			// bDescriptorType: Endpoint
	0x03,	// bEndpointAddress: (OUT3)
	0x02,	// bmAttributes: Bulk
	64,		// wMaxPacketSize:
	0x00,
	0x00,	// bInterval: ignore for Bulk transfer

	// Endpoint 3 Descriptor
	0x07,	// bLength: Endpoint Descriptor size
	USB_DT_ENDPOINT,
			// bDescriptorType: Endpoint
	0x83,	// bEndpointAddress: (IN3)
	0x02,	// bmAttributes: Bulk
	64,		// wMaxPacketSize:
	0x00,
	0x00	// bInterval
};

static const uint8_t USB_StringLangID[] =
{
	4,
	USB_DT_STRING,
	0x09,
	0x04
};

static const uint8_t USB_StringVendor[] =
{
	20,
	USB_DT_STRING,
	'S', 0, 'i', 0, 'm', 0, 'o', 0, 'n', 0, 'Q', 0, 'i', 0, 'a', 0,
	'n', 0
};

static const uint8_t USB_StringSerial[50] =
{
	50,
	USB_DT_STRING,
	'0', 0, '1', 0, '2', 0, '3', 0, '4', 0, '5', 0, '6', 0, '7', 0,
	'8', 0, '9', 0, 'A', 0, 'B', 0, 'C', 0, 'D', 0, 'E', 0, 'F', 0,
	'0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0,
};

static const uint8_t USB_StringProduct[] =
{
	14,
	USB_DT_STRING,
	'V', 0, 'S', 0, 'F', 0, 'U', 0, 'S', 0, 'B', 0
};

static const uint8_t CDC_StringFunc[] =
{
	14,
	USB_DT_STRING,
	'V', 0, 'S', 0, 'F', 0, 'C', 0, 'D', 0, 'C', 0
};

static const struct vsfusbd_desc_filter_t USB_descriptors[] =
{
	VSFUSBD_DESC_DEVICE(0, USB_DeviceDescriptor, sizeof(USB_DeviceDescriptor), NULL),
	VSFUSBD_DESC_CONFIG(0, 0, USB_ConfigDescriptor, sizeof(USB_ConfigDescriptor), NULL),
	VSFUSBD_DESC_STRING(0, 0, USB_StringLangID, sizeof(USB_StringLangID), NULL),
	VSFUSBD_DESC_STRING(0x0409, 1, USB_StringVendor, sizeof(USB_StringVendor), NULL),
	VSFUSBD_DESC_STRING(0x0409, 2, USB_StringProduct, sizeof(USB_StringProduct), NULL),
	VSFUSBD_DESC_STRING(0x0409, 3, USB_StringSerial, sizeof(USB_StringSerial), NULL),
	VSFUSBD_DESC_STRING(0x0409, 4, CDC_StringFunc, sizeof(CDC_StringFunc), NULL),
	VSFUSBD_DESC_NULL
};

// app state machine events
#define APP_EVT_USBPU_TO				VSFSM_EVT_USER_LOCAL_INSTANT + 0

static struct vsfsm_state_t* app_evt_handler(struct vsfsm_t *sm, vsfsm_evt_t evt);
static void app_pendsv_activate(struct vsfsm_evtq_t *q);

struct vsfapp_t
{
	// hw
	struct usb_pullup_port_t
	{
		uint8_t port;
		uint8_t pin;
	} usb_pullup;

	// usb & shell
	struct
	{
		struct
		{
			struct vsfusbd_CDCACM_param_t param;
			struct vsf_stream_t stream_tx;
			struct vsf_stream_t stream_rx;
			struct vsf_fifo_t fifo_tx;
			struct vsf_fifo_t fifo_rx;
			uint8_t txbuff[65];
			uint8_t rxbuff[65];
		} cdc;
		struct vsfusbd_iface_t ifaces[2];
		struct vsfusbd_config_t config[1];
		struct vsfusbd_device_t device;
	} usbd;
	struct vsfshell_t shell;

	struct vsfsm_t sm;

	struct vsfsm_evtq_t pendsvq;
	struct vsfsm_evtq_t mainq;

	// private
	// buffer mamager
	VSFPOOL_DEFINE(vsftimer_pool, struct vsftimer_t, APPCFG_VSFTIMER_NUM);

	struct vsfsm_evtq_element_t pendsvq_ele[APPCFG_VSFSM_PENDSVQ_LEN];
	struct vsfsm_evtq_element_t mainq_ele[APPCFG_VSFSM_MAINQ_LEN];
	uint8_t bufmgr_buffer[8 * 1024];
} static app =
{
	.usb_pullup.port						= USB_PULLUP_PORT,
	.usb_pullup.pin							= USB_PULLUP_PIN,

	.usbd.cdc.param.CDC_param.ep_out		= 3,
	.usbd.cdc.param.CDC_param.ep_in			= 3,
	.usbd.cdc.param.CDC_param.stream_tx		= &app.usbd.cdc.stream_tx,
	.usbd.cdc.param.CDC_param.stream_rx		= &app.usbd.cdc.stream_rx,
	.usbd.cdc.param.line_coding.bitrate		= 115200,
	.usbd.cdc.param.line_coding.stopbittype	= 0,
	.usbd.cdc.param.line_coding.paritytype	= 0,
	.usbd.cdc.param.line_coding.datatype	= 8,
	.usbd.cdc.stream_tx.user_mem			= &app.usbd.cdc.fifo_tx,
	.usbd.cdc.stream_tx.op					= &fifo_stream_op,
	.usbd.cdc.fifo_tx.buffer.buffer			= (uint8_t *)&app.usbd.cdc.txbuff,
	.usbd.cdc.fifo_tx.buffer.size			= sizeof(app.usbd.cdc.txbuff),
	.usbd.cdc.stream_rx.user_mem			= &app.usbd.cdc.fifo_rx,
	.usbd.cdc.stream_rx.op					= &fifo_stream_op,
	.usbd.cdc.fifo_rx.buffer.buffer			= (uint8_t *)&app.usbd.cdc.rxbuff,
	.usbd.cdc.fifo_rx.buffer.size			= sizeof(app.usbd.cdc.rxbuff),
	.usbd.ifaces[0].class_protocol			= (struct vsfusbd_class_protocol_t *)&vsfusbd_CDCACMControl_class,
	.usbd.ifaces[0].protocol_param			= &app.usbd.cdc.param,
	.usbd.ifaces[1].class_protocol			= (struct vsfusbd_class_protocol_t *)&vsfusbd_CDCACMData_class,
	.usbd.ifaces[1].protocol_param			= &app.usbd.cdc.param,
	.usbd.config[0].num_of_ifaces			= dimof(app.usbd.ifaces),
	.usbd.config[0].iface					= app.usbd.ifaces,
	.usbd.device.num_of_configuration		= dimof(app.usbd.config),
	.usbd.device.config						= app.usbd.config,
	.usbd.device.desc_filter				= (struct vsfusbd_desc_filter_t *)USB_descriptors,
	.usbd.device.device_class_iface			= 0,
	.usbd.device.drv						= (struct interface_usbd_t *)&core_interfaces.usbd,
	.usbd.device.int_priority				= 0,

	.shell.stream_tx						= &app.usbd.cdc.stream_tx,
	.shell.stream_rx						= &app.usbd.cdc.stream_rx,

	.sm.init_state.evt_handler				= app_evt_handler,

	.pendsvq.size							= dimof(app.pendsvq_ele),
	.pendsvq.queue							= app.pendsvq_ele,
	.pendsvq.activate						= app_pendsv_activate,
	.mainq.size								= dimof(app.mainq_ele),
	.mainq.queue							= app.mainq_ele,
	.mainq.activate							= NULL,
};

// tickclk interrupt, simply call vsftimer_callback_int
static void app_tickclk_callback_int(void *param)
{
	vsftimer_callback_int();
}

static void app_pendsv_activate(struct vsfsm_evtq_t *q)
{
	vsfhal_core_pendsv_trigger();
}

// vsftimer buffer mamager
static struct vsftimer_t* vsftimer_memop_alloc(void)
{
	return VSFPOOL_ALLOC(&app.vsftimer_pool, struct vsftimer_t);
}

static void vsftimer_memop_free(struct vsftimer_t *timer)
{
	VSFPOOL_FREE(&app.vsftimer_pool, timer);
}

struct vsftimer_mem_op_t vsftimer_memop =
{
	.alloc	= vsftimer_memop_alloc,
	.free	= vsftimer_memop_free,
};

static struct vsfsm_state_t *
app_evt_handler(struct vsfsm_t *sm, vsfsm_evt_t evt)
{
	switch (evt)
	{
	case VSFSM_EVT_INIT:
		vsfhal_core_init(NULL);
		vsfhal_tickclk_init();
		vsfhal_tickclk_start();

		VSFPOOL_INIT(&app.vsftimer_pool, struct vsftimer_t, APPCFG_VSFTIMER_NUM);
		vsftimer_init(&vsftimer_memop);
		vsfhal_tickclk_set_callback(app_tickclk_callback_int, NULL);

		vsf_bufmgr_init(app.bufmgr_buffer, sizeof(app.bufmgr_buffer));

		stream_init(&app.usbd.cdc.stream_rx);
		stream_init(&app.usbd.cdc.stream_tx);
		vsfusbd_device_init(&app.usbd.device);
		vsfshell_init(&app.shell);

		if (app.usb_pullup.port != IFS_DUMMY_PORT)
		{
			vsfhal_gpio_init(app.usb_pullup.port);
			vsfhal_gpio_clear(app.usb_pullup.port, 1 << app.usb_pullup.pin);
			vsfhal_gpio_config_pin(app.usb_pullup.port,
											app.usb_pullup.pin, GPIO_OUTPP);
		}
		app.usbd.device.drv->disconnect();
		vsftimer_create(sm, 200, 1, APP_EVT_USBPU_TO);
		break;
	case APP_EVT_USBPU_TO:
		if (app.usb_pullup.port != IFS_DUMMY_PORT)
		{
			vsfhal_gpio_set(app.usb_pullup.port,
										1 << app.usb_pullup.pin);
		}
		app.usbd.device.drv->connect();
		break;
	}
	return NULL;
}

static void app_on_pendsv(void *param)
{
	struct vsfsm_evtq_t *evtq_cur = param, *evtq_old = vsfsm_evtq_get();

	vsfsm_evtq_set(evtq_cur);
	while (vsfsm_get_event_pending())
	{
		vsfsm_poll();
	}
	vsfsm_evtq_set(evtq_old);
}

int main(void)
{
	vsf_enter_critical();
	vsfsm_evtq_init(&app.pendsvq);
	vsfsm_evtq_init(&app.mainq);

	vsfsm_evtq_set(&app.pendsvq);
	vsfsm_init(&app.sm);

	vsfhal_core_pendsv_config(app_on_pendsv, &app.pendsvq);
	vsf_leave_critical();

	vsfsm_evtq_set(&app.mainq);
	while (1)
	{
		// no thread runs in mainq, just sleep in main loop
		vsfhal_core_sleep(SLEEP_WFI);
	}
}
