/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

//#define DEBUG_SPRITES

#ifdef DEBUG_SPRITES
#include <stdio.h>
FILE	*sprite_log;
#endif

/* video/control register 1  */
static unsigned char videoctlreg;

/* use this to select palette */
static unsigned char palreg;

/* used to select video bank */
static int bankreg;

int exerion_vh_start (void)
{
#ifdef DEBUG_SPRITES
	sprite_log = fopen ("sprite.log","w");
#endif
	return generic_vh_start();
}

void exerion_vh_stop (void)
{
#ifdef DEBUG_SPRITES
	fclose (sprite_log);
#endif
	generic_vh_stop();
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void exerion_vh_screenrefresh (struct osd_bitmap *bitmap)
{
   int sx,sy,offs,i;

	for (offs = videoram_size - 1;offs >= 0;offs -- )
	{
		dirtybuffer[offs] = 0;

		{
			sx = 29 - offs / 64;
			sy = offs % 64 - 12;
		}

		drawgfx(tmpbitmap,Machine->gfx[0],
			videoram[offs] + 256*bankreg,
			1,
			0,0,
			8*sx,8*sy,
			0,TRANSPARENCY_NONE,0);

	}

#ifdef DEBUG_SPRITES
	if (sprite_log)
	{
		int i;

		for (i = 0; i < spriteram_size; i+= 4)
		{
			if (spriteram[i+2] == 0x02)
			{
				fprintf (sprite_log, "%02x %02x %02x %02x\n",spriteram[i], spriteram[i+1], spriteram[i+2], spriteram[i+3]);
			}
		}
	}
#endif

	/* draw sprites */
	for (i=0; i < spriteram_size; i+=4)
	{
		int x, y, s, s2;
		int xflip, yflip, wide;
		int wider;
		int color;

		x = spriteram[i+1]-32;
		y = spriteram[i+3]*2-32;
		s = spriteram[i+2];
		/* decode the sprite number */
		s = s2 = ((s & 0x07) << 5) | ((s & 0xf0) >> 4) | ((s & 0x08) << 1);
		xflip = spriteram[i] & 0x40;
		yflip = spriteram[i] & 0x80;
		wide = spriteram[i] & 0x08;
		wider = spriteram[i] & 0x10;

		color = 1;

		if (wide)
		{
			if (xflip)
				s++;
			else
				s2++;

			drawgfx (tmpbitmap,Machine->gfx[1],
				s2,
				color,
				xflip,yflip,
				x-16,y,
				0, TRANSPARENCY_PEN,0);

			if (wider)
			{
				drawgfx (tmpbitmap,Machine->gfx[1],
					s,
					color,
					xflip,yflip,
					x-32,y,
					0, TRANSPARENCY_PEN,0);
				drawgfx (tmpbitmap,Machine->gfx[1],
					s2,
					color,
					xflip,yflip,
					x-48,y,
					0, TRANSPARENCY_PEN,0);
			}
		}

		drawgfx (tmpbitmap,Machine->gfx[1],
			s,
			color,
			xflip,yflip,
			x,y,
			0, TRANSPARENCY_PEN,0);
	}
	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}

/* JB 971127 */
void exerion_videoreg_w (int offset,int data)
{
	videoctlreg = data;

	/*   REMEMBER - both bits 1&2 are used to set the palette
	 *   Don't forget to add in bit 2, which doubles as the bank
	 *   select
	 */

	palreg  = (videoctlreg >> 1) & 0x01;	/* palette sel is bit 1 */
	bankreg = (videoctlreg >> 2) & 0x01;	/* banksel is bit 2 */
}

