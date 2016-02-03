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

#ifndef __VSFUSBD_CDCACM_H_INCLUDED__
#define __VSFUSBD_CDCACM_H_INCLUDED__

struct vsfusbd_CDCACM_line_coding_t
{
	uint32_t bitrate;
	uint8_t stopbittype;
	uint8_t paritytype;
	uint8_t datatype;
};

#define USBCDCACM_CONTROLLINE_RTS			0x02
#define USBCDCACM_CONTROLLINE_DTR			0x01
#define USBCDCACM_CONTROLLINE_MASK			0x03

enum usb_CDCACM_req_t
{
	USB_CDCACMREQ_SET_LINE_CODING			= 0x20,
	USB_CDCACMREQ_GET_LINE_CODING			= 0x21,
	USB_CDCACMREQ_SET_CONTROL_LINE_STATE	= 0x22,
	USB_CDCACMREQ_SEND_BREAK				= 0x23,
};

#ifndef VSFCFG_STANDALONE_MODULE
extern const struct vsfusbd_class_protocol_t vsfusbd_CDCACMControl_class;
extern const struct vsfusbd_class_protocol_t vsfusbd_CDCACMData_class;
#endif

struct vsfusbd_CDCACM_param_t
{
	struct vsfusbd_CDC_param_t CDC_param;
	
	struct
	{
		vsf_err_t (*set_line_coding)(struct vsfusbd_CDCACM_line_coding_t *line_coding);
		vsf_err_t (*set_control_line)(uint8_t control_line);
		vsf_err_t (*get_control_line)(uint8_t *control_line);
		vsf_err_t (*send_break)(void);
	} callback;
	
	struct vsfusbd_CDCACM_line_coding_t line_coding;
	
	// no need to initialize below by user
	uint8_t control_line;
	uint8_t line_coding_buffer[7];
};

#endif	// __VSFUSBD_CDCACM_H_INCLUDED__
