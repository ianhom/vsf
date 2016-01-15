#include "vsf.h"
#include "app_hw_cfg.h"

#define VSF_MODULE_SHELL_NAME			"vsf.framework.shell"
#define VSF_MODULE_USBD_NAME			"vsf.stack.usb.device"

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
};

// app
static struct vsfsm_state_t *
app_evt_handler(struct vsfsm_t *sm, vsfsm_evt_t evt)
{
	struct vsfapp_t *app = (struct vsfapp_t *)sm->user_data;

	switch (evt)
	{
	case VSFSM_EVT_INIT:
		// Application
		// app init
		app->usbd.cdc.param.CDC_param.ep_out = 3;
		app->usbd.cdc.param.CDC_param.ep_in = 3;
		app->usbd.cdc.param.CDC_param.stream_tx = &app->usbd.cdc.stream_tx;
		app->usbd.cdc.param.CDC_param.stream_rx = &app->usbd.cdc.stream_rx;

		app->usbd.cdc.fifo_tx.buffer.buffer = app->usbd.cdc.txbuff;
		app->usbd.cdc.fifo_tx.buffer.size = sizeof(app->usbd.cdc.txbuff);
		app->usbd.cdc.fifo_rx.buffer.buffer = app->usbd.cdc.rxbuff;
		app->usbd.cdc.fifo_rx.buffer.size = sizeof(app->usbd.cdc.rxbuff);
		app->usbd.cdc.stream_tx.user_mem = &app->usbd.cdc.fifo_tx;
		app->usbd.cdc.stream_tx.op = &fifo_stream_op;
		app->usbd.cdc.stream_rx.user_mem = &app->usbd.cdc.fifo_rx;
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

		// disable usb pull up
		if (app->hwcfg->usbd.pullup.port != IFS_DUMMY_PORT)
		{
			core_interfaces.gpio.init(app->hwcfg->usbd.pullup.port);
			core_interfaces.gpio.clear(app->hwcfg->usbd.pullup.port, 1 << app->hwcfg->usbd.pullup.pin);
			core_interfaces.gpio.config_pin(app->hwcfg->usbd.pullup.port, app->hwcfg->usbd.pullup.pin, GPIO_OUTPP);
		}
		app->usbd.device.drv->disconnect();
		vsftimer_create(sm, 200, 1, USB_EVT_PULLUP_TO);
		break;
	case USB_EVT_PULLUP_TO:
		if (app->hwcfg->usbd.pullup.port != IFS_DUMMY_PORT)
		{
			core_interfaces.gpio.set(app->hwcfg->usbd.pullup.port, 1 << app->hwcfg->usbd.pullup.pin);
		}
		app->usbd.device.drv->connect();
		break;
	}
	return NULL;
}

// dummy main, make compiler happy
int main(void)
{
	return 0;
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

#ifdef VSFCFG_MODULE_USBD
	if (NULL == vsf_module_load(VSF_MODULE_USBD_NAME))
	{
		goto fail_load_usbd;
	}
#endif
#ifdef VSFCFG_MODULE_SHELL
	if (NULL == vsf_module_load(VSF_MODULE_SHELL_NAME))
	{
		goto fail_load_shell;
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

	app->sm.init_state.evt_handler = app_evt_handler;
	app->sm.user_data = app;
	return vsfsm_init(&app->sm);

fail_malloc_app:
#ifdef VSFCFG_MODULE_SHELL
	vsf_module_unload(VSF_MODULE_SHELL_NAME);
fail_load_shell:
#endif
#ifdef VSFCFG_MODULE_USBD
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
	if (module->ifs != NULL)
	{
		vsf_bufmgr_free(module->ifs);
	}
	return VSFERR_NONE;
}
