/***************************************************************************

						  -= Puzzle Bancho (Fuuki) =-

					driver by	Luca Elia (eliavit@unina.it)


Note:	if MAME_DEBUG is defined, pressing Z with:

		Q		shows the background
		W		shows the foreground
		A		shows the sprites

		Keys can be used together!


	[ 2 Scrolling Layers ]

	They're 1024 x 512 in size. Tiles are 16 x 16 x 4 in one, 16 x 16 x 8
	in the other. The layers can be swapped.

	[ 1024? Zooming Sprites ]

	Sprites are made of 16 x 16 x 4 tiles. Size can vary from 1 to 16
	tiles both horizontally and vertically.
	There is zooming (from full size to half size) and 4 levels of
	priority (wrt layers)

	* Note: the game does hardware assisted raster effects *

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Variables that driver has access to: */

data16_t *pbancho_vram_0, *pbancho_vram_1;
data16_t *pbancho_vregs,  *pbancho_unknown, *pbancho_priority;


/***************************************************************************


									Tilemaps

	Offset: 	Bits:					Value:

		0.w								Code

		2.w		fedc ba98 ---- ----
				---- ---- 7--- ----		Flip Y
				---- ---- -6-- ----		Flip X
				---- ---- --54 3210		Color


***************************************************************************/

static struct tilemap *tilemap_0, *tilemap_1;

#define NX	(0x40)
#define NY	(0x20)

static void get_tile_info_0(int tile_index)
{
	data16_t code = pbancho_vram_0[ 2 * tile_index + 0 ];
	data16_t attr = pbancho_vram_0[ 2 * tile_index + 1 ];
	SET_TILE_INFO(1, code, attr & 0x3f);
	tile_info.flags = TILE_FLIPYX( (attr >> 6) & 3 );
}

static void get_tile_info_1(int tile_index)
{
	data16_t code = pbancho_vram_1[ 2 * tile_index + 0 ];
	data16_t attr = pbancho_vram_1[ 2 * tile_index + 1 ];
	SET_TILE_INFO(2, code, attr & 0x3f);
	tile_info.flags = TILE_FLIPYX( (attr >> 6) & 3 );
}

WRITE16_HANDLER( pbancho_vram_0_w )
{
	data16_t old_data	=	pbancho_vram_0[offset];
	data16_t new_data	=	COMBINE_DATA(&pbancho_vram_0[offset]);
	if (old_data != new_data)	tilemap_mark_tile_dirty(tilemap_0,offset/2);
}

WRITE16_HANDLER( pbancho_vram_1_w )
{
	data16_t old_data	=	pbancho_vram_1[offset];
	data16_t new_data	=	COMBINE_DATA(&pbancho_vram_1[offset]);
	if (old_data != new_data)	tilemap_mark_tile_dirty(tilemap_1,offset/2);
}


/***************************************************************************


							Video Hardware Init


***************************************************************************/

void pbancho_vh_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int color, pen;

	/* Layer 0 has 8 bits per pixel, but the color code has
	   a 16 color granularity */
	for( color = 0; color < 64; color++ )
		for( pen = 0; pen < 256; pen++ )
			colortable[color * 256 + pen + 0x400] = ((color * 16 + pen)%(64*16)) + 0x400;

	/* The game does not initialise the palette at startup. It should
	   be totally black */
	memset(palette, 0, 3 * Machine->drv->total_colors);
}

int pbancho_vh_start(void)
{
	tilemap_0 = tilemap_create(	get_tile_info_0, tilemap_scan_rows,
								TILEMAP_TRANSPARENT,	// layers can be swapped
								16,16,
								NX,NY);

	tilemap_1 = tilemap_create(	get_tile_info_1,tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16,16,
								NX,NY);

	if (!tilemap_0 || !tilemap_1)	return 1;

	tilemap_set_scroll_rows( tilemap_0, 1);
	tilemap_set_scroll_rows( tilemap_1, 1);

	tilemap_set_transparent_pen(tilemap_0,0xff);
	tilemap_set_transparent_pen(tilemap_1,0x0f);
	return 0;
}


/***************************************************************************


								Sprites Drawing

	Offset: 	Bits:					Value:

		0.w		fedc ---- ---- ----		Number Of Tiles Along X - 1
				---- b--- ---- ----		Flip X
				---- -a-- ---- ----		1 = Don't Draw This Sprite
				---- --98 7654 3210		X (Signed)

		2.w		fedc ---- ---- ----		Number Of Tiles Along Y - 1
				---- b--- ---- ----		Flip Y
				---- -a-- ---- ----
				---- --98 7654 3210		Y (Signed)

		4.w		fedc ---- ---- ----		Zoom X ($0 = Full Size, $F = Half Size)
				---- ba98 ---- ----		Zoom Y ""
				---- ---- 7--- ----		0 = Priority Over Foreground
				---- ---- -6-- ----		0 = Priority Over Background
				---- ---- --54 3210		Color

		6.w								Code


***************************************************************************/

static void pbancho_draw_sprites(struct osd_bitmap *bitmap)
{
	int offs;

	int max_x		=	Machine->visible_area.max_x+1;
	int max_y		=	Machine->visible_area.max_y+1;

	/* Draw them backwards, for pdrawgfx */
	for ( offs = (spriteram_size-8)/2; offs >=0; offs -= 8/2 )
	{
		int x, y, xstart, ystart, xend, yend, xinc, yinc;
		int xnum, ynum, xzoom, yzoom, flipx, flipy;
		int pri_mask;

		int sx			=		spriteram16[offs + 0];
		int sy			=		spriteram16[offs + 1];
		int attr		=		spriteram16[offs + 2];
		int code		=		spriteram16[offs + 3];

		if (sx & 0x400)		continue;

		flipx		=		sx & 0x0800;
		flipy		=		sy & 0x0800;

		xnum		=		((sx >> 12) & 0xf) + 1;
		ynum		=		((sy >> 12) & 0xf) + 1;

		xzoom		=		16*8 - (8 * ((attr >> 12) & 0xf))/2;
		yzoom		=		16*8 - (8 * ((attr >>  8) & 0xf))/2;

		switch( (attr >> 6) & 3 )
		{
			case 3:		pri_mask = (1<<1)|(1<<2)|(1<<3);	break;
			case 2:		pri_mask = (1<<2)|(1<<3);			break;
			case 1:		pri_mask = (1<<1)|(1<<3);			break;
			case 0:
			default:	pri_mask = 0;
		}

		sx = (sx & 0x1ff) - (sx & 0x200);
		sy = (sy & 0x1ff) - (sy & 0x200);

		if (flip_screen)
		{	flipx = !flipx;		sx = max_x - sx - xnum * 16;
			flipy = !flipy;		sy = max_y - sy - ynum * 16;		}

		if (flipx)	{ xstart = xnum-1;  xend = -1;    xinc = -1; }
		else		{ xstart = 0;       xend = xnum;  xinc = +1; }

		if (flipy)	{ ystart = ynum-1;  yend = -1;    yinc = -1; }
		else		{ ystart = 0;       yend = ynum;  yinc = +1; }

		for (y = ystart; y != yend; y += yinc)
		{
			for (x = xstart; x != xend; x += xinc)
			{
				if (xzoom == (16*8) && yzoom == (16*8))
					pdrawgfx(		bitmap,Machine->gfx[0],
									code++,
									attr & 0x3f,
									flipx, flipy,
									sx + x * 16, sy + y * 16,
									&Machine->visible_area,TRANSPARENCY_PEN,15,
									pri_mask	);
				else
					pdrawgfxzoom(	bitmap,Machine->gfx[0],
									code++,
									attr & 0x3f,
									flipx, flipy,
									sx + (x * xzoom) / 8, sy + (y * yzoom) / 8,
									&Machine->visible_area,TRANSPARENCY_PEN,15,
									(0x10000/0x10/8) * (xzoom + 8),(0x10000/0x10/8) * (yzoom + 8),	// nearest greater integer value to avoid holes
									pri_mask	);
			}
		}

#ifdef MAME_DEBUG
#if 1
if (keyboard_pressed(KEYCODE_X))
{	/* Display some info on each sprite */
	struct DisplayText dt[2];	char buf[10];
	sprintf(buf, "%Xx%X %X",xnum,ynum,(attr>>6)&3);
	dt[0].text = buf;	dt[0].color = UI_COLOR_NORMAL;
	dt[0].x = sx;		dt[0].y = sy;
	dt[1].text = 0;	/* terminate array */
	displaytext(Machine->scrbitmap,dt);		}
#endif
#endif

	}

}

static void pbancho_mark_sprites_colors(void)
{
	memset(palette_used_colors,PALETTE_COLOR_USED,Machine->drv->total_colors);
}



/***************************************************************************


								Screen Drawing

	Video Registers (pbancho_vregs):

		00.w		Layer 1 Scroll Y
		02.w		Layer 1 Scroll X
		04.w		Layer 0 Scroll Y
		06.w		Layer 0 Scroll X
		08.w		? Y Offset ?
		0a.w		? X Offset ?
		0c.w		Layers Y Offset
		0e.w		Layers X Offset

		10-1a.w		? 0
		1c.w		Trigger a level 5 irq on this raster line
		1e.w		? $3390/$3393 (Flip Screen Off/On)

	Priority Register (pbancho_priority):

		fedc ba98 7654 3---
		---- ---- ---- -2--		?
		---- ---- ---- --1-
		---- ---- ---- ---0		Swap Layers


	Unknown Registers (pbancho_unknown):

		00.w		? $0200/$0201	(Flip Screen Off/On)
		02.w		? $f300/$0330

***************************************************************************/


void pbancho_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	data16_t layer0_scrollx, layer0_scrolly;
	data16_t layer1_scrollx, layer1_scrolly;
	data16_t scrollx_offs,   scrolly_offs;

	struct tilemap *background, *foreground;

	int layers_ctrl = -1;

	flip_screen_set(pbancho_vregs[0x1e/2] & 1);

	/* Layers scrolling */

	scrolly_offs = pbancho_vregs[0xc/2] - (flip_screen ? 0x103 : 0x1f3);
	scrollx_offs = pbancho_vregs[0xe/2] - (flip_screen ? 0x2a7 : 0x3f6);

	layer1_scrolly = pbancho_vregs[0x0/2] + scrolly_offs;
	layer1_scrollx = pbancho_vregs[0x2/2] + scrollx_offs;
	layer0_scrolly = pbancho_vregs[0x4/2] + scrolly_offs;
	layer0_scrollx = pbancho_vregs[0x6/2] + scrollx_offs;

	tilemap_set_scrollx(tilemap_0, 0, layer0_scrollx);
	tilemap_set_scrolly(tilemap_0, 0, layer0_scrolly);
	tilemap_set_scrollx(tilemap_1, 0, layer1_scrollx);
	tilemap_set_scrolly(tilemap_1, 0, layer1_scrolly);

#ifdef MAME_DEBUG
if (keyboard_pressed(KEYCODE_Z))
{
	int msk = 0;
	if (keyboard_pressed(KEYCODE_Q))	msk |= 1;
	if (keyboard_pressed(KEYCODE_W))	msk |= 2;
//	if (keyboard_pressed(KEYCODE_E))	msk |= 4;
	if (keyboard_pressed(KEYCODE_A))	msk |= 8;
	if (msk != 0) layers_ctrl &= msk;

#if 1
{	char buf[10];
	sprintf(buf,"%04X %04X %04X",
		pbancho_unknown[0],pbancho_unknown[1],*pbancho_priority);
	usrintf_showmessage(buf);	}
#endif
}
#endif

	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();

	pbancho_mark_sprites_colors();

	palette_recalc();

	background = (( *pbancho_priority & 1) ? tilemap_1 : tilemap_0);
	foreground = ((~*pbancho_priority & 1) ? tilemap_1 : tilemap_0);

	/* The backmost tilemap decides the background color(s) but sprites can
	   go below the opaque pixels of that tilemap. We thus need to mark the
	   transparent pixels of this layer with a different priority value */
	if (layers_ctrl & 1)	tilemap_draw(bitmap,background,TILEMAP_IGNORE_TRANSPARENCY,0);
	else
	{						fillbitmap(bitmap,palette_transparent_pen,&Machine->visible_area);
							fillbitmap(priority_bitmap,0,NULL);			}

	if (layers_ctrl & 1)	tilemap_draw(bitmap,background,0,1);

	if (layers_ctrl & 2)	tilemap_draw(bitmap,foreground,0,2);

	if (layers_ctrl & 8)	pbancho_draw_sprites(bitmap);
}
