/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

  aeroboto (preliminary)


Revision:

4-18-2002 Acho A. Tang
- rewrote VIDEO_UPDATE
- code unoptimized but speed is not an issue

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


#if 0
unsigned char *aeroboto_videoram;
unsigned char *aeroboto_fgscroll,*aeroboto_bgscroll;

int aeroboto_charbank;



/***************************************************************************

  Draw the game screen in the given mame_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
VIDEO_UPDATE( aeroboto )
{
	int offs;


	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy;


		sx = offs % 32;
		sy = offs / 32;

		drawgfx(bitmap,Machine->gfx[0],
				videoram[offs] + 256 * aeroboto_charbank,
				0,
				0,0,
				8*sx - aeroboto_bgscroll[sy],8*sy,
				&Machine->visible_area,TRANSPARENCY_NONE,0);
		drawgfx(bitmap,Machine->gfx[0],
				videoram[offs] + 256 * aeroboto_charbank,
				0,
				0,0,
				8*sx - aeroboto_bgscroll[sy] + 256,8*sy,
				&Machine->visible_area,TRANSPARENCY_NONE,0);
	}

	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy;

		sx = offs % 32;
		sy = offs / 32;

		drawgfx(bitmap,Machine->gfx[0],
				aeroboto_videoram[offs] + 256 * aeroboto_charbank,
				0,
				0,0,
				8*sx - aeroboto_fgscroll[sy],8*sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
		drawgfx(bitmap,Machine->gfx[0],
				aeroboto_videoram[offs] + 256 * aeroboto_charbank,
				0,
				0,0,
				8*sx - aeroboto_fgscroll[sy] + 256,8*sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}

	for (offs = spriteram_size-4;offs >= 0;offs -= 4)
	{
		int sx,sy;


		sx = spriteram[offs + 3];
		sy = 239 - spriteram[offs];

		drawgfx(bitmap,Machine->gfx[2],
				spriteram[offs + 1],
				spriteram[offs + 2] & 0x0f,
				0,0,
				sx,sy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
	}
}
#endif

data8_t *aeroboto_hscroll, *aeroboto_vscroll, *aeroboto_tilecolor;
data8_t *aeroboto_starx, *aeroboto_stary, *aeroboto_starcolor;
int aeroboto_fgfill;
int aeroboto_charbank;

VIDEO_UPDATE( aeroboto )
{
        static unsigned int lx=0, ly=0, sx=0, sy=0;
        static int debug = 0;
        struct GfxElement *gfx;
        int xoffs, yoffs, x, y, xend, yend, xdisp, ydisp, bankbase, i, j;

if (code_pressed_memory(KEYCODE_F1)) debug ^= 1;
if (debug) aeroboto_fgfill = 0;

        bankbase = aeroboto_charbank << 8;

        // draw star map (total guesswork)
        if (!aeroboto_fgfill)
        {
                gfx = Machine->gfx[1];

                x = *aeroboto_starx;
                i = x - lx;
                lx = x;
                if (i<-128) i+=0x100; else if (i>127) i-=0x100;
                sx += i;
                i = (sx >> 3) & 0xff;
                xdisp = -(i & 7);
                xoffs = i >> 3;
                xend = xdisp + 256;

                y = *aeroboto_stary;
                i = y - ly;
                ly = y;
                if (i<-128) i+=0x100; else if (i>127) i-=0x100;
                if (*aeroboto_vscroll != 0xff) sy += i;
                i = (sy >> 3) & 0xff;
                j = (i >> 3) + 3;
                ydisp = 24 -(i & 7);
                yoffs = (j<<5) & 0x1ff;
                yend = ydisp + 224;

                j = *aeroboto_starcolor;

                for (y=ydisp; y<yend; y+=8)
                {
                        for (x=xdisp; x<xend; xoffs++, x+=8)
                        {
                                i = yoffs + (xoffs & 0x1f);
                                drawgfx(bitmap, gfx, i, j,
                                        0, 0, x, y, cliprect, TRANSPARENCY_NONE, 0);
                        }
                        yoffs = (yoffs + 32) & 0x1ff;
                }
        }

        // draw background
        gfx = Machine->gfx[0];

        i = *aeroboto_vscroll + 1;
        j = (i >> 3) + 5;
        ydisp = 40 -(i & 7);
        yoffs = j << 5;
        yend = ydisp + 208;

        for (y=ydisp; y<yend; j++, yoffs+=32, y+=8)
        {
                i = aeroboto_hscroll[j];
                xdisp = -(i & 7);
                xoffs = i >> 3;
                xend = xdisp + 256;

                for (x=xdisp; x<xend; xoffs++, x+=8)
                {
                        i = videoram[yoffs+(xoffs&0x1f)];
                        if (aeroboto_fgfill || i!=0x5a)
                                drawgfx(bitmap, gfx, bankbase+i, aeroboto_tilecolor[i],
                                        0, 0, x, y, cliprect, TRANSPARENCY_NONE, 0);
                }
        }

        // draw split screen
        gfx = Machine->gfx[0];

        for (j=0, yoffs=0, y=0; y<40; j++, yoffs+=32, y+=8)
        {
                i = aeroboto_hscroll[j];
                xdisp = -(i & 7);
                xoffs = i >> 3;
                xend = xdisp + 256;

                for (x=xdisp; x<xend; xoffs++, x+=8)
                {
                        i = videoram[yoffs+(xoffs&0x1f)];
                        if (aeroboto_fgfill || i!=0x5a)
                                drawgfx(bitmap, gfx, bankbase+i, aeroboto_tilecolor[i],
                                        0, 0, x, y, cliprect, TRANSPARENCY_NONE, 0);
                }
        }

        // draw sprites
        gfx = Machine->gfx[2];

        for (i=0; i<spriteram_size; i+=4)
                if ((j = spriteram[i+1]))
                {
                        x = spriteram[i+3];
                        y = (0xf0-spriteram[i]) & 0xff;

                        drawgfx(bitmap, gfx, j, spriteram[i+2],
                                0, 0, x, y, cliprect, TRANSPARENCY_PEN, 0);
                }
}
