/***************************************************************************

	Battlelane

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct osd_bitmap *screen_bitmap;

int battlane_bitmap_size;
unsigned char *battlane_bitmap;
static int battlane_video_ctrl;

int battlane_spriteram_size;
unsigned char *battlane_spriteram;


void battlane_video_ctrl_w(int offset, int data)
{
#if 1
	if (errorlog)
	{
                fprintf(errorlog, "Video control =%02x)\n", data);
	}
#endif

	battlane_video_ctrl=data;
}

int battlane_video_ctrl_r(int offset)
{
	return battlane_video_ctrl;
}

void battlane_bitmap_w(int offset, int data)
{
	int i, orval;

#if 0
	unsigned char *RAM =
		Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if (errorlog)
	{
		fprintf(errorlog, "%04x=%02x   (0x1c00=%02x)\n",
			offset+0x2000, data, RAM[0x1c00]);
	}
#endif

    orval=(~battlane_video_ctrl>>1)&0x07;

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

#if 0
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

#endif



#ifdef MAME_DEBUG
void battlane_dump_bitmap(void)
{
    int i;
    FILE *fp=fopen("SCREEN.DMP", "w+b");
    if (fp)
    {
        for ( i=0; i<0x20*8-1; i++)
        {
            fwrite(screen_bitmap->line[i], 0x20*8, 1, fp);
        }
        fclose(fp);
    }
	fp=fopen("SPRITES.DMP", "w+b");
	if (fp)
	{
		fwrite(battlane_spriteram, 0x100, 1, fp);
		fclose(fp);
	}


}
#endif




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

	memset(battlane_spriteram, 0, battlane_spriteram_size);

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

  Build palette from palette RAM

***************************************************************************/

INLINE void battlane_build_palette(void)
{
	int offset;
    unsigned char *PALETTE =
        Machine->memory_region[3];

/*    unsigned char *RAM =
		Machine->memory_region[Machine->drv->cpu[0].memory_region];
 */
    for (offset = 0; offset < 0x40; offset++)
	{
          int palette = PALETTE[offset];
          int red, green, blue;
          red   = (palette&0x07) * 16*2;
          green = ((palette>>3)&0x07) * 16*2;
          blue  = ((palette>>6)&0x07) * 16*4;
          palette_change_color (offset, red, green, blue);
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

	if (keyboard_pressed(KEYCODE_PGDN))
	{
		while (keyboard_pressed(KEYCODE_PGDN)) ;
		s++;
	}

	if (keyboard_pressed(KEYCODE_PGUP))
	{
		while (keyboard_pressed(KEYCODE_PGUP)) ;
		s--;
	}


    battlane_build_palette();
	if (palette_recalc ())
    {

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
	      int code=battlane_spriteram[offs+3]+((battlane_spriteram[offs+1]&0x80)<<1);
          //if (battlane_spriteram[offs+1])
	      {
               drawgfx(bitmap,Machine->gfx[0],
					 code,
					 battlane_spriteram[offs+1]&0x07,
					 0,1,
					 battlane_spriteram[offs+2],
					 battlane_spriteram[offs],
					 &Machine->drv->visible_area,
					 TRANSPARENCY_PEN, 15);
          }
	}

    for (y=0; y<0x20*8; y++)
    {
        for (x=0; x<0x20*8; x++)
        {
            int data=screen_bitmap->line[y][x];
            if (data)
            {
                bitmap->line[y][x]=Machine->pens[data];
            }
       }
	}

#ifdef MAME_DEBUG
    if (keyboard_pressed(KEYCODE_SPACE))
    {
        /* Display current palette */
        const int numblocks=0x08;
        const int left=(0x20*8-numblocks*16)/2;
        int pal;
        int nX=left;
        int nY=64;
        for (pal=0; pal<0x40; pal++)
        {
            for (y=0; y<16;y++)
            {
                for (x=0; x<16;x++)
                {
                    bitmap->line[nY+y][nX+x]=Machine->pens[pal];
                }
            }
            nX+=16;
            if (nX >= left+numblocks*16)
            {
                nX=left;
                nY+=16;
            }
        }
    }
    if (keyboard_pressed(KEYCODE_S))
    {
         battlane_dump_bitmap();
    }
#endif

}
