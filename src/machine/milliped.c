/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


/* JB 971220 */
/* This wrapper routine is necessary because Millipede requires a direction bit
   to be set or cleared. We have to look at the change in the input port to
   determine whether to set or clear the bit. Why doesn't Millipede do this
   itself instead of requiring a direction bit?

   The other reason it is necessary is that Millipede uses the same address to
   read the dipswitches. */
int milliped_IN0_r (int offset)
{
	int res, delta;
	static int last = 0;
	static int lastdelta = 0;

	/* Return dipswitch when 4000 cycles remain before interrupt. */
	if (cpu_geticount () < 4000)
		return readinputport (0);

	/* Read the fake input port to get the trackball input. */
	res = readinputport (6);

	/* Determine the change since last time. */
	delta = (res & 0x0f) - last;
	last = (res & 0x0f);

	/* Skanky hack to determine correct direction bit,
	   necessary because input port is updated half as often as Millipede
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

	/* Millipede expects a direction bit. */
	if (delta<0)
		res |= 0x80;

	return res;
}


/* JB 971220 */
/* This wrapper routine is necessary because Millipede requires a direction bit
   to be set or cleared. We have to look at the change in the input port to
   determine whether to set or clear the bit. Why doesn't Millipede do this
   itself instead of requiring a direction bit?

   The other reason it is necessary is that Millipede uses the same address to
   read the dipswitches. */
int milliped_IN1_r (int offset)
{
	int res, delta;
	static int last = 0;
	static int lastdelta = 0;

	/* Return dipswitch when 4000 cycles remain before interrupt. */
	if (cpu_geticount () < 4000)
		return readinputport (1);

	/* Read the fake input port to get the trackball input. */
	res = readinputport (7);

	/* Determine the change since last time. */
	delta = (res & 0x0f) - last;
	last = (res & 0x0f);

	/* Skanky hack to determine correct direction bit,
	   necessary because input port is updated half as often as Millipede
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

	/* Millipede expects a direction bit. */
	if (delta<0)
		res |= 0x80;

	return res;
}
