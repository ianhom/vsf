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
#include "app_cfg.h"
#include "app_type.h"

#include "interfaces.h"
#include "tool/list/list.h"
#include "tool/buffer/buffer.h"

#include "stack/usb/core/vsfusbh.h"
#include "vsfohci_priv.h"

#define OHCI_ISO_DELAY			4

#define CC_TO_ERROR(cc) (cc == 0 ? VSFERR_NONE : VSFERR_FAIL)

#define usb_maxpacket(dev, pipe, out) (out \
							? (dev)->epmaxpacketout[usb_pipeendpoint(pipe)] \
							: (dev)->epmaxpacketin [usb_pipeendpoint(pipe)] )

static struct td_t *td_alloc(struct vsfohci_t *vsfohci)
{
	uint32_t i;
	struct td_t *td = NULL;
	for (i = 0; i < TD_MAX_NUM; i++)
	{
		if (vsfohci->td_pool[i].busy == 0)
		{
			td = &vsfohci->td_pool[i];
			td->busy = 1;
			break;
		}
	}
	return td;
}
static void td_free(struct td_t *td)
{
	memset(td, 0, sizeof(struct td_t));
}

static struct ed_t *ed_alloc(struct vsfohci_t *vsfohci)
{
	uint32_t i;
	struct ed_t *ed = NULL;
	for (i = 0; i < ED_MAX_NUM; i++)
	{
		if (vsfohci->ed_pool[i].busy == 0)
		{
			ed = &vsfohci->ed_pool[i];
			memset(ed, 0, sizeof(struct ed_t));
			ed->busy = 1;
			break;
		}
	}
	return ed;
}
static void ed_free(struct ed_t *ed)
{
	ed->busy = 0;
}

static void urb_free_priv(struct urb_priv_t *urb_priv)
{
	uint32_t i;
	for (i = 0; i < urb_priv->length; i++)
	{
		if (urb_priv->td[i] != NULL)
		{
			td_free(urb_priv->td[i]);
		}
	}
	if (urb_priv->extra_buf != NULL)
		vsf_bufmgr_free(urb_priv->extra_buf);
	vsf_bufmgr_free(urb_priv);
}


static uint8_t ep_int_ballance(struct ohci_t *ohci, uint8_t interval,
	uint8_t load)
{
	uint8_t i, branch = 0;
	for (i = 0; i < 32; i++)
		if (ohci->ohci_int_load[branch] > ohci->ohci_int_load[i])
			branch = i;
	branch = branch % interval;

	for (i = branch; i < 32; i += interval)
		ohci->ohci_int_load[i] += load;
	return branch;
}
static uint8_t ep_2_n_interval(uint8_t inter)
{
	uint8_t i;
	for (i = 0; ((inter >> i) > 1) && (i < 5); i++);
	return 1 << i;
}
static uint8_t ep_rev(uint8_t num_bits, uint8_t word)
{
	uint8_t i, wout = 0;
	for (i = 0; i < num_bits; i++)
		wout |= (((word >> i) & 1) << (num_bits - i - 1));
	return wout;
}


static void ep_link(struct ohci_t *ohci, struct ed_t *edi)
{
	uint32_t inter, i;
	uint32_t *ed_p;
	volatile struct ed_t *ed;

	ed = edi;

	ed->state = ED_OPER;
	switch (ed->type)
	{
	case PIPE_CONTROL:
		ed->hwNextED = 0;
		if (ohci->ed_controltail == NULL)
			ohci->regs->ed_controlhead = (uint32_t)ed;
		else
			ohci->ed_controltail->hwNextED = (uint32_t)ed;
		ed->prev = ohci->ed_controltail;
		if (!ohci->ed_controltail &&
			!ohci->ed_rm_list[0] &&
			!ohci->ed_rm_list[1])
		{
			ohci->hc_control |= OHCI_CTRL_CLE;
			ohci->regs->control = ohci->hc_control;
		}
		ohci->ed_controltail = edi;
		break;
	case PIPE_BULK:
		ed->hwNextED = 0;
		if (ohci->ed_bulktail == NULL)
			ohci->regs->ed_bulkhead = (uint32_t)ed;
		else
			ohci->ed_bulktail->hwNextED = (uint32_t)ed;
		ed->prev = ohci->ed_bulktail;
		if (!ohci->ed_bulktail &&
			!ohci->ed_rm_list[0] &&
			!ohci->ed_rm_list[1])
		{
			ohci->hc_control |= OHCI_CTRL_BLE;
			ohci->regs->control = ohci->hc_control;
		}
		ohci->ed_bulktail = edi;
		break;
	case PIPE_INTERRUPT:
	{
		uint8_t load, interval, int_branch;
		load = ed->int_load;
		interval = ep_2_n_interval(ed->int_period);
		ed->int_interval = interval;
		int_branch = ep_int_ballance(ohci, interval, load);
		for (i = 0; i < ep_rev(6, interval); i += inter)
		{
			inter = 1;
			for (ed_p = &(ohci->hcca->int_table[ep_rev(5, i) + int_branch]);
				(*ed_p != 0) && (((struct ed_t *)(*ed_p))->int_interval >= interval);
				ed_p = &(((struct ed_t *)(*ed_p))->hwNextED))
			{
				inter = ep_rev(6, ((struct ed_t *)(*ed_p))->int_interval);
			}
			ed->hwNextED = *ed_p;
			*ed_p = (uint32_t)ed;
		}
	}
		break;
	case PIPE_ISOCHRONOUS:
		ed->hwNextED = 0;
		ed->int_interval = 1;
		if (ohci->ed_isotail != NULL)
		{
			ohci->ed_isotail->hwNextED = (uint32_t)ed;
			ed->prev = ohci->ed_isotail;
		}
		else
		{
			for (i = 0; i < 32; i+= inter)
			{
				inter = 1;
				for (ed_p = &(ohci->hcca->int_table[ep_rev(5, i)]);
						(*ed_p != 0) && ed_p;
						ed_p = &(((struct ed_t *)(*ed_p))->hwNextED))
				{
					inter = ep_rev(6, ((struct ed_t *)(*ed_p))->int_interval);
				}
				*ed_p = (uint32_t)ed;
			}
			ed->prev = NULL;
		}
		ohci->ed_isotail = edi;
		break;
	}
}

static void ep_unlink(struct ohci_t *ohci, struct ed_t *ed)
{
	uint8_t i;
	uint32_t *edp;

	ed->hwINFO |= OHCI_ED_SKIP;
	switch (ed->type)
	{
	case PIPE_CONTROL:
		if (ed->prev == NULL)
		{
			if (!ed->hwNextED)
			{
				ohci->hc_control &= ~OHCI_CTRL_CLE;
				ohci->regs->control = ohci->hc_control;
			}
			ohci->regs->ed_controlhead = ed->hwNextED;
		}
		else
		{
			ed->prev->hwNextED = ed->hwNextED;
		}
		if (ohci->ed_controltail == ed)
			ohci->ed_controltail = ed->prev;
		else
			((struct ed_t *)(ed->hwNextED))->prev = ed->prev;
		break;
	case PIPE_BULK:
		if (ed->prev == NULL)
		{
			if (!ed->hwNextED)
			{
				ohci->hc_control &= ~OHCI_CTRL_BLE;
				ohci->regs->control = ohci->hc_control;
			}
			ohci->regs->ed_bulkhead = ed->hwNextED;
		}
		else
		{
			ed->prev->hwNextED = ed->hwNextED;
		}

		if (ohci->ed_bulktail == ed)
			ohci->ed_bulktail = ed->prev;
		else
			((struct ed_t *)(ed->hwNextED))->prev = ed->prev;
		break;
	case PIPE_INTERRUPT:
		for (i = 0; i < 32; i++)
		{
			for (edp = &ohci->hcca->int_table[i]; *edp != 0;
				edp = (uint32_t *)&(((struct ed_t *)(*edp))->hwNextED))
			{
				if (edp == (uint32_t *)&(((struct ed_t *)(*edp))->hwNextED))
					break;
				if ((struct ed_t *)(*edp) == ed)
				{
					*edp = ((struct ed_t *)(*edp))->hwNextED;
					break;
				}
			}
		}
		break;
	case PIPE_ISOCHRONOUS:
		if (ed == ohci->ed_isotail)
			ohci->ed_isotail = ed->prev;
		if (ed->hwNextED != 0)
			((struct ed_t *)(ed->hwNextED))->prev = ed->prev;
		if (ed->prev != NULL)
			ed->prev->hwNextED = ed->hwNextED;
		break;
	}
	ed->state = ED_UNLINK;
}

static struct ed_t *ep_add_ed(struct vsfohci_t *vsfohci,
struct vsfusbh_device_t *vsfdev, uint32_t pipe,
	uint8_t interval, uint8_t load)
{
	uint32_t i, ep_num, is_in;
	struct ohci_device_t *ohci_device;
	struct ed_t *ed = NULL;
	struct td_t *td = NULL;

	ohci_device = vsfdev->priv;
	ep_num = usb_pipeendpoint(pipe);
	is_in = (pipe & 0x80) ? 1 : 0;

	for (i = 0; i < MAX_EP_NUM_EACH_DEVICE; i++)
	{
		if ((ohci_device->ed[i] != NULL) &&
			(((ohci_device->ed[i]->hwINFO >> 7) & 0xf) == ep_num) &&
			(((ohci_device->in_flag >> i) & 0x1) == is_in))
		{
			ed = ohci_device->ed[i];
			break;
		}
	}

	if (ed == NULL)
	{
		for (i = 0; i < MAX_EP_NUM_EACH_DEVICE; i++)
		{
			if (ohci_device->ed[i] == NULL)
			{
				ohci_device->ed[i] = ed_alloc(vsfohci);
				if (ohci_device->ed[i] == NULL)
					return NULL;
				ohci_device->in_flag &= ~(0x1ul << i);
				ohci_device->in_flag |= is_in << i;
				ohci_device->ed_cnt++;
				ed = ohci_device->ed[i];
				break;
			}
		}
	}

	if (ed == NULL)
		return NULL;

	if ((ed->state & ED_DEL) || (ed->state & ED_URB_DEL))
		return NULL;

	if (ed->state == ED_NEW)
	{
		ed->hwINFO = OHCI_ED_SKIP;
		td = td_alloc(vsfohci);
		if (!td)
			return NULL;
		ed->hwTailP = (uint32_t)td;
		ed->hwHeadP = ed->hwTailP;
		ed->state = ED_UNLINK;
		ed->type = usb_pipetype(pipe);
	}

	ed->vsfdev = vsfdev;

	ed->hwINFO = usb_pipedevice(pipe)
		| usb_pipeendpoint(pipe) << 7
		| (usb_pipeisoc(pipe) ? 0x8000 : 0)
		| (usb_pipecontrol(pipe) ? 0 : (usb_pipeout(pipe) ? 0x800 : 0x1000))
		| usb_pipeslow(pipe) << 13
		| usb_maxpacket(vsfdev, pipe, usb_pipeout(pipe)) << 16;

	if ((ed->type == PIPE_INTERRUPT) && (ed->state == ED_UNLINK))
	{
		ed->int_period = interval;
		ed->int_load = load;
	}
	return ed;
}

static void ep_rm_ed(struct vsfohci_t *vsfohci, struct ed_t *ed)
{
	uint16_t frame;
	struct ohci_t *ohci = vsfohci->ohci;

	if ((ed->state & ED_DEL) || (ed->state & ED_URB_DEL))
		return;

	ed->hwINFO |= OHCI_ED_SKIP;

	if (!ohci->disabled)
	{
		switch (ed->type)
		{
		case PIPE_CONTROL:
			ohci->hc_control &= ~OHCI_CTRL_CLE;
			ohci->regs->control = ohci->hc_control;
			break;
		case PIPE_BULK:
			ohci->hc_control &= ~OHCI_CTRL_BLE;
			ohci->regs->control = ohci->hc_control;
			break;
		}
	}
	frame = ohci->hcca->frame_no & 0x1;
	ed->ed_rm_list = ohci->ed_rm_list[frame];
	ohci->ed_rm_list[frame] = ed;

	if (!ohci->disabled)
	{
		ohci->regs->intrstatus = OHCI_INTR_SF;
		ohci->regs->intrenable = OHCI_INTR_SF;
	}
}

static void td_fill(uint32_t info, void *data, uint16_t len, uint16_t index,
		struct urb_priv_t *urb_priv)
{
	struct td_t *td, *td_pt;

	if (index > urb_priv->length)
		return;

	td_pt = urb_priv->td[index];

	// fill the old dummy TD
	td = urb_priv->td[index] = (struct td_t *)\
		((uint32_t)urb_priv->ed->hwTailP & 0xfffffff0);
	td->ed = urb_priv->ed;
	td->next_dl_td = NULL;
	td->index = index;
	td->urb_priv = urb_priv;

	td->hwINFO = info;
	td->hwCBP = (uint32_t)((!data || !len) ? 0 : data);
	td->hwBE = (uint32_t)((!data || !len) ? 0 : (uint32_t)data + len - 1);
	td->hwNextTD = (uint32_t)td_pt;
	td->is_iso = 0;

	td_pt->hwNextTD = 0;
	td->ed->hwTailP = td->hwNextTD;
}
#if USBH_CFG_ENABLE_ISO
static void iso_td_fill(uint32_t info, void *data, uint16_t len, uint16_t index,
		struct urb_priv_t *urb_priv)
{
	struct td_t *td, *td_pt;
	uint32_t bufferStart;
	struct vsfusbh_urb_t *vsfurb = urb_priv->vsfurb;

	if (index > urb_priv->length)
		return;

	td_pt = urb_priv->td[index];

	// fill the old dummy TD
	td = urb_priv->td[index] = (struct td_t *)\
		((uint32_t)urb_priv->ed->hwTailP & 0xfffffff0);
	td->ed = urb_priv->ed;
	td->ed->last_iso = info & 0xffff;
	td->next_dl_td = NULL;
	td->index = index;
	td->urb_priv = urb_priv;

	bufferStart = (uint32_t)data + vsfurb->iso_frame_desc[index].offset;
	len = vsfurb->iso_frame_desc[index].length;

	td->hwINFO = info;
	td->hwCBP = (uint32_t)((!bufferStart || !len) ? 0 : bufferStart) & 0xfffff000;
	td->hwBE = (uint32_t)((!bufferStart || !len) ? 0 : (uint32_t)bufferStart + len - 1);
	td->hwNextTD = (uint32_t)td_pt;
	td->is_iso = 1;

	td->hwPSW[0] = ((uint32_t)data + vsfurb->iso_frame_desc[index].offset) & 0x0FFF | 0xE000;

	td_pt->hwNextTD = 0;
	td->ed->hwTailP = td->hwNextTD;
}
#endif // USBH_CFG_ENABLE_ISO

static void td_submit_urb(struct ohci_t *ohci, struct vsfusbh_urb_t *vsfurb)
{
	uint32_t data_len, info = 0, toggle = 0, cnt = 0;
	void *data;
	struct urb_priv_t *urb_priv = vsfurb->urb_priv;

	data = vsfurb->transfer_buffer;
	data_len = vsfurb->transfer_length;

	if (usb_gettoggle(vsfurb->vsfdev, usb_pipeendpoint(vsfurb->pipe), usb_pipeout(vsfurb->pipe)))
	{
		toggle = TD_T_TOGGLE;
	}
	else
	{
		toggle = TD_T_DATA0;
		usb_settoggle(vsfurb->vsfdev, usb_pipeendpoint(vsfurb->pipe), usb_pipeout(vsfurb->pipe), 1);
	}
	urb_priv->td_cnt = 0;

	switch (usb_pipetype(vsfurb->pipe))
	{
	case PIPE_CONTROL:
		info = TD_CC | TD_DP_SETUP | TD_T_DATA0;
		td_fill(info, (void *)&vsfurb->setup_packet, 8, cnt++, urb_priv);
		if (data_len > 0)
		{
			info = usb_pipeout(vsfurb->pipe) ?
				TD_CC | TD_R | TD_DP_OUT | TD_T_DATA1 :
				TD_CC | TD_R | TD_DP_IN | TD_T_DATA1;
			td_fill(info, data, data_len, cnt++, urb_priv);
		}
		info = usb_pipeout(vsfurb->pipe) ?
			(TD_CC | TD_DP_IN | TD_T_DATA1) :
			(TD_CC | TD_DP_OUT | TD_T_DATA1);
		td_fill(info, NULL, 0, cnt++, urb_priv);
		ohci->regs->cmdstatus = OHCI_CLF;
		break;
	case PIPE_BULK:
		info = usb_pipeout(vsfurb->pipe) ? (TD_CC | TD_DP_OUT) : (TD_CC | TD_DP_IN);
		while (data_len > 4096)
		{
			td_fill(info | (cnt ? TD_T_TOGGLE : toggle), data, 4096, cnt, urb_priv);
			data = (void *)((uint32_t)data + 4096);
			data_len -= 4096;
			cnt++;
		}
		info = usb_pipeout(vsfurb->pipe) ?
					(TD_CC | TD_DP_OUT) : (TD_CC | TD_R | TD_DP_IN);
		td_fill(info | (cnt ? TD_T_TOGGLE : toggle), data, data_len, cnt, urb_priv);
		cnt++;
		ohci->regs->cmdstatus = OHCI_BLF;
		break;
	case PIPE_INTERRUPT:
		info = usb_pipeout(vsfurb->pipe) ? (TD_CC | TD_DP_OUT | toggle) :
			(TD_CC | TD_R | TD_DP_IN | toggle);
		td_fill(info, data, data_len, cnt++, urb_priv);
		break;
#if USBH_CFG_ENABLE_ISO
	case PIPE_ISOCHRONOUS:
		for (cnt = 0; cnt < vsfurb->number_of_packets; cnt++)
		{
			iso_td_fill(TD_CC | TD_ISO | ((vsfurb->start_frame + cnt) & 0xffff),
					data, data_len, cnt, urb_priv);
		}
		break;
#endif // USBH_CFG_ENABLE_ISO
	}
}

static void dl_transfer_length(struct vsfusbh_urb_t *vsfurb, struct td_t *td)
{
	struct urb_priv_t *urb_priv = vsfurb->urb_priv;

#if USBH_CFG_ENABLE_ISO
	if (td->is_iso)
	{
		uint16_t cc, tdPSW;
		uint32_t dlen;

		tdPSW = td->hwPSW[0] & 0xffff;
		cc = (tdPSW >> 12) & 0xf;
		if (cc < 0xe)
		{
			if (usb_pipeout(vsfurb->pipe))
				dlen = vsfurb->iso_frame_desc[td->index].length;
			else
				dlen = tdPSW & 0x3ff;
			vsfurb->actual_length += dlen;
			vsfurb->iso_frame_desc[td->index].actual_length = dlen;
			if ((!vsfurb->transfer_flags & USB_DISABLE_SPD) &&
					(cc == TD_DATAUNDERRUN))
			{
				cc = TD_CC_NOERROR;
			}
			vsfurb->iso_frame_desc[td->index].status = CC_TO_ERROR(cc);
		}
	}
	else
#endif // USBH_CFG_ENABLE_ISO
	{
		if (!(usb_pipetype(vsfurb->pipe) == PIPE_CONTROL &&
			((td->index == 0) || (td->index == urb_priv->length - 1))))
		{
			if (td->hwBE != 0)
			{
				if (td->hwCBP == 0)
					vsfurb->actual_length = td->hwBE -
					(uint32_t)vsfurb->transfer_buffer + 1;
				else
					vsfurb->actual_length = td->hwCBP -
					(uint32_t)vsfurb->transfer_buffer;
			}
		}
	}
}

static struct td_t *dl_reverse_done_list(struct ohci_t *ohci)
{
	struct td_t *td_prev = NULL;
	struct td_t *td_next = NULL;
	struct td_t *td = NULL;

	td_next = (struct td_t *)(ohci->hcca->done_head & 0xfffffff0);
	ohci->hcca->done_head = 0;

	while (td_next)
	{
		td = td_next;

		// error check
		if (TD_CC_GET(td->hwINFO))
		{
			if (td->ed->hwHeadP & 0x1)
			{
				struct urb_priv_t *urb_priv = td->urb_priv;

				if ((td->index + 1) < urb_priv->length)
				{
					td->ed->hwHeadP = urb_priv->td[urb_priv->length - 1]->hwNextTD & 0xfffffff0 |
							(td->ed->hwHeadP & 0x02);
					urb_priv->td_cnt += urb_priv->length - td->index - 1;
				}
				else
				{
					td->ed->hwHeadP &= 0xfffffff2;
				}
			}
		}

		/*
		if ((td->ed->type == PIPE_ISOCHRONOUS) &&
				(td->hwPSW[0] >> 12) &&
				((td->hwPSW[0] >> 12) != TD_DATAUNDERRUN) &&
				((td->hwPSW[0] >> 12) != TD_NOTACCESSED))
		{
			// error report
		}
		*/

		td->next_dl_td = td_prev;
		td_prev = td;
		td_next = (struct td_t *)(td->hwNextTD & 0xfffffff0);
	}
	return td;
}

static void dl_del_urb(struct vsfusbh_urb_t *vsfurb)
{
	if (vsfurb->transfer_flags & USB_ASYNC_UNLINK)
	{
		vsfurb->status = URB_FAIL;
		// vsfsm_post_evt_pending(vsfurb->sm, VSFSM_EVT_URB_REMOVE);
	}
	else
	{
		vsfurb->status = URB_FAIL;
	}
}

static void sohci_return_urb(struct ohci_t *ohci, struct vsfusbh_urb_t *vsfurb)
{
	struct urb_priv_t *urb_priv = vsfurb->urb_priv;

	switch (usb_pipetype(vsfurb->pipe))
	{
	case PIPE_INTERRUPT:
	case PIPE_ISOCHRONOUS:
		vsfsm_post_evt_pending(vsfurb->sm, VSFSM_EVT_URB_COMPLETE);
		break;
	case PIPE_BULK:
	case PIPE_CONTROL:
		urb_free_priv(urb_priv);
		vsfsm_post_evt_pending(vsfurb->sm, VSFSM_EVT_URB_COMPLETE);
		break;
	}
}

static void dl_done_list(struct ohci_t *ohci)
{
	uint8_t cc;
	struct td_t *td_next;
	struct td_t *td = dl_reverse_done_list(ohci);
	struct vsfusbh_urb_t *vsfurb;
	struct urb_priv_t *urb_priv;
	struct ed_t *ed;

	while (td)
	{
		td_next = td->next_dl_td;

		urb_priv = td->urb_priv;
		ed = td->ed;

		if (urb_priv->state == URB_PRIV_DEL)
		{
			if (++(urb_priv->td_cnt) == urb_priv->length)
				urb_free_priv(urb_priv);
		}
		else
		{
			vsfurb = urb_priv->vsfurb;

			dl_transfer_length(vsfurb, td);
			cc = TD_CC_GET(td->hwINFO);
			if (cc == TD_CC_STALL)
			{
				usb_endpoint_halt(vsfurb->vsfdev, usb_pipeendpoint(vsfurb->pipe),
					usb_pipeout(vsfurb->pipe));
			}
			if (!(vsfurb->transfer_flags & USB_DISABLE_SPD) && (cc == TD_DATAUNDERRUN))
				cc = TD_CC_NOERROR;

			if (++(urb_priv->td_cnt) == urb_priv->length)
			{
				if ((ed->state & (ED_OPER | ED_UNLINK)) &&
					(urb_priv->state != URB_PRIV_DEL))
				{
					vsfurb->status = CC_TO_ERROR(cc);
					sohci_return_urb(ohci, vsfurb);
				}
				else
				{
					dl_del_urb(vsfurb);
				}
			}
		}

		if ((ed->state == ED_OPER) &&
			((ed->hwHeadP & 0xfffffff0) == ed->hwTailP) &&
			((ed->type == PIPE_CONTROL) || (ed->type == PIPE_BULK)))
		{
			ep_unlink(ohci, ed);
		}

		td = td_next;
	}
}

static void dl_del_list(struct ohci_t *ohci, uint16_t frame)
{
	struct ed_t *ed;
	struct td_t *td = NULL, *td_next = NULL, *tdHeadP = NULL, *tdTailP = NULL;
	uint32_t *td_p;
	struct ohci_device_t *ohci_device;
	uint8_t ctrl = 0, bulk = 0;

	for (ed = ohci->ed_rm_list[frame]; ed != NULL; ed = ed->ed_rm_list)
	{
		tdTailP = (struct td_t *)((uint32_t)(ed->hwTailP) & 0xfffffff0);
		tdHeadP = (struct td_t *)((uint32_t)(ed->hwHeadP) & 0xfffffff0);
		td_p = &ed->hwHeadP;

		for (td = tdHeadP; td != tdTailP; td = td_next)
		{
			struct urb_priv_t *urb_priv = td->urb_priv;

			td_next = (struct td_t *)((uint32_t)(td->hwNextTD) & 0xfffffff0);
			if ((urb_priv->state == URB_PRIV_DEL) || (ed->state & ED_DEL))
			{
				//tdINFO = td->hwINFO;
				//if(TD_CC_GET(tdINFO) < 0xE)
				//	dl_transfer_length(td);
				*td_p = td->hwNextTD | (*td_p & 0x3);

				if (++(urb_priv->td_cnt) == urb_priv->length)
					urb_free_priv(urb_priv);
			}
			else
			{
				td_p = &td->hwNextTD;
			}
		}

		ohci_device = ed->vsfdev->priv;
		if (ed->state & ED_DEL)
		{
			td_free(tdTailP);
			ed->hwINFO = OHCI_ED_SKIP;
			ed->state = ED_NEW;
			ohci_device->ed_cnt--;
		}
		else
		{
			ed->state &= ~ED_URB_DEL;
			tdHeadP = (struct td_t *)((uint32_t)(ed->hwHeadP) & 0xfffffff0);
			if (tdHeadP == tdTailP)
			{
				if (ed->state == ED_OPER)
					ep_unlink(ohci, ed);
				td_free(tdTailP);
				ed->hwINFO = OHCI_ED_SKIP;
				ed->state = ED_NEW;
				ohci_device->ed_cnt--;
			}
			else
				ed->hwINFO &= ~OHCI_ED_SKIP;
		}

		switch (ed->type)
		{
		case PIPE_CONTROL:
			ctrl = 1;
			break;
		case PIPE_BULK:
			bulk = 1;
			break;
		}
	}

	if (!ohci->disabled)
	{
		if (ctrl)
			ohci->regs->ed_controlcurrent = 0;
		if (bulk)
			ohci->regs->ed_bulkcurrent = 0;
		if (!ohci->ed_rm_list[!frame])
		{
			if (ohci->ed_controltail)
				ohci->hc_control |= OHCI_CTRL_CLE;
			if (ohci->ed_bulktail)
				ohci->hc_control |= OHCI_CTRL_BLE;
			ohci->regs->control = ohci->hc_control;
		}
	}
	ohci->ed_rm_list[frame] = NULL;
}

static int32_t vsfohci_interrupt(void *param)
{
	struct vsfohci_t *vsfohci = (struct vsfohci_t *)param;
	struct ohci_t *ohci = vsfohci->ohci;
	uint32_t intrstatus = ohci->regs->intrstatus;

	if ((ohci->hcca->done_head != 0) && !(ohci->hcca->done_head & 0x1))
		intrstatus = OHCI_INTR_WDH;

	if (intrstatus & OHCI_INTR_UE)
	{
		ohci->disabled++;
		// ohci_reset();
		vsfohci->ohci->regs->intrdisable = OHCI_INTR_MIE;
		vsfohci->ohci->regs->control = 0;
		vsfohci->ohci->regs->cmdstatus = OHCI_HCR;
		// NO delay
		// while((vsfohci->ohci->regs->cmdstatus & OHCI_HCR) != 0);
		vsfohci->ohci->hc_control = OHCI_USB_RESET;

		return -1;
	}

	if (intrstatus & OHCI_INTR_OC)
	{
		// output info
	}

	if (intrstatus & OHCI_INTR_WDH)
	{
		dl_done_list(ohci);
		ohci->regs->intrstatus = OHCI_INTR_WDH;
	}

	if (intrstatus & OHCI_INTR_SO)
	{
		ohci->regs->intrdisable = OHCI_INTR_SO;
		ohci->regs->intrstatus = OHCI_INTR_SO;
	}

	if (intrstatus & OHCI_INTR_SF)
	{
		uint32_t frame = ohci->hcca->frame_no & 0x1;
		ohci->regs->intrdisable = OHCI_INTR_SF;
		if (ohci->ed_rm_list[!frame] != NULL)
			dl_del_list(ohci, !frame);
		if (ohci->ed_rm_list[frame] != NULL)
			ohci->regs->intrenable = OHCI_INTR_SF;
	}

	if (intrstatus & OHCI_INTR_RHSC)
	{
		ohci->regs->intrdisable = OHCI_INTR_RHSC;
		ohci->regs->intrstatus = OHCI_INTR_RHSC;
	}

	if (intrstatus & OHCI_INTR_RD)
	{
		ohci->regs->intrdisable = OHCI_INTR_RD;
		ohci->regs->intrstatus = OHCI_INTR_RD;
	}

	ohci->regs->intrenable = OHCI_INTR_MIE;

	return 0;
}

static vsf_err_t vsfohci_init_get_resource(struct vsfusbh_t *usbh,
	uint32_t reg_base)
{
	struct vsfohci_t *vsfohci;

	vsfohci = vsf_bufmgr_malloc(sizeof(struct vsfohci_t));
	if (vsfohci == NULL)
		return VSFERR_NOT_ENOUGH_RESOURCES;
	memset(vsfohci, 0, sizeof(struct vsfohci_t));
	usbh->hcd_data = vsfohci;

	vsfohci->ohci = vsf_bufmgr_malloc(sizeof(struct ohci_t));
	if (vsfohci->ohci == NULL)
		goto err_failed_alloc_ohci;
	memset(vsfohci->ohci, 0, sizeof(struct ohci_t));
	vsfohci->ohci->vsfohci = vsfohci;

	vsfohci->ohci->hcca = vsf_bufmgr_malloc_aligned(sizeof(struct ohci_hcca_t),
		256);
	if (vsfohci->ohci->hcca == NULL)
		goto err_failed_alloc_hcca;
	memset(vsfohci->ohci->hcca, 0, sizeof(struct ohci_hcca_t));

	vsfohci->ed_pool = vsf_bufmgr_malloc_aligned(sizeof(struct ed_t) *\
		ED_MAX_NUM, 16);
	if (vsfohci->ed_pool == NULL)
		goto err_failed_alloc_ohci_ed;
	memset(vsfohci->ed_pool, 0, sizeof(struct ed_t) * ED_MAX_NUM);

	vsfohci->td_pool = vsf_bufmgr_malloc_aligned(sizeof(struct td_t) *\
		TD_MAX_NUM, 32);
	if (vsfohci->td_pool == NULL)
		goto err_failed_alloc_ohci_td;
	memset(vsfohci->td_pool, 0, sizeof(struct td_t) * TD_MAX_NUM);

	__asm("NOP");

	vsfohci->ohci->regs = (struct ohci_regs_t *)reg_base;
	return VSFERR_NONE;

err_failed_alloc_ohci_td:
	vsf_bufmgr_free(vsfohci->ed_pool);
err_failed_alloc_ohci_ed:
	vsf_bufmgr_free(vsfohci->ohci->hcca);
err_failed_alloc_hcca:
	vsf_bufmgr_free(vsfohci->ohci);
err_failed_alloc_ohci:
	vsf_bufmgr_free(vsfohci);
	usbh->hcd_data = NULL;
	return VSFERR_NOT_ENOUGH_RESOURCES;
}

static uint32_t vsfohci_init_hc_start(struct vsfohci_t *vsfohci)
{
	uint32_t temp;
	struct ohci_t *ohci = vsfohci->ohci;

	ohci->disabled = 1;

	ohci->regs->ed_controlhead = 0;
	ohci->regs->ed_bulkhead = 0;

	ohci->regs->hcca = (uint32_t)ohci->hcca;

	ohci->hc_control = OHCI_CONTROL_INIT | OHCI_USB_OPER;
	ohci->disabled = 0;
	ohci->regs->control = ohci->hc_control;

	ohci->regs->fminterval = 0x2edf | (((0x2edf - 210) * 6 / 7) << 16);
	ohci->regs->periodicstart = (0x2edf * 9) / 10;
	ohci->regs->lsthresh = 0x628;

	temp = OHCI_INTR_MIE | OHCI_INTR_UE | OHCI_INTR_WDH | OHCI_INTR_SO;
	ohci->regs->intrstatus = temp;
	ohci->regs->intrenable = temp;

	temp = ohci->regs->roothub.a & ~(RH_A_PSM | RH_A_OCPM);
	ohci->regs->roothub.a = temp;
	ohci->regs->roothub.status = RH_HS_LPSC;

	return (ohci->regs->roothub.a >> 23) & 0x1fe;
}

static vsf_err_t vsfohci_init_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	vsf_err_t err;
	struct vsfohci_t *vsfohci;
	struct vsfusbh_t *usbh = (struct vsfusbh_t *)pt->user_data;
	vsfohci = (struct vsfohci_t *)usbh->hcd_data;

	vsfsm_pt_begin(pt);

	err = vsfohci_init_get_resource(usbh,
			(uint32_t)core_interfaces.hcd.regbase(usbh->hcd_index));
	if (err)
		return err;
	vsfohci = (struct vsfohci_t *)usbh->hcd_data;

	core_interfaces.hcd.init(usbh->hcd_index, vsfohci_interrupt, usbh->hcd_data);

	vsfohci->ohci->regs->intrdisable = OHCI_INTR_MIE;
	vsfohci->ohci->regs->control = 0;
	vsfohci->ohci->regs->cmdstatus = OHCI_HCR;
	vsfohci->loops = 30;
	while ((vsfohci->ohci->regs->cmdstatus & OHCI_HCR) != 0)
	{
		if (--vsfohci->loops == 0)
		{
			return VSFERR_FAIL;
		}
		vsfsm_pt_delay(pt, 10);
	}

	vsfohci->ohci->hc_control = OHCI_USB_RESET;

	vsfsm_pt_delay(pt, 100);

	vsfsm_pt_delay(pt, vsfohci_init_hc_start(vsfohci));

	vsfsm_pt_end(pt);

	return VSFERR_NONE;
}

static vsf_err_t vsfohci_fini(void *param)
{
	return VSFERR_NONE;
}

static vsf_err_t vsfohci_suspend(void *param)
{
	return VSFERR_NONE;
}

static vsf_err_t vsfohci_resume(void *param)
{
	return VSFERR_NONE;
}

static vsf_err_t vsfohci_alloc_device(void *param, struct vsfusbh_device_t *dev)
{
	dev->priv = vsf_bufmgr_malloc(sizeof(struct ohci_device_t));
	if (dev->priv == NULL)
		return VSFERR_FAIL;
	memset(dev->priv, 0, sizeof(struct ohci_device_t));
	return VSFERR_NONE;
}

static vsf_err_t vsfohci_free_device(void *param, struct vsfusbh_device_t *dev)
{
	uint16_t i;
	struct ed_t *ed;
	struct vsfohci_t *vsfohci = param;
	struct ohci_device_t *ohci_device = (struct ohci_device_t *)dev->priv;

	if ((vsfohci == NULL) || (ohci_device == NULL))
		return VSFERR_NONE;
	else if (ohci_device->ed_cnt != 0)
	{
		for (i = 0; i < MAX_EP_NUM_EACH_DEVICE; i++)
		{
			ed = ohci_device->ed[i];
			if ((ed != NULL) && (ed->state != ED_NEW))
			{
				if (ed->state == ED_OPER)
				{
					ep_unlink(vsfohci->ohci, ed);
				}
				ep_rm_ed(vsfohci, ed);
				ed_free(ed);
				ed->state = ED_DEL;
			}
		}
	}
	vsf_bufmgr_free(dev->priv);
	dev->priv = NULL;
	return VSFERR_NONE;
}

static vsf_err_t vsfohci_submit_urb(void *param, struct vsfusbh_urb_t *vsfurb)
{
	struct ed_t *ed;
	uint32_t i, pipe, size = 0;
	struct urb_priv_t *urb_priv;
	struct vsfohci_t *vsfohci = (struct vsfohci_t *)param;
	struct ohci_t *ohci = vsfohci->ohci;

	pipe = vsfurb->pipe;

	if (usb_endpoint_halted(vsfurb->vsfdev, usb_pipeendpoint(pipe), usb_pipeout(pipe)))
		return VSFERR_FAIL;

	// rh address check
	if (usb_pipedevice(pipe) == 1)
		return VSFERR_FAIL;

	if (ohci->disabled)
		return VSFERR_FAIL;

	ed = ep_add_ed(vsfohci, vsfurb->vsfdev, pipe, vsfurb->interval, 1);
	if (ed == NULL)
		return VSFERR_FAIL;

	switch (usb_pipetype(pipe))
	{
	case PIPE_CONTROL:/* 1 TD for setup, 1 for ACK and 1 for every 4096 B */
		size = (vsfurb->transfer_length == 0) ?
					2 : (vsfurb->transfer_length - 1) / 4096 + 3;
		break;
	case PIPE_BULK:
		size = (vsfurb->transfer_length - 1) / 4096 + 1;
		break;
	case PIPE_INTERRUPT:
		size = 1;
		break;
#if USBH_CFG_ENABLE_ISO
	case PIPE_ISOCHRONOUS:
		size = vsfurb->number_of_packets;
		if (size == 0)
			return VSFERR_FAIL;
		for (i = 0; i < size; i++)
		{
			vsfurb->iso_frame_desc[i].actual_length = 0;
			vsfurb->iso_frame_desc[i].status = URB_XDEV;
		}
		break;
#endif // USBH_CFG_ENABLE_ISO
	}

	if (size > TD_MAX_NUM_EACH_UARB)
		return VSFERR_FAIL;

	urb_priv = vsf_bufmgr_malloc(sizeof(struct urb_priv_t));
	if (urb_priv == NULL)
		return VSFERR_FAIL;
	memset(urb_priv, 0, sizeof(struct urb_priv_t));

	for (i = 0; i < size; i++)
	{
		urb_priv->td[i] = td_alloc(vsfohci);
		if (NULL == urb_priv->td[i])
		{
			urb_free_priv(urb_priv);
			return VSFERR_FAIL;
		}
	}

	urb_priv->ed = ed;
	urb_priv->length = size;

#if USBH_CFG_ENABLE_ISO
	if (usb_pipetype(pipe) == PIPE_ISOCHRONOUS)
	{
		if (vsfurb->transfer_flags & USB_ISO_ASAP)
		{
			vsfurb->start_frame = ((ed->state == ED_OPER) ?
									(ed->last_iso + 1 + OHCI_ISO_DELAY) :
									(ohci->hcca->frame_no + OHCI_ISO_DELAY))
									& 0xffff;
		}
	}
#endif // USBH_CFG_ENABLE_ISO

	vsfurb->actual_length = 0;
	vsfurb->status = URB_PENDING;

	vsfurb->urb_priv = urb_priv;
	urb_priv->vsfurb = vsfurb;

	if (ed->state != ED_OPER)
		ep_link(ohci, ed);

	td_submit_urb(ohci, vsfurb);
	return VSFERR_NONE;
}

/*
example_1:
err = vsfohci_unlink_urb (vsfohci, vsfurb, NULL);
if (err == 0) // app can free all resource now!
{
	free(vsfurb->transfer_buffer);
	free(vsfurb->setup_packet);
	free(vsfurb);
	return VSFERR_NONE;
}
else
	err_deal(err);

example_2: // only one pointer can be free by ohci driver
err = vsfohci_unlink_urb (vsfohci, vsfurb, vsfurb->transfer_buffer);
if (err == 0) // app can free all resource now!
{
	free(vsfurb->transfer_buffer);
	free(vsfurb);
	return VSFERR_NONE;
}
// ohci driver will free vsfurb->transfer_buffer next frame!
else if (err == VSFERR_NOT_READY)
{
	free(vsfurb);
	return VSFERR_NONE;
}
else
	err_deal();
*/
static vsf_err_t vsfohci_unlink_urb(void *param, struct vsfusbh_urb_t *vsfurb,
	void *delay_free_buf)
{
	struct vsfohci_t *vsfohci = (struct vsfohci_t *)param;
	struct ohci_t *ohci = vsfohci->ohci;

	if (vsfurb == NULL)
		return VSFERR_INVALID_PARAMETER;

	// rh address check
	if (usb_pipedevice(vsfurb->pipe) == 1)
		return VSFERR_FAIL;

	if (vsfurb->status == URB_PENDING)
	{
		if (ohci->disabled)
		{
			urb_free_priv(vsfurb->urb_priv);
			//if (vsfurb->transfer_flags & USB_ASYNC_UNLINK)
			//{
			//	vsfurb->status = URB_FAIL;
			//}
			//else
			//	vsfurb->status = URB_FAIL;
			vsfurb->status = URB_FAIL;
		}
		else
		{
			if (vsfurb->urb_priv->state == URB_PRIV_DEL)
				return VSFERR_NOT_SUPPORT;

			vsfurb->urb_priv->state = URB_PRIV_DEL;

			ep_rm_ed(vsfohci, vsfurb->urb_priv->ed);
			vsfurb->urb_priv->ed->state |= ED_URB_DEL;

			if (!(vsfurb->transfer_flags & USB_ASYNC_UNLINK))
			{
				vsfurb->urb_priv->extra_buf = delay_free_buf;
				return VSFERR_NOT_READY;
			}
			else
			{
				vsfurb->status = URB_PENDING;
				return VSFERR_FAIL;
			}
		}
	}
	return VSFERR_NONE;
}

// use for int/iso urb
static vsf_err_t vsfohci_relink_urb(void *param, struct vsfusbh_urb_t *vsfurb)
{
	struct vsfohci_t *vsfohci = (struct vsfohci_t *)param;
	struct ohci_t *ohci = vsfohci->ohci;
	struct urb_priv_t *urb_priv = vsfurb->urb_priv;

	switch (usb_pipetype(vsfurb->pipe))
	{
	case PIPE_INTERRUPT:
		vsfurb->actual_length = 0;
		if ((urb_priv->state != URB_PRIV_DEL) && (vsfurb->status == URB_OK))
		{
			vsfurb->status = URB_PENDING;
			td_submit_urb(ohci, vsfurb);
			return VSFERR_NONE;
		}
		break;
	case PIPE_ISOCHRONOUS:
		vsfurb->actual_length = 0;
		if ((urb_priv->state != URB_PRIV_DEL) && (vsfurb->status == URB_OK))
		{
			uint8_t i;
			vsfurb->status = URB_PENDING;
			vsfurb->start_frame = (ohci->hcca->frame_no + 1) & 0xffff;
			for (i = 0; i < vsfurb->number_of_packets; i++)
			{
				vsfurb->iso_frame_desc[i].actual_length = 0;
				vsfurb->iso_frame_desc[i].status = URB_XDEV;
			}
			td_submit_urb(ohci, vsfurb);
			return VSFERR_NONE;
		}
	default:
		break;
	}
	return VSFERR_FAIL;
}

const static uint8_t root_hub_str_index0[] =
{
	0x04,				/* u8  bLength; */
	0x03,				/* u8  bDescriptorType; String-descriptor */
	0x09,				/* u8  lang ID */
	0x04,				/* u8  lang ID */
};
const static uint8_t root_hub_str_index1[] =
{
	28,					/* u8  bLength; */
	0x03,				/* u8  bDescriptorType; String-descriptor */
	'O',				/* u8  Unicode */
	0,					/* u8  Unicode */
	'H',				/* u8  Unicode */
	0,					/* u8  Unicode */
	'C',				/* u8  Unicode */
	0,					/* u8  Unicode */
	'I',				/* u8  Unicode */
	0,					/* u8  Unicode */
	' ',				/* u8  Unicode */
	0,					/* u8  Unicode */
	'R',				/* u8  Unicode */
	0,					/* u8  Unicode */
	'o',				/* u8  Unicode */
	0,					/* u8  Unicode */
	'o',				/* u8  Unicode */
	0,					/* u8  Unicode */
	't',				/* u8  Unicode */
	0,					/* u8  Unicode */
	' ',				/* u8  Unicode */
	0,					/* u8  Unicode */
	'H',				/* u8  Unicode */
	0,					/* u8  Unicode */
	'u',				/* u8  Unicode */
	0,					/* u8  Unicode */
	'b',				/* u8  Unicode */
	0,					/* u8  Unicode */
};

static vsf_err_t vsfohci_roothub_control(void *param,
		struct vsfusbh_urb_t *vsfurb)
{
	uint16_t typeReq, wValue, wIndex, wLength;
	struct vsfohci_t *vsfohci = (struct vsfohci_t *)param;
	struct ohci_regs_t *regs = vsfohci->ohci->regs;
	struct usb_ctrlrequest_t *cmd = &vsfurb->setup_packet;
	uint32_t datadw[4], temp;
	uint8_t *data = (uint8_t*)datadw;
	uint8_t len = 0;


	typeReq = (cmd->bRequestType << 8) | cmd->bRequest;
	wValue = cmd->wValue;
	wIndex = cmd->wIndex;
	wLength = cmd->wLength;

	switch (typeReq)
	{
	case GetHubStatus:
		datadw[0] = RD_RH_STAT;
		len = 4;
		break;
	case GetPortStatus:
		datadw[0] = RD_RH_PORTSTAT;
		len = 4;
		break;
	case SetPortFeature:
		switch (wValue)
		{
		case(RH_PORT_SUSPEND):
			WR_RH_PORTSTAT(RH_PS_PSS);
			len = 0;
			break;
		case(RH_PORT_RESET):	/* BUG IN HUP CODE *********/
			if (RD_RH_PORTSTAT & RH_PS_CCS)
				WR_RH_PORTSTAT(RH_PS_PRS);
			len = 0;
			break;
		case(RH_PORT_POWER):
			WR_RH_PORTSTAT(RH_PS_PPS);
			len = 0;
			break;
		case(RH_PORT_ENABLE):	/* BUG IN HUP CODE *********/
			if (RD_RH_PORTSTAT & RH_PS_CCS)
				WR_RH_PORTSTAT(RH_PS_PES);
			len = 0;
			break;
		default:
			goto error;
		}
		break;
	case ClearPortFeature:
		switch (wValue)
		{
		case(RH_PORT_ENABLE):
			WR_RH_PORTSTAT(RH_PS_CCS);
			len = 0;
			break;
		case(RH_PORT_SUSPEND):
			WR_RH_PORTSTAT(RH_PS_POCI);
			len = 0;
			break;
		case(RH_PORT_POWER):
			WR_RH_PORTSTAT(RH_PS_LSDA);
			len = 0;
			break;
		case(RH_C_PORT_CONNECTION):
			WR_RH_PORTSTAT(RH_PS_CSC);
			len = 0;
			break;
		case(RH_C_PORT_ENABLE):
			WR_RH_PORTSTAT(RH_PS_PESC);
			len = 0;
			break;
		case(RH_C_PORT_SUSPEND):
			WR_RH_PORTSTAT(RH_PS_PSSC);
			len = 0;
			break;
		case(RH_C_PORT_OVER_CURRENT):
			WR_RH_PORTSTAT(RH_PS_OCIC);
			len = 0;
			break;
		case(RH_C_PORT_RESET):
			WR_RH_PORTSTAT(RH_PS_PRSC);
			len = 0;
			break;
		default:
			goto error;
		}
		break;
	case GetHubDescriptor:
		temp = regs->roothub.a;
		data[0] = 9;			// min length;
		data[1] = 0x29;
		data[2] = temp & RH_A_NDP;
		data[3] = 0;
		if (temp & RH_A_PSM)		/* per-port power switching? */
			data[3] |= 0x1;
		if (temp & RH_A_NOCP)		/* no over current reporting? */
			data[3] |= 0x10;
		else if (temp & RH_A_OCPM)	/* per-port over current reporting? */
			data[3] |= 0x8;
		datadw[1] = 0;
		data[5] = (temp & RH_A_POTPGT) >> 24;
		temp = regs->roothub.b;
		data[7] = temp & RH_B_DR;
		if (data[2] < 7)
		{
			data[8] = 0xff;
		}
		else
		{
			data[0] += 2;
			data[8] = (temp & RH_B_DR) >> 8;
			data[10] = data[9] = 0xff;
		}
		len = min(data[0], wLength);
		break;
	case DeviceRequest | USB_REQ_GET_DESCRIPTOR:
		switch (wValue & 0xff00)
		{
		case USB_DT_STRING << 8:
			if (wValue == 0x0300)
			{
				data = (uint8_t *)root_hub_str_index0;
				len = min(sizeof(root_hub_str_index0), wLength);
			}
			if (wValue == 0x0301)
			{
				data = (uint8_t *)root_hub_str_index1;
				len = min(sizeof(root_hub_str_index0), wLength);
			}
			break;
		default:
			goto error;
		}
		break;
	default:
		goto error;
	}

	if (len)
	{
		if (vsfurb->transfer_length < len)
			len = vsfurb->transfer_length;
		vsfurb->actual_length = len;

		memcpy (vsfurb->transfer_buffer, data, len);
	}
	vsfurb->status = URB_OK;

	vsfsm_post_evt_pending(vsfurb->sm, VSFSM_EVT_URB_COMPLETE);
	return VSFERR_NONE;

error:
	vsfurb->status = URB_FAIL;
	return VSFERR_FAIL;
}

const struct vsfusbh_hcddrv_t ohci_drv =
{
	vsfohci_init_thread,
	vsfohci_fini,
	vsfohci_suspend,
	vsfohci_resume,
	vsfohci_alloc_device,
	vsfohci_free_device,
	vsfohci_submit_urb,
	vsfohci_unlink_urb,
	vsfohci_relink_urb,
	vsfohci_roothub_control,
};
