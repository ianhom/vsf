#include "vsf.h"
#include "app_hw_cfg.h"

vsf_err_t vsfip_httpc_on_connect_stream(struct vsfsm_pt_t *pt, vsfsm_evt_t evt);
vsf_err_t vsfip_httpc_on_recv_stream(struct vsfsm_pt_t *, vsfsm_evt_t, uint32_t,
										struct vsfip_buffer_t *);
vsf_err_t vsfip_httpc_on_recv_buffer(struct vsfsm_pt_t *, vsfsm_evt_t, uint32_t,
										struct vsfip_buffer_t *);

// dummy main, make compiler happy
int main(void)
{
	return 0;
}

ROOTFUNC vsf_err_t module_exit(struct vsf_module_t *module)
{
	struct vsf_tcpip_api_t *api = vsf.stack.net.tcpip;
	memset(api, 0, sizeof(*api));
	module->ifs = NULL;
	return VSFERR_NONE;
}

ROOTFUNC vsf_err_t __iar_program_start(struct vsf_module_t *module,
							struct app_hwcfg_t const *hwcfg)
{
	struct vsf_tcpip_api_t *api = vsf.stack.net.tcpip;

	// check board and api version
	if (strcmp(hwcfg->board, APP_BOARD_NAME) ||
		(api_ver != VSF_API_VERSION))
	{
		return VSFERR_NOT_SUPPORT;
	}

	memset(api, 0, sizeof(*api));
	api->init = vsfip_init;
	api->fini = vsfip_fini;
	api->netif_add = vsfip_netif_add;
	api->netif_remove = vsfip_netif_remove;
	api->buffer_get = vsfip_buffer_get;
	api->appbuffer_get = vsfip_appbuffer_get;
	api->buffer_reference = vsfip_buffer_reference;
	api->buffer_release = vsfip_buffer_release;
	api->socket = vsfip_socket;
	api->close = vsfip_close;
	api->listen = vsfip_listen;
	api->bind = vsfip_bind;
	api->tcp_connect = vsfip_tcp_connect;
	api->tcp_accept = vsfip_tcp_accept;
	api->tcp_async_send = vsfip_tcp_async_send;
	api->tcp_send = vsfip_tcp_send;
	api->tcp_async_recv = vsfip_tcp_async_recv;
	api->tcp_recv = vsfip_tcp_recv;
	api->tcp_close = vsfip_tcp_close;
	api->udp_async_send = vsfip_udp_async_send;
	api->udp_send = vsfip_udp_send;
	api->udp_async_recv = vsfip_udp_async_recv;
	api->udp_recv = vsfip_udp_recv;

	api->protocol.dhcpc.local.xid = VSFIP_DHCPC_XID;
	api->protocol.dhcpc.start = vsfip_dhcpc_start;

	api->protocol.dns.init = vsfip_dns_init;
	api->protocol.dns.setserver = vsfip_dns_setserver;
	api->protocol.dns.gethostbyname = vsfip_dns_gethostbyname;

	api->protocol.httpc.op_stream.on_connect = vsfip_httpc_on_connect_stream;
	api->protocol.httpc.op_stream.on_recv = vsfip_httpc_on_recv_stream;
	api->protocol.httpc.op_buffer.on_recv = vsfip_httpc_on_recv_buffer;

	module->ifs = (void *)api;
	return VSFERR_NONE;
}
