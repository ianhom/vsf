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
	const struct interfaces_info_t *ifs;
	
	struct vsf_framework_t
	{
		vsf_err_t (*sm_init)(struct vsfsm_t *sm);
		vsf_err_t (*pt_init)(struct vsfsm_t *sm, struct vsfsm_pt_t *pt);
		vsf_err_t (*post_evt)(struct vsfsm_t *sm, vsfsm_evt_t evt);
		vsf_err_t (*post_evt_pending)(struct vsfsm_t *sm, vsfsm_evt_t evt);
		
		void (*enter_critical)(void);
		void (*leave_critical)(void);
		
		vsf_err_t (*sem_init)(struct vsfsm_sem_t *sem, uint32_t cnt,
								vsfsm_evt_t evt);
		vsf_err_t (*sem_post)(struct vsfsm_sem_t *sem);
		vsf_err_t (*sem_pend)(struct vsfsm_sem_t *sem, struct vsfsm_t *sm);
		
		vsf_err_t (*crit_init)(struct vsfsm_crit_t *crit, vsfsm_evt_t evt);
		vsf_err_t (*crit_enter)(struct vsfsm_crit_t *crit, struct vsfsm_t *sm);
		vsf_err_t (*crit_leave)(struct vsfsm_crit_t *crit);
		
		struct
		{
			vsf_err_t (*add)(struct vsftimer_timer_t *timer);
			vsf_err_t (*remove)(struct vsftimer_timer_t *timer);
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

extern const struct vsf_t vsf;

#endif		// __VSF_H_INCLUDED__
