#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *layer0, *layer1;
static int spritebank;


/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_tile_info_fg(int col,int row)
{
	int tile_index = 32*row+col;
	int color = colorram[tile_index];
	int code = videoram[tile_index] | ((color & 0x0f) << 9) | ((color & 0x40) << 2);

	tile_info.flags = TILE_FLIPYX((color & 0x30) >> 4);
	SET_TILE_INFO(0,code,0);
}

static void get_tile_info_bg(int col,int row)
{
	int tile_index = 32*row+col;
	int color = colorram[tile_index + 0x400];
	int code = videoram[tile_index + 0x400] | ((color & 0x0f) << 9) | ((color & 0x40) << 2);

	tile_info.flags = TILE_FLIPYX((color & 0x30) >> 4);
	SET_TILE_INFO(0,code,0);
}



/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/

int battlnts_vh_start(void)
{
	layer0 = tilemap_create(get_tile_info_fg, TILEMAP_TRANSPARENT, 8, 8, 32, 32);
	layer1 = tilemap_create(get_tile_info_bg, TILEMAP_OPAQUE, 8, 8, 32, 32);

	if( layer0 && layer1 )
	{
		layer0->transparent_pen = 0;

		return 0;
	}
	return 1;
}



/***************************************************************************

  Memory Handlers

***************************************************************************/

void battlnts_fg_vram_w(int offset,int data)
{
	if (videoram[offset] != data)
	{
		tilemap_mark_tile_dirty(layer0, offset%32, offset/32);
		videoram[offset] = data;
	}
}

void battlnts_bg_vram_w(int offset,int data)
{
	if (videoram[offset + 0x400] != data)
	{
		tilemap_mark_tile_dirty(layer1, offset%32, offset/32);
		videoram[offset + 0x400] = data;
	}
}

void battlnts_fg_cram_w(int offset,int data)
{
	if (colorram[offset] != data)
	{
		tilemap_mark_tile_dirty(layer0, offset%32, offset/32);
		colorram[offset] = data;
	}
}

void battlnts_bg_cram_w(int offset,int data)
{
	if (colorram[offset + 0x400] != data)
	{
		tilemap_mark_tile_dirty(layer1, offset%32, offset/32);
		colorram[offset + 0x400] = data;
	}
}

void battlnts_spritebank_w(int offset,int data)
{
	spritebank = 1024 * (data & 1);
}



/***************************************************************************

  Screen Refresh

***************************************************************************/

static void draw_sprites(struct osd_bitmap *bitmap)
{
	int offs;

	for (offs = spriteram_size - 8;offs >= 0;offs -= 8)
	{
		int ox,oy,code,flipx,flipy,zoom,w,h,x,y;
		static int xoffset[2] = { 0, 1 };
		static int yoffset[2] = { 0, 2 };

		code = spriteram[offs+1] + ((spriteram[offs+2] & 0xc0) << 2) + spritebank;
		ox = spriteram[offs+3];
		oy = 256 - spriteram[offs+0];
		flipx = spriteram[offs+4] & 0x04;
		flipy = spriteram[offs+4] & 0x08;

		/* 0x080 = normal scale, 0x040 = double size, 0x100 half size */
		zoom = spriteram[offs+5] + ((spriteram[offs+4] & 0x03) << 8);
		if (!zoom) continue;
		zoom = 0x10000 * 128 / zoom;

		w = h = 1;
		if (spriteram[offs+4] & 0x40)	/* 32x32 */
			w = h = 2;

		if (zoom == 0x10000)
		{
			int sx,sy;

			for (y = 0;y < h;y++)
			{
				sy = oy + 16 * y;

				for (x = 0;x < w;x++)
				{
					int c = code;

					sx = ox + 16 * x;
					if (flipx) c += xoffset[(w-1-x)];
					else c += xoffset[x];
					if (flipy) c += yoffset[(h-1-y)];
					else c += yoffset[y];

					drawgfx(bitmap,Machine->gfx[1],
							c,
							0,
							flipx,flipy,
							sx,sy,
							&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
				}
			}
		}
		else
		{
			int sx,sy,zw,zh;
			for (y = 0;y < h;y++)
			{
				sy = oy + ((zoom * y + (1<<11)) >> 12);
				zh = (oy + ((zoom * (y+1) + (1<<11)) >> 12)) - sy;

				for (x = 0;x < w;x++)
				{
					int c = code;

					sx = ox + ((zoom * x + (1<<11)) >> 12);
					zw = (ox + ((zoom * (x+1) + (1<<11)) >> 12)) - sx;
					if (flipx) c += xoffset[(w-1-x)];
					else c += xoffset[x];
					if (flipy) c += yoffset[(h-1-y)];
					else c += yoffset[y];

					drawgfxzoom(bitmap,Machine->gfx[1],
							c,
							0,
							flipx,flipy,
							sx,sy,
							&Machine->drv->visible_area,TRANSPARENCY_PEN,0,
							(zw << 16) / 16,(zh << 16) / 16);
				}
			}
		}
	}
}


void battlnts_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
//	char baf[240];
//	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	tilemap_update( ALL_TILEMAPS );
	palette_recalc();
	tilemap_render( ALL_TILEMAPS );

	tilemap_draw( bitmap, layer1, 0 );
	tilemap_draw( bitmap, layer0, 0 );

	draw_sprites(bitmap);

//	sprintf(baf, "%02x %02x %02x %02x %02x",
//			RAM[0x2600], RAM[0x2601], RAM[0x2602], RAM[0x2603], RAM[0x2604]);
//	usrintf_showmessage(baf);
}
