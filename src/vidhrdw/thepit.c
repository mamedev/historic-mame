/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


static struct rectangle spritevisiblearea =
{
	2*8+1, 32*8-1,
	2*8, 30*8-1
};
static struct rectangle spritevisibleareaflipx =
{
	0*8, 30*8-2,
	2*8, 30*8-1
};



/***************************************************************************

  I'm assuming Frogger resistor values

***************************************************************************/
void thepit_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2;


		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		palette[3*i]     = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		palette[3*i + 1] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = (color_prom[i] >> 6) & 0x01;
		bit2 = (color_prom[i] >> 7) & 0x01;
		palette[3*i + 2] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	}

	for (i = 0;i < 4 * 8;i++)
	{
		colortable[i] = i;
	}
}


void thepit_flipx_w(int offset,int data)
{
	if (*flip_screen_x != (data & 1))
	{
		*flip_screen_x = data & 1;
		memset(dirtybuffer,1,videoram_size);
	}
}

void thepit_flipy_w(int offset,int data)
{
	if (*flip_screen_y != (data & 1))
	{
		*flip_screen_y = data & 1;
		memset(dirtybuffer,1,videoram_size);
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void thepit_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;

			dirtybuffer[offs] = 0;

			sx = (offs % 32);
			sy = (offs / 32);

			if (*flip_screen_x) sx = 31 - sx;
			if (*flip_screen_y) sy = 31 - sy;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					colorram[offs] & 0x0f,
					*flip_screen_x,*flip_screen_y,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* draw the sprites */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int sx,sy,flipx,flipy;

		if (spriteram[offs + 0] == 0 ||
		    spriteram[offs + 3] == 0)
		{
		    continue;
		}

		sx = (spriteram[offs+3] + 1) & 0xff;
		sy = 240 - spriteram[offs];

		flipx = spriteram[offs + 1] & 0x40;
		flipy = spriteram[offs + 1] & 0x80;

		if (*flip_screen_x)
		{
		    sx = 242 - sx;
		    flipx = !flipx;
		}

		if (*flip_screen_y)
		{
		    sy = 240 - sy;
		    flipy = !flipy;
		}

		/* Sprites 0-3 are drawn one pixel to the left */
		if (offs <= 3*4) sy++;

		drawgfx(bitmap,Machine->gfx[1],
				spriteram[offs + 1] & 0x3f,
				spriteram[offs + 2] & 0x07,
				flipx,flipy,
				sx,sy,
				*flip_screen_x & 1 ? &spritevisibleareaflipx : &spritevisiblearea,TRANSPARENCY_PEN,0);
	}
}
