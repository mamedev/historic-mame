/***************************************************************************

    Kyugo hardware games

***************************************************************************/

/* defined in machine/kyugo.c */
extern data8_t *kyugo_sharedram;

MACHINE_INIT( kyugo );

READ8_HANDLER( kyugo_sharedram_r );

WRITE8_HANDLER( kyugo_sharedram_w );
WRITE8_HANDLER( kyugo_sub_cpu_control_w );

/* defined in vidhrdw/kyugo.c */
extern data8_t *kyugo_fgvideoram;
extern data8_t *kyugo_bgvideoram;
extern data8_t *kyugo_bgattribram;
extern data8_t *kyugo_spriteram_1;
extern data8_t *kyugo_spriteram_2;

READ8_HANDLER( kyugo_spriteram_2_r );

WRITE8_HANDLER( kyugo_fgvideoram_w );
WRITE8_HANDLER( kyugo_bgvideoram_w );
WRITE8_HANDLER( kyugo_bgattribram_w );
WRITE8_HANDLER( kyugo_scroll_x_lo_w );
WRITE8_HANDLER( kyugo_gfxctrl_w );
WRITE8_HANDLER( kyugo_scroll_y_w );
WRITE8_HANDLER( kyugo_flipscreen_w );

VIDEO_START( kyugo );

VIDEO_UPDATE( kyugo );
