/***************************************************************************

							-= Kaneko 16 Bit Games =-

***************************************************************************/

/* Tile Layers: */

extern data16_t *kaneko16_vram_0, *kaneko16_vram_1, *kaneko16_layers_0_regs;
extern data16_t *kaneko16_vram_2, *kaneko16_vram_3, *kaneko16_layers_1_regs;

WRITE16_HANDLER( kaneko16_vram_0_w );
WRITE16_HANDLER( kaneko16_vram_1_w );
WRITE16_HANDLER( kaneko16_vram_2_w );
WRITE16_HANDLER( kaneko16_vram_3_w );

WRITE16_HANDLER( kaneko16_layers_0_regs_w );
WRITE16_HANDLER( kaneko16_layers_1_regs_w );


/* Sprites: */

extern int kaneko16_sprite_type;
extern data16_t kaneko16_sprite_xoffs, kaneko16_sprite_flipx;
extern data16_t kaneko16_sprite_yoffs, kaneko16_sprite_flipy;
extern data16_t *kaneko16_sprites_regs;

READ16_HANDLER ( kaneko16_sprites_regs_r );
WRITE16_HANDLER( kaneko16_sprites_regs_w );

void kaneko16_draw_sprites(struct osd_bitmap *bitmap, int pri);

/* Pixel Layer: */

extern data16_t *kaneko16_bg15_select, *kaneko16_bg15_reg;

READ16_HANDLER ( kaneko16_bg15_select_r );
WRITE16_HANDLER( kaneko16_bg15_select_w );

READ16_HANDLER ( kaneko16_bg15_reg_r );
WRITE16_HANDLER( kaneko16_bg15_reg_w );

void berlwall_init_palette(unsigned char *obsolete,unsigned short *colortable,const unsigned char *color_prom);


/* Priorities: */

typedef struct
{
	int tile[4];
	int sprite[4];
}	kaneko16_priority_t;

extern kaneko16_priority_t kaneko16_priority;


/* Machine */

int kaneko16_vh_start_sprites(void);
int kaneko16_vh_start_1xVIEW2(void);
int kaneko16_vh_start_2xVIEW2(void);
int berlwall_vh_start(void);
int sandscrp_vh_start_1xVIEW2(void);

void kaneko16_vh_stop(void);
void berlwall_vh_stop(void);

void kaneko16_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


void kaneko16_init_machine(void);
