/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

unsigned char *docastle_sharedram;
unsigned char *docastle_sharedram2;


int docastle_sharedram2_r(int offset)
{
	return docastle_sharedram2[offset];
}


void docastle_sharedram2_w(int offset,int data)
{
	docastle_sharedram2[offset] = data;
}


int docastle_sharedram_r(int offset)
{
	return docastle_sharedram[offset];
}


void docastle_sharedram_w(int offset,int data)
{
	docastle_sharedram[offset] = data;
}


int docastle_init_machine(const char *gamename)
{
	/* patch the roms to pass the communication test */
/*	RAM[0x0015] = 0x86;
	RAM[0x01e2] = 0xc3;
*/
	return 0;
}
