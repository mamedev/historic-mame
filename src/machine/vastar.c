/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"



unsigned char *vastar_sharedram;



void vastar_init_machine(void)
{
	/* we must start with the second CPU halted */
	cpu_set_reset_line(1,ASSERT_LINE);
}

WRITE_HANDLER( vastar_hold_cpu2_w )
{
	/* I'm not sure that this works exactly like this */
	if (data & 1)
		cpu_set_reset_line(1,CLEAR_LINE);
	else
		cpu_set_reset_line(1,ASSERT_LINE);
}



READ_HANDLER( vastar_sharedram_r )
{
	return vastar_sharedram[offset];
}

WRITE_HANDLER( vastar_sharedram_w )
{
	vastar_sharedram[offset] = data;
}
