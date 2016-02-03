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

#ifndef __VSFUSBD_RNDIS_H_INCLUDED__
#define __VSFUSBD_RNDIS_H_INCLUDED__

#include "vsfusbd_CDCACM.h"

#ifndef VSFCFG_STANDALONE_MODULE
extern const struct vsfusbd_class_protocol_t vsfusbd_RNDISControl_class;
extern const struct vsfusbd_class_protocol_t vsfusbd_RNDISData_class;
#endif

struct vsfusbd_RNDIS_param_t
{
	struct vsfusbd_CDCACM_param_t CDCACM_param;
};

#endif	// __VSFUSBD_RNDIS_H_INCLUDED__
