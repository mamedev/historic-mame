/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

  Todo:
  7 unknown attribute bits on scroll 2/3.
  Speed, speed, speed...

  OUTPUT PORTS (preliminary)
  0x00-0x01     OBJ RAM base (/256)
  0x02-0x03     Scroll1 RAM base (/256)
  0x04-0x05     Scroll2 RAM base (/256)
  0x06-0x07     Scroll3 RAM base (/256)
  0x08-0x09     "Other" RAM - Scroll distortion (/256)
  0x0a-0x0b     Palette base (/256)
  0x0c-0x0d     Scroll 1 X
  0x0e-0x0f     Scroll 1 Y
  0x10-0x11     Scroll 2 X
  0x12-0x13     Scroll 2 Y
  0x14-0x15     Scroll 3 X
  0x16-0x17     Scroll 3 Y

  0x18-0x19     ????
  0x20-0x21     ????
  0x22-0x23     ????
  0x24-0x25     ????


  0x4e-0x4f     ????
  0x5e-0x5f     ????

  Registers move from game to game.. following example strider
  0x66-0x67     Video control register (location varies according to game)
  0x6a-0x6b     Priority mask
  0x6c-0x6d     Priority mask
  0x6e-0x6f     Priority mask
  0x70-0x72     Control register (usually 0x003f)

  Fixed registers
  0x80-0x81     ????            Sound output.
  0x88-0x89     ????            Port thingy (sound fade???)

  Known Bug List
  ==============
  All games
  * Palette fades overflow palette.

  Magic Sword.
  * Distortion effect is mapped wrong.
  * Rogue scroll 2 character at end of level 1

  King of Dragons.
  * Distortion effect missing on player selection screen.

  1941
  * Brief flicker of scroll 2 when coining up / starting game.

  Captain Commando
  * Partial priority for scroll 3 is wrong (entrance of building on level 1)

  SF2
  * Sprite lag on screens.
  * Japanese level still doesn't look 100%


  Todo
  ====

  Implement CPS2 QSound system for Rockman.


  Unknown issues
  ==============

  There are often some redundant high bits in the scroll layer's attributes.


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "drivers/cps1.h"

#include "osdepend.h"

#ifdef MAME_DEBUG
#define LAYER_DEBUG
#endif

#define VERBOSE 0

struct CPS1VIDCFG
{
	int layer_control;
	int s2_priority1;
	int s2_priority2;
	int s2_priority3;
	int control_reg;  /* Control register? Ususally contains 0x3f */
	int distort_alt;
};

/* Configuration tables */
static struct CPS1VIDCFG cps1_vid_cfg[]=
{
	{0x66,0x00,0x00,0x00,0x70,0}, /* 00 = Un Squad / Pnickies */
	{0x70,0x6c,0x6a,0x68,0x66,0}, /* 01 = Willow 6e 6c 6a 68 */
	{0x6e,0x70,0x68,0x72,0x6a,1}, /* 02 = Final Fight 66 70 68 72 */
	{0x66,0x6a,0x6c,0x6e,0x70,1}, /* 03 = ffightu 68 6a 6c 6e */
	{0x66,0x6c,0x6a,0x6e,0x70,0}, /* 04 = Strider 6a 6c 6e */
	{0x66,0x6a,0x6c,0x6e,0x70,0}, /* 05 = Ghouls 68 6a 6c 6e */
	{0x68,0x62,0x64,0x66,0x70,0}, /* 06 = Knights 66 64 62 60 */
	{0x62,0x66,0x68,0x6a,0x6c,2}, /* 07 = Magic Sword 64 66 68 6a */
	{0x42,0x46,0x48,0x4a,0x4c,0}, /* 08 = Nemo 44 46 48 4a */
	{0x6c,0x68,0x66,0x64,0x62,0}, /* 09 = DWJ 6a 68 66 64 */
	{0x60,0x6c,0x6a,0x68,0x70,0}, /* 10 = Captain Commando / KOD 6e 6c 6a 68 */
	{0x4c,0x48,0x46,0x44,0x42,0}, /* 11 = Carrier Air Wing 4a 48 46 44 */
	{0x54,0x00,0x00,0x00,0x4a,3}, /* 12 = Street Fighter 2 52 50 4e 4c */
	{0x62,0x00,0x00,0x00,0x6c,3}, /* 13 = sf2j 64 66 68 6a */
	{0x66,0x6c,0x6a,0x68,0x70,3}, /* 14 = Street Fighter 2 (Turbo) 68 6a 6c 6e */
	{0xdc,0x00,0x00,0x00,0xd2,3}, /* 15 = SF2 (Rev E) da d8 d6 d4 */
	{0x68,0x62,0x64,0x60,0x70,3}, /* 16 = 3 Wonders (66) 64 62 60 */
	{0x6e,0x70,0x68,0x72,0x6a,1}, /* 17 = Varth 66 70 68 72 */
	{0x60,0x6c,0x6a,0x68,0x70,1}, /* 18 = varthj 6e 6c 6a 68 */
	{0x68,0x00,0x00,0x00,0x72,0}, /* 19 = 1941 6c 6e 70 */
	{0x66,0x6a,0x6c,0x6e,0x70,0}, /* 20 = Megaman 68 6a 6c 6e */
	{0x52,0x56,0x58,0x5a,0x5c,0}, /* 21 = Mega Twins 54 56 58 5a */
	{0x6c,0x00,0x00,0x00,0x62,0}, /* 22 = Mercs */
};


/* Public variables */
unsigned char *cps1_gfxram;
unsigned char *cps1_output;
int cps1_gfxram_size;
int cps1_output_size;

/* Private */

/* Offset of each palette entry */
const int cps1_obj_palette    =0;
const int cps1_scroll1_palette=32;
const int cps1_scroll2_palette=32+32;
const int cps1_scroll3_palette=32+32+32;
#define cps1_palette_entries (32*4)  /* Number colour schemes in palette */

const int cps1_scroll1_size=0x4000;
const int cps1_scroll2_size=0x4000;
const int cps1_scroll3_size=0x4000;
const int cps1_obj_size    =0x4000;
const int cps1_other_size  =0x4000;
const int cps1_palette_size=cps1_palette_entries*32; /* Size of palette RAM */

static unsigned char *cps1_scroll1;
static unsigned char *cps1_scroll2;
static unsigned char *cps1_scroll3;
static unsigned char *cps1_obj;
static unsigned char *cps1_palette;
static unsigned char *cps1_other;
static unsigned char *cps1_old_palette;

/* Working variables */
static int cps1_last_sprite_offset;     /* Offset of the last sprite */
static int cps1_layer_control;          /* Layer control register */
static int cps1_layer_enabled[4];       /* Layer enabled [Y/N] */

int scroll1x, scroll1y, scroll2x, scroll2y, scroll3x, scroll3y;
struct CPS1config *cps1_game_config;
static unsigned char *cps1_scroll2_old;
static struct osd_bitmap *cps1_scroll2_bitmap;


/* Output ports */
#define CPS1_OBJ_BASE           0x00    /* Base address of objects */
#define CPS1_SCROLL1_BASE       0x02    /* Base address of scroll 1 */
#define CPS1_SCROLL2_BASE       0x04    /* Base address of scroll 2 */
#define CPS1_SCROLL3_BASE       0x06    /* Base address of scroll 3 */
#define CPS1_OTHER_BASE         0x08    /* Base address of other video */
#define CPS1_PALETTE_BASE       0x0a    /* Base address of palette */
#define CPS1_SCROLL1_SCROLLX    0x0c    /* Scroll 1 X */
#define CPS1_SCROLL1_SCROLLY    0x0e    /* Scroll 1 Y */
#define CPS1_SCROLL2_SCROLLX    0x10    /* Scroll 2 X */
#define CPS1_SCROLL2_SCROLLY    0x12    /* Scroll 2 Y */
#define CPS1_SCROLL3_SCROLLX    0x14    /* Scroll 3 X */
#define CPS1_SCROLL3_SCROLLY    0x16    /* Scroll 3 Y */

#define CPS1_SCROLL2_WIDTH      0x40
#define CPS1_SCROLL2_HEIGHT     0x40



static int cps1_transparency_scroll2[4]=
{
	0xffff, /* 0x0000 */
	0xffff, /* 0x0800 */
	0xffff, /* 0x1000 */
	0xffff  /* 0x1800 */
};

static int cps1_transparency_scroll3[4]=
{
	0xffff, /* 0x0000 */
	0xffff, /* 0x0800 */
	0xffff, /* 0x1000 */
	0xffff  /* 0x1800 */
};


INLINE int cps1_port(int offset)
{
	return READ_WORD(&cps1_output[offset]);
}

INLINE unsigned char * cps1_base(int offset)
{
	int base=cps1_port(offset)*256;
	return &cps1_gfxram[base&0x3ffff];
}

int cps1_output_r(int offset)
{
#if VERBOSE
if (errorlog && offset >= 0x18) fprintf(errorlog,"PC %06x: read output port %02x\n",cpu_getpc(),offset);
#endif
	return READ_WORD(&cps1_output[offset]);
}

void cps1_output_w(int offset, int data)
{
	int value;

#if VERBOSE
if (errorlog && offset >= 0x18) fprintf(errorlog,"PC %06x: write %02x to output port %02x\n",cpu_getpc(),data,offset);
#endif
	COMBINE_WORD_MEM(&cps1_output[offset],data);

	/* protections */
	switch (offset)
	{
		case 0x40:
		case 0x42:
			if (cps1_game_config->alternative==14 )
			{
				/* Only apply protection to sf2t (other games use 0x40-0x47)*/
				value=cps1_port(0x40);
				value*=cps1_port(0x42);
				/* Slight variation ... only one word result */
				WRITE_WORD(&cps1_output[0x44], value&0xffff);
			}
			else if (cps1_game_config->alternative==11)
			{
				/* Carrier Air Wing... unknown protection */
				WRITE_WORD(&cps1_output[0x40], 0x0406);
			}
			break;

		case 0x44:
		case 0x46:
		if (cps1_game_config->alternative==6)
			{
				/* Only apply protection to knights (other games use 0x40-0x47)*/
				value=cps1_port(0x44);
				value*=cps1_port(0x46);
				WRITE_WORD(&cps1_output[0x40], value>>16);
				WRITE_WORD(&cps1_output[0x42], value&0xffff);
			}
			break;

		case 0x4c:
		case 0x4e:
			if (cps1_game_config->alternative==16   /* 3wonders */
					|| cps1_game_config->alternative==18)   /* varthj */
			{
				/* Only apply protection to 3wonders (other games use 0x4c-0x4f)*/
				value=cps1_port(0x4c);
				value*=cps1_port(0x4e);
				WRITE_WORD(&cps1_output[0x48], value>>16);
				WRITE_WORD(&cps1_output[0x4a], value&0xffff);
			}
			break;

		case 0x5c:
		case 0x5e:
		if (cps1_game_config->alternative==10)
			{
				/* Only apply protection to KOD (other games use 0x58-0x5b)*/
				value=cps1_port(0x5c);
				value*=cps1_port(0x5e);
				WRITE_WORD(&cps1_output[0x58], value>>16);
				WRITE_WORD(&cps1_output[0x5a], value&0xffff);
			}
			break;
	}
}




#ifdef MAME_DEBUG
void cps1_dump_video(void)
{
	FILE *fp;
	fp=fopen("SCROLL1.DMP", "w+b");
	if (fp)
	{
		fwrite(cps1_scroll1, cps1_scroll1_size, 1, fp);
		fclose(fp);
	}
	fp=fopen("SCROLL2.DMP", "w+b");
	if (fp)
	{
		fwrite(cps1_scroll2, cps1_scroll2_size, 1, fp);
		fclose(fp);
	}
	fp=fopen("SCROLL3.DMP", "w+b");
	if (fp)
	{
		fwrite(cps1_scroll3, cps1_scroll3_size, 1, fp);
		fclose(fp);
	}
	fp=fopen("OBJ.DMP", "w+b");
	if (fp)
	{
		fwrite(cps1_obj, cps1_obj_size, 1, fp);
		fclose(fp);
	}

	fp=fopen("OTHER.DMP", "w+b");
	if (fp)
	{
		fwrite(cps1_other, cps1_other_size, 1, fp);
		fclose(fp);
	}

	fp=fopen("PALETTE.DMP", "w+b");
	if (fp)
	{
		fwrite(cps1_palette, cps1_palette_size, 1, fp);
		fclose(fp);
	}

	fp=fopen("OUTPUT.DMP", "w+b");
	if (fp)
	{
		fwrite(cps1_output, cps1_output_size, 1, fp);
		fclose(fp);
	}
	fp=fopen("VIDEO.DMP", "w+b");
	if (fp)
	{
		fwrite(cps1_gfxram, cps1_gfxram_size, 1, fp);
		fclose(fp);
	}

}
#endif


INLINE void cps1_get_video_base(void )
{
	struct CPS1VIDCFG *pCFG;

	/* Re-calculate the VIDEO RAM base */
	cps1_scroll1=cps1_base(CPS1_SCROLL1_BASE);
	cps1_scroll2=cps1_base(CPS1_SCROLL2_BASE);
	cps1_scroll3=cps1_base(CPS1_SCROLL3_BASE);
	cps1_obj=cps1_base(CPS1_OBJ_BASE);
	cps1_palette=cps1_base(CPS1_PALETTE_BASE);
	cps1_other=cps1_base(CPS1_OTHER_BASE);

	/* Get scroll values */
	scroll1x=cps1_port(CPS1_SCROLL1_SCROLLX);
	scroll1y=cps1_port(CPS1_SCROLL1_SCROLLY);
	scroll2x=cps1_port(CPS1_SCROLL2_SCROLLX);
	scroll2y=cps1_port(CPS1_SCROLL2_SCROLLY);
	scroll3x=cps1_port(CPS1_SCROLL3_SCROLLX);
	scroll3y=cps1_port(CPS1_SCROLL3_SCROLLY);

	/* Get transparency registers */
	pCFG=&cps1_vid_cfg[cps1_game_config->alternative];
	if (pCFG->s2_priority1)
	{
		cps1_transparency_scroll2[1]=~cps1_port(pCFG->s2_priority1);
		cps1_transparency_scroll2[2]=~cps1_port(pCFG->s2_priority2);
		cps1_transparency_scroll2[3]=~cps1_port(pCFG->s2_priority3);
		cps1_transparency_scroll3[1]=~cps1_port(pCFG->s2_priority2);
		cps1_transparency_scroll3[2]=~cps1_port(pCFG->s2_priority1);
		cps1_transparency_scroll3[3]=0x8000;

		if (cps1_game_config->alternative==7)
		{
			cps1_transparency_scroll3[1]=0xffff;
			cps1_transparency_scroll3[2]=0xffff;
			cps1_transparency_scroll3[3]=~cps1_port(pCFG->s2_priority3);
		}
	 }
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int cps1_vh_start(void)
{

	int i;

	cps1_scroll2_bitmap=osd_new_bitmap(CPS1_SCROLL2_WIDTH*16,
		CPS1_SCROLL2_HEIGHT*16, Machine->scrbitmap->depth );
	if (!cps1_scroll2_bitmap)
	{
		return -1;
	}
	cps1_scroll2_old=malloc(cps1_scroll2_size);
	if (!cps1_scroll2_old)
	{
		return -1;
	}
	memset(cps1_scroll2_old, 0xff, cps1_scroll2_size);


	cps1_old_palette=(unsigned char *)malloc(cps1_palette_size);
	if (!cps1_old_palette)
	{
		return -1;
	}
	memset(cps1_old_palette, 0xff, cps1_palette_size);

	memset(cps1_gfxram, 0, cps1_gfxram_size);   /* Clear GFX RAM */
	memset(cps1_output, 0, cps1_output_size);   /* Clear output ports */

	if (!cps1_game_config)
	{
		if (errorlog)
		{
			fprintf(errorlog, "cps1_game_config hasn't been set up yet");
		}
		return -1;
	}

	if (cps1_game_config->alternative >
		sizeof(cps1_vid_cfg) / sizeof(cps1_vid_cfg[0]))
	{
		if (errorlog)
		{
			fprintf(errorlog, "cps1_game_config out of range value");
		}
		return -1;
	}


	cps1_layer_control=cps1_vid_cfg[cps1_game_config->alternative].layer_control;

	/* Put in some defaults */
	WRITE_WORD(&cps1_output[0x00], 0x9200);
	WRITE_WORD(&cps1_output[0x02], 0x9000);
	WRITE_WORD(&cps1_output[0x04], 0x9040);
	WRITE_WORD(&cps1_output[0x06], 0x9080);
	WRITE_WORD(&cps1_output[0x08], 0x9100);
	WRITE_WORD(&cps1_output[0x0a], 0x90c0);



	/* Set up old base */
	cps1_get_video_base();   /* Calculate base pointers */
	cps1_get_video_base();   /* Calculate old base pointers */


	/*
		Some games interrogate a couple of registers on bootup.
		These are CPS1 board B self test checks. They wander from game to
		game.
	*/
	if (cps1_game_config->cpsb_addr)
	{
		if (errorlog)
		{
			fprintf(errorlog, "CPSB port %02x=%04x\n",
			cps1_game_config->cpsb_addr,
			cps1_game_config->cpsb_value);
		}
		WRITE_WORD(&cps1_output[cps1_game_config->cpsb_addr],
			   cps1_game_config->cpsb_value);
	}

	for (i=0; i<4; i++)
	{
	       cps1_transparency_scroll2[i]=0xffff;
	       cps1_transparency_scroll3[i]=0xffff;
	}
	return 0;
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void cps1_vh_stop(void)
{
	if (cps1_old_palette)
		free(cps1_old_palette);
	if (cps1_scroll2_bitmap)
		osd_free_bitmap(cps1_scroll2_bitmap);
	if (cps1_scroll2_old)
		free(cps1_scroll2_old);
}

/***************************************************************************

  Build palette from palette RAM

  12 bit RGB with a 4 bit brightness value.

***************************************************************************/

INLINE void cps1_build_palette(void)
{
	int offset;

	for (offset = 0; offset < cps1_palette_entries*16; offset++)
	{
		int palette = READ_WORD (&cps1_palette[offset * 2]);

		if (palette != READ_WORD (&cps1_old_palette[offset * 2]) )
		{
		   int red, green, blue, bright;

		   bright= (palette>>12);
		   if (bright) bright += 2;

		   red   = ((palette>>8)&0x0f) * bright;
		   green = ((palette>>4)&0x0f) * bright;
		   blue  = (palette&0x0f) * bright;

		   palette_change_color (offset, red, green, blue);
		   WRITE_WORD(&cps1_old_palette[offset * 2], palette);
		}
	}
}

/***************************************************************************

  Scroll 1 (8x8)

  Attribute word layout:
  0x0001        colour
  0x0002        colour
  0x0004        colour
  0x0008        colour
  0x0010        colour
  0x0020        X Flip
  0x0040        Y Flip
  0x0080
  0x0100
  0x0200
  0x0400
  0x0800
  0x1000
  0x2000
  0x4000
  0x8000


***************************************************************************/

INLINE void cps1_palette_scroll1(unsigned short *base)
{
	int x,y, offs, offsx;
	int base_scroll1=cps1_game_config->base_scroll1;
	int space_char=cps1_game_config->space_scroll1;

	int scrlxrough=(scroll1x>>3)+8;
	int scrlyrough=(scroll1y>>3);

	int elements = Machine->gfx[0]->total_elements;

	for (x=0; x<0x34; x++)
	{
		 offsx=(scrlxrough+x)*0x80;
		 offsx&=0x1fff;

		 for (y=0; y<0x20; y++)
		 {
			int code, offsy;
			int n=scrlyrough+y;
			offsy=( (n&0x1f)*4 | ((n&0x20)*0x100)) & 0x3fff;
			offs=offsy+offsx;
			offs &= 0x3fff;

			code=READ_WORD(&cps1_scroll1[offs]);

			if (code >= base_scroll1 && code != space_char)
			{
				/* Optimization: only draw non-spaces */
				int colour=READ_WORD(&cps1_scroll1[offs+2]);
				code -= base_scroll1;
				base[colour&0x1f] |= Machine->gfx[0]->pen_usage[code % elements];
			}
		}
	}
}

INLINE void cps1_render_scroll1(struct osd_bitmap *bitmap, int priority)
{
	int x,y, offs, offsx, sx, sy;
	int base_scroll1=cps1_game_config->base_scroll1;
	int space_char=cps1_game_config->space_scroll1;

	int scrlxrough=(scroll1x>>3)+8;
	int scrlyrough=(scroll1y>>3);

	sx=-(scroll1x&0x07);

	for (x=0; x<0x34; x++)
	{
		 sy=-(scroll1y&0x07);
		 offsx=(scrlxrough+x)*0x80;
		 offsx&=0x1fff;

		 for (y=0; y<0x20; y++)
		 {
			int code, offsy;
			int n=scrlyrough+y;
			offsy=( (n&0x1f)*4 | ((n&0x20)*0x100)) & 0x3fff;
			offs=offsy+offsx;
			offs &= 0x3fff;

			code=READ_WORD(&cps1_scroll1[offs]);
#ifdef LAYER_DEBUG
			if (errorlog && code != space_char &&
				(code < base_scroll1 ||
				 code > (base_scroll1+Machine->gfx[0]->total_elements)))
			{
			      fprintf(errorlog,"WARNING: Out of range scroll 1 character %04x\n", code);
			}
#endif

			if (code >= base_scroll1 && code != space_char)
			{
				/* Optimization: only draw non-spaces */
				int colour=READ_WORD(&cps1_scroll1[offs+2]);
				code -= base_scroll1;
				if (!priority || colour & 0x0180)
				{
					drawgfx(bitmap,Machine->gfx[0],
					code,
					colour&0x1f,
					colour&0x20,colour&0x40,sx,sy,
					&Machine->drv->visible_area,
					TRANSPARENCY_PEN,15);
				}
			 }
			 sy+=8;
		 }
		 sx+=8;
	}
}




/***************************************************************************

								Sprites
								=======

  Sprites are represented by a number of 8 byte values

  xx xx yy yy nn nn aa aa

  where xxxx = x position
		yyyy = y position
		nnnn = tile number
		aaaa = attribute word
					0x0001        colour
					0x0002        colour
					0x0004        colour
					0x0008        colour
					0x0010        colour
					0x0020        X Flip
					0x0040        Y Flip
					0x0080        unknown
					0x0100        X block size (in sprites)
					0x0200        X block size
					0x0400        X block size
					0x0800        X block size
					0x1000        Y block size (in sprites)
					0x2000        Y block size
					0x4000        Y block size
					0x8000        Y block size

  The end of the table (may) be marked by an attribute value of 0xff00.

***************************************************************************/

INLINE void cps1_find_last_sprite(void)    /* Find the offset of last sprite */
{
	int offset=6;
	/* Locate the end of table marker */
	while (offset < cps1_obj_size)
	{
		int colour=READ_WORD(&cps1_obj[offset]);
		if (colour == 0xff00)
		{
			/* Marker found. This is the last sprite. */
			cps1_last_sprite_offset=offset-6-8;
			return;
		}
		offset+=8;
	}
	/* Sprites must use full sprite RAM */
	cps1_last_sprite_offset=cps1_obj_size-8;
}

/* Find used colours */
INLINE void cps1_palette_sprites(unsigned short *base)
{
	int i;
	int base_obj=cps1_game_config->base_obj;

	for (i=cps1_last_sprite_offset; i>=0; i-=8)
	{
		int x=READ_WORD(&cps1_obj[i]);
		int y=READ_WORD(&cps1_obj[i+2]);
		if (x && y)
		{
			int col=READ_WORD(&cps1_obj[i+6])& 0x1f;
	    base[col] |= 0x7fff;
		}
	}
}



INLINE void cps1_render_sprites(struct osd_bitmap *bitmap, int priority)
{
	int i;
	int base_obj=cps1_game_config->base_obj;    /* Start tile number for objects */
	int gng_obj_kludge=cps1_game_config->gng_sprite_kludge;
	int bank;

	/* Draw the sprites */
	for (i=cps1_last_sprite_offset; i>=0; i-=8)
	{
		int x=READ_WORD(&cps1_obj[i]);
		int y=READ_WORD(&cps1_obj[i+2]);
		if (x && y )
		{
			unsigned int code=READ_WORD(&cps1_obj[i+4]);
			int colour=READ_WORD(&cps1_obj[i+6]);
			int col=colour&0x1f;

			if (y & 0x0300)
			{
				y-=0x200;
			}

			x-=0x40;

#ifdef LAYER_DEBUG
			if (errorlog &&
				(code < base_obj ||
				 code > (base_obj+Machine->gfx[1]->total_elements)))
			{
			      fprintf(errorlog,"WARNING: Out of range object %04x\n", code);
			}
#endif


			if (code >= base_obj)
			{
				code -= base_obj;

				bank=1;
				if (gng_obj_kludge && code >= 0x01000)
				{
					code &= 0xfff;
					bank=4;
				}

				if (colour & 0xff00 )
				{
					/* handle blocked sprites */
					int nx=(colour & 0x0f00) >> 8;
					int ny=(colour & 0xf000) >> 12;
					int nxs, nys;
					nx++;
					ny++;

					if (colour & 0x40)
					{
						/* Y flip */
						if (colour &0x20)
						{
							for (nys=0; nys<ny; nys++)
							{
								for (nxs=0; nxs<nx; nxs++)
								{
									drawgfx(bitmap,Machine->gfx[bank],
										code+(nx-1)-nxs+0x10*(ny-1-nys),
										col&0x1f,
										1,1,
										x+nxs*16,y+nys*16,
										&Machine->drv->visible_area,TRANSPARENCY_PEN,15);
								}
							}
						}
						else
						{
							for (nys=0; nys<ny; nys++)
							{
								for (nxs=0; nxs<nx; nxs++)
								{
									drawgfx(bitmap,Machine->gfx[bank],
										code+nxs+0x10*(ny-1-nys),
										col&0x1f,
										0,1,
										x+nxs*16,y+nys*16,
										&Machine->drv->visible_area,TRANSPARENCY_PEN,15);
								}
							}
						}
					}
					else
					{
						if (colour &0x20)
						{
							for (nys=0; nys<ny; nys++)
							{
								for (nxs=0; nxs<nx; nxs++)
								{
									drawgfx(bitmap,Machine->gfx[bank],
										code+(nx-1)-nxs+0x10*nys,
										col&0x1f,
										1,0,
										x+nxs*16,y+nys*16,
										&Machine->drv->visible_area,TRANSPARENCY_PEN,15);
								}
							}
						}
						else
						{
							for (nys=0; nys<ny; nys++)
							{
								for (nxs=0; nxs<nx; nxs++)
								{
									drawgfx(bitmap,Machine->gfx[bank],
										code+nxs+0x10*nys,
										col&0x1f,
										0,0,
										x+nxs*16,y+nys*16,
										&Machine->drv->visible_area,TRANSPARENCY_PEN,15);
								}
							}
						}
					}
				}
				else
				{
					/* Simple case... 1 sprite */
					drawgfx(bitmap,Machine->gfx[bank],
						   code,
						   col&0x1f,
						   colour&0x20,colour&0x40,
						   x,y,
						   &Machine->drv->visible_area,
						   TRANSPARENCY_PEN,15);
				}
			}
		}
	}
}




/***************************************************************************

  Scroll 2 (16x16 layer)

  Attribute word layout:
  0x0001        colour
  0x0002        colour
  0x0004        colour
  0x0008        colour
  0x0010        colour
  0x0020        X Flip
  0x0040        Y Flip
  0x0080        ??? Priority
  0x0100        ??? Priority
  0x0200
  0x0400
  0x0800
  0x1000
  0x2000
  0x4000
  0x8000


***************************************************************************/

INLINE void cps1_render_scroll2(struct osd_bitmap *bitmap, int priority)
{
#ifdef LAYER_DEBUG
	static int s=0;
#endif
	int base_scroll2=cps1_game_config->base_scroll2;
	int space_char=cps1_game_config->space_scroll2;
	int sx, sy;
	int nxoffset=scroll2x&0x0f;    /* Smooth X */
	int nyoffset=scroll2y&0x0f;    /* Smooth Y */
	int nx=(scroll2x>>4)+4;        /* Rough X */
	int ny=(scroll2y>>4);          /* Rough Y */

	for (sx=0; sx<0x32/2+1; sx++)
	{
		for (sy=0; sy<0x09*2; sy++)
		{
			int offsy, offsx, offs, colour, code;
			int n;
			n=ny+sy;
			offsy  = ((n&0x0f)*4 | ((n&0x30)*0x100))&0x3fff;
			offsx=((nx+sx)*0x040)&0xfff;
			offs=offsy+offsx;
			offs &= 0x3fff;

			code=READ_WORD(&cps1_scroll2[offs]);
			if (code >= base_scroll2 && code != space_char )
			{
				code -= base_scroll2;
				colour=READ_WORD(&cps1_scroll2[offs+2]);
				if (priority)
				{
					int mask=colour & 0x0180;
					int transp;
					transp=cps1_transparency_scroll2[mask>>7];

					if ((transp & 0x7fff) != 0x7fff)
					{
						drawgfx(bitmap,Machine->gfx[2],
								code,
								colour&0x1f,
								colour&0x20,colour&0x40,
								16*sx-nxoffset,16*sy-nyoffset,
								&Machine->drv->visible_area,
								TRANSPARENCY_PENS,transp);
					}
				}
				else
				{
					/* Draw entire tile */
					drawgfx(bitmap,Machine->gfx[2],
						code,
						colour&0x1f,
						colour&0x20,colour&0x40,
						16*sx-nxoffset,16*sy-nyoffset,
						&Machine->drv->visible_area,
						TRANSPARENCY_PEN,15);
				}
#ifdef LAYER_DEBUG
					{

					if (osd_key_pressed(OSD_KEY_T))
					{
						while (osd_key_pressed(OSD_KEY_T));
						s=~s;
					}
					if (s)
					{
					    char szBuffer[10];
					    int x,y;
					    sprintf(szBuffer, "%x",colour&0x180>>7);
					    x = 16*sx-nxoffset;
					    y = 16*sy-nyoffset ;

					    drawgfx(bitmap,Machine->uifont,
						szBuffer[0], 1,
						0,0,x,y,0,TRANSPARENCY_NONE,0);
					    x += Machine->uifont->width;
					}
					}
#endif

			}
		}
	}
}


INLINE void cps1_palette_scroll2_distort(unsigned short *base)
{
	int base_scroll2=cps1_game_config->base_scroll2;
	int space_char=cps1_game_config->space_scroll2;
	int sx, sy;
	int nx=(scroll2x>>4)+4;        /* Rough X */
	int ny=(scroll2y>>4);          /* Rough Y */

	int elements = Machine->gfx[2]->total_elements;

	for (sx=0; sx<0x32/2+1; sx++)
	{
		int other=ny*0x20;  /* scroll2y * 2 */

		for (sy=0; sy<0x09*2; sy++)
		{
			int offsy, offsx, offs, colour, code;
			int n;
			int xdistort=READ_WORD(&cps1_other[other]);

			n=ny+sy;
			offsy  = ((n&0x0f)*4 | ((n&0x30)*0x100))&0x3fff;
			offsx=((nx+sx+(xdistort>>4))*0x040)&0xfff;
			offs=offsy+offsx;
			offs &= 0x3fff;

			code=READ_WORD(&cps1_scroll2[offs]);
			if (code >= base_scroll2 && code != space_char )
			{
				code -= base_scroll2;
				colour=READ_WORD(&cps1_scroll2[offs+2]);

				base[colour&0x1f] |= Machine->gfx[2]->pen_usage[code % elements];
			}
			other+=32;
			other&=0x1fff;
		}

	}
}

INLINE void cps1_palette_scroll2(unsigned short *base)
{
	int base_scroll2=cps1_game_config->base_scroll2;
	int space_char=cps1_game_config->space_scroll2;
	int elements = Machine->gfx[2]->total_elements;
	int offs, code, colour;
	for (offs=cps1_scroll2_size-4; offs>=0; offs-=4)
	{
	     code=READ_WORD(&cps1_scroll2[offs]);
	     if (code>=base_scroll2 )
	     {
		    code -= base_scroll2;
		    colour=READ_WORD(&cps1_scroll2[offs+2])&0x1f;
		    base[colour] |= Machine->gfx[2]->pen_usage[code % elements];
	     }
	}
}

INLINE void cps1_render_scroll2_bitmap(struct osd_bitmap *bitmap, int priority)
{
	int base_scroll2=cps1_game_config->base_scroll2;
	int space_char=cps1_game_config->space_scroll2;
	int sx, sy, scrly;
	int ny=(scroll2y>>4);          /* Rough Y */

	for (sx=0; sx<CPS1_SCROLL2_WIDTH; sx++)
	{
		int n=ny;
		for (sy=0; sy<0x09*2; sy++)
		{
			long newvalue;
			int offsy, offsx, offs, colour, code;

			n&=0x3f;
			offsy  = ((n&0x0f)*4 | ((n&0x30)*0x100))&0x3fff;
			offsx=(sx*0x040)&0xfff;
			offs=offsy+offsx;

			colour=READ_WORD(&cps1_scroll2[offs+2]);

			newvalue=*(long*)(&cps1_scroll2[offs]);
			if ( newvalue != *(long*)(&cps1_scroll2_old[offs]) )
			{
				*(long*)(&cps1_scroll2_old[offs])=newvalue;
				code=READ_WORD(&cps1_scroll2[offs]);

#ifdef LAYER_DEBUG
			if (errorlog &&  code != space_char &&
				(code < base_scroll2 ||
				 code > (base_scroll2+Machine->gfx[2]->total_elements)))
			{
			      fprintf(errorlog,"WARNING: Out of range scroll 2 character %04x\n", code);
			}
#endif

				if (code < base_scroll2)
					code=space_char;
				if (code >= base_scroll2)
				{
					code -= base_scroll2;
					/* Draw entire tile */
					drawgfx(bitmap,Machine->gfx[2],
						code,
						colour&0x1f,
						colour&0x20,colour&0x40,
						16*sx,16*n,
						NULL,
						TRANSPARENCY_NONE,
						0);
				}
			}
			n++;
		}
	}
}


INLINE void cps1_render_scroll2_low(struct osd_bitmap *bitmap, int priority)
{
      int scrly=-scroll2y;
      int scrlx=-(scroll2x+0x40);

      cps1_render_scroll2_bitmap(cps1_scroll2_bitmap, priority);

      copyscrollbitmap(bitmap,cps1_scroll2_bitmap,
	1,&scrlx,1,&scrly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
}


INLINE void cps1_render_scroll2_distort(struct osd_bitmap *bitmap, int priority)
{
      #define lines (0x40*0x10)
      int other=0;
      int scrly=-scroll2y;
      int i,scroll[lines];
      int mask;

      cps1_render_scroll2_bitmap(cps1_scroll2_bitmap, priority);

      switch (cps1_vid_cfg[cps1_game_config->alternative].distort_alt)
      {
		case 1:
		      other+=0x100;
		      for (i = 0;i < lines;i++)
		      {
			     int xdistort;
			     other&=0x1ff;
			     xdistort=READ_WORD(&cps1_other[other]);
			     other+=2;
			     scroll[i] = -(scroll2x+xdistort+0x40);
		      }
		      break;
		case 2:
		      other+=0x200;
		      for (i = 0;i < lines;i++)
		      {
			     int xdistort;
			     int n;
			     other&=0x7ff;
			     n=other;
			     /*if (other & 0x100)
			     {
				n=0x400-(other&0xff);
			     } */
			     xdistort=READ_WORD(&cps1_other[n]);
			     other+=2;
			     scroll[i]   = -(scroll2x+xdistort+0x40);
		      }
		      break;
		case 3:
		      for (i = 0;i < lines;i++)
		      {
			     int xdistort;
			     other&=0x7ff;
			     xdistort=READ_WORD(&cps1_other[other]);
			     other+=2;
			     scroll[i] = -(scroll2x+xdistort+0x40);
		      }
		      break;

		default:
		      for (i = 0;i < lines;i++)
		      {
			     scroll[i] = 0;
		      }
		      break;
      }

      copyscrollbitmap(bitmap,cps1_scroll2_bitmap,
	lines,scroll,1,&scrly,&Machine->drv->visible_area,TRANSPARENCY_PEN,palette_transparent_pen);
}


/***************************************************************************

  Scroll 3 (32x32 layer)

  Attribute word layout:
  0x0001        colour
  0x0002        colour
  0x0004        colour
  0x0008        colour
  0x0010        colour
  0x0020        X Flip
  0x0040        Y Flip
  0x0080
  0x0100
  0x0200
  0x0400
  0x0800
  0x1000
  0x2000
  0x4000
  0x8000

***************************************************************************/

INLINE void cps1_palette_scroll3(unsigned short *base)
{
	int base_scroll3=cps1_game_config->base_scroll3;
	int space_char=cps1_game_config->space_scroll3;
	int sx,sy;
	int nx=(scroll3x>>5)+2;
	int ny=(scroll3y>>5);

	int elements = Machine->gfx[3]->total_elements;

	for (sx=0; sx<0x32/4+1; sx++)
	{
		for (sy=0; sy<0x20/4+1; sy++)
		{
			int offsy, offsx, offs, colour, code;
			int n;
			n=ny+sy;
			offsy  = ((n&0x07)*4 | ((n&0xf8)*0x0100))&0x3fff;
			offsx=((nx+sx)*0x020)&0x7ff;
			offs=offsy+offsx;
			offs &= 0x3fff;
			code=READ_WORD(&cps1_scroll3[offs]);
			if (code>=base_scroll3 && code != space_char)
			{
				code -= base_scroll3;
				colour=READ_WORD(&cps1_scroll3[offs+2]);
				base[colour&0x1f] |= Machine->gfx[3]->pen_usage[code % elements] /*& 0x7fff*/;
			}
		}
	}
}


INLINE void cps1_render_scroll3(struct osd_bitmap *bitmap, int priority)
{
#ifdef LAYER_DEBUG
	static int s=0;
#endif

	int base_scroll3=cps1_game_config->base_scroll3;
	int space_char=cps1_game_config->space_scroll3;
	int sx,sy;
	/* CPS1_SCROLL 3 */
	int nxoffset=scroll3x&0x1f;
	int nyoffset=scroll3y&0x1f;
	int nx=(scroll3x>>5)+2;
	int ny=(scroll3y>>5);

	for (sx=0; sx<0x32/4+1; sx++)
	{
		for (sy=0; sy<0x20/4+1; sy++)
		{
			int offsy, offsx, offs, colour, code, transp;
			int n;
			n=ny+sy;
			offsy  = ((n&0x07)*4 | ((n&0xf8)*0x0100))&0x3fff;
			offsx=((nx+sx)*0x020)&0x7ff;
			offs=offsy+offsx;
			offs &= 0x3fff;
			code=READ_WORD(&cps1_scroll3[offs]);

#ifdef LAYER_DEBUG
			if (errorlog && code != space_char &&
				(code < base_scroll3 ||
				 code > (base_scroll3+Machine->gfx[3]->total_elements)))
			{
			      fprintf(errorlog,"WARNING: Out of range scroll 3 character %04x\n", code);
			}
#endif

			if (code>=base_scroll3 && code != space_char)
			{
				code -= base_scroll3;
				colour=READ_WORD(&cps1_scroll3[offs+2]);
				if (priority)
				{
					int mask=colour & 0x0180;
					if (mask && !(colour & 0xf000))
					{
						transp=cps1_transparency_scroll3[mask>>7];
						drawgfx(bitmap,Machine->gfx[3],
								code,
								colour&0x1f,
								colour&0x20,colour&0x40,
								32*sx-nxoffset,32*sy-nyoffset,
								&Machine->drv->visible_area,
								TRANSPARENCY_PENS,transp);
					}
				}
				else
				{

					drawgfx(bitmap,Machine->gfx[3],
						code,
						colour&0x1f,
						colour&0x20,colour&0x40,
						32*sx-nxoffset,32*sy-nyoffset,
						&Machine->drv->visible_area,
						TRANSPARENCY_PEN,15);

				}
#ifdef LAYER_DEBUG


					if (osd_key_pressed(OSD_KEY_U))
					{
						while (osd_key_pressed(OSD_KEY_U));
						s=~s;
					}
					if (s)
					{
					    char szBuffer[10];
					    int x,y;
					    sprintf(szBuffer, "%x",(colour>>7)&0x03);
					    x = 32*sx-nxoffset;
					    y = 32*sy-nyoffset ;

					    drawgfx(bitmap,Machine->uifont,
						szBuffer[0], 1,
						0,0,x,y,0,TRANSPARENCY_NONE,0);
					    x += Machine->uifont->width;


					}


#endif

			}
		}
	}
}


INLINE void cps1_render_layer(struct osd_bitmap *bitmap, int layer, int distort)
{
	if (cps1_layer_enabled[layer])
	{
		switch (layer)
		{
			case 0:
				cps1_render_sprites(bitmap, 0);
				break;
			case 1:
				cps1_render_scroll1(bitmap, 0);
				break;
			case 2:
				if (distort)
					cps1_render_scroll2_distort(bitmap, 0);
				else
					cps1_render_scroll2_low(bitmap, 0);
				break;
			case 3:
				cps1_render_scroll3(bitmap, 0);
				break;
		}
	}
}

INLINE void cps1_render_high_layer(struct osd_bitmap *bitmap, int layer)
{
	if (cps1_layer_enabled[layer])
	{
		switch (layer)
		{
			case 0:
				//cps1_render_sprites(bitmap);
			       break;
			case 1:
				//cps1_render_scroll1(bitmap, 0);
				break;
			case 2:
				cps1_render_scroll2(bitmap, 1);
				break;
			case 3:
				cps1_render_scroll3(bitmap, 1);
				break;
		}
	}
}


/***************************************************************************

	Refresh screen

***************************************************************************/

void cps1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	unsigned short palette_usage[cps1_palette_entries];
	int layercontrol;
	int scrl1on, scrl2on, scrl3on;
	int i,offset;
	int distort_scroll2=0;
	int layer;

	layercontrol=READ_WORD(&cps1_output[cps1_layer_control]);
	scrl1on=1;
	scrl2on=1;
	scrl3on=1;
	distort_scroll2=layercontrol & 0x01;

	/* Special cases for scroll on / off */
	switch (cps1_game_config->alternative)
	{
		case  2:
		case  9:
			scrl1on=layercontrol&0x02;
			scrl2on=layercontrol&0x04;
			scrl3on=layercontrol&0x08;
			distort_scroll2=layercontrol & 0x01;
			break;

		case  4: /* Strider */
		case  5: /* Ghouls and Ghosts */
			scrl1on=layercontrol&0x02;
			scrl2on=layercontrol&0x04;
			scrl3on=layercontrol&0x08;
			break;

		case  7: /* Magic Sword */
			distort_scroll2=layercontrol & 0x01;
			break;

		case 12: /* Street Fighter 2 */
		case 13: /* Street Fighter 2 (Jap )*/
		case 14: /* Street Fighter 2 (turbo) */
		case 15: /* Street Fighter 2 (Rev E ) */
			distort_scroll2=1;
			break;

		case  19: /* 1941 */
			scrl3on=layercontrol&0x20;
			break;

		default:
			break;
	}

	/* Get video memory base registers */
	cps1_get_video_base();

	/* Find the offset of the last sprite in the sprite table */
	cps1_find_last_sprite();

	/* Build palette */
	cps1_build_palette();

	/* Compute the used portion of the palette */
	memset (palette_usage, 0, sizeof (palette_usage));
	cps1_palette_sprites (&palette_usage[cps1_obj_palette]);
	if (scrl1on)
		cps1_palette_scroll1 (&palette_usage[cps1_scroll1_palette]);
	if (scrl2on)
		cps1_palette_scroll2 (&palette_usage[cps1_scroll2_palette]);
	if (scrl3on)
		cps1_palette_scroll3 (&palette_usage[cps1_scroll3_palette]);

	for (i = offset = 0; i < cps1_palette_entries; i++)
	{
		int usage = palette_usage[i];
		if (usage)
		{
			int j;
			for (j = 0; j < 15; j++)
			{
				if (usage & (1 << j))
					palette_used_colors[offset++] = PALETTE_COLOR_USED;
				else
					palette_used_colors[offset++] = PALETTE_COLOR_UNUSED;
			}
			palette_used_colors[offset++] = PALETTE_COLOR_TRANSPARENT;
		}
		else
		{
			memset (&palette_used_colors[offset], PALETTE_COLOR_UNUSED, 16);
			offset += 16;
		}
	}

	if (palette_recalc ())
	{
		/* Mark all of scroll 2 as dirty */
		memset(cps1_scroll2_old, 0xff, cps1_scroll2_size);
	}

	/* Blank screen */
	fillbitmap(bitmap,palette_transparent_pen,&Machine->drv->visible_area);

	cps1_layer_enabled[0]=1;
	cps1_layer_enabled[1]=scrl1on;
	cps1_layer_enabled[2]=scrl2on;
	cps1_layer_enabled[3]=scrl3on;

	/* Draw layers */
	cps1_render_layer(bitmap, (layercontrol>>0x06)&03, distort_scroll2);
	cps1_render_layer(bitmap, (layercontrol>>0x08)&03, distort_scroll2);
	cps1_render_layer(bitmap, (layercontrol>>0x0a)&03, distort_scroll2);
	layer=(layercontrol>>0x0c)&03;
	if (layer != 1)
	{
		/*
		Don't draw layer 1 if it is the highest priority later
		We will draw it later.
		*/
		cps1_render_layer(bitmap, (layercontrol>>0x0c)&03, distort_scroll2);
	}

	/* Draw high priority layers */
	cps1_render_high_layer(bitmap, (layercontrol>>0x06)&03);
	cps1_render_high_layer(bitmap, (layercontrol>>0x08)&03);
	cps1_render_high_layer(bitmap, (layercontrol>>0x0a)&03);
	if (layer==1)
	{
		/* High priority sprites */
		//cps1_render_sprites(bitmap, 0x80);
		/*
		Scroll 1 is highest priority. Must draw it over high
		priority scroll parts of 2 and 3. This is correct
		behaviour since Magic Sword interludes do not
		look correct unless we do this.
		*/
		if (scrl1on)
			cps1_render_scroll1(bitmap, 0);
	}
	else
	{
		cps1_render_high_layer(bitmap, layer );
		/* High priority sprites */
		//cps1_render_sprites(bitmap, 0x80);
	}
	/* Render really high priority chars */
	cps1_render_scroll1(bitmap, 1);
}

