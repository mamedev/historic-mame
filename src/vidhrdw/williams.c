/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#define VIDEO_RAM_SIZE 0x1000

unsigned char *williams_videoram,*williams_colorram,*williams_backgroundram;
unsigned char *williams_background_pos,*williams_palette_bank;

unsigned char *dirtybuffer, *VideoChanged;
struct osd_bitmap *tmpbitmap, *ForeBitmap;

static unsigned char *colortab;

char *williams_dirty;


void williams_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;

	for (i = 0;i < 256;i++)
	{
		int bit0,bit1,bit2;

		bit0 = (i >> 0) & 0x01;
		bit1 = (i >> 1) & 0x01;
		bit2 = (i >> 2) & 0x01;
		palette[3*i] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = (i >> 3) & 0x01;
		bit1 = (i >> 4) & 0x01;
		bit2 = (i >> 5) & 0x01;
		palette[3*i + 1] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		bit0 = 0;
		bit1 = (i >> 6) & 0x01;
		bit2 = (i >> 7) & 0x01;
		palette[3*i + 2] = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	}

}

void williams_videoram_w(int offset,int data)
{
/*	if (errorlog) fprintf(errorlog, "video write %x = %x\n", offset, data); */
    williams_videoram[offset] = data;
}

void williams_colorram_w(int offset,int data)
{
	if (errorlog) fprintf(errorlog, "colortab write %x = %x\n", offset, data);
	if (colortab[offset] != data)
	{
		colortab[offset] = data;
		memset(williams_dirty, 1, 256);
	}
}

void williams_palettebank_w(int offset,int data)
{
	/*
	if ((data & 0x08) != (*williams_palette_bank & 0x08))
	{
		int i;


		for (i = 0;i < VIDEO_RAM_SIZE;i++)
			dirtybuffer[i] = 1;
	}

	*williams_palette_bank = data;
	*/
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int williams_vh_start(void)
{
	int len;

	len = (Machine->drv->screen_width/8) * (Machine->drv->screen_height/8);

	/* round the length to the next 0x400 boundary. This is necessary to */
	/* allocate a buffer large enough for Pac Man and other games using */
	/* 224x288 and 288x224 video modes, but still a 32x32 video memory */
	/* layout, and for williams, using a 512x480 screen but a 64x64 memory. */

	len = (len + 0x3ff) & 0xfffffc00;

	if ((dirtybuffer = malloc(len)) == 0)
		return 1;

	memset(dirtybuffer,0,len);

	if ((VideoChanged = malloc(len)) == 0)
		return 1;

	memset(VideoChanged,0,len);

	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer);
		free(VideoChanged);
		return 1;
	}

	if ((ForeBitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer);
		free(VideoChanged);
		osd_free_bitmap(tmpbitmap);
		return 1;
	}

	if ((colortab = malloc(16)) == 0)
		return 1;

	if ((williams_dirty = malloc(256)) == 0)
		return 1;
	memset(williams_dirty, 1, 256);
		
	return 0;
}


/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void williams_vh_stop(void)
{
	free(dirtybuffer);
	free(VideoChanged);
	osd_free_bitmap(tmpbitmap);
	osd_free_bitmap(ForeBitmap);
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void williams_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int x,y;

	for (x=0; x<151; x++)
	{
		if (williams_dirty[x])
		{
			int xx = x*2;
			int addr = x*256;
			for (y=0; y<256; y++)
			{
				int v = RAM[0x10000 + addr + y];
				bitmap->line[y][xx] = colortab[v>>4];
				bitmap->line[y][xx+1] = colortab[v & 0xf];
			}
			williams_dirty[x] = 0;
		}
	}

}
