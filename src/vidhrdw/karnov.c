/*******************************************************************************

	Karnov - Bryan McPhail, mish@tendril.force9.net

  This file contains video emulation for:

  	* Karnov (Data East USA, 1987)
    * Karnov (Japan rom set - Data East Corp, 1987)
    * Chelnov (Japan rom set - Data East Corp, 1988)


Sprite Y-flip still not found...is it needed?

*******************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *karnov_foreground,*karnov_sprites,*dirty_f;
static struct osd_bitmap *bitmap_f;
int karnov_scroll[4];

/******************************************************************************/

void karnov_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int my,mx,offs,color,tile;
  int scrollx=READ_WORD (&karnov_scroll[0]);
  int scrolly=READ_WORD (&karnov_scroll[2]);

  /* 1st area is stored along X-axis... */
  mx=-1; my=0;
  for (offs = 0;offs < 0x800; offs += 2) {
    mx++;
    if (mx==32) {mx=0; my++;}

    if (!dirty_f[offs]) continue; else dirty_f[offs]=0;

    tile=READ_WORD (&karnov_foreground[offs]);
    color = 32+((tile & 0xf000) >> 12);
    tile=tile&0x7ff;

		drawgfx(bitmap_f,Machine->gfx[1],tile,
			color, 0,0, 16*mx,16*my,
		 	0,TRANSPARENCY_NONE,0);
	}

  /* 2nd area is stored along Y-axis... */
	mx=0; my=-1;
  for (offs = 0x800 ;offs < 0x1000; offs += 2) {
    my++;
    if (my==32) {my=0; mx++;}

    if (!dirty_f[offs]) continue; else dirty_f[offs]=0;

    tile=READ_WORD (&karnov_foreground[offs]);
    color = 32+((tile & 0xf000) >> 12);
    tile=tile&0x7ff;

		drawgfx(bitmap_f,Machine->gfx[1],tile,
			color, 0,0, 16*mx,16*my,
		 	0,TRANSPARENCY_NONE,0);
	}

  scrolly=-scrolly;
  scrollx=-scrollx;
  copyscrollbitmap(bitmap,bitmap_f,1,&scrollx,1,&scrolly,0,TRANSPARENCY_NONE,0);

  /* Sprites */
  for (offs = 0;offs <0x800;offs += 8) {
   	int x,y,sprite,bank,colour,fx,fy,extra;

    /* bit 7 of 1st bytes seems to be whether sprite is displayed... */
    y=READ_WORD (&karnov_sprites[offs]);
    if (!(y&0x8000)) continue;

    y=y&0x1ff;
    sprite=READ_WORD (&karnov_sprites[offs+6]);
    colour=(sprite&0xf000)>>12;
    sprite=0xfff&sprite;
    if (sprite<2048) bank=2; else {bank=3; sprite-=2048;}
    x=0x1ff&READ_WORD (&karnov_sprites[offs+4]);

    /* Convert the co-ords..*/
    x=(x+16)%0x200; y=(y+16)%0x200;
	  x=(16*16) - x; y=(16*16) - y;

 		fx=READ_WORD (&karnov_sprites[offs+2]);

    if ((fx&0x10)) extra=1; else extra=0;

    fx=fx&0x4;
    fy=0; /* Y Flip still unknown */

    if (extra) y=y-16;

    drawgfx(bitmap,Machine->gfx[bank],
				sprite,
				colour+16,fx,0,x,y,
				0,TRANSPARENCY_PEN,0);

    /* 1 more sprite */
    if (extra) {
    	drawgfx(bitmap,Machine->gfx[bank],
				sprite+1,
				colour+16,fx,fy,x,y+16,
				0,TRANSPARENCY_PEN,0);
    }
   }

   /* Draw character tiles */
   for (offs = videoram_size - 2;offs >= 0;offs -= 2) {
      tile=READ_WORD (&videoram[offs]);
      if (!tile) continue;
      color=(0x8000&tile)>>15;
      tile=0xfff&tile;
			mx = (offs/2) % 32;
			my = (offs/2) / 32;
			drawgfx(bitmap,Machine->gfx[0],
				tile,color,0,0,8*mx,8*my,
				&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
   }
}

/******************************************************************************/

void karnov_foreground_w(int offset, int data)
{
  WRITE_WORD (&karnov_foreground[offset], data);
  dirty_f[offset] = 1;
}

/******************************************************************************/


void karnov_vh_stop (void)
{
  if (dirty_f) free(dirty_f);
  if (karnov_foreground) free(karnov_foreground);
  if (bitmap_f) osd_free_bitmap (bitmap_f);
  generic_vh_stop();
}

int karnov_vh_start (void)
{
	/* Allocate bitmaps */
	if ((bitmap_f = osd_create_bitmap(512,512)) == 0) {
		karnov_vh_stop ();
		return 1;
	}

  dirty_f=malloc(0x1000);
	karnov_foreground=malloc(0x1000);
  memset(karnov_foreground,0,0x1000);
  memset(dirty_f,1,0x1000);

	return generic_vh_start();
}

/******************************************************************************/

void karnov_palette(void)
{
  int kColours_Allocated=2,pen,r,g,b,i,z,offset;
  int kdirty_pal_r[1024];
  int kdirty_pal_g[1024];
  int kdirty_pal_b[1024];

  for (i=0; i<1024; i++) {
  	kdirty_pal_r[i]=0;
    kdirty_pal_b[i]=0;
    kdirty_pal_g[i]=0;
  }

  osd_modify_pen(Machine->pens[0],0,0,0);
  osd_modify_pen(Machine->pens[1],1,0,0);

  /* initialize the color table */
	for (i = 0;i < Machine->drv->total_colors;i++)
		Machine->gfx[0]->colortable[i] = i;

  for (i=0; i<1024; i++) {
    r=15*(Machine->memory_region[3][i] & 0xf);
    g=15*(Machine->memory_region[3][i] >> 4);
    b=15*(Machine->memory_region[3][i+1024]);

    offset=i*2;

    /* Search list of pens to see if this colour is already allocated */
    pen=-1;
 		for (z=0; z<kColours_Allocated; z++)
     	if (kdirty_pal_r[z]==r && kdirty_pal_b[z]==b && kdirty_pal_g[z]==g) {
       	pen=z;
        break;
      }

    if (pen>-1) {
      Machine->gfx[0]->colortable[offset/2]=Machine->pens[pen];
      Machine->gfx[1]->colortable[offset/2]=Machine->pens[pen];
      Machine->gfx[2]->colortable[offset/2]=Machine->pens[pen];
      Machine->gfx[3]->colortable[offset/2]=Machine->pens[pen];
   	  continue;
    }

  /* Allocate a pen number for this colour, Neither Karnov nor Chelnov overflows.. */
  if (kColours_Allocated>255) {
  	kColours_Allocated++;
  	continue;
  }

  osd_modify_pen(Machine->pens[kColours_Allocated],r,g,b);

  kdirty_pal_r[kColours_Allocated]=r;
  kdirty_pal_g[kColours_Allocated]=g;
  kdirty_pal_b[kColours_Allocated]=b;

  Machine->gfx[0]->colortable[offset/2]=Machine->pens[kColours_Allocated];
  Machine->gfx[1]->colortable[offset/2]=Machine->pens[kColours_Allocated];
  Machine->gfx[2]->colortable[offset/2]=Machine->pens[kColours_Allocated];
  Machine->gfx[3]->colortable[offset/2]=Machine->pens[kColours_Allocated];
  kColours_Allocated++;
  }
}

/******************************************************************************/

