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



void exerion_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


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
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		color_prom++;
	}


	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = (*(color_prom++) & 0x0f);
}



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
void exerion_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
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
		int doubled;
		int color;

		x = spriteram[i+1]-32;
		y = spriteram[i+3]*2-32;
		s = spriteram[i+2];
		/* decode the sprite number */
		s = s2 = ((s & 0x07) << 5) | ((s & 0xf0) >> 4) | ((s & 0x08) << 1);
		xflip = spriteram[i] & 0x40;
		yflip = spriteram[i] & 0x80;
		wide = spriteram[i] & 0x08;
		doubled = spriteram[i] & 0x10;

		color = 1;

		if (wide)
		{
			if (xflip)
				s++;
			else
				s2++;

			if (doubled)
			{
				drawgfx (tmpbitmap,Machine->gfx[2],
					s2,
					color,
					xflip,yflip,
					x-48,y,
					0, TRANSPARENCY_PEN,0);
			}
			else
			{
				drawgfx (tmpbitmap,Machine->gfx[1],
					s2,
					color,
					xflip,yflip,
					x-16,y,
					0, TRANSPARENCY_PEN,0);
			}
		}

		if (doubled)
		{
			drawgfx (tmpbitmap,Machine->gfx[2],
			s,
			color,
			xflip,yflip,
			x-16,y,
			0, TRANSPARENCY_PEN,0);
		}
		else
		{
			drawgfx (tmpbitmap,Machine->gfx[1],
			s,
			color,
			xflip,yflip,
			x,y,
			0, TRANSPARENCY_PEN,0);
		}

		if (doubled) i += 4;
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

