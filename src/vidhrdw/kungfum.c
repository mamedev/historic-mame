/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define VIDEO_RAM_SIZE 0x800

unsigned char *kungfum_scroll_low;
unsigned char *kungfum_scroll_high;



static struct rectangle spritevisiblearea =
{
	0*8, 32*8-1,
	10*8, 32*8-1
};



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int kungfum_vh_start(void)
{
	int len;


	/* Kung Fu Master has a virtual screen twice as large as the visible screen */
	len = 2 * (Machine->drv->screen_width/8) * (Machine->drv->screen_height/8);

	if ((dirtybuffer = malloc(len)) == 0)
		return 1;
	memset(dirtybuffer,0,len);

	if ((tmpbitmap = osd_create_bitmap(2 * Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer);
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void kungfum_vh_stop(void)
{
	free(dirtybuffer);
	osd_free_bitmap(tmpbitmap);
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void kungfum_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,i;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 8 * (offs % 64);
			sy = 8 * (offs / 64);

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + 4 * (colorram[offs] & 0xc0),
					colorram[offs] & 0x1f,
					colorram[offs] & 0x20,0,
					sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scroll[32];


		for (i = 0;i < 6;i++)
			scroll[i] = -128;
		for (i = 6;i < 32;i++)
			scroll[i] = -(*kungfum_scroll_low + 256 * *kungfum_scroll_high) - 128;

		copyscrollbitmap(bitmap,tmpbitmap,32,scroll,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites. */
	for (offs = 0;offs < 24*8;offs += 8)
	{
		int bank,i,code,col,flipx,sx,sy;


		bank = spriteram[offs+5] & 0x03;
		code = spriteram[offs+4];

		if (code != 0 || bank != 0)
		{
			col = spriteram[offs+0];
			flipx = spriteram[offs+5] & 0x40;
			sx = (256 * spriteram[offs+7] + spriteram[offs+6]) - 128,
			sy = 256+128-12 - (256 * spriteram[offs+3] + spriteram[offs+2]),

			i = 0;
			if (bank == 1 && code >= 0x80)
			{
				i = 3;	/* quadruple height */
				code &= 0xfc;
				sy -= 3*16;
			}
			else if ((bank == 0 && code >= 0x40)
					|| (bank == 1 && code >= 0x40)
					|| (bank == 2 && code >= 0x20 && code < 0xe0)
					|| (bank == 3 && code >= 0x20 && code < 0x60)
					|| (bank == 3 && code >= 0x80))
			{
				i = 1;	/* double height */
				code &= 0xfe;
				sy -= 16;
			}


			do
			{
				drawgfx(bitmap,Machine->gfx[1 + bank],
						code + i,col,
						flipx, 0,
						sx,sy + 16 * i,
						&spritevisiblearea,TRANSPARENCY_PEN,0);

				i--;
			} while (i >= 0);
		}
	}
}
