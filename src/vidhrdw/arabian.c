/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80.h"

static struct osd_bitmap *tmpbitmap2;

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int arabian_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((tmpbitmap2 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		generic_vh_stop();
		return 1;
	}

	return 0;
}


/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void arabian_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap2);
	generic_vh_stop();
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void arabian_blit_byte(int offset, int val, int val2, int plane)
{
  int plane1,plane2,plane3,plane4;
  unsigned char *bm;
  unsigned char *pom;
  
  int	sy;
  int	sx;

  plane1 = plane & 0x01;
  plane2 = plane & 0x02;
  plane3 = plane & 0x04;
  plane4 = plane & 0x08;

	sx = offset % 256;
	sy = (0x3f - (offset / 256)) * 4;
	
/*     pom = tmpbitmap->line[0];                                    */
/*     pom += ( (offset % 256 ) + ( (0x3f-(offset /256)) * 1024 ) );*/
	pom = tmpbitmap->line[sy] + sx;

     bm = pom;

     if (plane1)
     {	
	*bm &= 0x0c;
	if (val & 0x80) *bm |= 1;
	if (val & 0x08) *bm |= 2;
	bm = tmpbitmap->line[sy+1] + sx;

	*bm &= 0x0c;
	if (val & 0x40) *bm |= 1;
	if (val & 0x04) *bm |= 2;
	bm = tmpbitmap->line[sy+2] + sx;

	*bm &= 0x0c;
	if (val & 0x20) *bm |= 1;
	if (val & 0x02) *bm |= 2;
	bm = tmpbitmap->line[sy+3] + sx;

	*bm &= 0x0c;
	if (val & 0x10) *bm |= 1;
	if (val & 0x01) *bm |= 2;
/* TODO: remap the pixel color thru the color table */
     }

     bm=pom;
     if (plane2)
     {	
	*bm &= 0x03;
	if (val2 & 0x80) *bm |= 4;
	if (val2 & 0x08) *bm |= 8;
	bm = tmpbitmap->line[sy+1] + sx;

	*bm &= 0x03;
	if (val2 & 0x40) *bm |= 4;
	if (val2 & 0x04) *bm |= 8;
	bm = tmpbitmap->line[sy+2] + sx;

	*bm &= 0x03;
	if (val2 & 0x20) *bm |= 4;
	if (val2 & 0x02) *bm |= 8;
	bm = tmpbitmap->line[sy+3] + sx;

	*bm &= 0x03;
	if (val2 & 0x10) *bm |= 4;
	if (val2 & 0x01) *bm |= 8;
/* TODO: remap the pixel color thru the color table */

     }


/*     pom = tmpbitmap2->line[0];                                   */
/*     pom += ( (offset % 256 ) + ( (0x3f-(offset /256)) * 1024 ) );*/
	pom = tmpbitmap2->line[sy] + sx;

     bm = pom;

     if (plane3)
     {	
	*bm &= 0x0c;
	if (val & 0x80) *bm |= 1;
	if (val & 0x08) *bm |= 2;
	bm = tmpbitmap2->line[sy+1] + sx;

	*bm &= 0x0c;
	if (val & 0x40) *bm |= 1;
	if (val & 0x04) *bm |= 2;
	bm = tmpbitmap2->line[sy+2] + sx;

	*bm &= 0x0c;
	if (val & 0x20) *bm |= 1;
	if (val & 0x02) *bm |= 2;
	bm = tmpbitmap2->line[sy+3] + sx;

	*bm &= 0x0c;
	if (val & 0x10) *bm |= 1;
	if (val & 0x01) *bm |= 2;
/* TODO: remap the pixel color thru the color table */

     }

     bm=pom;
     if (plane4)
     {	
	*bm &= 0x03;
	if (val2 & 0x80) *bm |= 4;
	if (val2 & 0x08) *bm |= 8;
	bm = tmpbitmap2->line[sy+1] + sx;

	*bm &= 0x03;
	if (val2 & 0x40) *bm |= 4;
	if (val2 & 0x04) *bm |= 8;
	bm = tmpbitmap2->line[sy+2] + sx;

	*bm &= 0x03;
	if (val2 & 0x20) *bm |= 4;
	if (val2 & 0x02) *bm |= 8;
	bm = tmpbitmap2->line[sy+3] + sx;

	*bm &= 0x03;
	if (val2 & 0x10) *bm |= 4;
	if (val2 & 0x01) *bm |= 8;
/* TODO: remap the pixel color thru the color table */

     }
}


void arabian_blit_area(int plane, int src, int trg, int xb, int yb)
{
int x,y;
int offs;

plane|=(plane<<1);

  for (y=0; y<=yb; y++)
  {
    offs=trg-0x8000+y*0x100;
    for (x=0; x<=xb; x++)
    {
      if (offs < 0x3fff)
        arabian_blit_byte(offs,Machine->memory_region[2][src], Machine->memory_region[2][src+0x4000], plane);
      src++;
      offs++;
    }
  }

}


void arabian_spriteramw(int offset,int val)
{
  Z80_Regs reg;
  int pl, src,trg, xb,yb;

  Z80_GetRegs(&reg);

  spriteram[offset]=val;

  if ( (offset%8) ==6 )
  {
     pl  = spriteram[offset-6];
     src = spriteram[offset-5] + 256 * spriteram[offset-4];
     trg = spriteram[offset-3] + 256 * spriteram[offset-2];
     xb  = spriteram[offset-1];
     yb  = spriteram[offset];
     arabian_blit_area(pl,src,trg,xb,yb);
/*     if (errorlog)
       fprintf(errorlog,"SPRITE %02x : Plane: %02x, S:%04x, T:%04x, XB:%02x, YB:%02x\n", offset/8,pl,src,trg,xb,yb);
*/
  }

}



void arabian_videoramw(int offset, int val)
{
  int plane1,plane2,plane3,plane4;
  unsigned char *bm;
  unsigned char *pom;

	int sx;
	int	sy;
	
  plane1 = Machine->memory_region[0][0xe000] & 0x01;
  plane2 = Machine->memory_region[0][0xe000] & 0x02;
  plane3 = Machine->memory_region[0][0xe000] & 0x04;
  plane4 = Machine->memory_region[0][0xe000] & 0x08;


	sx = offset % 256;
	sy = (0x3f - (offset / 256)) * 4;
	
/*     pom = tmpbitmap->line[0];                                     */
/*     pom += ( (offset % 256 ) + ( (0x3f-(offset /256)) * 1024 ) ); */
	pom = tmpbitmap->line[sy] + sx;

     bm = pom;

     if (plane1)
     {	
	*bm &= 0x0c;
	if (val & 0x80) *bm |= 1;
	if (val & 0x08) *bm |= 2;
	bm = tmpbitmap->line[sy+1] + sx;

	*bm &= 0x0c;
	if (val & 0x40) *bm |= 1;
	if (val & 0x04) *bm |= 2;
	bm = tmpbitmap->line[sy+2] + sx;

	*bm &= 0x0c;
	if (val & 0x20) *bm |= 1;
	if (val & 0x02) *bm |= 2;
	bm = tmpbitmap->line[sy+3] + sx;

	*bm &= 0x0c;
	if (val & 0x10) *bm |= 1;
	if (val & 0x01) *bm |= 2;
/* TODO: remap the pixel color thru the color table */

     }

     bm=pom;
     if (plane2)
     {	
	*bm &= 0x03;
	if (val & 0x80) *bm |= 4;
	if (val & 0x08) *bm |= 8;
	bm = tmpbitmap->line[sy+1] + sx;

	*bm &= 0x03;
	if (val & 0x40) *bm |= 4;
	if (val & 0x04) *bm |= 8;
	bm = tmpbitmap->line[sy+2] + sx;

	*bm &= 0x03;
	if (val & 0x20) *bm |= 4;
	if (val & 0x02) *bm |= 8;
	bm = tmpbitmap->line[sy+3] + sx;

	*bm &= 0x03;
	if (val & 0x10) *bm |= 4;
	if (val & 0x01) *bm |= 8;
/* TODO: remap the pixel color thru the color table */

     }


/*     pom = tmpbitmap2->line[0];                                     */
/*     pom += ( (offset % 256 ) + ( (0x3f-(offset /256)) * 1024 ) );  */
 	pom = tmpbitmap2->line[sy] + sx;
    bm = pom;

     if (plane3)
     {	
	*bm &= 0x0c;
	if (val & 0x80) *bm |= 1;
	if (val & 0x08) *bm |= 2;
	bm = tmpbitmap2->line[sy+1] + sx;

	*bm &= 0x0c;
	if (val & 0x40) *bm |= 1;
	if (val & 0x04) *bm |= 2;
	bm = tmpbitmap2->line[sy+2] + sx;

	*bm &= 0x0c;
	if (val & 0x20) *bm |= 1;
	if (val & 0x02) *bm |= 2;
	bm = tmpbitmap2->line[sy+3] + sx;

	*bm &= 0x0c;
	if (val & 0x10) *bm |= 1;
	if (val & 0x01) *bm |= 2;
/* TODO: remap the pixel color thru the color table */

     }

     bm=pom;
     if (plane4)
     {	
	*bm &= 0x03;
	if (val & 0x80) *bm |= 4;
	if (val & 0x08) *bm |= 8;
	bm = tmpbitmap2->line[sy+1] + sx;

	*bm &= 0x03;
	if (val & 0x40) *bm |= 4;
	if (val & 0x04) *bm |= 8;
	bm = tmpbitmap2->line[sy+2] + sx;

	*bm &= 0x03;
	if (val & 0x20) *bm |= 4;
	if (val & 0x02) *bm |= 8;
	bm = tmpbitmap2->line[sy+3] + sx;

	*bm &= 0x03;
	if (val & 0x10) *bm |= 4;
	if (val & 0x01) *bm |= 8;
/* TODO: remap the pixel color thru the color table */

     }

}


void arabian_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap2,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
}
