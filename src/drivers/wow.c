/* ------------------------------------------------------------------ */
/* This driver is currently being worked on by Mike@Dissfulfils.co.uk */
/* ------------------------------------------------------------------ */

/***************************************************************************

Wizard of Wor memory map (preliminary)

0000-3fff ROM A/B/C/D but also "magic" RAM (which processes data and copies it to the video RAM)
4000-7fff SCREEN RAM (bitmap)
8000-afff ROM E/F/G
c000-cfff ROM X (Foreign language - not required)
d000-d3ff STATIC RAM

I/O ports:
IN:
08        intercept register (collision detector)
          bit 0: intercept in pixel 3 in an OR or XOR write since last reset
          bit 1: intercept in pixel 2 in an OR or XOR write since last reset
          bit 2: intercept in pixel 1 in an OR or XOR write since last reset
          bit 3: intercept in pixel 0 in an OR or XOR write since last reset
          bit 4: intercept in pixel 3 in last OR or XOR write
          bit 5: intercept in pixel 2 in last OR or XOR write
          bit 6: intercept in pixel 1 in last OR or XOR write
          bit 7: intercept in pixel 0 in last OR or XOR write
10        IN0
11        IN1
12        IN2
13        DSW
14		  Video Retrace
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
00-07     palette (00-03 = left part of screen; 04-07 right part)
09        position where to switch from the "left" to the "right" palette.
08        select video mode (0 = low res 160x102, 1 = high res 320x204)
0a        screen height
0b        color block transfer
0c        magic RAM control
          bit 7: ?
          bit 6: flip
          bit 5: draw in XOR mode
          bit 4: draw in OR mode
          bit 3: "expand" mode (convert 1bpp data to 2bpp)
          bit 2: ?
          bit 1:\ shift amount to be applied before copying
          bit 0:/
0d        set interrupt vector
10-18     sound
19        magic RAM expand mode color
78-7e     pattern board (see vidhrdw.c for details)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "Z80.h"

extern unsigned char *wow_videoram;
extern int wow_intercept_r(int offset);
extern void wow_videoram_w(int offset,int data);
extern void wow_magic_expand_color_w(int offset,int data);
extern void wow_magic_control_w(int offset,int data);
extern void wow_magicram_w(int offset,int data);
extern void wow_pattern_board_w(int offset,int data);
extern void wow_vh_screenrefresh(struct osd_bitmap *bitmap);

extern int wow_video_retrace_r(int offset);

extern void gorf_interrupt_w(int offset, int data);
extern void scanline_interrupt_w(int offset, int data);
extern int  gorf_interrupt(void);

/* These calls don't do anything (yet) but they stop
   unwanted lines appearing in the logfile! */

extern void colour_register_w(int offset, int data);
extern void paging_register_w(int offset, int data);

static struct MemoryReadAddress readmem[] =
{
        { 0xD000, 0xDfff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_RAM },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0xcfff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
        { 0xD000, 0xDfff, MWA_RAM },
	{ 0x4000, 0x7fff, wow_videoram_w, &wow_videoram },
	{ 0x0000, 0x3fff, wow_magicram_w },
	{ 0x8000, 0xcfff, MWA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress robby_readmem[] =
{
	{ 0xe000, 0xffff, MRA_RAM },
	{ 0x4000, 0x7fff, MRA_RAM },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8000, 0xdfff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress robby_writemem[] =
{
	{ 0xe000, 0xffff, MWA_RAM },
	{ 0x4000, 0x7fff, wow_videoram_w, &wow_videoram },
	{ 0x0000, 0x3fff, wow_magicram_w },
	{ 0x8000, 0xdfff, MWA_ROM },
	{ -1 }	/* end of table */
};


static struct IOReadPort readport[] =
{
	{ 0x08, 0x08, wow_intercept_r },
        { 0x0E, 0x0E, wow_video_retrace_r },
	{ 0x10, 0x10, input_port_0_r },
	{ 0x11, 0x11, input_port_1_r },
	{ 0x12, 0x12, input_port_2_r },
	{ 0x13, 0x13, input_port_3_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x0d, 0x0d, interrupt_vector_w },
	{ 0x19, 0x19, wow_magic_expand_color_w },
	{ 0x0c, 0x0c, wow_magic_control_w },
	{ 0x78, 0x7e, wow_pattern_board_w },
        { 0x0E, 0x0E, gorf_interrupt_w },
        { 0x0F, 0x0F, scanline_interrupt_w },
        { 0x00, 0x08, colour_register_w },
        { 0x5B, 0x5B, paging_register_w },
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


static struct KEYSet keys[] =
{
        { 2, 0, "MOVE UP" },
        { 2, 2, "MOVE LEFT"  },
        { 2, 3, "MOVE RIGHT" },
        { 2, 1, "MOVE DOWN" },
        { 2, 5, "FIRE1" },
        { 2, 4, "FIRE2" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 3, 0x10, "LIVES", { "3 7", "2 5" }, 1 },
	{ 3, 0x20, "BONUS", { "4TH LEVEL", "3RD LEVEL" }, 1 },
	{ 2, 0x80, "DEMO SOUNDS", { "OFF", "ON" } },
/*	{ 3, 0x08, "LANGUAGE", { "FOREIGN", "ENGLISH" }, 1 }, */
	{ -1 }
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



static struct MachineDriver wow_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,writemem,readport,writeport,
			interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	320, 204, { 0, 320-1, 0, 204-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	wow_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};

/* Space Zap */

/* This uses a scanline driven interrupt at scanlines 0,100 and 200 */
/* I haven't implemented this, instead do 3 interrupts per frame    */

static struct MachineDriver spacezap_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,writemem,readport,writeport,
			interrupt,3
		}
	},
	60,
	0,

	/* video hardware */
	320, 204, { 0, 320-1, 0, 204-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	wow_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};

/* Gorf - Not Working */

static struct MachineDriver gorf_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,writemem,readport,writeport,
			gorf_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	320, 204, { 0, 320-1, 0, 204-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	wow_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};

/* Robby Roto - Not Working */

/* This uses a scanline driven interrupt at scanlines 50,100 and 200 */
/* I haven't implemented this, instead do 3 interrupts per frame     */

static struct MachineDriver robby_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			robby_readmem,robby_writemem,readport,writeport,
			interrupt,3
		}
	},
	60,
	0,

	/* video hardware */
	320, 204, { 0, 320-1, 0, 204-1 },
	0,	/* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	0,
	generic_vh_start,
	generic_vh_stop,
	wow_vh_screenrefresh,

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

ROM_START( wow_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "wow.x1", 0x0000, 0x1000 )
	ROM_LOAD( "wow.x2", 0x1000, 0x1000 )
	ROM_LOAD( "wow.x3", 0x2000, 0x1000 )
	ROM_LOAD( "wow.x4", 0x3000, 0x1000 )
	ROM_LOAD( "wow.x5", 0x8000, 0x1000 )
	ROM_LOAD( "wow.x6", 0x9000, 0x1000 )
	ROM_LOAD( "wow.x7", 0xa000, 0x1000 )
/*	ROM_LOAD( "wow.x8", 0xc000, 0x1000 )	here would go the foreign language ROM */
ROM_END

ROM_START( spacezap_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "0662.01", 0x0000, 0x1000 )
	ROM_LOAD( "0663.xx", 0x1000, 0x1000 )
	ROM_LOAD( "0664.xx", 0x2000, 0x1000 )
	ROM_LOAD( "0665.xx", 0x3000, 0x1000 )
ROM_END

ROM_START( robby_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "robbya", 0x0000, 0x1000 )
	ROM_LOAD( "robbyb", 0x1000, 0x1000 )
	ROM_LOAD( "robbyc", 0x2000, 0x1000 )
	ROM_LOAD( "robbyd", 0x3000, 0x1000 )
	ROM_LOAD( "robbye", 0x8000, 0x1000 )
	ROM_LOAD( "robbyf", 0x9000, 0x1000 )
	ROM_LOAD( "robbyg", 0xa000, 0x1000 )
	ROM_LOAD( "robbyh", 0xb000, 0x1000 )
	ROM_LOAD( "robbyi", 0xc000, 0x1000 )
	ROM_LOAD( "robbyj", 0xd000, 0x1000 )
ROM_END

ROM_START( gorf_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "gorf-a.bin", 0x0000, 0x1000 )
	ROM_LOAD( "gorf-b.bin", 0x1000, 0x1000 )
	ROM_LOAD( "gorf-c.bin", 0x2000, 0x1000 )
	ROM_LOAD( "gorf-d.bin", 0x3000, 0x1000 )
	ROM_LOAD( "gorf-e.bin", 0x8000, 0x1000 )
	ROM_LOAD( "gorf-f.bin", 0x9000, 0x1000 )
	ROM_LOAD( "gorf-g.bin", 0xa000, 0x1000 )
	ROM_LOAD( "gorf-h.bin", 0xb000, 0x1000 )
ROM_END



struct GameDriver wow_driver =
{
        "Wizard of Wor",
	"wow",
        "NICOLA SALMORIA\nMIKE COATES",
	&wow_machine_driver,

	wow_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	0, palette, 0,
	(320-8*6)/2, (204-10)/2,

	0, 0
};

struct GameDriver spacezap_driver =
{
        "Space Zap",
	"spacezap",
        "NICOLA SALMORIA\nMIKE COATES",
	&spacezap_machine_driver,

	spacezap_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	0, palette, 0,
	(320-8*6)/2, (204-10)/2,

	0, 0
};

struct GameDriver robby_driver =
{
        "Robby Roto",
	"robby",
        "NICOLA SALMORIA\nMIKE COATES",
	&robby_machine_driver,

	robby_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	0, palette, 0,
	(320-8*6)/2, (204-10)/2,

	0, 0
};

struct GameDriver gorf_driver =
{
        "Gorf",
	"gorf",
        "NICOLA SALMORIA\nMIKE COATES",
	&gorf_machine_driver,

	gorf_rom,
	0, 0,
	0,

	input_ports, dsw, keys,

	0, palette, 0,
	(320-8*6)/2, (204-10)/2,

	0, 0
};
