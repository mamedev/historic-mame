/***************************************************************************

Donkey Kong memory map (preliminary)

0000-3fff ROM
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
 * bit 6 : ?
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
 * bit 6 : ?
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
#include "machine.h"
#include "common.h"


unsigned char *dkong_videoram;
unsigned char *dkong_colorram;
unsigned char *dkong_spriteram;
void dkong_videoram_w(int offset,int data);
void dkong_colorram_w(int offset,int data);
int dkong_vh_start(void);
void dkong_vh_stop(void);
void dkong_vh_screenrefresh(struct osd_bitmap *bitmap);



static struct MemoryReadAddress readmem[] =
{
	{ 0x6000, 0x6fff, MRA_RAM },	/* including sprites ram */
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x7c00, 0x7c00, input_port_0_r },	/* IN0 */
	{ 0x7c80, 0x7c80, input_port_1_r },	/* IN1 */
	{ 0x7d00, 0x7d00, input_port_2_r },	/* IN2 */
	{ 0x7d80, 0x7d80, input_port_3_r },	/* DSW1 */
	{ 0x7400, 0x77ff, MRA_RAM },	/* video RAM */
	{ 0x7800, 0x7bff, MRA_RAM },	/* color RAM */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x6000, 0x68ff, MWA_RAM },
	{ 0x6a80, 0x6fff, MWA_RAM },
	{ 0x6900, 0x6a7f, MWA_RAM, &dkong_spriteram },
	{ 0x7d84, 0x7d84, interrupt_enable_w },
	{ 0x7400, 0x77ff, dkong_videoram_w, &dkong_videoram },
//	{ 0x7800, 0x7bff, dkong_colorram_w, &dkong_colorram },
	{ 0x0000, 0x3fff, MWA_ROM },
//	{ 0x7000, 0x73ff, MWA_RAM },	// ??
	{ 0x7800, 0x7803, MWA_RAM },	// ??
	{ 0x7808, 0x7808, MWA_RAM },	// ??
	{ 0x7c00, 0x7c00, MWA_RAM },	// ??
	{ 0x7d00, 0x7d07, MWA_RAM },	// ??
	{ 0x7d80, 0x7d83, MWA_RAM },	// ??
	{ 0x7d85, 0x7d87, MWA_RAM },	// ??
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


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 256*8*8 },	/* the two bitplanes are separated */
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



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0x10000, &charlayout,      0, 16 },
	{ 0x11000, &spritelayout,    0, 16 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */
	0x49,0x00,0x00,	/* DKRED1 */
	0x92,0x00,0x00,	/* DKRED2 */
	0xff,0x00,0x00,	/* RED */
	0x00,0x24,0x00,	/* DKGRN1 */
	0x92,0x24,0x00,	/* DKBRN1 */
	0xb6,0x24,0x00,	/* DKBRN2 */
	0xff,0x24,0x00,	/* LTRED1 */
	0xdb,0x49,0x00,	/* BROWN */
	0x00,0x6c,0x00,	/* DKGRN2 */
	0xff,0x6c,0x00,	/* LTORG1 */
	0x00,0x92,0x00,	/* DKGRN3 */
	0x92,0x92,0x00,	/* DKYEL */
	0xdb,0x92,0x00,	/* DKORG */
	0xff,0x92,0x00,	/* ORANGE */
	0x00,0xdb,0x00,	/* GREEN1 */
	0x6d,0xdb,0x00,	/* LTGRN1 */
	0x00,0xff,0x00,	/* GREEN2 */
	0x49,0xff,0x00,	/* LTGRN2 */
	0xff,0xff,0x00,	/* YELLOW */
	0x00,0x00,0x55,	/* DKBLU1 */
	0xff,0x00,0x55,	/* DKPNK1 */
	0xff,0x24,0x55,	/* DKPNK2 */
	0xff,0x6d,0x55,	/* LTRED2 */
	0xdb,0x92,0x55,	/* LTBRN */
	0xff,0x92,0x55,	/* LTORG2 */
	0x24,0xff,0x55,	/* LTGRN3 */
	0x49,0xff,0x55,	/* LTGRN4 */
	0xff,0xff,0x55,	/* LTYEL */
	0x00,0x00,0xaa,	/* DKBLU2 */
	0xff,0x00,0xaa,	/* PINK1 */
	0x00,0x24,0xaa,	/* DKBLU3 */
	0xff,0x24,0xaa,	/* PINK2 */
	0xdb,0xdb,0xaa,	/* CREAM */
	0xff,0xdb,0xaa,	/* LTORG3 */
	0x00,0x00,0xff,	/* BLUE */
	0xdb,0x00,0xff,	/* PURPLE */
	0x00,0xb6,0xff,	/* LTBLU1 */
	0x92,0xdb,0xff,	/* LTBLU2 */
	0xdb,0xdb,0xff,	/* WHITE1 */
	0xff,0xff,0xff	/* WHITE2 */
};

enum {BLACK,DKRED1,DKRED2,RED,DKGRN1,DKBRN1,DKBRN2,LTRED1,BROWN,DKGRN2,
	LTORG1,DKGRN3,DKYEL,DKORG,ORANGE,GREEN1,LTGRN1,GREEN2,LTGRN2,YELLOW,
	DKBLU1,DKPNK1,DKPNK2,LTRED2,LTBRN,LTORG2,LTGRN3,LTGRN4,LTYEL,DKBLU2,
	PINK1,DKBLU3,PINK2,CREAM,LTORG3,BLUE,PURPLE,LTBLU1,LTBLU2,WHITE1,
	WHITE2};

static unsigned char colortable[] =
{
	/* characters and sprites */
	BLACK,PINK1,RED,DKGRN3,               /* 1st level */
	BLACK,LTORG1,WHITE1,LTRED1,             /* pauline with kong */
	BLACK,RED,CREAM,BLUE,		/* Mario */
	BLACK,PINK1,RED,DKGRN3,                  /* 3rd level */
	BLACK,BLUE,LTBLU1,LTYEL,                /* 4th lvl */
	BLACK,BLUE,LTYEL,LTBLU1,               /* 2nd level */
	BLACK,RED,CREAM,BLUE,                  /* blue text */
	BLACK,LTYEL,BROWN,WHITE1,	/* hammers */
	BLACK,LTBRN,BROWN,CREAM,               /* kong */
	BLACK,RED,LTRED1,YELLOW,             /* oil flame */
	BLACK,LTBRN,CREAM,LTRED1,              /* pauline */
	BLACK,LTYEL,BLUE,BROWN,		/* barrels */
	BLACK,CREAM,LTBLU2,BLUE,	/* "oil" barrel */
	BLACK,YELLOW,BLUE,RED,               /* small mario, spring */
	BLACK,DKGRN3,LTBLU1,BROWN,            /* scared flame */
	BLACK,LTRED1,YELLOW,BLUE,            /* flame */
};



const struct MachineDriver dkong_driver =
{
	/* basic machine hardware */
	3072000,	/* 3.072 Mhz */
	60,
	readmem,
	writemem,
	input_ports,dsw,
	0,
	nmi_interrupt,
	0,

	/* video hardware */
	256,256,
	gfxdecodeinfo,
	41,4*24,
	0,0,palette,colortable,
	0,17,
	7,4,
	8*13,8*16,1,
	0,
	dkong_vh_start,
	dkong_vh_stop,
	dkong_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0,
	0,
	0
};
