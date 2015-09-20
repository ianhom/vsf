#include "app_type.h"

#include "framework/vsfsm/vsfsm.h"
#include "framework/vsftimer/vsftimer.h"

#include "../../../vsfip_netif.h"
#include "tool/buffer/buffer.h"

#ifndef __BCM_BUS_H_INCLUDED__
#define __BCM_BUS_H_INCLUDED__

#define BCM_CMD_READ			(uint8_t)0
#define BCM_CMD_WRITE			(uint8_t)1

#define BCM_ACCESS_FIX			(uint8_t)0
#define BCM_ACCESS_INCREMENTAL	(uint8_t)1

#define BCM_FUNC_BUS			(uint8_t)0
#define BCM_FUNC_BACKPLANE		(uint8_t)1
#define BCM_FUNC_F2				(uint8_t)2

struct bcm_bus_t;
struct bcm_bus_op_t
{
	vsf_err_t (*init)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt);
	uint32_t (*fix_u32)(struct bcm_bus_t *bcm_bus, uint32_t value);
	vsf_err_t (*transact)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
						uint8_t rw, uint8_t func, uint32_t addr, uint16_t size,
						struct vsf_buffer_t *buffer);
};

struct bcm_bus_t
{
	enum
	{
		BCM_BUS_TYPE_SPI,
		BCM_BUS_TYPE_SDIO,
	} type;
	struct bcm_bus_op_t const *op;
	struct
	{
		union
		{
			struct
			{
				uint8_t port;
				uint8_t cs_port;
				uint8_t cs_pin;
				uint32_t freq_khz;
			} spi;
			struct
			{
				uint8_t port;
				uint32_t freq_khz;
			} sdio;
		} comm;
		uint8_t rst_port;
		uint8_t rst_pin;
		uint8_t eint_port;
		uint8_t eint_pin;
		uint8_t eint;
	} port;
	struct vsfip_netif_t *sta_netif;
	struct vsfip_netif_t *ap_netif;
	
	// private
	uint32_t retry;
	bool is_up;
	uint32_t f1sig;
	uint16_t intf;
	uint32_t status;
	enum
	{
		BCM_BUS_ENDIAN_LE = 0,
		BCM_BUS_ENDIAN_BE = 1,
	} endian;
	enum
	{
		BCM_BUS_WORDLEN_16 = 0,
		BCM_BUS_WORDLEN_32 = 1,
	} word_length;
	uint32_t cur_backplane_base;
	
	// port_init_pt is used in init procedure in specificed port driver
	struct vsfsm_pt_t port_init_pt;
	// download_firmware_pt is used in bcm_bus_download_firmware
	struct vsfsm_pt_t download_firmware_pt;
	struct vsfsm_pt_t download_image_pt;
	struct vsf_buffer_t download_image_buffer;
	uint8_t download_image_mem[64];
	uint32_t download_progress;
	// core_ctrl_pt is used in bcm_bus_disable_device_core,
	// 	bcm_bus_reset_device_core and bcm_bus_device_core_isup
	struct vsfsm_pt_t core_ctrl_pt;
	// init_pt is used in bcm_bus_init
	struct vsfsm_crit_t crit_init;
	struct vsfsm_pt_t init_pt;
	// regacc is used in bcm_bus_read_reg and bcm_bus_write_reg
	struct vsfsm_crit_t crit_reg;
	struct vsfsm_pt_t regacc_pt;
	struct vsf_buffer_t regacc_buffer;
	uint8_t regacc_mem[8];
	// bufacc is used in bcm_bus_transact
	struct vsfsm_crit_t crit_buffer;
	struct vsfsm_pt_t bufacc_pt;
	struct vsf_buffer_t bufacc_buffer;
	uint32_t f2rdy_retrycnt;
	uint8_t bufacc_mem[8];
	// for bus drivers
	struct vsfsm_t *bus_ops_sm;
	uint32_t bushead;
	uint32_t buslen;
	uint32_t tmpreg;
};

extern const struct bcm_bus_op_t bcm_bus_spi_op;

vsf_err_t bcm_bus_construct(struct bcm_bus_t *bus);
vsf_err_t bcm_bus_init(struct vsfsm_pt_t *pt, vsfsm_evt_t evt);
vsf_err_t bcm_bus_fini(struct vsfsm_pt_t *pt, vsfsm_evt_t evt);
vsf_err_t bcm_bus_transact(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
			uint8_t rw, uint8_t func, uint32_t addr, uint16_t size,
			struct vsf_buffer_t *buffer);
vsf_err_t bcm_bus_read_reg(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
							uint32_t addr, uint8_t len, uint8_t *value);
vsf_err_t bcm_bus_write_reg(struct vsfsm_pt_t *pt, vsfsm_evt_t evt,
							uint32_t addr, uint8_t len, uint32_t value);

#endif		// __BCM_BUS_H_INCLUDED__
