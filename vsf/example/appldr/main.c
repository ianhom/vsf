#include "vsf.h"
#include "app_hw_cfg.h"

const struct app_hwcfg_t app_hwcfg =
{
	.board =				APP_BOARD_NAME,

	.key.port =				KEY_PORT,
	.key.pin =				KEY_PIN,

	.usbd.pullup.port =		USB_PULLUP_PORT,
	.usbd.pullup.pin =		USB_PULLUP_PIN,

#ifdef VSFCFG_FUNC_BCMWIFI
	.bcmwifi.type =			BCMWIFI_PORT_TYPE,
	.bcmwifi.index =		BCMWIFI_PORT,
	.bcmwifi.freq_khz =		BCMWIFI_FREQ,
	.bcmwifi.rst.port =		BCMWIFI_RST_PORT,
	.bcmwifi.rst.pin =		BCMWIFI_RST_PIN,
	.bcmwifi.wakeup.port =	BCMWIFI_WAKEUP_PORT,
	.bcmwifi.wakeup.pin =	BCMWIFI_WAKEUP_PIN,
	.bcmwifi.mode.port =	BCMWIFI_MODE_PORT,
	.bcmwifi.mode.pin =		BCMWIFI_MODE_PIN,
	.bcmwifi.spi.cs.port =	BCMWIFI_SPI_CS_PORT,
	.bcmwifi.spi.cs.pin =	BCMWIFI_SPI_CS_PIN,
	.bcmwifi.spi.eint.port =BCMWIFI_EINT_PORT,
	.bcmwifi.spi.eint.pin =	BCMWIFI_EINT_PIN,
	.bcmwifi.spi.eint_idx =	BCMWIFI_EINT,
	.bcmwifi.pwrctrl.port =	BCMWIFI_PWRCTRL_PORT,
	.bcmwifi.pwrctrl.pin =	BCMWIFI_PWRCTRL_PIN,
#endif
};

struct vsfapp_t
{
	struct vsf_module_t bootloader;
	uint8_t bufmgr_buffer[APPCFG_BUFMGR_SIZE];
} static app =
{
	.bootloader.flash = (struct vsf_module_info_t *)APPCFG_BOOTLOADER_ADDR,
};

void main(void)
{
	struct vsf_module_t *module;
	uint32_t module_base = APPCFG_MODULES_ADDR;
	uint32_t *flash = (uint32_t *)module_base;
	uint32_t *module_ptr;

	vsf_enter_critical();
	vsfhal_core_init(NULL);
	vsf_bufmgr_init(app.bufmgr_buffer, sizeof(app.bufmgr_buffer));

	// initialize modules
	module_ptr = (uint32_t *)(app.bootloader.flash);
	if (*module_ptr != 0xFFFFFFFF)
	{
		vsf_module_register(&app.bootloader);
	}

	while (*flash != 0xFFFFFFFF)
	{
		module_ptr = (uint32_t *)(module_base + *flash);
		if (*module_ptr != 0xFFFFFFFF)
		{
			module = vsf_bufmgr_malloc(sizeof(struct vsf_module_t));
			if (NULL == module)
			{
				break;
			}

			module->flash = (struct vsf_module_info_t *)module_ptr;
			vsf_module_register(module);
		}
		flash++;
	}

	vsfhal_gpio_init(app_hwcfg.key.port);
	vsfhal_gpio_config_pin(app_hwcfg.key.port, app_hwcfg.key.port, GPIO_INPU);

	// run bootloader if key pressed OR app load fail
	if (!vsfhal_gpio_get(app_hwcfg.key.port, 1 << app_hwcfg.key.pin) ||
		vsf_module_load("app"))
	{
		vsf_module_load("bootloader");
	}

	while (1)
	{
		// remove for debug
//		vsfhal_core_sleep(SLEEP_WFI);
	}
}
