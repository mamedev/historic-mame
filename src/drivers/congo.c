/***************************************************************************

Congo Bongo memory map (preliminary)

0000-1fff ROM 1
2000-3fff ROM 2
4000-5fff ROM 3
6000-7fff ROM 4

8000-87ff RAM 1
8800-8fff RAM 2
a000-a3ff Video RAM
a400-a7ff Color RAM

8400-8fff sprites

read:
c000      IN0
c001      IN1
c002      DSW1
c003      DSW2
c008      IN2

*
 * IN0 (bits NOT inverted)
 * bit 7 : ?
 * bit 6 : ?
 * bit 5 : ?
 * bit 4 : FIRE player 1
 * bit 3 : DOWN player 1
 * bit 2 : UP player 1
 * bit 1 : LEFT player 1
 * bit 0 : RIGHT player 1
 *
*
 * IN1 (bits NOT inverted)
 * bit 7 : ?
 * bit 6 : ?
 * bit 5 : ?
 * bit 4 : FIRE player 2 (TABLE only)
 * bit 3 : DOWN player 2 (TABLE only)
 * bit 2 : UP player 2 (TABLE only)
 * bit 1 : LEFT player 2 (TABLE only)
 * bit 0 : RIGHT player 2 (TABLE ony)
 *
*
 * IN2 (bits NOT inverted)
 * bit 7 : CREDIT
 * bit 6 : COIN 2
 * bit 5 : COIN 1
 * bit 4 : ?
 * bit 3 : START 2
 * bit 2 : START 1
 * bit 1 : ?
 * bit 0 : ?
 *
*
 * DSW1 (bits NOT inverted)
 * bit 7 :  COCKTAIL or UPRIGHT cabinet (0 = UPRIGHT)
 * bit 6 :  sound  0 = off  1 = on
 * bit 5 :\ lives
 * bit 4 :/ 00 = infinite  01 = 4  10 = 5  11 = 3
 * bit 3 :\  Difficulty
 * bit 2 :/ 00 = hardest   10 = hard 11 = medium 01 = easy
 * bit 1 :\ bonus
 * bit 0 :/ 00 = 40000  01 = 20000  10 = 30000  11 = 10000
 *
 * DSW2 (bits NOT inverted)
 * bit 7 :\
 * bit 6 :|  right coin slot
 * bit 5 :|
 * bit 4 :/
 * bit 3 :\
 * bit 2 :|  left coin slot
 * bit 1 :|
 * bit 0 :/
 *

write:
c000-c002 ?
c006      ?
c000      interrupt enable
c028-c029 background playfield position
c031      sprite enable ?
c032      sprite start
c033      sprite count

interrupts:
VBlank triggers IRQ, handled with interrupt mode 1
NMI causes a ROM/RAM test.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern int zaxxon_IN2_r(int offset);

extern unsigned char *congo_background_position;
extern unsigned char *congo_background_enable;
extern void congo_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern int  congo_vh_start(void);
extern void congo_vh_stop(void);
extern void congo_vh_screenrefresh(struct osd_bitmap *bitmap);



static struct MemoryReadAddress readmem[] =
{
        { 0x8000, 0x8fff, MRA_RAM },
        { 0xa000, 0x83ff, MRA_RAM },
        { 0xa000, 0xa7ff, MRA_RAM },
	{ 0xc000, 0xc000, input_port_0_r },
	{ 0xc001, 0xc001, input_port_1_r },
        { 0xc008, 0xc008, zaxxon_IN2_r },
	{ 0xc002, 0xc002, input_port_3_r },
	{ 0xc003, 0xc003, input_port_4_r },
        { 0x0000, 0x7fff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
        { 0x8000, 0x83ff, MWA_RAM },
        { 0xa000, 0xa3ff, videoram_w, &videoram },
        { 0xa400, 0xa7ff, colorram_w, &colorram },
        { 0x8400, 0x8fff, MWA_RAM, &spriteram },
        { 0xc01f, 0xc01f, interrupt_enable_w },
        { 0xc028, 0xc029, MWA_RAM, &congo_background_position },
        { 0xc01d, 0xc01d, MWA_RAM, &congo_background_enable },
        { 0x0000, 0x7fff, MWA_ROM },
	{ -1 }  /* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0x00,
                { OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_UP, OSD_KEY_DOWN,
				OSD_KEY_CONTROL, 0, 0, 0 },
                { OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_UP, OSD_JOY_DOWN,
				OSD_JOY_FIRE, 0, 0, 0 }
	},
	{	/* IN1 */
		0x00,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0x00,
		{ 0, 0, OSD_KEY_1, OSD_KEY_2,
				0, 0, 0, OSD_KEY_3 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0x7f,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0x3f,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};


static struct KEYSet keys[] =
{
        { 0, 1, "MOVE LEFT UP" },
        { 0, 2, "MOVE RIGHT UP"  },
        { 0, 0, "MOVE RIGHT DOWN" },
        { 0, 3, "MOVE LEFT DOWN" },
        { 0, 4, "JUMP" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 3, 0x30, "LIVES", { "INFINITE", "4", "5", "3" }, 1 },
	{ 3, 0x03, "BONUS", { "40000", "20000", "30000", "10000" }, 1 },
	{ 3, 0x0c, "DIFFICULTY", { "HARDEST", "MEDIUM", "HARD", "EASY" }, 1 },
	{ 3, 0x40, "SOUND", { "OFF", "ON" } },
	{ -1 }
};



static struct GfxLayout charlayout1 =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	32,32,	/* 32*32 sprites */
        128,     /* 64 sprites */
	3,	/* 3 bits per pixel */
        { 0, 2*1024*8*8, 2*2*1024*8*8 },    /* the bitplanes are separated */
	{ 103*8, 102*8, 101*8, 100*8, 99*8, 98*8, 97*8, 96*8,
			71*8, 70*8, 69*8, 68*8, 67*8, 66*8, 65*8, 64*8,
			39*8, 38*8, 37*8, 36*8, 35*8, 34*8, 33*8, 32*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 24*8+4, 24*8+5, 24*8+6, 24*8+7 },
	128*8	/* every sprite takes 128 consecutive bytes */
};

static struct GfxLayout charlayout2 =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	3,	/* 3 bits per pixel */
	{ 0, 1024*8*8, 2*1024*8*8 },	/* the bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout1,          0, 16 },	/* characters */
	{ 1, 0x1000, &spritelayout,      16*4, 32 },	/* sprites */
	{ 1, 0xd000, &charlayout2,  16*4+32*8, 16 },	/* background tiles */
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */

  0xff,0x00,0x00, /* RED */
  0x00,0xff,0x00, /* GREEN */
  0x00,0x00,0xff, /* BLUE */
  0xff,0xff,0x00, /* YELLOW */
  0xff,0x00,0xff, /* MAGENTA */
  0x00,0xff,0xff, /* CYAN */
  0xff,0xff,0xff, /* WHITE */
  0xE0,0xE0,0xE0, /* LTGRAY */
  0xC0,0xC0,0xC0, /* DKGRAY */

	0xe0,0xb0,0x70,	/* BROWN */
	0xd0,0xa0,0x60,	/* BROWN0 */
	0xc0,0x90,0x50,	/* BROWN1 */
	0xa3,0x78,0x3a,	/* BROWN2 */
	0x80,0x60,0x20,	/* BROWN3 */
	0x54,0x40,0x14,	/* BROWN4 */

  0x54,0xa8,0xff, /* LTBLUE */
  0x00,0xa0,0x00, /* DKGREEN */
  0x00,0xe0,0x00, /* GRASSGREEN */



	0xff,0xb6,0xdb,	/* PINK */
	0x49,0xb6,0xdb,	/* DKCYAN */
	0xff,96,0x49,	/* DKORANGE */
	0xff,128,0x00,	/* ORANGE */
	0xdb,0xdb,0xdb	/* GREY */
};

enum {BLACK,RED,GREEN,BLUE,YELLOW,MAGENTA,CYAN,WHITE,LTGRAY,DKGRAY,
       BROWN,BROWN0,BROWN1,BROWN2,BROWN3,BROWN4,
	LTBLUE,DKGREEN,GRASSGREEN,PINK,DKCYAN,DKORANGE,ORANGE,GREY};

static unsigned char colortable[] =
{
	/* chars (16) */
        0,0,BLACK,CYAN,   /* Top line */
        0,0,BLACK,WHITE,  /* Second line, HIGH SCORES, SEGA */
        0,0,BLACK,YELLOW, /* CREDITS, INSERT COIN */
        0,CYAN,BLACK,RED, /* scores, BONUS */
        0,0,BLACK,GREEN,  /* 1 PLAYER... */
        BLACK,WHITE,BLACK,BROWN,   /* Life */
        BROWN3,BROWN2,BLACK,BLACK, /* Life */
        0,0,0,0,
        0,0,0,0,
        0,0,0,0,
        0,0,0,0,
        0,0,0,0,
        0,0,0,0,
        0,0,0,0,
        0,0,0,0,
	0,0,0,0,


	/* sprites (32) */
        0,BROWN1,BLACK,BROWN,BROWN4,BROWN3,BROWN2,WHITE,   /* monkey1 */
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,BROWN4,BLACK,BROWN2,BLACK,BROWN3,BROWN4,WHITE,  /* monkey2 */
        0,WHITE,BLACK,BROWN,BROWN0,BROWN3,BROWN1,BROWN2,  /* hero */
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,LTGRAY,LTGRAY,LTGRAY,LTGRAY,LTGRAY,LTGRAY,LTGRAY, /* stage 3: beside holes */
        0,0,0,0,0,0,0,0,
        0,BROWN1,BROWN3,BROWN2,BROWN2,BROWN3,BROWN3,BROWN2, /* stage 1: falling bridge */
        0,0,0,0,0,0,0,0,
        0,DKGRAY,LTGRAY,LTBLUE,BLUE,WHITE,BLACK,DKCYAN,     /* hippo */
        0,RED,BROWN4,ORANGE,YELLOW,WHITE,BROWN1,BLACK,      /* scorpion */
        0,GREEN,ORANGE,GRASSGREEN,DKGREEN,BLACK,WHITE,BROWN4, /* snake & rino1 */
        0,RED,DKORANGE,YELLOW,ORANGE,BROWN2,BROWN3,BROWN,   /* barrels */
        0,BROWN4,BLACK,BROWN,WHITE,BROWN1,BROWN,WHITE,      /* hero asleep in tent */
        0,BROWN2,YELLOW,YELLOW,BLACK,WHITE,BROWN3,BROWN,    /* guy in hole */
        0,GREEN,DKGREEN,BLACK,GRASSGREEN,WHITE,CYAN,WHITE,  /* stage 4: moving leaf */
        0,DKGRAY,CYAN,LTBLUE,BLUE,BLACK,WHITE,BROWN4,       /* rino1 */
        0,BROWN0,BROWN0,BROWN2,BROWN4,BROWN3,WHITE,BLACK,   /* kong */
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,DKGRAY,LTBLUE,LTGRAY,BLUE,WHITE,WHITE,BLACK,      /* whale */
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,ORANGE,DKGREEN,BLUE,YELLOW,GREEN,LTBLUE,BLACK,    /* snoring zzzzz */

	/* background tiles (16) */
        0,0,0,0,0,0,0,0,  /* Score Back, all Black */
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
 BROWN4,BROWN1,BROWN2,BROWN3,DKCYAN,LTBLUE,WHITE,DKGREEN, /* Brown with trees */
 BROWN4,BROWN1,BROWN2,BROWN3,DKCYAN,BROWN0,WHITE,BROWN3, /* Brown wood:stage 1 bridge, stage 2 */
 BROWN4,BROWN1,BROWN2,BROWN3,DKCYAN,GRASSGREEN,LTGRAY,DKGREEN, /* stage 2 green */
 BLACK,BROWN1,BROWN2,BROWN3,DKCYAN,BROWN3,WHITE,LTGRAY, /* stage 1 under the skeleton, stage 3 */
        0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
        32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	congo_vh_start,
	congo_vh_stop,
	congo_vh_screenrefresh,

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

ROM_START( congo_rom )
	ROM_REGION(0x10000)	/* 64k for code */
        ROM_LOAD( "%s1.bin",  0x0000, 0x2000 )
        ROM_LOAD( "%s2.bin",  0x2000, 0x2000 )
        ROM_LOAD( "%s3.bin",  0x4000, 0x2000 )
        ROM_LOAD( "%s4.bin",  0x6000, 0x2000 )

        ROM_REGION(0x13000)      /* temporary space for graphics (disposed after conversion) */
        ROM_LOAD( "%s5.bin", 0x0000, 0x1000 )
        ROM_LOAD( "%s16.bin", 0x1000, 0x2000 )
        ROM_LOAD( "%s15.bin", 0x3000, 0x2000 )
        ROM_LOAD( "%s12.bin", 0x5000, 0x2000 )
        ROM_LOAD( "%s14.bin", 0x7000, 0x2000 )
        ROM_LOAD( "%s11.bin", 0x9000, 0x2000 )
        ROM_LOAD( "%s13.bin", 0xb000, 0x2000 )
        ROM_LOAD( "%s10.bin", 0xd000, 0x2000 )
        ROM_LOAD( "%s9.bin",  0xf000, 0x2000 )
        ROM_LOAD( "%s8.bin",  0x11000, 0x2000 )
/*
*        5 is characters, 3 bitplanes in one rom
*        16 and 15 are 1 plane of the sprites
*        12 and 14 the second
*        11 and 13 third one
*        10 - 8 are the background graphics
*/
        ROM_REGION(0x8000)      /* background data */
        ROM_LOAD( "%s6.bin",  0x0000, 0x2000 )
        ROM_LOAD( "%s6.bin",  0x2000, 0x2000 )
        ROM_LOAD( "%s7.bin",  0x4000, 0x2000 )
        ROM_LOAD( "%s7.bin", 0x6000, 0x2000 )
       /* I cheated a little with the background graphics - this
        * approach involved least writing ;)
        */
ROM_END



static int hiload(const char *name)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8030],"\x00\x89\x00",3) == 0 &&
			memcmp(&RAM[0x8099],"\x00\x37\x00",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x8020],1,21*6,f);
			RAM[0x80bd] = RAM[0x8030];
			RAM[0x80be] = RAM[0x8031];
			RAM[0x80bf] = RAM[0x8032];
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
		fwrite(&RAM[0x8020],1,21*6,f);
		fclose(f);
	}
}



struct GameDriver congo_driver =
{
	"Congo Bongo",
	"congo",
	"VILLE LAITINEN\nNICOLA SALMORIA\nMARC LAFONTAINE\nPAUL BERBERICH",
	&machine_driver,

	congo_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	0, palette, colortable,
	{ 0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,	/* numbers */
		0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,	/* letters */
		0x6e,0x6f,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a },
	0x00, 0x01,
	8*13, 8*16, 0x05,

	hiload, hisave
};
