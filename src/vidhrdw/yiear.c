/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

void yiear_videoram_w(int offset,int data)
{
	if (videoram[offset] != data)
	{
		dirtybuffer[offset/2] = 1;
		videoram[offset] = data;
	}
}

void yiear_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int i;

	/* draw background */
	{
		unsigned char *vr = videoram;
		for (i = 0;i < 1024;i++) if ( dirtybuffer[i] )
		{
			unsigned char byte1 = vr[i*2];
			unsigned char byte2 = vr[i*2+1];
			int code = 16*(byte1 & 0x10) + byte2;
			int flipx = byte1 & 0x80;
			int flipy = byte1 & 0x40;
			int sx = 8 * (i % 32);
			int sy = 8 * (i / 32);
			
			dirtybuffer[i] = 0;

			drawgfx(tmpbitmap,Machine->gfx[0],
				code,0,flipx,flipy,sx,sy,
				0,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,
		&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* draw sprites. */
	{
		unsigned char *sr  = spriteram;
		
		//for (i = 0;i < 24*16;i += 16)// if( sr[i+3] ) ?? sprite active flag?
		for (i = 23*16; i>=0; i -= 16)
		{
			int code = 256*(sr[i+0x0f] & 1) + sr[i+0x0e];
			int flipx = (sr[i+0x0f]&0x40)?0:1;
			int flipy = (sr[i+0x0f]&0x80)?1:0;
			int sx = sr[i+0x04];
			int sy = 239-sr[i+0x06];
			
			drawgfx(bitmap,Machine->gfx[1],
				code,0,flipx,flipy,sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
