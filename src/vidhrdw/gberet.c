/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define VIDEO_RAM_SIZE 0x800

unsigned char *gberet_scroll;
unsigned char *gberet_spritebank;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Green Beret has a 32 bytes palette PROM and two 256 bytes color lookup table
  PROMs (one for sprites, one for characters).
  I don't know for sure how the palette PROM is connected to the RGB output,
  but it's probably the usual:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/
void gberet_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2;


		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		palette[3*i] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		palette[3*i + 1] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = (color_prom[i] >> 6) & 0x01;
		bit2 = (color_prom[i] >> 7) & 0x01;
		palette[3*i + 2] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	}

	for (i = 0;i < 16*16;i++)
		colortable[i] = color_prom[i + 32];
	for (i = 16*16;i < 2*16*16;i++)
	{
		if (color_prom[i + 32]) colortable[i] = color_prom[i + 32] + 0x10;
		else colortable[i] = 0;
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int gberet_vh_start(void)
{
	int len;


	/* Green Beret has a virtual screen twice as large as the visible screen */
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
void gberet_vh_stop(void)
{
	free(dirtybuffer);
	osd_free_bitmap(tmpbitmap);
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void gberet_vh_screenrefresh(struct osd_bitmap *bitmap)
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
					videoram[offs] + 4 * (colorram[offs] & 0x40),
					colorram[offs] & 0x0f,
					colorram[offs] & 0x10,colorram[offs] & 0x20,
					sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		struct rectangle clip;


		clip.min_x = Machine->drv->visible_area.min_x;
		clip.max_x = Machine->drv->visible_area.max_x;

		for (i = 2;i < 32;i++)
		{
			int scroll;


			scroll = gberet_scroll[i] + 256 * gberet_scroll[i + 32];

			clip.min_y = 8 * i - 8;
			clip.max_y = clip.min_y + 7;
			copybitmap(bitmap,tmpbitmap,0,0,-scroll,-8,&clip,TRANSPARENCY_NONE,0);
			copybitmap(bitmap,tmpbitmap,0,0,512 - scroll,-8,&clip,TRANSPARENCY_NONE,0);
		}
	}


	/* Draw the sprites. */
	if ((*gberet_spritebank & 0x08) != 0)
	{
		for (offs = 0;offs < 48*4;offs += 4)
		{
			if (spriteram[offs+3])
			{
				drawgfx(bitmap,Machine->gfx[(spriteram[offs+1] & 0x40) ? 2 : 1],
						spriteram[offs],spriteram[offs+1] & 0x0f,
						spriteram[offs+1] & 0x10,spriteram[offs+1] & 0x20,
						spriteram[offs+2] - 2*(spriteram[offs+1] & 0x80),spriteram[offs+3]-8,
						&Machine->drv->visible_area,TRANSPARENCY_COLOR,Machine->background_pen);
			}
		}
	}
	else
	{
		for (offs = 64*4;offs < 64*4+48*4;offs += 4)
		{
			if (spriteram[offs+3])
			{
				drawgfx(bitmap,Machine->gfx[(spriteram[offs+1] & 0x40) ? 2 : 1],
						spriteram[offs],spriteram[offs+1] & 0x0f,
						spriteram[offs+1] & 0x10,spriteram[offs+1] & 0x20,
						spriteram[offs+2] - 2*(spriteram[offs+1] & 0x80),spriteram[offs+3]-8,
						&Machine->drv->visible_area,TRANSPARENCY_COLOR,Machine->background_pen);
			}
		}
	}
}
