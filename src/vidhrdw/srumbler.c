/***************************************************************************

   SPEED RUMBLER

   vidhrdw.c

   Functions to emulate the video hardware of the machine.

   Todo:
   Priority tiles (should be half transparent).

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *srumbler_paletteram;
unsigned char *srumbler_backgroundram;
unsigned char *srumbler_scrolly;
unsigned char *srumbler_scrollx;

int srumbler_paletteram_size;
int srumbler_backgroundram_size;

static unsigned char *bkgnd_dirty;
static unsigned char *dirtybufferpal;
static struct osd_bitmap *tmpbitmap2;

static unsigned char scrollpalette_dirty[0x40];


/* Unknown video control register */
static int srumbler_video_control;

/*
Currently not used. The end of level score-board should either be
transparent or the scroll tiles need to be turned off.
*/
static int chon=1,objon=1,scrollon=1;



/***************************************************************************

   Start the video hardware emulation.

***************************************************************************/

int srumbler_vh_start(void)
{
if (generic_vh_start() != 0)
return 1;

	 if ((bkgnd_dirty = malloc(srumbler_backgroundram_size)) == 0)
{
generic_vh_stop();
return 1;
}
	 memset(bkgnd_dirty,1,srumbler_backgroundram_size);


	 /* Palette RAM dirty buffer */
	 if ((dirtybufferpal = malloc(srumbler_paletteram_size)) == 0)
{
generic_vh_stop();
return 1;
}
	 memset(dirtybufferpal,1,srumbler_paletteram_size);


	 if ((tmpbitmap2 = osd_new_bitmap(0x40*16, 0x40*16, Machine->scrbitmap->depth )) == 0)
{
		 free(bkgnd_dirty);
generic_vh_stop();
return 1;
}
return 0;

}

/***************************************************************************

   Stop the video hardware emulation.

***************************************************************************/
void srumbler_vh_stop(void)
{
osd_free_bitmap(tmpbitmap2);
	 free(bkgnd_dirty);
	 free(dirtybufferpal);
generic_vh_stop();
}

void srumbler_4009_w(int offset, int data)
{
	 /* Unknown video control register */
	 srumbler_video_control=data;
}

void srumbler_background_w(int offset,int data)
{
	 if (srumbler_backgroundram[offset] != data)
{
		 bkgnd_dirty[offset] = 1;
		 srumbler_backgroundram[offset] = data;
}
}

void srumbler_paletteram_w(int offset,int data)
{
	 if (srumbler_paletteram[offset] != data)
{
		 dirtybufferpal[offset]=1;
		 srumbler_paletteram[offset] = data;
		 /*
		    Mark palettes as dirty.
		    We are only interested in whether tiles have changed.
		    This is done by colour code.
		 */
		 scrollpalette_dirty[(offset>>5)&0x3f]=1;
}
}

/***************************************************************************

   Draw the game screen in the given osd_bitmap.
   Do NOT call osd_update_display() from this function, it will be called by
   the main emulation engine.

***************************************************************************/

void srumbler_vh_screenrefresh(struct osd_bitmap *bitmap)
{
int scrollx, scrolly;
int offs;
int j, i;
int sx, sy, x, y;


/* rebuild the colour lookup table from RAM palette */
for (j=0; j<3; j++)
{
	/* CHARS  TILES   SPRITES */
static int start[3]={0x0280, 0x0000, 0x0100};
static int count[3]={0x0040, 0x0080, 0x0080};
int base=start[j];
int bluebase=base+1;
int max=count[j];

for (i=0; i<max; i++)
{
if (dirtybufferpal[base] || dirtybufferpal[bluebase])
{
int red, green, blue, redgreen;

redgreen=srumbler_paletteram[base];
red=redgreen >>4;
green=redgreen & 0x0f ;
blue=srumbler_paletteram[bluebase]>>4;

red = (red << 4) + red;
green = (green << 4) + green;
blue = (blue << 4) + blue;

dirtybufferpal[base] = dirtybufferpal[bluebase] = 0;

setgfxcolorentry (Machine->gfx[j], i, red, green, blue);
}
base+=2;
bluebase+=2;
}
}

	 scrollx = srumbler_scrollx[0]+256*srumbler_scrollx[1];
	 scrollx += 0x50;
	 scrollx &= 0x03ff;
	 scrolly = (srumbler_scrolly[0]+256*srumbler_scrolly[1]);

	 if (scrollon)
	 {
	    /* Dirty all touched tile colours */
	    for (j=srumbler_backgroundram_size-2; j>=0; j-=2)
	    {
		  if (scrollpalette_dirty[srumbler_backgroundram[j] >> 5])
		       bkgnd_dirty[j]=1;
	    }

	    offs=0;
	    for (sx=0; sx<0x40; sx++)
	    {
		 for (sy=0; sy<0x40; sy++)
		 {
			 /* TILES
			    =====
			    Attribute
			    0x80 Colour
			    0x40 Colour
			    0x20 Colour
			    0x10 Tile priority
			    0x08 Y flip
			    0x04 Code
			    0x02 Code
			    0x01 Code
			 */

			 if (bkgnd_dirty[offs]||bkgnd_dirty[offs+1])
			 {
				 int attr=srumbler_backgroundram[offs];
				 int code=attr&0x07;
				 code <<= 8;
				 code+=srumbler_backgroundram[offs+1];

				 bkgnd_dirty[offs]=bkgnd_dirty[offs+1]=0;

				 drawgfx(tmpbitmap2,Machine->gfx[1],
						 code,
						 (attr>>5),
						 0,
						 attr&0x08,
						 sx*16, sy*16,
						 0, TRANSPARENCY_NONE,0);
			}
			offs+=2;
		  }
	     }

	     memset(scrollpalette_dirty, 0, sizeof(scrollpalette_dirty));

	     /* copy the background graphics */
	     {
		int scrlx, scrly;
		scrlx =-scrollx;
		scrly=-scrolly;
		copyscrollbitmap(bitmap,tmpbitmap2,1,&scrlx,1,&scrly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	     }
	 }
	 else  fillbitmap(bitmap,Machine->pens[0],&Machine->drv->visible_area);


	 if (objon)
	 {
		 /* Draw the sprites. */
		 for (offs = spriteram_size-4; offs>=0;offs -= 4)
		 {
			 /* SPRITES
			    =====
			    Attribute
			    0x80 Code MSB
			    0x40 Code MSB
			    0x20 Code MSB
			    0x10 Colour
			    0x08 Colour
			    0x04 Colour
			    0x02 y Flip
			    0x01 X MSB
			 */


			 int code,colour,sx,sy;
			 int attr=spriteram[offs+1];
			 code = spriteram[offs];
			 code += ( (attr&0xe0) << 3 );
			 colour = (attr & 0x1c)>>2;
			 sy = spriteram[offs + 2];
			 sx = spriteram[offs + 3] + 0x100 * ( attr & 0x01) - 0x50;

			 drawgfx(bitmap,Machine->gfx[2],
					 code,
					 colour,
					 0,
					 attr&0x02,
					 sx, sy,
					 &Machine->drv->visible_area,TRANSPARENCY_PEN,15);
		  }

		 /* now draw high priority sprite parts */
		 if( 0 )for (offs = spriteram_size-4; offs>=0;offs -= 4)
		 {
			 /* SPRITES
			    =====
			    Attribute
			    0x80 Code MSB
			    0x40 Code MSB
			    0x20 Code MSB
			    0x10 Colour
			    0x08 Colour
			    0x04 Colour
			    0x02 y Flip
			    0x01 X MSB
			 */


			 int code,colour,sx,sy;
			 int attr=spriteram[offs+1];
			 code = spriteram[offs];
			 code += ( (attr&0xe0) << 3 );
			 colour = (attr & 0x1c)>>2;
			 sy = spriteram[offs + 2];
			 sx = spriteram[offs + 3] + 0x100 * ( attr & 0x01) - 0x50;

			 drawgfx(bitmap,Machine->gfx[2],
					 code,
					 colour,
					 0,
					 attr&0x02,
					 sx, sy,
					 &Machine->drv->visible_area,TRANSPARENCY_PENS,(1<<15)|0xFF);
		  }
	 }

/* redraw the background tiles which have priority over sprites */
	 if (scrollon)
	 {
		 x=-(scrollx & 0x0f);

		 for (sx=0; sx<0x18; sx++)
		 {
		       int offs=(scrollx >>4)*0x80;
		       offs+=(scrolly>>4)*2;
		       offs+=sx*0x80;
		       offs&=0x1fff;
		       y=-(scrolly & 0x0f);
		       for (sy=0; sy<0x11; sy++)
		       {
			      int attr=srumbler_backgroundram[offs];
			      if (attr & 0x10)
			      {
				     int code=attr&0x07;
				     code <<= 8;
				     code+=srumbler_backgroundram[offs+1];
				     drawgfx(bitmap,Machine->gfx[1],
						  code,
						  (attr>>5),
						  0,
						  attr&0x08,
						  x, y,
						  &Machine->drv->visible_area,

						  TRANSPARENCY_PENS,0xFF );
			     }
			     y+=16;
			     offs+=2;
		       }
		       x+=16;
		 }
	 }

	 if (chon)
	 {
		 /* Draw the frontmost playfield. They are characters, but draw them as sprites */
		 offs=0x0280;
		 for (i=0; i<0x40; i++)
		 {
			 for (j=0; j<0x20; j++)
			 {
				 int transparency;
				 int colour=videoram[offs];
				 int code=colour&0x03;
				 code <<= 8;
				 code|=videoram[offs+1];

				 if (colour & 0x40)
				 {
					 transparency=TRANSPARENCY_NONE;
				 }
				 else
				 {
					 transparency=TRANSPARENCY_PEN;
				 }

				 colour >>=2;
				 colour &= 0x0f;
				 drawgfx(bitmap,Machine->gfx[0],
					    code,
					   colour,
					   0,0,8*i,8*j,
					   &Machine->drv->visible_area,transparency,3);
				 offs+=2;
			 }
		 }
}
}



