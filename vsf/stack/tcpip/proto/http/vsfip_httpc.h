#ifndef __VSFIP_HTTPC_H_INCLUDED__
#define __VSFIP_HTTPC_H_INCLUDED__

#define HTTPC_DEBUG

struct vsfip_httpc_op_t
{
	vsf_err_t (*on_connect)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt, void *output);
	vsf_err_t (*on_recv)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt, void *output,
							uint32_t offset, struct vsfip_buffer_t *buf);
};

struct vsfip_httpc_param_t
{
	const struct vsfip_httpc_op_t *op;

	uint32_t resp_length;

	// private
	struct vsfsm_pt_t local_pt;
#ifdef HTTPC_DEBUG
	struct vsfsm_pt_t debug_pt;
#endif

	struct vsfip_socket_t *so;
	struct vsfip_sockaddr_t hostip;

	char *host;
	char *file;

	struct vsfip_buffer_t *buf;
	uint32_t resp_curptr;
	uint8_t *resp_type;
	uint8_t resp_code;
	uint16_t port;
	
};

#ifndef VSFCFG_STANDALONE_MODULE
extern const struct vsfip_httpc_op_t vsfip_httpc_op_stream;
extern const struct vsfip_httpc_op_t vsfip_httpc_op_buffer;
#endif

vsf_err_t vsfip_httpc_get(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
						char *wwwaddr, void *output);

#endif		// __VSFIP_HTTPD_H_INCLUDED__
