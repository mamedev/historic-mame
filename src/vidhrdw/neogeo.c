/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

Important!  There are two types of NeoGeo romdump - MVS & MGD2.  Thus, we have
two seperate draw functions for these, plus two seperate vh_start routines.



Mish:

0x00000 - ?			: Blocks of sprite data, each 0x80 bytes:
	Each 0x80 block is made up of 0x20 double words, their format is:
	Word: Sprite number (16 bits)
    Byte: Palette number (8 bits)
    Byte: Bit 0: X flip
          Bit 1: Y flip
          Bit 4: MSB of sprite number (onfirmed, Karnov_r, Mslug). See note.


          Bit x: Rest unknown for now

    Each double word sprite is drawn directly underneath the previous one,
    based on the starting coordinates.

0x0e000 - 0x0ea00	: Front plane tiles, 2 bytes each

0x10000: Control for sprites banks, arranged in words

	Bit 0 to 3 - Y zoom LSB
    Bit 4 to 7 - Y zoom MSB (ie, 1 byte for Y zoom).
    Bit 8 to 11 - X zoom, 0xf is full size (no scale).
    Bit 12 to 15 - Unknown, probably unused

0x10400: Control for sprite banks, arranged in words

	Bit 0 to 5: Number of sprites in this bank (see note below).
	Bit 6 - If set, this bank is placed to right of previous bank (same Y-coord).
	Bit 7 to 15 - Y position for sprite bank.

0x10800: Control for sprite banks, arranged in words
	Bit 0 to 5: Unknown
	Bit 7 to 15 - X position for sprite bank.

Notes:
* If Y zoom is less than 0xc0 then 1 must be added to the number of sprites
in the bank.
* If rom set has less than 0x10000 tiles then msb of tile must be ignored
(see Magician Lord).


***************************************************************************/

#include "driver.h"
#include "common.h"
#include "usrintrf.h"
#include "vidhrdw/generic.h"

#define NEO_DEBUG

static unsigned char *vidram;
static unsigned char *neogeo_paletteram;       /* pointer to 1 of the 2 palette banks */
static unsigned char *pal_bank1;		/* 0x100*16 2 byte palette entries */
static unsigned char *pal_bank2;		/* 0x100*16 2 byte palette entries */
static int palno,modulo,where,high_tile;

static int no_of_tiles;
static long int mgd2_plane;
static int fix_bank;

extern unsigned char *neogeo_sram;

void (*NeoDrawFunc)(struct osd_bitmap *dest,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
        int zx,int zy);

/* Table of lines to skip when drawing with Y zoom, all lines should add to 16 */
static int y_skip[][16]={
{1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
{8,8,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
{5,5,6,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
{4,4,4,4, 0,0,0,0, 0,0,0,0, 0,0,0,0},

{3,3,3,4, 3,0,0,0, 0,0,0,0, 0,0,0,0},
{3,2,3,3, 3,2,0,0, 0,0,0,0, 0,0,0,0},
{2,3,2,2, 3,2,2,0, 0,0,0,0, 0,0,0,0},
{2,2,2,2, 2,2,2,2, 0,0,0,0, 0,0,0,0}, /* 8 lines of 2 - so every 2nd line is skipped */

{2,2,1,2, 2,2,1,2, 2,0,0,0, 0,0,0,0},
{2,2,1,2, 1,2,2,1, 2,1,0,0, 0,0,0,0},
{1,2,1,2, 1,2,1,1, 2,1,2,0, 0,0,0,0},
{1,2,1,1, 1,2,1,1, 2,1,1,2, 0,0,0,0},

{1,1,1,2, 1,1,1,2, 1,1,2,1, 1,0,0,0},
{1,1,1,1, 2,1,1,1, 1,2,1,1, 1,1,0,0},
{1,1,1,1, 1,1,1,2, 1,1,1,1, 1,1,1,0}, /* For zoom value of 0xe */
{1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1}, /* No zoom - every line is drawn */
{1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1}  /* Extra, extra */
};

/* True/False table of lines to draw when skipping in X direction */
static int x_skip[][16]={
{0,0,0,0, 0,0,0,1, 0,0,0,0, 0,0,0,0},
{0,0,0,0, 0,1,0,0, 0,0,0,1, 0,0,0,0},
{0,0,1,0, 0,0,1,0, 0,0,0,0, 1,0,0,0},
{0,1,0,0, 0,0,1,0, 0,0,1,0, 0,0,1,0},
{0,1,0,0, 0,1,0,1, 0,0,1,0, 0,0,1,0}, /* 4 lines */
{0,1,0,1, 0,0,1,0, 0,1,0,1, 0,0,0,1},
{1,0,0,1, 0,1,0,1, 0,1,0,1, 0,1,0,0},
{1,0,1,0, 1,0,1,0, 1,0,1,0, 1,0,1,0},
{1,0,1,0, 1,1,1,0, 1,0,1,0, 1,0,0,1}, /* 8 */
{1,0,1,0, 1,1,1,0, 1,1,1,0, 1,0,1,0},
{0,1,1,1, 1,1,0,1, 1,0,1,0, 1,1,1,0},
{1,0,1,1, 1,0,1,1, 1,0,1,1, 1,0,1,1},
{1,1,0,1, 1,1,1,1, 0,1,1,1, 1,0,1,1},
{1,1,1,0, 1,1,1,1, 1,1,1,0, 1,1,1,1},
{1,1,1,1, 1,1,1,1, 0,1,1,1, 1,1,1,1},
{1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1}, /* Full size */
{1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1}  /* Extra, extra */
};

#ifdef NEO_DEBUG
int neo_unknown[32];
void neo_unknown1(int offset, int data) {WRITE_WORD(&neo_unknown[0],data);}
void neo_unknown2(int offset, int data) {WRITE_WORD(&neo_unknown[2],data);}
void neo_unknown3(int offset, int data) {WRITE_WORD(&neo_unknown[4],data);}
void neo_unknown4(int offset, int data) {WRITE_WORD(&neo_unknown[6],data);}

int dotiles = 0;
int screen_offs = 0x0000;

#endif

/******************************************************************************/

void setpalbank0(int offset,int data)
{
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

/******************************************************************************/

void neogeo_vh_stop(void)
{
    void *f;

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

    /* Save the SRAM settings */
	f = osd_fopen (Machine->gamedrv->name, 0, OSD_FILETYPE_HIGHSCORE, 1);
	if (f)
	{
		osd_fwrite (f, neogeo_sram, 0x10000);
		osd_fclose (f);
	}

#if 0
{
	/* Debug */
	FILE *fp;

    fp=fopen("neogeo.ram","wb");
    fwrite(neoram,0x10000, 1 ,fp);
    fclose(fp);
}
#endif

}

static int common_vh_start(void)
{
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
	palette_transparent_color = 4095;
    palno=0;
    modulo=1;
	where=0;
    fix_bank=0;

	return 0;
}


/******************************************************************************/

static const unsigned char *neogeo_palette(void)
{
    int color,code,pal_base,y,my=0,x,count,offs,i;
 	int colmask[256];

	memset(palette_used_colors,PALETTE_COLOR_UNUSED,4096 * sizeof(unsigned char));

	/* character foreground */
	pal_base = Machine->drv->gfxdecodeinfo[fix_bank].color_codes_start;
	for (color = 0;color < 16;color++) colmask[color] = 0;
	for (offs=0xe000;offs<0xea00;offs+=2) {
    	code = READ_WORD( &vidram[offs] );
    	color = code >> 12;
        colmask[color] |= Machine->gfx[fix_bank]->pen_usage[code&0xfff];
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
    pal_base = Machine->drv->gfxdecodeinfo[2].color_codes_start;
	for (color = 0;color < 256;color++) colmask[color] = 0;
	for (count=0;count<384;count++) {
		int t1 = READ_WORD( &vidram[0x10400 + (count << 1)] );

		if (! (t1 & 0x40))
			my = (t1 & 0x3f);

		offs = count<<7;

		for (y=0;y < my;y++) {
			code  = READ_WORD(&vidram[offs + 4*y]);
			color = (READ_WORD(&vidram[offs + 4*y+2]));
            if (high_tile && color&0x10) code+=0x10000;
			color=color>>8;

            colmask[color] |= Machine->gfx[2]->pen_usage[code];

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

	return palette_recalc();
}

/******************************************************************************/

int vidram_data_r (int offset) {
	return (READ_WORD(&vidram[where]));
}

void vidram_data_w (int offset,int data)
{
 if (where >= 0x10c00)
 return;
 COMBINE_WORD_MEM (&vidram[where], data);
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

/* Two routines to enable videoram to be read in debugger */
int mish_vid_r(int offset)
{
	return READ_WORD(&vidram[offset]);
}

void mish_vid_w(int offset, int data)
{
	COMBINE_WORD_MEM(&vidram[offset],data);
}

void neo_board_fix(int offset, int data)
{
	fix_bank=1;
}

void neo_game_fix(int offset, int data)
{
	fix_bank=0;
}

/******************************************************************************/

static void NeoMVSDrawGfx(struct osd_bitmap *dest,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,int sx,int sy,
        int zx,int zy)
{
	int ex,ey,y,start,dy;
	unsigned char *bm;
	int col,oy;
    int l,c; /* Line skipping counters */

	unsigned char *PL1 = Machine->memory_region[2];
    unsigned char *PL2 = Machine->memory_region[3];
    unsigned char *ptr,p1,p2,p3,p4;

	#define GET_PIX(shift) ((p1>>shift)&1) + (((p2>>shift)&1)<<1) + (((p3>>shift)&1)<<2) + (((p4>>shift)&1)<<3)
	#define DRAW_PIX_X(shift) \
    	if (x_skip[zx][c]) {col = ((p1>>shift)&1) + (((p2>>shift)&1)<<1) + (((p3>>shift)&1)<<2) + (((p4>>shift)&1)<<3); \
		if (col) *bm = paldata[col];	\
			bm++;						\
		}								\
		c++;		  					\

    /* Safety feature */
    code=code%no_of_tiles;

    /* Check for total transparency, no need to draw */
    if ((gfx->pen_usage[code] & ~1) == 0)
    	return;

	/* Check bounds */
	ex = sx + zx -1;   /* Clip for size of zoomed object */
	if (sx < -16) return; /* Should be ok as there is margin around visible area */
	if (ex >= dest->width) ex = dest->width-1;
	if (sx > ex) return;

    oy=sy;
  	ey = sy + zy -1; 	/* Clip for size of zoomed object */
	if (sy < 0) sy = 0;
	if (ey >= dest->height) ey = dest->height-1;
	if (sy > ey) return;

	if (flipy)	/* Y flip */
	{
	   	start = ((code+1) * 64) - 34; /* Needs oy as below */
		dy = 0;
	}
	else		/* normal */
	{
        start = (code * 64) + ((sy-oy)*2);
		dy = 1;
	}
	PL1+=start;
	PL2+=start;

	{
    const unsigned short *paldata;	/* ASG 980209 */
	paldata = &gfx->colortable[gfx->color_granularity * color];
	if (flipx)	/* X flip */
	{
		l=0;
		for (y = sy;y <= ey;y++)
        {
           	bm  = dest->line[y]+sx;
        	c=0;
            {
            	p1=*(PL1);
            	p2=*(PL1+1);
            	p3=*(PL2);
            	p4=*(PL2+1);

                DRAW_PIX_X(7);
                DRAW_PIX_X(6);
                DRAW_PIX_X(5);
                DRAW_PIX_X(4);
                DRAW_PIX_X(3);
                DRAW_PIX_X(2);
                DRAW_PIX_X(1);
                DRAW_PIX_X(0);

                p1=*(PL1+32);
                p2=*(PL1+33);
                p3=*(PL2+32);
                p4=*(PL2+33);

                DRAW_PIX_X(7);
                DRAW_PIX_X(6);
                DRAW_PIX_X(5);
                DRAW_PIX_X(4);
                DRAW_PIX_X(3);
                DRAW_PIX_X(2);
                DRAW_PIX_X(1);
                DRAW_PIX_X(0);
			}

			/* Calculate line skipping for Y zoom */
            if (dy) {
            	PL1+=y_skip[zy][l]*2;
				PL2+=y_skip[zy][l]*2; /* 16 bits per line */
            }
            else {
           		PL1-=y_skip[zy][l]*2;
                PL2-=y_skip[zy][l]*2;
            }

            l++;
        }
    }
	else		/* normal */
    {
      	l=0;
		for (y = sy ;y <= ey;y++)
        {
        	bm  = dest->line[y] + sx;
            c=0;
            {
            	p1=*(PL1+32);
                p2=*(PL1+33);
                p3=*(PL2+32);
            	p4=*(PL2+33);

               	DRAW_PIX_X(0);
               	DRAW_PIX_X(1);
               	DRAW_PIX_X(2);
               	DRAW_PIX_X(3);
               	DRAW_PIX_X(4);
               	DRAW_PIX_X(5);
               	DRAW_PIX_X(6);
               	DRAW_PIX_X(7);

                p1=*(PL1+0);
                p2=*(PL1+1);
                p3=*(PL2+0);
                p4=*(PL2+1);

               	DRAW_PIX_X(0);
                DRAW_PIX_X(1);
                DRAW_PIX_X(2);
                DRAW_PIX_X(3);
                DRAW_PIX_X(4);
                DRAW_PIX_X(5);
                DRAW_PIX_X(6);
                DRAW_PIX_X(7);
			}

            /* Calculate line skipping for Y zoom */
            if (dy) {
               	PL1+=y_skip[zy][l]*2;
				PL2+=y_skip[zy][l]*2; /* 16 bits per line */
            }
            else {
                PL1-=y_skip[zy][l]*2;
               	PL2-=y_skip[zy][l]*2;
            }

            l++;

        }
    }
	}
}

/******************************************************************************/

void neogeo_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int sx =0,sy =0,oy =0,my =0,zx = 1,zy = 1, zz = 0xf;
	int offs,pom,i,count,x,y,dzy;
    int tileno,tileatr,t1,t2,t3;

    int still;

    #ifdef NEO_DEBUG
	char buf[80];
	struct DisplayText dt[2];
    #endif


    /* Do compressed palette stuff */
    neogeo_palette();
  	fillbitmap(bitmap,Machine->pens[0],&Machine->drv->visible_area);

	if (!dotiles) { 					/* debug */

	for (count=0;count<384*2;count+=2) {
		t3 = READ_WORD( &vidram[0x10000 + count] );
		t1 = READ_WORD( &vidram[0x10400 + count] );
		t2 = READ_WORD( &vidram[0x10800 + count] );

        /* If this bit is set this new column is placed next to last one */
		if (t1 & 0x40) {
			sx += (zx + 1); /* Adjust using old zx */
			if ( sx >= 0x1F0 )
				sx -= 0x200;

            /* Get new zoom for this column */
            zx = (t3 >> 8) & 0x0f;
			sy = oy;
		} else {	/* nope it is a new block */
        	/* Sprite scaling */
			zx = (t3 >> 8) & 0x0f;
			zy = (t3 >> 4) & 0x0f; /* MSB of Y */
            zz =  t3 & 0x0f; /* LSB of Y */

			sx = (t2 >> 7);
			if ( sx >= 0x1F0 )
				sx -= 0x200;

			sy = 0x1F0 - (t1 >> 7);
			oy = sy;

			/* Adjust tiles for zoom, see Puzzle Bobble, MagLord, AoF */
            my = (t1 & 0x3f);
		 	if (my<0x10) { /* Not sure about this, see nam1975 title screen */
            	if (zy<0xc) my++;
            	if (zy<0x8) my++;
           		if (zy<0x4) my++;
            }
		}
		offs = count<<6;

        still=15-zz;

        /* my holds the number of tiles in each vertical multisprite block */
        for (y=0; y < my ;y++) {
			tileno  = READ_WORD(&vidram[offs + 4*y]);
			tileatr = READ_WORD(&vidram[offs + 4*y+2]);
            if (high_tile && tileatr&0x10) tileno+=0x10000;
            if (high_tile && tileatr&0x20) tileno+=0x20000;

            /* Calculate fine tune for y zoom */
            if (zz==0xf)
            	dzy=zy+1;
            else {

                if (still) {
            		dzy=zy+1 - 1; //(15 - zz);
                	still--;
                }
             	else
                 dzy=zy+1;

            }

		 	if (y != 0x20) 	/* fixes a bug in sidekicks */
               (*NeoDrawFunc)(bitmap,
					Machine->gfx[2],
					tileno,
					tileatr >> 8,
					tileatr & 0x01,tileatr & 0x02,
					sx,sy,zx+1,dzy
                );


			sy += dzy;
	   		if ( sy > 0x1F0 )
	   			sy = sy - 0x200;

			/* This works for full screen, 0x20 tile.. */
       //     if (sy > 0x1f0 /*- (0xff - ((zy<<4)+zz))*/ - (16*(15-zy))  )
	   //			sy = sy - 0x200 + /*(0xff - ((zy<<4)+zz))*/ + (16*(15-zy));

		}  /* for y */
	}  /* for count */

	/* Character foreground */
 	for (y=2;y<30;y++) {
 		for (x=0;x<40;x++) {
			int byte1 = (READ_WORD( &vidram[0xE000 + 2*y + x*64] ));
			int byte2 = byte1 >> 12;
            byte1 = byte1 & 0xfff;

            /* About 4% speedup, but I haven't checked if it breaks any games */
            if (byte1==0xff) continue;

			drawgfx(bitmap,
				Machine->gfx[fix_bank],
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

                tileno = byte1 + (byte2 << 8);
            	if (byte3&0x10) tileno+=0x10000;


                NeoMVSDrawGfx(bitmap,
					Machine->gfx[2],
					tileno,
					col,
					byte3 & 0x01,byte3 & 0x02,
					x*16,y*16,16,16
                 );

	/*			drawgfx(bitmap,
					Machine->gfx[1],
					byte1 + (byte2 << 8),
					col,
					byte3 & 0x01,byte3 & 0x02,
					x*16,y*16,
					&Machine->drv->visible_area,
					TRANSPARENCY_NONE,
					0); */
			}
		}

		sprintf(buf,"POS : %04X , VDP regs %04X",screen_offs, (screen_offs >> 6) );
		dt[0].text = buf;
		dt[0].color = DT_COLOR_RED;
		dt[0].x = 0;
		dt[0].y = 0;
		dt[1].text = 0;
		displaytext(dt,0,1);
	}	/* debug */


#ifdef NEO_DEBUG
/* More debug stuff :) */
{
/*
	int j;
	char mybuf[20];
	int trueorientation;
	struct osd_bitmap *mybitmap = Machine->scrbitmap;

	trueorientation = Machine->orientation;
	Machine->orientation = ORIENTATION_DEFAULT;
*/



//        fprintf(errorlog,"X: %04x Y: %04x Video: %04x\n",READ_WORD(&vidram[0x1089c]),READ_WORD(&vidram[0x1049c]),READ_WORD(&vidram[0x1009c]));

// fprintf(errorlog,"X: %04x Y: %04x Video: %04x\n",READ_WORD(&vidram[0x10a58]),READ_WORD(&vidram[0x10658]),READ_WORD(&vidram[0x10258]));


}
#endif

}


/*************************/

static void NeoMGD2DrawGfx(struct osd_bitmap *dest,const struct GfxElement *gfx,
		unsigned int code,unsigned int color,int flipx,int flipy,
        int sx,int sy,int zx,int zy)
{
	int ox,oy,ex,ey,y,start,dy;
	unsigned char *bm;
	int col;
    int l,c; /* Line skipping counters */

	unsigned char *PL1 = Machine->memory_region[2];
    unsigned char *PL2 = Machine->memory_region[3];
    unsigned char *ptr,p1,p2,p3,p4;

	#define GET_PIX(shift) ((p1>>shift)&1) + (((p2>>shift)&1)<<1) + (((p3>>shift)&1)<<2) + (((p4>>shift)&1)<<3)
	#define DRAW_PIX_X(shift) \
    	if (x_skip[zx][c]) {col = ((p1>>shift)&1) + (((p2>>shift)&1)<<1) + (((p3>>shift)&1)<<2) + (((p4>>shift)&1)<<3); \
		if (col) *bm = paldata[col];	\
			bm++;						\
		}								\
		c++;							\

    /* Safety feature */
    code=code%no_of_tiles;

    /* Check for total transparency, no need to draw */
    if ((gfx->pen_usage[code] & ~1) == 0)
    	return;

	/* Check bounds */
	ex = sx + zx -1;   /* Clip for size of zoomed object */
	if (sx < -16) return; /* Should be ok as there is margin around visible area */
	if (ex >= dest->width) ex = dest->width-1;
	if (sx > ex) return;

    oy=sy;
  	ey = sy + zy -1; 	/* Clip for size of zoomed object */
	if (sy < 0) sy = 0;
	if (ey >= dest->height) ey = dest->height-1;
	if (sy > ey) return;

	if (flipy)	/* Y flip */
	{
	   	start = ((code+1) * 32) - 17;
		dy = 0;
	}                           // NEED TO ADJUST FOR CLIPS
	else		/* normal */
	{
        start = (code * 32) + (sy-oy);
		dy = 1;
	}
	PL1+=start;
	PL2+=start;

	{
    const unsigned short *paldata;	/* ASG 980209 */
	paldata = &gfx->colortable[gfx->color_granularity * color];
	if (flipx)	/* X flip */
	{
		l=0;
		for (y = sy;y <= ey;y++)
        {
           	bm  = dest->line[y]+sx;
        	c=0;
            {
            	p1=*(PL1);
            	p2=*(PL1+mgd2_plane);
            	p3=*(PL2);
            	p4=*(PL2+mgd2_plane);

            	/* For speed - a version with x_skip, a version without */
                if (zx==16) {
                	col = GET_PIX(7); if (col) *bm = paldata[col]; bm++;
                    col = GET_PIX(6); if (col) *bm = paldata[col]; bm++;
					col = GET_PIX(5); if (col) *bm = paldata[col]; bm++;
					col = GET_PIX(4); if (col) *bm = paldata[col]; bm++;
                    col = GET_PIX(3); if (col) *bm = paldata[col]; bm++;
					col = GET_PIX(2); if (col) *bm = paldata[col]; bm++;
					col = GET_PIX(1); if (col) *bm = paldata[col]; bm++;
					col = GET_PIX(0); if (col) *bm = paldata[col]; bm++;
                }
                else {
                	DRAW_PIX_X(7);
                    DRAW_PIX_X(6);
                    DRAW_PIX_X(5);
                    DRAW_PIX_X(4);
                    DRAW_PIX_X(3);
                    DRAW_PIX_X(2);
                    DRAW_PIX_X(1);
                    DRAW_PIX_X(0);
                } /* End of x_skip draw version */

                p1=*(PL1+16);
                p2=*(PL1+mgd2_plane+16);
                p3=*(PL2+16);
                p4=*(PL2+mgd2_plane+16);

            	/* For speed - a version with x_skip, a version without */
                if (zx==16) {
                	col = GET_PIX(7); if (col) *bm = paldata[col]; bm++;
                    col = GET_PIX(6); if (col) *bm = paldata[col]; bm++;
					col = GET_PIX(5); if (col) *bm = paldata[col]; bm++;
					col = GET_PIX(4); if (col) *bm = paldata[col]; bm++;
                    col = GET_PIX(3); if (col) *bm = paldata[col]; bm++;
					col = GET_PIX(2); if (col) *bm = paldata[col]; bm++;
					col = GET_PIX(1); if (col) *bm = paldata[col]; bm++;
					col = GET_PIX(0); if (col) *bm = paldata[col]; bm++;
                }
                else {
                	DRAW_PIX_X(7);
                    DRAW_PIX_X(6);
                    DRAW_PIX_X(5);
                    DRAW_PIX_X(4);
                    DRAW_PIX_X(3);
                    DRAW_PIX_X(2);
                    DRAW_PIX_X(1);
                    DRAW_PIX_X(0);
                } /* End of x_skip draw version */

			}

			/* Calculate line skipping for Y zoom */
            if (dy) {
            	PL1+=y_skip[zy][l];
				PL2+=y_skip[zy][l]; /* 8 bits per line */
            }
            else {
           		PL1-=y_skip[zy][l];
                PL2-=y_skip[zy][l];
            }

            l++;
        }
    }
	else		/* normal */
    {
      	l=0;
		for (y = sy ;y <= ey;y++)
        {
        	bm  = dest->line[y] + sx;
            c=0;
            {
            	p1=*(PL1+16);
                p2=*(PL1+mgd2_plane+16);
                p3=*(PL2+16);
                p4=*(PL2+mgd2_plane+16);


            	/* For speed - a version with x_skip, a version without */
                if (zx==16) {
                	col = GET_PIX(0); if (col) *bm = paldata[col]; bm++;
                    col = GET_PIX(1); if (col) *bm = paldata[col]; bm++;
					col = GET_PIX(2); if (col) *bm = paldata[col]; bm++;
					col = GET_PIX(3); if (col) *bm = paldata[col]; bm++;
                    col = GET_PIX(4); if (col) *bm = paldata[col]; bm++;
					col = GET_PIX(5); if (col) *bm = paldata[col]; bm++;
					col = GET_PIX(6); if (col) *bm = paldata[col]; bm++;
					col = GET_PIX(7); if (col) *bm = paldata[col]; bm++;
                }
                else {
                	DRAW_PIX_X(0);
                    DRAW_PIX_X(1);
                    DRAW_PIX_X(2);
                    DRAW_PIX_X(3);
                    DRAW_PIX_X(4);
                    DRAW_PIX_X(5);
                    DRAW_PIX_X(6);
                    DRAW_PIX_X(7);
                } /* End of x_skip draw version */

                p1=*(PL1);
            	p2=*(PL1+mgd2_plane);
            	p3=*(PL2);
            	p4=*(PL2+mgd2_plane);
                if (zx==16) {
                	col = GET_PIX(0); if (col) *bm = paldata[col]; bm++;
                    col = GET_PIX(1); if (col) *bm = paldata[col]; bm++;
					col = GET_PIX(2); if (col) *bm = paldata[col]; bm++;
					col = GET_PIX(3); if (col) *bm = paldata[col]; bm++;
                    col = GET_PIX(4); if (col) *bm = paldata[col]; bm++;
					col = GET_PIX(5); if (col) *bm = paldata[col]; bm++;
					col = GET_PIX(6); if (col) *bm = paldata[col]; bm++;
					col = GET_PIX(7); if (col) *bm = paldata[col]; bm++;
                }
                else {
                	DRAW_PIX_X(0);
                    DRAW_PIX_X(1);
                    DRAW_PIX_X(2);
                    DRAW_PIX_X(3);
                    DRAW_PIX_X(4);
                    DRAW_PIX_X(5);
                    DRAW_PIX_X(6);
                    DRAW_PIX_X(7);
				} /* End of x_skip draw version */
			}

            /* Calculate line skipping for Y zoom */
            if (dy) {
               	PL1+=y_skip[zy][l];
				PL2+=y_skip[zy][l]; /* 8 bits per line */
            }
            else {
                PL1-=y_skip[zy][l];
               	PL2-=y_skip[zy][l];
            }

            l++;

        }
    }
	}
}






/* For MGD-2 dumps */
int neogeo_mgd2_vh_start(void)
{
	int i,x,y,tiles,p1,p2,p3,p4;
    unsigned char mybyte;
	unsigned char *PL1 = Machine->memory_region[2];
    unsigned char *PL2 = Machine->memory_region[3];

	#define GET_PIX(shift) ((p1>>shift)&1) + (((p2>>shift)&1)<<1) + (((p3>>shift)&1)<<2) + (((p4>>shift)&1)<<3)

    /* For MGD2 games we need to calculate the pen_usage array ourself */
	if (Machine->gfx[2]->pen_usage)
    	free(Machine->gfx[2]->pen_usage);
    tiles=Machine->memory_region_length[2]/64;
    no_of_tiles=tiles;
    if (no_of_tiles>0xffff) high_tile=1; else high_tile=0;
    Machine->gfx[2]->pen_usage=malloc(tiles * sizeof(int));
    mgd2_plane=Machine->memory_region_length[2]/2;
    NeoDrawFunc=NeoMGD2DrawGfx;


#if 0
{
	/* Debug */
	FILE *fp;
    char text[40];

	sprintf(text,"%s.p1",Machine->gamedrv->name);
    fp=fopen(text,"rb");
    if (fp) {
    	fread(Machine->gfx[2]->pen_usage,tiles * sizeof(int), 1 ,fp);
    	fclose(fp);
        return common_vh_start();
    }
}
#endif


    for (i=0; i<tiles; i++) {
    	Machine->gfx[2]->pen_usage[i] = 0;

		for (y = 0;y < 16;y++)
		{
			p1=*(PL1);
			p2=*(PL1+mgd2_plane);
			p3=*(PL2);
			p4=*(PL2+mgd2_plane);

			for (x = 0;x <8 ;x++)
			{
				mybyte=GET_PIX(x);
            	Machine->gfx[2]->pen_usage[i] |= 1 << mybyte;
        	}

        	p1=*(PL1+16);
        	p2=*(PL1+mgd2_plane+16);
       	 	p3=*(PL2+16);
        	p4=*(PL2+mgd2_plane+16);

        	for (x = 0;x <8 ;x++)
        	{
        		mybyte=GET_PIX(x);
        		Machine->gfx[2]->pen_usage[i] |= 1 << mybyte;
        	}

        	PL1+=1;
        	PL2+=1;
    	}
    	PL2+=16;
    	PL1+=16;
    }

    if (errorlog) fprintf(errorlog,"Calculated pen usage for %d tiles\n",tiles);


#if 0
{
	/* Debug */
	FILE *fp;
    char text[40];

	sprintf(text,"%s.p1",Machine->gamedrv->name);
    fp=fopen(text,"wb");
    fwrite(Machine->gfx[2]->pen_usage,tiles * sizeof(int), 1 ,fp);
    fclose(fp);
}
#endif

	#undef GET_PIX
	return common_vh_start();
}
/* For MGD-2 dumps */
int neogeo_vh_start(void)
{
	/* Stuff to go here later */
	return common_vh_start();
}

/* MVS cartidges */
int neogeo_mvs_vh_start(void)
{
	int i,x,y,tiles,p1,p2,p3,p4;
    unsigned char mybyte;
	unsigned char *PL1 = Machine->memory_region[2];
    unsigned char *PL2 = Machine->memory_region[3];

	#define GET_PIX(shift) ((p1>>shift)&1) + (((p2>>shift)&1)<<1) + (((p3>>shift)&1)<<2) + (((p4>>shift)&1)<<3)

    /* For MVS games we need to calculate the pen_usage array ourself */
	if (Machine->gfx[2]->pen_usage)
    	free(Machine->gfx[2]->pen_usage);
    tiles=Machine->memory_region_length[2]/64;
    no_of_tiles=tiles;
    if (no_of_tiles>0xffff) high_tile=1; else high_tile=0;
    Machine->gfx[2]->pen_usage=malloc(tiles * sizeof(int));
    NeoDrawFunc=NeoMVSDrawGfx;


#if 0
{
	/* Debug */
	FILE *fp;
    char text[40];

	sprintf(text,"%s.p1",Machine->gamedrv->name);
    fp=fopen(text,"rb");
    if (fp) {
    	fread(Machine->gfx[2]->pen_usage,sizeof(int), tiles ,fp);
    	fclose(fp);
		if (errorlog) fprintf(errorlog,"Loaded pen usage for %d tiles\n",tiles);
        return common_vh_start();
    }
}
#endif


    for (i=0; i<tiles; i++) {
    	Machine->gfx[2]->pen_usage[i] = 0;

		for (y = 0;y < 16;y++)
		{
			p1=*(PL1);
			p2=*(PL1+1);
			p3=*(PL2);
			p4=*(PL2+1);

			for (x = 0;x <8 ;x++)
			{
				mybyte=GET_PIX(x);
            	Machine->gfx[2]->pen_usage[i] |= 1 << mybyte;
        	}

        	p1=*(PL1+32);
        	p2=*(PL1+33);
       	 	p3=*(PL2+32);
        	p4=*(PL2+33);

        	for (x = 0;x <8 ;x++)
        	{
        		mybyte=GET_PIX(x);
        		Machine->gfx[2]->pen_usage[i] |= 1 << mybyte;
        	}

        	PL1+=2;
        	PL2+=2;
    	}
    	PL2+=32;
    	PL1+=32;
    }

    if (errorlog) fprintf(errorlog,"Calculated pen usage for %d tiles\n",tiles);


#if 0
{
	/* Debug */
	FILE *fp;
    char text[40];

	sprintf(text,"%s.p1",Machine->gamedrv->name);
    fp=fopen(text,"wb");
    fwrite(Machine->gfx[2]->pen_usage,sizeof(int), tiles ,fp);
    fclose(fp);
}
#endif

	#undef GET_PIX
	return common_vh_start();
}

