/*************************************************************************

	Atari Football hardware

*************************************************************************/

/*----------- defined in drivers/atarifb.c -----------*/

extern int atarifb_game;
extern int atarifb_lamp1, atarifb_lamp2;


/*----------- defined in machine/atarifb.c -----------*/

WRITE_HANDLER( atarifb_out1_w );
READ_HANDLER( atarifb_in0_r );
READ_HANDLER( atarifb_in2_r );
WRITE_HANDLER( atarifb4_out1_w );
READ_HANDLER( atarifb4_in0_r );
READ_HANDLER( atarifb4_in2_r );
WRITE_HANDLER( soccer_out1_w );


/*----------- defined in vidhrdw/atarifb.c -----------*/

extern size_t atarifb_alphap1_vram_size;
extern size_t atarifb_alphap2_vram_size;
extern unsigned char *atarifb_alphap1_vram;
extern unsigned char *atarifb_alphap2_vram;
extern unsigned char *atarifb_scroll_register;

WRITE_HANDLER( atarifb_scroll_w );
WRITE_HANDLER( atarifb_alphap1_vram_w );
WRITE_HANDLER( atarifb_alphap2_vram_w );

VIDEO_START( atarifb );
VIDEO_UPDATE( atarifb );
