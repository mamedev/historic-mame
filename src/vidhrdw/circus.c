/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#define MAPIDXTOCOL(idx) Machine->gfx[0]->colortable[idx * 2 + 1]

#define SCREEN_COL    4
#define OVERLAY1_COL  3
#define OVERLAY2_COL  2
#define OVERLAY3_COL  1

/* These four values are just guesses */
#define OVERLAY1_Y    20
#define OVERLAY2_Y    OVERLAY1_Y + 16
#define OVERLAY3_Y    OVERLAY2_Y + 16
#define OVERLAY_END_Y OVERLAY3_Y + 16


int Clown_X,Clown_Y,Clown_Z=0;

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
                         DAC_data_w(0,0);
                 break;

                case 1 : /* Music */
                                 DAC_data_w(0,0x7f);
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

/*  if(errorlog) fprintf(errorlog,"clown Z = %d\n",data); */
}

void DrawLine(int x1, int y1, int x2, int y2, int dotted)
{
        /* Draws horizontal and Vertical lines only! */
    int col = MAPIDXTOCOL(SCREEN_COL);

    int ex1,ex2,ey1,ey2;
    int count;

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

    if (dotted > 0)
    {
        if (ex1==ex2)
        {
            for (count=ey2;count>=ey1;count-=2)
            {
                            tmpbitmap->line[ex1][count] = col;
                    }
        }
        else
        {
            for (count=ex2;count>=ex1;count-=2)
            {
                            tmpbitmap->line[count][ey1] = col;
                    }
        }
    }
    else
    {
        if (ex1==ex2)
        {
            for (count=ey2;count>=ey1;count--)
            {
                            tmpbitmap->line[ex1][count] = col;
                    }
        }
        else
        {
            for (count=ex2;count>=ex1;count--)
            {
                            tmpbitmap->line[count][ey1] = col;
                    }
        }
    }
}

void RobotBox(int top, int left)
{
        int right,bottom;

    /* Box */

    right  = left + 24;
    bottom = top + 26;

    DrawLine(top,left,top,right,0);                     /* Top */
    DrawLine(bottom,left,bottom,right,0);       /* Bottom */
    DrawLine(top,left,bottom,left,0);           /* Left */
    DrawLine(top,right,bottom,right,0);         /* Right */

    /* Score Grid */

    bottom = top + 10;
    DrawLine(bottom,left+8,bottom,right,0);     /* Horizontal Divide Line */
    DrawLine(top,left+8,bottom,left+8,0);
    DrawLine(top,left+16,bottom,left+16,0);

}


void circus_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
        int offs, x, y, col;

        /* for every character in the Video RAM,        */
        /* check if it has been modified since          */
        /* last time and update it accordingly.         */

        for (offs = videoram_size - 1;offs >= 0;offs--)
        {
                if (dirtybuffer[offs])
                {
                        int sx,sy;

			            col=SCREEN_COL;

                        dirtybuffer[offs] = 0;

                        sx = offs % 32;
                        sy = offs / 32;

            /* Sort out colour overlay */

                        switch (sy)
                        {
                                case 2 :
                                case 3 :
					                    col = OVERLAY1_COL;
                                        break;

                                case 4 :
                                case 5 :
					                    col = OVERLAY2_COL;
                                        break;

                                case 6 :
                                case 7 :
					                    col = OVERLAY3_COL;
                                        break;
            }

                        drawgfx(tmpbitmap,Machine->gfx[0],
                                        videoram[offs],
                                        col,
                                        0,0,
                                        8*sx,8*sy,
                                        &Machine->drv->visible_area,TRANSPARENCY_NONE,0);
                }
        }

    /* The sync generator hardware is used to   */
    /* draw the border and diving boards        */

    DrawLine(18,0,18,255,0);
    DrawLine(249,0,249,255,1);
    DrawLine(18,0,248,0,0);
    DrawLine(18,247,248,247,0);

    DrawLine(137,0,137,17,0);
    DrawLine(137,231,137,248,0);
    DrawLine(193,0,193,17,0);
    DrawLine(193,231,193,248,0);

    /* Redraw portions that fall under the overlay */
    col = MAPIDXTOCOL(OVERLAY1_COL);
    for (y = OVERLAY1_Y; y < OVERLAY_END_Y; y++)
    {
      if (y == OVERLAY2_Y)
        col = MAPIDXTOCOL(OVERLAY2_COL);

      if (y == OVERLAY3_Y)
        col = MAPIDXTOCOL(OVERLAY3_COL);

      tmpbitmap->line[y][0] = col;
      tmpbitmap->line[y][247] = col;
    }

    /* copy the temporary bitmap to the screen */

    copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

    /* Draw the clown in white and afterwards compensate for the overlay */

    drawgfx(bitmap,Machine->gfx[1],
                        Clown_Z,
                        SCREEN_COL,
                        0,0,
                        Clown_Y,Clown_X,
                        &Machine->drv->visible_area,TRANSPARENCY_PEN,0);

    /* ZV980110 */

    /* The variable names Clown_X and Clown_Y are backwards! */
    for (y = Clown_X; y < Clown_X + 16; y++)
    {
      int currentcolidx, currentcol;
      int basecol = MAPIDXTOCOL(SCREEN_COL);

           if (y >= OVERLAY_END_Y) break;
      else if (y >= OVERLAY3_Y) currentcolidx = OVERLAY3_COL;
      else if (y >= OVERLAY2_Y) currentcolidx = OVERLAY2_COL;
      else if (y >= OVERLAY1_Y) currentcolidx = OVERLAY1_COL;
      else continue;

      currentcol = MAPIDXTOCOL(currentcolidx);

      for (x = Clown_Y; x < Clown_Y + 16; x++ )
      {
        /* Remap to overlay color */
        if (bitmap->line[y][x] == basecol)
        {
          bitmap->line[y][x] = currentcol;
        }
      }
    }
}


void robotbowl_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
        int offs;

        /* for every character in the Video RAM,        */
        /* check if it has been modified since          */
        /* last time and update it accordingly.         */

        for (offs = videoram_size - 1;offs >= 0;offs--)
        {
                if (dirtybuffer[offs])
                {
                        int sx,sy;
			            int col=SCREEN_COL;

                        dirtybuffer[offs] = 0;

                        sx = offs % 32;
                        sy = offs / 32;

                        drawgfx(tmpbitmap,Machine->gfx[0],
                                        videoram[offs],
                                        col,
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
        RobotBox(31, offs);
        RobotBox(63, offs);
        RobotBox(95, offs);

        RobotBox(31, offs+152);
        RobotBox(63, offs+152);
        RobotBox(95, offs+152);
    }

    RobotBox(127, 39);                  /* 10th Frame */
    DrawLine(137,39,137,47,0);          /* Extra digit box */

    RobotBox(127, 39+152);
    DrawLine(137,39+152,137,47+152,0);

    /* Bowling Alley */

    DrawLine(17,103,205,103,0);
    DrawLine(17,111,203,111,1);
    DrawLine(17,152,205,152,0);
    DrawLine(17,144,203,144,1);

    /* copy the temporary bitmap to the screen */

    copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

    /* Draw the Ball */

    drawgfx(bitmap,Machine->gfx[1],
                        Clown_Z,
                        SCREEN_COL,
                        0,0,
                        Clown_Y+8,Clown_X+8, /* Y is horizontal position */
                        &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
}

void crash_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;

	/* for every character in the Video RAM,	*/
	/* check if it has been modified since 		*/
	/* last time and update it accordingly. 	*/

	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;
            int col=4;

			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			drawgfx(tmpbitmap,Machine->gfx[0],
					videoram[offs],
					col,
					0,0,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}

    /* The sync generator hardware */

	/* copy the temporary bitmap to the screen */

	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* Draw the Car */

    drawgfx(bitmap,Machine->gfx[1],
			Clown_Z,
			4,
			0,0,
			Clown_Y,Clown_X, /* Y is horizontal position */
			&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
}
