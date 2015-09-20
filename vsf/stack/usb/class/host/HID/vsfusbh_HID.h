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

#ifndef __VSFUSBH_HID_H_INCLUDED__
#define __VSFUSBH_HID_H_INCLUDED__

#define HID_LONG_ITEM(x)			((x) == 0xFE)
#define HID_ITEM_SIZE(x)			((((x)&0x03) == 3)?4:(x)&0x03)

#define HID_ITEM_TYPE(x)			(((x) >> 2) & 0x03)
#define HID_ITEM_TYPE_MAIN			0
#define HID_ITEM_TYPE_GLOBAL		1
#define HID_ITEM_TYPE_LOCAL			2

#define HID_ITEM_TAG(x)				((x)&0xFC)
#define HID_ITEM_INPUT				0x80
#define HID_ITEM_OUTPUT				0x90
#define HID_ITEM_FEATURE			0xB0
#define HID_ITEM_COLLECTION			0xA0
#define HID_ITEM_END_COLLECTION		0xC0
#define HID_ITEM_USAGE_PAGE			0x04
#define HID_ITEM_LOGI_MINI			0x14
#define HID_ITEM_LOGI_MAXI			0x24
#define HID_ITEM_PHY_MINI			0x34
#define HID_ITEM_PHY_MAXI			0x44
#define HID_ITEM_UNIT_EXPT			0x54
#define HID_ITEM_UNIT				0x64
#define HID_ITEM_REPORT_SIZE		0x74
#define HID_ITEM_REPORT_ID			0x84
#define HID_ITEM_REPORT_COUNT		0x94
#define HID_ITEM_PUSH				0xA4
#define HID_ITEM_POP				0xB4
#define HID_ITEM_USAGE				0x08
#define HID_ITEM_USAGE_MIN			0x18
#define HID_ITEM_USAGE_MAX			0x28

PACKED_HEAD struct PACKED_MID PACKED_MID hid_descriptor_t
{
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bcdHID;
	uint8_t bCountryCode;
	uint8_t bNumDescriptors;
	uint8_t bDescriptorType2;
	uint16_t wDescriptorLength;
}; PACKED_TAIL

PACKED_HEAD struct PACKED_MID hid_desc_t
{
	int collection;
	int report_size;
	int report_count;
	int usage_page;
	int usage_num;
	int logical_min;
	int logical_max;
	int physical_min;
	int physical_max;
	int usage_min;
	int usage_max;
	int usages[16];
}; PACKED_TAIL

struct hid_usage_t
{
	uint16_t usage_page;
	uint8_t usage_min;
	uint8_t usage_max;
	int32_t logical_min;
	int32_t logical_max;
	int32_t bit_offset;
	int32_t bit_length;
	int32_t report_size;
	int32_t report_count;
	uint32_t data_flag;
	struct sllist list;
};

struct hid_report_t
{
	int input_bitlen;
	int output_bitlen;
	uint8_t *cur_value;
	uint8_t *pre_value;
	struct sllist input_list;
	struct sllist output_list;

	uint8_t need_setreport_flag;
	uint8_t need_ignore;
};

struct vsfusbh_hid_t
{
	/* hub timer */
	struct vsftimer_t timer;

	/* statemachine */
	struct vsfsm_t sm;
	struct vsfsm_pt_t hid_pt;

	struct vsfusbh_t *usbh;
	struct vsfusbh_device_t *dev;

	// interface 1
	struct vsfusbh_urb_t vsfurb_1;
	struct usb_endpoint_descriptor_t *int_ep_1;
	struct hid_report_t report_1;
	struct hid_descriptor_t hid_descriptor_1;

	// interface 2
	struct vsfusbh_urb_t vsfurb_2;
	struct usb_endpoint_descriptor_t *int_ep_2;
	struct hid_report_t report_2;
	struct hid_descriptor_t hid_descriptor_2;

};

extern const struct vsfusbh_class_drv_t vsfusbh_hid_drv;

#endif
