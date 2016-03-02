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
#ifndef __VSFIP_HTTPD_H_INCLUDED__
#define __VSFIP_HTTPD_H_INCLUDED__

//#define HTTPD_DEBUG
enum vsfip_httpd_service_req_t
{
	VSFIP_HTTP_POST,
	VSFIP_HTTP_GET,
};

enum vsfip_httpd_service_rsp_t
{
	VSFIP_HTTP_200_OK = 200,
	VSFIP_HTTP_404_NOTFOUND = 404,
};

enum vsfip_httpd_post_type
{
	VSFIP_HTTPD_TYPE_NONE = 0,
	VSFIP_HTTPD_TYPE_XWWW,
	VSFIP_HTTPD_TYPE_MUTIFORM,
	VSFIP_HTTPD_TYPE_HTML,
	VSFIP_HTTPD_TYPE_JPG,
	VSFIP_HTTPD_TYPE_TXT,
	VSFIP_HTTPD_TYPE_XML,
	VSFIP_HTTPD_TYPE_JS,
	VSFIP_HTTPD_TYPE_UNKNOW = 0xFF,
};

struct vsfip_post_t
{
	uint32_t size;
	uint8_t *buf;
	uint8_t type;
};

struct vsfip_httpd_posttarget_t
{
	char *name;
	vsf_err_t (*ondat)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt, uint8_t type,
				char *buf, uint32_t size, char **rspfilename);
	void *ondatparam;
};

struct vsfip_httpd_service_t
{
	struct vsfsm_t sm;
	struct vsfsm_pt_t pt;
	struct vsfsm_pt_t socket_pt;
	struct vsfsm_pt_t local_pt;
	struct vsfsm_pt_t file_pt;

	struct vsfip_post_t post;

	char *targetfilename;
	struct vsfile_t *targetfile;
	struct vsfile_t *root;
	struct vsfip_httpd_posttarget_t *posttarget;
	struct vsfip_httpd_posttarget_t *postroot;

	uint32_t fileoffset;

	struct vsfip_socket_t *so;

	struct vsfip_buffer_t *inbuf;
	struct vsfip_buffer_t *outbuf;

	struct vsfip_httpd_cb_t *cb;

	uint16_t req;
	uint16_t rsp;

#ifdef HTTPD_DEBUG
	struct vsfsm_pt_t output_pt;
#endif
};

struct vsfip_httpd_cb_t
{
	vsf_err_t (*onca)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
				char *reqfilename, struct vsfip_ipaddr_t *useripaddr,
				char **redirectfilename);
	void *oncaparam;
};

struct vsfip_httpd_t
{
	struct vsfsm_t sm;
	struct vsfsm_pt_t pt;
	struct vsfsm_pt_t daemon_pt;

	struct vsfip_httpd_service_t *service;
	uint32_t maxconnection;

	struct vsfip_socket_t *so;
	struct vsfip_socket_t *acceptso;

	struct vsfip_sockaddr_t sockaddr;

	struct vsfile_t *root;
	struct vsfip_httpd_posttarget_t *postroot;

	struct vsfip_httpd_cb_t cb;

	bool isactive;

#ifdef HTTPD_DEBUG
	struct vsfsm_pt_t *output_pt;
#endif
};

#define VSFIP_HTTPD_SUPPORTTYPECNT			8
struct vsfip_http_contenttype_t
{
	uint8_t type;
	char *str;
	char *ext;
};

#ifdef VSFCFG_STANDALONE_MODULE
#define VSFIP_HTTPD_MODNAME					"vsf.stack.net.tcpip.proto.httpd"

struct vsfip_httpd_modifs_t
{
	vsf_err_t (*start)(struct vsfip_httpd_t*, struct vsfip_httpd_service_t*,
						uint32_t, uint16_t, struct vsfile_t*);
	char* (*getpostvaluebyname)(char*, char*, uint32_t*);

	struct vsfip_http_contenttype_t supporttype[VSFIP_HTTPD_SUPPORTTYPECNT];
	struct vsfile_memfile_t http400;
	struct vsfile_memfile_t http404;
};

void vsfip_httpd_modexit(struct vsf_module_t*);
vsf_err_t vsfip_httpd_modinit(struct vsf_module_t*, struct app_hwcfg_t const*);

#define VSFIP_HTTPDMOD						\
	((struct vsfip_httpd_modifs_t *)vsf_module_get(VSFIP_HTTPD_MODNAME))
#define vsfip_httpd_start					VSFIP_HTTPDMOD->start
#define vsfip_http_getpostvaluebyname		VSFIP_HTTPDMOD->getpostvaluebyname
#define vsfip_http400						VSFIP_HTTPDMOD->http400
#define vsfip_http404						VSFIP_HTTPDMOD->http404
#define vsfip_httpd_supporttype				VSFIP_HTTPDMOD->supporttype

#else
vsf_err_t vsfip_httpd_start(struct vsfip_httpd_t *httpd,
			struct vsfip_httpd_service_t *servicemem, uint32_t maxconnection,
			uint16_t port, struct vsfile_t *root);

char* vsfip_http_getpostvaluebyname(char *src, char *name, uint32_t *valuesize);
#endif

#endif		// __VSFIP_HTTPD_H_INCLUDED__
