/*************************************************************************

	Atari System 1 hardware

*************************************************************************/

/*----------- defined in vidhrdw/atarisy1.c -----------*/

READ16_HANDLER( atarisys1_int3state_r );

WRITE16_HANDLER( atarisys1_spriteram_w );
WRITE16_HANDLER( atarisys1_bankselect_w );
WRITE16_HANDLER( atarisys1_hscroll_w );
WRITE16_HANDLER( atarisys1_vscroll_w );
WRITE16_HANDLER( atarisys1_priority_w );

void atarisys1_scanline_update(int scanline);

VIDEO_START( atarisy1 );
VIDEO_UPDATE( atarisy1 );
