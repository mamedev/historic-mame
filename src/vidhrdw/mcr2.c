/***************************************************************************

  vidhrdw/mcr2.c

  Functions to emulate the video hardware of the machine.

  Journey is an MCR/II game with a MCR/III sprite board so it has it's own
  routines.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *mcr2_paletteram;

static int sprite_transparency[128]; /* no mcr2 game has more than this many sprites */
static int sprite_color;


/***************************************************************************

  Generic MCR/II video routines

***************************************************************************/

void mcr2_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
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
	/* we reserve pen 0 for the background black which makes the */
	/* MS-DOS version look better */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = i + 1;

   /* sprite color is 1 by default */
  	sprite_color = 1;
}


void mcr2_palette_w(int offset,int data)
{
   int r;
   int g;
   int b;

   mcr2_paletteram[offset] = data;
	offset &= 0x7f;

   r = ((offset & 1) << 2) + (data >> 6);
   g = (data >> 0) & 7;
   b = (data >> 3) & 7;

   /* up to 8 bits */
   r = (r << 5) | (r << 2) | (r >> 1);
   g = (g << 5) | (g << 2) | (g >> 1);
   b = (b << 5) | (b << 2) | (b >> 1);

   osd_modify_pen(Machine->gfx[0]->colortable[offset/2], r, g, b);
}


void mcr2_videoram_w(int offset,int data)
{
	if (videoram[offset] != data)
	{
		dirtybuffer[offset & ~1] = 1;
		videoram[offset] = data;
	}
}


void mcr2_vh_screenrefresh(struct osd_bitmap *bitmap)
{
   int offs;
   int mx,my;
   int attr;
   int color;

   /* for every character in the Video RAM, check if it has been modified */
   /* since last time and update it accordingly. */
   for (offs = videoram_size - 2;offs >= 0;offs -= 2)
   {
	/*
		The attributes are as follows:
		0x01 = character bank
		0x02 = flip x
		0x04 = flip y
		0x18 = color
	*/

      if (dirtybuffer[offs])
      {
			dirtybuffer[offs] = 0;

			mx = (offs/2) % 32;
			my = (offs/2) / 32;
			attr = videoram[offs+1];
			color = (attr & 0x18) >> 3;

			drawgfx(tmpbitmap,Machine->gfx[0],
				videoram[offs]+256*(attr & 0x01),
				color,attr & 0x02,attr & 0x04,16*mx,16*my,
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

      code = spriteram[offs+1] & 0x3f;
      flags = spriteram[offs+3];
      color = sprite_color;
		flipx = spriteram[offs+1] & 0x40;
		flipy = spriteram[offs+1] & 0x80;
      sx = (spriteram[offs+2]-3)*2;
      sy = (241-spriteram[offs])*2;

      drawgfx(bitmap,Machine->gfx[1],
	      code,color,flipx,flipy,sx,sy,
	      &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
   }
}


/***************************************************************************

  Wacko-specific video routines

***************************************************************************/

void wacko_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	/* standard init */
	mcr2_vh_convert_color_prom (palette, colortable, color_prom);

	/* except sprite color is 0 */
	sprite_color = 0;
}


/***************************************************************************

  Journey-specific video routines

***************************************************************************/

void journey_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
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
	/* we reserve pen 0 for the background black which makes the */
	/* MS-DOS version look better */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = i + 1;

   /* now check our sprites and mark which ones have color 8 ('draw under') */
   {
      struct GfxElement *gfx;
      int i,x,y;
      unsigned char *dp;

      gfx = Machine->gfx[1];
      for (i=0;i<gfx->total_elements;i++)
      {
			sprite_transparency[i] = 0;

			for (y=0;y<gfx->height;y++)
			{
				dp = gfx->gfxdata->line[i * gfx->height + y];
				for (x=0;x<gfx->width;x++)
					if (dp[x] == 8)
						sprite_transparency[i] = 1;
			}

			if (sprite_transparency[i])
				if (errorlog)
					fprintf(errorlog,"sprite %i has transparency.\n",i);
      }
   }
}


void journey_vh_screenrefresh(struct osd_bitmap *bitmap)
{
   int offs;
   int mx,my;
   int attr;
   int color;

   /* for every character in the Video RAM, check if it has been modified */
   /* since last time and update it accordingly. */
   for (offs = videoram_size - 2;offs >= 0;offs -= 2)
   {
	/*
		The attributes are as follows:
		0x01 = character bank
		0x02 = flip x
		0x04 = flip y
		0x18 = color
	*/

      if (dirtybuffer[offs])
      {
			dirtybuffer[offs] = 0;

			mx = (offs/2) % 32;
			my = (offs/2) / 32;
	      attr = videoram[offs+1];
	      color = (attr & 0x18) >> 3;

			drawgfx(tmpbitmap,Machine->gfx[0],
				videoram[offs]+256*(attr & 0x01),
				color,attr & 0x02,attr & 0x04,16*mx,16*my,
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

      code = spriteram[offs+2];
      flags = spriteram[offs+1];
      color = 3 - (flags & 0x03);
      flipx = flags & 0x10;
      flipy = flags & 0x20;
      sx = (spriteram[offs+3]-3)*2;
      sy = (241-spriteram[offs])*2;

      drawgfx(bitmap,Machine->gfx[1],
	      code,color,flipx,flipy,sx,sy,
	      &Machine->drv->visible_area,TRANSPARENCY_PEN,0);

      /* sprites use color 0 for background pen and 8 for the 'under tile' pen.
			The color 8 is used to cover over other sprites.
      	At the beginning we scanned all sprites and marked the ones that contained
			at least one pixel of color 8, so we only need to worry about these few. */
      if (sprite_transparency[code])
      {
			struct rectangle clip;

			clip.min_x = sx;
			clip.max_x = sx+31;
			clip.min_y = sy;
			clip.max_y = sy+31;

			copybitmap(bitmap,tmpbitmap,0,0,0,0,&clip,TRANSPARENCY_THROUGH,8+(color*16)+1);
      }
   }
}
