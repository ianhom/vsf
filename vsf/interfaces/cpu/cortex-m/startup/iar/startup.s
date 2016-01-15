	SECTION	entry:CODE:ROOT(4)
	EXTERN	__iar_program_start
	EXTERN	module_exit
	DATA
	DCD		__iar_program_start - VSF_APP_BASE
	DCD		module_exit - VSF_APP_BASE
	DCD		MODULE_SIZE
	DCD		MODULE_NAME
	END
