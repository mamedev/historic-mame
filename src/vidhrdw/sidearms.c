/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#include "osdepend.h"

unsigned char *sidearms_bg_scrollx,*sidearms_bg_scrolly;
unsigned char *sidearms_bg2_scrollx,*sidearms_bg2_scrolly;
static struct mame_bitmap *tmpbitmap2;
static int flipscreen;
static int bgon,objon;


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

VIDEO_START( sidearms )
{
	if (video_start_generic() != 0)
		return 1;

	/* create a temporary bitmap slightly larger than the screen for the background */
	if ((tmpbitmap2 = auto_bitmap_alloc(48*8 + 32,Machine->drv->screen_height + 32)) == 0)
		return 1;

	return 0;
}




WRITE_HANDLER( sidearms_c804_w )
{
	/* bits 0 and 1 are coin counters */
	coin_counter_w(0,data & 0x01);
	coin_counter_w(1,data & 0x02);

	/* bit 4 probably resets the sound CPU */

	/* TODO: I don't know about the other bits (all used) */

	/* bit 7 flips screen */
	if (flipscreen != (data & 0x80))
	{
		flipscreen = data & 0x80;
/* TODO: support screen flip */
//		memset(dirtybuffer,1,c1942_backgroundram_size);
	}
}

WRITE_HANDLER( sidearms_gfxctrl_w )
{
	objon = data & 0x01;
	bgon = data & 0x02;
}


/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( sidearms )
{
	int offs, sx, sy;
	int scrollx,scrolly;
	static int lastoffs;


	/* There is a scrolling blinking star background behind the tile */
	/* background, but I have absolutely NO IDEA how to render it. */
	/* The scroll registers have a 64 pixels resolution. */
#ifdef IHAVETHEBACKGROUND
	scrollx = -(*sidearms_bg2_scrollx & 0x3f);
	scrolly = -(*sidearms_bg2_scrolly & 0x3f);
#endif


	if (bgon)
	{
		scrollx = sidearms_bg_scrollx[0] + 256 * sidearms_bg_scrollx[1] + 64;
		scrolly = sidearms_bg_scrolly[0] + 256 * sidearms_bg_scrolly[1];
		offs = 2 * (scrollx >> 5) + 0x100 * (scrolly >> 5);
		scrollx = -(scrollx & 0x1f);
		scrolly = -(scrolly & 0x1f);

		if (offs != lastoffs)
		{
			unsigned char *p=memory_region(REGION_GFX4);


			lastoffs = offs;

			/* Draw the entire background scroll */
			for (sy = 0;sy < 9;sy++)
			{
				for (sx = 0; sx < 13; sx++)
				{
					int offset;


					offset = offs + 2 * sx;

					/* swap bits 1-7 and 8-10 of the address to compensate for the */
					/* funny layout of the ROM data */
					offset = (offset & 0xf801) | ((offset & 0x0700) >> 7) | ((offset & 0x00fe) << 3);

					drawgfx(tmpbitmap2,Machine->gfx[1],
							p[offset] + 256 * (p[offset+1] & 0x01),
							(p[offset+1] & 0xf8) >> 3,
							p[offset+1] & 0x02,p[offset+1] & 0x04,
							32*sx,32*sy,
							0,TRANSPARENCY_NONE,0);
				}
				offs += 0x100;
			}
		}

	scrollx += 64;

	copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->visible_area,TRANSPARENCY_NONE,0);
	}
	else fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);

	/* Draw the sprites. */
	if (objon)
	{
		for (offs = spriteram_size - 32;offs >= 0;offs -= 32)
		{
			sx = spriteram[offs + 3] + ((spriteram[offs + 1] & 0x10) << 4);
			sy = spriteram[offs + 2];
			if (flipscreen)
			{
				sx = 496 - sx;
				sy = 240 - sy;
			}

			drawgfx(bitmap,Machine->gfx[2],
					spriteram[offs] + 8 * (spriteram[offs + 1] & 0xe0),
					spriteram[offs + 1] & 0x0f,
					flipscreen,flipscreen,
					sx,sy,
					&Machine->visible_area,TRANSPARENCY_PEN,15);
		}
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		sx = offs % 64;
		sy = offs / 64;

		if (flipscreen)
		{
			sx = 63 - sx;
			sy = 31 - sy;
		}

		drawgfx(bitmap,Machine->gfx[0],
				videoram[offs] + 4 * (colorram[offs] & 0xc0),
				colorram[offs] & 0x3f,
				flipscreen,flipscreen,
				8*sx,8*sy,
				&Machine->visible_area,TRANSPARENCY_PEN,3);
	}
}
