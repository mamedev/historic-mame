/*************************************************************************

	Atari Basketball hardware

*************************************************************************/

/*----------- defined in machine/bsktball.c -----------*/

WRITE8_HANDLER( bsktball_nmion_w );
INTERRUPT_GEN( bsktball_interrupt );
WRITE8_HANDLER( bsktball_ld1_w );
WRITE8_HANDLER( bsktball_ld2_w );
READ8_HANDLER( bsktball_in0_r );
WRITE8_HANDLER( bsktball_led1_w );
WRITE8_HANDLER( bsktball_led2_w );
WRITE8_HANDLER( bsktball_bounce_w );
WRITE8_HANDLER( bsktball_note_w );
WRITE8_HANDLER( bsktball_noise_reset_w );


/*----------- defined in vidhrdw/bsktball.c -----------*/

extern unsigned char *bsktball_motion;
VIDEO_UPDATE( bsktball );
