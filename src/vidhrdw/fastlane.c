#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *fastlane_k007121_regs;
static struct tilemap *layer0, *layer1;

/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_tile_info0(int col,int row)
{
	int tile_index = 32*row+col;
	int color = colorram[tile_index];
	int gfxbank = ((color & 0x80) >> 7) | ((color & 0x70) >> 3)
					| ((fastlane_k007121_regs[0x03] & 0x01) << 5);
	int code = videoram[tile_index];

	code += 256*gfxbank;

	SET_TILE_INFO(0,code,1);
}

static void get_tile_info1(int col,int row)
{
	int tile_index = 32*row+col;
	int color = colorram[tile_index + 0x400];
	int gfxbank = ((color & 0x80) >> 7) | ((color & 0x70) >> 3)
					| ((fastlane_k007121_regs[0x03] & 0x01) << 5);
	int code = videoram[tile_index + 0x400];

	code += 256*gfxbank;

	SET_TILE_INFO(0,code,0);
}

/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

int fastlane_vh_start(void)
{
	layer0 = tilemap_create(get_tile_info0, TILEMAP_TRANSPARENT, 8, 8, 32, 32);
	layer1 = tilemap_create(get_tile_info1, TILEMAP_TRANSPARENT, 8, 8, 32, 32);

	tilemap_set_scroll_rows( layer0, 32 );

	if( layer0 && layer1 )
	{
		layer0->transparent_pen = 0;
		layer1->transparent_pen = 0;

		return 0;
	}
	return 1;
}

/***************************************************************************

  Memory Handlers

***************************************************************************/

void fastlane_vram0_w(int offset,int data)
{
	if (videoram[offset] != data)
	{
		tilemap_mark_tile_dirty(layer0, offset%32, offset/32);
		videoram[offset] = data;
	}
}

void fastlane_vram1_w(int offset,int data)
{
	if (videoram[offset + 0x400] != data)
	{
		tilemap_mark_tile_dirty(layer1, offset%32, offset/32);
		videoram[offset + 0x400] = data;
	}
}

void fastlane_cram0_w(int offset,int data)
{
	if (colorram[offset] != data)
	{
		tilemap_mark_tile_dirty(layer0, offset%32, offset/32);
		colorram[offset] = data;
	}
}

void fastlane_cram1_w(int offset,int data)
{
	if (colorram[offset + 0x400] != data)
	{
		tilemap_mark_tile_dirty(layer1, offset%32, offset/32);
		colorram[offset + 0x400] = data;
	}
}

/***************************************************************************

	Fast Lane sprites. Each sprite has 5 bytes:

	0:	0xff	- Sprite number
	1:	0xf0	- Color
		0x08	- Fine control for 16x8/8x8 sprites (*may* have other use for 16x16 sprites)
		0x04	- Fine control for 16x8/8x8 sprites
		0x02	- Tile bank bit 1
		0x01	- Tile bank bit 0
	2:	0xff	- Y position
	3:	0xff	- X position
	4:	0x80	- Tile bank bit 3
		0x40	- Tile bank bit 2
		0x20	- Y flip
		0x10	- X flip
		0x08	- Size
		0x04	- Size
		0x02 	- Size
		0x01	- X position high bit

	Known sizes:
		0x08	- 32 x 32 sprite (Combination of 4 16x16's)
		0x06	- 8 x 8 sprite
		0x04 	- 8 x 16 sprite
		0x02 	- 16 x 8 sprite
		0x00 	- 16 x 16 sprite

If using 16x8 sprites then the tile number given needs to be multipled by 2!
If using 8x8 sprites then the tile number given needs to be multipled by 4!

The 'fine control' bits are then added to get the correct sprite within the group
of two/four.

See also Combat School, Haunted Castle, Contra.

***************************************************************************/

static void draw_sprites(struct osd_bitmap *bitmap, const unsigned char *source )
{
	const struct GfxElement *gfx = Machine->gfx[0];
	const unsigned char *finish;

	finish = source + 0x200;

	while( source < finish )
	{
		int number = source[0];				/* sprite number */
		int sprite_bank = source[1] & 0x0f;	/* sprite bank */
		int sx = source[3];					/* vertical position */
		int sy = source[2];					/* horizontal position */
		int attr = source[4];				/* attributes */
		int xflip = source[4] & 0x10;		/* flip x */
		int yflip = source[4] & 0x20;		/* flip y */
		int color = source[1] & 0xf0;		/* color */
		int width,height;

		if( attr&0x01 ) sx -= 256;

		number += ((sprite_bank&0x3)<<8) + ((attr&0xc0)<<4);
		number = number<<2;
		number += (sprite_bank>>2)&3;
		color = (color >> 4);

		switch( attr&0xe )
		{
			case 0x06: width = height = 1; break;
			case 0x04: width = 1; height = 2; number &= (~2); break;
			case 0x02: width = 2; height = 1; number &= (~1); break;
			case 0x00: width = height = 2; number &= (~3); break;
			case 0x08: width = height = 4; number &= (~3); break;
			default: width = 1; height = 1;
		}

		{
			static int x_offset[8] = {0x0,0x1,0x4,0x5,  0x0,0x1,0x4,0x5};
			static int y_offset[8] = {0x0,0x2,0x8,0xa,  0x0,0x1,0x4,0x5};
			int x,y, ex, ey;

			for( y=0; y<height; y++ ){
				for( x=0; x<width; x++ ){
					ex = xflip?(width-1-x):x;
					ey = yflip?(height-1-y):y;

					drawgfx(bitmap,gfx,
						number+x_offset[ex]+y_offset[ey],
						color,
						xflip,yflip,
						sx+x*8+16,sy+y*8,
						&Machine->drv->visible_area,
						TRANSPARENCY_PEN,0);
				}
			}
		}
		source += 5;
	}
}

/***************************************************************************

  Screen Refresh

***************************************************************************/

void fastlane_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i, xoffs;

	/* set scroll registers */
	xoffs = fastlane_k007121_regs[0] - 0xf;
	for( i=0; i<32; i++ ){
		tilemap_set_scrollx(layer0, i, fastlane_k007121_regs[0x20 + i] + xoffs);
	}
	tilemap_set_scrolly( layer0, 0, fastlane_k007121_regs[2] );

	tilemap_update( ALL_TILEMAPS );
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);
	tilemap_render( ALL_TILEMAPS );

	tilemap_draw( bitmap, layer0, TILEMAP_IGNORE_TRANSPARENCY );
	draw_sprites( bitmap, spriteram );
	tilemap_draw( bitmap, layer1, 0 );
}
