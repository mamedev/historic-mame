/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

  This is the driver for the "Galaxian" style board, used, with small
  variations, by an incredible amount of games in the early 80s.

  This video driver is used by the following drivers:
  - galaxian.c
  - mooncrst.c
  - scramble.c
  - scobra.c
  - ckongs.c

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct rectangle spritevisiblearea =
{
	2*8+1, 32*8-1,
	2*8, 30*8-1
};
static struct rectangle spritevisibleareaflipx =
{
	0*8, 30*8-2,
	2*8, 30*8-1
};

#define MAX_STARS 250
#define STARS_COLOR_BASE 32

unsigned char *galaxian_attributesram;
unsigned char *galaxian_bulletsram;
unsigned char *pisces_gfx_bank;

int galaxian_bulletsram_size = 0;
static int stars_on,stars_blink;
static int stars_type = 0;	/* 0 = Galaxian stars */
						    /* 1 = Scramble stars */
						    /* 2 = Rescue stars (same as Scramble, but only half screen) */
						    /* 3 = Mariner stars (same as Galaxian, but some parts are blanked */
static unsigned int stars_scroll;

struct star
{
	int x,y,code;
};
static struct star stars[MAX_STARS];
static int total_stars;
static void (*modify_charcode  )(int*,int)           = 0;  /* function to call to do character banking */
static void (*modify_spritecode)(int*,int*,int*,int) = 0;  /* function to call to do sprite banking */
static int gfx_extend;	/* used by Moon Cresta only */
static int flipscreen[2];

static int background_on;
static unsigned char backcolour[256];

static void mooncrst_modify_charcode  (int *charcode,int offs);
static void  moonqsr_modify_charcode  (int *charcode,int offs);
static void   pisces_modify_charcode  (int *charcode,int offs);
static void  mariner_modify_charcode  (int *charcode,int offs);

static void mooncrst_modify_spritecode(int *spritecode,int *flipx,int *flipy,int offs);
static void  moonqsr_modify_spritecode(int *spritecode,int *flipx,int *flipy,int offs);
static void   ckongs_modify_spritecode(int *spritecode,int *flipx,int *flipy,int offs);
static void  calipso_modify_spritecode(int *spritecode,int *flipx,int *flipy,int offs);
static void   pisces_modify_spritecode(int *spritecode,int *flipx,int *flipy,int offs);

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Moon Cresta has one 32 bytes palette PROM, connected to the RGB output
  this way:

  bit 7 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

  The output of the background star generator is connected this way:

  bit 5 -- 100 ohm resistor  -- BLUE
        -- 150 ohm resistor  -- BLUE
        -- 100 ohm resistor  -- GREEN
        -- 150 ohm resistor  -- GREEN
        -- 100 ohm resistor  -- RED
  bit 0 -- 150 ohm resistor  -- RED

  The blue background in Scramble and other games goes through a 390 ohm
  resistor.

***************************************************************************/
void galaxian_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])


	/* first, the char acter/sprite palette */
	for (i = 0;i < 32;i++)
	{
		int col,bit0,bit1,bit2;

		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = 0;
		bit1 = (*color_prom >> 6) & 0x01;
		bit2 = (*color_prom >> 7) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		color_prom++;
	}

	/* now the stars */
	for (i = 0;i < 64;i++)
	{
		int bits;
		int map[4] = { 0x00, 0x88, 0xcc, 0xff };


		bits = (i >> 0) & 0x03;
		*(palette++) = map[bits];
		bits = (i >> 2) & 0x03;
		*(palette++) = map[bits];
		bits = (i >> 4) & 0x03;
		*(palette++) = map[bits];
	}

	/* characters and sprites use the same palette */
	for (i = 0;i < TOTAL_COLORS(0);i++)
	{
		if (i & 3) COLOR(0,i) = i;
		else COLOR(0,i) = 0;	/* 00 is always black, regardless of the contents of the PROM */
	}

	/* bullets can be either white or yellow */

	COLOR(2,0) = 0;
	COLOR(2,1) = 0x0f + STARS_COLOR_BASE;	/* yellow */
	COLOR(2,2) = 0;
	COLOR(2,3) = 0x3f + STARS_COLOR_BASE;	/* white */

	/* default blue background */
	*(palette++) = 0;
	*(palette++) = 0;
	*(palette++) = 0x55;

	for (i = 0;i < TOTAL_COLORS(3);i++)
	{
		COLOR(3,i) = 96 + i;
	}
}

void minefld_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


    galaxian_vh_convert_color_prom(palette, colortable, color_prom);

	/* set up background colors */

   	/* Graduated Blue */

   	for (i = 0; i < 64; i++)
    {
		palette[96*3 + i*3 + 0] = 0;
       	palette[96*3 + i*3 + 1] = i * 2;
       	palette[96*3 + i*3 + 2] = i * 4;
    }

    /* Graduated Brown */

   	for (i = 0; i < 64; i++)
    {
       	palette[160*3 + i*3 + 0] = i * 3;
       	palette[160*3 + i*3 + 1] = i * 1.5;
       	palette[160*3 + i*3 + 2] = i;
    }
}

void rescue_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


    galaxian_vh_convert_color_prom(palette, colortable, color_prom);

	/* set up background colors */

   	/* Graduated Blue */

   	for (i = 0; i < 64; i++)
    {
		palette[96*3 + i*3 + 0] = 0;
       	palette[96*3 + i*3 + 1] = i * 2;
       	palette[96*3 + i*3 + 2] = i * 4;
    }
}

void mariner_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


    galaxian_vh_convert_color_prom(palette, colortable, color_prom);

	/* set up background colors */

   	/* nine shades of blue */

	palette[96*3 + 0] = 0;
	palette[96*3 + 1] = 0;
	palette[96*3 + 2] = 0;

   	for (i = 1; i < 10; i++)
    {
		palette[96*3 + i*3 + 0] = 0;
       	palette[96*3 + i*3 + 1] = 0;
       	palette[96*3 + i*3 + 2] = 0xea - 0x15 * (i - 1);
    }
}

static void decode_background(void)
{
	int i, j, k;
	unsigned char tile[32*8*8];


	for (i = 0; i < 32; i++)
	{
		for (j = 0; j < 8; j++)
		{
			for (k = 0; k < 8; k++)
			{
				tile[i*64 + j*8 + k] = backcolour[i*8+j];
			}
		}

		decodechar(Machine->gfx[3],i,tile,Machine->drv->gfxdecodeinfo[3].gfxlayout);
	}
}

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int galaxian_vh_start(void)
{
	int generator;
	int x,y;

	gfx_extend = 0;
	stars_on = 0;
	flipscreen[0] = 0;
	flipscreen[1] = 0;

	if (generic_vh_start() != 0)
		return 1;

    /* Default alternate background - Solid Blue */

    for (x=0; x<256; x++)
	{
		backcolour[x] = 0;
	}
	background_on = 0;

	decode_background();


	/* precalculate the star background */

	total_stars = 0;
	generator = 0;

	for (y = 255;y >= 0;y--)
	{
		for (x = 511;x >= 0;x--)
		{
			int bit1,bit2;


			generator <<= 1;
			bit1 = (~generator >> 17) & 1;
			bit2 = (generator >> 5) & 1;

			if (bit1 ^ bit2) generator |= 1;

			if (y >= Machine->drv->visible_area.min_y &&
				y <= Machine->drv->visible_area.max_y &&
				((~generator >> 16) & 1) &&
				(generator & 0xff) == 0xff)
			{
				int color;

				color = (~(generator >> 8)) & 0x3f;
				if (color && total_stars < MAX_STARS)
				{
					stars[total_stars].x = x;
					stars[total_stars].y = y;
					stars[total_stars].code = color;

					total_stars++;
				}
			}
		}
	}

	return 0;
}

int mooncrst_vh_start(void)
{
	modify_charcode   = mooncrst_modify_charcode;
	modify_spritecode = mooncrst_modify_spritecode;
	return galaxian_vh_start();
}

int moonqsr_vh_start(void)
{
	modify_charcode   = moonqsr_modify_charcode;
	modify_spritecode = moonqsr_modify_spritecode;
	return galaxian_vh_start();
}

int pisces_vh_start(void)
{
	modify_charcode   = pisces_modify_charcode;
	modify_spritecode = pisces_modify_spritecode;
	return galaxian_vh_start();
}

int scramble_vh_start(void)
{
	stars_type = 1;
	return galaxian_vh_start();
}

int rescue_vh_start(void)
{
	int ans,x;

	stars_type = 2;
	ans = galaxian_vh_start();

    /* Setup background colour array (blue sky, blue sea, black bottom line) */

    for (x=0;x<64;x++)
	{
		backcolour[x*2+0] = x;
		backcolour[x*2+1] = x;
    }

    for (x=0;x<60;x++)
	{
		backcolour[128+x*2+0] = x + 4;
		backcolour[128+x*2+1] = x + 4;
    }

    for (x=248;x<256;x++) backcolour[x] = 0;

    decode_background();

    return ans;
}

int minefld_vh_start(void)
{
	int ans,x;

	stars_type = 2;
	ans = galaxian_vh_start();

    /* Setup background colour array (blue sky, brown ground, black bottom line) */

    for (x=0;x<64;x++)
	{
		backcolour[x*2+0] = x;
		backcolour[x*2+1] = x;
    }

    for (x=0;x<60;x++)
	{
		backcolour[128+x*2+0] = x + 64;
		backcolour[128+x*2+1] = x + 64;
    }

    for (x=248;x<256;x++) backcolour[x] = 0;

    decode_background();

    return ans;
}

int ckongs_vh_start(void)
{
	stars_type = 1;
	modify_spritecode = ckongs_modify_spritecode;
	return galaxian_vh_start();
}

int calipso_vh_start(void)
{
	stars_type = 1;
	modify_spritecode = calipso_modify_spritecode;
	return galaxian_vh_start();
}

int mariner_vh_start(void)
{
	int ans,x;

	modify_charcode = mariner_modify_charcode;

	stars_type = 3;
	ans = galaxian_vh_start();

    /* Setup background colour array (blue water) */

    for (x=0;  x<63; x++) backcolour[x] = 0;
    for (x=63; x<71; x++) backcolour[x] = 1;
    for (x=71; x<79; x++) backcolour[x] = 2;
    for (x=79; x<87; x++) backcolour[x] = 3;
    for (x=87; x<95; x++) backcolour[x] = 4;
    for (x=95; x<111;x++) backcolour[x] = 5;
    for (x=111;x<135;x++) backcolour[x] = 6;
    for (x=135;x<167;x++) backcolour[x] = 7;
    for (x=167;x<207;x++) backcolour[x] = 8;
    for (x=207;x<247;x++) backcolour[x] = 9;
    for (x=247;x<256;x++) backcolour[x] = 0;

    decode_background();

	/* The background is always on */
    background_on = 1;

	return ans;
}


void galaxian_flipx_w(int offset,int data)
{
	if (flipscreen[0] != (data & 1))
	{
		flipscreen[0] = data & 1;
		memset(dirtybuffer,1,videoram_size);
	}
}

void galaxian_flipy_w(int offset,int data)
{
	if (flipscreen[1] != (data & 1))
	{
		flipscreen[1] = data & 1;
		memset(dirtybuffer,1,videoram_size);
	}
}



void galaxian_attributes_w(int offset,int data)
{
	if ((offset & 1) && galaxian_attributesram[offset] != data)
	{
		int i;


		for (i = offset / 2;i < videoram_size;i += 32)
			dirtybuffer[i] = 1;
	}

	galaxian_attributesram[offset] = data;
}


void scramble_background_w(int offset, int data)
{
	if (background_on != data)
    {
		background_on = data;
		memset(dirtybuffer,1,videoram_size);
    }
}


void galaxian_stars_w(int offset,int data)
{
	stars_on = (data & 1);
	stars_scroll = 0;
}


void mooncrst_gfxextend_w(int offset,int data)
{
	if (data) gfx_extend |= (1 << offset);
	else gfx_extend &= ~(1 << offset);
}


void mooncrgx_gfxextend_w(int offset,int data)
{
  /* for the Moon Cresta bootleg on Galaxian H/W the gfx_extend is
     located at 0x6000-0x6002.  Also, 0x6000 and 0x6001 are reversed. */
     if(offset == 1)
       offset = 0;
     else if(offset == 0)
       offset = 1;    /* switch 0x6000 and 0x6001 */
	mooncrst_gfxextend_w(offset, data);
}


INLINE void plot_star(struct osd_bitmap *bitmap, int x, int y, int code)
{
	int backcol;

	backcol = backcolour[x];

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp = x;
		x = y;
		y = temp;
	}
	if (Machine->orientation & ORIENTATION_FLIP_X)
	{
		x = 255 - x;
	}
	if (Machine->orientation & ORIENTATION_FLIP_Y)
	{
		y = 255 - y;
	}

	if (flipscreen[0])
	{
		x = 255 - x;
	}
	if (flipscreen[1])
	{
		y = 255 - y;
	}

	if ((bitmap->line[y][x] == Machine->pens[0]) ||
		(bitmap->line[y][x] == Machine->pens[96 + backcol]))
	{
		bitmap->line[y][x] = Machine->pens[STARS_COLOR_BASE + code];
	}
}


/* Character banking routines */
static void mooncrst_modify_charcode(int *charcode,int offs)
{
	if ((gfx_extend & 4) && (*charcode & 0xc0) == 0x80)
	{
		*charcode = (*charcode & 0x3f) | (gfx_extend << 6);
	}
}

static void moonqsr_modify_charcode(int *charcode,int offs)
{
	if (galaxian_attributesram[2 * (offs % 32) + 1] & 0x20)
	{
		*charcode += 256;
	}

    mooncrst_modify_charcode(charcode,offs);
}

static void pisces_modify_charcode(int *charcode,int offs)
{
	if (*pisces_gfx_bank & 1)
	{
		*charcode += 256;
	}
}

static void mariner_modify_charcode(int *charcode,int offs)
{
	/* I don't really know if this is correct, but I don't see
	   any other obvious way to switch character banks. */
	if (((offs & 0x1f) <= 4) ||
		((offs & 0x1f) >= 30))
	{
		*charcode += 256;
	}
}

/* Sprite banking routines */
static void mooncrst_modify_spritecode(int *spritecode,int *flipx,int *flipy,int offs)
{
	if ((gfx_extend & 4) && (*spritecode & 0x30) == 0x20)
	{
		*spritecode = (*spritecode & 0x0f) | (gfx_extend << 4);
	}
}

static void moonqsr_modify_spritecode(int *spritecode,int *flipx,int *flipy,int offs)
{
	if (spriteram[offs + 2] & 0x20)
	{
		*spritecode += 64;
	}

    mooncrst_modify_spritecode(spritecode, flipx, flipy, offs);
}

static void ckongs_modify_spritecode(int *spritecode,int *flipx,int *flipy,int offs)
{
	if (spriteram[offs + 2] & 0x10)
	{
		*spritecode += 64;
	}
}

static void calipso_modify_spritecode(int *spritecode,int *flipx,int *flipy,int offs)
{
	/* No flips */
	*spritecode = spriteram[offs + 1];
	*flipx = 0;
	*flipy = 0;
}

static void pisces_modify_spritecode(int *spritecode,int *flipx,int *flipy,int offs)
{
	if (*pisces_gfx_bank & 1)
	{
		*spritecode += 64;
	}
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void galaxian_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int i,offs;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy,charcode,background_charcode;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

            background_charcode = sx;

			charcode = videoram[offs];

			if (flipscreen[0]) sx = 31 - sx;
			if (flipscreen[1]) sy = 31 - sy;

			if (modify_charcode)
			{
				modify_charcode(&charcode, offs);
			}

            if (background_on)
            {
				/* Draw background */

 			    drawgfx(tmpbitmap,Machine->gfx[3],
					    background_charcode,
					    0,
					    flipscreen[0],flipscreen[1],
					    8*sx,8*sy,
					    0,TRANSPARENCY_NONE,0);
			}

 			drawgfx(tmpbitmap,Machine->gfx[0],
				    charcode,
					galaxian_attributesram[2 * (offs % 32) + 1] & 0x07,
				    flipscreen[0],flipscreen[1],
				    8*sx,8*sy,
				    0, background_on ? TRANSPARENCY_COLOR : TRANSPARENCY_NONE, 0);
		}
	}


	/* copy the temporary bitmap to the screen */
	{
		int scroll[32];


		if (flipscreen[0])
		{
			for (i = 0;i < 32;i++)
			{
				scroll[31-i] = -galaxian_attributesram[2 * i];
				if (flipscreen[1]) scroll[31-i] = -scroll[31-i];
			}
		}
		else
		{
			for (i = 0;i < 32;i++)
			{
				scroll[i] = -galaxian_attributesram[2 * i];
				if (flipscreen[1]) scroll[i] = -scroll[i];
			}
		}

		copyscrollbitmap(bitmap,tmpbitmap,0,0,32,scroll,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the bullets. Some games (Zig Zag don't have a bullets_ram. In that case
	   galaxian_bulletsram_size will be initialized by zero */
	for (offs = 0;offs < galaxian_bulletsram_size;offs += 4)
	{
		int x,y;
		int color;


		if (offs == 7*4) color = 0;	/* yellow */
		else color = 1;	/* white */

		x = 255 - galaxian_bulletsram[offs + 3] - Machine->drv->gfxdecodeinfo[2].gfxlayout->width;
		y = 255 - galaxian_bulletsram[offs + 1];
		if (flipscreen[1]) y = 255 - y;

		drawgfx(bitmap,Machine->gfx[2],
				0,	/* this is just a line, generated by the hardware */
				color,
				0,0,
				x,y,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}


	/* Draw the sprites */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int flipx,flipy,sx,sy,spritecode;


		sx = (spriteram[offs + 3] + 1) & 0xff; /* This is definately correct in Mariner. Look at
												  the 'gate' moving up/down. It stops at the
  												  right spots */
		sy = 240 - spriteram[offs];
		flipx = spriteram[offs + 1] & 0x40;
		flipy = spriteram[offs + 1] & 0x80;
		spritecode = spriteram[offs + 1] & 0x3f;

		if (modify_spritecode)
		{
			modify_spritecode(&spritecode, &flipx, &flipy, offs);
		}

		if (flipscreen[0])
		{
			sx = 240 - sx;	/* I checked a bunch of games including Scramble
							   (# of pixels the ship is from the top of the mountain),
			                   Mariner and Checkman. This is correct for them */
			flipx = !flipx;
		}
		if (flipscreen[1])
		{
			sy = 240 - sy;
			flipy = !flipy;
		}

		/* In Amidar, */
		/* Sprites #0, #1 and #2 need to be offset one pixel to be correctly */
		/* centered on the ladders in Turtles (we move them down, but since this */
		/* is a rotated game, we actually move them left). */
		/* Note that the adjustment must be done AFTER handling flipscreen, thus */
		/* proving that this is a hardware related "feature" */
		/* This is not Amidar, it is Galaxian/Scramble/hundreds of clones, and I'm */
		/* not sure it should be the same. A good game to test alignment is Armored Car */
/*		if (offs <= 2*4) sy++;*/

		drawgfx(bitmap,Machine->gfx[1],
				spritecode,
				spriteram[offs + 2] & 0x07,
				flipx,flipy,
				sx,sy,
				flipscreen[0] ? &spritevisibleareaflipx : &spritevisiblearea,TRANSPARENCY_PEN,0);
	}


	/* draw the stars */
	if (stars_on)
	{
		switch (stars_type)
		{
		case 0:	/* Galaxian stars */
		case 3:	/* Mariner stars */
			for (offs = 0;offs < total_stars;offs++)
			{
				int x,y;


				x = (stars[offs].x + stars_scroll/2) % 256;
				y = stars[offs].y;

				/* No stars below row (column) 64, between rows 176 and 215 or
				   between 224 and 247 */
				if ((stars_type == 3) &&
					((x < 64) ||
					((x >= 176) && (x < 216)) ||
					((x >= 224) && (x < 248)))) continue;

				if ((y & 1) ^ ((x >> 4) & 1))
				{
					plot_star(bitmap, x, y, stars[offs].code);
				}
			}
			break;

		case 1:	/* Scramble stars */
		case 2:	/* Rescue stars */
			for (offs = 0;offs < total_stars;offs++)
			{
				int x,y;


				x = stars[offs].x / 2;
				y = stars[offs].y;

				if ((stars_type != 2 || x < 128) &&	/* draw only half screen in Rescue */
				   ((y & 1) ^ ((x >> 4) & 1)))
				{
					/* Determine when to skip plotting */
					switch (stars_blink)
					{
					case 0:
						if (!(stars[offs].code & 1))  continue;
						break;
					case 1:
						if (!(stars[offs].code & 4))  continue;
						break;
					case 2:
						if (!(stars[offs].x & 4))  continue;
						break;
					case 3:
					    /* Always plot */
						break;
					}
					plot_star(bitmap, x, y, stars[offs].code);
				}
			}
			break;
		}
	}
}



int galaxian_vh_interrupt(void)
{
	stars_scroll++;

	return nmi_interrupt();
}

int scramble_vh_interrupt(void)
{
	static int blink_count;


	blink_count++;
	if (blink_count >= 45)
	{
		blink_count = 0;
		stars_blink = (stars_blink + 1) & 3;
	}

	return nmi_interrupt();
}

int mariner_vh_interrupt(void)
{
	stars_scroll--;

	return nmi_interrupt();
}

int devilfsh_vh_interrupt(void)
{
	stars_scroll++;

	return interrupt();
}
