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

vsf_err_t vsfmal_init(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfmal_t *mal = (struct vsfmal_t *)pt->user_data;
	return (NULL == mal->drv->init) ? VSFERR_NONE : mal->drv->init(pt, evt);
}

vsf_err_t vsfmal_fini(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfmal_t *mal = (struct vsfmal_t *)pt->user_data;
	return (NULL == mal->drv->fini) ? VSFERR_NONE : mal->drv->fini(pt, evt);
}

vsf_err_t vsfmal_erase_all(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfmal_t *mal = (struct vsfmal_t *)pt->user_data;
	return (NULL == mal->drv->erase_all) ?
				VSFERR_NOT_SUPPORT : mal->drv->erase_all(pt, evt);
}

vsf_err_t vsfmal_erase(struct vsfsm_pt_t *pt, vsfsm_evt_t evt, uint64_t addr,
					uint32_t size)
{
	struct vsfmal_t *mal = (struct vsfmal_t *)pt->user_data;
	vsf_err_t err;

	vsfsm_pt_begin(pt);
	mal->block_size = mal->drv->block_size(addr, size, VSFMAL_OP_ERASE);
	mal->offset = 0;

	while (mal->offset < size)
	{
		mal->pt.state = 0;
		vsfsm_pt_entry(pt);
		err = mal->drv->erase(&mal->pt, evt, addr + mal->offset,
								mal->block_size);
		if (err != 0) return err;

		mal->offset += mal->block_size;
	}
	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}

vsf_err_t vsfmal_read(struct vsfsm_pt_t *pt, vsfsm_evt_t evt, uint64_t addr,
					uint8_t *buff, uint32_t size)
{
	struct vsfmal_t *mal = (struct vsfmal_t *)pt->user_data;
	uint32_t offset;
	vsf_err_t err;

	vsfsm_pt_begin(pt);
	mal->block_size = mal->drv->block_size(addr, size, VSFMAL_OP_READ);
	mal->offset = 0;

	while (mal->offset < size)
	{
		mal->pt.state = 0;
		vsfsm_pt_entry(pt);
		offset = mal->offset;
		err = mal->drv->read(&mal->pt, evt, addr + offset, buff + offset,
								mal->block_size);
		if (err != 0) return err;

		mal->offset += mal->block_size;
	}
	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}

vsf_err_t vsfmal_write(struct vsfsm_pt_t *pt, vsfsm_evt_t evt, uint64_t addr,
					uint8_t *buff, uint32_t size)
{
	struct vsfmal_t *mal = (struct vsfmal_t *)pt->user_data;
	uint32_t offset;
	vsf_err_t err;

	vsfsm_pt_begin(pt);
	mal->block_size = mal->drv->block_size(addr, size, VSFMAL_OP_WRITE);
	mal->offset = 0;

	while (mal->offset < size)
	{
		mal->pt.state = 0;
		vsfsm_pt_entry(pt);
		offset = mal->offset;
		err = mal->drv->write(&mal->pt, evt, addr + offset, buff + offset,
								mal->block_size);
		if (err != 0) return err;

		mal->offset += mal->block_size;
	}
	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}

// mal stream
#define VSF_MALSTREAM_ON_INOUT			(VSFSM_EVT_USER + 0)
static void vsf_malstream_on_inout(void *param)
{
	struct vsf_malstream_t *malstream = (struct vsf_malstream_t *)param;
	vsfsm_post_evt_pending(&malstream->sm, VSF_MALSTREAM_ON_INOUT);
}

static vsf_err_t
vsf_malstream_read_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsf_malstream_t *malstream = (struct vsf_malstream_t *)pt->user_data;
	struct vsfmal_t *mal = malstream->mal;
	struct vsf_stream_t *stream = malstream->stream;
	uint64_t cur_addr;
	vsf_err_t err;

	vsfsm_pt_begin(pt);

	mal->block_size = mal->drv->block_size(malstream->addr, 0, VSFMAL_OP_READ);
	malstream->buffer.size = mal->block_size;
	while (malstream->offset < malstream->size)
	{
		mal->pt.state = 0;
		vsfsm_pt_entry(pt);
		cur_addr = malstream->addr + malstream->offset;
		err = mal->drv->read(&mal->pt, evt, cur_addr, malstream->buffer.buffer,
								mal->block_size);
		if (err > 0) return err; else if (err < 0) goto end;

		while (stream_get_free_size(stream) < mal->block_size)
		{
			vsfsm_pt_wfe(pt, VSF_MALSTREAM_ON_INOUT);
		}
		stream_write(stream, &malstream->buffer);
	}

end:
	stream_disconnect_tx(malstream->stream);
	if (malstream->on_finish != NULL)
	{
		malstream->on_finish(malstream);
	}

	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}

vsf_err_t vsf_malstream_read(struct vsf_malstream_t *malstream, uint64_t addr,
							uint32_t size)
{
	malstream->stream->callback_tx.param = malstream;
	malstream->stream->callback_tx.on_out_int = vsf_malstream_on_inout;
	malstream->stream->callback_tx.on_connect_rx = NULL;
	stream_connect_tx(malstream->stream);

	malstream->pt.thread = vsf_malstream_read_thread;
	return vsfsm_pt_init(&malstream->sm, &malstream->pt);
}

static vsf_err_t
vsf_malstream_write_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsf_malstream_t *malstream = (struct vsf_malstream_t *)pt->user_data;
	struct vsfmal_t *mal = malstream->mal;
	struct vsf_stream_t *stream = malstream->stream;
	uint64_t cur_addr;
	vsf_err_t err;

	vsfsm_pt_begin(pt);

	mal->block_size = mal->drv->block_size(malstream->addr, 0, VSFMAL_OP_WRITE);
	malstream->buffer.size = mal->block_size;
	while (malstream->offset < malstream->size)
	{
		while (stream_get_data_size(stream) < mal->block_size)
		{
			vsfsm_pt_wfe(pt, VSF_MALSTREAM_ON_INOUT);
		}

		stream_read(stream, &malstream->buffer);
		mal->pt.state = 0;
		vsfsm_pt_entry(pt);
		cur_addr = malstream->addr + malstream->offset;
		err = mal->drv->write(&mal->pt, evt, cur_addr, malstream->buffer.buffer,
								mal->block_size);
		if (err > 0) return err; else if (err < 0) goto end;
	}

end:
	stream_disconnect_tx(malstream->stream);
	if (malstream->on_finish != NULL)
	{
		malstream->on_finish(malstream);
	}

	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}

vsf_err_t vsf_malstream_write(struct vsf_malstream_t *malstream, uint64_t addr,
							uint32_t size)
{
	malstream->stream->callback_rx.param = malstream;
	malstream->stream->callback_rx.on_in_int = vsf_malstream_on_inout;
	malstream->stream->callback_rx.on_connect_tx = NULL;
	stream_connect_rx(malstream->stream);

	malstream->pt.thread = vsf_malstream_write_thread;
	return vsfsm_pt_init(&malstream->sm, &malstream->pt);
}
