/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "m6809/m6809.h"

unsigned char *zoo_sharedram;

/* Because all the scanlines are drawn at once, this has no real
   meaning. But the game sometimes waits for the scanline to be 
   a particular value, so we fake it.
*/
#if 0
int zookeeper_scanline_r(int offset)
{
	static unsigned char hack = 0x00;
	return hack++;
}
#else
/* JB 970928 */
int zookeeper_scanline_r(int offset)
{
	static unsigned char hack = 0;
	if (hack)
		hack = 0;
	else
		hack = 0xf7;
	return hack;
}
#endif


void zookeeper_bankswitch_w(int offset,int data)
{
	if (data & 0x04)
		cpu_setbank (1, &RAM[0x10000])
	else
		cpu_setbank (1, &RAM[0xa000]);
}


void zookeeper_init_machine(void)
{
	/* Set OPTIMIZATION FLAGS FOR M6809 */
	m6809_Flags = M6809_FAST_NONE;
}


int zoo_sharedram_r(int offset)
{
	return zoo_sharedram[offset];
}


void zoo_sharedram_w(int offset,int data)
{
	zoo_sharedram[offset] = data;
}

