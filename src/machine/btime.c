/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "M6502.h"


#define IN1_VBLANK (1<<7)


static int vblank;


int btime_DSW1_r(int offset)
{
	int res,pc;


	/* to speed up the emulation, detect when the program is looping waiting */
	/* for a vblank, and force an interrupt in that case */
	pc = cpu_getpc();
	if (RAM[pc+1] == 0xfb)
	{
		if (RAM[pc] == 0x10)	/* BPL */
		{
			if (vblank == 0)
			{
				cpu_seticount(0);
				return 0;
			}

			return IN1_VBLANK;
		}
		else if (RAM[pc] == 0x30) /* BMI */
		{
			if (vblank)
			{
				cpu_seticount(0);
				return IN1_VBLANK;
			}

			return 0;
		}
	}

	res = readinputport(3);

	if (vblank) res |= IN1_VBLANK;

	return res;
}



/***************************************************************************

  Burger Time doesn't have VBlank interrupts; the software polls port IN0 to
  know when a vblank is happening. Therefore we set a flag here, to let
  btime_DSW1_r() know when it has to report a vblank.

***************************************************************************/
int btime_interrupt(void)
{
	static int count;


	/* let btime_DSW1_r() know when it is time to report a vblank */
	/* I'm not yet sure about how the vertical blanking should be handled. */
	/* I think that IN1_VBLANK should be 1 during the whole vblank, which */
	/* should last roughly 1/12th of the frame. */
	count = (count + 1) % 12;
	if (count == 0) vblank = 1;
	else vblank = 0;

	/* IRQs are used to check coin insertion and diagnostic commands */
	return INT_IRQ;
}
