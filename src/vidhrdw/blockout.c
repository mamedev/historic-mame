#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *blockout_videoram;
unsigned char *blockout_frontvideoram;
unsigned char *blockout_paletteram;

/* Block Out has a 512 colors palette, but it never uses them all at the same */
/* time. Since it always uses less than 256, we use an optimized palette. */
static int palettemap[512];
static int firstfreepen;



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int blockout_vh_start (void)
{
	int i;


	/* Allocate temporary bitmaps */
	if ((tmpbitmap = osd_new_bitmap(Machine->drv->screen_width,Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
		return 1;

	firstfreepen = 2;	/* pen 1 is for the front plane */
	for (i = 0;i < 512;i++)
		palettemap[i] = -1;

	return 0;
}


/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void blockout_vh_stop (void)
{
	osd_free_bitmap (tmpbitmap);
	tmpbitmap = 0;
}



static void setcolor(int pen,int color)
{
	int bit0,bit1,bit2,bit3;
	int r,g,b;


	/* red component */
	bit0 = (color >> 0) & 0x01;
	bit1 = (color >> 1) & 0x01;
	bit2 = (color >> 2) & 0x01;
	bit3 = (color >> 3) & 0x01;
	r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	/* green component */
	bit0 = (color >> 4) & 0x01;
	bit1 = (color >> 5) & 0x01;
	bit2 = (color >> 6) & 0x01;
	bit3 = (color >> 7) & 0x01;
	g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	/* blue component */
	bit0 = (color >> 8) & 0x01;
	bit1 = (color >> 9) & 0x01;
	bit2 = (color >> 10) & 0x01;
	bit3 = (color >> 11) & 0x01;
	b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

	osd_modify_pen(pen,r,g,b);
}



static void updatepixels(int x,int y)
{
	int front,back;
	int color;


	if (x < Machine->drv->visible_area.min_x ||
			x > Machine->drv->visible_area.max_x ||
			y < Machine->drv->visible_area.min_y ||
			y > Machine->drv->visible_area.max_y)
		return;

	front = READ_WORD(&blockout_videoram[y*512+x]);
	back = READ_WORD(&blockout_videoram[0x20000 + y*512+x]);

	if (front>>8) color = front>>8;
	else color = (back>>8) + 256;
	if (palettemap[color] == -1)
	{
		if (firstfreepen < 256)
		{
			palettemap[color] = Machine->pens[firstfreepen++];
			setcolor(palettemap[color],READ_WORD(&blockout_paletteram[2 * color]));
		}
	}
	tmpbitmap->line[y][x] = palettemap[color];

	if (front&0xff) color = front&0xff;
	else color = (back&0xff) + 256;
	if (palettemap[color] == -1)
	{
		if (firstfreepen < 256)
		{
			palettemap[color] = Machine->pens[firstfreepen++];
			setcolor(palettemap[color],READ_WORD(&blockout_paletteram[2 * color]));
		}
	}
	tmpbitmap->line[y][x+1] = palettemap[color];
}



void blockout_videoram_w(int offset, int data)
{
	int oldword = READ_WORD(&blockout_videoram[offset]);
	int newword = COMBINE_WORD(oldword,data);

	if (oldword != newword)
	{
		WRITE_WORD(&blockout_videoram[offset],newword);
		updatepixels(offset % 512,(offset / 512) % 256);
	}
}

int blockout_videoram_r(int offset)
{
   return READ_WORD(&blockout_videoram[offset]);
}



void blockout_frontvideoram_w(int offset, int data)
{
	COMBINE_WORD_MEM(&blockout_frontvideoram[offset],data);
}

int blockout_frontvideoram_r(int offset)
{
   return READ_WORD(&blockout_frontvideoram[offset]);
}



void blockout_paletteram_w(int offset, int data)
{
	int oldword = READ_WORD(&blockout_paletteram[offset]);
	int newword = COMBINE_WORD(oldword,data);


	WRITE_WORD(&blockout_paletteram[offset],newword);

	if (palettemap[offset / 2] != -1)
		setcolor(palettemap[offset / 2],newword);
}

int blockout_paletteram_r(int offset)
{
   return READ_WORD(&blockout_paletteram[offset]);
}



void blockout_frontcolor_w(int offset, int data)
{
	if (offset == 2)
		setcolor(Machine->pens[1],data);
}


void blockout_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	/* if we ran out of palette entries, rebuild the whole screen */
	if (firstfreepen == 256)
	{
		int i,x,y;


		firstfreepen = 2;	/* pen 1 is for the front plane */
		for (i = 0;i < 512;i++)
			palettemap[i] = -1;

		for (y = 0;y < 256;y++)
		{
			for (x = 0;x < 320;x+=2)
			{
				updatepixels(x,y);
			}
		}
	}

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	{
		int x,y,color;


		color = Machine->pens[1];

		for (y = 0;y < 256;y++)
		{
			for (x = 0;x < 320;x+=8)
			{
				int d;


				d = READ_WORD(&blockout_frontvideoram[y*128+(x/4)]);

				if (d)
				{
					if (d&0x80) bitmap->line[y][x] = color;
					if (d&0x40) bitmap->line[y][x+1] = color;
					if (d&0x20) bitmap->line[y][x+2] = color;
					if (d&0x10) bitmap->line[y][x+3] = color;
					if (d&0x08) bitmap->line[y][x+4] = color;
					if (d&0x04) bitmap->line[y][x+5] = color;
					if (d&0x02) bitmap->line[y][x+6] = color;
					if (d&0x01) bitmap->line[y][x+7] = color;
				}
			}
		}
	}
}
