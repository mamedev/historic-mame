/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *pbaction_videoram2,*pbaction_colorram2;
unsigned char *pbaction_paletteram;
void pbaction_videoram2_w(int offset,int data);
void pbaction_colorram2_w(int offset,int data);
static unsigned char *dirtybuffer2;
static struct osd_bitmap *tmpbitmap2;




/***************************************************************************

  Pinball Action doesn't have a color PROM. It uses 512 bytes of RAM to
  dynamically create the palette. Each couple of bytes defines one
  color (4 bits per pixel; the high 4 bits of the second byte are unused).

  I don't know the exact values of the resistors between the RAM and the
  RGB output. I assumed these values (the same as Commando)
  bit 7 -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 2.2kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
        -- 1  kohm resistor  -- RED
  bit 0 -- 2.2kohm resistor  -- RED

  bit 7 -- unused
        -- unused
        -- unused
        -- unused
        -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 1  kohm resistor  -- BLUE
  bit 0 -- 2.2kohm resistor  -- BLUE

***************************************************************************/
void pbaction_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	/* initialize the color table */
	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		if (i < 128 && i % 8 == 0) colortable[i] = 0;
		else colortable[i] = i;
	}
}



void pbaction_paletteram_w(int offset,int data)
{
	int bit0,bit1,bit2,bit3;
	int r,g,b,val;


	pbaction_paletteram[offset] = data;

	/* red component */
	val = pbaction_paletteram[offset & ~1];
	bit0 = (val >> 0) & 0x01;
	bit1 = (val >> 1) & 0x01;
	bit2 = (val >> 2) & 0x01;
	bit3 = (val >> 3) & 0x01;
	r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	/* green component */
	val = pbaction_paletteram[offset & ~1];
	bit0 = (val >> 4) & 0x01;
	bit1 = (val >> 5) & 0x01;
	bit2 = (val >> 6) & 0x01;
	bit3 = (val >> 7) & 0x01;
	g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	/* blue component */
	val = pbaction_paletteram[offset | 1];
	bit0 = (val >> 0) & 0x01;
	bit1 = (val >> 1) & 0x01;
	bit2 = (val >> 2) & 0x01;
	bit3 = (val >> 3) & 0x01;
	b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	palette_change_color(offset / 2,r,g,b);
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int pbaction_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((dirtybuffer2 = malloc(videoram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
	memset(dirtybuffer2,1,videoram_size);

	if ((tmpbitmap2 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	return 0;
}


/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void pbaction_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap2);
	free(dirtybuffer2);
	generic_vh_stop();
}



void pbaction_videoram2_w(int offset,int data)
{
	if (pbaction_videoram2[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		pbaction_videoram2[offset] = data;
	}
}



void pbaction_colorram2_w(int offset,int data)
{
	if (pbaction_colorram2[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		pbaction_colorram2[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void pbaction_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* recalc the palette if necessary */
	if (palette_recalc ())
	{
		memset (dirtybuffer,1,videoram_size);
		memset (dirtybuffer2,1,videoram_size);
	}


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 31 - offs / 32;
			sy = offs % 32;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + 0x10 * (colorram[offs] & 0x30),
					colorram[offs] & 0x0f,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}

		if (dirtybuffer2[offs])
		{
			int sx,sy;


			dirtybuffer2[offs] = 0;

			sx = 31 - offs / 32;
			sy = offs % 32;

			drawgfx(tmpbitmap2,Machine->gfx[1],
					pbaction_videoram2[offs] + 0x10 * (pbaction_colorram2[offs] & 0x70),
					pbaction_colorram2[offs] & 0x0f,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the background */
	copybitmap(bitmap,tmpbitmap2,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. */
	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		drawgfx(bitmap,Machine->gfx[(spriteram[offs] & 0x80) ? 3 : 2],	/* normal or double size */
				spriteram[offs],
				spriteram[offs + 1] & 0x0f,
				spriteram[offs + 1] & 0x80,spriteram[offs + 1] & 0x40,
				spriteram[offs + 2],spriteram[offs + 3],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

		/* if double size, skip next sprite */
		if (spriteram[offs] & 0x80) offs += 4;
	}


	/* copy the foreground */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
}
