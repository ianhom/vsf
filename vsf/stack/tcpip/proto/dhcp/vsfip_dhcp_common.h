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
#ifndef __VSFIP_DHCP_COMMON_H_INCLUDED__
#define __VSFIP_DHCP_COMMON_H_INCLUDED__

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

#define VSFIP_DHCP_TOSERVER					1
#define VSFIP_DHCP_TOCLIENT					2

enum vsfip_dhcp_op_t
{
	VSFIP_DHCPOP_DISCOVER					= 1,
	VSFIP_DHCPOP_OFFER						= 2,
	VSFIP_DHCPOP_REQUEST					= 3,
	VSFIP_DHCPOP_DECLINE					= 4,
	VSFIP_DHCPOP_ACK						= 5,
	VSFIP_DHCPOP_NAK						= 6,
	VSFIP_DHCPOP_RELEASE					= 7,
	VSFIP_DHCPOP_INFORM						= 8,
};

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

void vsfip_dhcp_append_opt(struct vsfip_buffer_t *buf, uint32_t *optlen,
						uint8_t option, uint8_t len, uint8_t *data);
void vsfip_dhcp_end_opt(struct vsfip_buffer_t *buf, uint32_t *optlen);

#endif		// __VSFIP_DHCP_COMMON_H_INCLUDED__
