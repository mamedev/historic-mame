/*************************************************************************

    Atari Missile Command hardware

*************************************************************************/

/*----------- defined in drivers/missile.c -----------*/

MACHINE_RESET( missile );
READ8_HANDLER( missile_r );
WRITE8_HANDLER( missile_w );


/*----------- defined in vidhrdw/missile.c -----------*/

extern unsigned char *missile_video2ram;

VIDEO_START( missile );
VIDEO_UPDATE( missile );

WRITE8_HANDLER( missile_video_3rd_bit_w );
WRITE8_HANDLER( missile_video2_w );

READ8_HANDLER( missile_video_r );
WRITE8_HANDLER( missile_video_w );
WRITE8_HANDLER( missile_video_mult_w );
WRITE8_HANDLER( missile_palette_w );
