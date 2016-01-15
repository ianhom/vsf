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

// framework
#include "framework/vsfsm/vsfsm.h"
#include "framework/vsftimer/vsftimer.h"

#include "component/buffer/buffer.h"
#include "component/stream/stream.h"

#define VSF_API_VERSION						0x00000001

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

#ifdef VSFCFG_FUNC_SHELL
#include "framework/vsfshell/vsfshell.h"
struct vsf_shell_api_t
{
	vsf_err_t (*init)(struct vsfshell_t *shell);
	void (*register_handlers)(struct vsfshell_t *shell,
								struct vsfshell_handler_t *handlers);
	void (*free_handler_thread)(struct vsfshell_t *shell, struct vsfsm_t *sm);
};
#endif

#ifdef VSFCFG_FUNC_USBD
#include "stack/usb/core/vsfusbd.h"
#include "stack/usb/class/device/CDC/vsfusbd_CDCACM.h"
#include "stack/usb/class/device/HID/vsfusbd_HID.h"
struct vsf_usbd_api_t
{
	vsf_err_t (*init)(struct vsfusbd_device_t *device);
	vsf_err_t (*fini)(struct vsfusbd_device_t *device);

	vsf_err_t (*ep_send)(struct vsfusbd_device_t *device, uint8_t ep);
	vsf_err_t (*ep_recv)(struct vsfusbd_device_t *device, uint8_t ep);

	vsf_err_t (*set_IN_handler)(struct vsfusbd_device_t *device,
				uint8_t ep, vsf_err_t (*handler)(struct vsfusbd_device_t*, uint8_t));
	vsf_err_t (*set_OUT_handler)(struct vsfusbd_device_t *device,
				uint8_t ep, vsf_err_t (*handler)(struct vsfusbd_device_t*, uint8_t));

	struct
	{
		struct
		{
#if defined(VSFCFG_MODULE_USBD) && defined(VSFCFG_MODULE_ALLOC_RAM)
			struct vsfusbd_class_protocol_t protocol;
#else
			struct vsfusbd_class_protocol_t *protocol;
#endif
		} hid;
		struct
		{
#if defined(VSFCFG_MODULE_USBD) && defined(VSFCFG_MODULE_ALLOC_RAM)
			struct vsfusbd_class_protocol_t control_protocol;
			struct vsfusbd_class_protocol_t data_protocol;
#else
			struct vsfusbd_class_protocol_t *control_protocol;
			struct vsfusbd_class_protocol_t *data_protocol;
#endif
		} cdc;
		struct
		{
#if defined(VSFCFG_MODULE_USBD) && defined(VSFCFG_MODULE_ALLOC_RAM)
			struct vsfusbd_class_protocol_t control_protocol;
			struct vsfusbd_class_protocol_t data_protocol;
#else
			struct vsfusbd_class_protocol_t *control_protocol;
			struct vsfusbd_class_protocol_t *data_protocol;
#endif
		} cdcacm;
	} classes;
};
#endif

#ifdef VSFCFG_FUNC_USBH
#include "stack/usb/core/vsfusbh.h"
#include "stack/usb/core/hcd/ohci/vsfohci.h"
#include "stack/usb/class/host/HUB/vsfusbh_HUB.h"
struct vsf_usbh_api_t
{
	vsf_err_t (*init)(struct vsfusbh_t *usbh);
	vsf_err_t (*fini)(struct vsfusbh_t *usbh);
	vsf_err_t (*register_driver)(struct vsfusbh_t *usbh,
					const struct vsfusbh_class_drv_t *drv);

	vsf_err_t (*submit_urb)(struct vsfusbh_t *usbh,
					struct vsfusbh_urb_t *vsfurb);

#if IFS_HCD_EN
	struct
	{
		struct
		{
#if defined(VSFCFG_MODULE_USBH) && defined(VSFCFG_MODULE_ALLOC_RAM)
			struct vsfusbh_hcddrv_t driver;
#else
			struct vsfusbh_hcddrv_t *driver;
#endif
		} ohci;
	} hcd;
#endif

	struct
	{
		struct
		{
#if defined(VSFCFG_MODULE_USBH) && defined(VSFCFG_MODULE_ALLOC_RAM)
			struct vsfusbh_class_drv_t driver;
#else
			struct vsfusbh_class_drv_t *driver;
#endif
		} hub;
	} classes;
};
#endif

#ifdef VSFCFG_FUNC_TCPIP
#include "stack/tcpip/vsfip.h"
#include "stack/tcpip/proto/dhcp/vsfip_dhcpc.h"
#include "stack/tcpip/proto/dns/vsfip_dnsc.h"
//#include "stack/tcpip/proto/http/vsfip_httpd.h"
struct vsf_tcpip_api_t
{
	vsf_err_t (*init)(struct vsfip_mem_op_t *mem_op);
	vsf_err_t (*fini)(void);

	vsf_err_t (*netif_add)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
							struct vsfip_netif_t *netif);
	vsf_err_t (*netif_remove)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
							struct vsfip_netif_t *netif);

	struct vsfip_socket_t* (*socket)(enum vsfip_sockfamilt_t family,
									enum vsfip_sockproto_t protocol);
	vsf_err_t (*close)(struct vsfip_socket_t *socket);

	vsf_err_t (*listen)(struct vsfip_socket_t *socket, uint8_t backlog);
	vsf_err_t (*bind)(struct vsfip_socket_t *socket, uint16_t port);

	vsf_err_t (*tcp_connect)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
				struct vsfip_socket_t *socket, struct vsfip_sockaddr_t *sockaddr);
	vsf_err_t (*tcp_accept)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
				struct vsfip_socket_t *socket, struct vsfip_socket_t **acceptsocket);
	vsf_err_t (*tcp_async_send)(struct vsfip_socket_t *socket,
				struct vsfip_sockaddr_t *sockaddr, struct vsfip_buffer_t *buf);
	vsf_err_t (*tcp_send)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
				struct vsfip_socket_t *socket, struct vsfip_sockaddr_t *sockaddr,
				struct vsfip_buffer_t *buf, bool flush);
	vsf_err_t (*tcp_async_recv)(struct vsfip_socket_t *socket,
				struct vsfip_sockaddr_t *sockaddr, struct vsfip_buffer_t **buf);
	vsf_err_t (*tcp_recv)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
				struct vsfip_socket_t *socket, struct vsfip_sockaddr_t *sockaddr,
				struct vsfip_buffer_t **buf);
	vsf_err_t (*tcp_close)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
				struct vsfip_socket_t *socket);

	vsf_err_t (*udp_async_send)(struct vsfip_socket_t *socket,
				struct vsfip_sockaddr_t *sockaddr, struct vsfip_buffer_t *buf);
	vsf_err_t (*udp_send)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
				struct vsfip_socket_t *socket, struct vsfip_sockaddr_t *sockaddr,
				struct vsfip_buffer_t *buf);
	vsf_err_t (*udp_async_recv)(struct vsfip_socket_t *socket,
				struct vsfip_sockaddr_t *sockaddr, struct vsfip_buffer_t **buf);
	vsf_err_t (*udp_recv)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
				struct vsfip_socket_t *socket, struct vsfip_sockaddr_t *sockaddr,
				struct vsfip_buffer_t **buf);

	struct
	{
		struct
		{
			vsf_err_t (*start)(struct vsfip_netif_t *netif,
								struct vsfip_dhcpc_t *dhcp);
		} dhcpc;
	} protocol;
};
#endif

#ifdef VSFCFG_FUNC_BCMWIFI
#include "stack/tcpip/netif/eth/broadcom/bcm_wifi.h"
#include "stack/tcpip/netif/eth/broadcom/bus/bcm_bus.h"
struct vsf_bcm_api_t
{
#if defined(VSFCFG_MODULE_BCMWIFI) && defined(VSFCFG_MODULE_ALLOC_RAM)
	struct
	{
#if IFS_SPI_EN
		struct bcm_bus_op_t spi_op;
#endif
#if IFS_SDIO_EN
		struct bcm_bus_op_t sdio_op;
#endif
	} bus;
	struct vsfip_netdrv_op_t netdrv_op;
#else
	struct
	{
#if IFS_SPI_EN
		struct bcm_bus_op_t *spi_op;
#endif
#if IFS_SDIO_EN
		struct bcm_bus_op_t *sdio_op;
#endif
	} bus;
	struct vsfip_netdrv_op_t *netdrv_op;
#endif

	vsf_err_t (*scan)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
				struct vsfip_buffer_t **result, uint8_t interface);
	vsf_err_t (*join)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
				uint32_t *status, const char *ssid,
				enum bcm_authtype_t authtype, const uint8_t *key,
				uint8_t keylen);
	vsf_err_t (*leave)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt);
};
#endif

struct vsf_t
{
	uint32_t ver;
	struct interfaces_info_t const *ifs;

	struct vsf_framework_t
	{
		vsf_err_t (*sm_init)(struct vsfsm_t *sm);
		vsf_err_t (*sm_fini)(struct vsfsm_t *sm);
		vsf_err_t (*pt_init)(struct vsfsm_t *sm, struct vsfsm_pt_t *pt);
		vsf_err_t (*post_evt)(struct vsfsm_t *sm, vsfsm_evt_t evt);
		vsf_err_t (*post_evt_pending)(struct vsfsm_t *sm, vsfsm_evt_t evt);

		void (*enter_critical)(void);
		void (*leave_critical)(void);

		struct
		{
			vsf_err_t (*init)(struct vsfsm_sync_t *sync, uint32_t cur_value,
								uint32_t max_value, vsfsm_evt_t evt);
			vsf_err_t (*cancel)(struct vsfsm_sync_t *sync, struct vsfsm_t *sm);
			vsf_err_t (*increase)(struct vsfsm_sync_t *sync);
			vsf_err_t (*decrease)(struct vsfsm_sync_t *sync,
								struct vsfsm_t *sm);
		} sync;

		struct
		{
			struct vsftimer_t * (*create)(struct vsfsm_t *sm, uint32_t interval,
										int16_t trigger_cnt, vsfsm_evt_t evt);
			void (*free)(struct vsftimer_t *timer);
			void (*enqueue)(struct vsftimer_t *timer);
			void (*dequeue)(struct vsftimer_t *timer);
		} timer;

#ifdef VSFCFG_MODULE
		struct
		{
            void (*reg)(struct vsf_module_t *module);
            void (*unreg)(struct vsf_module_t *module);
			void* (*load)(char *name);
			void (*unload)(char *name);
		} module;
#endif

#ifdef VSFCFG_FUNC_SHELL
#ifdef VSFCFG_MODULE_SHELL
		struct vsf_shell_api_t *shell;
#else
		struct vsf_shell_api_t shell;
#endif
#endif
	} framework;

	struct
	{
		struct
		{
			struct
			{
				void (*init)(struct vsfq_t *q);
				void (*append)(struct vsfq_t *q, struct vsfq_node_t *n);
				void (*remove)(struct vsfq_t *q, struct vsfq_node_t *n);
				void (*enqueue)(struct vsfq_t *q, struct vsfq_node_t *n);
				struct vsfq_node_t* (*dequeue)(struct vsfq_t *q);
			} queue;

			struct
			{
				vsf_err_t (*init)(struct vsf_fifo_t *fifo);
				uint32_t (*push8)(struct vsf_fifo_t *fifo, uint8_t data);
				uint8_t (*pop8)(struct vsf_fifo_t *fifo);
				uint32_t (*push)(struct vsf_fifo_t *fifo, uint32_t size, uint8_t *data);
				uint32_t (*pop)(struct vsf_fifo_t *fifo, uint32_t size, uint8_t *data);
				uint32_t (*get_data_length)(struct vsf_fifo_t *fifo);
				uint32_t (*get_avail_length)(struct vsf_fifo_t *fifo);
			} fifo;

			struct
			{
				void* (*malloc)(uint32_t size);
				void* (*malloc_aligned)(uint32_t size, uint32_t align);
				void (*free)(void *ptr);
			} bufmgr;
		} buffer;

		struct
		{
			vsf_err_t (*init)(struct vsf_stream_t *stream);
			vsf_err_t (*fini)(struct vsf_stream_t *stream);
			uint32_t (*read)(struct vsf_stream_t *stream, struct vsf_buffer_t *buffer);
			uint32_t (*write)(struct vsf_stream_t *stream, struct vsf_buffer_t *buffer);
			uint32_t (*get_data_size)(struct vsf_stream_t *stream);
			uint32_t (*get_free_size)(struct vsf_stream_t *stream);
			void (*connect_rx)(struct vsf_stream_t *stream);
			void (*connect_tx)(struct vsf_stream_t *stream);
			void (*disconnect_rx)(struct vsf_stream_t *stream);
			void (*disconnect_tx)(struct vsf_stream_t *stream);

			const struct vsf_stream_op_t *fifo_stream_op;
		} stream;
	} component;

	struct
	{
		struct
		{
			uint8_t (*bit_reverse_u8)(uint8_t);
			uint16_t (*bit_reverse_u16)(uint16_t);
			uint32_t (*bit_reverse_u32)(uint32_t);
			uint64_t (*bit_reverse_u64)(uint64_t);

			uint16_t (*get_u16_msb)(uint8_t *p);
			uint32_t (*get_u24_msb)(uint8_t *p);
			uint32_t (*get_u32_msb)(uint8_t *p);
			uint64_t (*get_u64_msb)(uint8_t *p);
			uint16_t (*get_u16_lsb)(uint8_t *p);
			uint32_t (*get_u24_lsb)(uint8_t *p);
			uint32_t (*get_u32_lsb)(uint8_t *p);
			uint64_t (*get_u64_lsb)(uint8_t *p);

			void (*set_u16_msb)(uint8_t *p, uint16_t v16);
			void (*set_u24_msb)(uint8_t *p, uint32_t v32);
			void (*set_u32_msb)(uint8_t *p, uint32_t v32);
			void (*set_u64_msb)(uint8_t *p, uint64_t v64);
			void (*set_u16_lsb)(uint8_t *p, uint16_t v16);
			void (*set_u24_lsb)(uint8_t *p, uint32_t v32);
			void (*set_u32_lsb)(uint8_t *p, uint32_t v32);
			void (*set_u64_lsb)(uint8_t *p, uint64_t v64);

			uint16_t (*swap_u16)(uint16_t);
			uint32_t (*swap_u24)(uint32_t);
			uint32_t (*swap_u32)(uint32_t);
			uint64_t (*swap_u64)(uint64_t);

			struct
			{
				void (*set)(uint32_t *arr, int bit);
				void (*clr)(uint32_t *arr, int bit);
				int (*ffz)(uint32_t *arr, int arrlen);
			} mskarr;
		} bittool;
	} tool;

#if defined(VSFCFG_FUNC_TCPIP) || defined(VSFCFG_FUNC_BCMWIFI) || defined(VSFCFG_FUNC_USBD) || defined(VSFCFG_FUNC_USBH)
	struct
	{
#if defined(VSFCFG_FUNC_USBD) || defined(VSFCFG_FUNC_USBH)
		struct
		{
#ifdef VSFCFG_FUNC_USBD
#ifdef VSFCFG_MODULE_USBD
			struct vsf_usbd_api_t *device;
#else
			struct vsf_usbd_api_t device;
#endif
#endif

#ifdef VSFCFG_FUNC_USBH
#ifdef VSFCFG_MODULE_USBH
			struct vsf_usbh_api_t *host;
#else
			struct vsf_usbh_api_t host;
#endif
#endif
		} usb;
#endif

#if defined(VSFCFG_FUNC_TCPIP) || defined(VSFCFG_FUNC_BCMWIFI)
		struct
		{
#ifdef VSFCFG_FUNC_TCPIP
#ifdef VSFCFG_MODULE_TCPIP
			struct vsf_tcpip_api_t *tcpip;
#else
			struct vsf_tcpip_api_t tcpip;
#endif
#endif

#ifdef VSFCFG_FUNC_BCMWIFI
#ifdef VSFCFG_MODULE_BCMWIFI
			struct vsf_bcm_api_t *bcmwifi;
#else
			struct vsf_bcm_api_t bcmwifi;
#endif
#endif
		} net;
#endif
	} stack;
#endif
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
// more interfaces related MACROs here

#define vsfsm_init						vsf.framework.sm_init
#define vsfsm_fini						vsf.framework.sm_fini
#define vsfsm_pt_init					vsf.framework.pt_init
#define vsfsm_post_evt					vsf.framework.post_evt
#define vsfsm_post_evt_pending			vsf.framework.post_evt_pending

#undef vsf_enter_critical
#define vsf_enter_critical				vsf.framework.enter_critical
#undef vsf_leave_critical
#define vsf_leave_critical				vsf.framework.leave_critical

#define vsfsm_sync_init					vsf.framework.sync.init
#define vsfsm_sync_cancel				vsf.framework.sync.cancel
#define vsfsm_sync_increase				vsf.framework.sync.increase
#define vsfsm_sync_decrease				vsf.framework.sync.decrease

#define vsftimer_create					vsf.framework.timer.create
#define vsftimer_free					vsf.framework.timer.free
#define vsftimer_enqueue				vsf.framework.timer.enqueue
#define vsftimer_dequeue				vsf.framework.timer.dequeue

#ifdef VSFCFG_MODULE
#define vsf_module_register				vsf.framework.module.reg
#define vsf_module_unregister			vsf.framework.module.unreg
#define vsf_module_load					vsf.framework.module.load
#define vsf_module_unload				vsf.framework.module.unload
#endif

#ifdef VSFCFG_FUNC_SHELL
#ifdef VSFCFG_MODULE_SHELL
#define vsfshell_init					vsf.framework.shell->init
#define vsfshell_register_handlers		vsf.framework.shell->register_handlers
#define vsfshell_free_handler_thread	vsf.framework.shell->free_handler_thread
#else
#define vsfshell_init					vsf.framework.shell.init
#define vsfshell_register_handlers		vsf.framework.shell.register_handlers
#define vsfshell_free_handler_thread	vsf.framework.shell.free_handler_thread
#endif
#endif

#define stream_init						vsf.component.stream.init
#define stream_fini						vsf.component.stream.fini
#define stream_read						vsf.component.stream.read
#define stream_write					vsf.component.stream.write
#define stream_get_data_size			vsf.component.stream.get_data_size
#define stream_get_free_size			vsf.component.stream.get_free_size
#define stream_connect_rx				vsf.component.stream.connect_rx
#define stream_connect_tx				vsf.component.stream.connect_tx
#define stream_disconnect_rx			vsf.component.stream.disconnect_rx
#define stream_disconnect_tx			vsf.component.stream.disconnect_tx
#define fifo_stream_op					(*vsf.component.stream.fifo_stream_op)

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

#define vsf_bufmgr_malloc				vsf.component.buffer.bufmgr.malloc
#define vsf_bufmgr_malloc_aligned		vsf.component.buffer.bufmgr.malloc_aligned
#define vsf_bufmgr_free					vsf.component.buffer.bufmgr.free

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

#ifdef VSFCFG_FUNC_USBD
#ifdef VSFCFG_MODULE_USBD
#define vsfusbd_device_init				vsf.stack.usb.device->init
#define vsfusbd_device_fini				vsf.stack.usb.device->fini
#define vsfusbd_ep_send_nb				vsf.stack.usb.device->ep_send
#define vsfusbd_ep_receive_nb			vsf.stack.usb.device->ep_recv
#define vsfusbd_set_IN_handler			vsf.stack.usb.device->set_IN_handler
#define vsfusbd_set_OUT_handler			vsf.stack.usb.device->set_OUT_handler
#ifdef VSFCFG_MODULE_ALLOC_RAM
#define vsfusbd_HID_class				vsf.stack.usb.device->classes.hid.protocol
#define vsfusbd_CDCControl_class		vsf.stack.usb.device->classes.cdc.control_protocol
#define vsfusbd_CDCData_class			vsf.stack.usb.device->classes.cdc.data_protocol
#define vsfusbd_CDCACMControl_class		vsf.stack.usb.device->classes.cdcacm.control_protocol
#define vsfusbd_CDCACMData_class		vsf.stack.usb.device->classes.cdcacm.data_protocol
#else
#define vsfusbd_HID_class				(*vsf.stack.usb.device->classes.hid.protocol)
#define vsfusbd_CDCControl_class		(*vsf.stack.usb.device->classes.cdc.control_protocol)
#define vsfusbd_CDCData_class			(*vsf.stack.usb.device->classes.cdc.data_protocol)
#define vsfusbd_CDCACMControl_class		(*vsf.stack.usb.device->classes.cdcacm.control_protocol)
#define vsfusbd_CDCACMData_class		(*vsf.stack.usb.device->classes.cdcacm.data_protocol)
#endif
#else
#define vsfusbd_device_init				vsf.stack.usb.device.init
#define vsfusbd_device_fini				vsf.stack.usb.device.fini
#define vsfusbd_ep_send_nb				vsf.stack.usb.device.ep_send
#define vsfusbd_ep_receive_nb			vsf.stack.usb.device.ep_recv
#define vsfusbd_set_IN_handler			vsf.stack.usb.device.set_IN_handler
#define vsfusbd_set_OUT_handler			vsf.stack.usb.device.set_OUT_handler
#define vsfusbd_HID_class				(*vsf.stack.usb.device.classes.hid.protocol)
#define vsfusbd_CDCControl_class		(*vsf.stack.usb.device.classes.cdc.control_protocol)
#define vsfusbd_CDCData_class			(*vsf.stack.usb.device.classes.cdc.data_protocol)
#define vsfusbd_CDCACMControl_class		(*vsf.stack.usb.device.classes.cdcacm.control_protocol)
#define vsfusbd_CDCACMData_class		(*vsf.stack.usb.device.classes.cdcacm.data_protocol)
#endif
#endif

#ifdef VSFCFG_FUNC_TCPIP
#ifdef VSFCFG_MODULE_TCPIP
#define vsfip_init						vsf.stack.net.tcpip->init
#define vsfip_fini						vsf.stack.net.tcpip->fini
#define vsfip_netif_add					vsf.stack.net.tcpip->netif_add
#define vsfip_netif_remove				vsf.stack.net.tcpip->netif_remove
#define vsfip_socket					vsf.stack.net.tcpip->socket
#define vsfip_close						vsf.stack.net.tcpip->close
#define vsfip_listen					vsf.stack.net.tcpip->listen
#define vsfip_bind						vsf.stack.net.tcpip->bind
#define vsfip_tcp_connect				vsf.stack.net.tcpip->tcp_connect
#define vsfip_tcp_accept				vsf.stack.net.tcpip->tcp_accept
#define vsfip_tcp_send					vsf.stack.net.tcpip->tcp_send
#define vsfip_tcp_recv					vsf.stack.net.tcpip->tcp_recv
#define vsfip_tcp_close					vsf.stack.net.tcpip->tcp_close
#define vsfip_udp_send					vsf.stack.net.tcpip->udp_send
#define vsfip_udp_recv					vsf.stack.net.tcpip->udp_recv
#define vsfip_buffer_get				vsf.stack.net.tcpip->buffer_get
#define vsfip_buffer_reference			vsf.stack.net.tcpip->buffer_reference
#define vsfip_buffer_release			vsf.stack.net.tcpip->buffer_release

#define vsfip_dhcpc_start				vsf.stack.net.tcpip->protocol.dhcpc.start
#else
#define vsfip_init						vsf.stack.net.tcpip.init
#define vsfip_fini						vsf.stack.net.tcpip.fini
#define vsfip_netif_add					vsf.stack.net.tcpip.netif_add
#define vsfip_netif_remove				vsf.stack.net.tcpip.netif_remove
#define vsfip_socket					vsf.stack.net.tcpip.socket
#define vsfip_close						vsf.stack.net.tcpip.close
#define vsfip_listen					vsf.stack.net.tcpip.listen
#define vsfip_bind						vsf.stack.net.tcpip.bind
#define vsfip_tcp_connect				vsf.stack.net.tcpip.tcp_connect
#define vsfip_tcp_accept				vsf.stack.net.tcpip.tcp_accept
#define vsfip_tcp_send					vsf.stack.net.tcpip.tcp_send
#define vsfip_tcp_recv					vsf.stack.net.tcpip.tcp_recv
#define vsfip_tcp_close					vsf.stack.net.tcpip.tcp_close
#define vsfip_udp_send					vsf.stack.net.tcpip.udp_send
#define vsfip_udp_recv					vsf.stack.net.tcpip.udp_recv
#define vsfip_buffer_get				vsf.stack.net.tcpip.buffer_get
#define vsfip_buffer_reference			vsf.stack.net.tcpip.buffer_reference
#define vsfip_buffer_release			vsf.stack.net.tcpip.buffer_release

#define vsfip_dhcpc_start				vsf.stack.net.tcpip.protocol.dhcpc.start
#endif
#endif

#ifdef VSFCFG_FUNC_BCMWIFI
#ifdef VSFCFG_MODULE_BCMWIFI
#ifdef VSFCFG_MODULE_ALLOC_RAM
#define bcm_bus_spi_op					vsf.stack.net.bcmwifi->bus.spi_op
#define bcm_bus_sdio_op					vsf.stack.net.bcmwifi->bus.sdio_op
#define bcm_wifi_netdrv_op				vsf.stack.net.bcmwifi->netdrv_op
#else
#define bcm_bus_spi_op					(*vsf.stack.net.bcmwifi->bus.spi_op)
#define bcm_bus_sdio_op					(*vsf.stack.net.bcmwifi->bus.sdio_op)
#define bcm_wifi_netdrv_op				(*vsf.stack.net.bcmwifi->netdrv_op)
#endif
#define bcm_wifi_scan					vsf.stack.net.bcmwifi->scan
#define bcm_wifi_join					vsf.stack.net.bcmwifi->join
#define bcm_wifi_leave					vsf.stack.net.bcmwifi->leave
#else
#define bcm_bus_spi_op					(*vsf.stack.net.bcmwifi.bus.spi_op)
#define bcm_bus_sdio_op					(*vsf.stack.net.bcmwifi.bus.sdio_op)
#define bcm_wifi_netdrv_op				(*vsf.stack.net.bcmwifi.netdrv_op)
#define bcm_wifi_scan					vsf.stack.net.bcmwifi.scan
#define bcm_wifi_join					vsf.stack.net.bcmwifi.join
#define bcm_wifi_leave					vsf.stack.net.bcmwifi.leave
#endif
#endif

// undefine some MACROs to avoid collisions in some module environment
#ifdef VSFCFG_STANDALONE_MODULE
#include "vsf_undef.h"
#endif

#else

#define api_ver							vsf.ver
extern const struct vsf_t vsf;

#ifdef VSFCFG_MODULE
void vsf_module_register(struct vsf_module_t *module);
void vsf_module_unregister(struct vsf_module_t *module);
void* vsf_module_load(char *name);
void vsf_module_unload(char *name);
#endif
#endif		// VSFCFG_STANDALONE_MODULE

#endif		// __VSF_H_INCLUDED__
