/***************************************************************************

Millipede memory map (preliminary)

0400-040F       POKEY 1
0800-080F       POKEY 2
1000-13BF       SCREEN RAM (8x8 TILES, 32x30 SCREEN)
13C0-13CF       SPRITE IMAGE OFFSETS
13D0-13DF       SPRITE HORIZONTAL OFFSETS
13E0-13EF       SPRITE VERTICAL OFFSETS
13F0-13FF       SPRITE COLOR OFFSETS

2000            BIT 1-4 dip switch
                BIT 5 IS P1 FIRE
                BIT 6 IS P1 START
                BIT 7 IS SERVICE

2001            BIT 1-4 dip switch
                BIT 5 IS P2 FIRE
                BIT 6 IS P2 START
                BIT 7,8 (?)

2010            BIT 1 IS P1 RIGHT
                BIT 2 IS P1 LEFT
                BIT 3 IS P1 DOWN
                BIT 4 IS P1 UP
                BIT 5 IS SLAM, LEFT COIN, AND UTIL COIN
                BIT 6,7 (?)
                BIT 8 IS RIGHT COIN

2480-248F       COLOR RAM
2600            INTERRUPT ACKNOWLEDGE
2680            CLEAR WATCHDOG
4000-7FFF       GAME CODE

*************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern int centiped_rand_r(int offset);

extern void milliped_vh_screenrefresh(struct osd_bitmap *bitmap);

extern void milliped_pokey1_w(int offset,int data);
extern void milliped_pokey2_w(int offset,int data);
extern int milliped_sh_start(void);
extern void milliped_sh_stop(void);
extern void milliped_sh_update(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x0200, MRA_RAM },
	{ 0x1000, 0x13ff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_ROM },
	{ 0xf000, 0xffff, MRA_ROM },	/* for the reset / interrupt vectors */
	{ 0x2000, 0x2000, input_port_0_r },
	{ 0x2001, 0x2001, input_port_1_r },
	{ 0x2010, 0x2010, input_port_2_r },
        { 0x2011, 0x2011, input_port_3_r },
        { 0x0408, 0x0408, input_port_4_r },
        { 0x0808, 0x0808, input_port_5_r },
	{ 0x40a, 0x40a, centiped_rand_r },
	{ 0x80a, 0x80a, centiped_rand_r },
	{ -1 }	/* end of table */
};



static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x0200, MWA_RAM },
	{ 0x1000, 0x13ff, videoram_w, &videoram },
	{ 0x13c0, 0x13ff, MWA_RAM, &spriteram },
	{ 0x0400, 0x0408, milliped_pokey1_w },
	{ 0x0800, 0x0808, milliped_pokey2_w },
	{ 0x4000, 0x73ff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{
		0xf8,
		{ 0, 0, 0, 0, OSD_KEY_CONTROL, OSD_KEY_1, 0, 0},
		{ 0, 0, 0, 0, OSD_JOY_FIRE, 0, 0,0}
	},
	{
		0xf0,
		{ 0, 0, 0, 0, OSD_KEY_CONTROL, OSD_KEY_2, 0, 0 },
		{ 0, 0, 0, 0, OSD_JOY_FIRE, 0, 0, 0}
	},
	{
		0xff,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_DOWN, OSD_KEY_UP,
				0, 0, OSD_KEY_3, 0 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_DOWN, OSD_JOY_UP,
				0, 0, 0, 0}
	},
	{
		0xff,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_DOWN, OSD_KEY_UP,
				0, 0, 0, 0 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_DOWN, OSD_JOY_UP,
				0, 0, 0, 0}
	},
	{	/* DSW1 */
		0x14,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0x02,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};


static struct KEYSet keys[] =
{
        { 2, 3, "PL1 MOVE UP" },
        { 2, 1, "PL1 MOVE LEFT"  },
        { 2, 0, "PL1 MOVE RIGHT" },
        { 2, 2, "PL1 MOVE DOWN" },
        { 0, 4, "PL1 FIRE" },
        { 3, 3, "PL2 MOVE UP" },
        { 3, 1, "PL2 MOVE LEFT"  },
        { 3, 0, "PL2 MOVE RIGHT" },
        { 3, 2, "PL2 MOVE DOWN" },
        { 1, 4, "PL2 FIRE" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 4, 0x0c, "LIVES", { "2", "3", "4", "5" } },
	{ 4, 0x30, "BONUS", { "12000", "15000", "20000", "NONE" } },
	{ 4, 0x80, "STARTING SCORE SELECT", { "ON", "OFF" }, 1 },
	{ 0, 0x0c, "", { "0", "0 1XBONUS", "0 1XBONUS 2XBONUS", "0 1XBONUS 2XBONUS 3XBONUS" } },
	{ 4, 0x01, "MILLIPEDE HEAD", { "EASY", "HARD" } },
	{ 4, 0x02, "BEETLE", { "EASY", "HARD" } },
	{ 4, 0x40, "SPIDER", { "EASY", "HARD" } },
	{ 0, 0x03, "LANGUAGE", { "ENGLISH", "GERMAN", "FRENCH", "SPANISH" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,8,	/* 16*8 sprites */
	128,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 128*16*8 },	/* the two bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	16*8	/* every sprite takes 16 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 8 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,   /* black      */
	0x94,0x00,0xd8,   /* darkpurple */
	0xd8,0x00,0x00,   /* darkred    */
	0xf8,0x64,0xd8,   /* pink       */
	0x00,0xd8,0x00,   /* darkgreen  */
	0x00,0xf8,0xd8,   /* darkcyan   */
	0xd8,0xd8,0x94,   /* darkyellow */
	0xd8,0xf8,0xd8,   /* darkwhite  */
	0xf8,0x94,0x44,   /* orange     */
	0x00,0x00,0xd8,   /* blue   */
	0xf8,0x00,0x00,   /* red    */
	0xff,0x00,0xff,   /* purple */
	0x00,0xf8,0x00,   /* green  */
	0x00,0xff,0xff,   /* cyan   */
	0xf8,0xf8,0x00,   /* yellow */
	0xff,0xff,0xff    /* white  */
};

enum
{
	black, darkpurple, darkred, pink, darkgreen, darkcyan, darkyellow,
		darkwhite, orange, blue, red, purple, green, cyan, yellow, white
};

static unsigned char colortable[] =
{
	black, darkred,   blue,       darkyellow,
	black, green,     darkpurple, orange,
	black, darkgreen, darkred,    yellow,
	black, darkred,   darkgreen,  yellow,
	black, yellow,    darkgreen,  red,
	black, green,     orange,     yellow,
	black, darkwhite, red,        pink,
	black, darkcyan,  red,        darkwhite
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_M6502,
			1000000,	/* 1 Mhz ???? */
			0,
			readmem,writemem,0,0,
			interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	milliped_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	milliped_sh_start,
	milliped_sh_stop,
	milliped_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( milliped_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "%s.104", 0x4000, 0x1000 )
	ROM_LOAD( "%s.103", 0x5000, 0x1000 )
	ROM_LOAD( "%s.102", 0x6000, 0x1000 )
	ROM_LOAD( "%s.101", 0x7000, 0x1000 )
	ROM_LOAD( "%s.101", 0xf000, 0x1000 )	/* for the reset and interrupt vectors */

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "%s.106", 0x0000, 0x0800 )
	ROM_LOAD( "%s.107", 0x0800, 0x0800 )
ROM_END



static int hiload(const char *name)
{
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x0064],"\x75\x91\x08",3) == 0 &&
			memcmp(&RAM[0x0079],"\x16\x19\x04",3) == 0)
	{
		FILE *f;


		if ((f = fopen(name,"rb")) != 0)
		{
			fread(&RAM[0x0064],1,6*8,f);
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
		fwrite(&RAM[0x0064],1,6*8,f);
		fclose(f);
	}
}



struct GameDriver milliped_driver =
{
	"Millipede",
	"milliped",
	"IVAN MACKINTOSH\nNICOLA SALMORIA",
	&machine_driver,

	milliped_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	0, palette, colortable,
	8*13, 8*16,

	hiload, hisave
};

