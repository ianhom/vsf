#ifndef __VSFIP_DHCP_H_INCLUDED__
#define __VSFIP_DHCP_H_INCLUDED__

struct vsfip_dhcp_t
{
	enum vsfip_dhcp_state_t
	{
		VSFIP_DHCPSTAT_OFF,
		VSFIP_DHCPSTAT_EXIT,
	} state;
	
	struct vsfip_netif_t *netif;
	struct vsfsm_t sm;
	struct vsfsm_pt_t pt;
	struct vsftimer_timer_t timer_retry;
	struct vsfip_socket_t *so;
	struct vsfip_sockaddr_t sockaddr;
	struct vsfip_buffer_t *outbuffer;
	struct vsfip_buffer_t *inbuffer;
	struct vsfip_ipaddr_t ipaddr;
	struct vsfip_ipaddr_t gw;
	struct vsfip_ipaddr_t netmask;
	struct vsfip_buffer_t fakebuffer;
	struct vsfsm_pt_t caller_pt;
	struct vsfsm_sem_t update_sem;
	struct vsfsm_sem_t release_sem;
	uint32_t optlen;
	uint32_t starttick;
	uint32_t xid;
	uint32_t retry;
	uint32_t arp_retry;
	uint32_t leasetime;
	uint32_t t1;
	uint32_t t2;
	bool resume;
};

vsf_err_t vsfip_dhcp_start(struct vsfip_netif_t *netif,
							struct vsfip_dhcp_t *dhcp);

#endif		// __VSFIP_DHCP_H_INCLUDED__
