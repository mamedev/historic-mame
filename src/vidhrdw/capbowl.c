/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *capbowl_scanline;

static unsigned char *raw_video_ram;
static unsigned char *line_pens;
static unsigned char *dirtypalette;



void capbowl_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom     )
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
int capbowl_vh_start(void)
{
	if ((dirtypalette = malloc(256)) == 0)
		return 1;
	memset(dirtypalette,1,256);

	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(dirtypalette);
		return 1;
	}

	if ((raw_video_ram = malloc(256 * 256)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
		free(dirtypalette);
		return 1;
	}
	if ((line_pens = malloc(256 * 16)) == 0)
	{
		free(raw_video_ram);
		osd_free_bitmap(tmpbitmap);
		free(dirtypalette);
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void capbowl_vh_stop(void)
{
	free(line_pens);
	free(raw_video_ram);
	osd_free_bitmap(tmpbitmap);
	free(dirtypalette);
}


void capbowl_videoram_w(int offset,int data)
{
	if (raw_video_ram[*capbowl_scanline * 256 + offset] != data)
	{
		raw_video_ram[*capbowl_scanline * 256 + offset] = data;

		if (offset >= 0x20)
		{
			if (dirtypalette[*capbowl_scanline] == 0 && offset < 212)
			{
				tmpbitmap->line[359 - ((offset-0x20) << 1)][*capbowl_scanline] = line_pens[*capbowl_scanline * 16 + (data >> 4)  ];
				tmpbitmap->line[358 - ((offset-0x20) << 1)][*capbowl_scanline] = line_pens[*capbowl_scanline * 16 + (data & 0x0f)];
			}
		}
		else
		{
			/* Offsets 0-1f are the palette */
			dirtypalette[*capbowl_scanline] = 1;
		}
	}
}


int capbowl_videoram_r(int offset)
{
	return raw_video_ram[*capbowl_scanline * 256 + offset];
}




/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void capbowl_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int line;


	/* Remap colors */
	for (line = 0;line < 256;line++)
	{
		if (dirtypalette[line])
		{
			int i;
			unsigned char *pens = &line_pens[line * 16];
			int r,g,b;


			dirtypalette[line] = 0;

			for (i = 0;i < 0x20;i += 2)
			{
				r = (raw_video_ram[line * 256 + i] & 0x0f);
				g = (raw_video_ram[line * 256 + i + 1] & 0xf0) >> 4;
				b = (raw_video_ram[line * 256 + i + 1] & 0x0f);

				pens[i / 2] = Machine->pens[((r & 0x0e) >> 1) | ((g & 0x0e) << 2) | ((b & 0x0c) << 4)];
			}

			for (i = 0x20; i < 212; i++)
			{
				int data2 = raw_video_ram[line * 256 + i];

				tmpbitmap->line[359 - ((i-0x20) << 1)][line] = pens[data2 >> 4  ];
				tmpbitmap->line[358 - ((i-0x20) << 1)][line] = pens[data2 & 0x0f];
			}
		}
	}

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}
