/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define BGHEIGHT (64)

static unsigned char scrollreg[4];
static unsigned char bg1xpos,bg1ypos,bg2xpos,bg2ypos,bgcontrol;
static struct osd_bitmap *bgbitmap;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Moon Patrol has one 256x8 character palette PROM, one 32x8 background
  palette PROM, one 32x8 sprite palette PROM and one 256x4 sprite lookup
  table PROM.

  The character and background palette PROMs are connected to the RGB output
  this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

  The sprite palette PROM is connected to the RGB output this way. Note that
  RED and BLUE are swapped wrt the usual configuration.

  bit 7 -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
  bit 0 -- 1  kohm resistor  -- BLUE

***************************************************************************/
void mpatrol_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	/* character palette */
	for (i = 0;i < 128;i++)
	{
		int bit0,bit1,bit2;


		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		color_prom++;
	}

	color_prom += 128;	/* skip the bottom half of the PROM - not used */
	/* color_prom now points to the beginning of the background palette */


	/* character lookup table */
	for (i = 0;i < TOTAL_COLORS(0)/2;i++)
	{
		COLOR(0,i) = i;

		/* also create a color code with transparent pen 0 */
		if (i % 4 == 0)	COLOR(0,i + TOTAL_COLORS(0)/2) = 0;
		else COLOR(0,i + TOTAL_COLORS(0)/2) = i;
	}


	/* background palette */
	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2;


		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		color_prom++;
	}

	/* color_prom now points to the beginning of the sprite palette */

	/* sprite palette */
	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2;


		/* red component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		color_prom++;
	}

	/* color_prom now points to the beginning of the sprite lookup table */

	/* sprite lookup table */
	for (i = 0;i < TOTAL_COLORS(1);i++)
	{
		COLOR(1,i) = 128+32 + (*color_prom++);
		if (i % 4 == 3) color_prom += 4;	/* half of the PROM is unused */
	}

	color_prom += 128;	/* skip the bottom half of the PROM - not used */

	/* background */
	/* the palette is a 32x8 PROM with many colors repeated. The address of */
	/* the colors to pick is as follows: */
	/* xbb00: mountains */
	/* 0xxbb: hills */
	/* 1xxbb: city */
	COLOR(2,0) = 128;
	COLOR(2,1) = 128+4;
	COLOR(2,2) = 128+8;
	COLOR(2,3) = 128+12;
	COLOR(4,0) = 128;
	COLOR(4,1) = 128+1;
	COLOR(4,2) = 128+2;
	COLOR(4,3) = 128+3;
	COLOR(6,0) = 128;
	COLOR(6,1) = 128+16+1;
	COLOR(6,2) = 128+16+2;
	COLOR(6,3) = 128+16+3;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
int mpatrol_vh_start(void)
{
	int i,j,k;


	if (generic_vh_start() != 0)
		return 1;

	/* temp bitmap for the three background images */
	if ((bgbitmap = osd_create_bitmap(256,BGHEIGHT*3)) == 0)
	{
		generic_vh_stop();
		return 1;
	}

	/* prepare the background graphics */
	for (i = 0;i < 3;i++)
	{
		for (j = 0;j < 8;j++)
			for (k = 0;k < 2;k++)
				drawgfx(bgbitmap,Machine->gfx[2 + 2 * i + k],
						j,0,
						0,0,
						32 * j,BGHEIGHT * i + (BGHEIGHT / 2) * k,
						0,TRANSPARENCY_NONE,0);

		for (j = 0;j < BGHEIGHT-64;j++)
			memset(bgbitmap->line[BGHEIGHT*i + 64 + j],Machine->gfx[2+i]->colortable[3],256);
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void mpatrol_vh_stop(void)
{
	osd_free_bitmap(bgbitmap);
	generic_vh_stop();
}



void mpatrol_scroll_w(int offset,int data)
{
	scrollreg[offset] = data;
}



void mpatrol_bg1xpos_w(int offset,int data)
{
	bg1xpos = data;
}



void mpatrol_bg1ypos_w(int offset,int data)
{
	bg1ypos = data;
}



void mpatrol_bg2xpos_w(int offset,int data)
{
	bg2xpos = data;
}



void mpatrol_bg2ypos_w(int offset,int data)
{
	bg2ypos = data;
}



void mpatrol_bgcontrol_w(int offset,int data)
{
	bgcontrol = data;
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void mpatrol_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs,i;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,color;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			color = colorram[offs] & 0x1f;
			if (sy >= 7) color += 32;	/* lines 7-31 are transparent */

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + 2 * (colorram[offs] & 0x80),
					color,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the background */
	if (bgcontrol == 0x04)
	{
		struct rectangle clip;


		clip.min_x = Machine->drv->visible_area.min_x;
		clip.max_x = Machine->drv->visible_area.max_x;

		clip.min_y = 7*8;
		clip.max_y = bg2ypos-1;
		fillbitmap(bitmap,Machine->pens[0],&clip);

		clip.min_y = bg2ypos;
		clip.max_y = bg2ypos + BGHEIGHT-1;
		copybitmap(bitmap,bgbitmap,0,0,bg2xpos,bg2ypos,&clip,TRANSPARENCY_NONE,0);
		copybitmap(bitmap,bgbitmap,0,0,bg2xpos - 256,bg2ypos,&clip,TRANSPARENCY_NONE,0);

		clip.min_y = bg2ypos + BGHEIGHT;
		clip.max_y = bg1ypos + BGHEIGHT-1;
		fillbitmap(bitmap,Machine->gfx[2]->colortable[3],&clip);

		clip.min_y = bg1ypos;
		clip.max_y = bg1ypos + BGHEIGHT-1;
		copybitmap(bitmap,bgbitmap,0,0,bg1xpos,bg1ypos-BGHEIGHT,&clip,TRANSPARENCY_COLOR,128);
		copybitmap(bitmap,bgbitmap,0,0,bg1xpos - 256,bg1ypos-BGHEIGHT,&clip,TRANSPARENCY_COLOR,128);

		clip.min_y = bg1ypos + BGHEIGHT;
		clip.max_y = Machine->drv->visible_area.max_y;
		fillbitmap(bitmap,Machine->gfx[4]->colortable[3],&clip);
	}
	else if (bgcontrol == 0x03)
	{
		struct rectangle clip;


		clip.min_x = Machine->drv->visible_area.min_x;
		clip.max_x = Machine->drv->visible_area.max_x;

		clip.min_y = 7*8;
		clip.max_y = bg2ypos-1;
		fillbitmap(bitmap,Machine->pens[0],&clip);

		clip.min_y = bg2ypos;
		clip.max_y = bg2ypos + BGHEIGHT-1;
		copybitmap(bitmap,bgbitmap,0,0,bg2xpos,bg2ypos,&clip,TRANSPARENCY_NONE,0);
		copybitmap(bitmap,bgbitmap,0,0,bg2xpos - 256,bg2ypos,&clip,TRANSPARENCY_NONE,0);

		clip.min_y = bg2ypos + BGHEIGHT;
		clip.max_y = bg1ypos + BGHEIGHT-1;
		fillbitmap(bitmap,Machine->gfx[2]->colortable[3],&clip);

		clip.min_y = bg1ypos;
		clip.max_y = bg1ypos + BGHEIGHT-1;
		copybitmap(bitmap,bgbitmap,0,0,bg1xpos,bg1ypos-BGHEIGHT*2,&clip,TRANSPARENCY_COLOR,128);
		copybitmap(bitmap,bgbitmap,0,0,bg1xpos - 256,bg1ypos-BGHEIGHT*2,&clip,TRANSPARENCY_COLOR,128);

		clip.min_y = bg1ypos + BGHEIGHT;
		clip.max_y = Machine->drv->visible_area.max_y;
		fillbitmap(bitmap,Machine->gfx[6]->colortable[3],&clip);
	}
	else fillbitmap(bitmap,Machine->pens[0],&Machine->drv->visible_area);


	/* copy the temporary bitmap to the screen */
	{
		int scroll[32];
		struct rectangle clip;


		clip.min_x = Machine->drv->visible_area.min_x;
		clip.max_x = Machine->drv->visible_area.max_x;

		clip.min_y = 0;
		clip.max_y = 7 * 8 - 1;
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&clip,TRANSPARENCY_NONE,0);

		clip.min_y = 7 * 8;
		clip.max_y = 32 * 8 - 1;

		for (i = 0;i < 24;i++)
			scroll[i] = 0;
		for (i = 24;i < 32;i++)
			scroll[i] = scrollreg[0];

		copyscrollbitmap(bitmap,tmpbitmap,32,scroll,0,0,&clip,TRANSPARENCY_COLOR,0);
	}


	/* Draw the sprites. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		drawgfx(bitmap,Machine->gfx[1],
				spriteram_2[offs + 2],
				spriteram_2[offs + 1] & 0x3f,
				spriteram_2[offs + 1] & 0x40,spriteram_2[offs + 1] & 0x80,
				spriteram_2[offs + 3],241 - spriteram_2[offs],
				&Machine->drv->visible_area,TRANSPARENCY_COLOR,128+32);
	}
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		drawgfx(bitmap,Machine->gfx[1],
				spriteram[offs + 2],
				spriteram[offs + 1] & 0x3f,
				spriteram[offs + 1] & 0x40,spriteram[offs + 1] & 0x80,
				spriteram[offs + 3],241 - spriteram[offs],
				&Machine->drv->visible_area,TRANSPARENCY_COLOR,128+32);
	}
}
