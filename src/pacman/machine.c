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


#define IN0_CREDIT (1<<7)
#define IN0_RACK_TEST (1<<4)
#define IN0_DOWN (1<<3)
#define IN0_RIGHT (1<<2)
#define IN0_LEFT (1<<1)
#define IN0_UP (1<<0)
#define IN1_START2 (1<<6)
#define IN1_START1 (1<<5)
#define IN1_TEST (1<<4)


int pacman_init_machine(const char *gamename)
{
	/* check if the loaded set of ROMs allows the Pac Man speed hack */
	if (RAM[0x180b] == 0xbe && RAM[0x1ffd] == 0x00)
		speedcheat = 1;
	else speedcheat = 0;

	return 0;
}



int pacman_IN0_r(int address,int offset)
{
	int res = 0xff;


	if (offset != 0 && errorlog)
		fprintf(errorlog,"%04x: warning - read input port %04x from mirror address %04x\n",Z80_GetPC(),address-offset,address);

	if (osd_key_pressed(OSD_KEY_3)) res &= ~IN0_CREDIT;
	if (osd_key_pressed(OSD_KEY_F1)) res &= ~IN0_RACK_TEST;
	if (osd_key_pressed(OSD_KEY_DOWN) || osd_joy_down) res &= ~IN0_DOWN;
	if (osd_key_pressed(OSD_KEY_RIGHT) || osd_joy_right) res &= ~IN0_RIGHT;
	if (osd_key_pressed(OSD_KEY_LEFT) || osd_joy_left) res &= ~IN0_LEFT;
	if (osd_key_pressed(OSD_KEY_UP) || osd_joy_up) res &= ~IN0_UP;

	/* speed up cheat */
	if (speedcheat)
	{
		if (osd_key_pressed(OSD_KEY_CONTROL) || osd_joy_b1 || osd_joy_b2)
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

	return res;
}



int pacman_IN1_r(int address,int offset)
{
	int res = 0xff;


	if (offset != 0 && errorlog)
		fprintf(errorlog,"%04x: warning - read input port %04x from mirror address %04x\n",Z80_GetPC(),address-offset,address);

	if (osd_key_pressed(OSD_KEY_2)) res &= ~IN1_START2;
	if (osd_key_pressed(OSD_KEY_1)) res &= ~IN1_START1;
	if (osd_key_pressed(OSD_KEY_F2)) res &= ~IN1_TEST;
	return res;
}



int pacman_DSW1_r(int address,int offset)
{
	if (offset != 0 && errorlog)
		fprintf(errorlog,"%04x: warning - read input port %04x from mirror address %04x\n",Z80_GetPC(),address-offset,address);

	return Machine->dsw[0];
}



void pacman_interrupt_enable_w(int address,int offset,int data)
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
