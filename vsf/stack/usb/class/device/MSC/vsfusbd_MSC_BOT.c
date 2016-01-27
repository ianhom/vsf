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

static void vsfusbd_MSCBOT_on_idle(void *p);
static void vsfusbd_MSCBOT_on_data_finish(void *p);

static vsf_err_t vsfusbd_MSCBOT_SendCSW(struct vsfusbd_device_t *device,
							struct vsfusbd_MSCBOT_param_t *param)
{
	struct USBMSC_CSW_t *CSW = &param->CSW;
	uint32_t remain_size;

	CSW->dCSWSignature = SYS_TO_LE_U32(USBMSC_CSW_SIGNATURE);
	CSW->dCSWTag = param->CBW.dCBWTag;
	remain_size = param->CBW.dCBWDataTransferLength;
	if (param->scsi_transact != NULL)
	{
		remain_size -= param->scsi_transact->data_size;
		vsfscsi_release_transact(param->scsi_transact);
	}
	CSW->dCSWDataResidue = SYS_TO_LE_U32(remain_size);

	param->bufstream.buffer.buffer = (uint8_t *)&param->CSW;
	param->bufstream.buffer.size = sizeof(param->CSW);
	param->bufstream.read = true;
	param->stream.user_mem = &param->bufstream;
	param->stream.op = &buffer_stream_op;
	stream_init(&param->stream);

	param->transact.ep = param->ep_in;
	param->transact.data_size = sizeof(param->CSW);
	param->transact.stream = &param->stream;
	param->transact.cb.on_finish = vsfusbd_MSCBOT_on_idle;
	param->transact.cb.param = param;
	return vsfusbd_ep_send(param->device, &param->transact);
}

static vsf_err_t vsfusbd_MSCBOT_ErrHandler(struct vsfusbd_device_t *device,
			struct vsfusbd_MSCBOT_param_t *param,  uint8_t error)
{
	param->CSW.dCSWStatus = error;

	// OUT:	NACK(don't enable_OUT, if OUT is enabled, it will be disabled after data is sent)
	// IN:	if (dCBWDataTransferLength > 0)
	// 		  send_ZLP
	// 		send_CSW
	if (((param->CBW.bmCBWFlags & USBMSC_CBWFLAGS_DIR_MASK) ==
				USBMSC_CBWFLAGS_DIR_IN) &&
		(param->CBW.dCBWDataTransferLength > 0))
	{
		if (param->scsi_transact != NULL)
		{
			param->scsi_transact->data_size = 0;
		}

		param->transact.ep = param->ep_in;
		param->transact.data_size = 0;
		param->transact.stream = NULL;
		param->transact.cb.on_finish = vsfusbd_MSCBOT_on_data_finish;
		param->transact.cb.param = param;
		return vsfusbd_ep_send(param->device, &param->transact);
	}
	else
	{
		vsfusbd_MSCBOT_SendCSW(device, param);
	}

	return VSFERR_NONE;
}

static void vsfusbd_MSCBOT_on_data_finish(void *p)
{
	struct vsfusbd_MSCBOT_param_t *param = (struct vsfusbd_MSCBOT_param_t *)p;
	vsfusbd_MSCBOT_SendCSW(param->device, param);
}

static void vsfusbd_MSCBOT_on_cbw(void *p)
{
	struct vsfusbd_MSCBOT_param_t *param = (struct vsfusbd_MSCBOT_param_t *)p;
	struct vsfusbd_device_t *device = param->device;
	struct vsfscsi_lun_t *lun;

	if (param->transact.data_size ||
		(param->CBW.dCBWSignature != USBMSC_CBW_SIGNATURE) ||
		(param->CBW.bCBWCBLength < 1) || (param->CBW.bCBWCBLength > 16))
	{
		vsfusbd_MSCBOT_on_idle(param);
		return;
	}

	if (param->CBW.bCBWLUN > param->scsi_dev.max_lun)
	{
	reply_failure:
		vsfusbd_MSCBOT_ErrHandler(device, param, USBMSC_CSW_FAIL);
		return;
	}

	param->CSW.dCSWStatus = USBMSC_CSW_OK;
	lun = &param->scsi_dev.lun[param->CBW.bCBWLUN];
	if (vsfscsi_execute(lun, param->CBW.CBWCB))
	{
		goto reply_failure;
	}

	if (param->CBW.dCBWDataTransferLength)
	{
		struct vsfusbd_transact_t *transact = &param->transact;

		if (!lun->transact.data_size)
		{
			goto reply_failure;
		}
		param->scsi_transact = &lun->transact;
		transact->data_size = param->scsi_transact->data_size;
		transact->stream = param->scsi_transact->stream;
		transact->cb.on_finish = vsfusbd_MSCBOT_on_data_finish;
		transact->cb.param = param;

		if ((param->CBW.bmCBWFlags & USBMSC_CBWFLAGS_DIR_MASK) ==
					USBMSC_CBWFLAGS_DIR_IN)
		{
			transact->ep = param->ep_in;
			vsfusbd_ep_send(param->device, transact);
		}
		else
		{
			transact->ep = param->ep_out;
			vsfusbd_ep_recv(param->device, transact);
		}
	}
	else
	{
		if (param->scsi_transact->data_size && param->scsi_transact)
		{
			vsfscsi_cancel_transact(param->scsi_transact);
			param->scsi_transact = NULL;
		}
		vsfusbd_MSCBOT_SendCSW(device, param);
	}
}

static void vsfusbd_MSCBOT_on_idle(void *p)
{
	struct vsfusbd_MSCBOT_param_t *param = (struct vsfusbd_MSCBOT_param_t *)p;

	param->bufstream.buffer.buffer = (uint8_t *)&param->CBW;
	param->bufstream.buffer.size = sizeof(param->CBW);
	param->bufstream.read = false;
	param->stream.user_mem = &param->bufstream;
	param->stream.op = &buffer_stream_op;
	stream_init(&param->stream);

	param->transact.ep = param->ep_out;
	param->transact.data_size = sizeof(param->CBW);
	param->transact.stream = &param->stream;
	param->transact.cb.on_finish = vsfusbd_MSCBOT_on_cbw;
	param->transact.cb.param = param;
	vsfusbd_ep_recv(param->device, &param->transact);
}

vsf_err_t vsfusbd_MSCBOT_class_init(uint8_t iface,
											struct vsfusbd_device_t *device)
{
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	struct vsfusbd_MSCBOT_param_t *param =
		(struct vsfusbd_MSCBOT_param_t *)config->iface[iface].protocol_param;

	param->scsi_transact = NULL;
	param->device = device;
	vsfscsi_init(&param->scsi_dev);
	vsfusbd_MSCBOT_on_idle(param);
	return VSFERR_NONE;
}

vsf_err_t vsfusbd_MSCBOT_request_prepare(struct vsfusbd_device_t *device)
{
	struct interface_usbd_t *drv = device->drv;
	struct vsfusbd_ctrl_handler_t *ctrl_handler = &device->ctrl_handler;
	struct vsf_buffer_t *buffer = &ctrl_handler->bufstream.buffer;
	struct usb_ctrlrequest_t *request = &ctrl_handler->request;
	uint8_t iface = request->wIndex;
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	struct vsfusbd_MSCBOT_param_t *param =
		(struct vsfusbd_MSCBOT_param_t *)config->iface[iface].protocol_param;

	switch (request->bRequest)
	{
	case USB_MSCBOTREQ_GET_MAX_LUN:
		if ((request->wLength != 1) || (request->wValue != 0))
		{
			return VSFERR_FAIL;
		}
		buffer->buffer = &param->scsi_dev.max_lun;
		buffer->size = 1;
		break;
	case USB_MSCBOTREQ_RESET:
		if ((request->wLength != 0) || (request->wValue != 0) ||
			drv->ep.reset_IN_toggle(param->ep_in) ||
			drv->ep.reset_OUT_toggle(param->ep_out) ||
			vsfusbd_MSCBOT_class_init(iface, device))
		{
			return VSFERR_FAIL;
		}
		break;
	default:
		return VSFERR_FAIL;
	}
	ctrl_handler->data_size = buffer->size;
	return VSFERR_NONE;
}

#ifndef VSFCFG_STANDALONE_MODULE
const struct vsfusbd_class_protocol_t vsfusbd_MSCBOT_class =
{
	NULL,
	vsfusbd_MSCBOT_request_prepare, NULL,
	vsfusbd_MSCBOT_class_init, NULL
};
#endif
