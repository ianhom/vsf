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
	mal->op_block_size = mal->drv->block_size(mal, addr, size, VSFMAL_OP_ERASE);
	mal->offset = 0;

	while (mal->offset < size)
	{
		mal->pt.user_data = mal;
		mal->pt.state = 0;
		vsfsm_pt_entry(pt);
		err = mal->drv->erase(&mal->pt, evt, addr + mal->offset,
								mal->op_block_size);
		if (err != 0) return err;

		mal->offset += mal->op_block_size;
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
	mal->op_block_size = mal->drv->block_size(mal, addr, size, VSFMAL_OP_READ);
	mal->offset = 0;

	while (mal->offset < size)
	{
		mal->pt.user_data = mal;
		mal->pt.state = 0;
		vsfsm_pt_entry(pt);
		offset = mal->offset;
		err = mal->drv->read(&mal->pt, evt, addr + offset, buff + offset,
								mal->op_block_size);
		if (err != 0) return err;

		mal->offset += mal->op_block_size;
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
	mal->op_block_size = mal->drv->block_size(mal, addr, size, VSFMAL_OP_WRITE);
	mal->offset = 0;

	while (mal->offset < size)
	{
		mal->pt.user_data = mal;
		mal->pt.state = 0;
		vsfsm_pt_entry(pt);
		offset = mal->offset;
		err = mal->drv->write(&mal->pt, evt, addr + offset, buff + offset,
								mal->op_block_size);
		if (err != 0) return err;

		mal->offset += mal->op_block_size;
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
vsf_malstream_init_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsf_malstream_t *malstream = (struct vsf_malstream_t *)pt->user_data;
	struct vsfmal_t *mal = malstream->mal;
	vsf_err_t err;

	vsfsm_pt_begin(pt);

	mal->pt.user_data = mal;
	mal->pt.state = 0;
	vsfsm_pt_entry(pt);
	err = vsfmal_init(&mal->pt, evt);
	if (err) return err;
	malstream->mal_ready = true;

	malstream->stream.user_mem = &malstream->multibuf_stream;
	malstream->stream.op = &multibuf_stream_op;
	err = stream_init(&malstream->stream);
	if (err) return err;

	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}

vsf_err_t vsf_malstream_init(struct vsf_malstream_t *malstream)
{
	malstream->mal_ready = false;
	malstream->pt.thread = vsf_malstream_init_thread;
	malstream->pt.user_data = malstream;
	return vsfsm_pt_init(&malstream->sm, &malstream->pt);
}

static vsf_err_t
vsf_malstream_read_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsf_malstream_t *malstream = (struct vsf_malstream_t *)pt->user_data;
	struct vsfmal_t *mal = malstream->mal;
	struct vsf_stream_t *stream = &malstream->stream;
	uint64_t cur_addr;
	vsf_err_t err;

	vsfsm_pt_begin(pt);

	mal->op_block_size = mal->drv->block_size(mal, malstream->addr, 0,
												VSFMAL_OP_READ);
	if (mal->op_block_size != malstream->multibuf_stream.multibuf.size)
	{
		return VSFERR_BUG;
	}

	mal->pt.user_data = mal;
	malstream->offset = 0;
	while (malstream->offset < malstream->size)
	{
		while (stream_get_free_size(stream) < mal->op_block_size)
		{
			vsfsm_pt_wfe(pt, VSF_MALSTREAM_ON_INOUT);
		}

		mal->pt.state = 0;
		vsfsm_pt_entry(pt);
		cur_addr = malstream->addr + malstream->offset;
		err = mal->drv->read(&mal->pt, evt, cur_addr,
				vsf_multibuf_get_empty(&malstream->multibuf_stream.multibuf),
				mal->op_block_size);
		if (err > 0) return err; else if (err < 0) goto end;
		vsf_multibuf_push(&malstream->multibuf_stream.multibuf);
		malstream->offset += mal->op_block_size;
		if (malstream->offset >= malstream->size)
		{
			// fix before callback
			stream->callback_tx.on_inout = NULL;
		}
		if (stream->rx_ready && (stream->callback_rx.on_inout != NULL))
		{
			stream->callback_rx.on_inout(stream->callback_rx.param);
		}
	}

end:
	if (malstream->cb.on_finish != NULL)
	{
		malstream->cb.on_finish(malstream->cb.param);
	}

	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}

vsf_err_t vsf_malstream_read(struct vsf_malstream_t *malstream, uint64_t addr,
							uint32_t size)
{
	malstream->addr = addr;
	malstream->size = size;

	malstream->stream.callback_tx.param = malstream;
	malstream->stream.callback_tx.on_inout = vsf_malstream_on_inout;
	malstream->stream.callback_tx.on_connect = NULL;
	malstream->stream.callback_tx.on_disconnect = NULL;
	stream_connect_tx(&malstream->stream);

	malstream->pt.thread = vsf_malstream_read_thread;
	malstream->pt.user_data = malstream;
	return vsfsm_pt_init(&malstream->sm, &malstream->pt);
}

static vsf_err_t
vsf_malstream_write_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsf_malstream_t *malstream = (struct vsf_malstream_t *)pt->user_data;
	struct vsfmal_t *mal = malstream->mal;
	struct vsf_stream_t *stream = &malstream->stream;
	uint64_t cur_addr;
	vsf_err_t err;

	vsfsm_pt_begin(pt);

	mal->op_block_size = mal->drv->block_size(mal, malstream->addr, 0,
												VSFMAL_OP_WRITE);
	if (mal->op_block_size != malstream->multibuf_stream.multibuf.size)
	{
		return VSFERR_BUG;
	}

	mal->pt.user_data = mal;
	malstream->offset = 0;
	while (malstream->offset < malstream->size)
	{
		while (stream_get_data_size(stream) < mal->op_block_size)
		{
			vsfsm_pt_wfe(pt, VSF_MALSTREAM_ON_INOUT);
		}

		mal->pt.state = 0;
		vsfsm_pt_entry(pt);
		cur_addr = malstream->addr + malstream->offset;
		err = mal->drv->write(&mal->pt, evt, cur_addr,
				vsf_multibuf_get_payload(&malstream->multibuf_stream.multibuf),
				mal->op_block_size);
		if (err > 0) return err; else if (err < 0) goto end;
		vsf_multibuf_pop(&malstream->multibuf_stream.multibuf);
		malstream->offset += mal->op_block_size;
		if (malstream->offset >= malstream->size)
		{
			// fix before callback
			stream->callback_tx.on_inout = NULL;
		}
		if (stream->tx_ready && (stream->callback_tx.on_inout != NULL))
		{
			stream->callback_tx.on_inout(stream->callback_tx.param);
		}
	}

end:
	if (malstream->cb.on_finish != NULL)
	{
		malstream->cb.on_finish(malstream->cb.param);
	}

	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}

vsf_err_t vsf_malstream_write(struct vsf_malstream_t *malstream, uint64_t addr,
							uint32_t size)
{
	malstream->addr = addr;
	malstream->size = size;

	malstream->stream.callback_rx.param = malstream;
	malstream->stream.callback_rx.on_inout = vsf_malstream_on_inout;
	malstream->stream.callback_rx.on_connect = NULL;
	malstream->stream.callback_rx.on_disconnect = NULL;
	stream_connect_rx(&malstream->stream);

	malstream->pt.thread = vsf_malstream_write_thread;
	malstream->pt.user_data = malstream;
	return vsfsm_pt_init(&malstream->sm, &malstream->pt);
}
