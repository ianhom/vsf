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

#include "vsfip_dhcp_common.h"
#include "vsfip_dhcpc.h"

#define VSFIP_DHCPC_XID			0xABCD1234
#define VSFIP_DHCPC_RETRY_CNT	10

enum vsfip_dhcp_EVT_t
{
	VSFIP_DHCP_EVT_READY		= VSFSM_EVT_USER_LOCAL_INSTANT + 0,
	VSFIP_DHCP_EVT_SEND_REQUEST	= VSFSM_EVT_USER_LOCAL_INSTANT + 1,
	VSFIP_DHCP_EVT_TIMEROUT		= VSFSM_EVT_USER_LOCAL_INSTANT + 2,
};

static uint32_t vsfip_dhcpc_xid = VSFIP_DHCPC_XID;

static vsf_err_t vsfip_dhcpc_init_msg(struct vsfip_dhcpc_t *dhcp, uint8_t op)
{
	struct vsfip_netif_t *netif = dhcp->netif;
	struct vsfip_buffer_t *buf;
	struct vsfip_dhcphead_t *head;

	dhcp->outbuffer = vsfip_buffer_get(VSFIP_CFG_HEADLEN + VSFIP_UDP_HEADLEN +
										sizeof(struct vsfip_dhcphead_t));
	if (NULL == dhcp->outbuffer)
	{
		return VSFERR_FAIL;
	}
	buf = dhcp->outbuffer;
	buf->app.buffer += VSFIP_CFG_HEADLEN + VSFIP_UDP_HEADLEN;
	buf->app.size -= VSFIP_CFG_HEADLEN + VSFIP_UDP_HEADLEN;

	head = (struct vsfip_dhcphead_t *)buf->app.buffer;
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
	vsfip_dhcp_append_opt(buf, &dhcp->optlen, VSFIP_DHCPOPT_MSGTYPE,
							VSFIP_DHCPOPT_MSGTYPE_LEN, &op);
	{
		uint16_t tmp16 = SYS_TO_BE_U16(576);
		// RFC 2132 9.10, message size MUST be >= 576
		vsfip_dhcp_append_opt(buf, &dhcp->optlen, VSFIP_DHCPOPT_MAXMSGSIZE,
							VSFIP_DHCPOPT_MAXMSGSIZE_LEN, (uint8_t *)&tmp16);
	}
	{
		uint8_t requestlist[] = {VSFIP_DHCPOPT_SUBNET_MASK,
			VSFIP_DHCPOPT_ROUTER, VSFIP_DHCPOPT_DNS, VSFIP_DHCPOPT_BROADCAST};
		vsfip_dhcp_append_opt(buf, &dhcp->optlen, VSFIP_DHCPOPT_PARAMLIST,
								sizeof(requestlist), requestlist);
	}
#ifdef VSFIP_CFG_HOSTNAME
	vsfip_dhcp_append_opt(buf, &dhcp->optlen, VSFIP_DHCPOPT_HOSTNAME,
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

static void vsfip_dhcpc_input(void *param, struct vsfip_buffer_t *buf)
{
	struct vsfip_dhcpc_t *dhcp = (struct vsfip_dhcpc_t *)param;
	struct vsfip_netif_t *netif = dhcp->netif;
	struct vsfip_dhcphead_t *head;
	uint8_t optlen;
	uint8_t *optptr;

	head = (struct vsfip_dhcphead_t *)buf->app.buffer;
	if ((head->op != VSFIP_DHCP_TOCLIENT) ||
		memcmp(head->chaddr, netif->macaddr.addr.s_addr_buf,
				netif->macaddr.size) ||
		(head->xid != dhcp->xid))
	{
		goto exit;
	}

	optlen = vsfip_dhcp_get_opt(buf, VSFIP_DHCPOPT_MSGTYPE, &optptr);
	if (optlen != VSFIP_DHCPOPT_MSGTYPE_LEN)
	{
		goto exit;
	}

	switch (optptr[0])
	{
	case VSFIP_DHCPOP_OFFER:
		dhcp->ipaddr.size = 4;
		dhcp->ipaddr.addr.s_addr = head->yiaddr;
		vsfsm_post_evt(&dhcp->sm, VSFIP_DHCP_EVT_SEND_REQUEST);
		break;
	case VSFIP_DHCPOP_ACK:
		optlen = vsfip_dhcp_get_opt(buf, VSFIP_DHCPOPT_LEASE_TIME, &optptr);
		dhcp->leasetime = (4 == optlen) ? GET_BE_U32(optptr) : 0;
		optlen = vsfip_dhcp_get_opt(buf, VSFIP_DHCPOPT_T1, &optptr);
		dhcp->t1 = (4 == optlen) ? GET_BE_U32(optptr) : 0;
		optlen = vsfip_dhcp_get_opt(buf, VSFIP_DHCPOPT_T2, &optptr);
		dhcp->t2 = (4 == optlen) ? GET_BE_U32(optptr) : 0;
		optlen = vsfip_dhcp_get_opt(buf, VSFIP_DHCPOPT_SUBNET_MASK, &optptr);
		dhcp->netmask.size = optlen;
		dhcp->netmask.addr.s_addr = (4 == optlen) ? *(uint32_t *)optptr : 0;
		optlen = vsfip_dhcp_get_opt(buf, VSFIP_DHCPOPT_ROUTER, &optptr);
		dhcp->gw.size = optlen;
		dhcp->gw.addr.s_addr = (4 == optlen) ? *(uint32_t *)optptr : 0;
		optlen = vsfip_dhcp_get_opt(buf, VSFIP_DHCPOPT_DNS, &optptr);
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

		vsfsm_post_evt(&dhcp->sm, VSFIP_DHCP_EVT_READY);
		break;
	}

exit:
	vsfip_buffer_release(buf);
}

static struct vsfsm_state_t *
vsfip_dhcpc_evt_handler(struct vsfsm_t *sm, vsfsm_evt_t evt)
{
	struct vsfip_dhcpc_t *dhcp = (struct vsfip_dhcpc_t *)sm->user_data;
	struct vsfip_netif_t *netif = dhcp->netif;

	switch (evt)
	{
	case VSFSM_EVT_INIT:
		dhcp->ready = true;
		dhcp->retry = 0;

	retry:
		dhcp->xid = vsfip_dhcpc_xid++;
		dhcp->so = vsfip_socket(AF_INET, IPPROTO_UDP);
		if (NULL == dhcp->so)
		{
			goto cleanup;
		}
		dhcp->so->callback.param = dhcp;
		dhcp->so->callback.input = vsfip_dhcpc_input;
		if (vsfip_bind(dhcp->so, VSFIP_DHCP_CLIENT_PORT))
		{
			goto cleanup;
		}
		vsfip_listen(dhcp->so, 0);

		// if address already allocated, do resume, send request again
		if (dhcp->ipaddr.size != 0)
		{
			goto dhcp_request;
		}

		// discover
		memset(&netif->ipaddr, 0, sizeof(netif->ipaddr));
		dhcp->ipaddr.size = 0;
		if (vsfip_dhcpc_init_msg(dhcp, (uint8_t)VSFIP_DHCPOP_DISCOVER) < 0)
		{
			goto cleanup;
		}
		vsfip_dhcp_end_opt(dhcp->outbuffer, &dhcp->optlen);
		dhcp->sockaddr.sin_addr.addr.s_addr = 0xFFFFFFFF;
		vsfip_udp_send(NULL, 0, dhcp->so, &dhcp->sockaddr, dhcp->outbuffer);
		dhcp->so->remote_sockaddr.sin_addr.addr.s_addr = VSFIP_IPADDR_ANY;

		dhcp->to = vsftimer_create(sm, 5000, 1, VSFIP_DHCP_EVT_TIMEROUT);
		break;
	case VSFIP_DHCP_EVT_SEND_REQUEST:
	dhcp_request:
		vsftimer_free(dhcp->to);
		if (vsfip_dhcpc_init_msg(dhcp, (uint8_t)VSFIP_DHCPOP_REQUEST) < 0)
		{
			goto cleanup;
		}
		vsfip_dhcp_append_opt(dhcp->outbuffer, &dhcp->optlen,
								VSFIP_DHCPOPT_REQIP, dhcp->ipaddr.size,
								dhcp->ipaddr.addr.s_addr_buf);
		vsfip_dhcp_end_opt(dhcp->outbuffer, &dhcp->optlen);
		dhcp->sockaddr.sin_addr.addr.s_addr = 0xFFFFFFFF;
		vsfip_udp_send(NULL, 0, dhcp->so, &dhcp->sockaddr, dhcp->outbuffer);
		dhcp->so->remote_sockaddr.sin_addr.addr.s_addr = VSFIP_IPADDR_ANY;

		dhcp->to = vsftimer_create(sm, 2000, 1, VSFIP_DHCP_EVT_TIMEROUT);
		break;
	case VSFIP_DHCP_EVT_READY:
		vsftimer_free(dhcp->to);
		// update netif->ipaddr
		dhcp->ready = 1;
		netif->ipaddr = dhcp->ipaddr;
		netif->gateway = dhcp->gw;
		netif->netmask = dhcp->netmask;
		netif->dns[0] = dhcp->dns[0];
		netif->dns[1] = dhcp->dns[1];

		// timer out for resume
//		vsftimer_create(sm, 2000, 1, VSFIP_DHCP_EVT_TIMEROUT);
		goto cleanup;
		break;
	case VSFIP_DHCP_EVT_TIMEROUT:
		// maybe need to resume, set the ready to false
		dhcp->ready = false;
	cleanup:
		if (dhcp->so != NULL)
		{
			vsfip_close(dhcp->so);
		}
		if (!dhcp->ready && (++dhcp->retry < VSFIP_DHCPC_RETRY_CNT))
		{
			goto retry;
		}

		// notify callder
		if (dhcp->update_sem.evt != VSFSM_EVT_NONE)
		{
			vsfsm_sem_post(&dhcp->update_sem);
		}
		break;
	}

	return NULL;
}

vsf_err_t vsfip_dhcpc_start(struct vsfip_netif_t *netif,
							struct vsfip_dhcpc_t *dhcpc)
{
	if ((NULL == netif) || (NULL == dhcpc))
	{
		return VSFERR_FAIL;
	}

	netif->dhcpc = dhcpc;
	dhcpc->netif = netif;
	dhcpc->starttick = interfaces->tickclk.get_count();

	dhcpc->sockaddr.sin_port = VSFIP_DHCP_SERVER_PORT;
	dhcpc->sockaddr.sin_addr.size = 4;

	dhcpc->sm.init_state.evt_handler = vsfip_dhcpc_evt_handler;
	dhcpc->sm.user_data = dhcpc;
	return vsfsm_init(&dhcpc->sm);
}
