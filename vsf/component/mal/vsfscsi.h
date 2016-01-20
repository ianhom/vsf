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

#ifndef __VSFSCSI_H_INCLUDED__
#define __VSFSCSI_H_INCLUDED__

enum SCSI_errcode_t
{
	SCSI_ERRCODE_INVALID_COMMAND,
};

struct vsfscsi_lun_cparam_t
{
	bool removable;
	char vendor[8];
	char product[16];
	char revision[4];
//	enum vsfscsi_pdt_t type;
};

struct vsfscsi_lun_t
{
	struct vsfscsi_lun_cparam_t *cparam;
};

struct vsfscsi_device_t;
struct vsfscsi_transact_t
{
	struct vsfsm_t sm;
	struct vsfsm_pt_t pt;

	struct vsf_stream_t stream;
    struct vsf_multibuf_t multibuf;
};

struct vsfscsi_device_t
{
	uint8_t max_lun;
	struct vsfscsi_lun_t *lun;

	enum SCSI_errcode_t errcode;
	struct vsfscsi_transact_t transact[1];
};

vsf_err_t vsfscsi_init(struct vsfscsi_device_t *dev);
vsf_err_t vsfscsi_execute_nb(struct vsfscsi_device_t *dev, uint8_t *cmd,
					uint32_t *data_size, struct vsfscsi_transact_t **transact);
void vsfscsi_cancel_transact(struct vsfscsi_transact_t *transact);

#endif // __VSFSCSI_H_INCLUDED__
