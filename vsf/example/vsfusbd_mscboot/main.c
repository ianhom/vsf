#include "vsf.h"

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
	0x00,	// device class:
	0x00,	// device sub class
	0x00,	// device protocol
	0x40,	// max packet size
	0x83,
	0x04,	// vendor
	0x20,
	0x57,	// product
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
	32,		// wTotalLength:no of returned bytes*
	0x00,
	0x01,	// bNumInterfaces: 1 interface
	0x01,	// bConfigurationValue: Configuration value
	0x00,	// iConfiguration: Index of string descriptor describing the configuration
	0x80,	// bmAttributes: bus powered
	0x64,	// MaxPower 200 mA

	// Interface Descriptor for CDC
	0x09,	// bLength: Interface Descriptor size
	USB_DT_INTERFACE,
			// bDescriptorType: Interface
	0,		// bInterfaceNumber: Number of Interface
	0x00,	// bAlternateSetting: Alternate setting
	0x02,	// bNumEndpoints: Two endpoints used
	0x08,	// bInterfaceClass: MSC
	0x06,	// bInterfaceSubClass: SCSI
	0x50,	// bInterfaceProtocol:
	0x00,	// iInterface:

	// Endpoint 1 Descriptor
	0x07,	// bLength: Endpoint Descriptor size
	USB_DT_ENDPOINT,
			// bDescriptorType: Endpoint
	0x81,	// bEndpointAddress: (IN1)
	0x02,	// bmAttributes: Bulk
	64,		// wMaxPacketSize:
	0x00,
	0x00,	// bInterval: ignore for Bulk transfer

	// Endpoint 1 Descriptor
	0x07,	// bLength: Endpoint Descriptor size
	USB_DT_ENDPOINT,
			// bDescriptorType: Endpoint
	0x01,	// bEndpointAddress: (OUT1)
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
	'V', 0, 'S', 0, 'F', 0, 'M', 0, 'S', 0, 'C', 0
};

static const struct vsfusbd_desc_filter_t USB_descriptors[] =
{
	VSFUSBD_DESC_DEVICE(0, USB_DeviceDescriptor, sizeof(USB_DeviceDescriptor), NULL),
	VSFUSBD_DESC_CONFIG(0, 0, USB_ConfigDescriptor, sizeof(USB_ConfigDescriptor), NULL),
	VSFUSBD_DESC_STRING(0, 0, USB_StringLangID, sizeof(USB_StringLangID), NULL),
	VSFUSBD_DESC_STRING(0x0409, 1, USB_StringVendor, sizeof(USB_StringVendor), NULL),
	VSFUSBD_DESC_STRING(0x0409, 2, USB_StringProduct, sizeof(USB_StringProduct), NULL),
	VSFUSBD_DESC_STRING(0x0409, 3, USB_StringSerial, sizeof(USB_StringSerial), NULL),
	VSFUSBD_DESC_NULL
};

// app state machine events
#define APP_EVT_USBPU_TO				VSFSM_EVT_USER_LOCAL_INSTANT + 0

static struct vsfsm_state_t* app_evt_handler(struct vsfsm_t *sm, vsfsm_evt_t evt);
static void app_pendsv_activate(struct vsfsm_evtq_t *q);
const struct vsfmal_drv_t memmal_op;

struct vsfapp_t
{
	// hw
	struct usb_pullup_port_t
	{
		uint8_t port;
		uint8_t pin;
	} usb_pullup;

	// mal
	struct
	{
		struct vsfmal_t mal;
		struct vsf_mal2scsi_t mal2scsi;
		uint8_t *pbuffer[2];
		uint8_t buffer[2][512];

		uint8_t mal_mem[64 * 1024];
	} mal;

	// usb
	struct
	{
		struct
		{
			struct vsfusbd_MSCBOT_param_t param;
			struct vsfscsi_lun_t lun[1];
		} msc;
		struct vsfusbd_iface_t ifaces[1];
		struct vsfusbd_config_t config[1];
		struct vsfusbd_device_t device;
	} usbd;

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

	.mal.mal.drv							= &memmal_op,
	.mal.mal.param							= &app,
	.mal.mal.block_size						= 512,
	.mal.mal.block_num						= sizeof(app.mal.mal_mem) / 512,
	.mal.pbuffer[0]							= app.mal.buffer[0],
	.mal.pbuffer[1]							= app.mal.buffer[1],

	.mal.mal2scsi.malstream.mal				= &app.mal.mal,
	.mal.mal2scsi.malstream.multibuf_stream.multibuf.count = dimof(app.mal.buffer),
	.mal.mal2scsi.malstream.multibuf_stream.multibuf.size = sizeof(app.mal.buffer[0]),
	.mal.mal2scsi.malstream.multibuf_stream.multibuf.buffer_list = app.mal.pbuffer,
	.mal.mal2scsi.cparam.block_size			= 512,
	.mal.mal2scsi.cparam.removable			= false,
	.mal.mal2scsi.cparam.vendor				= "Simon   ",
	.mal.mal2scsi.cparam.product			= "MSCBoot         ",
	.mal.mal2scsi.cparam.revision			= "1.00",
	.mal.mal2scsi.cparam.type				= SCSI_PDT_DIRECT_ACCESS_BLOCK,

	.usbd.msc.param.ep_in					= 1,
	.usbd.msc.param.ep_out					= 1,
	.usbd.msc.param.scsi_dev.max_lun		= 0,
	.usbd.msc.param.scsi_dev.lun			= app.usbd.msc.lun,
	.usbd.msc.lun[0].op						= (struct vsfscsi_lun_op_t *)&vsf_mal2scsi_op,
	.usbd.msc.lun[0].param					= &app.mal.mal2scsi,

	.usbd.ifaces[0].class_protocol			= (struct vsfusbd_class_protocol_t *)&vsfusbd_MSCBOT_class,
	.usbd.ifaces[0].protocol_param			= &app.usbd.msc.param,
	.usbd.config[0].num_of_ifaces			= dimof(app.usbd.ifaces),
	.usbd.config[0].iface					= app.usbd.ifaces,
	.usbd.device.num_of_configuration		= dimof(app.usbd.config),
	.usbd.device.config						= app.usbd.config,
	.usbd.device.desc_filter				= (struct vsfusbd_desc_filter_t *)USB_descriptors,
	.usbd.device.device_class_iface			= 0,
	.usbd.device.drv						= (struct interface_usbd_t *)&core_interfaces.usbd,
	.usbd.device.int_priority				= 0,

	.sm.init_state.evt_handler				= app_evt_handler,

	.pendsvq.size							= dimof(app.pendsvq_ele),
	.pendsvq.queue							= app.pendsvq_ele,
	.pendsvq.activate						= app_pendsv_activate,
	.mainq.size								= dimof(app.mainq_ele),
	.mainq.queue							= app.mainq_ele,
	.mainq.activate							= NULL,
};

// mem mal
static uint32_t
memmal_block_size(uint64_t addr, uint32_t size, enum vsfmal_op_t op)
{
	return 512;
}

static vsf_err_t memmal_read(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
								uint64_t addr, uint8_t *buff, uint32_t size)
{
	struct vsfmal_t *mal = (struct vsfmal_t *)pt->user_data;
	struct vsfapp_t *app = (struct vsfapp_t *)mal->param;

	if ((addr + size) >= sizeof(app->mal.mal_mem))
	{
		return VSFERR_FAIL;
	}
	memcpy(buff, &app->mal.mal_mem[addr], size);
	return VSFERR_NONE;
}

static vsf_err_t memmal_write(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
								uint64_t addr, uint8_t *buff, uint32_t size)
{
	struct vsfmal_t *mal = (struct vsfmal_t *)pt->user_data;
	struct vsfapp_t *app = (struct vsfapp_t *)mal->param;

	if ((addr + size) >= sizeof(app->mal.mal_mem))
	{
		return VSFERR_FAIL;
	}
	memcpy(&app->mal.mal_mem[addr], buff, size);
	return VSFERR_NONE;
}

const struct vsfmal_drv_t memmal_op =
{
	memmal_block_size, NULL, NULL, NULL,
	NULL, memmal_read, memmal_write
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

		vsfusbd_device_init(&app.usbd.device);

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
			vsfhal_gpio_set(app.usb_pullup.port, 1 << app.usb_pullup.pin);
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
//		vsfhal_core_sleep(SLEEP_WFI);
	}
}
