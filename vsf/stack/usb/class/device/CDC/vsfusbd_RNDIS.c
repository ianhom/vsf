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

void vsfusbd_RNDIS_on_tx_finish(void *param)
{
	struct vsfusbd_RNDIS_param_t *rndis_param =
						(struct vsfusbd_RNDIS_param_t *)param;
	struct vsf_bufstream_t *bufstream = &rndis_param->tx_bufstream;

	if (!STREAM_GET_DATA_SIZE(bufstream))
	{
		struct vsfq_node_t *node = vsfq_dequeue(&rndis_param->netif.outq);
		struct vsfip_buffer_t *tmpbuf =
					container_of(node, struct vsfip_buffer_t, proto_node);

		if (rndis_param->tx_buffer != NULL)
		{
			vsfip_buffer_release(rndis_param->tx_buffer);
			rndis_param->tx_buffer = NULL;
		}

		if (tmpbuf != NULL)
		{
			rndis_param->tx_buffer = tmpbuf;
			bufstream->mem.buffer = tmpbuf->buf;
			STREAM_INIT(bufstream);
		}
	}
}

void vsfusbd_RNDIS_on_rx_finish(void *param)
{
	struct vsfusbd_RNDIS_param_t *rndis_param =
						(struct vsfusbd_RNDIS_param_t *)param;

	if (rndis_param->rx_buffer != NULL)
	{
		if (rndis_param->netif_inited)
		{
			vsfip_eth_input(rndis_param->rx_buffer);
		}
		else
		{
			vsfip_buffer_release(rndis_param->rx_buffer);
		}
		rndis_param->rx_buffer = NULL;
	}

	rndis_param->rx_buffer = vsfip_buffer_get(VSFIP_BUFFER_SIZE);
	if (NULL == rndis_param->rx_buffer)
	{
		rndis_param->statistics.rx_nobuf++;
		// will not receive any more later
	}
	else
	{
		struct vsf_bufstream_t *bufstream = &rndis_param->rx_bufstream;
		bufstream->mem.buffer = rndis_param->rx_buffer->buf;
		STREAM_INIT(bufstream);
	}
}

// netdrv
static struct vsfsm_state_t*
vsfusbd_RNDIS_evt_handler(struct vsfsm_t *sm, vsfsm_evt_t evt)
{
	struct vsfusbd_RNDIS_param_t *param =
						(struct vsfusbd_RNDIS_param_t *)sm->user_data;

	switch (evt)
	{
	case VSFSM_EVT_INIT:
	pend_loop:
		if (vsfsm_sem_pend(&param->netif.output_sem, sm))
		{
			break;
		}
	case VSFSM_EVT_USER_LOCAL:
		if (NULL == param->tx_buffer)
		{
			vsfusbd_RNDIS_on_tx_finish(param);
		}
		goto pend_loop;
	}
	return NULL;
}

vsf_err_t vsfusbd_RNDIS_netdrv_init(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfusbd_RNDIS_param_t *param =
						(struct vsfusbd_RNDIS_param_t *)pt->user_data;
	struct vsfip_netif_t *netif = &param->netif;

	netif->macaddr = param->mac;
	netif->mac_broadcast.size = param->mac.size;
	memset(netif->mac_broadcast.addr.s_addr_buf, 0xFF, param->mac.size);
	netif->mtu = VSFIP_CFG_MTU;
	netif->drv->netif_header_size = VSFIP_ETH_HEADSIZE;
	netif->drv->drv_header_size = 0;
	netif->drv->hwtype = VSFIP_ETH_HWTYPE;

	// start pt to receive output_sem
	param->sm.init_state.evt_handler = vsfusbd_RNDIS_evt_handler;
	param->sm.user_data = param;
	return vsfsm_init(&param->sm);
}

vsf_err_t vsfusbd_RNDIS_netdrv_fini(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	((struct vsfusbd_RNDIS_param_t *)pt->user_data)->netif_inited = false;
	return VSFERR_NONE;
}

vsf_err_t vsfusbd_RNDIS_netdrv_header(struct vsfip_buffer_t *buf,
			enum vsfip_netif_proto_t proto, const struct vsfip_macaddr_t *dest)
{
	return VSFERR_NONE;
}

static struct vsfip_netdrv_op_t vsfusbd_RNDIS_netdrv_op =
{
	vsfusbd_RNDIS_netdrv_init, vsfusbd_RNDIS_netdrv_fini,
	vsfusbd_RNDIS_netdrv_header
};

// rndis
const enum oid_t vsfusbd_RNDIS_OID_Supportlist[VSFUSBD_RNDIS_CFG_OIDNUM] =
{
	OID_GEN_SUPPORTED_LIST,
	OID_GEN_HARDWARE_STATUS,
	OID_GEN_MEDIA_SUPPORTED,
	OID_GEN_MEDIA_IN_USE,
	OID_GEN_MAXIMUM_FRAME_SIZE,
	OID_GEN_LINK_SPEED,
	OID_GEN_TRANSMIT_BLOCK_SIZE,
	OID_GEN_RECEIVE_BLOCK_SIZE,
	OID_GEN_VENDOR_ID,
	OID_GEN_VENDOR_DESCRIPTION,
	OID_GEN_VENDOR_DRIVER_VERSION,
	OID_GEN_CURRENT_PACKET_FILTER,
	OID_GEN_MAXIMUM_TOTAL_SIZE,
	OID_GEN_PROTOCOL_OPTIONS,
	OID_GEN_MAC_OPTIONS,
	OID_GEN_MEDIA_CONNECT_STATUS,
	OID_GEN_MAXIMUM_SEND_PACKETS,
	OID_802_3_PERMANENT_ADDRESS,
	OID_802_3_CURRENT_ADDRESS,
	OID_802_3_MULTICAST_LIST,
	OID_802_3_MAXIMUM_LIST_SIZE,
	OID_802_3_MAC_OPTIONS,
};

static vsf_err_t vsfusbd_RNDIS_on_encapsulated_command(
			struct vsfusbd_CDC_param_t *param, struct vsf_buffer_t *buffer)
{
	struct vsfusbd_RNDIS_param_t *rndis_param =
						(struct vsfusbd_RNDIS_param_t *)param;
	uint8_t *replybuf = param->encapsulated_response.buffer;
	uint32_t replylen = 0;

	switch (((struct rndis_msghead_t *)buffer->buffer)->MessageType.msg)
	{
	case RNDIS_INITIALIZE_MSG:
		{
			struct rndis_initialize_msg_t *msg =
						(struct rndis_initialize_msg_t *)buffer->buffer;
			struct rndis_initialize_cmplt_t *reply =
						(struct rndis_initialize_cmplt_t *)replybuf;

			reply->reply.request.head.MessageType.value = RNDIS_INITIALIZE_CMPLT;
			reply->reply.request.RequestId = msg->request.RequestId;
			reply->reply.status.value = NDIS_STATUS_SUCCESS;
			reply->MajorVersion = RNDIS_MAJOR_VERSION;
			reply->MinorVersion = RNDIS_MINOR_VERSION;
			reply->DeviceFlags.value = RNDIS_DF_CONNECTIONLESS;
			reply->Medium.value = NDIS_MEDIUM_802_3;
			reply->MaxPacketsPerMessage = 1;
			reply->MaxTransferSize = VSFIP_BUFFER_SIZE;
			reply->PacketAlignmentFactor = 0;
			reply->AFListOffset = 0;
			reply->AFListSize = 0;
			rndis_param->rndis_state = VSFUSBD_RNDIS_INITED;
			replylen = sizeof(struct rndis_initialize_cmplt_t);

			// prepare vsfip buffer and connect stream
			vsfusbd_CDCData_connect(param);
			stream_connect_tx(param->stream_tx);
			stream_connect_rx(param->stream_rx);
		}
		break;
	case RNDIS_HALT_MSG:
		break;
	case RNDIS_QUERY_MSG:
		{
			struct rndis_query_msg_t *msg =
						(struct rndis_query_msg_t *)buffer->buffer;
			struct rndis_query_cmplt_t *reply =
						(struct rndis_query_cmplt_t *)replybuf;
			enum ndis_status_t status = NDIS_STATUS_SUCCESS;
			uint32_t tmp32;
			uint8_t *replybuf;

			switch (msg->Oid.oid)
			{
			case OID_GEN_SUPPORTED_LIST:		replylen = 4 * VSFUSBD_RNDIS_CFG_OIDNUM; replybuf = (uint8_t *)vsfusbd_RNDIS_OID_Supportlist; break;
			case OID_802_3_MULTICAST_LIST:
			case OID_802_3_MAC_OPTIONS:			status = NDIS_STATUS_NOT_SUPPORTED;
			case OID_GEN_HARDWARE_STATUS:
			case OID_GEN_MAC_OPTIONS:
			case OID_GEN_RNDIS_CONFIG_PARAMETER:
			case OID_802_3_RCV_ERROR_ALIGNMENT:
			case OID_802_3_XMIT_ONE_COLLISION:
			case OID_802_3_XMIT_MORE_COLLISION:	tmp32 = SYS_TO_LE_U32(0);
			reply_tmp32:
												replylen = 4; replybuf = (uint8_t *)&tmp32; break;
			case OID_GEN_MEDIA_SUPPORTED:
			case OID_GEN_MEDIA_IN_USE:
			case OID_GEN_PHYSICAL_MEDIUM:		tmp32 = SYS_TO_LE_U32(NDIS_MEDIUM_802_3); goto reply_tmp32;
			case OID_GEN_MAXIMUM_FRAME_SIZE:	tmp32 = SYS_TO_LE_U32(VSFIP_CFG_MTU); goto reply_tmp32;
			case OID_GEN_LINK_SPEED:			tmp32 = SYS_TO_LE_U32(2500); goto reply_tmp32;
			case OID_GEN_TRANSMIT_BLOCK_SIZE:
			case OID_GEN_RECEIVE_BLOCK_SIZE:
			case OID_GEN_MAXIMUM_TOTAL_SIZE:	tmp32 = SYS_TO_LE_U32(VSFIP_BUFFER_SIZE); goto reply_tmp32;
			case OID_GEN_VENDOR_ID:				tmp32 = SYS_TO_LE_U32(0x00FFFFFF); goto reply_tmp32;
			case OID_GEN_VENDOR_DESCRIPTION:	replybuf = "vsfusbd_rndis"; replylen = strlen((char *)replybuf) + 1; break;
			case OID_GEN_VENDOR_DRIVER_VERSION:	tmp32 = SYS_TO_LE_U32(0x00001000); goto reply_tmp32;
			case OID_GEN_CURRENT_PACKET_FILTER:	tmp32 = SYS_TO_LE_U32(rndis_param->oid_packet_filter); goto reply_tmp32;
//			case OID_GEN_PROTOCOL_OPTIONS:
			case OID_GEN_MEDIA_CONNECT_STATUS:	tmp32 = SYS_TO_LE_U32(NDIS_MEDIA_STATE_CONNECTED); goto reply_tmp32;
//			case OID_GEN_MAXIMUM_SEND_PACKETS:	
			case OID_GEN_XMIT_OK:				tmp32 = SYS_TO_LE_U32(rndis_param->statistics.txok); goto reply_tmp32;
			case OID_GEN_RCV_OK:				tmp32 = SYS_TO_LE_U32(rndis_param->statistics.rxok); goto reply_tmp32;
			case OID_GEN_XMIT_ERROR:			tmp32 = SYS_TO_LE_U32(rndis_param->statistics.txbad); goto reply_tmp32;
			case OID_GEN_RCV_ERROR:				tmp32 = SYS_TO_LE_U32(rndis_param->statistics.rxbad); goto reply_tmp32;
			case OID_GEN_RCV_NO_BUFFER:			tmp32 = SYS_TO_LE_U32(rndis_param->statistics.rx_nobuf); goto reply_tmp32;
			case OID_802_3_PERMANENT_ADDRESS:
			case OID_802_3_CURRENT_ADDRESS:		replybuf = rndis_param->mac.addr.s_addr_buf; replylen = rndis_param->mac.size; break;
			case OID_802_3_MAXIMUM_LIST_SIZE:	tmp32 = SYS_TO_LE_U32(1); goto reply_tmp32;
			}
			if (replylen)
			{
				reply->InformationBufferLength = replylen;
				memcpy((uint8_t *)(reply + 1), replybuf, replylen);
				replylen += sizeof(*reply);
				reply->reply.request.head.MessageType.value = RNDIS_QUERY_CMPLT;
				reply->reply.request.head.MessageLength = replylen;
				reply->reply.request.RequestId = msg->request.RequestId;
				reply->reply.status.value = status;
				reply->InformationBufferOffset = 16;
			}
		}
		break;
	case RNDIS_SET_MSG:
		{
			struct rndis_set_msg_t *msg =
						(struct rndis_set_msg_t *)buffer->buffer;
			struct rndis_set_cmplt_t *reply =
						(struct rndis_set_cmplt_t *)replybuf;
			uint8_t *ptr = (uint8_t *)
						&msg->request.RequestId + msg->InformationBufferOffset;

			reply->reply.request.head.MessageType.value = RNDIS_SET_CMPLT;
			reply->reply.request.head.MessageLength = sizeof(*reply);
			replylen = reply->reply.request.head.MessageLength;
			reply->reply.request.RequestId = msg->request.RequestId;
			reply->reply.status.value = NDIS_STATUS_SUCCESS;
			switch (msg->Oid.oid)
			{
			case OID_GEN_RNDIS_CONFIG_PARAMETER:
				break;
			case OID_GEN_CURRENT_PACKET_FILTER:
				rndis_param->oid_packet_filter = GET_LE_U32(ptr);
				if (rndis_param->oid_packet_filter)
				{
					struct vsfsm_pt_t pt;
					rndis_param->rndis_state = VSFUSBD_RNDIS_DATA_INITED;
					// connect netif, for RNDIS netif_add is non-block
					pt.state = 0;
					pt.sm = 0;
					pt.user_data = rndis_param;
					vsfip_netif_add(&pt, 0, &rndis_param->netif);
				}
				else
				{
					rndis_param->rndis_state = VSFUSBD_RNDIS_INITED;
				}
				break;
			case OID_GEN_CURRENT_LOOKAHEAD:
				break;
			case OID_GEN_PROTOCOL_OPTIONS:
				break;
			case OID_802_3_MULTICAST_LIST:
				break;
			case OID_PNP_ADD_WAKE_UP_PATTERN:
			case OID_PNP_REMOVE_WAKE_UP_PATTERN:
			case OID_PNP_ENABLE_WAKE_UP:
			default:
				reply->reply.status.value = NDIS_STATUS_FAILURE;
			}
		}
		break;
	case RNDIS_RESET_MSG:
		{
			struct rndis_reset_cmplt_t *reply =
						(struct rndis_reset_cmplt_t *)replybuf;

			reply->head.MessageType.value = RNDIS_RESET_CMPLT;
			reply->head.MessageLength = sizeof(*reply);
			replylen = reply->head.MessageLength;
			reply->status.value = NDIS_STATUS_SUCCESS;
			reply->AddressingReset = 1;
			rndis_param->rndis_state = VSFUSBD_RNDIS_UNINITED;

			// free all vsfip buffer and reset stream
		}
		break;
	case RNDIS_INDICATE_STATUS_MSG:
		break;
	case RNDIS_KEEPALIVE_MSG:
		{
			struct rndis_keepalive_msg_t *msg =
						(struct rndis_keepalive_msg_t *)buffer->buffer;
			struct rndis_keepalive_cmplt_t *reply =
						(struct rndis_keepalive_cmplt_t *)replybuf;

			reply->reply.request.head.MessageType.value = RNDIS_KEEPALIVE_CMPLT;
			reply->reply.request.head.MessageLength = sizeof(*reply);
			reply->reply.request.RequestId = msg->request.RequestId;
			replylen = reply->reply.request.head.MessageLength;
			reply->reply.status.value = NDIS_STATUS_SUCCESS;
		}
		break;
	}
	if (replylen)
	{
		struct interface_usbd_t *drv = param->device->drv;
		uint8_t ep_notify = param->ep_notify;
		uint8_t notify_buf[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

		drv->ep.write_IN_buffer(ep_notify, notify_buf, sizeof(notify_buf));
		drv->ep.set_IN_count(ep_notify, sizeof(notify_buf));
	}
	return VSFERR_NONE;
}

vsf_err_t
vsfusbd_RNDISData_class_init(uint8_t iface, struct vsfusbd_device_t *device)
{
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	struct vsfusbd_RNDIS_param_t *param =
		(struct vsfusbd_RNDIS_param_t *)config->iface[iface].protocol_param;

	// private init
	param->netdrv.op = &vsfusbd_RNDIS_netdrv_op;
	param->netif.drv = &param->netdrv;
	param->netif_inited = false;
	param->tx_buffer = param->rx_buffer = NULL;
	param->rndis_state = VSFUSBD_RNDIS_UNINITED;
	param->statistics.rxok = 0;
	param->statistics.rxok = 0;
	param->statistics.txbad = 0;
	param->statistics.rxbad = 0;
	param->statistics.rx_nobuf = 0;

	// stream init
	memset(&param->tx_bufstream, 0, sizeof(param->tx_bufstream));
	param->tx_bufstream.stream.op = &bufstream_op;
	param->tx_bufstream.stream.callback_tx.param = param;
	param->tx_bufstream.mem.read = true;
	param->CDCACM.CDC.stream_tx = (struct vsf_stream_t *)&param->tx_bufstream;
	param->CDCACM.CDC.callback.on_tx_finish = vsfusbd_RNDIS_on_tx_finish;
	memset(&param->rx_bufstream, 0, sizeof(param->rx_bufstream));
	param->rx_bufstream.stream.op = &bufstream_op;
	param->rx_bufstream.stream.callback_rx.param = param;
	param->rx_bufstream.mem.read = false;
	param->CDCACM.CDC.callback.on_rx_finish = vsfusbd_RNDIS_on_rx_finish;
	param->CDCACM.CDC.stream_rx = (struct vsf_stream_t *)&param->rx_bufstream;

	// CDCACM and CDC init
	param->CDCACM.CDC.callback.send_encapsulated_command =
								vsfusbd_RNDIS_on_encapsulated_command;
	param->CDCACM.CDC.encapsulated_command.buffer = param->encapsulated_buf;
	param->CDCACM.CDC.encapsulated_command.size = sizeof(param->encapsulated_buf);
	param->CDCACM.CDC.encapsulated_response.buffer = param->encapsulated_buf;
	param->CDCACM.CDC.encapsulated_response.size = 0;
	return vsfusbd_CDCACMData_class.init(iface, device);
}

#ifndef VSFCFG_STANDALONE_MODULE
const struct vsfusbd_class_protocol_t vsfusbd_RNDISControl_class =
{
	NULL, NULL, NULL, NULL, NULL
};

const struct vsfusbd_class_protocol_t vsfusbd_RNDISData_class =
{
	NULL, NULL, NULL, vsfusbd_RNDISData_class_init, NULL
};
#endif
