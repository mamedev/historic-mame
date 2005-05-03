/*###################################################################################################
**
**
**      debugexp.h
**      Debugger expressions engine.
**      Written by Aaron Giles
**
**
**#################################################################################################*/

#ifndef __DEBUGEXP_H__
#define __DEBUGEXP_H__


/*###################################################################################################
**  CONSTANTS
**#################################################################################################*/

/* global symbol table */
#define GLOBAL_SYMBOL_TABLE					(NULL)

/* maximum number of parameters in a function call */
#define MAX_FUNCTION_PARAMS					(16)

/* values for symbol_entry.type */
#define SMT_REGISTER						(0)
#define SMT_FUNCTION						(1)

/* values for the error code in an expression error */
#define EXPRERR_NONE						(0)
#define EXPRERR_NOT_LVAL					(1)
#define EXPRERR_NOT_RVAL					(2)
#define EXPRERR_SYNTAX						(3)
#define EXPRERR_UNKNOWN_SYMBOL				(4)
#define EXPRERR_INVALID_NUMBER				(5)
#define EXPRERR_INVALID_TOKEN				(6)
#define EXPRERR_STACK_OVERFLOW				(7)
#define EXPRERR_STACK_UNDERFLOW				(8)
#define EXPRERR_UNBALANCED_PARENS			(9)
#define EXPRERR_DIVIDE_BY_ZERO				(10)
#define EXPRERR_OUT_OF_MEMORY				(11)
#define EXPRERR_INVALID_PARAM_COUNT			(12)
#define EXPRERR_UNBALANCED_QUOTES			(13)
#define EXPRERR_TOO_MANY_STRINGS			(14)



/*###################################################################################################
**  MACROS
**#################################################################################################*/

/* expression error assembly/disassembly macros */
#define EXPRERR_ERROR_CLASS(x)				((x) >> 16)
#define EXPRERR_ERROR_OFFSET(x)				((x) & 0xffff)
#define MAKE_EXPRERR(a,b)					(((a) << 16) | ((b) & 0xffff))

/* macros to assemble specific error conditions */
#define MAKE_EXPRERR_NOT_LVAL(x)			MAKE_EXPRERR(EXPRERR_NOT_LVAL, (x))
#define MAKE_EXPRERR_NOT_RVAL(x)			MAKE_EXPRERR(EXPRERR_NOT_RVAL, (x))
#define MAKE_EXPRERR_SYNTAX(x)				MAKE_EXPRERR(EXPRERR_SYNTAX, (x))
#define MAKE_EXPRERR_UNKNOWN_SYMBOL(x)		MAKE_EXPRERR(EXPRERR_UNKNOWN_SYMBOL, (x))
#define MAKE_EXPRERR_INVALID_NUMBER(x)		MAKE_EXPRERR(EXPRERR_INVALID_NUMBER, (x))
#define MAKE_EXPRERR_INVALID_TOKEN(x)		MAKE_EXPRERR(EXPRERR_INVALID_TOKEN, (x))
#define MAKE_EXPRERR_STACK_OVERFLOW(x)		MAKE_EXPRERR(EXPRERR_STACK_OVERFLOW, (x))
#define MAKE_EXPRERR_STACK_UNDERFLOW(x)		MAKE_EXPRERR(EXPRERR_STACK_UNDERFLOW, (x))
#define MAKE_EXPRERR_UNBALANCED_PARENS(x)	MAKE_EXPRERR(EXPRERR_UNBALANCED_PARENS, (x))
#define MAKE_EXPRERR_DIVIDE_BY_ZERO(x)		MAKE_EXPRERR(EXPRERR_DIVIDE_BY_ZERO, (x))
#define MAKE_EXPRERR_OUT_OF_MEMORY(x)		MAKE_EXPRERR(EXPRERR_OUT_OF_MEMORY, (x))
#define MAKE_EXPRERR_INVALID_PARAM_COUNT(x)	MAKE_EXPRERR(EXPRERR_INVALID_PARAM_COUNT, (x))
#define MAKE_EXPRERR_UNBALANCED_QUOTES(x)	MAKE_EXPRERR(EXPRERR_UNBALANCED_QUOTES, (x))
#define MAKE_EXPRERR_TOO_MANY_STRINGS(x)	MAKE_EXPRERR(EXPRERR_TOO_MANY_STRINGS, (x))



/*###################################################################################################
**  TYPE DEFINITIONS
**#################################################################################################*/

/* register_info provides symbol information that is specific to a register type */
struct register_info
{
	UINT64			(*getter)(UINT32);			/* value getter */
	void			(*setter)(UINT32, UINT64);	/* value setter */
};

/* function_info provides symbol information that is specific to a function type */
struct function_info
{
	UINT16			minparams;					/* minimum expected parameters */
	UINT16			maxparams;					/* maximum expected parameters */
	UINT64			(*execute)(UINT32, UINT32, UINT64 *);/* execute */
};

/* symbol_entry describes a symbol in a symbol table */
struct symbol_entry
{
	const char *	name;						/* name of the symbol */
	UINT32			ref;						/* internal reference */
	UINT32			type;						/* type of symbol */
	union
	{
		struct register_info reg;				/* register info */
		struct function_info func;				/* function info */
	} info;
};

/* symbol_table is an opaque structure for holding a collection of symbols */
struct symbol_table;

/* parsed_expression is an opaque structure for holding a pre-parsed expression */
struct parsed_expression;

/* EXPRERR is an error code for expression evaluation */
typedef UINT32 EXPRERR;



/*###################################################################################################
**  FUNCTION PROTOTYPES
**#################################################################################################*/

/* expression evaluation */
EXPRERR 					debug_expression_evaluate(const char *expression, const struct symbol_table *table, UINT64 *result);
EXPRERR 					debug_expression_parse(const char *expression, const struct symbol_table *table, struct parsed_expression **result);
EXPRERR 					debug_expression_execute(struct parsed_expression *expr, UINT64 *result);
void 						debug_expression_free(struct parsed_expression *expr);
const char *				debug_expression_original_string(struct parsed_expression *expr);
const char *				debug_exprerr_to_string(EXPRERR error);

/* symbol table manipulation */
struct symbol_table *		debug_symtable_alloc(void);
int 						debug_symtable_add(struct symbol_table *table, const struct symbol_entry *entry);
int 						debug_symtable_add_register(struct symbol_table *table, const char *name, UINT32 ref, UINT64 (*getter)(UINT32), void (*setter)(UINT32, UINT64));
int 						debug_symtable_add_function(struct symbol_table *table, const char *name, UINT32 ref, UINT16 minparams, UINT16 maxparams, UINT64 (*execute)(UINT32, UINT32, UINT64 *));
const struct symbol_entry *	debug_symtable_find(const struct symbol_table *table, const char *symbol);
void 						debug_symtable_free(struct symbol_table *table);

#endif
