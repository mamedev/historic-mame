/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "M6809.h"



#define VIDEO_RAM_SIZE 0x400
#define BACKGROUND_SIZE 0x400
#define SPRITES_SIZE (96*4)

#define GFX_CHAR 1
#define GFX_TILE 2
#define GFX_SPRITE 6
#define GFX_PALETTE 9

unsigned char *gng_paletteram;
static unsigned char dirtycolor[8];	/* keep track of modified background colors */

unsigned char *gng_bgvideoram,*gng_bgcolorram;
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


	for (i = 0;i < 256;i++)
	{
		int bits;


		bits = (i >> 0) & 0x07;
		palette[3*i + 0] = (bits >> 1) | (bits << 2) | (bits << 5);
		bits = (i >> 3) & 0x07;
		palette[3*i + 1] = (bits >> 1) | (bits << 2) | (bits << 5);
		bits = (i >> 6) & 0x03;
		palette[3*i + 2] = bits | (bits >> 2) | (bits << 4) | (bits << 6);
	}


	for (i = 0;i < 256;i++)
		COLOR(GFX_PALETTE,i) = i;


	/* set up colors for the copyright notice - the other colors will be */
	/* set later by the game. */
	for (i = 0;i < 3*4;i++) COLOR(0,i) = 0x00;	/* set all to black */
	COLOR(0,1) = 0xff;	/* white */
	COLOR(0,4+1) = 0x3f;	/* yellow */
	COLOR(0,2*4+1) = 0x07;	/* red */
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int gng_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((dirtybuffer2 = malloc(BACKGROUND_SIZE)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
	memset(dirtybuffer,0,BACKGROUND_SIZE);

	if ((spritebuffer1 = malloc(SPRITES_SIZE)) == 0)
	{
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	if ((spritebuffer2 = malloc(SPRITES_SIZE)) == 0)
	{
		free(spritebuffer1);
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	/* the background area is twice as tall and twice as large as the screen */
	if ((tmpbitmap2 = osd_create_bitmap(2*Machine->drv->screen_width,2*Machine->drv->screen_width)) == 0)
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
	memcpy(spritebuffer2,spritebuffer1,SPRITES_SIZE);
	memcpy(spritebuffer1,spriteram,SPRITES_SIZE);

	return INT_IRQ;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void gng_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int i,j,col,offs;


	/* rebuild the color lookup table */
	for (j = 0;j < 3;j++)
	{
                static char used[16*16*16];
                static int total;

		int conv[3] = {GFX_TILE, GFX_SPRITE, GFX_CHAR };
		/*
                        00-3f:  background palettes. (8x8 colours)
                        40-7f:  sprites palettes. (4*16 colours)
                        80-bf:  characters palettes (16*4 colours)
		*/

		for (i = 0;i < 64;i++)
		{
                        col = gng_paletteram[64*j+i] + 256*(gng_paletteram[64*j+i + 0x100]>>4);
                        if (used[col] == 0 && errorlog)
                        {
	                        used[col]++;
	                        total++;
                                fprintf(errorlog,"used: %d\n",total);
	                        for (i = 0;i < 16*16*16;i++)
	                        {
		                        if (used[i]) fprintf(errorlog,"0x%02x,0x%02x,",i & 0xff,(i >> 8) << 4);
	                        }
	                        fprintf(errorlog,"\n");
                        }

			col = (gng_paletteram[64*j+i] >> 5) & 0x07;	/* red component */
			col |= (gng_paletteram[64*j+i] << 2) & 0x38;	/* green component */
			col |= (gng_paletteram[64*j+i + 0x100]) & 0xc0;	/* blue component */

			Machine->gfx[conv[j]]->colortable[i] = Machine->gfx[GFX_PALETTE]->colortable[col];
		}
	}


	for (offs = 0;offs < BACKGROUND_SIZE;offs++)
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
	for (offs = 95*4;offs >= 0;offs -= 4)
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


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
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
