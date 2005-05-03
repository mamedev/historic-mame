/*###################################################################################################
**
**
**      debugcon.h
**      Debugger console engine.
**      Written by Aaron Giles
**
**
**#################################################################################################*/

#ifndef __DEBUGCON_H__
#define __DEBUGCON_H__


/*###################################################################################################
**  CONSTANTS
**#################################################################################################*/

#define MAX_COMMAND_LENGTH		512
#define MAX_COMMAND_PARAMS		16

/* values for the error code in an command error */
#define CMDERR_NONE							(0)
#define CMDERR_UNKNOWN_COMMAND				(1)
#define CMDERR_AMBIGUOUS_COMMAND			(2)
#define CMDERR_UNBALANCED_PARENS			(3)
#define CMDERR_UNBALANCED_QUOTES			(4)
#define CMDERR_NOT_ENOUGH_PARAMS			(5)
#define CMDERR_TOO_MANY_PARAMS				(6)



/*###################################################################################################
**  MACROS
**#################################################################################################*/

/* command error assembly/disassembly macros */
#define CMDERR_ERROR_CLASS(x)				((x) >> 16)
#define CMDERR_ERROR_OFFSET(x)				((x) & 0xffff)
#define MAKE_CMDERR(a,b)					(((a) << 16) | ((b) & 0xffff))

/* macros to assemble specific error conditions */
#define MAKE_CMDERR_UNKNOWN_COMMAND(x)		MAKE_CMDERR(CMDERR_UNKNOWN_COMMAND, (x))
#define MAKE_CMDERR_AMBIGUOUS_COMMAND(x)	MAKE_CMDERR(CMDERR_AMBIGUOUS_COMMAND, (x))
#define MAKE_CMDERR_UNBALANCED_PARENS(x)	MAKE_CMDERR(CMDERR_UNBALANCED_PARENS, (x))
#define MAKE_CMDERR_UNBALANCED_QUOTES(x)	MAKE_CMDERR(CMDERR_UNBALANCED_QUOTES, (x))
#define MAKE_CMDERR_NOT_ENOUGH_PARAMS(x)	MAKE_CMDERR(CMDERR_NOT_ENOUGH_PARAMS, (x))
#define MAKE_CMDERR_TOO_MANY_PARAMS(x)		MAKE_CMDERR(CMDERR_TOO_MANY_PARAMS, (x))



/*###################################################################################################
**  TYPE DEFINITIONS
**#################################################################################################*/

/* EXPRERR is an error code for command evaluation */
typedef UINT32 CMDERR;



/*###################################################################################################
**  FUNCTION PROTOTYPES
**#################################################################################################*/

/* initialization */
void				debug_console_init(void);
void				debug_console_exit(void);

/* command handling */
CMDERR				debug_console_execute_command(const char *command, int echo);
CMDERR				debug_console_validate_command(const char *command);
void				debug_console_register_command(const char *command, int ref, int minparams, int maxparams, void (*handler)(int ref, int params, const char **param));
const char *		debug_cmderr_to_string(CMDERR error);

/* console management */
void 				debug_console_clear(void);
void 				debug_console_write_line(const char *line);
#ifdef __GNUC__
void CLIB_DECL		debug_console_printf(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
#else
void CLIB_DECL		debug_console_printf(const char *format, ...);
#endif
const char *		debug_console_get_line(int index);
void				debug_console_get_size(int *rows, int *cols);

#endif
