#include "app_type.h"
#include "compiler.h"

#include "framework/vsfsm/vsfsm.h"

#include "vsfip.h"
#include "vsfip_buffer.h"

#define VSFIP_BUFFER_NUM					16
#define VSFIP_BUFFER_SIZE					1600

static uint8_t vsfip_buffer_mem[VSFIP_BUFFER_NUM][VSFIP_BUFFER_SIZE];
static struct vsfip_buffer_t vsfip_buffer[VSFIP_BUFFER_NUM];

void vsfip_buffer_init(void)
{
	int i;
	for (i = 0; i < VSFIP_BUFFER_NUM; i++)
	{
		vsfip_buffer[i].ref = 0;
		vsfip_buffer[i].buffer = vsfip_buffer_mem[i];
//		vsfip_buffer[i].size = sizeof(vsfip_buffer_mem[i]);
	}
}

struct vsfip_buffer_t * vsfip_buffer_get(uint32_t size)
{
	if (size < VSFIP_BUFFER_SIZE)
	{
		int i;
		for (i = 0; i < VSFIP_BUFFER_NUM; i++)
		{
			if (!vsfip_buffer[i].ref)
			{
				vsfip_buffer[i].ref++;
				vsfip_buffer[i].buf.buffer =\
						vsfip_buffer[i].app.buffer = vsfip_buffer[i].buffer;
				vsfip_buffer[i].buf.size =\
						vsfip_buffer[i].app.size = size;
				vsfip_buffer[i].buf.buffer = vsfip_buffer[i].buffer;
				vsfip_buffer[i].next = NULL;
				vsfip_buffer[i].netif = NULL;
				return &vsfip_buffer[i];
			}
		}
	}
	return NULL;
}

void vsfip_buffer_release(struct vsfip_buffer_t *buf)
{
	if ((buf != NULL) && buf->ref)
	{
		buf->ref--;
	}
}
