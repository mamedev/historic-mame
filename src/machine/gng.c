/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "M6809/M6809.h"



void gng_bankswitch_w(int offset,int data)
{
/* ASG 091197 -- old code looked like this:
	bankaddress = 0x10000 + data * 0x2000;*/

	static int bank[] = { 0x10000, 0x12000, 0x14000, 0x16000, 0x04000, 0x18000 };
	cpu_setbank (1, &RAM[bank[data]]);
}



/* JB 970823 - speed up busy loop
	617F: LDX $12    ; dp=0
	6181: LDD ,X     ; x={ $0128,012a,? }
	6183: ASLA
	6184: BCC $6160
	6186: BRA $617F
*/
#if 1
int gng_catch_loop_r(int offset)
{
	m6809_Regs	r;

	/* check to see if the branch will be taken */
	m6809_GetRegs (&r);
	if (r.cc & 0x01) cpu_seticount (0);
	return ROM[0x6184];
}
#else
/* this method works (hook on $0012), but is no faster */
int gng_catch_loop_r(int offset)
{
	unsigned char t,t2;

	t = RAM[0x0012];
	if (cpu_getpc()==0x6181)
	{
		t2 = RAM[ (t<<8) | RAM[0x0013] ];
		if (t2 & 0x80) cpu_seticount (0);
	}

	return t;
}
#endif

void gng_init_machine(void)
{
	/* Set optimization flags for M6809 */
	m6809_Flags = M6809_FAST_NONE;
}
