#ifndef MACHINE_H
#define MACHINE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "osdepend.h"
#include "common.h"


extern FILE *errorlog;

#define MAX_GFX_ELEMENTS 10
#define MAX_MEMORY_REGIONS 10

struct RunningMachine
{
	unsigned char *memory_region[MAX_MEMORY_REGIONS];
	struct osd_bitmap *scrbitmap;	/* bitmap to draw into */
	struct GfxElement *gfx[MAX_GFX_ELEMENTS];	/* graphic sets (chars, sprites) */
								/* the first one is used by DisplayText() */
	int background_pen;	/* pen to use to clear the bitmap (DON'T use 0) */
	const struct MachineDriver *drv;	/* contains the definition of the machine */
};


extern struct RunningMachine *Machine;
extern unsigned char *RAM;	/* pointer to the memory region of the active CPU */


int init_machine(const char *gamename,int argc,char **argv);
int run_machine(const char *gamename);
int updatescreen(void);

#endif
