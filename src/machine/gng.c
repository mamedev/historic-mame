/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"



static int bankaddress;


int gng_init_machine(const char *gamename)
{

        if (stricmp(gamename, "gng") == 0) {
         RAM[0x607a] = 0x12;    /* This patch skip the RAM/ROM test */
         RAM[0x607b] = 0x12;
         RAM[0x607c] = 0x12;
        }

        return 0;
}

void gng_bankswitch_w(int offset,int data)
{
        bankaddress = 0x10000 + data * 0x2000;
}



int gng_bankedrom_r(int offset)
{
	return RAM[bankaddress + offset];
}
