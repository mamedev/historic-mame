/***************************************************************************

						  -= Unico 16 Bit Games =-

					driver by	Luca Elia (l.elia@tin.it)


Note:	if MAME_DEBUG is defined, pressing Z with:

		Q / W / E		Shows Layer 0 / 1 / 2
		A				Shows Sprites

		Keys can be used together!


	[ 3 Scrolling Layers ]

		Tile Size:				16 x 16 x 8
		Layer Size (tiles):		64 x 64

	[ 512 Sprites ]

		Sprites are made of 16 x 16 x 8 tiles. Size can vary from 1 to
		16 tiles horizontally, while their height is always 1 tile.
		There seems to be 4 levels of priority (wrt layers) for each
		sprite, following this simple scheme:

		[if we denote the three layers with 0-3 (0 being the backmost)
		 and the sprite with S]

		Sprite Priority			Order (back -> front)
				0					S 0 1 2
				1					0 S 1 2
				2					0 1 S 2
				3					0 1 2 S

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Variables defined in drivers: */

extern int unico16_has_lightgun;

/* Variables needed by drivers: */

data16_t *unico16_vram_0, *unico16_scrollx_0, *unico16_scrolly_0;
data16_t *unico16_vram_1, *unico16_scrollx_1, *unico16_scrolly_1;
data16_t *unico16_vram_2, *unico16_scrollx_2, *unico16_scrolly_2;


/***************************************************************************

									Palette

	Byte: 	0	1	2	3
	Gun:	R	G	B	0

	6 Bits x Gun

***************************************************************************/

WRITE16_HANDLER( unico16_palette_w )
{
	data16_t data1, data2;
	COMBINE_DATA(&paletteram16[offset]);
	data1 = paletteram16[offset & ~1];
	data2 = paletteram16[offset |  1];
	palette_set_color( offset/2,
		 (data1 >> 8) & 0xFC,
		 (data1 >> 0) & 0xFC,
		 (data2 >> 8) & 0xFC	);
}



/***************************************************************************

								Tilemaps

	Offset:		Bits:					Value:

		0.w								Code
		2.w		fedc ba98 7--- ----
				---- ---- -6-- ----		Flip Y
				---- ---- --5- ----		Flip X
				---- ---- ---4 3210		Color

***************************************************************************/

#define LAYER( _N_ ) \
static struct tilemap *tilemap_##_N_; \
\
static void get_tile_info_##_N_(int tile_index) \
{ \
	data16_t code = unico16_vram_##_N_[ 2 * tile_index + 0 ]; \
	data16_t attr = unico16_vram_##_N_[ 2 * tile_index + 1 ]; \
	SET_TILE_INFO(1, code, attr & 0x1f, TILE_FLIPYX( attr >> 5 )) \
} \
\
WRITE16_HANDLER( unico16_vram_##_N_##_w ) \
{ \
	data16_t old_data	=	unico16_vram_##_N_[offset]; \
	data16_t new_data	=	COMBINE_DATA(&unico16_vram_##_N_[offset]); \
	if (old_data != new_data)	tilemap_mark_tile_dirty(tilemap_##_N_,offset/2); \
}

LAYER( 0 )
LAYER( 1 )
LAYER( 2 )



/***************************************************************************


							Video Hardware Init


***************************************************************************/

static int sprites_scrolldx, sprites_scrolldy;

VIDEO_START( unico16 )
{
	tilemap_0 = tilemap_create(	get_tile_info_0,tilemap_scan_rows,
								TILEMAP_TRANSPARENT,	16,16,	0x40, 0x40);

	tilemap_1 = tilemap_create(	get_tile_info_1,tilemap_scan_rows,
								TILEMAP_TRANSPARENT,	16,16,	0x40, 0x40);

	tilemap_2 = tilemap_create(	get_tile_info_2,tilemap_scan_rows,
								TILEMAP_TRANSPARENT,	16,16,	0x40, 0x40);

	if (!tilemap_0 || !tilemap_1 || !tilemap_2)	return 1;

	sprites_scrolldx = -0x40;
	sprites_scrolldy = -0x0f;
	tilemap_set_scrolldx(tilemap_0,-0x31,0);
	tilemap_set_scrolldx(tilemap_1,-0x2f,0);
	tilemap_set_scrolldx(tilemap_2,-0x2d,0);

	tilemap_set_scrolldy(tilemap_0,-0x10,0);
	tilemap_set_scrolldy(tilemap_1,-0x10,0);
	tilemap_set_scrolldy(tilemap_2,-0x10,0);

	tilemap_set_transparent_pen(tilemap_0,0x00);
	tilemap_set_transparent_pen(tilemap_1,0x00);
	tilemap_set_transparent_pen(tilemap_2,0x00);
	return 0;
}



/***************************************************************************


								Sprites Drawing


		0.w								X

		2.w								Y

		4.w								Code

		6.w		fedc ba98 7--- ----
				---- ---- -6-- ----		Flip Y?
				---- ---- --5- ----		Flip X
				---- ---- ---4 3210		Color


***************************************************************************/

static void unico16_draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int offs;

	/* Draw them backwards, for pdrawgfx */
	for ( offs = (spriteram_size-8)/2; offs >= 0 ; offs -= 8/2 )
	{
		int x, startx, endx, incx;

		int	sx			=	spriteram16[ offs + 0 ];
		int	sy			=	spriteram16[ offs + 1 ];
		int	code		=	spriteram16[ offs + 2 ];
		int	attr		=	spriteram16[ offs + 3 ];

		int	flipx		=	attr & 0x020;
		int	flipy		=	attr & 0x040;	// not sure

		int dimx		=	((attr >> 8) & 0xf) + 1;

		int priority	=	((attr >> 12) & 0x3);
		int pri_mask;

		switch( priority )
		{
			case 0:		pri_mask = 0xfe;	break;	// below all
			case 1:		pri_mask = 0xf0;	break;	// above layer 0
			case 2:		pri_mask = 0xfc;	break;	// above layer 1
			default:
			case 3:		pri_mask = 0x00;			// above all
		}

		sx	+=	sprites_scrolldx;
		sy	+=	sprites_scrolldy;

		sx	=	(sx & 0x1ff) - (sx & 0x200);
		sy	=	(sy & 0x1ff) - (sy & 0x200);

		if (flipx)	{	startx = sx+(dimx-1)*16;	endx = sx-16;		incx = -16;	}
		else		{	startx = sx;				endx = sx+dimx*16;	incx = +16;	}

		for (x = startx ; x != endx ; x += incx)
		{
			pdrawgfx(	bitmap, Machine->gfx[0],
						code++,
						attr & 0x1f,
						flipx, flipy,
						x, sy,
						cliprect, TRANSPARENCY_PEN,0x00,
						pri_mask	);
		}

#ifdef MAME_DEBUG
#if 0
if (keyboard_pressed(KEYCODE_X))
{	/* Display some info on each sprite */
	struct DisplayText dt[2];	char buf[10];
	sprintf(buf,"%X",priority);
	dt[0].text = buf;	dt[0].color = UI_COLOR_NORMAL;
	dt[0].x = sx;		dt[0].y = sy;
	dt[1].text = 0;	/* terminate array */
	displaytext(Machine->scrbitmap,dt);		}
#endif
#endif
	}
}



/***************************************************************************


								Screen Drawing


***************************************************************************/

VIDEO_UPDATE( unico16 )
{
	int gunx[256] = {
		0x160,0x162,0x164,0x165,
		0x167,0x169,0x16A,0x16C,0x16E,0x16F,0x171,0x172,0x174,0x176,0x177,0x179,0x17B,0x17C,0x17E,0x17F,
		0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,0x000,
		0x000,0x000,0x000,0x000,0x000,0x001,0x003,0x004,0x006,0x008,0x009,0x00B,0x00D,0x00E,0x010,0x011,
		0x013,0x015,0x016,0x018,0x01A,0x01B,0x01D,0x01F,0x020,0x022,0x023,0x025,0x027,0x028,0x02A,0x02C,
		0x02D,0x02F,0x031,0x032,0x034,0x035,0x037,0x039,0x03A,0x03C,0x03E,0x03F,0x041,0x042,0x044,0x046,
		0x047,0x049,0x04B,0x04C,0x04E,0x050,0x051,0x053,0x054,0x056,0x058,0x059,0x05B,0x05D,0x05E,0x060,
		0x062,0x063,0x065,0x066,0x068,0x06A,0x06B,0x06D,0x06F,0x070,0x072,0x074,0x075,0x077,0x078,0x07A,
		0x07C,0x07D,0x07F,0x081,0x082,0x084,0x085,0x087,0x089,0x08A,0x08C,0x08E,0x08F,0x091,0x093,0x094,
		0x096,0x097,0x099,0x09B,0x09C,0x09E,0x0A0,0x0A1,0x0A3,0x0A5,0x0A6,0x0A8,0x0A9,0x0AB,0x0AD,0x0AE,
		0x0B0,0x0B2,0x0B3,0x0B5,0x0B7,0x0B8,0x0BA,0x0BB,0x0BD,0x0BF,0x0C0,0x0C2,0x0C4,0x0C5,0x0C7,0x0C8,
		0x0CA,0x0CC,0x0CD,0x0CF,0x0D1,0x0D2,0x0D4,0x0D6,0x0D7,0x0D9,0x0DA,0x0DC,0x0DE,0x0DF,0x0E1,0x0E3,
		0x0E4,0x0E6,0x0E8,0x0E9,0x0EB,0x0EC,0x0EE,0x0F0,0x0F1,0x0F3,0x0F5,0x0F6,0x0F8,0x0FA,0x0FB,0x0FD,
		0x0FE,0x100,0x102,0x103,0x105,0x107,0x108,0x10A,0x10B,0x10D,0x10F,0x110,0x112,0x114,0x115,0x117,
		0x119,0x11A,0x11C,0x11D,0x11F,0x121,0x122,0x124,0x126,0x127,0x129,0x12B,0x12C,0x12E,0x12F,0x131,
		0x133,0x134,0x136,0x138,0x139,0x13B,0x13D,0x13E,0x140,0x141,0x143,0x145,0x146,0x148,0x14A,0x14B,
		0x14D,0x14E,0x150,0x152,0x153,0x155,0x157,0x158,0x15A,0x15C,0x15D,0x15F		};

	int layers_ctrl = -1;

	tilemap_set_scrollx(tilemap_0, 0, *unico16_scrollx_0);
	tilemap_set_scrolly(tilemap_0, 0, *unico16_scrolly_0);

	tilemap_set_scrollx(tilemap_1, 0, *unico16_scrollx_1);
	tilemap_set_scrolly(tilemap_1, 0, *unico16_scrolly_1);

	tilemap_set_scrolly(tilemap_2, 0, *unico16_scrolly_2);
	tilemap_set_scrollx(tilemap_2, 0, *unico16_scrollx_2);

#ifdef MAME_DEBUG
if ( keyboard_pressed(KEYCODE_Z) || keyboard_pressed(KEYCODE_X) )
{
	int msk = 0;
	if (keyboard_pressed(KEYCODE_Q))	msk |= 1;
	if (keyboard_pressed(KEYCODE_W))	msk |= 2;
	if (keyboard_pressed(KEYCODE_E))	msk |= 4;
	if (keyboard_pressed(KEYCODE_A))	msk |= 8;
	if (msk != 0) layers_ctrl &= msk;
}
#endif

	/* The background color is the first of the last palette */
	fillbitmap(bitmap,Machine->pens[0x1f00],cliprect);
	fillbitmap(priority_bitmap,0,cliprect);

	if (layers_ctrl & 1)	tilemap_draw(bitmap,cliprect,tilemap_0,0,1);
	if (layers_ctrl & 2)	tilemap_draw(bitmap,cliprect,tilemap_1,0,2);
	if (layers_ctrl & 4)	tilemap_draw(bitmap,cliprect,tilemap_2,0,4);

	/* Sprites are drawn last, using pdrawgfx */
	if (layers_ctrl & 8)	unico16_draw_sprites(bitmap,cliprect);

	/* Draw the gunsight for ligth gun games */
	if (unico16_has_lightgun)
		draw_crosshair(bitmap,
			gunx[readinputport(6)&0xff] - 0x08 -3,
			readinputport(5) -0x17 -3,
			cliprect);
}
