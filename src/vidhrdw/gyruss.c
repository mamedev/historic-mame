/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


#define VIDEO_RAM_SIZE 0x400

unsigned char *gyruss_spritebank,*gyruss_6809_drawplanet,*gyruss_6809_drawship;


typedef struct {
  unsigned char y;
  unsigned char shape;
  unsigned char attr;
  unsigned char x;
} Sprites;



void gyruss_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 256;i++)
	{
		int bits;


		bits = (i >> 5) & 0x07;
		palette[3*i] = (bits >> 1) | (bits << 2) | (bits << 5);
		bits = (i >> 2) & 0x07;
		palette[3*i + 1] = (bits >> 1) | (bits << 2) | (bits << 5);
		bits = (i >> 0) & 0x03;
		palette[3*i + 2] = bits | (bits >> 2) | (bits << 4) | (bits << 6);
	}

	for (i = 0;i < 320;i++)
		colortable[i] = color_prom[i];
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int gyruss_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	return 0;
}



/* convert sprite coordinates from polar to cartesian */
static int SprTrans(Sprites *u)
{
#define YTABLE_START (0x0000)
#define SINTABLE_START (0x0400)
#define COSTABLE_START (0x0600)
	int ro;
	int theta2;
	unsigned char *table;


	ro = Machine->memory_region[3][YTABLE_START + u->y];
	theta2 = 2 * u->x;

	/* cosine table */
	table = &Machine->memory_region[3][COSTABLE_START];

	u->y = (table[theta2+1] * ro) >> 8;
	if (u->y >= 0x80)
	{
		u->y = 0;
		return 0;
	}
	if (table[theta2] != 0)	/* negative */
	{
		if (u->y >= 0x78)	/* avoid wraparound from top to bottom of screen */
		{
			u->y = 0;
			return 0;
		}
		u->y = -u->y;
	}

	/* sine table */
	table = &Machine->memory_region[3][SINTABLE_START];

	u->x = (table[theta2+1] * ro) >> 8;
	if (u->x >= 0x80)
	{
		u->y = 0;
		return 0;
	}
	if (table[theta2] != 0)	/* negative */
		u->x = -u->x;


	/* convert from logical coordinates to screen coordinates */
	if (u->attr & 0x10)
		u->y += 0x78;
	else
		u->y += 0x7C;

	u->x += 0x78;


    return 1;	/* queue this sprite */
}



/* this macro queues 'nq' sprites at address 'u', into queue at 'q' */
#define SPR(n) ((Sprites*)&sr[4*(n)])



	/* Gyruss uses a semaphore system to queue sprites, and when critic
	   region is released, the 6809 processor writes queued sprites onto
	   screen visible area.
	   When a701 = 0 and a702 = 1 gyruss hardware queue sprites.
	   When a701 = 1 and a702 = 0 gyruss hardware draw sprites.

           both Z80 e 6809 are interrupted at the same time by the
           VBLANK interrupt.  If there is some work to do (example
           A7FF is not 0 or FF), 6809 waits for Z80 to store a 1 in
           A701 and draw currently queued sprites
	*/
void gyruss_queuereg_w (int offset,int data)
{
	if (data == 1)
	{
        int n;
		unsigned char *sr;


        /* Gyruss hardware stores alternatively sprites at position
           0xa000 and 0xa200.  0xA700 tells which one is used.
        */

		if (*gyruss_spritebank == 0)
			sr = spriteram;
		else sr = spriteram_2;


		/* #0-#3 - ship */

		/* #4-#23 */
        if (*gyruss_6809_drawplanet)	/* planet is on screen */
		{
			SprTrans(SPR(4));	/* #4 - polar coordinates - ship */

			SPR(5)->y = 0;	/* #5 - unused */

			/* #6-#23 - planet */
        }
		else
		{
			for (n = 4;n < 24;n += 2)	/* 10 double height sprites in polar coordinates - enemies flying */
			{
				SprTrans(SPR(n));

				SPR(n+1)->y = 0;
			}
		}


		/* #24-#59 */
		for (n = 24;n < 60;n++)	/* 36 sprites in polar coordinates - enemies at center of screen */
			SprTrans(SPR(n));


		/* #60-#63 - unused */


		/* #64-#77 */
        if (*gyruss_6809_drawship == 0)
		{
			for (n = 64;n < 78;n++)	/* 14 sprites in polar coordinates - bullets */
				SprTrans(SPR(n));
		}
		/* else 14 sprites - player ship being formed */


		/* #78-#93 - stars */
	    for (n = 78;n < 86;n++)
		{
			if (SprTrans(SPR(n)))
			{
				/* make a mirror copy */
				SPR(n+8)->x = SPR(n)->y - 4;
				SPR(n+8)->y = SPR(n)->x + 4;
			}
			else
				SPR(n+8)->y = 0;
		}
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void gyruss_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	clearbitmap(bitmap);


	/*
	   offs+0 :  Ypos
	   offs+1 :  Sprite number
	   offs+2 :  Attribute in the form HF-VF-BK-DH-p3-p2-p1-p0
				 where  HF is horizontal flip
						VF is vertical flip
						BK is for bank select
						DH is for double height (if set sprite is 16*16, else is 16*8)
						px is palette weight
	   offs+3 :  Xpos
	*/

	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	{
		unsigned char *sr;


		if (*gyruss_spritebank == 0)
			sr = spriteram;
		else sr = spriteram_2;


		for (offs = 96*4-8;offs >= 0;offs -= 8)
		{
			if (sr[2 + offs] & 0x10)	/* double height */
			{
				if (sr[offs + 0] > 2)
				{
					drawgfx(bitmap,Machine->gfx[(sr[offs + 2] & 0x20) ? 5 : 6],
							sr[offs + 1]/2,
							sr[offs + 2] & 0x0f,
							sr[offs + 2] & 0x80,!(sr[offs + 2] & 0x40),
							sr[offs + 3],sr[offs + 0],
							&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
				}
			}
			else	/* single height */
			{
				if (sr[offs + 0] > 2)
				{
					drawgfx(bitmap,Machine->gfx[((sr[offs + 2] & 0x20) ? 1 : 3) + (sr[offs + 1] & 1)],
							sr[offs + 1]/2,
							sr[offs + 2] & 0x0f,
							sr[offs + 2] & 0x80,!(sr[offs + 2] & 0x40),
							sr[offs + 3],sr[offs + 0],
							&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
				}

				if (sr[offs + 4] > 2)
				{
					drawgfx(bitmap,Machine->gfx[((sr[offs + 6] & 0x20) ? 1 : 3) + (sr[offs + 5] & 1)],
							sr[offs + 5]/2,
							sr[offs + 6] & 0x0f,
							sr[offs + 6] & 0x80,!(sr[offs + 6] & 0x40),
							sr[offs + 7],sr[offs + 4],
							&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
				}
			}
		}
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		int sx,sy,charcode;


		sx = 8 * (31 - offs / 32);
		sy = 8 * (offs % 32);

		charcode = videoram[offs] + 8 * (colorram[offs] & 0x20);

		if (charcode != 0x83) /* don't draw spaces */
			drawgfx(bitmap,Machine->gfx[0],
					charcode,
					colorram[offs] & 0x3f,
					colorram[offs] & 0x80,colorram[offs] & 0x40,
					sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}
