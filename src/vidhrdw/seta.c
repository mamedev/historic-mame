/***************************************************************************

							-= Seta Hardware =-

					driver by	Luca Elia (l.elia@tin.it)


Note:	if MAME_DEBUG is defined, pressing Z with:

		Q			shows layer 0
		W			shows layer 1
		A			shows the sprites

		Keys can be used togheter!


						[ 0, 1 Or 2 Scrolling Layers ]

	Each layer consists of 2 tilemaps: only one can be displayed at a
	given time (the games usually flip continuously between the two).
	The two tilemaps share the same scrolling registers.

		Layer Size:				1024 x 512
		Tiles:					16x16x4 (16x16x6 in some games)
		Tile Format:

			Offset + 0x0000:
							f--- ---- ---- ----		Flip X
							-e-- ---- ---- ----		Flip Y
							--dc ba98 7654 3210		Code

			Offset + 0x1000:

							fedc ba98 765- ----		-
							---- ---- ---4 3210		Color

			The other tilemap for this layer (always?) starts at
			Offset + 0x2000.


							[ 1024 Sprites ]

	Sprites are 16x16x4. They are just like those in "The Newzealand Story",
	"Revenge of DOH" etc (tnzs.c). Obviously they're hooked to a 16 bit
	CPU here, so they're mapped a bit differently in memory. Additionally,
	there are two banks of sprites. The game can flip between the two to
	do double buffering, writing to a bit of a control register(see below)


		Spriteram16_2 + 0x000.w

						f--- ---- ---- ----		Flip X
						-e-- ---- ---- ----		Flip Y
						--dc b--- ---- ----		-
						---- --98 7654 3210		Code (Lower bits)

		Spriteram16_2 + 0x400.w

						fedc b--- ---- ----		Color
						---- -a9- ---- ----		Code (Upper Bits)
						---- ---8 7654 3210		X

		Spriteram16   + 0x000.w

						fedc ba98 ---- ----		-
						---- ---- 7654 3210		Y



							[ Floating Tilemap ]

	There's a floating tilemap made of vertical colums composed of 2x16
	"sprites". Each 32 consecutive "sprites" define a column.

	For column I:

		Spriteram16_2 + 0x800 + 0x40 * I:

						f--- ---- ---- ----		Flip X
						-e-- ---- ---- ----		Flip Y
						--dc b--- ---- ----		-
						---- --98 7654 3210		Code (Lower bits)

		Spriteram16_2 + 0xc00 + 0x40 * I:

						fedc b--- ---- ----		Color
						---- -a9- ---- ----		Code (Upper Bits)
						---- ---8 7654 3210		-

	Each column	has a variable horizontal position and a vertical scrolling
	value (see also the Sprite Control Registers). For column I:


		Spriteram16   + 0x400 + 0x20 * I:

						fedc ba98 ---- ----		-
						---- ---- 7654 3210		Y

		Spriteram16   + 0x408 + 0x20 * I:

						fedc ba98 ---- ----		-
						---- ---- 7654 3210		Low Bits Of X



						[ Sprites Control Registers ]


		Spriteram16   + 0x601.b

						7--- ----		0
						-6-- ----		Flip Screen
						--5- ----		0
						---4 ----		1 (Sprite Enable?)
						---- 3210		???

		Spriteram16   + 0x603.b

						7--- ----		0
						-6-- ----		Sprite Bank
						--5- ----		0 = Sprite Buffering (blandia,msgundam,qzkklogy)
						---4 ----		0
						---- 3210		Columns To Draw (1 is the special value for 16)

		Spriteram16   + 0x605.b

						7654 3210		High Bit Of X For Columns 7-0

		Spriteram16   + 0x607.b

						7654 3210		High Bit Of X For Columns f-8




***************************************************************************/

#include "vidhrdw/generic.h"
#include "seta.h"

/* Variables only used here */

static struct tilemap *tilemap_0, *tilemap_1;	// Layer 0
static struct tilemap *tilemap_2, *tilemap_3;	// Layer 1
static int tilemaps_flip;

/* Variables used elsewhere */

int seta_tiles_offset;

data16_t *seta_vram_0, *seta_vram_1, *seta_vctrl_0;
data16_t *seta_vram_2, *seta_vram_3, *seta_vctrl_2;
data16_t *seta_vregs;

extern int seta_tiles_offset;


WRITE16_HANDLER( seta_vregs_w )
{
	COMBINE_DATA(&seta_vregs[offset]);
	switch (offset)
	{
		case 0/2:

/*		fedc ba98 76-- ----
		---- ---- --5- ----		Sound Enable
		---- ---- ---4 ----		?? 1 in oisipuzl, sokonuke (layers related)
		---- ---- ---- 3---		Coin #1 Lock Out
		---- ---- ---- -2--		Coin #0 Lock Out
		---- ---- ---- --1-		Coin #1 Counter
		---- ---- ---- ---0		Coin #0 Counter		*/
			if (ACCESSING_LSB)
			{
				seta_coin_lockout_w (data & 0x000f);
				seta_sound_enable_w (data & 0x0020);
			}
			break;

		case 2/2:
			if (ACCESSING_LSB)
			{
				unsigned char *RAM = memory_region(REGION_SOUND1);
				int new_bank;

				/* Partly handled in vh_screenrefresh:

						fedc ba98 76-- ----
						---- ---- --54 3---		Samples Bank (in blandia, eightfrc)
						---- ---- ---- -2--
						---- ---- ---- --1-		Sprites Above Frontmost Layer
						---- ---- ---- ---0		Layer 0 Above Layer 1
				*/

				new_bank = (data >> 3) & 0x7;

				if (new_bank != seta_samples_bank)
				{
					int samples_len, addr;

					seta_samples_bank = new_bank;

					samples_len = memory_region_length(REGION_SOUND1);

					addr = 0x40000 * new_bank;
					if (new_bank >= 3)	addr += 0x40000;

					if ( (samples_len > 0x100000) && ((addr+0x40000) <= samples_len) )
						memcpy(&RAM[0xc0000],&RAM[addr],0x40000);
					else
						logerror("PC %06X - Invalid samples bank %02X !\n", activecpu_get_pc(), new_bank);
				}

			}
			break;


		case 4/2:	// ?
			break;
	}
}




/***************************************************************************

						Callbacks for the TileMap code

							  [ Tiles Format ]

Offset + 0x0000:
					f--- ---- ---- ----		Flip X
					-e-- ---- ---- ----		Flip Y
					--dc ba98 7654 3210		Code

Offset + 0x1000:

					fedc ba98 765- ----		-
					---- ---- ---4 3210		Color


					  [ TileMaps Control Registers]

Offset + 0x0:								Scroll X
Offset + 0x2:								Scroll Y
Offset + 0x4:
					fedc ba98 7654 3210		-
					---- ---- ---- 3---		Tilemap Select (There Are 2 Tilemaps Per Layer)
					---- ---- ---- -21-		0 (1 only in eightfrc, when flip is on!)
					---- ---- ---- ---0		?

***************************************************************************/
#define DIM_NX		(64)
#define DIM_NY		(32)

#define SETA_TILEMAP(_n_) \
static void get_tile_info_##_n_( int tile_index ) \
{ \
	data16_t code =	seta_vram_##_n_[ tile_index ]; \
	data16_t attr =	seta_vram_##_n_[ tile_index + DIM_NX * DIM_NY ]; \
	SET_TILE_INFO( 1 + _n_/2, seta_tiles_offset + (code & 0x3fff), attr & 0x1f, TILE_FLIPXY( code >> (16-2) )) \
} \
\
WRITE16_HANDLER( seta_vram_##_n_##_w ) \
{ \
	data16_t oldword = seta_vram_##_n_[offset]; \
	data16_t newword = COMBINE_DATA(&seta_vram_##_n_[offset]); \
	if (oldword != newword) \
	{ \
		offset %= DIM_NX * DIM_NY; \
		tilemap_mark_tile_dirty(tilemap_##_n_, offset); \
	} \
}

SETA_TILEMAP(0)
SETA_TILEMAP(1)
SETA_TILEMAP(2)
SETA_TILEMAP(3)


/* 2 layers */
VIDEO_START( seta_2_layers )
{
	/* Each layer consists of 2 tilemaps: only one can be displayed
	   at any given time */

	/* layer 0 */
	tilemap_0 = tilemap_create(	get_tile_info_0, tilemap_scan_rows,
								TILEMAP_TRANSPARENT, 16,16, DIM_NX,DIM_NY );

	tilemap_1 = tilemap_create(	get_tile_info_1, tilemap_scan_rows,
								TILEMAP_TRANSPARENT, 16,16, DIM_NX,DIM_NY );


	/* layer 1 */
	tilemap_2 = tilemap_create(	get_tile_info_2, tilemap_scan_rows,
								TILEMAP_TRANSPARENT, 16,16, DIM_NX,DIM_NY );

	tilemap_3 = tilemap_create(	get_tile_info_3, tilemap_scan_rows,
								TILEMAP_TRANSPARENT, 16,16, DIM_NX,DIM_NY );

	if (tilemap_0 && tilemap_1 && tilemap_2 && tilemap_3)
	{
		tilemaps_flip = 0;

		tilemap_set_scroll_rows(tilemap_0,1);
		tilemap_set_scroll_cols(tilemap_0,1);
		tilemap_set_transparent_pen(tilemap_0,0);

		tilemap_set_scroll_rows(tilemap_1,1);
		tilemap_set_scroll_cols(tilemap_1,1);
		tilemap_set_transparent_pen(tilemap_1,0);

		tilemap_set_scroll_rows(tilemap_2,1);
		tilemap_set_scroll_cols(tilemap_2,1);
		tilemap_set_transparent_pen(tilemap_2,0);

		tilemap_set_scroll_rows(tilemap_3,1);
		tilemap_set_scroll_cols(tilemap_3,1);
		tilemap_set_transparent_pen(tilemap_3,0);

		tilemap_set_scrolldx(tilemap_0, -0x01, 0x00);	// see zingzip test mode
		tilemap_set_scrolldx(tilemap_1, -0x01, 0x00);
		tilemap_set_scrolldx(tilemap_2, -0x01, 0x00);
		tilemap_set_scrolldx(tilemap_3, -0x01, 0x00);

		tilemap_set_scrolldy(tilemap_0, 0x00, 0x00);
		tilemap_set_scrolldy(tilemap_1, 0x00, 0x00);
		tilemap_set_scrolldy(tilemap_2, 0x00, 0x00);
		tilemap_set_scrolldy(tilemap_3, 0x00, 0x00);

		return 0;
	}
	else return 1;
}


/* 1 layer */
VIDEO_START( seta_1_layer )
{
	/* Each layer consists of 2 tilemaps: only one can be displayed
	   at any given time */

	/* layer 0 */
	tilemap_0 = tilemap_create(	get_tile_info_0, tilemap_scan_rows,
								TILEMAP_TRANSPARENT, 16,16, DIM_NX,DIM_NY );

	tilemap_1 = tilemap_create(	get_tile_info_1, tilemap_scan_rows,
								TILEMAP_TRANSPARENT, 16,16, DIM_NX,DIM_NY );


	/* NO layer 1 */
	tilemap_2 = 0;
	tilemap_3 = 0;

	if (tilemap_0 && tilemap_1)
	{
		tilemaps_flip = 0;

		tilemap_set_scroll_rows(tilemap_0,1);
		tilemap_set_scroll_cols(tilemap_0,1);
		tilemap_set_transparent_pen(tilemap_0,0);

		tilemap_set_scroll_rows(tilemap_1,1);
		tilemap_set_scroll_cols(tilemap_1,1);
		tilemap_set_transparent_pen(tilemap_1,0);

		tilemap_set_scrolldx(tilemap_0, 0x00, 0x00); // see metafox test mode
		tilemap_set_scrolldx(tilemap_1, 0x00, 0x00);

		tilemap_set_scrolldy(tilemap_0, 0x00, 0x00);
		tilemap_set_scrolldy(tilemap_1, 0x00, 0x00);

		return 0;
	}
	else return 1;
}


/* NO layers, only sprites */
VIDEO_START( seta_no_layers )
{
	tilemap_0 = 0;
	tilemap_1 = 0;
	tilemap_2 = 0;
	tilemap_3 = 0;
	return 0;
}

VIDEO_START( seta_2_layers_y_offset_0x10 )
{
	if (video_start_seta_2_layers())
		return 1;

	tilemap_set_scrolldx(tilemap_0, 0x00, 0x00);
	tilemap_set_scrolldx(tilemap_1, 0x00, 0x00);
	tilemap_set_scrolldx(tilemap_2, 0x00, 0x00);
	tilemap_set_scrolldx(tilemap_3, 0x00, 0x00);

	tilemap_set_scrolldy(tilemap_0, 0x10, 0x00);
	tilemap_set_scrolldy(tilemap_1, 0x10, 0x00);
	tilemap_set_scrolldy(tilemap_2, 0x10, 0x00);
	tilemap_set_scrolldy(tilemap_3, 0x10, 0x00);
	return 0;
}

VIDEO_START( seta_2_layers_offset_0x02 )
{
	if (video_start_seta_2_layers())
		return 1;

	tilemap_set_scrolldx(tilemap_0, -0x02, 0x00);
	tilemap_set_scrolldx(tilemap_1, -0x02, 0x00);
	tilemap_set_scrolldx(tilemap_2, -0x02, 0x00);
	tilemap_set_scrolldx(tilemap_3, -0x02, 0x00);

	tilemap_set_scrolldy(tilemap_0, 0x00, 0x00);
	tilemap_set_scrolldy(tilemap_1, 0x00, 0x00);
	tilemap_set_scrolldy(tilemap_2, 0x00, 0x00);
	tilemap_set_scrolldy(tilemap_3, 0x00, 0x00);
	return 0;
}

VIDEO_START( oisipuzl_2_layers )
{
	if (video_start_seta_2_layers_offset_0x02())
		return 1;
	tilemaps_flip = 1;
	return 0;
}

VIDEO_START( seta_1_layer_offset_0x02 )
{
	if (video_start_seta_1_layer())	return 1;

	tilemap_set_scrolldx(tilemap_0, -0x02, 0x00); // see calibr50's rescue
	tilemap_set_scrolldx(tilemap_1, -0x02, 0x00);

	tilemap_set_scrolldy(tilemap_0, 0x00, 0x00);
	tilemap_set_scrolldy(tilemap_1, 0x00, 0x00);

	return 0;
}


/***************************************************************************


							Palette Init Functions


***************************************************************************/


/* 2 layers, 6 bit deep. The color codes have a 16 color granularity.

   One layer only uses the first 16 colors of the palette (repeated
   4 times to fill the 64 colors) and regardless of the color code!

   The other uses the first 64 colors of the palette regardless of
   the color code too!

   I think that's because this game's a prototype..
*/
PALETTE_INIT( blandia )
{
	int color, pen;
	for( color = 0; color < 32; color++ )
		for( pen = 0; pen < 64; pen++ )
		{
			colortable[color * 64 + pen + 16*32]       = (pen%16) + 16*32*1;
			colortable[color * 64 + pen + 16*32+64*32] = pen      + 16*32*2;
		}
}



/* layers have 6 bits per pixel, but the color code has a 16 colors granularity,
   even if the low 2 bits are ignored (so there are only 4 different palettes) */
PALETTE_INIT( gundhara )
{
	int color, pen;
	for( color = 0; color < 32; color++ )
		for( pen = 0; pen < 64; pen++ )
		{
			colortable[color * 64 + pen + 32*16 + 32*64*0] = (((color&~3) * 16 + pen)%(32*16)) + 32*16*2;
			colortable[color * 64 + pen + 32*16 + 32*64*1] = (((color&~3) * 16 + pen)%(32*16)) + 32*16*1;
		}
}



/* layers have 6 bits per pixel, but the color code has a 16 colors granularity */
PALETTE_INIT( jjsquawk )
{
	int color, pen;
	for( color = 0; color < 32; color++ )
		for( pen = 0; pen < 64; pen++ )
		{
			colortable[color * 64 + pen + 32*16 + 32*64*0] = ((color * 16 + pen)%(32*16)) + 32*16*2;
			colortable[color * 64 + pen + 32*16 + 32*64*1] = ((color * 16 + pen)%(32*16)) + 32*16*1;
		}
}


/* layer 0 is 6 bit per pixel, but the color code has a 16 colors granularity */
PALETTE_INIT( zingzip )
{
	int color, pen;
	for( color = 0; color < 32; color++ )
		for( pen = 0; pen < 64; pen++ )
			colortable[color * 64 + pen + 32*16*2] = ((color * 16 + pen)%(32*16)) + 32*16*2;
}




/* 6 bit layer. The colors are still WRONG */
PALETTE_INIT( usclssic )
{
	int color, pen;
	for( color = 0; color < 32; color++ )
		for( pen = 0; pen < 64; pen++ )
			colortable[color * 64 + pen + 512] = (((color & 0xf) * 16 + pen)%(512));
}




/***************************************************************************


								Sprites Drawing


***************************************************************************/


static void seta_draw_sprites_map(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int offs, col;
	int xoffs, yoffs;

	int total_color_codes	=	Machine->drv->gfxdecodeinfo[0].total_color_codes;

	int ctrl	=	spriteram16[ 0x600/2 ];
	int ctrl2	=	spriteram16[ 0x602/2 ];

	int flip	=	ctrl & 0x40;
	int numcol	=	ctrl2 & 0x000f;

	/* Sprites Banking and/or Sprites Buffering */
	data16_t *src = spriteram16_2 + ( ((ctrl2 ^ (~ctrl2<<1)) & 0x40) ? 0x2000/2 : 0 );

	int upper	=	( spriteram16[ 0x604/2 ] & 0xFF ) +
					( spriteram16[ 0x606/2 ] & 0xFF ) * 256;

	int max_y	=	0xf0;

	int col0;		/* Kludge, needed for krzybowl and kiwame */
	switch (ctrl & 0x0f)
	{
		case 0x01:	col0	=	0x4;	break;	// krzybowl
		case 0x06:	col0	=	0x8;	break;	// kiwame

		default:	col0	=	0x0;
	}

	xoffs	=	flip ? 0x10 : 0x10;	// see wrofaero test mode: made of sprites map
	yoffs	=	flip ? 0x09 : 0x07;

	/* Number of columns to draw - the value 1 seems special, meaning:
	   draw every column */
	if (numcol == 1)	numcol = 16;


	/* The first column is the frontmost, see twineagl test mode */
	for ( col = numcol - 1 ; col >= 0; col -- )
	{
		int x	=	spriteram16[(col * 0x20 + 0x08 + 0x400)/2] & 0xff;
		int y	=	spriteram16[(col * 0x20 + 0x00 + 0x400)/2] & 0xff;

		/* draw this column */
		for ( offs = 0 ; offs < 0x40/2; offs += 2/2 )
		{
			int	code	=	src[((col+col0)&0xf) * 0x40/2 + offs + 0x800/2];
			int	color	=	src[((col+col0)&0xf) * 0x40/2 + offs + 0xc00/2];

			int	flipx	=	code & 0x8000;
			int	flipy	=	code & 0x4000;

			int bank	=	(color & 0x0600) >> 9;

/*
twineagl:	010 02d 0f 10	(ship)
tndrcade:	058 02d 07 18	(start of game - yes, flip on!)
arbalest:	018 02d 0f 10	(logo)
metafox :	018 021 0f f0	(bomb)
zingzip :	010 02c 00 0f	(bomb)
wrofaero:	010 021 00 ff	(test mode)
thunderl:	010 06c 00 ff	(always?)
krzybowl:	011 028 c0 ff	(game)
kiwame  :	016 021 7f 00	(logo)
oisipuzl:	059 020 00 00	(game - yes, flip on!)
*/

			int sx		=	  x + xoffs  + (offs & 1) * 16;
//			int sy		=	-(y + yoffs) + (offs / 2) * 16;
			int sy		=	-(y + yoffs) + (offs / 2) * 16 -
							(Machine->drv->screen_height-(Machine->visible_area.max_y + 1));

			if (upper & (1 << col))	sx += 256;

			if (flip)
			{
				sy = max_y - 16 - sy - 0x100;
				flipx = !flipx;
				flipy = !flipy;
			}

			color	=	( color >> (16-5) ) % total_color_codes;
			code	=	(code & 0x3fff) + (bank * 0x4000);

#define DRAWTILE(_x_,_y_)  \
			drawgfx(bitmap,Machine->gfx[0], \
					code, \
					color, \
					flipx, flipy, \
					_x_,_y_, \
					cliprect,TRANSPARENCY_PEN,0);

			DRAWTILE(sx - 0x000, sy + 0x000)
			DRAWTILE(sx - 0x200, sy + 0x000)
			DRAWTILE(sx - 0x000, sy + 0x100)
			DRAWTILE(sx - 0x200, sy + 0x100)

		}
	/* next column */
	}

}



static void seta_draw_sprites(struct mame_bitmap *bitmap,const struct rectangle *cliprect)
{
	int offs;
	int xoffs, yoffs;

	int total_color_codes	=	Machine->drv->gfxdecodeinfo[0].total_color_codes;

	int ctrl	=	spriteram16[ 0x600/2 ];
	int ctrl2	=	spriteram16[ 0x602/2 ];

	int flip	=	ctrl & 0x40;

	/* Sprites Banking and/or Sprites Buffering */
	data16_t *src = spriteram16_2 + ( ((ctrl2 ^ (~ctrl2<<1)) & 0x40) ? 0x2000/2 : 0 );

//	int max_y	=	Machine->visible_area.max_y+1;
	int max_y	=	Machine->drv->screen_height;



	seta_draw_sprites_map(bitmap,cliprect);



//	xoffs	=	flip ? 0x10 : 0x11;	// see downtown test mode: made of normal sprites
	xoffs	=	flip ? 0x10 : 0x10;	// see blandia test mode: made of normal sprites
	yoffs	=	flip ? 0x06 : 0x06;

	for ( offs = (0x400-2)/2 ; offs >= 0/2; offs -= 2/2 )
	{
		int	code	=	src[offs + 0x000/2];
		int	x		=	src[offs + 0x400/2];

		int	y		=	spriteram16[offs + 0x000/2] & 0xff;

		int	flipx	=	code & 0x8000;
		int	flipy	=	code & 0x4000;

		int bank	=	(x & 0x0600) >> 9;
		int color	=	( x >> (16-5) ) % total_color_codes;

		if (flip)
		{
//			y = max_y - y;
			y = max_y - y
				+(Machine->drv->screen_height-(Machine->visible_area.max_y + 1));
			flipx = !flipx;
			flipy = !flipy;
		}

		code = (code & 0x3fff) + (bank * 0x4000);

		drawgfx(bitmap,Machine->gfx[0],
				code,
				color,
				flipx, flipy,
				(x + xoffs) & 0x1ff,
				max_y - ((y + yoffs) & 0x0ff),
				cliprect,TRANSPARENCY_PEN,0);
	}

}





/***************************************************************************


								Screen Drawing


***************************************************************************/

/* For games without tilemaps */
VIDEO_UPDATE( seta_no_layers )
{
	fillbitmap(bitmap,Machine->pens[0],cliprect);
	seta_draw_sprites(bitmap,cliprect);
}



/* For games with 1 or 2 tilemaps */
VIDEO_UPDATE( seta )
{
	int layers_ctrl = -1;
	int enab_0, enab_1, x_0, x_1, y_0, y_1;

	int order	= 	0;
	int flip	=	(spriteram16[ 0x600/2 ] & 0x40) >> 6;

//	int scr_dimx = Machine->drv->screen_width;
	int scr_dimy = Machine->drv->screen_height;
//	int vis_dimx = Machine->visible_area.max_x - Machine->visible_area.min_x + 1;
	int vis_dimy = Machine->visible_area.max_y - Machine->visible_area.min_y + 1;

	flip ^= tilemaps_flip;

	tilemap_set_flip(ALL_TILEMAPS, flip ? (TILEMAP_FLIPX|TILEMAP_FLIPY) : 0 );

	x_0		=	seta_vctrl_0[ 0/2 ];
	y_0		=	seta_vctrl_0[ 2/2 ];
	enab_0	=	seta_vctrl_0[ 4/2 ];

	/* Only one tilemap per layer is enabled! */
	tilemap_set_enable(tilemap_0, (!(enab_0 & 0x0008)) /*&& (enab_0 & 0x0001)*/ );
	tilemap_set_enable(tilemap_1, ( (enab_0 & 0x0008)) /*&& (enab_0 & 0x0001)*/ );

	/* the hardware wants different scroll values when flipped */

	/*	bg x scroll	     flip
		metafox		0000 025d = 0, $400-$1a3 = $400 - $190 - $13
		eightfrc	ffe8 0272
					fff0 0260 = -$10, $400-$190 -$10
					ffe8 0272 = -$18, $400-$190 -$18 + $1a		*/

	if (flip)	{	x_0 = -402 - x_0;
					y_0 = y_0 - vis_dimy - (scr_dimy - vis_dimy); }

	tilemap_set_scrollx (tilemap_0, 0, x_0);
	tilemap_set_scrollx (tilemap_1, 0, x_0);
	tilemap_set_scrolly (tilemap_0, 0, y_0);
	tilemap_set_scrolly (tilemap_1, 0, y_0);

	if (tilemap_2)
	{
		x_1		=	seta_vctrl_2[ 0/2 ];
		y_1		=	seta_vctrl_2[ 2/2 ];
		enab_1	=	seta_vctrl_2[ 4/2 ];

		tilemap_set_enable(tilemap_2, (!(enab_1 & 0x0008)) /*&& (enab_1 & 0x0001)*/ );
		tilemap_set_enable(tilemap_3, ( (enab_1 & 0x0008)) /*&& (enab_1 & 0x0001)*/ );

		if (flip)	{	x_1 = -402 - x_1;
						y_1 = y_1 - vis_dimy - (scr_dimy - vis_dimy); }

		tilemap_set_scrollx (tilemap_2, 0, x_1);
		tilemap_set_scrollx (tilemap_3, 0, x_1);
		tilemap_set_scrolly (tilemap_2, 0, y_1);
		tilemap_set_scrolly (tilemap_3, 0, y_1);

		order	=	seta_vregs[ 2/2 ];
	}


#ifdef MAME_DEBUG
if (keyboard_pressed(KEYCODE_Z))
{	int msk = 0;
	if (keyboard_pressed(KEYCODE_Q))	msk |= 1;
	if (keyboard_pressed(KEYCODE_W))	msk |= 2;
	if (keyboard_pressed(KEYCODE_A))	msk |= 8;
	if (msk != 0) layers_ctrl &= msk;

	if (tilemap_2)		usrintf_showmessage("VR:%04X-%04X-%04X L0:%04X L1:%04X",seta_vregs[0],seta_vregs[1],seta_vregs[2],seta_vctrl_0[4/2],seta_vctrl_2[4/2]);
	else if (tilemap_0)	usrintf_showmessage("L0:%04X",seta_vctrl_0[4/2]);
}
#endif

	fillbitmap(bitmap,Machine->pens[0],cliprect);

	if (order & 1)	// swap the layers?
	{
		if (tilemap_2)
		{
			if (layers_ctrl & 2)	tilemap_draw(bitmap,cliprect, tilemap_2, TILEMAP_IGNORE_TRANSPARENCY, 0);
			if (layers_ctrl & 2)	tilemap_draw(bitmap,cliprect, tilemap_3, TILEMAP_IGNORE_TRANSPARENCY, 0);
		}

		if (order & 2)	// layer-sprite priority?
		{
			if (layers_ctrl & 8)	seta_draw_sprites(bitmap,cliprect);
			if (layers_ctrl & 1)	tilemap_draw(bitmap,cliprect, tilemap_0,  0, 0);
			if (layers_ctrl & 1)	tilemap_draw(bitmap,cliprect, tilemap_1,  0, 0);
		}
		else
		{
			if (layers_ctrl & 1)	tilemap_draw(bitmap,cliprect, tilemap_0,  0, 0);
			if (layers_ctrl & 1)	tilemap_draw(bitmap,cliprect, tilemap_1,  0, 0);
			if (layers_ctrl & 8)	seta_draw_sprites(bitmap,cliprect);
		}
	}
	else
	{
		if (layers_ctrl & 1)	tilemap_draw(bitmap,cliprect, tilemap_0,  TILEMAP_IGNORE_TRANSPARENCY, 0);
		if (layers_ctrl & 1)	tilemap_draw(bitmap,cliprect, tilemap_1,  TILEMAP_IGNORE_TRANSPARENCY, 0);

		if (order & 2)	// layer-sprite priority?
		{
			if (layers_ctrl & 8)	seta_draw_sprites(bitmap,cliprect);

			if (tilemap_2)
			{
				if (layers_ctrl & 2)	tilemap_draw(bitmap,cliprect, tilemap_2,  0, 0);
				if (layers_ctrl & 2)	tilemap_draw(bitmap,cliprect, tilemap_3,  0, 0);
			}
		}
		else
		{
			if (tilemap_2)
			{
				if (layers_ctrl & 2)	tilemap_draw(bitmap,cliprect, tilemap_2,  0, 0);
				if (layers_ctrl & 2)	tilemap_draw(bitmap,cliprect, tilemap_3,  0, 0);
			}

			if (layers_ctrl & 8)	seta_draw_sprites(bitmap,cliprect);
		}
	}

}
