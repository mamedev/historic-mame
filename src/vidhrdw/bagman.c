/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "res_net.h"


unsigned char *bagman_video_enable;
static int flipscreen[2];

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Bagman has two 32 bytes palette PROMs, connected to the RGB output this
  way:

  bit 7 -- 220 ohm resistor  -- \
        -- 470 ohm resistor  -- | -- 470 ohm pulldown resistor -- BLUE

        -- 220 ohm resistor  -- \
        -- 470 ohm resistor  -- | -- 470 ohm pulldown resistor -- GREEN
        -- 1  kohm resistor  -- /

        -- 220 ohm resistor  -- \
        -- 470 ohm resistor  -- | -- 470 ohm pulldown resistor -- RED
  bit 0 -- 1  kohm resistor  -- /

***************************************************************************/
PALETTE_INIT( bagman )
{
	int i;
	const int resistances_rg[3] = { 1000, 470, 220 };
	const int resistances_b [2] = { 470, 220 };
	double weights_r[3], weights_g[3], weights_b[2];


	compute_resistor_weights(0,	255,	-1.0,
			3,	resistances_rg,	weights_r,	470,	0,
			3,	resistances_rg,	weights_g,	470,	0,
			2,	resistances_b,	weights_b,	470,	0);


	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		r = combine_3_weights(weights_r, bit0, bit1, bit2);
		/* green component */
		bit0 = (color_prom[i] >> 3) & 0x01;
		bit1 = (color_prom[i] >> 4) & 0x01;
		bit2 = (color_prom[i] >> 5) & 0x01;
		g = combine_3_weights(weights_g, bit0, bit1, bit2);
		/* blue component */
		bit0 = (color_prom[i] >> 6) & 0x01;
		bit1 = (color_prom[i] >> 7) & 0x01;
		b = combine_2_weights(weights_b, bit0, bit1);

		palette_set_color(i,r,g,b);
	}
}





WRITE_HANDLER( bagman_flipscreen_w )
{
	if ((data & 1) != flipscreen[offset])
	{
		flipscreen[offset] = data & 1;
		memset(dirtybuffer,1,videoram_size);
	}
}



/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( bagman )
{
	int offs;


	if (*bagman_video_enable == 0)
	{
		fillbitmap(bitmap,Machine->pens[0],&Machine->visible_area);

		return;
	}


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;
			int bank;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			if (flipscreen[0]) sx = 31 - sx;
			sy = offs / 32;
			if (flipscreen[1]) sy = 31 - sy;

			/* Pickin' doesn't have the second char bank */
			bank = 0;
			if (Machine->gfx[2] && (colorram[offs] & 0x10)) bank = 2;

			drawgfx(tmpbitmap,Machine->gfx[bank],
					videoram[offs] + 8 * (colorram[offs] & 0x20),
					colorram[offs] & 0x0f,
					flipscreen[0],flipscreen[1],
					8*sx,8*sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int sx,sy,flipx,flipy;


		sx = spriteram[offs + 3];
		sy = 240 - spriteram[offs + 2];
		flipx = spriteram[offs] & 0x40;
		flipy = spriteram[offs] & 0x80;
		if (flipscreen[0])
		{
			sx = 240 - sx +1;	/* compensate misplacement */
			flipx = !flipx;
		}
		if (flipscreen[1])
		{
			sy = 240 - sy;
			flipy = !flipy;
		}

		if (spriteram[offs + 2] && spriteram[offs + 3])
			drawgfx(bitmap,Machine->gfx[1],
					(spriteram[offs] & 0x3f) + 2 * (spriteram[offs + 1] & 0x20),
					spriteram[offs + 1] & 0x1f,
					flipx,flipy,
					sx,sy+1,	/* compensate misplacement */
					&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}
