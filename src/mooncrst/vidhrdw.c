/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mame.h"
#include "common.h"
#include "driver.h"
#include "machine.h"
#include "osdepend.h"



#define VIDEO_RAM_SIZE 0x400


unsigned char *mooncrst_videoram;
unsigned char *mooncrst_attributesram;
unsigned char *mooncrst_spriteram;
unsigned char *mooncrst_bulletsram;
static unsigned char dirtybuffer[VIDEO_RAM_SIZE];	/* keep track of modified portions of the screen */
											/* to speed up video refresh */

static int gfx_extend;	/* used by Moon Cresta only */
static int gfx_bank;	/* used by Pisces and "japirem" only */

static struct osd_bitmap *tmpbitmap;



static struct rectangle visiblearea =
{
	2*8, 30*8-1,
	0*8, 32*8-1
};



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Moon Cresta has one 32 bytes palette PROM, connected to the RGB output
  this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

***************************************************************************/
void mooncrst_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
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

	for (i = 0;i < 4 * 32;i++)
		colortable[i] = i;
}



int mooncrst_vh_start(void)
{
	int i;


	gfx_extend = 0;
	gfx_bank = 0;

	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;


	for (i = 0;i < tmpbitmap->height;i++)
		memset(tmpbitmap->line[i],Machine->background_pen,tmpbitmap->width);

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void mooncrst_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



void mooncrst_videoram_w(int offset,int data)
{
	if (mooncrst_videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		mooncrst_videoram[offset] = data;
	}
}



void mooncrst_attributes_w(int offset,int data)
{
	if ((offset & 1) && mooncrst_attributesram[offset] != data)
	{
		int i;


		for (i = offset / 2;i < VIDEO_RAM_SIZE;i += 32)
			dirtybuffer[i] = 1;
	}

	mooncrst_attributesram[offset] = data;
}



void mooncrst_gfxextend_w(int offset,int data)
{
	if (data) gfx_extend |= (1 << offset);
	else gfx_extend &= ~(1 << offset);
}



void pisces_gfxbank_w(int offset,int data)
{
	gfx_bank = data & 1;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void mooncrst_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int i,offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;
			int charnum;


			dirtybuffer[offs] = 0;

			sx = (31 - offs / 32);
			sy = (offs % 32);

			charnum = mooncrst_videoram[offs];
			if ((gfx_extend & 4) && (charnum & 0xc0) == 0x80)
				charnum = (charnum & 0x3f) | (gfx_extend << 6);

			drawgfx(tmpbitmap,Machine->gfx[0],
					charnum + 256 * gfx_bank,
					mooncrst_attributesram[2 * sy + 1],
					0,0,8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	for (i = 0;i < 32*8;i++)
	{
		int scroll;


/// TODO: clip to visible area
		scroll = mooncrst_attributesram[2 * (i / 8)];
		if (scroll)
		{
			memcpy(bitmap->line[i],tmpbitmap->line[i] + 256 - scroll,scroll);
			memcpy(bitmap->line[i] + scroll,tmpbitmap->line[i],256 - scroll);
		}
		else memcpy(bitmap->line[i],tmpbitmap->line[i],256);
	}

	/* Draw the bullets */
	for (offs = 0; offs < 4*8; offs += 4)
	{
		int x,y;


		int color = Machine->gfx[0]->colortable[(offs < 4*6) ?
				4*Machine->drv->white_text+3 : 4*Machine->drv->yellow_text+3];

		x = mooncrst_bulletsram[offs + 1];

		if (x != 0 && x != 255)
		{
			y = 256 - mooncrst_bulletsram[offs + 3] - 4;

			if (y >= 0)
			{
				int j;


				for (j = 0; j < 3; j++)
				{
					bitmap->line[y+j][x] = color;
				}
			}
		}
	}


	/* Draw the sprites */
	for (offs = 0;offs < 4*8;offs += 4)
	{
		if (mooncrst_spriteram[offs + 3] >= 16)	/* ???? */
		{
			int spritenum;


			spritenum = mooncrst_spriteram[offs + 1] & 0x3f;
			if ((gfx_extend & 4) && (spritenum & 0x30) == 0x20)
				spritenum = (spritenum & 0x0f) | (gfx_extend << 4);

			drawgfx(bitmap,Machine->gfx[1],
					spritenum + 64 * gfx_bank,
					mooncrst_spriteram[offs + 2],
					mooncrst_spriteram[offs + 1] & 0x80,mooncrst_spriteram[offs + 1] & 0x40,
					mooncrst_spriteram[offs],mooncrst_spriteram[offs + 3],
					&visiblearea,TRANSPARENCY_PEN,0);
		}
	}
}
