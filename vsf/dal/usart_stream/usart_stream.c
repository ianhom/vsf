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
#include "usart_stream.h"

#define USART_STREAM_BUF_SIZE	16

static void usart_stream_ontx_int(void *p)
{
	uint8_t buf[USART_STREAM_BUF_SIZE];
	struct vsf_buffer_t buffer;
	struct usart_stream_info_t *usart_stream = (struct usart_stream_info_t *)p;

	buffer.size = min(vsfhal_usart_tx_get_free_size(usart_stream->index),
			USART_STREAM_BUF_SIZE);
	if (buffer.size)
	{
		buffer.buffer = buf;
		buffer.size = stream_read(&usart_stream->stream_tx, &buffer);
		if (buffer.size)
		{
			vsfhal_usart_tx_bytes(usart_stream->index, buf, buffer.size);
		}
	}
}

static void usart_stream_onrx_int(void *p, uint16_t data)
{
	uint8_t buf[USART_STREAM_BUF_SIZE];
	struct vsf_buffer_t buffer;
	struct usart_stream_info_t *usart_stream = (struct usart_stream_info_t *)p;

	buffer.buffer = buf;
	buf[0] = data & 0xff;
	buffer.size = vsfhal_usart_rx_get_data_size(usart_stream->index);
	if (buffer.size)
	{
		buffer.size = min(buffer.size, USART_STREAM_BUF_SIZE - 1);
		buffer.size = vsfhal_usart_rx_bytes(usart_stream->index, &buf[1], buffer.size);
	}
	buffer.size++;
	stream_write(&usart_stream->stream_rx, &buffer);
}

vsf_err_t usart_stream_init(struct usart_stream_info_t *usart_stream)
{
	stream_init(&usart_stream->stream_tx);
	stream_init(&usart_stream->stream_rx);
	
	usart_stream->stream_tx.callback_rx.on_in_int = usart_stream_ontx_int;
	usart_stream->stream_tx.callback_rx.param = usart_stream;
	usart_stream->stream_tx.tx_ready = true;
	usart_stream->stream_tx.rx_ready = true;
	usart_stream->stream_rx.callback_tx.on_out_int = NULL;
	usart_stream->stream_tx.callback_tx.param = usart_stream;
	usart_stream->stream_rx.tx_ready = true;
	usart_stream->stream_rx.rx_ready = true;
	
	if (usart_stream->index != IFS_DUMMY_PORT)
	{
		vsfhal_usart_init(usart_stream->index);
		vsfhal_usart_config_callback(usart_stream->index,
				usart_stream, usart_stream_ontx_int, usart_stream_onrx_int);
		vsfhal_usart_config(usart_stream->index, usart_stream->baudrate,
				usart_stream->mode);
	}
	return VSFERR_NONE;
}

vsf_err_t usart_stream_fini(struct usart_stream_info_t *usart_stream)
{
	if (usart_stream->index != IFS_DUMMY_PORT)
	{
		vsfhal_usart_config_callback(usart_stream->index, NULL, NULL, NULL);
		vsfhal_usart_fini(usart_stream->index);
	}
	return VSFERR_NONE;
}

