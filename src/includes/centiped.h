/*************************************************************************

	Atari Centipede hardware

*************************************************************************/

/*----------- defined in vidhrdw/centiped.c -----------*/

WRITE_HANDLER( centiped_paletteram_w );

PALETTE_INIT( centiped );
VIDEO_UPDATE( centiped );

MACHINE_INIT( centiped );
INTERRUPT_GEN( centiped_interrupt );
