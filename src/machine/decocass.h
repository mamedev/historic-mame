/* set to 1 to display tape time offset */
#define TAPE_UI_DISPLAY 0

#ifdef MAME_DEBUG
#define LOGLEVEL  0
#define LOG(n,x)  if (LOGLEVEL >= n) logerror x
#else
#define LOG(n,x)
#endif

extern WRITE_HANDLER( decocass_coin_counter_w );
extern WRITE_HANDLER( decocass_sound_command_w );
extern READ_HANDLER( decocass_sound_data_r );
extern READ_HANDLER( decocass_sound_ack_r );
extern WRITE_HANDLER( decocass_sound_data_w );
extern READ_HANDLER( decocass_sound_command_r );
extern WRITE_HANDLER( decocass_sound_nmi_enable_w );
extern READ_HANDLER( decocass_sound_nmi_enable_r );
extern READ_HANDLER( decocass_sound_data_ack_reset_r );
extern WRITE_HANDLER( decocass_sound_data_ack_reset_w );
extern WRITE_HANDLER( decocass_nmi_reset_w );
extern WRITE_HANDLER( decocass_quadrature_decoder_reset_w );
extern WRITE_HANDLER( decocass_adc_w );
extern READ_HANDLER( decocass_input_r );
extern int tape_dir;
extern int tape_speed;
extern double tape_time0;
extern void *tape_timer;

extern WRITE_HANDLER( decocass_reset_w );
extern READ_HANDLER( decocass_type1_r );
extern READ_HANDLER( decocass_type1_map1_r );
extern READ_HANDLER( decocass_type1_map2_r );
extern READ_HANDLER( decocass_type1_map3_r );
extern READ_HANDLER( type2_r );
extern WRITE_HANDLER( type2_w );
extern READ_HANDLER( type3_r );
extern WRITE_HANDLER( type3_w );

extern READ_HANDLER( decocass_e5xx_r );
extern WRITE_HANDLER( decocass_e5xx_w );

extern void decocass_init_machine(void);
extern void ctsttape_init_machine(void);
extern void clocknch_init_machine(void);
extern void ctisland_init_machine(void);
extern void csuperas_init_machine(void);
extern void castfant_init_machine(void);
extern void cluckypo_init_machine(void);
extern void cterrani_init_machine(void);
extern void cexplore_init_machine(void);
extern void cprogolf_init_machine(void);
extern void cmissnx_init_machine(void);
extern void cdiscon1_init_machine(void);
extern void cptennis_init_machine(void);
extern void ctornado_init_machine(void);
extern void cbnj_init_machine(void);
extern void cburnrub_init_machine(void);
extern void cbtime_init_machine(void);
extern void cgraplop_init_machine(void);
extern void clapapa_init_machine(void);
extern void cfghtice_init_machine(void);
extern void cprobowl_init_machine(void);
extern void cnightst_init_machine(void);
extern void cprosocc_init_machine(void);
extern void cppicf_init_machine(void);
extern void cscrtry_init_machine(void);
extern void cbdash_init_machine(void);

extern WRITE_HANDLER( i8041_p1_w );
extern READ_HANDLER( i8041_p1_r );
extern WRITE_HANDLER( i8041_p2_w );
extern READ_HANDLER( i8041_p2_r );

/* from drivers/decocass.c */
extern WRITE_HANDLER( decocass_w );

/* from vidhrdw/decocass.c */
extern WRITE_HANDLER( decocass_paletteram_w );
extern WRITE_HANDLER( decocass_charram_w );
extern WRITE_HANDLER( decocass_fgvideoram_w );
extern WRITE_HANDLER( decocass_colorram_w );
extern WRITE_HANDLER( decocass_bgvideoram_w );
extern WRITE_HANDLER( decocass_tileram_w );
extern WRITE_HANDLER( decocass_objectram_w );
extern READ_HANDLER( decocass_mirrorvideoram_r );
extern READ_HANDLER( decocass_mirrorcolorram_r );
extern WRITE_HANDLER( decocass_mirrorvideoram_w );
extern WRITE_HANDLER( decocass_mirrorcolorram_w );

extern WRITE_HANDLER( decocass_watchdog_count_w );
extern WRITE_HANDLER( decocass_watchdog_flip_w );
extern WRITE_HANDLER( decocass_color_missiles_w );
extern WRITE_HANDLER( decocass_mode_set_w );
extern WRITE_HANDLER( decocass_color_center_bot_w );
extern WRITE_HANDLER( decocass_back_h_shift_w );
extern WRITE_HANDLER( decocass_back_vl_shift_w );
extern WRITE_HANDLER( decocass_back_vr_shift_w );
extern WRITE_HANDLER( decocass_part_h_shift_w );
extern WRITE_HANDLER( decocass_part_v_shift_w );
extern WRITE_HANDLER( decocass_center_h_shift_space_w );
extern WRITE_HANDLER( decocass_center_v_shift_w );

extern int decocass_vh_start (void);
extern void decocass_vh_stop (void);
extern void decocass_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);

extern unsigned char *decocass_charram;
extern unsigned char *decocass_fgvideoram;
extern unsigned char *decocass_colorram;
extern unsigned char *decocass_bgvideoram;
extern unsigned char *decocass_tileram;
extern unsigned char *decocass_objectram;
extern size_t decocass_fgvideoram_size;
extern size_t decocass_colorram_size;
extern size_t decocass_bgvideoram_size;
extern size_t decocass_tileram_size;
extern size_t decocass_objectram_size;

