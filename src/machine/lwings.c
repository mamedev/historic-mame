/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

void lwings_bankswitch_w(int offset,int data)
{
        int bankaddress;
        static int nOldData=0xff;
        if (data != nOldData)
        {
                bankaddress = 0x10000 + (data & 0x06) * 0x1000 * 2;
                cpu_setbank(1,&ROM[bankaddress]);
                nOldData=data;
        }
}

int lwings_interrupt(void)
{
	static int count;

	count = (count + 1) % 2;

	if (count) return 0x00cf;	/* RST 08h */
	else return 0x00d7;	/* RST 10h */
}
