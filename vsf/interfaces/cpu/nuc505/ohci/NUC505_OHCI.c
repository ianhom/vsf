/**************************************************************************
 *  Copyright (C) 2008 - 2010 by Simon Qian                               *
 *  SimonQian@SimonQian.com                                               *
 *                                                                        *
 *  Project:    Versaloon                                                 *
 *  File:       GPIO.h                                                    *
 *  Author:     SimonQian                                                 *
 *  Versaion:   See changelog                                             *
 *  Purpose:    GPIO interface header file                                *
 *  License:    See license                                               *
 *------------------------------------------------------------------------*
 *  Change Log:                                                           *
 *      YYYY-MM-DD:     What(by Who)                                      *
 *      2008-11-07:     created(by SimonQian)                             *
 **************************************************************************/

#include "app_type.h"
#include "interfaces.h"

#if IFS_OHCI_EN

#include "NUC400_OHCI.h"
#include "NUC472_442.h"

extern void nuc400_unlock_reg(void);
extern void nuc400_lock_reg(void);

//GPB_MFPL_PB0MFP
#define SYS_GPB_MFPL_PB0MFP_GPIO			(0x0UL<<SYS_GPB_MFPL_PB0MFP_Pos)
#define SYS_GPB_MFPL_PB0MFP_USB0_OTG5V_ST	(0x1UL<<SYS_GPB_MFPL_PB0MFP_Pos)
#define SYS_GPB_MFPL_PB0MFP_I2C4_SCL		(0x2UL<<SYS_GPB_MFPL_PB0MFP_Pos)
#define SYS_GPB_MFPL_PB0MFP_INT11			(0x8UL<<SYS_GPB_MFPL_PB0MFP_Pos)

//GPB_MFPL_PB1MFP
#define SYS_GPB_MFPL_PB1MFP_GPIO			(0x0UL<<SYS_GPB_MFPL_PB1MFP_Pos)
#define SYS_GPB_MFPL_PB1MFP_USB0_OTG5V_EN	(0x1UL<<SYS_GPB_MFPL_PB1MFP_Pos)
#define SYS_GPB_MFPL_PB1MFP_I2C4_SDA		(0x2UL<<SYS_GPB_MFPL_PB1MFP_Pos)
#define SYS_GPB_MFPL_PB1MFP_TM1_CNT_OUT		(0x3UL<<SYS_GPB_MFPL_PB1MFP_Pos)

//GPB_MFPL_PB2MFP
#define SYS_GPB_MFPL_PB2MFP_GPIO			(0x0UL<<SYS_GPB_MFPL_PB2MFP_Pos)
#define SYS_GPB_MFPL_PB2MFP_UART1_RXD		(0x1UL<<SYS_GPB_MFPL_PB2MFP_Pos)
#define SYS_GPB_MFPL_PB2MFP_SPI2_SS0		(0x2UL<<SYS_GPB_MFPL_PB2MFP_Pos)
#define SYS_GPB_MFPL_PB2MFP_USB1_D_N		(0x3UL<<SYS_GPB_MFPL_PB2MFP_Pos)
#define SYS_GPB_MFPL_PB2MFP_EBI_AD4			(0x7UL<<SYS_GPB_MFPL_PB2MFP_Pos)

//GPB_MFPL_PB3MFP
#define SYS_GPB_MFPL_PB3MFP_GPIO			(0x0UL<<SYS_GPB_MFPL_PB3MFP_Pos)
#define SYS_GPB_MFPL_PB3MFP_UART1_TXD		(0x1UL<<SYS_GPB_MFPL_PB3MFP_Pos)
#define SYS_GPB_MFPL_PB3MFP_SPI2_CLK		(0x2UL<<SYS_GPB_MFPL_PB3MFP_Pos)
#define SYS_GPB_MFPL_PB3MFP_USB1_D_P		(0x3UL<<SYS_GPB_MFPL_PB3MFP_Pos)
#define SYS_GPB_MFPL_PB3MFP_EBI_AD5			(0x7UL<<SYS_GPB_MFPL_PB3MFP_Pos)

struct interface_ohci_irq_t nuc400_ohci_irq;

ROOTFUNC void USBH_IRQHandler(void)
{
	if (nuc400_ohci_irq.irq != NULL)
	{
		nuc400_ohci_irq.irq(nuc400_ohci_irq.param);
	}
}

vsf_err_t nuc400_ohci_init(uint8_t index)
{
	switch (index)
	{
	case 0:
		nuc400_unlock_reg();
		// host-only
		SYS->GPB_MFPL = (SYS->GPB_MFPL & ~(SYS_GPB_MFPL_PB0MFP_USB0_OTG5V_ST | SYS_GPB_MFPL_PB1MFP_USB0_OTG5V_EN)) | 0x11;
		SYS->GPB_MFPL = (SYS->GPB_MFPL & ~(SYS_GPB_MFPL_PB2MFP_USB1_D_N | SYS_GPB_MFPL_PB3MFP_USB1_D_P)) | 0x3300;
		CLK->CLKDIV0 = (CLK->CLKDIV0 & ~CLK_CLKDIV0_USBHDIV_Msk) | (2 << CLK_CLKDIV0_USBHDIV_Pos);
		CLK->CLKSEL0 |= CLK_CLKSEL0_USBHSEL_Msk;
		CLK->AHBCLK |= CLK_AHBCLK_USBHCKEN_Msk;
		nuc400_lock_reg();
		
		if (nuc400_ohci_irq.irq != NULL)
		{
			NVIC_SetPriority(USBH_IRQn,5);
			NVIC_EnableIRQ(USBH_IRQn);
		}
		return VSFERR_NONE;
	default:
		return VSFERR_NOT_SUPPORT;
	}
}

vsf_err_t nuc400_ohci_fini(uint8_t index)
{
	switch (index)
	{
	case 0:
		return VSFERR_NONE;
	default:
		return VSFERR_NOT_SUPPORT;
	}
}

void* nuc400_ohci_regbase(uint8_t index)
{
	switch (index)
	{
	case 0:
		return (void*)USBH;
	default:
		return NULL;
	}
}

#endif
