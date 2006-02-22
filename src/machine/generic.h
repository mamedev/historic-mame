/*********************************************************************

    generic.h

    Generic simple machine functions.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#pragma once

#ifndef __MACHINE_GENERIC_H__
#define __MACHINE_GENERIC_H__

#include "romload.h"
#include "xmlfile.h"



/***************************************************************************

    Global variables

***************************************************************************/

#define COIN_COUNTERS	8	/* total # of coin counters */

extern UINT32 dispensed_tickets;
extern UINT32 coin_count[COIN_COUNTERS];
extern UINT32 coinlockedout[COIN_COUNTERS];



/***************************************************************************

    Function prototypes

***************************************************************************/

void generic_machine_init(void);

/* common LED helpers */
void set_led_status(int num, int value);

/* common coin counter helpers */
void coin_counter_w(int num,int on);
void coin_lockout_w(int num,int on);
void coin_lockout_global_w(int on);  /* Locks out all coin inputs */

/* generic NVRAM handler */
extern size_t generic_nvram_size;
extern UINT8 *generic_nvram;
extern UINT16 *generic_nvram16;
extern UINT32 *generic_nvram32;
void nvram_load(void);
void nvram_save(void);
void nvram_handler_generic_0fill(mame_file *file, int read_or_write);
void nvram_handler_generic_1fill(mame_file *file, int read_or_write);
void nvram_handler_generic_randfill(mame_file *file, int read_or_write);


#endif	/* __MACHINE_GENERIC_H__ */
