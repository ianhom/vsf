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
}

struct vsfip_buffer_t * vsfip_buffer_get(uint32_t size)
{
	if (size < VSFIP_BUFFER_SIZE)
	{
		int i;
		for (i = 0; i < VSFIP_BUFFER_NUM; i++)
		{
			if (!vsfip_buffer[i].ref)
			{
				vsfip_buffer[i].ref++;
				vsfip_buffer[i].buf.buffer =\
					vsfip_buffer[i].app.buffer = vsfip_buffer[i].buffer;
				vsfip_buffer[i].buf.size = vsfip_buffer[i].app.size = size;
				vsfip_buffer[i].node.next = NULL;
				vsfip_buffer[i].netif = NULL;
				return &vsfip_buffer[i];
			}
		}
	}
	return NULL;
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
