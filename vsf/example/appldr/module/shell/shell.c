#include "vsf.h"
#include "app_hw_cfg.h"

#ifdef VSF_STANDALONE_MODULE
// dummy main, make compiler happy
int main(void)
{
	return 0;
}

ROOTFUNC vsf_err_t __iar_program_start(struct vsf_module_t *module,
							struct app_hwcfg_t const *hwcfg)
#else
ROOTFUNC vsf_err_t shell_main(struct vsf_module_t *module,
							struct app_hwcfg_t const *hwcfg)
#endif
{
	struct vsf_shell_api_t *api = vsf.framework.shell;

	// check board and api version
	if (strcmp(hwcfg->board, APP_BOARD_NAME) ||
		(api_ver != VSF_API_VERSION))
	{
		return VSFERR_NOT_SUPPORT;
	}

	api->init = vsfshell_init;
	api->register_handlers = vsfshell_register_handlers;
	api->free_handler_thread = vsfshell_free_handler_thread;
	module->ifs = (void *)api;
	return VSFERR_NONE;
}

// for shell module, module_exit is just a place holder
ROOTFUNC vsf_err_t module_exit(struct vsf_module_t *module)
{
	return VSFERR_NONE;
}
