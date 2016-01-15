#include "vsf.h"
#include "app_hw_cfg.h"

const struct app_hwcfg_t app_hwcfg =
{
	.board =				APP_BOARD_NAME,

	.key.port =				KEY_PORT,
	.key.pin =				KEY_PIN,

	.usbd.pullup.port =		USB_PULLUP_PORT,
	.usbd.pullup.pin =		USB_PULLUP_PIN,

#if VSFCFG_FUNC_BCMWIFI
	.bcm.type =				BCM_PORT_TYPE,
	.bcm.index =			BCM_PORT,
	.bcm.freq_khz =			BCM_FREQ,
	.bcm.rst.port =			BCM_RST_PORT,
	.bcm.rst.pin =			BCM_RST_PIN,
	.bcm.wakeup.port =		BCM_WAKEUP_PORT,
	.bcm.wakeup.pin =		BCM_WAKEUP_PIN,
	.bcm.mode.port =		BCM_MODE_PORT,
	.bcm.mode.pin =			BCM_MODE_PIN,
	.bcm.spi.cs.port =		BCM_SPI_CS_PORT,
	.bcm.spi.cs.pin =		BCM_SPI_CS_PIN,
	.bcm.spi.eint.port =	BCM_EINT_PORT,
	.bcm.spi.eint.pin =		BCM_EINT_PIN,
	.bcm.spi.eint_idx =		BCM_EINT,
	.bcm.pwrctrl.port =		BCM_PWRCTRL_PORT,
	.bcm.pwrctrl.pin =		BCM_PWRCTRL_PIN,
#endif
};

struct vsfapp_t
{
	uint8_t bufmgr_buffer[APPCFG_BUFMGR_SIZE];
} static app;

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
