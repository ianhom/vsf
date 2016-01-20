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

#include "stream.h"

vsf_err_t stream_init(struct vsf_stream_t *stream)
{
	stream->overflow = false;
	stream->tx_ready = false;
	stream->rx_ready = false;
	if (stream->op->init != NULL)
	{
		stream->op->init(stream);
	}
	return VSFERR_NONE;
}

vsf_err_t stream_fini(struct vsf_stream_t *stream)
{
	if (stream->tx_ready)
	{
		stream_disconnect_tx(stream);
	}
	if (stream->rx_ready)
	{
		stream_disconnect_rx(stream);
	}
	if (stream->op->fini != NULL)
	{
		stream->op->fini(stream);
	}
	return VSFERR_NONE;
}

uint32_t stream_read(struct vsf_stream_t *stream, struct vsf_buffer_t *buffer)
{
	uint32_t count = stream->op->read(stream, buffer);

	if ((stream->callback_tx.on_out_int != NULL) && (count > 0))
	{
		stream->callback_tx.on_out_int(stream->callback_tx.param);
	}
	return count;
}

uint32_t stream_write(struct vsf_stream_t *stream, struct vsf_buffer_t *buffer)
{
	uint32_t count = stream->op->write(stream, buffer);

	if (count < buffer->size)
	{
		stream->overflow = true;
	}
	if ((stream->callback_rx.on_in_int != NULL) && (count > 0))
	{
		stream->callback_rx.on_in_int(stream->callback_rx.param);
	}
	return count;
}

uint32_t stream_get_data_size(struct vsf_stream_t *stream)
{
	return stream->op->get_data_length(stream);
}

uint32_t stream_get_free_size(struct vsf_stream_t *stream)
{
	return stream->op->get_avail_length(stream);
}

void stream_connect_rx(struct vsf_stream_t *stream)
{
	if (!stream->rx_ready && (stream->callback_tx.on_connect_rx != NULL))
	{
		stream->callback_tx.on_connect_rx(stream->callback_tx.param);
	}
	if ((stream->tx_ready) && (stream->callback_rx.on_connect_tx != NULL))
	{
		stream->callback_rx.on_connect_tx(stream->callback_rx.param);
	}
	stream->rx_ready = true;
}

void stream_connect_tx(struct vsf_stream_t *stream)
{
	if (!stream->tx_ready && (stream->callback_rx.on_connect_tx != NULL))
	{
		stream->callback_rx.on_connect_tx(stream->callback_rx.param);
	}
	if ((stream->rx_ready) && (stream->callback_tx.on_connect_rx != NULL))
	{
		stream->callback_tx.on_connect_rx(stream->callback_tx.param);
	}
	stream->tx_ready = true;
}

void stream_disconnect_rx(struct vsf_stream_t *stream)
{
	if (stream->rx_ready && (stream->callback_tx.on_disconnect_rx != NULL))
	{
		stream->callback_tx.on_disconnect_rx(stream->callback_tx.param);
		stream->rx_ready = false;
	}
}

void stream_disconnect_tx(struct vsf_stream_t *stream)
{
	if (stream->tx_ready && (stream->callback_rx.on_disconnect_tx != NULL))
	{
		stream->callback_rx.on_disconnect_tx(stream->callback_rx.param);
		stream->tx_ready = false;
	}
}

// fifo stream
static void fifo_stream_init(struct vsf_stream_t *stream)
{
	vsf_fifo_init((struct vsf_fifo_t *)stream->user_mem);
}

static uint32_t fifo_stream_get_data_length(struct vsf_stream_t *stream)
{
	return vsf_fifo_get_data_length((struct vsf_fifo_t *)stream->user_mem);
}

static uint32_t fifo_stream_get_avail_length(struct vsf_stream_t *stream)
{
	return vsf_fifo_get_avail_length((struct vsf_fifo_t *)stream->user_mem);
}

static uint32_t
fifo_stream_write(struct vsf_stream_t *stream, struct vsf_buffer_t *buffer)
{
	return vsf_fifo_push((struct vsf_fifo_t *)stream->user_mem,
							buffer->size, buffer->buffer);
}

static uint32_t
fifo_stream_read(struct vsf_stream_t *stream, struct vsf_buffer_t *buffer)
{
	return vsf_fifo_pop((struct vsf_fifo_t *)stream->user_mem,
							buffer->size, buffer->buffer);
}

const struct vsf_stream_op_t fifo_stream_op =
{
	fifo_stream_init, fifo_stream_init,
	fifo_stream_write, fifo_stream_read,
	fifo_stream_get_data_length, fifo_stream_get_avail_length
};

// multibuf stream
static void multibuf_stream_init(struct vsf_stream_t *stream)
{
	struct vsf_multibuf_stream_t *multibuf =
							(struct vsf_multibuf_stream_t *)stream->user_mem;

	multibuf->rpos = multibuf->wpos = 0;
	vsf_multibuf_init(&multibuf->multibuf);
}

static uint32_t multibuf_stream_get_data_length(struct vsf_stream_t *stream)
{
	struct vsf_multibuf_stream_t *multibuf =
							(struct vsf_multibuf_stream_t *)stream->user_mem;

	return  (multibuf->multibuf.length * multibuf->multibuf.size)
				+ multibuf->wpos - multibuf->rpos;
}

static uint32_t multibuf_stream_get_avail_length(struct vsf_stream_t *stream)
{
	struct vsf_multibuf_stream_t *multibuf =
							(struct vsf_multibuf_stream_t *)stream->user_mem;

	return (multibuf->multibuf.count * multibuf->multibuf.size) -
								multibuf_stream_get_avail_length(stream);
}

static uint32_t
multibuf_stream_write(struct vsf_stream_t *stream, struct vsf_buffer_t *buffer)
{
	struct vsf_multibuf_stream_t *multibuf =
							(struct vsf_multibuf_stream_t *)stream->user_mem;
	uint8_t *buf = vsf_multibuf_get_empty(&multibuf->multibuf);
	uint32_t wsize = 0, cur_size, remain_size = buffer->size;

	while ((buf != NULL) && (remain_size > 0))
	{
		cur_size = multibuf->multibuf.size - multibuf->wpos;
		cur_size = min(cur_size, remain_size);
		memcpy(&buf[multibuf->wpos], &buffer->buffer[wsize], cur_size);
		wsize += cur_size;
		remain_size -= cur_size;

		multibuf->wpos += cur_size;
		if (multibuf->wpos >= multibuf->multibuf.size)
		{
			vsf_multibuf_push(&multibuf->multibuf);
			buf = vsf_multibuf_get_empty(&multibuf->multibuf);
			multibuf->wpos = 0;
		}
	}
	return wsize;
}

static uint32_t
multibuf_stream_read(struct vsf_stream_t *stream, struct vsf_buffer_t *buffer)
{
	struct vsf_multibuf_stream_t *multibuf =
							(struct vsf_multibuf_stream_t *)stream->user_mem;
	uint8_t *buf = vsf_multibuf_get_payload(&multibuf->multibuf);
	uint32_t rsize = 0, cur_size, remain_size = buffer->size;

	while ((buf != NULL) && (remain_size > 0))
	{
		cur_size = multibuf->multibuf.size - multibuf->rpos;
		cur_size = min(cur_size, remain_size);
		memcpy(&buffer->buffer[rsize], &buf[multibuf->rpos], cur_size);
		rsize += cur_size;
		remain_size -= cur_size;

		multibuf->rpos += cur_size;
		if (multibuf->rpos >= multibuf->multibuf.size)
		{
			vsf_multibuf_pop(&multibuf->multibuf);
			buf = vsf_multibuf_get_payload(&multibuf->multibuf);
			multibuf->rpos = 0;
		}
	}
	return rsize;
}

const struct vsf_stream_op_t multibuf_stream_op =
{
	multibuf_stream_init, multibuf_stream_init,
	multibuf_stream_write, multibuf_stream_read,
	multibuf_stream_get_data_length, multibuf_stream_get_avail_length
};

// buffer stream
static void buffer_stream_init(struct vsf_stream_t *stream)
{
	struct vsf_buffer_stream_t *buf =
							(struct vsf_buffer_stream_t *)stream->user_mem;
	buf->pos = 0;
}

static uint32_t buffer_stream_get_data_length(struct vsf_stream_t *stream)
{
	struct vsf_buffer_stream_t *buf =
							(struct vsf_buffer_stream_t *)stream->user_mem;
	return buf->read ? buf->buffer.size - buf->pos : 0;
}

static uint32_t buffer_stream_get_avail_length(struct vsf_stream_t *stream)
{
	struct vsf_buffer_stream_t *buf =
							(struct vsf_buffer_stream_t *)stream->user_mem;
	return buf->read ? 0 : buf->buffer.size - buf->pos;
}

static uint32_t
buffer_stream_write(struct vsf_stream_t *stream, struct vsf_buffer_t *buffer)
{
	struct vsf_buffer_stream_t *buf =
							(struct vsf_buffer_stream_t *)stream->user_mem;
	uint32_t wsize = 0;

	if (!buf->read)
	{
		uint32_t avail_len = buffer_stream_get_avail_length(stream);
		wsize = min(avail_len, buffer->size);
		memcpy(buf->buffer.buffer + buf->pos, buffer->buffer, wsize);
		buf->pos += wsize;
	}
	return wsize;
}

static uint32_t
buffer_stream_read(struct vsf_stream_t *stream, struct vsf_buffer_t *buffer)
{
	struct vsf_buffer_stream_t *buf =
							(struct vsf_buffer_stream_t *)stream->user_mem;
	uint32_t rsize = 0;

	if (buf->read)
	{
		uint32_t data_len = buffer_stream_get_data_length(stream);
		rsize = min(data_len, buffer->size);
		memcpy(buffer->buffer, buf->buffer.buffer + buf->pos, rsize);
		buf->pos += rsize;
	}
	return rsize;
}

const struct vsf_stream_op_t buffer_stream_op =
{
	buffer_stream_init, buffer_stream_init,
	buffer_stream_write, buffer_stream_read,
	buffer_stream_get_data_length, buffer_stream_get_avail_length
};
