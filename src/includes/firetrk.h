/*************************************************************************

	Atari Fire Truck hardware

*************************************************************************/

/*----------- defined in drivers/firetrk.c -----------*/

extern UINT8 firetrk_invert_display;
extern UINT8 firetrk_bit7_flags;
extern UINT8 firetrk_bit0_flags;


/*----------- defined in vidhrdw/firetrk.c -----------*/

VIDEO_START( firetrk );
VIDEO_UPDATE( firetrk );
