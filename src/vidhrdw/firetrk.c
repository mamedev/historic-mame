/***************************************************************************

	Atari Fire Truck hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "firetrk.h"

static struct mame_bitmap *buf;

VIDEO_START( firetrk )
{
	buf = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height);
	if (!buf)
		return 1;
	
	return 0;
}

static void draw_sprites( struct mame_bitmap *bitmap )
{
	int color		= firetrk_invert_display?3:7; /* invert display */
	int track_color = Machine->pens[firetrk_invert_display?0:3];
	int bgcolor		= firetrk_invert_display?4:0;
	int cabrot		= videoram[0x1080];
	int hpos		= videoram[0x1460];
	int vpos		= videoram[0x1480];
	int tailrot		= videoram[0x14a0];
	int pen;
	int sx, sy;
	int x,y;
	int flipx,flipy;
	int pitch;

	pitch = (bitmap->depth==8)?bitmap->width:(bitmap->width*2);

	/* This isn't the most efficient way to handle collision detection, but it works:
	 *
	 *	1. draw background
	 *	2. copy background to screen-sized private buffer
	 *	3. draw sprites using background color
	 *	4. compare buffer and background; if they differ, a collision occured
	 *	5. use the color of the background pixel where a collision occured to
	 *		flag collision type appropriately.
	 *	6. draw sprites normally
	 *
	 */
	for( sy=0; sy<bitmap->height; sy++ )
	{
		memcpy( buf->line[sy], bitmap->line[sy], pitch );
	}

	flipx = tailrot&0x08;
	flipy = tailrot&0x10;
	sx = flipx?(hpos-47):(255-hpos-47);
	sy = flipy?(vpos-47):(255-vpos-47);

	drawgfx( bitmap,
		Machine->gfx[2],
		tailrot&0x07, /* code */
		bgcolor,
		flipx,flipy,
		sx,sy,
		&Machine->visible_area,
		TRANSPARENCY_PEN,0 );

	drawgfx( bitmap,
		Machine->gfx[(cabrot&0x10)?3:4],
		cabrot&0x03, /* code */
		bgcolor,
		cabrot&0x04, /* flipx */
		cabrot&0x08, /* flipy */
		124,120,
		&Machine->visible_area,
		TRANSPARENCY_PEN,0 );

	if( bitmap->depth==8 )
	{
		for( y=0; y<bitmap->height; y++ )
		{
			for( x=0; x<bitmap->width; x++ )
			{
				pen = ((UINT8 *)buf->line[y])[x];
				if( pen != ((UINT8 *)bitmap->line[y])[x] )
				{
					if( pen == track_color )
					{
						firetrk_bit7_flags |= 0x40; /* crash */
					}
					else
					{
						firetrk_bit0_flags |= 0x40; /* skid */
					}
				}
			}
		}
	}
	else
	{
		for( y=0; y<bitmap->height; y++ )
		{
			for( x=0; x<bitmap->width; x++ )
			{
				pen = ((UINT16 *)buf->line[y])[x];
				if( pen != ((UINT16 *)bitmap->line[y])[x] )
				{
					if( pen == track_color )
					{
						firetrk_bit7_flags |= 0x40; /* crash */
					}
					else
					{
						firetrk_bit0_flags |= 0x40; /* skid */
					}
				}
			}
		}
	}

	drawgfx( bitmap,
		Machine->gfx[2],
		tailrot&0x07, /* code */
		color,
		flipx,flipy,
		sx,sy,
		&Machine->visible_area,
		TRANSPARENCY_PEN,0 );

	drawgfx( bitmap,
		Machine->gfx[(cabrot&0x10)?3:4],
		cabrot&0x03, /* code */
		color,
		cabrot&0x04, /* flipx */
		cabrot&0x08, /* flipy */
		124,120,
		&Machine->visible_area,
		TRANSPARENCY_PEN,0 );
}

static void draw_text( struct mame_bitmap *bitmap )
{
	int color = firetrk_invert_display?3:7; /* invert display */
	int x,y,tile_number;
	const UINT8 *source = videoram;

	for( x=0; x<=256-16; x+=256-16 )
	{
		for( y=0; y<256; y+=16 )
		{
			tile_number = *source++;

			drawgfx( bitmap,
				Machine->gfx[0],
				tile_number,
				color,
				0,0, /* flip */
				x,y,
				&Machine->visible_area,
				TRANSPARENCY_NONE,0 );
		}
	}
}

static void draw_background( struct mame_bitmap *bitmap )
{
	int color = firetrk_invert_display?4:0; /* invert display */
	int pvpload = videoram[0x1000];
	int phpload = videoram[0x1020];

	int x,y,tile_number;
	const UINT8 *source = videoram+0x800;

	for( y=0; y<256; y+=16 )
	{
		for( x=0; x<256; x+=16 )
		{
			tile_number = *source++;

			drawgfx( bitmap,
				Machine->gfx[1],
				tile_number&0x3f,
				color+(tile_number>>6), /* color */
				0,0, /* flip */
				(x-phpload)&0xff,(y-pvpload)&0xff,
				&Machine->visible_area,
				TRANSPARENCY_NONE,0 );
		}
	}
}

VIDEO_UPDATE( firetrk )
{
	draw_background( bitmap );
	draw_text( bitmap );
	draw_sprites( bitmap );

	// Map horn button onto discrete sound emulation
	discrete_sound_w(0x01,input_port_6_r(0));
}

