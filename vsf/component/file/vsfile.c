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

#define VSFILE
#define VSFILE_EVT_CRIT					VSFSM_EVT_USER

struct vsfile_srch_t
{
	struct vsfile_t *cur_file;
	char *cur_name;
	uint32_t cur_idx;
	struct vsfsm_crit_t crit;
	struct vsfsm_pt_t file_pt;
} static vsfile_srch;

// helper
char* vsfile_getfileext(char* fname)
{
	char *ext, *tmp;

	ext = tmp = fname;
	while (1)
	{
		tmp = strchr(ext, '.');
		if (tmp != NULL)
			ext = tmp + 1;
		else
			break;
	}
	return ext;
}

bool vsfile_is_div(char ch)
{
	return ('\\' == ch) || ('/' == ch);
}

vsf_err_t vsfile_init(void)
{
	return vsfsm_crit_init(&vsfile_srch.crit, VSFILE_EVT_CRIT);
}

vsf_err_t vsfile_getfile(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir, char *name, struct vsfile_t **file)
{
	vsf_err_t err = VSFERR_NONE;

	vsfsm_pt_begin(pt);

	if (vsfsm_crit_enter(&vsfile_srch.crit, pt->sm))
	{
		vsfsm_pt_wfe(pt, VSFILE_EVT_CRIT);
	}

	vsfile_srch.cur_name = name;
	vsfile_srch.cur_file = dir;
	while (*vsfile_srch.cur_name != '\0')
	{
		if (vsfile_is_div(*vsfile_srch.cur_name))
		{
			if (!(vsfile_srch.cur_file->attr & VSFILE_ATTR_DIRECTORY))
			{
				// want to find something under a file not a directory
				return VSFERR_FAIL;
			}

			vsfile_srch.cur_name++;
			if ('\0' == *vsfile_srch.cur_name)
			{
				break;
			}
		}

		vsfile_srch.file_pt.state = 0;
		vsfsm_pt_entry(pt);
		err = vsfile_srch.cur_file->op->d_op.getchild_byname(
					&vsfile_srch.file_pt, evt, vsfile_srch.cur_file,
					vsfile_srch.cur_name, file);
		if (err > 0) return err; else if (err < 0)
		{
			goto end;
		}
		vsfile_srch.cur_name += strlen((*file)->name);
		vsfile_srch.cur_file = *file;
	}
	*file = vsfile_srch.cur_file;
end:
	vsfsm_crit_leave(&vsfile_srch.crit);

	vsfsm_pt_end(pt);
	return err;
}

vsf_err_t vsfile_findfirst(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir, struct vsfile_t **file)
{
	vsf_err_t err = VSFERR_NONE;

	vsfsm_pt_begin(pt);

	if (vsfsm_crit_enter(&vsfile_srch.crit, pt->sm))
	{
		vsfsm_pt_wfe(pt, VSFILE_EVT_CRIT);
	}

	vsfile_srch.cur_idx = 0;
	vsfile_srch.file_pt.state = 0;
	vsfsm_pt_entry(pt);
	err = dir->op->d_op.getchild_byidx(&vsfile_srch.file_pt, evt, dir,
					vsfile_srch.cur_idx, file);
	if (err != 0) return err;

	vsfile_srch.cur_idx++;
	vsfsm_pt_end(pt);
	return err;
}

vsf_err_t vsfile_findnext(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir, struct vsfile_t **file)
{
	vsf_err_t err = VSFERR_NONE;

	vsfsm_pt_begin(pt);

	vsfile_srch.file_pt.state = 0;
	vsfsm_pt_entry(pt);
	err = dir->op->d_op.getchild_byidx(&vsfile_srch.file_pt, evt, dir,
					vsfile_srch.cur_idx, file);
	if (err != 0) return err;

	vsfile_srch.cur_idx++;
	vsfsm_pt_end(pt);
	return err;
}

vsf_err_t vsfile_findend(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir)
{
	return vsfsm_crit_leave(&vsfile_srch.crit);
}

vsf_err_t vsfile_mount(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
						struct vsfile_fsop_t *op, struct vsfile_t *dir)
{
	return op->mount(pt, evt, dir);
}

vsf_err_t vsfile_unmount(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
						struct vsfile_t *dir)
{
	// TODO
	return VSFERR_NONE;
}

vsf_err_t vsfile_open(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file)
{
	if (!(file->attr & VSFILE_ATTR_ARCHIVE))
	{
		return VSFERR_NOT_ACCESSABLE;
	}

	return file->op->f_op.open(pt, evt, file);
}

vsf_err_t vsfile_close(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file)
{
	return file->op->f_op.close(pt, evt, file);
}

vsf_err_t vsfile_read(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file, uint64_t offset,
					uint32_t size, uint8_t *buff, uint32_t *rsize)
{
	if (file->attr & VSFILE_ATTR_WRITEONLY)
	{
		return VSFERR_NOT_ACCESSABLE;
	}

	return file->op->f_op.read(pt, evt, file, offset, size, buff, rsize);
}

vsf_err_t vsfile_write(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file, uint64_t offset,
					uint32_t size, uint8_t *buff, uint32_t *wsize)
{
	if (file->attr & VSFILE_ATTR_READONLY)
	{
		return VSFERR_NOT_ACCESSABLE;
	}

	return file->op->f_op.write(pt, evt, file, offset, size, buff, wsize);
}

// dummy
vsf_err_t vsfile_dummy_file(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir)
{
	return VSFERR_NONE;
}
vsf_err_t vsfile_dummy_rw(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *dir)
{
	return VSFERR_NONE;
}

// memfs
vsf_err_t vsfile_memfs_getchild_byname(struct vsfsm_pt_t *pt,
					vsfsm_evt_t evt, struct vsfile_t *dir, char *name,
					struct vsfile_t **file)
{
	struct vsfile_memfile_t *memfile = (struct vsfile_memfile_t *)dir;
	struct vsfile_t *child = memfile->child;
	char ch;

	while ((child != NULL) && (child->name != NULL))
	{
		if (strstr(name, child->name) == name)
		{
			ch = name[strlen(child->name)];
			if (('\0' == ch) || vsfile_is_div(ch))
			{
				break;
			}
		}
		child = (struct vsfile_t *)((uint32_t)child + memfile->child_file_size);
	}
	*file = child;
	return (NULL == child) ? VSFERR_NOT_AVAILABLE : VSFERR_NONE;
}

vsf_err_t vsfile_memfs_getchild_byidx(struct vsfsm_pt_t *pt,
					vsfsm_evt_t evt, struct vsfile_t *dir, uint32_t idx,
					struct vsfile_t **file)
{
	struct vsfile_memfile_t *memfile = (struct vsfile_memfile_t *)dir;
	struct vsfile_t *child = memfile->child;
	uint32_t i;

	for (i = 0; i < idx; i++)
	{
		if (NULL == child->name)
		{
			return VSFERR_NOT_AVAILABLE;
		}
		child = (struct vsfile_t *)((uint32_t)child + memfile->child_file_size);
	}

	*file = child;
	return VSFERR_NONE;
}

vsf_err_t vsfile_memfs_read(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file, uint64_t offset,
					uint32_t size, uint8_t *buff, uint32_t *rsize)
{
	struct vsfile_memfile_t *memfile = (struct vsfile_memfile_t *)file;

	if (offset >= file->size)
	{
		*rsize = 0;
		return VSFERR_NONE;
	}

	*rsize = min(size, file->size - offset);
	memcpy(buff, &memfile->buff[offset], *rsize);
	return VSFERR_NONE;
}

vsf_err_t vsfile_memfs_write(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
					struct vsfile_t *file, uint64_t offset,
					uint32_t size, uint8_t *buff, uint32_t *wsize)
{
	struct vsfile_memfile_t *memfile = (struct vsfile_memfile_t *)file;

	if (offset >= file->size)
	{
		*wsize = 0;
		return VSFERR_NONE;
	}

	*wsize = min(size, file->size - offset);
	memcpy(&memfile->buff[offset], buff, *wsize);
	return VSFERR_NONE;
}

const struct vsfile_fsop_t vsfile_memfs_op =
{
	// mount / unmount
	.mount = vsfile_dummy_mount,
	.unmount = vsfile_dummy_unmount,
	// f_op
	.f_op.open = vsfile_dummy_open,
	.f_op.close = vsfile_dummy_close,
	.f_op.read = vsfile_memfs_read,
	.f_op.write = vsfile_memfs_write,
	// d_op
	.d_op.getchild_byname = vsfile_memfs_getchild_byname,
	.d_op.getchild_byidx = vsfile_memfs_getchild_byidx,
};
