/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

// Uncomment this if you want to see all slapstic accesses
//#define LOG_SLAPSTICK

unsigned char *atetris_paletteram;


static int slapstic_primed   = 0;
static int slapstic_bank     = 0x4000;
static int slapstic_nextbank = -1;
static int slapstic_75xxcnt  = 0;
static int slapstic_last60xx = 0;
static int slapstic_last75xx = 0;

#define BANK1 0x4000;
#define BANK2 0x10000;

// I'm not sure if the information here is sufficient to figure how to the
// Slapstic chip really works in this game, because BANK1 seem to be only
// used rarely.
//
// But it seems like that reading 6090 twice in a row can select either bank.
// The main difference between the 2 cases is that when BANK1 is selected,
// there are 2 LD A,75XXh instructions between the 6090 reads, while when
// BANK2 gets selected, there are 3.


int atetris_slapstic_r(int offset)
{
	extern unsigned char *RAM;


    if (slapstic_nextbank != -1)
    {
        slapstic_bank = slapstic_nextbank;
        slapstic_nextbank = -1;
    }

    if ((offset & 0xff00) == 0x2000 ||
        (offset & 0xff00) == 0x3500)
    {
        if (offset == 0x2000)
        {
            // Reset
            slapstic_75xxcnt  = 0;
            slapstic_last60xx = 0;
            slapstic_primed   = 1;
        }
        else if (offset >= 0x3500)
        {
            slapstic_75xxcnt++;
            slapstic_last75xx = (offset & 0xff);
        }
        else
        {
            if (slapstic_primed)
            switch (offset & 0xff)
            {
            case 0x80:
                {
                    slapstic_nextbank = BANK2;
#ifdef LOG_SLAPSTICK
                    if (errorlog) fprintf(errorlog, "Selecting Bank 2 at %04X\n", cpu_getpc());
#endif
                }
                break;

            case 0x90:
                if ((slapstic_75xxcnt == 0) ||
                    (slapstic_75xxcnt == 2 && slapstic_last60xx == 0x90))
                {
                    slapstic_nextbank = BANK1;
#ifdef LOG_SLAPSTICK
                    if (errorlog) fprintf(errorlog, "Selecting Bank 1 at %04X\n", cpu_getpc());
#endif
                }
                else
                {
                    slapstic_nextbank = BANK2;
#ifdef LOG_SLAPSTICK
                    if (errorlog) fprintf(errorlog, "Selecting Bank 2 at %04X\n", cpu_getpc());
#endif
                }
                break;

            case 0xa0:
                if (slapstic_last60xx == 0xb0)
                {
                    slapstic_nextbank = BANK1;
#ifdef LOG_SLAPSTICK
                    if (errorlog) fprintf(errorlog, "Selecting Bank 1 at %04X\n", cpu_getpc());
#endif
                }
                else
                {
                    slapstic_nextbank = BANK2;
#ifdef LOG_SLAPSTICK
                    if (errorlog) fprintf(errorlog, "Selecting Bank 2 at %04X\n", cpu_getpc());
#endif
                }
                break;

            case 0xb0:
                if (slapstic_75xxcnt == 6 && slapstic_last60xx == 0xb0 &&
                    slapstic_last75xx == 0x53)
                {
                    slapstic_nextbank = BANK1;
#ifdef LOG_SLAPSTICK
                    if (errorlog) fprintf(errorlog, "Selecting Bank 1 at %04X\n", cpu_getpc());
#endif
                }
                else
                {
                    slapstic_nextbank = BANK2;
#ifdef LOG_SLAPSTICK
                    if (errorlog) fprintf(errorlog, "Selecting Bank 2 at %04X\n", cpu_getpc());
#endif
                }
                break;

            default:
                slapstic_primed = 0;
            }

            slapstic_last60xx = (offset & 0xff);
            slapstic_75xxcnt = 0;
        }
    }
    else
    {
        slapstic_primed = 0;
    }

    return RAM[slapstic_bank + offset];
}



void atetris_palette_w(int offset,int data)
{
	int r,g,b;
	int bit0,bit1,bit2;


	atetris_paletteram[offset] = data;

	bit0 = (data >> 5) & 0x01;
	bit1 = (data >> 6) & 0x01;
	bit2 = (data >> 7) & 0x01;
	r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	bit0 = (data >> 2) & 0x01;
	bit1 = (data >> 3) & 0x01;
	bit2 = (data >> 4) & 0x01;
	g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
	bit0 = 0;
	bit1 = (data >> 0) & 0x01;
	bit2 = (data >> 1) & 0x01;
	b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

	palette_change_color(offset,r,g,b);
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void atetris_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* recalc the palette if necessary */
	if (palette_recalc ())
		memset (dirtybuffer,1,videoram_size);


	/* for every character in the backround RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0; offs < videoram_size; offs += 2)
	{
		int charcode,sx,sy,color;

		if (!dirtybuffer[offs] && !dirtybuffer[offs + 1]) continue;

		dirtybuffer[offs] = dirtybuffer[offs + 1] = 0;

		sy = 8 * (offs / 128);
		sx = 4 * (offs % 128);

		if (sx >= 42*8) continue;

		charcode = videoram[offs] | ((videoram[offs + 1] & 0x07) << 8);

		color = ((videoram[offs + 1] & 0xf0) >> 4);

		drawgfx(tmpbitmap,Machine->gfx[0],
				charcode,
				color,
				0,0,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}
