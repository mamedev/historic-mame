/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



//#define VIDEO_RAM_SIZE 0x400
//static const unsigned char *colors;

static struct osd_bitmap *tmpbitmap2;
static struct osd_bitmap *tmpbitmap3;

static unsigned char *dirty1;		/* 16KB .L organization */
/*static unsigned char *dirty2;	* 16KB .W organization */
static unsigned char *dirty3;		/* 16KB .L organization */
/*static unsigned char *dirty4;	* 16KB .L organization */

static unsigned char *r_palram;   /*64KB for palette RAM*/
static unsigned char *r_sprram;   /*64KB for sprite RAM*/
static unsigned char pal_dirty[0x80];	  /*only 0x50 color schemes are in use*/

/*static unsigned char paltab[0x10000];	* rip the palette */

static unsigned char *r_vidram1;   /*16KB for video RAM 8x8   0x1000.L */
static unsigned char *r_vidram2;   /*16KB for video RAM 16x16 0x2000.W */
static unsigned char *r_vidram3;   /*16KB for video RAM 8x8   0x1000.L */
static unsigned char *r_vidram4;   /*16KB for video RAM 16x16 0x2000.W */

static unsigned char *r_ram;

static unsigned char scrollY[4]={0,0,0,0};
static unsigned char scrollX[4]={0,0,0,0};

static struct rectangle visiblearea =
{
  0, 64*8-1,
  0, 64*8-1
};

/***************************************************************************

  Convert the color PROMs into a more useable format.

  RASTAN doesn't have a color PROM. It uses 64KB bytes of RAM to
  dynamically create the palette. Each couple of bytes defines one
  color (5 bits per R,G,B; the highest 1 bit of the first byte is unused).
  Graphics use 4 bitplanes. It seems only first 0x50 colors are in use.
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
		colortable[0x50*16+i] = i;

}


int rastan_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	/* the background area is twice as tall and twice as large as the screen */
	if ((tmpbitmap2 = osd_create_bitmap(512,512)) == 0)
        {
		generic_vh_stop();
		return 1;
	}
	if ((tmpbitmap3 = osd_create_bitmap(512,512)) == 0)
        {
		generic_vh_stop();
		osd_free_bitmap(tmpbitmap2);
		return 1;
	}

	/* Allocate a bunch o' buffers */
	dirty1 = malloc (2 * 0x1000 + 2 * 0x10000 + 5 * 0x4000);
	if (!dirty1)
	{
		generic_vh_stop();
		osd_free_bitmap(tmpbitmap2);
		osd_free_bitmap(tmpbitmap3);
		return 1;
	}
	dirty3 = dirty1 + 0x1000;
	r_palram = dirty3 + 0x1000;
	r_sprram = r_palram + 0x10000;
	r_vidram1 = r_sprram + 0x10000;
	r_vidram2 = r_vidram1 + 0x4000;
	r_vidram3 = r_vidram2 + 0x4000;
	r_vidram4 = r_vidram3 + 0x4000;
	r_ram = r_vidram4 + 0x4000;

	cpu_setbank (1, r_ram);

	return 0;
}


static int getb(int i)
{
	return 2*((r_palram[i]>>2)&0x1f);
}
static int getg(int i)
{
	return 2*( ((r_palram[i]&0x03)<<3)|((r_palram[i+1]>>5)&0x07) );
}
static int getr(int i)
{
	return 2*(r_palram[i+1]&0x1f);
}

void rastan_vh_stop(void){

#if 0

tab=fopen("vidram1","wa");
for (i=0; i<0x4000; i+=4)
{
  for (pom=0; pom<4; pom++)
    fprintf(tab,"%02x ",r_vidram1[i+pom]);
  fprintf(tab,"\n");
}
fclose(tab);

tab=fopen("vidram3","wa");
for (i=0; i<0x4000; i+=4)
{
  for (pom=0; pom<4; pom++)
    fprintf(tab,"%02x ",r_vidram3[i+pom]);
  fprintf(tab,"\n");
}
fclose(tab);





tab=fopen("vidram2","wa");
for (i=0; i<0x4000; i+=4)
{
  for (pom=0; pom<4; pom++)
    fprintf(tab,"%02x ",r_vidram2[i+pom]);
  fprintf(tab,"\n");
}
fclose(tab);
tab=fopen("vidram4","wa");
for (i=0; i<0x4000; i+=4)
{
  for (pom=0; pom<4; pom++)
    fprintf(tab,"%02x ",r_vidram4[i+pom]);
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
    fprintf(tab,"%02x%02x ",r_sprram[i+pom],r_sprram[i+pom+1]);
  fprintf(tab,"\n");
}
fclose(tab);



tab=fopen("p0-200","wa");
fprintf(tab,"NeoPaint Palette File\n(C)1992-95 NeoSoft Corp.\n256\n");
for (i=0x000; i<0x200; i+=2)
{
//  fprintf(tab,"%04x %04x ",i, i/32);
//  fprintf(tab,"%02x%02x ",r_palram[i],r_palram[i+1]);
  fprintf(tab,"%3i %3i %3i",getr(i),getg(i),getb(i) );
  fprintf(tab,"\n");
}
fclose(tab);
tab=fopen("p200-400","wa");
fprintf(tab,"NeoPaint Palette File\n(C)1992-95 NeoSoft Corp.\n256\n");
for (i=0x200; i<0x400; i+=2)
{
//  fprintf(tab,"%04x %04x ",i, i/32);
//  fprintf(tab,"%02x%02x ",r_palram[i],r_palram[i+1]);
  fprintf(tab,"%3i %3i %3i",getr(i),getg(i),getb(i) );
  fprintf(tab,"\n");
}
fclose(tab);

tab=fopen("p400-600","wa");
fprintf(tab,"NeoPaint Palette File\n(C)1992-95 NeoSoft Corp.\n256\n");
for (i=0x400; i<0x600; i+=2)
{
//  fprintf(tab,"%04x %04x ",i, i/32);
//  fprintf(tab,"%02x%02x ",r_palram[i],r_palram[i+1]);
  fprintf(tab,"%3i %3i %3i",getr(i),getg(i),getb(i) );
  fprintf(tab,"\n");
}
fclose(tab);
tab=fopen("p600-800","wa");
fprintf(tab,"NeoPaint Palette File\n(C)1992-95 NeoSoft Corp.\n256\n");
for (i=0x600; i<0x800; i+=2)
{
//  fprintf(tab,"%04x %04x ",i, i/32);
//  fprintf(tab,"%02x%02x ",r_palram[i],r_palram[i+1]);
  fprintf(tab,"%3i %3i %3i",getr(i),getg(i),getb(i) );
  fprintf(tab,"\n");
}
fclose(tab);

tab=fopen("p800-a00","wa");
fprintf(tab,"NeoPaint Palette File\n(C)1992-95 NeoSoft Corp.\n256\n");
for (i=0x800; i<0xa00; i+=2)
{
//  fprintf(tab,"%04x %04x ",i, i/32);
//  fprintf(tab,"%02x%02x ",r_palram[i],r_palram[i+1]);
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

#endif

	/* Free everything */
	free (dirty1);
	osd_free_bitmap(tmpbitmap2);
	osd_free_bitmap(tmpbitmap3);
	generic_vh_stop();
}




void rastan_scrollY_w(int offset,int data)
{
	scrollY[offset]=data;
}

void rastan_scrollX_w(int offset,int data)
{
	scrollX[offset]=data;
}

void rastan_paletteram_w(int offset,int data)
{
	r_palram[offset]=data;
	if ((offset/32) < 0x80)
		pal_dirty[offset/32] = 1;
}

int rastan_paletteram_r(int offset)
{
	return r_palram[offset];
}


void rastan_spriteram_w(int offset,int data)
{
	r_sprram[offset]=data;
}

int rastan_spriteram_r(int offset)
{
	return r_sprram[offset];
}

void rastan_videoram1_w(int offset,int data)
{
	if (r_vidram1[offset] != data)
	{
		dirty1[offset >> 2] = 1;
		r_vidram1[offset]=data;
	}
}

int rastan_videoram1_r(int offset)
{
   return r_vidram1[offset];
}


void rastan_videoram2_w(int offset,int data)
{
	r_vidram2[offset]=data;
	if (errorlog)
		if (data>0)
			fprintf(errorlog,"Writing data>0 %02x to vidram2 !!!\n",data);
}

int rastan_videoram2_r(int offset)
{
	return r_vidram2[offset];
}

void rastan_videoram3_w(int offset,int data)
{
	if (r_vidram3[offset] != data)
	{
		dirty3[offset >> 2] = 1;
		r_vidram3[offset]=data;
	}
}

int rastan_videoram3_r(int offset)
{
	return r_vidram3[offset];
}

void rastan_videoram4_w(int offset,int data)
{
	r_vidram4[offset]=data;
	if (errorlog)
		if (data>0)
			fprintf(errorlog,"Writing data>0 %02x to vidram4 !!!\n",data);
}

int rastan_videoram4_r(int offset)
{
	return r_vidram4[offset];
}



int rastan_interrupt(void)
{
//static int inter=0;
//	inter = (inter+1)&0x01;
//	if (inter)
		return 5;  /*Interrupt vector 5*/
//	else
//		return 4;
}



void rastan_machine_init(void)
{

//	Machine->memory_region[0][0x3a204]=0x60;  /* Rastan infinite lives */
//	Machine->memory_region[0][0x517ec]=0x60;  /* Rastan infinite energy */
//	Machine->memory_region[0][0x527b2]=0x60;  /* Rastan infinite energy */

}

int rastan_input_r (int offset)
{
	switch (offset)
	{
		case 1:
		case 3:
			return input_port_0_r (offset);
		case 5:
			return input_port_1_r (offset);
		case 7:
			return input_port_2_r (offset);
		case 9:
			return input_port_3_r (offset);
		case 11:
			return input_port_4_r (offset);
		default:
			return 0;
	}
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

	for (pom=0; pom<0x50; pom++){
		if (pal_dirty[pom])
		{
			Machine->gfx[0]->colortable[pom*16] = Machine->pens[0];
			for (i = 1;i <16 ;i++) {
				col = (r_palram[pom*32+i*2+1] >> 2) & 0x07;   /* red component */
				col |= (r_palram[pom*32+i*2+1] >> 4) & 0x08;   /* green component LSB */
				col |= (r_palram[pom*32+i*2] << 4) & 0x30;   /* green component MSB */
				col |= (r_palram[pom*32+i*2] << 1) & 0xc0;   /* blue component */
				if (col==0)
					col=1;
				Machine->gfx[0]->colortable[pom*16+i] =
					Machine->gfx[8]->colortable[col];
			}
		}
	}

	for (pom = 0x0;pom < 0x4000;pom+=4)
	{
		offs=pom>>2;
		if ( dirty1[offs] || pal_dirty[ r_vidram1[pom+1] & 0x7f ] )

		{
			int sx,sy;
			int num,bank;

			num = ((r_vidram1[pom+2]<<8)+r_vidram1[pom+3]) & 0x1fff;
			bank= (r_vidram1[pom+2] & 0x20)>>5;

			dirty1[offs] = 0;

			sx = 8 * (offs % 64);
			sy = 8 * (offs / 64);

			drawgfx(tmpbitmap2, Machine->gfx[bank]   ,
					num,
					r_vidram1[pom+1]&0x7f,
					r_vidram1[pom]&0x40,r_vidram1[pom]&0x80,
					sx,sy,
					&visiblearea,TRANSPARENCY_NONE,0);

		}
	}

	for (pom = 0x0;pom < 0x4000;pom+=4)
	{
		offs=pom>>2;
		if ( dirty3[offs] || pal_dirty[ r_vidram3[pom+1] & 0x7f ] )
		{
			int sx,sy;
			int num,bank;

			num = ((r_vidram3[pom+2]<<8)+r_vidram3[pom+3]) & 0x1fff;
			bank= (r_vidram3[pom+2] & 0x20)>>5;

			dirty3[offs] = 0;

			sx = 8 * (offs % 64);
			sy = 8 * (offs / 64);

			drawgfx(tmpbitmap3, Machine->gfx[bank]   ,
					num,
					r_vidram3[pom+1]&0x7f,
					r_vidram3[pom]&0x40,r_vidram3[pom]&0x80,
					sx,sy,
					&visiblearea,TRANSPARENCY_NONE,0);

		}
	}

	for (pom=0; pom<0x80; pom++)
		pal_dirty[pom]=0;

	/* copy the character mapped graphics */

	scrollx = (scrollX[0]<<8) + scrollX[1] - 16;
	scrolly = (scrollY[0]<<8) + scrollY[1] - 8;
	copyscrollbitmap(bitmap,tmpbitmap2, 1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	scrollx = (scrollX[2]<<8) + scrollX[3] - 16;
	scrolly = (scrollY[2]<<8) + scrollY[3] - 8;
	copyscrollbitmap(bitmap,tmpbitmap3,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);


	/* Draw the sprites. 256 sprites in total */
	for (offs = 0x800-8;offs >=0;offs -= 8)
	{
		int sx,sy,col;
		int num,bank;

		sx = (r_sprram[offs+6]<<8) | r_sprram[offs+7];
		sy = (r_sprram[offs+2]<<8) | r_sprram[offs+3];

		sx = sx<32768 ? sx&0x1ff : -( (65536-sx) & 0x1ff );
		sy = sy<32768 ? sy&0x1ff : -( (65536-sy) & 0x1ff );

		num=(r_sprram[offs+4]<<8) | r_sprram[offs+5];
		if  ( num && (sx<320) && (sy<240) )
		{
			num &= 0x3ff;
			bank =(r_sprram[offs+4]>>2)&0x03;
			col = r_sprram[offs+1];
			if (col<0x40)
				col= col|0x30;
			if (offs/8>=0xbe) /*found experimentally*/
				col= ((col&0x3f)|0x30);
			drawgfx(bitmap,Machine->gfx[4+bank],
				num,
				col ,
				r_sprram[offs]&0x40,0,
				sx,sy-8,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
