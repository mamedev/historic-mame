/*************************************************************************

	Turbo - Sega - 1981
	Subroc 3D - Sega - 1982
	Buck Rogers: Planet of Zoom - Sega - 1982

*************************************************************************/

/*----------- defined in machine/turbo.c -----------*/

extern UINT8 turbo_opa, turbo_opb, turbo_opc;
extern UINT8 turbo_ipa, turbo_ipb, turbo_ipc;
extern UINT8 turbo_fbpla, turbo_fbcol;
extern UINT8 turbo_segment_data[32];
extern UINT8 turbo_speed;

extern UINT8 subroc3d_col, subroc3d_ply, subroc3d_chofs;

extern UINT8 buckrog_fchg, buckrog_mov, buckrog_obch;

void turbo_init_machine(void);
void subroc3d_init_machine(void);
void buckrog_init_machine(void);

READ_HANDLER( turbo_8279_r );
WRITE_HANDLER( turbo_8279_w );

READ_HANDLER( turbo_collision_r );
WRITE_HANDLER( turbo_collision_clear_w );
WRITE_HANDLER( turbo_coin_and_lamp_w );
void turbo_rom_decode(void);

READ_HANDLER( buckrog_cpu2_command_r );
READ_HANDLER( buckrog_port_2_r );
READ_HANDLER( buckrog_port_3_r );


/*----------- defined in vidhrdw/turbo.c -----------*/

extern UINT8 *sega_sprite_position;
extern UINT8 turbo_collision;

void turbo_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom);
int turbo_vh_start(void);
void turbo_vh_stop(void);
void turbo_vh_eof(void);
void turbo_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);

void subroc3d_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom);
int subroc3d_vh_start(void);
void subroc3d_vh_stop(void);
void subroc3d_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);

void buckrog_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom);
int buckrog_vh_start(void);
void buckrog_vh_stop(void);
void buckrog_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh);

WRITE_HANDLER( buckrog_led_display_w );
WRITE_HANDLER( buckrog_bitmap_w );
