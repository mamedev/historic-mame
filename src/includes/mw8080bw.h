/***************************************************************************

    Midway 8080-based black and white hardware

****************************************************************************/

/*----------- defined in drivers/mw8080bw.c -----------*/

#define MW8080BW_XAL					(19968000)

#define SEAWOLF_GUN_PORT_TAG			("GUN")

#define TORNBASE_CAB_TYPE_UPRIGHT_OLD	(0)
#define TORNBASE_CAB_TYPE_UPRIGHT_NEW	(1)
#define TORNBASE_CAB_TYPE_COCKTAIL		(2)
int tornbase_get_cabinet_type(void);

#define DESERTGU_GUN_X_PORT_TAG			("GUNX")
#define DESERTGU_GUN_Y_PORT_TAG			("GUNY")

UINT8 phantom2_get_cloud_pos(void);

int invaders_is_cabinet_cocktail(void);

#define BLUESHRK_SPEAR_PORT_TAG			("SPEAR")


/*----------- defined in sndhrdw/mw8080bw.c -----------*/

WRITE8_HANDLER( midway_tone_generator_lo_w );
WRITE8_HANDLER( midway_tone_generator_hi_w );

MACHINE_DRIVER_EXTERN( seawolf_sound );
WRITE8_HANDLER( seawolf_sh_port_w );

MACHINE_DRIVER_EXTERN( gunfight_sound );
WRITE8_HANDLER( gunfight_sh_port_w );

MACHINE_DRIVER_EXTERN( tornbase_sound );
WRITE8_HANDLER( tornbase_sh_port_w );

MACHINE_DRIVER_EXTERN( zzzap_sound );
WRITE8_HANDLER( zzzap_sh_port_1_w );
WRITE8_HANDLER( zzzap_sh_port_2_w );

MACHINE_DRIVER_EXTERN( maze_sound );

MACHINE_DRIVER_EXTERN( boothill_sound );
WRITE8_HANDLER( boothill_sh_port_w );

MACHINE_DRIVER_EXTERN( checkmat_sound );
WRITE8_HANDLER( checkmat_sh_port_w );

MACHINE_DRIVER_EXTERN( desertgu_sound );
WRITE8_HANDLER( desertgu_sh_port_1_w );
WRITE8_HANDLER( desertgu_sh_port_2_w );
UINT8 desertgun_get_controller_select(void);

MACHINE_DRIVER_EXTERN( dplay_sound );
WRITE8_HANDLER( dplay_sh_port_w );

MACHINE_DRIVER_EXTERN( gmissile_sound );
WRITE8_HANDLER( gmissile_sh_port_1_w );
WRITE8_HANDLER( gmissile_sh_port_2_w );
WRITE8_HANDLER( gmissile_sh_port_3_w );

MACHINE_DRIVER_EXTERN( m4_sound );
WRITE8_HANDLER( m4_sh_port_1_w );
WRITE8_HANDLER( m4_sh_port_2_w );

MACHINE_DRIVER_EXTERN( clowns_sound );
WRITE8_HANDLER( clowns_sh_port_1_w );
WRITE8_HANDLER( clowns_sh_port_2_w );
UINT8 clowns_get_controller_select(void);

MACHINE_DRIVER_EXTERN( shuffle_sound );
WRITE8_HANDLER( shuffle_sh_port_1_w );
WRITE8_HANDLER( shuffle_sh_port_2_w );

MACHINE_DRIVER_EXTERN( dogpatch_sound );
WRITE8_HANDLER( dogpatch_sh_port_w );

MACHINE_DRIVER_EXTERN( spcenctr_sound );
WRITE8_HANDLER( spcenctr_sh_port_1_w );
WRITE8_HANDLER( spcenctr_sh_port_2_w );
WRITE8_HANDLER( spcenctr_sh_port_3_w );

MACHINE_DRIVER_EXTERN( phantom2_sound );
WRITE8_HANDLER( phantom2_sh_port_1_w );
WRITE8_HANDLER( phantom2_sh_port_2_w );

MACHINE_DRIVER_EXTERN( bowler_sound );
WRITE8_HANDLER( bowler_sh_port_1_w );
WRITE8_HANDLER( bowler_sh_port_2_w );
WRITE8_HANDLER( bowler_sh_port_3_w );
WRITE8_HANDLER( bowler_sh_port_4_w );
WRITE8_HANDLER( bowler_sh_port_5_w );
WRITE8_HANDLER( bowler_sh_port_6_w );

MACHINE_DRIVER_EXTERN( invaders_sound );
WRITE8_HANDLER( invaders_sh_port_1_w );
WRITE8_HANDLER( invaders_sh_port_2_w );

MACHINE_DRIVER_EXTERN( blueshrk_sound );
WRITE8_HANDLER( blueshrk_sh_port_w );

MACHINE_DRIVER_EXTERN( invad2ct_sound );
WRITE8_HANDLER( invad2ct_sh_port_1_w );
WRITE8_HANDLER( invad2ct_sh_port_2_w );
WRITE8_HANDLER( invad2ct_sh_port_3_w );
WRITE8_HANDLER( invad2ct_sh_port_4_w );



/*----------- defined in vidhrdw/mw8080bw.c -----------*/

PALETTE_INIT( phantom2 );

VIDEO_UPDATE( mw8080bw );
VIDEO_UPDATE( phantom2 );
VIDEO_UPDATE( invaders );
