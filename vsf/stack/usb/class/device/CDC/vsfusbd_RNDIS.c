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

#include "vsf.h"

static vsf_err_t vsfusbd_RNDIS_on_encapsulated_command(
			struct vsfusbd_CDC_param_t *param, struct vsf_buffer_t *buffer)
{
	return VSFERR_NONE;
}

vsf_err_t
vsfusbd_RNDISData_class_init(uint8_t iface, struct vsfusbd_device_t *device)
{
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	struct vsfusbd_RNDIS_param_t *param =
		(struct vsfusbd_RNDIS_param_t *)config->iface[iface].protocol_param;

	if (vsfusbd_CDCACMData_class.init(iface, device))
	{
		return VSFERR_FAIL;
	}

	param->CDCACM_param.CDC_param.callback.send_encapsulated_command =
		vsfusbd_RNDIS_on_encapsulated_command;
	return VSFERR_NONE;
}

#ifndef VSFCFG_STANDALONE_MODULE
const struct vsfusbd_class_protocol_t vsfusbd_RNDISControl_class =
{
	NULL, NULL, NULL, NULL, NULL
};

const struct vsfusbd_class_protocol_t vsfusbd_RNDISData_class =
{
	NULL, NULL, NULL, vsfusbd_RNDISData_class_init, NULL
};
#endif
