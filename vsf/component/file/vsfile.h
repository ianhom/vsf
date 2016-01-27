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

#ifndef __VSFILE_H_INCLUDED__
#define __VSFILE_H_INCLUDED__

enum vsfile_attr_t
{
	VSFILE_ATTR_READONLY		= 1 << 0,
	VSFILE_ATTR_HIDDEN			= 1 << 1,
	VSFILE_ATTR_SYSTEM			= 1 << 2,
	VSFILE_ATTR_VOLUMID			= 1 << 3,
	VSFILE_ATTR_DIRECTORY		= 1 << 4,
	VSFILE_ATTR_ARCHIVE			= 1 << 5,
	VSFILE_ATTR_WRITEONLY		= 1 << 6,
};

struct vsfile_t;
struct vsfile_fop_t
{
	vsf_err_t (*open)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file);
	vsf_err_t (*close)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file);
	vsf_err_t (*read)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file, uint64_t offset,
					uint32_t size, uint8_t *buff, uint32_t *rsize);
	vsf_err_t (*write)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file, uint64_t offset,
					uint32_t size, uint8_t *buff, uint32_t *wsize);
};
struct vsfile_dop_t
{
	vsf_err_t (*getchild_byname)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir, char *name, struct vsfile_t **file);
	vsf_err_t (*getchild_byidx)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir, uint32_t idx, struct vsfile_t **file);
	vsf_err_t (*addfile)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir, char *name);
	vsf_err_t (*removefile)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir, char *name);
};
struct vsfile_fsop_t
{
	vsf_err_t (*mount)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir);
	vsf_err_t (*unmount)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir);
	struct vsfile_fop_t f_op;
	struct vsfile_dop_t d_op;
};

struct vsfile_t
{
	char *name;
	uint64_t size;
	enum vsfile_attr_t attr;
	struct vsfile_fsop_t *op;
	struct vsfile_t *parent;
};

vsf_err_t vsfile_init(void);

vsf_err_t vsfile_getfile(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir, char *name, struct vsfile_t **file);
vsf_err_t vsfile_findfirst(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir, struct vsfile_t **file);
vsf_err_t vsfile_findnext(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir, struct vsfile_t **file);
vsf_err_t vsfile_findend(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir);

vsf_err_t vsfile_open(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file);
vsf_err_t vsfile_close(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file);
vsf_err_t vsfile_read(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file, uint64_t offset,
					uint32_t size, uint8_t *buff, uint32_t *rsize);
vsf_err_t vsfile_write(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file, uint64_t offset,
					uint32_t size, uint8_t *buff, uint32_t *wsize);

// helper
char* vsfile_getfileext(char* name);

// dummy op
vsf_err_t vsfile_dummy_file(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir);
vsf_err_t vsfile_dummy_rw(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir);
#define vsfile_dummy_mount		vsfile_dummy_file
#define vsfile_dummy_unmount	vsfile_dummy_file
#define vsfile_dummy_open		vsfile_dummy_file
#define vsfile_dummy_close		vsfile_dummy_file
#define vsfile_dummy_read		vsfile_dummy_rw
#define vsfile_dummy_write		vsfile_dummy_rw

// memfile:
// ptr of file points to a buffer
// ptr of directory point to an array of children files
struct vsfile_memfile_t
{
	struct vsfile_t file;
	void *ptr;
};
extern struct vsfile_fsop_t vsf_memfs_op;

#endif		// __VSFILE_H_INCLUDED__
