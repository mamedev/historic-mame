/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"


#define VIDEO_RAM_SIZE 0x400
#define BIGSPRITE_SIZE 0x100
#define BIGSPRITE_WIDTH 128
#define BIGSPRITE_HEIGHT 128


unsigned char *ckong_videoram;
unsigned char *ckong_colorram;
unsigned char *ckong_bsvideoram;
unsigned char *ckong_spriteram;
unsigned char *ckong_bigspriteram;
static unsigned char dirtybuffer[VIDEO_RAM_SIZE];	/* keep track of modified portions of the screen */
											/* to speed up video refresh */
static unsigned char bsdirtybuffer[BIGSPRITE_SIZE];

static struct osd_bitmap *tmpbitmap,*bsbitmap;



int ckong_vh_start(void)
{
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	if ((bsbitmap = osd_create_bitmap(BIGSPRITE_WIDTH,BIGSPRITE_HEIGHT)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void ckong_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
	osd_free_bitmap(bsbitmap);
}



void ckong_videoram_w(int offset,int data)
{
	if (ckong_videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		ckong_videoram[offset] = data;
	}
}



void ckong_colorram_w(int offset,int data)
{
	if (ckong_colorram[offset] != data)
	{
		/* bit 5 of the address is not used for color memory. There is just */
		/* 512k of memory; every two consecutive rows share the same memory */
		/* region. */
		offset &= 0xffdf;

		dirtybuffer[offset] = 1;
		dirtybuffer[offset + 0x20] = 1;

		ckong_colorram[offset] = data;
		ckong_colorram[offset + 0x20] = data;
	}
}



void ckong_bigsprite_videoram_w(int offset,int data)
{
	if (ckong_bsvideoram[offset] != data)
	{
		bsdirtybuffer[offset] = 1;

		ckong_bsvideoram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void ckong_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int i,offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 8 * (31 - offs / 32);
			sy = 8 * (offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[(ckong_colorram[offs] & 0x10) ? 1 : 0],
					ckong_videoram[offs] + 8 * (ckong_colorram[offs] & 0x20),
					ckong_colorram[offs] & 0x0f,
					ckong_colorram[offs] & 0x40,ckong_colorram[offs] & 0x80,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* draw sprites (must be done before the "big sprite" to obtain the correct priority) */
	for (i = 0;i < 8*4;i+=4)
	{
		drawgfx(bitmap,Machine->gfx[ckong_spriteram[i + 1] & 0x10 ? 4 : 3],
				(ckong_spriteram[i] & 0x3f) + 2 * (ckong_spriteram[i + 1] & 0x20),
				ckong_spriteram[i + 1] & 0x0f,
				ckong_spriteram[i] & 0x80,ckong_spriteram[i] & 0x40,
				ckong_spriteram[i + 2] + 1,ckong_spriteram[i + 3],
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	/* draw the "big sprite" */
	{
		int newcol;
		static int lastcol;


		newcol = ckong_bigspriteram[1] & 0x07;

		/* first of all, update it. */
		for (offs = 0;offs < BIGSPRITE_SIZE;offs++)
		{
			int sx,sy;

			if (bsdirtybuffer[offs] || newcol != lastcol)
			{
				bsdirtybuffer[offs] = 0;

				sx = 8 * (15 - offs / 16);
				sy = 8 * (offs % 16);

				drawgfx(bsbitmap,Machine->gfx[2],
						ckong_bsvideoram[offs],newcol,
						0,0,sx,sy,
						0,TRANSPARENCY_NONE,0);
			}
		}

		lastcol = newcol;

		/* copy the temporary bitmap to the screen */
		copybitmap(bitmap,bsbitmap,
				ckong_bigspriteram[1] & 0x20,!(ckong_bigspriteram[1] & 0x10),
				ckong_bigspriteram[2],ckong_bigspriteram[3] - 8,
				&Machine->drv->visible_area,TRANSPARENCY_COLOR,Machine->background_pen);
	}
}
