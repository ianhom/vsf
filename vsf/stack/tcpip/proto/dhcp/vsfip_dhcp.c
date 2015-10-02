/***************************************************************************
 *   Copyright (C) 2009 - 2010 by Simon Qian <SimonQian@SimonQian.com>     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "app_type.h"
#include "compiler.h"

#include "interfaces.h"

#include "framework/vsfsm/vsfsm.h"
#include "framework/vsftimer/vsftimer.h"

#include "../../vsfip.h"
#include "../../vsfip_buffer.h"

#include "vsfip_dhcp.h"

#define VSFIP_DHCP_CLIENT_PORT				68
#define VSFIP_DHCP_SERVER_PORT				67
#define VSFIP_DHCP_MAGIC					0x63825363

#define VSFIP_DHCPOPT_SUBNET_MASK			1
#define VSFIP_DHCPOPT_ROUTER				3
#define VSFIP_DHCPOPT_DNS					6
#define VSFIP_DHCPOPT_HOSTNAME				12
#define VSFIP_DHCPOPT_BROADCAST				28
#define VSFIP_DHCPOPT_REQIP					50
#define VSFIP_DHCPOPT_LEASE_TIME			51
#define VSFIP_DHCPOPT_MSGTYPE				53
#define VSFIP_DHCPOPT_MSGTYPE_LEN			1
#define VSFIP_DHCPOPT_PARAMLIST				55
#define VSFIP_DHCPOPT_MAXMSGSIZE			57
#define VSFIP_DHCPOPT_MAXMSGSIZE_LEN		2
#define VSFIP_DHCPOPT_T1					58
#define VSFIP_DHCPOPT_T2					59
#define VSFIP_DHCPOPT_US					60
#define VSFIP_DHCPOPT_END					255
#define VSFIP_DHCPOPT_MINLEN				68

#define VSFIP_DHCP_XID						0xABCD1234
#define VSFIP_DHCP_RETRY_CNT				10

PACKED_HEAD struct PACKED_MID vsfip_dhcphead_t
{
	uint8_t op;
	uint8_t htype;
	uint8_t hlen;
	uint8_t hops;
	uint32_t xid;
	uint16_t secs;
	uint16_t flags;
	uint32_t ciaddr;
	uint32_t yiaddr;
	uint32_t siaddr;
	uint32_t giaddr;
	uint8_t chaddr[16];
	char sname[64];
	char file[128];
	uint32_t magic;
	uint8_t options[VSFIP_DHCPOPT_MINLEN];	// min option size
}; PACKED_TAIL

#define VSFIP_DHCP_TOSERVER		1
#define VSFIP_DHCP_TOCLIENT		2

enum vsfip_dhcp_op_t
{
	VSFIP_DHCPOP_DISCOVER		= 1,
	VSFIP_DHCPOP_OFFER			= 2,
	VSFIP_DHCPOP_REQUEST		= 3,
	VSFIP_DHCPOP_DECLINE		= 4,
	VSFIP_DHCPOP_ACK			= 5,
	VSFIP_DHCPOP_NAK			= 6,
	VSFIP_DHCPOP_RELEASE		= 7,
	VSFIP_DHCPOP_INFORM			= 8,
};

enum vsfip_dhcp_EVT_t
{
	VSFIP_DHCP_EVT_REPLY		= VSFSM_EVT_USER_LOCAL_INSTANT + 0,
	VSFIP_DHCP_EVT_TIMEROUT		= VSFSM_EVT_USER_LOCAL_INSTANT + 1,
};

static uint32_t vsfip_dhcp_xid = VSFIP_DHCP_XID;

// dhcp->outbuffer MUST be large enough for all the data
static void vsfip_dhcp_append_opt(struct vsfip_dhcp_t *dhcp,
						uint8_t option, uint8_t len, uint8_t *data)
{
	struct vsfip_buffer_t *buffer = dhcp->outbuffer;
	struct vsfip_dhcphead_t *head =
				(struct vsfip_dhcphead_t *)buffer->app.buffer;
	
	head->options[dhcp->optlen++] = option;
	head->options[dhcp->optlen++] = len;
	memcpy(&head->options[dhcp->optlen], data, len);
	dhcp->optlen += len;
}

static void vsfip_dhcp_end_opt(struct vsfip_dhcp_t *dhcp)
{
	struct vsfip_buffer_t *buffer = dhcp->outbuffer;
	struct vsfip_dhcphead_t *head =
				(struct vsfip_dhcphead_t *)buffer->app.buffer;
	
	head->options[dhcp->optlen++] = VSFIP_DHCPOPT_END;
	while ((dhcp->optlen < VSFIP_DHCPOPT_MINLEN) || (dhcp->optlen & 3))
	{
		head->options[dhcp->optlen++] = 0;
	}
	// tweak options length
	buffer->app.size -= sizeof(head->options);
	buffer->app.size += dhcp->optlen;
}

static vsf_err_t vsfip_dhcp_init_msg(struct vsfip_dhcp_t *dhcp, uint8_t op)
{
	struct vsfip_netif_t *netif = dhcp->netif;
	struct vsfip_dhcphead_t *head;
	
	dhcp->outbuffer = vsfip_buffer_get(VSFIP_CFG_HEADLEN + VSFIP_UDP_HEADLEN +
										sizeof(struct vsfip_dhcphead_t));
	if (NULL == dhcp->outbuffer)
	{
		return VSFERR_FAIL;
	}
	dhcp->outbuffer->app.buffer += VSFIP_CFG_HEADLEN + VSFIP_UDP_HEADLEN;
	dhcp->outbuffer->app.size -= VSFIP_CFG_HEADLEN + VSFIP_UDP_HEADLEN;
	
	head = (struct vsfip_dhcphead_t *)dhcp->outbuffer->app.buffer;
	memset(head, 0, sizeof(struct vsfip_dhcphead_t));
	head->op = VSFIP_DHCP_TOSERVER;
	head->htype = netif->drv->hwtype;
	head->hlen = netif->macaddr.size;
	head->xid = dhcp->xid;
	// shift right 10-bit for div 1000
	head->secs = SYS_TO_BE_U16(\
					(interfaces->tickclk.get_count() - dhcp->starttick) >> 10);
	memcpy(head->chaddr, netif->macaddr.addr.s_addr_buf, netif->macaddr.size);
	head->magic = SYS_TO_BE_U32(VSFIP_DHCP_MAGIC);
	dhcp->optlen = 0;
	vsfip_dhcp_append_opt(dhcp, VSFIP_DHCPOPT_MSGTYPE,
							VSFIP_DHCPOPT_MSGTYPE_LEN, &op);
	{
		uint16_t tmp16 = SYS_TO_BE_U16(576);
		// RFC 2132 9.10, message size MUST be >= 576
		vsfip_dhcp_append_opt(dhcp, VSFIP_DHCPOPT_MAXMSGSIZE,
							VSFIP_DHCPOPT_MAXMSGSIZE_LEN, (uint8_t *)&tmp16);
	}
	{
		uint8_t requestlist[] = {VSFIP_DHCPOPT_SUBNET_MASK,
			VSFIP_DHCPOPT_ROUTER, VSFIP_DHCPOPT_DNS, VSFIP_DHCPOPT_BROADCAST};
		vsfip_dhcp_append_opt(dhcp, VSFIP_DHCPOPT_PARAMLIST,
								sizeof(requestlist), requestlist);
	}
#ifdef VSFIP_CFG_HOSTNAME
	vsfip_dhcp_append_opt(dhcp, VSFIP_DHCPOPT_HOSTNAME,
					strlen(VSFIP_CFG_HOSTNAME), (uint8_t *)VSFIP_CFG_HOSTNAME);
#endif
	
	return VSFERR_NONE;
}

static uint8_t vsfip_dhcp_get_opt(struct vsfip_buffer_t *buf, uint8_t option,
									uint8_t **data)
{
	struct vsfip_dhcphead_t *head = (struct vsfip_dhcphead_t *)buf->app.buffer;
	uint8_t *ptr = head->options;
	
	while ((ptr[0] != VSFIP_DHCPOPT_END) &&
			((ptr - buf->app.buffer) < buf->app.size))
	{
		if (ptr[0] == option)
		{
			if (data != NULL)
			{
				*data = &ptr[2];
			}
			return ptr[1];
		}
		ptr += 2 + ptr[1];
	}
	return 0;
}

static vsf_err_t vsfip_dhcp_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfip_dhcp_t *dhcp = (struct vsfip_dhcp_t *)pt->user_data;
	struct vsfip_netif_t *netif = dhcp->netif;
	struct vsfip_dhcphead_t *head;
	vsf_err_t err = VSFERR_NONE;
	uint8_t optlen;
	uint8_t *optptr;
	
	vsfsm_pt_begin(pt);
	
	dhcp->ready = 0;
	dhcp->retry = 0;
retry:
	dhcp->xid = vsfip_dhcp_xid++;
	dhcp->so = vsfip_socket(AF_INET, IPPROTO_UDP);
	if (NULL == dhcp->so)
	{
		err = VSFERR_FAIL;
		goto cleanup;
	}
	if (vsfip_bind(dhcp->so, VSFIP_DHCP_CLIENT_PORT))
	{
		vsfip_close(dhcp->so);
		err = VSFERR_FAIL;
		goto cleanup;
	}
	vsfip_listen(dhcp->so, 0);
	
	// if allocated, just request again
	if (dhcp->ipaddr.size != 0)
	{
		dhcp->resume = true;
		goto dhcp_request;
	}
	
	// discover
	dhcp->ipaddr.size = 0;
	err = vsfip_dhcp_init_msg(dhcp, (uint8_t)VSFIP_DHCPOP_DISCOVER);
	if (err < 0)
	{
		goto cleanup;
	}
	vsfip_dhcp_end_opt(dhcp);
	dhcp->sockaddr.sin_addr.addr.s_addr = 0xFFFFFFFF;
	dhcp->caller_pt.state = 0;
	vsfsm_pt_entry(pt);
	err = vsfip_udp_send(&dhcp->caller_pt, evt, dhcp->so, &dhcp->sockaddr,
							dhcp->outbuffer);
	if (err > 0) return err;
	if (err < 0) {vsfip_buffer_release(dhcp->outbuffer); goto cleanup;}
	vsfip_buffer_release(dhcp->outbuffer);
	
wait_offer:
	dhcp->so->timeout_ms = 5000;
	dhcp->sockaddr.sin_addr.addr.s_addr = VSFIP_IPADDR_ANY;
	dhcp->caller_pt.state = 0;
	vsfsm_pt_entry(pt);
	err = vsfip_udp_recv(&dhcp->caller_pt, evt, dhcp->so, &dhcp->sockaddr,
							&dhcp->inbuffer);
	if (err > 0) return err; else if (err < 0) goto cleanup; else
	{
		// parse OFFER
		head = (struct vsfip_dhcphead_t *)dhcp->inbuffer->app.buffer;
		if ((head->op != VSFIP_DHCP_TOCLIENT) ||
			memcmp(head->chaddr, netif->macaddr.addr.s_addr_buf,
					netif->macaddr.size))
		{
			vsfip_buffer_release(dhcp->inbuffer);
			goto cleanup;
		}
		if (head->xid != dhcp->xid)
		{
			// wrong sequence
			goto wait_offer;
		}
		
		optlen = vsfip_dhcp_get_opt(dhcp->inbuffer, VSFIP_DHCPOPT_MSGTYPE,
									&optptr);
		if ((optlen != VSFIP_DHCPOPT_MSGTYPE_LEN) ||
			(optptr[0] != VSFIP_DHCPOP_OFFER))
		{
			vsfip_buffer_release(dhcp->inbuffer);
			goto cleanup;
		}
		
		dhcp->ipaddr.size = 4;
		dhcp->ipaddr.addr.s_addr = head->yiaddr;
		vsfip_buffer_release(dhcp->inbuffer);
	}
	
	// request
dhcp_request:
	err = vsfip_dhcp_init_msg(dhcp, (uint8_t)VSFIP_DHCPOP_REQUEST);
	if (err < 0)
	{
		goto cleanup;
	}
	vsfip_dhcp_append_opt(dhcp, VSFIP_DHCPOPT_REQIP, dhcp->ipaddr.size,
							dhcp->ipaddr.addr.s_addr_buf);
	vsfip_dhcp_end_opt(dhcp);
	dhcp->sockaddr.sin_addr.addr.s_addr = 0xFFFFFFFF;
	memset(&netif->ipaddr, 0, sizeof(netif->ipaddr));
	dhcp->caller_pt.state = 0;
	vsfsm_pt_entry(pt);
	err = vsfip_udp_send(&dhcp->caller_pt, evt, dhcp->so, &dhcp->sockaddr,
							dhcp->outbuffer);
	if (err > 0) return err;
	if (err < 0) {vsfip_buffer_release(dhcp->outbuffer); goto cleanup;}
	vsfip_buffer_release(dhcp->outbuffer);
	
wait_ack:
	dhcp->so->timeout_ms = 2000;
	dhcp->sockaddr.sin_addr.addr.s_addr = VSFIP_IPADDR_ANY;
	dhcp->caller_pt.state = 0;
	vsfsm_pt_entry(pt);
	err = vsfip_udp_recv(&dhcp->caller_pt, evt, dhcp->so, &dhcp->sockaddr,
							&dhcp->inbuffer);
	if (err > 0) return err; else if (err < 0) goto cleanup; else
	{
		// parse ACK
		head = (struct vsfip_dhcphead_t *)dhcp->inbuffer->app.buffer;
		if ((head->op != VSFIP_DHCP_TOCLIENT) ||
			memcmp(head->chaddr, netif->macaddr.addr.s_addr_buf,
					netif->macaddr.size))
		{
			vsfip_buffer_release(dhcp->inbuffer);
			goto cleanup;
		}
		if (head->xid != dhcp->xid)
		{
			// wrong sequence
			goto wait_ack;
		}
		
		optlen = vsfip_dhcp_get_opt(dhcp->inbuffer, VSFIP_DHCPOPT_MSGTYPE,
									&optptr);
		if ((optlen != VSFIP_DHCPOPT_MSGTYPE_LEN) ||
			(optptr[0] != VSFIP_DHCPOP_ACK))
		{
			vsfip_buffer_release(dhcp->inbuffer);
			goto cleanup;
		}
		
		optlen = vsfip_dhcp_get_opt(dhcp->inbuffer, VSFIP_DHCPOPT_LEASE_TIME,
									&optptr);
		dhcp->leasetime = (4 == optlen) ? GET_BE_U32(optptr) : 0;
		
		optlen = vsfip_dhcp_get_opt(dhcp->inbuffer, VSFIP_DHCPOPT_T1, &optptr);
		dhcp->t1 = (4 == optlen) ? GET_BE_U32(optptr) : 0;
		
		optlen = vsfip_dhcp_get_opt(dhcp->inbuffer, VSFIP_DHCPOPT_T2, &optptr);
		dhcp->t2 = (4 == optlen) ? GET_BE_U32(optptr) : 0;
		
		optlen = vsfip_dhcp_get_opt(dhcp->inbuffer, VSFIP_DHCPOPT_SUBNET_MASK,
									&optptr);
		dhcp->netmask.size = optlen;
		dhcp->netmask.addr.s_addr = (4 == optlen) ? *(uint32_t *)optptr : 0;
		
		optlen = vsfip_dhcp_get_opt(dhcp->inbuffer, VSFIP_DHCPOPT_ROUTER,
									&optptr);
		dhcp->gw.size = optlen;
		dhcp->gw.addr.s_addr = (4 == optlen) ? *(uint32_t *)optptr : 0;
		
		optlen = vsfip_dhcp_get_opt(dhcp->inbuffer, VSFIP_DHCPOPT_DNS,
									&optptr);
		dhcp->dns[0].size = dhcp->dns[1].size = 0;
		if (optlen >= 4)
		{
			dhcp->dns[0].size = 4;
			dhcp->dns[0].addr.s_addr = *(uint32_t *)optptr;
			if (optlen >= 8)
			{
				dhcp->dns[1].size = 4;
				dhcp->dns[1].addr.s_addr = *(uint32_t *)(optptr + 4);
			}
		}
		
		vsfip_buffer_release(dhcp->inbuffer);
	}
	
	// update netif->ipaddr
	dhcp->ready = 1;
	netif->ipaddr = dhcp->ipaddr;
	netif->gateway = dhcp->gw;
	netif->netmask = dhcp->netmask;
	netif->dns[0] = dhcp->dns[0];
	netif->dns[1] = dhcp->dns[1];
	if (dhcp->update_sem.evt != VSFSM_EVT_NONE)
	{
		vsfsm_sem_post(&dhcp->update_sem);
	}
	
	// initialize timer
	// TODO: 
	vsfsm_pt_entry(pt);
	if (dhcp->so != NULL)
	{
		vsfip_close(dhcp->so);
	}
	return VSFERR_NONE;
	
	// cleanup
cleanup:
	if (dhcp->so != NULL)
	{
		vsfip_close(dhcp->so);
	}
	if ((err < 0) && (++dhcp->retry < VSFIP_DHCP_RETRY_CNT))
	{
		vsfsm_pt_delay(pt, 2);
		goto retry;
	}
	
	if (dhcp->update_sem.evt != VSFSM_EVT_NONE)
	{
		vsfsm_sem_post(&dhcp->update_sem);
	}
	vsfsm_pt_end(pt);
	return err;
}

vsf_err_t vsfip_dhcp_start(struct vsfip_netif_t *netif,
							struct vsfip_dhcp_t *dhcp)
{
	if ((NULL == netif) || (NULL == dhcp))
	{
		return VSFERR_FAIL;
	}
	
	netif->dhcp = dhcp;
	dhcp->netif = netif;
	dhcp->starttick = interfaces->tickclk.get_count();
	dhcp->resume = false;
	
	dhcp->sockaddr.sin_port = VSFIP_DHCP_SERVER_PORT;
	dhcp->sockaddr.sin_addr.size = 4;
	
	dhcp->caller_pt.sm = &dhcp->sm;
	dhcp->caller_pt.user_data = netif;
	
	dhcp->pt.thread = vsfip_dhcp_thread;
	dhcp->pt.user_data = dhcp;
	vsfsm_pt_init(&dhcp->sm, &dhcp->pt);
	return VSFERR_NONE;
}
