/***************************************************************************

  panic.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Sprite number - Bank conversion */

static const unsigned char Remap[64][2] = {
{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0}, /* 00 */
{0x00,0},{0x26,0},{0x25,0},{0x24,0},{0x23,0},{0x22,0},{0x21,0},{0x20,0}, /* 08 */
{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0}, /* 10 */
{0x00,0},{0x16,0},{0x15,0},{0x14,0},{0x13,0},{0x12,0},{0x11,0},{0x10,0}, /* 18 */
{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0}, /* 20 */
{0x00,0},{0x06,0},{0x05,0},{0x04,0},{0x03,0},{0x02,0},{0x01,0},{0x00,0}, /* 28 */
{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0}, /* 30 */
{0x07,2},{0x06,2},{0x05,2},{0x04,2},{0x03,2},{0x02,2},{0x01,2},{0x00,2}, /* 38 */
};

/*
   Colour is via a ROM colour table, 8 screens selected by
   location's 42FC,42FD & 42FE. 16 colour bands are used for
   32 lines of screen. Banding is defined in array Band.
   Memory address of each lookup table is in CLT, along
   with a flag to indicate whether high or low nibble is
   used for that layout.

   The score screen however, uses a different system, which
   I have'nt been able to identify, so I detect that the
   screen is displayed using location 700C and colourmap 0
*/

static const int Band[32] = {
    15*32,
	15*32,
	15*32,
	14*32,
	13*32,
	13*32,
	12*32,
	12*32,
	11*32,
	11*32,
	10*32,
	10*32,
	9*32,
	9*32,
	8*32,
	8*32,
	7*32,
	7*32,
	6*32,
	6*32,
	5*32,
	5*32,
	4*32,
	4*32,
	3*32,
	3*32,
	2*32,
	2*32,
	1*32,
	1*32,
	0*32,
	0*32
};

static const int CLT[8][2] = {
    {0x3800,0},{0x3a00,0},{0x3800,1},{0x3a00,1},
    {0x3c00,0},{0x3e00,0},{0x3c00,1},{0x3e00,1}
};

unsigned char *panic_videoram;

static int ColourMap;



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int panic_vh_start(void)
{
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	/* initialize the bitmap to our background color */

	fillbitmap(tmpbitmap,Machine->gfx[0]->colortable[0],&Machine->drv->visible_area);

	return 0;
}


/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void panic_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



void panic_videoram_w(int offset,int data)
{
	/* TODO: get rid of this */
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((panic_videoram[offset] != data))
	{
	    panic_videoram[offset] = data;

        /* Restrict to visible area only */

        if (offset > 0x3ff)
	    {
		    int i,x,y;
		    int col;
            int ColTab;

		    x = offset / 32 + 16;
		    y = 256-8 - 8 * (offset % 32);

            /* Scoring Screen override - only on colour map 0 */

            if(RAM[0x700C] == 0x80 && ColourMap == 0)
			{
			    col = Machine->gfx[0]->colortable[30];   /* Green */
            }
            else
            {
	            ColTab = RAM[CLT[ColourMap][0] + Band[y / 8] + x / 8 - 2];

    	        if (CLT[ColourMap][1] == 1) col = Machine->gfx[0]->colortable[20+(ColTab >> 4)];
        	    else col = Machine->gfx[0]->colortable[20+(ColTab & 0x0F)];
            }

            /* Allow Rotate */

            if (Machine->orientation & ORIENTATION_SWAP_XY)
            {
				if (!(Machine->orientation & ORIENTATION_FLIP_X))
					x = 255 - x;

			    if (Machine->orientation & ORIENTATION_FLIP_Y)
                {
				    osd_mark_dirty(y,x,y+7,x,0);

			        for (i = 0;i < 8;i++)
			        {
				        if (data & 0x01) tmpbitmap->line[x][y] = col;
				        else tmpbitmap->line[x][y] = Machine->gfx[0]->colortable[0]; /* black */

				        y++;
				        data >>= 1;
                    }
                }
                else
                {
				    y = 255 - y;
				    osd_mark_dirty(y-7,x,y,x,0);

			        for (i = 0;i < 8;i++)
			        {
				        if (data & 0x01) tmpbitmap->line[x][y] = col;
				        else tmpbitmap->line[x][y] = Machine->gfx[0]->colortable[0]; /* black */

				        y--;
				        data >>= 1;
                    }
                }
            }
            else
            {
            	/* Normal */

				if (Machine->orientation & ORIENTATION_FLIP_X)
					x = 255 - x;

			    if (!(Machine->orientation & ORIENTATION_FLIP_Y))
                {
				    osd_mark_dirty(x,y,x,y+7,0);

			        for (i = 0;i < 8;i++)
			        {
				        if (data & 0x01) tmpbitmap->line[y][x] = col;
				        else tmpbitmap->line[y][x] = Machine->gfx[0]->colortable[0]; /* black */

				        y++;
				        data >>= 1;
                    }
                }
                else
                {
				    y = 255 - y;
				    osd_mark_dirty(x,y-7,x,y,0);

			        for (i = 0;i < 8;i++)
			        {
				        if (data & 0x01) tmpbitmap->line[y][x] = col;
				        else tmpbitmap->line[y][x] = Machine->gfx[0]->colortable[0]; /* black */

				        y--;
				        data >>= 1;
                    }
                }
            }
	    }
        else
        {
          if (offset >= 0x2FC && offset <= 0x2FE) /* Colour Map Registers */
    	    {
				/* Need to re-colour existing screen! */

				int x,y,col,ColTab;
               	int ex,ey;

                ColourMap = (RAM[0x42fc]>>7) + (RAM[0x42fd]>>6) + (RAM[0x42fe]>>5);

                if (Machine->orientation & ORIENTATION_SWAP_XY)
                {
                    for(x=0;x<256;x++)
				    {
				    	if (Machine->orientation & ORIENTATION_FLIP_X)
						    ex = 255 - x;
                        else
                        	ex = x;

                        for(y=0;y<256;y++)
                        {
					        if (!(Machine->orientation & ORIENTATION_FLIP_Y))
                            	ey = y;
                            else
                            	ey = 255 - y;

                            if(tmpbitmap->line[ex][ey] != Machine->gfx[0]->colortable[0])
                            {
                                ColTab = RAM[CLT[ColourMap][0] + Band[y / 8] + x / 8 - 2];

                                if (CLT[ColourMap][1] == 1) col = Machine->gfx[0]->colortable[20+(ColTab >> 4)];
                                else col = Machine->gfx[0]->colortable[20+(ColTab & 0x0F)];

                                tmpbitmap->line[ex][ey] = col;
                            }
                        }
                    }
                }
                else
                {
            	    /* Normal */

                    for(x=0;x<256;x++)
				    {
				    	if (Machine->orientation & ORIENTATION_FLIP_X)
						    ex = 255 - x;
                        else
                        	ex = x;

                        for(y=0;y<256;y++)
                        {
					        if (Machine->orientation & ORIENTATION_FLIP_Y)
                            	ey = 255 - y;
                            else
                            	ey = y;

                            if(tmpbitmap->line[ey][ex] != Machine->gfx[0]->colortable[0])
                            {
                                ColTab = RAM[CLT[ColourMap][0] + Band[y / 8] + x / 8 - 2];

                                if (CLT[ColourMap][1] == 1) col = Machine->gfx[0]->colortable[20+(ColTab >> 4)];
                                else col = Machine->gfx[0]->colortable[20+(ColTab & 0x0F)];

                                tmpbitmap->line[ey][ex] = col;
                            }
                        }
                    }
                }

			    osd_mark_dirty(0,0,255,255,0);	/* ASG 971015 */
            }
        }
	}
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void panic_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs, Sprite, Bank, Rotate;

	/* copy the character mapped graphics */

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* Draw the sprites */

	for (offs = 0;offs < spriteram_size;offs += 4)
	{
		if (spriteram[offs] != 0)
        {
        	/* Remap sprite number to my layout */

            Sprite = Remap[(spriteram[offs] & 0x3F)][0];
            Bank   = Remap[(spriteram[offs] & 0x3F)][1];
            Rotate = spriteram[offs] & 0x40;

            /* Switch Bank */

            if(spriteram[offs+3] & 0x08) Bank=1;

		    drawgfx(bitmap,Machine->gfx[Bank],
				    Sprite,
				    spriteram[offs+3] & 0x07,
				    Rotate,0,
				    spriteram[offs+1]+16,spriteram[offs+2]-16,
				    &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
        }
	}
}
