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
#ifndef __VSF_OHCI_H___
#define __VSF_OHCI_H___

#define CC_ERR_CRC				0xf0003001	/* OHCI CC - CRC Error */
#define CC_ERR_BITSTUFF			0xf0003002	/* OHCI CC - Bit Stuff */
#define CC_ERR_DATA_TOGGLE		0xf0003003	/* OHCI CC - Data Togg */
#define CC_ERR_STALL			0xf0003004	/* OHCI CC - Stall */
#define CC_ERR_NORESPONSE		0xf0003005	/* OHCI CC - Device Not Responding */
#define CC_ERR_PID_CHECK		0xf0003006	/* OHCI CC - PID Check Failure */
#define CC_ERR_INVALID_PID		0xf0003007	/* OHCI CC - Invalid PID */
#define CC_ERR_DATAOVERRUN		0xf0003008	/* OHCI CC - Data Overrun */
#define CC_ERR_DATAUNDERRUN		0xf0003009	/* OHCI CC - Data Underrun */
#define CC_ERR_NOT_DEFINED		0xf0003010
#define CC_ERR_BUFFEROVERRUN	0xf0003012	/* OHCI CC - Buffer Overrun */
#define CC_ERR_BUFFERUNDERRUN	0xf0003013	/* OHCI CC - Buffer Underrun */
#define CC_ERR_NOT_ACCESS		0xf0003014	/* OHCI CC - Not Access */


/**************************************************************************
 * host config
 **************************************************************************/
#define N_URB_TD				12

/**************************************************************************
 * export data
 **************************************************************************/
extern const struct vsfusbh_hcddrv_t ohci_drv;

#endif
