/***************************************************************************


***************************************************************************/

/* Variables defined in vidhrdw */

extern int cave_spritetype;

extern data16_t *cave_videoregs;

extern data16_t *cave_vram_0, *cave_vctrl_0;
extern data16_t *cave_vram_1, *cave_vctrl_1;
extern data16_t *cave_vram_2, *cave_vctrl_2;

/* Functions defined in vidhrdw */

WRITE16_HANDLER( cave_vram_0_w );
WRITE16_HANDLER( cave_vram_1_w );
WRITE16_HANDLER( cave_vram_2_w );

WRITE16_HANDLER( cave_vram_0_8x8_w );
WRITE16_HANDLER( cave_vram_1_8x8_w );
WRITE16_HANDLER( cave_vram_2_8x8_w );

void ddonpach_vh_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void dfeveron_vh_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void mazinger_vh_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

int cave_vh_start_16_16_16(void);
int cave_vh_start_16_16_8(void);
int cave_vh_start_16_16_0(void);
int cave_vh_start_16_0_0(void);
int cave_vh_start_8_8_0(void);

int sailormn_vh_start_16_16_8(void);

void cave_vh_stop(void);

void cave_vh_screenrefresh(struct mame_bitmap *bitmap,int full_refresh);

void sailormn_tilebank_w( int bank );
