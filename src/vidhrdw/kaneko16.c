/***************************************************************************

						-= Kaneko 16 Bit Games =-

				driver by	Luca Elia (eliavit@unina.it)


Note:	if MAME_DEBUG is defined, pressing:

		Q  with  X/C/V/B/ Z   shows layer 0 (tiles with priority 0/1/2/3/ All)
		W  with  X/C/V/B/ Z   shows layer 1 (tiles with priority 0/1/2/3/ All)
		A  with  X/C/V/B/ Z   shows sprites (tiles with priority 0/1/2/3/ All)

		Keys can be used togheter!

							[ 1 High Color Layer ]

	[Background 0]	In ROM	(Optional)

							[ 4 Scrolling Layers ]

	[Background 1]
	[Foreground 1]
	[Background 2?]	Unused
	[Foreground 2?]	Unused

		Layer Size:				512 x 512
		Tiles:					16x16x4
		Tile Format:

				0000.w			fedc ba-- ---- ----
								---- --98 ---- ----		Priority
								---- ---- 7654 32--		Color
								---- ---- ---- --1-		Flip X
								---- ---- ---- ---0		Flip Y

				0002.w									Code



							[ 1024 Sprites ]

	Sprites are 16x16x4 in the older games 16x16x8 in gtmr&gtmr2
	See below for the memory layouts.


**************************************************************************/

#include "vidhrdw/generic.h"

/* Variables only used here: */

static struct tilemap *bg_tilemap, *fg_tilemap;
static struct osd_bitmap *kaneko16_bg15_bitmap;
static int flipsprites;

struct tempsprite
{
	int code,color;
	int flipx,flipy;
	int x,y;
	int primask;
};
struct tempsprite *spritelist;


/* Variables that driver has access to: */

data16_t *kaneko16_bgram, *kaneko16_fgram;
data16_t *kaneko16_layers1_regs, *kaneko16_layers2_regs, *kaneko16_screen_regs;
data16_t *kaneko16_bg15_select, *kaneko16_bg15_reg;

int kaneko16_spritetype;

/* Variables defined in drivers: */


/***************************************************************************

							Layers Registers


Offset:			Format:						Value:

0000.w										FG Scroll X
0002.w										FG Scroll Y

0004.w										BG Scroll X
0006.w										BG Scroll Y

0008.w			Layers Control

					fed- ---- ---- ----
					---c ---- ---- ----		BG Disable
					---- b--- ---- ----		? Always 1 in berlwall & bakubrkr ?
					---- -a-- ---- ----		? Always 1 in gtmr     & bakubrkr ?
					---- --9- ---- ----		BG Flip X
					---- ---8 ---- ----		BG Flip Y

					---- ---- 765- ----
					---- ---- ---4 ----		FG Disable
					---- ---- ---- 3---		? Always 1 in berlwall & bakubrkr ?
					---- ---- ---- -2--		? Always 1 in gtmr     & bakubrkr ?
					---- ---- ---- --1-		FG Flip X
					---- ---- ---- ---0		FG Flip Y

000a.w										? always 0x0002 ?

There are more!

***************************************************************************/

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

WRITE16_HANDLER( kaneko16_layers1_regs_w )
{
	COMBINE_DATA(&kaneko16_layers1_regs[offset]);
}

WRITE16_HANDLER( kaneko16_layers2_regs_w )
{
	COMBINE_DATA(&kaneko16_layers2_regs[offset]);
}




/* Select the high color background image (out of 32 in the ROMs) */
READ16_HANDLER( kaneko16_bg15_select_r )
{
	return kaneko16_bg15_select[0];
}
WRITE16_HANDLER( kaneko16_bg15_select_w )
{
	COMBINE_DATA(&kaneko16_bg15_select[0]);
}

/* ? */
READ16_HANDLER( kaneko16_bg15_reg_r )
{
	return kaneko16_bg15_reg[0];
}
WRITE16_HANDLER( kaneko16_bg15_reg_w )
{
	COMBINE_DATA(&kaneko16_bg15_reg[0]);
}


/***************************************************************************

							Screen Registers

***************************************************************************/

/*	[gtmr]

Initial self test:
600000:4BC0 94C0 4C40 94C0-0404 0002 0000 0000		(Layers 1 regs)
680000:4BC0 94C0 4C40 94C0-1C1C 0002 0000 0000		(Layers 2 regs)

700000:0040 0000 0001 0180-0000 0000 0000 0000		(Screen regs)
700010:0040 0000 0040 0000-0040 0000 2840 1E00

Race start:
600000:DC00 7D00 DC80 7D00-0404 0002 0000 0000		(Layers 1 regs)
680000:DC00 7D00 DC80 7D00-1C1C 0002 0000 0000		(Layers 2 regs)

700000:0040 0000 0001 0180-0000 0000 0000 0000		(Screen regs)
700010:0040 0000 0040 0000-0040 0000 2840 1E00

*/

READ16_HANDLER( kaneko16_screen_regs_r )
{
	return kaneko16_screen_regs[offset];
}

WRITE16_HANDLER( kaneko16_screen_regs_w )
{
	data16_t new_data;

	COMBINE_DATA(&kaneko16_screen_regs[offset]);
	new_data  = kaneko16_screen_regs[offset];

	switch (offset)
	{
		case 0:
			if (ACCESSING_LSB)
				flipsprites = new_data & 3;
			break;
	}

	logerror("CPU #0 PC %06X : Warning, screen reg %04X <- %04X\n",cpu_get_pc(),offset*2,data);
}







/***************************************************************************

						Callbacks for the TileMap code

							  [ Tiles Format ]

Offset:

0000.w			fedc ba-- ---- ----		unused?
				---- --9- ---- ----		High Priority (vs Sprites)
				---- ---8 ---- ----		High Priority (vs Tiles)
				---- ---- 7654 32--		Color
				---- ---- ---- --1-		Flip X
				---- ---- ---- ---0		Flip Y

0002.w									Code

***************************************************************************/


/* Background */

#define BG_GFX (0)
#define BG_NX  (0x20)
#define BG_NY  (0x20)

static void get_bg_tile_info(int tile_index)
{
	data16_t code_hi = kaneko16_bgram[ 2 * tile_index + 0];
	data16_t code_lo = kaneko16_bgram[ 2 * tile_index + 1];
	SET_TILE_INFO(BG_GFX, code_lo,(code_hi >> 2) & 0x3f);
	tile_info.flags 	=	TILE_FLIPXY( code_hi & 3 );
	tile_info.priority	=	(code_hi >> 8) & 3;
}

WRITE16_HANDLER( kaneko16_bgram_w )
{
	data16_t old_data, new_data;

	old_data  = kaneko16_bgram[offset];
	COMBINE_DATA(&kaneko16_bgram[offset]);
	new_data  = kaneko16_bgram[offset];

	if (old_data != new_data)
		tilemap_mark_tile_dirty(bg_tilemap,offset/2);
}





/* Foreground */

#define FG_GFX (0)
#define FG_NX  (0x20)
#define FG_NY  (0x20)

static void get_fg_tile_info(int tile_index)
{
	data16_t code_hi = kaneko16_fgram[ 2 * tile_index + 0];
	data16_t code_lo = kaneko16_fgram[ 2 * tile_index + 1];
	SET_TILE_INFO(FG_GFX, code_lo,(code_hi >> 2) & 0x3f);
	tile_info.flags 	=	TILE_FLIPXY( code_hi & 3 );
	tile_info.priority	=	(code_hi >> 8) & 3;
}

WRITE16_HANDLER( kaneko16_fgram_w )
{
	data16_t old_data, new_data;

	old_data  = kaneko16_fgram[offset];
	COMBINE_DATA(&kaneko16_fgram[offset]);
	new_data  = kaneko16_fgram[offset];

	if (old_data != new_data)
		tilemap_mark_tile_dirty(fg_tilemap,offset/2);
}



WRITE16_HANDLER( kaneko16_layers1_w )
{
	if (offset < 0x1000/2)	kaneko16_fgram_w(offset,data,mem_mask);
	else
	{
		if (offset < 0x2000/2)	kaneko16_bgram_w((offset-0x1000/2),data,mem_mask);
		else
		{
			COMBINE_DATA(&kaneko16_fgram[offset]);
		}
	}
}








int kaneko16_vh_start(void)
{
	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_rows,
								TILEMAP_TRANSPARENT, /* to handle the optional hi-color bg */
								16,16,BG_NX,BG_NY);

	fg_tilemap = tilemap_create(get_fg_tile_info,tilemap_scan_rows,
								TILEMAP_TRANSPARENT,
								16,16,FG_NX,FG_NY);

	/* 0x400 sprites max */
	spritelist = malloc(0x400 * sizeof(spritelist[0]));

	if (!bg_tilemap || !fg_tilemap || !spritelist)
		return 1;

	{
/*
gtmr background:
		flipscreen off: write (x)-$33
		[x=fetch point (e.g. scroll *left* with incresing x)]

		flipscreen on:  write (x+320)+$33
		[x=fetch point (e.g. scroll *right* with incresing x)]

		W = 320+$33+$33 = $1a6 = 422

berlwall background:
6940 off	1a5 << 6
5680 on		15a << 6
*/
		int xdim = Machine->drv->screen_width;
		int ydim = Machine->drv->screen_height;
		int dx, dy;

//		dx   = (422 - xdim) / 2;
//		dy = ydim - (Machine->visible_area.max_y - Machine->visible_area.min_y + 1);

		switch (xdim)
		{
			case 320:	dx = 0x33;	break;
			case 256:	dx = 0x5b;	break;
			default:	dx = 0;
		}
		switch (Machine->visible_area.max_y - Machine->visible_area.min_y + 1)
		{
			case 240- 8:	dy = +0x08;	break;
			case 240-16:	dy = -0x08;	break;
			default:		dy = 0;
		}

		tilemap_set_scrolldx( bg_tilemap, -dx,		xdim + dx -1        );
		tilemap_set_scrolldx( fg_tilemap, -(dx+2),	xdim + (dx + 2) - 1 );

		tilemap_set_scrolldy( bg_tilemap, -dy,		ydim + dy -1 );
		tilemap_set_scrolldy( fg_tilemap, -dy,		ydim + dy -1);

		tilemap_set_transparent_pen(bg_tilemap,0);
		tilemap_set_transparent_pen(fg_tilemap,0);
		return 0;
	}
}


void kaneko16_vh_stop(void)
{
	if (spritelist)
		free(spritelist);
	spritelist = 0;		// multisession safety
}


/* Berlwall has an additional hi-color background */

void berlwall_init_palette(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	palette += 2048 * 3;	/* first 2048 colors are dynamic */

	/* initialize 555 RGB lookup */
	for (i = 0; i < 32768; i++)
	{
		int r,g,b;

		r = (i >>  5) & 0x1f;
		g = (i >> 10) & 0x1f;
		b = (i >>  0) & 0x1f;

		(*palette++) = (r << 3) | (r >> 2);
		(*palette++) = (g << 3) | (g >> 2);
		(*palette++) = (b << 3) | (b >> 2);
	}
}

int berlwall_vh_start(void)
{
	int sx, x,y;
	unsigned char *RAM	=	memory_region(REGION_GFX3);

	/* Render the hi-color static backgrounds held in the ROMs */

	if ((kaneko16_bg15_bitmap = bitmap_alloc_depth(256 * 32, 256 * 1, 16)) == 0)
		return 1;

/*
	8aba is used as background color
	8aba/2 = 455d = 10001 01010 11101 = $11 $0a $1d
*/

	for (sx = 0 ; sx < 32 ; sx++)	// horizontal screens
	 for (x = 0 ; x < 256 ; x++)	// horizontal pixels
	  for (y = 0 ; y < 256 ; y++)	// vertical pixels
	  {
			int addr  = sx * (256 * 256) + x + y * 256;

			int color = ( RAM[addr * 2 + 0] * 256 + RAM[addr * 2 + 1] ) >> 1;
//				color ^= (0x8aba/2);

			plot_pixel( kaneko16_bg15_bitmap,
						sx * 256 + x, y,
						Machine->pens[2048 + color] );
	  }

	return kaneko16_vh_start();
}

void berlwall_vh_stop(void)
{
	if (kaneko16_bg15_bitmap)
		bitmap_free(kaneko16_bg15_bitmap);

	kaneko16_bg15_bitmap = 0;	// multisession safety
	kaneko16_vh_stop();
}



/***************************************************************************

								Sprites Drawing

	Sprite data is layed out in RAM in 3 different ways for different games
	(type 0,1,2). This basically involves the bits in the attribute word
	to be shuffled around and/or the words being in different order.

	Each sprite is always stuffed in 4 words. There may be some extra padding
	words thioug (e.g. type 2 sprites are like type 0 but the data is held in
	the last 8 bytes of every 16). Examples are:

	Type 0: shogwarr, blazeon, bakubrkr.
	Type 1: gtmr.
	Type 2: berlwall

Offset:			Format:						Value:

0000.w			Attribute (type 0 & 2)

					f--- ---- ---- ----		Multisprite: Use Latched Code + 1
					-e-- ---- ---- ----		Multisprite: Use Latched Color (And Flip?)
					--d- ---- ---- ----		Multisprite: Use Latched X,Y As Offsets
					---c ba-- ---- ----
					---- --9- ---- ----		High Priority (vs FG Tiles Of High Priority)
					---- ---8 ---- ----		High Priority (vs BG Tiles Of High Priority)
					---- ---- 7654 32--		Color
					---- ---- ---- --1-		X Flip
					---- ---- ---- ---0		Y Flip

				Attribute (type 1)

					f--- ---- ---- ----		Multisprite: Use Latched Code + 1
					-e-- ---- ---- ----		Multisprite: Use Latched Color (And Flip?)
					--d- ---- ---- ----		Multisprite: Use Latched X,Y As Offsets
					---c ba-- ---- ----
					---- --9- ---- ----		X Flip
					---- ---8 ---- ----		Y Flip
					---- ---- 7--- ----		High Priority (vs FG Tiles Of High Priority)
					---- ---- -6-- ----		High Priority (vs BG Tiles Of High Priority)
					---- ---- --54 3210		Color

0002.w										Code
0004.w										X Position << 6
0006.w										Y Position << 6


IMPORTANT:	It's not really needed to cope with the different sprite data
			arrangement with 3 different drawing routines. We have 1 routine
			which just converts every sprite type to type 1 !
			(by shuffling the bits in the attribute word and/or the
			 words for every sprite)


***************************************************************************/

/* sprites -> gfx[1] */
#define SPRITE_GFX 1

/* Map the attribute word to that of the type 1 sprite hardware */

#define MAP_TO_TYPE1(attr) \
	if (kaneko16_spritetype != 1)	/* shogwarr, berlwall */ \
	{ \
		attr =	((attr & 0xfc00)     ) | \
				((attr & 0x03fc) >> 2) | \
				((attr & 0x0003) << 8) ; \
	}


/* Draw the sprites */

void kaneko16_draw_sprites(struct osd_bitmap *bitmap, int priority)
{
	int offs,inc;

	/* Sprites *must* be parsed from the first in ram to the last,
	   because of the multisprite feature. But they *must* be drawn
	   from the last in ram (frontmost) to the firtst in order to
	   cope with priorities using pdrawgfx.

	   Hence we parse them from first to last and put the result
	   in a temp buffer, then draw the buffer's contents from last
	   to first. */

	struct tempsprite *sprite_ptr = spritelist;

	int max_x	=	Machine->drv->screen_width  - 16;
	int max_y	=	Machine->drv->screen_height - 16;

	int sx = 0;
	int sy = 0;
	int scode = 0;
	int sattr = 0;
	int sflipx = 0;
	int sflipy = 0;

	switch (kaneko16_spritetype)
	{
		case 2:		inc = 8;	offs = 4;	break;
		default:	inc = 4;	offs = 0;	break;
	}

	for ( ; offs < spriteram_size/2 ; offs += inc )
	{
		int curr_pri;

		int attr	=	spriteram16[offs + 0];
		int code	=	spriteram16[offs + 1];
		int x		=	spriteram16[offs + 2];
		int y		=	spriteram16[offs + 3];

		/* Map the attribute word to that of the type 1 sprite hardware */
		MAP_TO_TYPE1(attr)

		if (x & 0x8000)	x -= 0x10000;
		if (y & 0x8000)	y -= 0x10000;

		x /= 0x40;		y /= 0x40;

		if (attr & 0x8000)		scode++;
		else					scode = code;

		if (!(attr & 0x4000))
		{
			sattr  = attr;
			sflipx = attr & 0x200;	sflipy = attr & 0x100;
		}

		if (attr & 0x2000)		{ sx += x;	sy += y; }
		else					{ sx  = x;	sy  = y; }

		/* Priority */
		curr_pri = (sattr & 0xc0) >> 6;

		/* You can choose which sprite priorities get displayed (for debug) */
		if ( ((1 << curr_pri) & priority) == 0 )	continue;

		if (flipsprites & 2) { sx = max_x - sx;		sflipx = !sflipx; }
		if (flipsprites & 1) { sy = max_y - sy;		sflipy = !sflipy; }

		/* We can buffer this sprite now */
		sprite_ptr->code		=		scode;
		sprite_ptr->color		=		sattr;
		sprite_ptr->flipx		=		sflipx;
		sprite_ptr->flipy		=		sflipy;
		sprite_ptr->x			=		sx;
		sprite_ptr->y			=		sy;
		sprite_ptr->primask		=		0x00000000;
#if 0
		switch ( curr_pri  )
		{
			case 0: sprite_ptr->primask |= 0xcccccccc;	/* below the top 3 "layers" */
			case 1: sprite_ptr->primask |= 0xf0f0f0f0;	/* below the top 2 "layers" */
			case 2: sprite_ptr->primask |= 0xff00ff00;	/* below the top "layer" */
//			case 3: sprite_ptr->primask |= 0x00000000;	/* above all */
		}
#else
		switch ( curr_pri  )
		{
			case 0: sprite_ptr->primask = 0xaaaaaaaa | 0xcccccccc;	break;	/* below bg, below fg */
			case 1: sprite_ptr->primask = 0x00000000 | 0xcccccccc;	break;	/* over  bg, below fg */
			case 2: sprite_ptr->primask = 0xaaaaaaaa | 0x00000000;	break;	/* below bg, over  fg */
			case 3: sprite_ptr->primask = 0x00000000 | 0x00000000;	break;	/* over all */
		}
#endif


		/* let's get back to normal to support multi sprites */
		if (flipsprites & 2) { sx = max_x - sx;		sflipx = !sflipx; }
		if (flipsprites & 1) { sy = max_y - sy;		sflipy = !sflipy; }

		sprite_ptr ++;
	}


	/* Let's finally draw the sprites we buffered */

	while (sprite_ptr != spritelist)
	{
		sprite_ptr --;

		pdrawgfx(	bitmap,Machine->gfx[SPRITE_GFX],
					sprite_ptr->code,
					sprite_ptr->color,
					sprite_ptr->flipx, sprite_ptr->flipy,
					sprite_ptr->x, sprite_ptr->y,
					&Machine->visible_area,TRANSPARENCY_PEN,0,
					sprite_ptr->primask );
	}

}




/* Mark the pens of visible sprites */

void kaneko16_mark_sprites_colors(void)
{
	int offs,inc;

	int xmin = Machine->visible_area.min_x - (16 - 1);
	int xmax = Machine->visible_area.max_x;
	int ymin = Machine->visible_area.min_y - (16 - 1);
	int ymax = Machine->visible_area.max_y;

	int nmax				=	Machine->gfx[SPRITE_GFX]->total_elements;
	int color_granularity	=	Machine->gfx[SPRITE_GFX]->color_granularity;
	int color_codes_start	=	Machine->drv->gfxdecodeinfo[SPRITE_GFX].color_codes_start;
	int total_color_codes	=	Machine->drv->gfxdecodeinfo[SPRITE_GFX].total_color_codes;

	int sx = 0;
	int sy = 0;
	int scode = 0;
	int scolor = 0;

	switch (kaneko16_spritetype)
	{
		case 2:		inc = 8;	offs = 4;	break;
		default:	inc = 4;	offs = 0;	break;
	}

	for ( ;  offs < spriteram_size/2 ; offs += inc)
	{
		int	attr	=	spriteram16[offs + 0];
		int	code	=	spriteram16[offs + 1] % nmax;
		int	x		=	spriteram16[offs + 2];
		int	y		=	spriteram16[offs + 3];

		/* Map the attribute word to that of the type 1 sprite hardware */
		MAP_TO_TYPE1(attr)

		if (x & 0x8000)	x -= 0x10000;
		if (y & 0x8000)	y -= 0x10000;

		x /= 0x40;		y /= 0x40;

		if (attr & 0x8000)		scode++;
		else					scode = code;

		if (!(attr & 0x4000))	scolor = attr % total_color_codes;

		if (attr & 0x2000)		{ sx += x;	sy += y; }
		else					{ sx  = x;	sy  = y; }

		/* Visibility check. No need to account for sprites flipping */
		if ((sx < xmin) || (sx > xmax))	continue;
		if ((sy < ymin) || (sy > ymax))	continue;

		memset(&palette_used_colors[color_granularity * scolor + color_codes_start + 1],PALETTE_COLOR_USED,color_granularity - 1);
	}

}




/***************************************************************************

								Screen Drawing

***************************************************************************/

void kaneko16_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i,pri,flag;
	int layers_ctrl = -1;
	int layers_flip = kaneko16_layers1_regs[ 4 ];

	tilemap_set_enable(fg_tilemap, (layers_flip & 0x0010) ? 0 : 1 );
	tilemap_set_enable(bg_tilemap, (layers_flip & 0x1000) ? 0 : 1 );

	tilemap_set_flip(fg_tilemap, ((layers_flip & 0x0001) ? TILEMAP_FLIPY : 0) |
								 ((layers_flip & 0x0002) ? TILEMAP_FLIPX : 0) );

	tilemap_set_flip(bg_tilemap, ((layers_flip & 0x0100) ? TILEMAP_FLIPY : 0) |
								 ((layers_flip & 0x0200) ? TILEMAP_FLIPX : 0) );

	tilemap_set_scrollx(fg_tilemap, 0, kaneko16_layers1_regs[ 0 ] >> 6 );
	tilemap_set_scrolly(fg_tilemap, 0, kaneko16_layers1_regs[ 1 ] >> 6 );

	tilemap_set_scrollx(bg_tilemap, 0, kaneko16_layers1_regs[ 2 ] >> 6 );
	tilemap_set_scrolly(bg_tilemap, 0, kaneko16_layers1_regs[ 3 ] >> 6 );


#ifdef MAME_DEBUG
if ( keyboard_pressed(KEYCODE_Z) ||
	 keyboard_pressed(KEYCODE_X) || keyboard_pressed(KEYCODE_C) ||
     keyboard_pressed(KEYCODE_V) || keyboard_pressed(KEYCODE_B) )
{
	int msk = 0, val = 0;

	if (keyboard_pressed(KEYCODE_X))	val = 1;	// priority 0 only
	if (keyboard_pressed(KEYCODE_C))	val = 2;	// ""       1
	if (keyboard_pressed(KEYCODE_V))	val = 4;	// ""       2
	if (keyboard_pressed(KEYCODE_B))	val = 8;	// ""       3

	if (keyboard_pressed(KEYCODE_Z))	val = 1|2|4|8;	// All of the above priorities

	if (keyboard_pressed(KEYCODE_Q))	msk |= val << 0;	// for layer 0
	if (keyboard_pressed(KEYCODE_W))	msk |= val << 4;	// for layer 1
//	if (keyboard_pressed(KEYCODE_E))	msk |= val << 8;	// for layer 2
	if (keyboard_pressed(KEYCODE_A))	msk |= val << 12;	// for sprites
	if (msk != 0) layers_ctrl &= msk;

#if 0
{
	char buf[256];
	sprintf (buf, "%04X %04X %04X %04X %04X %04X %04X %04X - %04X %04X %04X %04X %04X %04X %04X %04X",
		kaneko16_layers1_regs[0x0],kaneko16_layers1_regs[0x1],
		kaneko16_layers1_regs[0x2],kaneko16_layers1_regs[0x3],
		kaneko16_layers1_regs[0x4],kaneko16_layers1_regs[0x5],
		kaneko16_layers1_regs[0x6],kaneko16_layers1_regs[0x7],

		kaneko16_layers1_regs[0x8],kaneko16_layers1_regs[0x9],
		kaneko16_layers1_regs[0xa],kaneko16_layers1_regs[0xb],
		kaneko16_layers1_regs[0xc],kaneko16_layers1_regs[0xd],
		kaneko16_layers1_regs[0xe],kaneko16_layers1_regs[0xf],
	);
	usrintf_showmessage(buf);
}
#endif
}
#endif

	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();

	kaneko16_mark_sprites_colors();
	palette_used_colors[0] = PALETTE_COLOR_USED;	// fillbitmap uses Machine->pens[0]

	palette_recalc();

	flag = TILEMAP_IGNORE_TRANSPARENCY;

	/* Draw the high colour bg layer first, if any */

	if (kaneko16_bg15_bitmap)
	{
/*
	firstscreen	?				(hw: b8,00/06/7/8/9(c8). 202872 = 0880)
	press start	?				(hw: 80-d0,0a. 202872 = 0880)
	teaching	?				(hw: e0,1f. 202872 = 0880 )
	hiscores	!rom5,scr1($9)	(hw: b0,1f. 202872 = )
	lev1-1		rom6,scr2($12)	(hw: cc,0e. 202872 = 0880)
	lev2-1		?				(hw: a7,01. 202872 = 0880)
	lev2-2		rom6,scr1($11)	(hw: b0,0f. 202872 = 0880)
	lev2-4		rom6,scr0($10)	(hw: b2,10. 202872 = 0880)
	lev2-6?		rom5,scr7($f)	(hw: c0,11. 202872 = 0880)
	lev4-2		rom5,scr6($e)	(hw: d3,12. 202872 = 0880)
	redcross	?				(hw: d0,0a. 202872 = )
*/
		int select	=	kaneko16_bg15_select[ 0 ];
//		int reg		=	kaneko16_bg15_reg[ 0 ];
		int flip	=	select & 0x20;
		int sx, sy;

		if (flip)	select ^= 0x1f;

		sx		=	(select & 0x1f) * 256;
		sy		=	0;

		copybitmap(
			bitmap, kaneko16_bg15_bitmap,
			flip, flip,
			-sx, -sy,
			&Machine->visible_area, TRANSPARENCY_NONE,0 );

		flag = 0;
	}

	/* Fill the bitmap with pen 0. This is wrong, but will work most of
	   the times. To do it right, each pixel should be drawn with pen 0
	   of the bottomost tile that covers it (which is pretty tricky to do) */

	if (flag!=0)	fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);

	fillbitmap(priority_bitmap,0,NULL);

	/*
		First draw the tiles. I implemented the priorities like this:

		Sprite / Sprite:
			The order in sprite RAM (the first sprite is the bottomost)

	   	Tile / Tile:
			For tiles with the same priority, draw the bg first.
			Otherwise bit 0 = 1 means "high priority" onother tiles.

		Tile / Sprite:
			Bit 1 = 1 in tiles means "high priority" on sprites.
			Sprites with bit 0 = 1 have priority over "high priority" bg tiles
			Sprites with bit 1 = 1 have priority over "high priority" fg tiles

		In the code below, "high priority" bg tiles are assigned a priority
		mask of 1, "High priority" fg tiles a mask of 2.
	*/

	for ( i = 0; i < 4; i++ )
	{
		int sprite_pri;
		int order[] = {0,2,1,3};

		pri = order[i];
		sprite_pri = (pri & 2) ? 1 : 0;

		if (layers_ctrl&(1<<(pri+0)))	tilemap_draw(bitmap, bg_tilemap, pri, sprite_pri << 0 );
		if (layers_ctrl&(1<<(pri+4)))	tilemap_draw(bitmap, fg_tilemap, pri, sprite_pri << 1 );
	}

	/* Sprites last (rendered with pdrawgfx, so they can slip
	   in between the fg layer) */

	if (layers_ctrl & (0xf<<12))
		kaneko16_draw_sprites(bitmap, (layers_ctrl >> 12) & 0xf);
}
