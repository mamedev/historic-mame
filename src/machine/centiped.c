/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"

/* JB 971220 */
/* This wrapper routine is necessary because Centipede requires a direction bit
   to be set or cleared. We have to look at the change in the input port to
   determine whether to set or clear the bit. Why doesn't Centipede do this
   itself instead of requiring a direction bit? */
int centiped_IN0_r(int offset)
{
	int res, delta;
	static int last = 0;
	static int lastdelta = 0;

	res = readinputport(0);

	/* Determine the change since last time. */
	delta = (res & 0x0f) - last;
	last = (res & 0x0f);

	/* Skanky hack to determine correct direction bit,
	   necessary because input port is updated half as often as Centipede
	   reads it, which causes every second delta to be 0. */
	if (delta==0)
		delta = lastdelta;
	else
		lastdelta = delta;

	/* Since we know input delta is clipped to 7, if delta is greater than that, it
	   must have wrapped past 0, so adjust. */
	if (delta<-7)
		delta += 16;
	else
	if (delta>7)
		delta -= 16;

	/* Centipede expects a direction bit. */
	if (delta<0)
		res |= 0x80;

	return res;
}

/* JB 971220 */
/* This wrapper routine is necessary because Centipede requires a direction bit
   to be set or cleared. We have to look at the change in the input port to
   determine whether to set or clear the bit. Why doesn't Centipede do this
   itself instead of requiring a direction bit? */
int centiped_IN2_r(int offset)
{
	int res, delta;
	static int last = 0;
	static int lastdelta = 0;

	res = readinputport(2);

	/* Determine the change since last time. */
	delta = res - last;
	last = res;

	/* Skanky hack to determine correct direction bit,
	   necessary because input port is updated half as often as Centipede
	   reads it, which causes every second delta to be 0. */
	if (delta==0)
		delta = lastdelta;
	else
		lastdelta = delta;

	/* Since we know input delta is clipped to 7, if delta is greater than that, it
	   must have wrapped past 0, so adjust. */
	if (delta<-7)
		delta += 16;
	else
	if (delta>7)
		delta -= 16;

	/* Centipede expects a direction bit. */
	if (delta<0)
		res |= 0x80;

	return res;
}
