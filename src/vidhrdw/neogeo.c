/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.




Mish:

0x00000 - ?			: Blocks of sprite data, each 0x80 bytes:
	Each 0x80 block is made up of 0x20 double words, their format is:
	Word: Sprite number (16 bits)
    Byte: Palette number (8 bits)
    Byte: Bit 0: X flip
          Bit 1: Y flip
          Bit x: Rest unknown for now.

    Each double word sprite is drawn directly underneath the previous one,
    based on the starting coordinates.

0x0e000 - 0x0ea00	: Front plane tiles, 2 bytes each

0x10000: Control for sprites banks, arranged in words

	Bit 0 to 3 - Unknown
    Bit 4 to 7 - Y zoom, 0xf is full size (no scale).
    Bit 8 to 11 - X zoom, 0xf is full size (no scale).
    Bit 12 to 15 - Unknown

0x10400: Control for sprite banks, arranged in words

	Bit 0 to 5: Number of sprites in this bank.
	Bit 6 - If set, this bank is placed to right of previous bank (same Y-coord).
	Bit 7 to 15 - Y position for sprite bank.

0x10800: Control for sprite banks, arranged in words
	Bit 0 to 5: Unknown
	Bit 7 to 15 - X position for sprite bank.

***************************************************************************/

#include "driver.h"
#include "common.h"
#include "usrintrf.h"
#include "vidhrdw/generic.h"

unsigned char *vidram;
unsigned char *neogeo_paletteram;       /* pointer to 1 of the 2 palette banks */
unsigned char *pal_bank1;		/* 0x100*16 2 byte palette entries */
unsigned char *pal_bank2;		/* 0x100*16 2 byte palette entries */
int palno = 0;				/* start off in palettebank 0 */

static int can_bank_sprites;

int neo_unknown[32]; //debug
void neo_unknown1(int offset, int data) {WRITE_WORD(&neo_unknown[0],data);}
void neo_unknown2(int offset, int data) {WRITE_WORD(&neo_unknown[2],data);}
void neo_unknown3(int offset, int data) {WRITE_WORD(&neo_unknown[4],data);}
void neo_unknown4(int offset, int data) {WRITE_WORD(&neo_unknown[6],data);}

void setpalbank0(int offset,int data) {

int i;

	if (palno != 0) {
		palno = 0;
		neogeo_paletteram = pal_bank1;

		for (i=0; i<0x2000; i+=2) {
        	int newword = READ_WORD(&neogeo_paletteram[i]);

			int red=((newword>>8)&0x0f);
			int green=((newword>>4)&0x0f);
			int blue=((newword>>0)&0x0f);

            red			= red*0x11;
			green		= green*0x11;
			blue		= blue*0x11;

			palette_change_color(i / 2,red,green,blue);

        }



        if (errorlog) fprintf(errorlog,"Changed to palette bank 0\n");

	}
}

void setpalbank1(int offset,int data) {
int i;

	if (palno != 1) {
		palno = 1;
		neogeo_paletteram = pal_bank2;

        for (i=0; i<0x2000; i+=2) {
        	int newword = READ_WORD(&neogeo_paletteram[i]);

			int red=((newword>>8)&0x0f);
			int green=((newword>>4)&0x0f);
			int blue=((newword>>0)&0x0f);

            red			= red*0x11;
			green		= green*0x11;
			blue		= blue*0x11;

			palette_change_color(i / 2,red,green,blue);

        }

        if (errorlog) fprintf(errorlog,"Changed to palette bank 1\n");


	}
}

int neogeo_paletteram_r(int offset) {
	return READ_WORD(&neogeo_paletteram[offset]);
}

void neogeo_paletteram_w(int offset,int data)
{
	int oldword = READ_WORD (&neogeo_paletteram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	int red=((newword>>8)&0x0f);
	int green=((newword>>4)&0x0f);
	int blue=((newword>>0)&0x0f);


	WRITE_WORD (&neogeo_paletteram[offset], newword);
	red			= red*0x11;
	green		= green*0x11;
	blue		= blue*0x11;

	palette_change_color(offset / 2,red,green,blue);
}

void neogeo_vh_stop(void) {

	if (pal_bank1) {
		free (pal_bank1);
		pal_bank1 = 0;
	}

	if (pal_bank2) {
		free (pal_bank2);
		pal_bank2 = 0;
	}

	if (vidram) {
		free (vidram);
		vidram = 0;
	}

}

int neogeo_vh_start(void) {

	pal_bank1 = malloc(0x2000);
	if (!pal_bank1) {
		neogeo_vh_stop();
		return 1;
	}

	pal_bank2 = malloc(0x2000);
	if (!pal_bank2) {
		neogeo_vh_stop();
		return 1;
	}

	vidram = malloc(0x10C00);
	if (!vidram) {
		neogeo_vh_stop();
		return 1;
	}
	memset(vidram,0,0x10C00);

	neogeo_paletteram = pal_bank1;

    /* Unconfirmed, but works with SNK intro screen */
	palette_transparent_color = 4095;

	/* The sprite number will be masked to the total */
	if (Machine->drv->gfxdecodeinfo[1].gfxlayout->total >= 0x10000)
		can_bank_sprites = 1;
	else
		can_bank_sprites = 0;

	return 0;
}

/* LBO */
#ifdef LSB_FIRST
#define intelLong(x) (x)
#define BL0 0
#define BL1 1
#define BL2 2
#define BL3 3
#define WL0 0
#define WL1 1
#else
#define intelLong(x) (((x << 24) | (((unsigned long) x) >> 24) | (( x & 0x0000ff00) << 8) | (( x & 0x00ff0000) >> 8)))
#define BL0 3
#define BL1 2
#define BL2 1
#define BL3 0
#define WL0 1
#define WL1 0
#endif

#define TA

#ifdef ACORN /* GSL 980108 read/write nonaligned dword routine for ARM processor etc */

INLINE int read_dword(int *address)
{
	if ((int)address & 3)
	{
#ifdef LSB_FIRST  /* little endian version */
  		return (    *((unsigned char *)address) +
  			   (*((unsigned char *)address+1) << 8)  +
  		   	   (*((unsigned char *)address+2) << 16) +
  		           (*((unsigned char *)address+3) << 24) );
#else             /* big endian version */
  		return (    *((unsigned char *)address+3) +
  			   (*((unsigned char *)address+2) << 8)  +
  		   	   (*((unsigned char *)address+1) << 16) +
  		           (*((unsigned char *)address)   << 24) );
#endif
	}
	else
		return *(int *)address;
}

INLINE void write_dword(int *address, int data)
{
  	if ((int)address & 3)
	{
#ifdef LSB_FIRST
    		*((unsigned char *)address) =    data;
    		*((unsigned char *)address+1) = (data >> 8);
    		*((unsigned char *)address+2) = (data >> 16);
    		*((unsigned char *)address+3) = (data >> 24);
#else
    		*((unsigned char *)address+3) =  data;
    		*((unsigned char *)address+2) = (data >> 8);
    		*((unsigned char *)address+1) = (data >> 16);
    		*((unsigned char *)address)   = (data >> 24);
#endif
		return;
  	}
  	else
		*(int *)address = data;

}
#else
#define read_dword(address) *(int *)address
#define write_dword(address,data) *(int *)address=data
#endif


static void Neodrawgfx(struct osd_bitmap *dest,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,int zx,int zy,
		const struct rectangle *clip)
{
	int ox,oy,ex,ey,y,start,dy;
	const unsigned char *sd;
	unsigned char *bm,*bme;
	int col;
	int *sd4;
	int trans4,col4;
	struct rectangle myclip;

	int transparency = TRANSPARENCY_PEN;
	int transparent_color = 0;

	if (!gfx) return;

	code %= gfx->total_elements;
	color %= gfx->total_colors;

	if (gfx->pen_usage)
	{
		int transmask;

		transmask = 1 << transparent_color;

		if ((gfx->pen_usage[code] & ~transmask) == 0)
			/* character is totally transparent, no need to draw */
			return;
		else if ((gfx->pen_usage[code] & transmask) == 0)
			/* character is totally opaque, can disable transparency */
			transparency = TRANSPARENCY_NONE;
	}

	/* check bounds */
	ox = sx;
	oy = sy;
	ex = sx + gfx->width -1;   //************
	if (sx < 0) sx = 0;
	if (clip && sx < clip->min_x) sx = clip->min_x;
	if (ex >= dest->width) ex = dest->width-1;
	if (clip && ex > clip->max_x) ex = clip->max_x;
	if (sx > ex) return;

	ey = sy + gfx->height -1; /* Changing gfx->h for zy will skip TOP lines, not what we want */
	if (sy < 0) sy = 0;
	if (clip && sy < clip->min_y) sy = clip->min_y;
	if (ey >= dest->height) ey = dest->height-1;
	if (clip && ey > clip->max_y) ey = clip->max_y;
	if (sy > ey) return;

	osd_mark_dirty (sx,sy,ex,ey,0);	/* ASG 971011 */

	/* start = code * zy *gfx->height*; */
	if (flipy)	/* Y flop */
	{                       /* Mish - NOT zx */
		start = code * gfx->height + gfx->height-1 - (16-zy)   - (sy-oy);
		dy = -1;
	}
	else		/* normal */
	{
		start = code * gfx->height + (16-zy) + (sy-oy);
		dy = 1;
	}


	{
		const unsigned short *paldata;	/* ASG 980209 */

		paldata = &gfx->colortable[gfx->color_granularity * color];

		switch (transparency)
		{
			case TRANSPARENCY_NONE:
				if (flipx)	/* X flip */
				{
					for (y = sy;y <= ey;y++)
					{
						bm  = dest->line[y];
						bme = bm + ex;
						sd = gfx->gfxdata->line[start] + zx /*gfx->width*/ -1 - (sx-ox);
						for( bm += sx ; bm <= bme-7 ; bm+=8 )
						{
							sd-=8;
							bm[0] = paldata[sd[8]];
							bm[1] = paldata[sd[7]];
							bm[2] = paldata[sd[6]];
							bm[3] = paldata[sd[5]];
							bm[4] = paldata[sd[4]];
							bm[5] = paldata[sd[3]];
							bm[6] = paldata[sd[2]];
							bm[7] = paldata[sd[1]];
						}
						for( ; bm <= bme ; bm++ )
							*bm = paldata[*(sd--)];
						start+=dy;
					}
				}
				else		/* normal */
				{
					for (y = sy;y <= ey;y++)
					{
						bm  = dest->line[y];
						bme = bm + ex;
						sd = gfx->gfxdata->line[start] + (sx-ox);
						for( bm += sx ; bm <= bme-7 ; bm+=8 )
						{
							bm[0] = paldata[sd[0]];
							bm[1] = paldata[sd[1]];
							bm[2] = paldata[sd[2]];
							bm[3] = paldata[sd[3]];
							bm[4] = paldata[sd[4]];
							bm[5] = paldata[sd[5]];
							bm[6] = paldata[sd[6]];
							bm[7] = paldata[sd[7]];
							sd+=8;
						}
						for( ; bm <= bme ; bm++ )
							*bm = paldata[*(sd++)];
						start+=dy;
					}
				}
				break;

			case TRANSPARENCY_PEN:
				trans4 = transparent_color * 0x01010101;
				if (flipx)	/* X flip */
				{
					for (y = sy + (16-zy)  ;y <= ey;y++)
					{
						bm  = dest->line[y];
						bme = bm + ex;
						sd4 = (int *)(gfx->gfxdata->line[start] + zx /*gfx->width*/ -1 - (sx-ox) -3);
						for( bm += sx ; bm <= bme-3 ; bm+=4 )
						{
							if ((col4=read_dword(sd4)) != trans4){
								col = (col4>>24)&0xff;
								if (col != transparent_color) bm[BL0] = paldata[col];
								col = (col4>>16)&0xff;
								if (col != transparent_color) bm[BL1] = paldata[col];
								col = (col4>>8)&0xff;
								if (col != transparent_color) bm[BL2] = paldata[col];
								col = col4&0xff;
								if (col != transparent_color) bm[BL3] = paldata[col];
							}
							sd4--;
						}
						sd = (unsigned char *)sd4+3;
						for( ; bm <= bme ; bm++ )
						{
							col = *(sd--);
							if (col != transparent_color) *bm = paldata[col];
						}
						start+=dy;
					}
				}
				else		/* normal */
				{
					for (y = sy + (16-zy);y <= ey;y++)
					{
						bm  = dest->line[y];
						bme = bm + ex;
						sd4 = (int *)(gfx->gfxdata->line[start] + (sx-ox));
						for( bm += sx ; bm <= bme-3 ; bm+=4 )
						{
							if ((col4=read_dword(sd4)) != trans4){
								col = col4&0xff;
								if (col != transparent_color) bm[BL0] = paldata[col];
								col = (col4>>8)&0xff;
								if (col != transparent_color) bm[BL1] = paldata[col];
								col = (col4>>16)&0xff;
								if (col != transparent_color) bm[BL2] = paldata[col];
								col = (col4>>24)&0xff;
								if (col != transparent_color) bm[BL3] = paldata[col];
							}
							sd4++;
						}
						sd = (unsigned char *)sd4;
						for( ; bm <= bme ; bm++ )
						{
							col = *(sd++);
							if (col != transparent_color) *bm = paldata[col];
						}
						start+=dy;
					}
				}
				break;
			}
		}
}


/* debug */
int dotiles = 0;
int screen_offs = 0x0000;
/* end debug */

void neogeo_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int x,y;
	int sx =0;
	int sy =0;
	int oy =0;
	int my =0;
	int omy = 0;
	int zx = 1;
	int zy = 1;
	int offs;
	int pom,i;
	int count;

	/* debug */
	char buf[80];
	struct DisplayText dt[2];

    int color,code,pal_base;
 	int colmask[256];

  	fillbitmap(bitmap,Machine->pens[0],&Machine->drv->visible_area);

	memset(palette_used_colors,PALETTE_COLOR_UNUSED,Machine->drv->total_colors * sizeof(unsigned char));

	/* character foreground */
	pal_base = Machine->drv->gfxdecodeinfo[0].color_codes_start;
	for (color = 0;color < 16;color++) colmask[color] = 0;
	for (y=2;y<30;y++)
	{
		for (x=0;x<40;x++)
		{
			code = (READ_WORD( &vidram[0xE000 + 2*y + x*64] ) & 0xfff);
			color = ((READ_WORD( &vidram[0xE000 + 2*y + x*64] ) & 0xf000) >> 12);

			colmask[color] |= Machine->gfx[0]->pen_usage[code];
		}
	}
	for (color = 0;color < 16;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

    /* Tiles */
    pal_base = Machine->drv->gfxdecodeinfo[1].color_codes_start;
	for (color = 0;color < 256;color++) colmask[color] = 0;
	for (count=0;count<384;count++)
	{
		int t1 = READ_WORD( &vidram[0x10400 + (count << 1)] );

		if (! (t1 & 0x40))
			my = (t1 & 0x3f);

		offs = count<<7;

		for (y=0;y < my;y++)
		{
			code  = READ_WORD(&vidram[offs + 4*y]);
			color = (READ_WORD(&vidram[offs + 4*y+2]))>>8;

            colmask[color] |= Machine->gfx[1]->pen_usage[code];

		}
    }

	for (color = 0;color < 256;color++)
	{
		if (colmask[color] & (1 << 0))
			palette_used_colors[pal_base + 16 * color] = PALETTE_COLOR_TRANSPARENT;
		for (i = 1;i < 16;i++)
		{
			if (colmask[color] & (1 << i))
				palette_used_colors[pal_base + 16 * color + i] = PALETTE_COLOR_USED;
		}
	}

	if (palette_recalc())
	{
	/* Nothing to dirty at the moment */
	}

	if (!dotiles) { 					/* debug */

	for (count=0;count<384;count++)
	{
		int t3 = READ_WORD( &vidram[0x10000 + (count << 1)] );
		int t1 = READ_WORD( &vidram[0x10400 + (count << 1)] );
		int t2 = READ_WORD( &vidram[0x10800 + (count << 1)] );

        /* Mish: If this bit is set this new column is placed next to last one */
		if (t1 & 0x40) { /* is it a continue ? */

			sx += (zx + 1);
			if ( sx >= 0x1F0 )
				sx -= 0x200;

			sy = oy;
		} else {	/* nope it is a new block */

        	/* Sprite scaling */
			zx = (t3 >> 8) & 0x0f;
			zy = (t3 >> 4) & 0x0f;

			sx = (t2 >> 7);
			if ( sx >= 0x1F0 )
				sx -= 0x200;

			my = (t1 & 0x3f);

			sy = 0x1F0 - (t1 >> 7);
			oy = sy;
			omy = my;
		}
		offs = count<<7;

        /* my holds the number of tiles in each vertical multisprite block */
		for (y=0;y < my ;y++)
		{
			int tileno  = READ_WORD(&vidram[offs + 4*y]);
			int tileatr = READ_WORD(&vidram[offs + 4*y+2]);

			if (can_bank_sprites)
				if (tileatr & 0x10) tileno += 0x10000;

            /* Zoom - at first this seemed like line skipping on each 16 by 16 block,
            BUT, it looks like the skip should be applyed over the WHOLE tile strip,
            for ZY at least...  (zx to come :) */
            if (zy!=0xf || zx!=0xf)
            {

            	/* Dont draw if zero? */
            	if (zy && zx)
            	{
            		if ((tileatr & 0xfc) && errorlog)
            			fprintf (errorlog, "tileatr: %04x\n", tileatr & 0xff);

#if 0
					Neodrawgfx(bitmap,
						Machine->gfx[1],
						tileno,
						tileatr >> 8,
						tileatr & 0x01,tileatr & 0x02,
						sx,sy,zx+1,zy+1,
						&Machine->drv->visible_area);
#else
					drawgfxzoom(bitmap,
						Machine->gfx[1],
						tileno,
						tileatr >> 8,
						tileatr & 0x01,tileatr & 0x02,
						sx,sy,
						&Machine->drv->visible_area, TRANSPARENCY_PEN, 0, (zx+1) << 12, (zy+1) << 12);
#endif
				}
            }
            else if (y != 0x20)	/* fixes a bug in sidekicks */
			{
           		if ((tileatr & 0xfc) && errorlog)
           			fprintf (errorlog, "tileatr: %04x\n", tileatr & 0xff);

				drawgfx(bitmap,
					Machine->gfx[1],
					tileno,
					tileatr >> 8,
					tileatr & 0x01,tileatr & 0x02,
					sx,sy,
					&Machine->drv->visible_area, TRANSPARENCY_PEN, 0);
			}

			sy += (zy + 1);
			if ( sy > 0x1F0 )
				sy = sy - 0x200;

		}  /* for y */
	}  /* for count */


	/* Character foreground */
 	for (y=2;y<30;y++) {
 		for (x=0;x<40;x++) {
			int byte1 = (READ_WORD( &vidram[0xE000 + 2*y + x*64] ));
			int byte2 = byte1 >> 12;
            byte1 = byte1 & 0xfff;

            /* About 4% speedup, but I haven't checked if it breaks any games */
            if (byte1==0xff || byte1==0x20) continue;

			drawgfx(bitmap,
				Machine->gfx[0],
				byte1,
				byte2,
				0,0,
				x*8,(y-2)*8,
				&Machine->drv->visible_area,
				TRANSPARENCY_PEN,
				0);
  		}
	}

	} else {	/* debug */
		offs = screen_offs;
		for (y=0;y<15;y++) {
			for (x=0;x<20;x++) {

				unsigned char byte1 = vidram[offs + 4*y+x*128];
				unsigned char byte2 = vidram[offs + 4*y+x*128+1];
				unsigned char col = vidram[offs + 4*y+x*128+3];
				unsigned char byte3 = vidram[offs + 4*y+x*128+2];

				drawgfx(bitmap,
					Machine->gfx[1],
					byte1 + (byte2 << 8),
					col,
					byte3 & 0x01,byte3 & 0x02,
					x*16,y*16,
					&Machine->drv->visible_area,
					TRANSPARENCY_NONE,
					0);
			}
		}

		sprintf(buf,"POS : %04X , VDP regs %04X",screen_offs, (screen_offs >> 6) );
		dt[0].text = buf;
		dt[0].color = DT_COLOR_RED;
		dt[0].x = 0;
		dt[0].y = 0;
		dt[1].text = 0;
		displaytext(dt,0);
	}	/* debug */

    /* More debug stuff :)
{

	int i,j;
	char buf[20];
	int trueorientation;
	struct osd_bitmap *bitmap = Machine->scrbitmap;

	trueorientation = Machine->orientation;
	Machine->orientation = ORIENTATION_DEFAULT;


for (i = 0;i < 8;i+=2)
{
	sprintf(buf,"%04X",READ_WORD(&neo_unknown[i]));
	for (j = 0;j < 4;j++)
		drawgfx(bitmap,Machine->uifont,buf[j],DT_COLOR_WHITE,0,0,3*8*i+8*j,8*5,0,TRANSPARENCY_NONE,0);
}

}


*/




}

int modulo=1;
int where=0;

int vidram_data_r (int offset) {
	return (READ_WORD(&vidram[where]));
}

void vidram_data_w(int offset,int data) {
    COMBINE_WORD_MEM ( &vidram[ where ],data);
 //	WRITE_WORD ( &vidram[ where ],data);    /* Do byte writes occur here? */
	where += modulo;
}

/* Modulo can become negative , Puzzle Bobble Super Sidekicks and a lot */
/* of other games use this */
void vidram_modulo_w(int offset, int data) {
	if (data & 0x8000) {
		/* Sign extend it. */
		/* Where is the SEX instruction when you need it :-) */
		modulo = (data - 0x10000) << 1;
	}
	else
		modulo = data << 1;
}

int vidram_modulo_r(int offset) {
	return modulo >> 1;
}

void vidram_offset_w(int offset, int data) {
	where = data*2;
}

int vidram_offset_r(int offset) {
	return (where>>1);
}

int mish_vid_r(int offset)
{
	return READ_WORD(&vidram[offset]);
}

void mish_vid_w(int offset, int data)
{
	COMBINE_WORD_MEM(&vidram[offset],data);
}

