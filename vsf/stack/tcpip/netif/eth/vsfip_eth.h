#ifndef __VSFIP_ETH_H_INCLUDED__
#define __VSFIP_ETH_H_INCLUDED__

#define VSFIP_ETH_HWTYPE				1
#define VSFIP_ETH_HEADSIZE				14

vsf_err_t vsfip_eth_header(struct vsfip_buffer_t *buf,
	enum vsfip_netif_proto_t proto, const struct vsfip_macaddr_t *dest_addr);

void vsfip_eth_input(struct vsfip_buffer_t *buf);

#endif		// __VSFIP_ETH_H_INCLUDED__
