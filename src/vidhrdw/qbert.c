/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "common.h"


#define VIDEO_RAM_SIZE 0x400

extern struct osd_bitmap *tmpbitmap;
unsigned char *qbert_videoram;
unsigned char *qbert_paletteram;
unsigned char *qbert_spriteram;
static unsigned char dirtybuffer[VIDEO_RAM_SIZE];       /* keep track of modified portions of the screen */
static unsigned char color_to_lookup[16*16*16];
static int qbert_fb_priority=0;
											/* to speed up video refresh */
void qbert_vh_init_color_palette(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;
	unsigned short *colors=(unsigned short *)color_prom;

	for (i = 0;i<213 ;i++)
	{
		unsigned int red,green,blue;
		red=(colors[i]>>8) & 0x0F;
		green=(colors[i]>>4) & 0x0F;
		blue=colors[i] & 0x0F;
		palette[3*i]=(red<<4) | red;
		palette[3*i+1]=(green<<4) | green;
		palette[3*i+2]=(blue<<4) | blue;
		if (colors[i]) color_to_lookup[colors[i]]=i;
	}

	for (i = 0;i < 256;i++) colortable[i] = i;
	for (i=1;i<15;i++) {
		colortable[256+16+i]=1;	/* white */
		colortable[256+32+i]=2; /* a yellow */
	}
}

void mplanets_vh_init_color_palette(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
        int i;

        for (i = 0;i < 256;i++)
        {
                int bits;


                bits = (i >> 0) & 0x07;
                palette[3*i] = (bits >> 1) | (bits << 2) | (bits << 5);
                bits = (i >> 3) & 0x07;
                palette[3*i + 1] = (bits >> 1) | (bits << 2) | (bits << 5);
                bits = (i >> 6) & 0x03;
                palette[3*i + 2] = bits | (bits >> 2) | (bits << 4) | (bits << 6);
        }

        for (i = 0;i < 256;i++)
                colortable[i] = i;

	for (i=1;i<15;i++) {
		colortable[256+16+i]=0xFF;	/* white */
		colortable[256+32+i]=0x3F; /* yellow */
	}
}


void qbert_videoram_w(int offset,int data)
{
	if (qbert_videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		qbert_videoram[offset] = data;
	}
}

void qbert_output(int offset,int data)
{
	qbert_fb_priority= data & 1;
}


void qbert_paletteram_w(int offset,int data)
{
	int offs;
	if (qbert_paletteram[offset]!=data) {
		qbert_paletteram[offset] = data;
		if (offset%2)
			for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
				dirtybuffer[offs] = 1;
	}
/*
if (offset%2) printf("%x%02x\n",qbert_paletteram[offset],qbert_paletteram[offset-1]);
*/
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void qbert_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,i,col;

	/* rebuild the color lookup table */
	for (i=0;i<16;i++) {
		col = qbert_paletteram[2*i] + ((qbert_paletteram[2*i+1]&0x0F) << 8);
		Machine->gfx[0]->colortable[i] =
			Machine->gfx[1]->colortable[i] =
				Machine->gfx[2]->colortable[color_to_lookup[col]];

	}

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 8 * (offs / 32);
			sy = 8 * (31 - offs % 32);

			drawgfx(tmpbitmap,Machine->gfx[0],
				qbert_videoram[offs],  /* code */
				0, /* color tuple */
				0,      /* don't flip X */
				0,      /* don't flip Y */
				sx,sy,
				&Machine->drv->visible_area, /* clip */
				TRANSPARENCY_NONE,
				0       /* transparent color */
				);
		}
	}

	if (!qbert_fb_priority)
	/* copy the character mapped graphics */
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	else
	/* or clear it if sprites are to be drawn first */
		clearbitmap(bitmap);

	/* Draw the sprites. Note that it is important to draw them exactly in this */
	/* order, to have the correct priorities. */
	for (offs = 0;offs<64*4 ;offs += 4)
	{
	    if (qbert_spriteram[offs]||qbert_spriteram[offs+1])
		drawgfx(bitmap,Machine->gfx[1],
				255-qbert_spriteram[offs+2], /* object # */
				0, /* color tuple */
				0, /* flip x */
				0, /* flip y */
				qbert_spriteram[offs]-12, /* vert pos -> X pos */
				255-12-qbert_spriteram[offs+1], /* horiz pos -> Y pos */
				&Machine->drv->visible_area,
				TRANSPARENCY_PEN,
				0);
	}
	if (qbert_fb_priority)
		copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
}

/* same one with the screen upside down 8-) */

void mplanets_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,i,col;

	/* rebuild the color lookup table */
        for (i = 0;i < 16;i++)
        {
                col = (qbert_paletteram[2*i+1] >> 1) & 0x07;   /* red component */
                col |= (qbert_paletteram[2*i] >> 2) & 0x38;  /* green component */
                col |= (qbert_paletteram[2*i] << 4) & 0xc0;      /* blue component */
		Machine->gfx[0]->colortable[i] =
			Machine->gfx[1]->colortable[i] =
                		Machine->gfx[2]->colortable[col];
        }

        /* for every character in the Video RAM, check if it has been modified */
        /* since last time and update it accordingly. */
        for (offs = 0;offs < VIDEO_RAM_SIZE;offs++)
        {
                if (dirtybuffer[offs])
                {
                        int sx,sy;


                        dirtybuffer[offs] = 0;

                        sx = 8 * (29 - offs / 32);
                        sy = 8 * (offs % 32);

                        drawgfx(tmpbitmap,Machine->gfx[0],
                                qbert_videoram[offs],  /* code */
                                0, /* color */
                                0,      /* don't flip X */
                                0,      /* don't flip Y */
                                sx,sy,
                                &Machine->drv->visible_area, /* clip */
                                TRANSPARENCY_NONE,
                                0       /* transparent color */
                                );
                }
        }

        if (!qbert_fb_priority)
        /* copy the character mapped graphics */
                copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
        else
        /* or clear it if sprites are to be drawn first */
                clearbitmap(bitmap);

        /* Draw the sprites. Note that it is important to draw them exactly in this */
        /* order, to have the correct priorities. */
        for (offs = 0;offs<64*4 ;offs += 4)
        {
            if (qbert_spriteram[offs]||qbert_spriteram[offs+1])
                drawgfx(bitmap,Machine->gfx[1],
                                255-qbert_spriteram[offs+2], /* object # */
                                0, /* color */
                                0, /* don't flip x */
                                0, /* don't flip y */
                                255-12-qbert_spriteram[offs], /* vert pos -> X pos */
                                qbert_spriteram[offs+1]+4, /* horiz pos -> Y pos */
                                &Machine->drv->visible_area,
                                TRANSPARENCY_PEN,
                                0);
        }
        if (qbert_fb_priority)
                copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
}
