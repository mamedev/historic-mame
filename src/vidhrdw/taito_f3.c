/***************************************************************************

   Taito F3 Video emulation - Bryan McPhail, mish@tendril.co.uk

****************************************************************************

Brief overview:

	4 scrolling layers (512x512 or 1024x512) of 4/5/6 bpp tiles.
	1 scrolling text layer (512x512, characters generated in vram), 4bpp chars.
	1 scrolling pixel layer (512x256 pixels generated in pivot ram), 4bpp pixels.
	2 sprite banks (for double buffering of sprites)
	Sprites can be 4, 5 or 6 bpp
	Sprite scaling.
	Rowscroll on all playfields
	Line by line zoom on all playfields
	Column scroll on all playfields
	Line by line sprite and playfield priority mixing

Notes:
	All special effects are controlled by an area in 'line ram'.  Typically
	there are 256 values, one for each line of the screen (including clipped
	lines at the top of the screen).  For example, at 0x8000 in lineram,
	there are 4 sets of 256 values (one for each playfield) and each value
	is the scale control for that line in the destination bitmap (screen).
	Therefore each line can have a different zoom value for special effects.

	This also applies to playfield priority, rowscroll, column scroll, sprite
	priority and VRAM/pivot layers.

	However - at the start of line ram there are also sets of 256 values
	controlling each effect - effects can be selectively applied to individual
	playfields or only certain lines out of the 256 can be active - in which
	case the last allowed value can be latched (typically used so a game can
	use one zoom or row value over the whole playfield).

	The programmers of some of these games made strange use of flipscreen -
	some games have all their graphics flipped in ROM, and use the flipscreen
	bit to display them correctly!.

	Most games display 232 scanlines, but some use lineram effects to clip
	themselves to 224 or less.

****************************************************************************

Line ram memory map:

	Here 'playfield 1' refers to the first playfield in memory, etc

	0x0000: Column line control ram (256 lines)
		100x:	Where bit 0 of x enables effect on playfield 1
				Where bit 1 of x enables effect on playfield 2
				Where bit 2 of x enables effect on playfield 3
				Where bit 3 of x enables effect on playfield 4
	0x0200: Line control ram for 0x5000 section.
	0x0400: Line control ram for 0x6000 section.
		(Alpha control)
	0x0600: Sprite control ram
		1c0x:	Where x enables sprite control word for that line
	0x0800: Zoom line control ram (256 lines)
		200x:	Where bit 0 of x enables effect on playfield 1
				Where bit 1 of x enables effect on playfield 2
				Where bit 2 of x enables effect on playfield 3
				Where bit 3 of x enables effect on playfield 4
	0x0a00: Assumed unused.
	0x0c00: Rowscroll line control ram (256 lines)
		280x:	Where bit 0 of x enables effect on playfield 1
				Where bit 1 of x enables effect on playfield 2
				Where bit 2 of x enables effect on playfield 3
				Where bit 3 of x enables effect on playfield 4
	0x0e00: Priority line control ram (256 lines)
		2c0x:	Where bit 0 of x enables effect on playfield 1
				Where bit 1 of x enables effect on playfield 2
				Where bit 2 of x enables effect on playfield 3
				Where bit 3 of x enables effect on playfield 4

	0x4000: Playfield 1 column scroll (on source bitmap, not destination)
	0x4200: Playfield 2 column scroll (on source bitmap, not destination)
	0x4400: Playfield 3 column scroll (on source bitmap, not destination)
	0x4600: Playfield 4 column scroll (on source bitmap, not destination)

	0x5000: ?????

	0x6000:	Pivot layer control
		Cupfinal a255 display, 0255 don't
		Bubsymph a255 display  0255 don't
		Pbobbl4u a2ff display (colour 0xa) 02ff don't
		Landmakr 00df dont display
		qtheater 01ff dont display
		0x00p0 - p = priority? (See BubSymph continue screen)
	0x6200: Alpha blending control
		Cupfinal 3000
		Landmakr bbbb
		qtheater bbbb
		bubsymph 3000
	0x6400: Pivot layer control?
		Cupfinal 7000
		landmakr 7000
		qtheater 7000
		bubsymph 7000
		pbobbl4u 7000
	0x6600: Always zero?

	0x7000: ?
	0x7200: ?
	0x7400: ?
	0x7600: Sprite priority values
		0xf000:	Relative priority for sprites with pri value 0xc0
		0x0f00:	Relative priority for sprites with pri value 0x80
		0x00f0:	Relative priority for sprites with pri value 0x40
		0x000f:	Relative priority for sprites with pri value 0x00

	0x8000: Playfield 1 scale (1 word per line, 256 lines, 0x80 = no scale)
	0x8200: Playfield 2 scale
	0x8400: Playfield 3 scale
	0x8600: Playfield 4 scale
		0x0080 = No scale
		< 0x80 = Zoom Out
		> 0x80 = Zoom in

	0xa000: Playfield 1 rowscroll (1 word per line, 256 lines)
	0xa200: Playfield 2 rowscroll
	0xa400: Playfield 3 rowscroll
	0xa600: Playfield 4 rowscroll

	0xb000: Playfield 1 priority (1 word per line, 256 lines)
	0xb200: Playfield 2 priority
	0xb400: Playfield 3 priority
	0xb600: Playfield 4 priority
		0xf000 = Disable playfield (ElvAct2 second level)
		0x8000 = Enable alpha-blending for this line
		0x4000 = Playfield can be alpha-blended against?  (Otherwise, playfield shouldn't be in blend calculation?)
		0x2000 = Enable line? (Darius Gaiden 0x5000 = disable, 0x3000, 0xb000 & 0x7000 = display)
		0x1000 = ?
		0x0800 = Disable line (Used by KTiger2 to clip screen)
		0x07f0 = ?
		0x000f = Playfield priority

	0xc000 - 0xffff: Unused.

	When sprite priority==playfield priority sprite takes precedence (Bubble Symph title)

****************************************************************************

	F3 sprite format:

	Word 0:	0xffff		Tile number (LSB)
	Word 1:	0xff00		X zoom
			0x00ff		Y zoom
	Word 2:	0x03ff		X position
	Word 3:	0x03ff		Y position
	Word 4:	0xf000		Sprite block controls
			0x0800		Sprite block start
			0x0400		Use same colour on this sprite as block start
			0x0200		Y flip
			0x0100		X flip
			0x00ff		Colour
	Word 5: 0xffff		Tile number (MSB), probably only low bits used
	Word 6:	0x8000		If set, jump to sprite location in low bits
			0x03ff		Location to jump to.
	Word 7: 0xffff		Unused?  Always zero?

****************************************************************************

	Playfield control information (0x660000-1f):

	Word 0- 3: X scroll values for the 4 playfields.
	Word 4- 7: Y scroll values for the 4 playfields.
	Word 8-11: Unused.  Always zero.
	Word   12: Pixel + VRAM playfields X scroll
	Word   13: Pixel + VRAM playfields Y scroll
	Word   14: Unused. Always zero.
	Word   15: If set to 0x80, then 1024x512 playfields are used, else 512x512

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "taito_f3.h"
#include "state.h"

#define DARIUSG_KLUDGE
#define DEBUG_F3 0

static struct tilemap *pf1_tilemap,*pf2_tilemap,*pf3_tilemap,*pf4_tilemap;
static struct tilemap *pixel_layer;
static data32_t *spriteram32_buffered;
static int vram_dirty[256];
static int pivot_changed,vram_changed,scroll_kludge_y,scroll_kludge_x;
static data32_t f3_control_0[8];
static data32_t f3_control_1[8];
static int flipscreen;

static UINT8 *pivot_dirty;
static int pf23_y_kludge;
static struct rectangle pixel_layer_clip;

static data32_t *f3_pf_data_1,*f3_pf_data_2,*f3_pf_data_3,*f3_pf_data_4;

data32_t *f3_vram,*f3_line_ram;
data32_t *f3_pf_data,*f3_pivot_ram;

extern int f3_game;
#if DEBUG_F3
static int sprite_pri_word;
#endif
static int scroll_dirty,skip_this_frame;

/* Game specific data, some of this can be
removed when the software values are figured out */
struct F3config
{
	int name;
	int extend;
	int sx;
	int sy;
	int pivot;
	int sprite_lag;//ks
};

const struct F3config *f3_game_config;
//ks s
static const struct F3config f3_config_table[] =
{
	/* Name    Extend  X Offset, Y Offset  Flip Pivot Sprite Lag */  /* Uses 5000, uses control bits,works with line X zoom */
/**/{ RINGRAGE,  0,      0,        -30,         0,          2    }, //no,no,no
	{ ARABIANM,  0,      0,        -30,         0,          2    }, //ff00 in 5000, no,yes
/**/{ RIDINGF,   1,      0,        -30,         0,          1    }, //yes,yes,yes
	{ GSEEKER,   0,      1,        -30,         0,          1    }, //yes,yes,yes
	{ TRSTAR,    1,      0,          0,         0,          0    }, //
	{ GUNLOCK,   1,      1,        -30,         0,          2    }, //yes,yes,partial
/**/{ TWINQIX,   1,      0,        -30,         1,          1    },
	{ SCFINALS,  0,      0,        -30,         1,          1/**/},
	{ LIGHTBR,   1,      0,        -30,         0,          2    }, //yes,?,no
	{ KAISERKN,  0,      0,        -30,         1,          2    },
	{ DARIUSG,   0,      0,        -23,         0,          2    },
	{ BUBSYMPH,  1,      0,        -30,         0,          1/**/}, //yes,yes,?
	{ SPCINVDX,  1,      0,        -30,         1,          1/**/}, //yes,yes,?
	{ QTHEATER,  1,      0,          0,         0,          1/**/},
	{ HTHERO95,  0,      0,        -30,         1,          1/**/},
	{ SPCINV95,  0,      0,        -30,         0,          1/**/},
	{ EACTION2,  1,      0,        -23,         0,          2    }, //no,yes,?
	{ QUIZHUHU,  1,      0,        -23,         0,          1/**/},
	{ PBOBBLE2,  0,      0,          0,         1,          1/**/}, //no,no,?
	{ GEKIRIDO,  0,      0,        -23,         0,          1    },
	{ KTIGER2,   0,      0,        -23,         0,          0    },//no,yes,partial
	{ BUBBLEM,   1,      0,        -30,         0,          1/**/},
	{ CLEOPATR,  0,      0,        -30,         0,          1/**/},
	{ PBOBBLE3,  0,      0,          0,         0,          1/**/},
	{ ARKRETRN,  1,      0,        -23,         0,          1/**/},
	{ KIRAMEKI,  0,      0,        -30,         0,          1/**/},
	{ PUCHICAR,  1,      0,        -23,         0,          1/**/},
	{ PBOBBLE4,  0,      0,          0,         1,          1/**/},
	{ POPNPOP,   1,      0,        -23,         0,          1/**/},
	{ LANDMAKR,  1,      0,        -23,         0,          1/**/},
	{0}
};
//ks e

struct tempsprite
{
	int code,color;
	int flipx,flipy;
	int x,y;
	int zoomx,zoomy;
	int pri;	//ks
};
static struct tempsprite *spritelist;

//ks s
//ksdeb s
static int deb_enable=0;  //TODO
static int deb_alpha_disable=0;
static int deb_tileflag=0;
static int deb_tile_code=0;
//ksdeb e

static const struct tempsprite *sprite_end;
static void get_sprite_info(const data32_t *spriteram32_ptr);
static struct mame_bitmap *sprite_bitmap;
static int sprite_lag=1;

struct f3_line_inf
{
	int alpha_mode[256];
	int alpha_level[256];
	int pri[256];
	int spri[256];

	/* use for draw_scanlines */
	UINT16 *src[256],*src_s[256],*src_e[256];
	UINT8 *tsrc[256],*tsrc_s[256],*tsrc_e[256];
	int src_inc,tsrc_inc;
	int x_count[256];
	UINT32 x_zoom[256];
};
static struct f3_line_inf *line_inf;
static struct mame_bitmap *pri_alp_bitmap;
/*
pri_alp_bitmap
---- --xx    sprite priority 0-3
---- -1--    sprite
---- 1---    alpha level a 7000
---1 ----    alpha level b 7000
--1- ----    alpha level a b000
-1-- ----    alpha level b b000
1111 1111    opaque pixcel
*/
static int width_mask=0x1ff;
static int f3_alpha_level_2as=127;
static int f3_alpha_level_2ad=127;
static int f3_alpha_level_3as=127;
static int f3_alpha_level_3ad=127;
static int f3_alpha_level_2bs=127;
static int f3_alpha_level_2bd=127;
static int f3_alpha_level_3bs=127;
static int f3_alpha_level_3bd=127;
//ks e

/******************************************************************************/

#if DEBUG_F3
static void print_debug_info(int t0, int t1, int t2, int t3, int c0, int c1, int c2, int c3)
{
	struct mame_bitmap *bitmap = Machine->scrbitmap;
	int j,trueorientation,l[16];
	char buf[64];

	trueorientation = Machine->orientation;
	Machine->orientation = ROT0;

	sprintf(buf,"%04X %04X %04X %04X",f3_control_0[0]>>22,(f3_control_0[0]&0xffff)>>6,f3_control_0[1]>>22,(f3_control_0[1]&0xffff)>>6);
	for (j = 0;j< 16+3;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,40,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"%04X %04X %04X %04X",f3_control_0[2]>>23,(f3_control_0[2]&0xffff)>>7,f3_control_0[3]>>23,(f3_control_0[3]&0xffff)>>7);
	for (j = 0;j< 16+3;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,48,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"%04X %04X %04X %04X",f3_control_1[0]>>16,f3_control_1[0]&0xffff,f3_control_1[1]>>16,f3_control_1[1]&0xffff);
	for (j = 0;j< 16+3;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,58,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"%04X %04X %04X %04X",f3_control_1[2]>>16,f3_control_1[2]&0xffff,f3_control_1[3]>>16,f3_control_1[3]&0xffff);
	for (j = 0;j< 16+3;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,66,0,TRANSPARENCY_NONE,0);

	sprintf(buf,"%04X %04X %04X %04X %04X %04X %04X %04X",spriteram32_buffered[0]>>16,spriteram32_buffered[0]&0xffff,spriteram32_buffered[1]>>16,spriteram32_buffered[1]&0xffff,spriteram32_buffered[2]>>16,spriteram32_buffered[2]&0xffff,spriteram32_buffered[3]>>16,spriteram32_buffered[3]&0xffff);
	for (j = 0;j< 32+7;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,76,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"%04X %04X %04X %04X %04X %04X %04X %04X",spriteram32_buffered[4]>>16,spriteram32_buffered[4]&0xffff,spriteram32_buffered[5]>>16,spriteram32_buffered[5]&0xffff,spriteram32_buffered[6]>>16,spriteram32_buffered[6]&0xffff,spriteram32_buffered[7]>>16,spriteram32_buffered[7]&0xffff);
	for (j = 0;j< 32+7;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,84,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"%04X %04X %04X %04X %04X %04X %04X %04X",spriteram32_buffered[8]>>16,spriteram32_buffered[8]&0xffff,spriteram32_buffered[9]>>16,spriteram32_buffered[9]&0xffff,spriteram32_buffered[10]>>16,spriteram32_buffered[10]&0xffff,spriteram32_buffered[11]>>16,spriteram32_buffered[11]&0xffff);
	for (j = 0;j< 32+7;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,92,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x0040]&0xffff;
	l[1]=f3_line_ram[0x00c0]&0xffff;
	l[2]=f3_line_ram[0x0140]&0xffff;
	l[3]=f3_line_ram[0x01c0]&0xffff;
	sprintf(buf,"Ctr1: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*13,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x0240]&0xffff;
	l[1]=f3_line_ram[0x02c0]&0xffff;
	l[2]=f3_line_ram[0x0340]&0xffff;
	l[3]=f3_line_ram[0x03c0]&0xffff;
	sprintf(buf,"Ctr2: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*14,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x2c60]&0xffff;
	l[1]=f3_line_ram[0x2ce0]&0xffff;
	l[2]=f3_line_ram[0x2d60]&0xffff;
	l[3]=f3_line_ram[0x2de0]&0xffff;
	sprintf(buf,"Pri : %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*15,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x2060]&0xffff;
	l[1]=f3_line_ram[0x20e0]&0xffff;
	l[2]=f3_line_ram[0x2160]&0xffff;
	l[3]=f3_line_ram[0x21e0]&0xffff;
	sprintf(buf,"Zoom: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*16,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x2860]&0xffff;
	l[1]=f3_line_ram[0x28e0]&0xffff;
	l[2]=f3_line_ram[0x2960]&0xffff;
	l[3]=f3_line_ram[0x29e0]&0xffff;
	sprintf(buf,"Line: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*17,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x1c60]&0xffff;
	l[1]=f3_line_ram[0x1ce0]&0xffff;
	l[2]=f3_line_ram[0x1d60]&0xffff;
	l[3]=f3_line_ram[0x1de0]&0xffff;
	sprintf(buf,"Sprt: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*18,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x1860]&0xffff;
	l[1]=f3_line_ram[0x18e0]&0xffff;
	l[2]=f3_line_ram[0x1960]&0xffff;
	l[3]=f3_line_ram[0x19e0]&0xffff;
	sprintf(buf,"Pivt: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*19,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x1060]&0xffff;
	l[1]=f3_line_ram[0x10e0]&0xffff;
	l[2]=f3_line_ram[0x1160]&0xffff;
	l[3]=f3_line_ram[0x11e0]&0xffff;
	sprintf(buf,"Colm: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*20,0,TRANSPARENCY_NONE,0);

	l[0]=f3_line_ram[0x1460]&0xffff;
	l[1]=f3_line_ram[0x14e0]&0xffff;
	l[2]=f3_line_ram[0x1560]&0xffff;
	l[3]=f3_line_ram[0x15e0]&0xffff;
	sprintf(buf,"5000: %04x %04x %04x %04x",l[0],l[1],l[2],l[3]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*21,0,TRANSPARENCY_NONE,0);

	sprintf(buf,"SPri: %04x",sprite_pri_word);
	for (j = 0;j < 10; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*23,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"TPri: %04x %04x %04x %04x",t0,t1,t2,t3);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*24,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"Cstm: %04x %04x %04x %04x",c0,c1,c2,c3);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*25,0,TRANSPARENCY_NONE,0);
	sprintf(buf,"6000: %08x %08x %08x",f3_line_ram[0x1800],f3_line_ram[0x1890],f3_line_ram[0x1910]);
	for (j = 0;j < 16+9; j++)
		drawgfx(bitmap,Machine->uifont,buf[j],0,0,0,60+6*j,8*27,0,TRANSPARENCY_NONE,0);

	Machine->orientation = trueorientation;
}
#endif

/******************************************************************************/

INLINE void get_tile_info(int tile_index, data32_t *gfx_base)
{
	data32_t tile=gfx_base[tile_index];
//ksdeb s
if(deb_tileflag)
{
	int c=tile&0xffff;
	if(((tile>>(16+9))&0x1f)==deb_tile_code) c=0;

	SET_TILE_INFO(
			1,
			c,
			(tile>>16)&0x1ff,
			TILE_FLIPYX( tile >> 30 ))
	tile_info.priority = (tile>>(16+9))&0xf;	/* alpha blending type? */	//ks
}
else
{
	SET_TILE_INFO(
			1,
			tile&0xffff,
			(tile>>16)&0x1ff,
			TILE_FLIPYX( tile >> 30 ))
	tile_info.priority = (tile>>(16+9))&0xf;	/* alpha blending type? */	//ks
}
//ksdeb e
}

static void get_tile_info1(int tile_index)
{
	get_tile_info(tile_index,f3_pf_data_1);
}

static void get_tile_info2(int tile_index)
{
	get_tile_info(tile_index,f3_pf_data_2);
}

static void get_tile_info3(int tile_index)
{
	get_tile_info(tile_index,f3_pf_data_3);
}

static void get_tile_info4(int tile_index)
{
	get_tile_info(tile_index,f3_pf_data_4);
}

static void get_tile_info_pixel(int tile_index)
{
	int color,col_off;
	int y_offs=(f3_control_1[2]&0x1ff)+scroll_kludge_y;

	if (flipscreen) y_offs+=0x100;

	/* Colour is shared with VRAM layer */
	if ((((tile_index%32)*8 + y_offs)&0x1ff)>0xff)
		col_off=0x800+((tile_index%32)*0x40)+((tile_index&0xfe0)>>5);
	else
		col_off=((tile_index%32)*0x40)+((tile_index&0xfe0)>>5);

	if (col_off&1)
	   	color = ((videoram32[col_off>>1]&0xffff)>>9)&0x3f;
	else
		color = ((videoram32[col_off>>1]>>16)>>9)&0x3f;

	SET_TILE_INFO(
			3,
			tile_index,
			color&0x3f,
			0)
	tile_info.flags = f3_game_config->pivot ? TILE_FLIPX : 0;
}

/******************************************************************************/

VIDEO_EOF( f3 )
{
	if (sprite_lag==2)
	{
		if (osd_skip_this_frame() == 0)
		{
			get_sprite_info(spriteram32_buffered);
		}
		memcpy(spriteram32_buffered,spriteram32,spriteram_size);
	}
	else if (sprite_lag==1)
	{
		if (osd_skip_this_frame() == 0)
		{
			get_sprite_info(spriteram32);
		}
	}
}

VIDEO_STOP( f3 )
{
#define FWRITE32(pRAM,len,file)	\
{								\
	int i;						\
	for(i=0;i<len;i++)			\
	{							\
		unsigned char c;		\
		UINT32 d=*pRAM;			\
		c=(d&0xff000000)>>24;	\
		fwrite(&c,1, 1 ,file);	\
		d<<=8;					\
		c=(d&0xff000000)>>24;	\
		fwrite(&c,1, 1 ,file);	\
		d<<=8;					\
		c=(d&0xff000000)>>24;	\
		fwrite(&c,1, 1 ,file);	\
		d<<=8;					\
		c=(d&0xff000000)>>24;	\
		fwrite(&c,1, 1 ,file);	\
		pRAM++;					\
	}							\
}

	if (/*DEBUG_F3*/deb_enable) {//ksdeb
		FILE *fp;

		fp=fopen("line.dmp","wb");
		if (fp) {
			FWRITE32(f3_line_ram,0x10000/4,fp);
			fclose(fp);
		}
		fp=fopen("sprite.dmp","wb");
		if (fp) {
			FWRITE32(spriteram32,0x10000/4,fp);
			fclose(fp);
		}
		fp=fopen("vram.dmp","wb");
		if (fp) {
			FWRITE32(f3_pf_data,0xc000/4,fp);
			fclose(fp);
		}
	}
#undef FWRITE32
}

VIDEO_START( f3 )
{
	const struct F3config *pCFG=&f3_config_table[0];
	int tile;

	spritelist=0;
	spriteram32_buffered=0;
	pivot_dirty=0;
	pixel_layer_clip=Machine->visible_area;
	line_inf=0;			//ks
	pri_alp_bitmap=0;	//ks
	sprite_bitmap=0;	//ks

	/* Setup individual game */
	do {
		if (pCFG->name==f3_game)
		{
			break;
		}
		pCFG++;
	} while(pCFG->name);

	f3_game_config=pCFG;

	if (f3_game_config->extend) {
		pf1_tilemap = tilemap_create(get_tile_info1,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
		pf2_tilemap = tilemap_create(get_tile_info2,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
		pf3_tilemap = tilemap_create(get_tile_info3,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);
		pf4_tilemap = tilemap_create(get_tile_info4,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,64,32);

		f3_pf_data_1=f3_pf_data+0x0000;
		f3_pf_data_2=f3_pf_data+0x0800;
		f3_pf_data_3=f3_pf_data+0x1000;
		f3_pf_data_4=f3_pf_data+0x1800;
		width_mask=0x3ff;
	} else {
		pf1_tilemap = tilemap_create(get_tile_info1,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
		pf2_tilemap = tilemap_create(get_tile_info2,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
		pf3_tilemap = tilemap_create(get_tile_info3,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
		pf4_tilemap = tilemap_create(get_tile_info4,tilemap_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);

		f3_pf_data_1=f3_pf_data+0x0000;
		f3_pf_data_2=f3_pf_data+0x0400;
		f3_pf_data_3=f3_pf_data+0x0800;
		f3_pf_data_4=f3_pf_data+0x0c00;
		width_mask=0x1ff;
	}

	spriteram32_buffered = (UINT32 *)auto_malloc(0x10000);
	spritelist = auto_malloc(0x400 * sizeof(*spritelist));
	sprite_end = spritelist;
	pixel_layer = tilemap_create(get_tile_info_pixel,tilemap_scan_cols,TILEMAP_TRANSPARENT,8,8,64,32);
	pivot_dirty = (UINT8 *)auto_malloc(2048);
	line_inf = auto_malloc(4 * sizeof(struct f3_line_inf));//ks
	pri_alp_bitmap = auto_bitmap_alloc_depth( Machine->scrbitmap->width, Machine->scrbitmap->height, -8 );//ks
	sprite_bitmap = auto_bitmap_alloc_depth( Machine->scrbitmap->width, Machine->scrbitmap->height, -16 );//ks

	if (!pf1_tilemap || !pf2_tilemap || !pf3_tilemap || !pf4_tilemap || !line_inf || !pri_alp_bitmap					//ks
		 || !spritelist || !pixel_layer || !spriteram32_buffered || !pivot_dirty || !sprite_bitmap)	//ks
		return 1;

	tilemap_set_transparent_pen(pf1_tilemap,0);
	tilemap_set_transparent_pen(pf2_tilemap,0);
	tilemap_set_transparent_pen(pf3_tilemap,0);
	tilemap_set_transparent_pen(pf4_tilemap,0);
	tilemap_set_transparent_pen(pixel_layer,0);

	tilemap_set_scroll_rows(pf1_tilemap,512);
	tilemap_set_scroll_rows(pf2_tilemap,512);
	tilemap_set_scroll_rows(pf3_tilemap,512);
	tilemap_set_scroll_rows(pf4_tilemap,512);

	/* Y Offset is related to the first visible line in line ram in some way */
	scroll_kludge_y=f3_game_config->sy;
	scroll_kludge_x=f3_game_config->sx;

	/* Palettes have 4 bpp indexes despite up to 6 bpp data */
	Machine->gfx[1]->color_granularity=16;
	Machine->gfx[2]->color_granularity=16;

	flipscreen = 0;
	memset(spriteram32_buffered,0,spriteram_size);
	memset(spriteram32,0,spriteram_size);

	state_save_register_UINT32("f3", 0, "vcontrol0", f3_control_0, 8);
	state_save_register_UINT32("f3", 0, "vcontrol1", f3_control_1, 8);

	/* Why?!??  These games have different offsets for the two middle playfields only */
	if (f3_game==LANDMAKR || f3_game==EACTION2 || f3_game==DARIUSG || f3_game==GEKIRIDO)
		pf23_y_kludge=23;
	else
		pf23_y_kludge=0;

	for (tile = 0;tile < 256;tile++)
		vram_dirty[tile]=1;
	for (tile = 0;tile < 2048;tile++)
		pivot_dirty[tile]=1;

	scroll_dirty=1;
	skip_this_frame=0;

	sprite_lag=f3_game_config->sprite_lag;

	return 0;
}

/******************************************************************************/

WRITE32_HANDLER( f3_pf_data_w )
{
	COMBINE_DATA(&f3_pf_data[offset]);

	if (f3_game_config->extend) {
		if (offset<0x800) tilemap_mark_tile_dirty(pf1_tilemap,offset-0x0000);
		else if (offset<0x1000) tilemap_mark_tile_dirty(pf2_tilemap,offset-0x0800);
		else if (offset<0x1800) tilemap_mark_tile_dirty(pf3_tilemap,offset-0x1000);
		else if (offset<0x2000) tilemap_mark_tile_dirty(pf4_tilemap,offset-0x1800);
	} else {
		if (offset<0x400) tilemap_mark_tile_dirty(pf1_tilemap,offset-0x0000);
		else if (offset<0x800) tilemap_mark_tile_dirty(pf2_tilemap,offset-0x0400);
		else if (offset<0xc00) tilemap_mark_tile_dirty(pf3_tilemap,offset-0x0800);
		else if (offset<0x1000) tilemap_mark_tile_dirty(pf4_tilemap,offset-0xc00);
	}
}

WRITE32_HANDLER( f3_control_0_w )
{
	COMBINE_DATA(&f3_control_0[offset]);
}

WRITE32_HANDLER( f3_control_1_w )
{
	COMBINE_DATA(&f3_control_1[offset]);
}

WRITE32_HANDLER( f3_videoram_w )
{
	int tile,col_off;
	COMBINE_DATA(&videoram32[offset]);

	if (offset>0x3ff) offset-=0x400;

	tile=offset<<1;
	col_off=((tile&0x3f)*32)+((tile&0xfc0)>>6);

	tilemap_mark_tile_dirty(pixel_layer,col_off);
	tilemap_mark_tile_dirty(pixel_layer,col_off+32);
}

WRITE32_HANDLER( f3_vram_w )
{
	COMBINE_DATA(&f3_vram[offset]);
	vram_dirty[offset/8]=1;
	vram_changed=1;
}

WRITE32_HANDLER( f3_pivot_w )
{
	COMBINE_DATA(&f3_pivot_ram[offset]);
	pivot_dirty[offset/8]=1;
	pivot_changed=1;
}

WRITE32_HANDLER( f3_lineram_w )
{
	/* DariusGX has an interesting bug at the start of Round D - the clearing of lineram
	(0xa000->0x0xa7ff) overflows into priority RAM (0xb000) and creates garbage priority
	values.  I'm not sure what the real machine would do with these values, and this
	emulation certainly doesn't like it, so I've chosen to catch the bug here, and prevent
	the trashing of priority ram.  If anyone has information on what the real machine does,
	please let me know! */
	if (f3_game==DARIUSG) {
		if (skip_this_frame)
			return;
		if (offset==0xb000/4 && data==0x003f0000) {
			skip_this_frame=1;
			return;
		}
	}

	COMBINE_DATA(&f3_line_ram[offset]);

//	if ((offset&0xfe00)==0x2800)
//		scroll_dirty=1;

//	if (offset>=0x6000/4 && offset<0x7000/4)
//	if (offset==0x18c0)
//		logerror("%08x:  Write 6000 %08x, %08x\n",activecpu_get_pc(),offset,data);
//	if (offset>=0xa000/4 && offset<0xb000/4)
//		logerror("%08x:  Write a000 %08x, %08x\n",activecpu_get_pc(),offset,data);
//	if (offset>=0xb000/4 && offset<0xc000/4)
//		logerror("%08x:  Write b000 %08x, %08x\n",activecpu_get_pc(),offset,data);

}

WRITE32_HANDLER( f3_palette_24bit_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram32[offset]);

	/* 12 bit palette games - there has to be a palette select bit somewhere */
	if (f3_game==SPCINVDX || f3_game==RIDINGF || f3_game==ARABIANM || f3_game==RINGRAGE) {
		b = 15 * ((paletteram32[offset] >> 4) & 0xf);
		g = 15 * ((paletteram32[offset] >> 8) & 0xf);
		r = 15 * ((paletteram32[offset] >> 12) & 0xf);
	}

	/* This is weird - why are only the sprites and VRAM palettes 21 bit? */
	else if (f3_game==CLEOPATR) {
		if (offset<0x100 || offset>0x1000) {
		 	r = ((paletteram32[offset] >>16) & 0x7f)<<1;
			g = ((paletteram32[offset] >> 8) & 0x7f)<<1;
			b = ((paletteram32[offset] >> 0) & 0x7f)<<1;
		} else {
		 	r = (paletteram32[offset] >>16) & 0xff;
			g = (paletteram32[offset] >> 8) & 0xff;
			b = (paletteram32[offset] >> 0) & 0xff;
		}
	}

	/* Another weird one */
	else if (f3_game==TWINQIX) {
		if (offset>0x1c00) {
		 	r = ((paletteram32[offset] >>16) & 0x7f)<<1;
			g = ((paletteram32[offset] >> 8) & 0x7f)<<1;
			b = ((paletteram32[offset] >> 0) & 0x7f)<<1;
		} else {
		 	r = (paletteram32[offset] >>16) & 0xff;
			g = (paletteram32[offset] >> 8) & 0xff;
			b = (paletteram32[offset] >> 0) & 0xff;
		}
	}

	/* All other games - standard 24 bit palette */
	else {
	 	r = (paletteram32[offset] >>16) & 0xff;
		g = (paletteram32[offset] >> 8) & 0xff;
		b = (paletteram32[offset] >> 0) & 0xff;
	}

	palette_set_color(offset,r,g,b);
}

//ks s
/******************************************************************************/
/* alpha_blend */
//ksdebA s
#if DEBUG_F3
static int deb_alpha_level_a=0;
static int deb_alpha_level_b=0;
static int deb_alp_mode=0;
#endif
static int deb_loop=0;
//ksdebA e

static int alpha_limit=1;

static const UINT8 *alpha_s_1_1;
static const UINT8 *alpha_s_1_2;
static const UINT8 *alpha_s_1_4;
static const UINT8 *alpha_s_1_5;
static const UINT8 *alpha_s_1_6;
static const UINT8 *alpha_s_1_8;
static const UINT8 *alpha_s_1_9;
static const UINT8 *alpha_s_1_a;

static const UINT8 *alpha_s_2a_0;
static const UINT8 *alpha_s_2a_4;
static const UINT8 *alpha_s_2a_8;

static const UINT8 *alpha_s_2b_0;
static const UINT8 *alpha_s_2b_4;
static const UINT8 *alpha_s_2b_8;

static const UINT8 *alpha_s_3a_0;
static const UINT8 *alpha_s_3a_1;
static const UINT8 *alpha_s_3a_2;

static const UINT8 *alpha_s_3b_0;
static const UINT8 *alpha_s_3b_1;
static const UINT8 *alpha_s_3b_2;


#define SET_ALPHA_LEVEL(d,s)			\
{										\
	int level = s;						\
	if(level == 0) level = -1;			\
	d = alpha_cache.alpha[level+1];		\
}

INLINE void f3_alpha_set_level(void)
{
	alpha_limit = (f3_alpha_level_2as+f3_alpha_level_2ad>255 || f3_alpha_level_2bs+f3_alpha_level_2bd>255);

//	SET_ALPHA_LEVEL(alpha_s_1_1, f3_alpha_level_2ad)
	SET_ALPHA_LEVEL(alpha_s_1_1, 255-f3_alpha_level_2as)
//	SET_ALPHA_LEVEL(alpha_s_1_2, f3_alpha_level_2bd)
	SET_ALPHA_LEVEL(alpha_s_1_2, 255-f3_alpha_level_2bs)
	SET_ALPHA_LEVEL(alpha_s_1_4, f3_alpha_level_3ad)
//	SET_ALPHA_LEVEL(alpha_s_1_5, f3_alpha_level_3ad*f3_alpha_level_2ad/255)
	SET_ALPHA_LEVEL(alpha_s_1_5, f3_alpha_level_3ad*(255-f3_alpha_level_2as)/255)
//	SET_ALPHA_LEVEL(alpha_s_1_6, f3_alpha_level_3ad*f3_alpha_level_2bd/255)
	SET_ALPHA_LEVEL(alpha_s_1_6, f3_alpha_level_3ad*(255-f3_alpha_level_2bs)/255)
	SET_ALPHA_LEVEL(alpha_s_1_8, f3_alpha_level_3bd)
//	SET_ALPHA_LEVEL(alpha_s_1_9, f3_alpha_level_3bd*f3_alpha_level_2ad/255)
	SET_ALPHA_LEVEL(alpha_s_1_9, f3_alpha_level_3bd*(255-f3_alpha_level_2as)/255)
//	SET_ALPHA_LEVEL(alpha_s_1_a, f3_alpha_level_3bd*f3_alpha_level_2bd/255)
	SET_ALPHA_LEVEL(alpha_s_1_a, f3_alpha_level_3bd*(255-f3_alpha_level_2bs)/255)

	SET_ALPHA_LEVEL(alpha_s_2a_0, f3_alpha_level_2as)
	SET_ALPHA_LEVEL(alpha_s_2a_4, f3_alpha_level_2as*f3_alpha_level_3ad/255)
	SET_ALPHA_LEVEL(alpha_s_2a_8, f3_alpha_level_2as*f3_alpha_level_3bd/255)

	SET_ALPHA_LEVEL(alpha_s_2b_0, f3_alpha_level_2bs)
	SET_ALPHA_LEVEL(alpha_s_2b_4, f3_alpha_level_2bs*f3_alpha_level_3ad/255)
	SET_ALPHA_LEVEL(alpha_s_2b_8, f3_alpha_level_2bs*f3_alpha_level_3bd/255)

	SET_ALPHA_LEVEL(alpha_s_3a_0, f3_alpha_level_3as)
	SET_ALPHA_LEVEL(alpha_s_3a_1, f3_alpha_level_3as*f3_alpha_level_2ad/255)
	SET_ALPHA_LEVEL(alpha_s_3a_2, f3_alpha_level_3as*f3_alpha_level_2bd/255)

	SET_ALPHA_LEVEL(alpha_s_3b_0, f3_alpha_level_3bs)
	SET_ALPHA_LEVEL(alpha_s_3b_1, f3_alpha_level_3bs*f3_alpha_level_2ad/255)
	SET_ALPHA_LEVEL(alpha_s_3b_2, f3_alpha_level_3bs*f3_alpha_level_2bd/255)
}
#undef SET_ALPHA_LEVEL


INLINE UINT32 f3_alpha_blend32_0( const UINT8 *alphas, UINT32 s )
{
	return  alphas[s & 0xff] | (alphas[(s>>8) & 0xff]<<8) | (alphas[(s>>16) & 0xff]<<16);
}

INLINE UINT32 f3_alpha_blend32_1( const UINT8 *alphas, UINT32 d, UINT32 s )
{
	if(alpha_limit)
	{
		int c1,c2,c3;
		c1=alphas[s & 0xff]+(d & 0xff);
		if(c1&~0xff) c1=0xff;

		c2=alphas[(s>>8) & 0xff]+((d>>8) & 0xff);
		if(c2&~0xff) c2=0xff;

		c3=alphas[(s>>16) & 0xff]+((d>>16) & 0xff);
		if(c3&~0xff) c3=0xff;

		return  c1 | (c2<<8) | (c3<<16);
	}

	return (alphas[s & 0xff] | (alphas[(s>>8) & 0xff] << 8) | (alphas[(s>>16) & 0xff] << 16))
		+ ((d & 0xff) | (((d>>8) & 0xff) << 8) | (((d>>16) & 0xff) << 16));
}


INLINE UINT32 f3_alpha_blend_1_1( UINT32 d, UINT32 s ){return f3_alpha_blend32_1(alpha_s_1_1,d,s);}
INLINE UINT32 f3_alpha_blend_1_2( UINT32 d, UINT32 s ){return f3_alpha_blend32_1(alpha_s_1_2,d,s);}
INLINE UINT32 f3_alpha_blend_1_4( UINT32 d, UINT32 s ){return f3_alpha_blend32_1(alpha_s_1_4,d,s);}
INLINE UINT32 f3_alpha_blend_1_5( UINT32 d, UINT32 s ){return f3_alpha_blend32_1(alpha_s_1_5,d,s);}
INLINE UINT32 f3_alpha_blend_1_6( UINT32 d, UINT32 s ){return f3_alpha_blend32_1(alpha_s_1_6,d,s);}
INLINE UINT32 f3_alpha_blend_1_8( UINT32 d, UINT32 s ){return f3_alpha_blend32_1(alpha_s_1_8,d,s);}
INLINE UINT32 f3_alpha_blend_1_9( UINT32 d, UINT32 s ){return f3_alpha_blend32_1(alpha_s_1_9,d,s);}
INLINE UINT32 f3_alpha_blend_1_a( UINT32 d, UINT32 s ){return f3_alpha_blend32_1(alpha_s_1_a,d,s);}

INLINE UINT32 f3_alpha_blend_2a_0( UINT32 s ){return f3_alpha_blend32_0(alpha_s_2a_0,s);}
INLINE UINT32 f3_alpha_blend_2a_4( UINT32 d, UINT32 s ){return f3_alpha_blend32_1(alpha_s_2a_4,d,s);}
INLINE UINT32 f3_alpha_blend_2a_8( UINT32 d, UINT32 s ){return f3_alpha_blend32_1(alpha_s_2a_8,d,s);}

INLINE UINT32 f3_alpha_blend_2b_0( UINT32 s ){return f3_alpha_blend32_0(alpha_s_2b_0,s);}
INLINE UINT32 f3_alpha_blend_2b_4( UINT32 d, UINT32 s ){return f3_alpha_blend32_1(alpha_s_2b_4,d,s);}
INLINE UINT32 f3_alpha_blend_2b_8( UINT32 d, UINT32 s ){return f3_alpha_blend32_1(alpha_s_2b_8,d,s);}

INLINE UINT32 f3_alpha_blend_3a_0( UINT32 s ){return f3_alpha_blend32_0(alpha_s_3a_0,s);}
INLINE UINT32 f3_alpha_blend_3a_1( UINT32 d, UINT32 s ){return f3_alpha_blend32_1(alpha_s_3a_1,d,s);}
INLINE UINT32 f3_alpha_blend_3a_2( UINT32 d, UINT32 s ){return f3_alpha_blend32_1(alpha_s_3a_2,d,s);}

INLINE UINT32 f3_alpha_blend_3b_0( UINT32 s ){return f3_alpha_blend32_0(alpha_s_3b_0,s);}
INLINE UINT32 f3_alpha_blend_3b_1( UINT32 d, UINT32 s ){return f3_alpha_blend32_1(alpha_s_3b_1,d,s);}
INLINE UINT32 f3_alpha_blend_3b_2( UINT32 d, UINT32 s ){return f3_alpha_blend32_1(alpha_s_3b_2,d,s);}

/******************************************************************************/
/* draw scanline */
INLINE void f3_drawscanlines(
		struct mame_bitmap *bitmap,int x,int y,int xsize,int ysize,
		const struct f3_line_inf *line_t,
		int transparent,UINT32 orient,
		int *pri_sp,int pri_sl,
		int draw_type)
{
	pen_t *clut = &Machine->remapped_colortable[0];
	int length;
	UINT16 *src,*src_s,*src_e;
	UINT8 *tsrc,*tsrc_s,*tsrc_e;
	UINT32 x_count,x_zoom;
	int src_inc=line_t->src_inc,tsrc_inc=line_t->tsrc_inc;

	UINT32 *dsti0,*dsti;
	UINT8 *dstp0,*dstp;
	UINT16 *srcs0,*srcs;
	int yadv = bitmap->rowpixels;
	int xadv = 1;
	int ty=y;

	if (orient & ORIENTATION_FLIP_Y) {
		ty = bitmap->height - 1 - ty;
		yadv = -yadv;
	}

	dsti0 = (UINT32 *)bitmap->line[ty] + x;
	dstp0 = (UINT8 *)pri_alp_bitmap->line[ty] + x;
	srcs0 = (UINT16 *)sprite_bitmap->line[ty] + x;



	if(draw_type==1)	/* 3000 */
	{
		if (transparent!=TILEMAP_IGNORE_TRANSPARENCY)
		{
			while (ysize--)
			{
				length=xsize;
				dsti = dsti0;
				dstp = dstp0;
				srcs = srcs0;

				src=line_t->src[y];
				src_s=line_t->src_s[y];
				src_e=line_t->src_e[y];
				tsrc=line_t->tsrc[y];
				tsrc_s=line_t->tsrc_s[y];
				tsrc_e=line_t->tsrc_e[y];
				x_count=line_t->x_count[y];
				x_zoom=line_t->x_zoom[y];

				while (length--)
				{
					UINT8 p0=*dstp;
					if (p0!=0xff)
					{
						if(p0&7 && pri_sp[p0&3]>=pri_sl)
						{
							UINT8 p1 = (p0&0x78)>>3;
							if(!p1)				*dsti = clut[*srcs];
							else if(p1==1)	*dsti = f3_alpha_blend_1_1(*dsti, clut[*srcs]);
							else if(p1==2)	*dsti = f3_alpha_blend_1_2(*dsti, clut[*srcs]);
							else if(p1==4)	*dsti = f3_alpha_blend_1_4(*dsti, clut[*srcs]);
							else if(p1==5)	*dsti = f3_alpha_blend_1_5(*dsti, clut[*srcs]);
							else if(p1==6)	*dsti = f3_alpha_blend_1_6(*dsti, clut[*srcs]);
							else if(p1==8)	*dsti = f3_alpha_blend_1_8(*dsti, clut[*srcs]);
							else if(p1==9)	*dsti = f3_alpha_blend_1_9(*dsti, clut[*srcs]);
							else if(p1==10)	*dsti = f3_alpha_blend_1_a(*dsti, clut[*srcs]);
							else usrintf_showmessage("TaitoF3: pri buff error");
							*dstp = 0xff;
						}
						else if((*tsrc)&0xf0)
						{
							UINT8 p1 = (p0&0x78)>>3;
							if(!p1)				*dsti = clut[*src];
							else if(p1==1)	*dsti = f3_alpha_blend_1_1(*dsti, clut[*src]);
							else if(p1==2)	*dsti = f3_alpha_blend_1_2(*dsti, clut[*src]);
							else if(p1==4)	*dsti = f3_alpha_blend_1_4(*dsti, clut[*src]);
							else if(p1==5)	*dsti = f3_alpha_blend_1_5(*dsti, clut[*src]);
							else if(p1==6)	*dsti = f3_alpha_blend_1_6(*dsti, clut[*src]);
							else if(p1==8)	*dsti = f3_alpha_blend_1_8(*dsti, clut[*src]);
							else if(p1==9)	*dsti = f3_alpha_blend_1_9(*dsti, clut[*src]);
							else if(p1==10)	*dsti = f3_alpha_blend_1_a(*dsti, clut[*src]);
							else usrintf_showmessage("TaitoF3: pri buff error");
							*dstp = 0xff;
						}
					}

					dsti += xadv;
					dstp += xadv;
					srcs += xadv;
					x_count += x_zoom;
					if(x_count>>16)
					{
						x_count &= 0xffff;
						src += src_inc;
						tsrc += tsrc_inc;
						if(src==src_e) {src=src_s; tsrc=tsrc_s;}
					}
				}

				dsti0 += yadv;
				dstp0 += yadv;
				srcs0 += yadv;
				y++;
			}
		}
		else
		{
			while (ysize--)
			{
				length=xsize;
				dsti = dsti0;
				dstp = dstp0;
				srcs = srcs0;

				src=line_t->src[y];
				src_s=line_t->src_s[y];
				src_e=line_t->src_e[y];
				tsrc=line_t->tsrc[y];
				tsrc_s=line_t->tsrc_s[y];
				tsrc_e=line_t->tsrc_e[y];
				x_count=line_t->x_count[y];
				x_zoom=line_t->x_zoom[y];

				while (length--)
				{
					UINT8 p0=*dstp;
					if (p0!=0xff)
					{
						if(p0&7 && (pri_sp[p0&3]>=pri_sl || !((*tsrc)&0xf0)))
						{
							UINT8 p1 = (p0&0x78)>>3;
							if(!p1)			*dsti = clut[*srcs];
							else if(p1==1)	*dsti = f3_alpha_blend_1_1(*dsti, clut[*srcs]);
							else if(p1==2)	*dsti = f3_alpha_blend_1_2(*dsti, clut[*srcs]);
							else if(p1==4)	*dsti = f3_alpha_blend_1_4(*dsti, clut[*srcs]);
							else if(p1==5)	*dsti = f3_alpha_blend_1_5(*dsti, clut[*srcs]);
							else if(p1==6)	*dsti = f3_alpha_blend_1_6(*dsti, clut[*srcs]);
							else if(p1==8)	*dsti = f3_alpha_blend_1_8(*dsti, clut[*srcs]);
							else if(p1==9)	*dsti = f3_alpha_blend_1_9(*dsti, clut[*srcs]);
							else if(p1==10)	*dsti = f3_alpha_blend_1_a(*dsti, clut[*srcs]);
							else usrintf_showmessage("TaitoF3: pri buff error");
						}
						else
						{
							UINT8 p1 = (p0&0x78)>>3;
							if(!p1)			*dsti = clut[*src];
							else if(p1==1)	*dsti = f3_alpha_blend_1_1(*dsti, clut[*src]);
							else if(p1==2)	*dsti = f3_alpha_blend_1_2(*dsti, clut[*src]);
							else if(p1==4)	*dsti = f3_alpha_blend_1_4(*dsti, clut[*src]);
							else if(p1==5)	*dsti = f3_alpha_blend_1_5(*dsti, clut[*src]);
							else if(p1==6)	*dsti = f3_alpha_blend_1_6(*dsti, clut[*src]);
							else if(p1==8)	*dsti = f3_alpha_blend_1_8(*dsti, clut[*src]);
							else if(p1==9)	*dsti = f3_alpha_blend_1_9(*dsti, clut[*src]);
							else if(p1==10)	*dsti = f3_alpha_blend_1_a(*dsti, clut[*src]);
							else usrintf_showmessage("TaitoF3: pri buff error");
						}
					}

					dsti += xadv;
					dstp += xadv;
					srcs += xadv;
					x_count += x_zoom;
					if(x_count>>16)
					{
						x_count &= 0xffff;
						src += src_inc;
						tsrc += tsrc_inc;
						if(src==src_e) {src=src_s; tsrc=tsrc_s;}
					}
				}

				dsti0 += yadv;
				dstp0 += yadv;
				srcs0 += yadv;
				y++;
			}
		}
	}
	else if(draw_type==2)	/* 7000 */
	{
		if (transparent!=TILEMAP_IGNORE_TRANSPARENCY)
		{
			UINT8 pdest_a = f3_alpha_level_2ad ? 0x08 : 0xff;
			UINT8 pdest_b = f3_alpha_level_2bd ? 0x10 : 0xff;
			int tr_a =(f3_alpha_level_2as==0 && f3_alpha_level_2ad==255) ? -1 : 0;
			int tr_b =(f3_alpha_level_2bs==0 && f3_alpha_level_2bd==255) ? -1 : 1;

			while (ysize--)
			{
				length=xsize;
				dsti = dsti0;
				dstp = dstp0;
				srcs = srcs0;

				src=line_t->src[y];
				src_s=line_t->src_s[y];
				src_e=line_t->src_e[y];
				tsrc=line_t->tsrc[y];
				tsrc_s=line_t->tsrc_s[y];
				tsrc_e=line_t->tsrc_e[y];
				x_count=line_t->x_count[y];
				x_zoom=line_t->x_zoom[y];

				while (length--)
				{
					UINT8 p0=*dstp;
					if (p0!=0xff)
					{
						UINT8 tr=*tsrc;
						if(p0&7 && pri_sp[p0&3]>=pri_sl)
						{
							UINT8 p1 = (p0&0x78)>>3;
							if(!p1)			*dsti = clut[*srcs];
							else if(p1==1)	*dsti = f3_alpha_blend_1_1(*dsti, clut[*srcs]);
							else if(p1==2)	*dsti = f3_alpha_blend_1_2(*dsti, clut[*srcs]);
							else if(p1==4)	*dsti = f3_alpha_blend_1_4(*dsti, clut[*srcs]);
							else if(p1==5)	*dsti = f3_alpha_blend_1_5(*dsti, clut[*srcs]);
							else if(p1==6)	*dsti = f3_alpha_blend_1_6(*dsti, clut[*srcs]);
							else if(p1==8)	*dsti = f3_alpha_blend_1_8(*dsti, clut[*srcs]);
							else if(p1==9)	*dsti = f3_alpha_blend_1_9(*dsti, clut[*srcs]);
							else if(p1==10)	*dsti = f3_alpha_blend_1_a(*dsti, clut[*srcs]);
							else usrintf_showmessage("TaitoF3: pri buff error");
							*dstp = 0xff;
						}
						else if(tr&0xf0)
						{
							UINT8 tr2=tr&1;
							if(tr2==tr_b)
							{
								UINT8 p1 = (p0&0x78)>>3;
								if(!p1)			{*dsti = f3_alpha_blend_2b_0(clut[*src]);*dstp = p0|pdest_b;}
							//	else if(p1==1)	{}
							//	else if(p1==2)	{}
								else if(p1==4)	{*dsti = f3_alpha_blend_2b_4(*dsti, clut[*src]);*dstp = p0|pdest_b;}
							//	else if(p1==5)	{}
							//	else if(p1==6)	{}
								else if(p1==8)	{*dsti = f3_alpha_blend_2b_8(*dsti, clut[*src]);*dstp = p0|pdest_b;}
							//	else if(p1==9)	{}
							//	else if(p1==10)	{}
							//	else usrintf_showmessage("TaitoF3: pri buff error");
							}
							else if(tr2==tr_a)
							{
								UINT8 p1 = (p0&0x78)>>3;
								if(!p1)			{*dsti = f3_alpha_blend_2a_0(clut[*src]);*dstp = p0|pdest_a;}
							//	else if(p1==1)	{}
							//	else if(p1==2)	{}
								else if(p1==4)	{*dsti = f3_alpha_blend_2a_4(*dsti, clut[*src]);*dstp = p0|pdest_a;}
							//	else if(p1==5)	{}
							//	else if(p1==6)	{}
								else if(p1==8)	{*dsti = f3_alpha_blend_2a_8(*dsti, clut[*src]);*dstp = p0|pdest_a;}
							//	else if(p1==9)	{}
							//	else if(p1==10)	{}
							//	else usrintf_showmessage("TaitoF3: pri buff error");
							}
						}
					}

					dsti += xadv;
					dstp += xadv;
					srcs += xadv;
					x_count += x_zoom;
					if(x_count>>16)
					{
						x_count &= 0xffff;
						src += src_inc;
						tsrc += tsrc_inc;
						if(src==src_e) {src=src_s; tsrc=tsrc_s;}
					}
				}

				dsti0 += yadv;
				dstp0 += yadv;
				srcs0 += yadv;
				y++;
			}
		}
		else
		{
			while (ysize--)
			{
				length=xsize;
				dsti = dsti0;
				dstp = dstp0;
				srcs = srcs0;

				src=line_t->src[y];
				src_s=line_t->src_s[y];
				src_e=line_t->src_e[y];
				tsrc=line_t->tsrc[y];
				tsrc_s=line_t->tsrc_s[y];
				tsrc_e=line_t->tsrc_e[y];
				x_count=line_t->x_count[y];
				x_zoom=line_t->x_zoom[y];

				while (length--)
				{
					UINT8 p0=*dstp;
					if (p0!=0xff)
					{
						UINT8 tr=*tsrc;
						if(p0&7 && (pri_sp[p0&3]>=pri_sl || !(tr&0xf0)))
						{
							UINT8 p1 = (p0&0x78)>>3;
							if(!p1)			*dsti = clut[*srcs];
							else if(p1==1)	*dsti = f3_alpha_blend_1_1(*dsti, clut[*srcs]);
							else if(p1==2)	*dsti = f3_alpha_blend_1_2(*dsti, clut[*srcs]);
							else if(p1==4)	*dsti = f3_alpha_blend_1_4(*dsti, clut[*srcs]);
							else if(p1==5)	*dsti = f3_alpha_blend_1_5(*dsti, clut[*srcs]);
							else if(p1==6)	*dsti = f3_alpha_blend_1_6(*dsti, clut[*srcs]);
							else if(p1==8)	*dsti = f3_alpha_blend_1_8(*dsti, clut[*srcs]);
							else if(p1==9)	*dsti = f3_alpha_blend_1_9(*dsti, clut[*srcs]);
							else if(p1==10)	*dsti = f3_alpha_blend_1_a(*dsti, clut[*srcs]);
							else usrintf_showmessage("TaitoF3: pri buff error");
						}
						else
						{
							UINT8 p1 = (p0&0x78)>>3;
							if(tr&1)
							{
								if(!p1)			{*dsti = f3_alpha_blend_2b_0(clut[*src]);
												 if(p0&7) *dsti = f3_alpha_blend_1_2(*dsti, clut[*srcs]);}
								else if(p1==1)	{if(p0&7) *dsti = f3_alpha_blend_1_1(*dsti, clut[*srcs]);}
								else if(p1==2)	{if(p0&7) *dsti = f3_alpha_blend_1_2(*dsti, clut[*srcs]);}
								else if(p1==4)	{*dsti = f3_alpha_blend_2b_4(*dsti, clut[*src]);
												 if(p0&7) *dsti = f3_alpha_blend_1_6(*dsti, clut[*srcs]);}
								else if(p1==5)	{if(p0&7) *dsti = f3_alpha_blend_1_5(*dsti, clut[*srcs]);}
								else if(p1==6)	{if(p0&7) *dsti = f3_alpha_blend_1_6(*dsti, clut[*srcs]);}
								else if(p1==8)	{*dsti = f3_alpha_blend_2b_8(*dsti, clut[*src]);
												 if(p0&7) *dsti = f3_alpha_blend_1_a(*dsti, clut[*srcs]);}
								else if(p1==9)	{if(p0&7) *dsti = f3_alpha_blend_1_9(*dsti, clut[*srcs]);}
								else if(p1==10)	{if(p0&7) *dsti = f3_alpha_blend_1_a(*dsti, clut[*srcs]);}
								else usrintf_showmessage("TaitoF3: pri buff error");
							}
							else
							{
								if(!p1)			{*dsti = f3_alpha_blend_2a_0(clut[*src]);
												 if(p0&7) *dsti = f3_alpha_blend_1_1(*dsti, clut[*srcs]);}
								else if(p1==1)	{if(p0&7) *dsti = f3_alpha_blend_1_1(*dsti, clut[*srcs]);}
								else if(p1==2)	{if(p0&7) *dsti = f3_alpha_blend_1_2(*dsti, clut[*srcs]);}
								else if(p1==4)	{*dsti = f3_alpha_blend_2a_4(*dsti, clut[*src]);
												 if(p0&7) *dsti = f3_alpha_blend_1_5(*dsti, clut[*srcs]);}
								else if(p1==5)	{if(p0&7) *dsti = f3_alpha_blend_1_5(*dsti, clut[*srcs]);}
								else if(p1==6)	{if(p0&7) *dsti = f3_alpha_blend_1_6(*dsti, clut[*srcs]);}
								else if(p1==8)	{*dsti = f3_alpha_blend_2a_8(*dsti, clut[*src]);
												 if(p0&7) *dsti = f3_alpha_blend_1_9(*dsti, clut[*srcs]);}
								else if(p1==9)	{if(p0&7) *dsti = f3_alpha_blend_1_9(*dsti, clut[*srcs]);}
								else if(p1==10)	{if(p0&7) *dsti = f3_alpha_blend_1_a(*dsti, clut[*srcs]);}
								else usrintf_showmessage("TaitoF3: pri buff error");
							}
						}
					}

					dsti += xadv;
					dstp += xadv;
					srcs += xadv;
					x_count += x_zoom;
					if(x_count>>16)
					{
						x_count &= 0xffff;
						src += src_inc;
						tsrc += tsrc_inc;
						if(src==src_e) {src=src_s; tsrc=tsrc_s;}
					}
				}

				dsti0 += yadv;
				dstp0 += yadv;
				srcs0 += yadv;
				y++;
			}
		}
	}
	else if(draw_type==3)	/* b000 */
	{
		if (transparent!=TILEMAP_IGNORE_TRANSPARENCY)
		{
			UINT8 pdest_a = f3_alpha_level_3ad ? 0x20 : 0xff;
			UINT8 pdest_b = f3_alpha_level_3bd ? 0x40 : 0xff;
			int tr_a =(f3_alpha_level_3as==0 && f3_alpha_level_3ad==255) ? -1 : 0;
			int tr_b =(f3_alpha_level_3bs==0 && f3_alpha_level_3bd==255) ? -1 : 1;

			while (ysize--)
			{
				length=xsize;
				dsti = dsti0;
				dstp = dstp0;
				srcs = srcs0;

				src=line_t->src[y];
				src_s=line_t->src_s[y];
				src_e=line_t->src_e[y];
				tsrc=line_t->tsrc[y];
				tsrc_s=line_t->tsrc_s[y];
				tsrc_e=line_t->tsrc_e[y];
				x_count=line_t->x_count[y];
				x_zoom=line_t->x_zoom[y];

				while (length--)
				{
					UINT8 p0=*dstp;
					if (p0!=0xff)
					{
						UINT8 tr=*tsrc;
						if(p0&7 && pri_sp[p0&3]>=pri_sl)
						{
							UINT8 p1 = (p0&0x78)>>3;
							if(!p1)			*dsti = clut[*srcs];
							else if(p1==1)	*dsti = f3_alpha_blend_1_1(*dsti, clut[*srcs]);
							else if(p1==2)	*dsti = f3_alpha_blend_1_2(*dsti, clut[*srcs]);
							else if(p1==4)	*dsti = f3_alpha_blend_1_4(*dsti, clut[*srcs]);
							else if(p1==5)	*dsti = f3_alpha_blend_1_5(*dsti, clut[*srcs]);
							else if(p1==6)	*dsti = f3_alpha_blend_1_6(*dsti, clut[*srcs]);
							else if(p1==8)	*dsti = f3_alpha_blend_1_8(*dsti, clut[*srcs]);
							else if(p1==9)	*dsti = f3_alpha_blend_1_9(*dsti, clut[*srcs]);
							else if(p1==10)	*dsti = f3_alpha_blend_1_a(*dsti, clut[*srcs]);
							else usrintf_showmessage("TaitoF3: pri buff error");
							*dstp = 0xff;
						}
						else if(tr&0xf0)
						{
							UINT8 tr2=tr&1;
							if(tr2==tr_b)
							{
								UINT8 p1 = (p0&0x78)>>3;
								if(!p1)			{*dsti = f3_alpha_blend_3b_0(clut[*src]);*dstp = p0|pdest_b;}
								else if(p1==1)	{*dsti = f3_alpha_blend_3b_1(*dsti, clut[*src]);*dstp = p0|pdest_b;}
								else if(p1==2)	{*dsti = f3_alpha_blend_3b_2(*dsti, clut[*src]);*dstp = p0|pdest_b;}
							//	else if(p1==4)	{}
							//	else if(p1==5)	{}
							//	else if(p1==6)	{}
							//	else if(p1==8)	{}
							//	else if(p1==9)	{}
							//	else if(p1==10)	{}
							//	else usrintf_showmessage("TaitoF3: pri buff error");
							}
							else if(tr2==tr_a)
							{
								UINT8 p1 = (p0&0x78)>>3;
								if(!p1)			{*dsti = f3_alpha_blend_3a_0(clut[*src]);*dstp = p0|pdest_a;}
								else if(p1==1)	{*dsti = f3_alpha_blend_3a_1(*dsti, clut[*src]);*dstp = p0|pdest_a;}
								else if(p1==2)	{*dsti = f3_alpha_blend_3a_2(*dsti, clut[*src]);*dstp = p0|pdest_a;}
							//	else if(p1==4)	{}
							//	else if(p1==5)	{}
							//	else if(p1==6)	{}
							//	else if(p1==8)	{}
							//	else if(p1==9)	{}
							//	else if(p1==10)	{}
							//	else usrintf_showmessage("TaitoF3: pri buff error");
							}
						}
					}

					dsti += xadv;
					dstp += xadv;
					srcs += xadv;
					x_count += x_zoom;
					if(x_count>>16)
					{
						x_count &= 0xffff;
						src += src_inc;
						tsrc += tsrc_inc;
						if(src==src_e) {src=src_s; tsrc=tsrc_s;}
					}
				}

				dsti0 += yadv;
				dstp0 += yadv;
				srcs0 += yadv;
				y++;
			}
		}
		else
		{
			while (ysize--)
			{
				length=xsize;
				dsti = dsti0;
				dstp = dstp0;
				srcs = srcs0;

				src=line_t->src[y];
				src_s=line_t->src_s[y];
				src_e=line_t->src_e[y];
				tsrc=line_t->tsrc[y];
				tsrc_s=line_t->tsrc_s[y];
				tsrc_e=line_t->tsrc_e[y];
				x_count=line_t->x_count[y];
				x_zoom=line_t->x_zoom[y];

				while (length--)
				{
					UINT8 p0=*dstp;
					if (p0!=0xff)
					{
						UINT8 tr=*tsrc;
						if(p0&7 && (pri_sp[p0&3]>=pri_sl || !(tr&0xf0)))
						{
							UINT8 p1 = (p0&0x78)>>3;
							if(!p1)			*dsti = clut[*srcs];
							else if(p1==1)	*dsti = f3_alpha_blend_1_1(*dsti, clut[*srcs]);
							else if(p1==2)	*dsti = f3_alpha_blend_1_2(*dsti, clut[*srcs]);
							else if(p1==4)	*dsti = f3_alpha_blend_1_4(*dsti, clut[*srcs]);
							else if(p1==5)	*dsti = f3_alpha_blend_1_5(*dsti, clut[*srcs]);
							else if(p1==6)	*dsti = f3_alpha_blend_1_6(*dsti, clut[*srcs]);
							else if(p1==8)	*dsti = f3_alpha_blend_1_8(*dsti, clut[*srcs]);
							else if(p1==9)	*dsti = f3_alpha_blend_1_9(*dsti, clut[*srcs]);
							else if(p1==10)	*dsti = f3_alpha_blend_1_a(*dsti, clut[*srcs]);
							else usrintf_showmessage("TaitoF3: pri buff error");
						}
						else
						{
							UINT8 p1 = (p0&0x78)>>3;
							if(tr&1)
							{
								if(!p1)			{*dsti = f3_alpha_blend_3b_0(clut[*src]);
												 if(p0&7) *dsti = f3_alpha_blend_1_8(*dsti, clut[*srcs]);}
								else if(p1==1)	{*dsti = f3_alpha_blend_3b_1(*dsti, clut[*src]);
												 if(p0&7) *dsti = f3_alpha_blend_1_9(*dsti, clut[*srcs]);}
								else if(p1==2)	{*dsti = f3_alpha_blend_3b_2(*dsti, clut[*src]);
												 if(p0&7) *dsti = f3_alpha_blend_1_a(*dsti, clut[*srcs]);}
								else if(p1==4)	{if(p0&7) *dsti = f3_alpha_blend_1_4(*dsti, clut[*srcs]);}
								else if(p1==5)	{if(p0&7) *dsti = f3_alpha_blend_1_5(*dsti, clut[*srcs]);}
								else if(p1==6)	{if(p0&7) *dsti = f3_alpha_blend_1_6(*dsti, clut[*srcs]);}
								else if(p1==8)	{if(p0&7) *dsti = f3_alpha_blend_1_8(*dsti, clut[*srcs]);}
								else if(p1==9)	{if(p0&7) *dsti = f3_alpha_blend_1_9(*dsti, clut[*srcs]);}
								else if(p1==10)	{if(p0&7) *dsti = f3_alpha_blend_1_a(*dsti, clut[*srcs]);}
								else usrintf_showmessage("TaitoF3: pri buff error");
							}
							else
							{
								if(!p1)			{*dsti = f3_alpha_blend_3a_0(clut[*src]);
												 if(p0&7) *dsti = f3_alpha_blend_1_4(*dsti, clut[*srcs]);}
								else if(p1==1)	{*dsti = f3_alpha_blend_3a_1(*dsti, clut[*src]);
												 if(p0&7) *dsti = f3_alpha_blend_1_5(*dsti, clut[*srcs]);}
								else if(p1==2)	{*dsti = f3_alpha_blend_3a_2(*dsti, clut[*src]);
												 if(p0&7) *dsti = f3_alpha_blend_1_6(*dsti, clut[*srcs]);}
								else if(p1==4)	{if(p0&7) *dsti = f3_alpha_blend_1_4(*dsti, clut[*srcs]);}
								else if(p1==5)	{if(p0&7) *dsti = f3_alpha_blend_1_5(*dsti, clut[*srcs]);}
								else if(p1==6)	{if(p0&7) *dsti = f3_alpha_blend_1_6(*dsti, clut[*srcs]);}
								else if(p1==8)	{if(p0&7) *dsti = f3_alpha_blend_1_8(*dsti, clut[*srcs]);}
								else if(p1==9)	{if(p0&7) *dsti = f3_alpha_blend_1_9(*dsti, clut[*srcs]);}
								else if(p1==10)	{if(p0&7) *dsti = f3_alpha_blend_1_a(*dsti, clut[*srcs]);}
								else usrintf_showmessage("TaitoF3: pri buff error");
							}
						}
					}

					dsti += xadv;
					dstp += xadv;
					srcs += xadv;
					x_count += x_zoom;
					if(x_count>>16)
					{
						x_count &= 0xffff;
						src += src_inc;
						tsrc += tsrc_inc;
						if(src==src_e) {src=src_s; tsrc=tsrc_s;}
					}
				}

				dsti0 += yadv;
				dstp0 += yadv;
				srcs0 += yadv;
				y++;
			}
		}
	}
	else if(draw_type==0)	/* disable line  TILEMAP_IGNORE_TRANSPARENCY only */
	{
		while (ysize--)
		{
			length=xsize;
			dsti = dsti0;
			dstp = dstp0;
			srcs = srcs0;
			while (length--)
			{
				UINT8 p0=*dstp;
				if (p0!=0xff)
				{
					if(!p0) *dsti = 0;
					else if((p0&7))
					{
						UINT8 p1 = (p0&0x78)>>3;
						if(!p1)			*dsti = clut[*srcs];
						else if(p1==1)	*dsti = f3_alpha_blend_1_1(*dsti, clut[*srcs]);
						else if(p1==2)	*dsti = f3_alpha_blend_1_2(*dsti, clut[*srcs]);
						else if(p1==4)	*dsti = f3_alpha_blend_1_4(*dsti, clut[*srcs]);
						else if(p1==5)	*dsti = f3_alpha_blend_1_5(*dsti, clut[*srcs]);
						else if(p1==6)	*dsti = f3_alpha_blend_1_6(*dsti, clut[*srcs]);
						else if(p1==8)	*dsti = f3_alpha_blend_1_8(*dsti, clut[*srcs]);
						else if(p1==9)	*dsti = f3_alpha_blend_1_9(*dsti, clut[*srcs]);
						else if(p1==10)	*dsti = f3_alpha_blend_1_a(*dsti, clut[*srcs]);
						else usrintf_showmessage("TaitoF3: pri buff error");
					}
				}
				dsti += xadv;
				dstp += xadv;
				srcs += xadv;
			}
			dsti0 += yadv;
			dstp0 += yadv;
			srcs0 += yadv;
		}
	}
	else if(draw_type==-1)	/**/
	{
		while (ysize--)
		{
			length=xsize;
			dsti = dsti0;
			while (length--)
			{
				*dsti = 0;
				dsti += xadv;
			}
			dsti0 += yadv;
		}
	}
}

INLINE void f3_drawscanlines_noalpha(
		struct mame_bitmap *bitmap,int x,int y,int xsize,int ysize,
		const struct f3_line_inf *line_t,
		int transparent,UINT32 orient,
		int *pri_sp,int pri_sl)
{
	pen_t *clut = &Machine->remapped_colortable[0];
	int length;
	UINT16 *src,*src_s,*src_e;
	UINT8 *tsrc,*tsrc_s,*tsrc_e;
	UINT32 x_count,x_zoom;
	int src_inc=line_t->src_inc,tsrc_inc=line_t->tsrc_inc;

	UINT32 *dsti0,*dsti;
	UINT8 *dstp0,*dstp;
	UINT16 *srcs0,*srcs;
	int yadv = bitmap->rowpixels;
	int xadv = 1;
	int ty=y;

	if (orient & ORIENTATION_FLIP_Y) {
		ty = bitmap->height - 1 - ty;
		yadv = -yadv;
	}

	dsti0 = (UINT32 *)bitmap->line[ty] + x;
	dstp0 = (UINT8 *)pri_alp_bitmap->line[ty] + x;
	srcs0 = (UINT16 *)sprite_bitmap->line[ty] + x;


	{
		if (transparent!=TILEMAP_IGNORE_TRANSPARENCY)
		{
			while (ysize--)
			{
				length=xsize;
				dsti = dsti0;
				dstp = dstp0;
				srcs = srcs0;

				src=line_t->src[y];
				src_s=line_t->src_s[y];
				src_e=line_t->src_e[y];
				tsrc=line_t->tsrc[y];
				tsrc_s=line_t->tsrc_s[y];
				tsrc_e=line_t->tsrc_e[y];
				x_count=line_t->x_count[y];
				x_zoom=line_t->x_zoom[y];

				while (length--)
				{
					UINT8 p0=*dstp;
					if (p0!=0xff)
					{
						if(p0&7 && pri_sp[p0&3]>=pri_sl)
						{
							*dsti = clut[*srcs];
							*dstp = 0xff;
						}
						else if((*tsrc)&0xf0)
						{
							*dsti = clut[*src];
							*dstp = 0xff;
						}
					}

					dsti += xadv;
					dstp += xadv;
					srcs += xadv;
					x_count += x_zoom;
					if(x_count>>16)
					{
						x_count &= 0xffff;
						src += src_inc;
						tsrc += tsrc_inc;
						if(src==src_e) {src=src_s; tsrc=tsrc_s;}
					}
				}

				dsti0 += yadv;
				dstp0 += yadv;
				srcs0 += yadv;
				y++;
			}
		}
		else
		{
			while (ysize--)
			{
				length=xsize;
				dsti = dsti0;
				dstp = dstp0;
				srcs = srcs0;

				src=line_t->src[y];
				src_s=line_t->src_s[y];
				src_e=line_t->src_e[y];
				tsrc=line_t->tsrc[y];
				tsrc_s=line_t->tsrc_s[y];
				tsrc_e=line_t->tsrc_e[y];
				x_count=line_t->x_count[y];
				x_zoom=line_t->x_zoom[y];

				while (length--)
				{
					UINT8 p0=*dstp;
					if (p0!=0xff)
					{
						if(p0&7 && (pri_sp[p0&3]>=pri_sl || !((*tsrc)&0xf0)))
						{
							*dsti = clut[*srcs];
						}
						else
						{
							*dsti = clut[*src];
						}
					}

					dsti += xadv;
					dstp += xadv;
					srcs += xadv;
					x_count += x_zoom;
					if(x_count>>16)
					{
						x_count &= 0xffff;
						src += src_inc;
						tsrc += tsrc_inc;
						if(src==src_e) {src=src_s; tsrc=tsrc_s;}
					}
				}

				dsti0 += yadv;
				dstp0 += yadv;
				srcs0 += yadv;
				y++;
			}
		}
	}
}

/******************************************************************************/

/* sx and sy are 16.16 fixed point numbers */
static void get_line_ram_info(struct tilemap *tilemap,int sx,int sy,int pos)
{
	struct f3_line_inf *line_t=&line_inf[pos];
	const struct mame_bitmap *srcbitmap;
	const struct mame_bitmap *transbitmap;

	int y,y_start,y_end,y_inc;
	int line_base,zoom_base,col_base,pri_base,spri_base,inc;

	int line_enable;
	int colscroll=0,x_offset=0;
	UINT32 x_zoom=0x10000;
	UINT32 y_zoom=0;
	UINT16 pri=0;
	UINT16 spri=0;
	int alpha_level=0;
	int bit_select0=0x10000<<pos;
	int bit_select1=1<<pos;

	int _colscroll[256];
	UINT32 _x_offset[256];
	int y_index_fx;

	sx+=(46<<16);//+scroll_kludge_x;
	if (flipscreen)
	{
		line_base=0xa1fe + (pos*0x200);
		zoom_base=0x81fe + (pos*0x200);
		col_base =0x41fe + (pos*0x200);
		pri_base =0xb1fe + (pos*0x200);
		spri_base =0x77fe;
		inc=-2;
		y_start=255;
		y_end=-1;
		y_inc=-1;

		if (f3_game_config->extend)	sx=-sx+((188-512)<<16); else sx=-sx+(188<<16); /* Adjust for flipped scroll position */
		y_index_fx=-sy-(256<<16); /* Adjust for flipped scroll position */
	}
	else
	{
		line_base=0xa000 + (pos*0x200);
		zoom_base=0x8000 + (pos*0x200);
		col_base =0x4000 + (pos*0x200);
		pri_base =0xb000 + (pos*0x200);
		spri_base =0x7600;
		inc=2;
		y_start=0;
		y_end=256;
		y_inc=1;

		y_index_fx=sy;
	}

	y=y_start;
	while(y!=y_end)
	{
		/* The zoom, column and row values can latch according to control ram */
		if (y&1)
		{
			if (f3_line_ram[0x300+(y>>1)]&bit_select1)
				x_offset=(f3_line_ram[line_base/4]&0xffff)<<10;
			if (f3_line_ram[0x380+(y>>1)]&bit_select1)
				pri=f3_line_ram[pri_base/4]&0xffff;
			if (pri && !(pri&0x800) ) line_enable=1; else line_enable=0;
			if (f3_line_ram[0x200+(y>>1)]&bit_select1)
			{
				int line_ram_zoom=f3_line_ram[zoom_base/4]&0xffff;
				if (line_ram_zoom!=0)
				{
					x_zoom=0x10080 - line_ram_zoom;
					if (y_zoom==0 && line_enable) y_zoom=x_zoom;
				}
			}
			if (f3_line_ram[0x000+(y>>1)]&bit_select1)
				colscroll=(f3_line_ram[col_base/4]>> 0)&0x1ff;
			if (f3_line_ram[(0x0600/4)+(y>>1)]&0x8)
				spri=f3_line_ram[spri_base/4]&0xffff;
			if (f3_line_ram[(0x0400/4)+(y>>1)]&0x2)
				alpha_level=f3_line_ram[(spri_base-0x1400)/4]&0xffff;
		}
		else
		{
			if (f3_line_ram[0x300+(y>>1)]&bit_select0)
				x_offset=(f3_line_ram[line_base/4]&0xffff0000)>>6;
			if (f3_line_ram[0x380+(y>>1)]&bit_select0)
				pri=(f3_line_ram[pri_base/4]>>16)&0xffff;
			if (pri && !(pri&0x800) ) line_enable=1; else line_enable=0;
			if (f3_line_ram[0x200+(y>>1)]&bit_select0)
			{
				int line_ram_zoom=f3_line_ram[zoom_base/4]>>16;
				if (line_ram_zoom!=0)
				{
					x_zoom=0x10080 - line_ram_zoom;
					if (y_zoom==0 && line_enable) y_zoom=x_zoom;
				}
			}
			if (f3_line_ram[0x000+(y>>1)]&bit_select0)
				colscroll=(f3_line_ram[col_base/4]>>16)&0x1ff;
			if (f3_line_ram[(0x0600/4)+(y>>1)]&0x80000)
				spri=f3_line_ram[spri_base/4]>>16;
			if (f3_line_ram[(0x0400/4)+(y>>1)]&0x20000)
				alpha_level=f3_line_ram[(spri_base-0x1400)/4]>>16;
		}

		/* XYZoom? */
		if(y_zoom && line_enable && (colscroll!=0 || x_zoom!=y_zoom)) y_zoom=0x10000;


//		if (line_enable)
		{
			if (!pri || (!flipscreen && y<24) || (flipscreen && y>231) || (pri&0xc000)==0xc000)
 				line_enable=0;
			else if(pri&0x4000)	//alpha1
				line_enable=2;
			else if(pri&0x8000)	//alpha2
				line_enable=3;
			else
				line_enable=1;
		}

		_colscroll[y]=colscroll;
		_x_offset[y]=(x_offset&0xffff0000) - (x_offset&0x0000ffff);

		line_t->x_zoom[y]=x_zoom;
		line_t->alpha_mode[y]=line_enable;
		line_t->alpha_level[y]=alpha_level;
		line_t->spri[y]=spri;
		line_t->pri[y]=pri;

		zoom_base+=inc;
		line_base+=inc;
		col_base +=inc;
		pri_base +=inc;
		spri_base+=inc;
		y +=y_inc;
	}
	if(!y_zoom) y_zoom=0x10000;



	/* set pixmap pointer */

	srcbitmap = tilemap_get_pixmap(tilemap);
	transbitmap = tilemap_get_transparency_bitmap(tilemap);
//	if (Machine->orientation == ROT0)
	{
		y=y_start;
		line_t->src_inc=1;
		line_t->tsrc_inc=1;
		while(y!=y_end)
		{
			UINT32 x_index_fx;
			UINT32 y_index;

			if(line_t->alpha_mode[y]!=0)
			{
				UINT16 *src_s;
				UINT8 *tsrc_s;

				/* set pixmap index */
				x_index_fx = (sx+_x_offset[y]-(10*0x10000)+10*line_t->x_zoom[y])&((width_mask<<16)|0xffff);
				y_index = ((y_index_fx>>16)+_colscroll[y])&0x1ff;

				line_t->x_count[y]=x_index_fx & 0xffff;
				line_t->src_s[y]=src_s=(unsigned short *)srcbitmap->line[y_index];
				line_t->src_e[y]=&src_s[width_mask+1];
				line_t->src[y]=&src_s[x_index_fx>>16];

				line_t->tsrc_s[y]=tsrc_s=(unsigned char *)transbitmap->line[y_index];
				line_t->tsrc_e[y]=&tsrc_s[width_mask+1];
				line_t->tsrc[y]=&tsrc_s[x_index_fx>>16];
			}

			if(y_zoom==line_t->x_zoom[y])
				y_index_fx += y_zoom;
			else
				y_index_fx += 0x10000;

			y +=y_inc;
		}
	}
}

static void f3_tilemap_draw(struct mame_bitmap *bitmap,
							const struct rectangle *cliprect)
{
	int i,j,y,pos;
	UINT32 rot=0;
	unsigned char transparent;
	int layer_tmp[4];
	int enable[4];//ksdeb

//ksdeb s
	enable[0]=enable[1]=enable[2]=enable[3]=1;
	if(deb_enable)
	{
		if (keyboard_pressed(KEYCODE_Z)) enable[0]=0;
		if (keyboard_pressed(KEYCODE_X)) enable[1]=0;
		if (keyboard_pressed(KEYCODE_C)) enable[2]=0;
		if (keyboard_pressed(KEYCODE_V)) enable[3]=0;
	}
//ksdeb e

	if (flipscreen)
		rot=ORIENTATION_FLIP_Y;

deb_loop=0;//ksdeb
	y=0;
	while(y<256)
	{
		static int alpha_level_last=-1;
		int pri[4],alpha_mode[4],alpha_level;
		int spri,pri_sp[4],pri_sl[4];
		int y_end;
		int alpha,noalpha_all;

deb_loop++;//ksdeb

		/* find same status of scanlines */
		pri[0]=line_inf[0].pri[y];
		pri[1]=line_inf[1].pri[y];
		pri[2]=line_inf[2].pri[y];
		pri[3]=line_inf[3].pri[y];
		alpha_mode[0]=line_inf[0].alpha_mode[y];
		alpha_mode[1]=line_inf[1].alpha_mode[y];
		alpha_mode[2]=line_inf[2].alpha_mode[y];
		alpha_mode[3]=line_inf[3].alpha_mode[y];
		alpha_level=line_inf[0].alpha_level[y];
		spri=line_inf[0].spri[y];
		for(y_end=y+1;y_end<256;y_end++)
		{
			if(pri[0]!=line_inf[0].pri[y_end]) break;
			if(pri[1]!=line_inf[1].pri[y_end]) break;
			if(pri[2]!=line_inf[2].pri[y_end]) break;
			if(pri[3]!=line_inf[3].pri[y_end]) break;
			if(alpha_mode[0]!=line_inf[0].alpha_mode[y_end]) break;
			if(alpha_mode[1]!=line_inf[1].alpha_mode[y_end]) break;
			if(alpha_mode[2]!=line_inf[2].alpha_mode[y_end]) break;
			if(alpha_mode[3]!=line_inf[3].alpha_mode[y_end]) break;
			if(alpha_level!=line_inf[0].alpha_level[y_end]) break;
			if(spri!=line_inf[0].spri[y_end]) break;
		}


		/* for clear top & bottom border */
		if(
			(!alpha_mode[0] && !alpha_mode[1] && !alpha_mode[2] && !alpha_mode[3]) ||
			((pri[0]&0x800) && (pri[1]&0x800) && (pri[2]&0x800) && (pri[3]&0x800))		/*KTIGER2*/
		  )
		{
			f3_drawscanlines(bitmap,46,y,320,y_end-y,
							&line_inf[0],
							0,rot,
							0,
							0,
							-1);
			y=y_end;
			continue;
		}


		/* set sprite priority */
		pri_sp[0]=spri&0xf;
		pri_sp[1]=(spri>>4)&0xf;
		pri_sp[2]=(spri>>8)&0xf;
		pri_sp[3]=spri>>12;


//ksdeb
		/* set scanline priority */
		for(i=0;i<4;i++)
		{
			int p0=pri[i];
			int pri_sl1=p0&0x0f;
			int pri_sl2=(p0&0xf0)>>4;
			if(pri_sl2)
			{
				if(!(p0&0x200))	pri_sl1=pri_sl2>pri_sl1 ? pri_sl2 : pri_sl1;	//CLEOPATR
				else			pri_sl1=pri_sl2<pri_sl1 ? pri_sl2 : pri_sl1;	//ARKRETRN
			}
			pri_sl[i]=pri_sl1;
			layer_tmp[i]=i + (pri_sl1<<2);
		}

		/* sort layer_tmp */
		for(i=0;i<3;i++)
		{
			for(j=i+1;j<4;j++)
			{
				if(layer_tmp[i]<layer_tmp[j])
				{
					int temp = layer_tmp[i];
					layer_tmp[i] = layer_tmp[j];
					layer_tmp[j] = temp;
				}
			}
		}

//ksdeb s
		if(!enable[0]) alpha_mode[0]=0;
		if(!enable[1]) alpha_mode[1]=0;
		if(!enable[2]) alpha_mode[2]=0;
		if(!enable[3]) alpha_mode[3]=0;
//ksdeb e


		/* set alpha level */
		if(alpha_level!=alpha_level_last)		//ksdebA
		{
			int al_s,al_d;
			int a=alpha_level;
			int b=(a>>8)&0xf;
			int c=(a>>4)&0xf;
			int d=(a>>0)&0xf;
			a>>=12;

			/* b000 7000 */
			al_s = ( (15-d)*256) / 8;
			al_d = ( (15-b)*256) / 8;
			if(al_s>255) al_s = 255;
			if(al_d>255) al_d = 255;
			f3_alpha_level_3as = al_s;
			f3_alpha_level_3ad = al_d;
			f3_alpha_level_2as = al_d;
			f3_alpha_level_2ad = al_s;

			al_s = ( (15-c)*256) / 8;
			al_d = ( (15-a)*256) / 8;
			if(al_s>255) al_s = 255;
			if(al_d>255) al_d = 255;
			f3_alpha_level_3bs = al_s;
			f3_alpha_level_3bd = al_d;
			f3_alpha_level_2bs = al_d;
			f3_alpha_level_2bd = al_s;

			f3_alpha_set_level();
			alpha_level_last=alpha_level;
		}


		/* draw scanlines from front to back */
		transparent=0;
		alpha=0;
//ksdeb
		if (deb_alpha_disable || (alpha_mode[0]==2 && alpha_mode[1]==2 && alpha_mode[2]==2 && alpha_mode[3]==2 &&
			f3_alpha_level_2as==255 && f3_alpha_level_2bs==255 ))
			noalpha_all=1;			/*GUNLOCK*/
		else
			noalpha_all=0;

		for(i=0;i<4;i++)
		{
			struct f3_line_inf *line_t;

			pos=layer_tmp[i]&3;
			line_t=&line_inf[pos];
			if(i==3) transparent=TILEMAP_IGNORE_TRANSPARENCY;
			if(alpha_mode[pos]==0 && transparent!=TILEMAP_IGNORE_TRANSPARENCY) continue;
			if(!(pri[pos]&0x2000) && transparent!=TILEMAP_IGNORE_TRANSPARENCY) continue;/*BUBBLEM*/


			if(!noalpha_all && alpha_mode[pos]>1) alpha=1;

			if(alpha)
				f3_drawscanlines(bitmap,46,y,320,y_end-y,
								line_t,
								transparent,rot,
								pri_sp,
								pri_sl[pos],
								alpha_mode[pos]);
			else
				f3_drawscanlines_noalpha(bitmap,46,y,320,y_end-y,
								line_t,
								transparent,rot,
								pri_sp,
								pri_sl[pos]);
		}
		y=y_end;
	}
}
//ks e
/******************************************************************************/

static void f3_update_pivot_layer(void)
{
	struct rectangle pivot_clip;
	int tile,i,pivot_base;

	/* A poor way to guess if the pivot layer is enabled, but quicker than
		parsing control ram. */
	int ctrl  = f3_line_ram[0x180f]&0xa000; /* SpcInvDX sets only this address */
	int ctrl2 = f3_line_ram[0x1870]&0xa0000000; /* SpcInvDX flipscreen */
	int ctrl3 = f3_line_ram[0x1820]&0xa000; /* Other games set the whole range 0x6000-0x61ff */
	int ctrl4 = f3_line_ram[0x1840]&0xa000; /* ScFinals only sets a small range over the screen area */

	/* Quickly decide whether to process the rest of the pivot layer */
	if (!(ctrl || ctrl2 || ctrl3 || ctrl4)) {
		tilemap_set_enable(pixel_layer,0);
		return;
	}
	tilemap_set_enable(pixel_layer,1);

	if (flipscreen) {
		tilemap_set_scrollx( pixel_layer,0,(f3_control_1[2]>>16)-(512-320)-16);
		tilemap_set_scrolly( pixel_layer,0,-(f3_control_1[2]&0xff));
	} else {
		tilemap_set_scrollx( pixel_layer,0,-(f3_control_1[2]>>16)-5);
		tilemap_set_scrolly( pixel_layer,0,-(f3_control_1[2]&0xff));
	}

	/* Clip top scanlines according to line ram - Bubble Memories makes use of this */
	pivot_clip.min_x=0;//Machine->visible_area.min_x;
	pivot_clip.max_x=512;//Machine->visible_area.max_x;
	pivot_clip.min_y=Machine->visible_area.min_y;
	pivot_clip.max_y=Machine->visible_area.max_y;
	if (flipscreen)
		pivot_base=0x61fe;
	else
		pivot_base=0x6000;

	for (i=0; i<256; i++) {
		/* Loop through until first visible line */
		if (pivot_base&2) {
			if ((f3_line_ram[pivot_base/4]&0xa000)==0xa000) {
				pivot_clip.min_y=i;
				i=256;
			}
		} else {
			if ((f3_line_ram[pivot_base/4]&0xa0000000)==0xa0000000) {
				pivot_clip.min_y=i;
				i=256;
			}
		}
		if (flipscreen) pivot_base-=2; else pivot_base+=2;
	}

	if (!flipscreen)
		pixel_layer_clip = pivot_clip;

	/* Decode chars & mark tilemap dirty */
	if (pivot_changed)
		for (tile = 0;tile < 2048;tile++)
			if (pivot_dirty[tile]) {
				decodechar(Machine->gfx[3],tile,(UINT8 *)f3_pivot_ram,Machine->drv->gfxdecodeinfo[3].gfxlayout);
				tilemap_mark_tile_dirty(pixel_layer,tile);
				pivot_dirty[tile]=0;
			}
	pivot_changed=0;
}

/******************************************************************************/

static void f3_draw_vram_layer(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int offs,mx,my,tile,color,fx,fy,sx,sy;

	sx=(f3_control_1[2]>>16)+5;
	sy=f3_control_1[2]&0xffff;

   	for (offs = 0; offs < 0x2000 ;offs += 2)
	{
		mx = (offs%128)/2;
		my = offs/128;

		if (offs&2)
        	tile = videoram32[offs>>2]&0xffff;
		else
			tile = videoram32[offs>>2]>>16;

        /* Transparency hack, 6010 for PB2, 1205 for PB3 */
		if (tile==0x6010 || tile==0x1205) continue;

        color = (tile>>9) &0x3f;
		fx = tile&0x0100;
		fy = tile&0x8000;

        tile&=0xff;
        if (!tile) continue;

		/* Graphics flip */
		if (flipscreen) {
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			drawgfx(bitmap,Machine->gfx[0],
					tile,
					color,
					fx,fy,
					504+17-(((8*mx)+sx)&0x1ff),504-(((8*my)+sy)&0x1ff),
					cliprect,TRANSPARENCY_PEN,0);
		}
		else
	        drawgfx(bitmap,Machine->gfx[0],
					tile,
					color,
					fx,fy,
					((8*mx)+sx)&0x1ff,((8*my)+sy)&0x1ff,
					cliprect,TRANSPARENCY_PEN,0);
	}
}

/******************************************************************************/
//ks s
#define CALC_ZOOM(p)	{										\
	p##_addition = 0x100 - block_zoom_##p + p##_addition_left;	\
	p##_addition_left = p##_addition & 0xf;						\
	p##_addition = p##_addition >> 4;							\
	zoom##p = p##_addition << 12;								\
}


INLINE void f3_drawgfxzoom( struct mame_bitmap *dest_bmp,const struct GfxElement *gfx,
		unsigned int code,
		unsigned int color,
		int flipx,int flipy,
		int sx,int sy,
		const struct rectangle *clip,
		int scalex, int scaley,
		UINT8 pri_dst)
{
	struct rectangle myclip;
	int transparent_color=0;

	if (!scalex || !scaley) return;

	pri_dst|=4;

//	if (scalex == 0x10000 && scaley == 0x10000)
//	{
//		common_drawgfx(dest_bmp,gfx,code,color,flipx,flipy,sx,sy,clip,transparency,transparent_color,pri_alp_bitmap,pri_mask);
//		return;
//	}


	/*
	scalex and scaley are 16.16 fixed point numbers
	1<<15 : shrink to 50%
	1<<16 : uniform scale
	1<<17 : double to 200%
	*/



	/* KW 991012 -- Added code to force clip to bitmap boundary */
	if(clip)
	{
		myclip.min_x = clip->min_x;
		myclip.max_x = clip->max_x;
		myclip.min_y = clip->min_y;
		myclip.max_y = clip->max_y;

		if (myclip.min_x < 0) myclip.min_x = 0;
		if (myclip.max_x >= dest_bmp->width) myclip.max_x = dest_bmp->width-1;
		if (myclip.min_y < 0) myclip.min_y = 0;
		if (myclip.max_y >= dest_bmp->height) myclip.max_y = dest_bmp->height-1;

		clip=&myclip;
	}


	{
		if( gfx && gfx->colortable )
		{
//			const pen_t *pal = &gfx->colortable[gfx->color_granularity * (color % gfx->total_colors)]; /* ASG 980209 */
			int palBase = &gfx->colortable[gfx->color_granularity * (color % gfx->total_colors)] - Machine->remapped_colortable;
			int source_base = (code % gfx->total_elements) * gfx->height;

			int sprite_screen_height = (scaley*gfx->height+0x8000)>>16;
			int sprite_screen_width = (scalex*gfx->width+0x8000)>>16;

			if (sprite_screen_width && sprite_screen_height)
			{
				/* compute sprite increment per screen pixel */
				int dx = (gfx->width<<16)/sprite_screen_width;
				int dy = (gfx->height<<16)/sprite_screen_height;

				int ex = sx+sprite_screen_width;
				int ey = sy+sprite_screen_height;

				int x_index_base;
				int y_index;

				if( flipx )
				{
					x_index_base = (sprite_screen_width-1)*dx;
					dx = -dx;
				}
				else
				{
					x_index_base = 0;
				}

				if( flipy )
				{
					y_index = (sprite_screen_height-1)*dy;
					dy = -dy;
				}
				else
				{
					y_index = 0;
				}

				if( clip )
				{
					if( sx < clip->min_x)
					{ /* clip left */
						int pixels = clip->min_x-sx;
						sx += pixels;
						x_index_base += pixels*dx;
					}
					if( sy < clip->min_y )
					{ /* clip top */
						int pixels = clip->min_y-sy;
						sy += pixels;
						y_index += pixels*dy;
					}
					/* NS 980211 - fixed incorrect clipping */
					if( ex > clip->max_x+1 )
					{ /* clip right */
						int pixels = ex-clip->max_x-1;
						ex -= pixels;
					}
					if( ey > clip->max_y+1 )
					{ /* clip bottom */
						int pixels = ey-clip->max_y-1;
						ey -= pixels;
					}
				}

				if( ex>sx )
				{ /* skip if inner loop doesn't draw anything */
					int y;
					for( y=sy; y<ey; y++ )
					{
						UINT8 *source = gfx->gfxdata + (source_base+(y_index>>16)) * gfx->line_modulo;
						UINT16 *dest = (UINT16 *)dest_bmp->line[y];
						UINT8 *pri = pri_alp_bitmap->line[y];

						int x, x_index = x_index_base;
						for( x=sx; x<ex; x++ )
						{
							int c = source[x_index>>16];
							if( c != transparent_color )
							{
								UINT8 p=pri[x];
								if (p == 0 || p == 0xff)
								{
									dest[x] = palBase+c;
									pri[x] = pri_dst;
								}
							}
							x_index += dx;
						}
						y_index += dy;
					}
				}
			}
		}
	}
}
//ks e


static void get_sprite_info(const data32_t *spriteram32_ptr)	//ks
{
	int offs,spritecont,flipx,flipy,old_x,old_y,color,x,y;
	int sprite,global_x=0,global_y=0,subglobal_x=0,subglobal_y=0;
	int block_x=0, block_y=0;
	int last_color=0,last_x=0,last_y=0,block_zoom_x=0,block_zoom_y=0;
	int this_x,this_y;
	int y_addition=16, x_addition=16;
	int multi=0;
	int sprite_top;

	int x_addition_left = 8, y_addition_left = 8;
	int zoomx = 0x10000, zoomy = 0x10000;

	struct tempsprite *sprite_ptr = spritelist;

	color=0;
    flipx=flipy=0;
    old_y=old_x=0;
    y=x=0;

	sprite_top=0x1000;
	for (offs = 0; offs < sprite_top; offs += 4)
	{
		/* Check if special command bit is set */
		if (spriteram32_ptr[offs+1] & 0x8000) {
			data32_t cntrl=(spriteram32_ptr[offs+2])&0xffff;
			flipscreen=cntrl&0x2000;

			/*	cntrl&0x1000 = disabled?  (From F2 driver, doesn't seem used anywhere)
				cntrl&0x0010 = ???
				cntrl&0x0020 = ???
			*/

			/* Sprite bank select */
			if (cntrl&1) {
				offs=offs|0x2000;
				sprite_top=sprite_top|0x2000;
			}
			continue;
		}

		/* Check if the sprite list jump command bit is set */
		if ((spriteram32_ptr[offs+3]>>16) & 0x8000) {
			data32_t jump = (spriteram32_ptr[offs+3]>>16)&0x3ff;

			offs=((offs&0x2000)|((jump<<4)/4));
			continue;
		}

		/* Set global sprite scroll */
		if (((spriteram32_ptr[offs+1]>>16) & 0xf000) == 0xa000) {
			global_x = (spriteram32_ptr[offs+1]>>16) & 0xfff;
			if (global_x >= 0x800) global_x -= 0x1000;
			global_y = spriteram32_ptr[offs+1] & 0xfff;
			if (global_y >= 0x800) global_y -= 0x1000;
		}

		/* And sub-global sprite scroll */
		if (((spriteram32_ptr[offs+1]>>16) & 0xf000) == 0x5000) {
			subglobal_x = (spriteram32_ptr[offs+1]>>16) & 0xfff;
			if (subglobal_x >= 0x800) subglobal_x -= 0x1000;
			subglobal_y = spriteram32_ptr[offs+1] & 0xfff;
			if (subglobal_y >= 0x800) subglobal_y -= 0x1000;
		}

		if (((spriteram32_ptr[offs+1]>>16) & 0xf000) == 0xb000) {
			subglobal_x = (spriteram32_ptr[offs+1]>>16) & 0xfff;
			if (subglobal_x >= 0x800) subglobal_x -= 0x1000;
			subglobal_y = spriteram32_ptr[offs+1] & 0xfff;
			if (subglobal_y >= 0x800) subglobal_y -= 0x1000;
			global_y=subglobal_y;
			global_x=subglobal_x;
		}

		/* A real sprite to process! */
		sprite = (spriteram32_ptr[offs]>>16) | ((spriteram32_ptr[offs+2]&1)<<16);
		spritecont = spriteram32_ptr[offs+2]>>24;

/* These games either don't set the XY control bits properly (68020 bug?), or
	have some different mode from the others */
#ifdef DARIUSG_KLUDGE
		if (f3_game==DARIUSG || f3_game==GEKIRIDO || f3_game==CLEOPATR)
			multi=spritecont&0xf0;
#endif

		/* Check if this sprite is part of a continued block */
		if (multi) {
			/* Bit 0x4 is 'use previous colour' for this block part */
			if (spritecont&0x4) color=last_color;
			else color=(spriteram32_ptr[offs+2]>>16)&0xff;

#ifdef DARIUSG_KLUDGE
			if (f3_game==DARIUSG || f3_game==GEKIRIDO || f3_game==CLEOPATR) {
				/* Adjust X Position */
				if ((spritecont & 0x40) == 0) {
					if (spritecont & 0x4) {
						x = block_x;
					} else {
						this_x = spriteram32_ptr[offs+1]>>16;
						if (this_x&0x800) this_x= 0 - (0x800 - (this_x&0x7ff)); else this_x&=0x7ff;

						if ((spriteram32_ptr[offs+1]>>16)&0x8000) {
							this_x+=0;
						} else if ((spriteram32_ptr[offs+1]>>16)&0x4000) {
							/* Ignore subglobal (but apply global) */
							this_x+=global_x;
						} else { /* Apply both scroll offsets */
							this_x+=global_x+subglobal_x;
						}

						x = block_x = this_x;
					}
//ks s
					x_addition_left = 8;
					CALC_ZOOM(x)
//ks e
				}
				else if ((spritecont & 0x80) != 0) {
					x = last_x+x_addition;
//ks s
					CALC_ZOOM(x)
//ks e
				}

				/* Adjust Y Position */
				if ((spritecont & 0x10) == 0) {
					if (spritecont & 0x4) {
						y = block_y;
					} else {
						this_y = spriteram32_ptr[offs+1]&0xffff;
						if (this_y&0x800) this_y= 0 - (0x800 - (this_y&0x7ff)); else this_y&=0x7ff;

						if ((spriteram32_ptr[offs+1]>>16)&0x8000) {
							this_y+=0;
						} else if ((spriteram32_ptr[offs+1]>>16)&0x4000) {
							/* Ignore subglobal (but apply global) */
							this_y+=global_y;
						} else { /* Apply both scroll offsets */
							this_y+=global_y+subglobal_y;
						}

						y = block_y = this_y;
					}
//ks s
					y_addition_left = 8;
					CALC_ZOOM(y)
//ks e
				}
				else if ((spritecont & 0x20) != 0) {
					y = last_y+y_addition;
//ks s
					CALC_ZOOM(y)
//ks e
				}
			} else
#endif
			{
				/* Adjust X Position */
				if ((spritecont & 0x40) == 0) {
					x = block_x;
//ks s
					x_addition_left = 8;
					CALC_ZOOM(x)
//ks e
				}
				else if ((spritecont & 0x80) != 0) {
					x = last_x+x_addition;
//ks s
					CALC_ZOOM(x)
//ks e
				}
				/* Adjust Y Position */
				if ((spritecont & 0x10) == 0) {
					y = block_y;
//ks s
					y_addition_left = 8;
					CALC_ZOOM(y)
//ks e
				}
				else if ((spritecont & 0x20) != 0) {
					y = last_y+y_addition;
//ks s
					CALC_ZOOM(y)
//ks e
				}
				/* Both zero = reread block latch? */
			}
		}
		/* Else this sprite is the possible start of a block */
		else {
			color = (spriteram32_ptr[offs+2]>>16)&0xff;
			last_color=color;

			/* Sprite positioning */
			this_y = spriteram32_ptr[offs+1]&0xffff;
			this_x = spriteram32_ptr[offs+1]>>16;
			if (this_y&0x800) this_y= 0 - (0x800 - (this_y&0x7ff)); else this_y&=0x7ff;
			if (this_x&0x800) this_x= 0 - (0x800 - (this_x&0x7ff)); else this_x&=0x7ff;

			/* Ignore both scroll offsets for this block */
			if ((spriteram32_ptr[offs+1]>>16)&0x8000) {
				this_x+=0;
				this_y+=0;
			} else if ((spriteram32_ptr[offs+1]>>16)&0x4000) {
				/* Ignore subglobal (but apply global) */
				this_x+=global_x;
				this_y+=global_y;
			} else { /* Apply both scroll offsets */
				this_x+=global_x+subglobal_x;
				this_y+=global_y+subglobal_y;
			}

	        block_y = y = this_y;
            block_x = x = this_x;

			block_zoom_x=spriteram32_ptr[offs];
			block_zoom_y=(block_zoom_x>>8)&0xff;
			block_zoom_x&=0xff;
//ks s
			x_addition_left = 8;
			CALC_ZOOM(x)

			y_addition_left = 8;
			CALC_ZOOM(y)
//ks e
		}

		/* These features are common to sprite and block parts */
  		flipx = spritecont&0x1;
		flipy = spritecont&0x2;
		multi = spritecont&0x8;
		last_x=x;
		last_y=y;
//ks		if (block_zoom_x) x_addition=(0x110 - block_zoom_x) / 16; else x_addition=16;
//ks		if (block_zoom_y) y_addition=(0x110 - block_zoom_y) / 16; else y_addition=16;
		if (!sprite) continue;

		if (flipscreen) {
//ks s
			sprite_ptr->x = 512-x_addition-x;
			sprite_ptr->y = 256-y_addition-y;
//ks e
			sprite_ptr->flipx = !flipx;
			sprite_ptr->flipy = !flipy;
		} else {
			sprite_ptr->x = x;
			sprite_ptr->y = y;
			sprite_ptr->flipx = flipx;
			sprite_ptr->flipy = flipy;
		}

		sprite_ptr->code = sprite;
		sprite_ptr->color = color;
//ks s
		sprite_ptr->zoomx = zoomx;
		sprite_ptr->zoomy = zoomy;
		sprite_ptr->pri = (color & 0xc0) >> 6;
//ks e
		sprite_ptr++;
	}
	sprite_end = sprite_ptr;//ks
}

//ks s
static void f3_drawsprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	const struct tempsprite *sprite_ptr;

//	for(sprite_ptr = spritelist; sprite_ptr < sprite_end; sprite_ptr++)

	sprite_ptr = sprite_end;
	while (sprite_ptr != spritelist)
	{
		int col,x,y;
		sprite_ptr--;

		col=sprite_ptr->color;
		x=sprite_ptr->x;
		y=sprite_ptr->y;
//ksdeb s
		if (/*DEBUG_F3*/deb_enable && keyboard_pressed(KEYCODE_A) && sprite_ptr->pri==3) {x=300;y=300;}
		if (/*DEBUG_F3*/deb_enable && keyboard_pressed(KEYCODE_S) && sprite_ptr->pri==2) {x=300;y=300;}
		if (/*DEBUG_F3*/deb_enable && keyboard_pressed(KEYCODE_D) && sprite_ptr->pri==1) {x=300;y=300;}
		if (/*DEBUG_F3*/deb_enable && keyboard_pressed(KEYCODE_F) && sprite_ptr->pri==0) {x=300;y=300;}
		if (flipscreen && f3_game == GSEEKER )
		{
			x += -44;
			y += 17;
		}
//ksdeb e
		f3_drawgfxzoom(bitmap,Machine->gfx[2],
				sprite_ptr->code,
				col,
				sprite_ptr->flipx,sprite_ptr->flipy,
				x,y,
				cliprect,
				sprite_ptr->zoomx,sprite_ptr->zoomy,
				sprite_ptr->pri);
	}
}
//ks e

/******************************************************************************/

//ks s
VIDEO_UPDATE( f3 )
{
	struct rectangle tempclip;
	unsigned int sy_fix[4],sx_fix[4];
	int tile;
	static int deb_sc_x=0,deb_sc_y=0;//ksdeb
	struct mame_bitmap *priority_bitmap_bak;

	skip_this_frame=0;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);

	/* Dynamically decode VRAM chars if dirty */
	if (vram_changed)
		for (tile = 0;tile < 256;tile++)
			if (vram_dirty[tile]) {
				decodechar(Machine->gfx[0],tile,(UINT8 *)f3_vram,Machine->drv->gfxdecodeinfo[0].gfxlayout);
				vram_dirty[tile]=0;
			}
	vram_changed=0;

//	if (scroll_dirty)
	{
		sy_fix[0]=deb_sc_y+((f3_control_0[2]&0xffff0000)>> 7)+(scroll_kludge_y<<16);
		sy_fix[1]=deb_sc_y+((f3_control_0[2]&0x0000ffff)<< 9)+((scroll_kludge_y+pf23_y_kludge)<<16);
		sy_fix[2]=deb_sc_y+((f3_control_0[3]&0xffff0000)>> 7)+((scroll_kludge_y+pf23_y_kludge)<<16);
		sy_fix[3]=deb_sc_y+((f3_control_0[3]&0x0000ffff)<< 9)+(scroll_kludge_y<<16);
		sx_fix[0]=deb_sc_x+((f3_control_0[0]&0xffc00000)>> 6)-((6+scroll_kludge_x)<<16);
		sx_fix[1]=deb_sc_x+((f3_control_0[0]&0x0000ffc0)<<10)-((10+scroll_kludge_x)<<16);
		sx_fix[2]=deb_sc_x+((f3_control_0[1]&0xffc00000)>> 6)-((14+scroll_kludge_x)<<16);
		sx_fix[3]=deb_sc_x+((f3_control_0[1]&0x0000ffc0)<<10)-((18+scroll_kludge_x)<<16);

//		sx_fix[0]+=((f3_control_0[0]&0x003f0000)>> 6)^0xfc00;
//		sx_fix[1]+=((f3_control_0[0]&0x0000003f)<<10)^0xfc00;
//		sx_fix[2]+=((f3_control_0[1]&0x003f0000)>> 6)^0xfc00;
//		sx_fix[3]+=((f3_control_0[1]&0x0000003f)<<10)^0xfc00;

		sx_fix[0]-=((f3_control_0[0]&0x003f0000)>> 6)+0x0400-0x10000;
		sx_fix[1]-=((f3_control_0[0]&0x0000003f)<<10)+0x0400-0x10000;
		sx_fix[2]-=((f3_control_0[1]&0x003f0000)>> 6)+0x0400-0x10000;
		sx_fix[3]-=((f3_control_0[1]&0x0000003f)<<10)+0x0400-0x10000;

		if (flipscreen)
		{
			sy_fix[0]= 0x3000000-sy_fix[0];
			sy_fix[1]= 0x3000000-sy_fix[1];
			sy_fix[2]= 0x3000000-sy_fix[2];
			sy_fix[3]= 0x3000000-sy_fix[3];
			sx_fix[0]=-0x1a00000-sx_fix[0];
			sx_fix[1]=-0x1a00000-sx_fix[1];
			sx_fix[2]=-0x1a00000-sx_fix[2];
			sx_fix[3]=-0x1a00000-sx_fix[3];
		}
	}
//	scroll_dirty=1;// 0

	/* Update pivot layer */
	f3_update_pivot_layer();


	fillbitmap(pri_alp_bitmap,0,cliprect);
// fillbitmap(bitmap,255,cliprect);

	/* Pixel layer */
	tempclip = pixel_layer_clip;
	sect_rect(&tempclip,cliprect);
if (!deb_enable || !keyboard_pressed(KEYCODE_N))//ksdeb
{
	priority_bitmap_bak=priority_bitmap;
	priority_bitmap=pri_alp_bitmap;
//	tilemap_draw(NULL,&tempclip,pixel_layer,0,0xff);
	tilemap_draw(bitmap,&tempclip,pixel_layer,0,0xff);
	priority_bitmap=priority_bitmap_bak;
}


	/* sprites */
	if (sprite_lag==0) get_sprite_info(spriteram32);
if (!deb_enable || !keyboard_pressed(KEYCODE_B))//ksdeb
	f3_drawsprites(sprite_bitmap,cliprect);


//ksdeb s
deb_tileflag=0;
if (deb_enable && keyboard_pressed(KEYCODE_G))
{
	deb_tileflag=1;
	tilemap_mark_all_tiles_dirty( pf1_tilemap );
	tilemap_mark_all_tiles_dirty( pf2_tilemap );
	tilemap_mark_all_tiles_dirty( pf3_tilemap );
	tilemap_mark_all_tiles_dirty( pf4_tilemap );
}
//ksdeb e


	/* Playfield */
	get_line_ram_info(pf1_tilemap,sx_fix[0],sy_fix[0],0);
	get_line_ram_info(pf2_tilemap,sx_fix[1],sy_fix[1],1);
	get_line_ram_info(pf3_tilemap,sx_fix[2],sy_fix[2],2);
	get_line_ram_info(pf4_tilemap,sx_fix[3],sy_fix[3],3);
	f3_tilemap_draw(bitmap,cliprect);


	/* vram layer */
if (!deb_enable || !keyboard_pressed(KEYCODE_M))//ksdeb
	f3_draw_vram_layer(bitmap,cliprect);

#if DEBUG_F3
	if (keyboard_pressed(KEYCODE_O))
		print_debug_info(0,0,0,0,0,0,0,0);

//ksdeb s
	{/*******************************************************************************************/
		static int debdisp = 0;
		static char deb_buf[10][80];
		static int cz_pos=0,cz_line=24;


		if (!keyboard_pressed(KEYCODE_LSHIFT) && keyboard_pressed_memory(KEYCODE_F1))
		{
			deb_alpha_disable=!deb_alpha_disable;
			if(deb_alpha_disable)
				usrintf_showmessage("alpha blending:off");
			else usrintf_showmessage("alpha blending:on");

		}

		if (keyboard_pressed(KEYCODE_LSHIFT) && keyboard_pressed_memory(KEYCODE_F1))
		{
			deb_enable=!deb_enable;
			if(!deb_enable)
			{
				debdisp = 0;
				usrintf_showmessage("debug mode:off");
			}
			else usrintf_showmessage("debug mode:on");

		}

		if (deb_enable && keyboard_pressed_memory(KEYCODE_Q))
		{
			debdisp++;
			if(debdisp==4) debdisp = 0;
		}

		if(debdisp)
		{
			int sft;
			if (keyboard_pressed(KEYCODE_K))	cz_line--;
			if (keyboard_pressed(KEYCODE_L))	cz_line++;
			if (keyboard_pressed_memory(KEYCODE_I))	cz_line--;
			if (keyboard_pressed_memory(KEYCODE_O))	cz_line++;
			cz_line=cz_line & 0xff;
			sft=16*~(cz_line & 1);

			if(debdisp==2)
			{
				sprintf(deb_buf[0],"LINE:%3d Z:%04x R:%04x C:%04x",cz_line,
									(f3_line_ram[(0x0800+cz_line*2)/4]>>sft)&0xffff,
									(f3_line_ram[(0x0c00+cz_line*2)/4]>>sft)&0xffff,
									(f3_line_ram[(0x0000+cz_line*2)/4]>>sft)&0xffff
						);

				for(cz_pos=0;cz_pos<4;cz_pos++)
					sprintf(deb_buf[1+cz_pos],"Layer:%2d z:%04x r:%04x c:%04x",cz_pos,
									(f3_line_ram[(0x8000+cz_pos*256*2+cz_line*2)/4]>>sft)&0xffff,
									(f3_line_ram[(0xa000+cz_pos*256*2+cz_line*2)/4]>>sft)&0xffff,
									(f3_line_ram[(0x4000+cz_pos*256*2+cz_line*2)/4]>>sft)&0xffff
							);
			}
			else if(debdisp==3)
			{
				int deb_sx[4],deb_sx_fix[4];
				int deb_sy[4],deb_sy_fix[4];

				deb_sx[0]=(f3_control_0[0]&0xffc00000)>>22;
				deb_sx[1]=(f3_control_0[0]&0x0000ffc0)>>6;
				deb_sx[2]=(f3_control_0[1]&0xffc00000)>>22;
				deb_sx[3]=(f3_control_0[1]&0x0000ffc0)>>6;

				deb_sx_fix[0]=(f3_control_0[0]&0x003f0000)>>14;
				deb_sx_fix[1]=(f3_control_0[0]&0x0000003f)<<2;
				deb_sx_fix[2]=(f3_control_0[1]&0x003f0000)>>14;
				deb_sx_fix[3]=(f3_control_0[1]&0x0000003f)<<2;

				deb_sy[0]=(f3_control_0[2]&0xffff0000)>>23;
				deb_sy[1]=(f3_control_0[2]&0x0000ffff)>>7;
				deb_sy[2]=(f3_control_0[3]&0xffff0000)>>23;
				deb_sy[3]=(f3_control_0[3]&0x0000ffff)>>7;

				deb_sy_fix[0]=(f3_control_0[2]&0x007f0000)>>15;
				deb_sy_fix[1]=(f3_control_0[2]&0x0000007f)<<1;
				deb_sy_fix[2]=(f3_control_0[3]&0x007f0000)>>15;
				deb_sy_fix[3]=(f3_control_0[3]&0x0000007f)<<1;

				for(cz_pos=0;cz_pos<4;cz_pos++)
					sprintf(deb_buf[1+cz_pos],"Layer:%2d x:%03x.%02x y:%03x.%02x",cz_pos,
									deb_sx[cz_pos],deb_sx_fix[cz_pos],
									deb_sy[cz_pos],deb_sy_fix[cz_pos]
							);
			}
			else if(debdisp==1)
			{
				sprintf(deb_buf[0],"LINE:%3d S:%04x T:%04x A:%04x ?:%04x",cz_line,
									(f3_line_ram[(0x0600+cz_line*2)/4]>>sft)&0xffff,
									(f3_line_ram[(0x0e00+cz_line*2)/4]>>sft)&0xffff,
									(f3_line_ram[(0x0400+cz_line*2)/4]>>sft)&0xffff,
									(f3_line_ram[(0x0200+cz_line*2)/4]>>sft)&0xffff
						);

				for(cz_pos=0;cz_pos<4;cz_pos++)
					sprintf(deb_buf[1+cz_pos],"Layer:%2d s:%04x t:%04x a:%04x ?:%04x",cz_pos,
										(f3_line_ram[(0x7000+cz_pos*256*2+cz_line*2)/4]>>sft)&0xffff,
										(f3_line_ram[(0xb000+cz_pos*256*2+cz_line*2)/4]>>sft)&0xffff,
										(f3_line_ram[(0x6000+cz_pos*256*2+cz_line*2)/4]>>sft)&0xffff,
										(f3_line_ram[(0x5000+cz_pos*256*2+cz_line*2)/4]>>sft)&0xffff
							);
			}

			if(1)
			{
				if (keyboard_pressed_memory(KEYCODE_0_PAD))	deb_sc_x-=0x0400;
				if (keyboard_pressed_memory(KEYCODE_DEL_PAD))	deb_sc_x+=0x0400;
				if (keyboard_pressed_memory(KEYCODE_1_PAD))	deb_sc_x-=0x10000;
				if (keyboard_pressed_memory(KEYCODE_2_PAD))	deb_sc_x+=0x10000;

				if (keyboard_pressed_memory(KEYCODE_4_PAD))	deb_sc_y-=0x0200;
				if (keyboard_pressed_memory(KEYCODE_5_PAD))	deb_sc_y+=0x0200;
				if (keyboard_pressed_memory(KEYCODE_7_PAD))	deb_sc_y-=0x10000;
				if (keyboard_pressed_memory(KEYCODE_8_PAD))	deb_sc_y+=0x10000;
				sprintf(deb_buf[5],"sc offset x:%8x y:%8x",deb_sc_x,deb_sc_y);

				sprintf(deb_buf[6],"flipscreen:%d  loop:%d",flipscreen,deb_loop);

				if (keyboard_pressed_memory(KEYCODE_H))	deb_tile_code--;
				if (keyboard_pressed_memory(KEYCODE_J))	deb_tile_code++;
				deb_tile_code&=0x1f;
				sprintf(deb_buf[7],"tile code flg:%02x",deb_tile_code);
			}
			else
			{
				if (keyboard_pressed(KEYCODE_0_PAD))	deb_alpha_level_a-=1;
				if (keyboard_pressed(KEYCODE_DEL_PAD))	deb_alpha_level_a+=1;
				if (keyboard_pressed_memory(KEYCODE_1_PAD))	deb_alpha_level_a-=1;
				if (keyboard_pressed_memory(KEYCODE_2_PAD))	deb_alpha_level_a+=1;

				if (keyboard_pressed(KEYCODE_4_PAD))	deb_alpha_level_b-=1;
				if (keyboard_pressed(KEYCODE_5_PAD))	deb_alpha_level_b+=1;
				if (keyboard_pressed_memory(KEYCODE_7_PAD))	deb_alpha_level_b-=1;
				if (keyboard_pressed_memory(KEYCODE_8_PAD))	deb_alpha_level_b+=1;

				deb_alpha_level_a &= 0xff;
				deb_alpha_level_b &= 0xff;

				if (keyboard_pressed_memory(KEYCODE_6_PAD)) deb_alp_mode++;
				if (deb_alp_mode>2 ) deb_alp_mode=0;

				sprintf(deb_buf[5],"mode:%d alpha_a:%2x alpha_b:%2x",
									deb_alp_mode,deb_alpha_level_a,deb_alpha_level_b );

				{//ksdebA
					int al_s,al_d;
					int deb_alpha_level_s0,deb_alpha_level_d0;
					int deb_alpha_level_s1,deb_alpha_level_d1;

					int a=(f3_line_ram[(0x6000+1*256*2+cz_line*2)/4]>>sft)&0xffff;
					int b=(a>>8)&0xf;
//					int c=(a>>4)&0xf;
					int d=(a>>0)&0xf;
					a>>=12;

					/* b000 */
					al_s = ( (15-d)*256) / 8;
					al_d = ( (15-b)*256) / 8;
					if(al_s>255) al_s = 255;
					if(al_d>255) al_d = 255;
					deb_alpha_level_s0 = al_s;
					deb_alpha_level_d0 = al_d;

					/* 7000 */
					if(a==11 || a>=b)	al_s = ((15-b)*256) / 8;
					else 				al_s = ((15-a)*256) / 8;
					if(al_s>255) al_s = 255;
					al_d = 255-al_s;
					deb_alpha_level_s1 = al_s;
					deb_alpha_level_d1 = al_d;

					sprintf(deb_buf[6],"alpb src:%4d dst:%4d",deb_alpha_level_s0,deb_alpha_level_d0);
					sprintf(deb_buf[7],"alp7 src:%4d dst:%4d",deb_alpha_level_s1,deb_alpha_level_d1);
				}
			}

			ui_text(bitmap,deb_buf[0],0,Machine->uifontheight*0);
			ui_text(bitmap,deb_buf[1],0,Machine->uifontheight*1);
			ui_text(bitmap,deb_buf[2],0,Machine->uifontheight*2);
			ui_text(bitmap,deb_buf[3],0,Machine->uifontheight*3);
			ui_text(bitmap,deb_buf[4],0,Machine->uifontheight*4);
			ui_text(bitmap,deb_buf[5],0,Machine->uifontheight*5);
			ui_text(bitmap,deb_buf[6],0,Machine->uifontheight*6);
			ui_text(bitmap,deb_buf[7],0,Machine->uifontheight*7);
			memset(deb_buf, 0x00, sizeof(deb_buf));
		}
	}
//ksdeb e
#endif
}
//ks e
