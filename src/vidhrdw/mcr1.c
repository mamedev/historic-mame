/***************************************************************************

  vidhrdw/mcr1.c

  Functions to emulate the video hardware of an mcr 1 style machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *mcr1_paletteram;

static int spriteoffs;


/***************************************************************************

  Generic MCR/I video routines

***************************************************************************/

void mcr1_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	/* the palette will be initialized by the game. We just set it to some */
	/* pre-cooked values so the startup copyright notice can be displayed. */
	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		*(palette++) = ((i & 1) >> 0) * 0xff;
		*(palette++) = ((i & 2) >> 1) * 0xff;
		*(palette++) = ((i & 4) >> 2) * 0xff;
	}

	/* characters and sprites use the same palette */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = i;

	/* no sprite offset by default */
   spriteoffs = 0;
}


void mcr1_palette_w(int offset,int data)
{
   int color;
   int r;
   int g;
   int b;

   mcr1_paletteram[offset] = data;

	if ((offset >= 0x00 && offset < 0x40) || (offset >= 0x400 && offset < 0x440))
	{
      color = offset & 0x1f;

      b = mcr1_paletteram[color] >> 4;
      g = mcr1_paletteram[color] & 0x0f;
      r = mcr1_paletteram[color+0x400] & 0x0f;

      /* up to 8 bits */
      r = (r << 4) | r;
      g = (g << 4) | g;
      b = (b << 4) | b;

      osd_modify_pen(Machine->gfx[0]->colortable[color], r, g, b);
	}
}


void mcr1_vh_screenrefresh(struct osd_bitmap *bitmap)
{
   int offs;
   int mx,my;

   /* for every character in the Video RAM, check if it has been modified */
   /* since last time and update it accordingly. */

   for (offs = videoram_size - 1;offs >= 0;offs--)
   {
      if (dirtybuffer[offs])
      {
			dirtybuffer[offs] = 0;

			mx = (offs) % 32;
			my = (offs) / 32;

			drawgfx(tmpbitmap,Machine->gfx[0],
				videoram[offs],
				0,0,0,16*mx,16*my,
				&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
      }
   }

   /* copy the character mapped graphics */
   copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

   /* Draw the sprites. */
   for (offs = 0;offs < spriteram_size;offs += 4)
   {
      int code,color,flipx,flipy,sx,sy,flags;

      if (spriteram[offs] == 0)
			continue;

      sy = (241-spriteram[offs])*2;
      code = spriteram[offs+1] & 0x3f;
      sx = (spriteram[offs+2]-3+spriteoffs)*2;
      flags = spriteram[offs+3];
      color = 1;
      flipx = spriteram[offs+1] & 0x40;
      flipy = spriteram[offs+1] & 0x80;

      drawgfx(bitmap,Machine->gfx[1],
	      code,color,flipx,flipy,sx,sy,
	      &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
   }
}


/***************************************************************************

  Solar Fox-specific video routines

***************************************************************************/

void solarfox_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	/* standard init */
	mcr1_vh_convert_color_prom (palette, colortable, color_prom);

	/* except the sprites need to be offset */
	spriteoffs = 6;
}
