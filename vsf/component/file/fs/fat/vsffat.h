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

struct vsfile_fatfs_param_t
{
	struct vsfmal_t *mal;

	// protected
	uint8_t sectors_per_cluster;
	uint8_t fat_bitsize;
	uint8_t fat_num;
	union
	{
		// FAT starts after reserved sectors
		uint16_t fat_sector;
		uint16_t reserved_sectors;
	};
	uint32_t fat_size;
	uint32_t hidden_sectors;
	uint32_t root_cluster;
	uint32_t root_sector;
	uint16_t root_sector_num;		// FAT12 FAT 16 only

	// private
	struct vsfsm_crit_t crit;
	// resourced below are protected by crit
	uint8_t *sector_buffer;
	struct vsfsm_pt_t caller_pt;
};

struct vsfile_fatfile_t
{
	struct vsfile_t file;

	struct vsfile_fatfs_param_t *fs_param;
	uint32_t first_cluster;
};

#ifndef VSFCFG_STANDALONE_MODULE
extern const struct vsfile_fsop_t vsffat_op;
#endif

#endif	// __VSFFAT_H_INCLUDED__
