/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "Z80.h"


static int shift_data1,shift_data2,shift_amount;



int invaders_shift_data_r(int offset)
{
	return ((((shift_data1 << 8) | shift_data2) << shift_amount) >> 8) & 0xff;
}



void invaders_shift_amount_w(int offset,int data)
{
	shift_amount = data;
}



void invaders_shift_data_w(int offset,int data)
{
	shift_data2 = shift_data1;
	shift_data1 = data;
}



int invaders_interrupt(void)
{
	static int count;


	count++;

	if (count & 1) return 0x00cf;	/* RST 08h - 8080's IRQ */
	else
	{
		Z80_Regs R;


		Z80_GetRegs(&R);
		R.IFF1 = 1;	/* enable interrupts */
		Z80_SetRegs(&R);

		return 0x00d7;	/* RST 10h - 8080's NMI */
	}
}
