
#ifndef __VSFIP_BUFFER_INCLUDED__
#define __VSFIP_BUFFER_INCLUDED__

#include "vsfip_cfg.h"

#include "tool/buffer/buffer.h"

struct vsfip_netif_t;
struct vsfip_buffer_t
{
	struct vsf_buffer_t buf;
	struct vsf_buffer_t app;
	
	union
	{
		uint8_t *ipver;
		struct vsfip_ip4head_t *ip4head;
//		struct vsfip_ip6head_t *ip6head;
	} iphead;
	
	uint16_t ref;
	uint16_t ttl;
	
	uint8_t *buffer;
//	uint32_t size;
	
	struct vsfip_netif_t *netif;
	struct vsfip_buffer_t *next;
};

void vsfip_buffer_init(void);
struct vsfip_buffer_t * vsfip_buffer_get(uint32_t size);
void vsfip_buffer_release(struct vsfip_buffer_t *buf);

#endif		// __VSFIP_BUFFER_INCLUDED__
