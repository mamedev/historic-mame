/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

/*
 * This wrapper routine is necessary because Centipede requires a direction bit
 * to be set or cleared. The direction bit is held until the mouse is moved
 * again.
 */
int centiped_IN0_r(int offset)
{
	static int counter, sign;
	int delta;

	delta=readinputport(6);
	if (delta !=0)
	{
		counter=(counter+delta) & 0x0f;
		sign = delta & 0x80;
	}

	return (readinputport(0) | counter | sign );
}

int centiped_IN2_r(int offset)
{
	static int counter, sign;
	int delta;

	delta=readinputport(2);
	if (delta !=0)
	{
		counter=(counter+delta) & 0x0f;
		sign = delta & 0x80;
	}

	return (counter | sign );
}
