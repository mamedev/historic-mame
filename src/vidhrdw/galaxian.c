/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

  This is the driver for the "Galaxian" style board, used, with small
  variations, by an incredible amount of games in the early 80s.

  This video driver is used by the following drivers:
  - galaxian.c
  - scramble.c
  - scobra.c
  - frogger.c

***************************************************************************/

#include "driver.h"


static struct rectangle _spritevisiblearea =
{
	2*8+1, 32*8-1,
	2*8,   30*8-1
};
static struct rectangle _spritevisibleareaflipx =
{
	0*8, 30*8-2,
	2*8, 30*8-1
};

static struct rectangle* spritevisiblearea;
static struct rectangle* spritevisibleareaflipx;


#define STARS_COLOR_BASE 		32
#define BULLETS_COLOR_BASE		(STARS_COLOR_BASE + 64)
#define BACKGROUND_COLOR_BASE	(BULLETS_COLOR_BASE + 2)


unsigned char *galaxian_videoram;
unsigned char *galaxian_spriteram;
unsigned char *galaxian_attributesram;
unsigned char *galaxian_bulletsram;
size_t galaxian_spriteram_size;
size_t galaxian_bulletsram_size;


static int mooncrst_gfxextend;
static int pisces_gfxbank;
static int jumpbug_gfxbank[5];
static void (*modify_charcode)(int*,int);		/* function to call to do character banking */
static void mooncrst_modify_charcode(int *code,int x);
static void  moonqsr_modify_charcode(int *code,int x);
static void   pisces_modify_charcode(int *code,int x);
static void  batman2_modify_charcode(int *code,int x);
static void  mariner_modify_charcode(int *code,int x);
static void  jumpbug_modify_charcode(int *code,int x);

static void (*modify_spritecode)(int*,int*,int*,int);	/* function to call to do sprite banking */
static void mooncrst_modify_spritecode(int *code,int *flipx,int *flipy,int offs);
static void  moonqsr_modify_spritecode(int *code,int *flipx,int *flipy,int offs);
static void   ckongs_modify_spritecode(int *code,int *flipx,int *flipy,int offs);
static void  calipso_modify_spritecode(int *code,int *flipx,int *flipy,int offs);
static void   pisces_modify_spritecode(int *code,int *flipx,int *flipy,int offs);
static void  batman2_modify_spritecode(int *code,int *flipx,int *flipy,int offs);
static void  jumpbug_modify_spritecode(int *code,int *flipx,int *flipy,int offs);

static void (*modify_color)(int*);	/* function to call to do modify how the color codes map to the PROM */
static void frogger_modify_color(int *code);

static void (*modify_ypos)(UINT8*);	/* function to call to do modify how vertical positioning bits are connected */
static void frogger_modify_ypos(UINT8 *sy);

/* star circuit */
#define STAR_COUNT  250
struct star
{
	int x,y,color;
};
static struct star stars[STAR_COUNT];
       int galaxian_stars_on;
static int stars_blink_state;
static void *stars_blink_timer;
       void galaxian_init_stars(unsigned char **palette);
static void (*draw_stars)(struct osd_bitmap *);		/* function to call to draw the star layer */
static void galaxian_draw_stars(struct osd_bitmap *bitmap);
	   void scramble_draw_stars(struct osd_bitmap *bitmap);
static void   rescue_draw_stars(struct osd_bitmap *bitmap);
static void  mariner_draw_stars(struct osd_bitmap *bitmap);
static void  jumpbug_draw_stars(struct osd_bitmap *bitmap);
static void start_stars_blink_timer(double ra, double rb, double c);

/* bullets circuit */
static int darkplnt_bullet_color;
static void (*draw_bullets)(struct osd_bitmap *,int,int,int);	/* function to call to draw a bullet */
static void galaxian_draw_bullets(struct osd_bitmap *bitmap, int offs, int x, int y);
static void scramble_draw_bullets(struct osd_bitmap *bitmap, int offs, int x, int y);
static void   theend_draw_bullets(struct osd_bitmap *bitmap, int offs, int x, int y);
static void darkplnt_draw_bullets(struct osd_bitmap *bitmap, int offs, int x, int y);

/* background circuit */
static int background_red, background_green, background_blue;
static int background_start_pen;
static void (*draw_background)(struct osd_bitmap *);	/* function to call to draw the background */
static void scramble_draw_background(struct osd_bitmap *bitmap);
static void  turtles_draw_background(struct osd_bitmap *bitmap);
static void  mariner_draw_background(struct osd_bitmap *bitmap);
static void  frogger_draw_background(struct osd_bitmap *bitmap);
static void stratgyx_draw_background(struct osd_bitmap *bitmap);
static void  minefld_draw_background(struct osd_bitmap *bitmap);
static void   rescue_draw_background(struct osd_bitmap *bitmap);


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Galaxian has one 32 bytes palette PROM, connected to the RGB output this way:

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

  The bullet RGB outputs go through 100 ohm resistors.

  The RGB outputs have a 470 ohm pull-down each.

***************************************************************************/
void galaxian_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	/* first, the character/sprite palette */

	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2;

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
		bit0 = (*color_prom >> 6) & 0x01;
		bit1 = (*color_prom >> 7) & 0x01;
		*(palette++) = 0x4f * bit0 + 0xa8 * bit1;

		color_prom++;
	}


	galaxian_init_stars(&palette);


	/* bullets - yellow and white */

	*(palette++) = 0xef;
	*(palette++) = 0xef;
	*(palette++) = 0x00;

	*(palette++) = 0xef;
	*(palette++) = 0xef;
	*(palette++) = 0xef;


	/* black background */

	background_start_pen = BACKGROUND_COLOR_BASE;

	*(palette++) = 0;
	*(palette++) = 0;
	*(palette++) = 0;
}

void scramble_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	galaxian_vh_convert_color_prom(palette, colortable, color_prom);


	/* blue background - 390 ohm resistor */

	palette[(background_start_pen + 1) * 3 + 0] = 0;
	palette[(background_start_pen + 1) * 3 + 1] = 0;
	palette[(background_start_pen + 1) * 3 + 2] = 0x56;
}

void moonwar_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	scramble_vh_convert_color_prom(palette, colortable, color_prom);


	/* wire mod to connect the bullet blue output to the 220 ohm resistor */

	palette[BULLETS_COLOR_BASE * 3 + 2] = 0x97;
}

void turtles_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	galaxian_vh_convert_color_prom(palette, colortable, color_prom);


	/*  The background color generator is connected this way:

		RED   - 390 ohm resistor
		GREEN - 470 ohm resistor
		BLUE  - 390 ohm resistor */

	for (i = 0; i < 8; i++)
	{
		palette[(background_start_pen + i) * 3 + 0] = (i & 0x01) ? 0x55 : 0x00;
		palette[(background_start_pen + i) * 3 + 1] = (i & 0x02) ? 0x47 : 0x00;
		palette[(background_start_pen + i) * 3 + 2] = (i & 0x04) ? 0x55 : 0x00;
	}
}

void stratgyx_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	galaxian_vh_convert_color_prom(palette, colortable, color_prom);


	/*  The background color generator is connected this way:

		RED   - 270 ohm resistor
		GREEN - 560 ohm resistor
		BLUE  - 470 ohm resistor */

	for (i = 0; i < 8; i++)
	{
		palette[(background_start_pen + i) * 3 + 0] = (i & 0x01) ? 0x7c : 0x00;
		palette[(background_start_pen + i) * 3 + 1] = (i & 0x02) ? 0x3c : 0x00;
		palette[(background_start_pen + i) * 3 + 2] = (i & 0x04) ? 0x47 : 0x00;
	}
}

void frogger_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	galaxian_vh_convert_color_prom(palette, colortable, color_prom);


	/* blue background - 470 ohm resistor */

	palette[(background_start_pen + 1) * 3 + 0] = 0;
	palette[(background_start_pen + 1) * 3 + 1] = 0;
	palette[(background_start_pen + 1) * 3 + 2] = 0x47;
}


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Dark Planet has one 32 bytes palette PROM, connected to the RGB output this way:

  bit 5 -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 1  kohm resistor  -- BLUE
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
  bit 0 -- 1  kohm resistor  -- RED

  The bullet RGB outputs go through 100 ohm resistors.

  The RGB outputs have a 470 ohm pull-down each.

***************************************************************************/
void darkplnt_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


	/* first, the character/sprite palette */

	for (i = 0;i < 32;i++)
	{
		int bit0,bit1,bit2;

		/* red component */
		bit0 = (*color_prom >> 0) & 0x01;
		bit1 = (*color_prom >> 1) & 0x01;
		bit2 = (*color_prom >> 2) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		*(palette++) = 0x00;
		/* blue component */
		bit0 = (*color_prom >> 3) & 0x01;
		bit1 = (*color_prom >> 4) & 0x01;
		bit2 = (*color_prom >> 5) & 0x01;
		*(palette++) = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		color_prom++;
	}


	/* bullets - red and blue */

	*(palette++) = 0xef;
	*(palette++) = 0x00;
	*(palette++) = 0x00;

	*(palette++) = 0x00;
	*(palette++) = 0x00;
	*(palette++) = 0xef;


	/* black background */

	background_start_pen = 34;

	*(palette++) = 0;
	*(palette++) = 0;
	*(palette++) = 0;
}

void minefld_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


    galaxian_vh_convert_color_prom(palette, colortable, color_prom);


	/* set up background colors */

   	/* graduated blue */

   	for (i = 0; i < 64; i++)
    {
		palette[(background_start_pen + i) * 3 + 0] = 0;
       	palette[(background_start_pen + i) * 3 + 1] = i * 2;
       	palette[(background_start_pen + i) * 3 + 2] = i * 4;
    }

    /* graduated brown */

   	for (i = 0; i < 64; i++)
    {
       	palette[(background_start_pen + 64 + i) * 3 + 0] = i * 3;
       	palette[(background_start_pen + 64 + i) * 3 + 1] = i * 1.5;
       	palette[(background_start_pen + 64 + i) * 3 + 2] = i;
    }
}

void rescue_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


    galaxian_vh_convert_color_prom(palette, colortable, color_prom);


	/* set up background colors */

   	/* graduated blue */

   	for (i = 0; i < 64; i++)
    {
		palette[(background_start_pen + i) * 3 + 0] = 0;
       	palette[(background_start_pen + i) * 3 + 1] = i * 2;
       	palette[(background_start_pen + i) * 3 + 2] = i * 4;
    }
}

void mariner_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;


    galaxian_vh_convert_color_prom(palette, colortable, color_prom);


	/* set up background colors */

   	/* 16 shades of blue - the 4 bits are connected to the following resistors

  		bit 0 -- 4.7 kohm resistor
        	  -- 2.2 kohm resistor
        	  -- 1   kohm resistor
  		bit 0 -- .47 kohm resistor */

   	for (i = 0; i < 16; i++)
    {
		int bit0,bit1,bit2,bit3;

		bit0 = (i >> 0) & 0x01;
		bit1 = (i >> 1) & 0x01;
		bit2 = (i >> 2) & 0x01;
		bit3 = (i >> 3) & 0x01;

		palette[(background_start_pen + i) * 3 + 0] = 0;
       	palette[(background_start_pen + i) * 3 + 1] = 0;
       	palette[(background_start_pen + i) * 3 + 2] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
    }
}


/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/

int galaxian_plain_vh_start(void)
{
	extern struct GameDriver driver_newsin7;


    modify_charcode = 0;
    modify_spritecode = 0;
    modify_color = 0;
    modify_ypos = 0;

	mooncrst_gfxextend = 0;

	draw_bullets = 0;

	draw_background = 0;
	background_red = 0;
	background_green = 0;
	background_blue = 0;

	flip_screen_x_set(0);
	flip_screen_y_set(0);


	/* all the games except New Sinbad 7 clip the sprites at the top of the screen,
	   New Sinbad 7 does it at the bottom */
	if (Machine->gamedrv == &driver_newsin7)
	{
		spritevisiblearea      = &_spritevisibleareaflipx;
        spritevisibleareaflipx = &_spritevisiblearea;
	}
	else
	{
		spritevisiblearea      = &_spritevisiblearea;
        spritevisibleareaflipx = &_spritevisibleareaflipx;
	}


	return 0;
}

int galaxian_vh_start(void)
{
	int ret = galaxian_plain_vh_start();

	draw_stars = galaxian_draw_stars;

	draw_bullets = galaxian_draw_bullets;

	return ret;
}

int mooncrst_vh_start(void)
{
	int ret = galaxian_vh_start();

	modify_charcode   = mooncrst_modify_charcode;
	modify_spritecode = mooncrst_modify_spritecode;

	return ret;
}

int moonqsr_vh_start(void)
{
	int ret = galaxian_vh_start();

	modify_charcode   = moonqsr_modify_charcode;
	modify_spritecode = moonqsr_modify_spritecode;

	return ret;
}

int pisces_vh_start(void)
{
	int ret = galaxian_vh_start();

	modify_charcode   = pisces_modify_charcode;
	modify_spritecode = pisces_modify_spritecode;

	return ret;
}

int batman2_vh_start(void)
{
	int ret = galaxian_vh_start();

	modify_charcode   = batman2_modify_charcode;
	modify_spritecode = batman2_modify_spritecode;

	return ret;
}

int scramble_vh_start(void)
{
	int ret = galaxian_plain_vh_start();

	draw_stars = scramble_draw_stars;

	draw_bullets = scramble_draw_bullets;

	draw_background = scramble_draw_background;

	return ret;
}

int turtles_vh_start(void)
{
	int ret = scramble_vh_start();

	draw_background = turtles_draw_background;

	return ret;
}

int theend_vh_start(void)
{
	int ret = scramble_vh_start();

	draw_bullets = theend_draw_bullets;

	return ret;
}

int darkplnt_vh_start(void)
{
	int ret = galaxian_plain_vh_start();

	draw_bullets = darkplnt_draw_bullets;

	return ret;
}

int rescue_vh_start(void)
{
	int ret = scramble_vh_start();

	draw_stars = rescue_draw_stars;

	draw_background = rescue_draw_background;

    return ret;
}

int minefld_vh_start(void)
{
	int ret = scramble_vh_start();

	draw_stars = rescue_draw_stars;

	draw_background = minefld_draw_background;

    return ret;
}

int stratgyx_vh_start(void)
{
	int ret = galaxian_plain_vh_start();

	draw_background = stratgyx_draw_background;

    return ret;
}

int ckongs_vh_start(void)
{
	int ret = scramble_vh_start();

	modify_spritecode = ckongs_modify_spritecode;

	return ret;
}

int calipso_vh_start(void)
{
	int ret = galaxian_plain_vh_start();

	draw_bullets = scramble_draw_bullets;

	draw_background = scramble_draw_background;

	modify_spritecode = calipso_modify_spritecode;

	return ret;
}

int mariner_vh_start(void)
{
	int ret = galaxian_plain_vh_start();

	draw_stars = mariner_draw_stars;

	draw_bullets = scramble_draw_bullets;

	draw_background = mariner_draw_background;

	modify_charcode = mariner_modify_charcode;

	return ret;
}

int froggers_vh_start(void)
{
	int ret = galaxian_plain_vh_start();

	draw_background = frogger_draw_background;

	return ret;
}

int frogger_vh_start(void)
{
	int ret = froggers_vh_start();

	modify_color = frogger_modify_color;
	modify_ypos = frogger_modify_ypos;

	return ret;
}

int froggrmc_vh_start(void)
{
	int ret = froggers_vh_start();

	modify_color = frogger_modify_color;

	return ret;
}

int jumpbug_vh_start(void)
{
	int ret = scramble_vh_start();

	draw_stars = jumpbug_draw_stars;

	modify_charcode   = jumpbug_modify_charcode;
	modify_spritecode = jumpbug_modify_spritecode;

	return ret;
}


WRITE_HANDLER( galaxian_videoram_w )
{
	galaxian_videoram[offset] = data;
}

READ_HANDLER( galaxian_videoram_r )
{
	return galaxian_videoram[offset];
}


WRITE_HANDLER( galaxian_flip_screen_x_w )
{
	flip_screen_x_set(data);
}

WRITE_HANDLER( galaxian_flip_screen_y_w )
{
	flip_screen_y_set(data);
}


WRITE_HANDLER( scramble_background_red_w )
{
	background_red = data & 1;
}

WRITE_HANDLER( scramble_background_green_w )
{
	background_green = data & 1;
}

WRITE_HANDLER( scramble_background_blue_w )
{
	background_blue = data & 1;
}


WRITE_HANDLER( galaxian_stars_enable_w )
{
	galaxian_stars_on = data & 1;
}


WRITE_HANDLER( darkplnt_bullet_color_w )
{
	darkplnt_bullet_color = data & 1;
}


WRITE_HANDLER( mooncrst_gfxextend_w )
{
	if (data)
		mooncrst_gfxextend |= (1 << offset);
	else
		mooncrst_gfxextend &= ~(1 << offset);
}

WRITE_HANDLER( mooncrgx_gfxextend_w )
{
  /* for the Moon Cresta bootleg on Galaxian H/W the gfx_extend is
     located at 0x6000-0x6002.  Also, 0x6000 and 0x6001 are reversed. */
     if (offset == 1)
       offset = 0;
     else if(offset == 0)
       offset = 1;    /* switch 0x6000 and 0x6001 */
	mooncrst_gfxextend_w(offset, data);
}

WRITE_HANDLER( pisces_gfxbank_w )
{
	set_vh_global_attribute( &pisces_gfxbank, data & 1 );
}

WRITE_HANDLER( jumpbug_gfxbank_w )
{
	set_vh_global_attribute( &jumpbug_gfxbank[offset], data & 1 );
}


/* character banking functions */

static void mooncrst_modify_charcode(int *code,int x)
{
	if ((mooncrst_gfxextend & 4) && (*code & 0xc0) == 0x80)
	{
		*code = (*code & 0x3f) | (mooncrst_gfxextend << 6);
	}
}

static void moonqsr_modify_charcode(int *code,int x)
{
	if (galaxian_attributesram[(x << 1) | 1] & 0x20)
	{
		*code += 256;
	}

    mooncrst_modify_charcode(code,x);
}

static void pisces_modify_charcode(int *code,int x)
{
	if (pisces_gfxbank)
	{
		*code += 256;
	}
}

static void batman2_modify_charcode(int *code,int x)
{
	if ((*code & 0x80) && pisces_gfxbank)
	{
		*code += 256;
	}
}

static void mariner_modify_charcode(int *code,int x)
{
	UINT8 *prom;


	/* bit 0 of the PROM controls character banking */

	prom = memory_region(REGION_USER2);

	if (prom[x] & 0x01)
	{
		*code += 256;
	}
}

static void jumpbug_modify_charcode(int *code,int offs)
{
	if (((*code & 0xc0) == 0x80) &&
		 (jumpbug_gfxbank[2] & 1) != 0)
	{
		*code += 128 + (( jumpbug_gfxbank[0] & 1) << 6) +
					   (( jumpbug_gfxbank[1] & 1) << 7) +
					   ((~jumpbug_gfxbank[4] & 1) << 8);
	}
}


/* sprite banking functions */

static void mooncrst_modify_spritecode(int *code,int *flipx,int *flipy,int offs)
{
	if ((mooncrst_gfxextend & 4) && (*code & 0x30) == 0x20)
	{
		*code = (*code & 0x0f) | (mooncrst_gfxextend << 4);
	}
}

static void moonqsr_modify_spritecode(int *code,int *flipx,int *flipy,int offs)
{
	if (galaxian_spriteram[offs + 2] & 0x20)
	{
		*code += 64;
	}

    mooncrst_modify_spritecode(code, flipx, flipy, offs);
}

static void ckongs_modify_spritecode(int *code,int *flipx,int *flipy,int offs)
{
	if (galaxian_spriteram[offs + 2] & 0x10)
	{
		*code += 64;
	}
}

static void calipso_modify_spritecode(int *code,int *flipx,int *flipy,int offs)
{
	/* No flips */
	*code = galaxian_spriteram[offs + 1];
	*flipx = 0;
	*flipy = 0;
}

static void pisces_modify_spritecode(int *code,int *flipx,int *flipy,int offs)
{
	if (pisces_gfxbank)
	{
		*code += 64;
	}
}

static void batman2_modify_spritecode(int *code,int *flipx,int *flipy,int offs)
{
	/* only the upper 64 sprites are used */

	*code += 64;
}

static void jumpbug_modify_spritecode(int *code,int *flipx,int *flipy,int offs)
{
	if (((*code & 0x30) == 0x20) &&
		 (jumpbug_gfxbank[2] & 1) != 0)
	{
		*code += 32 + (( jumpbug_gfxbank[0] & 1) << 4) +
					  (( jumpbug_gfxbank[1] & 1) << 5) +
					  ((~jumpbug_gfxbank[4] & 1) << 6);
	}
}


/* color PROM mapping functions */

static void frogger_modify_color(int *color)
{
	*color = ((*color >> 1) & 0x03) | ((*color << 2) & 0x04);
}


/* y position mapping functions */

static void frogger_modify_ypos(UINT8 *sy)
{
	*sy = (*sy << 4) | (*sy >> 4);
}


/* bullet drawing functions */

static void galaxian_draw_bullets(struct osd_bitmap *bitmap, int offs, int x, int y)
{
	int i;


	for (i = 0; i < 3; i++)
	{
		x--;

		if (x >= Machine->visible_area.min_x &&
			x <= Machine->visible_area.max_x)
		{
			int color;


			/* yellow missile, white shells (this is the terminology on the schematics) */
			color = ((offs == 7*4) ? BULLETS_COLOR_BASE : BULLETS_COLOR_BASE + 1);

			plot_pixel(bitmap, x, y, Machine->pens[color]);
		}
	}
}

static void scramble_draw_bullets(struct osd_bitmap *bitmap, int offs, int x, int y)
{
	x = x - 7;

	if (x >= Machine->visible_area.min_x &&
		x <= Machine->visible_area.max_x)
	{
		/* yellow bullets */
		plot_pixel(bitmap, x, y, Machine->pens[BULLETS_COLOR_BASE]);
	}
}

static void darkplnt_draw_bullets(struct osd_bitmap *bitmap, int offs, int x, int y)
{
	x = x - 7;

	if (x >= Machine->visible_area.min_x &&
		x <= Machine->visible_area.max_x)
	{
		plot_pixel(bitmap, x, y, Machine->pens[32 + darkplnt_bullet_color]);
	}
}

static void theend_draw_bullets(struct osd_bitmap *bitmap, int offs, int x, int y)
{
	int i;


	x = x - 3;

	for (i = 0; i < 4; i++)
	{
		x--;

		if (x >= Machine->visible_area.min_x &&
			x <= Machine->visible_area.max_x)
		{
			plot_pixel(bitmap, x, y, Machine->pens[BULLETS_COLOR_BASE]);
		}
	}
}


/* background drawing functions */

static void scramble_draw_background(struct osd_bitmap *bitmap)
{
	fillbitmap(bitmap,Machine->pens[background_start_pen + background_blue],&Machine->visible_area);
}

static void turtles_draw_background(struct osd_bitmap *bitmap)
{
	int color = (background_blue << 2) | (background_green << 1) | background_red;

	fillbitmap(bitmap,Machine->pens[background_start_pen + color],&Machine->visible_area);
}

static void frogger_draw_background(struct osd_bitmap *bitmap)
{
	if (flip_screen_x)
	{
		plot_box(bitmap,   0, 0, 120, 256, Machine->pens[background_start_pen]);
		plot_box(bitmap, 120, 0, 136, 256, Machine->pens[background_start_pen + 1]);
	}
	else
	{
		plot_box(bitmap,   0, 0, 136, 256, Machine->pens[background_start_pen + 1]);
		plot_box(bitmap, 136, 0, 120, 256, Machine->pens[background_start_pen]);
	}
}

static void stratgyx_draw_background(struct osd_bitmap *bitmap)
{
	UINT8 x;
	UINT8 *prom;


    /* the background PROM is connected the following way:

       bit 0 = 0 enables the blue gun if BCB is asserted
       bit 1 = 0 enables the red gun if BCR is asserted and
                 the green gun if BCG is asserted
       bits 2-7 are unconnected */

	prom = memory_region(REGION_USER1);

	for (x = 0; x < 32; x++)
	{
		int sx,color;


		color = 0;

		if ((~prom[x] & 0x02) && background_red)   color |= 0x01;
		if ((~prom[x] & 0x02) && background_green) color |= 0x02;
		if ((~prom[x] & 0x01) && background_blue)  color |= 0x04;

		if (flip_screen_x)
		{
			sx = 8 * (31 - x);
		}
		else
		{
			sx = 8 * x;
		}

		plot_box(bitmap, sx, 0, 8, 256, Machine->pens[background_start_pen + color]);
	}
}

static void minefld_draw_background(struct osd_bitmap *bitmap)
{
	if (background_blue)
	{
		int x;


		for (x = 0; x < 64; x++)
		{
			plot_box(bitmap, x * 2,        0, 2, 256, Machine->pens[background_start_pen + x]);
		}

		for (x = 0; x < 60; x++)
		{
			plot_box(bitmap, (x + 64) * 2, 0, 2, 256, Machine->pens[background_start_pen + x + 64]);
		}

		plot_box(bitmap, 248, 0, 16, 256, Machine->pens[background_start_pen]);
	}
	else
	{
		fillbitmap(bitmap,Machine->pens[background_start_pen],&Machine->visible_area);
	}
}

static void rescue_draw_background(struct osd_bitmap *bitmap)
{
	if (background_blue)
	{
		int x;


		for (x = 0; x < 64; x++)
		{
			plot_box(bitmap, x * 2,        0, 2, 256, Machine->pens[background_start_pen + x]);
		}

		for (x = 0; x < 60; x++)
		{
			plot_box(bitmap, (x + 64) * 2, 0, 2, 256, Machine->pens[background_start_pen + x + 4]);
		}

		plot_box(bitmap, 248, 0, 16, 256, Machine->pens[background_start_pen]);
	}
	else
	{
		fillbitmap(bitmap,Machine->pens[background_start_pen],&Machine->visible_area);
	}
}

static void mariner_draw_background(struct osd_bitmap *bitmap)
{
	UINT8 x;
	UINT8 *prom;


    /* the background PROM contains the color codes for each 8 pixel
       line (column) of the screen.  The first 0x20 bytes for unflipped,
       and the 2nd 0x20 bytes for flipped screen. */

	prom = memory_region(REGION_USER1);

	if (flip_screen_x)
	{
		for (x = 0; x < 32; x++)
		{
			int color;


			if (x == 0)
				color = 0;
			else
				color = prom[0x20 + x - 1];

			plot_box(bitmap, 8 * (31 - x), 0, 8, 256, Machine->pens[background_start_pen + color]);
		}
	}
	else
	{
		for (x = 0; x < 32; x++)
		{
			int color;


			if (x == 31)
				color = 0;
			else
				color = prom[x + 1];

			plot_box(bitmap, 8 * x, 0, 8, 256, Machine->pens[background_start_pen + color]);
		}
	}
}


/* star drawing functions */

void galaxian_init_stars(unsigned char **palette)
{
	int i;
	int total_stars;
	UINT32 generator;
	int x,y;


	draw_stars = 0;
	galaxian_stars_on = 0;
	stars_blink_state = 0;
	if (stars_blink_timer)  timer_remove(stars_blink_timer);


	for (i = 0;i < 64;i++)
	{
		int bits;
		int map[4] = { 0x00, 0x88, 0xcc, 0xff };


		bits = (i >> 0) & 0x03;
		*((*palette)++) = map[bits];
		bits = (i >> 2) & 0x03;
		*((*palette)++) = map[bits];
		bits = (i >> 4) & 0x03;
		*((*palette)++) = map[bits];
	}


	/* precalculate the star background */

	total_stars = 0;
	generator = 0;

	for (y = 255;y >= 0;y--)
	{
		for (x = 511;x >= 0;x--)
		{
			UINT32 bit0;


			bit0 = ((~generator >> 16) & 1) ^ ((generator >> 4) & 1);

			generator = (generator << 1) | bit0;

			if (((~generator >> 16) & 1) && (generator & 0xff) == 0xff)
			{
				int color;

				color = (~(generator >> 8)) & 0x3f;
				if (color)
				{
					stars[total_stars].x = x;
					stars[total_stars].y = y;
					stars[total_stars].color = color;

					total_stars++;
				}
			}
		}
	}
}

static void plot_star(struct osd_bitmap *bitmap, int x, int y, int color)
{
	if (y < Machine->visible_area.min_y ||
		y > Machine->visible_area.max_y ||
	    x < Machine->visible_area.min_x ||
		x > Machine->visible_area.max_x)
		return;


	if (flip_screen_x)
	{
		x = 255 - x;
	}
	if (flip_screen_y)
	{
		y = 255 - y;
	}

	plot_pixel(bitmap, x, y, Machine->pens[STARS_COLOR_BASE + color]);
}

static void galaxian_draw_stars(struct osd_bitmap *bitmap)
{
	int offs;
	int currentframe;


	currentframe = cpu_getcurrentframe();

	for (offs = 0;offs < STAR_COUNT;offs++)
	{
		int x,y;


		x = ((stars[offs].x +   currentframe) & 0x01ff) >> 1;
		y = ( stars[offs].y + ((currentframe + stars[offs].x) >> 9)) & 0xff;

		if ((y & 1) ^ ((x >> 3) & 1))
		{
			plot_star(bitmap, x, y, stars[offs].color);
		}
	}
}

void scramble_draw_stars(struct osd_bitmap *bitmap)
{
	int offs;


	if (stars_blink_timer == 0)
	{
		start_stars_blink_timer(100000, 10000, 0.00001);
	}


	for (offs = 0;offs < STAR_COUNT;offs++)
	{
		int x,y;


		x = stars[offs].x >> 1;
		y = stars[offs].y;

		if ((y & 1) ^ ((x >> 3) & 1))
		{
			/* determine when to skip plotting */
			switch (stars_blink_state & 0x03)
			{
			case 0:
				if (!(stars[offs].color & 1))  continue;
				break;
			case 1:
				if (!(stars[offs].color & 4))  continue;
				break;
			case 2:
				if (!(stars[offs].y & 2))  continue;
				break;
			case 3:
				/* always plot */
				break;
			}

			plot_star(bitmap, x, y, stars[offs].color);
		}
	}
}

static void rescue_draw_stars(struct osd_bitmap *bitmap)
{
	int offs;


	/* same as Scramble, but only top (left) half of screen */

	if (stars_blink_timer == 0)
	{
		start_stars_blink_timer(100000, 10000, 0.00001);
	}


	for (offs = 0;offs < STAR_COUNT;offs++)
	{
		int x,y;


		x = stars[offs].x >> 1;
		y = stars[offs].y;

		if ((x < 128) && ((y & 1) ^ ((x >> 3) & 1)))
		{
			/* determine when to skip plotting */
			switch (stars_blink_state & 0x03)
			{
			case 0:
				if (!(stars[offs].color & 1))  continue;
				break;
			case 1:
				if (!(stars[offs].color & 4))  continue;
				break;
			case 2:
				if (!(stars[offs].y & 2))  continue;
				break;
			case 3:
				/* always plot */
				break;
			}

			plot_star(bitmap, x, y, stars[offs].color);
		}
	}
}

static void mariner_draw_stars(struct osd_bitmap *bitmap)
{
	int offs;
	UINT8 *prom;
	int currentframe;


	/* bit 2 of the PROM controls star visibility */

	prom = memory_region(REGION_USER2);

	currentframe = cpu_getcurrentframe();

	for (offs = 0;offs < STAR_COUNT;offs++)
	{
		int x,y;


		x = ((stars[offs].x +   -currentframe) & 0x01ff) >> 1;
		y = ( stars[offs].y + ((-currentframe + stars[offs].x) >> 9)) & 0xff;

		if ((y & 1) ^ ((x >> 3) & 1))
		{
			if (prom[(x/8 + 1) & 0x1f] & 0x04)
			{
				plot_star(bitmap, x, y, stars[offs].color);
			}
		}
	}
}

static void jumpbug_draw_stars(struct osd_bitmap *bitmap)
{
	int offs;
	int currentframe;


	if (stars_blink_timer == 0)
	{
		start_stars_blink_timer(100000, 10000, 0.00001);
	}


	currentframe = cpu_getcurrentframe();

	for (offs = 0;offs < STAR_COUNT;offs++)
	{
		int x,y;


		x = stars[offs].x >> 1;
		y = stars[offs].y;

		/* determine when to skip plotting */
		if ((y & 1) ^ ((x >> 3) & 1))
		{
			switch (stars_blink_state & 0x03)
			{
			case 0:
				if (!(stars[offs].color & 1))  continue;
				break;
			case 1:
				if (!(stars[offs].color & 4))  continue;
				break;
			case 2:
				if (!(stars[offs].y & 2))  continue;
				break;
			case 3:
				/* always plot */
				break;
			}

			x = ((stars[offs].x +   currentframe) & 0x01ff) >> 1;
			y = ( stars[offs].y + ((currentframe + stars[offs].x) >> 9)) & 0xff;

			/* no stars in the status area */
			if (x >= 240)  continue;

			plot_star(bitmap, x, y, stars[offs].color);
		}
	}
}


static void stars_blink_callback(int param)
{
	stars_blink_state++;
}

static void start_stars_blink_timer(double ra, double rb, double c)
{
	/* calculate the period using the formula given in the 555 datasheet */

	double period = 0.693 * (ra + 2.0 * rb) * c;

	stars_blink_timer = timer_pulse(TIME_IN_SEC(period), 0, stars_blink_callback);
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void galaxian_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int x,y;
	int offs,color_mask;
	int transparency;


	color_mask = (Machine->gfx[0]->color_granularity == 4) ? 7 : 3;


	/* draw the bacground */
	if (draw_background)
	{
		draw_background(bitmap);
	}
	else
	{
		if (draw_stars)
		{
			/* black base for stars */
			fillbitmap(bitmap,Machine->pens[background_start_pen],&Machine->visible_area);
		}
	}


	/* draw the stars */
	if (draw_stars && galaxian_stars_on)
	{
		draw_stars(bitmap);
	}


	/* draw the character layer */
	transparency = (draw_background || draw_stars) ? TRANSPARENCY_PEN : TRANSPARENCY_NONE;

	for (x = 0; x < 32; x++)
	{
		UINT8 sx,scroll;
		int color;


		scroll = galaxian_attributesram[ x << 1];
		color  = galaxian_attributesram[(x << 1) | 1] & color_mask;

		if (modify_color)
		{
			modify_color(&color);
		}

		if (modify_ypos)
		{
			modify_ypos(&scroll);
		}


		sx = 8 * x;

		if (flip_screen_x)
		{
			sx = 248 - sx;
		}


		for (y = 0; y < 32; y++)
		{
			UINT8 sy;
			int code;


			sy = (8 * y) - scroll;

			if (flip_screen_y)
			{
				sy = 248 - sy;
			}


			code = galaxian_videoram[(y << 5) | x];

			if (modify_charcode)
			{
				modify_charcode(&code, x);
			}

			drawgfx(bitmap,Machine->gfx[0],
					code,color,
					flip_screen_x,flip_screen_y,
					sx,sy,
					0, transparency, 0);
		}
	}


	/* draw the bullets */
	if (draw_bullets)
	{
		for (offs = 0;offs < galaxian_bulletsram_size;offs += 4)
		{
			y = 255 - galaxian_bulletsram[offs + 1];
			x = 255 - galaxian_bulletsram[offs + 3];

			if (y < Machine->visible_area.min_y ||
				y > Machine->visible_area.max_y)
				continue;

			if (flip_screen_y)  y = 255 - y;

			draw_bullets(bitmap, offs, x, y);
		}
	}


	/* draw the sprites */
	for (offs = galaxian_spriteram_size - 4;offs >= 0;offs -= 4)
	{
		UINT8 sx,sy;
		int flipx,flipy,code,color;


		sx = galaxian_spriteram[offs + 3]; /* This is definately correct in Mariner. Look at
												  the 'gate' moving up/down. It stops at the
  												  right spots */
		sy = galaxian_spriteram[offs];
		flipx = galaxian_spriteram[offs + 1] & 0x40;
		flipy = galaxian_spriteram[offs + 1] & 0x80;
		code = galaxian_spriteram[offs + 1] & 0x3f;
		color = galaxian_spriteram[offs + 2] & color_mask;

		if (modify_spritecode)
		{
			modify_spritecode(&code, &flipx, &flipy, offs);
		}

		if (modify_color)
		{
			modify_color(&color);
		}

		if (modify_ypos)
		{
			modify_ypos(&sy);
		}

		if (flip_screen_x)
		{
			sx = 240 - sx;	/* I checked a bunch of games including Scramble
							   (# of pixels the ship is from the top of the mountain),
			                   Mariner and Checkman. This is correct for them */
			flipx = !flipx;
		}
		else
		{
			sx += 2;
		}

		if (flip_screen_y)
		{
			flipy = !flipy;
			if (offs >= 3*4)
				sy++;
			else
				sy += 2;
		}
		else
		{
			sy = 240 - sy;
			if (offs >= 3*4) sy++;
		}

		/* In Amidar, */
		/* Sprites #0, #1 and #2 need to be offset one pixel to be correctly */
		/* centered on the ladders in Turtles (we move them down, but since this */
		/* is a rotated game, we actually move them left). */
		/* Note that the adjustment must be done AFTER handling flipscreen, thus */
		/* proving that this is a hardware related "feature" */
		/* This is not Amidar, it is Galaxian/Scramble/hundreds of clones, and I'm */
		/* not sure it should be the same. A good game to test alignment is Armored Car */

		drawgfx(bitmap,Machine->gfx[1],
				code,color,
				flipx,flipy,
				sx,sy,
				flip_screen_x ? spritevisibleareaflipx : spritevisiblearea,TRANSPARENCY_PEN,0);
	}
}


int hunchbks_vh_interrupt(void)
{
	cpu_irq_line_vector_w(0,0,0x03);
	cpu_set_irq_line(0,0,PULSE_LINE);

	return ignore_interrupt();
}
