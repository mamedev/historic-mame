/***************************************************************************

  vidhrdw/mcr68.c

  Xenophobe video hardware very similar to Rampage.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *mcr68_paletteram,*mcr68_spriteram;

static char sprite_transparency[512]; /* no mcr3 game has more than this many sprites */

/* Same as Rampage */
void mcr68_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	struct osd_bitmap *bitmap = Machine->gfx[0]->gfxdata;
	int y, x, i;

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

	/* the tile graphics are reverse colors; we preswap them here */
	for (y = bitmap->height - 1; y >= 0; y--)
		for (x = bitmap->width - 1; x >= 0; x--)
			bitmap->line[y][x] ^= 15;
}

/******************************************************************************/

int mcr68_palette_r(int offset)
{
	return READ_WORD(&mcr68_paletteram[offset]);
}

void mcr68_palette_w(int offset,int data)
{
	int r,g,b;

	WRITE_WORD(&mcr68_paletteram[offset],data);

	r = ((offset & 1) << 2) + (data >> 6);
	g = (data >> 0) & 7;
	b = (data >> 3) & 7;

	/* up to 8 bits */
	r = (r << 5) | (r << 2) | (r >> 1);
	g = (g << 5) | (g << 2) | (g >> 1);
	b = (b << 5) | (b << 2) | (b >> 1);

	palette_change_color(offset/2,r,g,b);
}

void mcr68_videoram_w(int offset,int data)
{
	dirtybuffer[offset] = 1;
	WRITE_WORD(&videoram[offset],data);
}

int mcr68_videoram_r(int offset)
{
	return READ_WORD(&videoram[offset]);
}

/******************************************************************************/

void xenophobe_vh_screenrefresh(struct osd_bitmap *bitmap)
{
   int offs;
   int mx,my;
   int tile,attr;
   int color;

   /* for every character in the Video RAM, check if it has been modified */
   /* since last time and update it accordingly. */
   for (offs = videoram_size - 4;offs >= 0;offs -= 4)
   {
      if (dirtybuffer[offs])
      {
				dirtybuffer[offs] = 0;
				mx = (offs/4) % 32;
				my = (offs/4) / 32;
				tile=READ_WORD(&videoram[offs])&0xff;
				attr=READ_WORD(&videoram[offs+2])&0xff;

				tile=tile+(256*(attr&0x3));

				if (attr&0x40) tile+=1024;  // CHECK NUMBER OF TILES>>>>

				color = (attr & 0x30) >> 4;
				drawgfx(tmpbitmap,Machine->gfx[0],
					tile,
					3-color,attr & 0x04,attr&0x08,16*mx,16*my,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
      }
		}

		/* copy the character mapped graphics */
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

   	/* Draw the sprites. */
		for (offs = 0; offs < 0x800; offs += 8)
   	{
   		int code,flipx,flipy,sx,sy,flags;

     	code=(READ_WORD(&mcr68_spriteram[offs+4]))&0xff;
			flags=(READ_WORD(&mcr68_spriteram[offs+2]))&0xff;

			code += 256 * ((flags >> 3) & 1);

			if (flags&0x80) code+=1024;

      if (!code)
				continue;

			color = 3 - (flags & 0x03);
      flipx = flags & 0x10;
      flipy = flags & 0x20;
      // check....
			sx=((READ_WORD(&mcr68_spriteram[offs+6])&0xff)-2)*2;
			sy=(240-(READ_WORD(&mcr68_spriteram[offs])&0xff))*2;

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

			copybitmap(bitmap,tmpbitmap,0,0,0,0,&clip,TRANSPARENCY_THROUGH,Machine->pens[8+(color*16)]);
      }
   }

		/* Redraw tiles with priority over sprites */
		for (offs = videoram_size - 4;offs >= 0;offs -= 4)
   	{
			mx = (offs/4) % 32;
			my = (offs/4) / 32;
			tile=READ_WORD(&videoram[offs])&0xff;
			attr=READ_WORD(&videoram[offs+2])&0xff;

			tile=tile+(256*(attr&0x3));
			if (attr&0x40) tile+=1024;

			/* MSB of tile */
			if (!(attr&0x80)) continue;

			color = (attr & 0x30) >> 4;
			drawgfx(bitmap,Machine->gfx[0],
				tile,
				3-color,attr & 0x04,attr&0x08,16*mx,16*my,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
}

