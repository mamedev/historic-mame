/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern int rastan_system_start (void);
extern int rastan_system_stop (void);

void rastan_vh_stop (void);

int rastan_paletteram_size;
int rastan_videoram1_size;
int rastan_videoram2_size;
int rastan_videoram3_size;
int rastan_videoram4_size;
int rastan_spriteram_size;

static struct osd_bitmap *tmpbitmap2;
static struct osd_bitmap *tmpbitmap3;

static unsigned char *dirty1;		/* 16KB .L organization */
static unsigned char *dirty3;		/* 16KB .L organization */

static unsigned char *paletteram;   /*64KB for palette RAM*/
static unsigned char *spriteram;   /*64KB for sprite RAM*/
static unsigned char pal_dirty[0x80];	  /*only 0x80 color schemes are in use*/

static unsigned char *videoram1;   /*16KB for video RAM 8x8   0x1000.L */
static unsigned char *videoram2;   /*16KB for video RAM 16x16 0x2000.W */
static unsigned char *videoram3;   /*16KB for video RAM 8x8   0x1000.L */
static unsigned char *videoram4;   /*16KB for video RAM 16x16 0x2000.W */

static unsigned char scrollY[4];
static unsigned char scrollX[4];


/***************************************************************************

  Convert the color PROMs into a more useable format.

  RASTAN doesn't have a color PROM. It uses 64KB bytes of RAM to
  dynamically create the palette. Each couple of bytes defines one
  color (5 bits per R,G,B; the highest 1 bit of the first byte is unused).
  Graphics use 4 bitplanes. It seems only first 0x80 colors are in use.
  Game uses 512+ colors at run time so there's no chance for optimized
  palette.

***************************************************************************/
void rastan_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
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
		colortable[0x80*16+i] = i;
}


/*
 *   video system start; we also initialize the system memory as well here
 */

int rastan_vh_start (void)
{
	/* Allocate RAM */
	if (rastan_system_start ())
		return 1;

	/* Allocate a video RAM */
	paletteram = malloc (rastan_paletteram_size +
	                     rastan_videoram1_size + rastan_videoram1_size / 4 +
	                     rastan_videoram2_size +
	                     rastan_videoram3_size + rastan_videoram3_size / 4 +
	                     rastan_videoram4_size +
	                     rastan_spriteram_size);
	if (!paletteram)
	{
		rastan_vh_stop ();
		return 1;
	}

	/* dole out the video RAM */
	videoram1 = paletteram + rastan_paletteram_size;
	dirty1    = videoram1  + rastan_videoram1_size;
	videoram2 = dirty1     + rastan_videoram1_size / 4;
	videoram3 = videoram2  + rastan_videoram2_size;
	dirty3    = videoram3  + rastan_videoram3_size;
	videoram4 = dirty3     + rastan_videoram3_size / 4;
	spriteram = videoram4  + rastan_videoram4_size;

	/* set up the banks */
	cpu_setbank (2, videoram2);
	cpu_setbank (3, videoram4);
	cpu_setbank (4, spriteram);

	/* Allocate temporary bitmaps */
	if ((tmpbitmap = osd_create_bitmap (Machine->drv->screen_width, Machine->drv->screen_height)) == 0)
	{
		rastan_vh_stop ();
		return 1;
	}
 	if ((tmpbitmap2 = osd_create_bitmap (512, 512)) == 0)
	{
		rastan_vh_stop ();
		return 1;
	}
	if ((tmpbitmap3 = osd_create_bitmap(512,512)) == 0)
	{
		rastan_vh_stop ();
		return 1;
	}

	return 0;
}


/*
 *   video system shutdown; we also bring down the system memory as well here
 */

void rastan_vh_stop (void)
{
	/*DebugOutput ();*/

	/* Free temporary bitmaps */
	if (tmpbitmap3)
		osd_free_bitmap (tmpbitmap3);
	tmpbitmap3 = 0;
	if (tmpbitmap2)
		osd_free_bitmap (tmpbitmap2);
	tmpbitmap2 = 0;
	if (tmpbitmap)
		osd_free_bitmap (tmpbitmap);
	tmpbitmap = 0;

	/* Free video RAM */
	if (paletteram)
		free (paletteram);
	paletteram = videoram1 = dirty1 = videoram2 = videoram3 = dirty3 = videoram4 = spriteram = 0;

	/* Free system */
	rastan_system_stop ();
}



/*
 *   scroll write handlers
 */

void rastan_scrollY_w (int offset, int data)
{
	scrollY[offset] = data;
}

void rastan_scrollX_w (int offset, int data)
{
	scrollX[offset] = data;
}


/*
 *   palette RAM read/write handlers
 */

void rastan_paletteram_w (int offset, int data)
{
	paletteram[offset] = data;
	offset /= 32;
	if (offset < 0x80)
		pal_dirty[offset] = 1;
}

int rastan_paletteram_r (int offset)
{
	return paletteram[offset];
}


/*
 *   video RAM 1 read/write handlers
 */

void rastan_videoram1_w (int offset, int data)
{
	if (videoram1[offset] != data)
	{
		dirty1[offset/4] = 1;
		videoram1[offset] = data;
	}
}

int rastan_videoram1_r (int offset)
{
   return videoram1[offset];
}


/*
 *   video RAM 3 read/write handlers
 */

void rastan_videoram3_w(int offset,int data)
{
	if (videoram3[offset] != data)
	{
		dirty3[offset/4] = 1;
		videoram3[offset] = data;
	}
}

int rastan_videoram3_r(int offset)
{
	return videoram3[offset];
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void rastan_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,pom;
	int scrollx,scrolly;
	int i,col;

	for (pom = 0; pom < 0x80; pom++)
	{
		if (pal_dirty[pom])
		{
			Machine->gfx[0]->colortable[pom*16] = Machine->pens[0];
			for (i = 1; i < 16; i++)
			{
				col  = (paletteram[pom*32+i*2+1] >> 2) & 0x07;   /* red component */
				col |= (paletteram[pom*32+i*2+1] >> 4) & 0x08;   /* green component LSB */
				col |= (paletteram[pom*32+i*2] << 4) & 0x30;   /* green component MSB */
				col |= (paletteram[pom*32+i*2] << 1) & 0xc0;   /* blue component */
				if (col == 0)
					col = 1;
				Machine->gfx[0]->colortable[pom*16+i] = Machine->gfx[8]->colortable[col];
			}
		}
	}

	for (pom = rastan_videoram1_size - 4; pom >= 0; pom -= 4)
	{
		offs = pom / 4;
		if (dirty1[offs] || pal_dirty[videoram1[pom+1] & 0x7f])
		{
			int sx,sy;
			int num,bank;

			num = ((videoram1[pom+2] << 8) + videoram1[pom+3]) & 0x1fff;
			bank = (videoram1[pom+2] & 0x20) >> 5;

			dirty1[offs] = 0;

			sx = 8 * (offs % 64);
			sy = 8 * (offs / 64);

			drawgfx(tmpbitmap2, Machine->gfx[bank],
					num,
					videoram1[pom+1] & 0x7f,
					videoram1[pom] & 0x40, videoram1[pom] & 0x80,
					sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	for (pom = rastan_videoram3_size - 4; pom >= 0; pom -= 4)
	{
		offs = pom / 4;
		if (dirty3[offs] || pal_dirty[videoram3[pom+1] & 0x7f])
		{
			int sx,sy;
			int num,bank;

			num = ((videoram3[pom+2] << 8) + videoram3[pom+3]) & 0x1fff;
			bank= (videoram3[pom+2] & 0x20) >> 5;

			dirty3[offs] = 0;

			sx = 8 * (offs % 64);
			sy = 8 * (offs / 64);

			drawgfx(tmpbitmap3, Machine->gfx[bank],
					num,
					videoram3[pom+1]&0x7f,
					videoram3[pom]&0x40,videoram3[pom]&0x80,
					sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}

	memset (pal_dirty, 0, sizeof (pal_dirty));

	/* copy the character mapped graphics */
	scrollx = (scrollX[0]<<8) + scrollX[1] - 16;
	scrolly = (scrollY[0]<<8) + scrollY[1] - 8;
	copyscrollbitmap(bitmap,tmpbitmap2, 1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	scrollx = (scrollX[2]<<8) + scrollX[3] - 16;
	scrolly = (scrollY[2]<<8) + scrollY[3] - 8;
	copyscrollbitmap(bitmap,tmpbitmap3,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);


	/* Draw the sprites. 256 sprites in total */
	for (offs = 0x800-8; offs >= 0; offs -= 8)
	{
		int sx,sy,col;
		int num,bank;

		sx = (spriteram[offs+6]<<8) | spriteram[offs+7];
		sy = (spriteram[offs+2]<<8) | spriteram[offs+3];

		sx = (sx < 32768) ? sx & 0x1ff : -((65536-sx) & 0x1ff);
		sy = (sy < 32768) ? sy & 0x1ff : -((65536-sy) & 0x1ff);

		num = (spriteram[offs+4] << 8) | spriteram[offs+5];
		if (num && sx < 320 && sy < 240)
		{
			num &= 0x3ff;
			bank = (spriteram[offs+4] >> 2) & 0x03;
			col = spriteram[offs+1];
			if (col < 0x40)
				col= col | 0x30;
			if (offs/8 >= 0xbe) /*found experimentally*/
				col= ((col & 0x3f) | 0x30);
			drawgfx(bitmap,Machine->gfx[4+bank],
				num,
				col ,
				spriteram[offs]&0x40,spriteram[offs]&0x80,
				sx,sy-8,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}


#if 0

static int getb(int i)
{
	return 2*((paletteram[i]>>2)&0x1f);
}
static int getg(int i)
{
	return 2*( ((paletteram[i]&0x03)<<3)|((paletteram[i+1]>>5)&0x07) );
}
static int getr(int i)
{
	return 2*(paletteram[i+1]&0x1f);
}

void DebugOutput (void)
{
	tab=fopen("vidram1","wa");
	for (i=0; i<0x4000; i+=4)
	{
	  for (pom=0; pom<4; pom++)
	    fprintf(tab,"%02x ",videoram1[i+pom]);
	  fprintf(tab,"\n");
	}
	fclose(tab);

	tab=fopen("vidram3","wa");
	for (i=0; i<0x4000; i+=4)
	{
	  for (pom=0; pom<4; pom++)
	    fprintf(tab,"%02x ",videoram3[i+pom]);
	  fprintf(tab,"\n");
	}
	fclose(tab);





	tab=fopen("vidram2","wa");
	for (i=0; i<0x4000; i+=4)
	{
	  for (pom=0; pom<4; pom++)
	    fprintf(tab,"%02x ",videoram2[i+pom]);
	  fprintf(tab,"\n");
	}
	fclose(tab);
	tab=fopen("vidram4","wa");
	for (i=0; i<0x4000; i+=4)
	{
	  for (pom=0; pom<4; pom++)
	    fprintf(tab,"%02x ",videoram4[i+pom]);
	  fprintf(tab,"\n");
	}
	fclose(tab);



	tab=fopen("paltab","wa");
	pom=0;
	for (i=0; i<0x10000; i++)
	{
	  if (paltab[i])
	  {
	    fprintf(tab,"%04x ",i);
	    fprintf(tab,"\n");
	    pom++;
	  }
	}
	  fprintf(tab,"\ntotal colors %04x\n",pom);
	fclose(tab);


	tab=fopen("sprram","wa");
	for (i=0; i<0x800; i+=8)
	{
	  fprintf(tab,"%04x  ",i/8);
	  for (pom=0; pom<8; pom+=2)
	    fprintf(tab,"%02x%02x ",spriteram[i+pom],spriteram[i+pom+1]);
	  fprintf(tab,"\n");
	}
	fclose(tab);



	tab=fopen("p0-200","wa");
	fprintf(tab,"NeoPaint Palette File\n(C)1992-95 NeoSoft Corp.\n256\n");
	for (i=0x000; i<0x200; i+=2)
	{
	//  fprintf(tab,"%04x %04x ",i, i/32);
	//  fprintf(tab,"%02x%02x ",paletteram[i],paletteram[i+1]);
	  fprintf(tab,"%3i %3i %3i",getr(i),getg(i),getb(i) );
	  fprintf(tab,"\n");
	}
	fclose(tab);
	tab=fopen("p200-400","wa");
	fprintf(tab,"NeoPaint Palette File\n(C)1992-95 NeoSoft Corp.\n256\n");
	for (i=0x200; i<0x400; i+=2)
	{
	//  fprintf(tab,"%04x %04x ",i, i/32);
	//  fprintf(tab,"%02x%02x ",paletteram[i],paletteram[i+1]);
	  fprintf(tab,"%3i %3i %3i",getr(i),getg(i),getb(i) );
	  fprintf(tab,"\n");
	}
	fclose(tab);

	tab=fopen("p400-600","wa");
	fprintf(tab,"NeoPaint Palette File\n(C)1992-95 NeoSoft Corp.\n256\n");
	for (i=0x400; i<0x600; i+=2)
	{
	//  fprintf(tab,"%04x %04x ",i, i/32);
	//  fprintf(tab,"%02x%02x ",paletteram[i],paletteram[i+1]);
	  fprintf(tab,"%3i %3i %3i",getr(i),getg(i),getb(i) );
	  fprintf(tab,"\n");
	}
	fclose(tab);
	tab=fopen("p600-800","wa");
	fprintf(tab,"NeoPaint Palette File\n(C)1992-95 NeoSoft Corp.\n256\n");
	for (i=0x600; i<0x800; i+=2)
	{
	//  fprintf(tab,"%04x %04x ",i, i/32);
	//  fprintf(tab,"%02x%02x ",paletteram[i],paletteram[i+1]);
	  fprintf(tab,"%3i %3i %3i",getr(i),getg(i),getb(i) );
	  fprintf(tab,"\n");
	}
	fclose(tab);

	tab=fopen("p800-a00","wa");
	fprintf(tab,"NeoPaint Palette File\n(C)1992-95 NeoSoft Corp.\n256\n");
	for (i=0x800; i<0xa00; i+=2)
	{
	//  fprintf(tab,"%04x %04x ",i, i/32);
	//  fprintf(tab,"%02x%02x ",paletteram[i],paletteram[i+1]);
	  fprintf(tab,"%3i %3i %3i",getr(i),getg(i),getb(i) );
	  fprintf(tab,"\n");
	}
	fclose(tab);

	tab=fopen("pa-2000","wa");
	fprintf(tab,"NeoPaint Palette File\n(C)1992-95 NeoSoft Corp.\n256\n");
	for (i=0xa00; i<0x2000; i+=2)
	{
	  fprintf(tab,"%3i %3i %3i",getr(i),getg(i),getb(i) );
	  fprintf(tab,"\n");
	}
	fclose(tab);
}
#endif
