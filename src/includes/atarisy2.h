/*************************************************************************

	Atari System 2 hardware

*************************************************************************/

/*----------- defined in vidhrdw/atarisy2.c -----------*/

READ16_HANDLER( atarisy2_slapstic_r );
READ16_HANDLER( atarisy2_videoram_r );

WRITE16_HANDLER( atarisy2_slapstic_w );
WRITE16_HANDLER( atarisy2_vscroll_w );
WRITE16_HANDLER( atarisy2_hscroll_w );
WRITE16_HANDLER( atarisy2_videoram_w );
WRITE16_HANDLER( atarisy2_paletteram_w );

void atarisy2_scanline_update(int scanline);

VIDEO_START( atarisy2 );
VIDEO_UPDATE( atarisy2 );

extern data16_t *atarisy2_slapstic;
