/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



static unsigned char scroll[2];
static int flipscreen;
static const unsigned char *sprite_height_prom;


static struct rectangle spritevisiblearea =
{
	8*8, (64-8)*8-1,
	1*8, 32*8-1
};
static struct rectangle flipspritevisiblearea =
{
	8*8, (64-8)*8-1,
	0*8, 31*8-1
};



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Kung Fu Master has a six 256x4 palette PROMs (one per gun; three for
  characters, three for sprites).
  I don't know the exact values of the resistors between the RAM and the
  RGB output. I assumed these values (the same as Commando)

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

***************************************************************************/
void ldrun_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;

		/* red component */
		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* green component */
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		/* blue component */
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) =  0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}

	color_prom += 2*Machine->drv->total_colors;
	/* color_prom now points to the beginning of the sprite height table */

	sprite_height_prom = color_prom;	/* we'll need this at run time */
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int ldrun_vh_start(void)
{
	if ((dirtybuffer = malloc(videoram_size)) == 0)
		return 1;
	memset(dirtybuffer,1,videoram_size);

	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer);
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void ldrun_vh_stop(void)
{
	free(dirtybuffer);
        osd_free_bitmap(tmpbitmap);
}




void ldrun2p_scroll_w(int offset,int data)
{
	scroll[offset] = data;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void ldrun_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size-2;offs >= 0;offs -= 2)
	{
		if ( (dirtybuffer[offs]) || (dirtybuffer[offs+1]) )
		{
			int sx,sy,flipx;


			dirtybuffer[offs] = 0;
			dirtybuffer[offs+1] = 0;

			sx = ( offs % 128 ) / 2;
			sy = offs / 128;
			flipx = videoram[offs+1] & 0x20;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + ((videoram[offs+1] & 0xc0) << 2),
					videoram[offs+1] & 0x1f,
					flipx,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. */
	for (offs = 0;offs < spriteram_size;offs += 8)
	{
		int i,incr,code,col,flipx,flipy,sx,sy;


		code = spriteram[offs+4];

		if (code != 0)
		{
			col = spriteram[offs+0] & 0x1f;
			sx = 256 * (spriteram[offs+7] & 1) + spriteram[offs+6],
			sy = 256+128-15 - (256 * (spriteram[offs+3] & 1) + spriteram[offs+2]),
			flipx = spriteram[offs+5] & 0x40;
			flipy = spriteram[offs+5] & 0x80;

			i = sprite_height_prom[code / 32];
			if (i == 1)	/* double height */
			{
				code &= ~1;
				sy -= 16;
			}
			else if (i == 2)	/* quadruple height */
			{
				i = 3;
				code &= ~3;
				sy -= 3*16;
			}

			if (flipscreen)
			{
				sx = 240 - sx;
				sy = 242 - i*16 - sy;	/* sprites are slightly misplaced by the hardware */
				flipx = !flipx;
				flipy = !flipy;
			}

			if (flipy)
			{
				incr = -1;
				code += i;
			}
			else incr = 1;

			do
			{
				drawgfx(bitmap,Machine->gfx[1],
						code + i * incr,col,
						flipx,flipy,
						sx,sy + 16 * i,
						flipscreen ? &flipspritevisiblearea : &spritevisiblearea,TRANSPARENCY_PEN,0);

				i--;
			} while (i >= 0);
		}
	}
}

/* almost identical but scrolling background, more characters, */
/* no char x flip, and more sprites */
void ldrun2p_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size-2;offs >= 0;offs -= 2)
	{
		if ( (dirtybuffer[offs]) || (dirtybuffer[offs+1]) )
		{
			int sx,sy;


			dirtybuffer[offs] = 0;
			dirtybuffer[offs+1] = 0;

			sx = ( offs % 128 ) / 2;
			sy = offs / 128;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + ((videoram[offs+1] & 0xc0) << 2) + ((videoram[offs+1] & 0x20) << 5),
					videoram[offs+1] & 0x1f,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	{
		int scrollx;

		scrollx = -(scroll[1] + 256 * scroll[0] - 2);

		copyscrollbitmap(bitmap,tmpbitmap,1,&scrollx,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites. */
	for (offs = 0;offs < spriteram_size;offs += 8)
	{
		int i,incr,code,col,flipx,flipy,sx,sy;


		code = spriteram[offs+4] + 256 * (spriteram[offs+5] & 0x03);

		if (code != 0)
		{
			col = spriteram[offs+0] & 0x1f;
			sx = 256 * (spriteram[offs+7] & 1) + spriteram[offs+6],
			sy = 256+128-15 - (256 * (spriteram[offs+3] & 1) + spriteram[offs+2]),
			flipx = spriteram[offs+5] & 0x40;
			flipy = spriteram[offs+5] & 0x80;

			i = sprite_height_prom[code / 32];
			if (i == 1)	/* double height */
			{
				code &= ~1;
				sy -= 16;
			}
			else if (i == 2)	/* quadruple height */
			{
				i = 3;
				code &= ~3;
				sy -= 3*16;
			}

			if (flipscreen)
			{
				sx = 240 - sx;
				sy = 242 - i*16 - sy;	/* sprites are slightly misplaced by the hardware */
				flipx = !flipx;
				flipy = !flipy;
			}

			if (flipy)
			{
				incr = -1;
				code += i;
			}
			else incr = 1;

			do
			{
				drawgfx(bitmap,Machine->gfx[1],
						code + i * incr,col,
						flipx,flipy,
						sx,sy + 16 * i,
						flipscreen ? &flipspritevisiblearea : &spritevisiblearea,TRANSPARENCY_PEN,0);

				i--;
			} while (i >= 0);
		}
	}
}
