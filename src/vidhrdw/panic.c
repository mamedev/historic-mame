/***************************************************************************

  panic.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* Sprite number - Bank conversion */

static const unsigned char Remap[64][2] = {
{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0}, /* 00 */
{0x00,0},{0x26,1},{0x25,1},{0x24,1},{0x23,1},{0x22,1},{0x21,1},{0x20,1}, /* 08 */
{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0}, /* 10 */
{0x00,0},{0x16,1},{0x15,1},{0x14,1},{0x13,1},{0x12,1},{0x11,1},{0x10,1}, /* 18 */
{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0}, /* 20 */
{0x00,0},{0x06,1},{0x05,1},{0x04,1},{0x03,1},{0x02,1},{0x01,1},{0x00,1}, /* 28 */
{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0},{0x00,0}, /* 30 */
{0x07,3},{0x06,3},{0x05,3},{0x04,3},{0x03,3},{0x02,3},{0x01,3},{0x00,3}, /* 38 */
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

void panic_videoram_w(int offset,int data)
{
	if ((panic_videoram[offset] != data))
	{
	    panic_videoram[offset] = data;

        /* Restrict to visible area only*/

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
            if (offset >= 0x2FC && offset <= 0x2FE) /* Colour Map Registers */
    	    {
                int x,y,col,ColTab;

                ColourMap = (RAM[0x42fc]>>7) + (RAM[0x42fd]>>6) + (RAM[0x42fe]>>5);

                for(x=0;x<256;x++) /* Need to re-colour existing screen! */
				{
                    for(y=0;y<256;y++)
                    {
                        if(tmpbitmap->line[y][x] != Machine->gfx[0]->colortable[0])
                            {
                                ColTab = RAM[CLT[ColourMap][0] + Band[y / 8] + x / 8 - 2];

                                if (CLT[ColourMap][1] == 1) col = Machine->gfx[0]->colortable[20+(ColTab >> 4)];
                                else col = Machine->gfx[0]->colortable[20+(ColTab & 0x0F)];

                                tmpbitmap->line[y][x] = col;
                            }
                    }
                }
            }
        }
	}
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void panic_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs, Sprite, Bank, Rotate;

	/* copy the character mapped graphics */

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */

	for (offs = 0;offs <= 7*4;offs += 4)
	{
		if (spriteram[offs] != 0)
        {
        	/* Remap sprite number to my layout */

            Sprite = Remap[(spriteram[offs] & 0x3F)][0];
            Bank   = Remap[(spriteram[offs] & 0x3F)][1];
            Rotate = spriteram[offs] & 0x40;

            /* Switch Bank */

            if(spriteram[offs+3] & 0x08) Bank=2;

		    drawgfx(bitmap,Machine->gfx[Bank],
				    Sprite,
				    spriteram[offs+3] & 0x07,
				    Rotate,0,
				    spriteram[offs+1]+16,spriteram[offs+2]-16,
				    &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
        }
	}
}
