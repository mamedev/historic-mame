/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80.h"



unsigned char *docastle_intkludge1,*docastle_intkludge2;

static unsigned char buffer0[9],buffer1[9];



int docastle_shared0_r(int offset)
{
	if (offset == 8)
	{
		if (errorlog) fprintf(errorlog,"shared0r\n");

		/* this shouldn't be done, however it's the only way I've found */
		/* to make dip switches work in most games. They still DON'T work */
		/* in Do's Castle, though. */
		cpu_cause_interrupt(1,Z80_NMI_INT);
		cpu_seticount(0);
	}
	return buffer0[offset];
}


int docastle_shared1_r(int offset)
{
if (errorlog && offset == 8) fprintf(errorlog,"shared1r\n");
	return buffer1[offset];
}


void docastle_shared0_w(int offset,int data)
{
if (errorlog && offset == 8) fprintf(errorlog,"shared0w %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		buffer0[0],buffer0[1],buffer0[2],buffer0[3],buffer0[4],buffer0[5],buffer0[6],buffer0[7],data);

	buffer0[offset] = data;
}


void docastle_shared1_w(int offset,int data)
{
	buffer1[offset] = data;

	if (offset == 8)
	{
		if (errorlog) fprintf(errorlog,"shared1w %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
				buffer1[0],buffer1[1],buffer1[2],buffer1[3],buffer1[4],buffer1[5],buffer1[6],buffer1[7],data);

		cpu_cause_interrupt(1,Z80_NMI_INT);
		cpu_seticount(0);	/* we must immediately run the second CPU */
	}
}



void docastle_nmitrigger(int offset,int data)
{
	/* we should cause a NMI interrupt on the second CPU here; however, to */
	/* make things tick the way they are supposed to be (due to the way the */
	/* hardware works) we trigger it in docastle_shared1_w(), when the */
	/* first CPU has finised writing to the shared area. */
//	cpu_cause_interrupt(1,Z80_NMI_INT);
}
