#include "vsf.h"
#include "app_hw_cfg.h"

// dummy main, make compiler happy
int main(void)
{
	return 0;
}

ROOTFUNC vsf_err_t module_exit(struct vsf_module_t *module)
{
	struct vsf_shell_api_t *api = vsf.framework.shell;
	memset(api, 0, sizeof(*api));
	module->ifs = NULL;
	return VSFERR_NONE;
}

ROOTFUNC vsf_err_t __iar_program_start(struct vsf_module_t *module,
							struct app_hwcfg_t const *hwcfg)
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
