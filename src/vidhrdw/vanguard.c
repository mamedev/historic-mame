/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



#define VIDEO_RAM_SIZE 0x400
static unsigned char dirtybuffer2[VIDEO_RAM_SIZE];

unsigned char *vanguard_videoram2;
static struct osd_bitmap *tmpbitmap2;

unsigned char *vanguard_characterram;
static unsigned char dirtycharacter[256];

static int vanguard_scrollx;
static int vanguard_scrolly;


void vanguard_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i,b;
        unsigned char intensity4[] = { 0, 31, 47, 63 };
        unsigned char intensity8[] = { 0, 15, 23, 31, 39, 47, 55, 63 };

	for (i = 0, b=0 ;i < 256; i++, b++)
	{
                palette[i*3  ] = intensity8[b&07] << 2;
                palette[i*3+1] = intensity8[(b&0x38)>>3] << 2;
                palette[i*3+2] = intensity4[(b&0xc0)>>6] << 2;
	}

	for (i = 0;i < 16; i++)
        {
	        colortable[i*4] = 0;
                colortable[i*4+2] = color_prom[i*4+1];
                colortable[i*4+1] = color_prom[i*4+2];
                colortable[i*4+3] = color_prom[i*4+3];
        }
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int vanguard_vh_start(void)
{
	vanguard_scrollx = 0;
	vanguard_scrolly = 0;


	if (generic_vh_start() != 0)
		return 1;

	/* small temp bitmap for the score display */
	if ((tmpbitmap2 = osd_create_bitmap(Machine->drv->screen_width,3*8)) == 0)
	{
		generic_vh_stop();
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void vanguard_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap2);
	generic_vh_stop();
}



void vanguard_videoram2_w(int offset,int data)
{
	if (vanguard_videoram2[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		vanguard_videoram2[offset] = data;
	}
}



void vanguard_scrollx_w (int offset,int data)
{
	vanguard_scrollx = data;
}



void vanguard_scrolly_w (int offset,int data)
{
	vanguard_scrolly = data;
}



void vanguard_characterram_w(int offset,int data)
{
	if (vanguard_characterram[offset] != data)
	{
		dirtycharacter[(offset / 8) & 0xff] = 1;

		vanguard_characterram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void vanguard_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	extern struct GfxLayout vanguard_charlayout;
	int offs,i;
	int sx,sy;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		/* background */
		if (dirtybuffer2[offs])
		{

			dirtybuffer2[offs] = 0;

			sx = (31 - offs / 32);
			sy = (offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[1],
					vanguard_videoram2[offs],
					colorram[offs] & 0x7,
					0,0,8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	sx = vanguard_scrollx - 2*8;
	if (sx < 0) sx += 256;
	sy = vanguard_scrolly /* + 2*8 */;
	if (sy >= 256) sy -= 256;


	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,sx,-sy,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	copybitmap(bitmap,tmpbitmap,0,0,sx,256-sy,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	copybitmap(bitmap,tmpbitmap,0,0,sx-256,-sy,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	copybitmap(bitmap,tmpbitmap,0,0,sx-256,256-sy,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		int charcode;

		charcode = videoram[offs];

		/* background */
/*		if (dirtybuffer[offs] || dirtycharacter[charcode])*/
		{
			int sx,sy;


		/* decode modified characters */
			if (dirtycharacter[charcode] == 1)
			{
				decodechar(Machine->gfx[0],charcode,vanguard_characterram,&vanguard_charlayout);
				dirtycharacter[charcode] = 2;
			}


			dirtybuffer[offs] = 0;

			sx = (31 - offs / 32) - 2;
			sy = (offs % 32);

			drawgfx(bitmap,Machine->gfx[0],
					charcode,
					colorram[offs] & 0xf,
					0,0,8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}

	for (i = 0;i < 256;i++)
	{
		if (dirtycharacter[i] == 2) dirtycharacter[i] = 0;
	}

}
