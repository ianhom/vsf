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
#include "vsffat.h"

PACKED_HEAD struct PACKED_MID fatfs_bpb_t
{
	uint16_t BytesPerSector;
	uint8_t SectorsPerCluster;
	uint16_t ReservedSector;
	uint8_t NumberOfFAT;
	uint16_t RootEntries;
	uint16_t SmallSector;
	uint8_t MediaDescriptor;
	uint16_t SectorsPerFAT;
	uint16_t SectorsPerTrack;
	uint16_t NumberOfHead;
	uint32_t HiddenSector;
	uint32_t LargeSector;
}; PACKED_TAIL

PACKED_HEAD struct PACKED_MID fatfs_ebpb_t
{
	uint8_t PhysicalDriveNumber;
	uint8_t Reserved;
	uint8_t ExtendedBootSignature;
	uint32_t VolumeSerialNumber;
	uint8_t VolumeLabel[11];
	uint8_t SystemID[8];
}; PACKED_TAIL

PACKED_HEAD struct PACKED_MID fatfs_dbr_t
{
	uint8_t jmp[3];
	uint8_t OEM[8];
	struct fatfs_bpb_t bpb;
	union
	{
		struct
		{
			struct
			{
				uint32_t SectorsPerFAT32;
				uint16_t ExtendedFlag;
				uint16_t FileSystemVersion;
				uint32_t RootClusterNumber;
				uint16_t FSINFO_SectorNumber;
				uint16_t BackupBootSector;
				uint8_t fill[12];
			} fat32_bpb;
			struct fatfs_ebpb_t ebpb;
			uint8_t Bootstrap[420];
		} fat32;
		struct
		{
			struct fatfs_ebpb_t ebpb;
			uint8_t Bootstrap[448];
		} fat1216;
	};
	uint16_t Magic;
}; PACKED_TAIL

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

static vsf_err_t vsffat_alloc(struct vsfile_fatfs_param_t *param)
{
	uint32_t pagesize = param->mal->cap.block_size;

	if (pagesize < 512)
	{
		return VSFERR_INVALID_PARAMETER;
	}

	param->sector_buffer = vsf_bufmgr_malloc(pagesize);
	return (NULL == param->sector_buffer) ?
					VSFERR_NOT_ENOUGH_RESOURCES : VSFERR_NONE;
}

static void vsffat_free(struct vsfile_fatfs_param_t *param)
{
	if (param->sector_buffer != NULL)
	{
		vsf_bufmgr_free(param->sector_buffer);
		param->sector_buffer = NULL;
	}
}

vsf_err_t vsffat_mount(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
								struct vsfile_t *dir)
{
	struct vsfile_fatfs_param_t *param =
							(struct vsfile_fatfs_param_t *)pt->user_data;
	uint32_t pagesize = param->mal->cap.block_size;
	struct fatfs_dbr_t *dbr;
	vsf_err_t err = VSFERR_NONE;

	vsfsm_pt_begin(pt);

	err = vsffat_alloc(param);
	if (err) return err;

	param->caller_pt.user_data = param->mal;
	param->caller_pt.sm = pt->sm;
	param->caller_pt.state = 0;
	vsfsm_pt_entry(pt);
	err = vsfmal_read(&param->caller_pt, evt, 0, param->sector_buffer, pagesize);
	if (err > 0) return err; else if (err < 0)
	{
		goto fail;
	}

	// parse DBR
	dbr = (struct fatfs_dbr_t *)param->sector_buffer;
	param->sectors_per_cluster = dbr->bpb.SectorsPerCluster;
	param->fat_bitsize = 32;		// we test on FAT32 first
	param->fat_num = dbr->bpb.NumberOfFAT;
	param->reserved_sectors = dbr->bpb.ReservedSector;
	// for FAT32, SectorsPerFAT MUST be 0
	switch (param->fat_bitsize)
	{
	case 12: case 16:
		param->fat_size = dbr->bpb.SectorsPerFAT;
		param->root_cluster = 2;
		param->root_sector_num = 32;
		break;
	case 32:
		param->fat_size = dbr->fat32.fat32_bpb.SectorsPerFAT32;
		param->root_cluster = dbr->fat32.fat32_bpb.RootClusterNumber;
		param->root_sector_num = 0;
		break;
	}
	param->hidden_sectors = dbr->bpb.HiddenSector;
	// ROOT follows FAT
	param->root_sector = param->fat_sector + param->fat_num * param->fat_size;

	vsfsm_pt_end(pt);
	return err;
fail:
	vsffat_free(param);
	return err;
}

vsf_err_t vsffat_addfile(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir, char *name, enum vsfile_attr_t attr)
{
	return VSFERR_NONE;
}

vsf_err_t vsffat_removefile(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir, char *name)
{
	return VSFERR_NONE;
}

vsf_err_t vsffat_getchild_byname(struct vsfsm_pt_t *pt,
					vsfsm_evt_t evt, struct vsfile_t *dir, char *name,
					struct vsfile_t **file)
{
	return VSFERR_NONE;
}

vsf_err_t vsffat_getchild_byidx(struct vsfsm_pt_t *pt,
					vsfsm_evt_t evt, struct vsfile_t *dir, uint32_t idx,
					struct vsfile_t **file)
{
	return VSFERR_NONE;
}

vsf_err_t vsffat_open(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file)
{
	return VSFERR_NONE;
}

vsf_err_t vsffat_close(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file)
{
	return VSFERR_NONE;
}

vsf_err_t vsffat_read(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file, uint64_t offset,
					uint32_t size, uint8_t *buff, uint32_t *rsize)
{
	return VSFERR_NONE;
}

vsf_err_t vsffat_write(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file, uint64_t offset,
					uint32_t size, uint8_t *buff, uint32_t *wsize)
{
	return VSFERR_NONE;
}

#ifndef VSFCFG_STANDALONE_MODULE
const struct vsfile_fsop_t vsffat_op =
{
	// mount / unmount
	.mount = vsffat_mount,
	.unmount = vsfile_dummy_unmount,
	// f_op
	.f_op.open = vsffat_open,
	.f_op.close = vsffat_close,
	.f_op.read = vsffat_read,
	.f_op.write = vsffat_write,
	// d_op
	.d_op.addfile = vsffat_addfile,
	.d_op.removefile = vsffat_removefile,
	.d_op.getchild_byname = vsffat_getchild_byname,
	.d_op.getchild_byidx = vsffat_getchild_byidx,
};
#endif
