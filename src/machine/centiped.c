/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

/*
 * This wrapper routine is necessary because Centipede requires a direction bit
 * to be set or cleared. The direction bit is held until the mouse is moved
 * again. Furthermore, the trackball x port should only be updated when the
 * code reaches 0x39b8. We still don't understand why the difference between
 * two consecutive reads must not exceed 7. After all, the input is 4 bits
 * wide, and we have a fifth bit for the sign...
 * JB 971220, BW 980121
 */
int centiped_IN0_r(int offset)
{
	static int counter, sign;
	int delta;

	if (cpu_getpc()==0x39b8)
	{
		delta=readinputport(6);
		if (delta !=0)
		{
			counter=(counter+delta) & 0x0f;
			sign=(delta<8)? 0: 0x80;
		}
	}

	return (readinputport(0) | counter | sign );
}

int centiped_IN2_r(int offset)
{
	static int counter, sign;
	int delta;

	if (cpu_getpc()==0x39b8)
	{
		delta=readinputport(2);
		if (delta !=0)
		{
			counter=(counter+delta) & 0x0f;
			sign=(delta<8)? 0: 0x80;
		}
	}

	return (counter | sign );
}
