/*************************************************************************

    Sega G-80 raster hardware

*************************************************************************/

/*----------- defined in machine/segag80r.c -----------*/

extern UINT8 (*sega_decrypt)(offs_t, UINT8);

void sega_security(int chip);


/*----------- defined in sndhrdw/segag80r.c -----------*/

MACHINE_DRIVER_EXTERN( monsterb_sound );

WRITE8_HANDLER( astrob_audio_ports_w );
WRITE8_HANDLER( spaceod_audio_ports_w );
WRITE8_HANDLER( monsterb_audio_8255_w );
 READ8_HANDLER( monsterb_audio_8255_r );

/* sample names */
extern const char *astrob_sample_names[];
extern const char *s005_sample_names[];
extern const char *spaceod_sample_names[];


/*----------- defined in vidhrdw/segag80r.c -----------*/

#define G80_BACKGROUND_NONE			0
#define G80_BACKGROUND_SPACEOD		1
#define G80_BACKGROUND_MONSTERB		2
#define G80_BACKGROUND_PIGNEWT		3
#define G80_BACKGROUND_SINDBADM		4

extern UINT8 segag80r_background_pcb;

INTERRUPT_GEN( segag80r_vblank_start );

READ8_HANDLER( segag80r_videoram_r );
WRITE8_HANDLER( segag80r_videoram_w );

READ8_HANDLER( segag80r_video_port_r );
WRITE8_HANDLER( segag80r_video_port_w );

VIDEO_START( segag80r );
VIDEO_UPDATE( segag80r );


READ8_HANDLER( spaceod_back_port_r );
WRITE8_HANDLER( spaceod_back_port_w );


READ8_HANDLER( monsterb_videoram_r );
WRITE8_HANDLER( monsterb_videoram_w );
WRITE8_HANDLER( monsterb_back_port_w );


READ8_HANDLER( pignewt_videoram_r );
WRITE8_HANDLER( pignewt_videoram_w );
WRITE8_HANDLER( pignewt_back_port_w );
WRITE8_HANDLER( pignewt_back_color_w );


INTERRUPT_GEN( sindbadm_vblank_start );

READ8_HANDLER( sindbadm_videoram_r );
WRITE8_HANDLER( sindbadm_videoram_w );
WRITE8_HANDLER( sindbadm_back_port_w );
