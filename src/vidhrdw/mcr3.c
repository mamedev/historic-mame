/***************************************************************************

  vidhrdw/mcr3.c

  Functions to emulate the video hardware of an mcr3-style machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *mcr3_paletteram;

static int sprite_transparency[512]; /* no mcr3 game has more than this many sprites */


/***************************************************************************

  Generic MCR/III video routines

***************************************************************************/

void mcr3_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
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


void mcr3_palette_w(int offset,int data)
{
   int r;
   int g;
   int b;

   mcr3_paletteram[offset] = data;
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


void mcr3_videoram_w(int offset,int data)
{
	if (videoram[offset] != data)
	{
		dirtybuffer[offset & ~1] = 1;
		videoram[offset] = data;
	}
}


void mcr3_vh_screenrefresh(struct osd_bitmap *bitmap)
{
   int offs;
   int mx,my;
   int attr;
   int color;

   /* for every character in the Video RAM, check if it has been modified */
   /* since last time and update it accordingly. */
   for (offs = videoram_size - 2;offs >= 0;offs -= 2)
   {
      if (dirtybuffer[offs])
      {
			dirtybuffer[offs] = 0;

			mx = (offs/2) % 32;
			my = (offs/2) / 32;
	      attr = videoram[offs+1];
	      color = (attr & 0x30) >> 4;

			drawgfx(tmpbitmap,Machine->gfx[0],
				videoram[offs]+256*(attr & 0x03),
				color,attr & 0x04,attr&0x08,16*mx,16*my,
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

			copybitmap(bitmap,tmpbitmap,0,0,0,0,&clip,TRANSPARENCY_THROUGH,8+(color*16));
      }
   }
}


/***************************************************************************

  Rampage-specific video routines

***************************************************************************/

void rampage_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	struct osd_bitmap *bitmap = Machine->gfx[0]->gfxdata;
	int y, x;

	/* standard init */
	mcr3_vh_convert_color_prom (palette, colortable, color_prom);

	/* the tile graphics for Rampage are reverse colors; we preswap them here */
	for (y = bitmap->height - 1; y >= 0; y--)
		for (x = bitmap->width - 1; x >= 0; x--)
			bitmap->line[y][x] ^= 15;
}


void rampage_vh_screenrefresh(struct osd_bitmap *bitmap)
{
   int offs;
   int mx,my;
   int attr;
   int color;

   /* for every character in the Video RAM, check if it has been modified */
   /* since last time and update it accordingly. */
   for (offs = videoram_size - 2;offs >= 0;offs -= 2)
   {
      if (dirtybuffer[offs])
      {
			dirtybuffer[offs] = 0;

			mx = (offs/2) % 32;
			my = (offs/2) / 32;
	      attr = videoram[offs+1];
	      color = (attr & 0x30) >> 4;

			drawgfx(tmpbitmap,Machine->gfx[0],
				videoram[offs]+256*(attr & 0x03),
				3-color,attr & 0x04,attr&0x08,16*mx,16*my,
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
      code += 256 * ((flags >> 3) & 1);
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

			copybitmap(bitmap,tmpbitmap,0,0,0,0,&clip,TRANSPARENCY_THROUGH,8+(color*16));
      }
   }
}


/***************************************************************************

  Spy Hunter-specific video routines

***************************************************************************/

unsigned char *spyhunt_alpharam;
int spyhunt_alpharam_size;
int spyhunt_scrollx,spyhunt_scrolly;

static struct osd_bitmap *backbitmap;	/* spy hunter only for scrolling background */
static struct rectangle backclip;


void spyhunt_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	/* standard init */
   mcr3_vh_convert_color_prom(palette,colortable,color_prom);

   /* plus some colors for the alpha RAM */
   palette[(8*16)*3+0] = 0;
   palette[(8*16)*3+1] = 0;
   palette[(8*16)*3+2] = 0;
   palette[(8*16+1)*3+0] = 0;
   palette[(8*16+1)*3+1] = 255;
   palette[(8*16+1)*3+2] = 0;
   palette[(8*16+2)*3+0] = 0;
   palette[(8*16+2)*3+1] = 0;
   palette[(8*16+2)*3+2] = 255;
   palette[(8*16+3)*3+0] = 255;
   palette[(8*16+3)*3+1] = 255;
   palette[(8*16+3)*3+2] = 255;

   /* put them into the color table */
   colortable[8*16+0] = 8*16;
   colortable[8*16+1] = 8*16+1;
   colortable[8*16+2] = 8*16+2;
   colortable[8*16+3] = 8*16+3;
}


int spyhunt_vh_start(void)
{
	dirtybuffer = malloc (videoram_size);
	if (!dirtybuffer)
		return 1;
	memset(dirtybuffer,1,videoram_size);

	backclip.min_x = backclip.min_y = 0;
	backclip.max_x = 64*64 - 1;
	backclip.max_y = 32*32 - 1;

	backbitmap = osd_create_bitmap(64*64,32*32);
	if (!backbitmap)
	{
		free(dirtybuffer);
		return 1;
	}

	spyhunt_scrollx = spyhunt_scrolly = 0;

	return 0;
}


void spyhunt_vh_stop(void)
{
	osd_free_bitmap(backbitmap);
	free(dirtybuffer);
}


void spyhunt_vh_screenrefresh(struct osd_bitmap *bitmap)
{
   int offs;
   int mx,my;


   /* for every character in the Video RAM, check if it has been modified */
   /* since last time and update it accordingly. */
   for (offs = videoram_size - 1; offs >= 0; offs--)
   {
		if (dirtybuffer[offs])
		{
			int code = videoram[offs];

			dirtybuffer[offs] = 0;

			mx = (offs >> 4) & 0x3f;
			my = (offs & 0x0f) | ((offs >> 6) & 0x10);

			drawgfx(backbitmap,Machine->gfx[0],
				(code & 0x3f) | ((code & 0x80) >> 1),
				1,0,(code & 0x40),64*mx,32*my,
				&backclip,TRANSPARENCY_NONE,0);
		}
   }

   /* copy the character mapped graphics */
   {
		int scrollx,scrolly;

		scrollx = -spyhunt_scrollx * 2 - 16;
		scrolly = -spyhunt_scrolly * 2;

		copyscrollbitmap(bitmap,backbitmap,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}

   /* Draw the sprites. */
   for (offs = 0;offs < spriteram_size;offs += 4)
   {
      int code,flipx,flipy,sx,sy,flags;

      if (spriteram[offs] == 0)
			 continue;

      code = spriteram[offs+2] ^ 0x80;
      flags = spriteram[offs+1];
      flipx = flags & 0x10;
      flipy = flags & 0x20;
      sx = (spriteram[offs+3]-3)*2;
      sy = (241-spriteram[offs])*2;

      drawgfx(bitmap,Machine->gfx[1],
	      code,0,flipx,flipy,sx-16,sy,
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

			copybitmap(bitmap,tmpbitmap,0,0,0,0,&clip,TRANSPARENCY_THROUGH,8+16);
      }
   }

	/* render any characters on top */
   for (offs = spyhunt_alpharam_size - 1; offs >= 0; offs--)
   {
   	int ch = spyhunt_alpharam[offs];
   	if (ch)
   	{
	      mx = (offs) / 32;
	      my = (offs) % 32;

	      drawgfx(bitmap,Machine->gfx[2],
		      ch,
		      0,0,0,16*mx-16,16*my,
		      &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
