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

#include "app_cfg.h"
#include "app_type.h"

#include "vsfpool.h"

void vsfpool_init(struct vsfpool_t *pool)
{
	memset(pool->flags, 0, (pool->num + 31) >> 3);
}

void* vsfpool_alloc(struct vsfpool_t *pool)
{
	int index = mskarr_ffz(pool->flags, (pool->num + 31) >> 5);

	if (index >= pool->num)
	{
		return NULL;
	}
	mskarr_set(pool->flags, index);
	return (uint8_t *)pool->buffer + index * pool->size;
}

void vsfpool_free(struct vsfpool_t *pool, void *buffer)
{
	int index = ((uint8_t *)buffer - (uint8_t *)pool->buffer) / pool->size;

	if (index < pool->num)
	{
		mskarr_clr(pool->flags, index);
	}
}
