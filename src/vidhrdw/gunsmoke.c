/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *gunsmoke_bg_scrolly;
unsigned char *gunsmoke_bg_scrollx;

static struct osd_bitmap * bgbitmap;
static unsigned char bgmap[9][9][2];


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Exed Exes has three 256x4 palette PROMs (one per gun) and three 256x4 lookup
  table PROMs (one for characters, one for sprites, one for background tiles).
  The palette PROMs are connected to the RGB output this way:

  bit 3 -- 220 ohm resistor  -- RED/GREEN/BLUE
        -- 470 ohm resistor  -- RED/GREEN/BLUE
        -- 1  kohm resistor  -- RED/GREEN/BLUE
  bit 0 -- 2.2kohm resistor  -- RED/GREEN/BLUE

  However, seing as this isn't Exed Exes the format of the colour lookup
  table may be different. In particular, there are no 16*16 upper layer
  tiles. This needs to be rewritten when the PROMS become available.

***************************************************************************/
void gunsmoke_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;

	for (i = 0;i < 256;i++)
	{
		int bit0,bit1,bit2,bit3;

		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		bit3 = (color_prom[i] >> 3) & 0x01;
		palette[3*i] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[i+256] >> 0) & 0x01;
		bit1 = (color_prom[i+256] >> 1) & 0x01;
		bit2 = (color_prom[i+256] >> 2) & 0x01;
		bit3 = (color_prom[i+256] >> 3) & 0x01;
		palette[3*i + 1] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[i+256*2] >> 0) & 0x01;
		bit1 = (color_prom[i+256*2] >> 1) & 0x01;
		bit2 = (color_prom[i+256*2] >> 2) & 0x01;
		bit3 = (color_prom[i+256*2] >> 3) & 0x01;
		palette[3*i + 2] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
	}

	/* characters use colors 192-207 */
	for (i = 0;i < 64*4;i++)
		colortable[i] = color_prom[i + 256*3] + 192;

        /* 32x32 tiles use colors 0-15 */
	for (i = 64*4;i < 2*64*4;i++)
		colortable[i] = color_prom[i + 256*3];

        /* 16x16 tiles use colors 64-79 . There are no 16*16 tiles in gunsmoke */
	for (i = 2*64*4;i < 2*64*4+16*16;i++)
		colortable[i] = color_prom[i + 256*3] + 64;

	/* sprites use colors 128-192 in four banks */
	for (i = 2*64*4+16*16;i < 2*64*4+16*16+16*16;i++)
		colortable[i] = color_prom[i + 256*3] + 128 + 16 * color_prom[i + 256*3 + 256];
}



int gunsmoke_vh_start(void)
{
	if ((bgbitmap = osd_create_bitmap(9*32,9*32)) == 0)
		return 1;
	
	if (generic_vh_start() == 1)
	{
		osd_free_bitmap(bgbitmap);
		return 1;
	}
	
	memset (bgmap, 0xff, sizeof (bgmap));

	return 0;
}


void gunsmoke_vh_stop(void)
{
	osd_free_bitmap(bgbitmap);
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void gunsmoke_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,sx,sy;
	int bg_scrolly, bg_scrollx;
	unsigned char *p=Machine->memory_region[3];
	int top,left,xscroll,yscroll;


	bg_scrolly = gunsmoke_bg_scrolly[0] + 256 * gunsmoke_bg_scrolly[1];
	bg_scrollx = gunsmoke_bg_scrollx[0];
	offs = 16 * ((bg_scrolly>>5)+8)+2*(bg_scrollx>>5) ;
	if (bg_scrollx & 0x80) offs -= 0x10;

	top = 8 - (bg_scrolly>>5) % 9;
	left = (bg_scrollx>>5) % 9;

	bg_scrolly&=0x1f;
	bg_scrollx&=0x1f;

	for (sy = 0;sy <9;sy++)
	{
		int ty = (sy + top) % 9;
		offs &= 0x7fff; /* Enforce limits (for top of scroll) */
		
		for (sx = 0;sx < 9;sx++)
		{
			int tile, attr, offset;
			int tx = (sx + left) % 9;
			unsigned char *map = &bgmap[ty][tx][0];
			offset=offs+(sx*2);

			tile=p[offset];
			attr=p[offset+1];
			
			if (tile != map[0] || attr != map[1])
			{
				map[0] = tile;
				map[1] = attr;
				tile+=256*(attr&0x01);
				drawgfx(bgbitmap,Machine->gfx[1],
						tile,
						(attr & 0x3c) >> 2,
						attr & 0x80,attr & 0x40,
						tx*32, ty*32,
						0,
						TRANSPARENCY_NONE,0);
			}
			map += 2;
		}
		offs-=0x10;
	}

	xscroll = -(left*32+bg_scrollx);
	yscroll = -(top*32+32-bg_scrolly);
	copyscrollbitmap(bitmap,bgbitmap,
		1,&xscroll,
		1,&yscroll,
		&Machine->drv->visible_area,
		TRANSPARENCY_NONE,0);

	/* Draw the entire background scroll */
#if 0
/* TODO: this is very slow, have to optimize it using a temporary bitmap */
	for (sy = 0;sy <9;sy++)
	{
		offs &= 0x7fff; /* Enforce limits (for top of scroll) */
		for (sx = 0;sx < 9;sx++)
		{
			int tile, attr, offset;
			offset=offs+(sx*2);

			tile=p[offset];
			attr=p[offset+1];
			tile+=256*(attr&0x01);
			drawgfx(bitmap,Machine->gfx[1],
					tile,
					(attr & 0x03c) >> 2,
					attr & 0x80,attr & 0x40,
					sx*32-bg_scrollx, sy*32+bg_scrolly-32,
					&Machine->drv->visible_area,
					TRANSPARENCY_NONE,0);
		}
		offs-=0x10;
	}
#endif


	/* Draw the sprites. */
	for (offs = spriteram_size - 32;offs >= 0;offs -= 32)
	{
		int sprite=spriteram[offs + 1]&0xc0;
		sprite<<=2;
		drawgfx(bitmap,Machine->gfx[2],
				spriteram[offs]+sprite,
				spriteram[offs + 1] & 0x0f,
				spriteram[offs + 1] & 0x10, spriteram[offs + 1] & 0x20,
				spriteram[offs + 2],240 - spriteram[offs + 3],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int code;


		code = videoram[offs] + ((colorram[offs] & 0xc0) << 2);

		if (code != 0x24 || colorram[offs] != 0)		/* don't draw spaces */
		{
			int sx,sy;


			sx = offs / 32;
			sy = 31 - offs % 32;

			drawgfx(bitmap,Machine->gfx[0],
					code,
					colorram[offs] & 0x0f,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_COLOR,207);
		}
	}
}
