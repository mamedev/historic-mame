/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *cop01_vram;
int cop01_vram_size;
unsigned char cop01_scrollx[1];

unsigned char *cop01_spram;
unsigned char cop01_gfxbank = 0;

static struct osd_bitmap *tmpbitmap2;
static unsigned char *dirtybuffer2;

void cop01_videoram_w(int offset,int data)
{
        if (cop01_vram[offset] != data)
        {  dirtybuffer2[offset] = 1;
           cop01_vram[offset] = data;}
}

/***************************************************************************
  Stop the video hardware emulation.
***************************************************************************/

void cop01_vh_stop(void)
{
        free(dirtybuffer2);
	osd_free_bitmap(tmpbitmap2);
	generic_vh_stop();
}

/***************************************************************************
  Start the video hardware emulation.
***************************************************************************/


int cop01_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

        if ((dirtybuffer2 = malloc(cop01_vram_size)) == 0)
	{
		cop01_vh_stop();
		return 1;
	}
        memset(dirtybuffer2,1,cop01_vram_size);

        if ((tmpbitmap2 = osd_new_bitmap(2*Machine->drv->screen_width,
                1*Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
	{       cop01_vh_stop();
		return 1;
	}

	return 0;
}


/***************************************************************************
  Convert color prom.
***************************************************************************/

void cop01_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;

		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[2*Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[2*Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[2*Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[2*Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}

}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

/* 0xf000-0xf3ff  tiles */
void cop01_fg(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;

        for (offs = 0x400 ;offs >= 0;offs--)
	{
                int numtile = videoram[offs];
		int sx,sy;

                sx = offs % 32;
		sy = offs / 32;

                if (numtile != 0)
                {
		drawgfx(bitmap,Machine->gfx[0],
		   numtile,
		   0, // Color
		   0,0,// FlipX, FlipY
		   8*sx,8*sy,
		   &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
                }
	}
}

/* 0xd000-0xd7ff  tiles
   0xd800-0xdfff  attributes */

void cop01_bg_old(struct osd_bitmap *bitmap,int full_refresh)
{
	int x,y;

        for (y = 0; y < 32; y++)
        { for (x = 0; x < 32; x++)
          {
             int numtile = cop01_vram[x+y*64];
             int bank = cop01_vram[x+y*64+0x800]&3;

	     drawgfx(bitmap,Machine->gfx[1+bank],
		   numtile,
		   0,
		   0,0,
		   8*x,8*y,
		   &Machine->drv->visible_area,TRANSPARENCY_NONE,0);
          }
        }
}


void cop01_bg(struct osd_bitmap *bitmap,int full_refresh)
{
	int x,y;

        for (y = 0; y < 32; y++)
        { for (x = 0; x < 64; x++)
          {
             int numtile = cop01_vram[x+y*64];
             int bank = cop01_vram[x+y*64+0x800]&3;

	     if ((dirtybuffer2[x + y*64]) || (dirtybuffer2[x+ y*64+0x800]))
	     {
                dirtybuffer2[x+ y*64] = dirtybuffer2[x + y*64+0x800] = 0;

                drawgfx(tmpbitmap2,Machine->gfx[1+bank],
		   numtile,
		   0,
		   0,0,
		   8*x,8*y,
		   0,TRANSPARENCY_NONE,0);
             }
          }
        }

        /* copy the background graphics */
	{
                int scrollx,scrolly;

                scrollx = -(cop01_scrollx[0]+ (256*cop01_scrollx[1]));
                scrolly = 0;

		copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,
                &scrolly,&Machine->drv->visible_area,
                TRANSPARENCY_NONE,0);
	}
}


/* YY NN AT XX (probably)
   e000-e0ff  -> 64 sprites
*/
void cop01_sp(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;

        /* Sobre los sprites:

           La pantalla del titulo:
             * NumTile > 0x80  (at used. 0x00, 0x01, 0x05)
               Todos pertenecen a la misma pagina, asi 0x01 no puede
               ser el mapeador y 0x04 tampoco porque es flipx.

           Intro tras pulsar start:
             * NumTile < 0x80 (at used. 0x00, 0x01)
               Todos pertenecen a la misma pagina.

        */

        for (offs = 0 ;offs < 0x100;offs+=4)
	{
                int attb = cop01_spram[offs+2];
                int numtile = cop01_spram[offs+1]&0x7f;
                int bank = ((cop01_spram[offs+1]&0x80)>>7);
                int flipx = (attb&4)>>2;
		int sx,sy;

                if ((bank == 1) && ((cop01_gfxbank&0x20) == 0))
                bank |= 0x2;

                sy = cop01_spram[offs];
		sx = (cop01_spram[offs+3]+0x80)&0xff;

                if ((sx != 0) && (sy != 0))
		{ drawgfx(bitmap,Machine->gfx[5+bank],
		   numtile,
		   0, /* Color */
		   flipx,0,
		   sx,0xf0-sy,
		   &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
                }
	}
}

void cop01_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
  cop01_bg(bitmap,full_refresh);
  cop01_sp(bitmap,full_refresh);
  cop01_fg(bitmap,full_refresh);
}

