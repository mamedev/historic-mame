/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "tilemap.h"

extern unsigned char *spriteram,*spriteram_2;
extern int spriteram_size;

unsigned char *timeplt_videoram,*timeplt_colorram;
static struct tilemap *bg_tilemap;
static int flipscreen;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Time Pilot has two 32x8 palette PROMs and two 256x4 lookup table PROMs
  (one for characters, one for sprites).
  The palette PROMs are connected to the RGB output this way:

  bit 7 -- 390 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 560 ohm resistor  -- BLUE
        -- 820 ohm resistor  -- BLUE
        -- 1.2kohm resistor  -- BLUE
        -- 390 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
  bit 0 -- 560 ohm resistor  -- GREEN

  bit 7 -- 820 ohm resistor  -- GREEN
        -- 1.2kohm resistor  -- GREEN
        -- 390 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
        -- 560 ohm resistor  -- RED
        -- 820 ohm resistor  -- RED
        -- 1.2kohm resistor  -- RED
  bit 0 -- not connected

***************************************************************************/
void timeplt_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3,bit4;


		bit0 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 4) & 0x01;
		bit4 = (color_prom[Machine->drv->total_colors] >> 5) & 0x01;
		*(palette++) = 0x19 * bit0 + 0x24 * bit1 + 0x35 * bit2 + 0x40 * bit3 + 0x4d * bit4;
		bit0 = (color_prom[Machine->drv->total_colors] >> 6) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 7) & 0x01;
		bit2 = (color_prom[0] >> 0) & 0x01;
		bit3 = (color_prom[0] >> 1) & 0x01;
		bit4 = (color_prom[0] >> 2) & 0x01;
		*(palette++) = 0x19 * bit0 + 0x24 * bit1 + 0x35 * bit2 + 0x40 * bit3 + 0x4d * bit4;
		bit0 = (color_prom[0] >> 3) & 0x01;
		bit1 = (color_prom[0] >> 4) & 0x01;
		bit2 = (color_prom[0] >> 5) & 0x01;
		bit3 = (color_prom[0] >> 6) & 0x01;
		bit4 = (color_prom[0] >> 7) & 0x01;
		*(palette++) = 0x19 * bit0 + 0x24 * bit1 + 0x35 * bit2 + 0x40 * bit3 + 0x4d * bit4;

		color_prom++;
	}

	color_prom += Machine->drv->total_colors;
	/* color_prom now points to the beginning of the lookup table */


	/* sprites */
	for (i = 0;i < TOTAL_COLORS(1);i++)
		COLOR(1,i) = *(color_prom++) & 0x0f;

	/* characters */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = (*(color_prom++) & 0x0f) + 0x10;
}



/***************************************************************************

  Callbacks for the TileMap code

***************************************************************************/

static void get_bg_tile_info(int col,int row)
{
	int tile_index = 32*row+col;
	unsigned char attr = timeplt_colorram[tile_index];
	SET_TILE_INFO(0,timeplt_videoram[tile_index] + ((attr & 0x20) << 3),attr & 0x1f)
	tile_info.flags = TILE_FLIPYX((attr & 0xc0) >> 6);
	tile_info.priority = (attr & 0x10) >> 4;
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

void timeplt_vh_stop(void)
{
	tilemap_dispose(bg_tilemap);
	tilemap_stop();
}

int timeplt_vh_start(void)
{
	if (tilemap_start() == 0)
	{
		bg_tilemap = tilemap_create(TILEMAP_SPLIT,8,8,32,32,0,0);

		if (bg_tilemap)
		{
			bg_tilemap->tile_get_info = get_bg_tile_info;

			return 0;
		}

		timeplt_vh_stop();
	}

	return 1;
}



/***************************************************************************

  Memory handlers

***************************************************************************/

void timeplt_videoram_w(int offset,int data)
{
	if (timeplt_videoram[offset] != data)
	{
		timeplt_videoram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset%32,offset/32);
	}
}

void timeplt_colorram_w(int offset,int data)
{
	if (timeplt_colorram[offset] != data)
	{
		timeplt_colorram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset%32,offset/32);
	}
}

void timeplt_flipscreen_w(int offset,int data)
{
	int attributes;

	flipscreen = data & 1;
	attributes = flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0;
	tilemap_set_attributes(bg_tilemap,attributes);
}

/* Return the current video scan line */
int timeplt_scanline_r(int offset)
{
	return cpu_scalebyfcount(256);
}



/***************************************************************************

  Display refresh

***************************************************************************/

static void draw_sprites(struct osd_bitmap *bitmap)
{
	const struct GfxElement *gfx = Machine->gfx[1];
	const struct rectangle *clip = &Machine->drv->visible_area;
	int offs;


	for (offs = spriteram_size - 2;offs >= 0;offs -= 2)
	{
		int code,color,sx,sy,flipx,flipy;

		code = spriteram[offs + 1];
		color = spriteram_2[offs] & 0x3f;
		sx = 240 - spriteram[offs];
		sy = spriteram_2[offs + 1]-1;
		flipx = spriteram_2[offs] & 0x40;
		flipy = !(spriteram_2[offs] & 0x80);

		drawgfx(bitmap,gfx,
				code,
				color,
				flipx,flipy,
				sx,sy,
				clip,TRANSPARENCY_PEN,0);

		if (sy < 240)
		{
			/* clouds are drawn twice, offset by 128 pixels horizontally and vertically */
			/* this is done by the program, multiplexing the sprites; we don't emulate */
			/* that, we just reproduce the behaviour. */
			if (offs <= 2*2 || offs >= 19*2)
			{
				drawgfx(bitmap,gfx,
						code,
						color,
						flipx,flipy,
						(sx + 128) & 0xff,(sy + 128) & 0xff,
						clip,TRANSPARENCY_PEN,0);
			}
		}
	}
}

void timeplt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	tilemap_update(bg_tilemap);

	tilemap_render(bg_tilemap);

	tilemap_draw(bitmap,bg_tilemap,0);
	draw_sprites(bitmap);
	tilemap_draw(bitmap,bg_tilemap,1);
}
