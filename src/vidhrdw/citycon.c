/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *citycon_paletteram,*citycon_charlookup;
unsigned char *citycon_scroll;
static struct osd_bitmap *tmpbitmap2;
static int bg_image,dirty_background,dirtypalette;
static unsigned char dirtylookup[32];
static int flipscreen;



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int citycon_vh_start(void)
{
	if ((dirtybuffer = malloc(videoram_size)) == 0)
		return 1;
	memset(dirtybuffer,1,videoram_size);

	dirty_background = 1;
	dirtypalette = 1;

	/* CityConnection has a virtual screen 4 times as large as the visible screen */
	if ((tmpbitmap = osd_new_bitmap(4 * Machine->drv->screen_width,Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
	{
		free(dirtybuffer);
		return 1;
	}

	/* And another one for background */
	if ((tmpbitmap2 = osd_new_bitmap(4 * Machine->drv->screen_width,Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
	{
                osd_free_bitmap(tmpbitmap);
		free(dirtybuffer);
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void citycon_vh_stop(void)
{
	free(dirtybuffer);
	osd_free_bitmap(tmpbitmap);
	osd_free_bitmap(tmpbitmap2);
}



void citycon_paletteram_w(int offset,int data)
{
	if (citycon_paletteram[offset] != data)
	{
		citycon_paletteram[offset] = data;

		dirtypalette = 1;
	}
}



void citycon_charlookup_w(int offset,int data)
{
	if (citycon_charlookup[offset] != data)
	{
		citycon_charlookup[offset] = data;

		dirtylookup[offset / 8] = 1;
	}
}



void citycon_background_w(int offset,int data)
{
	/* bits 4-7 control the background image */
	if (bg_image != (data >> 4))
	{
		bg_image = data >> 4;
		dirty_background = 1;
	}

	/* bit 0 flips screen */
	/* maybe it is also used to multiplex player 1 and player 2 controls */
	if (flipscreen != (data & 1))
	{
		flipscreen = data & 1;
		memset(dirtybuffer,1,videoram_size);
		dirty_background = 1;
	}

	/* bits 1-3 are unknown */
if (errorlog && (data & 0x0e) != 0) fprintf(errorlog,"background register = %02x\n",data);
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void citycon_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;
	int j, i;

	/* rebuild the colour lookup table from RAM palette */
	if (dirtypalette)
	{
		for (j=0; j<3; j++)
		{
			/*
			0000-01ff:  sprites palettes   (16x16 colours)
			0200-03ff:  background palette (16x16 colours)
			0400-04ff:  characters palette (32x4 colours)
			*/
			/* CHARS  SPRITES TILES   */
			int start[3]={0x0400, 0x0000, 0x0200};
			int count[3]={0x0080, 0x0100, 0x0100};
			int base=start[j];
			int max=count[j];


			for (i=0; i<max; i++)
			{
				int red, green, blue, redgreen;

				redgreen=citycon_paletteram[base + 2*i];
				red=redgreen >>4;
				green=redgreen & 0x0f ;
				blue=citycon_paletteram[base + 2*i + 1]>>4;

				red = (red << 4) + red;
				green = (green << 4) + green;
				blue = (blue << 4) + blue;

				if (j == 0)
				{
					if (i % 4 == 0) red = green = blue = 0;	/* ensure transparency */
					else if (!red && !green && !blue) red = 0x20;	/* avoid transparency */
					setgfxcolorentry (Machine->gfx[2*j],i,red,green,blue);
				}
				else
					setgfxcolorentry (Machine->gfx[2*j],i,red,green,blue);
			}
		}
	}


	/* Create the background */
	if (dirty_background || dirtypalette)
	{
		dirty_background = 0;

		for (offs = videoram_size - 1;offs >= 0;offs--)
		{
			int sx,sy,code;


			sy = offs / 32;
			sx = (offs % 32) + (sy & 0x60);
			sy = sy & 31;
			if (flipscreen)
			{
				sx = 127 - sx;
				sy = 31 - sy;
			}

			code = Machine->memory_region[2][0x1000 * bg_image + offs];

			drawgfx(tmpbitmap2,Machine->gfx[3 + bg_image],
					code,
					Machine->memory_region[2][0xc000 + 0x100 * bg_image + code],
					flipscreen,flipscreen,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the temporary bitmap to the screen */
	{
		int scroll;

		if (flipscreen)
			scroll = 256 + ((citycon_scroll[0]*256+citycon_scroll[1]) >> 1);
		else
			scroll = -((citycon_scroll[0]*256+citycon_scroll[1]) >> 1);

		copyscrollbitmap(bitmap,tmpbitmap2,1,&scroll,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy;


		sy = offs / 32;
		sx = (offs % 32) + (sy & 0x60);
		sy = sy & 0x1f;

		if (dirtybuffer[offs] || dirtylookup[sy] || dirtypalette)
		{
			int i;
			struct rectangle clip;


			dirtybuffer[offs] = 0;

			if (flipscreen)
			{
				sx = 127 - sx;
				sy = 31 - sy;
			}
			clip.min_x = 8*sx;
			clip.max_x = 8*sx+7;

			/* City Connection controls the color code for each _scanline_, not */
			/* for each character as happens in most games. Therefore, we have to draw */
			/* the character eight times, each time clipped to one line and using */
			/* the color code for that scanline */
			for (i = 0;i < 8;i++)
			{
				clip.min_y = 8*sy + i;
				clip.max_y = 8*sy + i;

				drawgfx(tmpbitmap,Machine->gfx[0],
						videoram[offs],
						citycon_charlookup[flipscreen ? (255 - 8*sy - i) : 8*sy + i],
						flipscreen,flipscreen,
						8*sx,8*sy,
						&clip,TRANSPARENCY_NONE,0);
			}
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int i,scroll[32];


		if (flipscreen)
		{
			for (i = 0;i < 6;i++)
				scroll[31-i] = 256;
			for (i = 6;i < 32;i++)
				scroll[31-i] = 256 + (citycon_scroll[0]*256+citycon_scroll[1]);
		}
		else
		{
			for (i = 0;i < 6;i++)
				scroll[i] = 0;
			for (i = 6;i < 32;i++)
				scroll[i] = -(citycon_scroll[0]*256+citycon_scroll[1]);
		}
		copyscrollbitmap(bitmap,tmpbitmap,32,scroll,0,0,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
	}


	for (offs = spriteram_size-4;offs >= 0;offs -= 4)
	{
		int sx,sy,flipx;


		sx = spriteram[offs + 3];
		sy = 239 - spriteram[offs];
		flipx = ~spriteram[offs + 2] & 0x10;
		if (flipscreen)
		{
			sx = 240 - sx;
			sy = 238 - sy;
			flipx = !flipx;
		}

		drawgfx(bitmap,Machine->gfx[spriteram[offs + 1] & 0x80 ? 2 : 1],
				spriteram[offs + 1] & 0x7f,
				spriteram[offs + 2] & 0x0f,
				flipx,flipscreen,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	dirtypalette = 0;

	for (offs = 0;offs < 32;offs++)
		dirtylookup[offs] = 0;
}
