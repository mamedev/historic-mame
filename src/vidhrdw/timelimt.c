#include "vidhrdw/generic.h"

/* globals */
data8_t *timelimt_bg_videoram;
size_t timelimt_bg_videoram_size;

/* locals */
static int scrollx, scrolly;

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Time Limit has two 32 bytes palette PROM, connected to the RGB output this
  way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/

void timelimt_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom) {
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2;

		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = (*color_prom >> 6) & 0x01;
		bit1 = (*color_prom >> 7) & 0x01;
		*(palette++) = 0x4f * bit0 + 0xa8 * bit1;

		color_prom++;
	}
}

/***************************************************************************

	Start the video hardware emulation.

***************************************************************************/


int timelimt_vh_start( void )
{
	dirtybuffer = 0;
	tmpbitmap = 0;

	if ( ( dirtybuffer = malloc( timelimt_bg_videoram_size ) ) == 0 )
		return 1;

	memset( dirtybuffer, 1, timelimt_bg_videoram_size );

	if ( ( tmpbitmap = bitmap_alloc( 64*8, 32*8 ) ) == 0 )
	{
		free( dirtybuffer );
		return 1;
	}

	return 0;
}

/***************************************************************************/

WRITE_HANDLER( timelimt_videoram_w )
{
	if (videoram[offset] != data)
	{
		videoram[offset] = data;
	}
}

WRITE_HANDLER( timelimt_bg_videoram_w )
{
	if (timelimt_bg_videoram[offset] != data)
	{
		timelimt_bg_videoram[offset] = data;
		dirtybuffer[offset] = 1;
	}
}

WRITE_HANDLER( timelimt_scroll_x_lsb_w )
{
	scrollx &= 0x100;
	scrollx |= data & 0xff;
}

WRITE_HANDLER( timelimt_scroll_x_msb_w )
{
	scrollx &= 0xff;
	scrollx |= ( data & 1 ) << 8;
}

WRITE_HANDLER( timelimt_scroll_y_w )
{
	scrolly = data;
}

/***************************************************************************

	Draw the sprites

***************************************************************************/
static void drawsprites( struct mame_bitmap *bitmap )
{
	int offs;

	for( offs = spriteram_size; offs >= 0; offs -= 4 )
	{
		int sy = 240 - spriteram[offs];
		int sx = spriteram[offs+3];
		int code = spriteram[offs+1] & 0x3f;
		int attr = spriteram[offs+2];
		int flipy = spriteram[offs+1] & 0x80;
		int flipx = spriteram[offs+1] & 0x40;

		code += ( attr & 0x80 ) ? 0x40 : 0x00;
		code += ( attr & 0x40 ) ? 0x80 : 0x00;

		drawgfx( bitmap, Machine->gfx[2],
				code,
				attr & 7,
				flipx,flipy,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}

/***************************************************************************

	Draw the background layer

***************************************************************************/

static void draw_background( struct mame_bitmap *bitmap )
{
	int offs;

	for ( offs = 0; offs < timelimt_bg_videoram_size; offs++ )
	{
		if ( dirtybuffer[offs] )
		{
			int sx, sy, code;

			sx = offs % 64;
			sy = offs / 64;
			code = timelimt_bg_videoram[offs];

			dirtybuffer[offs] = 0;

			drawgfx( tmpbitmap, Machine->gfx[1],
					code,
					0,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	{
		int tx = -scrollx;
		int ty = -scrolly;

		copyscrollbitmap( bitmap, tmpbitmap, 1, &tx, 1, &ty, &Machine->visible_area, TRANSPARENCY_NONE, 0 );
	}
}

/***************************************************************************

	Draw the foreground layer

***************************************************************************/

static void draw_foreground( struct mame_bitmap *bitmap )
{
	int offs;

	for ( offs = 0; offs < videoram_size; offs++ )
	{
		int sx, sy, code;

		sx = offs % 32;
		sy = offs / 32;

		code = videoram[offs];

		drawgfx( bitmap, Machine->gfx[0],
				 code,
				 0,
				 0,0,
				 8*sx,8*sy,
				 0,TRANSPARENCY_PEN,0);
	}
}

/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void timelimt_vh_screenrefresh( struct mame_bitmap *bitmap, int full_refresh )
{
	if ( full_refresh )
	{
		memset( dirtybuffer, 1, timelimt_bg_videoram_size );
	}

	draw_background( bitmap );
	drawsprites( bitmap );
	draw_foreground(  bitmap );
}
