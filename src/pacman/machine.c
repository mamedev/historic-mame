/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include <stdio.h>
#include <string.h>
#include "mame.h"
#include "driver.h"
#include "osdepend.h"



static int speedcheat = 0;	/* a well known hack allows to make Pac Man run at four times */
					/* his usual speed. When we start the emulation, we check if the */
					/* hack can be applied, and set this flag accordingly. */
static int interrupt_enable;



int pacman_init_machine(const char *gamename)
{
	/* check if the loaded set of ROMs allows the Pac Man speed hack */
	if (RAM[0x180b] == 0xbe && RAM[0x1ffd] == 0x00)
		speedcheat = 1;
	else speedcheat = 0;

	return 0;
}



void pacman_interrupt_enable_w(int offset,int data)
{
	interrupt_enable = data;
}



/***************************************************************************

  Interrupt handler. This function is called at regular intervals
  (determined by IPeriod) by the CPU emulation.

***************************************************************************/

int IntVector = 0xff;	/* Here we store the interrupt vector, if the code performs */
						/* an OUT to port $0. Not all games do it: many use */
						/* Interrupt Mode 1, which doesn't use an interrupt vector */
						/* (see Z80.c for details). */

int pacman_interrupt(void)
{
	/* speed up cheat */
	if (speedcheat)
	{
		if (osd_key_pressed(OSD_KEY_CONTROL) || osd_joy_pressed(OSD_JOY_FIRE))
		{
			RAM[0x180b] = 0x01;
			RAM[0x1ffd] = 0xbd;
		}
		else
		{
			RAM[0x180b] = 0xbe;
			RAM[0x1ffd] = 0x00;
		}
	}

	if (interrupt_enable == 0) return Z80_IGNORE_INT;
	else return IntVector;
}



/***************************************************************************

  The Pac Man machine uses OUT to port $0 to set the interrupt vector, so
  we have to remember the value passed.

***************************************************************************/
void pacman_out(byte Port,byte Value)
{
	/* OUT to port $0 is used to set the interrupt vector */
	if (Port == 0) IntVector = Value;
}
