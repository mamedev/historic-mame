/***************************************************************************
 
  vidhrdw.c
 
  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/


static struct osd_bitmap *tmpbitmap2;

int arabian_vh_start(void)
{
int p1,p2,p3,p4,v1,v2,offs;

	if (generic_vh_start() != 0)
		return 1;

	if ((tmpbitmap2 = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		generic_vh_stop();
		return 1;
	}

/*transform graphics data into more usable format*/
/*which is coded like this: 

  byte adr+0x4000  byte adr
  DCBA DCBA        DCBA DCBA
  
D-bits of pixel 4
C-bits of pixel 3
B-bits of pixel 2
A-bits of pixel 1

after conversion :

  byte adr+0x4000  byte adr
  DDDD CCCC        BBBB AAAA
*/

  for (offs=0; offs<0x3fff; offs++)
  {
     v1 = Machine->memory_region[2][offs];
     v2 = Machine->memory_region[2][offs+0x4000];

     p1 = (v1 & 0x01) | ( (v1 & 0x10) >> 3) | ( (v2 & 0x01) << 2 ) | ( (v2 & 0x10) >> 1);
     v1 = v1 >> 1;
     v2 = v2 >> 1;
     p2 = (v1 & 0x01) | ( (v1 & 0x10) >> 3) | ( (v2 & 0x01) << 2 ) | ( (v2 & 0x10) >> 1);
     v1 = v1 >> 1;
     v2 = v2 >> 1;
     p3 = (v1 & 0x01) | ( (v1 & 0x10) >> 3) | ( (v2 & 0x01) << 2 ) | ( (v2 & 0x10) >> 1);
     v1 = v1 >> 1;
     v2 = v2 >> 1;
     p4 = (v1 & 0x01) | ( (v1 & 0x10) >> 3) | ( (v2 & 0x01) << 2 ) | ( (v2 & 0x10) >> 1);

     Machine->memory_region[2][offs] = p1 | (p2<<4);
     Machine->memory_region[2][offs+0x4000] = p3 | (p4<<4);

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




void arabian_blit_byte(int offset, int val, int val2, int plane)
{
  int plane1,plane2,plane3,plane4;
  int p1,p2,p3,p4;
  unsigned char *bm;
  
  int	sy;
  int	sx;

  plane1 = plane & 0x01;
  plane2 = plane & 0x02;
  plane3 = plane & 0x04;
  plane4 = plane & 0x08;

  p4 = val & 0x0f;
  p3 = (val >> 4) & 0x0f;
  p2 = val2 & 0x0f;
  p1 = (val2 >> 4) & 0x0f;

	sx = offset % 256;
	sy = (0x3f - (offset / 256)) * 4;
	

     if (plane1)
     {	
	bm = tmpbitmap->line[sy] + sx;
        if (p1!=8)
		*bm = p1;

	bm = tmpbitmap->line[sy+1] + sx;
	if (p2!=8)
		*bm = p2;

	bm = tmpbitmap->line[sy+2] + sx;
	if (p3!=8)
		*bm = p3;

	bm = tmpbitmap->line[sy+3] + sx;
	if (p4!=8)
		*bm = p4;
     }


     if (plane3)
     {	
	bm = tmpbitmap2->line[sy] + sx;
        if (p1!=8)
		*bm = (16+p1);

	bm = tmpbitmap2->line[sy+1] + sx;
	if (p2!=8)
		*bm = (16+p2);

	bm = tmpbitmap2->line[sy+2] + sx;
	if (p3!=8)
		*bm = (16+p3);

	bm = tmpbitmap2->line[sy+3] + sx;
	if (p4!=8)
		*bm = (16+p4);

     }

}


void arabian_blit_area(int plane, int src, int trg, int xb, int yb)
{
int x,y;
int machine_scrn_x;
int offs;

  machine_scrn_x = (trg-0x8000) & 0xff;

  for (y=0; y<=yb; y++)
  {
    offs=trg-0x8000+y*0x100;
    for (x=0; x<=xb; x++)
    {
      if ( (offs < 0x3fff) && ( machine_scrn_x + x < 0xf8 ) )
        arabian_blit_byte(offs,Machine->memory_region[2][src], Machine->memory_region[2][src+0x4000] , plane);
      src++;
      offs++;
    }
  }

}


void arabian_spriteramw(int offset,int val)
{
  int pl, src,trg, xb,yb;

  spriteram[offset]=val;

  if ( (offset%8) ==6 )
  {
     pl  = spriteram[offset-6];
     src = spriteram[offset-5] + 256 * spriteram[offset-4];
     trg = spriteram[offset-3] + 256 * spriteram[offset-2];
     xb  = spriteram[offset-1];
     yb  = spriteram[offset];
     arabian_blit_area(pl,src,trg,xb,yb);
  }

}



void arabian_videoramw(int offset, int val)
{
  int plane1,plane2,plane3,plane4;
  unsigned char *bm;
  unsigned char *pom;

  int sx;
  int sy;
	
  plane1 = Machine->memory_region[0][0xe000] & 0x01;
  plane2 = Machine->memory_region[0][0xe000] & 0x02;
  plane3 = Machine->memory_region[0][0xe000] & 0x04;
  plane4 = Machine->memory_region[0][0xe000] & 0x08;


  sx = offset % 256;
  sy = (0x3f - (offset / 256)) * 4;
	
	pom = tmpbitmap->line[sy] + sx;
	bm = pom;

     if (plane1)
     {	

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

     bm=pom;
     if (plane2)
     {	
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


    pom = tmpbitmap2->line[sy] + sx;
    bm = pom;

     if (plane3)
     {	

	*bm &= 0xf3;
	if (val & 0x80) *bm |= (16+8);
	if (val & 0x08) *bm |= (16+4);

	bm = tmpbitmap2->line[sy+1] + sx;
	*bm &= 0xf3;
	if (val & 0x40) *bm |= (16+8);
	if (val & 0x04) *bm |= (16+4);

	bm = tmpbitmap2->line[sy+2] + sx;
	*bm &= 0xf3;
	if (val & 0x20) *bm |= (16+8);
	if (val & 0x02) *bm |= (16+4);

	bm = tmpbitmap2->line[sy+3] + sx;
	*bm &= 0xf3;
	if (val & 0x10) *bm |= (16+8);
	if (val & 0x01) *bm |= (16+4);

     }

     bm=pom;
     if (plane4)
     {	

	*bm &= 0xfc;
	if (val & 0x80) *bm |= (16+2);
	if (val & 0x08) *bm |= (16+1);

	bm = tmpbitmap2->line[sy+1] + sx;
	*bm &= 0xfc;
	if (val & 0x40) *bm |= (16+2);
	if (val & 0x04) *bm |= (16+1);

	bm = tmpbitmap2->line[sy+2] + sx;
	*bm &= 0xfc;
	if (val & 0x20) *bm |= (16+2);
	if (val & 0x02) *bm |= (16+1);

	bm = tmpbitmap2->line[sy+3] + sx;
	*bm &= 0xfc;
	if (val & 0x10) *bm |= (16+2);
	if (val & 0x01) *bm |= (16+1);

     }

}


void arabian_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap2,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
 	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
}

