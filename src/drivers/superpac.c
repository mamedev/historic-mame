/***************************************************************************

Super Pacman (preliminary)
(from Kevin Brisley's superpac.keg)

Game CPU

0000-03ff Video RAM
0400-07ff Color RAM
c000-dfff ROM SPC-2.1C
e000-ffff ROM SPC-1.1B


memory mapped ports:

read:
10c6      DSW-2         BITS 0-3: Difficulty  BIT 6: Sound  BIT 7: Screen
10c5      DSW-3			BITS 0-2: Coins/Credit  BITS 3-5: Bonus Pacman  BITS 6-7: Starting Pacmen
10c0	  Insert Coin   BIT 0: Insert coin
10c1      Start Game    BIT 0: 1 Player Start  BIT 1: 2 Player Start
10c2	  Controls		BIT 0: UP  BIT 1: RIGHT  BIT 2: DOWN  BIT 3: LEFT  BIT 5: Speedup


write:
0819-0f19 56 sets of 32 bytes
		  the first byte contains size and flip information: x flip (bit 1), y flip (bit 0)
		  	 big sprite (bits 2-3?)
		  the second byte contains the sprite image number
		  the third byte contains the color info
		  the fourth byte contains the X position
		  the fifth byte contains the Y position
1619-16?? at least 2 sets of 32 bytes
		  the first byte contains size and flip: x flip: (bit 1), y flip (bit 2)
		     big sprite (bits 2-3?)
		  the second byte contains the sprite image number
		  the third byte contains the color info
		  the fourth byte contains the X position
		  the fifth byte contains the Y position
8000	  Watchdog reset

I/O ports:


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern int superpac_vh_start(void);
extern void superpac_vh_screenrefresh(struct osd_bitmap *bitmap);
extern int superpac_init_machine(const char *gamename);

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x10bf, MRA_RAM },
	{ 0x10c7, 0xffff, MRA_RAM },
	{ 0x10c6, 0x10c6, input_port_0_r }, /* DSW-2 */
	{ 0x10c5, 0x10c5, input_port_1_r }, /* DSW-3 */
	{ 0x10c0, 0x10c0, input_port_2_r }, /* Insert Coin */
	{ 0x10c1, 0x10c1, input_port_3_r }, /* Start Game */
	{ 0x10c2, 0x10c2, input_port_4_r }, /* Controls */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x03ff, videoram_w, &videoram },
	{ 0x0400, 0x07ff, colorram_w, &colorram },	/* video and color RAM */
	{ 0x0819, 0x1019, MWA_RAM, &spriteram },	/* main sprites */
	{ 0x1619, 0x16ff, MWA_RAM, &spriteram_2 },	/* super pacman sprite(s) */
	{ 0xc000, 0xffff, MWA_ROM },				/* SPC-2.1C at 0xc000, SPC-1.1B at 0xe000 */
	{ -1 }	/* end of table */
};

static struct InputPort superpac_input_ports[] =
{
	{	/* DSW-2 */
		0x48,	/* default_value */
		{ 0, 0, 0, 0, 0, 0, 0, 0 }, /* not affected by keyboard */
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* DSW-3 */
		0x00,	/* default_value */
		{ 0, 0, 0, 0, 0, 0, 0, 0 }, /* not affected by keyboard */
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* Insert Coin */
		0x00,	/* default_value */
		{ OSD_KEY_3, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* Start Game */
		0x00,	/* default_value */
		{ OSD_KEY_1, OSD_KEY_2, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }	/* not affected by joystick */
	},
	{	/* Controls */
		0x00,	/* default_value */
		{ OSD_KEY_UP, OSD_KEY_RIGHT, OSD_KEY_DOWN, OSD_KEY_LEFT, 0, OSD_KEY_CONTROL, 0, 0 },
		{ OSD_JOY_UP, OSD_JOY_RIGHT, OSD_JOY_DOWN, OSD_JOY_LEFT, 0, OSD_JOY_FIRE, 0, 0 }
	},
	{ -1 }	/* end of table */
};


static struct KEYSet keys[] =
{
        { 4, 0, "MOVE UP" },
        { 4, 3, "MOVE LEFT"  },
        { 4, 1, "MOVE RIGHT" },
        { 4, 2, "MOVE DOWN" },
        { 4, 5, "FIRE" },
        { -1 }
};


/* This needs a lot of work... */
static struct DSW superpac_dsw[] =
{
	{ 0, 0x0F, "DIFFICULTY", { "R0 NORMAL","R1 EASIEST","R2","R3","R4","R5","R6","R7","R8 DEFAULT",\
								"R9","RA","RB HARDEST","RC EASIEST AUTO","RD","RE","RF HARDEST AUTO" } },
	{ 0, 0x40, "SOUND", { "ON", "OFF" } },
	{ 0, 0x80, "SCREEN", { "NORMAL", "FREEZE VIDEO" } },
	{ 1, 0x07, "COINS", { "1 COIN 1 CREDIT", "1 COIN 2 CREDIT", "1 COIN 3 CREDIT",\
						 "1 COIN 6 CREDIT", "1 COIN 7 CREDIT", "2 COIN 1 CREDIT", \
						 "2 COIN 3 CREDIT", "3 COIN 1 CREDIT" } },
	{ 1, 0x38, "BONUS AT", { "30000 100000", "30000 80000", "30000 120000", "30000 80000@@@",\
							 "30000 100000@@@", "30000 120000@@@", "80000", "NONE" } },
	{ 1, 0xc0, "LIVES", { "3", "1", "2", "5" } },
	{ -1 }
};

/* SUPERPAC -- ROM SPV-1.3C (4K) */
static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 4},	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 }, /* characters are rotated 90 degrees */
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },	/* bits are packed in groups of four */
	16*8	/* every char takes 16 bytes */
};

/* SUPERPAC -- ROM SPV-2.3F (8K) */
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 39 * 8, 38 * 8, 37 * 8, 36 * 8, 35 * 8, 34 * 8, 33 * 8, 32 * 8,
			7 * 8, 6 * 8, 5 * 8, 4 * 8, 3 * 8, 2 * 8, 1 * 8, 0 * 8 },
	{ 0, 1, 2, 3, 8*8, 8*8+1, 8*8+2, 8*8+3,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 24*8+0, 24*8+1, 24*8+2, 24*8+3 },
	64*8	/* every sprite takes 64 bytes */
};


/* bumped color table to 69 entries -- see color table for details */
static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 69 /*32*/ },
	{ 1, 0x1000, &spritelayout, 0, 69 /*32*/ },
	{ -1 } /* end of array */
};

static unsigned char superpac_palette[] =
{
	0x00, 0x00, 0x00,	/* 00 black */
	0xff, 0x00, 0x00,	/* 01 red */
	0xde, 0x97, 0x47,	/* 02 tan */
	0xff, 0xb8, 0xde,	/* 03 pink */
	0x00, 0x00, 0x00,	/* 04 black */
	0x00, 0xff, 0xde,	/* 05 lt blue*/
	0x47, 0xb8, 0xde,	/* 06 sky blue?*/
	0xff, 0xb8, 0x47,	/* 07 orange */
	0x00, 0x00, 0x00,	/* 08 black */
	0xff, 0xff, 0x00,	/* 09 yellow */
	0x00, 0x00, 0x00,	/* 0A black */
	0x21, 0x21, 0xde,	/* 0B blue */
	0x00, 0xff, 0x00,	/* 0C green */
	0x47, 0xb8, 0x97,	/* 0D bluish */
	0xff, 0xb8, 0x97,	/* 0E peach */
	0xde, 0xde, 0xde	/* 0F white */
};


/* CHARACTERS											SPRITES
#00	??													power pill blink off
#01 bottom 2 lines of screen							pacman
#02 CREDIT 0, ROM test, score digits					blinking pacman, fruit pts,
#03 1UP,HIGH SCORE,GAME OVER,attract maze&doors			blinking ghosts
#04 POWER,SUPER,(C)notice,doors,maze,READY!				keys, red ghost
#05 ??													pink ghost
#06	??													lt blue ghost
#07	??													lt orange ghost
#08 ??													blue ghosts
#09 top of logo (containing super)						??
#0a bottom of logo (containing PAC-MAN)					dead ghosts (eyes)
#0b ??													power pill blink on
#0c level indicator apple								super pill (big)
#0d level indicator bananas								super pill (medium)
#0e level indicator donut								super pill (small)
#0f level indicator burger								super pill blink off
#10 ??													apple
#11 ??													bananas
#12 ??													donut
#13 ??													hamburger
#14 ??													fried egg
#15 ??													ear of corn
#16 ??													sneaker
#17 ??													cake
#18 ??													peach
#19 ??													melon
#1a ??													coffee cup
#1b ??													mushroom
#1c ??													ghost points, bell
#1d ??													clover
#1e ??													galaxian
#1f ??													gift
*/

/* Color table is a little confused because SP has several large values it uses in color RAM.
   These values were being reduced with a modulus operator, which caused them to try to use the
   same color entry as another element. Also, I could be mistaken but it looks like characters
   and sprites do not share the same color table. So I put the sprite color table at offset $20
   and expanded it to 37 entries. $20 is added to all sprite entries unless they are >$20.
   Large values (>$20) are:	SPRITES					USES ENTRY		SHARES ENTRY WITH
	   						Bonus Star:		$28 	sprite+$08			blue ghosts
	   						Key:			$44 	sprite+$24			n/a
	   						Ghost Pts:		$3C		sprite+$1C			bell

	   						CHARACTERS				USES ENTRY		SHARES ENTRY WITH
	   						Pacmen Remain. 	$41		sprite+$21			n/a
	   						Score Digits	$42		sprite+$22			n/a
	   						1UP,HIGH SCORE	$43		sprite+$23			n/a

*/

static unsigned char superpac_color_table[] =
{
	0x00,0x00/*off*/,0x0B,0x0c, /* power pill blink OFF */
	0x00,0x09,0x09,0x00, /* pacman, [PAUSED] */
	0x00,0x0f,0x0f,0x0f, /* score digits,CREDIT 0, ROM test */
	0x00,0x03/*doors*/,0x0f/*text*/,0x0b/*maze*/, /* attract screen maze & doors */
	0x00,0x03/*doors*/,0x0f/*text*/,0x0b/*maze*/, /* POWER,SUPER,(C)notice,maze&doors,PUSH START,STAGE 1,READY! */
	0x00,0x03/*cloak*/,0x0F/*eyes*/,0x0B/*pupils*/, /* was ghost 1 */
	0x00,0x05/*cloak*/,0x0F/*eyes*/,0x0B/*pupils*/, /* was ghost 2 */
	0x00,0x07/*cloak*/,0x0F/*eyes*/,0x0B/*pupils*/, /* was ghost 3 */
	0x00,0x0b/*cloak*/,0x03/*mouth*/,0x0f, /* was blue ghosts */
	0x00,0x09,0x07,0x03, /* top of logo (containing SUPER) */
	0x00,0x09,0x07,0x01, /* bottom of logo (containing PAC-MAN)*/
	0x00,0x0f,0x0f,0x0f, /* maze/doors blink */
	0x00,0x01,0x02,0x0f, /* level indicator apple */
	0x00,0x09,0x0f,0x02, /* level indicator banana */
	0x00,0x02,0x07,0x0f, /* level indicator donut */
	0x00,0x02,0x0f,0x0c, /* level indicator burger*/
	0x00,0x0f,0x09,0x02, /* level indicator egg */
	0x00,0x02,0x09,0x0c, /* level indicator corn */
	0x00,0x01,0x03,0x0c, /* level indicator sneaker */
	0x00,0x0f,0x0e,0x01, /* level indicator cake */
	0x00,0x0e,0x03,0x0c, /* level indicator peach */
	0x00,0x0c,0x05,0x0b, /* level indicator melon */
	0x00,0x0e,0x0f,0x0b, /* level indicator coffee cup */
	0x00,0x07,0x02,0x0f, /* level indicator mushroom */
	0x00,0x09,0x0f,0x02, /* level indicator bell */
	0x00,0x0c,0x0f,0x0b, /* level indicator clover */
	0x00,0x01,0x09,0x0b, /* level indicator galaxian */
	0x00,0x0f,0x03,0x01, /* level indicator gift */
	0x00,0x0c,0x0c,0x0c, /* level indicator bosconian? */
	0x00,0x0c,0x0c,0x0c, /* ??? */
	0x00,0x0c,0x0c,0x0c, /* ??? */
	0x00,0x0c,0x0c,0x0c, /* ??? */
	/* ------begin sprites------ */
	0x00,0x00/*off*/,0x00/*eaten.ghst*/,0x0c, /* power pill blink OFF, ghost as eaten (invis.) */
	0x00,0x09,0x09,0x00, /* pacman */
	0x00,0x0f,0x0f,0x0f, /* fruit pts, blinking pacman */
	0x00,0x0f/*doors/bl.ghst*/,0x01/*text*/,0x0b/*maze*/, /* blinking ghost,1UP,HIGH SCORE,GAME OVER,attract maze,doors */
	0x00,0x01/*cloak*/,0x0F/*eyes*/,0x0B/*pupils*/, /* ghost0 */
	0x00,0x03/*cloak*/,0x0F/*eyes*/,0x0B/*pupils*/, /* ghost 1 */
	0x00,0x05/*cloak*/,0x0F/*eyes*/,0x0B/*pupils*/, /* ghost 2 */
	0x00,0x07/*cloak*/,0x0F/*eyes*/,0x0B/*pupils*/, /* ghost 3 */
	0x00,0x0b/*cloak*/,0x03/*mouth*/,0x0f, /* blue ghosts, bonus star */
	0x00,0x09,0x07,0x03, /* top of logo (containing SUPER) */
	0x00,0x00,0x0f,0x0b, /* dead ghosts (eyes) */
	0x00,0x0e,0x00,0x0b, /* power pill blink ON */
	0x00,0x0c,0x0c,0x0c, /* super pill blink (big) */
	0x00,0x0c,0x0c,0x00, /* super pill blink (intermediate) */
	0x00,0x0c,0x00,0x00, /* super pill blink (small) */
	0x00,0x00,0x00,0x00, /* super pill blink OFF */
	0x00,0x01/*red*/,0x02/*tan*/,0x0F/*white*/,	/* apple */
	0x00,0x09/*yellow*/,0x0F/*white*/,0x02/*tan*/, /* bananas */
	0x00,0x02/*tan*/,0x07/*orange*/,0x0f/*white*/, /* donut */
	0x00,0x02/*topbun*/,0x0f/*bt.bun*/,0x0c/*lettuce*/, /* hamburger */
	0x00,0x0f/*white*/,0x09/*yolk*/,0x02/*hilite*/, /* fried egg */
	0x00,0x02/*tan*/,0x09/*yellow*/,0x0c/*green*/, /* ear of corn */
	0x00,0x01/*red*/,0x03/*pink*/,0x0c/*green*/, /* sneaker */
	0x00,0x0f/*white*/,0x0e/*peach*/,0x01/*red*/, /* cake */
	0x00,0x0e/*peach*/,0x03/*pink*/,0x0c/*green*/, /* peach */
	0x00,0x0c/*green*/,0x05/*lt.blue*/,0x0b/*blue*/, /* melon */
	0x00,0x0e/*peach*/,0x0f/*white*/,0x0b/*blue*/, /* coffee cup */
	0x00,0x07/*orange*/,0x02/*tan*/,0x0f/*white*/, /* mushroom */
	0x00,0x09/*yellow*/,0x0f/*pts/white*/,0x02/*tan*/, /* ghost pts, bell */
	0x00,0x0c/*green*/,0x0f/*white*/,0x0b/*blue*/, /* clover */
	0x00,0x01/*red*/,0x09/*yellow*/,0x0b/*blue*/, /* galaxian */
	0x00,0x0f/*white*/,0x03/*pink*/,0x01/*red*/, /* gift package */
	/* -------miscellaneous------- */
	0x00,0x0c,0x0c,0x0c, /* Bosconian? */
	0x00,0x09,0x0c,0x0c, /* bottom 2 rows (pacmen remaining) */
	0x00,0x0f,0x0f,0x0f, /* 2nd row text -- score digits */
	0x00,0x01,0x01,0x01, /* top row text -- 1 UP, HIGH SCORE */
	0x00,0x05,0x0f,0x0c  /* keys */
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware  */
	{
		{
			CPU_M6809,
			1000000,			/* 1 Mhz */
			0,					/* memory region */
			readmem,			/* MemoryReadAddress */
			writemem,			/* MemoryWriteAddress */
			0,					/* IOReadPort */
			0,					/* IOWritePort */
			interrupt,			/* interrupt routine */
			1					/* interrupts per frame */
		}
	},
	60,							/* frames per second */
	superpac_init_machine,		/* init machine routine */

	/* video hardware */
	28*8, 36*8,					/* screen_width, screen_height */
	 { 0*8, 28*8-1, 0*8, 36*8-1 },	/* struct rectangle visible_area */
	gfxdecodeinfo,				/* GfxDecodeInfo * */
	16,							/* total colors */
	4*(32+37), /*32,*/						/* color table length */
	0,							/* convert color prom routine */

	0,							/* vh_init routine */
	superpac_vh_start,			/* vh_start routine */
	generic_vh_stop,			/* vh_stop routine */
	superpac_vh_screenrefresh,	/* vh_update routine */

	/* sound hardware */
	0,							/* pointer to samples */
	0,							/* sh_init routine */
	0,							/* sh_start routine */
	0,							/* sh_stop routine */
	0							/* sh_update routine */
};

ROM_START( superpac_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "SPC-2.1C", 0xc000, 0x2000 )
	ROM_LOAD( "SPC-1.1B", 0xe000, 0x2000 )

	ROM_REGION(0x3000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "SPV-1.3C", 0x0000, 0x1000 )
	ROM_LOAD( "SPV-2.3F", 0x1000, 0x2000 )
ROM_END

struct GameDriver superpac_driver =
{
	"Super Pac Man",
	"superpac",
	"KEVIN BRISLEY",
	&machine_driver,		/* MachineDriver * */

	superpac_rom,			/* RomModule * */
	0, 0,					/* ROM decrypt routines */
	0,						/* samplenames */

	superpac_input_ports,	/* InputPort  */
	superpac_dsw,		/* DSW        */
        keys,                   /* KEY def    */

	0,						/* color prom */
	superpac_palette,		/* palette */
	superpac_color_table,	/* color table */
	{ 0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,	/* numbers */
		0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,	/* letters */
		0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a },
	0x02, 0x01,				/* white_text, yellow_text for DIP switch menu */
	8*11, 8*20, 0x01,		/* paused_x, paused_y, paused_color for PAUSED */

	0,						/* highscore load routine */
	0						/* highscore save routine */
};

