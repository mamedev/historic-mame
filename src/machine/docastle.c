/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80.h"



unsigned char *docastle_intkludge1,*docastle_intkludge2;

static unsigned char buffer0[9],buffer1[9];
static int nmi;



int docastle_intkludge1_r(int offset)
{

	/* to speed up the emulation, detect when the program is looping waiting */
	/* for an interrupt, and force it in that case */
	if (cpu_getpc() == 0x09ba)
		cpu_seticount(0);

	return *docastle_intkludge1;
}



int docastle_intkludge2_r(int offset)
{
	/* to speed up the emulation, detect when the program is looping waiting */
	/* for an interrupt, and force it in that case */
	if (cpu_getpc() == 0x0218)
		cpu_seticount(0);

	return *docastle_intkludge2;
}



int docastle_shared0_r(int offset)
{
if (errorlog && offset == 8) fprintf(errorlog,"shared0r\n");
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
if (errorlog && offset == 8) fprintf(errorlog,"shared1w %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		buffer1[0],buffer1[1],buffer1[2],buffer1[3],buffer1[4],buffer1[5],buffer1[6],buffer1[7],data);

	buffer1[offset] = data;

	/* To prevent a "bad communication" error with Mr. Do's Wild Ride, prepare a valid */
	/* response. This hack is necessary because MAME currently doesn't interleave the */
	/* execution of the two CPUs, so the first CPU tries to read the reply from the second */
	/* before the second has had a chance to run. */
	if (offset == 8 && memcmp(buffer1,"\x04\x00\x00\x00\x00\x00\x00\x00\x04",9) == 0)
		memcpy(buffer0,"\x20\x11\x11\x00\x00\x00\x00\x00\x42",9);

	/* Horrible kludge to make Do's Castle read the dip switch settings during boot. */
	/* Again, this is necessary because of the limited control MAME has on multiple CPUs. */
	if (offset == 8 && memcmp(buffer1,"\x44\x00\x00\x00\x00\x00\x00\x00\x44",9) == 0)
		cpu_seticount(3000);
}



void docastle_nmitrigger(int offset,int data)
{
	nmi++;
}



int docastle_interrupt2(void)
{
	static int count;


//fprintf(errorlog,"interrupt %04x\n",cpu_getpc());
	count++;
	if (count & 1)
	{
		if (nmi)
		{
			nmi--;
			return Z80_NMI_INT;
		}
		else return Z80_IGNORE_INT;
	}
	else return 0xff;
}
