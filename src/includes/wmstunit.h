/*************************************************************************

	Driver for Williams/Midway T-unit games.

**************************************************************************/

#include "wmsyunit.h"

/*----------- defined in machine/wmstunit.c -----------*/

WRITE16_HANDLER( wms_tunit_cmos_enable_w );
WRITE16_HANDLER( wms_tunit_cmos_w );
READ16_HANDLER( wms_tunit_cmos_r );

READ16_HANDLER( wms_tunit_input_r );

void init_mk(void);
void init_nbajam(void);
void init_nbajam20(void);
void init_nbajamte(void);
void init_mk2(void);
void init_mk2r14(void);

void wms_tunit_init_machine(void);

READ16_HANDLER( wms_tunit_sound_state_r );
READ16_HANDLER( wms_tunit_sound_r );
WRITE16_HANDLER( wms_tunit_sound_w );


/*----------- defined in vidhrdw/wmstunit.c -----------*/

extern UINT8 wms_gfx_rom_large;

int wms_tunit_vh_start(void);
int wms_wolfu_vh_start(void);
int wms_revx_vh_start(void);

void wms_tunit_vh_stop(void);

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

void wms_tunit_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);
