#ifndef __VSFIP_NETIF_H_INCLUDED__
#define __VSFIP_NETIF_H_INCLUDED__

#include "../vsfip_buffer.h"
#include "../vsfip.h"

enum vsfip_netif_proto_t
{
	VSFIP_NETIF_PROTO_IP = 0x0800,
	VSFIP_NETIF_PROTO_ARP = 0x0806,
	VSFIP_NETIF_PROTO_RARP = 0x8035
};

struct vsfip_arp_entry_t
{
	struct vsfip_ipaddr_t ipaddr;
	struct vsfip_macaddr_t macaddr;
	uint32_t time;
};

struct vsfip_netif_t;
struct vsfip_netdrv_op_t
{
	vsf_err_t (*init)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt);
	vsf_err_t (*fini)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt);
	
	vsf_err_t (*header)(struct vsfip_buffer_t *buf,
						enum vsfip_netif_proto_t proto,
						const struct vsfip_macaddr_t *dest_addr);
};

struct vsfip_netdrv_t
{
	const struct vsfip_netdrv_op_t *op;
	void *param;
	uint32_t netif_header_size;
	uint32_t drv_header_size;
	uint16_t hwtype;
};

struct vsfip_netif_t
{
	struct vsfip_netdrv_t *drv;
	struct vsfip_macaddr_t macaddr;
	
	struct vsfip_ipaddr_t ipaddr;
	struct vsfip_ipaddr_t netmask;
	struct vsfip_ipaddr_t gateway;
	
	uint16_t mtu;
	
	// output bufferlist and semaphore
	struct vsfip_bufferlist_t output_queue;
	struct vsfsm_sem_t output_sem;
	
	// arp client
	struct
	{
		struct vsfsm_t sm;
		struct vsfsm_pt_t pt;
		struct vsfsm_sem_t sem;
		struct vsfsm_pt_t caller_pt;
		struct vsfsm_t *sm_pending;
		struct vsftimer_timer_t to;
		struct vsfip_buffer_t *buf;
		struct vsfip_bufferlist_t requestlist;
		struct vsfip_buffer_t *cur_request;
	} arpc;
	
	uint32_t arp_time;
	struct vsfip_macaddr_t mac_broadcast;
	struct vsfip_arp_entry_t arp_cache[VSFIP_CFG_ARPCACHE_SIZE];
	
	struct vsfsm_pt_t init_pt;
	bool connected;
	bool quit;
	
	// for DHCP
	struct vsfip_dhcp_t *dhcp;
	
	struct vsfip_netif_t *next;
};

vsf_err_t vsfip_netif_construct(struct vsfip_netif_t *netif);
vsf_err_t vsfip_netif_init(struct vsfsm_pt_t *pt, vsfsm_evt_t evt);
vsf_err_t vsfip_netif_fini(struct vsfsm_pt_t *pt, vsfsm_evt_t evt);
vsf_err_t vsfip_netif_ip_output(struct vsfip_buffer_t *buf);

void vsfip_netif_ip4_input(struct vsfip_buffer_t *buf);
void vsfip_netif_ip6_input(struct vsfip_buffer_t *buf);
void vsfip_netif_arp_input(struct vsfip_buffer_t *buf);

#endif		// __VSFIP_NETIF_H_INCLUDED__
