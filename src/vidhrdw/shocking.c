/***************************************************************************

								  -= Shocking =-

					driver by	Luca Elia (eliavit@unina.it)


Note:	if MAME_DEBUG is defined, pressing Z with:

		Q		shows the background
		W		shows the foreground
		A		shows the sprites

		Keys can be used together!


	[ 2 Scrolling Layers ]

	Tiles are 16 x 16 x 8. The layout of the tilemap is a bit weird:
	16 consecutive tile codes define a vertical column.
	16 columns form a page (256 x 256).
	The tilemap is made of 4 x 4 pages (1024 x 1024)

	[ 512? Sprites ]

	Sprites are 16 x 16 x 4 in size. There's RAM for 512, but
	the game just copies 384 entries.


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Variables that driver has access to: */

data16_t *shocking_vram_0,   *shocking_vram_1;
data16_t *shocking_scroll_0, *shocking_scroll_1;
data16_t *shocking_priority;


/***************************************************************************


									Tilemaps


***************************************************************************/

static struct tilemap *tilemap_0, *tilemap_1;

#define TMAP_GFX			(0)
#define TILES_PER_PAGE_X	(0x10)
#define TILES_PER_PAGE_Y	(0x10)
#define PAGES_PER_TMAP_X	(0x4)
#define PAGES_PER_TMAP_Y	(0x4)

UINT32 shocking_tilemap_scan_pages(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	return	(row / TILES_PER_PAGE_Y) * TILES_PER_PAGE_X * TILES_PER_PAGE_Y * PAGES_PER_TMAP_X +
			(row % TILES_PER_PAGE_Y) +

			(col / TILES_PER_PAGE_X) * TILES_PER_PAGE_X * TILES_PER_PAGE_Y +
			(col % TILES_PER_PAGE_X) * TILES_PER_PAGE_Y;
}

static void get_tile_info_0(int tile_index)
{
	data16_t code = shocking_vram_0[ 2 * tile_index + 0 ];
	data16_t attr = shocking_vram_0[ 2 * tile_index + 1 ];
	SET_TILE_INFO(TMAP_GFX, code, attr & 0xf);
	tile_info.priority	=	attr & 7;
}

static void get_tile_info_1(int tile_index)
{
	data16_t code = shocking_vram_1[ 2 * tile_index + 0 ];
	data16_t attr = shocking_vram_1[ 2 * tile_index + 1 ];
	SET_TILE_INFO(TMAP_GFX, code, attr & 0xf);
	tile_info.priority	=	attr & 7;
}

WRITE16_HANDLER( shocking_vram_0_w )
{
	data16_t old_data	=	shocking_vram_0[offset];
	data16_t new_data	=	COMBINE_DATA(&shocking_vram_0[offset]);
	if (old_data != new_data)	tilemap_mark_tile_dirty(tilemap_0,offset/2);
}

WRITE16_HANDLER( shocking_vram_1_w )
{
	data16_t old_data	=	shocking_vram_1[offset];
	data16_t new_data	=	COMBINE_DATA(&shocking_vram_1[offset]);
	if (old_data != new_data)	tilemap_mark_tile_dirty(tilemap_1,offset/2);
}


/***************************************************************************


							Video Hardware Init


***************************************************************************/

static int sprites_scrolldx, sprites_scrolldy;

int shocking_vh_start(void)
{
	tilemap_0 = tilemap_create(	get_tile_info_0,shocking_tilemap_scan_pages,
								TILEMAP_TRANSPARENT,
								16,16,
								TILES_PER_PAGE_X*PAGES_PER_TMAP_X,TILES_PER_PAGE_Y*PAGES_PER_TMAP_Y);

	tilemap_1 = tilemap_create(	get_tile_info_1,shocking_tilemap_scan_pages,
								TILEMAP_TRANSPARENT,
								16,16,
								TILES_PER_PAGE_X*PAGES_PER_TMAP_X,TILES_PER_PAGE_Y*PAGES_PER_TMAP_Y);

	if (!tilemap_0 || !tilemap_1)	return 1;

	sprites_scrolldx = -0x40;
	sprites_scrolldy = -0x0f;
	tilemap_set_scrolldx(tilemap_0,-0x34,0);
	tilemap_set_scrolldx(tilemap_1,-0x38,0);

	tilemap_set_scrolldy(tilemap_0,-0x10,0);
	tilemap_set_scrolldy(tilemap_1,-0x10,0);

	tilemap_set_transparent_pen(tilemap_0,0xff);
	tilemap_set_transparent_pen(tilemap_1,0xff);
	return 0;
}


/***************************************************************************


								Sprites Drawing


		0.w								X

		2.w								Y

		4.w								Code

		6.w		fedc ba98 765- ----
				---- ---- ---4 3210		Color

- Flip X&Y ?

***************************************************************************/

static void shocking_draw_sprites(struct osd_bitmap *bitmap)
{
	int offs;

	int max_x		=	Machine->visible_area.max_x+1;
	int max_y		=	Machine->visible_area.max_y+1;

	int pri			=	*shocking_priority & 7;
	int pri_mask	=	~((1 << (pri-1)) - 1);	// above the first "pri" levels

	if (pri == 7)	pri_mask = 0;	// above all

	for ( offs = (spriteram_size-8)/2 ; offs >= 0; offs -= 8/2 )
	{
		int x		=	spriteram16[offs + 0];
		int y		=	spriteram16[offs + 1];
		int code	=	spriteram16[offs + 2];
		int attr	=	spriteram16[offs + 3];
		int flipx	=	0;	// not used?
		int flipy	=	0;

		x	+=	sprites_scrolldx;
		y	+=	sprites_scrolldy;

		if (flip_screen)	// not used?
		{
			flipx = !flipx;		x = max_x - x - 16;
			flipy = !flipy;		y = max_y - y - 16;
		}

		pdrawgfx(	bitmap,Machine->gfx[1],
					code,
					attr,
					flipx, flipy,
					x,y,
					&Machine->visible_area,TRANSPARENCY_PEN,15,
					pri_mask	);
	}
}

static void shocking_mark_sprites_colors(void)
{
	int count = 0;
	int offs,i,col,colmask[0x20];

	unsigned int *pen_usage	=	Machine->gfx[1]->pen_usage;
	int total_elements		=	Machine->gfx[1]->total_elements;
	int color_codes_start	=	Machine->drv->gfxdecodeinfo[1].color_codes_start;
	int total_color_codes	=	Machine->drv->gfxdecodeinfo[1].total_color_codes;

	int xmin = Machine->visible_area.min_x;
	int xmax = Machine->visible_area.max_x;
	int ymin = Machine->visible_area.min_y;
	int ymax = Machine->visible_area.max_y;

	memset(colmask, 0, sizeof(colmask));

	for ( offs = 0 ; offs < (spriteram_size/2); offs += 8/2 )
	{
		int x		=	spriteram16[offs + 0];
		int y		=	spriteram16[offs + 1];
		int code	=	spriteram16[offs + 2] % total_elements;
		int color	=	spriteram16[offs + 3] % total_color_codes;

		x	+=	sprites_scrolldx;
		y	+=	sprites_scrolldy;

		if (((x+15) >= xmin) && (x <= xmax) &&
			((y+15) >= ymin) && (y <= ymax))
			colmask[color] |= pen_usage[code];
	}

	for (col = 0; col < total_color_codes; col++)
	 for (i = 0; i < 15; i++)	// pen 15 is transparent
	  if (colmask[col] & (1 << i))
	  {	palette_used_colors[16 * col + i + color_codes_start] = PALETTE_COLOR_USED;
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


void shocking_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i,layers_ctrl = -1;

	tilemap_set_scrollx(tilemap_0, 0, shocking_scroll_0[ 0 ]);
	tilemap_set_scrolly(tilemap_0, 0, shocking_scroll_0[ 1 ]);

	tilemap_set_scrollx(tilemap_1, 0, shocking_scroll_1[ 0 ]);
	tilemap_set_scrolly(tilemap_1, 0, shocking_scroll_1[ 1 ]);

#ifdef MAME_DEBUG
if (keyboard_pressed(KEYCODE_Z))
{
	int msk = 0;
	if (keyboard_pressed(KEYCODE_Q))	msk |= 1;
	if (keyboard_pressed(KEYCODE_W))	msk |= 2;
	if (keyboard_pressed(KEYCODE_E))	msk |= 4;
	if (keyboard_pressed(KEYCODE_A))	msk |= 8;
	if (msk != 0) layers_ctrl &= msk;

#if 0
{	char buf[10];
	sprintf(buf,"%04X", *shocking_priority);
	usrintf_showmessage(buf);	}
#endif
}
#endif

	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();

	shocking_mark_sprites_colors();

	palette_recalc();

	/* The color of the this layer's transparent pen goes below everything */
	if (layers_ctrl & 1)
		for (i = 0; i <= 7; i++)	tilemap_draw(bitmap,tilemap_0, TILEMAP_IGNORE_TRANSPARENCY | i, 0);
	else
	{
		fillbitmap(bitmap,palette_transparent_pen,&Machine->visible_area);
		fillbitmap(priority_bitmap,0,NULL);
	}

	/* Priority 0 seems the frontmost */
	if (layers_ctrl & 1)
		for (i = 0; i <= 7; i++)	tilemap_draw(bitmap,tilemap_0, i, (i-1)&7);

	if (layers_ctrl & 2)
		for (i = 0; i <= 7; i++)	tilemap_draw(bitmap,tilemap_1, i, (i-1)&7);

	if (layers_ctrl & 8)	shocking_draw_sprites(bitmap);

	/* tilemap.c only copes with screen widths which are a multiple of 8 pixels */
	if (Machine->visible_area.max_x < (Machine->drv->screen_width-1))
	{
		struct rectangle clip;
		clip.min_x = Machine->visible_area.max_x+1;
		clip.max_x = Machine->drv->screen_width-1;
		clip.min_y = Machine->visible_area.min_y;
		clip.max_y = Machine->visible_area.max_y;
		fillbitmap(bitmap,palette_transparent_pen,&clip);
	}
}
