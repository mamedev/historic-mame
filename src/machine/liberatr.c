/***************************************************************************

  liberator.c - 'machine.c'

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


UINT8 *liberatr_ctrld;


WRITE_HANDLER( liberatr_led_w )
{
	osd_led_w(offset, (data >> 4) & 0x01);
}


WRITE_HANDLER( liberatr_coin_counter_w )
{
	coin_counter_w(offset ^ 0x01, data);
}


READ_HANDLER( liberatr_input_port_0_r )
{
	int	res ;
	int xdelta, ydelta;


	/* CTRLD selects whether we're reading the stick or the coins,
	   see memory map */

	if(*liberatr_ctrld)
	{
		/* 	mouse support */
		xdelta = input_port_4_r(0);
		ydelta = input_port_5_r(0);
		res = ( ((ydelta << 4) & 0xf0)  |  (xdelta & 0x0f) );
	}
	else
	{
		res = input_port_0_r(offset);
	}

	return res;
}
