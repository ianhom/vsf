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

static void vsfsm_enter_critical_internal(void)
{
	vsf_enter_critical();
}
static void vsfsm_leave_critical_internal(void)
{
	vsf_leave_critical();
}

static struct vsf_module_t *modulelist = NULL;
static void vsf_module_callback(bool load, struct vsf_module_t *module)
{
	struct vsf_module_t *moduletmp = modulelist;
	void (*callback)(struct vsf_module_t *, struct vsf_module_t *);
	
	while (moduletmp != NULL)
	{
		if (moduletmp != module)
		{
			callback = load ? moduletmp->callback.on_load :
								moduletmp->callback.on_unload;
			if (callback != NULL)
			{
				callback(moduletmp, module);
			}
		}
		moduletmp = moduletmp->next;
	}
}

static vsf_err_t vsf_module_register(struct vsf_module_t *module)
{
	if ((NULL == module) || (NULL == module->name))
	{
		return VSFERR_NONE;
	}
	
	module->next = modulelist;
	modulelist = module;
	vsf_module_callback(true, module);
	return VSFERR_NONE;
}

vsf_err_t vsf_module_unregister(struct vsf_module_t *module)
{
	if (module == modulelist)
	{
		vsf_module_callback(false, module);
		modulelist = modulelist->next;
	}
	else
	{
		struct vsf_module_t *moduletmp = modulelist;
		
		while (moduletmp->next != NULL)
		{
			if (moduletmp->next == module)
			{
				vsf_module_callback(false, module);
				moduletmp->next = module->next;
			}
			moduletmp = moduletmp->next;
		}
	}
	return VSFERR_NONE;
}

struct vsf_module_t* vsf_module_get(char *name)
{
	struct vsf_module_t *moduletmp = modulelist;
	
	while (moduletmp != NULL)
	{
		if (!strcmp(moduletmp->name, name))
		{
			return moduletmp;
		}
		moduletmp = moduletmp->next;
	}
	return NULL;
}

ROOTFUNC const struct vsf_t vsf @ 0x08000200 =
{
	VSF_API_VERSION,		// api_ver
	&core_interfaces,
	
	{
		vsfsm_init,
		vsfsm_pt_init,
		vsfsm_post_evt,
		vsfsm_post_evt_pending,
		vsfsm_enter_critical_internal,
		vsfsm_leave_critical_internal,
		
		{
			vsfsm_sync_init,
			vsfsm_sync_cancel,
			vsfsm_sync_increase,
			vsfsm_sync_decrease,
		},					// struct sync;
		
		{
			vsftimer_create,
			vsftimer_free,
			vsftimer_enqueue,
			vsftimer_dequeue,
		},					// struct timer;
		
		{
			vsf_module_register,
			vsf_module_unregister,
			vsf_module_get,
		},					// struct module;
	},						// struct vsf_framework_t framework;
	
	{
		{
			vsf_fifo_init,
			vsf_fifo_push8,
			vsf_fifo_pop8,
			vsf_fifo_push,
			vsf_fifo_pop,
			vsf_fifo_get_data_length,
			vsf_fifo_get_avail_length,
		},					// struct fifo
		{
			vsf_bufmgr_malloc,
			vsf_bufmgr_malloc_aligned,
			vsf_bufmgr_free,
		},					// struct bufmgr;
	},						// struct buffer;
};
