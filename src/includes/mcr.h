/*************************************************************************

	Driver for Midway MCR games

**************************************************************************/

/* constants */
#define MAIN_OSC_MCR_I		19968000


/*----------- defined in machine/mcr.c -----------*/

extern INT16 spyhunt_scrollx, spyhunt_scrolly;
extern double mcr68_timing_factor;

extern Z80_DaisyChain mcr_daisy_chain[];
extern UINT8 mcr_cocktail_flip;

extern struct GfxLayout mcr_bg_layout;
extern struct GfxLayout mcr_sprite_layout;

MACHINE_INIT( mcr );
MACHINE_INIT( mcr68 );
MACHINE_INIT( zwackery );

INTERRUPT_GEN( mcr_interrupt );
INTERRUPT_GEN( mcr68_interrupt );

WRITE_HANDLER( mcr_control_port_w );
WRITE_HANDLER( mcrmono_control_port_w );
WRITE_HANDLER( mcr_scroll_value_w );

WRITE16_HANDLER( mcr68_6840_upper_w );
WRITE16_HANDLER( mcr68_6840_lower_w );
READ16_HANDLER( mcr68_6840_upper_r );
READ16_HANDLER( mcr68_6840_lower_r );


/*----------- defined in vidhrdw/mcr12.c -----------*/

extern INT8 mcr12_sprite_xoffs;
extern INT8 mcr12_sprite_xoffs_flip;

VIDEO_START( mcr12 );

WRITE_HANDLER( mcr1_videoram_w );
READ_HANDLER( mcr2_videoram_r );
READ_HANDLER( twotigra_videoram_r );
WRITE_HANDLER( mcr2_videoram_w );
WRITE_HANDLER( twotigra_videoram_w );

VIDEO_UPDATE( mcr1 );
VIDEO_UPDATE( mcr2 );
VIDEO_UPDATE( journey );


/*----------- defined in vidhrdw/mcr3.c -----------*/

extern UINT8 spyhunt_sprite_color_mask;
extern INT16 spyhunt_scrollx, spyhunt_scrolly;
extern INT16 spyhunt_scroll_offset;
extern UINT8 spyhunt_draw_lamps;
extern UINT8 spyhunt_lamp[8];

extern UINT8 *spyhunt_alpharam;
extern size_t spyhunt_alpharam_size;

WRITE_HANDLER( mcr3_paletteram_w );
WRITE_HANDLER( mcr3_videoram_w );

void mcr3_update_sprites(struct mame_bitmap *bitmap, const struct rectangle *cliprect, int color_mask, int code_xor, int dx, int dy);

VIDEO_UPDATE( mcr3 );
VIDEO_UPDATE( mcrmono );

PALETTE_INIT( spyhunt );
VIDEO_START( spyhunt );
VIDEO_UPDATE( spyhunt );

VIDEO_START( dotron );
VIDEO_UPDATE( dotron );
void dotron_change_light(int light);


/*----------- defined in vidhrdw/mcr68.c -----------*/

extern UINT8 mcr68_sprite_clip;
extern INT8 mcr68_sprite_xoffset;

WRITE16_HANDLER( mcr68_paletteram_w );
WRITE16_HANDLER( mcr68_videoram_w );

VIDEO_UPDATE( mcr68 );

WRITE16_HANDLER( zwackery_paletteram_w );
WRITE16_HANDLER( zwackery_videoram_w );
WRITE16_HANDLER( zwackery_spriteram_w );

PALETTE_INIT( zwackery );
VIDEO_UPDATE( zwackery );
