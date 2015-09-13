#include "app_cfg.h"
#include "app_type.h"

#include "interfaces.h"
#include "tool\list\list.h"
#include "tool\buffer\buffer.h"

#include "vsfusbh.h"
#include "vsfdwcotg_priv.h"


static vsf_err_t rh_submit_urb(struct dwcotg_hcd_t *hcd,
		struct vsfusbh_urb_t *vsfurb)
{
	uint16_t bmRType_bReq, wValue, wIndex, wLength;
	uint32_t datab[4], len, status = 0;
	uint8_t *data_buf = (uint8_t *)datab;
	void *data = vsfurb->transfer_buffer;
	uint32_t leni = vsfurb->transfer_length;
	uint32_t pipe = vsfurb->pipe;
	volatile uint32_t *hprt0 = hcd->hprt0;

	if (usb_pipeint(pipe))
	{
		// TODO: if need int transfer
		return VSFERR_NOT_SUPPORT;
	}

	bmRType_bReq = cmd->requesttype | (cmd->request << 8);
	wValue = cmd->value;
	wIndex = cmd->index;
	wLength = cmd->length;

	switch (bmRType_bReq)
	{
	case RH_GET_STATUS:
		*(uint16_t *)data_buf = 0x100;
		RH_OK(2);
	case RH_GET_STATUS | USB_RECIP_INTERFACE:
		*(uint16_t *)data_buf = 0;
		RH_OK(2);
	case RH_GET_STATUS | USB_RECIP_ENDPOINT:
		*(uint16_t *)data_buf = 0;
		RH_OK(2);
	case RH_GET_STATUS | USB_TYPE_CLASS:
		data_buf[0] = RD_RH_STAT & 0xFF;
		data_buf[1] = (RD_RH_STAT >> 8) & 0xFF;
		data_buf[2] = (RD_RH_STAT >> 16) & 0xFF;
		data_buf[3] = (RD_RH_STAT >> 24) & 0xFF;
		*(uint32_t *)data_buf = RD_RH_STAT & ~(RH_HS_CRWE | RH_HS_DRWE);
		RH_OK(4);
	case RH_GET_STATUS | USB_RECIP_OTHER | USB_TYPE_CLASS:
		*(uint32_t *)data_buf = RD_RH_PORTSTAT;
		RH_OK(4);
	case RH_CLEAR_FEATURE | USB_RECIP_ENDPOINT:
		switch (wValue)
		{
		case(RH_ENDPOINT_STALL):
			RH_OK(0);
		}
		break;
	case RH_CLEAR_FEATURE | USB_TYPE_CLASS:
		switch (wValue)
		{
		case RH_C_HUB_LOCAL_POWER:
			RH_OK(0);
		case(RH_C_HUB_OVER_CURRENT):
			WR_RH_STAT(RH_HS_OCIC);
			RH_OK(0);
		}
		break;
	case RH_CLEAR_FEATURE | USB_RECIP_OTHER | USB_TYPE_CLASS:
		switch (wValue)
		{
		case(RH_PORT_ENABLE):
			WR_RH_PORTSTAT(RH_PS_CCS);
			RH_OK(0);
		case(RH_PORT_SUSPEND):
			WR_RH_PORTSTAT(RH_PS_POCI);
			RH_OK(0);
		case(RH_PORT_POWER):
			WR_RH_PORTSTAT(RH_PS_LSDA);
			RH_OK(0);
		case(RH_C_PORT_CONNECTION):
			WR_RH_PORTSTAT(RH_PS_CSC);
			RH_OK(0);
		case(RH_C_PORT_ENABLE):
			WR_RH_PORTSTAT(RH_PS_PESC);
			RH_OK(0);
		case(RH_C_PORT_SUSPEND):
			WR_RH_PORTSTAT(RH_PS_PSSC);
			RH_OK(0);
		case(RH_C_PORT_OVER_CURRENT):
			WR_RH_PORTSTAT(RH_PS_OCIC);
			RH_OK(0);
		case(RH_C_PORT_RESET):
			WR_RH_PORTSTAT(RH_PS_PRSC);
			RH_OK(0);
		}
		break;
	case RH_SET_FEATURE | USB_RECIP_OTHER | USB_TYPE_CLASS:
		switch (wValue)
		{
		case(RH_PORT_SUSPEND):
			WR_RH_PORTSTAT(RH_PS_PSS);
			RH_OK(0);
		case(RH_PORT_RESET):	/* BUG IN HUP CODE *********/
			if (RD_RH_PORTSTAT & RH_PS_CCS)
				WR_RH_PORTSTAT(RH_PS_PRS);
			RH_OK(0);
		case(RH_PORT_POWER):
			WR_RH_PORTSTAT(RH_PS_PPS);
			RH_OK(0);
		case(RH_PORT_ENABLE):	/* BUG IN HUP CODE *********/
			if (RD_RH_PORTSTAT & RH_PS_CCS)
				WR_RH_PORTSTAT(RH_PS_PES);
			RH_OK(0);
		}
		break;
	case RH_SET_ADDRESS:
		vsfohci->ohci->rh.devnum = wValue;
		RH_OK(0);
	case RH_GET_DESCRIPTOR:
		switch ((wValue & 0xff00) >> 8)
		{
		case(0x01):		/* device descriptor */
			len = min(leni, min(sizeof(root_hub_dev_des), wLength));
			data_buf = (uint8_t *)root_hub_dev_des;
			RH_OK(len);
		case(0x02):		/* configuration descriptor */
			len = min(leni, min(sizeof(root_hub_config_des), wLength));
			data_buf = (uint8_t *)root_hub_config_des;
			RH_OK(len);
		case(0x03):		/* string descriptors */
			if (wValue == 0x0300)
			{
				len = min(leni, min(sizeof(root_hub_str_index0), wLength));
				data_buf = (uint8_t *)root_hub_str_index0;
				RH_OK(len);
			}
			if (wValue == 0x0301)
			{
				len = min(leni, min(sizeof(root_hub_str_index1), wLength));
				data_buf = (uint8_t *)root_hub_str_index1;
				RH_OK(len);
			}
		default:
			status = TD_CC_STALL;
		}
		break;
	case RH_GET_DESCRIPTOR | USB_TYPE_CLASS:
	{
		uint32_t temp = regs->roothub.a;

		data_buf[0] = 9;			// min length;
		data_buf[1] = 0x29;
		data_buf[2] = temp & RH_A_NDP;
		data_buf[3] = 0;
		if (temp & RH_A_PSM)		/* per-port power switching? */
			data_buf[3] |= 0x1;
		if (temp & RH_A_NOCP)		/* no over current reporting? */
			data_buf[3] |= 0x10;
		else if (temp & RH_A_OCPM)	/* per-port over current reporting? */
			data_buf[3] |= 0x8;

		datab[1] = 0;
		data_buf[5] = (temp & RH_A_POTPGT) >> 24;
		temp = regs->roothub.b;
		data_buf[7] = temp & RH_B_DR;
		if (data_buf[2] < 7)
		{
			data_buf[8] = 0xff;
		}
		else
		{
			data_buf[0] += 2;
			data_buf[8] = (temp & RH_B_DR) >> 8;
			data_buf[10] = data_buf[9] = 0xff;
		}
		len = min(leni, min(data_buf[0], wLength));
		RH_OK(len);
	}
	case RH_GET_CONFIGURATION:
		*(uint8_t *)data_buf = 0x01;
		RH_OK(1);
	case RH_SET_CONFIGURATION:
		WR_RH_STAT(0x10000);	/* set global port power */
		RH_OK(0);
	default:
		status = TD_CC_STALL;
	}
	len = min(len, leni);
	if (len != 0)
		memcpy(data, data_buf, len);
	vsfurb->actual_length = len;
	vsfurb->status = cc_to_error[status];

	vsfsm_post_evt_pending(vsfurb->sm, VSFSM_EVT_URB_COMPLETE);
	return 0;
}

static vsf_err_t rh_unlink_urb(struct dwcotg_hcd_t *vsfohci,
		struct vsfusbh_urb_t *vsfurb)
{
	// TODO: if need int transfer
	return VSFERR_NONE;
}










static vsf_err_t dwcotg_init_get_resource(struct vsfusbh_t *usbh,
	uint32_t reg_base)
{
	struct dwcotg_hcd_t *dwcotg_hcd;

	dwcotg_hcd = vsf_bufmgr_malloc(sizeof(struct dwcotg_hcd_t));
	if (dwcotg_hcd == NULL)
		return VSFERR_NOT_ENOUGH_RESOURCES;
	memset(dwcotg_hcd, 0, sizeof(struct dwcotg_hcd_t));
	usbh->hcd_data = dwcotg_hcd;

	/*
	dwcotg_hcd->core_if = vsf_bufmgr_malloc(sizeof(struct dwcotg_core_if_t));
	if (dwcotg_hcd->core_if == NULL)
		return VSFERR_NOT_ENOUGH_RESOURCES;
	memset(dwcotg_hcd->core_if, 0, sizeof(struct dwcotg_core_if_t));
	*/

	dwcotg_hcd->global_reg =
			(struct dwcotg_core_global_regs_t *)(reg_base + 0);
	dwcotg_hcd->dev_global_regs =
			(struct dwcotg_dev_global_regs_t *)(reg_base + 0x800);
	dwcotg_hcd->in_ep_regs =
			(struct dwcotg_dev_in_ep_regs_t *)(reg_base + 0x900);
	dwcotg_hcd->out_ep_regs =
			(struct dwcotg_dev_out_ep_regs_t *)(reg_base + 0xb00);
	dwcotg_hcd->host_global_regs =
			(struct dwcotg_host_global_regs_t *)(reg_base + 0x400);
	dwcotg_hcd->hprt0 =
			(volatile uint32_t *)(reg_base + 0x440);
	dwcotg_hcd->hc_regs =
			(struct dwcotg_hc_regs_t *)(reg_base + 0x500);

	return VSFERR_NONE;
}

static vsf_err_t dwcotgh_init_thread(struct vsfsm_pt_t *pt, vsfsm_evt_t evt)
{
	vsf_err_t err;
	struct vsfusbh_t *usbh = (struct vsfusbh_t *)pt->user_data;
	struct dwcotg_hcd_t *dwcotg_hcd = (struct dwcotg_hcd_t *)usbh->hcd_data;

	vsfsm_pt_begin(pt);

	err = dwcotg_init_get_resource(usbh,
			(uint32_t)core_interfaces.dwcotg.regbase(0));
	if (err)
		return err;
	dwcotg_hcd = (struct vsfohci_t *)usbh->hcd_data;

	core_interfaces.dwcotg.init(0, vsfdwcotg_interrupt, usbh->hcd_data);

	// disable global int
	dwcotg_hcd->global_reg->gahbcfg &= ~USB_OTG_GAHBCFG_GINT;

	// enable hs core
#if (__TARGET_CHIP__ == STM32F7)
	// GCCFG &= ~(USB_OTG_GCCFG_PWRDWN)
	dwcotg_hcd->global_reg->ggpio &= ~USB_OTG_GCCFG_PWRDWN;
#endif
	dwcotg_hcd->global_reg->gusbcfg &= ~(USB_OTG_GUSBCFG_TSDPS |
			USB_OTG_GUSBCFG_ULPIFSLS | USB_OTG_GUSBCFG_PHYSEL |
			USB_OTG_GUSBCFG_ULPIEVBUSD | USB_OTG_GUSBCFG_ULPIEVBUSI);
	dwcotg_hcd->global_reg->gusbcfg |= USB_OTG_GUSBCFG_ULPIEVBUSD;

	// Core Reset
	dwcotg_hcd->retry = 0;
	while ((dwcotg_hcd->global_reg->grstctl & USB_OTG_GRSTCTL_AHBIDL) == 0)
	{
		if (dwcotg_hcd->retry > 10)
			return VSFERR_FAIL;

		vsfsm_pt_delay(1);
		dwcotg_hcd->retry++;
	}
	dwcotg_hcd->global_reg->grstctl |= USB_OTG_GRSTCTL_CSRST;
	dwcotg_hcd->retry = 0;
	while ((dwcotg_hcd->global_reg->grstctl & USB_OTG_GRSTCTL_CSRST) ==
			USB_OTG_GRSTCTL_CSRST)
	{
		if (dwcotg_hcd->retry > 10)
			return VSFERR_FAIL;

		vsfsm_pt_delay(1);
		dwcotg_hcd->retry++;
	}

	// Enable DMA
	dwcotg_hcd->global_reg->gahbcfg |= USB_OTG_GAHBCFG_DMAEN |
			USB_OTG_GAHBCFG_HBSTLEN_1 | USB_OTG_GAHBCFG_HBSTLEN_2;

	// Force Host Mode
	dwcotg_hcd->global_reg->gusbcfg &= ~USB_OTG_GUSBCFG_FDMOD;
	dwcotg_hcd->global_reg->gusbcfg |= USB_OTG_GUSBCFG_FHMOD;

	vsfsm_pt_delay(50);

	// Start
	dwcotg_hcd->global_reg->gahbcfg |= USB_OTG_GAHBCFG_GINT;
	dwcotg_hcd->hprt0 = (dwcotg_hcd->hprt0 | USB_OTG_HPRT_PPWR) &
			(~(USB_OTG_HPRT_PENA | USB_OTG_HPRT_PCDET |
			USB_OTG_HPRT_PENCHNG | USB_OTG_HPRT_POCCHNG)) ;

	vsfsm_pt_delay(200);

	vsfsm_pt_end(pt);

	return VSFERR_NONE;
}

const struct vsfusbh_hcddrv_t vsfdwcotgh_drv =
{
	dwcotgh_init_thread,
	dwcotgh_fini,
	dwcotgh_suspend,
	dwcotgh_resume,
	dwcotgh_alloc_device,
	dwcotgh_free_device,
	dwcotgh_submit_urb,
	dwcotgh_unlink_urb,
	dwcotgh_relink_urb,
};
