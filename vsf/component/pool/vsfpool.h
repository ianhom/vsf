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

struct vsfpool_t
{
	uint32_t *flags;
	void *buffer;
	uint32_t size;
	uint32_t num;
};

#define VSFPOOL_DEFINE(name, type, num)			\
	struct vsfpool_##name##_t\
	{\
		struct vsfpool_t pool;\
        uint32_t mskarr[((num) + 31) >> 5];\
        type buffer[(num)];\
	} name

#define VSFPOOL_INIT(p, type, n)			\
	do {\
		(p)->pool.flags = (p)->mskarr;\
		(p)->pool.buffer = (p)->buffer;\
		(p)->pool.size = sizeof(type);\
		(p)->pool.num = (n);\
		vsfpool_init((struct vsfpool_t *)(p));\
	} while (0)

#define VSFPOOL_ALLOC(p, type)				\
    (type *)vsfpool_alloc((struct vsfpool_t *)(p))

#define VSFPOOL_FREE(p, buf)					\
	vsfpool_free((struct vsfpool_t *)(p), (buf))

void vsfpool_init(struct vsfpool_t *pool);
void* vsfpool_alloc(struct vsfpool_t *pool);
void vsfpool_free(struct vsfpool_t *pool, void *buffer);
