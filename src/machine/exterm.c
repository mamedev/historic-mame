#include "driver.h"
#include "cpu/tms34010/tms34010.h"

static int aimpos1, aimpos2;


WRITE_HANDLER( exterm_host_data_w )
{
	tms34010_host_w(1, offset / TOBYTE(0x00100000), data);
}


READ_HANDLER( exterm_host_data_r )
{
	return tms34010_host_r(1, TMS34010_HOST_DATA);
}


READ_HANDLER( exterm_input_port_0_1_r )
{
	int hi = input_port_1_r(offset);
	if (!(hi & 2)) aimpos1++;
	if (!(hi & 1)) aimpos1--;
	aimpos1 &= 0x3f;

	return ((hi & 0x80) << 8) | (aimpos1 << 8) | input_port_0_r(offset);
}

READ_HANDLER( exterm_input_port_2_3_r )
{
	int hi = input_port_3_r(offset);
	if (!(hi & 2)) aimpos2++;
	if (!(hi & 1)) aimpos2--;
	aimpos2 &= 0x3f;

	return (aimpos2 << 8) | input_port_2_r(offset);
}

WRITE_HANDLER( exterm_output_port_0_w )
{
	/* All the outputs are activated on the rising edge */

	static int last = 0;

	/* Bit 0-1= Resets analog controls */
	if ((data & 0x0001) && !(last & 0x0001))
	{
		aimpos1 = 0;
	}

	if ((data & 0x0002) && !(last & 0x0002))
	{
		aimpos2 = 0;
	}

	/* Bit 13 = Resets the slave CPU */
	if ((data & 0x2000) && !(last & 0x2000))
	{
		cpu_set_reset_line(1,PULSE_LINE);
	}

	/* Bits 14-15 = Coin counters */
	coin_counter_w(0, data & 0x8000);
	coin_counter_w(1, data & 0x4000);

	last = data;
}
