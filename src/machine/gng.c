/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "M6809/M6809.h"



void gng_bankswitch_w(int offset,int data)
{
/* ASG 091197 -- old code looked like this:
	bankaddress = 0x10000 + data * 0x2000;*/

	static int bank[] = { 0x10000, 0x12000, 0x14000, 0x16000, 0x04000, 0x18000 };
	cpu_setbank (1, &RAM[bank[data]]);
}



void gng_init_machine(void)
{
	/* Set optimization flags for M6809 */
	m6809_Flags = M6809_FAST_NONE;
}
