/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

  Video output ports:
    ????? (3)
        0x03 ?????

    Control (4)
        0x80 ???
        0x40 Table / Cocktail mode

    ???? (0x0c)
        0x04 ???
        0x02 ???

    Scroll page (0x0d)
        Read/write scroll page number register

    Screen layout (0x0e)
        Screen layout (8x4 or 4x8)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#include "osdepend.h"

unsigned char *blktiger_paletteram;
unsigned char *blktiger_backgroundram;
static unsigned char blktiger_video_control;
unsigned char *blktiger_screen_layout;
int blktiger_paletteram_size;
int blktiger_backgroundram_size;
void blktiger_scrollx_w(int offset,int data);
void blktiger_scrolly_w(int offset,int data);
void blktiger_scrollbank_w(int offset,int data);

static int blktiger_scroll_bank;
static const int scroll_page_count=4;
static unsigned char blktiger_scrolly[2];
static unsigned char blktiger_scrollx[2];
static unsigned char *scroll_ram;
static unsigned char *dirtybuffer2;
static unsigned char *dirtybufferpal;
static struct osd_bitmap *tmpbitmap2;
static struct osd_bitmap *tmpbitmap3;
static int screen_layout;

static unsigned char scrollpalette_dirty[0x40];

static int testoutput;
void blktiger_test_w(int offset,int data);
void blktiger_test_w(int offset,int data)
{
    testoutput=data;
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int blktiger_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

        if ((dirtybuffer2 = malloc(blktiger_backgroundram_size*scroll_page_count)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
        memset(dirtybuffer2,1,blktiger_backgroundram_size*scroll_page_count);

        if ((scroll_ram = malloc(blktiger_backgroundram_size*scroll_page_count)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
        memset(scroll_ram,0,blktiger_backgroundram_size*scroll_page_count);


        /* Palette RAM dirty buffer */
        if ((dirtybufferpal = malloc(blktiger_paletteram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
        memset(dirtybufferpal,1,blktiger_paletteram_size);


        /* the background area is 8 x 4 */
        if ((tmpbitmap2 = osd_new_bitmap(8*Machine->drv->screen_width,
                scroll_page_count*Machine->drv->screen_height,
                Machine->scrbitmap->depth)) == 0)
	{
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

        /* the alternative background area is 4 x 8 */
        if ((tmpbitmap3 = osd_new_bitmap(4*Machine->drv->screen_width,
                2*scroll_page_count*Machine->drv->screen_height,
                Machine->scrbitmap->depth)) == 0)
	{
		free(dirtybuffer2);
                osd_free_bitmap(tmpbitmap2);
		generic_vh_stop();
		return 1;
	}



	return 0;

}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void blktiger_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap2);
	free(dirtybuffer2);
        free(dirtybufferpal);
        free(scroll_ram);
	generic_vh_stop();
}

void blktiger_background_w(int offset,int data)
{
        offset+=blktiger_scroll_bank;

        if (scroll_ram[offset] != data)
	{
		dirtybuffer2[offset] = 1;
                scroll_ram[offset] = data;
	}
}


void blktiger_scrollbank_w(int offset, int data)
{
        blktiger_scroll_bank=(data&0x03)*blktiger_backgroundram_size;
}

int blktiger_background_r(int offset)
{
        offset+=blktiger_scroll_bank;
        return scroll_ram[offset];
}

void blktiger_scrolly_w(int offset,int data)
{
        blktiger_scrolly[offset]=data;
}

void blktiger_scrollx_w(int offset,int data)
{
        blktiger_scrollx[offset]=data;
}

void blktiger_paletteram_w(int offset,int data)
{
        if (blktiger_paletteram[offset] != data)
	{
                dirtybufferpal[offset]=1;
                blktiger_paletteram[offset] = data;
                /*
                   Mark palettes as dirty.
                   We are only interested in whether tiles have changed.
                   This is done by colour code.

                   Sprites are always drawn. I don't think that characters
                   change. If I am wrong then the code below needs to
                   be changed.
                */
                scrollpalette_dirty[(offset>>4)&0x3f]=1;
	}
}

void blktiger_video_control_w(int offset,int data)
{
        blktiger_video_control=data;
}

void blktiger_screen_layout_w(int offset,int data)
{
        screen_layout=data;
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void blktiger_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs, sx, sy;
	int j, i;

	/* rebuild the colour lookup table from RAM palette */
	for (j=0; j<3; j++)
	{
		/*
		   0000-0100:  background palettes. (16x16 colours)
		   0200-0280:  sprites palettes.    ( 8x16 colours)
		   0300-0380:  characters palettes  (32x4 colours)
		*/
		          /* CHARS  TILES   SPRITES */
		int start[3]={0x0300, 0x0000, 0x0200};
		int count[3]={0x0080, 0x0100, 0x0080};
		int base=start[j];
		int bluebase=base+0x0400;
		int max=count[j];

		for (i=0; i<max; i++)
		{
			if (dirtybufferpal[base] || dirtybufferpal[bluebase])
			{
				int red, green, blue, redgreen;

				redgreen=blktiger_paletteram[base];
				red=redgreen >>4;
				green=redgreen & 0x0f ;
				blue=blktiger_paletteram[bluebase]&0x0f;

				red = (red << 4) + red;
				green = (green << 4) + green;
				blue = (blue << 4) + blue;

				dirtybufferpal[base] = dirtybufferpal[bluebase] = 0;

				/* for tiles, pen 15 is the transparent color. However there */
				/* is no other plane behind them, so we just set them to black */
				if (j == 1 && i % 16 == 15)
					setgfxcolorentry (Machine->gfx[j], i, 0, 0, 0);
				else
					setgfxcolorentry (Machine->gfx[j], i, red, green, blue);
			}
			base++;
			bluebase++;
		}
	}

	/* Dirty all touched colours */
	for (j=blktiger_backgroundram_size*scroll_page_count-1; j>=0; j-=2)
	{
		int colour=(scroll_ram[j]&0x78)>>3;
		if (scrollpalette_dirty[colour])
			dirtybuffer2[j]=1;
	}

	/*
	Draw the tiles.

	This method may look unnecessarily complex. Only tiles that are
	likely to be visible are drawn. The rest are kept dirty until they
	become visible.

	The reason for this is that on level 3, the palette changes a lot
	if the whole virtual screen is checked and redrawn then the
	game will slow down to a crawl.
	*/

	if (screen_layout)
	{
		/* 8x4 screen */
		int offsetbase;
		int scrollx,scrolly, y;
		scrollx = ((blktiger_scrollx[0]>>4) + 16 * blktiger_scrollx[1]);
		scrolly = ((blktiger_scrolly[0]>>4) + 16 * blktiger_scrolly[1]);

		for (sy=0; sy<18; sy++)
		{
			y=(scrolly+sy)&(16*4-1);
			offsetbase=((y&0xf0)<<8)+32*(y&0x0f);
			for (sx=0; sx<18; sx++)
			{
				int colour, attr, code, x;
				x=(scrollx+sx)&(16*8-1);
				offs=offsetbase + ((x&0xf0)<<5)+2*(x&0x0f);

				if (dirtybuffer2[offs] || dirtybuffer2[offs+1] )
				{
					attr=scroll_ram[offs+1];
					colour=(attr&0x78)>>3;
					code=scroll_ram[offs];
					code+=256*(attr&0x07);

					dirtybuffer2[offs] =dirtybuffer2[offs+1] = 0 ;

					drawgfx(tmpbitmap2,Machine->gfx[1],
					        code,
					        colour,
					        attr & 0x80,
					        0,
					        x*16, y*16,
					        0,TRANSPARENCY_NONE,0);
				}
			}
		}

		/* copy the background graphics */
		{
			int scrollx,scrolly;

			scrollx = -(blktiger_scrollx[0] + 256 * blktiger_scrollx[1]);
			scrolly = -(blktiger_scrolly[0] + 256 * blktiger_scrolly[1]);
			copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}
	else
	{
		/* 4x8 screen */
		int offsetbase;
		int scrollx,scrolly, y;
		scrollx = ((blktiger_scrollx[0]>>4) + 16 * blktiger_scrollx[1]);
		scrolly = ((blktiger_scrolly[0]>>4) + 16 * blktiger_scrolly[1]);

		for (sy=0; sy<18; sy++)
		{
			y=(scrolly+sy)&(16*8-1);
			offsetbase=((y&0xf0)<<7)+32*(y&0x0f);
			for (sx=0; sx<18; sx++)
			{
				int colour, attr, code, x;
				x=(scrollx+sx)&(16*4-1);
				offs=offsetbase + ((x&0xf0)<<5)+2*(x&0x0f);

				if (dirtybuffer2[offs] || dirtybuffer2[offs+1] )
				{
					attr=scroll_ram[offs+1];
					colour=(attr&0x78)>>3;

					code=scroll_ram[offs];
					code+=256*(attr&0x07);

					dirtybuffer2[offs] =dirtybuffer2[offs+1] = 0 ;

					drawgfx(tmpbitmap3,Machine->gfx[1],
					        code,
					        colour,
					        attr & 0x80,0,
					        x*16, y*16,
					        0,TRANSPARENCY_NONE,0);
				}
			}
		}

		/* copy the background graphics */
		{
		int scrollx,scrolly;

		scrollx = -(blktiger_scrollx[0] + 256 * blktiger_scrollx[1]);
		scrolly = -(blktiger_scrolly[0] + 256 * blktiger_scrolly[1]);
		copyscrollbitmap(bitmap,tmpbitmap3,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	/* Draw the sprites. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		/*	SPRITES
			=====
			Attribute
			0x80 Code MSB
			0x40 Code MSB
			0x20 Code MSB
			0x10 X MSB
			0x08 X flip
			0x04 Colour
			0x02 Colour
			0x01 Colour
		*/

		int code,colour,sx,sy;

		code = spriteram[offs];
		code += ( ((int)(spriteram[offs+1]&0xe0)) << 3 );
		colour = spriteram[offs+1] & 0x07;

		sy = spriteram[offs + 2];
		sx = spriteram[offs + 3]-0x10 * ( spriteram[offs + 1] & 0x10);

		drawgfx(bitmap,Machine->gfx[2],
		        code,
		        colour,
		        spriteram[offs+1]&0x08,0,
		        sx, sy,
		        &Machine->drv->visible_area,TRANSPARENCY_PEN,15);
	}

	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy;

		sy = 8 * (offs / 32);
		sx = 8 * (offs % 32);

		drawgfx(bitmap,Machine->gfx[0],
				videoram[offs] + ((colorram[offs] & 0xe0) << 3),
				colorram[offs] & 0x1f,
				0,0,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,3);
	}

	memset(scrollpalette_dirty, 0, sizeof(scrollpalette_dirty));
}
