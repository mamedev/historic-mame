/***************************************************************************

	Battlelane

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct osd_bitmap *screen_bitmap;

int battlane_bitmap_size;
unsigned char *battlane_bitmap;


void battlane_bitmap_w(int offset, int data)
{
	int i,n, orval;

	unsigned char *RAM =
		Machine->memory_region[Machine->drv->cpu[0].memory_region];
#if 0
	if (errorlog)
	{
		fprintf(errorlog, "%04x=%02x   (0x1c00=%02x)\n",
			offset+0x2000, data, RAM[0x1c00]);
	}
#endif

	orval=(RAM[0x1c00]>>2)&0x07;

	if (orval==0)
		orval=7;

	for (i=0; i<8; i++)
	{
		if (data & 1<<i)
		{
		    screen_bitmap->line[(offset / 0x100) * 8+i][(0x2000-offset) % 0x100]|=orval;
		}
		else
		{
		    screen_bitmap->line[(offset / 0x100) * 8+i][(0x2000-offset) % 0x100]&=~orval;
		}

	}

	battlane_bitmap[offset]=data;
}

int battlane_bitmap_r(int offset)
{
	return battlane_bitmap[offset];
}


void battlane_vh_convert_color_prom (unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	unsigned char *pal;

	pal = palette;

	/* Set all the 256 colors because we need them to remap the color 0 */
	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2;

		/* red component */
		bit0 = (i >> 0) & 0x01;
		bit1 = (i >> 1) & 0x01;
		bit2 = (i >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (i >> 3) & 0x01;
		bit1 = (i >> 4) & 0x01;
		bit2 = (i >> 5) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (i >> 6) & 0x01;
		bit2 = (i >> 7) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	}

}







/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int battlane_vh_start(void)
{
	screen_bitmap = osd_create_bitmap(0x20*8, 0x20*8);
	if (!screen_bitmap)
	{
		return 1;
	}

	battlane_bitmap=malloc(battlane_bitmap_size);
	if (!battlane_bitmap)
	{
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void battlane_vh_stop(void)
{
	if (screen_bitmap)
	{
		osd_free_bitmap(screen_bitmap);
	}
	if (battlane_bitmap)
	{
		free(battlane_bitmap);
	}
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void battlane_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	static int s;
	int x,y, offs;
	unsigned char *RAM =
		Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if (osd_key_pressed(OSD_KEY_PGDN))
	{
		while (osd_key_pressed(OSD_KEY_PGDN)) ;
		s++;
	}

	if (osd_key_pressed(OSD_KEY_PGUP))
	{
		while (osd_key_pressed(OSD_KEY_PGUP)) ;
		s--;
	}




	/* Blank screen */
	fillbitmap(bitmap,0,&Machine->drv->visible_area);

	for (y=0; y<32;y++)
	{
		for (x=0; x<32; x++)
		{
			int sx=32-((s+x)&0x1f);
			int code;

			code=RAM[0x1200+((15-y)*16)+
				(sx&0x0f)+0x10*(sx&0x10)];
			if (code)
			{
				drawgfx(bitmap,Machine->gfx[1],
					 code,
					 0,
					 0,1,
					 x*16,y*16,
					 &Machine->drv->visible_area,
					 TRANSPARENCY_NONE, 0);
			}
		}
	}

	for (offs=0x0100-4; offs>=0; offs-=4)
	{
	      int code=RAM[0x00f3+offs];
	      if (!(RAM[0xf1]&0x80))
	      {
	      drawgfx(bitmap,Machine->gfx[0],
					 code,
					 RAM[0xf1+offs]&0x07,
					 0,1,
					 256-RAM[0x00f2+offs],
					 RAM[0x00f0+offs],
					 &Machine->drv->visible_area,
					 TRANSPARENCY_PEN, 15);
	       }
	}

	copybitmap(bitmap,screen_bitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
}
