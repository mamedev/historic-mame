/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"

#define VIDEO_RAM_SIZE 0x400

struct osd_bitmap *sprite_bm;
struct osd_bitmap *mask_bm;

int ccastles_vh_start(void)
{
  int len;


  len = (Machine->drv->screen_width/8) * (Machine->drv->screen_height/8);
  len = (len + 0x3ff) & 0xfffffc00;

  if((dirtybuffer = malloc(len)) == 0) {
    return 1;
  }
  
  memset(dirtybuffer,0,len);

  if((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0) {
    free(dirtybuffer);
    return 1;
  }

  if((sprite_bm = osd_create_bitmap(8,16)) == 0) {
    free(dirtybuffer);
    osd_free_bitmap(tmpbitmap);
    return(1);
  }

  if((mask_bm = osd_create_bitmap(8,16)) == 0) {
    free(dirtybuffer);
    osd_free_bitmap(tmpbitmap);
    osd_free_bitmap(sprite_bm);
    return(1);
  }

  return 0;
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void ccastles_vh_stop(void) {
  free(dirtybuffer);
  osd_free_bitmap(sprite_bm);
  osd_free_bitmap(mask_bm);
  osd_free_bitmap(tmpbitmap);
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void ccastles_vh_screenrefresh(struct osd_bitmap *bitmap) {
  int offs;
  int i,j;
  int x,y,x1,y1;
  int spriteaddr;
  int rowscroll[1];
  int colscroll[1];
  struct rectangle mask_rec = {0,7,0,15};
  char pixa;
  char pixb;
  
  j=0;

  /* No dirtybuffers yet...  The dirtybuffers will need to be set
     in the ccastls_bitmode_w funtion...  So for now, just blow the
     entire screen DRAM contents into the tmpbitmap. */
  for(i=0x0C00;i<0x8000;i+=2) {
    *(tmpbitmap->line[(j/256)]+(j%256)) = ((RAM[i] & 0xF0) >> 4) + 0x0A;
    *(tmpbitmap->line[(j/256)]+(j%256)+1) = (RAM[i] & 0x0F) + 0x0A;
    *(tmpbitmap->line[(j/256)]+(j%256)+2) = ((RAM[i+1] & 0xF0) >> 4) + 0x0A;
    *(tmpbitmap->line[(j/256)]+(j%256)+3) = (RAM[i+1] & 0x0F) + 0x0A;

    j+=4;
  }

  colscroll[0]=0x100-RAM[0x9D00];
  rowscroll[0]=0x100-RAM[0x9C80];

  copyscrollbitmap(bitmap,tmpbitmap,1,rowscroll,1,colscroll,
		   &Machine->drv->visible_area,
		   TRANSPARENCY_NONE,0);

  /*  copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,
	     TRANSPARENCY_NONE,0); */

  if(RAM[0x9F07]) {
    spriteaddr = 0x8F00;
  } else {
    spriteaddr = 0x8E00;
  }

  /* Draw the sprites */
  for (offs = 0x00;offs < 0x100;offs+=0x04) {
    if(RAM[spriteaddr+offs+2]&0x80) {

      /* Get the X and Y coordinates from the MOB RAM */ 
      x = RAM[spriteaddr+offs+3];
      y = 0xD8-RAM[spriteaddr+offs+1];

      for(j=0;j<16;j++) {
	y1 = y+j;

	for(i=0;i<8;i++) {
	  x1 = x+i;

	  if(x1 > Machine->drv->visible_area.max_x) {
	    mask_bm->line[j][i] = 0x0A;
	    break;
	  }

	  if(x1 < Machine->drv->visible_area.min_x) {
	    mask_bm->line[j][i] = 0x0A;
	    break;
	  }
	  
	  if(y1 > Machine->drv->visible_area.max_y) {
	    mask_bm->line[j][i] = 0x0A;
	    break;
	  }

	  if(y1 < Machine->drv->visible_area.min_y) {
	    mask_bm->line[j][i] = 0x0A;
	    break;
	  }
	  
	  mask_bm->line[j][i] = bitmap->line[y1][x1];
	}
      }

      drawgfx(sprite_bm,Machine->gfx[1],RAM[spriteaddr+offs],1,0,0,0,0,
	      &mask_rec,TRANSPARENCY_NONE,0);

      for(j=0;j<16;j++) {
	for(i=0;i<8;i++) {
	  pixa = sprite_bm->line[j][i];
	  pixb = mask_bm->line[j][i];

	  if((pixb >= 10) && (pixb <= 18) && (pixa != 9)) {
	    mask_bm->line[j][i] = pixa;
	  } else {
	    if(pixa == 2) {
	      mask_bm->line[j][i] = pixa;
	    } else {
	      pixb = mask_bm->line[j][i];
	    }
	  }
	}
      }

      copybitmap(bitmap,mask_bm,0,0,RAM[spriteaddr+offs+3],
		 0xD8-RAM[spriteaddr+offs+1],&Machine->drv->visible_area,
		 TRANSPARENCY_NONE,0);
    } else {
      drawgfx(bitmap,Machine->gfx[1],RAM[spriteaddr+offs],1,0,0,
	      RAM[spriteaddr+offs+3],0xD8-RAM[spriteaddr+offs+1],
	      &Machine->drv->visible_area,TRANSPARENCY_PEN,7);
    }
  }
}

