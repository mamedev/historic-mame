/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
//#include "Z80.h"


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
		R.IFF2 = 1;	/* enable interrupts */
		Z80_SetRegs(&R);

		return 0x00d7;	/* RST 10h - 8080's NMI */
	}
}

/****************************************************************************
	Extra / Different functions for Boot Hill                (MJC 300198)
****************************************************************************/

int boothill_shift_data_r(int offset)
{
	if (shift_amount < 0x10)
		return invaders_shift_data_r(0);
    else
    {
    	int reverse_data1,reverse_data2;

    	/* Reverse the bytes */

        reverse_data1 = ((shift_data1 & 0x01) << 7)
                      | ((shift_data1 & 0x02) << 5)
                      | ((shift_data1 & 0x04) << 3)
                      | ((shift_data1 & 0x08) << 1)
                      | ((shift_data1 & 0x10) >> 1)
                      | ((shift_data1 & 0x20) >> 3)
                      | ((shift_data1 & 0x40) >> 5)
                      | ((shift_data1 & 0x80) >> 7);

        reverse_data2 = ((shift_data2 & 0x01) << 7)
                      | ((shift_data2 & 0x02) << 5)
                      | ((shift_data2 & 0x04) << 3)
                      | ((shift_data2 & 0x08) << 1)
                      | ((shift_data2 & 0x10) >> 1)
                      | ((shift_data2 & 0x20) >> 3)
                      | ((shift_data2 & 0x40) >> 5)
                      | ((shift_data2 & 0x80) >> 7);

		return ((((reverse_data2 << 8) | reverse_data1) << (0xff-shift_amount)) >> 8) & 0xff;
    }
}

/* Grays Binary again! */

static const int BootHillTable[8] = {
	0x00, 0x40, 0x60, 0x70, 0x30, 0x10, 0x50, 0x50
};

int boothill_port_0_r(int offset)
{
    return (input_port_0_r(0) & 0x8F) | BootHillTable[input_port_3_r(0) >> 5];
}

int boothill_port_1_r(int offset)
{
    return (input_port_1_r(0) & 0x8F) | BootHillTable[input_port_4_r(0) >> 5];
}
