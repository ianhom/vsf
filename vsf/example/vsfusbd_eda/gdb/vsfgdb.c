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
#include "vsfgdb.h"

#define VSFGDB_EVT_RSP_PACKET				VSFSM_EVT_USER

static uint8_t hex2u8(char hex)
{
	if ((hex >= 'a') && (hex <= 'f'))
	{
		return hex - 'a' + 10;
	}
	else if ((hex >= 'A') && (hex <= 'F'))
	{
		return hex - 'A' + 10;
	}
	else if ((hex >= '0') && (hex <= '9'))
	{
		return hex - '0';
	}
	return 0;
}

const char hextbl[] = "0123456789ABCDEF";
static char u82hex(uint8_t u8)
{
	return hextbl[u8 & 0x0F];
}

static void vsfgdb_on_session_input(void *param, struct vsfip_buffer_t *buf)
{
	struct vsfgdb_t *gdb = (struct vsfgdb_t *)param;
	char *ptr = (char *)buf->app.buffer;
	uint32_t size = buf->app.size;

	if (!gdb->idle)
	{
		goto end;
	}

	while (size && ('\0' == gdb->dollar))
	{
		if ('$' == *ptr++)
		{
			gdb->dollar = '$';
			gdb->checksum = 0;
			gdb->checksum_calc = 0;
			gdb->checksum_size = 0;
			gdb->rsptbuf.position = 0;
		}
		size--;
	}

	while (size && ('#' != *ptr))
	{
		gdb->checksum_calc += *ptr;
		gdb->rsptbuf.buffer.buffer[gdb->rsptbuf.position] = *ptr++;
		size--;
	}
	while (size && (gdb->checksum_size < 2))
	{
		gdb->checksum_size++;
		gdb->checksum += hex2u8(*ptr++) << ((2 - gdb->checksum_size) << 2);
		if (2 == gdb->checksum_size)
		{
			struct vsfip_socket_t *so = gdb->session;

			buf->app.size = 1;
			if (gdb->checksum == gdb->checksum_calc)
			{
				buf->app.buffer[0] = '+';
				if (!vsfip_tcp_async_send(so, &so->remote_sockaddr, buf))
				{
					gdb->idle = false;
					vsfsm_post_evt(&gdb->sm, VSFGDB_EVT_RSP_PACKET);
				}
			}
			else
			{
				buf->app.buffer[0] = '-';
				vsfip_tcp_async_send(so, &so->remote_sockaddr, buf);
			}
			break;
		}
	}
end:
	vsfip_buffer_release(buf);
}

static vsf_err_t vsfgdb_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfgdb_t *gdb = (struct vsfgdb_t *)pt->user_data;
	vsf_err_t err;

	vsfsm_pt_begin(pt);

	// prepare socket
	gdb->so = vsfip_socket(AF_INET, IPPROTO_TCP);
	if (NULL == gdb->so)
	{
		gdb->errcode = VSFERR_NOT_ENOUGH_RESOURCES;
		goto fail_socket;
	}
	if ((vsfip_bind(gdb->so, gdb->port) != 0) ||
		(vsfip_listen(gdb->so, 1) != 0))
	{
		gdb->errcode = VSFERR_FAIL;
		goto fail_socket_connect;
	}

	gdb->caller_pt.state = 0;
	vsfsm_pt_entry(pt);
	err = vsfip_tcp_accept(&gdb->caller_pt, evt, gdb->so, &gdb->session);
	if (err > 0) return err; else if (err < 0)
	{
		gdb->errcode = VSFERR_FAIL;
		goto fail_socket_connect;
	}
	vsfip_socket_cb(gdb->session, gdb, vsfgdb_on_session_input, NULL);

	// target init, this will also read target information
	gdb->caller_pt.user_data = &gdb->target;
	gdb->caller_pt.state = 0;
	vsfsm_pt_entry(pt);
	err = gdb->target.op->init(&gdb->caller_pt, evt);
	if (err > 0) return err; else if (err < 0)
	{
		gdb->errcode = VSFERR_FAIL;
		goto fail_target_init;
	}

	// handle RSP
	while (!gdb->exit)
	{
		// wait for rsp packet
		vsfsm_pt_wfe(pt, VSFGDB_EVT_RSP_PACKET);

		gdb->outbuf = VSFIP_TCPBUF_GET(VSFIP_CFG_TCP_MSS);
		if (NULL == gdb->outbuf)
		{
			gdb->errcode = VSFERR_NOT_ENOUGH_RESOURCES;
			goto fail_connected;
		}
		gdb->outbuf->app.buffer[0] = '$';
		gdb->outbuf->app.size = 1;

		// rsp parser
		gdb->cmd = (char)*gdb->rsptbuf.buffer.buffer;
		if ('?' == gdb->cmd)
		{
		}
		else if ('q' == gdb->cmd)
		{
		}
		else if ('D' == gdb->cmd)
		{
		}
		else if ('g' == gdb->cmd)
		{
		}
		else if ('G' == gdb->cmd)
		{
		}
		else if ('p' == gdb->cmd)
		{
		}
		else if ('P' == gdb->cmd)
		{
		}
		else if ('m' == gdb->cmd)
		{
		}
		else if ('M' == gdb->cmd)
		{
		}
		else if ('z' == gdb->cmd)
		{
		}
		else if ('Z' == gdb->cmd)
		{
		}
		else if ('c' == gdb->cmd)
		{
		}
		else if ('C' == gdb->cmd)
		{
		}
		else if ('s' == gdb->cmd)
		{
		}
		else if ('S' == gdb->cmd)
		{
		}
		else if ('k' == gdb->cmd)
		{
		}
		else if ('v' == gdb->cmd)
		{
		}

		gdb->dollar = gdb->cmd = '\0';
		gdb->idle = true;
		// reply
		{
			uint8_t *ptr = &gdb->outbuf->app.buffer[gdb->outbuf->app.size];
			struct vsfip_socket_t *so = gdb->session;
			uint32_t i;

			gdb->checksum_calc = 0;
			for (i = 1; i < gdb->outbuf->app.size; i++)
			{
				gdb->checksum_calc += gdb->outbuf->app.buffer[i];
			}
			*ptr++ = '#';
			*ptr++ = u82hex(gdb->checksum_calc >> 4);
			*ptr++ = u82hex(gdb->checksum_calc >> 0);
			gdb->outbuf->app.size += 3;
			vsfip_tcp_async_send(so, &so->remote_sockaddr, gdb->outbuf);
		}
	}

	return VSFERR_NONE;

fail_connected:
	gdb->caller_pt.user_data = &gdb->target;
	gdb->caller_pt.state = 0;
	vsfsm_pt_entry(pt);
	err = gdb->target.op->fini(&gdb->caller_pt, evt);
	if (err > 0) return err;
fail_target_init:
	gdb->caller_pt.state = 0;
	vsfsm_pt_entry(pt);
	err = vsfip_tcp_close(&gdb->caller_pt, evt, gdb->so);
	if (err > 0) return err;
fail_socket_connect:
	vsfip_close(gdb->so);
	gdb->so = NULL;
fail_socket:
	return gdb->errcode;

	vsfsm_pt_end(pt);
	return VSFERR_NONE;
}

vsf_err_t vsfgdb_start(struct vsfgdb_t *gdb)
{
	gdb->idle = true;
	gdb->dollar = gdb->cmd = '\0';
	gdb->exit = false;
	gdb->errcode = VSFERR_NONE;
	gdb->target.regs = (struct vsfgdb_reg_t *)\
		vsf_bufmgr_malloc(gdb->target.regnum * sizeof(struct vsfgdb_reg_t));

	gdb->pt.thread = vsfgdb_thread;
	gdb->pt.user_data = gdb;
	return vsfsm_pt_init(&gdb->sm, &gdb->pt);
}
