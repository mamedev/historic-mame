#ifndef MACHINE_H
#define MACHINE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "osdepend.h"
#include "common.h"


extern FILE *errorlog;

#define MAX_GFX_ELEMENTS 10

struct RunningMachine
{
	struct osd_bitmap *scrbitmap;	/* bitmap to draw into */
	struct GfxElement *gfx[MAX_GFX_ELEMENTS];	/* graphic sets (chars, sprites) */
								/* the first one is used by DisplayText() */
	int background_pen;	/* pen to use to clear the bitmap (DON'T use 0) */
	const struct MachineDriver *drv;	/* contains the definition of the machine */
};


extern struct RunningMachine *Machine;


int init_machine(const char *gamename,int argc,char **argv);
int run_machine(const char *gamename);

/* some useful general purpose functions for the memory map */
int readinputport(int port);
int input_port_0_r(int offset);
int input_port_1_r(int offset);
int input_port_2_r(int offset);
int input_port_3_r(int offset);
int input_port_4_r(int offset);
int input_port_5_r(int offset);
void interrupt_enable_w(int offset,int data);
void interrupt_vector_w(int offset,int data);
int interrupt(void);
int nmi_interrupt(void);


#endif
