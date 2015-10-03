
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
		core_interfaces.gpio.config_pin(wifi->pwrctrl_port, wifi->pwrctrl_pin,
										GPIO_OUTPP);
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
dhcp_retry:
	memset(&wifi->dhcp, 0, sizeof(wifi->dhcp));
	vsfsm_sem_init(&wifi->dhcp.update_sem, 0, VSFSM_EVT_USER_LOCAL);
	vsfip_dhcp_start(&wifi->netif, &wifi->dhcp);
	if (vsfsm_sem_pend(&wifi->dhcp.update_sem, pt->sm))
	{
		vsfsm_pt_wfe(pt, VSFSM_EVT_USER_LOCAL);
	}
	if (!wifi->dhcp.ready)
	{
		goto dhcp_retry;
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

#ifndef VSF_STANDALONE_APP

#include <stdlib.h>

struct vsfshell_bcm_ioctrl_t
{
	struct vsfsm_pt_t local_pt;
	struct vsfip_buffer_t *cmd_buffer;
	struct vsfip_buffer_t *reply_buffer;
};
static vsf_err_t

vsfshell_bcm_ver_handler(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfshell_handler_param_t *param =
						(struct vsfshell_handler_param_t *)pt->user_data;
	struct vsfshell_bcm_wifi_t *wifi =
						(struct vsfshell_bcm_wifi_t *)param->context;
	struct vsfsm_pt_t *output_pt = &param->output_pt;
	struct vsfshell_bcm_ioctrl_t *ioctrl =
							(struct vsfshell_bcm_ioctrl_t *)param->priv;
	vsf_err_t err;
	
	vsfsm_pt_begin(pt);
	param->priv = vsf_bufmgr_malloc(sizeof(struct vsfshell_bcm_ioctrl_t));
	if (NULL == param->priv)
	{
		vsfshell_printf(output_pt, "not enough resources." VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	
	ioctrl = (struct vsfshell_bcm_ioctrl_t *)param->priv;
	bcm_sdpcm_get_iovar_buffer(&ioctrl->cmd_buffer, 256, BCMIOVAR_VER);
	if (NULL == ioctrl->cmd_buffer) return VSFERR_FAIL;
	ioctrl->local_pt.sm = pt->sm;
	ioctrl->local_pt.state = 0;
	ioctrl->local_pt.user_data = &wifi->bcm_wifi;
	vsfsm_pt_entry(pt);
	err = bcm_sdpcm_ioctrl(&ioctrl->local_pt, evt, BCM_INTERFACE_STA,
							BCM_SDPCM_CMDTYPE_GET, BCMIOCTRL_GET_VAR,
							ioctrl->cmd_buffer, &ioctrl->reply_buffer);
	if (err < 0)
	{
		vsfshell_printf(output_pt, "Fail to read version." VSFSHELL_LINEEND);
		vsfip_buffer_release(ioctrl->cmd_buffer);
		goto handler_thread_end;
	}
	else if (err != 0)
	{
		return err;
	}
	vsfip_buffer_release(ioctrl->cmd_buffer);
	if ((ioctrl->reply_buffer != NULL) && (ioctrl->reply_buffer->buf.size > 0))
	{
		vsfshell_printf(output_pt, "%s" VSFSHELL_LINEEND,
						ioctrl->reply_buffer->buf.buffer);
	}
	else
	{
		vsfshell_printf(output_pt, "Invalid version." VSFSHELL_LINEEND);
	}
	vsfip_buffer_release(ioctrl->reply_buffer);
	
handler_thread_end:
	if (param->priv != NULL)
	{
		vsf_bufmgr_free(param->priv);
	}
	vsfshell_handler_exit(pt);
	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}

bool input_filter(struct vsfip_buffer_t *buf)
{
	return true;
}
bool output_filter(struct vsfip_buffer_t *buf)
{
	return true;
}
extern void vsfip_eth_input(struct vsfip_buffer_t *buf);
static vsf_err_t
vsfip_netif_ip_input(struct vsfip_buffer_t *buf)
{
	vsfip_eth_input(buf);
	return VSFERR_NONE;
}
static vsf_err_t
vsfshell_bcm_sniffer_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt);
struct vsfshell_sniffer_t
{
	bool started;
	
	struct sniffer_t
	{
		char *head;
		bool (*filter)(struct vsfip_buffer_t *buf);
		vsf_err_t (*transact)(struct vsfip_buffer_t *buf);
		
		int num;
		struct vsfsm_pt_t pt;
		
		// private
		struct vsfq_t list;
		struct vsfsm_sem_t sem;
		
		struct vsfsm_t sm;
		struct vsfsm_pt_t output_pt;
		
		char line[16 * 3 + 2];
		struct vsfip_buffer_t *cur_buf;
		uint8_t *cur_bufptr;
		uint32_t cur_pos;
		uint32_t cur_len;
	} in, out;
	
	// private
	struct vsfsm_crit_t crit;
} static wifi_sniffer =
{
	false,						// bool started;
	{
		"input: ",				// char *head;
		input_filter,			// bool (*filter)(struct vsfip_buffer_t *buf);
		vsfip_netif_ip_input,	// vsf_err_t (*transact)(struct vsfip_buffer_t *buf);
		
		0,						// int num;
		{
			vsfshell_bcm_sniffer_thread,
								// vsfsm_pt_thread_t thread;
			&wifi_sniffer.in,	// void *user_data;
		},						// struct vsfsm_pt_t pt;
	},							// struct sniffer_t in;
	{
		"output: ",				// char *head;
		output_filter,			// bool (*filter)(struct vsfip_buffer_t *buf);
		vsfip_netif_ip_output,	// vsf_err_t (*transact)(struct vsfip_buffer_t *buf);
		
		0,						// int num;
		{
			vsfshell_bcm_sniffer_thread,
								// vsfsm_pt_thread_t thread;
			&wifi_sniffer.out,	// void *user_data;
		},						// struct vsfsm_pt_t pt;
	},							// struct sniffer_t out;
};
static void vsfshell_bcm_transact_sniffer(struct sniffer_t *s,
											struct vsfip_buffer_t *buf)
{
	if ((s->filter != NULL) && s->filter(buf))
	{
		vsfq_append(&s->list, &buf->node);
		vsfsm_sem_post(&s->sem);
	}
	else if (s->transact != NULL)
	{
		s->transact(buf);
	}
	else
	{
		vsfip_buffer_release(buf);
	}
}
static void vsfshell_bcm_input_sniffer(struct vsfip_buffer_t *buf)
{
	vsfshell_bcm_transact_sniffer(&wifi_sniffer.in, buf);
}
static void vsfshell_bcm_output_sniffer(struct vsfip_buffer_t *buf)
{
	vsfshell_bcm_transact_sniffer(&wifi_sniffer.out, buf);
}

static const char* convert = "0123456789ABCDEF";
static vsf_err_t
vsfshell_bcm_sniffer_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct sniffer_t *sniffer = (struct sniffer_t *)pt->user_data;
	struct vsfsm_pt_t *output_pt = &sniffer->output_pt;
	uint8_t byte;
	uint32_t i;
	
	vsfsm_pt_begin(pt);
	while (1)
	{
		if (vsfsm_sem_pend(&sniffer->sem, pt->sm))
		{
			vsfsm_pt_wfe(pt, VSFSM_EVT_USER_LOCAL);
		}
		
		// print the buffer if available
		sniffer->cur_buf = (struct vsfip_buffer_t *)vsfq_dequeue(&sniffer->list);
		if (sniffer->cur_buf != NULL)
		{
			sniffer->cur_bufptr = sniffer->cur_buf->buf.buffer;
			sniffer->cur_pos = 0;
			
			if (vsfsm_crit_enter(&wifi_sniffer.crit, pt->sm))
			{
				vsfsm_pt_wfe(pt, VSFSM_EVT_USER_LOCAL);
			}
			
			vsfshell_printf(output_pt, "%s%d %d %d:" VSFSHELL_LINEEND,
				sniffer->head, sniffer->num, interfaces->tickclk.get_count(),
				sniffer->cur_buf->buf.size);
			while (sniffer->cur_pos < sniffer->cur_buf->buf.size)
			{
				sniffer->cur_len =
						min(sniffer->cur_buf->buf.size - sniffer->cur_pos, 16);
				for (i = 0; i < sniffer->cur_len; i++)
				{
					byte = sniffer->cur_bufptr[sniffer->cur_pos + i];
					sniffer->line[3 * i + 0] = convert[(byte >> 4) & 0x0F];
					sniffer->line[3 * i + 1] = convert[(byte >> 0) & 0x0F];
					sniffer->line[3 * i + 2] = ' ';
				}
				sniffer->line[3 * i] = '\0';
				vsfshell_printf(output_pt, "%s%s" VSFSHELL_LINEEND,
								sniffer->head, sniffer->line);
				sniffer->cur_pos += sniffer->cur_len;
			}
			
			vsfsm_crit_leave(&wifi_sniffer.crit);
			
			if (sniffer->transact != NULL)
			{
				sniffer->transact(sniffer->cur_buf);
			}
			else
			{
				vsfip_buffer_release(sniffer->cur_buf);
			}
			sniffer->num++;
		}
	}
	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}

static vsf_err_t
vsfshell_bcm_sniffer_handler(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfshell_handler_param_t *param =
						(struct vsfshell_handler_param_t *)pt->user_data;
	
	vsfsm_pt_begin(pt);
	if (wifi_sniffer.started)
	{
		return VSFERR_FAIL;
	}
	wifi_sniffer.started = true;
	
	vsfsm_crit_init(&wifi_sniffer.crit, VSFSM_EVT_USER_LOCAL);
	
	wifi_sniffer.in.output_pt = param->output_pt;
	wifi_sniffer.in.output_pt.sm = &wifi_sniffer.in.sm;
	vsfq_init(&wifi_sniffer.in.list);
	vsfip_input_sniffer = vsfshell_bcm_input_sniffer;
	vsfsm_sem_init(&wifi_sniffer.in.sem, 0, VSFSM_EVT_USER_LOCAL);
	vsfsm_pt_init(&wifi_sniffer.in.sm, &wifi_sniffer.in.pt);
	
	wifi_sniffer.out.output_pt = param->output_pt;
	wifi_sniffer.out.output_pt.sm = &wifi_sniffer.out.sm;
	vsfq_init(&wifi_sniffer.out.list);
	vsfip_output_sniffer = vsfshell_bcm_output_sniffer;
	vsfsm_sem_init(&wifi_sniffer.out.sem, 0, VSFSM_EVT_USER_LOCAL);
	vsfsm_pt_init(&wifi_sniffer.out.sm, &wifi_sniffer.out.pt);
	
	vsfshell_handler_exit(pt);
	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}

struct vsfshell_bcm_reg_t
{
	struct vsfsm_pt_t local_pt;
	uint32_t addr;
	uint8_t len;
	uint32_t value;
};
static vsf_err_t
vsfshell_bcm_readreg_handler(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfshell_handler_param_t *param =
						(struct vsfshell_handler_param_t *)pt->user_data;
	struct vsfshell_bcm_wifi_t *wifi =
						(struct vsfshell_bcm_wifi_t *)param->context;
	struct vsfsm_pt_t *output_pt = &param->output_pt;
	vsf_err_t err;
	struct vsfshell_bcm_reg_t *reg = (struct vsfshell_bcm_reg_t *)param->priv;
	
	vsfsm_pt_begin(pt);
	if (param->argc != 3)
	{
		vsfshell_printf(output_pt, "invalid format." VSFSHELL_LINEEND);
		vsfshell_printf(output_pt, "format: bcm.readreg ADDR LEN" VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	
	param->priv = vsf_bufmgr_malloc(sizeof(struct vsfshell_bcm_reg_t));
	if (NULL == param->priv)
	{
		vsfshell_printf(output_pt, "not enough resources." VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	
	reg = (struct vsfshell_bcm_reg_t *)param->priv;
	reg->addr = (uint32_t)strtoul(param->argv[1], NULL, 0);
	reg->len = (uint8_t)strtoul(param->argv[2], NULL, 0);
	if (reg->len > 4)
	{
		vsfshell_printf(output_pt, "invalid register size." VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	
	reg->local_pt.sm = pt->sm;
	reg->local_pt.state = 0;
	reg->local_pt.user_data = &wifi->bcm_wifi.bus;
	vsfsm_pt_entry(pt);
	err = bcm_bus_read_reg(&reg->local_pt, evt, reg->addr, reg->len,
							(uint8_t *)&reg->value);
	if (err < 0)
	{
		vsfshell_printf(output_pt, "Fail." VSFSHELL_LINEEND);
	}
	else if (err > 0)
	{
		return VSFERR_NOT_READY;
	}
	else
	{
		vsfshell_printf(output_pt, "reg = 0x%08X" VSFSHELL_LINEEND, reg->value);
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
vsfshell_bcm_writereg_handler(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfshell_handler_param_t *param =
						(struct vsfshell_handler_param_t *)pt->user_data;
	struct vsfshell_bcm_wifi_t *wifi =
						(struct vsfshell_bcm_wifi_t *)param->context;
	struct vsfsm_pt_t *output_pt = &param->output_pt;
	vsf_err_t err;
	struct vsfshell_bcm_reg_t *reg = (struct vsfshell_bcm_reg_t *)param->priv;
	
	vsfsm_pt_begin(pt);
	if (param->argc != 4)
	{
		vsfshell_printf(output_pt, "invalid format." VSFSHELL_LINEEND);
		vsfshell_printf(output_pt, "format: bcm.writereg ADDR LEN VALUE" VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	
	param->priv = vsf_bufmgr_malloc(sizeof(struct vsfshell_bcm_reg_t));
	if (NULL == param->priv)
	{
		vsfshell_printf(output_pt, "not enough resources." VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	
	reg = (struct vsfshell_bcm_reg_t *)param->priv;
	reg->addr = (uint32_t)strtoul(param->argv[1], NULL, 0);
	reg->len = (uint8_t)strtoul(param->argv[2], NULL, 0);
	if (reg->len > 4)
	{
		vsfshell_printf(output_pt, "invalid register size." VSFSHELL_LINEEND);
		goto handler_thread_end;
	}
	reg->value = (uint32_t)strtoul(param->argv[3], NULL, 0);
	
	reg->local_pt.sm = pt->sm;
	reg->local_pt.state = 0;
	reg->local_pt.user_data = &wifi->bcm_wifi.bus;
	vsfsm_pt_entry(pt);
	err = bcm_bus_write_reg(&reg->local_pt, evt, reg->addr, reg->len,
							reg->value);
	if (err < 0)
	{
		vsfshell_printf(output_pt, "Fail." VSFSHELL_LINEEND);
	}
	else if (err > 0)
	{
		return VSFERR_NOT_READY;
	}
	else
	{
		vsfshell_printf(output_pt, "Succeed." VSFSHELL_LINEEND);
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

#endif		// VSF_STANDALONE_APP

vsf_err_t bcm_handlers_init(struct vsfshell_t *shell,
							struct app_hwcfg_t const *hwcfg)
{
#ifndef VSF_STANDALONE_APP
	uint32_t size = sizeof(struct vsfshell_handler_t) * 10;
#else
	uint32_t size = sizeof(struct vsfshell_handler_t) * 6;
#endif
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
	wifi->netdrv.op = &bcm_wifi_netdrv_op;
	wifi->netdrv.param = &wifi->bcm_wifi;

	wifi->bcm_wifi.bus.type = hwcfg->bcm_wifi_port.type;
	switch (wifi->bcm_wifi.bus.type)
	{
#if IFS_SPI_EN
	case BCM_BUS_TYPE_SPI:
		wifi->bcm_wifi.bus.op = &bcm_bus_spi_op;
		break;
#endif
#if IFS_SDIO_EN
	case BCM_BUS_TYPE_SDIO:
		wifi->bcm_wifi.bus.op = &bcm_bus_sdio_op;
		break;
#endif
	}
	wifi->bcm_wifi.bus.port.index = hwcfg->bcm_wifi_port.index;
	wifi->bcm_wifi.bus.port.freq_khz = hwcfg->bcm_wifi_port.freq_khz;

	wifi->bcm_wifi.bus.port.rst_port = hwcfg->bcm_wifi_port.rst_port;
	wifi->bcm_wifi.bus.port.rst_pin = hwcfg->bcm_wifi_port.rst_pin;
	wifi->bcm_wifi.bus.port.wakeup_port = hwcfg->bcm_wifi_port.wakeup_port;
	wifi->bcm_wifi.bus.port.wakeup_pin = hwcfg->bcm_wifi_port.wakeup_pin;
	wifi->bcm_wifi.bus.port.mode_port = hwcfg->bcm_wifi_port.mode_port;
	wifi->bcm_wifi.bus.port.mode_pin = hwcfg->bcm_wifi_port.mode_pin;

	wifi->bcm_wifi.bus.port.spi.cs_port = hwcfg->bcm_wifi_port.spi.cs_port;
	wifi->bcm_wifi.bus.port.spi.cs_pin = hwcfg->bcm_wifi_port.spi.cs_pin;
	wifi->bcm_wifi.bus.port.spi.eint_port = hwcfg->bcm_wifi_port.spi.eint_port;
	wifi->bcm_wifi.bus.port.spi.eint_pin = hwcfg->bcm_wifi_port.spi.eint_pin;
	wifi->bcm_wifi.bus.port.spi.eint = hwcfg->bcm_wifi_port.spi.eint;

	wifi->pwrctrl_port = hwcfg->bcm_wifi_port.pwrctrl_port;
	wifi->pwrctrl_pin = hwcfg->bcm_wifi_port.pwrctrl_pin;

	wifi->bcm_wifi.country = "US";

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
#ifndef VSF_STANDALONE_APP
	bcm_handlers[5].name = "bcm.ver";
	bcm_handlers[5].thread = vsfshell_bcm_ver_handler;
	bcm_handlers[5].context = wifi;
	bcm_handlers[6].name = "bcm.sniffer";
	bcm_handlers[6].thread = vsfshell_bcm_sniffer_handler;
	bcm_handlers[6].context = wifi;
	bcm_handlers[7].name = "bcm.readreg";
	bcm_handlers[7].thread = vsfshell_bcm_readreg_handler;
	bcm_handlers[7].context = wifi;
	bcm_handlers[8].name = "bcm.writereg";
	bcm_handlers[8].thread = vsfshell_bcm_writereg_handler;
	bcm_handlers[8].context = wifi;
#endif

	vsfshell_register_handlers(shell, bcm_handlers);
	return VSFERR_NONE;
}
