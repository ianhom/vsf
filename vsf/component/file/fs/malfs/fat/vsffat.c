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

// Refer to:
// 1. "Microsoft Extensible Firmware Initiative FAT32 File System Specification"

PACKED_HEAD struct PACKED_MID fatfs_bpb_t
{
	uint16_t BytsPerSec;
	uint8_t SecPerClus;
	uint16_t RsvdSecCnt;
	uint8_t NumFATs;
	uint16_t RootEntCnt;
	uint16_t TotSec16;
	uint8_t Media;
	uint16_t FATSz16;
	uint16_t SecPerTrk;
	uint16_t NumHeads;
	uint32_t HiddSec;
	uint32_t TotSec32;
}; PACKED_TAIL

PACKED_HEAD struct PACKED_MID fatfs_ebpb_t
{
	uint8_t DrvNum;
	uint8_t Reserved;
	uint8_t BootSig;
	uint32_t VolID;
	uint8_t VolLab[11];
	uint8_t FilSysType[8];
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
			PACKED_HEAD struct PACKED_MID
			{
				uint32_t FATSz32;
				uint16_t ExtFlags;
				uint16_t FSVer;
				uint32_t RootClus;
				uint16_t FSInfo;
				uint16_t BkBootSec;
				uint8_t Reserved[12];
			} bpb; PACKED_TAIL
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

static vsf_err_t vsffat_get_fat_entry(struct vsffat_t *fat, uint32_t cluster,
										uint32_t *sector, uint32_t *offset)
{
	return VSFERR_NONE;
}

static vsf_err_t vsffat_get_next_cluster(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
								uint32_t cur_cluster, uint32_t *next_cluster)
{
	return VSFERR_NONE;
}

static bool vsffat_is_cluster_eof(struct vsffat_t *fat, uint32_t cluster)
{
	return true;
}

static uint32_t vsffat_clus2sec(struct vsffat_t *fat, uint32_t cluster)
{
	return fat->database +
			(cluster - fat->root_cluster) >> fat->clustersize_bits;
}

static vsf_err_t vsffat_parse_dbr(struct vsffat_t *fat, struct fatfs_dbr_t *dbr)
{
	uint16_t BytsPerSec;
	uint32_t tmp32;
	uint8_t *ptr = (uint8_t *)dbr;
	int i;

	for (i = 0; i < 53; i++) if (*ptr++) break;
	if (i < 53)
	{
		// normal FAT12, FAT16, FAT32
		uint32_t sectornum, clusternum;

		// BytsPerSec MUST the same as mal blocksize, and MUST following value:
		// 		512, 1024, 2048, 4096
		BytsPerSec = dbr->bpb.BytsPerSec;
		if ((BytsPerSec != fat->malfs.malstream.mal->cap.block_size) ||
			((BytsPerSec != 512) && (BytsPerSec != 1024) &&
				(BytsPerSec != 2048) && (BytsPerSec != 4096)))
			return VSFERR_FAIL;

		// SecPerClus MUST be power of 2
		tmp32 = dbr->bpb.SecPerClus;
		fat->clustersize_bits = tmp32 & (tmp32 - 1);
		if (!tmp32 || !fat->clustersize_bits)
			return VSFERR_FAIL;

		// RsvdSecCnt CANNOT be 0
		fat->reserved_size = dbr->bpb.RsvdSecCnt;
		if (!fat->reserved_size)
			return VSFERR_FAIL;

		// NumFATs CANNOT be 0
		fat->fatnum = dbr->bpb.NumFATs;
		if (!fat->fatnum)
			return VSFERR_FAIL;

		sectornum = dbr->bpb.TotSec16;
		if (!sectornum)
			sectornum = dbr->bpb.TotSec32;
		if (!sectornum)
			return VSFERR_FAIL;

		fat->fatsize = dbr->bpb.FATSz16 ?
						dbr->bpb.FATSz16 : dbr->fat32.bpb.FATSz32;
		if (!fat->fatsize)
			return VSFERR_FAIL;

		tmp32 = fat->rootentry = dbr->bpb.RootEntCnt;
		if (tmp32)
			tmp32 = ((tmp32 >> 5) + BytsPerSec - 1) / BytsPerSec;
		fat->rootsize = tmp32;
		// calculate base
		fat->rootbase = fat->fatbase + fat->fatnum * fat->fatsize;
		fat->database = fat->rootbase + fat->rootsize;
		// calculate cluster number: note that cluster starts from root_cluster
		clusternum = (sectornum - fat->reserved_size) >> fat->clustersize_bits;

		// for FAT32 RootEntCnt MUST be 0
		if (!fat->rootentry)
		{
			// FAT32
			fat->type = VSFFAT_FAT32;

			// for FAT32, TotSec16 and FATSz16 MUST be 0
			if (dbr->bpb.FATSz16 || dbr->bpb.TotSec16)
				return VSFERR_FAIL;

			fat->fsinfo = dbr->fat32.bpb.FSInfo;

			// RootClus CANNOT be less than 2
			fat->root_cluster = dbr->fat32.bpb.RootClus;
			if (fat->root_cluster < 2)
			{
				return VSFERR_NONE;
			
			}

			fat->clusternum = fat->root_cluster + clusternum;
		}
		else
		{
			// FAT12 or FAT16
			fat->type = (clusternum < 4085) ? VSFFAT_FAT12 : VSFFAT_FAT16;

			// root has no cluster
			fat->root_cluster = 0;
			fat->clusternum = fat->root_cluster + clusternum;
		}
	}
	else
	{
		// bpb all 0, exFAT
		fat->type = VSFFAT_EXFAT;
	}

	return VSFERR_NONE;
}

vsf_err_t vsffat_mount(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
								struct vsfile_t *dir)
{
	struct vsffat_t *fatfs = (struct vsffat_t *)pt->user_data;
	struct vsf_malfs_t *malfs = &fatfs->malfs;
	struct fatfs_dentry_t *dentry;
	vsf_err_t err = VSFERR_NONE;

	vsfsm_pt_begin(pt);

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
	fatfs->type = VSFFAT_NONE;
	err = vsffat_parse_dbr(fatfs, (struct fatfs_dbr_t *)malfs->sector_buffer);
	if (err)
	{
		goto fail_crit;
	}

	vsf_malfs_read(malfs, fatfs->rootbase, malfs->sector_buffer, 1);
	vsfsm_pt_wait(pt);
	vsfsm_crit_leave(&malfs->crit);
	if (VSF_MALFS_EVT_IOFAIL == evt)
	{
		goto fail;
	}

	// check volume_id
	dentry = (struct fatfs_dentry_t *)malfs->sector_buffer;
	fatfs->volid[0] = '\0';
	if (VSFILE_ATTR_VOLUMID == dentry->Attr)
	{
		int i;

		memcpy(fatfs->volid, dentry->Name, 11);
		for (i = 10; i >= 0; i--)
		{
			if (fatfs->volid[i] != ' ')
			{
				break;
			}
			fatfs->volid[i] = '\0';
		}
	}
	malfs->volume_name = fatfs->volid;

	// initialize root
	fatfs->root.file.name = fatfs->volid;
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
	if (!fatfs->cur_cluster)
	{
		// it's ROOT for FAT12 and FAT16
		fatfs->cur_sector = fatfs->rootbase;
	}
	else
	{
		fatfs->cur_sector = vsffat_clus2sec(fatfs, fatfs->cur_cluster);
	}

	while (1)
	{
		vsf_malfs_read(malfs, fatfs->cur_sector, malfs->sector_buffer, 1);
		vsfsm_pt_wait(pt);
		if (VSF_MALFS_EVT_IOFAIL == evt)
		{
			err = VSFERR_FAIL;
			break;
		}

		dentry = (struct fatfs_dentry_t *)malfs->sector_buffer;
		// TODO: try to search file equal to name
		// TODO: if found, allocate and initialize file structure

		if (fatfs->cur_sector & ((1 << fatfs->clustersize_bits) - 1))
		{
			// not found in current sector, find next sector
			fatfs->cur_sector++;
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

			fatfs->cur_sector = vsffat_clus2sec(fatfs, fatfs->cur_cluster);
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
