#include "driver.h"
#include "vidhrdw/generic.h"

extern data8_t *battlex_vidram, *battlex_sprram;
static struct tilemap *battlex_tilemap;




static int battlex_scroll_lsb;
static int battlex_scroll_msb;


PALETTE_INIT( battlex )
{
	int i,col;

	for (col = 0;col < 8;col++)
	{
		for (i = 0;i < 16;i++)
		{
			int data = i | col;
			int g = ((data & 1) >> 0) * 0xff;
			int b = ((data & 2) >> 1) * 0xff;
			int r = ((data & 4) >> 2) * 0xff;

#if 0
			/* from Tim's shots, bit 3 seems to have no effect (see e.g. Laser Ship on title screen) */
			if (i & 8)
			{
				r /= 2;
				g /= 2;
				b /= 2;
			}
#endif

			palette_set_color(i + 16 * col,r,g,b);
		}
	}
}

WRITE_HANDLER( battlex_palette_w )
{
	int g = ((data & 1) >> 0) * 0xff;
	int b = ((data & 2) >> 1) * 0xff;
	int r = ((data & 4) >> 2) * 0xff;

	palette_set_color(16*8 + offset,r,g,b);
}



static void get_battlex_tile_info(int tile_index)
{
	int tile = (battlex_vidram[tile_index*2] | (((battlex_vidram[tile_index*2+1] & 0x01)) << 8));
	int color = (battlex_vidram[tile_index*2+1] & 0x0e) >> 1;

	SET_TILE_INFO(0,tile,color,0)
}


VIDEO_START( battlex )
{
	battlex_tilemap = tilemap_create(get_battlex_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,64,32);
	tilemap_set_transparent_pen(battlex_tilemap,0);

	return 0;
}

WRITE_HANDLER( battlex_scroll_x_lsb_w )
{
	battlex_scroll_lsb = data;
}

WRITE_HANDLER( battlex_scroll_x_msb_w )
{
	battlex_scroll_msb = data;
}

WRITE_HANDLER( battlex_vidram_w )
{
	battlex_vidram[offset] = data;
	tilemap_mark_tile_dirty(battlex_tilemap, offset/2);
}


static void battlex_drawsprites( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	const struct GfxElement *gfx = Machine->gfx[1];
	data8_t *source = battlex_sprram;
	data8_t *finish = battlex_sprram + 0x200;

	while( source<finish )
	{
		int xpos = (source[0] & 0x7f) * 2 - (source[0] & 0x80) * 2;
		int ypos = source[3];
		int tile = source[2] & 0x7f;
		int color = source[1] & 0x07;	/* bits 3,4,5 also used during explosions */
		int flipy = source[1] & 0x80;
		int flipx = source[1] & 0x40;

		drawgfx(bitmap,gfx,tile,color,flipx,flipy,xpos,ypos,cliprect,TRANSPARENCY_PEN,0);

		source += 4;
	}

}

VIDEO_UPDATE(battlex)
{
	fillbitmap(bitmap,get_black_pen(),cliprect);

	tilemap_set_scrollx(battlex_tilemap, 0, battlex_scroll_lsb | (battlex_scroll_msb << 8) );
	tilemap_draw(bitmap,cliprect,battlex_tilemap,0,0);

	battlex_drawsprites(bitmap,cliprect);
}
