/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

  (c) 12/2/1998 Lee Taylor

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



static int flipscreen;


WRITE_HANDLER( skychut_vh_flipscreen_w )
{
/*	if (flipscreen != (data & 0x8f))
	{
		flipscreen = (data & 0x8f);
		memset(dirtybuffer,1,videoram_size);
	}
*/
}


WRITE_HANDLER( skychut_colorram_w )
{
	if (colorram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		colorram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( skychut )
{
	int offs;
	if (get_vh_global_attribute_changed())
		memset (dirtybuffer, 1, videoram_size);

	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					 colorram[offs],
					flipscreen,flipscreen,
					8*sx,8*sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
}

UINT8* iremm15_chargen;

static void iremm15_drawgfx(struct mame_bitmap *bitmap, int ch,
							INT16 color, INT16 back, int x, int y)
{
	UINT8 mask;
	int i;

	for (i=0; i<8; i++, x++) {
		mask=iremm15_chargen[ch*8+i];
		plot_pixel(bitmap,x,y+7,mask&0x80?color:back);
		plot_pixel(bitmap,x,y+6,mask&0x40?color:back);
		plot_pixel(bitmap,x,y+5,mask&0x20?color:back);
		plot_pixel(bitmap,x,y+4,mask&0x10?color:back);
		plot_pixel(bitmap,x,y+3,mask&8?color:back);
		plot_pixel(bitmap,x,y+2,mask&4?color:back);
		plot_pixel(bitmap,x,y+1,mask&2?color:back);
		plot_pixel(bitmap,x,y,mask&1?color:back);
	}
}

/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( iremm15 )
{
	int offs;
	if (get_vh_global_attribute_changed())
		memset (dirtybuffer, 1, videoram_size);

	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			iremm15_drawgfx(tmpbitmap,
							videoram[offs],
							Machine->pens[colorram[offs]],
							Machine->pens[7], // space beam not color 0
							8*sx,8*sy);
		}
	}

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
}

