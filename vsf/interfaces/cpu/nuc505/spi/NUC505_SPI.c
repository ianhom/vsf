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
	uint32_t prtrd;
	uint32_t prtwr;
	uint32_t len;
	uint8_t start;
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
				//SPI_CTL_UNITIEN_Msk|
				SPI_CTL_SPIEN_Msk;
	//close SPI wait transfer start
	
	//config thr int
	spi->FIFOCTL = 		0;

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
	nuc505_spi_param[spi_idx].prtwr = 0;
	nuc505_spi_param[spi_idx].prtrd = 0;
	nuc505_spi_param[spi_idx].len = len;

	SPI_T *spi = spi_idx ? SPI1 : SPI0;

	
	if(len)
	{
		//enable spi to jump into interrupt
		spi->FIFOCTL |= SPI_FIFOCTL_TXTHIEN_Msk;
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

	return nuc505_spi_param[spi_idx].len - nuc505_spi_param[spi_idx].prtwr;
}

#if SPI0_ENABLE
ROOTFUNC void SPI0_IRQHandler(void)
{
	uint32_t transsize;
		
	if(nuc505_spi_param[0].start != 1)
	{
		//err
		return;
	}
	
	transsize = nuc505_spi_param[0].len - nuc505_spi_param[0].prtwr;
	
	if(transsize > 8)
		transsize = 8;
	
	while(!(SPI0->STATUS&SPI_STATUS_RXEMPTY_Msk))
	{
		if (nuc505_spi_param[0].in != NULL )
		{
			((uint8_t *)(nuc505_spi_param[0].in))[nuc505_spi_param[0].prtrd] = SPI0->RX;
			
		}
		else
		{
			volatile uint32_t dummy = SPI0->RX;
		}
		nuc505_spi_param[0].prtrd++;
	}
	
	if(transsize == 0)
	{
		//close FIFO TX
		SPI0->FIFOCTL &= ~SPI_FIFOCTL_TXTHIEN_Msk;
		//open FIFO RX INT
		SPI0->FIFOCTL |= SPI_FIFOCTL_RXTHIEN_Msk;
	}
	
	if(nuc505_spi_param[0].prtrd == nuc505_spi_param[0].len)
	{
		//last trans and  have get the rx dat
		//now close the interface
		nuc505_spi_param[0].start = 0;
		//close FIFO RX
		SPI0->FIFOCTL &= ~SPI_FIFOCTL_RXTHIEN_Msk;
		
		//finish cb
		if (nuc505_spi_param[0].onready != NULL)
		{
			nuc505_spi_param[0].onready(nuc505_spi_param[0].param);
		}
		
	}	
	
	while (transsize > 0)
	{
		if (nuc505_spi_param[0].out != NULL)
		{
			SPI0->TX = ((uint8_t *)(nuc505_spi_param[0].out))[nuc505_spi_param[0].prtwr];
		}
		else
		{
			SPI0->TX = 0xFF;
		}
		nuc505_spi_param[0].prtwr++;
		transsize--;
	}
}
#endif

#if SPI1_ENABLE
ROOTFUNC void SPI1_IRQHandler(void)
{
	uint32_t transsize;
		
	if(nuc505_spi_param[1].start != 1)
	{
		//err
		return;
	}
	
	transsize = nuc505_spi_param[1].len - nuc505_spi_param[1].prtwr;
	
	if(transsize > 8)
		transsize = 8;
	
	while(!(SPI1->STATUS&SPI_STATUS_RXEMPTY_Msk))
	{
		if (nuc505_spi_param[1].in != NULL )
		{
			((uint8_t *)(nuc505_spi_param[1].in))[nuc505_spi_param[1].prtrd] = SPI1->RX;
			
		}
		else
		{
			volatile uint32_t dummy = SPI1->RX;
		}
		nuc505_spi_param[1].prtrd++;
	}
	
	if(transsize == 0)
	{
		//close FIFO TX
		SPI1->FIFOCTL &= ~SPI_FIFOCTL_TXTHIEN_Msk;
		//open FIFO RX INT
		SPI1->FIFOCTL |= SPI_FIFOCTL_RXTHIEN_Msk;
	}
	
	if(nuc505_spi_param[1].prtrd == nuc505_spi_param[1].len)
	{
		//last trans and  have get the rx dat
		//now close the interface
		nuc505_spi_param[1].start = 0;
		//close FIFO RX
		SPI1->FIFOCTL &= ~SPI_FIFOCTL_RXTHIEN_Msk;
		
		//finish cb
		if (nuc505_spi_param[1].onready != NULL)
		{
			nuc505_spi_param[1].onready(nuc505_spi_param[1].param);
		}
		
	}	
	
	while (transsize > 0)
	{
		if (nuc505_spi_param[1].out != NULL)
		{
			SPI1->TX = ((uint8_t *)(nuc505_spi_param[1].out))[nuc505_spi_param[1].prtwr];
		}
		else
		{
			SPI1->TX = 0xFF;
		}
		nuc505_spi_param[1].prtwr++;
		transsize--;
	}
	

}
#endif

#endif

