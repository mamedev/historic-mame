#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *exprraid_bgcontrol;


static void drawbg(struct mame_bitmap *bitmap,int priority)
{
	unsigned char *map1 = &memory_region(REGION_GFX4)[0x0000];
	unsigned char *map2 = &memory_region(REGION_GFX4)[0x4000];
	int offs,scrolly,scrollx1,scrollx2;


	scrolly = exprraid_bgcontrol[4];
	/* TODO: bgcontrol[7] seems related to the y scroll as well, but I'm not sure how */
	scrollx1 = exprraid_bgcontrol[5];
	scrollx2 = exprraid_bgcontrol[6];

	for (offs = 0x100 - 1;offs >= 0;offs--)
	{
		int sx,sy,quadrant,base,bank;


		sx = 16 * (offs % 16);
		sy = 16 * (offs / 16) - scrolly;

		quadrant = 0;
		if (sy <= -8)
		{
			quadrant += 2;
			sy += 256;
			sx -= scrollx2;
		}
		else
			sx -= scrollx1;

		if (sx <= -8)
		{
			quadrant++;
			sx += 256;
		}

		base = (exprraid_bgcontrol[quadrant] & 0x3f) * 0x100;

		if (priority == 0 || (map2[offs+base] & 0x80))
		{
			bank = 2*(map2[offs+base] & 0x03)+((map1[offs+base] & 0x80) >> 7);

			drawgfx(bitmap,Machine->gfx[2+bank],
					map1[offs+base] & 0x7f,
					(map2[offs+base] & 0x18) >> 3,
					(map2[offs+base] & 0x04),0,
					sx,sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}
}



VIDEO_UPDATE( exprraid )
{
	int offs;


	/* draw the background */
	drawbg(bitmap,0);

	/* draw the sprites */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		int sx,sy,code,color,flipx;

		code = spriteram[offs+3] + ( ( spriteram[offs+1] & 0xe0 ) << 3 );

		sx = ((248 - spriteram[offs+2]) & 0xff) - 8;
		sy = spriteram[offs];
		color = (spriteram[offs+1] & 0x03) + ((spriteram[offs+1] & 0x08) >> 1);
		flipx = ( spriteram[offs+1] & 0x04 );

		drawgfx(bitmap,Machine->gfx[1],
				code,
				color,
				flipx,0,
				sx,sy,
				0,TRANSPARENCY_PEN,0);

		if ( spriteram[offs+1] & 0x10 ) { /* double height */
			drawgfx(bitmap,Machine->gfx[1],
					code + 1,
					color,
					flipx,0,
					sx,sy+16,
					0,TRANSPARENCY_PEN,0);
		}
	}


	/* redraw the tiles which have priority over the sprites */
	drawbg(bitmap,1);


	/* draw the foreground */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy;

		sx = offs % 32;
		sy = offs / 32;

		drawgfx(bitmap,Machine->gfx[0],
				videoram[offs] + ((colorram[offs] & 7) << 8),
				(colorram[offs] & 0x10) >> 4,
				0,0,
				8*sx,8*sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}
