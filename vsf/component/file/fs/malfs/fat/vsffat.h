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

#ifndef __VSFFAT_H_INCLUDED__
#define __VSFFAT_H_INCLUDED__

struct vsffat_t;
struct vsfile_fatfile_t
{
	struct vsfile_t file;

	struct vsffat_t *fat;
	uint32_t first_cluster;
};

enum vsffat_type_t
{
	VSFFAT_NONE = 0,
	VSFFAT_FAT12,
	VSFFAT_FAT16,
	VSFFAT_FAT32,
	VSFFAT_EXFAT,
};

struct vsffat_t
{
	struct vsf_malfs_t malfs;

	// protected
	enum vsffat_type_t type;

	// information parsed from bpb
	uint8_t sectorsize_bits;
	uint8_t clustersize_bits;
	uint8_t fatnum;
	uint16_t reserved_size;
	uint16_t rootentry;
	uint16_t fsinfo;
	uint32_t fatsize;
	uint32_t root_cluster;

	// information calculated
	uint32_t clusternum;
	uint32_t fatbase;
	uint32_t database;
	uint32_t rootbase;
	uint32_t rootsize;
	char volid[11];

	// private
	struct vsfile_fatfile_t root;
	struct vsfsm_pt_t caller_pt;

	// for getchild_byname and getchild_byidx
	uint32_t cur_cluster;
	uint32_t cur_sector;
	// for vsffat_get_FATentry
	uint32_t readbit;
	// for vsffat_alloc_cluschain
};

#ifndef VSFCFG_STANDALONE_MODULE
extern const struct vsfile_fsop_t vsffat_op;
#endif

#endif	// __VSFFAT_H_INCLUDED__
