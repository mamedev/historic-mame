/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#include "osdepend.h"

unsigned char *sidearms_bg_scrollx;
unsigned char *sidearms_bg_scrolly;
static unsigned char *dirtybufferpal;
unsigned char *sidearms_paletteram;
int sidearms_paletteram_size;


void sidearms_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
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

int sidearms_vh_start()
{
	if (generic_vh_start() != 0)
		return 1;

        /* Palette RAM dirty buffer */
        if ((dirtybufferpal = malloc(sidearms_paletteram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
        memset(dirtybufferpal,1,sidearms_paletteram_size);

	return 0;

}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void sidearms_vh_stop(void)
{
        free(dirtybufferpal);
        generic_vh_stop();
}

void sidearms_paletteram_w(int offset,int data)
{
        if (sidearms_paletteram[offset] != data)
	{
                dirtybufferpal[offset]=1;
                sidearms_paletteram[offset] = data;
	}
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void sidearms_vh_screenrefresh(struct osd_bitmap *bitmap)
{
        int offs, sx, sy;
        int j, i;
        int bg_scrolly, bg_scrollx;
        unsigned char *p=Machine->memory_region[3];

        static unsigned char chTableRED[0x10]=
        {
            0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03,
            0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07
        };

        static unsigned char chTableGREEN[0x10]=
        {
            0x00, 0x08, 0x08, 0x08, 0x10, 0x10, 0x18, 0x18,
            0x20, 0x20, 0x28, 0x28, 0x30, 0x30, 0x38, 0x38
        };

        static unsigned char chTableBLUE[0x10]=
        {
            0x00, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
            0x80, 0x80, 0x80, 0x80, 0xc0, 0xc0, 0xc0, 0xc0
        };

        /* rebuild the colour lookup table from RAM palette */
        for (j=0; j<3; j++)
	{
                /*
                     0000-0100:  background palettes. (16x16 colours)
                     0200-0280:  sprites palettes.    ( 8x16 colours)
                     0300-0340:  characters palettes  (32x4 colours)
                */
                            /* CHARS  TILES   SPRITES */
                int start[3]={0x0300, 0x0000, 0x0200};
                int count[3]={0x0080, 0x0100, 0x0100};
                int base=start[j];
                int bluebase=base+0x0400;
                int max=count[j];

                for (i=0; i<max; i++)
                {
                    if (dirtybufferpal[base] || dirtybufferpal[bluebase])
                    {
                        int red, green, blue, redgreen;

                        redgreen=sidearms_paletteram[base];
                        red=redgreen >>4;
                        green=redgreen & 0x0f ;
                        blue=sidearms_paletteram[bluebase]&0x0f;

                        dirtybufferpal[base] = dirtybufferpal[bluebase] = 0;

                        offs     = chTableGREEN[green];
                        offs    |= chTableRED[red];
                        offs    |= chTableBLUE[blue];

                        Machine->gfx[j]->colortable[i] = Machine->pens[offs];
                    }
                    base++;
                    bluebase++;
                }
        }

        bg_scrollx = sidearms_bg_scrollx[0] + 256 * sidearms_bg_scrollx[1];
        bg_scrolly = 0; //sidearms_bg_scrolly[0];
        offs  = 16 * ((bg_scrollx>>5)+2);
        offs += 2*(bg_scrolly>>5);

        bg_scrolly&=0x1f;
        bg_scrollx&=0x1f;

        /* Draw the entire background scroll */
        for (sx = 0; sx<14; sx++)
	{
                offs &= 0x7fff; /* Enforce limits (for top of scroll) */
                for (sy = 0;sy < 9;sy++)
		{                        
                        int tile, attr, offset;
                        offset=offs+(sy*2)%16;

                        tile=p[offset];
                        attr=p[offset+1];
                        tile+=256*(attr&0x01); /* Maybe */
                        drawgfx(bitmap,Machine->gfx[1],
                                tile,
                                (attr&0x38)>>3, /* Wrong */
                                attr&0x02, attr&0x04,
                                sx*32-bg_scrollx, sy*32+bg_scrolly-32,
                                &Machine->drv->visible_area,
                                TRANSPARENCY_NONE,0);
                }
                offs+=0x10;
        }


	/* Draw the sprites. */
        for (offs = spriteram_size - 32;offs >= 0;offs -= 32)
	{
                int attr=spriteram[offs + 1];
                int sprite=attr&0xe0;   /* Possibly wrong */
                sprite<<=3;

                drawgfx(bitmap,Machine->gfx[2],
                                spriteram[offs]+sprite,
                                attr & 0x0f,  /* seems OK */   
                                0, 0,   /* No flipping ? */
                                spriteram[offs + 3]+((attr&0x10)<<4)-0x040,
                                spriteram[offs + 2],
                                &Machine->drv->visible_area,
                                TRANSPARENCY_PEN,15);
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
        for (i=0; i<0x1e; i++)
        {
                offs=i*0x40+8;
                for (j=0; j<0x30; j++)
                {
                        int code=videoram[offs] + (((int)(colorram[offs] & 0xc0)<<2));
                        if (code != 0x27)     /* don't draw spaces */
                        {
                                int sx,sy;

                                sy = 8 * i;
                                sx = 8 * j;

                                drawgfx(bitmap,Machine->gfx[0],
                                        code,
                                        colorram[offs] & 0x1f,
                                        0,0,sx,sy,
                                        &Machine->drv->visible_area,TRANSPARENCY_PEN,3);
                        }
                        offs++;

                }
	}
}






