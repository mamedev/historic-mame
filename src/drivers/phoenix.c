/***************************************************************************
Note:
   pleiads is using another sound driver, sndhrdw\pleiads.c
 Andrew Scott (ascott@utkux.utcc.utk.edu)

Phoenix memory map

0000-3fff 16Kb Program ROM
4000-43ff 1Kb Video RAM Charset A (4340-43ff variables)
4400-47ff 1Kb Work RAM
4800-4bff 1Kb Video RAM Charset B (4840-4bff variables)
4c00-4fff 1Kb Work RAM
5000-53ff 1Kb Video Control write-only (mirrored)
5400-47ff 1Kb Work RAM
5800-5bff 1Kb Video Scroll Register (mirrored)
5c00-5fff 1Kb Work RAM
6000-63ff 1Kb Sound Control A (mirrored)
6400-67ff 1Kb Work RAM
6800-6bff 1Kb Sound Control B (mirrored)
6c00-6fff 1Kb Work RAM
7000-73ff 1Kb 8bit Game Control read-only (mirrored)
7400-77ff 1Kb Work RAM
7800-7bff 1Kb 8bit Dip Switch read-only (mirrored)
7c00-7fff 1Kb Work RAM

memory mapped ports:

read-only:
7000-73ff IN
7800-7bff DSW

 * IN (all bits are inverted)
 * bit 7 : barrier
 * bit 6 : Left
 * bit 5 : Right
 * bit 4 : Fire
 * bit 3 : -
 * bit 2 : Start 2
 * bit 1 : Start 1
 * bit 0 : Coin

 * DSW
 * bit 7 : VBlank
 * bit 6 : free play (pleiads only)
 * bit 5 : attract sound 0 = off 1 = on (pleiads only?)
 * bit 4 : coins per play  0 = 1 coin  1 = 2 coins
 * bit 3 :\ bonus
 * bit 2 :/ 00 = 3000  01 = 4000  10 = 5000  11 = 6000
 * bit 1 :\ number of lives
 * bit 0 :/ 00 = 3  01 = 4  10 = 5  11 = 6

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *phoenix_videoram2;

int phoenix_DSW_r (int offset);
int phoenix_interrupt (void);

void phoenix_videoram2_w(int offset,int data);
void phoenix_scrollreg_w (int offset,int data);
void phoenix_videoreg_w (int offset,int data);
int phoenix_vh_start(void);
void phoenix_vh_stop(void);
void phoenix_vh_screenrefresh(struct osd_bitmap *bitmap);

void phoenix_sound_control_a_w(int offset, int data);
void phoenix_sound_control_b_w(int offset, int data);
int phoenix_sh_init(const char *gamename);
int phoenix_sh_start(void);
void phoenix_sh_update(void);
void pleiads_sound_control_a_w(int offset, int data);
void pleiads_sound_control_b_w(int offset, int data);
int pleiads_sh_init(const char *gamename);
int pleiads_sh_start(void);
void pleiads_sh_update(void);


static struct MemoryReadAddress readmem[] =
{
	{ 0x7800, 0x7Bff, input_port_1_r },	/* DSW */
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4fff, MRA_RAM },	/* video RAM */
	{ 0x5000, 0x6fff, MRA_RAM },
	{ 0x7000, 0x73ff, input_port_0_r },	/* IN0 */
	{ 0x7400, 0x77ff, MRA_RAM },
	{ 0x7c00, 0x7fff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, phoenix_videoram2_w, &phoenix_videoram2 },
	{ 0x4400, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, videoram_w, &videoram, &videoram_size },
	{ 0x4C00, 0x4fff, MWA_RAM },
	{ 0x5000, 0x53ff, phoenix_videoreg_w },
	{ 0x5400, 0x57ff, MWA_RAM },
	{ 0x5800, 0x5bff, phoenix_scrollreg_w },
	{ 0x5C00, 0x5fff, MWA_RAM },
        { 0x6000, 0x63ff, phoenix_sound_control_a_w },
	{ 0x6400, 0x67ff, MWA_RAM },
        { 0x6800, 0x6bff, phoenix_sound_control_b_w },
	{ 0x6C00, 0x6fff, MWA_RAM },
	{ 0x7400, 0x77ff, MWA_RAM },
	{ 0x7C00, 0x7fff, MWA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress pl_writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, phoenix_videoram2_w, &phoenix_videoram2 },
	{ 0x4400, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, videoram_w, &videoram, &videoram_size },
	{ 0x4C00, 0x4fff, MWA_RAM },
	{ 0x5000, 0x53ff, phoenix_videoreg_w },
	{ 0x5400, 0x57ff, MWA_RAM },
	{ 0x5800, 0x5bff, phoenix_scrollreg_w },
	{ 0x5C00, 0x5fff, MWA_RAM },
        { 0x6000, 0x63ff, pleiads_sound_control_a_w },
	{ 0x6400, 0x67ff, MWA_RAM },
        { 0x6800, 0x6bff, pleiads_sound_control_b_w },
	{ 0x6C00, 0x6fff, MWA_RAM },
	{ 0x7400, 0x77ff, MWA_RAM },
	{ 0x7C00, 0x7fff, MWA_RAM },
	{ -1 }	/* end of table */
};


static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_3, OSD_KEY_1, OSD_KEY_2, 0,
			OSD_KEY_CONTROL, OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_ALT },
		{ 0, 0, 0, 0, OSD_JOY_FIRE1, OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_FIRE2 }
	},
	{	/* DSW */
		0x60,
		{ 0, 0, 0, 0, 0, 0, 0, IPB_VBLANK },
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
        { 0, 6, "MOVE LEFT"  },
        { 0, 5, "MOVE RIGHT" },
        { 0, 7, "BARRIER"    },
        { 0, 4, "FIRE" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 1, 0x03, "LIVES", { "3", "4", "5", "6" } },
	{ 1, 0x0c, "BONUS", { "3000", "4000", "5000", "6000" } },
	{ 1, 0x20, "DEMO SOUNDS", { "OFF", "ON" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },	/* pretty straightforward layout */
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,  0, 16 },
	{ 1, 0x1000, &charlayout, 64, 16 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */
	0xff,0xff,0xff,	/* WHITE */
	0xff,0x00,0x00,	/* RED */
	0x49,0xdb,0x00,	/* GREEN */
	0x24,0x24,0xdb,	/* BLUE */
	0x00,0xdb,0x95,	/* CYAN, */
	0xff,0xff,0x00,	/* YELLOW, */
	0xff,0xb6,0xdb,	/* PINK */
	0xff,0xb6,0x49,	/* ORANGE */
        0xff,0x24,0xb6, /* LTPURPLE */
	0xff,0xb6,0x00,	/* DKORANGE */
        0xb6,0x24,0xff, /* DKPURPLE */
        0x95,0x95,0x95, /* GREY */
	0xdb,0xdb,0x00,	/* DKYELLOW */
        0x00,0x95,0xff, /* BLUISH */
        0xff,0x00,0xff, /* PURPLE */
};

enum {pBLACK,pWHITE,pRED,pGREEN,pBLUE,pCYAN,pYELLOW,pPINK,pORANGE,pLTPURPLE,pDKORANGE,
                pDKPURPLE,pGREY,pDKYELLOW,pBLUISH,pPURPLE};

/* 4 colors per pixel * 8 groups of characters * 2 charsets * 2 pallettes */
static unsigned char colortable[] =
{
        /* charset A pallette A */
        pBLACK,pBLACK,pCYAN,pCYAN,          /* Background, Unused, Letters, asterisks */
        pBLACK,pYELLOW,pRED,pWHITE,         /* Background, Ship middle, Numbers/Ship, Ship edge */
        pBLACK,pYELLOW,pRED,pWHITE,         /* Background, Ship middle, Ship, Ship edge/bullets */
        pBLACK,pPINK,pPURPLE,pYELLOW,       /* Background, Bird eyes, Bird middle, Bird Wings */
        pBLACK,pPINK,pPURPLE,pYELLOW,       /* Background, Bird eyes, Bird middle, Bird Wings */
        pBLACK,pPINK,pPURPLE,pYELLOW,       /* Background, Bird eyes, Bird middle, Bird Wings */
        pBLACK,pWHITE,pPURPLE,pYELLOW,      /* Background, Explosions */
        pBLACK,pPURPLE,pGREEN,pWHITE,       /* Background, Barrier */
        /* charset A pallette B */
        pBLACK,pBLUE,pCYAN,pCYAN,           /* Background, Unused, Letters, asterisks */
        pBLACK,pYELLOW,pRED,pWHITE,         /* Background, Ship middle, Numbers/Ship, Ship edge */
        pBLACK,pYELLOW,pRED,pWHITE,         /* Background, Ship middle, Ship, Ship edge/bullets */
        pBLACK,pYELLOW,pGREEN,pPURPLE,      /* Background, Bird eyes, Bird middle, Bird Wings */
        pBLACK,pYELLOW,pGREEN,pPURPLE,      /* Background, Bird eyes, Bird middle, Bird Wings */
        pBLACK,pYELLOW,pGREEN,pPURPLE,      /* Background, Bird eyes, Bird middle, Bird Wings */
        pBLACK,pWHITE,pRED,pPURPLE,         /* Background, Explosions */
        pBLACK,pPURPLE,pGREEN,pWHITE,       /* Background, Barrier */
        /* charset B pallette A */
        pBLACK,pRED,pBLUE,pGREY,            /* Background, Starfield */
        pBLACK,pPURPLE,pBLUISH,pDKORANGE,   /* Background, Planets */
        pBLACK,pDKPURPLE,pGREEN,pDKORANGE,  /* Background, Mothership: turrets, u-body, l-body */
        pBLACK,pBLUISH,pDKPURPLE,pLTPURPLE, /* Background, Motheralien: face, body, feet */
        pBLACK,pPURPLE,pBLUISH,pGREEN,      /* Background, Eagles: face, body, shell */
        pBLACK,pPURPLE,pBLUISH,pGREEN,      /* Background, Eagles: face, body, feet */
        pBLACK,pPURPLE,pBLUISH,pGREEN,      /* Background, Eagles: face, body, feet */
        pBLACK,pPURPLE,pBLUISH,pGREEN,      /* Background, Eagles: face, body, feet */
        /* charset B pallette B */
        pBLACK,pRED,pBLUE,pGREY,            /* Background, Starfield */
        pBLACK,pPURPLE,pBLUISH,pDKORANGE,   /* Background, Planets */
        pBLACK,pDKPURPLE,pGREEN,pDKORANGE,  /* Background, Mothership: turrets, upper body, lower body */
        pBLACK,pBLUISH,pDKPURPLE,pLTPURPLE, /* Background, Motheralien: face, body, feet */
        pBLACK,pBLUISH,pLTPURPLE,pGREEN,    /* Background, Eagles: face, body, shell */
        pBLACK,pBLUISH,pLTPURPLE,pGREEN,    /* Background, Eagles: face, body, feet */
        pBLACK,pBLUISH,pLTPURPLE,pGREEN,    /* Background, Eagles: face, body, feet */
        pBLACK,pBLUISH,pLTPURPLE,pGREEN     /* Background, Eagles: face, body, feet */
};

static unsigned char pl_colortable[] =
{
        /* charset A pallette A */
        pBLACK,pBLACK,pCYAN,pCYAN,          /* Background, Unused, Letters, asterisks */
        pBLACK,pYELLOW,pRED,pWHITE,         /* Background, Ship sides, Numbers/Ship, Ship gun */
        pBLACK,pYELLOW,pRED,pWHITE,         /* Background, Ship sides, Ship body, Ship gun */
        pBLACK,pYELLOW,pBLACK,pYELLOW,      /* Background, Martian1 eyes, Martian1 body, Martian1 mouth */
        pBLACK,pYELLOW,pBLACK,pYELLOW,      /* Background, Martian1 eyes, Martian1 body, Martian1 mouth */
        pBLACK,pPINK,pPURPLE,pYELLOW,       /* Background, UFO edge, UFO middle, UFO eyes */
        pBLACK,pWHITE,pPURPLE,pYELLOW,      /* Background, Martian2 legs, Martian2 head, Martian2 eyes */
        pBLACK,pPURPLE,pGREEN,pWHITE,       /* Background, Explosions */
        /* charset A pallette B */
        pBLACK,pBLUE,pCYAN,pCYAN,           /* Background, Unused, Letters, asterisks */
        pBLACK,pYELLOW,pRED,pWHITE,         /* Background, Ship sides, Numbers/Ship, Ship gun */
        pBLACK,pYELLOW,pRED,pWHITE,         /* Background, Ship sides, Ship body, Ship gun */
        pBLACK,pYELLOW,pBLUISH,pWHITE,      /* Background, Martian1 eyes, Martian1 body, Martian1 mouth */
        pBLACK,pYELLOW,pBLUISH,pWHITE,      /* Background, Martian1 eyes, Martian1 body, Martian1 mouth */
        pBLACK,pYELLOW,pGREEN,pPURPLE,      /* Background, UFO edge, UFO middle, UFO eyes */
        pBLACK,pWHITE,pRED,pPURPLE,         /* Background, Martian2 legs, Martian2 head, Martian2 eyes */
        pBLACK,pPURPLE,pGREEN,pWHITE,       /* Background, Explosions */
        /* charset B pallette A */
        pBLACK,pRED,pBLUE,pGREY,            /* Background, Stars & planets */
        pBLACK,pPURPLE,pBLUISH,pDKORANGE,   /* Background, Battleship: body, eyes, edge */
        pBLACK,pDKPURPLE,pGREEN,pDKORANGE,  /* Background, Radar dish inside, Radar base/parked ship, Radar dish outside */
        pBLACK,pBLUISH,pDKPURPLE,pLTPURPLE, /* Background, Building/Landing light(off?), parked ship, Landing light(on?) */
        pBLACK,pPURPLE,pBLUISH,pGREEN,      /* Background, Space Monsters: face, body, horns/claws */
        pBLACK,pPURPLE,pBLUISH,pGREEN,      /* Background, Space Monsters: face, body, horns/claws */
        pBLACK,pPURPLE,pBLUISH,pGREEN,      /* Background, Space Monsters: face, body, horns/claws */
        pBLACK,pPURPLE,pBLUISH,pGREEN,      /* Background, Space Monsters: body, horns/claws */
        /* charset B pallette B */
        pBLACK,pRED,pBLUE,pGREY,            /* Background, Stars & planets */
        pBLACK,pPURPLE,pBLUISH,pDKORANGE,   /* Background, Battleship: body, eyes, edge */
        pBLACK,pDKPURPLE,pGREEN,pDKORANGE,  /* Background, Radar dish inside, Radar base/parked ship, Radar dish outside */
        pBLACK,pBLUISH,pDKPURPLE,pLTPURPLE, /* Background, Building/Landing light(off?), parked ship, Landing light(on?) */
        pBLACK,pBLUISH,pLTPURPLE,pGREEN,    /* Background, Space Monsters: face, body, horns/claws */
        pBLACK,pBLUISH,pLTPURPLE,pGREEN,    /* Background, Space Monsters: face, body, horns/claws */
        pBLACK,pBLUISH,pLTPURPLE,pGREEN,    /* Background, Space Monsters: face, body, horns/claws */
        pBLACK,pBLUISH,pLTPURPLE,pGREEN,    /* Background, Space Monsters: body, horns/claws */
};


/* waveforms for the audio hardware */
static unsigned char samples[141] =
{
        /* sine-wave */
        0x0F, 0x0F, 0x0F, 0x06, 0x06, 0x09, 0x09, 0x06, 0x06, 0x09, 0x06, 0x0D, 0x0F, 0x0F, 0x0D, 0x00,
        0xE6, 0xDE, 0xE1, 0xE6, 0xEC, 0xE6, 0xE7, 0xE7, 0xE7, 0xEC, 0xEC, 0xEC, 0xE7, 0xE1, 0xE1, 0xE7,
/*
	0x00,0x00,0x00,0x00,0x22,0x22,0x22,0x22,0x44,0x44,0x44,0x44,0x22,0x22,0x22,0x22,
        0x00,0x00,0x00,0x00,0xdd,0xdd,0xdd,0xdd,0xbb,0xbb,0xbb,0xbb,0xdd,0xdd,0xdd,0xdd,
*/
        /* white-noise ? */
        0x79, 0x75, 0x71, 0x72, 0x72, 0x6F, 0x70, 0x71, 0x71, 0x73, 0x75, 0x76, 0x74, 0x74, 0x78, 0x7A,
        0x79, 0x7A, 0x7B, 0x7C, 0x7C, 0x7C, 0x7C, 0x7C, 0x7C, 0x7C, 0x7D, 0x80, 0x85, 0x88, 0x88, 0x87,
        0x8B, 0x8B, 0x8A, 0x8A, 0x89, 0x87, 0x85, 0x87, 0x89, 0x86, 0x83, 0x84, 0x84, 0x85, 0x84, 0x84,
        0x85, 0x86, 0x87, 0x87, 0x88, 0x88, 0x86, 0x81, 0x7E, 0x7D, 0x7F, 0x7D, 0x7C, 0x7D, 0x7D, 0x7C,
        0x7E, 0x81, 0x7F, 0x7C, 0x7E, 0x82, 0x82, 0x82, 0x82, 0x83, 0x83, 0x84, 0x83, 0x82, 0x82, 0x83,
        0x82, 0x84, 0x88, 0x8C, 0x8E, 0x8B, 0x8B, 0x8C, 0x8A, 0x8A, 0x8A, 0x89, 0x85, 0x86, 0x89, 0x89,
        0x86, 0x85, 0x85, 0x85, 0x84, 0x83, 0x82, 0x83, 0x83, 0x83, 0x82, 0x83, 0x83

};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3 Mhz ? */
			0,
			readmem,writemem,0,0,
			phoenix_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 3*8, 29*8-1, 0*8, 31*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	phoenix_vh_start,
	phoenix_vh_stop,
	phoenix_vh_screenrefresh,

	/* sound hardware */
        samples,
	phoenix_sh_init,
        phoenix_sh_start,
	0,
        phoenix_sh_update
};

static struct MachineDriver pleiads_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3 Mhz ? */
			0,
                        readmem,pl_writemem,0,0,
			phoenix_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 3*8, 29*8-1, 0*8, 31*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	phoenix_vh_start,
	phoenix_vh_stop,
	phoenix_vh_screenrefresh,

	/* sound hardware */
        samples,
        pleiads_sh_init,
        pleiads_sh_start,
	0,
        pleiads_sh_update
};

static const char *phoenix_sample_names[] =
{
	"shot8.sam",
	"death8.sam",
	0	/* end of array */
};

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( phoenix_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "ic45", 0x0000, 0x0800, 0x2278c24a )
	ROM_LOAD( "ic46", 0x0800, 0x0800, 0xfefbcdb1 )
	ROM_LOAD( "ic47", 0x1000, 0x0800, 0x39e00a04 )
	ROM_LOAD( "ic48", 0x1800, 0x0800, 0xdc27f959 )
	ROM_LOAD( "ic49", 0x2000, 0x0800, 0x08391997 )
	ROM_LOAD( "ic50", 0x2800, 0x0800, 0x0e45f309 )
	ROM_LOAD( "ic51", 0x3000, 0x0800, 0x45f34a7b )
	ROM_LOAD( "ic52", 0x3800, 0x0800, 0xce53b2e1 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ic39", 0x0000, 0x0800, 0x721b653d )
	ROM_LOAD( "ic40", 0x0800, 0x0800, 0x8ee80800 )
	ROM_LOAD( "ic23", 0x1000, 0x0800, 0x1461bf99 )
	ROM_LOAD( "ic24", 0x1800, 0x0800, 0xa8b5c2b1 )
ROM_END

ROM_START( phoenixt_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "phoenix.45", 0x0000, 0x0800, 0xb0ae4830 )
	ROM_LOAD( "phoenix.46", 0x0800, 0x0800, 0xfafbc9b1 )
	ROM_LOAD( "phoenix.47", 0x1000, 0x0800, 0x687116a3 )
	ROM_LOAD( "phoenix.48", 0x1800, 0x0800, 0xeb71206d )
	ROM_LOAD( "phoenix.49", 0x2000, 0x0800, 0xc7f9d957 )
	ROM_LOAD( "phoenix.50", 0x2800, 0x0800, 0x0e45f309 )
	ROM_LOAD( "phoenix.51", 0x3000, 0x0800, 0x45f34a7b )
	ROM_LOAD( "phoenix.52", 0x3800, 0x0800, 0xd456acde )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "phoenix.39", 0x0000, 0x0800, 0x721b653d )
	ROM_LOAD( "phoenix.40", 0x0800, 0x0800, 0x8ee80800 )
	ROM_LOAD( "phoenix.23", 0x1000, 0x0800, 0x1461bf99 )
	ROM_LOAD( "phoenix.24", 0x1800, 0x0800, 0xa8b5c2b1 )
ROM_END

ROM_START( phoenix3_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "phoenix.45", 0x0000, 0x0800, 0x585c1ef6 )
	ROM_LOAD( "phoenix.46", 0x0800, 0x0800, 0x19d5cb29 )
	ROM_LOAD( "phoenix.47", 0x1000, 0x0800, 0x687116a3 )
	ROM_LOAD( "phoenix.48", 0x1800, 0x0800, 0x83722aac )
	ROM_LOAD( "phoenix.49", 0x2000, 0x0800, 0x08391997 )
	ROM_LOAD( "phoenix.50", 0x2800, 0x0800, 0x0e45f309 )
	ROM_LOAD( "phoenix.51", 0x3000, 0x0800, 0x45f34a7b )
	ROM_LOAD( "phoenix.52", 0x3800, 0x0800, 0xcc51b4e3 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "phoenix.39", 0x0000, 0x0800, 0xa93f0f43 )
	ROM_LOAD( "phoenix.40", 0x0800, 0x0800, 0x8ee80800 )
	ROM_LOAD( "phoenix.23", 0x1000, 0x0800, 0x1461bf99 )
	ROM_LOAD( "phoenix.24", 0x1800, 0x0800, 0xa8b5c2b1 )
ROM_END

ROM_START( pleiads_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "pleiades.47", 0x0000, 0x0800, 0x11a6373e )
	ROM_LOAD( "pleiades.48", 0x0800, 0x0800, 0x63c4c9d2 )
	ROM_LOAD( "pleiades.49", 0x1000, 0x0800, 0xc88cbee2 )
	ROM_LOAD( "pleiades.50", 0x1800, 0x0800, 0x0bc4e7c0 )
	ROM_LOAD( "pleiades.51", 0x2000, 0x0800, 0x48470843 )
	ROM_LOAD( "pleiades.52", 0x2800, 0x0800, 0xafa44e9c )
	ROM_LOAD( "pleiades.53", 0x3000, 0x0800, 0xa18f0cdd )
	ROM_LOAD( "pleiades.54", 0x3800, 0x0800, 0x1d125968 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "pleiades.27", 0x0000, 0x0800, 0x880280d4 )
	ROM_LOAD( "pleiades.26", 0x0800, 0x0800, 0x96ac4eb6 )
	ROM_LOAD( "pleiades.45", 0x1000, 0x0800, 0x3617f459 )
	ROM_LOAD( "pleiades.44", 0x1800, 0x0800, 0x35271f77 )
ROM_END



static int hiload(const char *name)
{
   FILE *f;

   /* get RAM pointer (this game is multiCPU, we can't assume the global */
   /* RAM pointer is pointing to the right place) */
   unsigned char *RAM = Machine->memory_region[0];

   /* check if the hi score table has already been initialized */
   if (memcmp(&RAM[0x438a],"\x00\x00\x0f",3) == 0)
   {
      if ((f = fopen(name,"rb")) != 0)
      {
         fread(&RAM[0x4388],1,4,f);

/*       // I suppose noone can do such an HISCORE!!! ;)
         phoenix_videoram2_w(0x0221, (RAM[0x4388] >> 4)+0x20);
         phoenix_videoram2_w(0x0201, (RAM[0x4388] & 0xf)+0x20);
*/
         phoenix_videoram2_w(0x01e1, (RAM[0x4389] >> 4)+0x20);
         phoenix_videoram2_w(0x01c1, (RAM[0x4389] & 0xf)+0x20);
         phoenix_videoram2_w(0x01a1, (RAM[0x438a] >> 4)+0x20);
         phoenix_videoram2_w(0x0181, (RAM[0x438a] & 0xf)+0x20);
         phoenix_videoram2_w(0x0161, (RAM[0x438b] >> 4)+0x20);
         phoenix_videoram2_w(0x0141, (RAM[0x438b] & 0xf)+0x20);
         fclose(f);
      }

      return 1;
   }
   else return 0; /* we can't load the hi scores yet */
}



static unsigned long get_score(char *score)
{
     return (score[3])+(256*score[2])+((unsigned long)(65536)*score[1])+((unsigned long)(65536)*256*score[0]);
}

static void hisave(const char *name)
{
   unsigned long score1,score2,hiscore;
   FILE *f;

   /* get RAM pointer (this game is multiCPU, we can't assume the global */
   /* RAM pointer is pointing to the right place) */
   unsigned char *RAM = Machine->memory_region[0];

   score1 = get_score(&RAM[0x4380]);
   score2 = get_score(&RAM[0x4384]);
   hiscore = get_score(&RAM[0x4388]);

   if (score1 > hiscore) RAM += 0x4380;         /* check consistency */
   else if (score2 > hiscore) RAM += 0x4384;
   else RAM += 0x4388;

   if ((f = fopen(name,"wb")) != 0)
   {
      fwrite(&RAM[0],1,4,f);
      fclose(f);
   }
}



struct GameDriver phoenix_driver =
{
	"Phoenix (Amstar)",
	"phoenix",
	"RICHARD DAVIES\nBRAD OLIVER\nMIRKO BUFFONI\nNICOLA SALMORIA\nSHAUN STEPHENSON\nANDREW SCOTT",
	&machine_driver,

	phoenix_rom,
	0, 0,
	phoenix_sample_names,

	input_ports, trak_ports, dsw, keys,

	0, palette, colortable,
	8*13, 8*16,

	hiload, hisave
};

struct GameDriver phoenixt_driver =
{
	"Phoenix (Taito)",
	"phoenixt",
	"RICHARD DAVIES\nBRAD OLIVER\nMIRKO BUFFONI\nNICOLA SALMORIA\nSHAUN STEPHENSON\nANDREW SCOTT",
	&machine_driver,

	phoenixt_rom,
	0, 0,
	phoenix_sample_names,

	input_ports, trak_ports, dsw, keys,

	0, palette, colortable,
	8*13, 8*16,

	hiload, hisave
};

struct GameDriver phoenix3_driver =
{
	"Phoenix (T.P.N.)",
	"phoenix3",
	"RICHARD DAVIES\nBRAD OLIVER\nMIRKO BUFFONI\nNICOLA SALMORIA\nSHAUN STEPHENSON\nANDREW SCOTT",
	&machine_driver,

	phoenix3_rom,
	0, 0,
	phoenix_sample_names,

	input_ports, trak_ports, dsw, keys,

	0, palette, colortable,
	8*13, 8*16,

	hiload, hisave
};

struct GameDriver pleiads_driver =
{
	"Pleiads",
	"pleiads",
	"RICHARD DAVIES\nBRAD OLIVER\nMIRKO BUFFONI\nNICOLA SALMORIA\nSHAUN STEPHENSON\nANDREW SCOTT",
	&pleiads_machine_driver,

	pleiads_rom,
	0, 0,
        phoenix_sample_names,

	input_ports, trak_ports, dsw, keys,

        0, palette, pl_colortable,
	8*13, 8*16,

	hiload, hisave
};
