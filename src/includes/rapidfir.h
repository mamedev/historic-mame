/*************************************************************************

	Hanaho Rapid Fire hardware

**************************************************************************/

/*----------- defined in vidhrdw/rapidfir.c -----------*/

extern data16_t *rapidfir_vram_base;

VIDEO_START( rapidfir );

void rapidfir_display_addr_changed(UINT32 offs, int rowbytes, int scanline);
void rapidfir_to_shiftreg(UINT32 address, UINT16 *shiftreg);
void rapidfir_from_shiftreg(UINT32 address, UINT16 *shiftreg);

WRITE16_HANDLER( rapidfir_transparent_w );
READ16_HANDLER( rapidfir_transparent_r );

READ16_HANDLER( rapidfir_p1_gun_r );
READ16_HANDLER( rapidfir_p2_gun_r );

WRITE16_HANDLER( rapidfir_palette_bank_w );
WRITE16_HANDLER( rapidfir_palette_w );
READ16_HANDLER( rapidfir_palette_r );

VIDEO_UPDATE( rapidfir );
