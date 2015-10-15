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

#include "framework/vsfsm/vsfsm.h"

#include "vsfip.h"
#include "vsfip_buffer.h"

static uint8_t vsfip_buffer128_mem[VSFIP_BUFFER_NUM][128];
static struct vsfip_buffer_t vsfip_buffer128[VSFIP_BUFFER_NUM];
static uint8_t vsfip_buffer_mem[VSFIP_BUFFER_NUM][VSFIP_BUFFER_SIZE];
static struct vsfip_buffer_t vsfip_buffer[VSFIP_BUFFER_NUM];

void vsfip_buffer_init(void)
{
	int i;
	for (i = 0; i < VSFIP_BUFFER_NUM; i++)
	{
		vsfip_buffer[i].ref = 0;
		vsfip_buffer[i].buffer = vsfip_buffer_mem[i];
//		vsfip_buffer[i].size = sizeof(vsfip_buffer_mem[i]);
	}
	for (i = 0; i < VSFIP_BUFFER_NUM; i++)
	{
		vsfip_buffer128[i].ref = 0;
		vsfip_buffer128[i].buffer = vsfip_buffer128_mem[i];
//		vsfip_buffer128[i].size = sizeof(vsfip_buffer128_mem[i]);
	}
}

struct vsfip_buffer_t* vsfip_buffer_get(uint32_t size)
{
	struct vsfip_buffer_t *base;
	int i;

	if (size <= sizeof(vsfip_buffer128_mem[0]))
	{
		base = vsfip_buffer128;
	}
	else if (size <= VSFIP_BUFFER_SIZE)
	{
		base = vsfip_buffer;
	}
	else
	{
		return NULL;
	}

retry:
	for (i = 0; i < VSFIP_BUFFER_NUM; i++)
	{
		if (!base[i].ref)
		{
			base[i].ref++;
			base[i].buf.buffer = base[i].app.buffer = base[i].buffer;
			base[i].buf.size = base[i].app.size = size;
			base[i].proto_node.next = base[i].netif_node.next = NULL;
			base[i].netif = NULL;
			return &base[i];
		}
	}
	if (base == vsfip_buffer128)
	{
		base = vsfip_buffer;
		goto retry;
	}
	return NULL;
}

struct vsfip_buffer_t* vsfip_appbuffer_get(uint32_t header, uint32_t app)
{
	struct vsfip_buffer_t *buf = vsfip_buffer_get(header + app);

	if (buf != NULL)
	{
		buf->app.buffer += header;
		buf->app.size -= header;
	}
	return buf;
}

void vsfip_buffer_reference(struct vsfip_buffer_t *buf)
{
	if (buf != NULL)
	{
		buf->ref++;
	}
}

void vsfip_buffer_release(struct vsfip_buffer_t *buf)
{
	if ((buf != NULL) && buf->ref)
	{
		buf->ref--;
	}
}
