/*************************************************************************

	Coors Light Bowling/Bowl-O-Rama hardware

*************************************************************************/

/*----------- defined in vidhrdw/capbowl.c -----------*/

VIDEO_START( capbowl );
VIDEO_UPDATE( capbowl );

extern UINT8 *capbowl_rowaddress;

WRITE_HANDLER( bowlrama_blitter_w );
READ_HANDLER( bowlrama_blitter_r );

WRITE_HANDLER( capbowl_tms34061_w );
READ_HANDLER( capbowl_tms34061_r );
