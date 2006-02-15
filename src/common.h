/*********************************************************************

    common.h

    Generic functions, mostly resource and memory related.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

*********************************************************************/

#pragma once

#ifndef __COMMON_H__
#define __COMMON_H__

#include "romload.h"
#include "xmlfile.h"



/***************************************************************************

    Constants and macros

***************************************************************************/

enum
{
	REGION_INVALID = 0x80,
	REGION_CPU1,
	REGION_CPU2,
	REGION_CPU3,
	REGION_CPU4,
	REGION_CPU5,
	REGION_CPU6,
	REGION_CPU7,
	REGION_CPU8,
	REGION_GFX1,
	REGION_GFX2,
	REGION_GFX3,
	REGION_GFX4,
	REGION_GFX5,
	REGION_GFX6,
	REGION_GFX7,
	REGION_GFX8,
	REGION_PROMS,
	REGION_SOUND1,
	REGION_SOUND2,
	REGION_SOUND3,
	REGION_SOUND4,
	REGION_SOUND5,
	REGION_SOUND6,
	REGION_SOUND7,
	REGION_SOUND8,
	REGION_USER1,
	REGION_USER2,
	REGION_USER3,
	REGION_USER4,
	REGION_USER5,
	REGION_USER6,
	REGION_USER7,
	REGION_USER8,
	REGION_DISKS,
	REGION_MAX
};



/***************************************************************************

    Global variables

***************************************************************************/

#define COIN_COUNTERS	8	/* total # of coin counters */

extern unsigned int dispensed_tickets;
extern unsigned int coin_count[COIN_COUNTERS];
extern unsigned int coinlockedout[COIN_COUNTERS];



/***************************************************************************

    Function prototypes

***************************************************************************/

void showdisclaimer(void);

/* return a pointer to the specified memory region - num can be either an absolute */
/* number, or one of the REGION_XXX identifiers defined above */
UINT8 *memory_region(int num);
size_t memory_region_length(int num);

/* allocate a new memory region - num can be either an absolute */
/* number, or one of the REGION_XXX identifiers defined above */
int new_memory_region(int num, size_t length, UINT32 flags);
void free_disposable_memory_regions(void);
void free_memory_region(int num);

/* common coin counter helpers */
void counters_load(int config_type, xml_data_node *parentnode);
void counters_save(int config_type, xml_data_node *parentnode);
void coin_counter_reset(void);
void coin_counter_w(int num,int on);
void coin_lockout_w(int num,int on);
void coin_lockout_global_w(int on);  /* Locks out all coin inputs */

/* generic NVRAM handler */
extern size_t generic_nvram_size;
extern UINT8 *generic_nvram;
extern UINT16 *generic_nvram16;
extern UINT32 *generic_nvram32;
void nvram_handler_generic_0fill(mame_file *file, int read_or_write);
void nvram_handler_generic_1fill(mame_file *file, int read_or_write);
void nvram_handler_generic_randfill(mame_file *file, int read_or_write);

/* bitmap allocation */
mame_bitmap *bitmap_alloc(int width,int height);
mame_bitmap *bitmap_alloc_depth(int width,int height,int depth);
void bitmap_free(mame_bitmap *bitmap);

/* automatic resource management */
void begin_resource_tracking(void);
void end_resource_tracking(void);
INLINE int get_resource_tag(void)
{
	extern int resource_tracking_tag;
	return resource_tracking_tag;
}

/* automatically-freeing memory */
void *auto_malloc(size_t size) ATTR_MALLOC;
char *auto_strdup(const char *str) ATTR_MALLOC;
mame_bitmap *auto_bitmap_alloc(int width,int height);
mame_bitmap *auto_bitmap_alloc_depth(int width,int height,int depth);

/*
  Save a screen shot of the game display. It is suggested to use the core
  function save_screen_snapshot() or save_screen_snapshot_as(), so the format
  of the screen shots will be consistent across ports. This hook is provided
  only to allow the display of a file requester to let the user choose the
  file name. This isn't scrictly necessary, so you can just call
  save_screen_snapshot() to let the core automatically pick a default name.
*/
void save_screen_snapshot_as(mame_file *fp, mame_bitmap *bitmap);
void save_screen_snapshot(mame_bitmap *bitmap);

/* Movie recording */
void record_movie_toggle(void);
void record_movie_stop(void);
void record_movie_frame(mame_bitmap *bitmap);


#endif	/* __COMMON_H__ */
