/***************************************************************************

Wizard of Wor memory map (preliminary)

0000-3fff ROM A/B/C/D but also masked video RAM
4000-7fff SCREEN RAM (bitmap)
8000-afff ROM E/F/G
c000-cfff ROM X (Foreign language - not required)
d000-d3ff STATIC RAM

I/O ports:
IN:
08        collision detector?
10        IN0
11        IN1
12        IN2
13        DSW
15        ?
17        ?

*
 * IN0 (all bits are inverted)
 * bit 7 : flip screen
 * bit 6 : START 2
 * bit 5 : START 1
 * bit 4 : SLAM
 * bit 3 : TEST
 * bit 2 : COIN 3
 * bit 1 : COIN 2
 * bit 0 : COIN 1
 *
*
 * IN1 (all bits are inverted)
 * bit 7 : ?
 * bit 6 : ?
 * bit 5 : FIRE player 1
 * bit 4 : MOVE player 1
 * bit 3 : RIGHT player 1
 * bit 2 : LEFT player 1
 * bit 1 : DOWN player 1
 * bit 0 : UP player 1
 *
*
 * IN2 (all bits are inverted)
 * bit 7 : ?
 * bit 6 : ?
 * bit 5 : FIRE player 2
 * bit 4 : MOVE player 2
 * bit 3 : RIGHT player 2
 * bit 2 : LEFT player 2
 * bit 1 : DOWN player 2
 * bit 0 : UP player 2
 *
*
 * DSW (all bits are inverted)
 * bit 7 : Attract mode sound  0 = Off  1 = On
 * bit 6 : 1 = Coin play  0 = Free Play
 * bit 5 : Bonus  0 = After fourth level  1 = After third level
 * bit 4 : Number of lives  0 = 3/7  1 = 2/5
 * bit 3 : language  1 = English  0 = Foreign
 * bit 2 : \ right coin slot  00 = 1 coin 5 credits  01 = 1 coin 3 credits
 * bit 1 : /                  10 = 2 coins 1 credit  11 = 1 coin 1 credit
 * bit 0 : left coin slot 0 = 2 coins 1 credit  1 = 1 coin 1 credit
 *


OUT:
0d        set interrupt vector
blitter registers:
0c        ?
19        plane mask?
78-7e     blitter (see vidhrdw.c for details)

***************************************************************************/

#include "driver.h"
#include "machine.h"
#include "common.h"



extern unsigned char *wow_videoram;
extern int wow_collision_r(int offset);
extern void wow_videoram_w(int offset,int data);
extern void wow_masked_videoram_w(int offset,int data);
extern void wow_blitter_mask_w(int offset,int data);
extern void wow_blitter_unknown_w(int offset,int data);
extern void wow_blitter_w(int offset,int data);
extern int wow_vh_start(void);
extern void wow_vh_stop(void);
extern void wow_vh_screenrefresh(struct osd_bitmap *bitmap);



static struct MemoryReadAddress readmem[] =
{
	{ 0xd000, 0xd3ff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_RAM },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0xafff, MRA_ROM },
	{ 0xc000, 0xcfff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0xd000, 0xd3ff, MWA_RAM },
	{ 0x4000, 0x7fff, wow_videoram_w, &wow_videoram },
	{ 0x0000, 0x3fff, wow_masked_videoram_w },
	{ 0x8000, 0xafff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct IOReadPort readport[] =
{
	{ 0x08, 0x08, wow_collision_r },
	{ 0x10, 0x10, input_port_0_r },
	{ 0x11, 0x11, input_port_1_r },
	{ 0x12, 0x12, input_port_2_r },
	{ 0x13, 0x13, input_port_3_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x0d, 0x0d, interrupt_vector_w },
	{ 0x19, 0x19, wow_blitter_mask_w },
	{ 0x0c, 0x0c, wow_blitter_unknown_w },
	{ 0x78, 0x7e, wow_blitter_w },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_3, 0, 0, OSD_KEY_F2,
				0, OSD_KEY_1, OSD_KEY_2, OSD_KEY_B },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0xef,
		{ OSD_KEY_E, OSD_KEY_D, OSD_KEY_S, OSD_KEY_F, OSD_KEY_X, OSD_KEY_G, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0xef,
		{ OSD_KEY_UP, OSD_KEY_DOWN, OSD_KEY_LEFT, OSD_KEY_RIGHT, OSD_KEY_ALT, OSD_KEY_CONTROL, 0, 0 },
		{ OSD_JOY_UP, OSD_JOY_DOWN, OSD_JOY_LEFT, OSD_JOY_RIGHT, 0, OSD_JOY_FIRE, 0, 0 }
	},
	{	/* DSW */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW dsw[] =
{
	{ 3, 0x10, "LIVES", { "3 7", "2 5" }, 1 },
	{ 3, 0x20, "BONUS", { "4TH LEVEL", "3RD LEVEL" }, 1 },
	{ 2, 0x80, "DEMO SOUNDS", { "OFF", "ON" } },
/*	{ 3, 0x08, "LANGUAGE", { "FOREIGN", "ENGLISH" }, 1 }, */
	{ -1 }
};



/* Wizard of Wor doesn't have character mapped graphics, this definition is here */
/* only for the dip switch menu */
static struct GfxLayout charlayout =
{
	8,10,	/* 8*10 characters */
	44,	/* 44 characters */
	1,	/* 1 bit per pixel */
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },	/* pretty straightforward layout */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8 },
	10*8	/* every char takes 10 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 0x004ce, &charlayout,      0, 4 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */
	0xff,0xff,0x00,	/* YELLOW */
	0x00,0x00,0xff,	/* BLUE */
	0xff,0x00,0x00,	/* RED */
	0xff,0xff,0xff	/* WHITE */
};

enum { BLACK,YELLOW,BLUE,RED,WHITE };

static unsigned char colortable[] =
{
	BLACK,YELLOW,BLUE,RED,
	BLACK,WHITE,BLACK,RED	/* not used by the game, here only for the dip switch menu */
};



const struct MachineDriver wow_driver =
{
	/* basic machine hardware */
	3072000,	/* 3.072 Mhz */
	60,
	readmem,writemem,readport,writeport,
	input_ports,dsw,
	0,
	interrupt,

	/* video hardware */
	320,204,
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,0,palette,colortable,
	0,11,
	2,0,
	(320-8*6)/2,(204-10)/2,3,
	0,
	wow_vh_start,
	wow_vh_stop,
	wow_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};
