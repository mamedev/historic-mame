/***************************************************************************

    state.h

    Save state management functions.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __STATE_H__
#define __STATE_H__

#include "osd_cpu.h"
#include "fileio.h"

/* Initializes the save state registrations */
void state_save_free(void);
void state_save_allow_registration(int allowed);
int state_save_registration_allowed(void);

/* Registering functions */
int state_save_get_reg_count(void);

void state_save_register_UINT8 (const char *module, int instance,
								const char *name, UINT8 *val, unsigned size);
void state_save_register_INT8  (const char *module, int instance,
								const char *name, INT8 *val, unsigned size);
void state_save_register_UINT16(const char *module, int instance,
								const char *name, UINT16 *val, unsigned size);
void state_save_register_INT16 (const char *module, int instance,
								const char *name, INT16 *val, unsigned size);
void state_save_register_UINT32(const char *module, int instance,
								const char *name, UINT32 *val, unsigned size);
void state_save_register_INT32 (const char *module, int instance,
								const char *name, INT32 *val, unsigned size);
void state_save_register_UINT64(const char *module, int instance,
								const char *name, UINT64 *val, unsigned size);
void state_save_register_INT64 (const char *module, int instance,
								const char *name, INT64 *val, unsigned size);
void state_save_register_double(const char *module, int instance,
								const char *name, double *val, unsigned size);
void state_save_register_float (const char *module, int instance,
								const char *name, float *val, unsigned size);
void state_save_register_int   (const char *module, int instance,
								const char *name, int *val);
void state_save_register_bitmap(const char *module, int instance,
								const char *name, mame_bitmap *val);

void state_save_register_func_presave(void (*func)(void));
void state_save_register_func_postload(void (*func)(void));

void state_save_register_func_presave_int(void (*func)(int), int param);
void state_save_register_func_postload_int(void (*func)(int), int param);

void state_save_register_func_presave_ptr(void (*func)(void *), void *param);
void state_save_register_func_postload_ptr(void (*func)(void *), void *param);

/* Save and load functions */
/* The tags are a hack around the current cpu structures */
int  state_save_save_begin(mame_file *file);
int  state_save_load_begin(mame_file *file);

void state_save_push_tag(int tag);
void state_save_pop_tag(void);

void state_save_save_continue(void);
void state_save_load_continue(void);

void state_save_save_finish(void);
void state_save_load_finish(void);

/* Display function */
void state_save_dump_registry(void);

/* Verification function; can be called from front ends */
int state_save_check_file(mame_file *file, const char *gamename, int validate_signature, void (CLIB_DECL *errormsg)(const char *fmt, ...));

/* Macros */
#define state_save_register_generic(_mod, _inst, _name, _val, _valsize, _count) 	\
	if (sizeof(_valsize) == 1)														\
		state_save_register_UINT8(_mod, _inst, _name, (UINT8 *)(_val), _count);		\
	else if (sizeof(_valsize) == 2)													\
		state_save_register_UINT16(_mod, _inst, _name, (UINT16 *)(_val), _count);	\
	else if (sizeof(_valsize) == 4)													\
		state_save_register_UINT32(_mod, _inst, _name, (UINT32 *)(_val), _count);	\
	else if (sizeof(_valsize) == 8)													\
		state_save_register_UINT64(_mod, _inst, _name, (UINT64 *)(_val), _count);

#define state_save_register_item(_mod, _inst, _val)	\
	state_save_register_generic(_mod, _inst, #_val, &_val, _val, 1);

#define state_save_register_item_array(_mod, _inst, _val) \
	state_save_register_generic(_mod, _inst, #_val, &_val[0], _val[0], sizeof(_val)/sizeof(_val[0]))

#define state_save_register_item_2d_array(_mod, _inst, _val) \
	state_save_register_generic(_mod, _inst, #_val, &_val[0][0], _val[0][0], sizeof(_val)/sizeof(_val[0][0]))

#define state_save_register_item_pointer(_mod, _inst, _val, _count) \
	state_save_register_generic(_mod, _inst, #_val, &_val[0], _val[0], _count)

#define state_save_register_global(_val) \
	state_save_register_item("globals", 0, _val)

#define state_save_register_global_array(_val) \
	state_save_register_item_array("globals", 0, _val)

#define state_save_register_global_2d_array(_val) \
	state_save_register_item_2d_array("globals", 0, _val)

#define state_save_register_global_pointer(_val, _count) \
	state_save_register_item_pointer("globals", 0, _val, _count)

#define state_save_register_global_bitmap(_val) \
	state_save_register_bitmap("globals", 0, #_val, _val)

#endif	/* __STATE_H__ */
