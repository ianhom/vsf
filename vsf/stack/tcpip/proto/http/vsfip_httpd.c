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

#undef vsfip_httpd_start
#undef vsfip_http_getpostvaluebyname

#ifdef HTTPD_DEBUG	
#include "framework/vsfshell/vsfshell.h"
#endif	

#define VSFIP_HTTP_SERVER_SOCKETTIMEOUT					4000

#define VSFIP_HTTP_HOMEPAGE								"index.htm"

static const char VSFIP_HTTP_HEAD_GET[] =				"GET ";
static const char VSFIP_HTTP_HEAD_POST[] =				"POST ";

static const char POST_ContentLength[] =				"Content-Length: ";
static const char POST_ContentDisposition[] =			"Content-Disposition: ";
static const char POST_ContentType[] =					"Content-Type: ";

static const char POST_ContentType_MutiFormData[] =		"multipart/form-data; ";
static const char POST_boundary[] =						"boundary=";
static const char POST_ContentType_ApplicationXWWW[] =	"application/x-www-form-urlencoded";
static const char POST_ContentType_ApplicationBin[] =	"application/octet-stream";

#ifndef VSFCFG_STANDALONE_MODULE
static const struct vsfip_http_contenttype_t\
					vsfip_httpd_supporttype[VSFIP_HTTPD_SUPPORTTYPECNT] =
{
	{VSFIP_HTTPD_TYPE_MUTIFORM, "multipart/form-data", " "},
	{VSFIP_HTTPD_TYPE_XWWW, "application/x-www-form-urlencoded", " "},
	{VSFIP_HTTPD_TYPE_HTML, "text/html", "htm"},
	{VSFIP_HTTPD_TYPE_JPG, "image/jpeg", "jpg"},
	{VSFIP_HTTPD_TYPE_TXT, "text/plain", "txt"},
	{VSFIP_HTTPD_TYPE_XML, "text/xml", "xml"},
	{VSFIP_HTTPD_TYPE_JS, "application/x-javascript", "js"},
	{VSFIP_HTTPD_TYPE_UNKNOW, "application/octet-stream", " "},
};

static const struct vsfile_memfile_t vsfip_http400 =
{
	.file.name = "400",
	.file.size = sizeof("HTTP/1.0 400 Bad Request\r\n\r\nBad Request"),
	.file.attr = VSFILE_ATTR_READONLY,
	.file.op = (struct vsfile_fsop_t *)&vsfile_memfs_op,
	.f.buff = "HTTP/1.0 400 Bad Request\r\n\r\nBad Request",
};

static const struct vsfile_memfile_t vsfip_http404 =
{
	.file.name = "404",
	.file.size = sizeof("HTTP/1.0 404 Not Found\r\n\r\nNot Found"),
	.file.attr = VSFILE_ATTR_READONLY,
	.file.op = (struct vsfile_fsop_t *)&vsfile_memfs_op,
	.f.buff = "HTTP/1.0 404 Not Found\r\n\r\nNot Found",
};
#endif

static char* vsfip_httpd_getnextline(char *buf, uint32_t size)
{
	char* bufend = buf + size - 1;
	while (buf < bufend)
	{
		if (*buf == '\r')
		{
			buf++;
			if (*buf == '\n')
				return buf + 1;
		}
		buf++;
	}
	return NULL;
}

static char* vsfip_httpd_gettypestr(uint8_t type)
{
	uint8_t i;
	for (i = 0; i < VSFIP_HTTPD_SUPPORTTYPECNT; i++)
	{
		if (type == vsfip_httpd_supporttype[i].type)
			return vsfip_httpd_supporttype[i].str;
	}
	return vsfip_httpd_supporttype[VSFIP_HTTPD_SUPPORTTYPECNT - 1].str;
}

static uint8_t vsfip_httpd_getfiletype(char *filename)
{
	uint8_t i;
	char *extname = vsfile_getfileext(filename);
	if (extname == NULL)
		return VSFIP_HTTPD_TYPE_UNKNOW;
	for (i = 0; i < VSFIP_HTTPD_SUPPORTTYPECNT; i++)
	{
		if (strcmp(extname, vsfip_httpd_supporttype[i].ext) == 0)
			return vsfip_httpd_supporttype[i].type;
	}
	return VSFIP_HTTPD_TYPE_UNKNOW;
}

static uint8_t vsfip_httpd_getcontenttype(char *str)
{
	uint8_t i;
	for (i = 0; i < VSFIP_HTTPD_SUPPORTTYPECNT; i++)
	{
		if (strcmp(str, vsfip_httpd_supporttype[i].str) == 0)
			return vsfip_httpd_supporttype[i].type;
	}
	return VSFIP_HTTPD_TYPE_UNKNOW;
}

char* vsfip_http_getpostvaluebyname(char *src, char *name,
										uint32_t *valuesize)
{
	uint32_t namesize = strlen(name);
	char *end;

	while (1)
	{
		end = strchr(src, '=');
		if (end == NULL)
			return NULL;
		if (memcmp(src, name, end - src) == 0)
		{
			//jump =
			end++;
			//getvalue
			src = strchr(src, '&');
			*valuesize = (src == NULL) ? strlen(end) : src - end;
			return end;
		}
		else
		{
			//getvalue
			src = strchr(src, '&');
			if (src == NULL)
				return NULL;
			src++;
		}
	}
}

static vsf_err_t vsfip_httpd_prase_req(struct vsfip_httpd_service_t *service,
										struct vsf_buffer_t *buf)
{
	char *rdptr = (char *)buf->buffer;
	uint32_t size = buf->size;
	char *filenameptr;

	if (!memcmp(rdptr, VSFIP_HTTP_HEAD_GET, sizeof(VSFIP_HTTP_HEAD_GET) - 1))
	{
		//Get a GET Requirst
		service->req = VSFIP_HTTP_GET;
		//move rdptr to destfile
 		rdptr += sizeof(VSFIP_HTTP_HEAD_GET) - 1;
		size -= sizeof(VSFIP_HTTP_HEAD_GET) - 1;
	}
	else
	if (!memcmp(rdptr, VSFIP_HTTP_HEAD_POST, sizeof(VSFIP_HTTP_HEAD_POST) - 1))
	{
		//Get a POST Requirst
		service->req = VSFIP_HTTP_POST;
		//move rdptr to destfile
 		rdptr += sizeof(VSFIP_HTTP_HEAD_POST) - 1;
		size -= sizeof(VSFIP_HTTP_HEAD_POST) - 1;
	}
	else
		return VSFERR_FAIL;

	filenameptr = rdptr;
	//getfilename
	while (*rdptr != ' ' && *rdptr != '\r' && *rdptr != '\n')
	{
		if (size <= 0)
			return VSFERR_FAIL;
		rdptr++;
		size--;
	}

	//set end of string
	*rdptr = 0;

	//ignore http version
	rdptr = vsfip_httpd_getnextline(rdptr, size);
	if (rdptr == NULL)
		buf->size = 0;
	else
	{
		buf->size -= buf->buffer - (uint8_t *)rdptr;
		buf->buffer = (uint8_t *)rdptr;
	}

	service->targetfilename = strcmp((char *)filenameptr, "/") ?
					filenameptr + 1 : VSFIP_HTTP_HOMEPAGE;
	return VSFERR_NONE;
}

static vsf_err_t vsfip_httpd_processpost(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfip_httpd_service_t *service =
							(struct vsfip_httpd_service_t *)pt->user_data;
	struct vsfip_post_t* post = &service->post;
	struct vsfip_buffer_t* inbuf = service->inbuf;
	char *rdptr = (char *)inbuf->app.buffer;
	uint32_t size = inbuf->app.size;
	vsf_err_t err;
	vsfsm_pt_begin(pt);
	//host ignore
	//UA ignore
	//accept ignore
	//content get
	post->size = 0;
	post->type = 0;

	while (1)
	{
		if (post->size != 0 && post->type != 0)
			break;

		if (memcmp(rdptr, "Content-", sizeof("Content-") - 1) == 0)
		{
			rdptr += sizeof("Content-") - 1;
			size -= sizeof("Content-") - 1;
			if (memcmp(rdptr, "Length: ", sizeof("Length: ") - 1) == 0)
			{
				char *nextline = vsfip_httpd_getnextline(rdptr, size);
				rdptr += sizeof("Length: ") - 1;
				size -= sizeof("Length: ") - 1;

				//setendof str
				*(nextline - 2) = 0;
				//getlength
				post->size = atoi(rdptr);

				size -= nextline - rdptr;
				rdptr = nextline;
			}
			else if (memcmp(rdptr, "Type: ", sizeof("Type: ") - 1) == 0)
			{
				char *nextline = vsfip_httpd_getnextline(rdptr, size);
				rdptr += sizeof("Type: ") - 1;
				size -= sizeof("Type: ") - 1;

				//setendof str
				*(nextline - 2) = 0;

				//gettype
				post->type = vsfip_httpd_getcontenttype(rdptr);

				size -= nextline - rdptr;
				rdptr = nextline;
			}
		}
		else
		{
			char *nextline = vsfip_httpd_getnextline(rdptr, size);
			if (nextline == NULL)
				goto errend;
			size -= nextline - rdptr;
			rdptr = nextline;
		}
	}

	if (post->type == VSFIP_HTTPD_TYPE_XWWW)
	{
		//double /r/n/r/n mean end of info
		while (memcmp(rdptr, "\r\n", 2) != 0)
		{
			char *nextline = vsfip_httpd_getnextline(rdptr, size);
			if (nextline == NULL)
				goto errend;
			size -= nextline - rdptr;
			rdptr = nextline;
		}

		//send post info to dat byoffset 0
		post->buf = (uint8_t *)vsfip_httpd_getnextline(rdptr, size);
		service->file_pt.user_data = service->posttarget->ondatparam;
		service->file_pt.state = 0;
		vsfsm_pt_entry(pt);
		err = service->posttarget->ondat(&service->file_pt, evt, post->type,
					(char *)post->buf, post->size, &service->targetfilename);
		if (err > 0) return err; else if (err < 0) goto errend;

		//Todo if post morethan onepkg
	}

	//set resp file
	if (service->targetfilename == NULL)
		service->targetfile = (struct vsfile_t *)&vsfip_http400;

	vsfip_buffer_release(service->inbuf);
	service->inbuf = NULL;
	return VSFERR_NONE;

errend:
	if (service->inbuf != NULL)
	{
		vsfip_buffer_release(service->inbuf);
		service->inbuf = NULL;
	}
	vsfsm_pt_end(pt);
	return VSFERR_FAIL;
}

static vsf_err_t vsfip_httpd_build_rsphead200(
			struct vsfip_httpd_service_t *service, struct vsf_buffer_t *buf)
{
	char *wrptr = (char *)buf->buffer;
	char numcache[10];
	uint8_t type;

	//here no check size please make sure buff enough

	strcpy(wrptr, "HTTP/1.1 200 OK\r\n");
	strcat(wrptr, "Server: vsfip/1.0\r\n");
//	itoa(service->targetfile->size, numcache, 10);
	snprintf(numcache, dimof(numcache), "%lld", service->targetfile->size);
	strcat(wrptr, "Content-Length: ");
	strcat(wrptr, numcache);
	strcat(wrptr, "\r\n");
	strcat(wrptr, "Content-Type: ");
	type = vsfip_httpd_getfiletype(service->targetfile->name);
	strcat(wrptr, vsfip_httpd_gettypestr(type));
	strcat(wrptr, "\r\n");
	strcat(wrptr, "\r\n");
	buf->size = strlen(wrptr);
	return VSFERR_NONE;
}

static vsf_err_t vsfip_httpd_getposttarget(
			struct vsfip_httpd_service_t *service, char *targetname)
{
	struct vsfip_httpd_posttarget_t *cur = service->postroot;
	while (cur != NULL && cur->name != NULL)
	{
		if (strcmp(cur->name, targetname) == 0)
		{
			service->posttarget = cur;
			return VSFERR_NONE;
		}
		cur++;
	}
	service->posttarget = NULL;
	return VSFERR_FAIL;
}

static vsf_err_t
vsfip_httpd_sendtargetfile(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfip_httpd_service_t *service =
							(struct vsfip_httpd_service_t *)pt->user_data;
	vsf_err_t err;

	vsfsm_pt_begin(pt);

	if (service->targetfile == NULL)
		return VSFERR_NONE;

	service->fileoffset = 0;

	//send httprsp head
	if (service->rsp == VSFIP_HTTP_200_OK)
	{
		service->outbuf = VSFIP_TCPBUF_GET(VSFIP_CFG_TCP_MSS);
		if (service->outbuf == NULL)
			return VSFERR_FAIL;

		err = vsfip_httpd_build_rsphead200(service, &service->outbuf->app);
		if (err < 0) goto exitfree; else goto httpsend;
	}

	do
	{
		service->outbuf = VSFIP_TCPBUF_GET(VSFIP_CFG_TCP_MSS);
		if (service->outbuf == NULL)
			return VSFERR_FAIL;

		//due to file map must set it when read write
		service->file_pt.user_data = service->targetfile;
		service->file_pt.state = 0;
		vsfsm_pt_entry(pt);
		err = vsfile_read(&service->file_pt, evt, service->targetfile,
				service->fileoffset, VSFIP_CFG_TCP_MSS,
				service->outbuf->app.buffer, &service->outbuf->app.size);
		if (err > 0) return err; else if (err < 0) goto exitfree;

		service->fileoffset += service->outbuf->app.size;

httpsend:
		service->socket_pt.state = 0;
		vsfsm_pt_entry(pt);
		err = vsfip_tcp_send(&service->socket_pt, evt, service->so,
				&service->so->remote_sockaddr, service->outbuf, false);
		if (err > 0) return err; else if (err < 0) goto exitnofree;
	} while (service->fileoffset < service->targetfile->size);

	vsfsm_pt_end(pt);
	return VSFERR_NONE;
exitfree:
	vsfip_buffer_release(service->outbuf);
exitnofree:
	return VSFERR_FAIL;
}

static vsf_err_t
vsfip_httpd_service_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfip_httpd_service_t *service =
							(struct vsfip_httpd_service_t *)pt->user_data;
	vsf_err_t err = VSFERR_NONE;

	vsfsm_pt_begin(pt);

	service->socket_pt.sm = pt->sm;
	service->local_pt.sm = pt->sm;
	service->file_pt.sm = pt->sm;

#ifdef HTTPD_DEBUG
	service->output_pt.sm = pt->sm;
#endif

	service->socket_pt.state = 0;
	vsfsm_pt_entry(pt);
	err = vsfip_tcp_recv(&service->socket_pt, evt, service->so,
			&service->so->remote_sockaddr, &service->inbuf);
	if (err > 0) return err; else if (err < 0) goto exit;

	err = vsfip_httpd_prase_req(service, &service->inbuf->app);
	if (err < 0) goto exit;

#ifdef HTTPD_DEBUG
	vsfshell_printf(&service->output_pt, "->HTTP %s %s" VSFSHELL_LINEEND,
					service->req == VSFIP_HTTP_GET? "GET" : "POST",
					service->targetfilename);
#endif
	if (service->cb->onca != NULL)
	{
		char *redirectfilename;
		service->local_pt.user_data = service->cb->oncaparam;
		service->local_pt.state = 0;
		vsfsm_pt_entry(pt);
		redirectfilename = NULL;
		err = service->cb->onca(&service->local_pt, evt,
				service->targetfilename, &service->so->remote_sockaddr.sin_addr,
				&redirectfilename);
		if (err > 0) return err; else if (err < 0) goto exit;

		if (redirectfilename != NULL)
		{
			service->targetfilename = redirectfilename;
			service->req = VSFIP_HTTP_GET;
		}
	}

#ifdef HTTPD_DEBUG
	vsfshell_printf(&service->output_pt,
					"<-HTTP %d targetfile: %s" VSFSHELL_LINEEND,
					service->rsp, service->targetfile->name);
#endif

	if (service->req == VSFIP_HTTP_POST)
	{
		err = vsfip_httpd_getposttarget(service, service->targetfilename);
		if (err < 0)
		{
			//filenotfound
			service->targetfilename = "http400";
		}
		else
		{
			//to do deal with post
			service->local_pt.user_data = service;
			service->local_pt.state = 0;
			vsfsm_pt_entry(pt);
			err = vsfip_httpd_processpost(&service->local_pt, evt);
			if (err > 0) return err; else if (err < 0) goto exit;
		}
	}

	//scan for rspfile
	service->file_pt.user_data = service->root;
	service->file_pt.state = 0;
	vsfsm_pt_entry(pt);
	err = vsfile_getfile(&service->file_pt, evt, service->root,
						service->targetfilename, &service->targetfile);
	if (err > 0) return err; else if (err < 0)
	{
		service->rsp = VSFIP_HTTP_404_NOTFOUND;
		//filenotfound
		service->targetfile = (struct vsfile_t *)&vsfip_http404;
	}
	else
	{
		service->rsp = VSFIP_HTTP_200_OK;
	}

	vsfip_buffer_release(service->inbuf);
	service->inbuf = NULL;

#ifdef HTTPD_DEBUG
	vsfshell_printf(&service->output_pt,
					"<-HTTP %d sendfile: %s" VSFSHELL_LINEEND,
					service->rsp, service->targetfile->name);
#endif
	//send targetfile
	service->local_pt.user_data = service;
	service->local_pt.state = 0;
	vsfsm_pt_entry(pt);
	err = vsfip_httpd_sendtargetfile(&service->local_pt, evt);
	if (err > 0) return err; if (err < 0) goto exit;

exit:
	//release socket
	if (service->inbuf != NULL)
	{
		vsfip_buffer_release(service->inbuf);
		service->inbuf = NULL;
	}

	service->socket_pt.state = 0;
	vsfsm_pt_entry(pt);
	err = vsfip_tcp_close(&service->socket_pt, evt,service->so);
	if (err > 0) return err;

	//release socket
	vsfip_close(service->so);
	service->so = NULL;

	vsfsm_pt_end(pt);
	return VSFERR_FAIL;
}

static vsf_err_t vsfip_httpd_attachtoservice(
			struct vsfip_httpd_t *httpd, struct vsfip_socket_t *acceptso)
{
	struct vsfip_httpd_service_t *service;
	uint8_t i;

	for (i = 0; i < httpd->maxconnection; i++)
	{
		if (httpd->service[i].so == NULL)
		{
			service = &httpd->service[i];

			//clear it mem
			memset(service , 0, sizeof(struct vsfip_httpd_service_t));

			//set so
			service->so = acceptso;
			service->root = httpd->root;
			service->postroot = httpd->postroot;

			service->pt.thread = vsfip_httpd_service_thread;
			service->pt.user_data = service;
			service->cb = &httpd->cb;

#ifdef HTTPD_DEBUG
			service->output_pt.thread = httpd->output_pt->thread;
			service->output_pt.user_data = httpd->output_pt->user_data;
			service->output_pt.state = 0;
#endif
			return vsfsm_pt_init(&service->sm, &service->pt);
		}
	}
	return VSFERR_NOT_ENOUGH_RESOURCES;
}

static vsf_err_t vsfip_httpd_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfip_httpd_t *httpd = (struct vsfip_httpd_t *)pt->user_data;
	vsf_err_t err = VSFERR_NONE;
	struct vsfip_socket_t *acceptso;

	vsfsm_pt_begin(pt);

	httpd->so = vsfip_socket(AF_INET, IPPROTO_TCP);
	httpd->so->rx_timeout_ms = 0;
	httpd->so->tx_timeout_ms = VSFIP_HTTP_SERVER_SOCKETTIMEOUT;

	err = vsfip_bind(httpd->so, httpd->sockaddr.sin_port);
	if (err < 0) goto close;

	err = vsfip_listen(httpd->so,httpd->maxconnection);
	if (err < 0) goto close;

	httpd->daemon_pt.sm = pt->sm;

	while (httpd->isactive)
	{
		httpd->daemon_pt.state = 0;
		vsfsm_pt_entry(pt);
		err = vsfip_tcp_accept(&httpd->daemon_pt, evt, httpd->so, &acceptso);
		if (err > 0) return err; else if (err < 0) httpd->isactive = false; else
		{
			err = vsfip_httpd_attachtoservice(httpd, acceptso);
			if (err != VSFERR_NONE)
			{
				httpd->acceptso = acceptso;
				httpd->daemon_pt.state = 0;
				vsfsm_pt_entry(pt);
				err = vsfip_tcp_close(&httpd->daemon_pt, evt, httpd->acceptso);
				if (err > 0) return err;
				vsfip_close(httpd->acceptso);
				httpd->acceptso = NULL;
			}
		}
	}

	vsfip_close(httpd->so);
	httpd->so = NULL;
	vsfsm_pt_end(pt);
close:
	return VSFERR_NONE;
}

vsf_err_t vsfip_httpd_start(struct vsfip_httpd_t *httpd,
			struct vsfip_httpd_service_t *servicemem, uint32_t maxconnection,
			uint16_t port, struct vsfile_t *root)
{
	if ((NULL == servicemem) || (NULL == httpd))
	{
		return VSFERR_FAIL;
	}

	//get the mem
	httpd->service = servicemem;
	httpd->maxconnection = maxconnection;
	httpd->root = root;

	httpd->sockaddr.sin_port = port;
	httpd->sockaddr.sin_addr.size = 4;

	httpd->pt.thread = vsfip_httpd_thread;
	httpd->pt.user_data = httpd;

	httpd->isactive = true;

	vsfsm_pt_init(&httpd->sm, &httpd->pt);
	return VSFERR_NONE;
}

#ifdef VSFCFG_STANDALONE_MODULE
void vsfip_httpd_modexit(struct vsf_module_t *module)
{
	vsf_bufmgr_free(module->ifs);
	module->ifs = NULL;
}

vsf_err_t vsfip_httpd_modinit(struct vsf_module_t *module,
								struct app_hwcfg_t const *cfg)
{
	struct vsfip_httpd_modifs_t *ifs;
	ifs = vsf_bufmgr_malloc(sizeof(struct vsfip_httpd_modifs_t));
	if (!ifs) return VSFERR_FAIL;
	memset(ifs, 0, sizeof(*ifs));

	ifs->start = vsfip_httpd_start;
	ifs->getpostvaluebyname = vsfip_http_getpostvaluebyname;
	ifs->http400.file.name = "400";
	ifs->http400.file.size = sizeof("HTTP/1.0 400 Bad Request\r\n\r\nBad Request");
	ifs->http400.file.attr = VSFILE_ATTR_READONLY;
	ifs->http400.file.op = (struct vsfile_fsop_t *)&vsfile_memfs_op;
	ifs->http400.f.buff = "HTTP/1.0 400 Bad Request\r\n\r\nBad Request";
	ifs->http404.file.name = "404";
	ifs->http404.file.size = sizeof("HTTP/1.0 404 Not Found\r\n\r\nNot Found");
	ifs->http404.file.attr = VSFILE_ATTR_READONLY;
	ifs->http404.file.op = (struct vsfile_fsop_t *)&vsfile_memfs_op;
	ifs->http404.f.buff = "HTTP/1.0 404 Not Found\r\n\r\nNot Found";
	ifs->supporttype[0].type = VSFIP_HTTPD_TYPE_MUTIFORM;
	ifs->supporttype[0].str = "multipart/form-data";
	ifs->supporttype[0].ext = " ";
	ifs->supporttype[1].type = VSFIP_HTTPD_TYPE_XWWW;
	ifs->supporttype[1].str = "application/x-www-form-urlencoded";
	ifs->supporttype[1].ext = " ";
	ifs->supporttype[2].type = VSFIP_HTTPD_TYPE_HTML;
	ifs->supporttype[2].str = "text/html";
	ifs->supporttype[2].ext = "htm";
	ifs->supporttype[3].type = VSFIP_HTTPD_TYPE_JPG;
	ifs->supporttype[3].str = "image/jpeg";
	ifs->supporttype[3].ext = "jpg";
	ifs->supporttype[4].type = VSFIP_HTTPD_TYPE_TXT;
	ifs->supporttype[4].str = "text/plain";
	ifs->supporttype[4].ext = "txt";
	ifs->supporttype[5].type = VSFIP_HTTPD_TYPE_XML;
	ifs->supporttype[5].str = "text/xml";
	ifs->supporttype[5].ext = "xml";
	ifs->supporttype[6].type = VSFIP_HTTPD_TYPE_JS;
	ifs->supporttype[6].str = "application/x-javascript";
	ifs->supporttype[6].ext = "js";
	ifs->supporttype[7].type = VSFIP_HTTPD_TYPE_UNKNOW;
	ifs->supporttype[7].str = "application/octet-stream";
	ifs->supporttype[7].ext = " ";
	module->ifs = ifs;
	return VSFERR_NONE;
}
#endif
