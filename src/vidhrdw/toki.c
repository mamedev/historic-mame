/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"


#define SPRITE_X	6
#define SPRITE_Y	0
#define SPRITE_TILE	2
#define SPRITE_FLIP_X	2
#define SPRITE_PAL_BANK	4

#define SPRITES_PAL	1
#define FOREGROUND_PAL	0
#define BACKGROUND1_PAL	2
#define BACKGROUND2_PAL	3


unsigned char 		*toki_foreground_videoram;
unsigned char 		*toki_foreground_paletteram;
unsigned char 		*toki_background1_videoram;
unsigned char 		*toki_background1_paletteram;
unsigned char 		*toki_background2_videoram;
unsigned char 		*toki_background2_paletteram;
unsigned char 		*toki_sprites_paletteram;
unsigned char 		*toki_sprites_dataram;
static unsigned char 	*colours;
static unsigned char 	*frg_dirtybuffer;		/* foreground */
static unsigned char 	*bg1_dirtybuffer;		/* background 1 */
static unsigned char 	*bg2_dirtybuffer;		/* background 2 */
static unsigned char 	*frg_palette_dirtybuffer;
static unsigned char 	*bg1_palette_dirtybuffer;
static unsigned char 	*bg2_palette_dirtybuffer;
static struct osd_bitmap *bitmap_frg; 			/* foreground bitmap */
static struct osd_bitmap *bitmap_bg1; 			/* background bitmap 1 */
static struct osd_bitmap *bitmap_bg2; 			/* background bitmap 2 */
static int    		bg1_scrollx,bg1_scrolly;
static int    		bg2_scrollx,bg2_scrolly;
static int		title_on; 			/* title on screen flag */




int toki_vh_start(void)
{
	int i;
	int r, g, b;
	int bit0,bit1,bit2,bit3;

	if ((frg_dirtybuffer = malloc(1024)) == 0)
	{
		return 1;
	}
	if ((bg1_dirtybuffer = malloc(1024)) == 0)
	{
		free(frg_dirtybuffer);
		return 1;
	}
	if ((bg2_dirtybuffer = malloc(1024)) == 0)
	{
		free(bg1_dirtybuffer);
		free(frg_dirtybuffer);
		return 1;
	}
	if ((frg_palette_dirtybuffer = malloc(16)) == 0)
	{
		free(bg1_dirtybuffer);
		free(bg2_dirtybuffer);
		free(frg_dirtybuffer);
		return 1;
	}
	if ((bg1_palette_dirtybuffer = malloc(16)) == 0)
	{
		free(bg1_dirtybuffer);
		free(bg2_dirtybuffer);
		free(frg_dirtybuffer);
		free(frg_palette_dirtybuffer);
		return 1;
	}
	if ((bg2_palette_dirtybuffer = malloc(16)) == 0)
	{
		free(frg_palette_dirtybuffer);
		free(bg1_palette_dirtybuffer);
		free(bg1_dirtybuffer);
		free(bg2_dirtybuffer);
		free(frg_dirtybuffer);
		return 1;
	}

	/* foreground bitmap */
	if ((bitmap_frg = osd_new_bitmap(Machine->drv->screen_width,Machine->drv->screen_height,Machine->scrbitmap->depth)) == 0)
	{
		free(frg_palette_dirtybuffer);
		free(bg1_palette_dirtybuffer);
		free(bg2_palette_dirtybuffer);
		free(bg1_dirtybuffer);
		free(bg2_dirtybuffer);
		free(frg_dirtybuffer);
		return 1;
	}

	/* background1 bitmap */
	if ((bitmap_bg1 = osd_new_bitmap(Machine->drv->screen_width*2,Machine->drv->screen_height*2,Machine->scrbitmap->depth)) == 0)
	{
		free(frg_palette_dirtybuffer);
		free(bg1_palette_dirtybuffer);
		free(bg2_palette_dirtybuffer);
		free(bg1_dirtybuffer);
		free(bg2_dirtybuffer);
		free(frg_dirtybuffer);
		osd_free_bitmap(bitmap_frg);
		return 1;
	}

	/* background2 bitmap */
	if ((bitmap_bg2 = osd_new_bitmap(Machine->drv->screen_width*2,Machine->drv->screen_height*2,Machine->scrbitmap->depth)) == 0)
	{
		free(frg_palette_dirtybuffer);
		free(bg1_palette_dirtybuffer);
		free(bg2_palette_dirtybuffer);
		free(bg1_dirtybuffer);
		free(bg2_dirtybuffer);
		free(frg_dirtybuffer);
		osd_free_bitmap(bitmap_bg1);
		osd_free_bitmap(bitmap_frg);
		return 1;
	}
	if ((colours = malloc(3*4096)) == 0)
	{
		free(frg_palette_dirtybuffer);
		free(bg1_palette_dirtybuffer);
		free(bg2_palette_dirtybuffer);
		free(bg1_dirtybuffer);
		free(bg2_dirtybuffer);
		free(frg_dirtybuffer);
		osd_free_bitmap(bitmap_bg1);
		osd_free_bitmap(bitmap_bg2);
		osd_free_bitmap(bitmap_frg);
		return 1;
	}
	/* Convert all possible colors into a more useable format */
	for(i=0;i<4096;i++)
	{
		bit0 = (i >> 0) & 0x01;
		bit1 = (i >> 1) & 0x01;
		bit2 = (i >> 2) & 0x01;
		bit3 = (i >> 3) & 0x01;
		r = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (i >> 4) & 0x01;
		bit1 = (i >> 5) & 0x01;
		bit2 = (i >> 6) & 0x01;
		bit3 = (i >> 7) & 0x01;
		g = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit1 = (i >> 8) & 0x01;
		bit1 = (i >> 9) & 0x01;
		bit2 = (i >> 10) & 0x01;
		bit3 = (i >> 11) & 0x01;
		b = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		colours[i] = r;
		colours[i+4096] = g;
		colours[i+8192] = b;
	}
	memset(frg_dirtybuffer,0,1024);
	memset(bg2_dirtybuffer,0,1024);
	memset(bg1_dirtybuffer,0,1024);
	memset(frg_palette_dirtybuffer,0,16);
	memset(bg2_palette_dirtybuffer,0,16);
	memset(bg1_palette_dirtybuffer,0,16);
	return 0;

}

void toki_vh_stop(void)
{
	osd_free_bitmap(bitmap_frg);
	osd_free_bitmap(bitmap_bg1);
	osd_free_bitmap(bitmap_bg2);
	free(frg_palette_dirtybuffer);
	free(bg1_palette_dirtybuffer);
	free(bg2_palette_dirtybuffer);
	free(bg1_dirtybuffer);
	free(bg2_dirtybuffer);
	free(frg_dirtybuffer);
	free(colours);
}


void toki_scrollxy_w(int offset, int data)
{
    switch(offset)
    {
	case 0:
	  	bg1_scrolly = ~(data+0x200);
		break;
	case 2:
	 	bg1_scrollx = -((data-0x103) & 0x1FF);
		break;
	case 4:
		bg2_scrolly = ~(data+0x200);
		break;
	case 6:
		bg2_scrollx = -((data-0x101) & 0x1FF);
		break;
    }
}

void toki_setpalette(int kind,int color,int data)
{
	int r, g, b;
	r = colours[data];
	g = colours[data+4096];
	b = colours[data+8192];
	if (kind==1)
	 setgfxcolorentry(Machine->gfx[kind], (color^15), r, g, b);
	else
	 setgfxcolorentry(Machine->gfx[kind], color, r, g, b);
}


void toki_sprites_paletteram_w(int offset, int data)
{
	WRITE_WORD(&toki_sprites_paletteram[offset],data);
	toki_setpalette(SPRITES_PAL,offset>>1,data);
}

int toki_sprites_paletteram_r(int offset)
{
	return READ_WORD(&toki_sprites_paletteram[offset]);
}


void toki_foreground_paletteram_w(int offset, int data)
{
	WRITE_WORD(&toki_foreground_paletteram[offset],data);
	if ((data & 0xF)<0x003 && (data & 0xF0)<0x030 && (data & 0xF00)<0x300 && (offset & 31)!=0x1E)
	   data = 0x3;
	toki_setpalette(FOREGROUND_PAL,offset>>1,data);
	frg_palette_dirtybuffer[offset>>5] = 1;
}

int toki_foreground_paletteram_r(int offset)
{
	return READ_WORD(&toki_foreground_paletteram[offset]);
}

void toki_background1_paletteram_w(int offset, int data)
{
	WRITE_WORD(&toki_background1_paletteram[offset],data);
	if (data==0 && ((offset & 31)!=0x1E)) data=3;
	toki_setpalette(BACKGROUND1_PAL,offset>>1,data);
	bg1_palette_dirtybuffer[offset>>5] = 1;
}

int toki_background1_paletteram_r(int offset)
{
	return READ_WORD(&toki_background1_paletteram[offset]);
}

void toki_background2_paletteram_w(int offset, int data)
{
	WRITE_WORD(&toki_background2_paletteram[offset],data);
	if ((Machine->scrbitmap->depth==8) && ((offset & 31)!=0x1E) &&
	   ((data & 0xF)<0x003 &&
	    (data & 0xF0)<0x030 &&
	    (data & 0xF00)<0x300)) data=3;
	toki_setpalette(BACKGROUND2_PAL,offset>>1,data);
	bg2_palette_dirtybuffer[offset>>5] = 1;
}

int toki_background2_paletteram_r(int offset)
{
	return READ_WORD(&toki_background2_paletteram[offset]);
}

void toki_foreground_videoram_w(int offset, int data)
{
   int oldword = READ_WORD(&toki_foreground_videoram[offset]);
   if (oldword != data)
   {
    WRITE_WORD(&toki_foreground_videoram[offset], data);
    frg_dirtybuffer[offset>>1] = 1;
   }
}

int toki_foreground_videoram_r(int offset)
{
   return READ_WORD(&toki_foreground_videoram[offset]);
}

void toki_background1_videoram_w(int offset, int data)
{
   int oldword = READ_WORD(&toki_background1_videoram[offset]);
   if (oldword != data)
   {
   	WRITE_WORD(&toki_background1_videoram[offset], data);
   	bg1_dirtybuffer[offset>>1] = 1;
   }
}

int toki_background1_videoram_r(int offset)
{
   return READ_WORD(&toki_background1_videoram[offset]);
}

void toki_background2_videoram_w(int offset, int data)
{
   int oldword = READ_WORD(&toki_background2_videoram[offset]);
   if (oldword != data)
   {
   	WRITE_WORD(&toki_background2_videoram[offset], data);
   	bg2_dirtybuffer[offset>>1] = 1;
   }
}

int toki_background2_videoram_r(int offset)
{
   return READ_WORD(&toki_background2_videoram[offset]);
}


void toki_render_sprites(struct osd_bitmap *bitmap)
{
 int SprX,SprY,Data,SprTile,SprFlipX,SprPalette,spr_number;
 unsigned char *SprRegs;

 /* Draw the sprites. 256 sprites in total */

 for(spr_number=0;spr_number<256;spr_number++)
 {
	 SprRegs = &toki_sprites_dataram[spr_number<<3];

	 if (READ_WORD(&SprRegs[SPRITE_Y])==0xF100) break;
	 if (READ_WORD(&SprRegs[SPRITE_PAL_BANK]))
	 {

		 Data = READ_WORD(&SprRegs[SPRITE_X]) & 0x1FF;
		 if (Data & 0x100)
		  SprX = -((Data^0xFF) & 0xFF)-1;
		 else
		  SprX = Data & 0xFF;

		 Data = READ_WORD(&SprRegs[SPRITE_Y]);
		 if (Data & 0x100)
		  SprY = 239+((Data^0xFF) & 0xFF)+1;
		 else
		  SprY = 239-(READ_WORD(&SprRegs[SPRITE_Y]) & 0x1FF);

		 SprFlipX   = READ_WORD(&SprRegs[SPRITE_FLIP_X]) & 0x4000;
		 SprTile    = READ_WORD(&SprRegs[SPRITE_TILE]) & 0x1FFF;
		 SprPalette = READ_WORD(&SprRegs[SPRITE_PAL_BANK])>>12;

		 drawgfx(bitmap,Machine->gfx[1],
			 	SprTile,
				SprPalette,
				SprFlipX,0,SprX,SprY,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	 }
 }
}

void toki_mark_video_dirtybuffer(int posx, int posy, unsigned char *dirtybuf)
{
	int x,y,offs;

	for (y = posy-1; y <= posy+16 ;y++)
	{
		offs = ((posx+16) & 0x1F)+((y & 0x1F)<<5);
		dirtybuf[offs] = 1;
		offs = ((posx-1) & 0x1F)+((y & 0x1F)<<5);
		dirtybuf[offs] = 1;
	}
	for (x = posx-1; x <= posx+16 ;x++)
	{
		offs = (x & 0x1F)+(((posy+16) & 0x1F)<<5);
		dirtybuf[offs] = 1;
		offs = (x & 0x1F)+(((posy-1) & 0x1F)<<5);
		dirtybuf[offs] = 1;
	}
}

void toki_draw_background1(struct osd_bitmap *bitmap)
{
	int sx,sy,code,palette,offs,x,y,scrollx,scrolly;

	scrollx = ((bg1_scrollx*-1) & 0x1FF)>>4;
	scrolly = ((bg1_scrolly*-1) & 0x1FF)>>4;

	toki_mark_video_dirtybuffer(scrollx,scrolly,bg1_dirtybuffer);

	for (y = 0; y <= 16 ;y++)
	{
		for (x = 0; x <= 16 ;x++)
		{

			offs = ((scrollx+x) % 32)+(((scrolly+y) % 32)<<5);
			code = READ_WORD(&toki_background1_videoram[offs<<1]);
			palette = code>>12;

	 		if (bg1_dirtybuffer[offs] || bg1_palette_dirtybuffer[palette])
	 		{
				sx = (offs  % 32) << 4;
				sy = (offs >>  5) << 4;
				bg1_dirtybuffer[offs] = 0;
				drawgfx (bitmap,Machine->gfx[2],
						code & 0xFFF,
						palette,
						0,0,sx,sy,
						0,TRANSPARENCY_NONE,0);
			}
		}

	}
	memset(bg1_palette_dirtybuffer,0,16);
}

void toki_draw_background2(struct osd_bitmap *bitmap)
{
	int sx,sy,code,palette,offs,x,y,scrollx,scrolly;

	scrollx = ((bg2_scrollx*-1) & 0x1FF)>>4;
	scrolly = ((bg2_scrolly*-1) & 0x1FF)>>4;

	toki_mark_video_dirtybuffer(scrollx,scrolly,bg2_dirtybuffer);

	for (y = 0; y <= 16 ;y++)
	{
		for (x = 0; x <= 16 ;x++)
		{
			offs = ((x+scrollx) % 32)+(((y+scrolly) % 32)<<5);
			code = READ_WORD(&toki_background2_videoram[offs<<1]);
			palette = code>>12;

		 	if (bg2_dirtybuffer[offs] || bg2_palette_dirtybuffer[palette])
		 	{
				sx = (offs  % 32) << 4;
				sy = (offs >>  5) << 4;
				bg2_dirtybuffer[offs] = 0;
				drawgfx (bitmap,Machine->gfx[3],
						code & 0xFFF,
						palette,
						0,0,sx,sy,
						0,TRANSPARENCY_NONE,0);
			}
		}
	}
	memset(bg2_palette_dirtybuffer,0,16);
}

void toki_draw_foreground(struct osd_bitmap *bitmap)
{
	int sx,sy,code,palette,offs;

	for (offs = 0;offs < 1024;offs++)
	{
		code = READ_WORD(&toki_foreground_videoram[offs<<1]);
		palette = code>>12;

		if (frg_dirtybuffer[offs] || frg_palette_dirtybuffer[palette])
		{
			sx = (offs % 32) << 3;
			sy = (offs >> 5) << 3;
			frg_dirtybuffer[offs] = 0;
			drawgfx (bitmap,Machine->gfx[0],
					code & 0xFFF,
					palette,
					0,0,sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}
	memset(frg_palette_dirtybuffer,0,16);
}

void toki_vh_screenrefresh(struct osd_bitmap *bitmap)
{

	title_on = (READ_WORD(&toki_foreground_videoram[0x710])==0x44) ? 1:0;

 	toki_draw_foreground(bitmap_frg);
	toki_draw_background1(bitmap_bg1);
 	toki_draw_background2(bitmap_bg2);

	if (title_on)
	{
	        copyscrollbitmap(bitmap,bitmap_bg1,1,&bg1_scrollx,1,&bg1_scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
        	copyscrollbitmap(bitmap,bitmap_bg2,1,&bg2_scrollx,1,&bg2_scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
	} else
	{
	        copyscrollbitmap(bitmap,bitmap_bg2,1,&bg2_scrollx,1,&bg2_scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
        	copyscrollbitmap(bitmap,bitmap_bg1,1,&bg1_scrollx,1,&bg1_scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
	}

	toki_render_sprites(bitmap);
   	copybitmap(bitmap,bitmap_frg,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
}
