/*************************************************************************

	Atari Toobin' hardware

*************************************************************************/

/*----------- defined in vidhrdw/toobin.c -----------*/

WRITE16_HANDLER( toobin_paletteram_w );
WRITE16_HANDLER( toobin_intensity_w );
WRITE16_HANDLER( toobin_hscroll_w );
WRITE16_HANDLER( toobin_vscroll_w );

VIDEO_START( toobin );
VIDEO_UPDATE( toobin );
