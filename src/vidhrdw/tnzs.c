/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "ctype.h"



extern unsigned char *tnzs_objram;
extern unsigned char *tnzs_vdcram;
extern unsigned char *tnzs_scrollram;
int tnzs_objram_size;
extern unsigned char *banked_ram_0, *banked_ram_1;


unsigned char *tnzs_objectram;
int tnzs_objectram_size;
static struct osd_bitmap *tnzs_column[16];
static int tnzs_dirty_map[32][16];
static int tnzs_screenflip, tnzs_insertcoin;

/***************************************************************************

  The New Zealand Story doesn't have a color PROM. It uses 1024 bytes of RAM
  to dynamically create the palette. Each couple of bytes defines one
  color (15 bits per pixel; the top bit of the second byte is unused).
  Since the graphics use 4 bitplanes, hence 16 colors, this makes for 32
  different color codes.

***************************************************************************/



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int tnzs_vh_start(void)
{
    int column,x,y;
    for (column=0;column<16;column++)
    {
        if ((tnzs_column[column] = osd_create_bitmap(32, 16 * 16)) == 0)
        {
            /* Free all the columns */
            for (column--;column;column--)
                osd_free_bitmap(tnzs_column[column]);
            return 1;
        }
    }

    for (x=0;x<32;x++)
    {
        for (y=0;y<16;y++)
        {
            tnzs_dirty_map[x][y] = -1;
        }
    }

    tnzs_insertcoin = 0;
	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void tnzs_vh_stop(void)
{
    int column;

    /* Free all the columns */
    for (column=0;column<16;column++)
        osd_free_bitmap(tnzs_column[column]);
}



void tnzs_videoram_w(int offset,int data)
{
    videoram[offset] = data;
}



void tnzs_objectram_w(int offset,int data)
{
    tnzs_objectram[offset] = data;
}



void tnzs_vh_draw_background(struct osd_bitmap *bitmap,
					  unsigned char *m)
{
    int i, b,c,tile,color, x,y, column;
    int scrollx, scrolly;
    unsigned int upperbits;

    /* The screen is split into 16 columns.
       So first, update the tiles. */
    for (i=0,column=0;column<16;column++)
    {
        for (y=0;y<16;y++)
        {
            for (x=0;x<2;x++,i++)
            {
                c = m[i];
                b = m[i + 0x1000] & 0x1f;
                color = m[i + 0x1200] >> 3; /* colours at d600-d7ff */

                /* Construct unique identifier for this tile/color */
                tile = (color << 16) + (b << 8) + c;

                if (tnzs_dirty_map[column*2+x][y] != tile)
                {
                    tnzs_dirty_map[column*2+x][y] = tile;

                    drawgfx(tnzs_column[column],
                        Machine->gfx[0],            /* bank */
                        b*0x100+c,                  /* code */
                        color,                      /* color */
                        tnzs_screenflip, 0,         /* flipx, flipy */
                        x*16,                       /* x */
                        y*16,                       /* y */
                        0, TRANSPARENCY_NONE, 0);   /* other stuff */
                }
            }
    	}
	}

    /* If the byte at f301 has bit 0 clear, then don't draw the
       background tiles */
    if ((tnzs_scrollram[0x101] & 1) == 0) return;

    /* The byte at f200 is the y-scroll value for the first column.
       The byte at f204 is the LSB of x-scroll value for the first column.

       The other columns follow at 16-byte intervals.

       The 9th bit of each x-scroll value is combined into 2 bytes
       at f302-f303 */

    /* First draw the background layer (8 columns) */
    upperbits = tnzs_scrollram[0x102];
    for (column=7;column >= 0;column--)
    {
        scrollx = tnzs_scrollram[column*16+4]
                - ((upperbits & 0x80) * 2);
        scrolly = -15 - tnzs_scrollram[column*16];

        copybitmap(bitmap,tnzs_column[column+8], 0,0, scrollx,scrolly,
                   &Machine->drv->visible_area,
                   TRANSPARENCY_COLOR,0);
        copybitmap(bitmap,tnzs_column[column+8], 0,0, scrollx,scrolly+(16*16),
                   &Machine->drv->visible_area,
                   TRANSPARENCY_COLOR,0);

        upperbits <<= 1;
    }

    /* If the byte at f301 has bit 3 clear, then don't draw columns 9-15

       This bit might have another meaning. For instance, it may just
       reverse the layer priority, which would have the same effect.
       However, the effect in TNZS is to not display columns 9-15, so
       we'll use that because it's quicker. */

    upperbits = tnzs_scrollram[0x103];
    /* If bit 3 is set, skip ahead to just do column 8 */
    if (tnzs_scrollram[0x101] & 8)
    {
        column = 8;
        upperbits <<= 7;
    } else {
        column = 15;
    }

    for (;column >= 8;column--)
    {
        scrollx = tnzs_scrollram[column*16+4]
                - ((upperbits & 0x80) * 2);
        scrolly = -15 - tnzs_scrollram[column*16];

        copybitmap(bitmap,tnzs_column[column-8], 0,0, scrollx,scrolly,
                   &Machine->drv->visible_area,
                   TRANSPARENCY_COLOR,0);
        copybitmap(bitmap,tnzs_column[column-8], 0,0, scrollx,scrolly+(16*16),
                   &Machine->drv->visible_area,
                   TRANSPARENCY_COLOR,0);

        upperbits <<= 1;
    }
}

void tnzs_vh_draw_foreground(struct osd_bitmap *bitmap,
							 unsigned char *char_pointer,
							 unsigned char *x_pointer,
							 unsigned char *y_pointer,
							 unsigned char *ctrl_pointer,
                             unsigned char *color_pointer)
{
    int i, c, flipscreen;

    flipscreen = tnzs_screenflip << 7;

    /* Draw all 512 sprites */
    for (i=0x1ff;i >= 0;i--)
	{
		c = char_pointer[i];
        if (y_pointer[i] < 0xf8) /* f8-ff is off-screen, so don't draw it */
		{
            drawgfx(bitmap,
                    Machine->gfx[0],
                    (ctrl_pointer[i] & 31) * 0x100 + c,      /* code */
                    color_pointer[i] >> 3,                   /* color */

                    (ctrl_pointer[i] & 0x80) ^ flipscreen,
                    0,                                       /* flipx, flipy */

                    x_pointer[i] - (color_pointer[i]&1)*256, /* x */
                    14*16 + 2 - y_pointer[i],                /* y */
                    0,                  /* clip */
                    TRANSPARENCY_PEN,   /* transparency */
                    0);                 /* transparent_color */
		}
	}
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
extern int number_of_credits;
void tnzs_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
    int color,code,i,offs,x,y, t;
    int colmask[32];

    /* Update credit counter. The game somehow tells the hardware when
       to decrease the counter. (maybe by writing to c001 on the 2nd cpu?) */

    t = osd_key_pressed(OSD_KEY_3);
    if (t && !tnzs_insertcoin)
	{
		number_of_credits++;
	}
    tnzs_insertcoin = t;

    /* If the byte at f300 has bit 6 set, flip the screen
       (I'm not 100% sure about this) */
    tnzs_screenflip = (tnzs_scrollram[0x100] & 0x40) >> 6;

    /* Remap dynamic palette */
    memset(palette_used_colors,PALETTE_COLOR_UNUSED,Machine->drv->total_colors * sizeof(unsigned char));

    for (color = 0;color < 32;color++) colmask[color] = 0;

    /* See what colours the tiles need */
    for (offs=32*16 - 1;offs >= 0;offs--)
	{
        code = tnzs_objram[offs + 0x400]
             + 0x100 * (tnzs_objram[offs + 0x1400] & 0x1f);
        color = tnzs_objram[offs + 0x1600] >> 3;

        colmask[color] |= Machine->gfx[0]->pen_usage[code];
	}

    /* See what colours the sprites need */
    for (offs=0x1ff;offs >= 0;offs--)
	{
        code = tnzs_objram[offs]
             + 0x100 * (tnzs_objram[offs + 0x1000] & 0x1f);
        color = tnzs_objram[offs + 0x1200] >> 3;

        colmask[color] |= Machine->gfx[0]->pen_usage[code];
	}

    /* Construct colour usage table */
    for (color=0;color<32;color++)
	{
		if (colmask[color] & (1 << 0))
            palette_used_colors[16 * color] = PALETTE_COLOR_TRANSPARENT;
        for (i=1;i<16;i++)
		{
			if (colmask[color] & (1 << i))
                palette_used_colors[16 * color + i] = PALETTE_COLOR_USED;
		}
	}

    if (palette_recalc())
    {
        for (x=0;x<32;x++)
        {
            for (y=0;y<16;y++)
            {
                tnzs_dirty_map[x][y] = -1;
            }
        }
    }

    /* Blank the background */
    fillbitmap(bitmap, Machine->pens[0], &Machine->drv->visible_area);

    /* Redraw the background tiles (c400-c5ff) */
    tnzs_vh_draw_background(bitmap, tnzs_objram + 0x400);

    /* Draw the sprites on top */
	tnzs_vh_draw_foreground(bitmap,
                            tnzs_objram + 0x0000, /*  chars : c000 */
                            tnzs_objram + 0x0200, /*      x : c200 */
                            tnzs_vdcram + 0x0000, /*      y : f000 */
                            tnzs_objram + 0x1000, /*   ctrl : d000 */
                            tnzs_objram + 0x1200); /* color : d200 */

}
