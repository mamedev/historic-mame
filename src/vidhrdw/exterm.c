/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "TMS34010/tms34010.h"

static struct osd_bitmap *tmpbitmap1, *tmpbitmap2;
unsigned char *exterm_master_videoram, *exterm_slave_videoram;

void exterm_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	unsigned int i;

	palette += 3*0x1000;

	for (i = 0;i < 0x8000; i++)
	{
		int r = (i >> 10) & 0x1f;
		int g = (i >>  5) & 0x1f;
		int b = (i >>  0) & 0x1f;

		r = (r << 3) | (r >> 2);
		g = (g << 3) | (g >> 2);
		b = (b << 3) | (b >> 2);

		*(palette++) = r;
		*(palette++) = g;
		*(palette++) = b;
	}
}


int exterm_vh_start(void)
{
	if ((tmpbitmap = osd_new_bitmap(Machine->drv->screen_width,Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
	{
		return 1;
	}

	if ((tmpbitmap1 = osd_new_bitmap(Machine->drv->screen_width,Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
		return 1;
	}

	if ((tmpbitmap2 = osd_new_bitmap(Machine->drv->screen_width,Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
		osd_free_bitmap(tmpbitmap1);
		return 1;
	}

	return 0;
}

void exterm_vh_stop (void)
{
	osd_free_bitmap(tmpbitmap);
	osd_free_bitmap(tmpbitmap1);
	osd_free_bitmap(tmpbitmap2);
}

int exterm_master_videoram_r(int offset)
{
    return READ_WORD(&exterm_master_videoram[offset]);
}

void exterm_master_videoram_w(int offset, int data)
{
	COMBINE_WORD_MEM(&exterm_master_videoram[offset], data);

	if (data & 0x8000)
	{
		data &= 0x0fff;
	}
	else
	{
		data += 0x1000;
	}

	if (tmpbitmap->depth == 16)
		((unsigned short *)tmpbitmap->line[offset >> 9])[(offset >> 1) & 0xff] = Machine->pens[data];
	else
		tmpbitmap->line[offset >> 9][(offset >> 1) & 0xff] = Machine->pens[data];
}

int exterm_slave_videoram_r(int offset)
{
    return READ_WORD(&exterm_slave_videoram[offset]);
}

void exterm_slave_videoram_w(int offset, int data)
{
	int x,y;
	struct osd_bitmap *foreground;

	COMBINE_WORD_MEM(&exterm_slave_videoram[offset], data);

	x = offset & 0xff;
	y = (offset >> 8) & 0xff;

	foreground = (offset & 0x10000) ? tmpbitmap2 : tmpbitmap1;

	if (tmpbitmap1->depth == 16)
	{
		((unsigned short *)foreground->line[y])[x  ] = Machine->pens[ (data       & 0xff)];
		((unsigned short *)foreground->line[y])[x+1] = Machine->pens[((data >> 8) & 0xff)];
	}
	else
	{
		foreground->line[y][x  ] = Machine->pens[ (data       & 0xff)];
		foreground->line[y][x+1] = Machine->pens[((data >> 8) & 0xff)];
	}
}

void exterm_paletteram_w(int offset, int data)
{
	if ((offset == 0xff*2) && (data == 0))
	{
		/* Turn shadow color into dark red */
		data = 0x400;
	}

	paletteram_xRRRRRGGGGGBBBBB_word_w(offset, data);
}

static struct rectangle foregroundvisiblearea =
{
	0, 255, 40, 238
};

void exterm_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	if (TMS34010_io_display_blanked(0))
	{
		fillbitmap(bitmap,palette_transparent_pen,&Machine->drv->visible_area);

		return;
	}

	if (palette_recalc() != 0)
	{
		/* Redraw screen */

		int offset;

		if (tmpbitmap1->depth == 16)
		{
			for (offset = 0; offset < 256*256; offset+=2)
			{
				int data1,data2,data3,data4,x1,y1,x2,y2;

				data3 = READ_WORD(&exterm_master_videoram[offset]);
				if (data3 & 0x8000)
				{
					data3 &= 0x0fff;
				}
				else
				{
					data3 += 0x1000;
				}

				x1 = offset & 0xff;
				y1 = (offset >> 8) & 0xff;

				x2 = (offset >> 1) & 0xff;
				y2 = offset >> 9;

				data4 = READ_WORD(&exterm_master_videoram[offset+256*256]);
				if (data4 & 0x8000)
				{
					data4 &= 0x0fff;
				}
				else
				{
					data4 += 0x1000;
				}

				data1 = READ_WORD(&exterm_slave_videoram[offset]);
				data2 = READ_WORD(&exterm_slave_videoram[offset+256*256]);

				((unsigned short *)tmpbitmap ->line[y2     ])[x2  ] = Machine->pens[data3];
				((unsigned short *)tmpbitmap ->line[y2|0x80])[x2  ] = Machine->pens[data4];
				((unsigned short *)tmpbitmap1->line[y1     ])[x1  ] = Machine->pens[ (data1       & 0xff)];
				((unsigned short *)tmpbitmap1->line[y1     ])[x1+1] = Machine->pens[((data1 >> 8) & 0xff)];
				((unsigned short *)tmpbitmap2->line[y1     ])[x1  ] = Machine->pens[ (data2       & 0xff)];
				((unsigned short *)tmpbitmap2->line[y1     ])[x1+1] = Machine->pens[((data2 >> 8) & 0xff)];
			}
		}
		else
		{
			for (offset = 0; offset < 256*256; offset+=2)
			{
				int data1,data2,data3,data4,x1,y1,x2,y2;

				x1 = offset & 0xff;
				y1 = (offset >> 8) & 0xff;

				x2 = (offset >> 1) & 0xff;
				y2 = offset >> 9;

				data3 = READ_WORD(&exterm_master_videoram[offset]);
				if (data3 & 0x8000)
				{
					data3 &= 0x0fff;
				}
				else
				{
					data3 += 0x1000;
				}

				data4 = READ_WORD(&exterm_master_videoram[offset+256*256]);
				if (data4 & 0x8000)
				{
					data4 &= 0x0fff;
				}
				else
				{
					data4 += 0x1000;
				}

				data1 = READ_WORD(&exterm_slave_videoram[offset]);
				data2 = READ_WORD(&exterm_slave_videoram[offset+256*256]);

				tmpbitmap ->line[y2     ][x2  ] = Machine->pens[data3];
				tmpbitmap ->line[y2|0x80][x2  ] = Machine->pens[data4];
				tmpbitmap1->line[y1     ][x1  ] = Machine->pens[ (data1       & 0xff)];
				tmpbitmap1->line[y1     ][x1+1] = Machine->pens[((data1 >> 8) & 0xff)];
				tmpbitmap2->line[y1     ][x1  ] = Machine->pens[ (data2       & 0xff)];
				tmpbitmap2->line[y1     ][x1+1] = Machine->pens[((data2 >> 8) & 0xff)];
			}
		}
	}

	copybitmap(bitmap,tmpbitmap, 0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

    if (TMS34010_get_DPYSTRT(1) & 0x800)
	{
		copybitmap(bitmap,tmpbitmap2,0,0,0,0,&foregroundvisiblearea,TRANSPARENCY_PEN, palette_transparent_pen);
	}
	else
	{
		copybitmap(bitmap,tmpbitmap1,0,0,0,0,&foregroundvisiblearea,TRANSPARENCY_PEN, palette_transparent_pen);
	}
}

