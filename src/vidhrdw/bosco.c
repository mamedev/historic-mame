/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define VIDEO_RAM_SIZE 0x400

unsigned char *bosco_videoram2,*bosco_colorram2;
unsigned char *bosco_radarx,*bosco_radary,*bosco_radarattr;
unsigned char *bosco_scrollx,*bosco_scrolly;
											/* to speed up video refresh */
static unsigned char *dirtybuffer2;	/* keep track of modified portions of the screen */
											/* to speed up video refresh */
static struct osd_bitmap *tmpbitmap1;



static struct rectangle visiblearea =
{
	0*8, 28*8-1,
	0*8, 28*8-1
};

static struct rectangle radarvisiblearea =
{
	28*8, 36*8-1,
	0*8, 28*8-1
};


void bosco_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2;


		bit0 = (color_prom[31-i] >> 0) & 0x01;
		bit1 = (color_prom[31-i] >> 1) & 0x01;
		bit2 = (color_prom[31-i] >> 2) & 0x01;
		palette[3*i] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (color_prom[31-i] >> 3) & 0x01;
		bit1 = (color_prom[31-i] >> 4) & 0x01;
		bit2 = (color_prom[31-i] >> 5) & 0x01;
		palette[3*i + 1] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = (color_prom[31-i] >> 6) & 0x01;
		bit2 = (color_prom[31-i] >> 7) & 0x01;
		palette[3*i + 2] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	}

	/* characters */
	for (i = 0;i < 32*4;i++)
		colortable[i] = 15 - (color_prom[i + 32] & 0x0f);
	/* sprites / radar */
	for (i = 32*4;i < 64*4;i++)
		colortable[i] = (15 - (color_prom[i + 32] & 0x0f)) + 0x10;


	/* now the stars */
	for (i = 32;i < 32 + 64;i++)
	{
		int bits;
		int map[4] = { 0x00, 0x88, 0xcc, 0xff };

		bits = ((i-32) >> 0) & 0x03;
		palette[3*i] = map[bits];
		bits = ((i-32) >> 2) & 0x03;
		palette[3*i + 1] = map[bits];
		bits = ((i-32) >> 4) & 0x03;
		palette[3*i + 2] = map[bits];
	}
}

int bosco_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((dirtybuffer2 = malloc(videoram_size)) == 0)
		return 1;
	memset(dirtybuffer2,1,videoram_size);

	if ((tmpbitmap1 = osd_create_bitmap(32*8,32*8)) == 0)
	{
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void bosco_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap1);
	free(dirtybuffer2);
	generic_vh_stop();
}



void bosco_videoram2_w(int offset,int data)
{
	if (bosco_videoram2[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		bosco_videoram2[offset] = data;
	}
}



void bosco_colorram2_w(int offset,int data)
{
	if (bosco_colorram2[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		bosco_colorram2[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void bosco_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,sx,sy;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size-1; offs >= 0;offs--)
	{
		if (dirtybuffer2[offs])
		{
			dirtybuffer2[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			drawgfx(tmpbitmap1,Machine->gfx[0],
					bosco_videoram2[offs],
					bosco_colorram2[offs] & 0x1f,
					!(bosco_colorram2[offs] & 0x40),bosco_colorram2[offs] & 0x80,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* update radar */
	for (sy = 0;sy < 32;sy++)
	{
		for (sx = 0;sx < 8;sx++)
		{
			offs = sy * 32 + sx;

			if (dirtybuffer[offs])
			{
				dirtybuffer[offs] = 0;

				drawgfx(tmpbitmap,Machine->gfx[1],
						videoram[offs],
						colorram[offs] & 0x1f,
						!(colorram[offs] & 0x40),colorram[offs] & 0x80,
						8 * ((sx ^ 4) + 28),8*(sy-2),
						&radarvisiblearea,TRANSPARENCY_NONE,0);
			}
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scrollx,scrolly;


		scrollx = -(*bosco_scrollx - 3);
		scrolly = -(*bosco_scrolly + 16);

		copyscrollbitmap(bitmap,tmpbitmap1,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* radar */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&radarvisiblearea,TRANSPARENCY_NONE,0);


	/* draw the sprites */
	for (offs = 0;offs < spriteram_size;offs += 2)
	{
		drawgfx(bitmap,Machine->gfx[2],
				spriteram[offs] >> 2,spriteram_2[offs + 1],
				spriteram[offs] & 1,spriteram[offs] & 2,
				spriteram[offs + 1] - 1,224 - spriteram_2[offs],
				&visiblearea,TRANSPARENCY_THROUGH,0);
	}

	/* draw the dots on the radar and the bullets */
	for (offs = 0; offs < 12; offs++)
	{
		int x,y;
		int color;
		int	attr;


		attr = bosco_radarattr[offs];

		x = bosco_radarx[offs] + 256 * (1 - (bosco_radarattr[offs] & 1));
		y = 238 - bosco_radary[offs];

		if (attr & 8)	/* Long bullets */
		{
			color = Machine->pens[1];
			switch ((attr & 6) >> 1)
			{
				case 0:		/* Diagonal Left to Right */
					if (x >= Machine->drv->visible_area.min_x &&
						x < (Machine->drv->visible_area.max_x - 3) &&
						y > (Machine->drv->visible_area.min_y + 3) &&
						y < Machine->drv->visible_area.max_y)
					{
						bitmap->line[y][x+3] = color;
						bitmap->line[y-1][x+3] = color;
						bitmap->line[y-1][x+2] = color;
						bitmap->line[y-2][x+2] = color;
						bitmap->line[y-2][x+1] = color;
						bitmap->line[y-3][x+1] = color;
					}
					break;
				case 1:		/* Diagonal Right to Left */
					if (x >= Machine->drv->visible_area.min_x &&
						x < (Machine->drv->visible_area.max_x - 3) &&
						y > (Machine->drv->visible_area.min_y + 3) &&
						y < Machine->drv->visible_area.max_y)
					{
						bitmap->line[y][x+1] = color;
						bitmap->line[y-1][x+1] = color;
						bitmap->line[y-1][x+2] = color;
						bitmap->line[y-2][x+2] = color;
						bitmap->line[y-2][x+3] = color;
						bitmap->line[y-3][x+3] = color;
					}
					break;
				case 2:		/* Up and Down */
					if (x >= Machine->drv->visible_area.min_x &&
						x < Machine->drv->visible_area.max_x &&
						y > (Machine->drv->visible_area.min_y + 3) &&
						y < Machine->drv->visible_area.max_y)
					{
						bitmap->line[y][x] = color;
						bitmap->line[y][x+1] = color;
						bitmap->line[y-1][x] = color;
						bitmap->line[y-1][x+1] = color;
						bitmap->line[y-2][x] = color;
						bitmap->line[y-2][x+1] = color;
						bitmap->line[y-3][x] = color;
						bitmap->line[y-3][x+1] = color;
					}
					break;
				case 3:		/* Left and Right */
					if (x >= Machine->drv->visible_area.min_x &&
						x < (Machine->drv->visible_area.max_x - 2) &&
						y > Machine->drv->visible_area.min_y &&
						y < Machine->drv->visible_area.max_y)
					{
						bitmap->line[y-1][x] = color;
						bitmap->line[y-1][x+1] = color;
						bitmap->line[y][x] = color;
						bitmap->line[y][x+1] = color;
						bitmap->line[y-1][x+2] = color;
						bitmap->line[y-1][x+3] = color;
						bitmap->line[y][x+2] = color;
						bitmap->line[y][x+3] = color;
					}
					break;
			}
		}
		else
		{
			color = Machine->pens[(bosco_radarattr[offs] >> 1) & 3];

			/* normal size dots */
			if (x >= Machine->drv->visible_area.min_x &&
				x < Machine->drv->visible_area.max_x &&
				y > Machine->drv->visible_area.min_y &&
				y <= Machine->drv->visible_area.max_y)
			{
				bitmap->line[y-1][x] = color;
				bitmap->line[y-1][x+1] = color;
				bitmap->line[y][x] = color;
				bitmap->line[y][x+1] = color;
			}
		}
	}
}
