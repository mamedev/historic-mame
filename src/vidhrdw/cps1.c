/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

  Todo:
  Scroll 2 to scroll 3 priority (not quite right in Strider).
  Layer-sprite priority (almost there... masking not correct)
  Loads of unknown attribute bits on scroll 2.
  Entire unknown graphic RAM region
  Speed, speed, speed...

  OUTPUT PORTS (preliminary)
  0x00-0x01     OBJ RAM base (/256)
  0x02-0x03     Scroll1 RAM base (/256)
  0x04-0x05     Scroll2 RAM base (/256)
  0x06-0x07     Scroll3 RAM base (/256)
  0x08-0x09     ??? Base address of something (this could be important) ???
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

  0x66-0x67     Video control register (location varies according to game)
  0x6a-0x6b     ????
  0x6c-0x6d     ????
  0x6e-0x6f     ????
  0x70-0x72     ????


  0x80-0x81     ????            Sound output.
  0x88-0x89     ????            Port thingy (sound fade???)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "drivers/cps1.h"

#include "osdepend.h"

/* Public variables */
unsigned char *cps1_gfxram;
unsigned char *cps1_output;
int cps1_gfxram_size;
int cps1_output_size;

/* Private */
const int cps1_scroll1_size=0x4000;
const int cps1_scroll2_size=0x4000;
const int cps1_scroll3_size=0x4000;
const int cps1_obj_size    =0x4000;
const int cps1_unknown_size=0x4000;
const int cps1_palette_size=0x1000;

static unsigned char *cps1_scroll1;
static unsigned char *cps1_scroll2;
static unsigned char *cps1_scroll3;
static unsigned char *cps1_obj;
static unsigned char *cps1_palette;
static unsigned char *cps1_unknown;
static unsigned char *cps1_old_palette;

int scroll1x, scroll1y, scroll2x, scroll2y, scroll3x, scroll3y;
struct CPS1config *cps1_game_config;

/* Output ports */
#define CPS1_OBJ_BASE           0x00    /* Base address of objects */
#define CPS1_SCROLL1_BASE       0x02    /* Base address of scroll 1 */
#define CPS1_SCROLL2_BASE       0x04    /* Base address of scroll 2 */
#define CPS1_SCROLL3_BASE       0x06    /* Base address of scroll 3 */
#define CPS1_UNKNOWN_BASE       0x08    /* Base address of unknown video */
#define CPS1_PALETTE_BASE       0x0a    /* Base address of palette */
#define CPS1_SCROLL1_SCROLLX    0x0c    /* Scroll 1 X */
#define CPS1_SCROLL1_SCROLLY    0x0e    /* Scroll 1 Y */
#define CPS1_SCROLL2_SCROLLX    0x10    /* Scroll 2 X */
#define CPS1_SCROLL2_SCROLLY    0x12    /* Scroll 2 Y */
#define CPS1_SCROLL3_SCROLLX    0x14    /* Scroll 3 X */
#define CPS1_SCROLL3_SCROLLY    0x16    /* Scroll 3 Y */

#define CPS1_LAYER_CONTROL      0x66    /* (or maybe not) */

INLINE int cps1_port(int offset)
{
	return READ_WORD(&cps1_output[offset]);
}

INLINE unsigned char * cps1_base(int offset)
{
	int base=cps1_port(offset)*256;
	return &cps1_gfxram[base&0x3ffff];
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

	fp=fopen("UNKNOWN.DMP", "w+b");
	if (fp)
	{
		fwrite(cps1_unknown, cps1_unknown_size, 1, fp);
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
}
#endif


INLINE void cps1_get_video_base(void)
{
	/* Re-calculate the VIDEO RAM base */
	cps1_scroll1=cps1_base(CPS1_SCROLL1_BASE);
	cps1_scroll2=cps1_base(CPS1_SCROLL2_BASE);
	cps1_scroll3=cps1_base(CPS1_SCROLL3_BASE);
	cps1_obj    =cps1_base(CPS1_OBJ_BASE);
	cps1_palette=cps1_base(CPS1_PALETTE_BASE);
	cps1_unknown=cps1_base(CPS1_UNKNOWN_BASE);
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int cps1_vh_start(void)
{
	cps1_old_palette=(unsigned char *)malloc(cps1_palette_size);
	if (!cps1_old_palette)
	{
		return -1;
	}
	memset(cps1_old_palette, 0xff, cps1_palette_size);

	memset(cps1_gfxram, 0, cps1_gfxram_size);   /* Clear GFX RAM */
	memset(cps1_output, 0, cps1_output_size);   /* Clear output ports */

	cps1_get_video_base();   /* Calculate base pointers */

	if (!cps1_game_config)
	{
		if (errorlog)
		{
			fprintf(errorlog, "cps1_game_config hasn't been set up yet");
		}
		return -1;
	}

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

	return 0;
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void cps1_vh_stop(void)
{
	if (cps1_old_palette)
		free(cps1_old_palette);
}

/***************************************************************************

  Build palette from palette RAM

  12 bit RGB with a 4 bit brightness value.

***************************************************************************/

INLINE void cps1_build_palette(void)
{
	int offset;

	for (offset = 0; offset < 32*16*4; offset++)
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

			code=READ_WORD(&cps1_scroll1[offs])&0x0fff;

			if (code >= base_scroll1 && code != space_char)
			{
				/* Optimization: only draw non-spaces */
				int colour=READ_WORD(&cps1_scroll1[offs+2]);
				code -= base_scroll1;
				base[colour&0x1f] |= Machine->gfx[0]->pen_usage[code % elements] & 0x7fff;
			}
		}
	}
}

INLINE void cps1_render_scroll1(struct osd_bitmap *bitmap)
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

			code=READ_WORD(&cps1_scroll1[offs])&0x0fff;

			if (code >= base_scroll1 && code != space_char)
			{
				/* Optimization: only draw non-spaces */
				int colour=READ_WORD(&cps1_scroll1[offs+2]);
				code -= base_scroll1;
				drawgfx(bitmap,Machine->gfx[0],
						code,
						colour&0x1f,
						colour&0x20,colour&0x40,sx,sy,
						&Machine->drv->visible_area,
						TRANSPARENCY_PEN,15);
			 }
			 sy+=8;
		 }
		 sx+=8;
	}
}

/***************************************************************************

  Sprites

  Attribute word layout:
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


***************************************************************************/

INLINE void cps1_palette_sprites(unsigned short *base)
{
	int i;
	int base_obj=cps1_game_config->base_obj;
	int obj_size=cps1_obj_size;

	int gng_obj_kludge=cps1_game_config->gng_sprite_kludge;
	int bank;
	if (cps1_game_config->size_obj)
	{
		obj_size=cps1_game_config->size_obj;
	}

	if (gng_obj_kludge)
	{
		/*
		Ghouls and Ghosts splits the sprites over two separate
		graphics layouts */
		bank=4;
	}

	/* Draw the sprites */
	for (i=obj_size-8; i>=0; i-=8)
	{
		int x=READ_WORD(&cps1_obj[i]);
		int y=READ_WORD(&cps1_obj[i+2]);
		if (x && y)
		{
			int code=READ_WORD(&cps1_obj[i+4]);
			int colour=READ_WORD(&cps1_obj[i+6]);
			int col=colour&0x1f;
			x-=0x40;
			code &= 0x3fff;
			if (code >= base_obj)
			{
				base[col] |= 0x7fff;
			}
		}
	}
}

INLINE void cps1_render_sprites(struct osd_bitmap *bitmap)
{
	int i;
	int base_obj=cps1_game_config->base_obj;
	int obj_size=cps1_obj_size;

	int gng_obj_kludge=cps1_game_config->gng_sprite_kludge;
	int bank;
	if (cps1_game_config->size_obj)
	{
		obj_size=cps1_game_config->size_obj;
	}

	if (gng_obj_kludge)
	{
		/*
		Ghouls and Ghosts splits the sprites over two separate
		graphics layouts */
		bank=4;
	}

	/* Draw the sprites */
	for (i=obj_size-8; i>=0; i-=8)
	{
		int x=READ_WORD(&cps1_obj[i]);
		int y=READ_WORD(&cps1_obj[i+2]);
		if (x && y)
		{
			int code=READ_WORD(&cps1_obj[i+4]);
			int colour=READ_WORD(&cps1_obj[i+6]);
			int col=colour&0x1f;
			x-=0x40;
			code &= 0x3fff;
			if (code >= base_obj)
			{
				code -= base_obj;
				bank=1;
				if (gng_obj_kludge && code >= 0x01000)
				{
					code &= 0xfff;
					bank=4;
				}

				if (colour & 0xff00)
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

#if 0
void cps1_debug(int colour)
{
	static int s;
	static int reset;
	int pressed=0;
	int n=0;
	int i;
	int keys[]={OSD_KEY_1, OSD_KEY_2, OSD_KEY_3, OSD_KEY_4,
	            OSD_KEY_5, OSD_KEY_6, OSD_KEY_7, OSD_KEY_8};

	for (i=0; i<8; i++)
	{
		if (osd_key_pressed(i))
		{
			int no=0;
			if (osd_key_pressed(OSD_KEY_LSHIFT))
			{
				no+=8;
			}
			s++;
//			setgfxcolorentry (Machine->gfx[2], colour*16+i-1+no, s,s,s);
			reset=1;
			pressed=1;
		}
	}
	if (!pressed)
	{
		if (reset)
		{
			reset=0;
			memset(cps1_old_palette, 0xff, cps1_palette_size);
			cps1_build_palette();
		}
	}
}
#endif

/*
This is a test table.
*/

int cps1_transparency_table[512]=
{
   0x0000,                                                /* 0000 */
   ~(0x0010|0x0008|0x0004|0x0002|0x0001),                 /* 0080 */
   ~(0x0020|0x0010|0x0008),                               /* 0100 */
   ~(0x1000|0x0800|0x0400|0x0200|0x0080|0x0040|0x0002),   /* 0180 .??X???? ??....?. */
   0x8000,                                                /* 0200 */
   0x8000,      /* 0280 */
   0x8000,      /* 0300 */
   0x8000,      /* 0380 */
   0x8000,      /* 0400 */
   0x8000,      /* 0480 */
   0x8000,      /* 0500 */
   0x8000,      /* 0580 */
   0x8000,      /* 0600 */
   0x8000,      /* 0680 */
   0x8000,      /* 0700 */
   0x8000,      /* 0780 */
   0x8000,      /* 0800 */
   0x8000,      /* 0880 */
   ~(0x20|0x0010|0x0008|0x0004|0x0002),      /* 0900 - Willow forest floor */
   0x8000,      /* 0980 */
   0x8000,      /* 0a00 */
   0x8000,      /* 0a80 */
   0x8000,      /* 0b00 */
   0x8000,      /* 0b80 */
   0x8000,      /* 0c00 */
   0x8000,      /* 0c80 */
   0x8000,      /* 0d00 */
   0x8000,      /* 0d80 */
   0x8000,      /* 0e00 */
   0x8000,      /* 0e80 */
   0x8000,      /* 0f00 */
   0x8000,      /* 0f80 */
   0x8000,      /* 1000 */
   0x8000,      /* 1080 */
   0x8000,      /* 1100 */
   0x8000,      /* 1180 */
   0x8000,      /* 1200 */
   0x8000,      /* 1280 */
   0x8000,      /* 1300 */
   0x8000,      /* 1380 */
   0x8000,      /* 1400 */
   0x8000,      /* 1480 */
   0x8000,      /* 1500 */
   0x8000,      /* 1580 */
   0x8000,      /* 1600 */
   0x8000,      /* 1680 */
   0x8000,      /* 1700 */
   0x8000,      /* 1780 */
   0x8000,      /* 1800 */
   0x0000,      /* 1880 */
   0x8000,      /* 1900 */
   0x8000,      /* 1980 */
   0x8000,      /* 1a00 */
   0x8000,      /* 1a80 */
   0x8000,      /* 1b00 */
   0x8000,      /* 1b80 */
   0x8000,      /* 1c00 */
   0x8000,      /* 1c80 */
   0x8000,      /* 1d00 */
   0x8000,      /* 1d80 */
   0x8000,      /* 1e00 */
   0x8000,      /* 1e80 */
   0x8000,      /* 1f00 */
   0x8000,      /* 1f80 */
   0x8000,      /* 2000 */
   0x8000,      /* 2080 */
   0x8000,      /* 2100 */
   0x8000,      /* 2180 */
   0x8000,      /* 2200 */
   0x8000,      /* 2280 */
   0x8000,      /* 2300 */
   0x8000,      /* 2380 */
   0x8000,      /* 2400 */
   0x8000,      /* 2480 */
   0x8000,      /* 2500 */
   0x8000,      /* 2580 */
   0x8000,      /* 2600 */
   0x8000,      /* 2680 */
   0x8000,      /* 2700 */
   0x8000,      /* 2780 */
   0x8000,      /* 2800 */
   0x8000,      /* 2880 */
   0x8000,      /* 2900 */
   0x8000,      /* 2980 */
   0x8000,      /* 2a00 */
   0x8000,      /* 2a80 */
   0x8000,      /* 2b00 */
   0x8000,      /* 2b80 */
   0x8000,      /* 2c00 */
   0x8000,      /* 2c80 */
   0x8000,      /* 2d00 */
   0x8000,      /* 2d80 */
   0x8000,      /* 2e00 */
   0x8000,      /* 2e80 */
   0x8000,      /* 2f00 */
   0x8000,      /* 2f80 */
   0x8000,      /* 3000 */
   0x8000,      /* 3080 */
   0x8000,      /* 3100 */
   0x8000,      /* 3180 */
   0x8000,      /* 3200 */
   0x8000,      /* 3280 */
   0x8000,      /* 3300 */
   0x8000,      /* 3380 */
   0x8000,      /* 3400 */
   0x8000,      /* 3480 */
   0x8000,      /* 3500 */
   0x8000,      /* 3580 */
   0x8000,      /* 3600 */
   0x8000,      /* 3680 */
   0x8000,      /* 3700 */
   0x8000,      /* 3780 */
   0x8000,      /* 3800 */
   0x8000,      /* 3880 */
   0x8000,      /* 3900 */
   0x8000,      /* 3980 */
   0x8000,      /* 3a00 */
   0x8000,      /* 3a80 */
   0x8000,      /* 3b00 */
   0x8000,      /* 3b80 */
   0x8000,      /* 3c00 */
   0x8000,      /* 3c80 */
   0x8000,      /* 3d00 */
   0x8000,      /* 3d80 */
   0x8000,      /* 3e00 */
   0x8000,      /* 3e80 */
   0x8000,      /* 3f00 */
   0x8000,      /* 3f80 */
   0x8000,      /* 4000 */
   0x8000,      /* 4080 */
   0x8000,      /* 4100 */
   0x8000,      /* 4180 */
   0x8000,      /* 4200 */
   0x8000,      /* 4280 */
   0x8000,      /* 4300 */
   0x8000,      /* 4380 */
   0x8000,      /* 4400 */
   0x8000,      /* 4480 */
   0x8000,      /* 4500 */
   0x8000,      /* 4580 */
   0x8000,      /* 4600 */
   0x8000,      /* 4680 */
   0x8000,      /* 4700 */
   0x8000,      /* 4780 */
   0x8000,      /* 4800 */
   0x8000,      /* 4880 */
   0x8000,      /* 4900 */
   0x8000,      /* 4980 */
   0x8000,      /* 4a00 */
   0x8000,      /* 4a80 */
   0x8000,      /* 4b00 */
   0x8000,      /* 4b80 */
   0x8000,      /* 4c00 */
   0x8000,      /* 4c80 */
   0x8000,      /* 4d00 */
   0x8000,      /* 4d80 */
   0x8000,      /* 4e00 */
   0x8000,      /* 4e80 */
   0x8000,      /* 4f00 */
   0x8000,      /* 4f80 */
   0x8000,      /* 5000 */
   0x8000,      /* 5080 */
   0x8000,      /* 5100 */
   0x8000,      /* 5180 */
   0x8000,      /* 5200 */
   0x8000,      /* 5280 */
   0x8000,      /* 5300 */
   0x8000,      /* 5380 */
   0x8000,      /* 5400 */
   0x8000,      /* 5480 */
   0x8000,      /* 5500 */
   0x8000,      /* 5580 */
   0x8000,      /* 5600 */
   0x8000,      /* 5680 */
   0x8000,      /* 5700 */
   0x8000,      /* 5780 */
   0x8000,      /* 5800 */
   0x8000,      /* 5880 */
   0x8000,      /* 5900 */
   0x8000,      /* 5980 */
   0x8000,      /* 5a00 */
   0x8000,      /* 5a80 */
   0x8000,      /* 5b00 */
   0x8000,      /* 5b80 */
   0x8000,      /* 5c00 */
   0x8000,      /* 5c80 */
   0x8000,      /* 5d00 */
   0x8000,      /* 5d80 */
   0x8000,      /* 5e00 */
   0x8000,      /* 5e80 */
   0x8000,      /* 5f00 */
   0x8000,      /* 5f80 */
   0x8000,      /* 6000 */
   0x8000,      /* 6080 */
   0x8000,      /* 6100 */
   0x8000,      /* 6180 */
   0x8000,      /* 6200 */
   0x8000,      /* 6280 */
   0x8000,      /* 6300 */
   0x8000,      /* 6380 */
   0x8000,      /* 6400 */
   0x8000,      /* 6480 */
   0x8000,      /* 6500 */
   0x8000,      /* 6580 */
   0x8000,      /* 6600 */
   0x8000,      /* 6680 */
   0x8000,      /* 6700 */
   0x8000,      /* 6780 */
   0x8000,      /* 6800 */
   0x8000,      /* 6880 */
   0x8000,      /* 6900 */
   0x8000,      /* 6980 */
   0x8000,      /* 6a00 */
   0x8000,      /* 6a80 */
   0x8000,      /* 6b00 */
   0x8000,      /* 6b80 */
   0x8000,      /* 6c00 */
   0x8000,      /* 6c80 */
   0x8000,      /* 6d00 */
   0x8000,      /* 6d80 */
   0x8000,      /* 6e00 */
   0x8000,      /* 6e80 */
   0x8000,      /* 6f00 */
   0x8000,      /* 6f80 */
   0x8000,      /* 7000 */
   0x8000,      /* 7080 */
   0x8000,      /* 7100 */
   0x8000,      /* 7180 */
   0x8000,      /* 7200 */
   0x8000,      /* 7280 */
   0x8000,      /* 7300 */
   0x8000,      /* 7380 */
   0x8000,      /* 7400 */
   0x8000,      /* 7480 */
   0x8000,      /* 7500 */
   0x8000,      /* 7580 */
   0x8000,      /* 7600 */
   0x8000,      /* 7680 */
   0x8000,      /* 7700 */
   0x8000,      /* 7780 */
   0x8000,      /* 7800 */
   0x8000,      /* 7880 */
   0x8000,      /* 7900 */
   0x8000,      /* 7980 */
   0x8000,      /* 7a00 */
   0x8000,      /* 7a80 */
   0x8000,      /* 7b00 */
   0x8000,      /* 7b80 */
   0x8000,      /* 7c00 */
   0x8000,      /* 7c80 */
   0x8000,      /* 7d00 */
   0x8000,      /* 7d80 */
   0x8000,      /* 7e00 */
   0x8000,      /* 7e80 */
   0x8000,      /* 7f00 */
   0x8000,      /* 7f80 */
   0x8000,      /* 8000 */
   0x8000,      /* 8080 */
   0x8000,      /* 8100 */
   0x0000,      /* 8180 - Un Squadron bottom of screen */
   0x8000,      /* 8200 */
   0x8000,      /* 8280 */
   0x8000,      /* 8300 */
   0x8000,      /* 8380 */
   0x8000,      /* 8400 */
   0x8000,      /* 8480 */
   0x8000,      /* 8500 */
   0x8000,      /* 8580 */
   0x8000,      /* 8600 */
   0x8000,      /* 8680 */
   0x8000,      /* 8700 */
   0x8000,      /* 8780 */
   0x8000,      /* 8800 */
   0x8000,      /* 8880 */
   0x8000,      /* 8900 */
   0x8000,      /* 8980 */
   0x8000,      /* 8a00 */
   0x8000,      /* 8a80 */
   0x8000,      /* 8b00 */
   0x8000,      /* 8b80 */
   0x8000,      /* 8c00 */
   0x8000,      /* 8c80 */
   0x8000,      /* 8d00 */
   0x8000,      /* 8d80 */
   0x8000,      /* 8e00 */
   0x8000,      /* 8e80 */
   0x8000,      /* 8f00 */
   0x8000,      /* 8f80 */
   0x8000,      /* 9000 */
   0x8000,      /* 9080 */
   0x8000,      /* 9100 */
   0x8000,      /* 9180 */
   0x8000,      /* 9200 */
   0x8000,      /* 9280 */
   0x8000,      /* 9300 */
   0x8000,      /* 9380 */
   0x8000,      /* 9400 */
   0x8000,      /* 9480 */
   0x8000,      /* 9500 */
   0x8000,      /* 9580 */
   0x8000,      /* 9600 */
   0x8000,      /* 9680 */
   0x8000,      /* 9700 */
   0x8000,      /* 9780 */
   0x8000,      /* 9800 */
   0x8000,      /* 9880 */
   0x8000,      /* 9900 */
   0x8000,      /* 9980 */
   0x8000,      /* 9a00 */
   0x8000,      /* 9a80 */
   0x8000,      /* 9b00 */
   0x8000,      /* 9b80 */
   0x8000,      /* 9c00 */
   0x8000,      /* 9c80 */
   0x8000,      /* 9d00 */
   0x8000,      /* 9d80 */
   0x8000,      /* 9e00 */
   0x8000,      /* 9e80 */
   0x8000,      /* 9f00 */
   0x8000,      /* 9f80 */
   0x8000,      /* a000 */
   0x0000,      /* a080 */
   0x8000,      /* a100 */
   0x0000,      /* a180 */
   0x8000,      /* a200 */
   0x8000,      /* a280 */
   0x8000,      /* a300 */
   0x8000,      /* a380 */
   0x8000,      /* a400 */
   0x8000,      /* a480 */
   0x8000,      /* a500 */
   0x8000,      /* a580 */
   0x8000,      /* a600 */
   0x8000,      /* a680 */
   0x8000,      /* a700 */
   0x8000,      /* a780 */
   0x8000,      /* a800 */
   0x8000,      /* a880 */
   0x8000,      /* a900 */
   0x8000,      /* a980 */
   0x8000,      /* aa00 */
   0x8000,      /* aa80 */
   0x8000,      /* ab00 */
   0x8000,      /* ab80 */
   0x8000,      /* ac00 */
   0x8000,      /* ac80 */
   0x8000,      /* ad00 */
   0x8000,      /* ad80 */
   0x8000,      /* ae00 */
   0x8000,      /* ae80 */
   0x8000,      /* af00 */
   0x8000,      /* af80 */
   0x8000,      /* b000 */
   0x8000,      /* b080 */
   0x8000,      /* b100 */
   0x8000,      /* b180 */
   0x8000,      /* b200 */
   0x8000,      /* b280 */
   0x8000,      /* b300 */
   0x8000,      /* b380 */
   0x8000,      /* b400 */
   0x8000,      /* b480 */
   0x8000,      /* b500 */
   0x8000,      /* b580 */
   0x8000,      /* b600 */
   0x8000,      /* b680 */
   0x8000,      /* b700 */
   0x8000,      /* b780 */
   0x8000,      /* b800 */
   0x8000,      /* b880 */
   0x8000,      /* b900 */
   0x8000,      /* b980 */
   0x8000,      /* ba00 */
   0x8000,      /* ba80 */
   0x8000,      /* bb00 */
   0x8000,      /* bb80 */
   0x8000,      /* bc00 */
   0x8000,      /* bc80 */
   0x8000,      /* bd00 */
   0x8000,      /* bd80 */
   0x8000,      /* be00 */
   0x8000,      /* be80 */
   0x8000,      /* bf00 */
   0x8000,      /* bf80 */
   0x8000,      /* c000 */
   0x8000,      /* c080 */
   0x8000,      /* c100 */
   0x8000,      /* c180 */
   0x8000,      /* c200 */
   0x8000,      /* c280 */
   0x8000,      /* c300 */
   0x8000,      /* c380 */
   0x8000,      /* c400 */
   0x8000,      /* c480 */
   0x8000,      /* c500 */
   0x8000,      /* c580 */
   0x8000,      /* c600 */
   0x8000,      /* c680 */
   0x8000,      /* c700 */
   0x8000,      /* c780 */
   0x8000,      /* c800 */
   0x8000,      /* c880 */
   0x8000,      /* c900 */
   0x8000,      /* c980 */
   0x8000,      /* ca00 */
   0x8000,      /* ca80 */
   0x8000,      /* cb00 */
   0x8000,      /* cb80 */
   0x8000,      /* cc00 */
   0x8000,      /* cc80 */
   0x8000,      /* cd00 */
   0x8000,      /* cd80 */
   0x8000,      /* ce00 */
   0x8000,      /* ce80 */
   0x8000,      /* cf00 */
   0x8000,      /* cf80 */
   0x8000,      /* d000 */
   0x8000,      /* d080 */
   0x8000,      /* d100 */
   0x8000,      /* d180 */
   0x8000,      /* d200 */
   0x8000,      /* d280 */
   0x8000,      /* d300 */
   0x8000,      /* d380 */
   0x8000,      /* d400 */
   0x8000,      /* d480 */
   0x8000,      /* d500 */
   0x8000,      /* d580 */
   0x8000,      /* d600 */
   0x8000,      /* d680 */
   0x8000,      /* d700 */
   0x8000,      /* d780 */
   0x8000,      /* d800 */
   0x8000,      /* d880 */
   0x8000,      /* d900 */
   0x8000,      /* d980 */
   0x8000,      /* da00 */
   0x8000,      /* da80 */
   0x8000,      /* db00 */
   0x8000,      /* db80 */
   0x8000,      /* dc00 */
   0x8000,      /* dc80 */
   0x8000,      /* dd00 */
   0x8000,      /* dd80 */
   0x8000,      /* de00 */
   0x8000,      /* de80 */
   0x8000,      /* df00 */
   0x8000,      /* df80 */
   0x8000,      /* e000 */
   0x8000,      /* e080 */
   0x8000,      /* e100 */
   0x8000,      /* e180 */
   0x8000,      /* e200 */
   0x8000,      /* e280 */
   0x8000,      /* e300 */
   0x8000,      /* e380 */
   0x8000,      /* e400 */
   0x8000,      /* e480 */
   0x8000,      /* e500 */
   0x8000,      /* e580 */
   0x8000,      /* e600 */
   0x8000,      /* e680 */
   0x8000,      /* e700 */
   0x8000,      /* e780 */
   0x8000,      /* e800 */
   0x8000,      /* e880 */
   0x8000,      /* e900 */
   0x8000,      /* e980 */
   0x8000,      /* ea00 */
   0x8000,      /* ea80 */
   0x8000,      /* eb00 */
   0x8000,      /* eb80 */
   0x8000,      /* ec00 */
   0x8000,      /* ec80 */
   0x8000,      /* ed00 */
   0x8000,      /* ed80 */
   0x8000,      /* ee00 */
   0x8000,      /* ee80 */
   0x8000,      /* ef00 */
   0x8000,      /* ef80 */
   0x8000,      /* f000 */
   0x8000,      /* f080 */
   0x8000,      /* f100 */
   0x8000,      /* f180 */
   0x8000,      /* f200 */
   0x8000,      /* f280 */
   0x8000,      /* f300 */
   0x8000,      /* f380 */
   0x8000,      /* f400 */
   0x8000,      /* f480 */
   0x8000,      /* f500 */
   0x8000,      /* f580 */
   0x8000,      /* f600 */
   0x8000,      /* f680 */
   0x8000,      /* f700 */
   0x8000,      /* f780 */
   0x8000,      /* f800 */
   0x8000,      /* f880 */
   0x8000,      /* f900 */
   0x8000,      /* f980 */
   0x8000,      /* fa00 */
   0x8000,      /* fa80 */
   0x8000,      /* fb00 */
   0x8000,      /* fb80 */
   0x8000,      /* fc00 */
   0x8000,      /* fc80 */
   0x8000,      /* fd00 */
   0x8000,      /* fd80 */
   0x8000,      /* fe00 */
   0x8000,      /* fe80 */
   0x8000,      /* ff00 */
   0x8000,      /* ff80 */

};



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


INLINE void cps1_palette_scroll2(unsigned short *base)
{
	int base_scroll2=cps1_game_config->base_scroll2;
	int space_char=cps1_game_config->space_scroll2;
	int sx, sy;
	int nx=(scroll2x>>4)+4;        /* Rough X */
	int ny=(scroll2y>>4);          /* Rough Y */

	int elements = Machine->gfx[2]->total_elements;

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

				base[colour&0x1f] |= Machine->gfx[2]->pen_usage[code % elements];
			}
		}
	}
}


INLINE void cps1_render_scroll2(struct osd_bitmap *bitmap, int priority)
{
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

					if (mask)
					{

						int transp=0;
#if 0
						cps1_debug(colour & 0x1f);
						if (errorlog && osd_key_pressed(OSD_KEY_L))
						{
							fprintf(errorlog, "%04x\n", colour);
						}
#endif
						transp=cps1_transparency_table[colour>>7];

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
			}
		}
	}
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
				base[colour&0x1f] |= Machine->gfx[3]->pen_usage[code % elements] & 0x7fff;
			}
		}
	}
}


INLINE void cps1_render_scroll3(struct osd_bitmap *bitmap, int priority)
{
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
				if (priority)
				{
					int mask=colour & 0x0180;
					if (mask)
					{
						int transp=0;
						switch (mask)
						{
							case 0x080:
								transp=0xf001;
								break;
							case 0x0100:
								transp=0x8000;
								break;
							case 0x0180:
								transp=0x8000;
								break;
							case 0x0200:
								transp=0x8000;
								break;
						}

						if (transp)
						{
							drawgfx(bitmap,Machine->gfx[3],
							     code,
							    colour&0x1f,
							    colour&0x20,colour&0x40,
							    32*sx-nxoffset,32*sy-nyoffset,
							    &Machine->drv->visible_area,
							    TRANSPARENCY_PENS,transp);
						}
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
			}
		}
	}
}


/***************************************************************************

        Refresh screen

***************************************************************************/

void cps1_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	unsigned short palette_usage[32*4];
	int layercontrolport;
	int layercontrol;
	int scrl1on, scrl2on, scrl3on;
	int scroll1priority;
	int scroll2priority;
	int i,offset;

	/* Get video memory base registers */
	cps1_get_video_base();

	/* Get scroll values */
	scroll1x=cps1_port(CPS1_SCROLL1_SCROLLX);
	scroll1y=cps1_port(CPS1_SCROLL1_SCROLLY);
	scroll2x=cps1_port(CPS1_SCROLL2_SCROLLX);
	scroll2y=cps1_port(CPS1_SCROLL2_SCROLLY);
	scroll3x=cps1_port(CPS1_SCROLL3_SCROLLX);
	scroll3y=cps1_port(CPS1_SCROLL3_SCROLLY);

	/*
	 Welcome to kludgesville... I have no idea how this is supposed
	 to work. The layer priority register is different from machine
	 to machine.
	*/
	switch (cps1_game_config->alternative)
	{
		/* Wandering video control ports */
		case 0:
			/* default */
			layercontrolport=0x66;
			layercontrol=READ_WORD(&cps1_output[layercontrolport]);
			/* Layers on / off */
			scrl1on=1;
			scrl2on=layercontrol&0x08;      /* Not quite should probably */
			scrl3on=layercontrol&0x08;      /* be individually turn-offable  */

			scroll2priority=(layercontrol&0x040);
			break;
		case 1:
			/* Willow */
			layercontrolport=0x70;
			layercontrol=READ_WORD(&cps1_output[layercontrolport]);
			/* Layers on / off */
			scrl1on=1;
			scrl2on=layercontrol&0x08;      /* Not quite should probably */
			scrl3on=layercontrol&0x08;      /* be individually turn-offable  */

			/* Scroll 2 / 3 priority */
			scroll2priority=(layercontrol&0x040);
			break;
		case 2:
			layercontrolport=0x6e;
			layercontrol=READ_WORD(&cps1_output[layercontrolport]);
			/* Layers on / off */
			scrl1on=1;
			scrl2on=layercontrol&0x08;      /* Not quite should probably */
			scrl3on=layercontrol&0x08;      /* be individually turn-offable  */

			/* Scroll 2 / 3 priority */
			scroll2priority=(layercontrol&0x040);
			break;

		case 4:
			/* Strider (completely kludged) */
			layercontrolport=0x66;
			layercontrol=READ_WORD(&cps1_output[layercontrolport]);
			/* Layers on / off */
			scrl1on=1;
			scrl2on=layercontrol&0x08;      /* Not quite should probably */
			scrl3on=layercontrol&0x08;      /* be individually turn-offable  */

			/* Priority */
			scroll2priority=(layercontrol&0x060)==0x40;
			if (layercontrol == 0x0e4e )
			{
			    scroll2priority=0;
			}

			break;

		case 5: /* Ghouls and Ghosts */
			layercontrolport=0x66;
			layercontrol=READ_WORD(&cps1_output[layercontrolport]);

			/* Layers on / off */
			scrl1on=layercontrol&0x02;
			scrl2on=layercontrol&0x04;
			scrl3on=layercontrol&0x08;
			scroll2priority=(layercontrol&0x060)==0x40;
			if (layercontrol == 0x12ca ||
			        layercontrol == 0x3948)
			{
			        scroll2priority=0;
			}
			break;
		case 6: /* 1941 */
			layercontrolport=0x68;
			layercontrol=READ_WORD(&cps1_output[layercontrolport]);
			/* Layers on / off */
			scrl1on=1;
			scrl2on=1;
			scrl3on=layercontrol&0x20;
			scroll2priority=(layercontrol&0x040);
			break;

		case 7: /* Magic sword */
			layercontrolport=0x62;
			layercontrol=READ_WORD(&cps1_output[layercontrolport]);
			/* Layers on / off */
			scrl1on=1;
			scrl2on=layercontrol&0x04;
			scrl3on=layercontrol&0x06;
			/* Priority */
			scroll2priority=(layercontrol&0x040);
			break;

		case 8: /* Nemo */
			layercontrolport=0x42;
			layercontrol=READ_WORD(&cps1_output[layercontrolport]);
			/* Layers on / off */
			scrl1on=1;
			scrl2on=1; //layercontrol&0x04;
			scrl3on=1; //layercontrol&0x06;
			/* Priority */
			scroll2priority=(layercontrol&0x040);
			break;

		default:
			layercontrolport=0;
			layercontrol=0xffff;
			scrl1on=1;
			scrl2on=layercontrol&0x08;      /* Not quite should probably */
			scrl3on=layercontrol&0x08;      /* be individually turn-offable  */

			layercontrol|=0x0080;           /* Force scroll1 priority */

			/* Scroll 2 / 3 priority */
			scroll2priority=1;
			break;
	}

	/* Build palette */
	cps1_build_palette();

	/* Compute the used portion */
	memset (palette_usage, 0, sizeof (palette_usage));
	cps1_palette_sprites (&palette_usage[0]);
	if (scrl1on)
		cps1_palette_scroll1 (&palette_usage[32]);
	if (scrl2on)
		cps1_palette_scroll2 (&palette_usage[32+32]);
	if (scrl3on)
		cps1_palette_scroll3 (&palette_usage[32+32+32]);
	for (i = offset = 0; i < 32*4; i++)
	{
		int usage = palette_usage[i];
		if (usage)
		{
			int j;
			for (j = 0; j < 16; j++)
				if (usage & (1 << j))
					palette_used_colors[offset++] = PALETTE_COLOR_USED;
				else
					palette_used_colors[offset++] = PALETTE_COLOR_UNUSED;
		}
		else
		{
			memset (&palette_used_colors[offset], PALETTE_COLOR_UNUSED, 16);
			offset += 16;
		}
	}
	palette_recalc ();

	/* Blank screen */
	fillbitmap(bitmap,palette_transparent_pen,&Machine->drv->visible_area);


	/* Scroll 1 priority */
	scroll1priority=(layercontrol&0x0080);

	/* Scroll 1 priority */
	if (!scroll1priority && scrl1on)
	{
		cps1_render_scroll1(bitmap);
	}

	if (scroll2priority)
	{
		if (scrl3on)
		{
			cps1_render_scroll3(bitmap, 0);
		}
		if (scrl2on)
		{
			cps1_render_scroll2(bitmap, 0);
		}
	}
	else
	{
		if (scrl2on)
		{
			cps1_render_scroll2(bitmap, 0);
		}
		if (scrl3on)
		{
			cps1_render_scroll3(bitmap, 0);
		}
	}

	cps1_render_sprites(bitmap);

	if (scroll2priority)
	{
		if (scrl3on)
		{
			cps1_render_scroll3(bitmap, 0x0100);
		}
		if (scrl2on)
		{
			cps1_render_scroll2(bitmap, 0x0100);
		}
	}
	else
	{
		if (scrl2on)
		{
			cps1_render_scroll2(bitmap, 0x0100);
		}
		if (scrl3on)
		{
			cps1_render_scroll3(bitmap, 0x0100);
		}
	}

	if (scroll1priority && scrl1on)
	{
		cps1_render_scroll1(bitmap);
	}

#if 0
	{
		int i;
		int nReg=layercontrolport;
		int nVal=cps1_port(nReg);
		struct DisplayText dt[2];
		char szBuffer[40];
		int count = 0;

		char *pszPriority="SCROLL 2";
		if (!scroll2priority)
		{
			pszPriority="SCROLL 3";
		}
		sprintf(szBuffer, "%04x=%04x %s", nReg, nVal, pszPriority);
		dt[0].text = szBuffer;
		dt[0].color = DT_COLOR_RED;
		dt[0].x = (Machine->uiwidth - Machine->uifont->width * strlen(dt[0].text)) / 2;
		dt[0].y = (Machine->uiheight - Machine->uifont->height) / 2 + 2;
		dt[1].text = 0;
		displaytext(dt,0);
	}
#endif
}

