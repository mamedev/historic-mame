/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#include "osdepend.h"

unsigned char *lwings_paletteram;
unsigned char *lwings_backgroundram;
unsigned char *lwings_backgroundattribram;
int lwings_paletteram_size;
int lwings_backgroundram_size;
unsigned char *lwings_scrolly;
unsigned char *lwings_scrollx;

static unsigned char *dirtybuffer2;
static unsigned char *dirtybuffer3;
static unsigned char *dirtybuffer4;
static struct osd_bitmap *tmpbitmap2;
static unsigned char lwings_paletteram_dirty[0x40];


void lwings_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom     )
{
     int i;

     for (i=0; i<Machine->drv->total_colors; i++)
     {
         /*
         Convert 12 bit RGB to 8 bit RGB. Some blue will be lost.
         */
         int red, green, blue;
         red = (i & 0x07)<<5;
         green=(i & 0x38)<<2;
         blue= (i & 0xc0);

         palette[3*i]   =  red;
         palette[3*i+1] =  green;
         palette[3*i+2] =  blue;
     }
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int lwings_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

        if ((dirtybuffer2 = malloc(lwings_backgroundram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
        memset(dirtybuffer2,1,lwings_backgroundram_size);

        /* Palette RAM dirty buffer */
        if ((dirtybuffer3 = malloc(lwings_paletteram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
        memset(dirtybuffer3,1,lwings_paletteram_size);
        if ((dirtybuffer4 = malloc(lwings_paletteram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
        memset(dirtybuffer4,1,lwings_paletteram_size);


	/* the background area is twice as tall as the screen */
        if ((tmpbitmap2 = osd_create_bitmap(2*Machine->drv->screen_width,
                2*Machine->drv->screen_height)) == 0)
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
void lwings_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap2);
	free(dirtybuffer2);
        free(dirtybuffer3);
        free(dirtybuffer4);
	generic_vh_stop();
}

void lwings_background_w(int offset,int data)
{
        if (lwings_backgroundram[offset] != data)
	{
		dirtybuffer2[offset] = 1;

                lwings_backgroundram[offset] = data;
	}
}

void lwings_backgroundattrib_w(int offset,int data)
{
        if (lwings_backgroundattribram[offset] != data)
	{
                dirtybuffer4[offset] = 1;

                lwings_backgroundattribram[offset] = data;
	}
}


void lwings_paletteram_w(int offset,int data)
{
        if (lwings_paletteram[offset] != data)
	{
                dirtybuffer3[offset]=1;
                lwings_paletteram[offset] = data;
                /* Mark entire colour schemes dirty if touched */
                lwings_paletteram_dirty[(offset>>4)&0x3f]=1;
	}
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void lwings_vh_screenrefresh(struct osd_bitmap *bitmap)
{
        int j, i, offs;

        static unsigned char chTableRED[0x10]=
        {
            0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03,
            0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07
        };

        static unsigned char chTableGREEN[0x10]=
        {
            0x00, 0x08, 0x08, 0x08, 0x10, 0x10, 0x18, 0x18,
            0x20, 0x20, 0x28, 0x28, 0x30, 0x30, 0x38, 0x38
        };


        static unsigned char chTableBLUE[0x10]=
        {
            0x00, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x80,
            0x80, 0x80, 0x80, 0x80, 0xc0, 0xc0, 0xc0, 0xc0
        };

        /* rebuild the colour lookup table */
        for (j=0; j<3; j++)
	{
                /*
                    0000-007f:  background palette. (8x16 colours)
                    0180-01ff:  sprite palette.     (8x16 colours)
                    0200-0240:  character palette   (16x4 colours)
                    0300-0340:  character palette   (16x4 colours)
                */
                             /* CHARS TILES SPRITES */
                int start[3]={0x0200, 0x0000, 0x0180};
                int count[3]={  0x40,   0x80,   0x80};
                int base=start[j];
                int max=count[j];

                for (i=0; i<max; i++)
		{
                        if (dirtybuffer3[base] || dirtybuffer3[base+0x0400])
                        {
                           int redgreen=lwings_paletteram[base];
                           int blue=lwings_paletteram[base+0x0400]>>4;
                           int red=redgreen >> 4;
                           int green=redgreen & 0x0f;

                           dirtybuffer3[base] = dirtybuffer3[base+0x0400] = 0;

                           offs     = chTableGREEN[green];
                           offs    |= chTableRED[red];
                           offs    |= chTableBLUE[blue];
                           Machine->gfx[j]->colortable[i] = Machine->pens[offs];
                        }
                        base++;
		}
        }

        for (offs = lwings_backgroundram_size - 1;offs >= 0;offs--)
        {
                int sx,sy, colour;
                /*
                        Tiles
                        =====
                        0x80 Tile code MSB
                        0x40 Tile code MSB
                        0x20 Tile code MSB
                        0x10 X flip
                        0x08 Y flip
                        0x04 Colour
                        0x02 Colour
                        0x01 Colour
                */

                colour=(lwings_backgroundattribram[offs] & 0x07);
                if (dirtybuffer2[offs] != 0 || dirtybuffer4[offs] != 0 ||
                      lwings_paletteram_dirty[colour])
                {
                      int code;
                      dirtybuffer2[offs] = dirtybuffer4[offs] = 0;

                      sx = offs / 32;
                      sy = offs % 32;
                      code=lwings_backgroundram[offs];
                      code+=((((int)lwings_backgroundattribram[offs]) &0xe0) <<3);

                      drawgfx(tmpbitmap2,Machine->gfx[1],
                                   code,
                                   colour,
                                   (lwings_backgroundattribram[offs] & 0x08),
                                   (lwings_backgroundattribram[offs] & 0x10),
                                   16 * sx,16 * sy,
                                   0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the background graphics */
	{
		int scrollx,scrolly;


		scrolly = -(lwings_scrollx[0] + 256 * lwings_scrollx[1]);
		scrollx = -(lwings_scrolly[0] + 256 * lwings_scrolly[1]);

		copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int code,sx,sy;

		/*
		Sprites
		=======
		0x80 Sprite code MSB
		0x40 Sprite code MSB
		0x20 Colour
		0x10 Colour
		0x08 Colour
		0x04 Y flip
		0x02 X flip
		0x01 X MSB
		*/
		code = spriteram[offs];
		code += (spriteram[offs + 1] & 0xc0) << 2;
		sx = spriteram[offs + 3] - 0x100 * (spriteram[offs + 1] & 0x01);
		sy = spriteram[offs + 2];

		drawgfx(bitmap,Machine->gfx[2],
				code,
				(spriteram[offs + 1] & 0x38) >> 3,
				spriteram[offs + 1] & 0x02,spriteram[offs + 1] & 0x04,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,15);
	}

	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int code;


		code = videoram[offs] + 4 * (colorram[offs] & 0xc0);
		if (code != 0x20)     /* don't draw spaces */
		{
			int sx,sy,flipx,flipy;


			sx = offs % 32;
			sy = offs / 32;
			flipx = colorram[offs] & 0x10;
			flipy = colorram[offs] & 0x20;

			drawgfx(bitmap,Machine->gfx[0],
					code,
					colorram[offs] & 0x0f,
					flipx,flipy,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,3);
		}
	}

	memset(lwings_paletteram_dirty, 0, sizeof(lwings_paletteram_dirty));
}
