/*************************************************************************

	Atari Gauntlet hardware

*************************************************************************/

/*----------- defined in vidhrdw/gauntlet.c -----------*/

WRITE16_HANDLER( gauntlet_xscroll_w );
WRITE16_HANDLER( gauntlet_yscroll_w );

VIDEO_START( gauntlet );
VIDEO_UPDATE( gauntlet );

extern UINT8 vindctr2_screen_refresh;
extern data16_t *gauntlet_yscroll;
