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

#define VSFOSCFG_HANDLER_NUM				16

static vsf_err_t vsfos_busybox_uname(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	return VSFERR_NONE;
}

static vsf_err_t vsfos_busybox_free(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	return VSFERR_NONE;
}

static vsf_err_t vsfos_busybox_top(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	return VSFERR_NONE;
}

// module handlers
static vsf_err_t vsfos_busybox_lsmod(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	return VSFERR_NONE;
}

static vsf_err_t vsfos_busybox_repo(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	return VSFERR_NONE;
}

// fs handlers
struct vsfos_fsctx_t
{
	struct vsfile_t *curfile;
};
static vsf_err_t vsfos_busybox_ls(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	return VSFERR_NONE;
}

static vsf_err_t vsfos_busybox_cd(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	return VSFERR_NONE;
}

static vsf_err_t vsfos_busybox_mkdir(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	return VSFERR_NONE;
}

static vsf_err_t vsfos_busybox_rmdir(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	return VSFERR_NONE;
}

static vsf_err_t vsfos_busybox_rm(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	return VSFERR_NONE;
}

static vsf_err_t vsfos_busybox_mv(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	return VSFERR_NONE;
}

static vsf_err_t vsfos_busybox_cp(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	return VSFERR_NONE;
}

static vsf_err_t vsfos_busybox_cat(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	return VSFERR_NONE;
}

// net handlers
static vsf_err_t vsfos_busybox_ipconfig(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	return VSFERR_NONE;
}

static vsf_err_t vsfos_busybox_httpd(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	return VSFERR_NONE;
}

static vsf_err_t vsfos_busybox_dns(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	return VSFERR_NONE;
}

// usb host
static vsf_err_t vsfos_busybox_lsusb(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	return VSFERR_NONE;
}

vsf_err_t vsfos_busybox_init(struct vsfshell_t *shell)
{
	struct vsfshell_handler_t *handlers = vsf_bufmgr_malloc(VSFOSCFG_HANDLER_NUM * sizeof(*handlers));
	int idx = 0;
	if (!handlers) return VSFERR_NOT_ENOUGH_RESOURCES;

	memset(handlers, 0, VSFOSCFG_HANDLER_NUM * sizeof(struct vsfshell_handler_t));

	handlers[idx++] = (struct vsfshell_handler_t){"uname", vsfos_busybox_uname};
	handlers[idx++] = (struct vsfshell_handler_t){"free", vsfos_busybox_free};
	handlers[idx++] = (struct vsfshell_handler_t){"top", vsfos_busybox_top};

	// module handlers
	handlers[idx++] = (struct vsfshell_handler_t){"lsmod", vsfos_busybox_lsmod};
	handlers[idx++] = (struct vsfshell_handler_t){"repo", vsfos_busybox_repo};

	// fs handlers
	if (vsf_module_get(VSFILE_MODNAME) != NULL)
	{
		struct vsfos_fsctx_t *ctx = vsf_bufmgr_malloc(sizeof(*ctx));
		if (!ctx) return VSFERR_NOT_ENOUGH_RESOURCES;

		ctx->curfile = (struct vsfile_t *)&vsfile.rootfs;
		handlers[idx++] = (struct vsfshell_handler_t){"ls", vsfos_busybox_ls, ctx};
		handlers[idx++] = (struct vsfshell_handler_t){"cd", vsfos_busybox_cd, ctx};
		handlers[idx++] = (struct vsfshell_handler_t){"mkdir", vsfos_busybox_mkdir, ctx};
		handlers[idx++] = (struct vsfshell_handler_t){"rmdir", vsfos_busybox_rmdir, ctx};
		handlers[idx++] = (struct vsfshell_handler_t){"rm", vsfos_busybox_rm, ctx};
		handlers[idx++] = (struct vsfshell_handler_t){"mv", vsfos_busybox_mv, ctx};
		handlers[idx++] = (struct vsfshell_handler_t){"cp", vsfos_busybox_cp, ctx};
		handlers[idx++] = (struct vsfshell_handler_t){"cat", vsfos_busybox_cat, ctx};
	}

	// net handlers
	if (vsf_module_get(VSFIP_MODNAME) != NULL)
	{
		handlers[idx++] = (struct vsfshell_handler_t){"ipconfig", vsfos_busybox_ipconfig};
		handlers[idx++] = (struct vsfshell_handler_t){"ping", vsfos_busybox_ipconfig};
		if (vsf_module_get(VSFIP_HTTPD_MODNAME) != NULL)
		{
			handlers[idx++] = (struct vsfshell_handler_t){"httpd", vsfos_busybox_httpd};
		}
		if (vsf_module_get(VSFIP_DNSC_MODNAME) != NULL)
		{
			handlers[idx++] = (struct vsfshell_handler_t){"dns", vsfos_busybox_dns};
		}
	}

	// usb host
	if (vsf_module_get(VSFUSBH_MODNAME) != NULL)
	{
		handlers[idx++] = (struct vsfshell_handler_t){"lsusb", vsfos_busybox_lsusb};
	}

	handlers[idx++] = (struct vsfshell_handler_t)VSFSHELL_HANDLER_NONE;
	vsfshell_register_handlers(shell, handlers);
	return VSFERR_NONE;
}
