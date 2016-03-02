#include "vsf.h"
#include "app_hw_cfg.h"

#include "bcmwifi_handlers.h"

#define VSF_MODULE_SHELL_NAME			"vsf.framework.shell"
#define VSF_MODULE_USBD_NAME			"vsf.stack.usb.device"
#define VSF_MODULE_TCPIP_NAME			"vsf.stack.net.tcpip"
#define VSF_MODULE_BCMWIFI_NAME			"vsf.stack.net.bcmwifi"

#define USB_EVT_PULLUP_TO				(VSFSM_EVT_USER_LOCAL + 0)

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
	1,	// manu facturer
	2,	// product
	3,	// serial number
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
	0x02,	// bNumInterfaces: 2 interface
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

struct vsfapp_t
{
	struct app_hwcfg_t const *hwcfg;

	struct vsfsm_t sm;

	// Application
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
		struct vsfusbd_desc_filter_t descriptors[8];
		struct vsfusbd_iface_t ifaces[2];
		struct vsfusbd_config_t config[1];
		struct vsfusbd_device_t device;
	} usbd;

	struct vsfshell_t shell;

	struct vsftimer_mem_op_t vsftimer_memop;
	VSFPOOL_DEFINE(vsftimer_pool, struct vsftimer_t, 16);
	struct vsfsm_evtq_t pendsvq;
	struct vsfsm_evtq_element_t pendsvq_ele[16];

	struct vsfshell_bcmwifi_t bcmwifi;
	struct vsfshell_handler_t bcmwifi_handlers[16];
};

// app
// vsftimer
static void app_tickclk_callback_int(void *param)
{
	vsftimer_callback_int();
}

static struct vsftimer_t* vsftimer_memop_alloc(void)
{
	struct vsf_module_t *module = vsf_module_get("app");
	struct vsfapp_t *app = module->ifs;

	return VSFPOOL_ALLOC(&app->vsftimer_pool, struct vsftimer_t);
}

static void vsftimer_memop_free(struct vsftimer_t *timer)
{
	struct vsf_module_t *module = vsf_module_get("app");
	struct vsfapp_t *app = module->ifs;

	VSFPOOL_FREE(&app->vsftimer_pool, timer);
}

// wifi
static struct vsfip_buffer_t* app_bcmwifi_get_buffer(uint32_t size)
{
	struct vsf_module_t *module = vsf_module_get("app");
	struct vsfapp_t *app = module->ifs;
	struct vsfip_buffer_t *buffer = NULL;

	if (size <= sizeof(app->bcmwifi.vsfip_buffer128_mem[0]))
	{
		buffer = VSFPOOL_ALLOC(&app->bcmwifi.vsfip_buffer128_pool, struct vsfip_buffer_t);
	}
	if (NULL == buffer)
	{
		buffer = VSFPOOL_ALLOC(&app->bcmwifi.vsfip_buffer_pool, struct vsfip_buffer_t);
	}
	return buffer;
}

static void app_bcmwifi_release_buffer(struct vsfip_buffer_t *buffer)
{
	struct vsf_module_t *module = vsf_module_get("app");
	struct vsfapp_t *app = module->ifs;

	VSFPOOL_FREE(&app->bcmwifi.vsfip_buffer128_pool, buffer);
	VSFPOOL_FREE(&app->bcmwifi.vsfip_buffer_pool, buffer);
}

static struct vsfip_socket_t* app_bcmwifi_get_socket(void)
{
	struct vsf_module_t *module = vsf_module_get("app");
	struct vsfapp_t *app = module->ifs;

	return VSFPOOL_ALLOC(&app->bcmwifi.vsfip_socket_pool, struct vsfip_socket_t);
}

static void app_bcmwifi_release_socket(struct vsfip_socket_t *socket)
{
	struct vsf_module_t *module = vsf_module_get("app");
	struct vsfapp_t *app = module->ifs;

	VSFPOOL_FREE(&app->bcmwifi.vsfip_socket_pool, socket);
}

static struct vsfip_tcppcb_t* app_bcmwifi_get_tcppcb(void)
{
	struct vsf_module_t *module = vsf_module_get("app");
	struct vsfapp_t *app = module->ifs;

	return VSFPOOL_ALLOC(&app->bcmwifi.vsfip_tcppcb_pool, struct vsfip_tcppcb_t);
}

static void app_bcmwifi_release_tcppcb(struct vsfip_tcppcb_t *tcppcb)
{
	struct vsf_module_t *module = vsf_module_get("app");
	struct vsfapp_t *app = module->ifs;

	VSFPOOL_FREE(&app->bcmwifi.vsfip_tcppcb_pool, tcppcb);
}

static struct vsfsm_state_t *
app_evt_handler(struct vsfsm_t *sm, vsfsm_evt_t evt)
{
	struct vsfapp_t *app = (struct vsfapp_t *)sm->user_data;

	switch (evt)
	{
	case VSFSM_EVT_INIT:
		vsfhal_core_init(NULL);
		vsfhal_tickclk_init();
		vsfhal_tickclk_start();

		VSFPOOL_INIT(&app->vsftimer_pool, struct vsftimer_t, 16);
		app->vsftimer_memop.alloc = vsftimer_memop_alloc;
		app->vsftimer_memop.free = vsftimer_memop_free;
		vsftimer_init(&app->vsftimer_memop);
		vsfhal_tickclk_set_callback(app_tickclk_callback_int, NULL);

		{
			struct vsfip_buffer_t *buffer;
			int i;

			buffer = &app->bcmwifi.vsfip_buffer128_pool.buffer[0];
			for (i = 0; i < 16; i++)
			{
				buffer->buffer = app->bcmwifi.vsfip_buffer128_mem[i];
				buffer++;
			}
			buffer = &app->bcmwifi.vsfip_buffer_pool.buffer[0];
			for (i = 0; i < 8; i++)
			{
				buffer->buffer = app->bcmwifi.vsfip_buffer_mem[i];
				buffer++;
			}
		}
		VSFPOOL_INIT(&app->bcmwifi.vsfip_buffer128_pool, struct vsfip_buffer_t, 16);
		VSFPOOL_INIT(&app->bcmwifi.vsfip_buffer_pool, struct vsfip_buffer_t, 8);
		VSFPOOL_INIT(&app->bcmwifi.vsfip_socket_pool, struct vsfip_socket_t, 16);
		VSFPOOL_INIT(&app->bcmwifi.vsfip_tcppcb_pool, struct vsfip_tcppcb_t, 16);
		app->bcmwifi.mem_op.get_buffer = app_bcmwifi_get_buffer;
		app->bcmwifi.mem_op.release_buffer = app_bcmwifi_release_buffer;
		app->bcmwifi.mem_op.get_socket = app_bcmwifi_get_socket;
		app->bcmwifi.mem_op.release_socket = app_bcmwifi_release_socket;
		app->bcmwifi.mem_op.get_tcppcb = app_bcmwifi_get_tcppcb;
		app->bcmwifi.mem_op.release_tcppcb = app_bcmwifi_release_tcppcb;

		// Application
		// app init
		app->usbd.cdc.param.CDC_param.ep_out = 3;
		app->usbd.cdc.param.CDC_param.ep_in = 3;
		app->usbd.cdc.param.CDC_param.stream_tx = &app->usbd.cdc.stream_tx;
		app->usbd.cdc.param.CDC_param.stream_rx = &app->usbd.cdc.stream_rx;

		app->usbd.cdc.fifo_tx.buffer.buffer = app->usbd.cdc.txbuff;
		app->usbd.cdc.fifo_rx.buffer.buffer = app->usbd.cdc.rxbuff;
		app->usbd.cdc.fifo_tx.buffer.size = sizeof(app->usbd.cdc.txbuff);
		app->usbd.cdc.fifo_rx.buffer.size = sizeof(app->usbd.cdc.rxbuff);
		app->usbd.cdc.stream_tx.user_mem = &app->usbd.cdc.fifo_tx;
		app->usbd.cdc.stream_rx.user_mem = &app->usbd.cdc.fifo_rx;
		app->usbd.cdc.stream_tx.op = &fifo_stream_op;
		app->usbd.cdc.stream_rx.op = &fifo_stream_op;

		app->usbd.ifaces[0].class_protocol =
				(struct vsfusbd_class_protocol_t *)&vsfusbd_CDCACMControl_class;
		app->usbd.ifaces[0].protocol_param = (void *)&app->usbd.cdc.param;
		app->usbd.ifaces[1].class_protocol =
				(struct vsfusbd_class_protocol_t *)&vsfusbd_CDCACMData_class;
		app->usbd.ifaces[1].protocol_param = (void *)&app->usbd.cdc.param;
		app->usbd.config[0].num_of_ifaces = dimof(app->usbd.ifaces);
		app->usbd.config[0].iface = (struct vsfusbd_iface_t *)&app->usbd.ifaces;
		app->usbd.device.num_of_configuration = dimof(app->usbd.config);
		app->usbd.device.config = (struct vsfusbd_config_t *)&app->usbd.config;
		app->usbd.device.desc_filter = app->usbd.descriptors;
		app->usbd.device.drv = (struct interface_usbd_t *)&core_interfaces.usbd;

		app->usbd.descriptors[0].type = USB_DT_DEVICE;
		app->usbd.descriptors[0].index = 0;
		app->usbd.descriptors[0].lanid = 0;
		app->usbd.descriptors[0].buffer.buffer = (uint8_t *)USB_DeviceDescriptor;
		app->usbd.descriptors[0].buffer.size = sizeof(USB_DeviceDescriptor);
		app->usbd.descriptors[1].type = USB_DT_CONFIG;
		app->usbd.descriptors[1].index = 0;
		app->usbd.descriptors[1].lanid = 0;
		app->usbd.descriptors[1].buffer.buffer = (uint8_t *)USB_ConfigDescriptor;
		app->usbd.descriptors[1].buffer.size = sizeof(USB_ConfigDescriptor);
		app->usbd.descriptors[2].type = USB_DT_STRING;
		app->usbd.descriptors[2].index = 0;
		app->usbd.descriptors[2].lanid = 0;
		app->usbd.descriptors[2].buffer.buffer = (uint8_t *)USB_StringLangID;
		app->usbd.descriptors[2].buffer.size = sizeof(USB_StringLangID);
		app->usbd.descriptors[3].type = USB_DT_STRING;
		app->usbd.descriptors[3].index = 1;
		app->usbd.descriptors[3].lanid = 0x0409;
		app->usbd.descriptors[3].buffer.buffer = (uint8_t *)USB_StringVendor;
		app->usbd.descriptors[3].buffer.size = sizeof(USB_StringVendor);
		app->usbd.descriptors[4].type = USB_DT_STRING;
		app->usbd.descriptors[4].index = 2;
		app->usbd.descriptors[4].lanid = 0x0409;
		app->usbd.descriptors[4].buffer.buffer = (uint8_t *)USB_StringProduct;
		app->usbd.descriptors[4].buffer.size = sizeof(USB_StringProduct);
		app->usbd.descriptors[5].type = USB_DT_STRING;
		app->usbd.descriptors[5].index = 3;
		app->usbd.descriptors[5].lanid = 0x0409;
		app->usbd.descriptors[5].buffer.buffer = (uint8_t *)USB_StringSerial;
		app->usbd.descriptors[5].buffer.size = sizeof(USB_StringSerial);
		app->usbd.descriptors[6].type = USB_DT_STRING;
		app->usbd.descriptors[6].index = 4;
		app->usbd.descriptors[6].lanid = 0x0409;
		app->usbd.descriptors[6].buffer.buffer = (uint8_t *)CDC_StringFunc;
		app->usbd.descriptors[6].buffer.size = sizeof(CDC_StringFunc);

		stream_init(&app->usbd.cdc.stream_rx);
		stream_init(&app->usbd.cdc.stream_tx);
		vsfusbd_device_init(&app->usbd.device);

		app->shell.stream_tx = &app->usbd.cdc.stream_tx;
		app->shell.stream_rx = &app->usbd.cdc.stream_rx;
		vsfshell_init(&app->shell);

		bcmwifi_handlers_init(&app->bcmwifi, &app->shell,
			(struct vsfshell_handler_t *)&app->bcmwifi_handlers, app->hwcfg);

		// disable usb pull up
		if (app->hwcfg->usbd.pullup.port != IFS_DUMMY_PORT)
		{
			vsfhal_gpio_init(app->hwcfg->usbd.pullup.port);
			vsfhal_gpio_clear(app->hwcfg->usbd.pullup.port, 1 << app->hwcfg->usbd.pullup.pin);
			vsfhal_gpio_config_pin(app->hwcfg->usbd.pullup.port, app->hwcfg->usbd.pullup.pin, GPIO_OUTPP);
		}
		app->usbd.device.drv->disconnect();
		vsftimer_create(sm, 200, 1, USB_EVT_PULLUP_TO);
		break;
	case USB_EVT_PULLUP_TO:
		if (app->hwcfg->usbd.pullup.port != IFS_DUMMY_PORT)
		{
			vsfhal_gpio_set(app->hwcfg->usbd.pullup.port, 1 << app->hwcfg->usbd.pullup.pin);
		}
		app->usbd.device.drv->connect();
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

static void app_pendsv_activate(struct vsfsm_evtq_t *q)
{
	vsfhal_core_pendsv_trigger();
}

vsf_err_t main(struct vsfapp_t *app)
{
	app->pendsvq.size = dimof(app->pendsvq_ele);
	app->pendsvq.queue = app->pendsvq_ele;
	app->pendsvq.activate = app_pendsv_activate;
	vsfsm_evtq_init(&app->pendsvq);
	vsfsm_evtq_set(&app->pendsvq);

	app->sm.init_state.evt_handler = app_evt_handler;
	app->sm.user_data = app;
	vsfsm_init(&app->sm);

	vsfhal_core_pendsv_config(app_on_pendsv, &app->pendsvq);
	vsf_leave_critical();
	return VSFERR_NONE;
}

ROOTFUNC vsf_err_t __iar_program_start(struct vsf_module_t *module,
							struct app_hwcfg_t const *hwcfg)
{
	struct vsfapp_t *app;
	vsf_err_t err = VSFERR_FAIL;

	// check board and api version
	if (strcmp(hwcfg->board, APP_BOARD_NAME) ||
		(api_ver != VSF_API_VERSION))
	{
		return VSFERR_NOT_SUPPORT;
	}

#ifdef VSFCFG_STANDALONE_MODULE
	if (NULL == vsf_module_load(VSF_MODULE_USBD_NAME))
	{
		goto fail_load_usbd;
	}
	if (NULL == vsf_module_load(VSF_MODULE_SHELL_NAME))
	{
		goto fail_load_shell;
	}
	if (NULL == vsf_module_load(VSF_MODULE_BCMWIFI_NAME))
	{
		goto fail_load_bcmwifi;
	}
	if (NULL == vsf_module_load(VSF_MODULE_TCPIP_NAME))
	{
		goto fail_load_tcpip;
	}
#endif

	app = vsf_bufmgr_malloc(sizeof(struct vsfapp_t));
	if (NULL == app)
	{
		err = VSFERR_NOT_ENOUGH_RESOURCES;
		goto fail_malloc_app;
	}
	module->ifs = (void *)app;

	memset(app, 0, sizeof(*app));
	app->hwcfg = hwcfg;

	return main(app);

fail_malloc_app:
#ifdef VSFCFG_STANDALONE_MODULE
	vsf_module_unload(VSF_MODULE_TCPIP_NAME);
fail_load_tcpip:
	vsf_module_unload(VSF_MODULE_BCMWIFI_NAME);
fail_load_bcmwifi:
	vsf_module_unload(VSF_MODULE_SHELL_NAME);
fail_load_shell:
	vsf_module_unload(VSF_MODULE_USBD_NAME);
fail_load_usbd:
#endif
	return err;
}

// for app module, module_exit is just a place holder
ROOTFUNC vsf_err_t module_exit(struct vsf_module_t *module)
{
#ifdef VSFCFG_MODULE_USBD
	vsf_module_unload(VSF_MODULE_USBD_NAME);
#endif
#ifdef VSFCFG_MODULE_SHELL
	vsf_module_unload(VSF_MODULE_SHELL_NAME);
#endif
#ifdef VSFCFG_MODULE_BCMWIFI
	vsf_module_unload(VSF_MODULE_BCMWIFI_NAME);
#endif
#ifdef VSFCFG_MODULE_TCPIP
	vsf_module_unload(VSF_MODULE_TCPIP_NAME);
#endif
	if (module->ifs != NULL)
	{
		vsf_bufmgr_free(module->ifs);
	}
	return VSFERR_NONE;
}
