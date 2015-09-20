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
	struct vsfip_socket_t *so;
	struct vsfip_sockaddr_t sockaddr;
	struct vsfip_buffer_t *outbuffer;
	struct vsfip_buffer_t *inbuffer;
	struct vsfip_ipaddr_t ipaddr;
	struct vsfip_ipaddr_t gw;
	struct vsfip_ipaddr_t netmask;
	struct vsfip_ipaddr_t dns[2];
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
