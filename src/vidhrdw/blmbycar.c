/***************************************************************************

							  -= Blomby Car =-

					driver by	Luca Elia (eliavit@unina.it)


Note:	if MAME_DEBUG is defined, pressing Z with:

		Q		shows the background
		W		shows the foreground
		A		shows the sprites

		Keys can be used together!


	[ 2 Scrolling Layers ]

	The Tilemaps are 64 x 32 tiles in size (1024 x 512).
	Tiles are 16 x 16 x 4, with 32 color codes and 2 priority
	leves (wrt sprites). Each tile needs 4 bytes.

	[ 1024? Sprites ]

	They use the same graphics the tilemaps use (16 x 16 x 4 tiles)
	with 16 color codes and 2 levels of priority


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Variables needed by driver: */

data16_t *blmbycar_vram_0, *blmbycar_scroll_0;
data16_t *blmbycar_vram_1, *blmbycar_scroll_1;


/***************************************************************************


								Palette


***************************************************************************/

/* xxxxBBBBGGGGRRRR */

WRITE16_HANDLER( blmbycar_palette_w )
{
	int r,g,b;
	data = COMBINE_DATA(&paletteram16[offset]);
	g = (data >> 0) & 0xF;
	r = (data >> 4) & 0xF;
	b = (data >> 8) & 0xF;
	palette_change_color( offset, r * 0x11, g * 0x11, b * 0x11 );
}



/***************************************************************************


								Tilemaps

	Offset:		Bits:					Value:

		0.w								Code
		2.w		fedc ba98 ---- ----
				---- ---- 7--- ----		Flip Y
				---- ---- -6-- ----		Flip X
				---- ---- --5- ----		Priority (0 = Low)
				---- ---- ---4 3210		Color

***************************************************************************/

static struct tilemap *tilemap_0, *tilemap_1;

#define DIM_NX		(0x40)
#define DIM_NY		(0x20)

static void get_tile_info_0( int tile_index )
{
	data16_t code = blmbycar_vram_0[ tile_index * 2 + 0 ];
	data16_t attr = blmbycar_vram_0[ tile_index * 2 + 1 ];
	SET_TILE_INFO( 0 , code, attr & 0x1f );
	tile_info.priority = (attr >> 5) & 1;
	tile_info.flags = TILE_FLIPYX( (attr >> 6) & 3 );
}

static void get_tile_info_1( int tile_index )
{
	data16_t code = blmbycar_vram_1[ tile_index * 2 + 0 ];
	data16_t attr = blmbycar_vram_1[ tile_index * 2 + 1 ];
	SET_TILE_INFO( 0 , code, attr & 0x1f );
	tile_info.priority = (attr >> 5) & 1;
	tile_info.flags = TILE_FLIPYX( (attr >> 6) & 3 );
}


WRITE16_HANDLER( blmbycar_vram_0_w )
{
	data16_t oldword = blmbycar_vram_0[offset];
	data16_t newword = COMBINE_DATA(&blmbycar_vram_0[offset]);
	if (oldword != newword)	tilemap_mark_tile_dirty(tilemap_0, offset/2);
}

WRITE16_HANDLER( blmbycar_vram_1_w )
{
	data16_t oldword = blmbycar_vram_1[offset];
	data16_t newword = COMBINE_DATA(&blmbycar_vram_1[offset]);
	if (oldword != newword)	tilemap_mark_tile_dirty(tilemap_1, offset/2);
}


/***************************************************************************


								Video Init


***************************************************************************/

int blmbycar_vh_start(void)
{
	tilemap_0 = tilemap_create(	get_tile_info_0, tilemap_scan_rows,
								TILEMAP_OPAQUE, 16,16, DIM_NX, DIM_NY );

	tilemap_1 = tilemap_create(	get_tile_info_1, tilemap_scan_rows,
								TILEMAP_TRANSPARENT, 16,16, DIM_NX, DIM_NY );

	if (tilemap_0 && tilemap_1)
	{
		tilemap_set_scroll_rows(tilemap_0,1);
		tilemap_set_scroll_cols(tilemap_0,1);
		tilemap_set_transparent_pen(tilemap_0,0);

		tilemap_set_scroll_rows(tilemap_1,1);
		tilemap_set_scroll_cols(tilemap_1,1);
		tilemap_set_transparent_pen(tilemap_1,0);

		return 0;
	}
	else return 1;
}


/***************************************************************************


								Sprites Drawing

	Offset:		Bits:					Value:

		0.w		f--- ---- ---- ----		End Of Sprites
				-edc ba9- ---- ----
				---- ---8 7654 3210		Y (Signed)

		2.w								Code

		4.w		f--- ---- ---- ----		Flip Y
				-e-- ---- ---- ----		Flip X
				--dc ba98 7654 ----
				---- ---- ---- 3210		Color (Bit 3 = Priority)

		6.w		f--- ---- ---- ----		? Is this ever used ?
				-e-- ---- ---- ----		? 1 = Don't Draw ?
				--dc ba9- ---- ----
				---- ---8 7654 3210		X (Signed)


***************************************************************************/

static void blmbycar_draw_sprites(struct osd_bitmap *bitmap)
{
	data16_t *source, *finish;

	source = spriteram16 + 0x6/2;				// !
	finish = spriteram16 + spriteram_size/2;

	/* Find "the end of sprites" marker */

	for ( ; source < finish; source += 8/2 )
		if (source[0] & 0x8000)	break;

	/* Draw sprites in reverse order for pdrawfgfx */

	source -= 8/2;
	finish = spriteram16;

	for ( ; source >= finish; source -= 8/2 )
	{
		int	y			=	source[0];
		int	code		=	source[1];
		int	attr		=	source[2];
		int	x			=	source[3];

		int	flipx		=	attr & 0x4000;
		int	flipy		=	attr & 0x8000;
		int	pri			=	(~attr >> 3) & 0x1;		// Priority (1 = Low)
		int pri_mask	=	~((1 << (pri+1)) - 1);	// Above the first "pri" levels

		if (x & 0x4000)	continue;	// ? To get rid of the "shadow" blocks

		x	=	(x & 0x1ff) - 0x10;
		y	=	0xf0 - ((y & 0xff)  - (y & 0x100));

		pdrawgfx(	bitmap, Machine->gfx[0],
					code,
					0x20 + (attr & 0xf),
					flipx, flipy,
					x, y,
					&Machine->visible_area, TRANSPARENCY_PEN,0,
					pri_mask	);
	}
}


static void blmbycar_mark_sprites_colors(void)
{
	int count = 0;
	int i,col,colmask[0x10];

	unsigned int *pen_usage	=	Machine->gfx[0]->pen_usage;
	int total_elements		=	Machine->gfx[0]->total_elements;

	int xmin = Machine->visible_area.min_x;
	int xmax = Machine->visible_area.max_x;
	int ymin = Machine->visible_area.min_y;
	int ymax = Machine->visible_area.max_y;

	data16_t *source, *finish;

	memset(colmask, 0, sizeof(colmask));

	source = spriteram16 + 0x6/2;				// !
	finish = spriteram16 + spriteram_size/2;

	for ( ; source < finish; source += 8/2 )
	{
		int	y			=	source[0];
		int	code		=	source[1] % total_elements;
		int	color		=	source[2] & 0xf;
		int	x			=	source[3];

		if (y & 0x8000)	break;
		if (x & 0x4000)	continue;	// ? To get rid of the "shadow" blocks

		x	=	(x & 0x1ff) - 0x10;
		y	=	0xf0 - ((y & 0xff)  - (y & 0x100));

		if (((x+15) >= xmin) && (x <= xmax) &&
			((y+15) >= ymin) && (y <= ymax))
			colmask[color] |= pen_usage[code];
	}

	for (col = 0; col < 0x10; col++)
	 for (i = 1; i < 16; i++)	// pen 0 is transparent
	  if (colmask[col] & (1 << i))
	  {	palette_used_colors[16 * col + i + 0x20 * 0x10] = PALETTE_COLOR_USED;
		count++;	}

#if 0
{	char buf[80];
	sprintf(buf,"%d",count);
	usrintf_showmessage(buf);	}
#endif
}


/***************************************************************************


								Screen Drawing


***************************************************************************/

void blmbycar_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i,layers_ctrl = -1;

	tilemap_set_scrolly( tilemap_0, 0, blmbycar_scroll_0[ 0 ]);
	tilemap_set_scrollx( tilemap_0, 0, blmbycar_scroll_0[ 1 ]);

	tilemap_set_scrolly( tilemap_1, 0, blmbycar_scroll_1[ 0 ]);
	tilemap_set_scrollx( tilemap_1, 0, blmbycar_scroll_1[ 1 ]);

#ifdef MAME_DEBUG
if (keyboard_pressed(KEYCODE_Z))
{
	int msk = 0;

	if (keyboard_pressed(KEYCODE_Q))	msk |= 1;
	if (keyboard_pressed(KEYCODE_W))	msk |= 2;
//	if (keyboard_pressed(KEYCODE_E))	msk |= 4;
	if (keyboard_pressed(KEYCODE_A))	msk |= 8;
	if (msk != 0) layers_ctrl &= msk;
}
#endif

	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();

	blmbycar_mark_sprites_colors();

	palette_recalc();

	fillbitmap(priority_bitmap,0,NULL);

	if (layers_ctrl&1)
		for (i = 0; i <= 1; i++)
			tilemap_draw(bitmap, tilemap_0, i, i);
	else	fillbitmap(bitmap,palette_transparent_pen,&Machine->visible_area);

	if (layers_ctrl&2)
		for (i = 0; i <= 1; i++)
			tilemap_draw(bitmap, tilemap_1, i, i);

	if (layers_ctrl&8)
		blmbycar_draw_sprites(bitmap);
}
