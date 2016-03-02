#include "vsf.h"
#include "app_hw_cfg.h"

// dummy main, make compiler happy
int main(void)
{
	return 0;
}

ROOTFUNC void module_exit(struct vsf_module_t *module)
{
	MODULE_EXIT(module);
}

ROOTFUNC vsf_err_t __iar_program_start(struct vsf_module_t *module,
							struct app_hwcfg_t const *hwcfg)
{
	// check api version
	if (api_ver != VSF_API_VERSION)
	{
		return VSFERR_NOT_SUPPORT;
	}
	return MODULE_INIT(module, hwcfg);
}
