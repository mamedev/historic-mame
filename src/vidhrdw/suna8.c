/***************************************************************************

							-=  SunA 8 Bit Games =-

					driver by	Luca Elia (eliavit@unina.it)

	These games have only sprites, of a peculiar type:

	there is a region of memory where 4 pages of 32x32 tile codes can
	be written like a tilemap made of 4 pages of 256x256 pixels. Each
	tile uses 2 bytes.

	Sprites are rectangular regions of *tiles* fetched from there and
	sent to the screen. Each sprite uses 4 bytes, held within the last
	page of tiles.


								[ Sprites Format ]


	Offset:			Bits:				Value:

		0.b								Y (Bottom up)

		1.b			7--- ----			Sprite Size (1 = 2x32 tiles; 0 = 2x2)

					2x32 Sprites:
					-65- ----			Tiles Row (height = 8 tiles)
					---4 ----			Page

					2x2 Sprites:
					-6-- ----			Ignore X (Multisprite)
					--54 ----			Page

					---- 3210			Tiles Column (width = 2 tiles)

		2.b								X

		3.b			7--- ----
					-6-- ----			X (Sign Bit)
					--54 3---
					---- -210			Tiles Bank


							[ Sprite's Tiles Format ]


	Offset: 		Bits:					Value:

		0.b								Code (Low Bits)

		1.b			7--- ----			Flip Y
					-6-- ----			Flip X
					--54 32--			Color
					---- --10			Code (High Bits)


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

int suna8_gfx_type; /* for text layer */

/* Functions defined in vidhrdw: */

WRITE_HANDLER( hardhead_spriteram_w );	// for debug

int  suna8_vh_start(void);
void suna8_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


/***************************************************************************
	For Debug: there's no tilemap, just sprites.
***************************************************************************/

static struct tilemap *tilemap;
static int page;
static int tiles;

#define NX  (0x20*4)
#define NY  (0x20)

static void get_tile_info(int tile_index)
{
	data8_t code = spriteram[ 2 * tile_index + 0 ];
	data8_t attr = spriteram[ 2 * tile_index + 1 ];
	SET_TILE_INFO(0, ( (attr & 0x03) << 8 ) + code + (tiles & 7)*0x400,(attr >> 2) & 0xf);
	tile_info.flags = TILE_FLIPYX( (attr >> 6) & 3 );
}

WRITE_HANDLER( hardhead_spriteram_w )
{
	if (spriteram[offset] != data)
	{
		spriteram[offset] = data;
		tilemap_mark_tile_dirty(tilemap,offset/2);
	}
}

int suna8_vh_start(void)
{
	tilemap = tilemap_create(	get_tile_info, tilemap_scan_cols,
								TILEMAP_TRANSPARENT,
								8,8,NX,NY);
	if ( tilemap != NULL )
	{
		tilemap_set_transparent_pen(tilemap,15);
		return 0;
	}
	else	return 1;
}

/***************************************************************************


								Sprites Drawing


***************************************************************************/

void suna8_draw_sprites(struct osd_bitmap *bitmap)
{
	int i;

	int px = 0;

	for (i = 0x1d00; i < 0x2000; i += 4)
	{
		int bank;

		int srcpg, srcx,srcy, dimx,dimy;
		int tile_x, tile_xinc, tile_xstart;
		int tile_y, tile_yinc;
		int dx, dy;
		int flipx, y0;

		int y		=	spriteram[i + 0];
		int code	=	spriteram[i + 1];
		int x		=	spriteram[i + 2];
		int dim		=	spriteram[i + 3];

		bank	=	(dim & 7) * 0x400;

		if (code & 0x80)
		{
			dimx = 2;					dimy =	32;
			srcx  = (code & 0xf) * 2;	srcy = 0;
			srcpg = (code >> 4) & 3;
			y0 = 0x100;
		}
		else
		{
			dimx = 2;					dimy =	2;
			srcx  = (code & 0xf) * 2;	srcy = ((code >> 5) & 0x3) * 8 + 6;
			srcpg = (code >> 4) & 1;
			y0 = 0x100;
		}

		x = x - ((dim & 0x40) ? 0x100 : 0);
		y = (y0 - y - dimy*8 ) & 0xff;

		if ( (dimy==32) && (code & 0x40) )
		{
			px += 16;
			x = px;
		}
		else
			px = x;

		flipx = 0;

		if (flipx)	{ tile_xstart = dimx-1; tile_xinc = -1; }
		else		{ tile_xstart = 0;		tile_xinc = +1; }

		tile_y = 0; 	tile_yinc = +1;

		for (dy = 0; dy < dimy * 8; dy += 8)
		{
			tile_x = tile_xstart;

			for (dx = 0; dx < dimx * 8; dx += 8)
			{
				int addr	=	(srcpg * 0x20 * 0x20) +
								((srcx + tile_x) & 0x1f) * 0x20 +
								((srcy + tile_y) & 0x1f);

				int tile	=	spriteram[addr*2 + 0];
				int attr	=	spriteram[addr*2 + 1];

				int tile_flipx	=	attr & 0x40;
				int tile_flipy	=	attr & 0x80;

				int sx		=	x + dx;
				int sy		=	(y + dy) & 0xff;

				if (flipx)	tile_flipx = !tile_flipx;

				drawgfx(	bitmap,Machine->gfx[0],
							tile + (attr & 0x3)*0x100 + bank,
							(attr >> 2) & 0xf,
							tile_flipx, tile_flipy,
							sx, sy,
							&Machine->visible_area,TRANSPARENCY_PEN,15);

				tile_x += tile_xinc;
			}

			tile_y += tile_yinc;
		}

	}
}


/***************************************************************************


								Screen Drawing


***************************************************************************/

static void draw_strip( struct osd_bitmap *bitmap, const UINT8 *source, int sx, int sy, int num_rows )
{
	int attr,tile;
	int r,c;
	for( r=0; r<num_rows; r++ )
	{
		for( c=0; c<8; c++ )
		{
			tile = source[c*64];
			attr = source[c*64+1];
			/*
				xx------	flip
				--xxxx--	color
				------xx	bank
			*/
			drawgfx( bitmap,
				Machine->gfx[0],
				tile + 256*(attr&3),
				(attr>>2)&0xf, /* color */
				attr&0x40,attr&0x80,
				sx+c*8,sy+r*8,
				&Machine->visible_area,
				TRANSPARENCY_PEN,0xf );
		}
		source+=2;
	}
}

static void draw_text( struct osd_bitmap *bitmap )
{
	if( suna8_gfx_type == 1 )
	{
		/* hard head */
		draw_strip( bitmap, &spriteram[0x1a00], 0*8, 2*8, 4 );
		draw_strip( bitmap, &spriteram[0x1a08], 12*8, 2*8, 4 );
		draw_strip( bitmap, &spriteram[0x1a10], 24*8, 26*8, 4 );
		draw_strip( bitmap, &spriteram[0x1a20], 24*8, 2*8, 4 );
	}
	else
	{
		/* rough ranger, super ranger */
		draw_strip( bitmap, &spriteram[0x1a00], 24*8, 28*8, 2 );
		draw_strip( bitmap, &spriteram[0x1a04], 0*8, 2*8, 2 );
		draw_strip( bitmap, &spriteram[0x1a08], 12*8, 2*8, 2 );
		draw_strip( bitmap, &spriteram[0x1a0c], 24*8, 2*8, 2 );
		draw_strip( bitmap, &spriteram[0x1a38], 0*8, 28*8, 2 ); // ?
		draw_strip( bitmap, &spriteram[0x1a3c], 12*8, 28*8, 2 ); // ?
	}
}

void suna8_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{

/*
Uncomment to debug. Press Z (you see the "tilemaps", use R-T to cycle
through the 4 pages. Use Y to cycle through the tiles banks.
*/
#if 0
{
	char buf[80];

	if (keyboard_pressed_memory(KEYCODE_R))
	{
		page--;
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
	if (keyboard_pressed_memory(KEYCODE_T))
	{
		page++;
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}
	if (keyboard_pressed_memory(KEYCODE_Y))
	{
		tiles++;
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}

	page  &= 3;
	tiles &= 3;

	sprintf(buf,"p: %d - g: %d",page,tiles);
	usrintf_showmessage(buf);
}
#endif

	tilemap_set_scrollx( tilemap, 0, 0x100 * page);
	tilemap_set_scrolly( tilemap, 0, 0);

	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	memset(palette_used_colors, PALETTE_COLOR_USED, Machine->drv->total_colors);

	palette_recalc();

	fillbitmap(bitmap,palette_transparent_pen,&Machine->visible_area);

#if 0
	if (keyboard_pressed(KEYCODE_Z))	tilemap_draw(bitmap, tilemap, 0, 0);
	else
#endif
	suna8_draw_sprites(bitmap);

	draw_text( bitmap );
}
