/*************************************************************************

	Driver for Williams/Midway T-unit games.

**************************************************************************/

#include "wmsyunit.h"

/*----------- defined in machine/wmstunit.c -----------*/

WRITE16_HANDLER( wms_tunit_cmos_enable_w );
WRITE16_HANDLER( wms_tunit_cmos_w );
READ16_HANDLER( wms_tunit_cmos_r );

READ16_HANDLER( wms_tunit_input_r );

DRIVER_INIT( mk );
DRIVER_INIT( jdredd );
DRIVER_INIT( nbajam );
DRIVER_INIT( nbajam20 );
DRIVER_INIT( nbajamte );
DRIVER_INIT( mk2 );
DRIVER_INIT( mk2r14 );
DRIVER_INIT( mk2r21 );

MACHINE_INIT( wms_tunit );

READ16_HANDLER( wms_tunit_sound_state_r );
READ16_HANDLER( wms_tunit_sound_r );
WRITE16_HANDLER( wms_tunit_sound_w );


/*----------- defined in vidhrdw/wmstunit.c -----------*/

extern UINT8 wms_gfx_rom_large;

VIDEO_START( wms_tunit );
VIDEO_START( wms_wolfu );
VIDEO_START( revx );

READ16_HANDLER( wms_tunit_gfxrom_r );
READ16_HANDLER( wms_wolfu_gfxrom_r );

WRITE16_HANDLER( wms_tunit_vram_w );
WRITE16_HANDLER( wms_tunit_vram_data_w );
WRITE16_HANDLER( wms_tunit_vram_color_w );

READ16_HANDLER( wms_tunit_vram_r );
READ16_HANDLER( wms_tunit_vram_data_r );
READ16_HANDLER( wms_tunit_vram_color_r );

void wms_tunit_to_shiftreg(UINT32 address, UINT16 *shiftreg);
void wms_tunit_from_shiftreg(UINT32 address, UINT16 *shiftreg);

WRITE16_HANDLER( wms_tunit_control_w );
WRITE16_HANDLER( wms_wolfu_control_w );
READ16_HANDLER( wms_wolfu_control_r );

WRITE16_HANDLER( wms_tunit_paletteram_w );
WRITE16_HANDLER( revx_paletteram_w );
READ16_HANDLER( revx_paletteram_r );

READ16_HANDLER( wms_tunit_dma_r );
WRITE16_HANDLER( wms_tunit_dma_w );

VIDEO_UPDATE( wms_tunit );
