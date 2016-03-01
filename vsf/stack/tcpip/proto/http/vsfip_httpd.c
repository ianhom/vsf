#include "vsf.h"
#include <stdio.h>

#ifdef HTTPD_DEBUG	
#include "framework/vsfshell/vsfshell.h"
#endif	

#define VSFIP_HTTP_SERVER_SOCKETTIMEOUT        	 	4000

#define VSFIP_HTTP_HOMEPAGE				"index.htm"

const char  VSFIP_HTTP_HEAD_GET[] =			"GET ";
const char  VSFIP_HTTP_HEAD_POST[] =			"POST ";

const char  POST_ContentLength[] =			"Content-Length: ";
const char  POST_ContentDisposition[] =			"Content-Disposition: ";
const char  POST_ContentType[] =			"Content-Type: ";

const char  POST_ContentType_MutiFormData[] =		"multipart/form-data; ";
const char  POST_boundary[] =				"boundary=";
const char  POST_ContentType_ApplicationXWWW[] =	"application/x-www-form-urlencoded";
const char  POST_ContentType_ApplicationBin[] =		"application/octet-stream";

const struct vsfile_memfile_t http400 = 
{
	{
		"400",
		sizeof("HTTP/1.0 400 Bad Request\r\n\r\nBad Request"),
		VSFILE_ATTR_READONLY,
		(struct vsfile_fsop_t *)&vsfile_memfs_op,
		NULL
	},
	{
		"HTTP/1.0 400 Bad Request\r\n\r\nBad Request",
	}
};

const struct vsfile_memfile_t http404 = 
{
	{
		"400",
		sizeof("HTTP/1.0 404 Not Found\r\n\r\nNot Found"),
		VSFILE_ATTR_READONLY,
		(struct vsfile_fsop_t *)&vsfile_memfs_op,
		NULL
	},
	{
		"HTTP/1.0 404 Not Found\r\n\r\nNot Found",
	}
};

static uint8_t* vsfip_httpd_getnextline(uint8_t *buf, uint32_t size)
{
	uint8_t* bufend = buf + size - 1;
	while (buf < bufend)
	{
		if (*buf == '\r')
		{
			buf++;
			if (*buf == '\n')
				return buf+1;
		}
		buf++;
	}
	return NULL;
}

struct vsfip_http_contenttype_t
{
	uint8_t 	type;
	uint8_t		*str;
	uint8_t		*ext;
};

#define VSFIP_HTTPD_SUPPORTTYPECNT						8
const struct vsfip_http_contenttype_t vsfip_httpd_supporttype[VSFIP_HTTPD_SUPPORTTYPECNT] = 
{
	{VSFIP_HTTPD_TYPE_MUTIFORM, "multipart/form-data", " "},
	{VSFIP_HTTPD_TYPE_XWWW, "application/x-www-form-urlencoded", " "},
	{VSFIP_HTTPD_TYPE_HTML, "text/html", "htm"},
	{VSFIP_HTTPD_TYPE_JPG, "image/jpeg", "jpg"},
	{VSFIP_HTTPD_TYPE_TXT, "text/plain", "txt"},
	{VSFIP_HTTPD_TYPE_XML, "text/xml", "xml"},
	{VSFIP_HTTPD_TYPE_JS, "application/x-javascript", "js"},
	{VSFIP_HTTPD_TYPE_UNKNOW, "application/octet-stream", " "}
};

static uint8_t* vsfip_httpd_gettypestr(uint8_t type)
{
	uint8_t i;
	for (i = 0;i < VSFIP_HTTPD_SUPPORTTYPECNT;i++)
	{
		if (type == vsfip_httpd_supporttype[i].type)
			return vsfip_httpd_supporttype[i].str;
	}
	return vsfip_httpd_supporttype[VSFIP_HTTPD_SUPPORTTYPECNT - 1].str;
}

static uint8_t vsfip_httpd_getfiletype(uint8_t *filename)
{
	uint8_t i;
	uint8_t *extname;
	extname = strchr(filename, '.');
	if (extname == NULL)
		return VSFIP_HTTPD_TYPE_UNKNOW;
	extname++;
	for (i = 0;i < VSFIP_HTTPD_SUPPORTTYPECNT;i++)
	{
		if (strcmp(extname, vsfip_httpd_supporttype[i].ext) == 0)
			return vsfip_httpd_supporttype[i].type;
	}
	return VSFIP_HTTPD_TYPE_UNKNOW;
}

static uint8_t vsfip_httpd_getcontenttype(uint8_t *str)
{
	uint8_t i;
	for (i = 0;i < VSFIP_HTTPD_SUPPORTTYPECNT;i++)
	{
		if (strcmp(str, vsfip_httpd_supporttype[i].str) == 0)
			return vsfip_httpd_supporttype[i].type;
	}
	return VSFIP_HTTPD_TYPE_UNKNOW;
}

uint8_t* vsfip_http_getpostvaluebyname(uint8_t *src, uint8_t *name, uint32_t *valuesize)
{
	uint32_t namesize = strlen(name);
	uint8_t *end;
search:	
	end = strchr(src, '=');
	if (end == NULL)
		return NULL;
	if (memcmp(src, name, end - src) == 0)
	{
		//jump =
		end++;
		//getvalue
		src = strchr(src, '&');
		if(src == NULL)
		{
			*valuesize = strlen(end);
			
		}
		else
		{
			*valuesize = src - end;
		}
		return end;
	}
	else
	{
		//getvalue
		src = strchr(src, '&');
		if(src == NULL)
			return NULL;
		src++;
		goto search;
	}
}

static vsf_err_t vsfip_httpd_prase_req(struct vsfip_httpd_service_t * service, struct vsf_buffer_t *buf)
{
	uint8_t * rdptr = buf->buffer;
	uint32_t size = buf->size;
	uint8_t * filenameptr;
	
	if (memcmp(rdptr,VSFIP_HTTP_HEAD_GET , sizeof(VSFIP_HTTP_HEAD_GET) - 1) == 0)
	{
		//Get a GET Requirst
		service->req = VSFIP_HTTP_GET;
		//move rdptr to destfile
 		rdptr += sizeof(VSFIP_HTTP_HEAD_GET) - 1;
		size -= sizeof(VSFIP_HTTP_HEAD_GET) - 1;
	}
	else
	if (memcmp(rdptr,VSFIP_HTTP_HEAD_POST,sizeof(VSFIP_HTTP_HEAD_POST) - 1) == 0)
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
	while (*rdptr != ' '&&*rdptr != '\r'&&*rdptr != '\n')
	{
		if(size <= 0)
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
		buf->size -= buf->buffer - rdptr;
		buf->buffer = rdptr;
	}
	
	if (strcmp((char *)filenameptr, "/") == 0)
	{
		//homepage
		service->targetfilename = VSFIP_HTTP_HOMEPAGE;
	}
	else
	{
		service->targetfilename = filenameptr + 1;
	}
	
	return VSFERR_NONE;
}

static vsf_err_t vsfip_httpd_processpost(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfip_httpd_service_t * service = (struct vsfip_httpd_service_t *)pt->user_data;
	struct vsfip_post_t* post = &service->post;
	struct vsfip_buffer_t* inbuf = service->inbuf;
	uint8_t * rdptr = inbuf->app.buffer;
	uint32_t size = inbuf->app.size;
	vsf_err_t err;
	vsfsm_pt_begin(pt);
	//host ignore
	//UA ignore
	//accept ignore
	//content get
	post->size = 0;
	post->type = 0;
findcontentinfo:
	
	if (post->size != 0 && post->type != 0)
		goto getpostdat;
	if (memcmp(rdptr, "Content-", sizeof("Content-") - 1) == 0)
	{
		rdptr += sizeof("Content-") - 1;
		size -= sizeof("Content-") - 1;
		if(memcmp(rdptr, "Length: ", sizeof("Length: ") - 1) == 0)
		{
			uint8_t * nextline = vsfip_httpd_getnextline(rdptr, size);
			rdptr += sizeof("Length: ") - 1;
			size -= sizeof("Length: ") - 1;
			
			//setendof str
			*(nextline - 2) = 0;
			//getlength
			post->size = atoi(rdptr);
			
			size -= nextline - rdptr;
			rdptr = nextline;
		}
		else
		if (memcmp(rdptr, "Type: ", sizeof("Type: ") - 1) == 0)
		{
			uint8_t * nextline = vsfip_httpd_getnextline(rdptr, size);
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
		uint8_t * nextline = vsfip_httpd_getnextline(rdptr, size);
		if (nextline == NULL)
			goto errend;
		size -= nextline - rdptr;
		rdptr = nextline;
		
	}
	goto findcontentinfo;
getpostdat:
	if (post->type == VSFIP_HTTPD_TYPE_XWWW)
	{
		//double /r/n/r/n mean end of info
		while (memcmp(rdptr, "\r\n", 2) != 0)
		{
			uint8_t * nextline = vsfip_httpd_getnextline(rdptr, size);
			if (nextline == NULL)
				goto errend;
			size -= nextline - rdptr;
			rdptr = nextline;
		}
		
		//send post info to dat byoffset 0
		post->buf = vsfip_httpd_getnextline(rdptr, size);
		service->file_pt.user_data = service->posttarget->ondatparam;
		service->file_pt.state = 0;
		vsfsm_pt_entry(pt);
		err = service->posttarget->ondat(&service->file_pt, evt, 
                                         post->type, post->buf, post->size, 
                                         &service->targetfilename); 
		if (err > 0) return err; else if (err < 0) goto errend;
		
		//Todo if post morethan onepkg
	}
    
	//set resp file
	if (service->targetfilename == NULL)
		service->targetfile = (struct vsfile_t *)&http400;    
	
	vsfip_buffer_release(service->inbuf);
	service->inbuf = NULL;
	return VSFERR_NONE;

errend:
	if (service->inbuf != NULL)
	{
		vsfip_buffer_release(service->inbuf);
		service->inbuf = NULL;
	}
	return VSFERR_FAIL;
	vsfsm_pt_end(pt);
}

static vsf_err_t vsfip_httpd_build_rsphead200(struct vsfip_httpd_service_t * service, struct vsf_buffer_t *buf)
{
	uint8_t * wrptr = buf->buffer;
	uint8_t 	numcache[10];
	uint8_t		type;
	
	//here no check size please make sure buff enough
	
	strcpy(wrptr, "HTTP/1.1 200 OK\r\n");
	strcat(wrptr, "Server: vsfip/1.0\r\n");
	//itoa(service->targetfile->size, numcache, 10);
	sprintf(numcache, "%d", service->targetfile->size);
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

static vsf_err_t vsfip_httpd_getposttarget(struct vsfip_httpd_service_t * service, char *targetname)
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

static vsf_err_t vsfip_httpd_sendtargetfile(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfip_httpd_service_t * service = (struct vsfip_httpd_service_t *)pt->user_data;
	struct vsfip_buffer_t* outbuf;
	vsf_err_t err;
	vsfsm_pt_begin(pt);
	
	if (service->targetfile == NULL)
		return VSFERR_NONE;
	
	service->fileoffset = 0;
	
	//send httprsp head
	if (service->rsp == VSFIP_HTTP_200_OK)
	{
		outbuf = service->outbuf = vsfip_buffer_get(VSFIP_CFG_NETIF_HEADLEN + VSFIP_TCP_HEADLEN + VSFIP_CFG_TCP_MSS);
		if (outbuf == NULL)
			return VSFERR_FAIL;
		
		outbuf->app.buffer += VSFIP_CFG_NETIF_HEADLEN + VSFIP_TCP_HEADLEN;
		outbuf->app.size = VSFIP_CFG_TCP_MSS;

		err = vsfip_httpd_build_rsphead200(service, &outbuf->app);
		if (err < 0) goto exit;
		
		goto httpsend;
	}
	
	do
	{
		outbuf = service->outbuf = vsfip_buffer_get(VSFIP_CFG_NETIF_HEADLEN + VSFIP_TCP_HEADLEN + VSFIP_CFG_TCP_MSS);
		if (outbuf == NULL)
			return VSFERR_FAIL;
		
		outbuf->app.buffer += VSFIP_CFG_NETIF_HEADLEN + VSFIP_TCP_HEADLEN;
		outbuf->app.size = VSFIP_CFG_TCP_MSS;
		
		//due to file map must set it when read write
		service->file_pt.user_data = service->targetfile;
		service->file_pt.state = 0;
		vsfsm_pt_entry(pt);
		err = vsfile_read(&service->file_pt, evt, service->targetfile, 
						  service->fileoffset, VSFIP_CFG_TCP_MSS, 
						  outbuf->app.buffer, &outbuf->app.size);
		if (err > 0) return err; else if (err < 0) goto exit;
	
		service->fileoffset += outbuf->app.size;
		
httpsend:		
		service->socket_pt.state = 0;
		vsfsm_pt_entry(pt);
		err = vsfip_tcp_send(&service->socket_pt, evt, service->so, &service->so->remote_sockaddr,
                         	service->outbuf, false);
		if (err > 0) return err; else if (err < 0) goto exitnofree;
		
		service->outbuf = NULL;
		
	}while(service->fileoffset < service->targetfile->size);
	
	return VSFERR_NONE;
	
	vsfsm_pt_end(pt);
exit:
	if (service->outbuf != NULL)
	{
		vsfip_buffer_release(service->outbuf);
		service->outbuf = NULL;
	}
exitnofree:
	return VSFERR_FAIL;
}

static vsf_err_t vsfip_httpd_service_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfip_httpd_service_t * service = (struct vsfip_httpd_service_t *)pt->user_data;
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
	err = vsfip_tcp_recv(&service->socket_pt, evt, service->so, &service->so->remote_sockaddr,
                         &service->inbuf);
	if (err > 0) return err; else if (err < 0) goto exit;
	
	err = vsfip_httpd_prase_req(service, &service->inbuf->app);
	if (err < 0) goto exit;
	
#ifdef HTTPD_DEBUG
	vsfshell_printf(&service->output_pt, "->HTTP %s %s" VSFSHELL_LINEEND,
					service->req == VSFIP_HTTP_GET? "GET" : "POST", service->targetfilename);
#endif	
	if (service->cb->onca != NULL)
	{
		char *redirectfilename = NULL;
		service->local_pt.user_data = service->cb->oncaparam;
		service->local_pt.state = 0;
		vsfsm_pt_entry(pt);
		err = service->cb->onca(&service->local_pt, evt, service->targetfilename,
                                &service->so->remote_sockaddr.sin_addr, &redirectfilename);
		if (err > 0) return err; else if (err < 0) goto exit;
        
		if (redirectfilename != NULL)
        	{
			service->targetfilename = redirectfilename;
			service->req = VSFIP_HTTP_GET;
		}
	}
    
#ifdef HTTPD_DEBUG
	vsfshell_printf(&service->output_pt, "<-HTTP %d targetfile: %s" VSFSHELL_LINEEND,
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
	if (err > 0) return err; 
	else if (err < 0)
	{
		service->rsp = VSFIP_HTTP_404_NOTFOUND;
		//filenotfound
		service->targetfile = (struct vsfile_t *)&http404;
	}
	else
	{
		service->rsp = VSFIP_HTTP_200_OK;
	}    
    
	vsfip_buffer_release(service->inbuf);
	service->inbuf = NULL;
    
#ifdef HTTPD_DEBUG
	vsfshell_printf(&service->output_pt, "<-HTTP %d sendfile: %s" VSFSHELL_LINEEND,
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

static vsf_err_t vsfip_httpd_attachtoservice(struct vsfip_httpd_t *httpd, struct vsfip_socket_t *acceptso)
{
	struct vsfip_httpd_service_t * service;
	uint8_t i;
	for (i = 0;i < httpd->maxconnection;i++)
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

	while(httpd->isactive)
	{
		httpd->daemon_pt.state = 0;
		vsfsm_pt_entry(pt);
		err = vsfip_tcp_accept(&httpd->daemon_pt, evt, httpd->so, &acceptso);
		
		if (err > 0) return err; 
		else if (err < 0) httpd->isactive = false;
		else
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
