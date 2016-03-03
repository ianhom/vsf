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
#include "tool/fakefat32/fakefat32.h"

#define VSFOSCFG_VSFSM_PENDSVQ_LEN				16
#define VSFOSCFG_VSFTIMER_NUM					16

#define VSFOSCFG_VSFIP_BUFFER_NUM				4
#define VSFOSCFG_VSFIP_SOCKET_NUM				8
#define VSFOSCFG_VSFIP_TCPPCB_NUM				8

struct vsfos_modifs_t
{
	struct app_hwcfg_t const *hwcfg;

	struct
	{
		struct vsfscsi_device_t scsi_dev;
		struct vsfscsi_lun_t lun[1];

		struct vsf_mal2scsi_t mal2scsi;
		struct vsfmal_t mal;
		struct fakefat32_param_t fakefat32;
		uint8_t *pbuffer[2];
		uint8_t buffer[2][512];
	} mal;

	struct
	{
		struct
		{
			struct vsfusbd_RNDIS_param_t param;
		} rndis;
		struct
		{
			struct vsfusbd_CDCACM_param_t param;
			struct vsf_fifostream_t stream_tx;
			struct vsf_fifostream_t stream_rx;
			uint8_t txbuff[65];
			uint8_t rxbuff[65];
		} cdc;
		struct
		{
			struct vsfusbd_MSCBOT_param_t param;
		} msc;
		struct vsfusbd_iface_t ifaces[5];
		struct vsfusbd_config_t config[1];
		struct vsfusbd_device_t device;
	} usbd;

	struct
	{
		VSFPOOL_DEFINE(buffer_pool, struct vsfip_buffer_t, VSFOSCFG_VSFIP_BUFFER_NUM);
		VSFPOOL_DEFINE(socket_pool, struct vsfip_socket_t, VSFOSCFG_VSFIP_SOCKET_NUM);
		VSFPOOL_DEFINE(tcppcb_pool, struct vsfip_tcppcb_t, VSFOSCFG_VSFIP_TCPPCB_NUM);
		uint8_t buffer_mem[VSFOSCFG_VSFIP_BUFFER_NUM][VSFIP_BUFFER_SIZE];

		struct
		{
			struct vsfip_telnetd_t telnetd;
			struct vsfip_telnetd_session_t sessions[1];

			struct vsf_fifostream_t stream_tx;
			struct vsf_fifostream_t stream_rx;
			uint8_t txbuff[65];
			uint8_t rxbuff[65];
		} telnetd;
	} tcpip;

	struct vsfshell_t shell;

	struct vsfsm_t sm;
	struct vsfsm_pt_t pt;

	VSFPOOL_DEFINE(vfsfile_pool, struct vsfile_vfsfile_t, 2);

	struct vsftimer_mem_op_t vsftimer_memop;
	VSFPOOL_DEFINE(vsftimer_pool, struct vsftimer_t, VSFOSCFG_VSFTIMER_NUM);
	struct vsfsm_evtq_t pendsvq;
	struct vsfsm_evtq_element_t pendsvq_ele[VSFOSCFG_VSFSM_PENDSVQ_LEN];
};

// event queue
static void vsfos_pendsv_activate(struct vsfsm_evtq_t *q)
{
	vsfhal_core_pendsv_trigger();
}

static void vsfos_on_pendsv(void *param)
{
	struct vsfsm_evtq_t *evtq_cur = param, *evtq_old = vsfsm_evtq_get();

	vsfsm_evtq_set(evtq_cur);
	while (vsfsm_get_event_pending())
	{
		vsfsm_poll();
	}
	vsfsm_evtq_set(evtq_old);
}

// vsftimer
static void vsfos_tickclk_callback_int(void *param)
{
	vsftimer_callback_int();
}

static struct vsftimer_t* vsftimer_memop_alloc(void)
{
	struct vsf_module_t *module = vsf_module_get("vsf.os");
	struct vsfos_modifs_t *ifs = (struct vsfos_modifs_t *)module->ifs;

	return VSFPOOL_ALLOC(&ifs->vsftimer_pool, struct vsftimer_t);
}

static void vsftimer_memop_free(struct vsftimer_t *timer)
{
	struct vsf_module_t *module = vsf_module_get("vsf.os");
	struct vsfos_modifs_t *ifs = (struct vsfos_modifs_t *)module->ifs;

	VSFPOOL_FREE(&ifs->vsftimer_pool, timer);
}

// vsfos
static vsf_err_t vsfos_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfos_modifs_t *ifs = (struct vsfos_modifs_t *)pt->user_data;

	vsfsm_pt_begin(pt);

	vsfhal_core_init(NULL);
	vsfhal_tickclk_init();
	vsfhal_tickclk_start();

	VSFPOOL_INIT(&ifs->vsftimer_pool, struct vsftimer_t, VSFOSCFG_VSFSM_PENDSVQ_LEN);
	ifs->vsftimer_memop.alloc = vsftimer_memop_alloc;
	ifs->vsftimer_memop.free = vsftimer_memop_free;
	vsftimer_init(&ifs->vsftimer_memop);
	vsfhal_tickclk_set_callback(vsfos_tickclk_callback_int, NULL);

	while (1)
	{
		vsfsm_pt_delay(pt, 1000);
		asm("nop");
		vsfsm_pt_delay(pt, 1000);
		asm("nop");
		vsfsm_pt_delay(pt, 1000);
		asm("nop");
		vsfsm_pt_delay(pt, 1000);
		asm("nop");
		vsfsm_pt_delay(pt, 1000);
		asm("nop");
		vsfsm_pt_delay(pt, 1000);
		asm("nop");
	}

	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}

void vsfos_modexit(struct vsf_module_t *module)
{
	vsf_bufmgr_free(module->ifs);
	module->ifs = NULL;
}

vsf_err_t vsfos_modinit(struct vsf_module_t *module,
								struct app_hwcfg_t const *cfg)
{
	struct vsfos_modifs_t *ifs;
	ifs = vsf_bufmgr_malloc(sizeof(struct vsfos_modifs_t));
	if (!ifs) return VSFERR_FAIL;
	memset(ifs, 0, sizeof(*ifs));

	ifs->hwcfg = cfg;

	ifs->pendsvq.size = dimof(ifs->pendsvq_ele);
	ifs->pendsvq.queue = ifs->pendsvq_ele;
	ifs->pendsvq.activate = vsfos_pendsv_activate;

	ifs->usbd.cdc.stream_tx.stream.op = &fifostream_op;
	ifs->usbd.cdc.stream_tx.mem.buffer.buffer = ifs->usbd.cdc.txbuff;
	ifs->usbd.cdc.stream_tx.mem.buffer.size = sizeof(ifs->usbd.cdc.txbuff);
	ifs->usbd.cdc.stream_rx.stream.op = &fifostream_op;
	ifs->usbd.cdc.stream_rx.mem.buffer.buffer = ifs->usbd.cdc.rxbuff;
	ifs->usbd.cdc.stream_rx.mem.buffer.size = sizeof(ifs->usbd.cdc.rxbuff);
	STREAM_INIT(&ifs->usbd.cdc.stream_rx);
	STREAM_INIT(&ifs->usbd.cdc.stream_tx);

	ifs->pt.user_data = ifs;
	ifs->pt.thread = vsfos_thread;
	module->ifs = ifs;

	vsfsm_evtq_init(&ifs->pendsvq);
	vsfsm_evtq_set(&ifs->pendsvq);
	vsfsm_pt_init(&ifs->sm, &ifs->pt);

	vsfhal_core_pendsv_config(vsfos_on_pendsv, &ifs->pendsvq);
	vsf_leave_critical();

	vsfsm_evtq_set(NULL);
	while (1)
	{
		// no thread runs in mainq, just sleep in main loop
		vsfhal_core_sleep(0);
	}
}
