#include "dwcotg_regs.h"

/* USB HUB CONSTANTS (not OHCI-specific; see hub.h) */
/* Requests: bRequest << 8 | bmRequestType */
#define RH_GET_STATUS			0x0080
#define RH_CLEAR_FEATURE		0x0100
#define RH_SET_FEATURE			0x0300
#define RH_SET_ADDRESS			0x0500
#define RH_GET_DESCRIPTOR		0x0680
#define RH_SET_DESCRIPTOR		0x0700
#define RH_GET_CONFIGURATION	0x0880
#define RH_SET_CONFIGURATION	0x0900
#define RH_GET_STATE			0x0280
#define RH_GET_INTERFACE		0x0A80
#define RH_SET_INTERFACE		0x0B00
#define RH_SYNC_FRAME			0x0C80
/* Our Vendor Specific Request */
#define RH_SET_EP		0x2000


/* Hub port features */
#define RH_PORT_CONNECTION		0x00
#define RH_PORT_ENABLE			0x01
#define RH_PORT_SUSPEND			0x02
#define RH_PORT_OVER_CURRENT	0x03
#define RH_PORT_RESET			0x04
#define RH_PORT_POWER			0x08
#define RH_PORT_LOW_SPEED		0x09

#define RH_C_PORT_CONNECTION	0x10
#define RH_C_PORT_ENABLE		0x11
#define RH_C_PORT_SUSPEND		0x12
#define RH_C_PORT_OVER_CURRENT	0x13
#define RH_C_PORT_RESET			0x14

/* Hub features */
#define RH_C_HUB_LOCAL_POWER	0x00
#define RH_C_HUB_OVER_CURRENT	0x01

#define RH_DEVICE_REMOTE_WAKEUP	0x00
#define RH_ENDPOINT_STALL		0x01

#define RH_ACK					0x01
#define RH_REQ_ERR				-1
#define RH_NACK					0x00

#define RH_OK(x)				len = (x); break
#define WR_RH_STAT(x)			(regs->roothub.status = (x))
#define WR_RH_PORTSTAT(x)		(regs->roothub.portstatus[wIndex-1] = (x))
#define RD_RH_STAT				(regs->roothub.status)
#define RD_RH_PORTSTAT			(regs->roothub.portstatus[wIndex-1])

/* OHCI ROOT HUB REGISTER MASKS */

/* roothub.portstatus [i] bits */
#define RH_PS_CCS				0x00000001		/* current connect status */
#define RH_PS_PES				0x00000002		/* port enable status*/
#define RH_PS_PSS				0x00000004		/* port suspend status */
#define RH_PS_POCI				0x00000008		/* port over current indicator */
#define RH_PS_PRS				0x00000010		/* port reset status */
#define RH_PS_PPS				0x00000100		/* port power status */
#define RH_PS_LSDA				0x00000200		/* low speed device attached */
#define RH_PS_CSC				0x00010000		/* connect status change */
#define RH_PS_PESC				0x00020000		/* port enable status change */
#define RH_PS_PSSC				0x00040000		/* port suspend status change */
#define RH_PS_OCIC				0x00080000		/* over current indicator change */
#define RH_PS_PRSC				0x00100000		/* port reset status change */

/* roothub.status bits */
#define RH_HS_LPS				0x00000001		/* local power status */
#define RH_HS_OCI				0x00000002		/* over current indicator */
#define RH_HS_DRWE				0x00008000		/* device remote wakeup enable */
#define RH_HS_LPSC				0x00010000		/* local power status change */
#define RH_HS_OCIC				0x00020000		/* over current indicator change */
#define RH_HS_CRWE				0x80000000		/* clear remote wakeup enable */

/* roothub.b masks */
#define RH_B_DR					0x0000ffff		/* device removable flags */
#define RH_B_PPCM				0xffff0000		/* port power control mask */

/* roothub.a masks */
#define RH_A_NDP				(0xfful << 0)		/* number of downstream ports */
#define RH_A_PSM				(0x1ul << 8)		/* power switching mode */
#define RH_A_NPS				(0x1ul << 9)		/* no power switching */
#define RH_A_DT					(0x1ul << 10)		/* device type (mbz) */
#define RH_A_OCPM				(0x1ul << 11)		/* over current protection mode */
#define RH_A_NOCP				(0x1ul << 12)		/* no over current protection */
#define RH_A_POTPGT				(0xfful << 24)		/* power on to power good time */



/**
 * The following parameters may be specified when starting the module. These
 * parameters define how the DWC_otg controller should be configured.
 */
struct dwcotg_core_param_t
{
	int32_t opt;

	/**
	 * Specifies the OTG capabilities. The driver will automatically
	 * detect the value for this parameter if none is specified.
	 * 0 - HNP and SRP capable (default)
	 * 1 - SRP Only capable
	 * 2 - No HNP/SRP capable
	 */
	int32_t otg_cap;

	/**
	 * Specifies whether to use slave or DMA mode for accessing the data
	 * FIFOs. The driver will automatically detect the value for this
	 * parameter if none is specified.
	 * 0 - Slave
	 * 1 - DMA (default, if available)
	 */
	int32_t dma_enable;

	/**
	 * When DMA mode is enabled specifies whether to use address DMA or DMA
	 * Descriptor mode for accessing the data FIFOs in device mode. The driver
	 * will automatically detect the value for this if none is specified.
	 * 0 - address DMA
	 * 1 - DMA Descriptor(default, if available)
	 */
	int32_t dma_desc_enable;
	/** The DMA Burst size (applicable only for External DMA
	 * Mode). 1, 4, 8 16, 32, 64, 128, 256 (default 32)
	 */
	int32_t dma_burst_size;	/* Translate this to GAHBCFG values */

	/**
	 * Specifies the maximum speed of operation in host and device mode.
	 * The actual speed depends on the speed of the attached device and
	 * the value of phy_type. The actual speed depends on the speed of the
	 * attached device.
	 * 0 - High Speed (default)
	 * 1 - Full Speed
	 */
	int32_t speed;
	/** Specifies whether low power mode is supported when attached
	 *	to a Full Speed or Low Speed device in host mode.
	 * 0 - Don't support low power mode (default)
	 * 1 - Support low power mode
	 */
	int32_t host_support_fs_ls_low_power;

	/** Specifies the PHY clock rate in low power mode when connected to a
	 * Low Speed device in host mode. This parameter is applicable only if
	 * HOST_SUPPORT_FS_LS_LOW_POWER is enabled. If PHY_TYPE is set to FS
	 * then defaults to 6 MHZ otherwise 48 MHZ.
	 *
	 * 0 - 48 MHz
	 * 1 - 6 MHz
	 */
	int32_t host_ls_low_power_phy_clk;

	/**
	 * 0 - Use cC FIFO size parameters
	 * 1 - Allow dynamic FIFO sizing (default)
	 */
	int32_t enable_dynamic_fifo;

	/** Total number of 4-byte words in the data FIFO memory. This
	 * memory includes the Rx FIFO, non-periodic Tx FIFO, and periodic
	 * Tx FIFOs.
	 * 32 to 32768 (default 8192)
	 * Note: The total FIFO memory depth in the FPGA configuration is 8192.
	 */
	int32_t data_fifo_size;

	/** Number of 4-byte words in the Rx FIFO in device mode when dynamic
	 * FIFO sizing is enabled.
	 * 16 to 32768 (default 1064)
	 */
	int32_t dev_rx_fifo_size;

	/** Number of 4-byte words in the non-periodic Tx FIFO in device mode
	 * when dynamic FIFO sizing is enabled.
	 * 16 to 32768 (default 1024)
	 */
	int32_t dev_nperio_tx_fifo_size;

	/** Number of 4-byte words in each of the periodic Tx FIFOs in device
	 * mode when dynamic FIFO sizing is enabled.
	 * 4 to 768 (default 256)
	 */
	uint32_t dev_perio_tx_fifo_size[MAX_PERIO_FIFOS];

	/** Number of 4-byte words in the Rx FIFO in host mode when dynamic
	 * FIFO sizing is enabled.
	 * 16 to 32768 (default 1024)
	 */
	int32_t host_rx_fifo_size;

	/** Number of 4-byte words in the non-periodic Tx FIFO in host mode
	 * when Dynamic FIFO sizing is enabled in the core.
	 * 16 to 32768 (default 1024)
	 */
	int32_t host_nperio_tx_fifo_size;

	/** Number of 4-byte words in the host periodic Tx FIFO when dynamic
	 * FIFO sizing is enabled.
	 * 16 to 32768 (default 1024)
	 */
	int32_t host_perio_tx_fifo_size;

	/** The maximum transfer size supported in bytes.
	 * 2047 to 65,535  (default 65,535)
	 */
	int32_t max_transfer_size;

	/** The maximum number of packets in a transfer.
	 * 15 to 511  (default 511)
	 */
	int32_t max_packet_count;

	/** The number of host channel registers to use.
	 * 1 to 16 (default 12)
	 * Note: The FPGA configuration supports a maximum of 12 host channels.
	 */
	int32_t host_channels;

	/** The number of endpoints in addition to EP0 available for device
	 * mode operations.
	 * 1 to 15 (default 6 IN and OUT)
	 * Note: The FPGA configuration supports a maximum of 6 IN and OUT
	 * endpoints in addition to EP0.
	 */
	int32_t dev_endpoints;

		/**
		 * Specifies the type of PHY interface to use. By default, the driver
		 * will automatically detect the phy_type.
		 *
		 * 0 - Full Speed PHY
		 * 1 - UTMI+ (default)
		 * 2 - ULPI
		 */
	int32_t phy_type;

	/**
	 * Specifies the UTMI+ Data Width. This parameter is
	 * applicable for a PHY_TYPE of UTMI+ or ULPI. (For a ULPI
	 * PHY_TYPE, this parameter indicates the data width between
	 * the MAC and the ULPI Wrapper.) Also, this parameter is
	 * applicable only if the OTG_HSPHY_WIDTH cC parameter was set
	 * to "8 and 16 bits", meaning that the core has been
	 * configured to work at either data path width.
	 *
	 * 8 or 16 bits (default 16)
	 */
	int32_t phy_utmi_width;

	/**
	 * Specifies whether the ULPI operates at double or single
	 * data rate. This parameter is only applicable if PHY_TYPE is
	 * ULPI.
	 *
	 * 0 - single data rate ULPI interface with 8 bit wide data
	 * bus (default)
	 * 1 - double data rate ULPI interface with 4 bit wide data
	 * bus
	 */
	int32_t phy_ulpi_ddr;

	/**
	 * Specifies whether to use the internal or external supply to
	 * drive the vbus with a ULPI phy.
	 */
	int32_t phy_ulpi_ext_vbus;

	/**
	 * Specifies whether to use the I2Cinterface for full speed PHY. This
	 * parameter is only applicable if PHY_TYPE is FS.
	 * 0 - No (default)
	 * 1 - Yes
	 */
	int32_t i2c_enable;

	int32_t ulpi_fs_ls;

	int32_t ts_dline;

	/**
	 * Specifies whether dedicated transmit FIFOs are
	 * enabled for non periodic IN endpoints in device mode
	 * 0 - No
	 * 1 - Yes
	 */
	int32_t en_multiple_tx_fifo;

	/** Number of 4-byte words in each of the Tx FIFOs in device
	 * mode when dynamic FIFO sizing is enabled.
	 * 4 to 768 (default 256)
	 */
	uint32_t dev_tx_fifo_size[MAX_TX_FIFOS];

	/** Thresholding enable flag-
	 * bit 0 - enable non-ISO Tx thresholding
	 * bit 1 - enable ISO Tx thresholding
	 * bit 2 - enable Rx thresholding
	 */
	uint32_t thr_ctl;

	/** Thresholding length for Tx
	 *	FIFOs in 32 bit DWORDs
	 */
	uint32_t tx_thr_length;

	/** Thresholding length for Rx
	 *	FIFOs in 32 bit DWORDs
	 */
	uint32_t rx_thr_length;

	/**
	 * Specifies whether LPM (Link Power Management) support is enabled
	 */
	int32_t lpm_enable;

	/** Per Transfer Interrupt
	 *	mode enable flag
	 * 1 - Enabled
	 * 0 - Disabled
	 */
	int32_t pti_enable;

	/** Multi Processor Interrupt
	 *	mode enable flag
	 * 1 - Enabled
	 * 0 - Disabled
	 */
	int32_t mpi_enable;

	/** IS_USB Capability
	 * 1 - Enabled
	 * 0 - Disabled
	 */
	int32_t ic_usb_cap;

	/** AHB Threshold Ratio
	 * 2'b00 AHB Threshold = 	MAC Threshold
	 * 2'b01 AHB Threshold = 1/2 	MAC Threshold
	 * 2'b10 AHB Threshold = 1/4	MAC Threshold
	 * 2'b11 AHB Threshold = 1/8	MAC Threshold
	 */
	int32_t ahb_thr_ratio;

	/** ADP Support
	 * 1 - Enabled
	 * 0 - Disabled
	 */
	int32_t adp_supp_enable;

	/** HFIR Reload Control
	 * 0 - The HFIR cannot be reloaded dynamically.
	 * 1 - Allow dynamic reloading of the HFIR register during runtime.
	 */
	int32_t reload_ctl;

	/** DCFG: Enable device Out NAK
	 * 0 - The core does not set NAK after Bulk Out transfer complete.
	 * 1 - The core sets NAK after Bulk OUT transfer complete.
	 */
	int32_t dev_out_nak;

	/** DCFG: Enable Continue on BNA
	 * After receiving BNA interrupt the core disables the endpoint,when the
	 * endpoint is re-enabled by the application the core starts processing
	 * 0 - from the DOEPDMA descriptor
	 * 1 - from the descriptor which received the BNA.
	 */
	int32_t cont_on_bna;

	/** GAHBCFG: AHB Single Support
	 * This bit when programmed supports SINGLE transfers for remainder
	 * data in a transfer for DMA mode of operation.
	 * 0 - in this case the remainder data will be sent using INCR burst size.
	 * 1 - in this case the remainder data will be sent using SINGLE burst size.
	 */
	int32_t ahb_single;

	/** Core Power down mode
	 * 0 - No Power Down is enabled
	 * 1 - Reserved
	 * 2 - Complete Power Down (Hibernation)
	 */
	int32_t power_down;

	/** OTG revision supported
	 * 0 - OTG 1.3 revision
	 * 1 - OTG 2.0 revision
	 */
	int32_t otg_ver;

};
struct dwcotg_regs_t
{

};

struct dwcotg_core_if_t
{
	/** Parameters that define how the core should be configured.*/
	struct dwcotg_core_param_t core_param;

	// DWC-OTG registers
	struct dwcotg_regs_t regs;

	// DEVICE
	// TODO

	// HOST
	/* Host configuration information */
	/** Number of Host Channels (range: 1-16) */
	uint8_t num_host_channels;
	/** Periodic EPs supported (0: no, 1: yes) */
	uint8_t perio_eps_supported;
	/** Periodic Tx FIFO Size (Only 1 host periodic Tx FIFO) */
	uint16_t perio_tx_fifo_size;

	// IGNORE: SNPSID

	// TODO
};


struct dwcotg_hcd_t
{
	/** DWCOTG REGS */
	// Core Global registers starting at offset 000h
	struct dwcotg_core_global_regs_t *global_reg;

	// Device Global Registers starting at offset 800h
	struct dwcotg_dev_global_regs_t *dev_global_regs;
#define DWC_DEV_GLOBAL_REG_OFFSET 0x800

	// Device Logical IN Endpoint-Specific Registers 900h-AFCh
	struct dwcotg_dev_in_ep_regs_t *in_ep_regs[MAX_EPS_CHANNELS];
#define DWC_DEV_IN_EP_REG_OFFSET 0x900
#define DWC_EP_REG_OFFSET 0x20

	// Device Logical OUT Endpoint-Specific Registers B00h-CFCh
	struct dwcotg_dev_out_ep_regs_t *out_ep_regs[MAX_EPS_CHANNELS];
#define DWC_DEV_OUT_EP_REG_OFFSET 0xB00

	// Host Global Registers starting at offset 400h.
	struct dwcotg_host_global_regs_t *host_global_regs;
#define DWC_OTG_HOST_GLOBAL_REG_OFFSET 0x400

	// Host Port 0 Control and Status Register
	volatile uint32_t *hprt0;
#define DWC_OTG_HOST_PORT_REGS_OFFSET 0x440

	// Host Channel Specific Registers at offsets 500h-5FCh.
	struct dwcotg_hc_regs_t *hc_regs[MAX_EPS_CHANNELS];
#define DWC_OTG_HOST_CHAN_REGS_OFFSET 0x500
#define DWC_OTG_CHAN_REGS_OFFSET 0x20





	struct dwcotg_core_if_t *core_if;






	uint16_t retry;








	/** Internal DWC HCD Flags */
	volatile union dwc_otg_hcd_internal_flags_t
	{
		uint32_t d32;
		struct
		{
			unsigned port_connect_status_change:1;
			unsigned port_connect_status:1;
			unsigned port_reset_change:1;
			unsigned port_enable_change:1;
			unsigned port_suspend_change:1;
			unsigned port_over_current_change:1;
			unsigned port_l1_change:1;
			unsigned reserved:26;
		} b;
	} flags;

	/**
	 * Inactive items in the non-periodic schedule. This is a list of
	 * Queue Heads. Transfers associated with these Queue Heads are not
	 * currently assigned to a host channel.
	 */
	struct sllist non_periodic_inactive;

	/**
	 * Active items in the non-periodic schedule. This is a list of
	 * Queue Heads. Transfers associated with these Queue Heads are
	 * currently assigned to a host channel.
	 */
	struct sllist non_periodic_active;

	/**
	 * Pointer to the next Queue Head to process in the active
	 * non-periodic schedule.
	 */
	struct sllist *non_periodic_ptr;

	/**
	 * Number of host channels assigned to periodic transfers. Currently
	 * assuming that there is a dedicated host channel for each periodic
	 * transaction and at least one host channel available for
	 * non-periodic transactions.
	 */
	int8_t periodic_channels; /* microframe_schedule==0 */

	/**
	 * Number of host channels assigned to non-periodic transfers.
	 */
	int8_t non_periodic_channels; /* microframe_schedule==0 */

	/**
	 * Number of host channels assigned to non-periodic transfers.
	 */
	int8_t available_host_channels;


};
