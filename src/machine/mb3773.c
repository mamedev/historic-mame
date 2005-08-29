/*
 * mb3773 - Power Supply Monitor with Watch Dog Timer
 *
 * Todo:
 *  Calculate the timeout from parameters.
 *
 */

#include "state.h"
#include "machine/mb3773.h"

static void *watchdog_timer;
static UINT8 ck = 0;

static void watchdog_timeout( int unused )
{
	machine_reset();
}

static void reset_timer( void )
{
	timer_adjust( watchdog_timer, TIME_IN_SEC( 5 ), 0, 0 );
}

void mb3773_set_ck( UINT8 new_ck )
{
	if( new_ck == 0 && ck != 0 )
	{
		reset_timer();
	}
	ck = new_ck;
}

void mb3773_init( void )
{
	watchdog_timer = timer_alloc( watchdog_timeout );
	reset_timer();
	state_save_register_UINT8( "mb3773", 0, "ck", &ck, 1 );
}
