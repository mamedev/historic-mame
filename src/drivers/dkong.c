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
7c00      ?
7c80      gfx bank select (Donkey Kong Jr. only)
7d00-7d07 sound related? (digital sound trigger?)
7d80      ?
7d82      ?
7d83      ?
7d84      interrupt enable
7d85      ?
7d86      ?
7d87      ?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern void dkongjr_gfxbank_w(int offset,int data);
extern int dkong_vh_start(void);
extern void dkong_vh_screenrefresh(struct osd_bitmap *bitmap);



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
	{ 0x6900, 0x6a7f, MWA_RAM, &spriteram },
	{ 0x7d84, 0x7d84, interrupt_enable_w },
	{ 0x7400, 0x77ff, videoram_w, &videoram },
	{ 0x7c80, 0x7c80, dkongjr_gfxbank_w },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x7800, 0x7803, MWA_RAM },	/* ???? */
	{ 0x7808, 0x7808, MWA_RAM },	/* ???? */
	{ 0x7c00, 0x7c00, MWA_RAM },	/* ???? */
	{ 0x7d00, 0x7d07, MWA_RAM },	/* ???? */
	{ 0x7d80, 0x7d83, MWA_RAM },	/* ???? */
	{ 0x7d85, 0x7d87, MWA_RAM },	/* ???? */
	{ -1 }	/* end of table */
};
static struct MemoryWriteAddress dkongjr_writemem[] =
{
	{ 0x6000, 0x68ff, MWA_RAM },
	{ 0x6a80, 0x6fff, MWA_RAM },
	{ 0x6900, 0x6a7f, MWA_RAM, &spriteram },
	{ 0x7d84, 0x7d84, interrupt_enable_w },
	{ 0x7400, 0x77ff, videoram_w, &videoram },
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
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
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
	{ 0, 128*16*16 },	/* the two bitplanes are separated */
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



static unsigned char palette[] =
{
	0x00,0x00,0x00, /* BLACK */
	0xff,0xff,0xff, /* WHITE */
	0xff,0x00,0x00, /* RED */
	0xff,0x00,0xff, /* PURPLE */
	0x00,0xff,0xff, /* CYAN */
	0xff,0xff,0x80, /* LTORANGE */
	0xdb,0x00,0x00, /* DKRED */
	0x00,0x00,0xff, /* BLUE */
	0xff,0xff,0x00, /* YELLOW */
	239,3,239,      /* PINK */
	3,180,239,      /* LTBLUE */
	255,131,3,      /* ORANGE */
	0x00,0xff,0x00, /* GREEN */
	167,3,3,        /* DKBROWN */
	255,183,115,    /* LTBROWN */
	0x00,0x46,0x00, /* DKGREEN */
};

enum { BLACK,WHITE,RED,PURPLE,CYAN,LTORANGE,DKRED,BLUE,YELLOW,PINK,
		LTBLUE,ORANGE,GREEN,DKBROWN,LTBROWN,DKGREEN };

static unsigned char dkong_colortable[] =
{
	/* chars */
	BLACK,0,0,WHITE,        /* 0-9 */
	BLACK,0,0,WHITE,        /* 0-9 */
	BLACK,0,0,WHITE,        /* 0-9 */
	BLACK,0,0,WHITE,        /* 0-9 */
	BLACK,0,0,RED,  /* A-Z */
	BLACK,0,0,RED,  /* A-Z */
	BLACK,0,0,RED,  /* A-Z */
	BLACK,0,0,RED,  /* A-Z */
	BLACK,0,0,RED,  /* A-Z */
	BLACK,0,0,RED,  /* A-Z */
	BLACK,0,0,RED,  /* A-Z */
	BLACK,0,0,RED,  /* A-Z */
	BLACK,0,0,RED,  /* A-Z */
	BLACK,0,0,RED,  /* A-Z */
	BLACK,0,0,RED,  /* A-Z */
	BLACK,0,0,RED,  /* A-Z */
	0,0,0,RED,      /* RUB END */
	0,0,0,RED,      /* RUB END */
	0,5,6,RED,      /* (C), ITC */
	0,5,6,RED,      /* (C), ITC */
	0,LTORANGE,DKRED,WHITE, /* Kong in intermission */
	0,LTORANGE,DKRED,WHITE, /* Kong in intermission */
	0,LTORANGE,DKRED,WHITE, /* Kong in intermission */
	0,LTORANGE,DKRED,WHITE, /* Kong in intermission */
	0,LTORANGE,DKRED,WHITE, /* Kong in intermission */
	0,LTORANGE,DKRED,WHITE, /* Kong in intermission */
	BLACK,RED,PURPLE,CYAN,  /* BONUS */
	BLACK,RED,PURPLE,CYAN,  /* BONUS */
	BLACK,RED,PURPLE,CYAN,  /* BONUS */
	BLACK,RED,PURPLE,CYAN,  /* BONUS */
	BLACK,RED,PURPLE,CYAN,  /* BONUS */
	BLACK,RED,PURPLE,CYAN,  /* BONUS */
	BLACK,0,0,LTORANGE,     /* 0-9 in intermission */
	BLACK,0,0,LTORANGE,     /* 0-9 in intermission */
	BLACK,0,0,LTORANGE,     /* 0-9 in intermission */
	BLACK,RED,PURPLE,CYAN,  /* 0-9 in intermission, BONUS */
	0,0,0,0,        /* unused */
	0,0,0,0,        /* unused */
	0,0,0,0,        /* unused */
	0,0,0,RED,      /* (TM) */
	0,0,0,0,        /* unused */
	0,0,0,0,        /* unused */
	0,0,0,0,        /* unused */
	0,0,0,0,        /* unused */
	0,CYAN,BLUE,YELLOW,     /* ziqqurat level */
	0,CYAN,BLUE,YELLOW,     /* ziqqurat level */
	0,CYAN,0,0,     /* _ ziqqurat level */
	0,0,0,0,        /* unused */
	BLACK,RED,PURPLE,CYAN,  /* barrel level */
	BLACK,RED,PURPLE,CYAN,  /* barrel level */
	0,0,0,0,        /* unused */
	0,0,0,0,        /* unused */
	BLACK,RED,PURPLE,CYAN,  /* barrel level */
	BLACK,RED,PURPLE,CYAN,  /* barrel level */
	0,0,0,0,        /* unused */
	0,0,0,CYAN,     /* HELP! */
	BLACK,RED,PURPLE,CYAN,  /* barrel level */
	BLACK,RED,PURPLE,CYAN,  /* barrel level */
	0,0,0,0,        /* unused */
	0,0,0,CYAN,     /* HELP! */
	BLACK,RED,PURPLE,CYAN,  /* barrel level */
	BLACK,RED,PURPLE,CYAN,  /* barrel level */
	0,0,0,RED,      /* ? */
	0,RED,LTORANGE,BLUE,    /* lives */

	/* sprites */
	BLACK,LTBLUE,LTBROWN,RED,       /* Fireball (When Mario has hammer) */
							/* Rotating ends on conveyors */
							/* Springy things (lift screen) */
	BLACK,RED,YELLOW,WHITE, /* Fireball (normal) */
							/* Flames (on top of oil tank) */
	BLACK,RED,LTORANGE,BLUE,        /* Mario */
	BLACK,1,2,3,                    /* -Moving Ladder (conveyor screen) */
							/* Moving Lift */
	BLACK,4,5,6,
	BLACK,7,8,9,
	BLACK,10,11,12,
	BLACK,LTBROWN,DKBROWN,WHITE,    /* Kong (Head), Hammer, Scores (100,200,500,800 etc) */
	BLACK,LTBROWN,DKBROWN,ORANGE,   /* Kong (body) */
	BLACK,ORANGE,WHITE,PINK,        /* girl (Head), Heart (when screen completed) */
	BLACK,WHITE,BLUE,PINK,  /* Girl (lower half), Umbrella, Purse, hat */
	BLACK,ORANGE,BLUE,YELLOW,       /* Rolling Barrel (type 1), Standing Barrel (near Kong) */
	BLACK,WHITE,LTBLUE,BLUE,        /* Oil tank, Rolling Barrel (type 2), Explosion (barrel hit withhammer) */
	BLACK,3,4,5,
	BLACK,GREEN,1,2,        /* -Pies (Conveyor screen) */
	BLACK,YELLOW,RED,BLACK  /* -Thing at top/bottom of lifts, Clipping sprite (all black) */
};

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
	BLACK,BLACK,BLUE,BLACK,        /* Screen4: Rope Cage-Lock */
	BLACK,BLACK,BLUE,BLACK,        /* Screen4: Rope Cage-Lock */
	BLACK,GREEN,RED,LTBROWN,       /* Screen1: Vines */
	BLACK,GREEN,RED,RED,           /* Rope & Chains */
	BLACK,GREEN,GREEN,WHITE,       /* Screen4: Wall */
	0,0,0,0,
	0,0,0,0,
	0,0,0,0,
	0,1,2,3,                       /* ?? */
	0,4,5,6,                       /* ?? */
	BLACK,GREEN,DKGREEN,WHITE,     /* Screen1: Islands Top */
	BLACK,GREEN,DKGREEN,WHITE,     /* Screen1&2: Top Floor Bottom Part */
	0,7,8,9,                       /* ?? */
	0,10,11,12,                    /* ?? */
	BLACK,WHITE,ORANGE,YELLOW,     /* Screen1&2: Platform */
	0,13,14,15,                    /* ?? */
	0,1,1,1,                       /* ?? */
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
	BLACK,LTORANGE,RED,BLACK,      /* Logo: Donkey Kong */
	BLACK,LTORANGE,RED,BLACK,      /* Logo: Donkey Kong */
	BLACK,LTORANGE,RED,BLACK,      /* Logo: Donkey Kong */
	BLACK,LTORANGE,RED,BLACK,      /* Logo: Donkey Kong */
	BLACK,LTORANGE,RED,BLACK,      /* Logo: Donkey Kong */
	BLACK,LTORANGE,RED,BLACK,      /* Logo: Donkey Kong */
	BLACK,LTORANGE,RED,BLACK,      /* Logo: Donkey Kong */
	BLACK,LTORANGE,RED,BLACK,      /* Logo: Donkey Kong */
	BLACK,LTORANGE,RED,BLACK,      /* Logo: Donkey Kong */
	BLACK,LTORANGE,RED,BLACK,      /* Logo: Donkey Kong */
	BLACK,LTORANGE,RED,BLACK,      /* Logo: Donkey Kong */
	BLACK,LTORANGE,RED,BLACK,      /* Logo: Donkey Kong */
	BLACK,LTORANGE,RED,BLACK,      /* Logo: Donkey Kong */
	BLACK,LTORANGE,RED,BLUE,       /* Logo: Junior */
	BLACK,LTORANGE,RED,BLUE,       /* Logo: Junior */
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
	BLACK,GREEN,BLACK,BLACK,       /* Logo: (c) 198 */
	BLACK,GREEN,BLACK,BLACK,       /*       2 Nin   */
	BLACK,GREEN,BLACK,BLACK,       /*       tendo   */
	BLACK,GREEN,BLACK,BLACK,       /*       of Am   */
	BLACK,GREEN,BLACK,BLACK,       /*       erica   */
	BLACK,GREEN,BLACK,BLACK,       /*       Inc.    */
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
	BLACK,LTBROWN,BLUE,RED,        /* Mario */
	BLACK,BLACK,BLACK,WHITE,       /* Bonus Score */
	BLACK,GREEN,ORANGE,WHITE,      /* Screen2: Moving Platform */
	BLACK,1,2,3,                   /* ?? */
	BLACK,YELLOW,BLUE,WHITE,       /* Cursor */
	BLACK,4,5,6,                   /* ?? */
	BLACK,7,8,9,                   /* ?? */
	BLACK,WHITE,DKBROWN,LTBROWN,   /* Kong Jr */
	BLACK,BLUE,BLUE,LTBLUE,        /* Cage */
	BLACK,ORANGE,DKBROWN,LTBROWN,  /* Kong */
	BLACK,1,7,9,                   /* ?? */
	BLACK,GREEN,ORANGE,YELLOW,     /* Banana & Pear */
	BLACK,GREEN,RED,WHITE,         /* Apple */
	BLACK,CYAN,BLUE,WHITE,         /* Blue Creature */
	BLACK,YELLOW,RED,WHITE,        /* Red Creature */
	BLACK,YELLOW,PURPLE,WHITE      /* Key */
};



const struct MachineDriver dkong_driver =
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
	input_ports,dsw,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	dkong_gfxdecodeinfo,
	sizeof(palette)/3,sizeof(dkong_colortable),
	0,0,palette,dkong_colortable,
	0,17,
	0,45,
	8*13,8*16,4,
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



const struct MachineDriver dkongjr_driver =
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
	input_ports,dsw,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	dkongjr_gfxdecodeinfo,
	sizeof(palette)/3,sizeof(dkongjr_colortable),
	0,0,palette,dkongjr_colortable,
	0,17,
	0,60,
	8*13,8*16,4,
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
