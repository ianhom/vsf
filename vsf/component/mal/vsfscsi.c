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

vsf_err_t vsfscsi_init(struct vsfscsi_device_t *dev)
{
	uint32_t i;
	struct vsfscsi_lun_t *lun = dev->lun;

	for (i = 0; i <= dev->max_lun; i++, lun++)
	{
		lun->sensekey = SCSI_SENSEKEY_NO_SENSE;
		lun->asc = (enum SCSI_asc_t)0;
		if (lun->op->init != NULL)
		{
			lun->op->init(lun);
		}
	}
	return VSFERR_NONE;
}

static struct vsfscsi_handler_t*
vsfscsi_get_handler(struct vsfscsi_handler_t *handlers, enum SCSI_cmd_t cmd)
{
	while (handlers->handler != NULL)
	{
		if (handlers->cmd == cmd)
		{
			return handlers;
		}
		handlers++;
	}
	return NULL;
}

vsf_err_t vsfscsi_execute_nb(struct vsfscsi_lun_t *lun, uint8_t *CB,
					uint32_t *data_size, struct vsfscsi_transact_t **transact)
{
	if (lun->op->execute != NULL)
	{
		return lun->op->execute(lun, CB, data_size, transact);
	}
	else if (lun->op->handlers != NULL)
	{
		struct vsfscsi_handler_t *handler =
				vsfscsi_get_handler(lun->op->handlers, (enum SCSI_cmd_t)CB[0]);
		if (handler != NULL)
		{
			return handler->handler(lun, CB, data_size, transact);
		}
	}

	return VSFERR_FAIL;
}

void vsfscsi_cancel_transact(struct vsfscsi_transact_t *transact)
{
}

// mal2scsi
#define MAL2SCSI_HANDLER_NAME(cmd)		SCSI_HANDLER_NAME(mal2scsi, cmd)
#define MAL2SCSI_HANDLER(cmd)			\
static vsf_err_t						\
MAL2SCSI_HANDLER_NAME(cmd)(struct vsfscsi_lun_t *lun, uint8_t *CB,\
			uint32_t *data_size, struct vsfscsi_transact_t **transact)

MAL2SCSI_HANDLER(TEST_UNIT_READY)
{
	struct vsf_mal2scsi_t *mal2scsi = (struct vsf_mal2scsi_t *)lun->param;

	if (!mal2scsi->mal_ready)
	{
		lun->sensekey = SCSI_SENSEKEY_NOT_READY;
		lun->asc = (enum SCSI_asc_t)0;
		return VSFERR_FAIL;
	}

	lun->sensekey = SCSI_SENSEKEY_NO_SENSE;
	lun->asc = (enum SCSI_asc_t)0;
	return VSFERR_NONE;
}

MAL2SCSI_HANDLER(INQUIRY)
{
	return VSFERR_NONE;
}

MAL2SCSI_HANDLER(REQUEST_SENSE)
{
	return VSFERR_NONE;
}

#define MAL2SCSI_HANDLER_DEFINE(cmd)	\
	{SCSI_CMD_##cmd, MAL2SCSI_HANDLER_NAME(cmd)}
static const struct vsfscsi_handler_t vsf_mal2scsi_handlers[] =
{
	MAL2SCSI_HANDLER_DEFINE(TEST_UNIT_READY),
	MAL2SCSI_HANDLER_DEFINE(INQUIRY),
	MAL2SCSI_HANDLER_DEFINE(REQUEST_SENSE),
	SCSI_HANDLER_NULL,
};

vsf_err_t vsf_mal2scsi_init(struct vsfscsi_lun_t *lun)
{
	struct vsf_mal2scsi_t *mal2scsi = (struct vsf_mal2scsi_t *)lun->param;

	mal2scsi->mal_ready = false;
	return vsf_malstream_init(&mal2scsi->malstream);
}

const struct vsfscsi_lun_op_t vsf_mal2scsi_op =
{
	.init = vsf_mal2scsi_init,
	.execute = NULL,
	.cancel = NULL,
	.handlers = (struct vsfscsi_handler_t *)vsf_mal2scsi_handlers,
};
