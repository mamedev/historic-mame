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

void init_mk3(void);
void init_mk3r20(void);
void init_mk3r10(void);
void init_umk3(void);
void init_umk3r11(void);

void init_openice(void);
void init_nbahangt(void);
void init_wwfmania(void);
void init_rmpgwt(void);
void init_revx(void);

void wms_wolfu_init_machine(void);
void revx_init_machine(void);

READ16_HANDLER( wms_wolfu_security_r );
WRITE16_HANDLER( wms_wolfu_security_w );
WRITE16_HANDLER( revx_security_w );
WRITE16_HANDLER( revx_security_clock_w );

READ16_HANDLER( wms_wolfu_sound_r );
WRITE16_HANDLER( wms_wolfu_sound_w );
