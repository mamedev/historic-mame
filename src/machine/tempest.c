/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"

#define IN0_VBLANK (1<<7)


static int vblank;



int tempest_IN0_r(int offset)
{
	int res;


	res = readinputport(0);

#ifdef 0
	if (cpu_geticount() > (Machine->drv->cpu[0].cpu_clock / Machine->drv->frames_per_second) / 12) 
#endif

	if (vblank)
		res &= ~IN0_VBLANK;
	else vblank = 0;
	
	return res;
}
	

int tempest_IN3_r(int offset)
{
	static int spinner = 0;
	int res, trak_in;

	res = readinputport(3);
	if (res & 1) 
		spinner--;
	if (res & 2)
		spinner++;
	trak_in = readtrakport(0);
	spinner += trak_in;
	spinner &= 0x0f;	
	return ((res & 0xf0) | spinner);
}

int tempest_spinner(int data)
{
	if (data>7)
		data=7;
	if (data<-7)
		data=-7;
	return (data);
}	


int tempest_interrupt(void)
{
	vblank = 1;

	return interrupt();
}
