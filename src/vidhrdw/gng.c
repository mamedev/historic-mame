/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6809.h"



#define GFX_CHAR 0
#define GFX_TILE 1
#define GFX_SPRITE 5

unsigned char *gng_paletteram;
static const unsigned char *colors;
static unsigned char dirtycolor[8];	/* keep track of modified background colors */

unsigned char *gng_bgvideoram,*gng_bgcolorram;
int gng_bgvideoram_size;
unsigned char *gng_scrollx, *gng_scrolly;
static unsigned char *dirtybuffer2;
static unsigned char *spritebuffer1,*spritebuffer2;
static struct osd_bitmap *tmpbitmap2;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Ghosts 'n Goblins doesn't have color PROMs, it uses RAM instead.

  I don't know the exact values of the resistors between the RAM and the
  RGB output. I assumed these values (the same as Commando)
  bit 7 -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
        -- 1  kohm resistor  -- RED
        -- 2.2kohm resistor  -- RED
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
  bit 0 -- 2.2kohm resistor  -- GREEN

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 1  kohm resistor  -- BLUE
        -- 2.2kohm resistor  -- BLUE
        -- unused
        -- unused
        -- unused
  bit 0 -- unused

***************************************************************************/
void gng_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;
	#define COLOR(gfx,offs) (colortable[Machine->drv->gfxdecodeinfo[gfx].color_codes_start + offs])


	colors = color_prom;	/* we'll need the colors later to dynamically remap the characters */

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;


		bit0 = (color_prom[2*i] >> 4) & 0x01;
		bit1 = (color_prom[2*i] >> 5) & 0x01;
		bit2 = (color_prom[2*i] >> 6) & 0x01;
		bit3 = (color_prom[2*i] >> 7) & 0x01;
		palette[3*i] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[2*i] >> 0) & 0x01;
		bit1 = (color_prom[2*i] >> 1) & 0x01;
		bit2 = (color_prom[2*i] >> 2) & 0x01;
		bit3 = (color_prom[2*i] >> 3) & 0x01;
		palette[3*i + 1] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[2*i+1] >> 4) & 0x01;
		bit1 = (color_prom[2*i+1] >> 5) & 0x01;
		bit2 = (color_prom[2*i+1] >> 6) & 0x01;
		bit3 = (color_prom[2*i+1] >> 7) & 0x01;
		palette[3*i + 2] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
	}

	/* initialize the colortable so the power on self test can be seen */
	for (i = 0;i < Machine->drv->color_table_len;i++)
	{
		int j;


		j = rand() % Machine->drv->total_colors;
		gng_paletteram[i] = colors[2*j];
		gng_paletteram[i + 0x100] = colors[2*j+1];
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int gng_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((dirtybuffer2 = malloc(gng_bgvideoram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
	memset(dirtybuffer2,0,gng_bgvideoram_size);

	if ((spritebuffer1 = malloc(spriteram_size)) == 0)
	{
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	if ((spritebuffer2 = malloc(spriteram_size)) == 0)
	{
		free(spritebuffer1);
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	/* the background area is twice as tall and twice as large as the screen */
	if ((tmpbitmap2 = osd_create_bitmap(2*Machine->drv->screen_width,2*Machine->drv->screen_height)) == 0)
	{
		free(spritebuffer2);
		free(spritebuffer1);
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void gng_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap2);
	free(spritebuffer2);
	free(spritebuffer1);
	free(dirtybuffer2);
	generic_vh_stop();
}



void gng_bgvideoram_w(int offset,int data)
{
	if (gng_bgvideoram[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		gng_bgvideoram[offset] = data;
	}
}



void gng_bgcolorram_w(int offset,int data)
{
	if (gng_bgcolorram[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		gng_bgcolorram[offset] = data;
	}
}



void gng_paletteram_w(int offset,int data)
{
	if (gng_paletteram[offset] != data)
	{
		if ((offset & 0xff) < 64)
			dirtycolor[(offset & 0xff) / 8] = 1;

		gng_paletteram[offset] = data;
	}
}



int gng_interrupt(void)
{
	/* we must store previous sprite data in a buffer and draw that instead of */
	/* the latest one, otherwise sprites will not be synchronized with */
	/* background scrolling */
	memcpy(spritebuffer2,spritebuffer1,spriteram_size);
	memcpy(spritebuffer1,spriteram,spriteram_size);

	return INT_IRQ;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void gng_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int i,j,offs;


	/* rebuild the color lookup table */
	for (j = 0;j < 3;j++)
	{
		int conv[3] = {GFX_TILE, GFX_SPRITE, GFX_CHAR };
		/*
        00-3f:  background palettes. (8x8 colours)
        40-7f:  sprites palettes. (4*16 colours)
        80-bf:  characters palettes (16*4 colours)
		*/

		for (i = 0;i < 64;i++)
		{
			offs = Machine->drv->total_colors - 1;
			while (offs > 0)
			{
				if (gng_paletteram[64*j+i] == colors[2*offs] &&
						(gng_paletteram[64*j+i + 0x100] & 0xf0) == colors[2*offs+1])
					break;

				offs--;
			}

if (errorlog && offs == 0 && (gng_paletteram[64*j+i] || gng_paletteram[64*j+i + 0x100]))
	fprintf(errorlog,"warning: unknown color %02x %02x\n",
			gng_paletteram[64*j+i],gng_paletteram[64*j+i + 0x100]);

			Machine->gfx[conv[j]]->colortable[i] = Machine->pens[offs];
		}
	}


	for (offs = gng_bgvideoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy;


		if (dirtybuffer2[offs] || dirtycolor[gng_bgcolorram[offs] & 0x07])
		{
			dirtybuffer2[offs] = 0;

			sx = offs / 32;
			sy = offs % 32;

			drawgfx(tmpbitmap2,Machine->gfx[GFX_TILE + ((gng_bgcolorram[offs] >> 6) & 0x03)],
					gng_bgvideoram[offs],
					gng_bgcolorram[offs] & 0x07,
					gng_bgcolorram[offs] & 0x10,gng_bgcolorram[offs] & 0x20,
					16 * sx,16 * sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	for (i = 0;i < 8;i++)
		dirtycolor[i] = 0;


	/* copy the background graphics */
	{
		int scrollx,scrolly;


		scrollx = -(gng_scrollx[0] + 256 * gng_scrollx[1]);
		scrolly = -(gng_scrolly[0] + 256 * gng_scrolly[1]);

		copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int bank;


		/* the meaning of bit 1 of [offs+1] is unknown */

		bank = ((spritebuffer2[offs + 1] >> 6) & 3);

		if (bank < 3)
			drawgfx(bitmap,Machine->gfx[GFX_SPRITE + bank],
					spritebuffer2[offs],(spritebuffer2[offs + 1] >> 4) & 3,
					spritebuffer2[offs + 1] & 0x04,spritebuffer2[offs + 1] & 0x08,
					spritebuffer2[offs + 3] - 0x100 * (spritebuffer2[offs + 1] & 0x01),spritebuffer2[offs + 2],
					&Machine->drv->visible_area,TRANSPARENCY_PEN,15);
	}


	/* redraw the background tiles which have priority over sprites */
	{
		int scrollx,scrolly;


		scrollx = -(gng_scrollx[0] + 256 * gng_scrollx[1]);
		scrolly = -(gng_scrolly[0] + 256 * gng_scrolly[1]);

		for (offs = gng_bgvideoram_size - 1;offs >= 0;offs--)
		{
			int sx,sy;


			if (gng_bgcolorram[offs] & 0x08)
			{
				sx = ((16 * (offs / 32) + scrollx + 16) & 0x1ff) - 16;
				sy = ((16 * (offs % 32) + scrolly + 16) & 0x1ff) - 16;

				drawgfx(bitmap,Machine->gfx[GFX_TILE + ((gng_bgcolorram[offs] >> 6) & 0x03)],
						gng_bgvideoram[offs],
						gng_bgcolorram[offs] & 0x07,
						gng_bgcolorram[offs] & 0x10,gng_bgcolorram[offs] & 0x20,
						sx,sy,
						&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			}
		}
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int charcode;


		charcode = videoram[offs] + 4 * (colorram[offs] & 0x40);

		if (charcode != 0x20)	/* don't draw spaces */
		{
			int sx,sy;


			sx = 8 * (offs % 32);
			sy = 8 * (offs / 32);

			drawgfx(bitmap,Machine->gfx[GFX_CHAR],
					charcode,
					colorram[offs] & 0x0f,
					0,0,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,3);
		}
	}
}
