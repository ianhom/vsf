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

#ifndef __APP_TYPE_H_INCLUDED__
#define __APP_TYPE_H_INCLUDED__

#include <stdint.h>
#include <stdbool.h>

#include "vsf_err.h"

#ifndef __CONNECT
#	define __CONNECT(a, b)			a ## b
#endif

#ifndef NULL
#	define NULL						((void *)0)
#endif

#ifndef dimof
#	define dimof(arr)				(sizeof(arr) / sizeof((arr)[0]))
#endif
#ifndef offset_of
#	define offset_of(s, m)			(uint32_t)(&(((s *)0)->m))
#endif
#ifndef container_of
#	define container_of(ptr, type, member)	\
		(ptr ? (type *)((char *)(ptr) - offset_of(type, member)) : NULL)
#endif
#ifndef min
#	define min(a, b)				(((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#	define max(a, b)				(((a) > (b)) ? (a) : (b))
#endif
#ifndef TO_STR
#	define TO_STR(name)				#name
#endif
#ifndef REFERENCE_PARAMETER
#	define REFERENCE_PARAMETER(a)	(a) = (a)
#endif

#include "tool/bittool/bittool.h"

#define GET_LE_U16(p)				GET_U16_LSBFIRST(p)
#define GET_LE_U24(p)				GET_U24_LSBFIRST(p)
#define GET_LE_U32(p)				GET_U32_LSBFIRST(p)
#define GET_LE_U64(p)				GET_U64_LSBFIRST(p)
#define GET_BE_U16(p)				GET_U16_MSBFIRST(p)
#define GET_BE_U24(p)				GET_U24_MSBFIRST(p)
#define GET_BE_U32(p)				GET_U32_MSBFIRST(p)
#define GET_BE_U64(p)				GET_U64_MSBFIRST(p)
#define SET_LE_U16(p, v)			SET_U16_LSBFIRST(p, v)
#define SET_LE_U24(p, v)			SET_U24_LSBFIRST(p, v)
#define SET_LE_U32(p, v)			SET_U32_LSBFIRST(p, v)
#define SET_LE_U64(p, v)			SET_U64_LSBFIRST(p, v)
#define SET_BE_U16(p, v)			SET_U16_MSBFIRST(p, v)
#define SET_BE_U24(p, v)			SET_U24_MSBFIRST(p, v)
#define SET_BE_U32(p, v)			SET_U32_MSBFIRST(p, v)
#define SET_BE_U64(p, v)			SET_U64_MSBFIRST(p, v)

#if defined(__BIG_ENDIAN__) && (__BIG_ENDIAN__ == 1)
#	define LE_TO_SYS_U16(v)			SWAP_U16(v)
#	define LE_TO_SYS_U24(v)			SWAP_U24(v)
#	define LE_TO_SYS_U32(v)			SWAP_U32(v)
#	define LE_TO_SYS_U64(v)			SWAP_U64(v)
#	define BE_TO_SYS_U16(v)			((uint16_t)(v))
#	define BE_TO_SYS_U24(v)			((uint32_t)(v))
#	define BE_TO_SYS_U32(v)			((uint32_t)(v))
#	define BE_TO_SYS_U64(v)			((uint64_t)(v))

#	define SYS_TO_LE_U16(v)			SWAP_U16(v)
#	define SYS_TO_LE_U24(v)			SWAP_U24(v)
#	define SYS_TO_LE_U32(v)			SWAP_U32(v)
#	define SYS_TO_LE_U64(v)			SWAP_U64(v)
#	define SYS_TO_BE_U16(v)			((uint16_t)(v))
#	define SYS_TO_BE_U24(v)			((uint32_t)(v))
#	define SYS_TO_BE_U32(v)			((uint32_t)(v))
#	define SYS_TO_BE_U64(v)			((uint64_t)(v))

#	define SET_SYS_U16(p, v)		SET_BE_U16(p, v)
#	define SET_SYS_U24(p, v)		SET_BE_U24(p, v)
#	define SET_SYS_U32(p, v)		SET_BE_U32(p, v)
#	define SET_SYS_U64(p, v)		SET_BE_U64(p, v)
#	define GET_SYS_U16(p)			GET_BE_U16(p)
#	define GET_SYS_U24(p)			GET_BE_U24(p)
#	define GET_SYS_U32(p)			GET_BE_U32(p)
#	define GET_SYS_U64(p)			GET_BE_U64(p)
#else
#	define LE_TO_SYS_U16(v)			((uint16_t)(v))
#	define LE_TO_SYS_U24(v)			((uint32_t)(v))
#	define LE_TO_SYS_U32(v)			((uint32_t)(v))
#	define LE_TO_SYS_U64(v)			((uint64_t)(v))
#	define BE_TO_SYS_U16(v)			SWAP_U16(v)
#	define BE_TO_SYS_U24(v)			SWAP_U24(v)
#	define BE_TO_SYS_U32(v)			SWAP_U32(v)
#	define BE_TO_SYS_U64(v)			SWAP_U64(v)

#	define SYS_TO_LE_U16(v)			((uint16_t)(v))
#	define SYS_TO_LE_U24(v)			((uint32_t)(v))
#	define SYS_TO_LE_U32(v)			((uint32_t)(v))
#	define SYS_TO_LE_U64(v)			((uint64_t)(v))
#	define SYS_TO_BE_U16(v)			SWAP_U16(v)
#	define SYS_TO_BE_U24(v)			SWAP_U24(v)
#	define SYS_TO_BE_U32(v)			SWAP_U32(v)
#	define SYS_TO_BE_U64(v)			SWAP_U64(v)

#	define SET_SYS_U16(p, v)		SET_LE_U16(p, v)
#	define SET_SYS_U24(p, v)		SET_LE_U24(p, v)
#	define SET_SYS_U32(p, v)		SET_LE_U32(p, v)
#	define SET_SYS_U64(p, v)		SET_LE_U64(p, v)
#	define GET_SYS_U16(p)			GET_LE_U16(p)
#	define GET_SYS_U24(p)			GET_LE_U24(p)
#	define GET_SYS_U32(p)			GET_LE_U32(p)
#	define GET_SYS_U64(p)			GET_LE_U64(p)
#endif

#endif	// __APP_TYPE_H_INCLUDED__

