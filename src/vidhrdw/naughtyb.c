/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *naughtyb_videoram2;

/* use these to draw charset B */
unsigned char *naughtyb_scrollreg;

/* video/control register 1  */
static unsigned char videoctlreg;

/* use this to select palette */
static unsigned char palreg;

/* used in Naughty Boy to select video bank */
static int bankreg;


static struct rectangle scrollvisiblearea =
{
	0*8, 28*8-1,
	2*8, 34*8-1
};

static struct rectangle topvisiblearea =
{
	0*8, 28*8-1,
	0*8, 2*8-1
};

static struct rectangle bottomvisiblearea =
{
	0*8, 28*8-1,
	34*8, 36*8-1
};



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int naughtyb_vh_start(void)
{
	palreg    = 0;
	bankreg   = 0;
	videoctlreg = 0;

	/* Naughty Boy has a virtual screen twice as large as the visible screen */
	if ((dirtybuffer = malloc(videoram_size)) == 0) return 1;
	memset(dirtybuffer,0,videoram_size);

	if ((tmpbitmap = osd_create_bitmap(28*8,68*8)) == 0)
	{
		free(dirtybuffer);
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void naughtyb_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
	free(dirtybuffer);
}



void naughtyb_videoram2_w(int offset,int data)
{
	if (naughtyb_videoram2[offset] != data)
	{
		dirtybuffer[offset] = 1;

		naughtyb_videoram2[offset] = data;
	}
}



void naughtyb_videoreg_w (int offset,int data)
{
//        if (videoctlreg != data) {

           videoctlreg = data;

  	   /*   REMEMBER - both bits 1&2 are used to set the pallette
   	    *   Don't forget to add in bit 2, which doubles as the bank
    	    *   select
            */

	   palreg  = (videoctlreg >> 1) & 0x01;       /* pallette sel is bit 1 */
	   bankreg = ((videoctlreg >> 2) & 0x01); /* banksel is bit 2      */

//        }
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

  The Naughty Boy screen is split into two sections by the hardware

  NonScrolled = 28x4 - (rows 0,1,34,35, as shown below)
  this area is split between the top and bottom of the screen,
  and the address mapping is really funky.

  Scrolled = 28x64, with a 28x32 viewport, as shown below
  Each column in the virtual screen is 64 (40h) characters high.
  Thus, column 27 is stored in VRAm at address 0-3fh,
  column 26 is stored at 40-7f, and so on.
  This illustration shows the horizonal scroll register set to zero,
  so the topmost 32 rows of the virtual screen are shown.

  The following screen-to-memory mapping. This is shown from player's viewpoint,
  which with the CRT rotated 90 degrees CCW. This example shows the horizonal
  scroll register set to zero.


                          COLUMN
                0   1   2    -    25  26  27
              -------------------------------
            0| 76E 76A 762   -   70A 706 702 |
             |                               |  Nonscrolled display
            1| 76F 76B 762   -   70B 707 703 |
             |-------------------------------| -----
            2| 6C0 680 640   -    80  40  00 |
             |                               |
        R   3| 6C1 681 641   -    81  41  01 |
        O    |                               |  28 x 32 viewport
        W   ||      |                 |      |  into 28x64 virtual,
             |                               |  scrollable screen
           32| 6DE 69E 65E        9E  5E  1E |
             |                               |
           33| 6DF 69F 65F   -    9F  5F  1F |
             |-------------------------------| -----
           34| 76C 768 764       708 704 700 |
             |                               |  Nonscrolled display
           35| 76D 769 765       709 705 701 |
              -------------------------------


***************************************************************************/
void naughtyb_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			if (offs < 0x700)
			{
				sx = 27 - offs / 64;
				sy = offs % 64;
			}
			else
			{
				sx = 27 - (offs - 0x700) / 4;
				sy = 64 + (offs - 0x700) % 4;
			}

			drawgfx(tmpbitmap,Machine->gfx[1],
					naughtyb_videoram2[offs] + 256*bankreg,
					(naughtyb_videoram2[offs] >> 5) + 8*palreg,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_NONE,0);

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs] + 256*bankreg,
					(videoram[offs] >> 5) + 8*palreg,
					0,0,
					8*sx,8*sy,
					0,TRANSPARENCY_PEN,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scrolly;


		copybitmap(bitmap,tmpbitmap,0,0,0,-66*8,&topvisiblearea,TRANSPARENCY_NONE,0);
		copybitmap(bitmap,tmpbitmap,0,0,0,-30*8,&bottomvisiblearea,TRANSPARENCY_NONE,0);

		scrolly = -*naughtyb_scrollreg + 16;
		copyscrollbitmap(bitmap,tmpbitmap,0,0,1,&scrolly,&scrollvisiblearea,TRANSPARENCY_NONE,0);
	}
}
