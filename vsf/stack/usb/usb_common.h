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

#ifndef __VSF_USBD_COMMON_H_INCLUDED__
#define __VSF_USBD_COMMON_H_INCLUDED__

#include "compiler.h"

#define USB_SETUP_PKG_SIZE					8

#define USB_DESC_SIZE_DEVICE				18
#define USB_DESC_SIZE_CONFIGURATION			9
#define USB_DESC_SIZE_INTERFACE				7
#define USB_DESC_SIZE_ENDPOINT				7

#define USB_STATUS_SIZE						2
#define USB_CONFIGURATION_SIZE				1
#define USB_ALTERNATE_SETTING_SIZE			1

#define USB_DESC_DEVICE_OFF_EP0SIZE			7
#define USB_DESC_DEVICE_OFF_CFGNUM			17
#define USB_DESC_CONFIG_OFF_CFGVAL			5
#define USB_DESC_CONFIG_OFF_BMATTR			7
#define USB_DESC_CONFIG_OFF_IFNUM			4
#define USB_DESC_EP_OFF_EPADDR				2
#define USB_DESC_EP_OFF_EPATTR				3
#define USB_DESC_EP_OFF_EPSIZE				4

// description type
enum usb_description_type_t
{
	USB_DESC_TYPE_DEVICE					= 0x01,
	USB_DESC_TYPE_CONFIGURATION				= 0x02,
	USB_DESC_TYPE_STRING					= 0x03,
	USB_DESC_TYPE_INTERFACE					= 0x04,
	USB_DESC_TYPE_ENDPOINT					= 0x05,
	USB_DESC_TYPE_IAD						= 0x0B,
};

// request
enum usb_stdreq_t
{
	// standard request
	USB_REQ_GET_STATUS						= 0x00,
	USB_REQ_CLEAR_FEATURE					= 0x01,
	USB_REQ_SET_FEATURE						= 0x03,
	USB_REQ_SET_ADDRESS						= 0x05,
	USB_REQ_GET_DESCRIPTOR					= 0x06,
	USB_REQ_SET_DESCRIPTOR					= 0x07,
	USB_REQ_GET_CONFIGURATION				= 0x08,
	USB_REQ_SET_CONFIGURATION				= 0x09,
	USB_REQ_GET_INTERFACE					= 0x0A,
	USB_REQ_SET_INTERFACE					= 0x0B,
};

#define USB_REQ_TYPE_STANDARD				0x00
#define USB_REQ_TYPE_CLASS					0x20
#define USB_REQ_TYPE_VENDOR					0x40
#define USB_REQ_TYPE_MASK					0x60
#define USB_REQ_GET_TYPE(req)				((req) & USB_REQ_TYPE_MASK)

#define USB_REQ_DIR_HTOD					0x00
#define USB_REQ_DIR_DTOH					0x80
#define USB_REQ_DIR_MASK					0x80
#define USB_REQ_GET_DIR(req)				((req) & USB_REQ_DIR_MASK)

#define USB_REQ_RECP_DEVICE					0x00
#define USB_REQ_RECP_INTERFACE				0x01
#define USB_REQ_RECP_ENDPOINT				0x02
#define USB_REQ_RECP_OTHER					0x03
#define USB_REQ_RECP_MASK					0x1F
#define USB_REQ_GET_RECP(req)				((req) & USB_REQ_RECP_MASK)

#define USB_DCLASS_HUB						0x09

// feature
enum usb_feature_cmd_t
{
	USB_EP_FEATURE_CMD_HALT					= 0x00,
	USB_DEV_FEATURE_CMD_REMOTE_WAKEUP		= 0x01,
};

// attributes
enum usb_config_attributes_t
{
	USB_CFGATTR_SELFPOWERED					= 0x40,
	USB_CFGATTR_REMOTE_WEAKUP				= 0x20,
};

// device feature
enum usb_device_feature_t
{
	USB_DEV_FEATURE_SELFPOWERED				= 0x01,
	USB_DEV_FEATURE_REMOTE_WEAKUP			= 0x02,
};

PACKED_HEAD struct PACKED_MID usb_ctrl_request_t
{
	uint8_t type;
	uint8_t request;
	uint16_t value;
	uint16_t index;
	uint16_t length;
}; PACKED_TAIL



/* USB constants */

/* Device and/or Interface Class codes */
#define USB_CLASS_PER_INTERFACE				0	/* for DeviceClass */
#define USB_CLASS_AUDIO						1
#define USB_CLASS_COMM						2
#define USB_CLASS_HID						3
#define USB_CLASS_PRINTER					7
#define USB_CLASS_MASS_STORAGE				8
#define USB_CLASS_HUB						9
#define USB_CLASS_DATA						10
#define USB_CLASS_VENDOR_SPEC				0xff

/* some HID sub classes */
#define USB_SUB_HID_NONE					0
#define USB_SUB_HID_BOOT					1

/* some UID Protocols */
#define USB_PROT_HID_NONE					0
#define USB_PROT_HID_KEYBOARD				1
#define USB_PROT_HID_MOUSE					2


/* Sub STORAGE Classes */
#define US_SC_RBC							1		/* Typically, flash devices */
#define US_SC_8020							2		/* CD-ROM */
#define US_SC_QIC							3		/* QIC-157 Tapes */
#define US_SC_UFI							4		/* Floppy */
#define US_SC_8070							5		/* Removable media */
#define US_SC_SCSI							6		/* Transparent */
#define US_SC_MIN							US_SC_RBC
#define US_SC_MAX							US_SC_SCSI

/* STORAGE Protocols */
#define US_PR_CB							1		/* Control/Bulk w/o interrupt */
#define US_PR_CBI							0		/* Control/Bulk/Interrupt */
#define US_PR_BULK							0x50	/* bulk only */

/* USB types */
#define USB_TYPE_STANDARD					(0x00 << 5)
#define USB_TYPE_CLASS						(0x01 << 5)
#define USB_TYPE_VENDOR						(0x02 << 5)
#define USB_TYPE_RESERVED					(0x03 << 5)

/* USB recipients */
#define USB_RECIP_DEVICE					0x00
#define USB_RECIP_INTERFACE					0x01
#define USB_RECIP_ENDPOINT					0x02
#define USB_RECIP_OTHER						0x03

/* USB directions */
#define USB_DIR_OUT							0x00
#define USB_DIR_IN							0x80

/* USB device speeds */
#define USB_SPEED_FULL						0x0		/* 12Mbps */
#define USB_SPEED_LOW						0x1		/* 1.5Mbps */
#define USB_SPEED_HIGH						0x2		/* 480Mbps */
#define USB_SPEED_RESERVED					0x3

/* Descriptor types */
#define USB_DT_DEVICE						0x01
#define USB_DT_CONFIG						0x02
#define USB_DT_STRING						0x03
#define USB_DT_INTERFACE					0x04
#define USB_DT_ENDPOINT						0x05

#define USB_DT_HID							(USB_TYPE_CLASS | 0x01)
#define USB_DT_REPORT						(USB_TYPE_CLASS | 0x02)
#define USB_DT_PHYSICAL						(USB_TYPE_CLASS | 0x03)
#define USB_DT_HUB							(USB_TYPE_CLASS | 0x09)

/* Descriptor sizes per descriptor type */
#define USB_DT_DEVICE_SIZE					18
#define USB_DT_CONFIG_SIZE					9
#define USB_DT_INTERFACE_SIZE				9
#define USB_DT_ENDPOINT_SIZE				7
#define USB_DT_ENDPOINT_AUDIO_SIZE			9		/* Audio extension */
#define USB_DT_HUB_NONVAR_SIZE				7
#define USB_DT_HID_SIZE						9

/* Endpoints */
#define USB_ENDPOINT_NUMBER_MASK			0x0f	/* in bEndpointAddress */
#define USB_ENDPOINT_DIR_MASK				0x80

#define USB_ENDPOINT_XFERTYPE_MASK			0x03	/* in bmAttributes */
#define USB_ENDPOINT_XFER_CONTROL			0
#define USB_ENDPOINT_XFER_ISOC				1
#define USB_ENDPOINT_XFER_BULK				2
#define USB_ENDPOINT_XFER_INT				3

/* USB Packet IDs (PIDs) */
#define USB_PID_UNDEF_0						0xf0
#define USB_PID_OUT							0xe1
#define USB_PID_ACK							0xd2
#define USB_PID_DATA0						0xc3
#define USB_PID_UNDEF_4						0xb4
#define USB_PID_SOF							0xa5
#define USB_PID_UNDEF_6						0x96
#define USB_PID_UNDEF_7						0x87
#define USB_PID_UNDEF_8						0x78
#define USB_PID_IN							0x69
#define USB_PID_NAK							0x5a
#define USB_PID_DATA1						0x4b
#define USB_PID_PREAMBLE					0x3c
#define USB_PID_SETUP						0x2d
#define USB_PID_STALL						0x1e
#define USB_PID_UNDEF_F						0x0f

/* Standard requests */
/*
#define USB_REQ_GET_STATUS					0x00
#define USB_REQ_CLEAR_FEATURE				0x01
#define USB_REQ_SET_FEATURE					0x03
#define USB_REQ_SET_ADDRESS					0x05
#define USB_REQ_GET_DESCRIPTOR				0x06
#define USB_REQ_SET_DESCRIPTOR				0x07
#define USB_REQ_GET_CONFIGURATION			0x08
#define USB_REQ_SET_CONFIGURATION			0x09
#define USB_REQ_GET_INTERFACE				0x0A
#define USB_REQ_SET_INTERFACE				0x0B
#define USB_REQ_SYNCH_FRAME					0x0C
*/
/* HID requests */
#define USB_REQ_GET_REPORT					0x01
#define USB_REQ_GET_IDLE					0x02
#define USB_REQ_GET_PROTOCOL				0x03
#define USB_REQ_SET_REPORT					0x09
#define USB_REQ_SET_IDLE					0x0A
#define USB_REQ_SET_PROTOCOL				0x0B


/* "pipe" definitions */
#define PIPE_ISOCHRONOUS					0
#define PIPE_INTERRUPT						1
#define PIPE_CONTROL						2
#define PIPE_BULK							3
#define PIPE_DEVEP_MASK						0x0007ff00

#define USB_ISOCHRONOUS						0
#define USB_INTERRUPT						1
#define USB_CONTROL							2
#define USB_BULK							3

/* USB-status codes: */
#define USB_ST_ACTIVE						0x0001	/* TD is active */
#define USB_ST_STALLED						0x0002	/* TD is stalled */
#define USB_ST_BUF_ERR						0x0004	/* buffer error */
#define USB_ST_BABBLE_DET					0x0008	/* Babble detected */
#define USB_ST_NAK_REC						0x0010	/* NAK Received*/
#define USB_ST_CRC_ERR						0x0020	/* CRC/timeout Error */
#define USB_ST_BIT_ERR						0x0040	/* Bitstuff error */
#define USB_ST_NOT_PROC						(-1)	/* Not yet processed */


/*************************************************************************
* Hub defines
*/

/*
 * Hub request types
 */

#define USB_RT_HUB							(USB_TYPE_CLASS | USB_RECIP_DEVICE)
#define USB_RT_PORT							(USB_TYPE_CLASS | USB_RECIP_OTHER)

/*
 * Hub Class feature numbers
 */
#define C_HUB_LOCAL_POWER					0
#define C_HUB_OVER_CURRENT					1

/*
 * Port feature numbers
 */
#define USB_PORT_FEAT_CONNECTION			0
#define USB_PORT_FEAT_ENABLE				1
#define USB_PORT_FEAT_SUSPEND				2
#define USB_PORT_FEAT_OVER_CURRENT			3
#define USB_PORT_FEAT_RESET					4
#define USB_PORT_FEAT_POWER					8
#define USB_PORT_FEAT_LOWSPEED				9
#define USB_PORT_FEAT_HIGHSPEED				10
#define USB_PORT_FEAT_C_CONNECTION			16
#define USB_PORT_FEAT_C_ENABLE				17
#define USB_PORT_FEAT_C_SUSPEND				18
#define USB_PORT_FEAT_C_OVER_CURRENT		19
#define USB_PORT_FEAT_C_RESET				20

/* wPortStatus bits */
#define USB_PORT_STAT_CONNECTION			0x0001
#define USB_PORT_STAT_ENABLE				0x0002
#define USB_PORT_STAT_SUSPEND				0x0004
#define USB_PORT_STAT_OVERCURRENT			0x0008
#define USB_PORT_STAT_RESET					0x0010
#define USB_PORT_STAT_POWER					0x0100
#define USB_PORT_STAT_LOW_SPEED				0x0200
#define USB_PORT_STAT_HIGH_SPEED			0x0400	/* support for EHCI */
#define USB_PORT_STAT_SPEED	\
	(USB_PORT_STAT_LOW_SPEED | USB_PORT_STAT_HIGH_SPEED)

/* wPortChange bits */
#define USB_PORT_STAT_C_CONNECTION			0x0001
#define USB_PORT_STAT_C_ENABLE				0x0002
#define USB_PORT_STAT_C_SUSPEND				0x0004
#define USB_PORT_STAT_C_OVERCURRENT			0x0008
#define USB_PORT_STAT_C_RESET				0x0010

/* wHubCharacteristics (masks) */
#define HUB_CHAR_LPSM						0x0003
#define HUB_CHAR_COMPOUND					0x0004
#define HUB_CHAR_OCPM						0x0018

/*
*Hub Status & Hub Change bit masks
*/
#define HUB_STATUS_LOCAL_POWER				0x0001
#define HUB_STATUS_OVERCURRENT				0x0002

#define HUB_CHANGE_LOCAL_POWER				0x0001
#define HUB_CHANGE_OVERCURRENT				0x0002

/*
 * Calling this entity a "pipe" is glorifying it. A USB pipe
 * is something embarrassingly simple: it basically consists
 * of the following information:
 *  - device number (7 bits)
 *  - endpoint number (4 bits)
 *  - current Data0/1 state (1 bit)
 *  - direction (1 bit)
 *  - speed (2 bits)
 *  - max packet size (2 bits: 8, 16, 32 or 64)
 *  - pipe type (2 bits: control, interrupt, bulk, isochronous)
 *
 * That's 18 bits. Really. Nothing more. And the USB people have
 * documented these eighteen bits as some kind of glorious
 * virtual data structure.
 *
 * Let's not fall in that trap. We'll just encode it as a simple
 * unsigned int. The encoding is:
 *
 *  - max size:     bits 0-1    (00 = 8, 01 = 16, 10 = 32, 11 = 64)
 *  - direction:    bit 7       (0 = Host-to-Device [Out],
 *                  (1 = Device-to-Host [In])
 *  - device:       bits 8-14
 *  - endpoint:     bits 15-18
 *  - Data0/1:      bit 19
 *  - speed:        bit 26      (0 = Full, 1 = Low Speed, 2 = High)
 *  - pipe type:    bits 30-31  (00 = isochronous, 01 = interrupt,
 *                   10 = control, 11 = bulk)
 *
 * Why? Because it's arbitrary, and whatever encoding we select is really
 * up to us. This one happens to share a lot of bit positions with the UHCI
 * specification, so that much of the uhci driver can just mask the bits
 * appropriately.
 */
/* Create various pipes... */
#define create_pipe(dev,endpoint)			\
			(((dev)->devnum << 8) | (endpoint << 15) | \
					(((dev)->speed == USB_SPEED_LOW) << 26))
#define default_pipe(dev)					((dev)->speed << 26)

#define usb_sndctrlpipe(dev, endpoint)		\
			((PIPE_CONTROL << 30) | create_pipe(dev, endpoint) | USB_DIR_OUT)
#define usb_rcvctrlpipe(dev, endpoint)		\
			((PIPE_CONTROL << 30) | create_pipe(dev, endpoint) | USB_DIR_IN)
#define usb_sndisocpipe(dev, endpoint)		\
			((PIPE_ISOCHRONOUS << 30) | create_pipe(dev, endpoint) | USB_DIR_OUT)
#define usb_rcvisocpipe(dev, endpoint)		\
			((PIPE_ISOCHRONOUS << 30) | create_pipe(dev, endpoint) | USB_DIR_IN)
#define usb_sndbulkpipe(dev, endpoint)		\
			((PIPE_BULK << 30) | create_pipe(dev, endpoint) | USB_DIR_OUT)
#define usb_rcvbulkpipe(dev, endpoint)		\
			((PIPE_BULK << 30) | create_pipe(dev, endpoint) | USB_DIR_IN)
#define usb_sndintpipe(dev, endpoint)		\
			((PIPE_INTERRUPT << 30) | create_pipe(dev, endpoint) | USB_DIR_OUT)
#define usb_rcvintpipe(dev, endpoint)		\
			((PIPE_INTERRUPT << 30) | create_pipe(dev, endpoint) | USB_DIR_IN)
#define usb_snddefctrl(dev)					\
			((PIPE_CONTROL << 30) | default_pipe(dev) | USB_DIR_OUT)
#define usb_rcvdefctrl(dev)					\
			((PIPE_CONTROL << 30) | default_pipe(dev) | USB_DIR_IN)

/* The D0/D1 toggle bits */
#define usb_gettoggle(dev, ep, out)			(((dev)->toggle[out] >> ep) & 1)
#define usb_dotoggle(dev, ep, out)			((dev)->toggle[out] ^= (1 << ep))
#define usb_settoggle(dev, ep, out, bit)	\
			((dev)->toggle[out] = ((dev)->toggle[out] & ~(1 << ep)) | ((bit) << ep))

/* Endpoint halt control/status */
#define usb_endpoint_out(ep_dir)			(((ep_dir >> 7) & 1) ^ 1)
#define usb_endpoint_halt(dev, ep, out)		((dev)->halted[out] |= (1 << (ep)))
#define usb_endpoint_running(dev, ep, out)	((dev)->halted[out] &= ~(1 << (ep)))
#define usb_endpoint_halted(dev, ep, out)	((dev)->halted[out] & (1 << (ep)))

#define usb_packetid(pipe)					\
			(((pipe) & USB_DIR_IN) ? USB_PID_IN : USB_PID_OUT)

#define usb_pipeout(pipe)					((((pipe) >> 7) & 1) ^ 1)
#define usb_pipein(pipe)					(((pipe) >> 7) & 1)
#define usb_pipedevice(pipe)				(((pipe) >> 8) & 0x7f)
#define usb_pipe_endpdev(pipe)				(((pipe) >> 8) & 0x7ff)
#define usb_pipeendpoint(pipe)				(((pipe) >> 15) & 0xf)
#define usb_pipedata(pipe)					(((pipe) >> 19) & 1)
#define usb_pipespeed(pipe)					(((pipe) >> 26) & 3)
#define usb_pipeslow(pipe)					(usb_pipespeed(pipe) == USB_SPEED_LOW)
#define usb_pipetype(pipe)					(((pipe) >> 30) & 3)
#define usb_pipeisoc(pipe)					(usb_pipetype((pipe)) == PIPE_ISOCHRONOUS)
#define usb_pipeint(pipe)					(usb_pipetype((pipe)) == PIPE_INTERRUPT)
#define usb_pipecontrol(pipe)				(usb_pipetype((pipe)) == PIPE_CONTROL)
#define usb_pipebulk(pipe)					(usb_pipetype((pipe)) == PIPE_BULK)




/*******************************************************
 * limit
 *******************************************************/
#define USB_MAXINTERFACES					4
#define USB_MAXENDPOINTS					8
#define USB_MAXCHILDREN						4	/* This is arbitrary */
#define USB_ALTSETTINGALLOC					4
#define USB_MAXALTSETTING					32

/* String descriptor */
PACKED_HEAD struct PACKED_MID usb_string_descriptor_t {
	unsigned char	bLength;
	unsigned char	bDescriptorType;
	unsigned short	wData[1];
};

/* device request (setup) */
PACKED_HEAD struct PACKED_MID device_request_t {
	unsigned char	requesttype;
	unsigned char	request;
	unsigned short	value;
	unsigned short	index;
	unsigned short	length;
};

/* All standard descriptors have these 2 fields in common */
PACKED_HEAD struct PACKED_MID usb_descriptor_header_t {
	unsigned char	bLength;
	unsigned char	bDescriptorType;
};

/* Device descriptor */
PACKED_HEAD struct PACKED_MID usb_device_descriptor_t {
	unsigned char	bLength;
	unsigned char	bDescriptorType;
	unsigned short	bcdUSB;
	unsigned char	bDeviceClass;
	unsigned char	bDeviceSubClass;
	unsigned char	bDeviceProtocol;
	unsigned char	bMaxPacketSize0;
	unsigned short	idVendor;
	unsigned short	idProduct;
	unsigned short	bcdDevice;
	unsigned char	iManufacturer;
	unsigned char	iProduct;
	unsigned char	iSerialNumber;
	unsigned char	bNumConfigurations;
};

/* Endpoint descriptor */
PACKED_HEAD struct PACKED_MID usb_endpoint_descriptor_t {
	unsigned char	bLength;
	unsigned char	bDescriptorType;
	unsigned char	bEndpointAddress;
	unsigned char	bmAttributes;
	unsigned short	wMaxPacketSize;
	unsigned char	bInterval;
	unsigned char	bRefresh;
	unsigned char	bSynchAddress;
};

/* Interface descriptor */
PACKED_HEAD struct PACKED_MID usb_interface_descriptor_t {
	unsigned char	bLength;
	unsigned char	bDescriptorType;
	unsigned char	bInterfaceNumber;
	unsigned char	bAlternateSetting;
	unsigned char	bNumEndpoints;
	unsigned char	bInterfaceClass;
	unsigned char	bInterfaceSubClass;
	unsigned char	bInterfaceProtocol;
	unsigned char	iInterface;

	struct usb_endpoint_descriptor_t *ep_desc;
};

PACKED_HEAD struct PACKED_MID usb_interface_t {
	struct usb_interface_descriptor_t *interface_desc;
	unsigned char act_altsetting;			/* active alternate setting */
	unsigned char num_altsetting;			/* number of alternate settings */
	unsigned char max_altsetting;			/* total memory allocated */
	struct usb_driver *driver;				/* driver */
	void *private_data;
};

/* Configuration descriptor information.. */
PACKED_HEAD struct PACKED_MID usb_config_descriptor_t {
	unsigned char	bLength;
	unsigned char	bDescriptorType;
	unsigned short	wTotalLength;
	unsigned char	bNumInterfaces;
	unsigned char	bConfigurationValue;
	unsigned char	iConfiguration;
	unsigned char	bmAttributes;
	unsigned char	MaxPower;

	struct usb_interface_t *interface;
};

/* hub stuff */
PACKED_HEAD struct PACKED_MID usb_port_status_t {
	unsigned short	wPortStatus;
	unsigned short	wPortChange;
};

PACKED_HEAD struct PACKED_MID usb_hub_status_t {
	unsigned short	wHubStatus;
	unsigned short	wHubChange;
};

/* Hub descriptor */
PACKED_HEAD struct PACKED_MID usb_hub_descriptor_t {
	unsigned char	bLength;
	unsigned char	bDescriptorType;
	unsigned char	bNbrPorts;
	unsigned short	wHubCharacteristics;
	unsigned char	bPwrOn2PwrGood;
	unsigned char	bHubContrCurrent;
	unsigned char	DeviceRemovable[(USB_MAXCHILDREN + 1 + 7) / 8];
	unsigned char	PortPowerCtrlMask[(USB_MAXCHILDREN + 1 + 7) / 8];
	/* DeviceRemovable and PortPwrCtrlMask want to be variable-length
	bitmaps that hold max 255 entries. (bit0 is ignored) */
};

#endif	// __VSF_USBD_COMMON_H_INCLUDED__
