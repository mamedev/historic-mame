/***************************************************************************

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
 * bit 6 : free play (Pleiades only)
 * bit 5 : attract sound 0 = off 1 = on (Pleiades only?)
 * bit 4 : coins per play  0 = 1 coin  1 = 2 coins
 * bit 3 :\ bonus
 * bit 2 :/ 00 = 3000  01 = 4000  10 = 5000  11 = 6000
 * bit 1 :\ number of lives
 * bit 0 :/ 00 = 3  01 = 4  10 = 5  11 = 6

***************************************************************************/

#include "driver.h"


extern unsigned char *phoenix_videoram1;
extern unsigned char *phoenix_videoram2;

int phoenix_DSW_r (int offset);
int phoenix_interrupt (void);

void phoenix_videoram1_w(int offset,int data);
void phoenix_videoram2_w(int offset,int data);
void phoenix_scrollreg_w (int offset,int data);
void phoenix_videoreg_w (int offset,int data);
int phoenix_vh_start(void);
void phoenix_vh_stop(void);
void phoenix_vh_screenrefresh(struct osd_bitmap *bitmap);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x4fff, MRA_RAM },	/* video RAM */
	{ 0x5000, 0x6fff, MRA_RAM },
	{ 0x7000, 0x73ff, input_port_0_r },	/* IN0 */
	{ 0x7400, 0x77ff, MRA_RAM },
	{ 0x7800, 0x7Bff, phoenix_DSW_r },	/* DSW */
	{ 0x7c00, 0x7fff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, phoenix_videoram1_w, &phoenix_videoram1 },
	{ 0x4400, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, phoenix_videoram2_w, &phoenix_videoram2 },
	{ 0x4C00, 0x4fff, MWA_RAM },
	{ 0x5000, 0x53ff, phoenix_videoreg_w },
	{ 0x5400, 0x57ff, MWA_RAM },
	{ 0x5800, 0x5bff, phoenix_scrollreg_w },
	{ 0x5C00, 0x5fff, MWA_RAM },
//	{ 0x6000, 0x63ff, phoenix_sounda_w },
	{ 0x6400, 0x67ff, MWA_RAM },
//	{ 0x6800, 0x6bff, phoenix_soundb_w },
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
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
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
	{ 0x10000, &charlayout,  0, 16 },
	{ 0x11000, &charlayout, 64, 16 },
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
	0x00,0xdb,0xdb,	/* DKCYAN */
	0xdb,0xdb,0x00,	/* DKYELLOW */
        0x00,0x95,0xff, /* BLUISH */
        0xff,0x00,0xff, /* PURPLE */
};

enum {pBLACK,pWHITE,pRED,pGREEN,pBLUE,pCYAN,pYELLOW,pPINK,pORANGE,pLTPURPLE,pDKORANGE,
                pDKPURPLE,pDKCYAN,pDKYELLOW,pBLUISH,pPURPLE};

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
        pBLACK,pRED,pBLUE,pWHITE,           /* Background, Starfield */
        pBLACK,pPURPLE,pBLUISH,pDKORANGE,   /* Background, Planets */
        pBLACK,pDKPURPLE,pGREEN,pDKORANGE,  /* Background, Mothership: turrets, u-body, l-body */
        pBLACK,pBLUISH,pDKPURPLE,pLTPURPLE, /* Background, Motheralien: face, body, feet */
        pBLACK,pPURPLE,pBLUISH,pGREEN,      /* Background, Eagles: face, body, shell */
        pBLACK,pPURPLE,pBLUISH,pGREEN,      /* Background, Eagles: face, body, feet */
        pBLACK,pPURPLE,pBLUISH,pGREEN,      /* Background, Eagles: face, body, feet */
        pBLACK,pPURPLE,pBLUISH,pGREEN,      /* Background, Eagles: face, body, feet */
        /* charset B pallette B */
        pBLACK,pRED,pBLUE,pWHITE,           /* Background, Starfield */
        pBLACK,pPURPLE,pBLUISH,pDKORANGE,   /* Background, Planets */
        pBLACK,pDKPURPLE,pGREEN,pDKORANGE,  /* Background, Mothership: turrets, upper body, lower body */
        pBLACK,pBLUISH,pDKPURPLE,pLTPURPLE, /* Background, Motheralien: face, body, feet */
        pBLACK,pBLUISH,pLTPURPLE,pGREEN,    /* Background, Eagles: face, body, shell */
        pBLACK,pBLUISH,pLTPURPLE,pGREEN,    /* Background, Eagles: face, body, feet */
        pBLACK,pBLUISH,pLTPURPLE,pGREEN,    /* Background, Eagles: face, body, feet */
        pBLACK,pBLUISH,pLTPURPLE,pGREEN    /* Background, Eagles: face, body, feet */
};



const struct MachineDriver phoenix_driver =
{
	/* basic machine hardware */
	1000000,	/* 1 Mhz ? */
	60,
	readmem,writemem,0,0,
	input_ports,dsw,
	0,
	phoenix_interrupt,

	/* video hardware */
	32*8, 32*8, { 3*8, 29*8-1, 0*8, 31*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,0,palette,colortable,
	32,1,
	0x00,0x04,
	8*13,8*16,0x01,
	0,
	phoenix_vh_start,
	phoenix_vh_stop,
	phoenix_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};
