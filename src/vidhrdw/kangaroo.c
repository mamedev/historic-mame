/* CHANGELOG
        97/06/19        some minor cleanup -V-

        97/05/07        wrote a few comments ;-) -V-

        97/04/xx        renamed the arabian.c to kangaroo.c and
                        was a bit disappointed when it did not work
                        straight away: the arabian videohardware is
                        just as kludgy as kangaroo's
*/
/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80.h"

static struct osd_bitmap *tmpbitmap2;
static unsigned char inverse_palette[256]; /* JB 970727 */

static void kangaroo_color_shadew(int val);



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int kangaroo_vh_start(void)
{
	int i;	/* JB 970727 */


	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	if ((tmpbitmap2 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		osd_free_bitmap(tmpbitmap);
		return 1;
	}

	/* JB 970727 */
	for (i = 0;i < Machine->drv->total_colors;i++)
		inverse_palette[ Machine->pens[i] ] = i;

	return 0;
}


/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void kangaroo_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap2);
	osd_free_bitmap(tmpbitmap);
}



/* There has to be a better way to do this ;-)
   and faster -V-
*/
void kangaroo_spriteramw(int offset,int val)
{
  int ofsx, ofsy,x, y, pl, src, xb,yb;


  spriteram[offset]=val;

  if ( (offset) ==5 )
  {
     pl  = spriteram[8];
     src = spriteram[0] + 256 * spriteram[1] - 0xc000;
/*     trg = spriteram[2] + 256 * spriteram[3] - 0x8000;*/
/*     xb  = spriteram[4] ; */
/*     yb  = spriteram[5] * 4; */
/*     pl |= ( pl << 1 ); */

     ofsx = spriteram[2];
     ofsy = ( 0xbf - spriteram[3] )*4;
     yb = ofsy - ( spriteram[5] *4 );
     xb = ofsx + spriteram[4] ;

     for (y=ofsy; y>=yb; y-=4)
       for (x=ofsx; x<=xb; x++, src++)
       {
         if(pl & 0x0c)
           drawgfx(tmpbitmap2,Machine->gfx[0],src,0,0,0,\
             x , y  ,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
         if(pl & 0x03)
           drawgfx(tmpbitmap,Machine->gfx[0],src,0,0,0,\
             x , y ,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
        }

  }
/*  if( (offset)  == 0x0a ) kangaroo_color_shadew(val);
    this will be back when I figure a simple way to implement
    the flashing. -V-
*/
}

static void kangaroo_color_shadew(int val)
{
/* Kangaroo has a color intensity latch at 0xe80a
   values are: (a guess ;)
   bit            plane          colour
---------------------------------------
   0(0x01)        sprite         Blue
   1(0x02)        sprite         Green
   2(0x04)        sprite         Red
   3(0x08)        playfield      Blue
   4(0x10)        playfield      Green
   5(0x20)        playfield      Red
   normal value for the latch seems to be 0x14.
   The value pulsates between 0x34 and 0x14 at the beginning
   of a new game when the topmost kangaroo is falling down
   and the 'trees' are supposed to flash.

   Currently nothing is done here as the framework does not allow
   for changing colours on the fly and drawing the whole screen
   takes too much time at the moment
*/


}

/* this is almost the same as in arabian.c, the planes are arranged
   a bit differently. -V-
*/
void kangaroo_videoramw(int offset, int val)
{
	int plane1,plane2,plane3,plane4;
	unsigned char *bm;
	int sx, sy;


	plane1 = Machine->memory_region[0][0xe808] & 0x01;
	plane2 = Machine->memory_region[0][0xe808] & 0x02;
	plane3 = Machine->memory_region[0][0xe808] & 0x04;
	plane4 = Machine->memory_region[0][0xe808] & 0x08;


	sx = offset % 256;
	sy = (0x3f - (offset / 256)) * 4;

	/* JB 970727 */
	tmpbitmap->line[sy][sx] = inverse_palette[ tmpbitmap->line[sy][sx] ];
	tmpbitmap->line[sy+1][sx] = inverse_palette[ tmpbitmap->line[sy+1][sx] ];
	tmpbitmap->line[sy+2][sx] = inverse_palette[ tmpbitmap->line[sy+2][sx] ];
	tmpbitmap->line[sy+3][sx] = inverse_palette[ tmpbitmap->line[sy+3][sx] ];
	tmpbitmap2->line[sy][sx] = inverse_palette[ tmpbitmap2->line[sy][sx] ];
	tmpbitmap2->line[sy+1][sx] = inverse_palette[ tmpbitmap2->line[sy+1][sx] ];
	tmpbitmap2->line[sy+2][sx] = inverse_palette[ tmpbitmap2->line[sy+2][sx] ];
	tmpbitmap2->line[sy+3][sx] = inverse_palette[ tmpbitmap2->line[sy+3][sx] ];


	if (plane2)
	{
		bm = tmpbitmap->line[sy] + sx;
		*bm &= 0xfc;
		if (val & 0x80) *bm |= 2;
		if (val & 0x08) *bm |= 1;

		bm = tmpbitmap->line[sy+1] + sx;
		*bm &= 0xfc;
		if (val & 0x40) *bm |= 2;
		if (val & 0x04) *bm |= 1;

		bm = tmpbitmap->line[sy+2] + sx;
		*bm &= 0xfc;
		if (val & 0x20) *bm |= 2;
		if (val & 0x02) *bm |= 1;

		bm = tmpbitmap->line[sy+3] + sx;
		*bm &= 0xfc;
		if (val & 0x10) *bm |= 2;
		if (val & 0x01) *bm |= 1;
	}

	if (plane1)
	{
		bm = tmpbitmap->line[sy] + sx;
		*bm &= 0xf3;
		if (val & 0x80) *bm |= 8;
		if (val & 0x08) *bm |= 4;

		bm = tmpbitmap->line[sy+1] + sx;
		*bm &= 0xf3;
		if (val & 0x40) *bm |= 8;
		if (val & 0x04) *bm |= 4;

		bm = tmpbitmap->line[sy+2] + sx;
		*bm &= 0xf3;
		if (val & 0x20) *bm |= 8;
		if (val & 0x02) *bm |= 4;

		bm = tmpbitmap->line[sy+3] + sx;
		*bm &= 0xf3;
		if (val & 0x10) *bm |= 8;
		if (val & 0x01) *bm |= 4;
	}

	if (plane4)
	{
		bm = tmpbitmap2->line[sy] + sx;
		*bm &= 0xfc;
		if (val & 0x80) *bm |= 2;
		if (val & 0x08) *bm |= 1;

		bm = tmpbitmap2->line[sy+1] + sx;
		*bm &= 0xfc;
		if (val & 0x40) *bm |= 2;
		if (val & 0x04) *bm |= 1;

		bm = tmpbitmap2->line[sy+2] + sx;
		*bm &= 0xfc;
		if (val & 0x20) *bm |= 2;
		if (val & 0x02) *bm |= 1;

		bm = tmpbitmap2->line[sy+3] + sx;
		*bm &= 0xfc;
		if (val & 0x10) *bm |= 2;
		if (val & 0x01) *bm |= 1;
	}

	if (plane3)
	{
		bm = tmpbitmap2->line[sy] + sx;
		*bm &= 0xf3;
		if (val & 0x80) *bm |= 8;
		if (val & 0x08) *bm |= 4;

		bm = tmpbitmap2->line[sy+1] + sx;
		*bm &= 0xf3;
		if (val & 0x40) *bm |= 8;
		if (val & 0x04) *bm |= 4;

		bm = tmpbitmap2->line[sy+2] + sx;
		*bm &= 0xf3;
		if (val & 0x20) *bm |= 8;
		if (val & 0x02) *bm |= 4;

		bm = tmpbitmap2->line[sy+3] + sx;
		*bm &= 0xf3;
		if (val & 0x10) *bm |= 8;
		if (val & 0x01) *bm |= 4;
	}

	/* JB 970727 */
	tmpbitmap->line[sy][sx] = Machine->pens[ tmpbitmap->line[sy][sx] ];
	tmpbitmap->line[sy+1][sx] = Machine->pens[ tmpbitmap->line[sy+1][sx] ];
	tmpbitmap->line[sy+2][sx] = Machine->pens[ tmpbitmap->line[sy+2][sx] ];
	tmpbitmap->line[sy+3][sx] = Machine->pens[ tmpbitmap->line[sy+3][sx] ];
	tmpbitmap2->line[sy][sx] = Machine->pens[ tmpbitmap2->line[sy][sx] ];
	tmpbitmap2->line[sy+1][sx] = Machine->pens[ tmpbitmap2->line[sy+1][sx] ];
	tmpbitmap2->line[sy+2][sx] = Machine->pens[ tmpbitmap2->line[sy+2][sx] ];
	tmpbitmap2->line[sy+3][sx] = Machine->pens[ tmpbitmap2->line[sy+3][sx] ];

	osd_mark_dirty (sx,sy,sx,sy+3,0);	/* ASG 971015 */
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void kangaroo_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	/* copy the character mapped graphics */
        copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
        copybitmap(bitmap,tmpbitmap2,0,0,0,0,&Machine->drv->visible_area,\
        	TRANSPARENCY_COLOR,0);/* JB 970727 */
}
