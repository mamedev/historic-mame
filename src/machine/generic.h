/*********************************************************************

    generic.h

    Generic simple machine functions.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#pragma once

#ifndef __MACHINE_GENERIC_H__
#define __MACHINE_GENERIC_H__

#include "mamecore.h"



/***************************************************************************
    CONSTANTS
***************************************************************************/

/* total # of coin counters */
#define COIN_COUNTERS		8

/* memory card actions */
#define MEMCARD_CREATE		0
#define MEMCARD_INSERT		1
#define MEMCARD_EJECT		2



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

extern UINT32 dispensed_tickets;
extern UINT32 coin_count[COIN_COUNTERS];
extern UINT32 coinlockedout[COIN_COUNTERS];

extern size_t generic_nvram_size;
extern UINT8 *generic_nvram;
extern UINT16 *generic_nvram16;
extern UINT32 *generic_nvram32;



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/


/* ----- initialization ----- */

/* set up all the common systems */
void generic_machine_init(void);



/* ----- coin counters ----- */

/* write to a particular coin counter (clocks on active high edge) */
void coin_counter_w(int num, int on);

/* enable/disable coin lockout for a particular coin */
void coin_lockout_w(int num, int on);

/* enable/disable global coin lockout */
void coin_lockout_global_w(int on);



/* ----- NVRAM management ----- */

/* load NVRAM from a file */
void nvram_load(void);

/* save NVRAM to a file */
void nvram_save(void);

/* generic NVRAM handler that defaults to a 0 fill */
void nvram_handler_generic_0fill(mame_file *file, int read_or_write);

/* generic NVRAM handler that defaults to a 1 fill */
void nvram_handler_generic_1fill(mame_file *file, int read_or_write);

/* generic NVRAM handler that defaults to a random fill */
void nvram_handler_generic_randfill(mame_file *file, int read_or_write);



/* ----- memory card management ----- */

/* create a new memory card with the given index */
int memcard_create(int index, int overwrite);

/* "insert" a memory card with the given index and load its data */
int memcard_insert(int index);

/* "eject" a memory card and save its data */
void memcard_eject(void);

/* returns the index of the current memory card, or -1 if none */
int memcard_present(void);



/* ----- miscellaneous bits & pieces ----- */

/* set the status of an LED */
void set_led_status(int num, int value);


#endif	/* __MACHINE_GENERIC_H__ */
