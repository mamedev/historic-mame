/***************************************************************************


***************************************************************************/

/* Variables defined in vidhrdw */

extern int cave_spritetype;

extern data16_t *cave_videoregs;

extern data16_t *cave_vram_0, *cave_vctrl_0;
extern data16_t *cave_vram_1, *cave_vctrl_1;
extern data16_t *cave_vram_2, *cave_vctrl_2;
extern data16_t *cave_vram_3, *cave_vctrl_3;

/* Functions defined in vidhrdw */

WRITE16_HANDLER( cave_vram_0_w );
WRITE16_HANDLER( cave_vram_1_w );
WRITE16_HANDLER( cave_vram_2_w );
WRITE16_HANDLER( cave_vram_3_w );

WRITE16_HANDLER( cave_vram_0_8x8_w );
WRITE16_HANDLER( cave_vram_1_8x8_w );
WRITE16_HANDLER( cave_vram_2_8x8_w );
WRITE16_HANDLER( cave_vram_3_8x8_w );

void ddonpach_vh_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void dfeveron_vh_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void mazinger_vh_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void sailormn_vh_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

int cave_vh_start_1_layer (void);
int cave_vh_start_2_layers(void);
int cave_vh_start_3_layers(void);
int cave_vh_start_4_layers(void);

int sailormn_vh_start_3_layers(void);

void cave_vh_stop(void);

void cave_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);

void sailormn_tilebank_w( int bank );
