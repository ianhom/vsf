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

#include "../vsf_malfs.h"
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

PACKED_HEAD struct PACKED_MID fatfs_dentry_t
{
	char Name[11];
	uint8_t Attr;
	uint8_t NTRes;
	uint8_t CrtTimeTenth;
	uint16_t CrtTime;
	uint16_t CrtData;
	uint16_t LstAccData;
	uint16_t FstClusHI;
	uint16_t WrtTime;
	uint16_t WrtData;
	uint16_t FstClusLO;
	uint32_t FileSize;
}; PACKED_TAIL

static vsf_err_t vsffat_get_fat_entry(struct vsffat_t *fatfs, uint32_t cluster,
										uint32_t *page, uint32_t *offset)
{
	return VSFERR_NONE;
}

static vsf_err_t vsffat_get_next_cluster(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
								uint32_t cur_cluster, uint32_t *next_cluster)
{
	return VSFERR_NONE;
}

static bool vsffat_is_cluster_eof(struct vsffat_t *fatfs, uint32_t cluster)
{
	return true;
}

static uint32_t vsffat_cluster_page(struct vsffat_t *fatfs, uint32_t cluster)
{
	return 0;
}

vsf_err_t vsffat_mount(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
								struct vsfile_t *dir)
{
	struct vsffat_t *fatfs = (struct vsffat_t *)pt->user_data;
	struct vsf_malfs_t *malfs = &fatfs->malfs;
	uint32_t pagesize = malfs->malstream.mal->cap.block_size;
	struct fatfs_dbr_t *dbr;
	struct fatfs_dentry_t *dentry;
	vsf_err_t err = VSFERR_NONE;

	vsfsm_pt_begin(pt);

	if (pagesize < 512)
	{
		return VSFERR_INVALID_PARAMETER;
	}
	err = vsf_malfs_init(malfs);
	if (err) goto fail;

	if (vsfsm_crit_enter(&malfs->crit, pt->sm))
	{
		vsfsm_pt_wfe(pt, VSF_MALFS_EVT_CRIT);
	}
	malfs->notifier_sm = pt->sm;

	vsf_malfs_read(malfs, 0, malfs->sector_buffer, 1);
	vsfsm_pt_wait(pt);
	if (VSF_MALFS_EVT_IOFAIL == evt)
	{
		goto fail_crit;
	}

	// parse DBR
	dbr = (struct fatfs_dbr_t *)malfs->sector_buffer;
	fatfs->sectors_per_cluster = dbr->bpb.SectorsPerCluster;
	fatfs->fat_bitsize = 32;		// we test on FAT32 first
	fatfs->fat_num = dbr->bpb.NumberOfFAT;
	fatfs->reserved_sectors = dbr->bpb.ReservedSector;
	// for FAT32, SectorsPerFAT MUST be 0
	switch (fatfs->fat_bitsize)
	{
	case 12: case 16:
		fatfs->fat_size = dbr->bpb.SectorsPerFAT;
		fatfs->root_cluster = 2;
		fatfs->root_sector_num = 32;
		break;
	case 32:
		fatfs->fat_size = dbr->fat32.fat32_bpb.SectorsPerFAT32;
		fatfs->root_cluster = dbr->fat32.fat32_bpb.RootClusterNumber;
		fatfs->root_sector_num = 0;
		break;
	}
	fatfs->hidden_sectors = dbr->bpb.HiddenSector;
	// ROOT follows FAT
	fatfs->root_sector = fatfs->fat_sector + fatfs->fat_num * fatfs->fat_size;

	vsf_malfs_read(malfs, fatfs->root_sector, malfs->sector_buffer, 1);
	vsfsm_pt_wait(pt);
	vsfsm_crit_leave(&malfs->crit);
	if (VSF_MALFS_EVT_IOFAIL == evt)
	{
		goto fail;
	}

	// check volume_id
	dentry = (struct fatfs_dentry_t *)malfs->sector_buffer;
	fatfs->volume_id[0] = '\0';
	if (VSFILE_ATTR_VOLUMID == dentry->Attr)
	{
		int i;

		memcpy(fatfs->volume_id, dentry->Name, 11);
		for (i = 10; i >= 0; i--)
		{
			if (fatfs->volume_id[i] != ' ')
			{
				break;
			}
			fatfs->volume_id[i] = '\0';
		}
	}
	malfs->volume_name = fatfs->volume_id;

	// initialize root
	fatfs->root.file.name = fatfs->volume_id;
	fatfs->root.file.size = 0;
	fatfs->root.file.attr = VSFILE_ATTR_DIRECTORY;
	fatfs->root.file.op = (struct vsfile_fsop_t *)&vsffat_op;
	fatfs->root.file.parent = NULL;
	fatfs->root.fat = fatfs;
	fatfs->root.first_cluster = fatfs->root_cluster;

	vsfsm_pt_end(pt);
	return err;
fail_crit:
	vsfsm_crit_leave(&malfs->crit);
fail:
	vsf_malfs_fini(malfs);
	return err;
}

vsf_err_t vsffat_unmount(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
								struct vsfile_t *dir)
{
	struct vsffat_t *fatfs = (struct vsffat_t *)pt->user_data;
	vsf_malfs_fini(&fatfs->malfs);
	return VSFERR_NONE;
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
	struct vsffat_t *fatfs = (struct vsffat_t *)pt->user_data;
	struct vsf_malfs_t *malfs = &fatfs->malfs;
	struct vsfile_fatfile_t *fatdir;
	struct fatfs_dentry_t *dentry;
	vsf_err_t err = VSFERR_NONE;

	if (NULL == dir)
	{
		dir = (struct vsfile_t *)&((struct vsffat_t *)pt->user_data)->root;
	}
	fatdir = (struct vsfile_fatfile_t *)dir;

	vsfsm_pt_begin(pt);

	if (vsfsm_crit_enter(&malfs->crit, pt->sm))
	{
		vsfsm_pt_wfe(pt, VSF_MALFS_EVT_CRIT);
	}
	malfs->notifier_sm = pt->sm;
	fatfs->cur_cluster = fatdir->first_cluster;
	fatfs->cur_page = vsffat_cluster_page(fatfs, fatfs->cur_cluster);

	while (1)
	{
		vsf_malfs_read(malfs, fatfs->cur_page, malfs->sector_buffer, 1);
		vsfsm_pt_wait(pt);
		if (VSF_MALFS_EVT_IOFAIL == evt)
		{
			err = VSFERR_FAIL;
			break;
		}

		dentry = (struct fatfs_dentry_t *)malfs->sector_buffer;
		// TODO: try to search file equal to name
		// TODO: if found, allocate and initialize file structure

		if (fatfs->cur_page % fatfs->sectors_per_cluster)
		{
			// not found in current page, find next page
			fatfs->cur_page++;
		}
		else
		{
			// not found in current cluster, find next cluster if exists
			fatfs->caller_pt.sm = pt->sm;
			fatfs->caller_pt.user_data = fatfs;
			fatfs->caller_pt.state = 0;
			vsfsm_pt_entry(pt);
			err = vsffat_get_next_cluster(&fatfs->caller_pt, evt,
									fatfs->cur_cluster, &fatfs->cur_cluster);
			if (err > 0) return err; else if (err < 0) break;

			if (vsffat_is_cluster_eof(fatfs, fatfs->cur_cluster))
			{
				// not found
				err = VSFERR_NOT_AVAILABLE;
				break;
			}

			fatfs->cur_page = vsffat_cluster_page(fatfs, fatfs->cur_cluster);
		}
	}

	vsfsm_crit_leave(&malfs->crit);
	vsfsm_pt_end(pt);
	return err;
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
	.unmount = vsffat_unmount,
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
