/***************************************************************************

Donkey Kong memory map (preliminary)

0000-3fff ROM (Donkey Kong Jr.: 0000-5fff)
6000-6fff RAM
7000-73ff ?
7400-77ff Video RAM


memory mapped ports:

read:
7c00      IN0
7c80      IN1
7d00      IN2
7d80      DSW1

*
 * IN0 (bits NOT inverted)
 * bit 7 : ?
 * bit 6 : reset (when player 1 active)
 * bit 5 : ?
 * bit 4 : JUMP player 1
 * bit 3 : DOWN player 1
 * bit 2 : UP player 1
 * bit 1 : LEFT player 1
 * bit 0 : RIGHT player 1
 *
*
 * IN1 (bits NOT inverted)
 * bit 7 : ?
 * bit 6 : reset (when player 2 active)
 * bit 5 : ?
 * bit 4 : JUMP player 2
 * bit 3 : DOWN player 2
 * bit 2 : UP player 2
 * bit 1 : LEFT player 2
 * bit 0 : RIGHT player 2
 *
*
 * IN2 (bits NOT inverted)
 * bit 7 : COIN
 * bit 6 : ?
 * bit 5 : ?
 * bit 4 : ?
 * bit 3 : START 2
 * bit 2 : START 1
 * bit 1 : ?
 * bit 0 : ? if this is 1, the code jumps to $4000, outside the rom space
 *
*
 * DSW1 (bits NOT inverted)
 * bit 7 : COCKTAIL or UPRIGHT cabinet (1 = UPRIGHT)
 * bit 6 : \ 000 = 1 coin 1 play   001 = 2 coins 1 play  010 = 1 coin 2 plays
 * bit 5 : | 011 = 3 coins 1 play  100 = 1 coin 3 plays  101 = 4 coins 1 play
 * bit 4 : / 110 = 1 coin 4 plays  111 = 5 coins 1 play
 * bit 3 : \bonus at
 * bit 2 : / 00 = 7000  01 = 10000  10 = 15000  11 = 20000
 * bit 1 : \ 00 = 3 lives  01 = 4 lives
 * bit 0 : / 10 = 5 lives  11 = 6 lives
 *

write:
6900-6a7f sprites
7800-7803 ?
7808      ?
7c00      Background sound/music select:
          00 - nothing
		  01 - Intro tune
		  02 - How High? (intermisson) tune
		  03 - Out of time
		  04 - Hammer
		  05 - Rivet level 2 completed (end tune)
		  06 - Hammer hit
		  07 - Standard level end
		  08 - Background 1	(first screen)
		  09 - ???
		  0A - Background 3	(springs)
		  0B - Background 2 (rivet)
		  0C - Rivet level 1 completed (end tune)
		  0D - Rivet removed
		  0E - Rivet level completed
		  0F - Gorilla roar
7c80      gfx bank select (Donkey Kong Jr. only)
7d00      digital sound trigger - walk
7d01      digital sound trigger - jump
7d02      digital sound trigger - boom (gorilla stomps foot)
7d03      digital sound trigger - coin input/spring
7d04      digital sound trigger	- gorilla fall
7d05      digital sound trigger - barrel jump/prize
7d06      ?
7d07      ?
7d80      digital sound trigger - dead
7d82      ?
7d83      ?
7d84      interrupt enable
7d85      0/1 toggle
7d86      sound processor control?
7d87      sound processor control?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



void dkongjr_gfxbank_w(int offset,int data);
void dkong_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
int dkong_vh_start(void);
void dkong_vh_screenrefresh(struct osd_bitmap *bitmap);

void dkong_sh1_w(int offset,int data);
void dkong_sh2_w(int offset,int data);
void dkong_sh3_w(int offset,int data);
void dkong_sh_update(void);



static struct MemoryReadAddress dkong_readmem[] =
{
	{ 0x6000, 0x6fff, MRA_RAM },	/* including sprites RAM */
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x7c00, 0x7c00, input_port_0_r },	/* IN0 */
	{ 0x7c80, 0x7c80, input_port_1_r },	/* IN1 */
	{ 0x7d00, 0x7d00, input_port_2_r },	/* IN2 */
	{ 0x7d80, 0x7d80, input_port_3_r },	/* DSW1 */
	{ 0x7400, 0x77ff, MRA_RAM },	/* video RAM */
	{ -1 }	/* end of table */
};
static struct MemoryReadAddress dkongjr_readmem[] =
{
	{ 0x6000, 0x6fff, MRA_RAM },	/* including sprites RAM */
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x7c00, 0x7c00, input_port_0_r },	/* IN0 */
	{ 0x7c80, 0x7c80, input_port_1_r },	/* IN1 */
	{ 0x7d00, 0x7d00, input_port_2_r },	/* IN2 */
	{ 0x7d80, 0x7d80, input_port_3_r },	/* DSW1 */
	{ 0x7400, 0x77ff, MRA_RAM },	/* video RAM */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress dkong_writemem[] =
{
	{ 0x6000, 0x68ff, MWA_RAM },
	{ 0x6a80, 0x6fff, MWA_RAM },
	{ 0x6900, 0x6a7f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x7d84, 0x7d84, interrupt_enable_w },
	{ 0x7400, 0x77ff, videoram_w, &videoram, &videoram_size },
	{ 0x7c80, 0x7c80, dkongjr_gfxbank_w },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x7800, 0x7803, MWA_RAM },	/* ???? */
	{ 0x7808, 0x7808, MWA_RAM },	/* ???? */
	{ 0x7c00, 0x7c00, dkong_sh2_w },    	/* ???? */
	{ 0x7d00, 0x7d07, dkong_sh1_w },    /* ???? */
	{ 0x7d80, 0x7d80, dkong_sh3_w },
	{ 0x7d81, 0x7d83, MWA_RAM },	/* ???? */
	{ 0x7d85, 0x7d87, MWA_RAM },	/* ???? */
	{ -1 }	/* end of table */
};
static struct MemoryWriteAddress dkongjr_writemem[] =
{
	{ 0x6000, 0x68ff, MWA_RAM },
	{ 0x6a80, 0x6fff, MWA_RAM },
	{ 0x6900, 0x6a7f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x7d84, 0x7d84, interrupt_enable_w },
	{ 0x7400, 0x77ff, videoram_w, &videoram, &videoram_size },
	{ 0x7c80, 0x7c80, dkongjr_gfxbank_w },
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x7800, 0x7803, MWA_RAM },	/* ???? */
	{ 0x7808, 0x7808, MWA_RAM },	/* ???? */
	{ 0x7c00, 0x7c00, MWA_RAM },	/* ???? */
	{ 0x7d00, 0x7d07, MWA_RAM },	/* ???? */
	{ 0x7d80, 0x7d83, MWA_RAM },	/* ???? */
	{ 0x7d85, 0x7d87, MWA_RAM },	/* ???? */
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_UP, OSD_KEY_DOWN,
				OSD_KEY_CONTROL, 0, 0, 0 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_UP, OSD_JOY_DOWN,
				OSD_JOY_FIRE, 0, 0, 0 },
	},
	{	/* IN1 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0x00,
		{ 0, 0, OSD_KEY_1, OSD_KEY_2, 0, 0, 0, OSD_KEY_3 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0x84,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};

static struct KEYSet keys[] =
{
        { 0, 2, "MOVE UP" },
        { 0, 1, "MOVE LEFT"  },
        { 0, 0, "MOVE RIGHT" },
        { 0, 3, "MOVE DOWN" },
        { 0, 4, "JUMP" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 3, 0x03, "LIVES", { "3", "4", "5", "6" } },
	{ 3, 0x0c, "BONUS", { "7000", "10000", "15000", "20000" } },
	{ -1 }
};


static struct GfxLayout dkong_charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 256*8*8, 0 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout dkongjr_charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 128*16*16, 0 },	/* the two bitplanes are separated */
	{ 15*8, 14*8, 13*8, 12*8, 11*8, 10*8, 9*8, 8*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,	/* the two halves of the sprite are separated */
			64*16*16+0, 64*16*16+1, 64*16*16+2, 64*16*16+3, 64*16*16+4, 64*16*16+5, 64*16*16+6, 64*16*16+7 },
	16*8	/* every sprite takes 16 consecutive bytes */
};



static struct GfxDecodeInfo dkong_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &dkong_charlayout,   0, 64 },
	{ 1, 0x1000, &spritelayout,    64*4, 16 },
	{ -1 } /* end of array */
};
static struct GfxDecodeInfo dkongjr_gfxdecodeinfo[] =
{
	{ 1, 0x0000, &dkongjr_charlayout,     0, 128 },
	{ 1, 0x2000, &spritelayout,       128*4,  16 },
	{ -1 } /* end of array */
};



static unsigned char dkong_color_prom[] =
{
	/* 2j - palette high 4 bits (inverted) */
	0xFF,0xF6,0xFE,0xF1,0xFF,0xF0,0xF1,0xF0,0xFF,0xF0,0xF1,0xFF,0xFF,0xFF,0xF1,0xFE,
	0xFF,0xF1,0xF0,0xF1,0xFF,0xF0,0xF1,0xF0,0xFF,0xF1,0xFF,0xFE,0xFF,0xF5,0xF0,0xF0,
	0xFF,0xF5,0xF0,0xF1,0xFF,0xF0,0xF1,0xF1,0xFF,0xFF,0xF0,0xF1,0xFF,0xFF,0xF1,0xF0,
	0xFF,0xF0,0xFE,0xFF,0xFF,0xF1,0xF0,0xF1,0xFF,0xF2,0xF0,0xFF,0xFF,0xF0,0xF1,0xFF,
	0xFF,0xF6,0xFE,0xF1,0xFF,0xF0,0xF1,0xF0,0xFF,0xF0,0xF1,0xFF,0xFF,0xF1,0xF0,0xF0,
	0xFF,0xF1,0xF0,0xF0,0xFF,0xF1,0xF0,0xF0,0xFF,0xF1,0xF0,0xF0,0xFF,0xF5,0xF0,0xF0,
	0xFF,0xF5,0xF0,0xF1,0xFF,0xF0,0xF1,0xF1,0xFF,0xFF,0xF0,0xF1,0xFF,0xFF,0xF1,0xF0,
	0xFF,0xF0,0xFE,0xFF,0xFF,0xF1,0xF0,0xF1,0xFF,0xF2,0xF0,0xFF,0xFF,0xF0,0xF1,0xFF,
	0xFF,0xF6,0xFE,0xF1,0xFF,0xF0,0xF1,0xF0,0xFF,0xF0,0xF1,0xFF,0xFF,0xF1,0xF7,0xFE,
	0xFF,0xF1,0xF7,0xFE,0xFF,0xF1,0xF7,0xFE,0xFF,0xF1,0xF7,0xFE,0xFF,0xF5,0xF0,0xF0,
	0xFF,0xF5,0xF0,0xF1,0xFF,0xF0,0xF1,0xF1,0xFF,0xFF,0xF0,0xF1,0xFF,0xFF,0xF1,0xF0,
	0xFF,0xF0,0xFE,0xFF,0xFF,0xF1,0xF0,0xF1,0xFF,0xF2,0xF0,0xFF,0xFF,0xF0,0xF1,0xFF,
	0xFF,0xF6,0xFE,0xF1,0xFF,0xF0,0xF1,0xF0,0xFF,0xF0,0xF1,0xFF,0xFF,0xFF,0xFE,0xF0,
	0xFF,0xFF,0xFE,0xF0,0xFF,0xFF,0xFE,0xF0,0xFF,0xFF,0xFE,0xF0,0xFF,0xF5,0xF0,0xF0,
	0xFF,0xF5,0xF0,0xF1,0xFF,0xF0,0xF1,0xF1,0xFF,0xFF,0xF0,0xF1,0xFF,0xFF,0xF1,0xF0,
	0xFF,0xF0,0xFE,0xFF,0xFF,0xF1,0xF0,0xF1,0xFF,0xF2,0xF0,0xFF,0xFF,0xF0,0xF1,0xFF,
	/* 2k - palette low 4 bits (inverted) */
	0xFF,0xFC,0xF0,0xFF,0xFF,0xFB,0xFF,0xF0,0xFF,0xFA,0xFF,0xFD,0xFF,0xFC,0xFF,0xF3,
	0xFF,0xF3,0xFB,0xFF,0xFF,0xF0,0xFD,0xF3,0xFF,0xFF,0xFC,0xF0,0xFF,0xFF,0xFA,0xF0,
	0xFF,0xFF,0xFA,0xF3,0xFF,0xF0,0xF3,0xF5,0xFF,0xFD,0xF0,0xF5,0xFF,0xFC,0xF3,0xFA,
	0xFF,0xF0,0xF0,0xFC,0xFF,0xFF,0xFB,0xF3,0xFF,0xFE,0xFA,0xFC,0xFF,0xFB,0xFF,0xFF,
	0xFF,0xFC,0xF0,0xFF,0xFF,0xFB,0xFF,0xF0,0xFF,0xFA,0xFF,0xFD,0xFF,0xF3,0xFA,0xF0,
	0xFF,0xF3,0xFA,0xF0,0xFF,0xF3,0xFA,0xF0,0xFF,0xF3,0xFA,0xF0,0xFF,0xFF,0xFA,0xF0,
	0xFF,0xFF,0xFA,0xF3,0xFF,0xF0,0xF3,0xF5,0xFF,0xFD,0xF0,0xF5,0xFF,0xFC,0xF3,0xFA,
	0xFF,0xF0,0xF0,0xFC,0xFF,0xFF,0xFB,0xF3,0xFF,0xFE,0xFA,0xFC,0xFF,0xFB,0xFF,0xFF,
	0xFF,0xFC,0xF0,0xFF,0xFF,0xFB,0xFF,0xF0,0xFF,0xFA,0xFF,0xFD,0xFF,0xFA,0xFF,0xF0,
	0xFF,0xFA,0xFF,0xF0,0xFF,0xFA,0xFF,0xF0,0xFF,0xFA,0xFF,0xF0,0xFF,0xFF,0xFA,0xF0,
	0xFF,0xFF,0xFA,0xF3,0xFF,0xF0,0xF3,0xF5,0xFF,0xFD,0xF0,0xF5,0xFF,0xFC,0xF3,0xFA,
	0xFF,0xF0,0xF0,0xFC,0xFF,0xFF,0xFB,0xF3,0xFF,0xFE,0xFA,0xFC,0xFF,0xFB,0xFF,0xFF,
	0xFF,0xFC,0xF0,0xFF,0xFF,0xFB,0xFF,0xF0,0xFF,0xFA,0xFF,0xFD,0xFF,0xFC,0xF0,0xFB,
	0xFF,0xFC,0xF0,0xFB,0xFF,0xFC,0xF0,0xFB,0xFF,0xFC,0xF0,0xFB,0xFF,0xFF,0xFA,0xF0,
	0xFF,0xFF,0xFA,0xF3,0xFF,0xF0,0xF3,0xF5,0xFF,0xFD,0xF0,0xF5,0xFF,0xFC,0xF3,0xFA,
	0xFF,0xF0,0xF0,0xFC,0xFF,0xFF,0xFB,0xF3,0xFF,0xFE,0xFA,0xFC,0xFF,0xFB,0xFF,0xFF,
	/* 5f - lookup table??? */
	0xF0,0xF1,0xF1,0xF2,0xF6,0xF6,0xF4,0xF6,0xF6,0xF6,0xF6,0xF3,0xF6,0xF3,0xF4,0xF3,
	0xF6,0xF6,0xF6,0xF6,0xF4,0xF3,0xF4,0xF5,0xF4,0xF6,0xF5,0xF3,0xF5,0xF3,0xF6,0xF3,
	0xF0,0xF1,0xF1,0xF2,0xF6,0xF6,0xF4,0xF6,0xF6,0xF6,0xF6,0xF3,0xF6,0xF3,0xF4,0xF3,
	0xF6,0xF6,0xF6,0xF6,0xF4,0xF3,0xF4,0xF5,0xF4,0xF6,0xF5,0xF3,0xF5,0xF3,0xF6,0xF3,
	0xF0,0xF1,0xF1,0xF2,0xF6,0xF6,0xF4,0xF6,0xF6,0xF6,0xF6,0xF3,0xF6,0xF3,0xF4,0xF3,
	0xF6,0xF6,0xF6,0xF6,0xF4,0xF3,0xF4,0xF5,0xF4,0xF6,0xF5,0xF3,0xF5,0xF3,0xF6,0xF3,
	0xF0,0xF1,0xF1,0xF2,0xF6,0xF6,0xF4,0xF6,0xF6,0xF6,0xF6,0xF3,0xF6,0xF3,0xF4,0xF3,
	0xF6,0xF6,0xF6,0xF6,0xF4,0xF3,0xF4,0xF5,0xF4,0xF6,0xF5,0xF3,0xF5,0xF3,0xF6,0xF3,
	0xF0,0xF1,0xF1,0xF2,0xF6,0xF6,0xF4,0xF6,0xF6,0xF6,0xF6,0xF3,0xF6,0xF3,0xF4,0xF3,
	0xF6,0xF6,0xF6,0xF6,0xF4,0xF3,0xF4,0xF5,0xF4,0xF6,0xF5,0xF3,0xF5,0xF3,0xF6,0xF3,
	0xF0,0xF1,0xF1,0xF2,0xF6,0xF6,0xF4,0xF6,0xF6,0xF6,0xF6,0xF3,0xF6,0xF3,0xF4,0xF3,
	0xF6,0xF6,0xF6,0xF6,0xF4,0xF3,0xF4,0xF5,0xF4,0xF6,0xF5,0xF3,0xF5,0xF3,0xF6,0xF3,
	0xF0,0xF1,0xF1,0xF2,0xF6,0xF6,0xF4,0xF6,0xF6,0xF6,0xF6,0xF3,0xF6,0xF3,0xF4,0xF3,
	0xF6,0xF6,0xF6,0xF6,0xF4,0xF3,0xF4,0xF5,0xF4,0xF6,0xF5,0xF3,0xF5,0xF3,0xF6,0xF3,
	0xF0,0xF1,0xF1,0xF2,0xF6,0xF6,0xF4,0xF6,0xF6,0xF6,0xF6,0xF3,0xF6,0xF3,0xF4,0xF3,
	0xF6,0xF6,0xF6,0xF6,0xF4,0xF3,0xF4,0xF5,0xF4,0xF6,0xF5,0xF3,0xF5,0xF3,0xF6,0xF3
};



static unsigned char palette[] =
{
	0x00,0x00,0x00, /* BLACK */
	0xff,0xff,0xff, /* WHITE */
	0xff,0x00,0x00, /* RED */
	0xff,0x00,0xff, /* PURPLE */
	168,0xff,0xff,  /* CYAN */
	255,200,184,    /* LTORANGE */
	184,0x00,0x00,  /* DKRED */
	0x00,0x00,0xff, /* BLUE */
	0xff,0xff,0x00, /* YELLOW */
	255,128,255,    /* PINK */
	120,192,255,    /* LTBLUE */
	0xff,96,0x00,   /* ORANGE */
	0x00,0xff,0x00, /* GREEN */
	136,0x00,0x00,  /* DKBROWN */
	255,176,112,    /* LTBROWN */
	0x00,96,0x00,   /* DKGREEN */
};

enum { BLACK,WHITE,RED,PURPLE,CYAN,LTORANGE,DKRED,BLUE,YELLOW,PINK,
		LTBLUE,ORANGE,GREEN,DKBROWN,LTBROWN,DKGREEN };

static unsigned char dkongjr_colortable[] =
{
	/* chars (first bank) */
	BLACK,BLACK,BLACK,WHITE,       /* 0-9 */
	BLACK,BLACK,BLACK,WHITE,       /* 0-9 */
	BLACK,BLACK,BLACK,WHITE,       /* 0-9 */
	BLACK,BLACK,BLACK,WHITE,       /* 0-9 */
	BLACK,BLACK,BLACK,RED,         /* A-Z */
	BLACK,BLACK,BLACK,RED,         /* A-Z */
	BLACK,BLACK,BLACK,RED,         /* A-Z */
	BLACK,BLACK,BLACK,RED,         /* A-Z */
	BLACK,BLACK,BLACK,RED,         /* A-Z */
	BLACK,BLACK,BLACK,RED,         /* A-Z */
	BLACK,BLACK,BLACK,RED,         /* A-Z */
	BLACK,BLACK,BLACK,RED,         /* A-Z */
	BLACK,BLACK,BLACK,RED,         /* A-Z */
	BLACK,BLACK,BLACK,RED,         /* A-Z */
	BLACK,BLACK,BLACK,RED,         /* A-Z */
	BLACK,BLACK,BLACK,RED,         /* A-Z */
	BLACK,BLACK,BLACK,WHITE,       /* RUB END */
	BLACK,BLACK,BLACK,WHITE,       /* RUB END */
	BLACK,BLACK,BLACK,WHITE,       /* RUB END */
	BLACK,BLACK,BLACK,WHITE,       /* RUB END */
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	BLACK,BLACK,RED,BLUE,          /* Bonus */
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	BLACK,BLACK,RED,BLUE,          /* Bonus Box */
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	BLACK,BLACK,RED,BLUE,          /* Bonus Box */
	BLACK,BLACK,RED,BLUE,          /* Bonus Box */
	BLACK,BLACK,RED,BLUE,          /* Bonus Box */
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	BLACK,BLACK,ORANGE,BLACK,      /* Screen1: Islands Bottom */
	BLACK,LTBLUE,BLUE,RED,         /* Screen4: Locks */
	BLACK,LTBLUE,BLUE,WHITE,       /* Screen1: Water */
	BLACK,BLACK,BLUE,BLACK,        /* Screen2: Rope Cage-Lock */
	BLACK,BLACK,BLUE,BLACK,        /* Screen2: Rope Cage-Lock */
	BLACK,GREEN,RED,LTBROWN,       /* Screen1: Vines */
	BLACK,GREEN,BLUE,LTBLUE,       /* Rope & Chains */
	BLACK,CYAN,BLUE,LTBLUE,        /* Screen2: Wall */
	BLACK,LTBLUE,BLUE,BLUE,        /* Screen4: Mario's Platform */
	BLACK,LTBLUE,BLUE,BLUE,        /* Screen4: Mario's Platform */
	BLACK,LTBLUE,BLUE,BLUE,        /* Screen4: Mario's Platform */
	0,0,0,0,                       /* Unused */
	0,0,0,0,                       /* Unused */
	BLACK,GREEN,DKGREEN,WHITE,     /* Screen1: Islands Top */
	BLACK,LTBLUE,BLUE,WHITE,       /* Screen1&2: Top Floor Bottom Part */
	BLACK,LTBLUE,BLUE,BLUE,        /* Screen4: Bottom Platform */
	BLACK,LTBLUE,BLUE,BLUE,        /* Screen4: Bottom Platform */
	BLACK,LTBLUE,ORANGE,YELLOW,    /* Screen1,2,4: Platform */
	BLACK,LTBLUE,BLUE,BLUE,        /* Screen4: Bottom Platform */
	BLACK,LTBLUE,BLUE,BLUE,        /* Screen4: Bottom Platform */
	BLACK,DKGREEN,DKBROWN,LTBROWN, /* Lives */

	/* chars (second bank) */
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	BLACK,YELLOW,RED,BLACK,        /* Logo: Donkey Kong */
	BLACK,YELLOW,RED,BLACK,        /* Logo: Donkey Kong */
	BLACK,YELLOW,RED,BLACK,        /* Logo: Donkey Kong */
	BLACK,YELLOW,RED,BLACK,        /* Logo: Donkey Kong */
	BLACK,YELLOW,RED,BLACK,        /* Logo: Donkey Kong */
	BLACK,YELLOW,RED,BLACK,        /* Logo: Donkey Kong */
	BLACK,YELLOW,RED,BLACK,        /* Logo: Donkey Kong */
	BLACK,YELLOW,RED,BLACK,        /* Logo: Donkey Kong */
	BLACK,YELLOW,RED,BLACK,        /* Logo: Donkey Kong */
	BLACK,YELLOW,RED,BLACK,        /* Logo: Donkey Kong */
	BLACK,YELLOW,RED,BLACK,        /* Logo: Donkey Kong */
	BLACK,YELLOW,RED,BLACK,        /* Logo: Donkey Kong */
	BLACK,YELLOW,RED,BLACK,        /* Logo: Donkey Kong */
	BLACK,YELLOW,RED,BLACK,        /* Logo: Donkey Kong */
	BLACK,YELLOW,RED,BLUE,         /* Logo: Donkey Kong, TM, Junior */
	BLACK,BLACK,RED,BLUE,          /* Logo: Junior */
	BLACK,BLACK,RED,BLUE,          /* Logo: Junior */
	BLACK,BLACK,RED,BLUE,          /* Logo: Junior */
	BLACK,BLACK,RED,BLUE,          /* Logo: Junior */
	BLACK,BLACK,RED,BLUE,          /* Logo: Junior */
	BLACK,BLACK,RED,BLUE,          /* Logo: Junior */
	BLACK,BLACK,RED,BLUE,          /* Logo: Junior */
	BLACK,BLACK,RED,BLUE,          /* Logo: Junior */
	BLACK,BLACK,RED,BLUE,          /* Logo: Junior */
	BLACK,BLACK,RED,BLUE,          /* Logo: Junior */
	BLACK,BLACK,RED,BLUE,          /* Logo: Junior */
	BLACK,BLACK,RED,BLUE,          /* Logo: Junior */
	BLACK,BLACK,RED,BLUE,          /* Logo: Junior */
	BLACK,BLACK,RED,BLUE,          /* Logo: Junior */
	BLACK,BLACK,RED,BLUE,          /* Logo: Junior */
	BLACK,BLACK,RED,BLUE,          /* Logo: Junior */
	BLACK,BLACK,RED,BLUE,          /* Logo: Junior */
	BLACK,BLACK,RED,BLUE,          /* Logo: Junior */
	BLACK,BLACK,RED,BLUE,          /* Logo: Junior */
	BLACK,BLACK,RED,BLUE,          /* Logo: Junior */
	0,0,0,0,
	BLACK,RED,BLACK,BLACK,         /* Logo: (c) 198 */
	BLACK,RED,BLACK,BLACK,         /*       2 Nin   */
	BLACK,RED,BLACK,BLACK,         /*       tendo   */
	BLACK,RED,BLACK,BLACK,         /*       of Am   */
	BLACK,RED,BLACK,BLACK,         /*       erica   */
	BLACK,RED,BLACK,BLACK,         /*       Inc.    */
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,

	/* sprites */
	BLACK,BLUE,LTORANGE,RED,       /* Mario */
	BLACK,BLACK,BLACK,WHITE,       /* Bonus Score */
	BLACK,ORANGE,GREEN,WHITE,      /* Screen2: Moving Platform */
	BLACK,1,2,3,                   /* ?? */
	BLACK,BLUE,YELLOW,WHITE,       /* Cursor */
	BLACK,4,5,6,                   /* ?? */
	BLACK,7,8,9,                   /* ?? */
	BLACK,DKBROWN,WHITE,LTBROWN,   /* Kong Jr */
	BLACK,BLUE,BLUE,LTBLUE,        /* Cage */
	BLACK,DKBROWN,ORANGE,LTBROWN,  /* Kong */
	BLACK,1,7,9,                   /* ?? */
	BLACK,ORANGE,GREEN,YELLOW,     /* Banana & Pear */
	BLACK,RED,GREEN,WHITE,         /* Apple */
	BLACK,BLUE,CYAN,WHITE,         /* Blue Creature */
	BLACK,RED,YELLOW,WHITE,        /* Red Creature */
	BLACK,PURPLE,YELLOW,WHITE      /* Key */
};



static struct MachineDriver dkong_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz (?) */
			0,
			dkong_readmem,dkong_writemem,0,0,
			nmi_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	dkong_gfxdecodeinfo,
	256, 64*4+16*4,
	dkong_vh_convert_color_prom,

	0,
	dkong_vh_start,
	generic_vh_stop,
	dkong_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	dkong_sh_update
};



static struct MachineDriver dkongjr_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz (?) */
			0,
			dkongjr_readmem,dkongjr_writemem,0,0,
			nmi_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	dkongjr_gfxdecodeinfo,
	sizeof(palette)/3,sizeof(dkongjr_colortable),
	0,

	0,
	dkong_vh_start,
	generic_vh_stop,
	dkong_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( dkong_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "dk.5e",  0x0000, 0x1000, 0xa0bfe0f7 )
	ROM_LOAD( "dk.5c",  0x1000, 0x1000, 0x36320606 )
	ROM_LOAD( "dk.5b",  0x2000, 0x1000, 0x57b81038 )
	ROM_LOAD( "dk.5a",  0x3000, 0x1000, 0xe2f03e46 )

	ROM_REGION(0x3000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "dk.3n",  0x0000, 0x0800, 0x5947fc8f )
	ROM_LOAD( "dk.3p",  0x0800, 0x0800, 0xcf971207 )
	ROM_LOAD( "dk.7c",  0x1000, 0x0800, 0xeca7e655 )
	ROM_LOAD( "dk.7d",  0x1800, 0x0800, 0xd8700f2a )
	ROM_LOAD( "dk.7e",  0x2000, 0x0800, 0x3dd5410b )
	ROM_LOAD( "dk.7f",  0x2800, 0x0800, 0xcc1d7c97 )

	ROM_REGION(0x1000)	/* sound */
	ROM_LOAD( "dk.3h",  0x0000, 0x0800, 0x52574a61 )
	ROM_LOAD( "dk.3f",  0x0800, 0x0800, 0x2a6cd3fa )
ROM_END

ROM_START( dkongjp_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "5f.cpu",  0x0000, 0x1000, 0x949b12d3 )
	ROM_LOAD( "5g.cpu",  0x1000, 0x1000, 0xf81386e7 )
	ROM_LOAD( "5h.cpu",  0x2000, 0x1000, 0x45b7e62b )
	ROM_LOAD( "5k.cpu",  0x3000, 0x1000, 0x97dd25c5 )

	ROM_REGION(0x3000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5h.vid",  0x0000, 0x0800, 0x5947fc8f )
	ROM_LOAD( "5k.vid",  0x0800, 0x0800, 0x8b237079 )
	ROM_LOAD( "4m.clk",  0x1000, 0x0800, 0xeca7e655 )
	ROM_LOAD( "4n.clk",  0x1800, 0x0800, 0xd8700f2a )
	ROM_LOAD( "4r.clk",  0x2000, 0x0800, 0x3dd5410b )
	ROM_LOAD( "4s.clk",  0x2800, 0x0800, 0xcc1d7c97 )

	ROM_REGION(0x1000)	/* sound */
	ROM_LOAD( "3i.sou",  0x0000, 0x0800, 0x52574a61 )
	ROM_LOAD( "3j.sou",  0x0800, 0x0800, 0x2a6cd3fa )
ROM_END

ROM_START( dkongjr_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "dkj.5b",  0x0000, 0x1000, 0x831d73dd )
	ROM_CONTINUE(        0x3000, 0x1000 )
	ROM_LOAD( "dkj.5c",  0x2000, 0x0800, 0xe0078007 )
	ROM_CONTINUE(        0x4800, 0x0800 )
	ROM_CONTINUE(        0x1000, 0x0800 )
	ROM_CONTINUE(        0x5800, 0x0800 )
	ROM_LOAD( "dkj.5e",  0x4000, 0x0800, 0xb31be9f1 )
	ROM_CONTINUE(        0x2800, 0x0800 )
	ROM_CONTINUE(        0x5000, 0x0800 )
	ROM_CONTINUE(        0x1800, 0x0800 )

	ROM_REGION(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "dkj.3n",  0x0000, 0x1000, 0x4c96d5b0 )
	ROM_LOAD( "dkj.3p",  0x1000, 0x1000, 0xfbaafe4a )
	ROM_LOAD( "dkj.7c",  0x2000, 0x0800, 0x9ecd901b )
	ROM_LOAD( "dkj.7d",  0x2800, 0x0800, 0xac9378bb )
	ROM_LOAD( "dkj.7e",  0x3000, 0x0800, 0x9785a0b9 )
	ROM_LOAD( "dkj.7f",  0x3800, 0x0800, 0xecc39067 )

	ROM_REGION(0x1000)	/* sound? */
	ROM_LOAD( "dkj.3h",  0x0000, 0x1000, 0x65e71f9f )
ROM_END



static const char *sample_names[] =
{
	"effect00.sam",
	"effect01.sam",
	"effect02.sam",
	"effect03.sam",
	"effect04.sam",
	"effect05.sam",
	"effect06.sam",
	"effect07.sam",
	"tune00.sam",
	"tune01.sam",
	"tune02.sam",
	"tune03.sam",
	"tune04.sam",
	"tune05.sam",
	"tune06.sam",
	"tune07.sam",
	"tune08.sam",
	"tune09.sam",
	"tune0a.sam",
	"tune0b.sam",
	"tune0c.sam",
	"tune0d.sam",
	"tune0e.sam",
	"tune0f.sam",
	"interupt.sam",
    0	/* end of array */
};



static int hiload(const char *name)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x611d],"\x50\x76\x00",3) == 0 &&
			memcmp(&RAM[0x61a5],"\x00\x43\x00",3) == 0 &&
			memcmp(&RAM[0x60b8],"\x50\x76\x00",3) == 0)	/* high score */
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x6100],1,34*5,f);
			RAM[0x60b8] = RAM[0x611d];
			RAM[0x60b9] = RAM[0x611e];
			RAM[0x60ba] = RAM[0x611f];
			/* also copy the high score to the screen, otherwise it won't be */
			/* updated until a new game is started */
			videoram_w(0x0221,RAM[0x6108]);
			videoram_w(0x0201,RAM[0x6109]);
			videoram_w(0x01e1,RAM[0x610a]);
			videoram_w(0x01c1,RAM[0x610b]);
			videoram_w(0x01a1,RAM[0x610c]);
			fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(const char *name)
{
	FILE *f;


	if ((f = fopen(name,"wb")) != 0)
	{
		fwrite(&RAM[0x6100],1,34*5,f);
		fclose(f);
	}
}



struct GameDriver dkong_driver =
{
	"Donkey Kong (US version)",
	"dkong",
	"GARY SHEPHERDSON\nBRAD THOMAS\nEDWARD MASSEY\nNICOLA SALMORIA\nRON FRIES\nPAUL BERBERICH",
	&dkong_machine_driver,

	dkong_rom,
	0, 0,
	sample_names,

	input_ports, trak_ports, dsw, keys,

	dkong_color_prom, 0, 0,
	8*13, 8*16,

	hiload, hisave
};

struct GameDriver dkongjp_driver =
{
	"Donkey Kong (Japanese version)",
	"dkongjp",
	"GARY SHEPHERDSON\nBRAD THOMAS\nEDWARD MASSEY\nNICOLA SALMORIA\nRON FRIES\nPAUL BERBERICH",
	&dkong_machine_driver,

	dkongjp_rom,
	0, 0,
	sample_names,

	input_ports, trak_ports, dsw, keys,

	dkong_color_prom, 0, 0,
	8*13, 8*16,

	hiload, hisave
};

struct GameDriver dkongjr_driver =
{
	"Donkey Kong Jr.",
	"dkongjr",
	"GARY SHEPHERDSON\nBRAD THOMAS\nNICOLA SALMORIA\nPAUL BERBERICH",
	&dkongjr_machine_driver,

	dkongjr_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	0, palette, dkongjr_colortable,
	8*13, 8*16,

	hiload, hisave
};
