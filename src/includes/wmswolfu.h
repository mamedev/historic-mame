/*************************************************************************

	Driver for Williams/Midway Wolf-unit games.

**************************************************************************/

#include "wmstunit.h"

/*----------- defined in machine/wmswolfu.c -----------*/

extern UINT8 *wms_wolfu_decode_memory;

WRITE16_HANDLER( wms_wolfu_cmos_enable_w );
WRITE16_HANDLER( wms_wolfu_cmos_w );
WRITE16_HANDLER( revx_cmos_w );
READ16_HANDLER( wms_wolfu_cmos_r );

WRITE16_HANDLER( wms_wolfu_io_w );
WRITE16_HANDLER( revx_io_w );
WRITE16_HANDLER( revx_unknown_w );

READ16_HANDLER( wms_wolfu_io_r );
READ16_HANDLER( revx_io_r );
READ16_HANDLER( revx_analog_r );
WRITE16_HANDLER( revx_analog_select_w );
READ16_HANDLER( revx_status_r );

READ16_HANDLER( revx_uart_r );
WRITE16_HANDLER( revx_uart_w );

DRIVER_INIT( mk3 );
DRIVER_INIT( mk3r20 );
DRIVER_INIT( mk3r10 );
DRIVER_INIT( umk3 );
DRIVER_INIT( umk3r11 );

DRIVER_INIT( openice );
DRIVER_INIT( nbahangt );
DRIVER_INIT( wwfmania );
DRIVER_INIT( rmpgwt );
DRIVER_INIT( revx );

MACHINE_INIT( wms_wolfu );
MACHINE_INIT( revx );

READ16_HANDLER( wms_wolfu_security_r );
WRITE16_HANDLER( wms_wolfu_security_w );
WRITE16_HANDLER( revx_security_w );
WRITE16_HANDLER( revx_security_clock_w );

READ16_HANDLER( wms_wolfu_sound_r );
WRITE16_HANDLER( wms_wolfu_sound_w );
