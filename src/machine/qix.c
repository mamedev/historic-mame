/***************************************************************************

  machine\qix.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "M6809.h"



unsigned char *qix_sharedram;



int qix_sharedram_r_1(int offset)
{
	return qix_sharedram[offset];
}


int qix_sharedram_r_2(int offset)
{
	return qix_sharedram[offset];
}


void qix_sharedram_w(int offset,int data)
{
	qix_sharedram[offset] = data;
}



void qix_video_firq_w(int offset, int data)
{
	/* generate firq for video cpu */
	cpu_cause_interrupt(1,M6809_INT_FIRQ);
}



void qix_data_firq_w(int offset, int data)
{
	/* generate firq for data cpu */
	cpu_cause_interrupt(0,M6809_INT_FIRQ);
}



/* Return the current video scan line */
int qix_scanline_r(int offset)
{
	return 255 - (cpu_getfcount() * 256 / cpu_getfperiod());
}



void qix_init_machine(void)
{
	/* Set OPTIMIZATION FLAGS FOR M6809 */
	m6809_Flags = M6809_FAST_S;/* | M6809_FAST_U;*/
}
