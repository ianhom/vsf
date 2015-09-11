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

#ifndef __VSF_H_INCLUDED__
#define __VSF_H_INCLUDED__

#include "app_cfg.h"
#include "app_type.h"

#include "compiler.h"

#include "interfaces.h"

#include "framework/vsfsm/vsfsm.h"
#include "framework/vsftimer/vsftimer.h"

#include "tool/buffer/buffer.h"

#define VSF_API_VERSION						0x00000001

struct vsf_module_t
{
	char *name;
	void *ifs;
	struct
	{
		void (*on_load)(struct vsf_module_t *me, struct vsf_module_t *new);
		void (*on_unload)(struct vsf_module_t *me, struct vsf_module_t *old);
	} callback;
	struct vsf_module_t *next;
};

struct vsf_t
{
	uint32_t api_ver;
	const struct interfaces_info_t *ifs;
	
	struct vsf_framework_t
	{
		vsf_err_t (*sm_init)(struct vsfsm_t *sm);
		vsf_err_t (*pt_init)(struct vsfsm_t *sm, struct vsfsm_pt_t *pt);
		vsf_err_t (*post_evt)(struct vsfsm_t *sm, vsfsm_evt_t evt);
		vsf_err_t (*post_evt_pending)(struct vsfsm_t *sm, vsfsm_evt_t evt);
		
		void (*enter_critical)(void);
		void (*leave_critical)(void);
		
		struct
		{
			vsf_err_t (*init)(struct vsfsm_sync_t *sync, uint32_t cur_value,
								uint32_t max_value, vsfsm_evt_t evt);
			vsf_err_t (*cancel)(struct vsfsm_sync_t *sync, struct vsfsm_t *sm);
			vsf_err_t (*increase)(struct vsfsm_sync_t *sync);
			vsf_err_t (*decrease)(struct vsfsm_sync_t *sync,
								struct vsfsm_t *sm);
		} sync;
		
		struct
		{
			struct vsftimer_t * (*create)(struct vsfsm_t *sm, uint32_t interval,
										int16_t trigger_cnt, vsfsm_evt_t evt);
			void (*free)(struct vsftimer_t *timer);
			void (*enqueue)(struct vsftimer_t *timer);
			void (*dequeue)(struct vsftimer_t *timer);
		} timer;
		
		struct
		{
			vsf_err_t (*load)(struct vsf_module_t *module);
			vsf_err_t (*unload)(struct vsf_module_t *module);
			struct vsf_module_t* (*get)(char *name);
		} module;
	} framework;
	
	struct
	{
		struct
		{
			vsf_err_t (*init)(struct vsf_fifo_t *fifo);
			uint32_t (*push8)(struct vsf_fifo_t *fifo, uint8_t data);
			uint8_t (*pop8)(struct vsf_fifo_t *fifo);
			uint32_t (*push)(struct vsf_fifo_t *fifo, uint32_t size, uint8_t *data);
			uint32_t (*pop)(struct vsf_fifo_t *fifo, uint32_t size, uint8_t *data);
			uint32_t (*get_data_length)(struct vsf_fifo_t *fifo);
			uint32_t (*get_avail_length)(struct vsf_fifo_t *fifo);
		} fifo;
		
		struct
		{
			void* (*malloc)(uint32_t size);
			void* (*malloc_aligned)(uint32_t size, uint32_t align);
			void (*free)(void *ptr);
		} bufmgr;
	} buffer;
};

#ifndef VSF_SYS

#define VSF_BASE						((struct vsf_t *)VSF_BASE_ADDR)
#define vsf_api_ver						(VSF_BASE->api_ver)
#define core_interfaces					(*(VSF_BASE->ifs))

#define vsfsm_init						VSF_BASE->framework.sm_init
#define vsfsm_pt_init					VSF_BASE->framework.pt_init
#define vsfsm_post_evt					VSF_BASE->framework.post_evt
#define vsfsm_post_evt_pending			VSF_BASE->framework.post_evt_pending

#undef vsf_enter_critical
#define vsf_enter_critical				VSF_BASE->framework.enter_critical
#undef vsf_leave_critical
#define vsf_leave_critical				VSF_BASE->framework.leave_critical

#define vsfsm_sync_init					VSF_BASE->framework.sync.init
#define vsfsm_sync_cancel				VSF_BASE->framework.sync.cancel
#define vsfsm_sync_increase				VSF_BASE->framework.sync.increase
#define vsfsm_sync_decrease				VSF_BASE->framework.sync.decrease

#define vsftimer_create					VSF_BASE->framework.timer.create
#define vsftimer_free					VSF_BASE->framework.timer.free
#define vsftimer_enqueue				VSF_BASE->framework.timer.enqueue
#define vsftimer_dequeue				VSF_BASE->framework.timer.dequeue

#define vsf_module_load					VSF_BASE->framework.module.load
#define vsf_module_unload				VSF_BASE->framework.module.unload
#define vsf_module_get					VSF_BASE->framework.module.get

#define vsf_fifo_init					VSF_BASE->buffer.fifo.init
#define vsf_fifo_push8					VSF_BASE->buffer.fifo.push8
#define vsf_fifo_pop8					VSF_BASE->buffer.fifo.pop8
#define vsf_fifo_push					VSF_BASE->buffer.fifo.push
#define vsf_fifo_pop					VSF_BASE->buffer.fifo.pop
#define vsf_fifo_get_data_length		VSF_BASE->buffer.fifo.get_data_length
#define vsf_fifo_get_avail_length		VSF_BASE->buffer.fifo.get_avail_length

#define vsf_bufmgr_malloc				VSF_BASE->buffer.bufmgr.malloc
#define vsf_bufmgr_malloc_aligned		VSF_BASE->buffer.bufmgr.malloc_aligned
#define vsf_bufmgr_free					VSF_BASE->buffer.bufmgr.free

#endif		// VSF_SYS

#endif		// __VSF_H_INCLUDED__
