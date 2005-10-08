/***************************************************************************

    state.c

    Save state management functions.

    Save state file format:

     0.. 7  'MAMESAVE"
     8      Format version (this is format 1)
     9      Flags
     a..13  Game name padded with \0
    14..17  Signature
    18..end Save game data

***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "driver.h"
#include "zlib.h"



/*************************************
 *
 *  Debugging
 *
 *************************************/

//#define VERBOSE

#ifdef VERBOSE
#define TRACE(x) do {x;} while (0)
#else
#define TRACE(x)
#endif



/*************************************
 *
 *  Constants
 *
 *************************************/

#define SAVE_VERSION		1

#define TAG_STACK_SIZE		4

/* Available flags */
enum
{
	SS_NO_SOUND = 0x01,
	SS_MSB_FIRST = 0x02
};

enum
{
	MAX_INSTANCES = 64
};

enum
{
	FUNC_NOPARAM,
	FUNC_INTPARAM,
	FUNC_PTRPARAM
};

enum
{
	SS_INT8,
	SS_UINT8,
	SS_INT16,
	SS_UINT16,
	SS_INT32,
	SS_UINT32,
	SS_INT64,
	SS_UINT64,
	SS_INT,
	SS_DOUBLE,
	SS_FLOAT
};



/*************************************
 *
 *  Type definitions
 *
 *************************************/

struct _ss_entry
{
	struct _ss_entry *next;
	char *name;
	int type;
	void *data;
	unsigned size;
	int tag;
	int restag;
	unsigned offset;
};
typedef struct _ss_entry ss_entry;


struct _ss_module
{
	struct _ss_module *next;
	char *name;
	ss_entry *instances[MAX_INSTANCES];
};
typedef struct _ss_module ss_module;


struct _ss_func
{
	struct _ss_func *next;
	int type;
	union
	{
		void (*voidf)(void);
		void (*intf)(int param);
		void (*ptrf)(void *param);
	} func;
	union
	{
		int intp;
		void *ptrp;
	} param;
	int tag;
	int restag;
};
typedef struct _ss_func ss_func;



/*************************************
 *
 *  Statics
 *
 *************************************/

#ifdef VERBOSE
static const char *ss_type[] =	{ "i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64", "int", "dbl", "flt" };
#endif
static int         ss_size[] =	{  1,   1,     2,     2,     4,     4,     8,     8,     4,     8,     4 };

static void ss_c2(UINT8 *, unsigned);
static void ss_c4(UINT8 *, unsigned);
static void ss_c8(UINT8 *, unsigned);

static void (*ss_conv[])(UINT8 *, unsigned) = {
	0, 0, ss_c2, ss_c2, ss_c4, ss_c4, ss_c8, ss_c8, 0, ss_c8, ss_c4
};



static int ss_illegal_regs;
static ss_module *ss_registry;
static ss_func *ss_prefunc_reg;
static ss_func *ss_postfunc_reg;

static int ss_tag_stack[TAG_STACK_SIZE];
static int ss_tag_stack_index;

static int ss_current_tag;
static int ss_registration_allowed;

static UINT8 *ss_dump_array;
static mame_file *ss_dump_file;
static UINT32 ss_dump_size;

#ifdef MESS
static const char ss_magic_num[8] = { 'M', 'E', 'S', 'S', 'S', 'A', 'V', 'E' };
#else
static const char ss_magic_num[8] = { 'M', 'A', 'M', 'E', 'S', 'A', 'V', 'E' };
#endif



/*************************************
 *
 *  Compute the total size and
 *  offsets of each individual item
 *
 *************************************/

static int compute_size_and_offsets(void)
{
	int total_size;
	ss_module *module;

	/* start with the header size */
	total_size = 0x18;

	/* iterate over modules */
	for (module = ss_registry; module; module = module->next)
	{
		int instnum;

		/* iterate over instances */
		for (instnum = 0; instnum < MAX_INSTANCES; instnum++)
		{
			ss_entry *entry;

			/* iterate over entries */
			for (entry = module->instances[instnum]; entry; entry = entry->next)
			{
				/* note the offset and accumulate a total size */
				entry->offset = total_size;
				total_size += ss_size[entry->type] * entry->size;
			}
		}
	}

	/* return the total size */
	return total_size;
}



/*************************************
 *
 *  Loop through all the hook
 *  functions and call them
 *
 *************************************/

static int call_hook_functions(ss_func *funclist)
{
	ss_func *func;
	int count = 0;

	/* iterate over the list of functions */
	for (func = funclist; func; func = func->next)
		if (func->tag == ss_current_tag)
		{
			count++;

			/* call with the appropriate parameters */
			switch (func->type)
			{
				case FUNC_NOPARAM:	(func->func.voidf)();					break;
				case FUNC_INTPARAM:	(func->func.intf)(func->param.intp);	break;
				case FUNC_PTRPARAM:	(func->func.ptrf)(func->param.ptrp);	break;
			}
		}

	return count;
}



/*************************************
 *
 *  Compute the signature, which is
 *  a CRC over the structure of the
 *  data
 *
 *************************************/

static UINT32 ss_get_signature(void)
{
	ss_module *module;
	UINT32 crc = 0;

	/* iterate over modules */
	for (module = ss_registry; module; module = module->next)
	{
		int instnum;

		/* add the module name to the CRC */
		crc = crc32(crc, (UINT8 *)module->name, strlen(module->name) + 1);

		/* iterate over instances */
		for (instnum = 0; instnum < MAX_INSTANCES; instnum++)
		{
			ss_entry *entry;
			UINT8 temp[5];

			/* add this instance number to the CRC */
			temp[0] = instnum;
			crc = crc32(crc, temp, 1);

			/* iterate over entries */
			for (entry = module->instances[instnum]; entry; entry = entry->next)
			{
				/* add the entry name to the CRC */
				crc = crc32(crc, (UINT8 *)entry->name, strlen(entry->name) + 1);

				/* add the type and size to the CRC */
				temp[0] = entry->type;
				*(UINT32 *)&temp[1] = LITTLE_ENDIANIZE_INT32(entry->size);
				crc = crc32(crc, temp, 5);
			}
		}
	}
	return crc;
}



/*************************************
 *
 *  Check the header data
 *
 *************************************/

static int validate_header(const UINT8 *header, const char *gamename, UINT32 signature,
	void (CLIB_DECL *errormsg)(const char *fmt, ...), const char *error_prefix)
{
	/* check magic number */
	if (memcmp(header, ss_magic_num, 8))
	{
		if (errormsg)
			errormsg("%sThis is not a " APPNAME " save file", error_prefix);
		return -1;
	}

	/* check save state version */
	if (header[8] != SAVE_VERSION)
	{
		if (errormsg)
			errormsg("%sWrong version in save file (%d, 1 expected)", error_prefix, header[8]);
		return -1;
	}

	/* check gamename, if we were asked to */
	if (gamename && strcmp(gamename, (const char *)&header[10]))
	{
		if (errormsg)
			errormsg("%s'%s' is not a valid savestate file for game '%s'.", error_prefix, gamename);
		return -1;
	}

	/* check signature, if we were asked to */
	if (signature)
	{
		UINT32 rawsig = *(UINT32 *)&header[0x14];
		UINT32 filesig = LITTLE_ENDIANIZE_INT32(rawsig);
		if (signature != filesig)
		{
			if (errormsg)
				errormsg("%sIncompatible save file (signature %08x, expected %08x)",
					error_prefix, filesig, signature);
			return -1;
		}
	}
	return 0;
}



/*************************************
 *
 *  Free registered functions attached
 *  to the current resource tag
 *
 *************************************/

static void func_free(ss_func **root)
{
	int restag = get_resource_tag();
	ss_func **func;

	/* iterate over the function list */
	for (func = root; *func; )
	{
		/* if this entry matches, free it */
		if ((*func)->restag == restag)
		{
			ss_func *func_to_free = *func;

			/* de-link us from the list and free our memory */
			*func = (*func)->next;
			free(func_to_free);
		}

		/* otherwise, advance */
		else
			func = &(*func)->next;
	}
}



/*************************************
 *
 *  Free registered pointers attached
 *  to the current resource tag
 *
 *************************************/

void state_save_free(void)
{
	int restag = get_resource_tag();
	ss_module **module;

	/* free everything attached to the current resource tag */
	for (module = &ss_registry; *module; )
	{
		int instnum, remaining = 0;

		/* iterate over instances */
		for (instnum = 0; instnum < MAX_INSTANCES; instnum++)
		{
			ss_entry **entry;

			/* iterate over entries */
			for (entry = &(*module)->instances[instnum]; *entry; )
			{
				/* if this entry matches, free it */
				if ((*entry)->restag == restag)
				{
					ss_entry *entry_to_free = *entry;

					/* de-link us from the list and free our memory */
					*entry = (*entry)->next;
					free(entry_to_free->name);
					free(entry_to_free);
				}

				/* if not a match, move on */
				else
					entry = &(*entry)->next;
			}

			/* if anything left, remember that we have some remaining */
			if ((*module)->instances[instnum] != NULL)
				remaining++;
		}

		/* if nothing is left in this module, free the entire module */
		if (remaining == 0)
		{
			ss_module *module_to_free = *module;

			/* de-link us from the list and free our memory */
			*module = (*module)->next;
			free(module_to_free->name);
			free(module_to_free);
		}

		/* otherwise, advance */
		else
			module = &(*module)->next;
	}

	/* now do the same with the function lists */
	func_free(&ss_prefunc_reg);
	func_free(&ss_postfunc_reg);

	/* if we're clear of all registrations, reset the invalid counter */
	if (ss_registry == NULL && ss_prefunc_reg == NULL && ss_postfunc_reg == NULL)
		ss_illegal_regs = 0;
}



/*************************************
 *
 *  Get a module by name or allocate
 *  a new one
 *
 *************************************/

static ss_module *ss_get_module(const char *name)
{
	ss_module **module;

	/* find a match */
	for (module = &ss_registry; *module; module = &(*module)->next)
		if (!strcmp((*module)->name, name))
			return *module;

	/* didn't find one; allocate a new one */
	*module = malloc(sizeof(**module));
	if (!*module)
		osd_die("Out of memory allocating a new save state module");
	memset(*module, 0, sizeof(**module));

	/* allocate space for the name */
	(*module)->name = malloc(strlen(name) + 1);
	if (!(*module)->name)
		osd_die("Out of memory allocating a new save state module");
	strcpy((*module)->name, name);

	return *module;
}



/*************************************
 *
 *  Count the number of registered
 *  entries on the current tag
 *
 *************************************/

int state_save_get_reg_count(void)
{
	ss_module *module;
	int count = 0;

	/* iterate over modules */
	for (module = ss_registry; module; module = module->next)
	{
		int instnum;

		/* iterate over instances */
		for (instnum = 0; instnum < MAX_INSTANCES; instnum++)
		{
			ss_entry *entry;

			/* iterate over entries with matching tags */
			for (entry = module->instances[instnum]; entry; entry = entry->next)
				if (entry->tag == ss_current_tag)
					count++;
		}
	}
	return count;
}



/*************************************
 *
 *  Register a new entry
 *
 *************************************/

static ss_entry *ss_register_entry(const char *modname, int instance, const char *name, int type, void *data, UINT32 size)
{
	ss_module *module = ss_get_module(modname);
	ss_entry **entry;

	/* check for invalid timing */
	if (!ss_registration_allowed)
	{
		logerror("Attempt to register save state entry after state registration is closed!");
		ss_illegal_regs++;
		return NULL;
	}

	/* look for duplicates and find the end */
	for (entry = &module->instances[instance]; *entry; entry = &(*entry)->next)
		if ((*entry)->tag == ss_current_tag && !strcmp((*entry)->name, name))
			osd_die("Duplicate save state registration entry (%d, %s, %d, %s)\n", ss_current_tag, modname, instance, name);

	/* didn't find one; allocate a new one */
	*entry = malloc(sizeof(**entry));
	if (!*entry)
		osd_die("Out of memory allocating a new save state entry");
	memset(*entry, 0, sizeof(**entry));

	/* allocate space for the name */
	(*entry)->name = malloc(strlen(name) + 1);
	if (!(*entry)->name)
		osd_die("Out of memory allocating a new save state entry");
	strcpy((*entry)->name, name);

	/* fill in the rest */
	(*entry)->type   = type;
	(*entry)->data   = data;
	(*entry)->size   = size;
	(*entry)->offset = 0;
	(*entry)->tag    = ss_current_tag;
	(*entry)->restag = get_resource_tag();

	return *entry;
}

void state_save_register_UINT8 (const char *module, int instance,
								const char *name, UINT8 *val, unsigned size)
{
	ss_register_entry(module, instance, name, SS_UINT8, val, size);
}

void state_save_register_INT8  (const char *module, int instance,
								const char *name, INT8 *val, unsigned size)
{
	ss_register_entry(module, instance, name, SS_INT8, val, size);
}

void state_save_register_UINT16(const char *module, int instance,
								const char *name, UINT16 *val, unsigned size)
{
	ss_register_entry(module, instance, name, SS_UINT16, val, size);
}

void state_save_register_INT16 (const char *module, int instance,
								const char *name, INT16 *val, unsigned size)
{
	ss_register_entry(module, instance, name, SS_INT16, val, size);
}

void state_save_register_UINT32(const char *module, int instance,
								const char *name, UINT32 *val, unsigned size)
{
	ss_register_entry(module, instance, name, SS_UINT32, val, size);
}

void state_save_register_INT32 (const char *module, int instance,
								const char *name, INT32 *val, unsigned size)
{
	ss_register_entry(module, instance, name, SS_INT32, val, size);
}

void state_save_register_UINT64(const char *module, int instance,
								const char *name, UINT64 *val, unsigned size)
{
	ss_register_entry(module, instance, name, SS_UINT64, val, size);
}

void state_save_register_INT64 (const char *module, int instance,
								const char *name, INT64 *val, unsigned size)
{
	ss_register_entry(module, instance, name, SS_INT64, val, size);
}

void state_save_register_int   (const char *module, int instance,
								const char *name, int *val)
{
	ss_register_entry(module, instance, name, SS_INT, val, 1);
}

void state_save_register_double(const char *module, int instance,
								const char *name, double *val, unsigned size)
{
	ss_register_entry(module, instance, name, SS_DOUBLE, val, size);
}

void state_save_register_float(const char *module, int instance,
							   const char *name, float *val, unsigned size)
{
	ss_register_entry(module, instance, name, SS_FLOAT, val, size);
}




/*************************************
 *
 *  Tagging and registration
 *
 *************************************/

void state_save_push_tag(int tag)
{
	if (ss_tag_stack_index == TAG_STACK_SIZE - 1)
		osd_die("state_save tag stack overflow");
	ss_tag_stack[ss_tag_stack_index++] = ss_current_tag;
	ss_current_tag = tag;
}


void state_save_pop_tag(void)
{
	if (ss_tag_stack_index == 0)
		osd_die("state_save tag stack underflow");
	ss_current_tag = ss_tag_stack[--ss_tag_stack_index];
}


void state_save_allow_registration(int allowed)
{
	/* allow/deny registration */
	ss_registration_allowed = allowed;
}


int state_save_registration_allowed(void)
{
	return ss_registration_allowed;
}




/*************************************
 *
 *  Callback function registration
 *
 *************************************/

static void ss_register_func_void(ss_func **root, void (*func)(void))
{
	ss_func **cur;

	/* check for invalid timing */
	if (!ss_registration_allowed)
	{
		logerror("Attempt to register callback function after state registration is closed!");
		ss_illegal_regs++;
		return;
	}

	/* scan for duplicates and push through to the end */
	for (cur = root; *cur; cur = &(*cur)->next)
		if ((*cur)->func.voidf == func && (*cur)->tag == ss_current_tag)
			osd_die("Duplicate save state function (%d, 0x%x)\n", ss_current_tag, (int)func);

	/* allocate a new entry */
	*cur = malloc(sizeof(ss_func));
	if (*cur == NULL)
		osd_die("malloc failed in ss_register_func\n");

	/* fill it in */
	(*cur)->next       = NULL;
	(*cur)->type       = FUNC_NOPARAM;
	(*cur)->func.voidf = func;
	(*cur)->tag        = ss_current_tag;
	(*cur)->restag     = get_resource_tag();
}

void state_save_register_func_presave(void (*func)(void))
{
	ss_register_func_void(&ss_prefunc_reg, func);
}

void state_save_register_func_postload(void (*func)(void))
{
	ss_register_func_void(&ss_postfunc_reg, func);
}



static void ss_register_func_int(ss_func **root, void (*func)(int), int param)
{
	ss_func **cur;

	/* check for invalid timing */
	if (!ss_registration_allowed)
		osd_die("Attempt to register callback function after state registration is closed!");

	/* scan for duplicates and push through to the end */
	for (cur = root; *cur; cur = &(*cur)->next)
		if ((*cur)->func.intf == func && (*cur)->param.intp == param && (*cur)->tag == ss_current_tag)
			osd_die("Duplicate save state function (%d, %d, 0x%x)\n", ss_current_tag, param, (int)func);

	/* allocate a new entry */
	*cur = malloc(sizeof(ss_func));
	if (*cur == NULL)
		osd_die("malloc failed in ss_register_func\n");

	/* fill it in */
	(*cur)->next       = NULL;
	(*cur)->type       = FUNC_INTPARAM;
	(*cur)->func.intf  = func;
	(*cur)->param.intp = param;
	(*cur)->tag        = ss_current_tag;
	(*cur)->restag     = get_resource_tag();
}

void state_save_register_func_presave_int(void (*func)(int), int param)
{
	ss_register_func_int(&ss_prefunc_reg, func, param);
}

void state_save_register_func_postload_int(void (*func)(int), int param)
{
	ss_register_func_int(&ss_postfunc_reg, func, param);
}



static void ss_register_func_ptr(ss_func **root, void (*func)(void *), void *param)
{
	ss_func **cur;

	/* check for invalid timing */
	if (!ss_registration_allowed)
		osd_die("Attempt to register callback function after state registration is closed!");

	/* scan for duplicates and push through to the end */
	for (cur = root; *cur; cur = &(*cur)->next)
		if ((*cur)->func.ptrf == func && (*cur)->param.ptrp == param && (*cur)->tag == ss_current_tag)
			osd_die("Duplicate save state function (%d, %p, 0x%x)\n", ss_current_tag, param, (int)func);

	/* allocate a new entry */
	*cur = malloc(sizeof(ss_func));
	if (*cur == NULL)
		osd_die("malloc failed in ss_register_func\n");

	/* fill it in */
	(*cur)->next       = NULL;
	(*cur)->type       = FUNC_PTRPARAM;
	(*cur)->func.ptrf  = func;
	(*cur)->param.ptrp = param;
	(*cur)->tag        = ss_current_tag;
	(*cur)->restag     = get_resource_tag();
}

void state_save_register_func_presave_ptr(void (*func)(void *), void * param)
{
	ss_register_func_ptr(&ss_prefunc_reg, func, param);
}

void state_save_register_func_postload_ptr(void (*func)(void *), void * param)
{
	ss_register_func_ptr(&ss_postfunc_reg, func, param);
}



/*************************************
 *
 *  Endian conversion functions
 *
 *************************************/

static void ss_c2(UINT8 *data, unsigned size)
{
	UINT16 *convert = (UINT16 *)data;
	unsigned i;

	for (i = 0; i < size; i++)
		convert[i] = FLIPENDIAN_INT16(convert[i]);
}


static void ss_c4(UINT8 *data, unsigned size)
{
	UINT32 *convert = (UINT32 *)data;
	unsigned i;

	for (i = 0; i < size; i++)
		convert[i] = FLIPENDIAN_INT32(convert[i]);
}


static void ss_c8(UINT8 *data, unsigned size)
{
	UINT64 *convert = (UINT64 *)data;
	unsigned i;

	for (i = 0; i < size; i++)
		convert[i] = FLIPENDIAN_INT64(convert[i]);
}



/*************************************
 *
 *  Save state processing
 *
 *************************************/

int state_save_save_begin(mame_file *file)
{
	/* if our stack isn't clear, something is wrong */
	if (ss_tag_stack_index != 0)
		osd_die("State tag stack not reset properly");

	/* if we have illegal registrations, return an error */
	if (ss_illegal_regs > 0)
		return 1;

	TRACE(logerror("Beginning save\n"));
	ss_dump_file = file;

	/* compute the total dump size and the offsets of each element */
	ss_dump_size = compute_size_and_offsets();
	TRACE(logerror("   total size %u\n", ss_dump_size));

	/* allocate memory for the array */
	ss_dump_array = malloc(ss_dump_size);
	if (!ss_dump_array)
		logerror("malloc failed in state_save_save_begin\n");
	return 0;
}


void state_save_save_continue(void)
{
	ss_module *module;
	int count;

	TRACE(logerror("Saving tag %d\n", ss_current_tag));

	/* call the pre-save functions */
	TRACE(logerror("  calling pre-save functions\n"));
	count = call_hook_functions(ss_prefunc_reg);
	TRACE(logerror("    %d functions called\n", count));

	/* then copy in all the data */
	TRACE(logerror("  copying data\n"));

	/* iterate over modules */
	for (module = ss_registry; module; module = module->next)
	{
		int instnum;

		/* iterate over instances */
		for (instnum = 0; instnum < MAX_INSTANCES; instnum++)
		{
			ss_entry *entry;

			/* iterate over entries with matching tags */
			for (entry = module->instances[instnum]; entry; entry = entry->next)
				if (entry->tag == ss_current_tag)
				{
					/* write int entries in consistent 32-bit little-endian order */
					if (entry->type == SS_INT)
					{
						int v = *(int *)(entry->data);
						*(UINT32 *)&ss_dump_array[entry->offset] = LITTLE_ENDIANIZE_INT32(v);
						TRACE(logerror("    %s.%d.%s: %x..%x\n", module->name, instnum, entry->name, entry->offset, entry->offset + 3));
					}

					/* write everything else by memcpy'ing the data */
					else
					{
						memcpy(ss_dump_array + entry->offset, entry->data, ss_size[entry->type] * entry->size);
						TRACE(logerror("    %s.%d.%s: %x..%x\n", module->name, instnum, entry->name, entry->offset, entry->offset + ss_size[entry->type] * entry->size - 1));
					}
				}
		}
	}
}


void state_save_save_finish(void)
{
	UINT32 signature;
	UINT8 flags = 0;

	TRACE(logerror("Finishing save\n"));

	/* compute the flags */
	if (!Machine->sample_rate)
		flags |= SS_NO_SOUND;
#ifndef LSB_FIRST
	flags |= SS_MSB_FIRST;
#endif

	/* build up the header */
	memcpy(ss_dump_array, ss_magic_num, 8);
	ss_dump_array[8] = SAVE_VERSION;
	ss_dump_array[9] = flags;
	memset(ss_dump_array+0xa, 0, 10);
	strcpy((char *)ss_dump_array+0xa, Machine->gamedrv->name);

	/* copy in the signature */
	signature = ss_get_signature();
	*(UINT32 *)&ss_dump_array[0x14] = LITTLE_ENDIANIZE_INT32(signature);

	/* write the file */
	mame_fwrite(ss_dump_file, ss_dump_array, ss_dump_size);

	/* free memory and reset the global states */
	free(ss_dump_array);
	ss_dump_array = NULL;
	ss_dump_size = 0;
	ss_dump_file = NULL;
}



/*************************************
 *
 *  Load state processing
 *
 *************************************/

int state_save_load_begin(mame_file *file)
{
	TRACE(logerror("Beginning load\n"));

	/* if our stack isn't clear, something is wrong */
	if (ss_tag_stack_index != 0)
		osd_die("State tag stack not reset properly");

	/* read the file into memory */
	ss_dump_size = mame_fsize(file);
	ss_dump_array = malloc(ss_dump_size);
	ss_dump_file = file;
	mame_fread(ss_dump_file, ss_dump_array, ss_dump_size);

	/* verify the header and report an error if it doesn't match */
	if (validate_header(ss_dump_array, NULL, ss_get_signature(), ui_popup, "Error: "))
	{
		free(ss_dump_array);
		return 1;
	}

	/* compute the total size and offset of all the entries */
	compute_size_and_offsets();
	return 0;
}


void state_save_load_continue(void)
{
	ss_module *module;
	int need_convert;
	int count;

	/* first determine whether or not we need to convert the endianness of the data */
#ifdef LSB_FIRST
	need_convert = (ss_dump_array[9] & SS_MSB_FIRST) != 0;
#else
	need_convert = (ss_dump_array[9] & SS_MSB_FIRST) == 0;
#endif

	TRACE(logerror("Loading tag %d\n", ss_current_tag));
	TRACE(logerror("  copying data\n"));

	/* iterate over modules */
	for (module = ss_registry; module; module = module->next)
	{
		int instnum;

		/* iterate over instances */
		for (instnum = 0; instnum < MAX_INSTANCES; instnum++)
		{
			ss_entry *entry;

			/* iterate over entries with matching tags */
			for (entry = module->instances[instnum]; entry; entry = entry->next)
				if (entry->tag == ss_current_tag)
				{
					/* read int entries in consistent 32-bit little-endian order */
					if (entry->type == SS_INT)
					{
						UINT32 rawdata = *(UINT32 *)&ss_dump_array[entry->offset];
						*(int *)entry->data = LITTLE_ENDIANIZE_INT32(rawdata);
						TRACE(logerror("    %s.%d.%s: %x..%x\n", module->name, instnum, entry->name, entry->offset, entry->offset+3));
					}

					/* read everything else with a memcpy and convert */
					else
					{
						memcpy(entry->data, ss_dump_array + entry->offset, ss_size[entry->type] * entry->size);
						if (need_convert && ss_conv[entry->type])
							(*ss_conv[entry->type])(entry->data, entry->size);
						TRACE(logerror("    %s.%d.%s: %x..%x\n", module->name, instnum, entry->name, entry->offset, entry->offset+ss_size[entry->type]*entry->size-1));
					}
				}
		}
	}

	/* call the post-load functions */
	TRACE(logerror("  calling post-load functions\n"));
	count = call_hook_functions(ss_postfunc_reg);
	TRACE(logerror("    %d functions called\n", count));
}


void state_save_load_finish(void)
{
	TRACE(logerror("Finishing load\n"));

	/* free memory and reset the global states */
	free(ss_dump_array);
	ss_dump_array = NULL;
	ss_dump_size = 0;
	ss_dump_file = NULL;
}



/*************************************
 *
 *  State file validation
 *
 *************************************/

int state_save_check_file(mame_file *file, const char *gamename, int validate_signature, void (CLIB_DECL *errormsg)(const char *fmt, ...))
{
	UINT32 signature = 0;
	UINT8 header[0x18];

	/* if we want to validate the signature, compute it */
	if (validate_signature)
		signature = ss_get_signature();

	/* seek to the beginning and read the header */
	mame_fseek(file, 0, SEEK_SET);
	if (mame_fread(file, header, sizeof(header)) != sizeof(header))
	{
		if (errormsg)
			errormsg("Could not read " APPNAME " save file header");
		return -1;
	}

	/* let the generic header check work out the rest */
	return validate_header(header, gamename, signature, errormsg, "");
}



/*************************************
 *
 *  Debugging: dump the registry
 *
 *************************************/

void state_save_dump_registry(void)
{
#ifdef VERBOSE
	ss_module *module;

	for (module = ss_registry; module; module = module->next)
	{
		int instnum;

		for (instnum = 0; instnum < MAX_INSTANCES; instnum++)
		{
			ss_entry *entry;

			for (entry = module->instances[instnum]; entry; entry=entry->next)
				logerror("%d %s.%d.%s: %s, %x\n", entry->tag, module->name, instnum, entry->name, ss_type[entry->type], entry->size);
		}
	}
#endif
}

