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

#include "interfaces.h"

// framework
#include "framework/vsfsm/vsfsm.h"
#include "framework/vsftimer/vsftimer.h"
#include "framework/vsfshell/vsfshell.h"

#include "tool/buffer/buffer.h"
#include "dal/stream/stream.h"

// stack
#include "stack/usb/core/vsfusbd.h"
#include "stack/usb/class/device/CDC/vsfusbd_CDCACM.h"
#include "stack/usb/class/device/HID/vsfusbd_HID.h"

#include "stack/usb/core/vsfusbh.h"
#include "stack/usb/core/hcd/ohci/vsfohci.h"
#include "stack/usb/class/host/HUB/vsfusbh_HUB.h"

#include "stack/tcpip/vsfip.h"
#include "stack/tcpip/proto/dhcp/vsfip_dhcpc.h"
#include "stack/tcpip/proto/dns/vsfip_dnsc.h"
#include "stack/tcpip/proto/http/vsfip_httpd.h"

#include "stack/tcpip/netif/eth/broadcom/bcm_wifi.h"
#include "stack/tcpip/netif/eth/broadcom/bus/bcm_bus.h"

#define VSF_API_VERSION						0x00000001

struct vsf_module_t
{
	char *name;
	void *ifs;
	struct
	{
		void (*on_load)(struct vsf_module_t *me, struct vsf_module_t *new);
		void (*on_unload)(struct vsf_module_t *me, struct vsf_module_t *old);
	} callback;
	struct vsf_module_t *next;
};

struct vsf_t
{
	uint32_t api_ver;
	const struct interfaces_info_t *ifs;
	
	struct vsf_framework_t
	{
		vsf_err_t (*sm_init)(struct vsfsm_t *sm);
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
		
		struct
		{
			vsf_err_t (*load)(struct vsf_module_t *module);
			vsf_err_t (*unload)(struct vsf_module_t *module);
			struct vsf_module_t* (*get)(char *name);
		} module;
		
		struct
		{
			vsf_err_t (*init)(struct vsfshell_t *shell);
			void (*register_handlers)(struct vsfshell_t *shell,
										struct vsfshell_handler_t *handlers);
			void (*free_handler_thread)(struct vsfshell_t *shell, struct vsfsm_t *sm);
		} shell;
		
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
		} stream;
	} framework;
	
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
		struct
		{
			uint8_t (*BIT_REVERSE_U8)(uint8_t);
			uint16_t (*BIT_REVERSE_U16)(uint16_t);
			uint32_t (*BIT_REVERSE_U32)(uint32_t);
			uint64_t (*BIT_REVERSE_U64)(uint64_t);
			
			uint16_t (*GET_U16_MSBFIRST)(uint8_t *p);
			uint32_t (*GET_U24_MSBFIRST)(uint8_t *p);
			uint32_t (*GET_U32_MSBFIRST)(uint8_t *p);
			uint64_t (*GET_U64_MSBFIRST)(uint8_t *p);
			uint16_t (*GET_U16_LSBFIRST)(uint8_t *p);
			uint32_t (*GET_U24_LSBFIRST)(uint8_t *p);
			uint32_t (*GET_U32_LSBFIRST)(uint8_t *p);
			uint64_t (*GET_U64_LSBFIRST)(uint8_t *p);
			
			void (*SET_U16_MSBFIRST)(uint8_t *p, uint16_t v16);
			void (*SET_U24_MSBFIRST)(uint8_t *p, uint32_t v32);
			void (*SET_U32_MSBFIRST)(uint8_t *p, uint32_t v32);
			void (*SET_U64_MSBFIRST)(uint8_t *p, uint64_t v64);
			void (*SET_U16_LSBFIRST)(uint8_t *p, uint16_t v16);
			void (*SET_U24_LSBFIRST)(uint8_t *p, uint32_t v32);
			void (*SET_U32_LSBFIRST)(uint8_t *p, uint32_t v32);
			void (*SET_U64_LSBFIRST)(uint8_t *p, uint64_t v64);
			
			uint16_t (*SWAP_U16)(uint16_t);
			uint32_t (*SWAP_U24)(uint32_t);
			uint32_t (*SWAP_U32)(uint32_t);
			uint64_t (*SWAP_U64)(uint64_t);

			struct
			{
				void (*set)(uint32_t *arr, int bit);
				void (*clr)(uint32_t *arr, int bit);
				int (*ffz)(uint32_t *arr, int arrlen);
			} mskarr;
		} bittool;
	} tool;
	
	struct
	{
		struct
		{
			struct
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
						struct vsfusbd_class_protocol_t *protocol;
					} hid;
					struct
					{
						struct vsfusbd_class_protocol_t *control_protocol;
						struct vsfusbd_class_protocol_t *data_protocol;
					} cdc;
				} classes;
			} device;
			
			struct
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
						struct vsfusbh_hcddrv_t *driver;
					} ohci;
				} hcd;
#endif
				
				struct
				{
					struct
					{
						struct vsfusbh_class_drv_t *driver;
					} hub;
				} classes;
			} host;
		} usb;
		
		struct
		{
			vsf_err_t (*init)(void);
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
			vsf_err_t (*tcp_send)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
						struct vsfip_socket_t *socket, struct vsfip_sockaddr_t *sockaddr,
						struct vsfip_buffer_t *buf, bool flush);
			vsf_err_t (*tcp_recv)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
						struct vsfip_socket_t *socket, struct vsfip_sockaddr_t *sockaddr,
						struct vsfip_buffer_t **buf);
			vsf_err_t (*tcp_close)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
						struct vsfip_socket_t *socket);
			
			vsf_err_t (*udp_send)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
						struct vsfip_socket_t *socket, struct vsfip_sockaddr_t *sockaddr,
						struct vsfip_buffer_t *buf);
			vsf_err_t (*udp_recv)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
						struct vsfip_socket_t *socket, struct vsfip_sockaddr_t *sockaddr,
						struct vsfip_buffer_t **buf);
			
			struct
			{
				struct vsfip_buffer_t* (*get)(uint32_t size);
				void (*reference)(struct vsfip_buffer_t *buf);
				void (*release)(struct vsfip_buffer_t *buf);
			} buffer;
			
			struct
			{
				struct
				{
					vsf_err_t (*start)(struct vsfip_netif_t *netif,
										struct vsfip_dhcpc_t *dhcp);
				} dhcpc;
			} protocol;
			
			struct
			{
				struct
				{
					struct bcm_bus_op_t *spi_op;
					struct bcm_bus_op_t *sdio_op;
				} bus;
				struct vsfip_netdrv_op_t *netdrv_op;
				
				vsf_err_t (*scan)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
							struct vsfip_buffer_t **result, uint8_t interface);
				vsf_err_t (*join)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
							uint32_t *status, const char *ssid,
							enum bcm_authtype_t authtype, const uint8_t *key,
							uint8_t keylen);
				vsf_err_t (*leave)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt);
			} broadcom_wifi;
		} tcpip;
	} stack;
};

#ifdef VSF_STANDALONE_APP

#define VSF_BASE						((struct vsf_t *)VSF_BASE_ADDR)
#define vsf								(*VSF_BASE)
#define api_ver							vsf.api_ver
#define core_interfaces					(*vsf.ifs)

// interfaces constants
#define GPIO_INFLOAT					core_interfaces.gpio.constants.INFLOAT
#define GPIO_INPU						core_interfaces.gpio.constants.INPU
#define GPIO_INPD						core_interfaces.gpio.constants.INPD
#define GPIO_OUTPP						core_interfaces.gpio.constants.OUTPP
#define GPIO_OUTOD						core_interfaces.gpio.constants.OUTOD

#define vsfsm_init						vsf.framework.sm_init
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

#define vsf_module_load					vsf.framework.module.load
#define vsf_module_unload				vsf.framework.module.unload
#define vsf_module_get					vsf.framework.module.get

#define vsfshell_init					vsf.framework.shell.init
#define vsfshell_register_handlers		vsf.framework.shell.register_handlers
#define vsfshell_free_handler_thread	vsf.framework.shell.free_handler_thread

#define stream_init						vsf.framework.stream.init
#define stream_fini						vsf.framework.stream.fini
#define stream_read						vsf.framework.stream.read
#define stream_write					vsf.framework.stream.write
#define stream_get_data_size			vsf.framework.stream.get_data_size
#define stream_get_free_size			vsf.framework.stream.get_free_size
#define stream_connect_rx				vsf.framework.stream.connect_rx
#define stream_connect_tx				vsf.framework.stream.connect_tx

#define vsfq_init						vsf.buffer.queue.init
#define vsfq_append						vsf.buffer.queue.append
#define vsfq_remove						vsf.buffer.queue.remove
#define vsfq_enqueue					vsf.buffer.queue.enqueue
#define vsfq_dequeue					vsf.buffer.queue.dequeue

#define vsf_fifo_init					vsf.buffer.fifo.init
#define vsf_fifo_push8					vsf.buffer.fifo.push8
#define vsf_fifo_pop8					vsf.buffer.fifo.pop8
#define vsf_fifo_push					vsf.buffer.fifo.push
#define vsf_fifo_pop					vsf.buffer.fifo.pop
#define vsf_fifo_get_data_length		vsf.buffer.fifo.get_data_length
#define vsf_fifo_get_avail_length		vsf.buffer.fifo.get_avail_length

#define vsf_bufmgr_malloc				vsf.buffer.bufmgr.malloc
#define vsf_bufmgr_malloc_aligned		vsf.buffer.bufmgr.malloc_aligned
#define vsf_bufmgr_free					vsf.buffer.bufmgr.free

#define BIT_REVERSE_U8					vsf.tool.bittool.BIT_REVERSE_U8
#define BIT_REVERSE_U16					vsf.tool.bittool.BIT_REVERSE_U16
#define BIT_REVERSE_U32					vsf.tool.bittool.BIT_REVERSE_U32
#define BIT_REVERSE_U64					vsf.tool.bittool.BIT_REVERSE_U64
#define GET_U16_MSBFIRST				vsf.tool.bittool.GET_U16_MSBFIRST
#define GET_U24_MSBFIRST				vsf.tool.bittool.GET_U24_MSBFIRST
#define GET_U32_MSBFIRST				vsf.tool.bittool.GET_U32_MSBFIRST
#define GET_U64_MSBFIRST				vsf.tool.bittool.GET_U64_MSBFIRST
#define GET_U16_LSBFIRST				vsf.tool.bittool.GET_U16_LSBFIRST
#define GET_U24_LSBFIRST				vsf.tool.bittool.GET_U24_LSBFIRST
#define GET_U32_LSBFIRST				vsf.tool.bittool.GET_U32_LSBFIRST
#define GET_U64_LSBFIRST				vsf.tool.bittool.GET_U64_LSBFIRST
#define SET_U16_MSBFIRST				vsf.tool.bittool.SET_U16_MSBFIRST
#define SET_U24_MSBFIRST				vsf.tool.bittool.SET_U24_MSBFIRST
#define SET_U32_MSBFIRST				vsf.tool.bittool.SET_U32_MSBFIRST
#define SET_U64_MSBFIRST				vsf.tool.bittool.SET_U64_MSBFIRST
#define SET_U16_LSBFIRST				vsf.tool.bittool.SET_U16_LSBFIRST
#define SET_U24_LSBFIRST				vsf.tool.bittool.SET_U24_LSBFIRST
#define SET_U32_LSBFIRST				vsf.tool.bittool.SET_U32_LSBFIRST
#define SET_U64_LSBFIRST				vsf.tool.bittool.SET_U64_LSBFIRST
#define SWAP_U16						vsf.tool.bittool.SWAP_U16
#define SWAP_U24						vsf.tool.bittool.SWAP_U24
#define SWAP_U32						vsf.tool.bittool.SWAP_U32
#define SWAP_U64						vsf.tool.bittool.SWAP_U64

#define mskarr_set						vsf.tool.bittool.mskarr.set
#define mskarr_clr						vsf.tool.bittool.mskarr.clr
#define mskarr_ffz						vsf.tool.bittool.mskarr.ffz

#define vsfusbd_device_init				vsf.stack.usb.device.init
#define vsfusbd_device_fini				vsf.stack.usb.device.fini
#define vsfusbd_ep_send_nb				vsf.stack.usb.device.ep_send
#define vsfusbd_ep_receive_nb			vsf.stack.usb.device.ep_recv
#define vsfusbd_set_IN_handler			vsf.stack.usb.device.set_IN_handler
#define vsfusbd_set_OUT_handler			vsf.stack.usb.device.set_OUT_handler
#define vsfusbd_HID_class				(*vsf.stack.usb.device.classes.hid.protocol)
#define vsfusbd_CDCACMControl_class		(*vsf.stack.usb.device.classes.cdc.control_protocol)
#define vsfusbd_CDCACMData_class		(*vsf.stack.usb.device.classes.cdc.data_protocol)

#define vsfip_init						vsf.stack.tcpip.init
#define vsfip_fini						vsf.stack.tcpip.fini
#define vsfip_netif_add					vsf.stack.tcpip.netif_add
#define vsfip_netif_remove				vsf.stack.tcpip.netif_remove
#define vsfip_socket					vsf.stack.tcpip.socket
#define vsfip_close						vsf.stack.tcpip.close
#define vsfip_listen					vsf.stack.tcpip.listen
#define vsfip_bind						vsf.stack.tcpip.bind
#define vsfip_tcp_connect				vsf.stack.tcpip.tcp_connect
#define vsfip_tcp_accept				vsf.stack.tcpip.tcp_accept
#define vsfip_tcp_send					vsf.stack.tcpip.tcp_send
#define vsfip_tcp_recv					vsf.stack.tcpip.tcp_recv
#define vsfip_tcp_close					vsf.stack.tcpip.tcp_close
#define vsfip_udp_send					vsf.stack.tcpip.udp_send
#define vsfip_udp_recv					vsf.stack.tcpip.udp_recv

#define vsfip_buffer_get				vsf.stack.tcpip.buffer.get
#define vsfip_buffer_reference			vsf.stack.tcpip.buffer.reference
#define vsfip_buffer_release			vsf.stack.tcpip.buffer.release

#define vsfip_dhcpc_start				vsf.stack.tcpip.protocol.dhcpc.start

#define bcm_bus_spi_op					(*vsf.stack.tcpip.broadcom_wifi.bus.spi_op)
#define bcm_wifi_netdrv_op				(*vsf.stack.tcpip.broadcom_wifi.netdrv_op)
#define bcm_wifi_scan					vsf.stack.tcpip.broadcom_wifi.scan
#define bcm_wifi_join					vsf.stack.tcpip.broadcom_wifi.join
#define bcm_wifi_leave					vsf.stack.tcpip.broadcom_wifi.leave

#else

#define api_ver							vsf.api_ver
extern const struct vsf_t vsf;

#endif		// VSF_STANDALONE_APP

#endif		// __VSF_H_INCLUDED__
