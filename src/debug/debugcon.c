/*###################################################################################################
**
**
**		debugcon.c
**		Debugger console engine.
**		Written by Aaron Giles
**
**
**#################################################################################################*/

#include "driver.h"
#include "debugcon.h"
#include "debugcpu.h"
#include "debugexp.h"
#include "debughlp.h"
#include "debugvw.h"
#include <stdarg.h>



/*###################################################################################################
**	DEBUGGING
**#################################################################################################*/



/*###################################################################################################
**	CONSTANTS
**#################################################################################################*/

#define CONSOLE_HISTORY		(10000)
#define CONSOLE_LINE_CHARS	(100)



/*###################################################################################################
**	TYPE DEFINITIONS
**#################################################################################################*/

struct debug_command
{
	struct debug_command *	next;
	const char *			command;
	void					(*handler)(int ref, int params, const char **param);
	int						ref;
	int						minparams;
	int						maxparams;
};



/*###################################################################################################
**	LOCAL VARIABLES
**#################################################################################################*/

static char *console_history;
static int total_lines;

static struct debug_command *commandlist;



/*###################################################################################################
**	PROTOTYPES
**#################################################################################################*/



/*###################################################################################################
**	CODE
**#################################################################################################*/

/*-------------------------------------------------
	debug_console_init - initializes the console
	system
-------------------------------------------------*/

void debug_console_init(void)
{
	/* allocate memory */
	console_history = malloc(CONSOLE_HISTORY * CONSOLE_LINE_CHARS);
	if (!console_history)
		return;

	/* print the opening lines */
	debug_console_printf("MAME new debugger version %s\n", build_version);
	debug_console_printf("Currently targeting %s (%s)\n", Machine->gamedrv->name, Machine->gamedrv->description);
}


/*-------------------------------------------------
	debug_console_exit - frees the console
	system
-------------------------------------------------*/

void debug_console_exit(void)
{
	/* free allocated memory */
	if (console_history)
		free(console_history);
	console_history = NULL;
}



/*###################################################################################################
**	COMMAND HANDLING
**#################################################################################################*/

/*-------------------------------------------------
	trim_parameter - executes a 
	command
-------------------------------------------------*/

static void trim_parameter(char **paramptr)
{
	char *param = *paramptr;
	int len = strlen(param);
	int repeat;

	/* loop until all adornments are gone */
	do
	{
		repeat = 0;
		
		/* check for begin/end quotes */
		if (len >= 2 && param[0] == '"' && param[len - 1] == '"')
		{
			param[len - 1] = 0;
			param++;
			len -= 2;
		}
		
		/* check for start/end braces */
		else if (len >= 2 && param[0] == '{' && param[len - 1] == '}')
		{
			param[len - 1] = 0;
			param++;
			len -= 2;
			repeat = 1;
		}
		
		/* check for leading spaces */
		else if (len >= 1 && param[0] == ' ')
		{
			param++;
			len--;
			repeat = 1;
		}
		
		/* check for trailing spaces */
		else if (len >= 1 && param[len - 1] == ' ')
		{
			param[len - 1] = 0;
			len--;
			repeat = 1;
		}
	} while (repeat);
	
	*paramptr = param;
}


/*-------------------------------------------------
	internal_execute_command - executes a 
	command
-------------------------------------------------*/

static CMDERR internal_execute_command(int validate_only, int params, char **param)
{
	struct debug_command *cmd, *found = NULL;
	int len, i, foundcount = 0;
	char *p, *command;

	/* no params is an error */
	if (params == 0)
		return CMDERR_NONE;
	
	/* the first parameter has the command and the real first parameter; separate them */
	for (p = param[0]; *p && isspace(*p); p++) { }
	for (command = p; *p && !isspace(*p); p++) { }
	if (*p != 0)
	{
		*p++ = 0;
		for ( ; *p && isspace(*p); p++) { }
		if (*p != 0)
			param[0] = p;
		else
			params = 0;
	}
	else
		params = 0;
	
	/* NULL-terminate and trim space around all the parameters */
	for (i = 1; i < params; i++)
		*param[i]++ = 0;
	
	/* now go back and trim quotes and braces and any spaces they reveal*/
	for (i = 0; i < params; i++)
		trim_parameter(&param[i]);

	/* search the command list */
	len = strlen(command);
	for (cmd = commandlist; cmd != NULL; cmd = cmd->next)
		if (!strncmp(command, cmd->command, len))
		{
			foundcount++;
			found = cmd;
			if (strlen(cmd->command) == len)
			{
				foundcount = 1;
				break;
			}
		}
	
	/* error if not found */
	if (!found)
		return MAKE_CMDERR_UNKNOWN_COMMAND(0);
	if (foundcount > 1)
		return MAKE_CMDERR_AMBIGUOUS_COMMAND(0);
	
	/* see if we have the right number of parameters */
	if (params < found->minparams)
		return MAKE_CMDERR_NOT_ENOUGH_PARAMS(0);
	if (params > found->maxparams)
		return MAKE_CMDERR_TOO_MANY_PARAMS(0);
	
	/* execute the handler */
	if (!validate_only)
		(*found->handler)(found->ref, params, (const char **)param);
	return CMDERR_NONE;
}


/*-------------------------------------------------
	internal_parse_command - parses a command
	and either executes or just validates it
-------------------------------------------------*/

static CMDERR internal_parse_command(const char *original_command, int validate_only)
{
	char command[MAX_COMMAND_LENGTH], parens[MAX_COMMAND_LENGTH];
	char *params[MAX_COMMAND_PARAMS];
	CMDERR result = CMDERR_NONE;
	char *command_start;
	char *p, c;
	
	/* make a copy of the command */
	strcpy(command, original_command);
	
	/* loop over all semicolon-separated stuff */
	for (p = command; *p != 0; )
	{
		int paramcount = 0, foundend = 0, instring = 0, parendex = 0;
	
		/* find a semicolon or the end */
		for (params[paramcount++] = p; !foundend; p++)
		{
			c = *p;
			if (instring)
			{
				if (c == '"' && p[-1] != '\\')
					instring = 0;
			}
			else
			{
				switch (c)
				{
					case '"':	instring = 1; break;
					case '(':
					case '[':
					case '{':	parens[parendex++] = c; break;
					case ')':	if (parendex == 0 || parens[--parendex] != '(') return MAKE_CMDERR_UNBALANCED_PARENS(p - command); break;
					case ']':	if (parendex == 0 || parens[--parendex] != '[') return MAKE_CMDERR_UNBALANCED_PARENS(p - command); break;
					case '}':	if (parendex == 0 || parens[--parendex] != '{') return MAKE_CMDERR_UNBALANCED_PARENS(p - command); break;
					case ',':	if (parendex == 0) params[paramcount++] = p; break;
					case ';': 	if (parendex == 0) foundend = 1; break;
					case 0:		foundend = 1; break;
					default:	*p = tolower(c); break;
				}
			}
		}
		
		/* check for unbalanced parentheses or quotes */
		if (instring)
			return MAKE_CMDERR_UNBALANCED_QUOTES(p - command);
		if (parendex != 0)
			return MAKE_CMDERR_UNBALANCED_PARENS(p - command);
		
		/* NULL-terminate if we ended in a semicolon */
		p--;
		if (c == ';') *p++ = 0;
		
		/* process the command */
		command_start = params[0];
		result = internal_execute_command(validate_only, paramcount, &params[0]);
		if (result != CMDERR_NONE)
			return MAKE_CMDERR(CMDERR_ERROR_CLASS(result), command_start - command);
	}
	return CMDERR_NONE;
}


/*-------------------------------------------------
	debug_console_execute_command - execute a
	command string
-------------------------------------------------*/

CMDERR debug_console_execute_command(const char *command, int echo)
{
	CMDERR result;
	
	/* echo if requested */
	if (echo)
		debug_console_printf(">%s", command);
	
	/* parse and execute */
	result = internal_parse_command(command, 0);
	
	/* display errors */
	if (result != CMDERR_NONE)
	{
		if (!echo)
			debug_console_printf(">%s", command);
		debug_console_printf(" %*s^", CMDERR_ERROR_OFFSET(result), "");
		debug_console_printf("%s\n", debug_cmderr_to_string(result));
	}
	
	/* update all views */
	if (echo)
	{
		debug_view_update_all();
		debug_refresh_display();
	}
	return result;
}


/*-------------------------------------------------
	debug_console_validate_command - validate a
	command string
-------------------------------------------------*/

CMDERR debug_console_validate_command(const char *command)
{
	return internal_parse_command(command, 1);
}


/*-------------------------------------------------
	debug_console_register_command - register a
	command handler
-------------------------------------------------*/

void debug_console_register_command(const char *command, int ref, int minparams, int maxparams, void (*handler)(int ref, int params, const char **param))
{
	struct debug_command *cmd = malloc(sizeof(*cmd));
	if (!cmd)
		osd_die("Out of memory registering new command with the debugger!");
		
	/* fill in the command */
	cmd->command = command;
	cmd->ref = ref;
	cmd->minparams = minparams;
	cmd->maxparams = maxparams;
	cmd->handler = handler;
	
	/* link it */
	cmd->next = commandlist;
	commandlist = cmd;
}



/*###################################################################################################
**	ERROR HANDLING
**#################################################################################################*/

/*-------------------------------------------------
	debug_cmderr_to_string - return a friendly 
	string for a given command error
-------------------------------------------------*/

const char *debug_cmderr_to_string(CMDERR error)
{
	switch (CMDERR_ERROR_CLASS(error))
	{
		case CMDERR_UNKNOWN_COMMAND:		return "unknown command";
		case CMDERR_AMBIGUOUS_COMMAND:		return "ambiguous command";
		case CMDERR_UNBALANCED_PARENS:		return "unbalanced parentheses";
		case CMDERR_UNBALANCED_QUOTES:		return "unbalanced quotes";
		case CMDERR_NOT_ENOUGH_PARAMS:		return "not enough parameters for command";
		case CMDERR_TOO_MANY_PARAMS:		return "too many parameters for command";
		default:							return "unknown error";
	}
}



/*###################################################################################################
**	CONSOLE MANAGEMENT
**#################################################################################################*/

/*-------------------------------------------------
	debug_console_clear - clears the console
	buffer
-------------------------------------------------*/

void debug_console_clear(void)
{
	total_lines = 0;
}


/*-------------------------------------------------
	debug_console_write_line - writes a line to
	the output ring buffer
-------------------------------------------------*/

void debug_console_write_line(const char *line)
{
	/* add the line to the history */
	if (console_history)
	{
		while (*line)
		{
			char *dest = &console_history[(total_lines++ % CONSOLE_HISTORY) * CONSOLE_LINE_CHARS];
			int col = 0;
			while (*line && col < CONSOLE_LINE_CHARS - 1)
			{
				char c = *line++;
				if (c != '\n')
					dest[col++] = c++;
				else
					break;
			}
			dest[col] = 0;
		}
		
		/* force an update of any console views */
		debug_view_update_type(DVT_CONSOLE);
	}
}


/*-------------------------------------------------
	debug_console_printf - printfs the given
	arguments using the format to the debug
	console
-------------------------------------------------*/

void CLIB_DECL debug_console_printf(const char *format, ...)
{
	char buffer[512];
	va_list arg;

	va_start(arg, format);
	vsprintf(buffer, format, arg);
	va_end(arg);
	
	debug_console_write_line(buffer);
}


/*-------------------------------------------------
	debug_console_get_line - retrieves the nth
	line from the buffer
-------------------------------------------------*/

const char *debug_console_get_line(int index)
{
	/* return a line from the history */
	if (console_history)
	{
		/* if we're less than CONSOLE_HISTORY lines, start at 0 */
		if (total_lines < CONSOLE_HISTORY)
			return &console_history[(index % CONSOLE_HISTORY) * CONSOLE_LINE_CHARS];
		else
			return &console_history[((total_lines + index) % CONSOLE_HISTORY) * CONSOLE_LINE_CHARS];
	}
	return "";
}


/*-------------------------------------------------
	debug_console_get_size - retrieves the size
	in rows and columns
-------------------------------------------------*/

void debug_console_get_size(int *rows, int *cols)
{
	*rows = (total_lines < CONSOLE_HISTORY) ? total_lines : CONSOLE_HISTORY;
	*cols = CONSOLE_LINE_CHARS;
}
