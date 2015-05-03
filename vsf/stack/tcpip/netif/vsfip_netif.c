#include "app_type.h"
#include "compiler.h"

#include "framework/vsfsm/vsfsm.h"
#include "framework/vsftimer/vsftimer.h"

#include "../vsfip.h"
#include "../vsfip_priv.h"
#include "vsfip_netif.h"

enum vsfip_netif_EVT_t
{
	VSFIP_NETIF_EVT_ARPS_REQUEST	= VSFSM_EVT_USER_LOCAL + 0,
	VSFIP_NETIF_EVT_ARPS_REPLIED	= VSFSM_EVT_USER_LOCAL + 1,
	VSFIP_NETIF_EVT_ARPC_REQUEST	= VSFSM_EVT_USER_LOCAL + 2,
	
	VSFIP_NETIF_EVT_ARPC_TIMEOUT = VSFSM_EVT_USER_LOCAL_INSTANT + 0,
	VSFIP_NETIF_EVT_ARPC_UPDATE	= VSFSM_EVT_USER_LOCAL_INSTANT + 1,
	VSFIP_NETIF_EVT_ARPC_REQOUT = VSFSM_EVT_USER_LOCAL_INSTANT + 2,
};

enum vsfip_netif_ARP_op_t
{
	ARP_REQUEST = 1,
	ARP_REPLY = 2,
	RARP_REQUEST = 3,
	RARP_REPLY = 4,
};

// ARP
PACKED_HEAD struct PACKED_MID vsfip_arphead_t
{
	uint16_t hwtype;
	uint16_t prototype;
	uint8_t hwlen;
	uint8_t protolen;
	uint16_t op;
}; PACKED_TAIL

static vsf_err_t vsfip_netif_arp_client_thread(struct vsfsm_pt_t *pt,
												vsfsm_evt_t evt);
vsf_err_t vsfip_netif_construct(struct vsfip_netif_t *netif)
{
	vsfip_bufferlist_init(&netif->output_queue);
	netif->arpc.to.evt = VSFIP_NETIF_EVT_ARPC_TIMEOUT;
	netif->arpc.buf = NULL;
	netif->arp_time = 1;
	
	vsfsm_sem_init(&netif->output_sem, 0, VSFSM_EVT_USER_LOCAL);
	
	vsfsm_sem_init(&netif->arpc.sem, 0, VSFIP_NETIF_EVT_ARPC_REQUEST);
	netif->arpc.pt.user_data = netif;
	netif->arpc.pt.thread = vsfip_netif_arp_client_thread;
	return vsfsm_pt_init(&netif->arpc.sm, &netif->arpc.pt);
}

vsf_err_t vsfip_netif_init(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	return ((struct vsfip_netif_t *)pt->user_data)->drv->op->init(pt, evt);
}

vsf_err_t vsfip_netif_fini(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	return ((struct vsfip_netif_t *)pt->user_data)->drv->op->fini(pt, evt);
}

static struct vsfip_macaddr_t *
vsfip_netif_get_mac(struct vsfip_netif_t *netif, struct vsfip_ipaddr_t *ip)
{
	uint32_t i;
	if (0xFFFFFFFF == ip->addr.s_addr)
	{
		return &netif->mac_broadcast;
	}
	for (i = 0; i < VSFIP_CFG_ARPCACHE_SIZE; i++)
	{
		if (netif->arp_cache[i].time &&
			(ip->addr.s_addr == netif->arp_cache[i].ipaddr.addr.s_addr))
		{
			return &netif->arp_cache[i].macaddr;
		}
	}
	return NULL;
}

static vsf_err_t vsfip_netif_ip_output_do(struct vsfip_buffer_t *buf,
					enum vsfip_netif_proto_t proto, struct vsfip_macaddr_t *mac)
{
	struct vsfip_netif_t *netif = buf->netif;
	vsf_err_t err = netif->drv->op->header(buf, proto, mac);
	if (err) goto cleanup;
	
	buf->next = NULL;
	vsfip_bufferlist_queue(&netif->output_queue, buf);
	err = vsfsm_sem_post(&netif->output_sem);
	if (err) goto cleanup; else goto end;
	
cleanup:
	vsfip_buffer_release(buf);
end:
	return err;
}

static void vsfip_netif_get_ipaddr(struct vsfip_buffer_t *buf,
									struct vsfip_ipaddr_t *ipaddr)
{
	uint8_t ipver = *buf->iphead.ipver >> 4;
	if (4 == ipver)
	{
		// IPv4
		ipaddr->size = 4;
		ipaddr->addr.s_addr = buf->iphead.ip4head->ipdest;
	}
	else //if (6 == ipver)
	{
		// IPv6
		ipaddr->size = 6;
	}
}

vsf_err_t vsfip_netif_ip_output(struct vsfip_buffer_t *buf)
{
	struct vsfip_netif_t *netif = buf->netif;
	struct vsfip_macaddr_t *mac;
	struct vsfip_ipaddr_t dest;
	vsf_err_t err = VSFERR_NONE;
	
	vsfip_netif_get_ipaddr(buf, &dest);
	mac = vsfip_netif_get_mac(netif, &dest);
	if (NULL == mac)
	{
		vsfip_bufferlist_queue(&netif->arpc.requestlist, buf);
		err = vsfsm_sem_post(&netif->arpc.sem);
		if (err) goto cleanup; else goto end;
	}
	else if (buf->buf.size)
	{
		return vsfip_netif_ip_output_do(buf, VSFIP_NETIF_PROTO_IP, mac);
	}
	
cleanup:
	vsfip_buffer_release(buf);
end:
	return err;
}

void vsfip_netif_ip4_input(struct vsfip_buffer_t *buf)
{
	vsfip_ip4_input(buf);
}

void vsfip_netif_ip6_input(struct vsfip_buffer_t *buf)
{
	vsfip_ip6_input(buf);
}

void vsfip_netif_arp_input(struct vsfip_buffer_t *buf)
{
	struct vsfip_netif_t *netif = buf->netif;
	struct vsfip_arphead_t *head = (struct vsfip_arphead_t *)buf->buf.buffer;
	uint8_t *ptr = (uint8_t *)head + sizeof(struct vsfip_arphead_t);
	struct vsfip_arp_entry_t *entry;
	uint8_t *bufptr;
	uint32_t i;
	
	// endian fix
	head->hwtype = BE_TO_SYS_U16(head->hwtype);
	head->prototype = BE_TO_SYS_U16(head->prototype);
	head->op = BE_TO_SYS_U16(head->op);
	
	switch (head->op)
	{
	case ARP_REQUEST:
		bufptr = (uint8_t *)head + sizeof(struct vsfip_arphead_t);
	
		if ((head->hwlen == netif->macaddr.size) &&
			(head->protolen == netif->ipaddr.size) &&
			(buf->buf.size == (sizeof(struct vsfip_arphead_t) +
								2 * (head->hwlen + head->protolen))) &&
			!memcmp(bufptr + 2 * head->hwlen + head->protolen,
					netif->ipaddr.addr.s_addr_buf, head->protolen))
		{
			struct vsfip_macaddr_t macaddr;
			struct vsfip_ipaddr_t ipaddr;
			
			// process the ARP request
			head->hwtype = SYS_TO_BE_U16(netif->drv->hwtype);
			head->prototype = SYS_TO_BE_U16(VSFIP_NETIF_PROTO_IP);
			head->op = SYS_TO_BE_U16(ARP_REPLY);
			macaddr.size = head->hwlen;
			memcpy(macaddr.addr.s_addr_buf, bufptr, head->hwlen);
			memcpy(ipaddr.addr.s_addr_buf, bufptr + head->hwlen,
					head->protolen);
			memcpy(bufptr, netif->macaddr.addr.s_addr_buf, head->hwlen);
			memcpy(bufptr + head->hwlen, netif->ipaddr.addr.s_addr_buf,
					head->protolen);
			memcpy(bufptr + head->hwlen + head->protolen,
					macaddr.addr.s_addr_buf, head->hwlen);
			memcpy(bufptr + 2 * head->hwlen + head->protolen,
					ipaddr.addr.s_addr_buf, head->protolen);
			
			// send ARP reply
			vsfip_netif_ip_output_do(buf, VSFIP_NETIF_PROTO_ARP, &macaddr);
			return;
		}
		break;
	case ARP_REPLY:
		// for ARP reply, cache and send UPDATE event to netif->arpc.sm_pending
		if ((head->hwlen != netif->macaddr.size) ||
			(head->protolen != netif->ipaddr.size) ||
			((sizeof(struct vsfip_arphead_t) + 2 * (head->hwlen + head->protolen)) != buf->buf.size))
		{
			break;
		}
		
		// search a most suitable slot in the ARP cache
		entry = &netif->arp_cache[0];
		for (i = 0; i < VSFIP_CFG_ARPCACHE_SIZE; i++)
		{
			if (0 == netif->arp_cache[i].time)
			{
				entry = &netif->arp_cache[i];
				break;
			}
			if (netif->arp_cache[i].time < entry->time)
			{
				entry = &netif->arp_cache[i];
			}
		}
		entry->macaddr.size = head->hwlen;
		memcpy(entry->macaddr.addr.s_addr_buf, ptr, entry->macaddr.size);
		ptr += entry->macaddr.size;
		entry->ipaddr.size = head->protolen;
		memcpy(entry->ipaddr.addr.s_addr_buf, ptr, entry->ipaddr.size);
		entry->time = netif->arp_time++;
		
		if (netif->arpc.sm_pending != NULL)
		{
			vsfsm_post_evt(netif->arpc.sm_pending, VSFIP_NETIF_EVT_ARPC_UPDATE);
		}
		break;
	}
	vsfip_buffer_release(buf);
}

static vsf_err_t vsfip_netif_arp_client_thread(struct vsfsm_pt_t *pt,
												vsfsm_evt_t evt)
{
	struct vsfip_netif_t *netif = (struct vsfip_netif_t *)pt->user_data;
	struct vsfip_macaddr_t *mac;
	struct vsfip_ipaddr_t dest;
	struct vsfip_arphead_t *head;
	uint8_t *ptr;
	uint32_t headsize = netif->drv->netif_header_size +
						netif->drv->drv_header_size;
	
	vsfsm_pt_begin(pt);
	
	while (!netif->quit)
	{
		if (vsfsm_sem_pend(&netif->arpc.sem, pt->sm))
		{
			vsfsm_pt_wfe(pt, VSFIP_NETIF_EVT_ARPC_REQUEST);
		}
		
		netif->arpc.cur_request =
						vsfip_bufferlist_dequeue(&netif->arpc.requestlist);
		if (netif->arpc.cur_request != NULL)
		{
			vsfip_netif_get_ipaddr(netif->arpc.cur_request, &dest);
			mac = vsfip_netif_get_mac(netif, &dest);
			if (NULL == mac)
			{
				// generate ARP request body
				netif->arpc.buf = vsfip_buffer_get(128);
				if (netif->arpc.buf != NULL)
				{
					netif->arpc.buf->netif = netif;
					
					netif->arpc.buf->buf.buffer += headsize;
					head =
						(struct vsfip_arphead_t *)netif->arpc.buf->buf.buffer;
					head->hwtype = SYS_TO_BE_U16(netif->drv->hwtype);
					head->prototype = SYS_TO_BE_U16(VSFIP_NETIF_PROTO_IP);
					head->hwlen = (uint8_t)netif->macaddr.size;
					head->protolen = (uint8_t)netif->ipaddr.size;
					head->op = SYS_TO_BE_U16(ARP_REQUEST);
					ptr = (uint8_t *)head + sizeof(struct vsfip_arphead_t);
					memcpy(ptr, netif->macaddr.addr.s_addr_buf,
								netif->macaddr.size);
					ptr += netif->macaddr.size;
					memcpy(ptr, netif->ipaddr.addr.s_addr_buf,
								netif->ipaddr.size);
					ptr += netif->ipaddr.size;
					memset(ptr, 0, netif->macaddr.size);
					ptr += netif->macaddr.size;
					memcpy(ptr, dest.addr.s_addr_buf, dest.size);
					ptr += dest.size;
					netif->arpc.buf->buf.size = ptr - (uint8_t *)head;
					
					if (!vsfip_netif_ip_output_do(netif->arpc.buf,
								VSFIP_NETIF_PROTO_ARP, &netif->mac_broadcast))
					{
						netif->arpc.sm_pending = pt->sm;
						// wait for reply with timeout
						netif->arpc.to.interval = VSFIP_CFG_ARP_TIMEOUT_MS;
						netif->arpc.to.sm = pt->sm;
						vsftimer_register(&netif->arpc.to);
						
						evt = VSFSM_EVT_NONE;
						vsfsm_pt_entry(pt);
						if ((evt != VSFIP_NETIF_EVT_ARPC_UPDATE) &&
							(evt != VSFIP_NETIF_EVT_ARPC_TIMEOUT))
						{
							return VSFERR_NOT_READY;
						}
						vsftimer_unregister(&netif->arpc.to);
						netif->arpc.sm_pending = NULL;
					}
					
					vsfip_netif_get_ipaddr(netif->arpc.cur_request, &dest);
					mac = vsfip_netif_get_mac(netif, &dest);
				}
			}
			
			if (NULL == mac)
			{
				vsfip_buffer_release(netif->arpc.cur_request);
			}
			else
			{
				vsfip_netif_ip_output_do(netif->arpc.cur_request,
											VSFIP_NETIF_PROTO_IP, mac);
			}
		}
	}
	
	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}
