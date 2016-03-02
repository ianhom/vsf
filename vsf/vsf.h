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

#ifndef __VSF_H_INCLUDED__
#define __VSF_H_INCLUDED__

#include "app_cfg.h"
#include "app_type.h"

#include "compiler.h"
#include "vsf_cfg.h"
#include "interfaces.h"

#ifdef VSFCFG_STANDALONE_MODULE
#include "app_hw_cfg.h"
#endif

#ifdef VSFCFG_MODULE
struct vsf_module_info_t
{
	uint32_t entry;			// entry offset
	uint32_t exit;			// exit offset
	uint32_t size;			// module size
	char name[1];			// module name
};

struct vsf_module_t
{
	struct vsf_module_info_t *flash;

	void *ifs;
	uint8_t *code_buff;

	struct vsf_module_t *next;
};
#endif		// VSFCFG_MODULE

// framework
#include "framework/vsfsm/vsfsm.h"
#include "framework/vsftimer/vsftimer.h"

#ifdef VSFCFG_BUFFER
#include "component/buffer/buffer.h"
#endif
#ifdef VSFCFG_STREAM
#include "component/stream/stream.h"
#endif
#ifdef VSFCFG_MAL
#include "component/mal/vsfmal.h"
#ifdef VSFCFG_SCSI
#include "component/mal/vsfscsi.h"
#endif
#endif
#ifdef VSFCFG_FILE
#include "component/file/vsfile.h"
#include "component/file/fs/malfs/vsf_malfs.h"
#include "component/file/fs/malfs/fat/vsffat.h"
#endif
#ifdef VSFCFG_DEBUG
#include "component/debug/debug.h"
#endif

#define VSF_API_VERSION						0x00000001

#ifdef VSFCFG_FUNC_SHELL
#include "framework/vsfshell/vsfshell.h"
#endif

#ifdef VSFCFG_FUNC_TCPIP
#include "stack/tcpip/vsfip.h"
#include "stack/tcpip/netif/eth/vsfip_eth.h"
#include "stack/tcpip/proto/dhcp/vsfip_dhcpc.h"
#include "stack/tcpip/proto/dhcp/vsfip_dhcpd.h"
#include "stack/tcpip/proto/dns/vsfip_dnsc.h"
#include "stack/tcpip/proto/http/vsfip_httpc.h"
#include "stack/tcpip/proto/http/vsfip_httpd.h"
#include "stack/tcpip/proto/telnet/vsfip_telnetd.h"
#endif

#ifdef VSFCFG_FUNC_USBD
#include "stack/usb/core/vsfusbd.h"
#include "stack/usb/class/device/CDC/vsfusbd_CDC.h"
#include "stack/usb/class/device/CDC/vsfusbd_CDCACM.h"
#ifdef VSFCFG_FUNC_TCPIP
#include "stack/usb/class/device/CDC/vsfusbd_RNDIS.h"
#endif
#include "stack/usb/class/device/HID/vsfusbd_HID.h"
#ifdef VSFCFG_SCSI
#include "stack/usb/class/device/MSC/vsfusbd_MSC_BOT.h"
#endif
#endif

#ifdef VSFCFG_FUNC_USBH
#include "stack/usb/core/vsfusbh.h"
#include "stack/usb/core/hcd/ohci/vsfohci.h"
#include "stack/usb/class/host/HUB/vsfusbh_HUB.h"
#include "stack/usb/class/host/HID/vsfusbh_HID.h"
#endif

#ifdef VSFCFG_FUNC_BCMWIFI
#include "stack/tcpip/netif/eth/broadcom/bcm_wifi.h"
#include "stack/tcpip/netif/eth/broadcom/bus/bcm_bus.h"
#endif

#ifdef VSFCFG_STANDALONE_MODULE
// libc is included in VSF system
#include <stdarg.h>
typedef struct
{
	int quot;
	int rem;
} div_t;

typedef struct
{
	long quot;
	long rem;
} ldiv_t;

typedef struct
{
	long long quot;
	long long rem;
} lldiv_t;
#else
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#endif

struct vsf_t
{
	uint32_t ver;
	struct interfaces_info_t const *ifs;

	struct
	{
		struct
		{
			int (*abs)(int);
			long (*labs)(long);
			long long (*llabs)(long long);

			div_t (*div)(int, int);
			ldiv_t (*ldiv)(long, long);
			lldiv_t (*lldiv)(long long, long long);

			double (*strtod)(const char*, char**);
			float (*strtof)(const char*, char**);
			long double (*strtold)(const char*, char**);
			long (*strtol)(const char*, char**, int);
			unsigned long (*strtoul)(const char*, char**, int);
			long long (*strtoll)(const char*, char**, int);
			unsigned long long (*strtoull)(const char*, char**, int);

			double (*atof)(const char*);
			int (*atoi)(const char*);
			long (*atol)(const char*);
			long long (*atoll)(const char*);
		} stdlib;
		struct
		{
			int (*sprintf)(char*, const char*, ...);
			int (*snprintf)(char*, size_t, const char*, ...);
			int (*vsprintf)(char*, const char*, va_list);
			int (*vsnprintf)(char*, size_t, const char*, va_list);
		} stdio;
		struct
		{
			int (*memcmp)(const void*, const void*, size_t);
			void* (*memcpy)(void*, const void*, size_t);
			void* (*memmove)(void*, const void*, size_t);
			void* (*memset)(void*, int, size_t);
			char* (*strcat)(char*, const char*);
			int (*strcmp)(const char*, const char*);
			char* (*strcpy)(char*, const char*);
			size_t (*strcspn)(const char*, const char*);
			size_t (*strlen)(const char*);
			char* (*strncat)(char*, const char*, size_t);
			int (*strncmp)(const char*, const char*, size_t);
			char* (*strncpy)(char*, const char*, size_t);
			size_t (*strspn)(const char*, const char*);
			char* (*strtok)(char*, const char*);
			size_t (*strxfrm)(char*, const char*, size_t);
			int (*strcasecmp)(const char*, const char*);
			int (*strncasecmp)(const char*, const char*, size_t);
			char* (*strtok_r)(char*, const char*, char**);
			size_t (*strnlen)(char const*, size_t);
			void* (*memchr)(const void*, int, size_t);
			char* (*strchr)(const char*, int);
			char* (*strpbrk)(const char*, const char*);
			char* (*strrchr)(const char*, int);
			char* (*strstr)(const char*, const char*);
		} string;
		struct
		{
			int (*isdigit)(int);
			int (*isspace)(int);
			int (*isalpha)(int);
			int (*isalnum)(int);
			int (*isprint)(int);
			int (*isupper)(int);
			int (*islower)(int);
			int (*isxdigit)(int);
			int (*isblank)(int);
			int (*isgraph)(int);
			int (*iscntrl)(int);
			int (*ispunct)(int);
			int (*tolower)(int);
			int (*toupper)(int);
		} ctype;
	} libc;

	struct vsf_framework_t
	{
		void (*evtq_init)(struct vsfsm_evtq_t*);
		void (*evtq_set)(struct vsfsm_evtq_t*);
		struct vsfsm_evtq_t* (*evtq_get)(void);
		vsf_err_t (*poll)(void);
		uint32_t (*get_event_pending)(void);
		vsf_err_t (*sm_init)(struct vsfsm_t*);
		vsf_err_t (*sm_fini)(struct vsfsm_t*);
#if VSFSM_CFG_PT_EN
		vsf_err_t (*pt_init)(struct vsfsm_t*, struct vsfsm_pt_t*);
#endif
		vsf_err_t (*post_evt)(struct vsfsm_t*, vsfsm_evt_t);
		vsf_err_t (*post_evt_pending)(struct vsfsm_t*, vsfsm_evt_t);

		void (*enter_critical)(void);
		void (*leave_critical)(void);

#if VSFSM_CFG_SYNC_EN
		struct
		{
			vsf_err_t (*init)(struct vsfsm_sync_t*, uint32_t, uint32_t,
								vsfsm_evt_t);
			vsf_err_t (*cancel)(struct vsfsm_sync_t*, struct vsfsm_t*);
			vsf_err_t (*increase)(struct vsfsm_sync_t*);
			vsf_err_t (*decrease)(struct vsfsm_sync_t*, struct vsfsm_t*);
		} sync;
#endif

		struct
		{
			vsf_err_t (*init)(struct vsftimer_mem_op_t*);
			struct vsftimer_t * (*create)(struct vsfsm_t*, uint32_t, int16_t,
								vsfsm_evt_t);
			void (*free)(struct vsftimer_t*);
			void (*enqueue)(struct vsftimer_t*);
			void (*dequeue)(struct vsftimer_t*);
			void (*callback)(void);
		} timer;

#ifdef VSFCFG_MODULE
		struct
		{
			struct vsf_module_t* (*get)(char*);
			void (*reg)(struct vsf_module_t*);
			void (*unreg)(struct vsf_module_t*);
			void* (*load)(char*);
			void (*unload)(char*);
		} module;
#endif
	} framework;

	struct
	{
#ifdef VSFCFG_BUFFER
		struct
		{
			struct
			{
				void (*init)(struct vsfq_t*);
				void (*append)(struct vsfq_t*, struct vsfq_node_t*);
				void (*remove)(struct vsfq_t*, struct vsfq_node_t*);
				void (*enqueue)(struct vsfq_t*, struct vsfq_node_t*);
				struct vsfq_node_t* (*dequeue)(struct vsfq_t*);
			} queue;

			struct
			{
				vsf_err_t (*init)(struct vsf_fifo_t*);
				uint32_t (*push8)(struct vsf_fifo_t*, uint8_t);
				uint8_t (*pop8)(struct vsf_fifo_t*);
				uint32_t (*push)(struct vsf_fifo_t*, uint32_t, uint8_t*);
				uint32_t (*pop)(struct vsf_fifo_t*, uint32_t, uint8_t*);
				uint32_t (*get_data_length)(struct vsf_fifo_t*);
				uint32_t (*get_avail_length)(struct vsf_fifo_t*);
			} fifo;

			struct
			{
				vsf_err_t (*init)(struct vsf_multibuf_t*);
				uint8_t* (*get_empty)(struct vsf_multibuf_t*);
				vsf_err_t (*push)(struct vsf_multibuf_t*);
				uint8_t* (*get_payload)(struct vsf_multibuf_t*);
				vsf_err_t (*pop)(struct vsf_multibuf_t*);
			} multibuf;

			struct
			{
				void* (*malloc_aligned_do)(uint32_t, uint32_t);
				void (*free_do)(void*);
			} bufmgr;

			struct
			{
				void (*init)(struct vsfpool_t*);
				void* (*alloc)(struct vsfpool_t*);
				void (*free)(struct vsfpool_t*, void*);
			} pool;
		} buffer;
#endif

#ifdef VSFCFG_LIST
		struct
		{
			int (*is_in)(struct sllist*, struct sllist*);
			int (*remove)(struct sllist**, struct sllist*);
			void (*append)(struct sllist*, struct sllist*);
			void (*delete_next)(struct sllist*);
		} list;
#endif
	} component;

	struct
	{
		struct
		{
			uint8_t (*bit_reverse_u8)(uint8_t);
			uint16_t (*bit_reverse_u16)(uint16_t);
			uint32_t (*bit_reverse_u32)(uint32_t);
			uint64_t (*bit_reverse_u64)(uint64_t);

			uint16_t (*get_u16_msb)(uint8_t*);
			uint32_t (*get_u24_msb)(uint8_t*);
			uint32_t (*get_u32_msb)(uint8_t*);
			uint64_t (*get_u64_msb)(uint8_t*);
			uint16_t (*get_u16_lsb)(uint8_t*);
			uint32_t (*get_u24_lsb)(uint8_t*);
			uint32_t (*get_u32_lsb)(uint8_t*);
			uint64_t (*get_u64_lsb)(uint8_t*);

			void (*set_u16_msb)(uint8_t*, uint16_t);
			void (*set_u24_msb)(uint8_t*, uint32_t);
			void (*set_u32_msb)(uint8_t*, uint32_t);
			void (*set_u64_msb)(uint8_t*, uint64_t);
			void (*set_u16_lsb)(uint8_t*, uint16_t);
			void (*set_u24_lsb)(uint8_t*, uint32_t);
			void (*set_u32_lsb)(uint8_t*, uint32_t);
			void (*set_u64_lsb)(uint8_t*, uint64_t);

			uint16_t (*swap_u16)(uint16_t);
			uint32_t (*swap_u24)(uint32_t);
			uint32_t (*swap_u32)(uint32_t);
			uint64_t (*swap_u64)(uint64_t);

			struct
			{
				void (*set)(uint32_t*, int);
				void (*clr)(uint32_t*, int);
				int (*ffz)(uint32_t*, int);
			} mskarr;
		} bittool;
	} tool;
};

#ifdef VSFCFG_STANDALONE_MODULE

#define vsf								(*(struct vsf_t *)VSFCFG_API_ADDR)
#define api_ver							vsf.ver
#define core_interfaces					(*vsf.ifs)

// interfaces constants
#define GPIO_INFLOAT					core_interfaces.gpio.constants.INFLOAT
#define GPIO_INPU						core_interfaces.gpio.constants.INPU
#define GPIO_INPD						core_interfaces.gpio.constants.INPD
#define GPIO_OUTPP						core_interfaces.gpio.constants.OUTPP
#define GPIO_OUTOD						core_interfaces.gpio.constants.OUTOD
#define vsfhal_gpio_init				core_interfaces.gpio.init
#define vsfhal_gpio_fini				core_interfaces.gpio.fini
#define vsfhal_gpio_config_pin			core_interfaces.gpio.config_pin
#define vsfhal_gpio_config				core_interfaces.gpio.config
#define vsfhal_gpio_in					core_interfaces.gpio.in
#define vsfhal_gpio_out					core_interfaces.gpio.out
#define vsfhal_gpio_set					core_interfaces.gpio.set
#define vsfhal_gpio_clear				core_interfaces.gpio.clear
#define vsfhal_gpio_get					core_interfaces.gpio.get

#define vsfhal_core_init				core_interfaces.core.init
#define vsfhal_core_sleep				core_interfaces.core.sleep
#define vsfhal_core_pendsv_config		core_interfaces.core.pendsv_config
#define vsfhal_core_pendsv_trigger		core_interfaces.core.pendsv_trigger

#define vsfhal_tickclk_init				core_interfaces.tickclk.init
#define vsfhal_tickclk_fini				core_interfaces.tickclk.fini
#define vsfhal_tickclk_start			core_interfaces.tickclk.start
#define vsfhal_tickclk_stop				core_interfaces.tickclk.stop
#define vsfhal_tickclk_get_count		core_interfaces.tickclk.get_count
#define vsfhal_tickclk_set_callback		core_interfaces.tickclk.set_callback

#define SPI_MASTER						core_interfaces.spi.constants.MASTER
#define SPI_SLAVE						core_interfaces.spi.constants.SLAVE
#define SPI_MODE0						core_interfaces.spi.constants.MODE0
#define SPI_MODE1						core_interfaces.spi.constants.MODE1
#define SPI_MODE2						core_interfaces.spi.constants.MODE2
#define SPI_MODE3						core_interfaces.spi.constants.MODE3
#define SPI_MSB_FIRST					core_interfaces.spi.constants.MSB_FIRST
#define SPI_LSB_FIRST					core_interfaces.spi.constants.LSB_FIRST
#define vsfhal_spi_init					core_interfaces.spi.init
#define vsfhal_spi_fini					core_interfaces.spi.fini
#define vsfhal_spi_get_ability			core_interfaces.spi.get_ability
#define vsfhal_spi_enable				core_interfaces.spi.enable
#define vsfhal_spi_disable				core_interfaces.spi.disable
#define vsfhal_spi_config				core_interfaces.spi.config
#define vsfhal_spi_config_callback		core_interfaces.spi.config_callback
#define vsfhal_spi_select				core_interfaces.spi.select
#define vsfhal_spi_deselect				core_interfaces.spi.deselect
#define vsfhal_spi_start				core_interfaces.spi.start
#define vsfhal_spi_stop					core_interfaces.spi.stop

#define EINT_ONFALL						core_interfaces.eint.constants.ONFALL
#define EINT_ONRISE						core_interfaces.eint.constants.ONRISE
#define EINT_ONLOW						core_interfaces.eint.constants.ONLOW
#define EINT_ONHIGH						core_interfaces.eint.constants.ONHIGH
#define vsfhal_eint_init				core_interfaces.eint.init
#define vsfhal_eint_fini				core_interfaces.eint.fini
#define vsfhal_eint_config				core_interfaces.eint.config
#define vsfhal_eint_enable				core_interfaces.eint.enable
#define vsfhal_eint_disable				core_interfaces.eint.disable

#define vsfhal_hcd_init					core_interfaces.hcd.init
#define vsfhal_hcd_fini					core_interfaces.hcd.fini
#define vsfhal_hcd_regbase				core_interfaces.hcd.regbase
// more interfaces related MACROs here

// libc
#define abs								vsf.libc.stdlib.abs
#define labs							vsf.libc.stdlib.labs
#define llabs							vsf.libc.stdlib.llabs
#define div								vsf.libc.stdlib.div
#define ldiv							vsf.libc.stdlib.ldiv
#define lldiv							vsf.libc.stdlib.lldiv
#define strtod							vsf.libc.stdlib.strtod
#define strtof							vsf.libc.stdlib.strtof
#define strtold							vsf.libc.stdlib.strtold
#define strtol							vsf.libc.stdlib.strtol
#define strtoul							vsf.libc.stdlib.strtoul
#define strtoll							vsf.libc.stdlib.strtoll
#define strtoull						vsf.libc.stdlib.strtoull
#define atof							vsf.libc.stdlib.atof
#define atoi							vsf.libc.stdlib.atoi
#define atol							vsf.libc.stdlib.atol
#define atoll							vsf.libc.stdlib.atoll
#define sprintf							vsf.libc.stdio.sprintf
#define snprintf						vsf.libc.stdio.snprintf
#define vsprintf						vsf.libc.stdio.vsprintf
#define vsnprintf						vsf.libc.stdio.vsnprintf
#define memcmp							vsf.libc.string.memcmp
#define memcpy							vsf.libc.string.memcpy
#define memmove							vsf.libc.string.memmove
#define memset							vsf.libc.string.memset
#define strcat							vsf.libc.string.strcat
#define strcmp							vsf.libc.string.strcmp
#define strcpy							vsf.libc.string.strcpy
#define strcspn							vsf.libc.string.strcspn
#define strlen							vsf.libc.string.strlen
#define strncat							vsf.libc.string.strncat
#define strncmp							vsf.libc.string.strncmp
#define strncpy							vsf.libc.string.strncpy
#define strspn							vsf.libc.string.strspn
#define strtok							vsf.libc.string.strtok
#define strxfrm							vsf.libc.string.strxfrm
#define strcasecmp						vsf.libc.string.strcasecmp
#define strncasecmp						vsf.libc.string.strncasecmp
#define strtok_r						vsf.libc.string.strtok_r
#define strnlen							vsf.libc.string.strnlen
#define memchr							vsf.libc.string.memchr
#define strchr							vsf.libc.string.strchr
#define strpbrk							vsf.libc.string.strpbrk
#define strrchr							vsf.libc.string.strrchr
#define strstr							vsf.libc.string.strstr
#define isdigit							vsf.libc.ctype.isdigit
#define isspace							vsf.libc.ctype.isspace
#define isalpha							vsf.libc.ctype.isalpha
#define isalnum							vsf.libc.ctype.isalnum
#define isprint							vsf.libc.ctype.isprint
#define isupper							vsf.libc.ctype.isupper
#define islower							vsf.libc.ctype.islower
#define isxdigit						vsf.libc.ctype.isxdigit
#define isblank							vsf.libc.ctype.isblank
#define isgraph							vsf.libc.ctype.isgraph
#define iscntrl							vsf.libc.ctype.iscntrl
#define ispunct							vsf.libc.ctype.ispunct
#define tolower							vsf.libc.ctype.tolower
#define toupper							vsf.libc.ctype.toupper

#define vsfsm_evtq_init					vsf.framework.evtq_init
#define vsfsm_evtq_set					vsf.framework.evtq_set
#define vsfsm_evtq_get					vsf.framework.evtq_get
#define vsfsm_poll						vsf.framework.poll
#define vsfsm_get_event_pending			vsf.framework.get_event_pending
#define vsfsm_init						vsf.framework.sm_init
#define vsfsm_fini						vsf.framework.sm_fini
#if VSFSM_CFG_PT_EN
#define vsfsm_pt_init					vsf.framework.pt_init
#endif
#define vsfsm_post_evt					vsf.framework.post_evt
#define vsfsm_post_evt_pending			vsf.framework.post_evt_pending

#undef vsf_enter_critical
#define vsf_enter_critical				vsf.framework.enter_critical
#undef vsf_leave_critical
#define vsf_leave_critical				vsf.framework.leave_critical

#if VSFSM_CFG_SYNC_EN
#define vsfsm_sync_init					vsf.framework.sync.init
#define vsfsm_sync_cancel				vsf.framework.sync.cancel
#define vsfsm_sync_increase				vsf.framework.sync.increase
#define vsfsm_sync_decrease				vsf.framework.sync.decrease
#endif

#define vsftimer_init					vsf.framework.timer.init
#define vsftimer_create					vsf.framework.timer.create
#define vsftimer_free					vsf.framework.timer.free
#define vsftimer_enqueue				vsf.framework.timer.enqueue
#define vsftimer_dequeue				vsf.framework.timer.dequeue
#define vsftimer_callback_int			vsf.framework.timer.callback

#ifdef VSFCFG_MODULE
#define vsf_module_get					vsf.framework.module.get
#define vsf_module_register				vsf.framework.module.reg
#define vsf_module_unregister			vsf.framework.module.unreg
#define vsf_module_load					vsf.framework.module.load
#define vsf_module_unload				vsf.framework.module.unload
#endif

#ifdef VSFCFG_BUFFER
#define vsfq_init						vsf.component.buffer.queue.init
#define vsfq_append						vsf.component.buffer.queue.append
#define vsfq_remove						vsf.component.buffer.queue.remove
#define vsfq_enqueue					vsf.component.buffer.queue.enqueue
#define vsfq_dequeue					vsf.component.buffer.queue.dequeue

#define vsf_fifo_init					vsf.component.buffer.fifo.init
#define vsf_fifo_push8					vsf.component.buffer.fifo.push8
#define vsf_fifo_pop8					vsf.component.buffer.fifo.pop8
#define vsf_fifo_push					vsf.component.buffer.fifo.push
#define vsf_fifo_pop					vsf.component.buffer.fifo.pop
#define vsf_fifo_get_data_length		vsf.component.buffer.fifo.get_data_length
#define vsf_fifo_get_avail_length		vsf.component.buffer.fifo.get_avail_length

#define vsf_multibuf_init				vsf.component.buffer.multibuf.init
#define vsf_multibuf_get_empty			vsf.component.buffer.multibuf.get_empty
#define vsf_multibuf_push				vsf.component.buffer.multibuf.push
#define vsf_multibuf_get_payload		vsf.component.buffer.multibuf.get_payload
#define vsf_multibuf_pop				vsf.component.buffer.multibuf.pop

#define vsf_bufmgr_malloc_aligned_do	vsf.component.buffer.bufmgr.malloc_aligned_do
#define vsf_bufmgr_free_do				vsf.component.buffer.bufmgr.free_do

#define vsfpool_init					vsf.component.buffer.pool.init
#define vsfpool_alloc					vsf.component.buffer.pool.alloc
#define vsfpool_free					vsf.component.buffer.pool.free
#endif

#ifdef VSFCFG_LIST
#define sllist_is_in					vsf.component.list.is_in
#define sllist_remove					vsf.component.list.remove
#define sllist_append					vsf.component.list.append
#define sllist_delete_next				vsf.component.list.delete_next
#endif

#define BIT_REVERSE_U8					vsf.tool.bittool.bit_reverse_u8
#define BIT_REVERSE_U16					vsf.tool.bittool.bit_reverse_u16
#define BIT_REVERSE_U32					vsf.tool.bittool.bit_reverse_u32
#define BIT_REVERSE_U64					vsf.tool.bittool.bit_reverse_u64
#define GET_U16_MSBFIRST				vsf.tool.bittool.get_u16_msb
#define GET_U24_MSBFIRST				vsf.tool.bittool.get_u24_msb
#define GET_U32_MSBFIRST				vsf.tool.bittool.get_u32_msb
#define GET_U64_MSBFIRST				vsf.tool.bittool.get_u64_msb
#define GET_U16_LSBFIRST				vsf.tool.bittool.get_u16_lsb
#define GET_U24_LSBFIRST				vsf.tool.bittool.get_u24_lsb
#define GET_U32_LSBFIRST				vsf.tool.bittool.get_u32_lsb
#define GET_U64_LSBFIRST				vsf.tool.bittool.get_u64_lsb
#define SET_U16_MSBFIRST				vsf.tool.bittool.set_u16_msb
#define SET_U24_MSBFIRST				vsf.tool.bittool.set_u24_msb
#define SET_U32_MSBFIRST				vsf.tool.bittool.set_u32_msb
#define SET_U64_MSBFIRST				vsf.tool.bittool.set_u64_msb
#define SET_U16_LSBFIRST				vsf.tool.bittool.set_u16_lsb
#define SET_U24_LSBFIRST				vsf.tool.bittool.set_u24_lsb
#define SET_U32_LSBFIRST				vsf.tool.bittool.set_u32_lsb
#define SET_U64_LSBFIRST				vsf.tool.bittool.set_u64_lsb
#define SWAP_U16						vsf.tool.bittool.swap_u16
#define SWAP_U24						vsf.tool.bittool.swap_u24
#define SWAP_U32						vsf.tool.bittool.swap_u32
#define SWAP_U64						vsf.tool.bittool.swap_u64

#define mskarr_set						vsf.tool.bittool.mskarr.set
#define mskarr_clr						vsf.tool.bittool.mskarr.clr
#define mskarr_ffz						vsf.tool.bittool.mskarr.ffz

#else

#define api_ver							vsf.ver
extern const struct vsf_t vsf;

#ifdef VSFCFG_MODULE
struct vsf_module_t* vsf_module_get(char *name);
void vsf_module_register(struct vsf_module_t *module);
void vsf_module_unregister(struct vsf_module_t *module);
void* vsf_module_load(char *name);
void vsf_module_unload(char *name);
#endif
#endif		// VSFCFG_STANDALONE_MODULE

#endif		// __VSF_H_INCLUDED__
