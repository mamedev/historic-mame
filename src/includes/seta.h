/***************************************************************************

							-= Seta Hardware =-

***************************************************************************/

/* Variables and functions defined in drivers/seta.c */

void seta_coin_lockout_w(int data);


/* Variables and functions defined in vidhrdw/seta.c */

extern data16_t *seta_vram_0, *seta_vram_1, *seta_vctrl_0;
extern data16_t *seta_vram_2, *seta_vram_3, *seta_vctrl_2;
extern data16_t *seta_vregs;

extern int seta_tiles_offset;

WRITE16_HANDLER( seta_vram_0_w );
WRITE16_HANDLER( seta_vram_1_w );
WRITE16_HANDLER( seta_vram_2_w );
WRITE16_HANDLER( seta_vram_3_w );
WRITE16_HANDLER( seta_vregs_w );

void blandia_vh_init_palette (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void gundhara_vh_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void jjsquawk_vh_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void usclssic_vh_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void zingzip_vh_init_palette (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

int seta_vh_start_no_layers(void);

int seta_vh_start_1_layer(void);
int seta_vh_start_1_layer_offset_0x02(void);

int seta_vh_start_2_layers(void);
int seta_vh_start_2_layers_offset_0x02(void);
int oisipuzl_vh_start_2_layers(void);

void seta_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);
void seta_vh_screenrefresh_no_layers(struct mame_bitmap *bitmap,int full_refresh);


/* Variables and functions defined in vidhrdw/seta2.c */

extern data16_t *seta2_vregs;

WRITE16_HANDLER( seta2_vregs_w );

void seta2_vh_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void seta2_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);


/* Variables and functions defined in sndhrdw/seta.c */

extern int seta_samples_bank;

READ_HANDLER ( seta_sound_r );
WRITE_HANDLER( seta_sound_w );

READ16_HANDLER ( seta_sound_word_r );
WRITE16_HANDLER( seta_sound_word_w );

void seta_sound_enable_w(int);

int seta_sh_start(const struct MachineSound *msound);

struct CustomSound_interface seta_sound_interface;
