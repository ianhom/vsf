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
#include "compiler.h"
#include "interfaces.h"

#if IFS_SPI_EN

#include "NUC505Series.h"
#include "core.h"

struct
{
	void (*onready)(void *);
	void *param;
	void *in;
	void *out;
	uint32_t prt;
	uint32_t len;
	uint8_t start : 1;
	uint8_t in_num: 7;
} static nuc505_spi_param[SPI_NUM];

vsf_err_t nuc505_spi_init(uint8_t index)
{
	uint8_t spi_idx = index & 0x0F;

#if __VSF_DEBUG__
	if (spi_idx >= SPI_NUM)
	{
		return VSFERR_NOT_SUPPORT;
	}
#endif

	switch (spi_idx)
	{
	#if SPI0_ENABLE
	case 0:
		// clk, mosi, miso
		SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB3MFP_Msk);
		SYS->GPB_MFPL |= 1 << SYS_GPB_MFPL_PB3MFP_Pos;
		SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB4MFP_Msk);
		SYS->GPB_MFPL |= 1 << SYS_GPB_MFPL_PB4MFP_Pos;
		SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB5MFP_Msk);
		SYS->GPB_MFPL |= 1 << SYS_GPB_MFPL_PB5MFP_Pos;
		CLK->CLKDIV2 &= ~CLK_CLKDIV2_SPI0SEL_Msk;
		CLK->APBCLK |= CLK_APBCLK_SPI0CKEN_Msk;
		SYS->IPRST1 |= SYS_IPRST1_SPI0RST_Msk;
		SYS->IPRST1 &= ~SYS_IPRST1_SPI0RST_Msk;
		NVIC_EnableIRQ(SPI0_IRQn);
		break;
	#endif // SPI0_ENABLE
	#if SPI1_ENABLE
	case 1:
		// clk, mosi, miso
		SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB11MFP_Msk);
		SYS->GPB_MFPH |= 1 << SYS_GPB_MFPH_PB11MFP_Pos;
		SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk);
		SYS->GPB_MFPH |= 1 << SYS_GPB_MFPH_PB12MFP_Pos;
		SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB13MFP_Msk);
		SYS->GPB_MFPH |= 1 << SYS_GPB_MFPH_PB13MFP_Pos;
		CLK->CLKDIV2 &= ~CLK_CLKDIV2_SPI1SEL_Msk;
		CLK->APBCLK |= CLK_APBCLK_SPI1CKEN_Msk;
		SYS->IPRST1 |= SYS_IPRST1_SPI1RST_Msk;
		SYS->IPRST1 &= ~SYS_IPRST1_SPI1RST_Msk;
		NVIC_EnableIRQ(SPI1_IRQn);
		break;
	#endif // SPI1_ENABLE
	default:
		return VSFERR_NOT_SUPPORT;
	}

	return VSFERR_NONE;
}

vsf_err_t nuc505_spi_fini(uint8_t index)
{
	uint8_t spi_idx = index & 0x0F;

#if __VSF_DEBUG__
	if (spi_idx >= SPI_NUM)
	{
		return VSFERR_NOT_SUPPORT;
	}
#endif

	switch (spi_idx)
	{
	#if SPI0_ENABLE
	case 0:
		SYS->IPRST1 |= SYS_IPRST1_SPI0RST_Msk;
		SYS->IPRST1 &= ~SYS_IPRST1_SPI0RST_Msk;
		CLK->APBCLK &= ~CLK_APBCLK_SPI0CKEN_Msk;
		NVIC_DisableIRQ(SPI0_IRQn);
		break;
	#endif // SPI0_ENABLE
	#if SPI1_ENABLE
	case 1:
		SYS->IPRST1 |= SYS_IPRST1_SPI1RST_Msk;
		SYS->IPRST1 &= ~SYS_IPRST1_SPI1RST_Msk;
		CLK->APBCLK &= ~CLK_APBCLK_SPI1CKEN_Msk;
		NVIC_DisableIRQ(SPI1_IRQn);
		break;
	#endif // SPI1_ENABLE
	default:
		return VSFERR_NOT_SUPPORT;
	}

	return VSFERR_NONE;
}

vsf_err_t nuc505_spi_enable(uint8_t index)
{
	uint8_t spi_idx = index & 0x0F;

#if __VSF_DEBUG__
	if (spi_idx >= SPI_NUM)
	{
		return VSFERR_NOT_SUPPORT;
	}
#endif

	#if SPI0_ENABLE
	if (spi_idx == 0)
	{
		SPI0->CTL |= SPI_CTL_SPIEN_Msk;
	}
	#endif
	#if SPI1_ENABLE
	if (spi_idx == 1)
	{
		SPI1->CTL |= SPI_CTL_SPIEN_Msk;
	}
	#endif

	return VSFERR_NONE;
}

vsf_err_t nuc505_spi_disable(uint8_t index)
{
	uint8_t spi_idx = index & 0x0F;

#if __VSF_DEBUG__
	if (spi_idx >= SPI_NUM)
	{
		return VSFERR_NOT_SUPPORT;
	}
#endif

	#if SPI0_ENABLE
	if (spi_idx == 0)
	{
		SPI0->CTL &= ~SPI_CTL_SPIEN_Msk;
	}
	#endif
	#if SPI1_ENABLE
	if (spi_idx == 1)
	{
		SPI1->CTL &= ~SPI_CTL_SPIEN_Msk;
	}
	#endif

	return VSFERR_NONE;
}

vsf_err_t nuc505_spi_get_ability(uint8_t index, struct spi_ability_t *ability)
{
	struct nuc505_info_t *info;

#if __VSF_DEBUG__
	if (spi_idx >= SPI_NUM)
	{
		return VSFERR_NOT_SUPPORT;
	}
#endif

	if (nuc505_interface_get_info(&info))
	{
		return VSFERR_FAIL;
	}

	ability->max_freq_hz = ability->min_freq_hz = info->osc_freq_hz;
	ability->min_freq_hz /= 256;

	return VSFERR_NONE;
}

vsf_err_t nuc505_spi_config(uint8_t index, uint32_t kHz, uint32_t mode)
{
	uint8_t spi_idx = index & 0x0F;
	struct nuc505_info_t *info;

#if __VSF_DEBUG__
	if (spi_idx >= SPI_NUM)
	{
		return VSFERR_NOT_SUPPORT;
	}
#endif

	if (nuc505_interface_get_info(&info))
	{
		return VSFERR_FAIL;
	}

	SPI_T *spi = spi_idx ? SPI1 : SPI0;

	spi->CLKDIV = info->osc_freq_hz / kHz / 1000 - 1;
	spi->CTL = mode |
				(0x1ul << SPI_CTL_SUSPITV_Pos) | 	// suspend interval = 1.5 spiclk
				(0x8ul << SPI_CTL_DWIDTH_Pos) |		// transmit bit width = 8
				SPI_CTL_UNITIEN_Msk;

	return VSFERR_NONE;
}

vsf_err_t nuc505_spi_config_callback(uint8_t index, uint32_t int_priority,
										void *p, void (*onready)(void *))
{
	uint8_t spi_idx = index & 0x0F;

#if __VSF_DEBUG__
	if (spi_idx >= SPI_NUM)
	{
		return VSFERR_NOT_SUPPORT;
	}
#endif

	nuc505_spi_param[spi_idx].onready = onready;
	nuc505_spi_param[spi_idx].param = p;

	return VSFERR_NONE;
}

vsf_err_t nuc505_spi_select(uint8_t index, uint8_t cs)
{
	return VSFERR_NONE;
}
vsf_err_t nuc505_spi_deselect(uint8_t index, uint8_t cs)
{
	return VSFERR_NONE;
}


static uint8_t nuc505_spi_dummy;
vsf_err_t nuc505_spi_start(uint8_t index, uint8_t *out, uint8_t *in,
							uint32_t len)
{
	uint8_t spi_idx = index & 0x0F;

#if __VSF_DEBUG__
	if ((spi_idx >= SPI_NUM) || (len > 0xFFFF))
	{
		return VSFERR_NOT_SUPPORT;
	}
#endif

	if (nuc505_spi_param[spi_idx].start)
		return VSFERR_NOT_READY;

	nuc505_spi_param[spi_idx].start = 1;
	nuc505_spi_param[spi_idx].out = (out == NULL) ? &nuc505_spi_dummy : out;
	nuc505_spi_param[spi_idx].in = in;
	nuc505_spi_param[spi_idx].prt = 0;
	nuc505_spi_param[spi_idx].len = len;

	SPI_T *spi = spi_idx ? SPI1 : SPI0;

	if (len <= 8)
	{
		nuc505_spi_param[spi_idx].in_num = len;
		spi->FIFOCTL = (0x0ul << SPI_FIFOCTL_TXTH_Pos) |
						//(len << SPI_FIFOCTL_RXTH_Pos) |
						SPI_FIFOCTL_TXFBCLR_Msk | SPI_FIFOCTL_RXFBCLR_Msk |
						//(in == NULL ? 0 : SPI_FIFOCTL_RXTOIEN_Msk | SPI_FIFOCTL_RXTHIEN_Msk) |
						SPI_FIFOCTL_TXTHIEN_Msk |
						SPI_FIFOCTL_TXRST_Msk | SPI_FIFOCTL_RXRST_Msk;
		while (nuc505_spi_param[spi_idx].prt < len)
			spi->TX = out[nuc505_spi_param[spi_idx].prt++];
	}
	else
	{
		nuc505_spi_param[spi_idx].in_num = 4;
		spi->FIFOCTL = (0x4ul << SPI_FIFOCTL_TXTH_Pos) |
						//(0x4ul << SPI_FIFOCTL_RXTH_Pos) |
						SPI_FIFOCTL_TXFBCLR_Msk | SPI_FIFOCTL_RXFBCLR_Msk |
						//(in == NULL ? 0 : SPI_FIFOCTL_RXTOIEN_Msk | SPI_FIFOCTL_RXTHIEN_Msk) |
						SPI_FIFOCTL_TXTHIEN_Msk |
						SPI_FIFOCTL_TXRST_Msk | SPI_FIFOCTL_RXRST_Msk;
		while (nuc505_spi_param[spi_idx].prt < 8)
			spi->TX = out[nuc505_spi_param[spi_idx].prt++];
	}

	return VSFERR_NONE;
}

uint32_t nuc505_spi_stop(uint8_t index)
{
	uint8_t spi_idx = index & 0x0F;

#if __VSF_DEBUG__
	if ((spi_idx >= SPI_NUM) || (len > 0xFFFF))
	{
		return 0;
	}
#endif

	SPI_T *spi = spi_idx ? SPI1 : SPI0;
	spi->FIFOCTL = SPI_FIFOCTL_TXRST_Msk | SPI_FIFOCTL_RXRST_Msk;
	nuc505_spi_param[spi_idx].start = 0;

	return nuc505_spi_param[spi_idx].len - nuc505_spi_param[spi_idx].prt;
}

#if SPI0_ENABLE
ROOTFUNC void SPI0_IRQHandler(void)
{
	while (nuc505_spi_param[0].in_num > 0)
	{
		if (nuc505_spi_param[0].in)
		{
			((uint8_t *)(nuc505_spi_param[0].in))[nuc505_spi_param[0].prt - nuc505_spi_param[0].in_num] = SPI0->RX;
			nuc505_spi_param[0].in_num--;
		}
		else
		{
			volatile uint32_t dummy = SPI0->RX;
		}
	}

	if (nuc505_spi_param[0].len > nuc505_spi_param[0].prt)
	{
		while ((nuc505_spi_param[0].prt < nuc505_spi_param[0].len) &&(nuc505_spi_param[0].in_num++ < 4))
			SPI0->TX = ((uint8_t *)(nuc505_spi_param[0].out))[nuc505_spi_param[0].prt++];
	}
	else
	{
		SPI0->FIFOCTL &= ~SPI_FIFOCTL_TXTHIEN_Msk;
		if (nuc505_spi_param[0].onready)
			nuc505_spi_param[0].onready(nuc505_spi_param[0].param);
	}
}
#endif

#if SPI1_ENABLE
ROOTFUNC void SPI1_IRQHandler(void)
{
	while (nuc505_spi_param[1].in_num > 0)
	{
		if (nuc505_spi_param[1].in)
		{
			((uint8_t *)(nuc505_spi_param[1].in))[nuc505_spi_param[1].prt - nuc505_spi_param[1].in_num] = SPI1->RX;
			nuc505_spi_param[1].in_num--;
		}
		else
		{
			volatile uint32_t dummy = SPI1->RX;
		}
	}

	if (nuc505_spi_param[1].len > nuc505_spi_param[1].prt)
	{
		while ((nuc505_spi_param[1].prt < nuc505_spi_param[1].len) &&(nuc505_spi_param[1].in_num++ < 4))
			SPI1->TX = ((uint8_t *)(nuc505_spi_param[1].out))[nuc505_spi_param[1].prt++];
	}
	else
	{
		SPI1->FIFOCTL &= ~SPI_FIFOCTL_TXTHIEN_Msk;
		if (nuc505_spi_param[1].onready)
			nuc505_spi_param[1].onready(nuc505_spi_param[1].param);
	}
}
#endif

#endif

