
#include "compiler.h"

#include "app_cfg.h"
#include "app_type.h"

#include "app_hw_cfg.h"

#include "vsf.h"

#include "bcm_handlers.h"

struct vsfshell_bcm_wifi_t
{
	struct bcm_wifi_t bcm_wifi;
	struct vsfip_netdrv_t netdrv;
	struct vsfip_dhcp_t dhcp;
	struct vsfip_netif_t netif;

	uint8_t pwrctrl_port;
	uint8_t pwrctrl_pin;
};

struct vsfshell_bcm_init_t
{
	struct vsfsm_pt_t local_pt;
};
static vsf_err_t
vsfshell_bcm_init_handler(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfshell_handler_param_t *param =
						(struct vsfshell_handler_param_t *)pt->user_data;
	struct vsfshell_bcm_wifi_t *wifi =
						(struct vsfshell_bcm_wifi_t *)param->context;
	struct vsfsm_pt_t *output_pt = &param->output_pt;
	struct vsfshell_bcm_init_t *init =
							(struct vsfshell_bcm_init_t *)param->priv;
	vsf_err_t err;

	vsfsm_pt_begin(pt);
	if (param->argc != 1)
	{
		vsfshell_printf(output_pt, "invalid format." VSFSHELL_LINEEND);
		vsfshell_printf(output_pt, "format: bcm.init" VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	vsfshell_printf(output_pt, "Initialize BCM wifi module." VSFSHELL_LINEEND);

	param->priv = vsf_bufmgr_malloc(sizeof(struct vsfshell_bcm_init_t));
	if (NULL == param->priv)
	{
		vsfshell_printf(output_pt, "not enough resources." VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	init = (struct vsfshell_bcm_init_t *)param->priv;

	// power cycle
	if (wifi->pwrctrl_port != IFS_DUMMY_PORT)
	{
		core_interfaces.gpio.init(wifi->pwrctrl_port);
		core_interfaces.gpio.set(wifi->pwrctrl_port, 1 << wifi->pwrctrl_pin);
		core_interfaces.gpio.config_pin(wifi->pwrctrl_port,
					wifi->pwrctrl_pin, core_interfaces.gpio.constants.OUTPP);
		vsfsm_pt_delay(pt, 100);
		core_interfaces.gpio.clear(wifi->pwrctrl_port, 1 << wifi->pwrctrl_pin);
	}

	vsfip_init();

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
vsfshell_bcm_fini_handler(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfshell_handler_param_t *param =
						(struct vsfshell_handler_param_t *)pt->user_data;
	struct vsfshell_bcm_wifi_t *wifi =
						(struct vsfshell_bcm_wifi_t *)param->context;
	struct vsfsm_pt_t *output_pt = &param->output_pt;
	struct vsfshell_bcm_init_t *init =
							(struct vsfshell_bcm_init_t *)param->priv;
	vsf_err_t err;

	vsfsm_pt_begin(pt);
	if (param->argc != 1)
	{
		vsfshell_printf(output_pt, "invalid format." VSFSHELL_LINEEND);
		vsfshell_printf(output_pt, "format: bcm.init" VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	vsfshell_printf(output_pt, "Finalize BCM wifi module." VSFSHELL_LINEEND);

	param->priv = vsf_bufmgr_malloc(sizeof(struct vsfshell_bcm_init_t));
	if (NULL == param->priv)
	{
		vsfshell_printf(output_pt, "not enough resources." VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	init = (struct vsfshell_bcm_init_t *)param->priv;

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
		core_interfaces.gpio.set(wifi->pwrctrl_port, 1 << wifi->pwrctrl_pin);
		core_interfaces.gpio.config_pin(wifi->pwrctrl_port, wifi->pwrctrl_pin,
						core_interfaces.gpio.constants.INFLOAT);
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

struct vsfshell_bcm_scan_t
{
	struct vsfsm_pt_t local_pt;
	uint32_t index;
	struct bcm_wifi_bss_info_t bss_info[16];
	int i;
};
static vsf_err_t
vsfshell_bcm_scan_handler(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfshell_handler_param_t *param =
						(struct vsfshell_handler_param_t *)pt->user_data;
	struct vsfshell_bcm_wifi_t *wifi =
						(struct vsfshell_bcm_wifi_t *)param->context;
	struct vsfsm_pt_t *output_pt = &param->output_pt;
	struct vsfshell_bcm_scan_t *scan = (struct vsfshell_bcm_scan_t *)param->priv;
	vsf_err_t err;

	struct vsfip_buffer_t *result_buffer;
	struct bcm_wifi_escan_result_t *scan_result;
	int i, j;
	bool match;

	vsfsm_pt_begin(pt);
	param->priv = vsf_bufmgr_malloc(sizeof(struct vsfshell_bcm_scan_t));
	if (NULL == param->priv)
	{
		vsfshell_printf(output_pt, "not enough resources." VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	scan = (struct vsfshell_bcm_scan_t *)param->priv;

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

struct vsfshell_bcm_join_t
{
	struct vsfsm_pt_t local_pt;
	uint32_t cur_status;
};
static vsf_err_t
vsfshell_bcm_join_handler(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfshell_handler_param_t *param =
						(struct vsfshell_handler_param_t *)pt->user_data;
	struct vsfshell_bcm_wifi_t *wifi =
						(struct vsfshell_bcm_wifi_t *)param->context;
	struct vsfsm_pt_t *output_pt = &param->output_pt;
	struct vsfshell_bcm_join_t *join =
							(struct vsfshell_bcm_join_t *)param->priv;
	vsf_err_t err;

	vsfsm_pt_begin(pt);
	if (param->argc < 2)
	{
		vsfshell_printf(output_pt, "invalid format." VSFSHELL_LINEEND);
		vsfshell_printf(output_pt, "format: bcm.join SSID [PASSWORD]" VSFSHELL_LINEEND);
		goto handler_thread_end;
	}

	param->priv = vsf_bufmgr_malloc(sizeof(struct vsfshell_bcm_join_t));
	if (NULL == param->priv)
	{
		vsfshell_printf(output_pt, "not enough resources." VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	join = (struct vsfshell_bcm_join_t *)param->priv;

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
	memset(&wifi->dhcp, 0, sizeof(wifi->dhcp));
	vsfsm_sem_init(&wifi->dhcp.update_sem, 0, VSFSM_EVT_USER_LOCAL);
	vsfip_dhcp_start(&wifi->netif, &wifi->dhcp);
	if (vsfsm_sem_pend(&wifi->dhcp.update_sem, pt->sm))
	{
		vsfsm_pt_wfe(pt, VSFSM_EVT_USER_LOCAL);
	}
	vsfshell_printf(output_pt, "dhcp update:" VSFSHELL_LINEEND);
	vsfshell_printf(output_pt, "IP: %d:%d:%d:%d" VSFSHELL_LINEEND,
			wifi->netif.ipaddr.addr.s_addr_buf[0],
			wifi->netif.ipaddr.addr.s_addr_buf[1],
			wifi->netif.ipaddr.addr.s_addr_buf[2],
			wifi->netif.ipaddr.addr.s_addr_buf[3]);
	vsfshell_printf(output_pt, "subnet: %d:%d:%d:%d" VSFSHELL_LINEEND,
			wifi->netif.netmask.addr.s_addr_buf[0],
			wifi->netif.netmask.addr.s_addr_buf[1],
			wifi->netif.netmask.addr.s_addr_buf[2],
			wifi->netif.netmask.addr.s_addr_buf[3]);
	vsfshell_printf(output_pt, "gateway: %d:%d:%d:%d" VSFSHELL_LINEEND,
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
vsfshell_bcm_leave_handler(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfshell_handler_param_t *param =
						(struct vsfshell_handler_param_t *)pt->user_data;
	struct vsfshell_bcm_wifi_t *wifi =
						(struct vsfshell_bcm_wifi_t *)param->context;
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

vsf_err_t bcm_handlers_init(struct vsfshell_t *shell,
							struct app_hwcfg_t const *hwcfg)
{
	uint32_t size = sizeof(struct vsfshell_handler_t) * 6;
	struct vsfshell_handler_t *bcm_handlers =
				(struct vsfshell_handler_t *)vsf_bufmgr_malloc(size);
	struct vsfshell_bcm_wifi_t *wifi =
				(struct vsfshell_bcm_wifi_t *)vsf_bufmgr_malloc(sizeof(*wifi));

	if ((NULL == bcm_handlers) || (NULL == wifi))
	{
		if (bcm_handlers != NULL)
		{
			vsf_bufmgr_free(bcm_handlers);
		}
		if (wifi != NULL)
		{
			vsf_bufmgr_free(wifi);
		}
		return VSFERR_NOT_ENOUGH_RESOURCES;
	}

	memset(wifi, 0, sizeof(*wifi));
	wifi->netif.drv = &wifi->netdrv;
	wifi->netdrv.op = vsf.stack.tcpip.broadcom_wifi.netdrv_op;
	wifi->netdrv.param = &wifi->bcm_wifi;
	wifi->bcm_wifi.bus.type = hwcfg->bcm_wifi_port.type;
	wifi->bcm_wifi.bus.op = vsf.stack.tcpip.broadcom_wifi.bus.spi_op;
	wifi->bcm_wifi.bus.port.comm.spi.port = hwcfg->bcm_wifi_port.comm.spi.port;
	wifi->bcm_wifi.bus.port.comm.spi.cs_port = hwcfg->bcm_wifi_port.comm.spi.cs_port;
	wifi->bcm_wifi.bus.port.comm.spi.cs_pin = hwcfg->bcm_wifi_port.comm.spi.cs_pin;
	wifi->bcm_wifi.bus.port.comm.spi.freq_khz = hwcfg->bcm_wifi_port.comm.spi.freq_khz;
	wifi->bcm_wifi.bus.port.rst_port = hwcfg->bcm_wifi_port.rst_port;
	wifi->bcm_wifi.bus.port.rst_pin = hwcfg->bcm_wifi_port.rst_pin;
	wifi->bcm_wifi.bus.port.eint_port = hwcfg->bcm_wifi_port.eint_port;
	wifi->bcm_wifi.bus.port.eint_pin = hwcfg->bcm_wifi_port.eint_pin;
	wifi->bcm_wifi.bus.port.eint = hwcfg->bcm_wifi_port.eint;
	wifi->bcm_wifi.country = "US";

	wifi->pwrctrl_port = hwcfg->bcm_wifi_port.pwrctrl_port;
	wifi->pwrctrl_pin = hwcfg->bcm_wifi_port.pwrctrl_pin;

	memset(bcm_handlers, 0, size);
	bcm_handlers[0].name = "bcm.init";
	bcm_handlers[0].thread = vsfshell_bcm_init_handler;
	bcm_handlers[0].context = wifi;
	bcm_handlers[1].name = "bcm.fini";
	bcm_handlers[1].thread = vsfshell_bcm_fini_handler;
	bcm_handlers[1].context = wifi;
	bcm_handlers[2].name = "bcm.scan";
	bcm_handlers[2].thread = vsfshell_bcm_scan_handler;
	bcm_handlers[2].context = wifi;
	bcm_handlers[3].name = "bcm.join";
	bcm_handlers[3].thread = vsfshell_bcm_join_handler;
	bcm_handlers[3].context = wifi;
	bcm_handlers[4].name = "bcm.leave";
	bcm_handlers[4].thread = vsfshell_bcm_leave_handler;
	bcm_handlers[4].context = wifi;

	vsfshell_register_handlers(shell, bcm_handlers);
	return VSFERR_NONE;
}
