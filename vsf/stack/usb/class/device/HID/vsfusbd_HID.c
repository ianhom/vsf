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

enum vsfusbd_HID_EVT_t
{
	VSFUSBD_HID_EVT_TIMER4MS = VSFSM_EVT_USER_LOCAL_INSTANT + 0,
	VSFUSBD_HID_EVT_INREPORT = VSFSM_EVT_USER_LOCAL_INSTANT + 1,
};

static struct vsfusbd_HID_report_t* vsfusbd_HID_find_report(
		struct vsfusbd_HID_param_t *param, uint8_t type, uint8_t id)
{
	uint8_t i;

	if (NULL == param)
	{
		return NULL;
	}

	for(i = 0; i < param->num_of_report; i++)
	{
		if ((param->reports[i].type == type) && (param->reports[i].id == id))
		{
			return &param->reports[i];
		}
	}

	return NULL;
}

static vsf_err_t vsfusbd_HID_OUT_hanlder(struct vsfusbd_device_t *device,
											uint8_t ep)
{
	struct interface_usbd_t *drv = device->drv;
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	int8_t iface = config->ep_OUT_iface_map[ep];
	struct vsfusbd_HID_param_t *param;
	uint16_t pkg_size, ep_size;
	uint8_t buffer[64], *pbuffer = buffer;
	uint8_t report_id;
	struct vsfusbd_HID_report_t *report;

	if (iface < 0)
	{
		return VSFERR_FAIL;
	}
	param = (struct vsfusbd_HID_param_t *)config->iface[iface].protocol_param;
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
	drv->ep.read_OUT_buffer(ep, buffer, pkg_size);

	switch (param->output_state)
	{
	case HID_OUTPUT_STATE_WAIT:
		report_id = buffer[0];
		report = vsfusbd_HID_find_report(param, USB_HID_REPORT_OUTPUT,
											report_id);
		if ((NULL == report) || (pkg_size > report->buffer.size))
		{
			return VSFERR_FAIL;
		}

		memcpy(report->buffer.buffer, pbuffer, pkg_size);
		if (pkg_size < report->buffer.size)
		{
			report->pos = pkg_size;
			param->output_state = HID_OUTPUT_STATE_RECEIVING;
			param->current_output_report_id = report_id;
		}
		else if (report->on_set_report != NULL)
		{
			report->on_set_report(param, report);
		}
		break;
	case HID_OUTPUT_STATE_RECEIVING:
		report_id = param->current_output_report_id;
		report = vsfusbd_HID_find_report(param, USB_HID_REPORT_OUTPUT,
											report_id);
		if ((NULL == report) ||
			((pkg_size + report->pos) > report->buffer.size))
		{
			return VSFERR_FAIL;
		}

		memcpy(report->buffer.buffer + report->pos, pbuffer, pkg_size);
		report->pos += pkg_size;
		if (report->pos >= report->buffer.size)
		{
			report->pos = 0;
			if (report->on_set_report != NULL)
			{
				report->on_set_report(param, report);
			}
			param->output_state = HID_OUTPUT_STATE_WAIT;
		}
		break;
	default:
		return VSFERR_NONE;
	}
	return drv->ep.enable_OUT(param->ep_out);
}

// state machines
static void vsfusbd_HID_INREPORT_callback(void *param)
{
	struct vsfusbd_HID_param_t *HID_param =
								(struct vsfusbd_HID_param_t *)param;

	HID_param->busy = false;
	if (HID_param->on_report_out != NULL)
	{
		HID_param->on_report_out(HID_param);
	}
	vsfsm_post_evt(&HID_param->iface->sm, VSFUSBD_HID_EVT_INREPORT);
}

vsf_err_t vsfusbd_HID_IN_report_changed(struct vsfusbd_HID_param_t *param,
										struct vsfusbd_HID_report_t *report)
{
	report->changed = true;
	if (!param->busy)
	{
		vsfsm_post_evt(&param->iface->sm, VSFUSBD_HID_EVT_INREPORT);
	}
	return VSFERR_NONE;
}

static struct vsfsm_state_t *
vsfusbd_HID_evt_handler(struct vsfsm_t *sm, vsfsm_evt_t evt)
{
	struct vsfusbd_HID_param_t *param =
						(struct vsfusbd_HID_param_t *)sm->user_data;
	struct vsfusbd_device_t *device = param->device;
	uint8_t i;

	switch (evt)
	{
	case VSFSM_EVT_INIT:
		if (param->ep_out != 0)
		{
			vsfusbd_set_OUT_handler(device, param->ep_out,
										vsfusbd_HID_OUT_hanlder);
			device->drv->ep.enable_OUT(param->ep_out);
		}

		param->output_state = HID_OUTPUT_STATE_WAIT;
		param->busy = false;
		param->num_of_INPUT_report = 0;
		param->num_of_OUTPUT_report = 0;
		param->num_of_FEATURE_report = 0;
		for(i = 0; i < param->num_of_report; i++)
		{
			param->reports[i].pos = 0;
			param->reports[i].idle_cnt = 0;
			param->reports[i].changed = true;
			switch (param->reports[i].type)
			{
			case USB_HID_REPORT_OUTPUT:
				param->num_of_OUTPUT_report++;
				break;
			case USB_HID_REPORT_INPUT:
				param->num_of_INPUT_report++;
				break;
			case USB_HID_REPORT_FEATURE:
				param->num_of_FEATURE_report++;
				break;
			}
		}

		// enable timer
		param->timer4ms.sm = sm;
		param->timer4ms.evt = VSFUSBD_HID_EVT_TIMER4MS;
		param->timer4ms.interval = 4;
		param->timer4ms.trigger_cnt = -1;
		vsftimer_enqueue(&param->timer4ms);
		break;
	case VSFUSBD_HID_EVT_TIMER4MS:
		{
			struct vsfusbd_HID_report_t *report;
			uint8_t i;

			for (i = 0; i < param->num_of_report; i++)
			{
				report = &param->reports[i];
				if ((report->type == USB_HID_REPORT_INPUT) &&
					(report->idle != 0))
				{
					report->idle_cnt++;
				}
			}
			if (param->busy)
			{
				break;
			}
		}
		// fall through
	case VSFUSBD_HID_EVT_INREPORT:
		{
			uint8_t ep = param->ep_in;
			struct vsfusbd_transact_t *transact = &device->IN_transact[ep];
			struct vsfusbd_HID_report_t *report;
			uint8_t i;

			for (i = 0; i < param->num_of_report; i++)
			{
				report = &param->reports[i];
				if ((report->type == USB_HID_REPORT_INPUT) &&
					(report->changed || ((report->idle != 0) &&
							(report->idle_cnt >= report->idle))))
				{
					report->idle_cnt = 0;
					transact->zlp = false;
					transact->tbuffer.buffer = report->buffer;
					transact->callback.callback = vsfusbd_HID_INREPORT_callback;
					transact->callback.param = param;
					vsfusbd_ep_send_nb(device, ep);
					report->changed = false;
					param->busy = true;
					break;
				}
			}
		}
		break;
	}

	return NULL;
}

vsf_err_t vsfusbd_HID_class_init(uint8_t iface, struct vsfusbd_device_t *device)
{
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	struct vsfusbd_iface_t *ifs = &config->iface[iface];
	struct vsfusbd_HID_param_t *param =
						(struct vsfusbd_HID_param_t *)ifs->protocol_param;

	// state machine init
	ifs->sm.init_state.evt_handler = vsfusbd_HID_evt_handler;
	param->iface = ifs;
	param->device = device;
	ifs->sm.user_data = (void*)param;
	return vsfsm_init(&ifs->sm);
}

vsf_err_t vsfusbd_HID_GetReport_prepare(
	struct vsfusbd_device_t *device, struct vsf_buffer_t *buffer,
		uint8_t* (*data_io)(void *param))
{
	struct usb_ctrlrequest_t *request = &device->ctrl_handler.request;
	uint8_t iface = request->wIndex;
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	struct vsfusbd_HID_param_t *param =
		(struct vsfusbd_HID_param_t *)config->iface[iface].protocol_param;
	uint8_t type = request->wValue >> 8, id = request->wValue;
	struct vsfusbd_HID_report_t *report =
									vsfusbd_HID_find_report(param, type, id);

	if ((NULL == param) || (NULL == report) || (type != report->type))
	{
		return VSFERR_FAIL;
	}

	buffer->size = report->buffer.size;
	buffer->buffer = report->buffer.buffer;
	return VSFERR_NONE;
}

vsf_err_t vsfusbd_HID_GetIdle_prepare(
	struct vsfusbd_device_t *device, struct vsf_buffer_t *buffer,
		uint8_t* (*data_io)(void *param))
{
	struct usb_ctrlrequest_t *request = &device->ctrl_handler.request;
	uint8_t id = request->wValue;
	uint8_t iface = request->wIndex;
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	struct vsfusbd_HID_param_t *param =
		(struct vsfusbd_HID_param_t *)config->iface[iface].protocol_param;
	struct vsfusbd_HID_report_t *report =
				vsfusbd_HID_find_report(param, USB_HID_REPORT_INPUT, id);

	if ((NULL == param) || (NULL == report) || (request->wLength != 1))
	{
		return VSFERR_FAIL;
	}

	buffer->size = 1;
	buffer->buffer = &report->idle;
	return VSFERR_NONE;
}

vsf_err_t vsfusbd_HID_GetProtocol_prepare(
	struct vsfusbd_device_t *device, struct vsf_buffer_t *buffer,
		uint8_t* (*data_io)(void *param))
{
	struct usb_ctrlrequest_t *request = &device->ctrl_handler.request;
	uint8_t iface = request->wIndex;
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	struct vsfusbd_HID_param_t *param =
		(struct vsfusbd_HID_param_t *)config->iface[iface].protocol_param;

	if ((NULL == param) || (request->wValue != 0) || (request->wLength != 1))
	{
		return VSFERR_FAIL;
	}

	buffer->size = 1;
	buffer->buffer = &param->protocol;
	return VSFERR_NONE;
}

vsf_err_t vsfusbd_HID_SetReport_prepare(
	struct vsfusbd_device_t *device, struct vsf_buffer_t *buffer,
		uint8_t* (*data_io)(void *param))
{
	struct usb_ctrlrequest_t *request = &device->ctrl_handler.request;
	uint8_t type = request->wValue >> 8, id = request->wValue;
	uint8_t iface = request->wIndex;
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	struct vsfusbd_HID_param_t *param =
		(struct vsfusbd_HID_param_t *)config->iface[iface].protocol_param;
	struct vsfusbd_HID_report_t *report =
									vsfusbd_HID_find_report(param, type, id);

	if ((NULL == param) || (NULL == report) || (type != report->type))
	{
		return VSFERR_FAIL;
	}

	buffer->size = report->buffer.size;
	buffer->buffer = report->buffer.buffer;
	return VSFERR_NONE;
}
vsf_err_t vsfusbd_HID_SetReport_process(
	struct vsfusbd_device_t *device, struct vsf_buffer_t *buffer)
{
	struct usb_ctrlrequest_t *request = &device->ctrl_handler.request;
	uint8_t type = request->wValue >> 8, id = request->wValue;
	uint8_t iface = request->wIndex;
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	struct vsfusbd_HID_param_t *param =
		(struct vsfusbd_HID_param_t *)config->iface[iface].protocol_param;
	struct vsfusbd_HID_report_t *report =
									vsfusbd_HID_find_report(param, type, id);

	if ((NULL == param) || (NULL == report) || (type != report->type))
	{
		return VSFERR_FAIL;
	}

	if (report->on_set_report != NULL)
	{
		return report->on_set_report(param, report);
	}
	return VSFERR_NONE;
}

vsf_err_t vsfusbd_HID_SetIdle_prepare(
	struct vsfusbd_device_t *device, struct vsf_buffer_t *buffer,
		uint8_t* (*data_io)(void *param))
{
	struct usb_ctrlrequest_t *request = &device->ctrl_handler.request;
	uint8_t id = request->wValue & 0xFF;
	uint8_t iface = request->wIndex;
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	struct vsfusbd_HID_param_t *param =
		(struct vsfusbd_HID_param_t *)config->iface[iface].protocol_param;
	uint8_t i;

	if ((NULL == param) || (request->wLength != 0))
	{
		return VSFERR_FAIL;
	}

	for(i = 0; i < param->num_of_report; i++)
	{
		if ((param->reports[i].type == USB_HID_REPORT_INPUT) &&
			((0 == id) || (param->reports[i].id == id)))
		{
			param->reports[i].idle = request->wValue >> 8;
		}
	}
	return VSFERR_NONE;
}

vsf_err_t vsfusbd_HID_SetProtocol_prepare(
	struct vsfusbd_device_t *device, struct vsf_buffer_t *buffer,
		uint8_t* (*data_io)(void *param))
{
	struct usb_ctrlrequest_t *request = &device->ctrl_handler.request;
	uint8_t iface = request->wIndex;
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	struct vsfusbd_HID_param_t *param =
		(struct vsfusbd_HID_param_t *)config->iface[iface].protocol_param;

	if ((NULL == param) || (request->wLength != 1) ||
		((request->wValue != USB_HID_PROTOCOL_BOOT) &&
		 	(request->wValue != USB_HID_PROTOCOL_REPORT)))
	{
		return VSFERR_FAIL;
	}

	param->protocol = request->wValue;
	return VSFERR_NONE;
}

vsf_err_t vsfusbd_HID_get_desc(struct vsfusbd_device_t *device, uint8_t type,
			uint8_t index, uint16_t lanid, struct vsf_buffer_t *buffer)
{
	struct usb_ctrlrequest_t *request = &device->ctrl_handler.request;
	uint8_t iface = request->wIndex;
	struct vsfusbd_config_t *config = &device->config[device->configuration];
	struct vsfusbd_HID_param_t *param =
		(struct vsfusbd_HID_param_t *)config->iface[iface].protocol_param;

	if ((NULL == param) || (NULL == param->desc))
	{
		return VSFERR_FAIL;
	}

	return vsfusbd_device_get_descriptor(device, param->desc, type, index,
											lanid, buffer);
}

#ifndef VSFCFG_STANDALONE_MODULE
static const struct vsfusbd_setup_filter_t vsfusbd_HID_class_setup[] =
{
	{
		USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		USB_HIDREQ_GET_REPORT,
		vsfusbd_HID_GetReport_prepare,
		NULL
	},
	{
		USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		USB_HIDREQ_GET_IDLE,
		vsfusbd_HID_GetIdle_prepare,
		NULL
	},
	{
		USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		USB_HIDREQ_GET_PROTOCOL,
		vsfusbd_HID_GetProtocol_prepare,
		NULL
	},
	{
		USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		USB_HIDREQ_SET_REPORT,
		vsfusbd_HID_SetReport_prepare,
		vsfusbd_HID_SetReport_process
	},
	{
		USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		USB_HIDREQ_SET_IDLE,
		vsfusbd_HID_SetIdle_prepare,
		NULL
	},
	{
		USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		USB_HIDREQ_SET_PROTOCOL,
		vsfusbd_HID_SetProtocol_prepare,
		NULL
	},
	VSFUSBD_SETUP_NULL
};

const struct vsfusbd_class_protocol_t vsfusbd_HID_class =
{
	vsfusbd_HID_get_desc, NULL,
	(struct vsfusbd_setup_filter_t *)vsfusbd_HID_class_setup, NULL,

	vsfusbd_HID_class_init, NULL
};
#endif
