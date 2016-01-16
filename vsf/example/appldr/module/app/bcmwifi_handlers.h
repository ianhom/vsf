#ifndef __BCM_HANDLERS_H_INCLUDED__
#define __BCM_HANDLERS_H_INCLUDED__

struct vsfshell_bcmwifi_t
{
	struct bcm_wifi_t bcm_wifi;
	struct vsfip_netdrv_t netdrv;
	struct vsfip_dhcpc_t dhcpc;
	struct vsfip_netif_t netif;

	uint8_t pwrctrl_port;
	uint8_t pwrctrl_pin;

	VSFPOOL_DEFINE(vsfip_buffer128_pool, struct vsfip_buffer_t, 16);
	VSFPOOL_DEFINE(vsfip_buffer_pool, struct vsfip_buffer_t, 8);
	VSFPOOL_DEFINE(vsfip_socket_pool, struct vsfip_socket_t, 16);
	VSFPOOL_DEFINE(vsfip_tcppcb_pool, struct vsfip_tcppcb_t, 16);
	struct vsfip_mem_op_t mem_op;

	uint8_t vsfip_buffer128_mem[16][128];
	uint8_t vsfip_buffer_mem[8][VSFIP_BUFFER_SIZE];
};

vsf_err_t bcmwifi_handlers_init(struct vsfshell_bcmwifi_t *bcmwifi,
			struct vsfshell_t *shell, struct vsfshell_handler_t *handlers, 
			struct app_hwcfg_t const *hwcfg);

#endif		// __BCM_HANDLERS_H_INCLUDED__
