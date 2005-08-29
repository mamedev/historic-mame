/***************************************************************************

    Kyugo hardware games

***************************************************************************/

#include "driver.h"
#include "kyugo.h"


UINT8 *kyugo_sharedram;


/*************************************
 *
 *  Machine initialization
 *
 *************************************/

MACHINE_INIT( kyugo )
{
	// must start with interrupts and sub CPU disabled
	cpu_interrupt_enable(0, 0);
	kyugo_sub_cpu_control_w(0, 0);
}


WRITE8_HANDLER( kyugo_sub_cpu_control_w )
{
	cpunum_set_input_line(1, INPUT_LINE_HALT, data ? CLEAR_LINE : ASSERT_LINE);
}


/*************************************
 *
 *  Shared RAM handlers
 *
 *************************************/

WRITE8_HANDLER( kyugo_sharedram_w )
{
	kyugo_sharedram[offset] = data;
}


READ8_HANDLER( kyugo_sharedram_r )
{
	return kyugo_sharedram[offset];
}
