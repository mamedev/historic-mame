/*************************************************************************

	Atari Subs hardware

*************************************************************************/

/*----------- defined in machine/subs.c -----------*/

MACHINE_INIT( subs );
INTERRUPT_GEN( subs_interrupt );
WRITE8_HANDLER( subs_steer_reset_w );
READ8_HANDLER( subs_control_r );
READ8_HANDLER( subs_coin_r );
READ8_HANDLER( subs_options_r );
WRITE8_HANDLER( subs_lamp1_w );
WRITE8_HANDLER( subs_lamp2_w );
WRITE8_HANDLER( subs_noise_reset_w );
WRITE8_HANDLER( subs_sonar2_w );
WRITE8_HANDLER( subs_sonar1_w );
WRITE8_HANDLER( subs_crash_w );
WRITE8_HANDLER( subs_explode_w );


/*----------- defined in vidhrdw/subs.c -----------*/

VIDEO_UPDATE( subs );

WRITE8_HANDLER( subs_invert1_w );
WRITE8_HANDLER( subs_invert2_w );
