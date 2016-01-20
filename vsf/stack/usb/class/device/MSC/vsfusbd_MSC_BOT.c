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

static uint16_t vsfusbd_MSCBOT_GetPkgSize(struct interface_usbd_t *drv, bool in,
											uint8_t ep, uint32_t size)
{
	uint16_t epsize = in ? drv->ep.get_IN_epsize(ep) :
							drv->ep.get_OUT_epsize(ep);
	return min(epsize, size);
}

static vsf_err_t vsfusbd_MSCBOT_SendCSW(struct vsfusbd_device_t *device,
							struct vsfusbd_MSCBOT_param_t *param)
{
	struct interface_usbd_t *drv = device->drv;
	struct USBMSC_CSW_t CSW;

	CSW.dCSWSignature = SYS_TO_LE_U32(USBMSC_CSW_SIGNATURE);
	CSW.dCSWTag = param->CBW.dCBWTag;
	CSW.dCSWDataResidue = SYS_TO_LE_U32(
			param->CBW.dCBWDataTransferLength - param->data_size);
	CSW.dCSWStatus = param->dCSWStatus;

	param->bot_status = VSFUSBD_MSCBOT_STATUS_CSW;
	drv->ep.write_IN_buffer(param->ep_in, (uint8_t *)&CSW, USBMSC_CSW_SIZE);
	drv->ep.set_IN_count(param->ep_in, USBMSC_CSW_SIZE);
	param->scsi_transact = NULL;
	return VSFERR_NONE;
}

static vsf_err_t vsfusbd_MSCBOT_ErrHandler(struct vsfusbd_device_t *device,
			struct vsfusbd_MSCBOT_param_t *param,  uint8_t error)
{
	param->dCSWStatus = error;

	// OUT:	NACK(don't enable_OUT, if OUT is enabled, it will be disabled after data is sent)
	// IN:	if (dCBWDataTransferLength > 0)
	// 		  send_ZLP
	// 		send_CSW
	if (((param->CBW.bmCBWFlags & USBMSC_CBWFLAGS_DIR_MASK) ==
				USBMSC_CBWFLAGS_DIR_IN) &&
		(param->CBW.dCBWDataTransferLength > 0))
	{
		param->data_size = 0;
		param->bot_status = VSFUSBD_MSCBOT_STATUS_IN;
		device->drv->ep.set_IN_count(param->ep_in, 0);
	}
	else
	{
		vsfusbd_MSCBOT_SendCSW(device, param);
	}

	return VSFERR_NONE;
}

static vsf_err_t vsfusbd_MSCBOT_IN_hanlder(struct vsfusbd_device_t *device,
											uint8_t ep)
{
	struct interface_usbd_t *drv = device->drv;
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	int8_t iface = config->ep_OUT_iface_map[ep];
	struct vsfusbd_MSCBOT_param_t *param = NULL;
	uint16_t pkg_size;
	uint8_t buffer_mem[64];

	if (iface < 0)
	{
		return VSFERR_FAIL;
	}
	param = (struct vsfusbd_MSCBOT_param_t *)config->iface[iface].protocol_param;
	if (NULL == param)
	{
		return VSFERR_FAIL;
	}

	switch (param->bot_status)
	{
	case VSFUSBD_MSCBOT_STATUS_ERROR:
	case VSFUSBD_MSCBOT_STATUS_CSW:
		param->bot_status = VSFUSBD_MSCBOT_STATUS_IDLE;
		drv->ep.enable_OUT(param->ep_out);
		break;
	case VSFUSBD_MSCBOT_STATUS_IN:
		if (param->data_size > 0)
		{
			pkg_size = vsfusbd_MSCBOT_GetPkgSize(drv, 1, ep, param->data_size);
			if (stream_get_data_size(param->scsi_transact->stream) >= pkg_size)
			{
				struct vsf_buffer_t buffer =
				{
					.buffer = buffer_mem,
					.size = pkg_size,
				};
				stream_read(param->scsi_transact->stream, &buffer);
				drv->ep.write_IN_buffer(param->ep_in, buffer_mem, pkg_size);
				drv->ep.set_IN_count(param->ep_in, pkg_size);
				param->data_size -= pkg_size;
				param->usb_idle = false;
			}
			else
			{
				param->usb_idle = true;
			}
			return VSFERR_NONE;
		}

		return vsfusbd_MSCBOT_SendCSW(device, param);
	default:
		return VSFERR_FAIL;
	}
	return VSFERR_NONE;
}

static void vsfusbd_MSCBOT_stream_on_in(void *p)
{
	struct vsfusbd_MSCBOT_param_t *param = (struct vsfusbd_MSCBOT_param_t *)p;

	if (param->usb_idle)
	{
		vsfusbd_MSCBOT_IN_hanlder(param->device, param->ep_in);
	}
}

static void vsfusbd_MSCBOT_stream_on_out(void *p)
{
	struct vsfusbd_MSCBOT_param_t *param = (struct vsfusbd_MSCBOT_param_t *)p;

	if (param->usb_idle)
	{
		param->device->drv->ep.enable_OUT(param->ep_out);
	}
}

static vsf_err_t vsfusbd_MSCBOT_OUT_hanlder(struct vsfusbd_device_t *device,
											uint8_t ep)
{
	struct interface_usbd_t *drv = device->drv;
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	int8_t iface = config->ep_OUT_iface_map[ep];
	struct vsfusbd_MSCBOT_param_t *param = NULL;
	uint16_t pkg_size, ep_size;
	uint8_t buffer_mem[64];

	if (iface < 0)
	{
		return VSFERR_FAIL;
	}
	param = (struct vsfusbd_MSCBOT_param_t *)config->iface[iface].protocol_param;
	if (NULL == param)
	{
		return VSFERR_FAIL;
	}

	ep_size = drv->ep.get_OUT_epsize(ep);
	pkg_size = drv->ep.get_OUT_count(ep);
	if (pkg_size > ep_size)
	{
		return VSFERR_FAIL;
	}
	drv->ep.read_OUT_buffer(ep, buffer_mem, pkg_size);

	switch (param->bot_status)
	{
	case VSFUSBD_MSCBOT_STATUS_IDLE:
		memcpy(&param->CBW, buffer_mem, USBMSC_CBW_SIZE);

		if ((param->CBW.dCBWSignature != USBMSC_CBW_SIGNATURE) ||
			(param->CBW.bCBWCBLength < 1) || (param->CBW.bCBWCBLength > 16) ||
			(param->CBW.bCBWLUN >= param->scsi_dev.max_lun) ||
			(pkg_size != USBMSC_CBW_SIZE))
		{
			// TODO: invalid CBW, how to process this error?
			return VSFERR_FAIL;
		}

		param->dCSWStatus = USBMSC_CSW_OK;
		if (vsfscsi_execute_nb(&param->scsi_dev.lun[param->CBW.bCBWLUN],
					param->CBW.CBWCB, &param->data_size, &param->scsi_transact))
		{
			vsfusbd_MSCBOT_ErrHandler(device, param, USBMSC_CSW_FAIL);
			return VSFERR_FAIL;
		}

		if (param->CBW.dCBWDataTransferLength)
		{
			struct vsf_stream_t *stream;

			if (!param->data_size || !param->scsi_transact)
			{
				vsfusbd_MSCBOT_ErrHandler(device, param, USBMSC_CSW_FAIL);
				return VSFERR_FAIL;
			}
			stream = param->scsi_transact->stream;

			if ((param->CBW.bmCBWFlags & USBMSC_CBWFLAGS_DIR_MASK) ==
						USBMSC_CBWFLAGS_DIR_IN)
			{
				stream->callback_rx.param = param;
				stream->callback_rx.on_in_int = vsfusbd_MSCBOT_stream_on_in;
				stream->callback_rx.on_connect_tx = NULL;
				stream->callback_rx.on_disconnect_tx = NULL;
				stream_connect_rx(stream);

				param->bot_status = VSFUSBD_MSCBOT_STATUS_IN;
				return vsfusbd_MSCBOT_IN_hanlder(device, param->ep_in);
			}
			else
			{
				stream->callback_tx.param = param;
				stream->callback_tx.on_out_int = vsfusbd_MSCBOT_stream_on_out;
				stream->callback_tx.on_connect_rx = NULL;
				stream->callback_tx.on_disconnect_rx = NULL;
				stream_connect_tx(stream);

				param->usb_idle = false;
				param->bot_status = VSFUSBD_MSCBOT_STATUS_OUT;
				return drv->ep.enable_OUT(param->ep_out);
			}
		}
		else
		{
			if (param->data_size && param->scsi_transact)
			{
				vsfscsi_cancel_transact(param->scsi_transact);
				param->scsi_transact = NULL;
			}
			return vsfusbd_MSCBOT_SendCSW(device, param);
		}
		break;
	case VSFUSBD_MSCBOT_STATUS_OUT:
		if (param->data_size)
		{
			struct vsf_buffer_t buffer =
			{
				.buffer = buffer_mem,
				.size = pkg_size,
			};
			stream_write(param->scsi_transact->stream, &buffer);
			param->data_size -= pkg_size;

			pkg_size = vsfusbd_MSCBOT_GetPkgSize(drv, 0, ep, param->data_size);
			if (stream_get_free_size(param->scsi_transact->stream) > pkg_size)
			{
				drv->ep.enable_OUT(param->ep_out);
				param->usb_idle = false;
			}
			else
			{
				param->usb_idle = true;
			}
			break;
		}
	default:
		if (param->scsi_transact != NULL)
		{
			vsfscsi_cancel_transact(param->scsi_transact);
			param->scsi_transact = NULL;
		}
		vsfusbd_MSCBOT_ErrHandler(device, param, USBMSC_CSW_PHASE_ERROR);
		return VSFERR_FAIL;
	}
	return VSFERR_NONE;
}

static vsf_err_t vsfusbd_MSCBOT_class_init(uint8_t iface,
											struct vsfusbd_device_t *device)
{
	struct interface_usbd_t *drv = device->drv;
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	struct vsfusbd_MSCBOT_param_t *param =
		(struct vsfusbd_MSCBOT_param_t *)config->iface[iface].protocol_param;

	if ((NULL == param) ||
		// minium ep size of MSCBOT is 32 bytes,
		// so that CBW and CSW can be transfered in one package
		(drv->ep.get_IN_epsize(param->ep_in) < 32) ||
		(drv->ep.get_OUT_epsize(param->ep_in) < 32) ||
		vsfusbd_set_IN_handler(device, param->ep_in,
											vsfusbd_MSCBOT_IN_hanlder) ||
		vsfusbd_set_OUT_handler(device, param->ep_out,
											vsfusbd_MSCBOT_OUT_hanlder) ||
		drv->ep.enable_OUT(param->ep_out))
	{
		return VSFERR_FAIL;
	}

	param->scsi_transact = NULL;
	param->device = device;
	vsfscsi_init(&param->scsi_dev);
	param->bot_status = VSFUSBD_MSCBOT_STATUS_IDLE;
	return VSFERR_NONE;
}

static vsf_err_t vsfusbd_MSCBOT_GetMaxLun_prepare(
	struct vsfusbd_device_t *device, struct vsf_buffer_t *buffer,
		uint8_t* (*data_io)(void *param))
{
	struct usb_ctrlrequest_t *request = &device->ctrl_handler.request;
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	uint8_t iface = request->wIndex;
	struct vsfusbd_MSCBOT_param_t *param =
		(struct vsfusbd_MSCBOT_param_t *)config->iface[iface].protocol_param;

	if ((NULL == param) || (request->wLength != 1) || (request->wValue != 0))
	{
		return VSFERR_FAIL;
	}

	buffer->buffer = &param->scsi_dev.max_lun;
	buffer->size = 1;

	return VSFERR_NONE;
}

static vsf_err_t vsfusbd_MSCBOT_Reset_prepare(
	struct vsfusbd_device_t *device, struct vsf_buffer_t *buffer,
		uint8_t* (*data_io)(void *param))
{
	struct interface_usbd_t *drv = device->drv;
	struct usb_ctrlrequest_t *request = &device->ctrl_handler.request;
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	uint8_t iface = request->wIndex;
	struct vsfusbd_MSCBOT_param_t *param =
		(struct vsfusbd_MSCBOT_param_t *)config->iface[iface].protocol_param;

	if ((NULL == param) || (request->wLength != 0) || (request->wValue != 0) ||
		drv->ep.reset_IN_toggle(param->ep_in) ||
		drv->ep.reset_OUT_toggle(param->ep_out) ||
		drv->ep.enable_OUT(param->ep_out))
	{
		return VSFERR_FAIL;
	}

	return VSFERR_NONE;
}

static const struct vsfusbd_setup_filter_t vsfusbd_MSCBOT_class_setup[] =
{
	{
		USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		USB_MSCBOTREQ_GET_MAX_LUN,
		vsfusbd_MSCBOT_GetMaxLun_prepare,
		NULL
	},
	{
		USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		USB_MSCBOTREQ_RESET,
		vsfusbd_MSCBOT_Reset_prepare,
		NULL
	},
	VSFUSBD_SETUP_NULL
};

const struct vsfusbd_class_protocol_t vsfusbd_MSCBOT_class =
{
	NULL, NULL,
	(struct vsfusbd_setup_filter_t *)vsfusbd_MSCBOT_class_setup, NULL,

	vsfusbd_MSCBOT_class_init, NULL
};
