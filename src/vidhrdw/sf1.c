#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *sf1_video_ram;
unsigned char *sf1_object_ram;

int sf1_deltaxb = 0;
int sf1_deltaxm = 0;
int sf1_active  = 0;

INLINE int sf1_invert(int nb)
{
  static int delta[4] = {0x00, 0x18, 0x18, 0x00};
  return nb^delta[(nb>>3)&3];
}

void sf1_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
  unsigned char *pt;
  short x, y;

  palette_recalc();

  if(sf1_active & 0x20) {
    pt = Machine->memory_region[2] + (sf1_deltaxb & 0xfff0)*2 + 0x80;
    for(x=-(sf1_deltaxb & 0xf);x<384;x+=16)
      for(y=0;y<256;y+=16) {
	int c;
	c=(pt[65537]<<8) | pt[1];
	drawgfx(bitmap,
		Machine->gfx[1],
		c,
		pt[0],
		pt[65536]&1, pt[65536]&2,
		x, y,
		&Machine->drv->visible_area, TRANSPARENCY_NONE, 0);
	pt+=2;
      }
  } else
    osd_clearbitmap(bitmap);

  if(sf1_active & 0x40) {
    pt = Machine->memory_region[2] + (sf1_deltaxm & 0xfff0)*2 + 0x80 + 0x20000;
    for(x=-(sf1_deltaxm & 0xf);x<384;x+=16)
      for(y=0;y<256;y+=16) {
	int c;
	c=(pt[65537]<<8) | pt[1];
	if(c)
	  drawgfx(bitmap,
		  Machine->gfx[0],
		  c,
		  pt[0],
		  pt[65536]&1, pt[65536]&2,
		  x, y,
		  &Machine->drv->visible_area, TRANSPARENCY_PEN, 15);
	pt+=2;
      }
  }

  if(sf1_active & 0x80) {
    int zz=0;
    pt = sf1_object_ram + 0x2000-0x40;

    while(pt>=sf1_object_ram) {
      int c, at;
      c = READ_WORD(pt);
      at = READ_WORD(pt+2);
      y = READ_WORD(pt+4);
      x = READ_WORD(pt+6);

      if(x>32 && x<415 && y>-32 && y<288) {
	x -= 64;

	if(!(at&0x400)) {
	  drawgfx(bitmap,
		  Machine->gfx[2],
		  sf1_invert(c),
		  at & 0xf,
		  at & 0x100, at & 0x200,
		  x, y,
		  &Machine->drv->visible_area, TRANSPARENCY_PEN, 15);
	} else {
	  int c1, c2, c3, c4;
	  switch(at & 0x300) {
	  case 0x000:
	  default:
	    c1 = c;
	    c2 = c+1;
	    c3 = c+16;
	    c4 = c+17;
	    break;
	  case 0x100:
	    c1 = c+1;
	    c2 = c;
	    c3 = c+17;
	    c4 = c+16;
	    break;
	  case 0x200:
	    c1 = c+16;
	    c2 = c+17;
	    c3 = c;
	    c4 = c+1;
	    break;
	  case 0x300:
	    c1 = c+17;
	    c2 = c+16;
	    c3 = c+1;
	    c4 = c;
	    break;
	  }
	  drawgfx(bitmap,
		  Machine->gfx[2],
		  sf1_invert(c1),
		  at & 0xf,
		  at & 0x100, at & 0x200,
		  x, y,
		  &Machine->drv->visible_area, TRANSPARENCY_PEN, 15);
	  drawgfx(bitmap,
		  Machine->gfx[2],
		  sf1_invert(c2),
		  at & 0xf,
		  at & 0x100, at & 0x200,
		  x+16, y,
		  &Machine->drv->visible_area, TRANSPARENCY_PEN, 15);
	  drawgfx(bitmap,
		  Machine->gfx[2],
		  sf1_invert(c3),
		  at & 0xf,
		  at & 0x100, at & 0x200,
		  x, y+16,
		  &Machine->drv->visible_area, TRANSPARENCY_PEN, 15);
	  drawgfx(bitmap,
		  Machine->gfx[2],
		  sf1_invert(c4),
		  at & 0xf,
		  at & 0x100, at & 0x200,
		  x+16, y+16,
		  &Machine->drv->visible_area, TRANSPARENCY_PEN, 15);
	}
      }
      pt -= 0x40;
      zz+=0x40;
    }
  }

  if(sf1_active & 8) {
    pt = sf1_video_ram+16;

    for(y=0;y<256;y+=8) {
      for(x=0;x<384;x+=8) {
	int c;
	c=READ_WORD(pt);
	drawgfx(bitmap,
		Machine->gfx[3],
		c & 0x3ff,
		c >> 12,
		c & 0x400, c & 0x800,
		x, y,
		&Machine->drv->visible_area, TRANSPARENCY_PEN, 3);
	pt+=2;
      }
      pt += 32;
    }
  }
}
