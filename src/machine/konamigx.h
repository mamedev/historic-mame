#ifndef __MACH_KONAMIGX_H
#define __MACH_KONAMIGX_H

void tms57002_init(void);

READ_HANDLER( tms57002_data_r );
READ_HANDLER( tms57002_status_r );
WRITE_HANDLER( tms57002_control_w );
WRITE_HANDLER( tms57002_data_w );

READ16_HANDLER( tms57002_data_word_r );
READ16_HANDLER( tms57002_status_word_r );
WRITE16_HANDLER( tms57002_control_word_w );
WRITE16_HANDLER( tms57002_data_word_w );



// 2nd-Tier GX/MW Hardware Functions
void K053936GP_set_offset(int chip, int xoffs, int yoffs);
void K053936GP_clip_enable(int chip, int status);
void K053936GP_set_cliprect(int chip, int minx, int maxx, int miny, int maxy);
void K053936GP_0_zoom_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect, struct tilemap *tilemap, int tilebpp, int blend);
void K053936GP_1_zoom_draw(struct mame_bitmap *bitmap, const struct rectangle *cliprect, struct tilemap *tilemap, int tilebpp, int blend);

void K054338_fill_backcolor(struct mame_bitmap *bitmap, int mode); // unified fill, 0=solid, 1=gradient
void K054338_update_all_shadows(void);
int  K054338_set_alpha_level(int pblend);



// 1st-Tier GX/MW Variables and Functions
extern int konamigx_cfgport;
extern data8_t  konamigx_wrport1_0, konamigx_wrport1_1;
extern data16_t konamigx_wrport2;

// Tile/Sprite Decoders and Callbacks
int K055555GX_decode_vmixcolor(int layer, int *color);
int K055555GX_decode_osmixcolor(int layer, int *color);
void konamigx_type2_sprite_callback(int *code, int *color, int *priority);
void konamigx_dragoonj_sprite_callback(int *code, int *color, int *priority);
void konamigx_salmndr2_sprite_callback(int *code, int *color, int *priority);

// Final Mixer and Layer Blend Flags
/*
	flags composition:
	fedcba9876543210fedcba9876543210
	--------------------FFEEDDCCBBAA (layer A-F blend modes)
	----------------DCBA------------ (layer A-D line/row scroll disables)
	----FFEEDDCCBBAA---------------- (layer A-F mix codes in forced blending)
	---x---------------------------- (disable shadows)
	--x----------------------------- (disable z-buffering)
*/
#define GXMIX_BLEND_AUTO  0       // emulate all blend effects
#define GXMIX_BLEND_NONE  1       // disable all blend effects
#define GXMIX_BLEND_FAST  2       // simulate translucency
#define GXMIX_BLEND_FORCE 3       // force mix code on selected layer(s)
#define GXMIX_NOLINESCROLL 0x1000 // disable linescroll on selected layer(s)
#define GXMIX_NOSHADOW 0x10000000 // disable all shadows (shadow pens will be skipped)
#define GXMIX_NOZBUF   0x20000000 // disable z-buffering (shadow pens will be drawn as solid)

int konamigx_mixer_init(int objdma);
void konamigx_mixer_primode(int mode);
void konamigx_mixer(struct mame_bitmap *bitmap, const struct rectangle *cliprect,
					struct tilemap *sub1, int sub1bpp, struct tilemap *sub2, int sub2bpp, int flags);

void konamigx_objdma(void);


// K055550/K053990/ESC protection devices handlers
READ16_HANDLER ( K055550_word_r );
WRITE16_HANDLER( K055550_word_w );
WRITE16_HANDLER( K053990_martchmp_word_w );
void konamigx_esc_alert(UINT32 *srcbase, int srcoffs, int count, int mode);
#endif
