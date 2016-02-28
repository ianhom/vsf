#ifndef __VSFIP_HTTPD_H_INCLUDED__
#define __VSFIP_HTTPD_H_INCLUDED__



//#define HTTPD_DEBUG

enum vsfip_httpd_service_req_t
{
	VSFIP_HTTP_POST,
	VSFIP_HTTP_GET		
};

enum vsfip_httpd_service_rsp_t
{
	VSFIP_HTTP_200_OK = 200,
	VSFIP_HTTP_404_NOTFOUND = 404	
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
	VSFIP_HTTPD_TYPE_UNKNOW = 0xFF	
};

struct vsfip_post_t
{
    uint32_t    size;
    uint8_t     *buf;
    uint8_t     type;
};

struct vsfip_httpd_posttarget_t
{
    char                        *name;
    vsf_err_t 			        (*ondat)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
                                          uint8_t type, uint8_t	*buf, uint32_t size,
                                          char **rspfilename);
	void 				        *ondatparam;
};

struct vsfip_httpd_service_t
{
	struct vsfsm_t 			sm;
	struct vsfsm_pt_t 		pt;
	struct vsfsm_pt_t 		socket_pt;
	struct vsfsm_pt_t 		local_pt;
	struct vsfsm_pt_t 		file_pt;
	
	struct vsfip_post_t	    post;
	
	uint8_t  				*targetfilename;
	struct vsfile_t 		*targetfile;
	struct vsfile_t 		*root;
    struct vsfip_httpd_posttarget_t 
                            *posttarget;
    struct vsfip_httpd_posttarget_t 
                            *postroot;
    
	uint32_t 				fileoffset;
        
	struct vsfip_socket_t 	*so;
        
	struct vsfip_buffer_t 	*inbuf;
	struct vsfip_buffer_t 	*outbuf;
	
	struct vsfip_httpd_cb_t	*cb;
	
	uint16_t 				req;
    uint16_t 				rsp;
       
#ifdef HTTPD_DEBUG
	struct vsfsm_pt_t  				output_pt;
#endif
};


struct vsfip_httpd_cb_t
{
	vsf_err_t 			(*onca)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
								uint8_t *reqfilename, 
								struct vsfip_ipaddr_t *useripaddr, char **redirectfilename);
	void 				*oncaparam;
};

struct vsfip_httpd_t
{
	struct vsfsm_t 					sm;
	struct vsfsm_pt_t 				pt;
	struct vsfsm_pt_t 				daemon_pt;

	struct vsfip_httpd_service_t 	*service;
	uint32_t						maxconnection;

	struct vsfip_socket_t 			*so;
    struct vsfip_socket_t 			*acceptso;
        
	struct vsfip_sockaddr_t 		sockaddr;
	
	struct vsfile_t 				*root;
	struct vsfip_httpd_posttarget_t *postroot;
    
	struct vsfip_httpd_cb_t			cb;
	
	bool isactive;

#ifdef HTTPD_DEBUG
	struct vsfsm_pt_t  				*output_pt;
#endif
        
};


vsf_err_t vsfip_httpd_start(struct vsfip_httpd_t *httpd,
							struct vsfip_httpd_service_t *servicemem, uint32_t maxconnection,
							uint16_t port, struct vsfile_t *root);

uint8_t* vsfip_http_getpostvaluebyname(uint8_t *src, uint8_t *name, uint32_t *valuesize);
#endif		// __VSFIP_HTTPD_H_INCLUDED__
