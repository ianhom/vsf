#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#include "debug.h"

#ifdef VSFCFG_DEBUG

#define VSFCFG_DEBUG_INFO_PARSE_LEN 	(VSFCFG_DEBUG_INFO_LEN + 100)
static uint8_t debug_info[VSFCFG_DEBUG_INFO_PARSE_LEN];

static struct vsf_stream_t *debug_stream = NULL;

uint32_t debug(const char *format, ...)
{
	va_list ap;
	struct vsf_buffer_t buffer;
	uint32_t size_avail, size_out, i, colon;
	uint8_t *ptr;

	if (debug_stream == NULL)
		return 0;

	size_avail = stream_get_free_size(debug_stream);
	if (size_avail < 3)
		return 0;

	// TODO: manual parse format
	va_start(ap, format);
	size_out = vsnprintf((char *)debug_info, VSFCFG_DEBUG_INFO_PARSE_LEN, format, ap);
	va_end(ap);

	if (size_out < 3)
		return 0;

	ptr = debug_info;
	i = 0;
	colon = 0;
	while (ptr < debug_info + size_out - 3)
	{
		if ((*ptr == ':') && (colon++ > 0))
			break;

		if ((*ptr == '/') || (*ptr == '\\'))
			 i = ptr - debug_info + 1;
		ptr++;
	}

	size_avail = min(size_avail, size_out - i);
	debug_info[i + size_avail - 2] = '\r';
	debug_info[i + size_avail - 1] = '\n';

	buffer.buffer = debug_info + i;
	buffer.size = size_avail;

	return stream_write(debug_stream, &buffer);
}

void debug_init(struct vsf_stream_t *stream)
{
	debug_stream = stream;
}

void debug_fini(void)
{
	debug_stream = NULL;
}

#endif

