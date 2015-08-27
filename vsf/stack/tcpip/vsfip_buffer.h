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
#ifndef __VSFIP_BUFFER_H_INCLUDED__
#define __VSFIP_BUFFER_H_INCLUDED__

#include "vsfip_cfg.h"

#include "tool/buffer/buffer.h"

struct vsfip_netif_t;
struct vsfip_buffer_t
{
	struct vsf_buffer_t buf;
	struct vsf_buffer_t app;
	
	union
	{
		uint8_t *ipver;
		struct vsfip_ip4head_t *ip4head;
//		struct vsfip_ip6head_t *ip6head;
	} iphead;
	
	uint16_t ref;
	uint16_t ttl;
	
	uint8_t *buffer;
//	uint32_t size;
	
	struct vsfip_netif_t *netif;
	struct vsfip_buffer_t *next;
};

void vsfip_buffer_init(void);
struct vsfip_buffer_t * vsfip_buffer_get(uint32_t size);
void vsfip_buffer_reference(struct vsfip_buffer_t *buf);
void vsfip_buffer_release(struct vsfip_buffer_t *buf);

#endif		// __VSFIP_BUFFER_H_INCLUDED__
