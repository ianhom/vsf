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

static vsf_err_t vsfsm_sem_init_internal(struct vsfsm_sem_t *sem, uint32_t cnt,
											vsfsm_evt_t evt)
{
	return vsfsm_sync_init(sem, cnt, 0xFFFFFFFF, evt);
}
static vsf_err_t vsfsm_sem_post_internal(struct vsfsm_sem_t *sem)
{
	return vsfsm_sync_increase(sem);
}
static vsf_err_t vsfsm_sem_pend_internal(struct vsfsm_sem_t *sem,
											struct vsfsm_t *sm)
{
	return vsfsm_sync_decrease(sem, sm);
}
static vsf_err_t vsfsm_crit_init_internal(struct vsfsm_crit_t *crit,
											vsfsm_evt_t evt)
{
	return vsfsm_sync_init(crit, 1, 1, evt);
}
static vsf_err_t vsfsm_crit_enter_internal(struct vsfsm_crit_t *crit,
											struct vsfsm_t *sm)
{
	return vsfsm_sync_decrease(crit, sm);
}
static vsf_err_t vsfsm_crit_leave_internal(struct vsfsm_crit_t *crit)
{
	return vsfsm_sync_increase(crit);
}
static void vsfsm_enter_critical_internal(void)
{
	vsf_enter_critical();
}
static void vsfsm_leave_critical_internal(void)
{
	vsf_leave_critical();
}

const struct vsf_t vsf =
{
	&core_interfaces,
	
	{
		vsfsm_init,
		vsfsm_pt_init,
		vsfsm_post_evt,
		vsfsm_post_evt_pending,
		vsfsm_enter_critical_internal,
		vsfsm_leave_critical_internal,
		vsfsm_sem_init_internal,
		vsfsm_sem_post_internal,
		vsfsm_sem_pend_internal,
		vsfsm_crit_init_internal,
		vsfsm_crit_enter_internal,
		vsfsm_crit_leave_internal,
		vsftimer_register,
		vsftimer_unregister,
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
			vsf_bufmgr_free,
		},					// struct bufmgr;
	},						// struct buffer;
};
