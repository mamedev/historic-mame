/***************************************************************************


  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *gaiden_videoram;    /* 4 KB for video RAM */
unsigned char *gaiden_spriteram;   /* 8 KB for sprite RAM */
unsigned char *gaiden_paletteram;  /* 8 KB for palette RAM */
unsigned char *gaiden_videoram3;   /*16 KB for video RAM */

void gaiden_vh_stop (void);

int gaiden_videoram_size;
int gaiden_spriteram_size;
int gaiden_paletteram_size;
int gaiden_videoram3_size;

static struct osd_bitmap *tmpbitmap1;
static struct osd_bitmap *tmpbitmap2;
static struct osd_bitmap *tmpbitmap3;

static unsigned char *gaiden_dirty;
static unsigned char *gaiden_dirty2;
static unsigned char *gaiden_dirty3;
static unsigned char gaiden_dirtypal[0x100];
static unsigned char gaiden_dirty_bgcol;

unsigned char *gaiden_scrolla;
unsigned char *gaiden_scrollb;
static unsigned int col_test;

/*
 *   video system start
 */

int gaiden_vh_start (void)
{
	/* Allocate a video RAM */
        gaiden_dirty = malloc ( gaiden_videoram_size / 2);
	gaiden_dirty3 = malloc ( gaiden_videoram3_size/2);

	if (!gaiden_dirty)

	{
                gaiden_vh_stop ();
		return 1;
	}
	if (!gaiden_dirty3)
	{
                gaiden_vh_stop ();
		return 1;
	}

	/* Allocate temporary bitmaps */
	if ((tmpbitmap1 = osd_new_bitmap (Machine->drv->screen_width, Machine->drv->screen_height, Machine->scrbitmap->depth)) == 0)
	{
                gaiden_vh_stop ();
		return 1;
	}
 	if ((tmpbitmap2 = osd_new_bitmap (1024 , 512, Machine->scrbitmap->depth)) == 0)
	{
                gaiden_vh_stop ();
		return 1;
	}
	if ((tmpbitmap3 = osd_new_bitmap (1024 , 512, Machine->scrbitmap->depth)) == 0)
	{
                gaiden_vh_stop ();
		return 1;
	}
	if (Machine->scrbitmap->depth==8) col_test=2; else col_test = 1;
	return 0;
}


/*
 *   video system shutdown; we also bring down the system memory as well here
 */

void gaiden_vh_stop (void)
{
	/* Free temporary bitmaps */
	if (tmpbitmap3)
		osd_free_bitmap (tmpbitmap3);
	tmpbitmap3 = 0;
	if (tmpbitmap2)
		osd_free_bitmap (tmpbitmap2);
	tmpbitmap2 = 0;
	if (tmpbitmap1)
		osd_free_bitmap (tmpbitmap1);
	tmpbitmap = 0;

	/* Free video RAM */
	if (gaiden_dirty)
		free (gaiden_dirty);
	if (gaiden_dirty2)
	        free (gaiden_dirty2);
	if (gaiden_dirty3)
	        free (gaiden_dirty3);
	gaiden_dirty = gaiden_dirty2 = gaiden_dirty3 = 0;
}



/*
 *   scroll write handlers
 */

void gaiden_scrolla_w (int offset, int data)
{
	COMBINE_WORD_MEM (&gaiden_scrolla[offset], data);
}

void gaiden_scrollb_w (int offset, int data)
{
	COMBINE_WORD_MEM (&gaiden_scrollb[offset], data);
}



/*
 *   palette RAM read/write handlers
 */
void gaiden_paletteram_w(int offset,int data)
{
	int oldword = READ_WORD(&gaiden_paletteram[offset]);
	int newword = COMBINE_WORD(oldword,data);


	if (oldword != newword)
	{
		WRITE_WORD(&gaiden_paletteram[offset],newword);
                /* set the entire palette to dirty */
	        /* palette is offset/0x20          */
		gaiden_dirtypal[offset>>5] = 1;
		if (offset==0x0400) gaiden_dirty_bgcol=1;
	}
}
int gaiden_paletteram_r(int offset)
{
	return READ_WORD (&gaiden_paletteram[offset]);
}

/*
 *   video RAM 3 read/write handlers
 */
void gaiden_videoram3_w (int offset, int data)
{
	int oldword = READ_WORD(&gaiden_videoram3[offset]);
	int newword = COMBINE_WORD(oldword,data);


	if (oldword != newword)
	{
		WRITE_WORD(&gaiden_videoram3[offset],newword);
		gaiden_dirty3[offset / 2] = 1;
	}
}
int gaiden_videoram3_r (int offset)
{
   return READ_WORD (&gaiden_videoram3[offset]);
}

void gaiden_videoram_w (int offset, int data)
{
	int oldword = READ_WORD(&gaiden_videoram[offset]);
	int newword = COMBINE_WORD(oldword,data);


	if (oldword != newword)
	{
		WRITE_WORD(&gaiden_videoram[offset],newword);
		gaiden_dirty[offset / 2] = 1;
	}
}

int gaiden_videoram_r (int offset)
{
   return READ_WORD (&gaiden_videoram[offset]);
}

void gaiden_spriteram_w (int offset, int data)
{
	COMBINE_WORD_MEM (&gaiden_spriteram[offset], data);
}

int gaiden_spriteram_r (int offset)
{
   return READ_WORD (&gaiden_spriteram[offset]);
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
/*
 *  some ideas from tecmo.c
 *  (sprites in separate function and handling of 16x16 sprites)
 */

void gaiden_drawsprites(struct osd_bitmap *bitmap, int priority)
{
    /*
     * Sprite Format
     * ------------------
     *
     * Byte(s) | Bit(s)   | Use
     * --------+-76543210-+----------------
     *    0    | -------- | unknown
     *    1    | -------x | flip x
     *    1    | ------x- | flip y
     *    1    | -----x-- | active (show this sprite)
     *    1    | x------- | occluded (sprite is behind front background layer
     *    2    | xxxxxxxx | Sprite Number (High 8 bits)
     *    3    | xxxx---- | Sprite Number (Low 4 bits)
     *    3    | ----xx-- | Additional Index (for small sprites)
     *    4    | -------- | unknown
     *    5    | ------xx | size 1=16x16, 2=32x32;
     *    5    | xxxx---- | palette
     *    6    | xxxxxxxx | y position (high 8 bits)
     *    7    | xxxxxxxx | y position (low 8 bits)
     *    8    | xxxxxxxx | x position (negative of high 8 bits)
     *    9    | -------- | x position (low 8 bits)
     */
        int offs;
	for (offs = 0; offs<0x800 ; offs += 16)
	{
		int sx,sy,col,spr_pri;
		int num,bank,flip,size;
//		sx = -(READ_WORD(&gaiden_spriteram[offs+8])&0xff00)
//		     + (READ_WORD(&gaiden_spriteram[offs+8])&0x00ff);
		sx = READ_WORD(&gaiden_spriteram[offs+8]) & 0x1ff;
		if (sx >= 256) sx -= 512;
		sy = READ_WORD(&gaiden_spriteram[offs+6]) & 0x1ff;
		if (sy >= 256) sy -= 512;
		num = READ_WORD(&gaiden_spriteram[offs+2])>>4;
		spr_pri=(READ_WORD(&gaiden_spriteram[offs])&0x0080)>>7;
		if ( (READ_WORD(&gaiden_spriteram[offs])&0x0004) && sx>=(-31) && sx < 256 && sy < 256 && (spr_pri==priority))
		{
		        num &= 0x7ff;
                        bank=num/0x200;
			size = READ_WORD(&gaiden_spriteram[offs+4])&0x0003;
			flip = READ_WORD(&gaiden_spriteram[offs])&0x0003;
			col = ((READ_WORD(&gaiden_spriteram[offs+4])&0x00f0)>>4);
			if (size==1)
			{
			        num=(num<<2)+((READ_WORD(&gaiden_spriteram[offs+2])&0x000c)>>2);
				bank+=4;
			}
			drawgfx(bitmap, Machine->gfx[3+bank],
				num,
				col,
				flip&0x01,flip&0x02,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);

		}

	}


}

void gaiden_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,pom;
	int scrollx,scrolly;
	int i,col,value;
	int abc,red,green,blue;

	/* update palette */
	for (pom = 0x0 ; pom < 0x100 ; pom++)
	{
	        if (gaiden_dirtypal[pom])
		{
		        for (i=1;i<16;i++)
			{
			        abc = READ_WORD(&gaiden_paletteram[(pom<<5)+i*2]);
				blue  = (abc&0x0f00)>>8;
				green = (abc&0x00f0)>>4;
				red   = (abc&0x000f);
				if ((red<=col_test) && (green<=col_test) && (blue<=(col_test<<1)) && (i%16!=0)) red = col_test;
				setgfxcolorentry (Machine->gfx[0], (pom<<4)+i, (red<<4)+red, (green<<4)+green, (blue<<4)+blue);
			}
			setgfxcolorentry (Machine->gfx[0], (pom<<4), 0, 0, 0); /* hack */
		}
	}
	if (gaiden_dirty_bgcol)
	{
	        abc = READ_WORD(&gaiden_paletteram[0x400]);
		blue  = (abc&0x0f00)>>8;
		green = (abc&0x00f0)>>4;
		red   = (abc&0x000f);
		for (pom = 0x30;pom<0x40;pom++)
		{
		        setgfxcolorentry (Machine->gfx[0], (pom<<4), (red<<4)+red, (green<<4)+green, (blue<<4)+blue);
			gaiden_dirtypal[pom]=1;
		}
		gaiden_dirty_bgcol=0;
	}

	/* update text tiles */
	for (pom = 0x0 ; pom < 0x1000/2 ; pom +=2)
	{
		int sx,sy;
		col = ((READ_WORD(&gaiden_videoram[pom])&0x00f0)>>4)+0x10;
		offs=pom/2;
		if (gaiden_dirty[offs] || gaiden_dirty[offs+0x400] || gaiden_dirtypal[col] )
		{
			gaiden_dirty[offs] = gaiden_dirty[offs+0x400] = 0;
			sx = 8 * (offs % 32);
			sy = 8 * (offs / 32);
			value=READ_WORD(&gaiden_videoram[pom+0x800]);
			i=(value&0x7FF);
			drawgfx(tmpbitmap1, Machine->gfx[0],
					i,
					col,   /* color */
					0,0, /* no flip */
					sx,sy, /* x,y */
					0,TRANSPARENCY_NONE,0);

		}
	}
	/* update graphics tiles */
	for (pom = 0x0 ; pom < 0x1000 ; pom+=2)
	{
	        int sx,sy,offs;
		offs=pom/2;
		col = ((READ_WORD(&gaiden_videoram3[pom])&0x00f0)>>4)+0x20;
		if ( gaiden_dirty3[offs] || gaiden_dirty3[offs+0x0800] || gaiden_dirtypal[col])
		{
			gaiden_dirty3[offs]=gaiden_dirty3[offs+0x0800]=0;
		        sx = 16 * (offs % 64);
			sy = 16 * (offs / 64);
			i = READ_WORD(&gaiden_videoram3[pom+0x1000])&0x0fff;
			drawgfx(tmpbitmap2, Machine->gfx[2],
				        i,
				        col,  /* color */
					0,0, /* no flip */
					sx,sy, /* x,y */
					0,TRANSPARENCY_NONE,0);
		}
		offs=(0x2000+pom)/2;
		col = ((READ_WORD(&gaiden_videoram3[pom+0x2000])&0x00f0)>>4)+0x30;
		if ( gaiden_dirty3[offs] || gaiden_dirty3[offs+0x0800] || gaiden_dirtypal[col])
		{
			gaiden_dirty3[offs]=gaiden_dirty3[offs+0x0800]=0;
		        offs=pom/2;
		        sx = 16 * (offs % 64);
			sy = 16 * (offs / 64);
			i = READ_WORD(&gaiden_videoram3[pom+0x3000])&0x0fff;
			drawgfx(tmpbitmap3, Machine->gfx[1],
				i,
				col,  /* color */
				0,0, /* no flip */
				sx,sy, /* x,y */
				0,TRANSPARENCY_NONE,0);
		}


	}

	scrollx = (-(READ_WORD(&gaiden_scrollb[0x0c]))%1024);
	scrolly = -(READ_WORD(&gaiden_scrollb[0x04]));
	copyscrollbitmap(bitmap,tmpbitmap3,1,&scrollx,1,&scrolly,&Machine->drv->visible_area, TRANSPARENCY_NONE,0);
	scrollx = (-(READ_WORD(&gaiden_scrolla[0x0c]))%1024);
	scrolly = -(READ_WORD(&gaiden_scrolla[0x04]));

	/* draw occluded sprites */
	gaiden_drawsprites(bitmap,1);

	copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area, TRANSPARENCY_COLOR,0);

	/* draw non-occluded sprites */
	gaiden_drawsprites(bitmap,0);

	copybitmap(bitmap,tmpbitmap1,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);


	memset(gaiden_dirtypal,0,0x100);
	return;
}

