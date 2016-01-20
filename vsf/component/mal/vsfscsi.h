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

#ifndef __VSFSCSI_H_INCLUDED__
#define __VSFSCSI_H_INCLUDED__

enum SCSI_sensekey_t
{
	SCSI_SENSEKEY_NO_SENSE						= 0,
	SCSI_SENSEKEY_RECOVERED_ERROR				= 1,
	SCSI_SENSEKEY_NOT_READY						= 2,
	SCSI_SENSEKEY_MEDIUM_ERROR					= 3,
	SCSI_SENSEKEY_HARDWARE_ERROR				= 4,
	SCSI_SENSEKEY_ILLEGAL_REQUEST				= 5,
	SCSI_SENSEKEY_UNIT_ATTENTION				= 6,
	SCSI_SENSEKEY_DATA_PROTECT					= 7,
	SCSI_SENSEKEY_BLANK_CHECK					= 8,
	SCSI_SENSEKEY_VENDOR_SPECIFIC				= 9,
	SCSI_SENSEKEY_COPY_ABORTED					= 10,
	SCSI_SENSEKEY_ABORTED_COMMAND				= 11,
	SCSI_SENSEKEY_VOLUME_OVERFLOW				= 13,
	SCSI_SENSEKEY_MISCOMPARE					= 14,
};

enum SCSI_asc_t
{
	SCSI_ASC_PARAMETER_LIST_LENGTH_ERROR		= 0x1A,
	SCSI_ASC_INVALID_COMMAND					= 0x20,
	SCSI_ASC_INVALID_FIELD_IN_COMMAND			= 0x24,
	SCSI_ASC_INVALID_FIELD_IN_PARAMETER_LIST	= 0x26,
	SCSI_ASC_MEDIUM_HAVE_CHANGED				= 0x28,
	SCSI_ASC_ADDRESS_OUT_OF_RANGE				= 0x21,
	SCSI_ASC_MEDIUM_NOT_PRESENT					= 0x3A,
};

enum SCSI_cmd_t
{
	SCSI_CMD_FORMAT_UNIT						= 0x04,
	SCSI_CMD_INQUIRY							= 0x12,
	SCSI_CMD_MODE_SELECT6						= 0x15,
	SCSI_CMD_MODE_SELECT10						= 0x55,
	SCSI_CMD_MODE_SENSE6						= 0x1A,
	SCSI_CMD_MODE_SENSE10						= 0x5A,
	SCSI_CMD_ALLOW_MEDIUM_REMOVAL				= 0x1E,
	SCSI_CMD_READ6								= 0x08,
	SCSI_CMD_READ10								= 0x28,
	SCSI_CMD_READ12								= 0xA8,
	SCSI_CMD_READ16								= 0x88,
	SCSI_CMD_READ_CAPACITY10					= 0X25,
	SCSI_CMD_READ_CAPACITY16					= 0x9E,
	SCSI_CMD_REQUEST_SENSE						= 0x03,
	SCSI_CMD_START_STOP_UNIT					= 0x1B,
	SCSI_CMD_TEST_UNIT_READY					= 0x00,
	SCSI_CMD_WRITE6								= 0x0A,
	SCSI_CMD_WRITE10							= 0x2A,
	SCSI_CMD_WRITE12							= 0xAA,
	SCSI_CMD_WRITE16							= 0x8A,
	SCSI_CMD_VERIFY10							= 0x2F,
	SCSI_CMD_VERIFY12							= 0xAF,
	SCSI_CMD_VERIFY16							= 0x8F,
	SCSI_CMD_SEND_DIAGNOSTIC					= 0x1D,
	SCSI_CMD_READ_FORMAT_CAPACITIES				= 0x23,
	SCSI_CMD_GET_EVENT_STATUS_NOTIFICATION		= 0x4A,
	SCSI_CMD_READ_TOC							= 0x43,
};

struct vsfscsi_device_t;
struct vsfscsi_lun_t;
struct vsfscsi_transact_t
{
	struct vsfsm_t sm;
	struct vsfsm_pt_t pt;

	struct vsf_stream_t *stream;
	struct vsfscsi_lun_t *lun;
};

struct vsfscsi_lun_op_t
{
	vsf_err_t (*init)(struct vsfscsi_lun_t *lun);
	vsf_err_t (*execute)(struct vsfscsi_lun_t *lun, uint8_t *CB,
					uint32_t *data_size, struct vsfscsi_transact_t **transact);
	void (*cancel)(struct vsfscsi_transact_t *transact);

	struct vsfscsi_handler_t *handlers;
};

struct vsfscsi_lun_t
{
	struct vsfscsi_lun_op_t *op;
	void *param;

	enum SCSI_sensekey_t sensekey;
	enum SCSI_asc_t asc;

	// private
	struct vsfscsi_transact_t transact[1];
};

#define SCSI_HANDLER_NAME(title, cmd)			title##_##cmd
#define SCSI_HANDLER_NULL						{(enum SCSI_cmd_t)0, NULL}
struct vsfscsi_handler_t
{
	enum SCSI_cmd_t cmd;
	vsf_err_t (*handler)(struct vsfscsi_lun_t *lun, uint8_t *CB,
					uint32_t *data_size, struct vsfscsi_transact_t **transact);
};

struct vsfscsi_device_t
{
	uint8_t max_lun;
	struct vsfscsi_lun_t *lun;
};

vsf_err_t vsfscsi_init(struct vsfscsi_device_t *dev);
vsf_err_t vsfscsi_execute_nb(struct vsfscsi_lun_t *lun, uint8_t *CB,
					uint32_t *data_size, struct vsfscsi_transact_t **transact);
void vsfscsi_cancel_transact(struct vsfscsi_transact_t *transact);

// mal2scsi
// lun->param is pointer to vsf_mal2scsi_t
struct vsf_mal2scsi_cparam_t
{
	const bool removalbe;
	const char vendor[8];
	const char product[16];
	const char revision[4];
};

struct vsf_mal2scsi_t
{
	struct vsf_mal2scsi_cparam_t const cparam;
	bool mal_ready;
	struct vsf_malstream_t malstream;
	struct vsf_buffer_stream_t bufstream;
};
extern const struct vsfscsi_lun_op_t vsf_mal2scsi_op;

#endif // __VSFSCSI_H_INCLUDED__
