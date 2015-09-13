vsf_err_t nuc505_hcd_init(uint8_t index, vsf_err_t (*hcd_irq)(void *), void *param);
vsf_err_t nuc505_hcd_fini(uint8_t index);
void* nuc505_hcd_regbase(uint8_t index);
