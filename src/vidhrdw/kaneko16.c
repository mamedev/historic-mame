/***************************************************************************

						-= Kaneko 16 Bit Games =-

				driver by	Luca Elia (eliavit@unina.it)


Note:	if MAME_DEBUG is defined, pressing Z with:

		Q			shows the background
		W/E/R/T		shows the foreground (priority 0/1/2/3)
		A/S/D/F		shows the sprites    (priority 0/1/2/3)

		Keys can be used togheter!


							[ 4 Scrolling Layers ]

	[Background 1]
	[Foreground 1]
	[Background 2?]	Unused
	[Foreground 2?]	Unused

		Layer Size:				512 x 512
		Tiles:					16x16x4
		Tile Format:

				0000.w			fedc ba-- ---- ----		unused?
								---- --98 ---- ----		Priority
								---- ---- 7654 32--		Color
								---- ---- ---- --1-		Flip X
								---- ---- ---- ---0		Flip Y

				0002.w									Code




							[ 1024 Sprites ]

	Sprites are 16x16x4 in the older games 16x16x8 in gtmr&gtmr2

Offset:			Format:						Value:

0000.w			Attribute (type 0, older games: shogwarr, berlwall?)

					fed- ---- ---- ----		Multisprite
					---c ba-- ---- ----
					---- --98 ---- ----		Priority?
					---- ---- 7654 32--		Color
					---- ---- ---- --1-		X Flip
					---- ---- ---- ---0		Y Flip

				Attribute (type 1: gtmr, gtmr2)

					fed- ---- ---- ----		Multisprite
					---c ba-- ---- ----		unused?
					---- --9- ---- ----		X Flip
					---- ---8 ---- ----		Y Flip
					---- ---- 76-- ----		Priority
					---- ---- --54 3210		Color

0002.w										Code
0004.w										X Position << 6
0006.w										Y Position << 6


**************************************************************************/
#include "vidhrdw/generic.h"

/* Variables only used here: */

static struct tilemap *bg_tilemap, *fg_tilemap;
static int flipsprites;

#ifdef MAME_DEBUG
static int debugsprites;	// for debug
#endif

/* Variables that driver has access to: */

unsigned char *kaneko16_bgram, *kaneko16_fgram;
unsigned char *kaneko16_layers1_regs, *kaneko16_layers2_regs, *kaneko16_screen_regs;
int kaneko16_spritetype;

/* Variables defined in drivers: */


/***************************************************************************

								Palette RAM

***************************************************************************/

void kaneko16_paletteram_w(int offset, int data)
{
	/*	byte 0    	byte 1		*/
	/*	xGGG GGRR   RRRB BBBB	*/
	/*	x432 1043 	2104 3210	*/

	int newword, r,g,b;

	COMBINE_WORD_MEM(&paletteram[offset], data);

	newword = READ_WORD (&paletteram[offset]);
	r = (newword >>  5) & 0x1f;
	g = (newword >> 10) & 0x1f;
	b = (newword >>  0) & 0x1f;

	palette_change_color( offset/2,	 (r * 0xFF) / 0x1F,
									 (g * 0xFF) / 0x1F,
									 (b * 0xFF) / 0x1F	 );
}

void gtmr_paletteram_w(int offset, int data)
{
	if (offset < 0x10000)	kaneko16_paletteram_w(offset, data);
	else					COMBINE_WORD_MEM(&paletteram[offset], data);
}




/***************************************************************************

							Video Registers

***************************************************************************/

/*	[gtmr]

Initial self test:
600000:4BC0 94C0 4C40 94C0-0404 0002 0000 0000
680000:4BC0 94C0 4C40 94C0-1C1C 0002 0000 0000

700000:0040 0000 0001 0180-0000 0000 0000 0000
700010:0040 0000 0040 0000-0040 0000 2840 1E00

Race start:
600000:DC00 7D00 DC80 7D00-0404 0002 0000 0000
680000:DC00 7D00 DC80 7D00-1C1C 0002 0000 0000

700000:0040 0000 0001 0180-0000 0000 0000 0000
700010:0040 0000 0040 0000-0040 0000 2840 1E00

*/

int kaneko16_screen_regs_r(int offset)
{
	return READ_WORD(&kaneko16_screen_regs[offset]);
}

void kaneko16_screen_regs_w(int offset,int data)
{
	int new_data;

	COMBINE_WORD_MEM(&kaneko16_screen_regs[offset],data);
	new_data  = READ_WORD(&kaneko16_screen_regs[offset]);

	switch (offset)
	{
		case 0x00:	flipsprites = new_data & 3;	break;
	}

	if (errorlog) fprintf(errorlog, "CPU #0 PC %06X : Warning, screen reg %04X <- %04X\n",cpu_get_pc(),offset,data);
}



/*	[gtmr]

	car select screen scroll values:
	Flipscreen off:
		$6x0000: $72c0 ; $fbc0 ; 7340 ; 0
		$72c0/$40 = $1cb = $200-$35	/	$7340/$40 = $1cd = $1cb+2

		$fbc0/$40 = -$11

	Flipscreen on:
		$6x0000: $5d00 ; $3780 ; $5c80 ; $3bc0
		$5d00/$40 = $174 = $200-$8c	/	$5c80/$40 = $172 = $174-2

		$3780/$40 = $de	/	$3bc0/$40 = $ef

*/

void kaneko16_layers1_regs_w(int offset,int data)
{
	COMBINE_WORD_MEM(&kaneko16_layers1_regs[offset],data);
}

void kaneko16_layers2_regs_w(int offset,int data)
{
	COMBINE_WORD_MEM(&kaneko16_layers2_regs[offset],data);
}



/***************************************************************************

						Callbacks for the TileMap code

							  [ Tiles Format ]

Offset:

0000.w			fedc ba-- ---- ----		unused?
				---- --98 ---- ----		Priority
				---- ---- 7654 32--		Color
				---- ---- ---- --1-		Flip X
				---- ---- ---- ---0		Flip Y

0002.w									Code

***************************************************************************/


/* Background */

#define BG_GFX (1)
#define BG_NX  (0x20)
#define BG_NY  (0x20)

static void get_bg_tile_info( int col, int row )
{
	int tile_index = col + row * BG_NX;
	int code_hi = READ_WORD(&kaneko16_bgram[tile_index*4 + 0]);
	int code_lo = READ_WORD(&kaneko16_bgram[tile_index*4 + 2]);
	SET_TILE_INFO(BG_GFX, code_lo,(code_hi >> 2) & 0x3f);
	tile_info.flags 	=	TILE_FLIPXY( code_hi & 3 );
}

void kaneko16_bgram_w(int offset,int data)
{
int old_data, new_data;

	old_data  = READ_WORD(&kaneko16_bgram[offset]);
	COMBINE_WORD_MEM(&kaneko16_bgram[offset],data);
	new_data  = READ_WORD(&kaneko16_bgram[offset]);

	if (old_data != new_data)
		tilemap_mark_tile_dirty(bg_tilemap,(offset/4) % BG_NX,(offset/4) / BG_NX);
}





/* Foreground */

#define FG_GFX (1)
#define FG_NX  (0x20)
#define FG_NY  (0x20)

static void get_fg_tile_info( int col, int row )
{
	int tile_index = col + row * FG_NX;
	int code_hi = READ_WORD(&kaneko16_fgram[tile_index*4 + 0]);
	int code_lo = READ_WORD(&kaneko16_fgram[tile_index*4 + 2]);
	SET_TILE_INFO(FG_GFX, code_lo,(code_hi >> 2) & 0x3f);
	tile_info.flags 	=	TILE_FLIPXY( code_hi & 3 );
	tile_info.priority	=	(code_hi >> 8) & 3;
}

void kaneko16_fgram_w(int offset,int data)
{
int old_data, new_data;

	old_data  = READ_WORD(&kaneko16_fgram[offset]);
	COMBINE_WORD_MEM(&kaneko16_fgram[offset],data);
	new_data  = READ_WORD(&kaneko16_fgram[offset]);

	if (old_data != new_data)
		tilemap_mark_tile_dirty(fg_tilemap,(offset/4) % FG_NX,(offset/4) / FG_NX);
}



void kaneko16_layers1_w(int offset, int data)
{
	if (offset < 0x1000)	kaneko16_fgram_w(offset,data);
	else
	{
		if (offset < 0x2000)	kaneko16_bgram_w((offset-0x1000),data);
		else
		{
			COMBINE_WORD_MEM(&kaneko16_fgram[offset],data);
		}
	}
}








int kaneko16_vh_start(void)
{
	bg_tilemap = tilemap_create(get_bg_tile_info,
								TILEMAP_OPAQUE,
								16,16,
								BG_NX,BG_NY );

	fg_tilemap = tilemap_create(get_fg_tile_info,
								TILEMAP_TRANSPARENT,
								16,16,
								FG_NX,FG_NY );

	if (bg_tilemap && fg_tilemap)
	{
/*
gtmr background:
		flipscreen off: write (x)-$33
		[x=fetch point (e.g. scroll *left* with incresing x)]

		flipscreen on:  write (x+320)+$33
		[x=fetch point (e.g. scroll *right* with incresing x)]

		W = 320+$33+$33 = $1a6 = 422

berlwall background:
6940 off	1a5
5680 on		15a
*/
		int xdim = Machine->drv->screen_width;
		int ydim = Machine->drv->screen_height;
		int dx   = (422 - xdim) / 2;

		tilemap_set_scrolldx( bg_tilemap, -dx,     xdim + dx       );
		tilemap_set_scrolldx( fg_tilemap, -(dx+2), xdim + (dx + 2) );

		tilemap_set_scrolldy( bg_tilemap, 0x00, ydim-0x00 );
		tilemap_set_scrolldy( fg_tilemap, 0x00, ydim-0x00 );

		fg_tilemap->transparent_pen = 0;
		return 0;
	}
	else
		return 1;
}








/***************************************************************************

								Sprites Drawing

Offset:			Format:						Value:

0000.w			Attribute (type 0, older games: shogwarr, berlwall?)

					fed- ---- ---- ----		Multisprite
					---c ba-- ---- ----
					---- --98 ---- ----		Priority?
					---- ---- 7654 32--		Color
					---- ---- ---- --1-		X Flip
					---- ---- ---- ---0		Y Flip

				Attribute (type 1: gtmr, gtmr2)

					fed- ---- ---- ----		Multisprite
					---c ba-- ---- ----		unused?
					---- --9- ---- ----		X Flip
					---- ---8 ---- ----		Y Flip
					---- ---- 76-- ----		Priority
					---- ---- --54 3210		Color

0002.w										Code
0004.w										X Position << 6
0006.w										Y Position << 6

***************************************************************************/


/* Map the attribute word to that of the type 1 sprite hardware */

#define MAP_TO_TYPE1(attr) \
	if (kaneko16_spritetype == 0)	/* shogwarr, berlwall? */ \
	{ \
		attr =	((attr & 0xfc00)     ) | \
				((attr & 0x03fc) >> 2) | \
				((attr & 0x0003) << 8) ; \
	}


/* Mark the pens of visible sprites */

void kaneko16_mark_sprites_colors(void)
{
	int offs;

	int xmin = Machine->drv->visible_area.min_x - (16 - 1);
	int xmax = Machine->drv->visible_area.max_x;
	int ymin = Machine->drv->visible_area.min_y - (16 - 1);
	int ymax = Machine->drv->visible_area.max_y;

	int nmax				=	Machine->gfx[0]->total_elements;
	int color_granularity	=	Machine->gfx[0]->color_granularity;
	int color_codes_start	=	Machine->drv->gfxdecodeinfo[0].color_codes_start;
	int total_color_codes	=	Machine->drv->gfxdecodeinfo[0].total_color_codes;

	int sx = 0;
	int sy = 0;
	int scode = 0;
	int scolor = 0;

	for (offs = 0 ; offs < spriteram_size ; offs += 8)
	{
		int	attr	=	READ_WORD(&spriteram[offs + 0]);
		int	code	=	READ_WORD(&spriteram[offs + 2]) % nmax;
		int	x		=	READ_WORD(&spriteram[offs + 4]);
		int	y		=	READ_WORD(&spriteram[offs + 6]);

		/* Map the attribute word to that of the type 1 sprite hardware */
		MAP_TO_TYPE1(attr)

		if (x & 0x8000)	x -= 0x10000;
		if (y & 0x8000)	y -= 0x10000;

		x /= 0x40;		y /= 0x40;

		if ((attr|0xe000) == attr)
		{ sx += x;	sy += y;	scode++; }
		else
		{ sx  = x;	sy  = y; 	scode = code; 	scolor = attr % total_color_codes;}

		/* Visibility check. No need to account for sprites flipping */
		if ((sx < xmin) || (sx > xmax))	continue;
		if ((sy < ymin) || (sy > ymax))	continue;

		memset(&palette_used_colors[color_granularity * scolor + color_codes_start + 1],PALETTE_COLOR_USED,color_granularity - 1);
	}

}



/* Draw the sprites */

void kaneko16_draw_sprites(struct osd_bitmap *bitmap, int priority)
{
	int offs;

	int max_x = Machine->drv->visible_area.max_x - 16;
	int max_y = Machine->drv->visible_area.max_y - 16;

	int sx = 0;
	int sy = 0;
	int scode = 0;
	int sattr = 0;
	int sflipx = 0;
	int sflipy = 0;

	priority = ( priority & 3 ) << 6;

	for ( offs = 0 ; offs < spriteram_size ; offs += 8 )
	{
		int	attr	=	READ_WORD(&spriteram[offs + 0]);
		int	code	=	READ_WORD(&spriteram[offs + 2]);
		int	x		=	READ_WORD(&spriteram[offs + 4]);
		int	y		=	READ_WORD(&spriteram[offs + 6]);

		/* Map the attribute word to that of the type 1 sprite hardware */
		MAP_TO_TYPE1(attr)

		if (x & 0x8000)	x -= 0x10000;
		if (y & 0x8000)	y -= 0x10000;

		x /= 0x40;		y /= 0x40;

		if ((attr|0xe000) == attr)
		{
			sx += x;	sy += y;
			scode++;
		}
		else
		{
			sx  = x;				sy  = y;
			scode  = code;			sattr  = attr;
			sflipx = attr & 0x200;	sflipy = attr & 0x100;
		}

		if ((sattr & 0xc0) != priority)	continue;

#ifdef MAME_DEBUG
	if ( debugsprites && ( ((sattr >> 6) & 3) != debugsprites-1 ) )	continue;
#endif

		if (flipsprites & 2) { sx = max_x - sx;		sflipx = !sflipx; }
		if (flipsprites & 1) { sy = max_y - sy;		sflipy = !sflipy; }

		drawgfx(bitmap,Machine->gfx[0],
				scode,
				sattr,
				sflipx, sflipy,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

		/* let's get back to normal to support multi sprites */
		if (flipsprites & 2) { sx = max_x - sx;		sflipx = !sflipx; }
		if (flipsprites & 1) { sy = max_y - sy;		sflipy = !sflipy; }
	}

}






/***************************************************************************

								Screen Drawing

***************************************************************************/

void kaneko16_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int layers_ctrl = -1;
	int layers_flip = READ_WORD(&kaneko16_layers1_regs[0x08]);

	tilemap_set_flip(fg_tilemap,((layers_flip & 0x0001) ? TILEMAP_FLIPY : 0) |
								((layers_flip & 0x0002) ? TILEMAP_FLIPX : 0) );

	tilemap_set_flip(bg_tilemap,((layers_flip & 0x0100) ? TILEMAP_FLIPY : 0) |
								((layers_flip & 0x0200) ? TILEMAP_FLIPX : 0) );

	tilemap_set_scrollx(fg_tilemap, 0, READ_WORD(&kaneko16_layers1_regs[0x00]) >> 6 );
	tilemap_set_scrolly(fg_tilemap, 0, READ_WORD(&kaneko16_layers1_regs[0x02]) >> 6 );

	tilemap_set_scrollx(bg_tilemap, 0, READ_WORD(&kaneko16_layers1_regs[0x04]) >> 6 );
	tilemap_set_scrolly(bg_tilemap, 0, READ_WORD(&kaneko16_layers1_regs[0x06]) >> 6 );


#ifdef MAME_DEBUG
debugsprites = 0;
if (keyboard_pressed(KEYCODE_Z))
{
int msk = 0;

	if (keyboard_pressed(KEYCODE_Q))	{ msk |= 0x01;}
	if (keyboard_pressed(KEYCODE_W))	{ msk |= 0x02;}
	if (keyboard_pressed(KEYCODE_E))	{ msk |= 0x04;}
	if (keyboard_pressed(KEYCODE_R))	{ msk |= 0x10;}
	if (keyboard_pressed(KEYCODE_T))	{ msk |= 0x20;}
	if (keyboard_pressed(KEYCODE_A))	{ msk |= 0x08; debugsprites = 1;}
	if (keyboard_pressed(KEYCODE_S))	{ msk |= 0x08; debugsprites = 2;}
	if (keyboard_pressed(KEYCODE_D))	{ msk |= 0x08; debugsprites = 3;}
	if (keyboard_pressed(KEYCODE_F))	{ msk |= 0x08; debugsprites = 4;}
	if (msk != 0) layers_ctrl &= msk;
}
#endif


	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();

	kaneko16_mark_sprites_colors();

	if (palette_recalc())	tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);

	if (layers_ctrl & 0x01)	tilemap_draw(bitmap, bg_tilemap, 0);
	else					osd_clearbitmap(Machine->scrbitmap);

	if (layers_ctrl & 0x02)	tilemap_draw(bitmap, fg_tilemap, 0);
	if (layers_ctrl & 0x08)	kaneko16_draw_sprites(bitmap,0);

	if (layers_ctrl & 0x04)	tilemap_draw(bitmap, fg_tilemap, 1);
	if (layers_ctrl & 0x08)	kaneko16_draw_sprites(bitmap,1);

	if (layers_ctrl & 0x10)	tilemap_draw(bitmap, fg_tilemap, 2);
	if (layers_ctrl & 0x08)	kaneko16_draw_sprites(bitmap,2);

	if (layers_ctrl & 0x20)	tilemap_draw(bitmap, fg_tilemap, 3);
	if (layers_ctrl & 0x08)	kaneko16_draw_sprites(bitmap,3);
}
