/*************************************************************************

    Sega vector hardware

*************************************************************************/

#include "sound/custom.h"


/*----------- defined in sndhrdw/sega.c -----------*/

WRITE8_HANDLER( elim1_sh_w );
WRITE8_HANDLER( elim2_sh_w );
WRITE8_HANDLER( spacfury1_sh_w );
WRITE8_HANDLER( spacfury2_sh_w );
WRITE8_HANDLER( zektor1_sh_w );
WRITE8_HANDLER( zektor2_sh_w );

void * tacscan_sh_start (int clock, const struct CustomSound_interface *config);

WRITE8_HANDLER( tacscan_sh_w );
WRITE8_HANDLER( startrek_sh_w );


/*----------- defined in vidhrdw/sega.c -----------*/

VIDEO_START( sega );
VIDEO_UPDATE( sega );
