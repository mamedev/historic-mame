/*************************************************************************

	Atari G42 hardware

*************************************************************************/

/*----------- defined in machine/asic65.c -----------*/

void asic65_reset(int state);

READ16_HANDLER( asic65_status_r );
READ16_HANDLER( asic65_data_r );
WRITE16_HANDLER( asic65_data_w );


/*----------- defined in vidhrdw/atarig42.c -----------*/

VIDEO_START( atarig42 );
VIDEO_START( atarigx2 );
VIDEO_START( atarigt );

VIDEO_UPDATE( atarig42 );

WRITE16_HANDLER( atarig42_mo_control_w );

void atarig42_scanline_update(int scanline);
void atarigx2_scanline_update(int scanline);

extern UINT8 atarig42_swapcolors;
