/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

  CHANGES:
  MAB 05 MAR 99 - changed overlay support to use artwork functions

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "artwork.h"

static int Clown_X=0,Clown_Y=0,Clown_Z=0;

static struct artwork *overlay;

/* The first entry defines the color with which the bitmap is filled initially */
/* The array is terminated with an entry with negative coordinates. */
/* At least two entries are needed. */
static const struct artwork_element circus_ol[]={
	{{	0, 256,   0, 256}, 0xFF, 0xFF, 0xFF,   0xFF},	/* white */
	{{  0, 256,  20,  36}, 0x20, 0x20, 0xFF,   0xFF},	/* blue */
	{{  0, 256,  36,  48}, 0x20, 0xFF, 0x20,   0xFF},	/* green */
	{{  0, 256,  48,  64}, 0xFF, 0xFF, 0x20,   0xFF},	/* yellow */
	{{-1,-1,-1,-1},0,0,0,0}
};


/***************************************************************************
***************************************************************************/

int circus_vh_start(void)
{
	int start_pen = 2;

	if (generic_vh_start()!=0)
		return 1;

	if ((overlay = artwork_create(circus_ol, start_pen, Machine->drv->total_colors-start_pen))==NULL)
		return 1;

	return 0;
}

/***************************************************************************
***************************************************************************/

void circus_vh_stop(void)
{
	if (overlay)
		artwork_free(overlay);

	generic_vh_stop();
}

/***************************************************************************
***************************************************************************/

void circus_clown_x_w(int offset, int data)
{
	Clown_X = 240-data;
}

void circus_clown_y_w(int offset, int data)
{
	Clown_Y = 240-data;
}

/* This register controls the clown image currently displayed */
/* and also is used to enable the amplifier and trigger the   */
/* discrete circuitry that produces sound effects and music   */

void circus_clown_z_w(int offset, int data)
{
	Clown_Z = (data & 0x0f);

	/* Bits 4-6 enable/disable trigger different events */
	/* descriptions are based on Circus schematics      */

	switch ((data & 0x70) >> 4)
	{
		case 0 : /* All Off */
			DAC_data_w (0,0);
			break;

		case 1 : /* Music */
			DAC_data_w (0,0x7f);
			break;

		case 2 : /* Pop */
			break;

		case 3 : /* Normal Video */
			break;

		case 4 : /* Miss */
			break;

		case 5 : /* Invert Video */
			break;

		case 6 : /* Bounce */
			break;

		case 7 : /* Don't Know */
			break;
	};

	/* Bit 7 enables amplifier (1 = on) */

//	if(errorlog) fprintf(errorlog,"clown Z = %02x\n",data);
}

static void DrawLine(struct osd_bitmap *bitmap, int x1, int y1, int x2, int y2, int dotted)
{
	/* Draws horizontal and Vertical lines only! */
    int col = Machine->pens[1];

    int ex1,ex2,ey1,ey2;
    int count, skip;

    /* Allow flips & rotates */

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		ex1 = y1;
		ex2 = y2;
		ey1 = x1;
		ey2 = x2;
	}
    else
    {
		ex1 = x1;
		ex2 = x2;
		ey1 = y1;
		ey2 = y2;
	}

	if (Machine->orientation & ORIENTATION_FLIP_X)
    {
		count = 255 - ey1;
		ey1 = 255 - ey2;
		ey2 = count;
	}

	if (Machine->orientation & ORIENTATION_FLIP_Y)
	{
		count = 255 - ex1;
		ex1 = 255 - ex2;
		ex2 = count;
	}

	/* Draw the Line */

	osd_mark_dirty (ex1,ey1,ex1,ey2,0);

	if (dotted > 0)
		skip = 2;
	else
		skip = 1;

	if (ex1==ex2)
	{
		for (count=ey2;count>=ey1;count -= skip)
		{
			bitmap->line[ex1][count] = col;
		}
	}
	else
	{
		for (count=ex2;count>=ex1;count -= skip)
		{
			bitmap->line[count][ey1] = col;
		}
	}
}

static void RobotBox (struct osd_bitmap *bitmap, int top, int left)
{
	int right,bottom;

	/* Box */

	right  = left + 24;
	bottom = top + 26;

	DrawLine(bitmap,top,left,top,right,0);				/* Top */
	DrawLine(bitmap,bottom,left,bottom,right,0);		/* Bottom */
	DrawLine(bitmap,top,left,bottom,left,0);			/* Left */
	DrawLine(bitmap,top,right,bottom,right,0);			/* Right */

	/* Score Grid */

	bottom = top + 10;
	DrawLine(bitmap,bottom,left+8,bottom,right,0);     /* Horizontal Divide Line */
	DrawLine(bitmap,top,left+8,bottom,left+8,0);
	DrawLine(bitmap,top,left+16,bottom,left+16,0);
}


void circus_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int sx,sy;

	if (full_refresh)
	{
		memset (dirtybuffer, 1, videoram_size);
	}

	/* for every character in the Video RAM,        */
	/* check if it has been modified since          */
	/* last time and update it accordingly.         */

	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			dirtybuffer[offs] = 0;

			sy = offs / 32;
			sx = offs % 32;

			drawgfx(bitmap,Machine->gfx[0],
					videoram[offs],
					1,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}

    /* The sync generator hardware is used to   */
    /* draw the border and diving boards        */

    DrawLine (bitmap,18,0,18,255,0);
    DrawLine (bitmap,249,0,249,255,1);
    DrawLine (bitmap,18,0,248,0,0);
    DrawLine (bitmap,18,247,248,247,0);

    DrawLine (bitmap,137,0,137,17,0);
    DrawLine (bitmap,137,231,137,248,0);
    DrawLine (bitmap,193,0,193,17,0);
    DrawLine (bitmap,193,231,193,248,0);

    /* Draw the clown in white and afterwards compensate for the overlay */
	drawgfx(bitmap,Machine->gfx[1],
			Clown_Z,
			1,
			0,0,
			Clown_Y,Clown_X,
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

	/* mark tiles underneath as dirty */
	sx = Clown_Y >> 3;
	sy = Clown_X >> 3;

	{
		int max_x = 2;
		int max_y = 2;
		int x2, y2;

		if (Clown_Y & 0x0f) max_x ++;
		if (Clown_X & 0x0f) max_y ++;

		for (y2 = sy; y2 < sy + max_y; y2 ++)
		{
			for (x2 = sx; x2 < sx + max_x; x2 ++)
			{
				if ((x2 < 32) && (y2 < 32) && (x2 >= 0) && (y2 >= 0))
					dirtybuffer[x2 + 32*y2] = 1;
			}
		}
	}

	overlay_draw(bitmap,overlay);
}


void robotbowl_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int sx,sy;

	if (full_refresh)
	{
		memset (dirtybuffer, 1, videoram_size);
	}

	/* for every character in the Video RAM,  */
	/* check if it has been modified since    */
	/* last time and update it accordingly.   */

	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			drawgfx(bitmap,Machine->gfx[0],
					videoram[offs],
					1,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}

    /* The sync generator hardware is used to   */
    /* draw the bowling alley & scorecards      */

    /* Scoreboards */

    for(offs=15;offs<=63;offs+=24)
    {
        RobotBox(bitmap, 31, offs);
        RobotBox(bitmap, 63, offs);
        RobotBox(bitmap, 95, offs);

        RobotBox(bitmap, 31, offs+152);
        RobotBox(bitmap, 63, offs+152);
        RobotBox(bitmap, 95, offs+152);
    }

    RobotBox(bitmap, 127, 39);                  /* 10th Frame */
    DrawLine(bitmap, 137,39,137,47,0);          /* Extra digit box */

    RobotBox(bitmap, 127, 39+152);
    DrawLine(bitmap, 137,39+152,137,47+152,0);

    /* Bowling Alley */

    DrawLine(bitmap, 17,103,205,103,0);
    DrawLine(bitmap, 17,111,203,111,1);
    DrawLine(bitmap, 17,152,205,152,0);
    DrawLine(bitmap, 17,144,203,144,1);

	/* Draw the Ball */

	drawgfx(bitmap,Machine->gfx[1],
			Clown_Z,
			1,
			0,0,
			Clown_Y+8,Clown_X+8, /* Y is horizontal position */
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

	/* mark tiles underneath as dirty */
	sx = Clown_Y >> 3;
	sy = Clown_X >> 3;

	{
		int max_x = 2;
		int max_y = 2;
		int x2, y2;

		if (Clown_Y & 0x0f) max_x ++;
		if (Clown_X & 0x0f) max_y ++;

		for (y2 = sy; y2 < sy + max_y; y2 ++)
		{
			for (x2 = sx; x2 < sx + max_x; x2 ++)
			{
				if ((x2 < 32) && (y2 < 32) && (x2 >= 0) && (y2 >= 0))
					dirtybuffer[x2 + 32*y2] = 1;
			}
		}
	}

}

void crash_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	int sx,sy;

	if (full_refresh)
	{
		memset (dirtybuffer, 1, videoram_size);
	}

	/* for every character in the Video RAM,	*/
	/* check if it has been modified since 		*/
	/* last time and update it accordingly. 	*/

	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			drawgfx(bitmap,Machine->gfx[0],
					videoram[offs],
					1,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	/* Draw the Car */
    drawgfx(bitmap,Machine->gfx[1],
			Clown_Z,
			1,
			0,0,
			Clown_Y,Clown_X, /* Y is horizontal position */
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0);

	/* mark tiles underneath as dirty */
	sx = Clown_Y >> 3;
	sy = Clown_X >> 3;

	{
		int max_x = 2;
		int max_y = 2;
		int x2, y2;

		if (Clown_Y & 0x0f) max_x ++;
		if (Clown_X & 0x0f) max_y ++;

		for (y2 = sy; y2 < sy + max_y; y2 ++)
		{
			for (x2 = sx; x2 < sx + max_x; x2 ++)
			{
				if ((x2 < 32) && (y2 < 32) && (x2 >= 0) && (y2 >= 0))
					dirtybuffer[x2 + 32*y2] = 1;
			}
		}
	}
}
