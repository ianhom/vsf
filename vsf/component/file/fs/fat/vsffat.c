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

PACKED_HEAD struct PACKED_MID fatfs_record_t
{
	uint8_t CrtTimeTenth;
	uint16_t CrtTime;
	uint16_t CrtData;
	uint16_t LstAccData;
	uint16_t FstClusHI;
	uint16_t WrtTime;
	uint16_t WrtData;
	uint16_t FstClusLO;
}; PACKED_TAIL

vsf_err_t fatfs_mount(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
								struct vsfile_t *dir)
{
	return VSFERR_NONE;
}

vsf_err_t fatfs_addfile(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir, char *name, enum vsfile_attr_t attr)
{
	return VSFERR_NONE;
}

vsf_err_t fatfs_removefile(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir, char *name)
{
}

vsf_err_t fatfs_getchild_byname(struct vsfsm_pt_t *pt,
					vsfsm_evt_t evt, struct vsfile_t *dir, char *name,
					struct vsfile_t **file)
{
	return VSFERR_NONE;
}

vsf_err_t fatfs_getchild_byidx(struct vsfsm_pt_t *pt,
					vsfsm_evt_t evt, struct vsfile_t *dir, uint32_t idx,
					struct vsfile_t **file)
{
	return VSFERR_NONE;
}

vsf_err_t fatfs_open(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file)
{
	return VSFERR_NONE;
}

vsf_err_t fatfs_close(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file)
{
	return VSFERR_NONE;
}

vsf_err_t fatfs_read(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file, uint64_t offset,
					uint32_t size, uint8_t *buff, uint32_t *rsize)
{
	return VSFERR_NONE;
}

vsf_err_t fatfs_write(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file, uint64_t offset,
					uint32_t size, uint8_t *buff, uint32_t *wsize)
{
	return VSFERR_NONE;
}

#ifndef VSFCFG_STANDALONE_MODULE
const struct vsfile_fsop_t fatfs_op =
{
	// mount / unmount
	.mount = fatfs_mount,
	.unmount = vsfile_dummy_unmount,
	// f_op
	.f_op.open = fatfs_open,
	.f_op.close = fatfs_close,
	.f_op.read = fatfs_read,
	.f_op.write = fatfs_write,
	// d_op
	.d_op.addfile
	.d_op.getchild_byname = fatfs_getchild_byname,
	.d_op.getchild_byidx = fatfs_getchild_byidx,
};
#endif
