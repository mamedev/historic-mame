#include "vidhrdw/generic.h"

static data16_t tigeroad_scrollram[2];
static int flipscreen,bgcharbank;



WRITE16_HANDLER( tigeroad_videoctrl_w )
{
	if (ACCESSING_MSB)
	{
		data = (data >> 8) & 0xff;

		/* bit 1 flips screen */
		flipscreen = data & 0x02;

		/* bit 2 selects bg char bank */
		bgcharbank = (data & 0x04) >> 2;

		/* bits 4-5 are unknown, but used */

		/* bits 6-7 are coin counters */
		coin_counter_w(0,data & 0x40);
		coin_counter_w(1,data & 0x80);
	}
}

WRITE16_HANDLER( tigeroad_scroll_w )
{
	COMBINE_DATA(&tigeroad_scrollram[offset]);
}


static void render_background( struct osd_bitmap *bitmap, int priority )
{
	int scrollx = 	tigeroad_scrollram[0] & 0xfff; /* 0..4096 */
	int scrolly =	tigeroad_scrollram[1] & 0xfff; /* 0..4096 */

	unsigned char *p = memory_region(REGION_GFX4);

	int alignx = scrollx%32;
	int aligny = scrolly%32;

	int row = scrolly/32;	/* 0..127 */
	int sy = 224+aligny;

	int transp0,transp1;

	if( priority ){ /* foreground */
		transp0 = 0xFFFF;	/* draw nothing (all pens transparent) */
		transp1 = 0x01FF;	/* high priority half of tile */
	}
	else { /* background */
		transp0 = 0;		/* NO_TRANSPARENCY */
		transp1 = 0xFE00;	/* low priority half of tile */
	}

	while( sy>-32 ){
		int col = scrollx/32;	/* 0..127 */
		int sx = -alignx;

		while( sx<256 ){
			int offset = 2*(col%8) + 16*(row%8) + 128*(col/8) + 2048*(row/8);

			int code = p[offset];
			int attr = p[offset+1];

			int flipx = attr & 0x20;
			int flipy = 0;
			int color = attr & 0x0f;

			if (flipscreen)
				drawgfx(bitmap,Machine->gfx[1],
						code + ((attr & 0xc0) << 2) + (bgcharbank << 10),
						color,
						!flipx,!flipy,
						224-sx,224-sy,
						&Machine->visible_area,
						TRANSPARENCY_PENS,(attr & 0x10) ? transp1 : transp0);
			else
				drawgfx(bitmap,Machine->gfx[1],
						code + ((attr & 0xc0) << 2) + (bgcharbank << 10),
						color,
						flipx,flipy,
						sx,sy,
						&Machine->visible_area,
						TRANSPARENCY_PENS,(attr & 0x10) ? transp1 : transp0);

			sx+=32;
			col++;
			if( col>=128 ) col-=128;
		}
		sy-=32;
		row++;
		if( row>=128 ) row-=128;
	}
}

static void render_sprites( struct osd_bitmap *bitmap )
{
	data16_t *source = &buffered_spriteram16[spriteram_size/2] - 4;
	data16_t *finish = buffered_spriteram16;

	while( source>=finish )
	{
		int tile_number = source[0];
		if( tile_number!=0xFFF ){
			int attributes = source[1];
			int sy = source[2] & 0x1ff;
			int sx = source[3] & 0x1ff;

			int flipx = attributes&2;
			int flipy = attributes&1;
			int color = (attributes>>2)&0xf;

			if( sx>0x100 ) sx -= 0x200;
			if( sy>0x100 ) sy -= 0x200;

			if (flipscreen)
			{
				sx = 240 - sx;
				sy = 240 - sy;
				flipx = !flipx;
				flipy = !flipy;
			}

			drawgfx(bitmap,Machine->gfx[2],
				tile_number,
				color,
				flipx,flipy,
				sx,240-sy,
				&Machine->visible_area,
				TRANSPARENCY_PEN,15);
		}
		source-=4;
	}
}

static void render_text( struct osd_bitmap *bitmap )
{
	int offs;


	for (offs = 0;offs < videoram_size/2;offs++)
	{
		int sx,sy;
		int data = videoram16[offs];
		int attr = data >> 8;
		int code = data & 0xff;
		int color = attr & 0x0f;
		int flipy = attr & 0x10;

		sx = offs % 32;
		sy = offs / 32;

		if (flipscreen)
		{
			sx = 31 - sx;
			sy = 31 - sy;
			flipy = !flipy;
		}

		drawgfx(bitmap,Machine->gfx[0],
				code + ((attr & 0xc0) << 2) + ((attr & 0x20) << 5),
				color,
				flipscreen,flipy,
				8*sx,8*sy,
				&Machine->visible_area, TRANSPARENCY_PEN,3);
	}
}



void tigeroad_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	palette_recalc();
	/* no need to check the return code since we redraw everything each frame */

	render_background( bitmap,0 );
	render_sprites( bitmap );
	render_background( bitmap,1 );
	render_text( bitmap );
}

void tigeroad_eof_callback(void)
{
	buffer_spriteram16_w(0,0,0);
}
