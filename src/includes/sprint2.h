/*************************************************************************

	Atari Sprint 2 hardware

*************************************************************************/

/*----------- defined in machine/sprint2.c -----------*/

extern UINT8 sprintx_is_sprint2;

extern int sprint2_collision1_data;
extern int sprint2_collision2_data;
extern int sprint2_gear1;
extern int sprint2_gear2;

READ_HANDLER( sprintx_read_ports_r );
READ_HANDLER( sprint2_read_sync_r );
READ_HANDLER( sprint2_coins_r );
READ_HANDLER( sprint2_steering1_r );
READ_HANDLER( sprint2_steering2_r );
READ_HANDLER( sprint2_collision1_r );
READ_HANDLER( sprint2_collision2_r );
WRITE_HANDLER( sprint2_collision_reset1_w );
WRITE_HANDLER( sprint2_collision_reset2_w );
WRITE_HANDLER( sprint2_steering_reset1_w );
WRITE_HANDLER( sprint2_steering_reset2_w );
WRITE_HANDLER( sprint2_lamp1_w );
WRITE_HANDLER( sprint2_lamp2_w );


/*----------- defined in vidhrdw/sprint2.c -----------*/

VIDEO_START( sprint2 );
VIDEO_UPDATE( sprint1 );
VIDEO_UPDATE( sprint2 );

extern unsigned char *sprint2_vert_car_ram;
extern unsigned char *sprint2_horiz_ram;
