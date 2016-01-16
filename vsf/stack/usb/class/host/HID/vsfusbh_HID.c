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

#define vsfusbh_hid_check_ret_value()	\
	do {\
		if (err != VSFERR_NONE)\
		{\
			return err;\
		}\
	} while (0)

#define vsfusbh_hid_check_urb_status()	\
	do {\
		if (vsfurb->dev->status != VSFERR_NONE)\
		{\
			return VSFERR_FAIL;\
		}\
	} while (0)

static vsf_err_t vsfusbh_hid_parse_item(struct hid_desc_t *desc, uint8_t tag,
	int size, uint8_t *buf, struct hid_report_t *hidrpt)
{
	int i;
	int value;
	struct hid_usage_t *usage;

	if (hidrpt->need_ignore == 1)
	{
		return VSFERR_NONE;
	}

	if (size == 1)
		value = *buf;
	else if (size == 2)
		value = *(uint16_t *)buf;
	else if (size == 4)
		value = *(uint32_t *)buf;

	switch (tag)
	{
	case HID_ITEM_INPUT:
		if ((desc->usage_min != -1) && (desc->usage_max != -1))
		{
			desc->usage_num = desc->usage_max - desc->usage_min + 1;
			usage = vsf_bufmgr_malloc(sizeof(struct hid_usage_t));
			if (usage == NULL)
				return VSFERR_FAIL;

			usage->data_flag = (uint32_t)value;
			usage->report_size = (int32_t)desc->report_size;
			usage->report_count = (int32_t)desc->report_count;

			usage->usage_page = (uint16_t)desc->usage_page;
			usage->usage_min = (uint8_t)desc->usage_min;
			usage->usage_max = (uint8_t)desc->usage_max;
			usage->bit_offset = (int32_t)hidrpt->input_bitlen;
			usage->bit_length = (int32_t)(desc->report_size * desc->report_count);

			usage->logical_min = desc->logical_min;
			usage->logical_max = desc->logical_max;

			sllist_append(&hidrpt->input_list, &usage->list);

			desc->usage_min = -1;
			desc->usage_max = -1;
		}
		else
		{
			for (i = 0; i < desc->usage_num; i++)
			{
				usage = vsf_bufmgr_malloc(sizeof(struct hid_usage_t));
				if (usage == NULL)
					return VSFERR_FAIL;

				usage->report_size = (int32_t)desc->report_size;
				usage->report_count = (int32_t)desc->report_count / desc->usage_num;
				usage->data_flag = (uint32_t)value;

				usage->usage_page = (uint16_t)desc->usage_page;
				usage->usage_min = (uint8_t)desc->usages[i];
				usage->usage_max = (uint8_t)desc->usages[i];
				usage->bit_length = (int32_t)(desc->report_size * desc->report_count / desc->usage_num);
				usage->bit_offset = (int32_t)(hidrpt->input_bitlen + i*usage->bit_length);

				usage->logical_min = desc->logical_min;
				usage->logical_max = desc->logical_max;

				sllist_append(&hidrpt->input_list, &usage->list);
			}
		}

		desc->usage_num = 0;
		hidrpt->input_bitlen += (desc->report_size * desc->report_count);
		break;

	case HID_ITEM_OUTPUT:
		if ((desc->usage_min != -1) && (desc->usage_max != -1))
		{
			desc->usage_num = desc->usage_max - desc->usage_min + 1;
			usage = vsf_bufmgr_malloc(sizeof(struct hid_usage_t));
			if (usage == NULL)
				return VSFERR_FAIL;

			usage->report_size = desc->report_size;
			usage->report_count = desc->report_count;
			usage->data_flag = value;

			usage->usage_page = desc->usage_page;
			usage->usage_min = desc->usage_min;
			usage->usage_max = desc->usage_max;
			usage->bit_offset = hidrpt->output_bitlen;
			usage->bit_length = desc->report_size * desc->report_count;

			usage->logical_min = desc->logical_min;
			usage->logical_max = desc->logical_max;

			sllist_append(&hidrpt->output_list, &usage->list);

			desc->usage_min = -1;
			desc->usage_max = -1;
		}
		else
		{
			for (i = 0; i < desc->usage_num; i++)
			{
				usage = vsf_bufmgr_malloc(sizeof(struct hid_usage_t));
				if (usage == NULL)
					return VSFERR_FAIL;

				usage->report_size = desc->report_size;
				usage->report_count = desc->report_count;
				usage->data_flag = value;
				value = desc->report_size * desc->report_count / desc->usage_num;

				usage->usage_page = desc->usage_page;
				usage->usage_min = desc->usages[i];
				usage->usage_max = desc->usages[i];
				usage->bit_offset = hidrpt->output_bitlen + i*value;
				usage->bit_length = value;

				usage->logical_min = desc->logical_min;
				usage->logical_max = desc->logical_max;

				sllist_append(&hidrpt->output_list, &usage->list);
			}
		}

		desc->usage_num = 0;
		hidrpt->output_bitlen += (desc->report_size * desc->report_count);
		break;

	case HID_ITEM_FEATURE:
		break;

	case HID_ITEM_COLLECTION:
		desc->collection++;
		desc->usage_num = 0;
		break;

	case HID_ITEM_END_COLLECTION:
		desc->collection--;
		break;

	case HID_ITEM_USAGE_PAGE:
		desc->usage_page = value;
		break;

	case HID_ITEM_LOGI_MINI:
		if (size == 1)
			value = *(int8_t *)buf;
		else if (size == 2)
			value = *(int16_t *)buf;
		else if (size == 4)
			value = *(int32_t *)buf;
		desc->logical_min = value;
		break;

	case HID_ITEM_LOGI_MAXI:
		if (size == 1)
			value = *(int8_t *)buf;
		else if (size == 2)
			value = *(int16_t *)buf;
		else if (size == 4)
			value = *(int32_t *)buf;
		desc->logical_max = value;
		break;

	case HID_ITEM_PHY_MINI:
		break;

	case HID_ITEM_PHY_MAXI:
		break;

	case HID_ITEM_UNIT_EXPT:
		break;

	case HID_ITEM_UNIT:
		break;

	case HID_ITEM_REPORT_SIZE:
		desc->report_size = value;
		break;

	case HID_ITEM_REPORT_ID:
		if (hidrpt->need_setreport_flag == 0)
		{
			hidrpt->need_setreport_flag = 1;
			hidrpt->input_bitlen += 8;
		}
		else
		{
			desc->collection = 0;
			hidrpt->need_ignore = 1;
		}
		break;

	case HID_ITEM_REPORT_COUNT:
		desc->report_count = value;
		break;

	case HID_ITEM_PUSH:
		break;

	case HID_ITEM_POP:
		break;

	case HID_ITEM_USAGE:
		desc->usages[desc->usage_num++] = value;
		break;

	case HID_ITEM_USAGE_MAX:
		desc->usage_max = value;
		break;

	case HID_ITEM_USAGE_MIN:
		desc->usage_min = value;
		break;
	}

	return VSFERR_NONE;
}

/***************************************************************************
* free hid report
**************************************************************************/
static void vsfusbh_hid_free_report(struct hid_report_t *rpt)
{
	struct sllist *list;
	struct hid_usage_t *usage;

	/* free input list */
	list = rpt->input_list.next;
	while (list)
	{
		usage = sllist_get_container(list, struct hid_usage_t, list);
		list = list->next;

		vsf_bufmgr_free(usage);
	}

	/* free output list */
	list = rpt->output_list.next;
	while (list)
	{
		usage = sllist_get_container(list, struct hid_usage_t, list);
		list = list->next;

		vsf_bufmgr_free(usage);
	}

	/* free buffer */
	if (rpt->cur_value)
		vsf_bufmgr_free(rpt->cur_value);
	if (rpt->pre_value)
		vsf_bufmgr_free(rpt->pre_value);

	rpt->cur_value = NULL;
	rpt->pre_value = NULL;
}

/***************************************************************************
* hid parser
**************************************************************************/
static vsf_err_t vsfusbh_hid_parse_report(uint8_t *buf, int len, struct hid_report_t *hidrpt, struct usb_endpoint_descriptor_t *ep)
{
	uint8_t b;
	uint8_t *end = buf + len;
	int item_size;
	vsf_err_t err;
	struct hid_desc_t *desc = vsf_bufmgr_malloc(sizeof(struct hid_desc_t));

	if (desc == NULL)
		return VSFERR_FAIL;

	memset(hidrpt, 0, sizeof(struct hid_report_t));
	memset(desc, 0, sizeof(struct hid_desc_t));

	sllist_init_node(hidrpt->input_list);
	sllist_init_node(hidrpt->output_list);

	desc->usage_min = -1;
	desc->usage_max = -1;

	while (buf < end)
	{
		b = *buf;

		if (HID_LONG_ITEM(b))
		{
			item_size = *(buf + 1);
		}
		else
		{
			item_size = HID_ITEM_SIZE(b);
			err = vsfusbh_hid_parse_item(desc, HID_ITEM_TAG(b), item_size, buf + 1, hidrpt);
			if (err)
			{
				err = -1;
				break;
			}
		}

		buf += (item_size + 1);
	}

	/* error happened */
	if ((desc->collection != 0) || err)
		goto free_hid_report;

	/* alloc memory for input event data */
	hidrpt->cur_value = vsf_bufmgr_malloc(max(hidrpt->input_bitlen >> 3, ep->wMaxPacketSize));
	if (hidrpt->cur_value == NULL)
		goto free_hid_report;

	hidrpt->pre_value = vsf_bufmgr_malloc(max(hidrpt->input_bitlen >> 3, ep->wMaxPacketSize));
	if (hidrpt->pre_value == NULL)
		goto free_cur_value;

	memset(hidrpt->pre_value, 0, hidrpt->input_bitlen >> 3);
	memset(hidrpt->cur_value, 0, hidrpt->input_bitlen >> 3);

	vsf_bufmgr_free(desc);
	return VSFERR_NONE;

	/* error process */
free_cur_value:
	vsf_bufmgr_free(hidrpt->cur_value);
free_hid_report:
	vsf_bufmgr_free(desc);
	vsfusbh_hid_free_report(hidrpt);

	return VSFERR_FAIL;
}

/***************************************************************************
* get bits
**************************************************************************/
static uint32_t get_bits(uint8_t *buf, uint32_t bit_offset, uint32_t length)
{
	uint16_t temp = 0, temp2;

	if (NULL == buf)
	{
		return 0;
	}
	temp = (bit_offset & 0x07) + length;
	if (temp <= 8)
	{
		return (buf[bit_offset >> 3] & (((0x01ul << length) - 1) << (bit_offset & 0x07))) >> (bit_offset & 0x07);
	}
	else if (temp <= 16)
	{
		temp2 = (buf[bit_offset >> 3] & (0xff << (bit_offset & 0x07))) >> (bit_offset & 0x07);
		return temp2 + ((buf[(bit_offset >> 3) + 1] & (0xff >> (16 - temp))) << (8 - (bit_offset & 0x07)));
	}
	else
	{
		// not support
		return 0;
	}
}

#define HID_VALUE_TYPE_UNKNOWN			0
#define HID_VALUE_TYPE_SIGN_8BIT		0x01
#define HID_VALUE_TYPE_UNSIGN_8BIT		0x02
#define HID_VALUE_TYPE_SIGN_16BIT		0x03
#define HID_VALUE_TYPE_UNSIGN_16BIT		0x04
#define HID_VALUE_TYPE_ABS				0x0000
#define HID_VALUE_TYPE_REL				0x0100

PACKED_HEAD struct PACKED_MID usbh_hid_event_t
{
	uint16_t usage_page;
	uint16_t usage_id;
	int32_t pre_value;
	int32_t cur_value;
	uint32_t type;
}; PACKED_TAIL

extern struct vsfusbb_t usbb;

static void vsfusbh_hid_process_input(struct hid_report_t *report_x)
{
	struct hid_report_t *report = report_x;
	struct hid_usage_t *usage;
	uint32_t cur_value, pre_value;
	int32_t i;

	/* have a quick check */
	if (0 == memcmp(report->cur_value, report->pre_value, report->input_bitlen >> 3))
		return;

	usage = sllist_get_container(report->input_list.next, struct hid_usage_t, list);
	while (usage != NULL)
	{
		for (i = 0; i<usage->report_count; ++i)
		{
			/* get usage value */
			cur_value = get_bits(report->cur_value, usage->bit_offset + i*usage->report_size, usage->report_size);
			pre_value = get_bits(report->pre_value, usage->bit_offset + i*usage->report_size, usage->report_size);

			/* compare and process */
			if (cur_value != pre_value)
			{
				struct usbh_hid_event_t event;
				event.type = HID_VALUE_TYPE_UNKNOWN;

				event.usage_page = usage->usage_page;
				event.usage_id = (usage->data_flag & 0x2) ? (usage->usage_min + i) :
					(cur_value ? (uint16_t)cur_value : (uint16_t)pre_value);
				if ((usage->logical_min == -127) && (usage->logical_max == 127))
				{
					event.type = HID_VALUE_TYPE_SIGN_8BIT;
					event.pre_value = (pre_value <= 127) ? pre_value : (pre_value - 256);
					event.cur_value = (cur_value <= 127) ? cur_value : (cur_value - 256);
				}
				else if ((usage->logical_min == -2047) && (usage->logical_max == 2047))
				{
					event.type = HID_VALUE_TYPE_SIGN_8BIT;
					if (pre_value <= 127)
					{
						event.pre_value = pre_value;
					}
					else if (pre_value <= 2047)
					{
						event.pre_value = 127;
					}
					else if (pre_value <= 4095)
					{
						int32_t temp = pre_value - 4096;
						if (temp >= -127)
						{
							event.pre_value = temp;
						}
						else
						{
							event.pre_value = -127;
						}
					}
					else
					{
						// error
						event.pre_value = 0;
					}

					if (cur_value <= 127)
					{
						event.cur_value = cur_value;
					}
					else if (cur_value <= 2047)
					{
						event.cur_value = 127;
					}
					else if (cur_value <= 4095)
					{
						int32_t temp = cur_value - 4096;
						if (temp >= -127)
						{
							event.cur_value = temp;
						}
						else
						{
							event.cur_value = -127;
						}
					}
					else
					{
						// error
						event.cur_value = 0;
					}
				}
				else if ((usage->logical_min == 0) && (usage->logical_max <= 127))
				{
					event.type = HID_VALUE_TYPE_SIGN_8BIT;
					event.pre_value = (pre_value <= 127) ? pre_value : (pre_value - 256);
					event.cur_value = (cur_value <= 127) ? cur_value : (cur_value - 256);
				}
				else if ((usage->logical_min == 0) && (usage->logical_max == 255))
				{
					event.type = HID_VALUE_TYPE_UNSIGN_8BIT;
					event.pre_value = pre_value;
					event.cur_value = cur_value;
				}
				else if ((usage->logical_min == -32767) && (usage->logical_max == 32767))
				{
					event.type = HID_VALUE_TYPE_SIGN_16BIT;
					event.pre_value = (pre_value <= 32767) ? pre_value : (pre_value - 65536);
					event.cur_value = (cur_value <= 32767) ? cur_value : (cur_value - 65536);
				}
				else if ((usage->logical_min == 0) && (usage->logical_max <= 32767))
				{
					event.type = HID_VALUE_TYPE_SIGN_16BIT;
					event.pre_value = (pre_value <= 32767) ? pre_value : (pre_value - 65536);
					event.cur_value = (cur_value <= 32767) ? cur_value : (cur_value - 65536);
				}
				else if ((usage->logical_min == 0) && (usage->logical_max == 65535))
				{
					event.type = HID_VALUE_TYPE_UNSIGN_16BIT;
					event.pre_value = pre_value;
					event.cur_value = cur_value;
				}
				if (usage->data_flag & 0x4)
				{
					event.type |= HID_VALUE_TYPE_REL;
				}
			}
		}
		usage = sllist_get_container(usage->list.next, struct hid_usage_t, list);
	}
}

/***************************************************************************
* PT thread to init the hub
**************************************************************************/
static vsf_err_t vsfusbh_hid_poll_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	struct vsfusbh_hid_t *hdata = (struct vsfusbh_hid_t *)pt->user_data;
	struct vsfusbh_urb_t *vsfurb = &hdata->vsfurb_1;
	vsf_err_t err;
	static uint16_t int_urb_id;
	uint16_t len;

	vsfsm_pt_begin(pt);

	/* init timer for device */
	hdata->timer.sm = &hdata->sm;

	hdata->vsfurb_1.transfer_len = hdata->dev->config.wTotalLength;
	if (hdata->vsfurb_1.transfer_len <= 1024)
	{
		hdata->vsfurb_1.buffer = vsf_bufmgr_malloc(hdata->vsfurb_1.transfer_len);
		if (hdata->vsfurb_1.buffer == NULL)
		{
			return VSFERR_FAIL;
		}
	}
	else
	{
		return VSFERR_FAIL;
	}
	hdata->vsfurb_1.sm = &hdata->sm;
	err = vsfusbh_get_descriptor(hdata->usbh, &hdata->vsfurb_1, USB_DT_CONFIG, 0);

	vsfusbh_hid_check_ret_value();
	vsfsm_pt_wfe(pt, VSFSM_EVT_WAIT_URB);		/* wait for urb complete */
	vsfusbh_hid_check_urb_status();

	if (hdata->vsfurb_1.actual_length != hdata->dev->config.wTotalLength)
	{
		vsf_bufmgr_free(hdata->vsfurb_1.buffer);
		return VSFERR_FAIL;
	}

	int_urb_id = 0;
	len = 0;
	while (len < hdata->dev->config.wTotalLength)
	{
		struct usb_descriptor_header_t *head;

		head = (struct usb_descriptor_header_t *)&hdata->vsfurb_1.buffer[len];
		if (head->bDescriptorType == USB_DT_HID)
		{
			int_urb_id++;
			if (int_urb_id == 1)
			{
				memcpy(&hdata->hid_descriptor_1, &hdata->vsfurb_1.buffer[len], sizeof(struct hid_descriptor_t));
			}
			else if (int_urb_id == 2)
			{
				memcpy(&hdata->hid_descriptor_2, &hdata->vsfurb_1.buffer[len], sizeof(struct hid_descriptor_t));
				break;
			}
		}
		len += head->bLength;
	}
	vsf_bufmgr_free(hdata->vsfurb_1.buffer);
	hdata->vsfurb_1.buffer = NULL;
	if (int_urb_id == 0)
	{
		return VSFERR_FAIL;
	}

	hdata->vsfurb_1.buffer = NULL;
	hdata->vsfurb_1.transfer_len = 0;
	err = vsfusbh_set_config(hdata->usbh, &hdata->vsfurb_1, hdata->dev->config.bConfigurationValue, &hdata->sm);
	int_urb_id = 1;

	vsfusbh_hid_check_ret_value();
	vsfsm_pt_wfe(pt, VSFSM_EVT_WAIT_URB);		/* wait for urb complete */
	vsfusbh_hid_check_urb_status();

	if ((int_urb_id == 1) && (hdata->int_ep_1 != NULL))
	{
		/*get report descriptor*/
		if (hdata->hid_descriptor_1.bDescriptorType != USB_DT_HID)
		{
			return VSFERR_FAIL;
		}
		hdata->vsfurb_1.buffer = vsf_bufmgr_malloc(hdata->hid_descriptor_1.wDescriptorLength);
		hdata->vsfurb_1.transfer_len = hdata->hid_descriptor_1.wDescriptorLength;
		err = vsfusbh_get_hid_descriptor(hdata->usbh, &hdata->vsfurb_1, USB_DT_REPORT, 0, &hdata->sm);

		vsfusbh_hid_check_ret_value();
		vsfsm_pt_wfe(pt, VSFSM_EVT_WAIT_URB);	/* wait for urb complete */
		vsfusbh_hid_check_urb_status();

		/* parse report descriptor */
		if (hdata->vsfurb_1.actual_length != hdata->hid_descriptor_1.wDescriptorLength)
		{
			vsf_bufmgr_free(hdata->vsfurb_1.buffer);
			return VSFERR_FAIL;
		}

		if (VSFERR_NONE != vsfusbh_hid_parse_report(hdata->vsfurb_1.buffer, hdata->hid_descriptor_1.wDescriptorLength, &hdata->report_1, hdata->int_ep_1))
		{
			vsf_bufmgr_free(hdata->vsfurb_1.buffer);
			return VSFERR_FAIL;
		}

		vsf_bufmgr_free(hdata->vsfurb_1.buffer);

		int_urb_id++;
	}

	if ((int_urb_id == 2) && (hdata->int_ep_2 != NULL))
	{
		if (hdata->hid_descriptor_2.bDescriptorType != USB_DT_HID)
		{
			return VSFERR_FAIL;
		}
		hdata->vsfurb_1.buffer = vsf_bufmgr_malloc(hdata->hid_descriptor_2.wDescriptorLength);
		hdata->vsfurb_1.transfer_len = hdata->hid_descriptor_2.wDescriptorLength;
		err = vsfusbh_get_hid_descriptor(hdata->usbh, &hdata->vsfurb_1, USB_DT_REPORT, 1, &hdata->sm);

		vsfusbh_hid_check_ret_value();
		vsfsm_pt_wfe(pt, VSFSM_EVT_WAIT_URB);	/* wait for urb complete */
		vsfusbh_hid_check_urb_status();

		/* parse report descriptor */
		if (hdata->vsfurb_1.actual_length != hdata->hid_descriptor_2.wDescriptorLength)
		{
			vsf_bufmgr_free(hdata->vsfurb_1.buffer);
			return VSFERR_FAIL;
		}

		if (VSFERR_NONE != vsfusbh_hid_parse_report(hdata->vsfurb_1.buffer, hdata->hid_descriptor_2.wDescriptorLength, &hdata->report_2, hdata->int_ep_2))
		{
			vsf_bufmgr_free(hdata->vsfurb_1.buffer);
			return VSFERR_FAIL;
		}

		vsf_bufmgr_free(hdata->vsfurb_1.buffer);

		int_urb_id++;
		if (hdata->report_2.need_setreport_flag != 0)
		{
			static uint8_t report_id = 0x01;
			hdata->vsfurb_1.buffer = &report_id;
			hdata->vsfurb_1.transfer_len = 1;
			hdata->vsfurb_1.pipe = usb_sndctrlpipe(hdata->vsfurb_1.dev, 0);
			err = vsfusbh_control_msg(hdata->usbh, &hdata->vsfurb_1, 0x21, 0x09, 0x0200, 0);

			vsfusbh_hid_check_ret_value();
			vsfsm_pt_wfe(pt, VSFSM_EVT_WAIT_URB);/* wait for urb complete */
			vsfusbh_hid_check_urb_status();
		}
	}

	if (hdata->int_ep_1 != NULL)
	{
		hdata->vsfurb_1.buffer = hdata->report_1.cur_value;
		hdata->vsfurb_1.transfer_len = max(hdata->report_1.input_bitlen >> 3, hdata->int_ep_1->wMaxPacketSize);
		hdata->vsfurb_1.dev->status = USB_ST_NOT_PROC;
		hdata->vsfurb_1.setup = NULL;
		hdata->vsfurb_1.pipe = usb_rcvintpipe(hdata->dev, hdata->int_ep_1->bEndpointAddress);
		hdata->usbh->hcd->submit_urb(hdata->usbh->hcd_data, &hdata->vsfurb_1);
	}
	if (hdata->int_ep_2 != NULL)
	{
		hdata->vsfurb_2.buffer = hdata->report_2.cur_value;
		hdata->vsfurb_2.transfer_len = max(hdata->report_2.input_bitlen >> 3, hdata->int_ep_2->wMaxPacketSize);
		hdata->vsfurb_2.dev->status = USB_ST_NOT_PROC;
		hdata->vsfurb_2.setup = NULL;
		hdata->vsfurb_2.pipe = usb_rcvintpipe(hdata->dev, hdata->int_ep_2->bEndpointAddress);
		hdata->usbh->hcd->submit_urb(hdata->usbh->hcd_data, &hdata->vsfurb_2);
	}

	vsfurb = &hdata->vsfurb_1;
	vsfsm_pt_wfe(pt, VSFSM_EVT_WAIT_URB);		/* wait for urb complete */
	vsfusbh_hid_check_urb_status();

	if ((err == VSFERR_NONE) || (err < 0))
	{
		if (err == VSFERR_NONE)
		{
			interfaces->gpio.clear(0, 1 << 11);
			vsfusbh_hid_process_input(&hdata->report_1);
			memcpy(hdata->report_1.pre_value, hdata->report_1.cur_value, hdata->report_1.input_bitlen >> 3);
		}
	}
	if (hdata->int_ep_2 != NULL)
	{
		vsfurb = &hdata->vsfurb_2;
		vsfsm_pt_wfe(pt, VSFSM_EVT_WAIT_URB);	/* wait for urb complete */
		vsfusbh_hid_check_urb_status();
		if ((err == VSFERR_NONE) || (err < 0))
		{
			if (err == VSFERR_NONE)
			{
				interfaces->gpio.clear(0, 1 << 11);
				vsfusbh_hid_process_input(&hdata->report_2);
				memcpy(hdata->report_2.pre_value, hdata->report_2.cur_value, hdata->report_2.input_bitlen >> 3);
			}
		}
	}

	vsfsm_pt_end(pt);

	return VSFERR_NONE;
}

static struct vsfsm_state_t *vsfusbh_hid_evt_handler_init(struct vsfsm_t *sm, vsfsm_evt_t evt)
{
	vsf_err_t err;
	struct vsfusbh_hid_t *hdata = (struct vsfusbh_hid_t *)sm->user_data;

	switch (evt)
	{
	case VSFSM_EVT_INIT:
		hdata->hid_pt.thread = vsfusbh_hid_poll_thread;
		hdata->hid_pt.user_data = hdata;
		hdata->hid_pt.sm = sm;
		hdata->hid_pt.state = 0;

		evt = VSFSM_EVT_INIT;

	default:
		if (NULL == hdata->hid_pt.thread)
			return NULL;

		/* call the init PT */
		err = hdata->hid_pt.thread(&hdata->hid_pt, evt);

		if (err < 0)
		{
			/* device disconnectted */
			vsfusbh_remove_device(hdata->usbh, hdata->dev);

		}
		break;
	}

	return NULL;
}

static void *vsfusbh_hid_init(struct vsfusbh_t *usbh, struct vsfusbh_device_t *dev)
{
	struct vsfusbh_class_data_t *cdata;
	struct usb_endpoint_descriptor_t *ep_desc;
	struct vsfusbh_hid_t *hdata;
	uint16_t int_urb_id = 1;
	int i, j;

	/* malloc data for class driver */
	cdata = vsf_bufmgr_malloc(sizeof(struct vsfusbh_class_data_t));
	if (NULL == cdata)
		return NULL;

	hdata = vsf_bufmgr_malloc(sizeof(struct vsfusbh_hid_t));
	if (NULL == hdata)
	{
		vsf_bufmgr_free(cdata);
		return NULL;
	}

	/* clear the data */
	memset(cdata, 0, sizeof(struct vsfusbh_class_data_t));
	memset(hdata, 0, sizeof(struct vsfusbh_hid_t));

	/* init the data */
	cdata->dev = dev;
	hdata->dev = dev;
	hdata->usbh = usbh;
	hdata->vsfurb_1.dev = dev;
	hdata->vsfurb_1.timeout = 200;
	hdata->vsfurb_2.dev = dev;
	hdata->vsfurb_2.timeout = 200;
	dev->priv = cdata;

	/* find the intterupt endpoint */
	for (i = 0; i < dev->config.no_of_if; i++)
	{
		if (dev->config.if_desc[i].bInterfaceClass == USB_CLASS_HID)
		{
			for (j = 0; j < dev->config.if_desc[i].bNumEndpoints; j++)
			{
				ep_desc = &dev->config.if_desc[i].ep_desc[j];
				if (((ep_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT) &&
					((ep_desc->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_DIR_IN))
				{
					if (int_urb_id == 1)
					{
						int_urb_id++;
						hdata->int_ep_1 = ep_desc;
					}
					else if (int_urb_id == 2)
					{
						int_urb_id++;
						hdata->int_ep_2 = ep_desc;
					}
					else
					{
						break;
					}
				}
			}
			if (int_urb_id > 2)
				break;
		}
	}

	/* no interrupt endpoint found */
	if (hdata->int_ep_1 == NULL)
	{
		vsf_bufmgr_free(cdata);
		vsf_bufmgr_free(hdata);
		return NULL;
	}

	/* init the main FSM */
	cdata->param = hdata;
	hdata->sm.init_state.evt_handler = vsfusbh_hid_evt_handler_init;
	hdata->sm.user_data = (void*)hdata;
	vsfsm_init(&hdata->sm);

	return cdata;
}

static vsf_err_t vsfusbh_hid_match(struct vsfusbh_device_t *dev)
{
	int i;

	if (dev->descriptor.bDeviceClass == USB_CLASS_HID)
	{
		return VSFERR_NONE;
	}
	else if (dev->descriptor.bDeviceClass == USB_CLASS_PER_INTERFACE)
	{
		for (i = 0; i < dev->config.no_of_if; i++)
		{
			if (dev->config.if_desc[i].bInterfaceClass == USB_CLASS_HID)
			{
				return VSFERR_NONE;
			}
		}
	}
	return VSFERR_FAIL;
}

static void vsfusbh_hid_free(struct vsfusbh_device_t *dev)
{
	struct vsfusbh_class_data_t *cdata = (struct vsfusbh_class_data_t *)(dev->priv);
	struct vsfusbh_hid_t *hdata = (struct vsfusbh_hid_t *)cdata->param;

	// TODO
	if (hdata->int_ep_1 != NULL)
		hdata->usbh->hcd->free_resource(hdata->usbh, hdata->vsfurb_1.urb);
	if (hdata->int_ep_2 != NULL)
		hdata->usbh->hcd->free_resource(hdata->usbh, hdata->vsfurb_2.urb);

	vsf_bufmgr_free(hdata);
	vsf_bufmgr_free(cdata);
}

const struct vsfusbh_class_drv_t vsfusbh_hid_drv = {
	vsfusbh_hid_init,
	vsfusbh_hid_free,
	vsfusbh_hid_match,
};