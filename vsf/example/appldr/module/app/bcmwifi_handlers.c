#include "vsf.h"
#include "app_hw_cfg.h"
#include "bcmwifi_handlers.h"

struct vsfshell_bcmwifi_init_t
{
	struct vsfsm_pt_t local_pt;
};
static vsf_err_t
vsfshell_bcmwifi_init_handler(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfshell_handler_param_t *param =
						(struct vsfshell_handler_param_t *)pt->user_data;
	struct vsfshell_bcmwifi_t *wifi =
						(struct vsfshell_bcmwifi_t *)param->context;
	struct vsfsm_pt_t *output_pt = &param->output_pt;
	struct vsfshell_bcmwifi_init_t *init =
							(struct vsfshell_bcmwifi_init_t *)param->priv;
	vsf_err_t err;

	vsfsm_pt_begin(pt);
	if (param->argc != 1)
	{
		vsfshell_printf(output_pt, "invalid format." VSFSHELL_LINEEND);
		vsfshell_printf(output_pt, "format: bcm.init" VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	vsfshell_printf(output_pt, "Initialize BCM wifi module." VSFSHELL_LINEEND);

	param->priv = vsf_bufmgr_malloc(sizeof(struct vsfshell_bcmwifi_init_t));
	if (NULL == param->priv)
	{
		vsfshell_printf(output_pt, "not enough resources." VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	init = (struct vsfshell_bcmwifi_init_t *)param->priv;

	// power cycle
	if (wifi->pwrctrl_port != IFS_DUMMY_PORT)
	{
		vsfhal_gpio_init(wifi->pwrctrl_port);
		vsfhal_gpio_set(wifi->pwrctrl_port, 1 << wifi->pwrctrl_pin);
		vsfhal_gpio_config_pin(wifi->pwrctrl_port, wifi->pwrctrl_pin,
										GPIO_OUTPP);
		vsfsm_pt_delay(pt, 100);
		vsfhal_gpio_clear(wifi->pwrctrl_port, 1 << wifi->pwrctrl_pin);
	}

	vsfip_init(&wifi->mem_op);

	init->local_pt.sm = pt->sm;
	init->local_pt.user_data = (void *)&wifi->netif;
	init->local_pt.state = 0;
	vsfsm_pt_entry(pt);
	err = vsfip_netif_add(&init->local_pt, evt, &wifi->netif);
	if (err < 0)
	{
		vsfip_fini();
		vsfshell_printf(output_pt, "Fail." VSFSHELL_LINEEND);
	}
	else if (err > 0)
	{
		return VSFERR_NOT_READY;
	}
	else
	{
		vsfshell_printf(output_pt, "Succeed." VSFSHELL_LINEEND
						"F1SIG = 0x%08X" VSFSHELL_LINEEND
						"MAC(%d) = %02X:%02X:%02X:%02X:%02X:%02X"
						VSFSHELL_LINEEND,
						wifi->bcm_wifi.bus.f1sig,
						wifi->netif.macaddr.size,
						wifi->netif.macaddr.addr.s_addr_buf[0],
						wifi->netif.macaddr.addr.s_addr_buf[1],
						wifi->netif.macaddr.addr.s_addr_buf[2],
						wifi->netif.macaddr.addr.s_addr_buf[3],
						wifi->netif.macaddr.addr.s_addr_buf[4],
						wifi->netif.macaddr.addr.s_addr_buf[5]);
	}

handler_thread_end:
	if (param->priv != NULL)
	{
		vsf_bufmgr_free(param->priv);
	}
	vsfshell_handler_exit(pt);
	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}

static vsf_err_t
vsfshell_bcmwifi_fini_handler(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfshell_handler_param_t *param =
						(struct vsfshell_handler_param_t *)pt->user_data;
	struct vsfshell_bcmwifi_t *wifi =
						(struct vsfshell_bcmwifi_t *)param->context;
	struct vsfsm_pt_t *output_pt = &param->output_pt;
	struct vsfshell_bcmwifi_init_t *init =
							(struct vsfshell_bcmwifi_init_t *)param->priv;
	vsf_err_t err;

	vsfsm_pt_begin(pt);
	if (param->argc != 1)
	{
		vsfshell_printf(output_pt, "invalid format." VSFSHELL_LINEEND);
		vsfshell_printf(output_pt, "format: bcm.init" VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	vsfshell_printf(output_pt, "Finalize BCM wifi module." VSFSHELL_LINEEND);

	param->priv = vsf_bufmgr_malloc(sizeof(struct vsfshell_bcmwifi_init_t));
	if (NULL == param->priv)
	{
		vsfshell_printf(output_pt, "not enough resources." VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	init = (struct vsfshell_bcmwifi_init_t *)param->priv;

	init->local_pt.sm = pt->sm;
	init->local_pt.user_data = (void *)&wifi->netif;
	init->local_pt.state = 0;
	vsfsm_pt_entry(pt);
	err = vsfip_netif_remove(&init->local_pt, evt, &wifi->netif);
	if (err < 0)
	{
		vsfshell_printf(output_pt, "Fail." VSFSHELL_LINEEND);
	}
	else if (err > 0)
	{
		return VSFERR_NOT_READY;
	}

	vsfip_fini();

	if (wifi->pwrctrl_port != IFS_DUMMY_PORT)
	{
		vsfhal_gpio_set(wifi->pwrctrl_port, 1 << wifi->pwrctrl_pin);
		vsfhal_gpio_config_pin(wifi->pwrctrl_port, wifi->pwrctrl_pin,
										GPIO_INFLOAT);
	}

handler_thread_end:
	if (param->priv != NULL)
	{
		vsf_bufmgr_free(param->priv);
	}
	vsfshell_handler_exit(pt);
	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}

struct vsfshell_bcmwifi_scan_t
{
	struct vsfsm_pt_t local_pt;
	uint32_t index;
	struct bcm_wifi_bss_info_t bss_info[16];
	int i;
};
static vsf_err_t
vsfshell_bcmwifi_scan_handler(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfshell_handler_param_t *param =
						(struct vsfshell_handler_param_t *)pt->user_data;
	struct vsfshell_bcmwifi_t *wifi =
						(struct vsfshell_bcmwifi_t *)param->context;
	struct vsfsm_pt_t *output_pt = &param->output_pt;
	struct vsfshell_bcmwifi_scan_t *scan = (struct vsfshell_bcmwifi_scan_t *)param->priv;
	vsf_err_t err;

	struct vsfip_buffer_t *result_buffer;
	struct bcm_wifi_escan_result_t *scan_result;
	int i, j;
	bool match;

	vsfsm_pt_begin(pt);
	param->priv = vsf_bufmgr_malloc(sizeof(struct vsfshell_bcmwifi_scan_t));
	if (NULL == param->priv)
	{
		vsfshell_printf(output_pt, "not enough resources." VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	scan = (struct vsfshell_bcmwifi_scan_t *)param->priv;

	scan->local_pt.user_data = &wifi->netif;
	scan->local_pt.sm = pt->sm;
	scan->local_pt.state = 0;
	scan->index = 0;
	vsfsm_pt_entry(pt);
	err = bcm_wifi_scan(&scan->local_pt, evt, &result_buffer,
						BCM_INTERFACE_STA);
	if (err < 0)
	{
		vsfshell_printf(output_pt, "Fail to scan." VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	else if (result_buffer != NULL)
	{
		// process result, non-block code here
		scan_result =
			(struct bcm_wifi_escan_result_t *)result_buffer->buf.buffer;
		for (i = 0; i < scan_result->bss_count; i++)
		{
			match = false;
			for (j = 0; j < scan->index; j++)
			{
				if (!strcmp((const char *)scan->bss_info[j].ssid,
							(const char *)scan_result->bss_info[i].ssid))
				{
					match = true;
				}
			}
			if (!match && (scan->index < dimof(scan->bss_info)))
			{
				scan->bss_info[scan->index] = scan_result->bss_info[i];
				scan->index++;
			}
		}

		vsfip_buffer_release(result_buffer);
	}
	if (err != 0) return err;

	for (scan->i = 0; scan->i < scan->index; scan->i++)
	{
		vsfshell_printf(output_pt,
			"%d: %02X:%02X:%02X:%02X:%02X:%02X %s" VSFSHELL_LINEEND,
			scan->i,
			scan->bss_info[scan->i].bssid[0], scan->bss_info[scan->i].bssid[1],
			scan->bss_info[scan->i].bssid[2], scan->bss_info[scan->i].bssid[3],
			scan->bss_info[scan->i].bssid[4], scan->bss_info[scan->i].bssid[5],
			scan->bss_info[scan->i].ssid);
	}

handler_thread_end:
	if (param->priv != NULL)
	{
		vsf_bufmgr_free(param->priv);
	}
	vsfshell_handler_exit(pt);
	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}

struct vsfshell_bcmwifi_join_t
{
	struct vsfsm_pt_t local_pt;
	uint32_t cur_status;
};
static vsf_err_t
vsfshell_bcmwifi_join_handler(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfshell_handler_param_t *param =
						(struct vsfshell_handler_param_t *)pt->user_data;
	struct vsfshell_bcmwifi_t *wifi =
						(struct vsfshell_bcmwifi_t *)param->context;
	struct vsfsm_pt_t *output_pt = &param->output_pt;
	struct vsfshell_bcmwifi_join_t *join =
							(struct vsfshell_bcmwifi_join_t *)param->priv;
	vsf_err_t err;

	vsfsm_pt_begin(pt);
	if (param->argc < 2)
	{
		vsfshell_printf(output_pt, "invalid format." VSFSHELL_LINEEND);
		vsfshell_printf(output_pt, "format: bcm.join SSID [PASSWORD]" VSFSHELL_LINEEND);
		goto handler_thread_end;
	}

	param->priv = vsf_bufmgr_malloc(sizeof(struct vsfshell_bcmwifi_join_t));
	if (NULL == param->priv)
	{
		vsfshell_printf(output_pt, "not enough resources." VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	join = (struct vsfshell_bcmwifi_join_t *)param->priv;

	join->local_pt.user_data = &wifi->netif;
	join->local_pt.sm = pt->sm;
	join->local_pt.state = 0;
	vsfsm_pt_entry(pt);
	if (3 == param->argc)
	{
		err = bcm_wifi_join(&join->local_pt, evt, &join->cur_status,
							param->argv[1], BCM_AUTHTYPE_WPA2_AES_PSK,
							(const uint8_t *)param->argv[2],
							strlen(param->argv[2]));
	}
	else
	{
		err = bcm_wifi_join(&join->local_pt, evt, &join->cur_status,
							param->argv[1], BCM_AUTHTYPE_OPEN, NULL, 0);
	}
	if (err < 0)
	{
		vsfshell_printf(output_pt, "Fail to join %s" VSFSHELL_LINEEND,
						param->argv[1]);
		goto handler_thread_end;
	}
	else if (join->cur_status != 0)
	{
		// process join status change, non-block code here
	}
	if (err != 0) return err;

	vsfshell_printf(output_pt, "%s connected" VSFSHELL_LINEEND, param->argv[1]);

	// start dhcp
dhcp_retry:
	memset(&wifi->dhcpc, 0, sizeof(wifi->dhcpc));
	vsfsm_sem_init(&wifi->dhcpc.update_sem, 0, VSFSM_EVT_USER_LOCAL);
	vsfip_dhcpc_start(&wifi->netif, &wifi->dhcpc);
	if (vsfsm_sem_pend(&wifi->dhcpc.update_sem, pt->sm))
	{
		vsfsm_pt_wfe(pt, VSFSM_EVT_USER_LOCAL);
	}
	if (!wifi->dhcpc.ready)
	{
		goto dhcp_retry;
	}
	vsfshell_printf(output_pt, "dhcp update:" VSFSHELL_LINEEND);
	vsfshell_printf(output_pt, "IP: %d.%d.%d.%d" VSFSHELL_LINEEND,
			wifi->netif.ipaddr.addr.s_addr_buf[0],
			wifi->netif.ipaddr.addr.s_addr_buf[1],
			wifi->netif.ipaddr.addr.s_addr_buf[2],
			wifi->netif.ipaddr.addr.s_addr_buf[3]);
	vsfshell_printf(output_pt, "subnet: %d.%d.%d.%d" VSFSHELL_LINEEND,
			wifi->netif.netmask.addr.s_addr_buf[0],
			wifi->netif.netmask.addr.s_addr_buf[1],
			wifi->netif.netmask.addr.s_addr_buf[2],
			wifi->netif.netmask.addr.s_addr_buf[3]);
	vsfshell_printf(output_pt, "gateway: %d.%d.%d.%d" VSFSHELL_LINEEND,
			wifi->netif.gateway.addr.s_addr_buf[0],
			wifi->netif.gateway.addr.s_addr_buf[1],
			wifi->netif.gateway.addr.s_addr_buf[2],
			wifi->netif.gateway.addr.s_addr_buf[3]);

handler_thread_end:
	if (param->priv != NULL)
	{
		vsf_bufmgr_free(param->priv);
	}
	vsfshell_handler_exit(pt);
	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}

static vsf_err_t
vsfshell_bcmwifi_leave_handler(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfshell_handler_param_t *param =
						(struct vsfshell_handler_param_t *)pt->user_data;
	struct vsfshell_bcmwifi_t *wifi =
						(struct vsfshell_bcmwifi_t *)param->context;
	struct vsfsm_pt_t *output_pt = &param->output_pt;
	struct vsfsm_pt_t *local_pt = (struct vsfsm_pt_t *)param->priv;
	vsf_err_t err;

	vsfsm_pt_begin(pt);

	param->priv = vsf_bufmgr_malloc(sizeof(struct vsfsm_pt_t));
	if (NULL == param->priv)
	{
		vsfshell_printf(output_pt, "not enough resources." VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	local_pt = (struct vsfsm_pt_t *)param->priv;

	local_pt->sm = pt->sm;
	local_pt->user_data = &wifi->netif;
	local_pt->state = 0;
	vsfsm_pt_entry(pt);
	err = bcm_wifi_leave(local_pt, evt);
	if (err < 0)
	{
		vsfshell_printf(output_pt, "Fail to leave current AP" VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	else if (err != 0) return err;

handler_thread_end:
	if (param->priv != NULL)
	{
		vsf_bufmgr_free(param->priv);
	}
	vsfshell_handler_exit(pt);
	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}

struct vsfshell_bcmwifi_dns_t
{
	struct vsfsm_pt_t local_pt;
	struct vsfip_ipaddr_t domainip;
};
static vsf_err_t
vsfshell_bcmwifi_dns_handler(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfshell_handler_param_t *param =
						(struct vsfshell_handler_param_t *)pt->user_data;
	struct vsfshell_bcmwifi_t *wifi =
						(struct vsfshell_bcmwifi_t *)param->context;
	struct vsfip_netif_t *netif = &wifi->netif;
	struct vsfsm_pt_t *output_pt = &param->output_pt;
	struct vsfshell_bcmwifi_dns_t *dns = (struct vsfshell_bcmwifi_dns_t *)param->priv;
	vsf_err_t err;

	vsfsm_pt_begin(pt);
	if (param->argc != 2)
	{
		vsfshell_printf(output_pt, "invalid format." VSFSHELL_LINEEND);
		vsfshell_printf(output_pt, "format: bcm.dns TARGET " VSFSHELL_LINEEND);
		goto handler_thread_end;
	}

	if ((0 == netif->ipaddr.addr.s_addr) || (0 == netif->ipaddr.size))
	{
		vsfshell_printf(output_pt, "ipaddr not configured" VSFSHELL_LINEEND);
		goto handler_thread_end;
	}

	param->priv = vsf_bufmgr_malloc(sizeof(struct vsfshell_bcmwifi_dns_t));
	if (NULL == param->priv)
	{
		vsfshell_printf(output_pt, "not enough resources." VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	dns = (struct vsfshell_bcmwifi_dns_t *)param->priv;

	vsfip_dns_init();
	err = vsfip_dns_setserver(0, &netif->dns[0]);
	if (err != VSFERR_NONE)
		goto failed;

	vsfshell_printf(output_pt, "dns server: %d:%d:%d:%d" VSFSHELL_LINEEND,
					netif->dns[0].addr.s_addr_buf[0], netif->dns[0].addr.s_addr_buf[1],
					netif->dns[0].addr.s_addr_buf[2], netif->dns[0].addr.s_addr_buf[3]);

	dns->local_pt.sm = pt->sm;
	dns->local_pt.state = 0;
	dns->local_pt.user_data = NULL;
	vsfsm_pt_entry(pt);
	err = vsfip_dns_gethostbyname(&dns->local_pt, evt, param->argv[1], &dns->domainip);
	if (err > 0) return err; else if (err < 0)
	{
	failed:
		vsfshell_printf(output_pt, "Failed" VSFSHELL_LINEEND);
		goto handler_thread_end;
	}

	vsfshell_printf(output_pt, "domainip: %d:%d:%d:%d" VSFSHELL_LINEEND,
					dns->domainip.addr.s_addr_buf[0], dns->domainip.addr.s_addr_buf[1],
					dns->domainip.addr.s_addr_buf[2], dns->domainip.addr.s_addr_buf[3]);

handler_thread_end:
	if (param->priv != NULL)
	{
		vsf_bufmgr_free(param->priv);
	}

	vsfshell_handler_exit(pt);
	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}

vsf_err_t bcmwifi_handlers_init(struct vsfshell_bcmwifi_t *bcmwifi,
			struct vsfshell_t *shell, struct vsfshell_handler_t *handlers, 
			struct app_hwcfg_t const *hwcfg)
{
	bcmwifi->netif.drv = &bcmwifi->netdrv;
	bcmwifi->netdrv.op = &bcm_wifi_netdrv_op;
	bcmwifi->netdrv.param = &bcmwifi->bcm_wifi;

	bcmwifi->bcm_wifi.bus.type = hwcfg->bcmwifi.type;
	switch (bcmwifi->bcm_wifi.bus.type)
	{
#if IFS_SPI_EN
	case BCM_BUS_TYPE_SPI:
		bcmwifi->bcm_wifi.bus.op = &bcm_bus_spi_op;
		break;
#endif
#if IFS_SDIO_EN
	case BCM_BUS_TYPE_SDIO:
		bcmwifi->bcm_wifi.bus.op = &bcm_bus_sdio_op;
		break;
#endif
	}
	bcmwifi->bcm_wifi.bus.port.index = hwcfg->bcmwifi.index;
	bcmwifi->bcm_wifi.bus.port.freq_khz = hwcfg->bcmwifi.freq_khz;

	bcmwifi->bcm_wifi.bus.port.rst_port = hwcfg->bcmwifi.rst.port;
	bcmwifi->bcm_wifi.bus.port.rst_pin = hwcfg->bcmwifi.rst.pin;
	bcmwifi->bcm_wifi.bus.port.wakeup_port = hwcfg->bcmwifi.wakeup.port;
	bcmwifi->bcm_wifi.bus.port.wakeup_pin = hwcfg->bcmwifi.wakeup.pin;
	bcmwifi->bcm_wifi.bus.port.mode_port = hwcfg->bcmwifi.mode.port;
	bcmwifi->bcm_wifi.bus.port.mode_pin = hwcfg->bcmwifi.mode.pin;

	bcmwifi->bcm_wifi.bus.port.spi.cs_port = hwcfg->bcmwifi.spi.cs.port;
	bcmwifi->bcm_wifi.bus.port.spi.cs_pin = hwcfg->bcmwifi.spi.cs.pin;
	bcmwifi->bcm_wifi.bus.port.spi.eint_port = hwcfg->bcmwifi.spi.eint.port;
	bcmwifi->bcm_wifi.bus.port.spi.eint_pin = hwcfg->bcmwifi.spi.eint.pin;
	bcmwifi->bcm_wifi.bus.port.spi.eint = hwcfg->bcmwifi.spi.eint_idx;

	bcmwifi->pwrctrl_port = hwcfg->bcmwifi.pwrctrl.port;
	bcmwifi->pwrctrl_pin = hwcfg->bcmwifi.pwrctrl.pin;

	bcmwifi->bcm_wifi.country = "US";

	handlers[0].name = "bcm.init";
	handlers[0].thread = vsfshell_bcmwifi_init_handler;
	handlers[0].context = bcmwifi;
	handlers[1].name = "bcm.fini";
	handlers[1].thread = vsfshell_bcmwifi_fini_handler;
	handlers[1].context = bcmwifi;
	handlers[2].name = "bcm.scan";
	handlers[2].thread = vsfshell_bcmwifi_scan_handler;
	handlers[2].context = bcmwifi;
	handlers[3].name = "bcm.join";
	handlers[3].thread = vsfshell_bcmwifi_join_handler;
	handlers[3].context = bcmwifi;
	handlers[4].name = "bcm.leave";
	handlers[4].thread = vsfshell_bcmwifi_leave_handler;
	handlers[4].context = bcmwifi;
	handlers[5].name = "bcm.dns";
	handlers[5].thread = vsfshell_bcmwifi_dns_handler;
	handlers[5].context = bcmwifi;
	handlers[6].name = NULL;

	vsfshell_register_handlers(shell, handlers);
	return VSFERR_NONE;
}
