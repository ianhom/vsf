/***************************************************************************
 *   Copyright (C) 2009 - 2010 by Simon Qian <SimonQian@SimonQian.com>     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef __VSFSHELL_H_INCLUDED__
#define __VSFSHELL_H_INCLUDED__

#define VSFSHELL_HANDLER(name, thread, context)\
											{(name), (thread), (context), NULL}
#define VSFSHELL_HANDLER_NONE				VSFSHELL_HANDLER(NULL, NULL, NULL)
struct vsfshell_handler_t
{
	const char * name;
	vsf_err_t (*thread)(struct vsfsm_pt_t *pt, vsfsm_evt_t evt);
	void *context;
	struct vsfshell_handler_t *next;
};

#define VSFSHELL_LINEEND					"\n\r"
#define VSFSHELL_PROMPT						">>>"

struct vsfshell_t
{
	bool echo;
	struct vsf_stream_t *stream_tx;
	struct vsf_stream_t *stream_rx;
	
	struct vsfsm_t sm;
	
	// private
	char cmd_buff[128];
	char printf_buff[256];
	char *printf_pos;
	struct vsf_transaction_buffer_t tbuffer;
	struct vsfsm_t *input_sm;
	struct vsfsm_t *output_sm;
	struct vsfshell_handler_t *handlers;
	struct vsfsm_pt_t input_pt;
	struct vsfsm_pt_t output_pt;
	struct vsfsm_crit_t output_crit;
	bool output_interrupted;
	bool prompted;
	char ch;
};

struct vsfshell_handler_param_t
{
	uint8_t argc;
	char *argv[16];
	
	// private
	struct vsfshell_t *shell;
	void *context;
	void *priv;
	struct vsfsm_t sm;
	struct vsfsm_pt_t pt;
	struct vsfsm_pt_t output_pt;
};

typedef vsf_err_t (*vsfshell_printf_thread_t)(struct vsfsm_pt_t *pt,
									vsfsm_evt_t evt, const char *format, ...);
#define vsfshell_printf(output_pt, format, ...)\
	do {\
		(output_pt)->state = 0;\
		(output_pt)->sm = (pt)->sm;\
		vsfsm_pt_entry(pt);\
		{\
			vsfshell_printf_thread_t thread =\
					(vsfshell_printf_thread_t)(output_pt)->thread;\
			vsf_err_t __err = thread(output_pt, evt, format, ##__VA_ARGS__);\
			if (__err != VSFERR_NONE)\
			{\
				return __err;\
			}\
		}\
	} while (0)

// for handlers
// vsfshell_handler_release_io is called when pt want to run in back-end
#define vsfshell_handler_release_io(pt)\
	do {\
		if (((struct vsfshell_handler_param_t *)(pt)->user_data)->shell->input_sm ==\
				&((struct vsfshell_handler_param_t *)(pt)->user_data)->sm)\
		{\
			vsfshell_printf(&((struct vsfshell_handler_param_t *)(pt)->user_data)->output_pt, VSFSHELL_PROMPT);\
			((struct vsfshell_handler_param_t *)(pt)->user_data)->shell->input_sm =\
				&(((struct vsfshell_handler_param_t *)(pt)->user_data))->shell->sm;\
		}\
	} while (0)
// vsfshell_handler_exit is called when pt exit
#define vsfshell_handler_exit(pt)\
	do {\
		vsfshell_handler_release_io(pt);\
		{\
			struct vsfshell_handler_param_t *param =\
							(struct vsfshell_handler_param_t *)(pt)->user_data;\
			struct vsfshell_t *shell = param->shell;\
			vsfshell_free_handler_thread(shell, &param->sm);\
		}\
	} while (0)

#ifdef VSFCFG_STANDALONE_MODULE
#define VSFSHELL_MODNAME					"vsf.framework.shell"

struct vsfshell_modifs_t
{
	vsf_err_t (*init)(struct vsfshell_t*);
	void (*register_handlers)(struct vsfshell_t*, struct vsfshell_handler_t*);
	void (*free_handler_thread)(struct vsfshell_t*, struct vsfsm_t*);
};

void vsfshell_modexit(struct vsf_module_t*);
vsf_err_t vsfshell_modinit(struct vsf_module_t*, struct app_hwcfg_t const*);

#define VSFSHELL_MOD						\
	((struct vsfshell_modifs_t *)vsf_module_load(VSFSHELL_MODNAME))
#define vsfshell_init						VSFSHELL_MOD->init
#define vsfshell_register_handlers			VSFSHELL_MOD->register_handlers
#define vsfshell_free_handler_thread		VSFSHELL_MOD->free_handler_thread

#else
vsf_err_t vsfshell_init(struct vsfshell_t*);
void vsfshell_register_handlers(struct vsfshell_t*, struct vsfshell_handler_t*);
void vsfshell_free_handler_thread(struct vsfshell_t*, struct vsfsm_t*);
#endif

#endif	// __VSFSHELL_H_INCLUDED__
