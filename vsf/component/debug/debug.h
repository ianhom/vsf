#ifndef __DEBUG_H_INCLUDED__
#define __DEBUG_H_INCLUDED__

#include "app_type.h"
#include "vsf_cfg.h"

#include "component/stream/stream.h"

#ifdef VSFCFG_DEBUG
void debug_init(struct vsf_stream_t *stream);
void debug_fini(void);
uint32_t debug(const char *format, ...);
#define vsf_debug(format, ...) debug("%s:%d "format"\r\n", __FILE__,\
										__LINE__, ##__VA_ARGS__)
#define vsf_debug_init(s) debug_init(s)
#define vsf_debug_fini() debug_fini()
#else
#define vsf_debug(format, ...)
#define vsf_debug_init(s)
#define vsf_debug_fini()
#endif // VSFCFG_DEBUG

#endif // __DEBUG_H_INCLUDED__