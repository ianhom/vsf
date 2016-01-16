#include "vsf.h"
#include "app_hw_cfg.h"

// dummy main, make compiler happy
int main(void)
{
	return 0;
}

vsf_err_t bcm_bus_spi_init(struct vsfsm_pt_t *, vsfsm_evt_t);
vsf_err_t bcm_bus_spi_waitf2(struct vsfsm_pt_t *, vsfsm_evt_t);
void bcm_bus_spi_init_int(struct bcm_bus_t *, void (*)(void *param), void *);
void bcm_bus_spi_fini_int(struct bcm_bus_t *);
void bcm_bus_spi_enable_int(struct bcm_bus_t *);
bool bcm_bus_spi_is_int(struct bcm_bus_t *);
vsf_err_t bcm_bus_spi_f2_avail(struct vsfsm_pt_t *, vsfsm_evt_t, uint16_t *);
vsf_err_t bcm_bus_spi_f2_read(struct vsfsm_pt_t *, vsfsm_evt_t, uint16_t,
					struct vsf_buffer_t *);
uint32_t bcm_bus_spi_fix_u32(struct bcm_bus_t *, uint32_t);
vsf_err_t bcm_bus_spi_transact(struct vsfsm_pt_t *, vsfsm_evt_t, uint8_t,
					uint8_t, uint32_t, uint16_t, struct vsf_buffer_t *);

vsf_err_t bcm_wifi_init(struct vsfsm_pt_t *, vsfsm_evt_t);
vsf_err_t bcm_wifi_fini(struct vsfsm_pt_t *, vsfsm_evt_t);
vsf_err_t bcm_sdpcm_header_sta(struct vsfip_buffer_t *,
					enum vsfip_netif_proto_t, const struct vsfip_macaddr_t *);

ROOTFUNC vsf_err_t module_exit(struct vsf_module_t *module)
{
	struct vsf_bcmwifi_api_t *api = vsf.stack.net.bcmwifi;
	memset(api, 0, sizeof(*api));
	module->ifs = NULL;
	return VSFERR_NONE;
}

ROOTFUNC vsf_err_t __iar_program_start(struct vsf_module_t *module,
							struct app_hwcfg_t const *hwcfg)
{
	struct vsf_bcmwifi_api_t *api = vsf.stack.net.bcmwifi;

	// check board and api version
	if (strcmp(hwcfg->board, APP_BOARD_NAME) ||
		(api_ver != VSF_API_VERSION))
	{
		return VSFERR_NOT_SUPPORT;
	}
	memset(api, 0, sizeof(*api));

#if IFS_SPI_EN
	api->bus.spi_op.init = bcm_bus_spi_init;
//	api->bus.spi_op.enable = bcm_bus_spi_enable;
	api->bus.spi_op.waitf2 = bcm_bus_spi_waitf2;
	api->bus.spi_op.init_int = bcm_bus_spi_init_int;
	api->bus.spi_op.fini_int = bcm_bus_spi_fini_int;
	api->bus.spi_op.enable_int = bcm_bus_spi_enable_int;
	api->bus.spi_op.is_int = bcm_bus_spi_is_int;
	api->bus.spi_op.f2_avail = bcm_bus_spi_f2_avail;
	api->bus.spi_op.f2_read = bcm_bus_spi_f2_read;
//	api->bus.spi_op.f2_abort = bcm_bus_spi_f2_abort;
	api->bus.spi_op.fix_u32 = bcm_bus_spi_fix_u32;
	api->bus.spi_op.transact = bcm_bus_spi_transact;
#endif

#if IFS_SDIO_EN
	api->bus.sdio_op.init = bcm_bus_sdio_init;
	api->bus.sdio_op.enable = bcm_bus_sdio_enable;
	api->bus.sdio_op.waitf2 = bcm_bus_sdio_waitf2;
	api->bus.sdio_op.init_int = bcm_bus_sdio_init_int;
	api->bus.sdio_op.fini_int = bcm_bus_sdio_fini_int;
	api->bus.sdio_op.enable_int = bcm_bus_sdio_enable_int;
	api->bus.sdio_op.is_int = bcm_bus_sdio_is_int;
	api->bus.sdio_op.f2_avail = bcm_bus_sdio_f2_avail;
	api->bus.sdio_op.f2_read = bcm_bus_sdio_f2_read;
	api->bus.sdio_op.f2_abort = bcm_bus_sdio_f2_abort;
	api->bus.sdio_op.fix_u32 = bcm_bus_sdio_fix_u32;
	api->bus.sdio_op.transact = bcm_bus_sdio_transact;
#endif

	api->netdrv_op.init = bcm_wifi_init;
	api->netdrv_op.fini = bcm_wifi_fini;
	api->netdrv_op.header = bcm_sdpcm_header_sta;
	api->scan = bcm_wifi_scan;
	api->join = bcm_wifi_join;
	api->leave = bcm_wifi_leave;

	module->ifs = (void *)api;
	return VSFERR_NONE;
}
