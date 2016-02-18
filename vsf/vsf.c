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

#include "vsf.h"
#include "app_hw_cfg.h"

static void vsfsm_enter_critical_internal(void)
{
	vsf_enter_critical();
}
static void vsfsm_leave_critical_internal(void)
{
	vsf_leave_critical();
}

#ifdef VSFCFG_MODULE
static struct vsf_module_t *modulelist = NULL;

void vsf_module_register(struct vsf_module_t *module)
{
	module->code_buff = NULL;
	module->next = modulelist;
	modulelist = module;
}

void vsf_module_unregister(struct vsf_module_t *module)
{
	if (module == modulelist)
	{
		modulelist = modulelist->next;
	}
	else
	{
		struct vsf_module_t *moduletmp = modulelist;

		while (moduletmp->next != NULL)
		{
			if (moduletmp->next == module)
			{
				moduletmp->next = module->next;
			}
			moduletmp = moduletmp->next;
		}
	}
}

struct vsf_module_t* vsf_module_get(char *name)
{
	struct vsf_module_t *module = modulelist;

	while ((module != NULL) && strcmp(module->flash->name, name))
		module = module->next;
	return module;
}

void vsf_module_unload(char *name)
{
	struct vsf_module_t *module = vsf_module_get(name);
	void (*mod_exit)(struct vsf_module_t *);

	if ((module != NULL) && (module->code_buff != NULL))
	{
		if (module->flash->exit)
		{
			mod_exit = (void (*)(struct vsf_module_t *))
							(module->code_buff + module->flash->exit);
			mod_exit(module);
		}

#ifdef VSFCFG_MODULE_ALLOC_RAM
		if (module->flash->size > 0)
		{
			vsf_bufmgr_free(module->code_buff);
		}
#endif
		module->code_buff = NULL;
	}
}

void* vsf_module_load(char *name)
{
	struct vsf_module_t *module = vsf_module_get(name);
	vsf_err_t (*mod_entry)(struct vsf_module_t *, struct app_hwcfg_t const *);

	if ((module != NULL) && module->flash->entry)
	{
		if (module->code_buff != NULL)
		{
			goto succeed;
		}
#ifdef VSFCFG_MODULE_ALLOC_RAM
		if (module->flash->size > 0)
		{
			module->code_buff = vsf_bufmgr_malloc(module->flash->size);
			if (NULL == module->code_buff)
			{
				goto fail;
			}
			memcpy(module->code_buff, module->flash, module->flash->size);
		}
		else
		{
			module->code_buff = (uint8_t *)module->flash;
		}
#else
		module->code_buff = (uint8_t *)module->flash;
#endif
		mod_entry =
			(vsf_err_t (*)(struct vsf_module_t *, struct app_hwcfg_t const *))
						(module->code_buff + module->flash->entry);

		if (mod_entry(module, &app_hwcfg))
		{
#ifdef VSFCFG_MODULE_ALLOC_RAM
			if (module->flash->size > 0)
			{
				vsf_bufmgr_free(module->code_buff);
			}
#endif
			module->code_buff = NULL;
			goto fail;
		}
succeed:
		return module->ifs;
	}
fail:
	return NULL;
}

#if defined(VSFCFG_FUNC_SHELL) && defined(VSFCFG_MODULE_SHELL)
static struct vsf_shell_api_t vsf_shell_api;
#endif
#if defined(VSFCFG_FUNC_USBD) && defined(VSFCFG_MODULE_USBD)
static struct vsf_usbd_api_t vsf_usbd_api;
#endif
#if defined(VSFCFG_FUNC_USBH) && defined(VSFCFG_MODULE_USBH)
static struct vsf_usbh_api_t vsf_usbh_api;
#endif
#if defined(VSFCFG_FUNC_TCPIP) && defined(VSFCFG_MODULE_TCPIP)
static struct vsf_tcpip_api_t vsf_tcpip_api;
#endif
#if defined(VSFCFG_FUNC_BCMWIFI) && defined(VSFCFG_MODULE_BCMWIFI)
static struct vsf_bcmwifi_api_t vsf_bcmwifi_api;
#endif
#endif		// VSFCFG_MODULE
// reserve 512 bytes for vector table
ROOTFUNC const struct vsf_t vsf @ VSFCFG_API_ADDR =
{
	.ver = VSF_API_VERSION,
	.ifs = &core_interfaces,

	.framework.evtq_init = vsfsm_evtq_init,
	.framework.evtq_set = vsfsm_evtq_set,
	.framework.evtq_get = vsfsm_evtq_get,
	.framework.poll = vsfsm_poll,
	.framework.get_event_pending = vsfsm_get_event_pending,
	.framework.sm_init = vsfsm_init,
	.framework.sm_fini = vsfsm_fini,
#if VSFSM_CFG_PT_EN
	.framework.pt_init = vsfsm_pt_init,
#endif
	.framework.post_evt = vsfsm_post_evt,
	.framework.post_evt_pending = vsfsm_post_evt_pending,
	.framework.enter_critical = vsfsm_enter_critical_internal,
	.framework.leave_critical = vsfsm_leave_critical_internal,
#if VSFSM_CFG_SYNC_EN
	.framework.sync.init = vsfsm_sync_init,
	.framework.sync.cancel = vsfsm_sync_cancel,
	.framework.sync.increase = vsfsm_sync_increase,
	.framework.sync.decrease = vsfsm_sync_decrease,
#endif
	.framework.timer.init = vsftimer_init,
	.framework.timer.create = vsftimer_create,
	.framework.timer.free = vsftimer_free,
	.framework.timer.enqueue = vsftimer_enqueue,
	.framework.timer.dequeue = vsftimer_dequeue,
	.framework.timer.callback = vsftimer_callback_int,
#ifdef VSFCFG_MODULE
	.framework.module.get = vsf_module_get,
	.framework.module.reg = vsf_module_register,
	.framework.module.unreg = vsf_module_unregister,
	.framework.module.load = vsf_module_load,
	.framework.module.unload = vsf_module_unload,
#endif
#ifdef VSFCFG_FUNC_SHELL
#ifdef VSFCFG_MODULE_SHELL
	.framework.shell = &vsf_shell_api,
#else
	.framework.shell.init = vsfshell_init,
	.framework.shell.register_handlers = vsfshell_register_handlers,
	.framework.shell.free_handler_thread = vsfshell_free_handler_thread,
#endif
#endif

#ifdef VSFCFG_BUFFER
	.component.buffer.queue.init = vsfq_init,
	.component.buffer.queue.append = vsfq_append,
	.component.buffer.queue.remove = vsfq_remove,
	.component.buffer.queue.enqueue = vsfq_enqueue,
	.component.buffer.queue.dequeue = vsfq_dequeue,
	.component.buffer.fifo.init = vsf_fifo_init,
	.component.buffer.fifo.push8 = vsf_fifo_push8,
	.component.buffer.fifo.pop8 = vsf_fifo_pop8,
	.component.buffer.fifo.push = vsf_fifo_push,
	.component.buffer.fifo.pop = vsf_fifo_pop,
	.component.buffer.fifo.get_data_length = vsf_fifo_get_data_length,
	.component.buffer.fifo.get_avail_length = vsf_fifo_get_avail_length,
	.component.buffer.bufmgr.malloc_aligned_do = vsf_bufmgr_malloc_aligned_do,
	.component.buffer.bufmgr.free_do = vsf_bufmgr_free_do,
	.component.buffer.pool.init = vsfpool_init,
	.component.buffer.pool.alloc = vsfpool_alloc,
	.component.buffer.pool.free = vsfpool_free,
#endif
#ifdef VSFCFG_STREAM
	.component.stream.init = stream_init,
	.component.stream.fini = stream_fini,
	.component.stream.read = stream_read,
	.component.stream.write = stream_write,
	.component.stream.get_data_size = stream_get_data_size,
	.component.stream.get_free_size = stream_get_free_size,
	.component.stream.connect_rx = stream_connect_rx,
	.component.stream.connect_tx = stream_connect_tx,
	.component.stream.disconnect_rx = stream_disconnect_rx,
	.component.stream.disconnect_tx = stream_disconnect_tx,
	.component.stream.fifostream_op = &fifostream_op,
	.component.stream.mbufstream_op = &mbufstream_op,
	.component.stream.bufstream_op = &bufstream_op,
#endif
#ifdef VSFCFG_MAL
	.component.mal.init = vsfmal_init,
	.component.mal.fini = vsfmal_fini,
	.component.mal.erase_all = vsfmal_erase_all,
	.component.mal.erase = vsfmal_erase,
	.component.mal.read = vsfmal_read,
	.component.mal.write = vsfmal_write,
	.component.mal.malstream.init = vsf_malstream_init,
	.component.mal.malstream.read = vsf_malstream_read,
	.component.mal.malstream.write = vsf_malstream_write,
#ifdef VSFCFG_SCSI
	.component.mal.scsi.init = vsfscsi_init,
	.component.mal.scsi.execute = vsfscsi_execute,
	.component.mal.scsi.cancel_transact = vsfscsi_cancel_transact,
	.component.mal.scsi.release_transact = vsfscsi_release_transact,
	.component.mal.scsi.mal2scsi_op = &vsf_mal2scsi_op,
#endif
#endif
#ifdef VSFCFG_FILE
	.component.file.init = vsfile_init,
	.component.file.mount = vsfile_mount,
	.component.file.unmount = vsfile_unmount,
	.component.file.getfile = vsfile_getfile,
	.component.file.findfirst = vsfile_findfirst,
	.component.file.findnext = vsfile_findnext,
	.component.file.findend = vsfile_findend,
	.component.file.open = vsfile_open,
	.component.file.close = vsfile_close,
	.component.file.read = vsfile_read,
	.component.file.write = vsfile_write,
	.component.file.getfileext = vsfile_getfileext,
	.component.file.is_div = vsfile_is_div,
	.component.file.dummy_file = vsfile_dummy_file,
	.component.file.dummy_rw = vsfile_dummy_rw,
	.component.file.memfs_op = &vsfile_memfs_op,
#endif

	.tool.bittool.bit_reverse_u8 = BIT_REVERSE_U8,
	.tool.bittool.bit_reverse_u16 = BIT_REVERSE_U16,
	.tool.bittool.bit_reverse_u32 = BIT_REVERSE_U32,
	.tool.bittool.bit_reverse_u64 = BIT_REVERSE_U64,
	.tool.bittool.get_u16_msb = GET_U16_MSBFIRST,
	.tool.bittool.get_u24_msb = GET_U24_MSBFIRST,
	.tool.bittool.get_u32_msb = GET_U32_MSBFIRST,
	.tool.bittool.get_u64_msb = GET_U64_MSBFIRST,
	.tool.bittool.get_u16_lsb = GET_U16_LSBFIRST,
	.tool.bittool.get_u24_lsb = GET_U24_LSBFIRST,
	.tool.bittool.get_u32_lsb = GET_U32_LSBFIRST,
	.tool.bittool.get_u64_lsb = GET_U64_LSBFIRST,
	.tool.bittool.set_u16_msb = SET_U16_MSBFIRST,
	.tool.bittool.set_u24_msb = SET_U24_MSBFIRST,
	.tool.bittool.set_u32_msb = SET_U32_MSBFIRST,
	.tool.bittool.set_u64_msb = SET_U64_MSBFIRST,
	.tool.bittool.set_u16_lsb = SET_U16_LSBFIRST,
	.tool.bittool.set_u24_lsb = SET_U24_LSBFIRST,
	.tool.bittool.set_u32_lsb = SET_U32_LSBFIRST,
	.tool.bittool.set_u64_lsb = SET_U64_LSBFIRST,
	.tool.bittool.swap_u16 = SWAP_U16,
	.tool.bittool.swap_u24 = SWAP_U24,
	.tool.bittool.swap_u32 = SWAP_U32,
	.tool.bittool.swap_u64 = SWAP_U64,
	.tool.bittool.mskarr.set = mskarr_set,
	.tool.bittool.mskarr.clr = mskarr_clr,
	.tool.bittool.mskarr.ffz = mskarr_ffz,

#ifdef VSFCFG_FUNC_USBD
#ifdef VSFCFG_MODULE_USBD
	.stack.usb.device = &vsf_usbd_api,
#else
	.stack.usb.device.init = vsfusbd_device_init,
	.stack.usb.device.fini = vsfusbd_device_fini,
	.stack.usb.device.ep_send = vsfusbd_ep_send,
	.stack.usb.device.ep_recv = vsfusbd_ep_recv,
	.stack.usb.device.set_IN_handler = vsfusbd_set_IN_handler,
	.stack.usb.device.set_OUT_handler = vsfusbd_set_OUT_handler,
	.stack.usb.device.classes.hid.protocol = (struct vsfusbd_class_protocol_t *)&vsfusbd_HID_class,
	.stack.usb.device.classes.cdc.control_protocol = (struct vsfusbd_class_protocol_t *)&vsfusbd_CDCControl_class,
	.stack.usb.device.classes.cdc.data_protocol = (struct vsfusbd_class_protocol_t *)&vsfusbd_CDCData_class,
	.stack.usb.device.classes.cdcacm.control_protocol = (struct vsfusbd_class_protocol_t *)&vsfusbd_CDCACMControl_class,
	.stack.usb.device.classes.cdcacm.data_protocol = (struct vsfusbd_class_protocol_t *)&vsfusbd_CDCACMData_class,
#ifdef VSFCFG_FUNC_TCPIP
	.stack.usb.device.classes.rndis.control_protocol = (struct vsfusbd_class_protocol_t *)&vsfusbd_RNDISControl_class,
	.stack.usb.device.classes.rndis.data_protocol = (struct vsfusbd_class_protocol_t *)&vsfusbd_RNDISData_class,
#endif
#ifdef VSFCFG_SCSI
	.stack.usb.device.classes.mscbot.protocol = (struct vsfusbd_class_protocol_t *)&vsfusbd_MSCBOT_class,
#endif
#endif
#endif

#ifdef VSFCFG_FUNC_USBH
#ifdef VSFCFG_MODULE_USBH
	.stack.usb.host = &vsf_usbh_api,
#else
	.stack.usb.host.init = vsfusbh_init,
	.stack.usb.host.fini = vsfusbh_fini,
	.stack.usb.host.register_driver = vsfusbh_register_driver,
#if IFS_HCD_EN
	.stack.usb.host.hcd.ohci.driver = (struct vsfusbh_hcddrv_t *)&vsfusbh_ohci_drv,
#endif
	.stack.usb.host.classes.hub.driver = (struct vsfusbh_class_drv_t *)&vsfusbh_hub_drv,
#endif
#endif

#ifdef VSFCFG_FUNC_TCPIP
#ifdef VSFCFG_MODULE_TCPIP
	.stack.net.tcpip = &vsf_tcpip_api,
#else
	.stack.net.tcpip.init = vsfip_init,
	.stack.net.tcpip.fini = vsfip_fini,
	.stack.net.tcpip.netif_add = vsfip_netif_add,
	.stack.net.tcpip.netif_remove = vsfip_netif_remove,
	.stack.net.tcpip.socket = vsfip_socket,
	.stack.net.tcpip.close = vsfip_close,
	.stack.net.tcpip.listen = vsfip_listen,
	.stack.net.tcpip.bind = vsfip_bind,
	.stack.net.tcpip.tcp_connect = vsfip_tcp_connect,
	.stack.net.tcpip.tcp_accept = vsfip_tcp_accept,
	.stack.net.tcpip.tcp_async_send = vsfip_tcp_async_send,
	.stack.net.tcpip.tcp_send = vsfip_tcp_send,
	.stack.net.tcpip.tcp_async_recv = vsfip_tcp_async_recv,
	.stack.net.tcpip.tcp_recv = vsfip_tcp_recv,
	.stack.net.tcpip.tcp_close = vsfip_tcp_close,
	.stack.net.tcpip.udp_async_send = vsfip_udp_async_send,
	.stack.net.tcpip.udp_send = vsfip_udp_send,
	.stack.net.tcpip.udp_async_recv = vsfip_udp_async_recv,
	.stack.net.tcpip.udp_recv = vsfip_udp_recv,

	.stack.net.tcpip.protocol.dhcpc.start = vsfip_dhcpc_start,
#endif
#endif

#ifdef VSFCFG_FUNC_BCMWIFI
#ifdef VSFCFG_MODULE_BCMWIFI
	.stack.net.bcmwifi = &vsf_bcmwifi_api,
#else
	.stack.net.bcmwifi.bus.spi_op = (struct bcm_bus_op_t *)&bcm_bus_spi_op,
	.stack.net.bcmwifi.bus.sdio_op = (struct bcm_bus_op_t *)&bcm_bus_sdio_op,
	.stack.net.bcmwifi.netdrv_op = (struct vsfip_netdrv_op_t *)&bcm_wifi_netdrv_op,
	.stack.net.bcmwifi.scan = bcm_wifi_scan,
	.stack.net.bcmwifi.join = bcm_wifi_join,
	.stack.net.bcmwifi.leave = bcm_wifi_leave,
#endif
#endif
};
