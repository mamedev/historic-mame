/***************************************************************************

The following doc was provided by Paul Leaman (paull@phonelink.com)


                                    1942

                            Hardware Description


                                Revision 0.4



INTRODUCTION
------------

This document describes the 1942 hardware. This will only be useful
to other emulator authors or for the curious.


LEGAL
-----

This document is freely distributable (with or without the emulator).
You may place it on a WEB page if you want.

You are free to use this information for whatever purpose you want providing
that:

* No profit is made
* You credit me somewhere in the documentation.
* The document is not changed




HARDWARE ARRANGEMENT
--------------------

1942 is a two board system. Board 1 contains 2 Z80A CPUs. One is used for
the sound, the other for the game.

The sound system uses 2 YM-2203 synth chips. These are compatible with the
AY-8910. Michael Cuddy has information (and source code) for these chips
on his Web page.

The second board contains the custom graphics hardware. There are three
graphics planes. The test screen refers to them as Scroll, Sprite and Tile.

The scroll and sprites are arranged in 16*16 blocks. The graphics ROMS
are not memory mapped. They are accessed directly by the hardware.

Early Capcom games seem to have similar hardware.


ROM descriptions:
=================

The generally available ROMs are as follows:

Board 1 - Code

Sound CPU:
1-C11.BIN       16K Sound ROM 0000-3fff

Main CPU:
1-N4.BIN        16K CODE ROM 0000-4000
1-N3.BIN        16K CODE ROM 4000-7fff
1-N5.BIN        16K CODE ROM 8000-bfff (paged)
1-N6.BIN         8K CODE ROM 8000-9fff (paged)
1-N7.BIN        16K CODE ROM 8000-bfff (paged)

1-F2.BIN         8K  Character ROM (Not mapped)

Board 2 - Graphics board
2-A5.BIN         8K TILE PLANE 1
2-A3.BIN         8K TILE PLANE 2
2-A1.BIN         8K TILE PLANE 3

2-A6.BIN         8K TILE PLANE 1
2-A4.BIN         8K TILE PLANE 2
2-A2.BIN         8K TILE PLANE 3

2-N1.BIN        16K OBJECT PLANE 1&2
2-L1.BIN        16K OBJECT PLANE 3&4

2-L2.BIN        16K OBJECT PLANE 1&2
2-N2.BIN        16K OBJECT PLANE 3&4


SOUND CPU MEMORY MAP
====================

0000-3fff Sound board CODE.
4000-47ff RAM data area and stack
6000      Sound input port. 0-0x1f
8000      PSG 1 Address
8001      PSG 1 Data
c000      PSG 2 Address
c001      PSG 2 Data

Runs in interrupt mode 1.

After initialization, Most of the sound code is driven by interrupt (0x38).
The code sits around waiting for the value in 0x6000 to change.

All Capcom games seem to share the same music hardware. The addresses of
the PSG chips and input vary according to the game.



MAIN CPU MEMORY MAP
===================

0000-bfff ROM Main code. Area (8000-bfff) is paged in

          Input ports
c000      Coin mech and start buttons
          0x10 Coin up
          0x08 Plater 4 start ????
          0x04 Player 3 start ????
          0x02 Player 2 start
          0x01 Player 1 start
c001      Joystick
c002      Joystick
c003      DIP switch 1 (1=off 0=on)
c004      DIP switch 2 (1=off 0=on)

          Output ports
c800      Sound output
c801      Unused
c802      Scroll register (lower 4 bits smooth, upper 4 bits rough)
c803      Scroll register MSB
c804      Watchdog circuit flip-flop ????
c805      Unknown
c806      Bits
            0-1 ROM paging   0=1-N5.BIN
                             1=1-N6.BIN
                             2=1-N7.BIN

          Video
cc00-cc7f Sprite RAM
          32 * 4 bytes

d000-d3ff Character RAM
d400-d7ff Character attribute
          Bits
             0x80 MSB character
             0x40
             0x20
             rest Attribute
d800-dbff Scroll RAM / attributes
             Alternating 16 byte rows of characters / attributes

             Attribute
                0x80 MSB tile
                0x40 Flip X
                0x20 Flip Y
                rest Attribute
e000-efff    RAM data / stack area
F000-FFFF    Unused

Game runs in interrupt mode 0 (the devices supply the interrupt number).

Two interrupts must be triggered per refresh for the game to function
correctly.

0x10 is the video retrace. This controls the speed of the game and generally
     drives the code. This must be triggerd for each video retrace.
0x08 is the sound card service interupt. The game uses this to throw sounds
     at the sound CPU.



Character RAM arrangement
-------------------------

The characters are rotated 90 degrees vertically. Each column is 32 bytes.

The attributes are arranged so that they correspond to each column.

Attribute
      0x80  Char MSB Set to get characters from 0x100 to 0x1ff
      0x40  Unknown
      rest  Character palette colour.


Tile system
-----------

Tiles are arranged in rotational buffer. Each line consists of 32 bytes.
The first 16 bytes are the tile values, the second 16 bytes contain the
attributes.

This arrangement may vary according to the machine. For example, Vulgus,
which is a horizontal/ vertical scroller uses 32 bytes per line. The
attributes are in a separate block of memory. This is probably done to
accommodate horizontal scrolling.

      0x80 Tile MSB (Set to obtain tiles 0x100-0x1ff.
      0x40 Tile flip X
      0x20 Tile Flip Y
      rest Palette colour scheme.

The scroll rough register determines the starting point for the bottom of the
screen. The buffer is circular. The bottom of the screen is at the start of
tile memory. To address the start of a line:

     lineaddress=(roughscroll * 0x20) & 0x3ff
     attributeaddress=lineaddress+0x10

Make sure to combine the MSB value to make up the rough scroll address.


Sprite arrangement
------------------

Sprites are 16*16 blocks. Attribute bits determine whether or not the
sprite is wider.

32 Sprites. 4 bytes for each sprite
    00 Sprite number 0-255
    01 Colour Code
         0x80 Sprite number MSB (256-512)
         0x40 Sprite size 16*64 (Very wide sprites)
         0x20 Sprite size 16*32 (Wide sprites)
         0x10 Sprite Y position MSB
         rest colour code
    02 Sprite X position
    03 Sprite Y position

The sprite sequence is slightly odd within the sprite data ROMS. It is
necessary to swap sprites 0x0080-0x00ff with sprites 0x0100-0x017f to get
the correct order. This is best done at load-up time.

If 0x40 is set, the next 4 sprite objects are combined into one from left
to right.
if 0x20 is set, the next 2 sprite objects are combined into one from left
to right.
if none of the above bits are set, the sprite is a simple 16*16 object.


Sprite clipping:
----------------

The title sprites are supposed to appear to move into each other. I
have not yet found the mechanism to do this.


Palette:
--------

Palette system is in hardware. There are 16 colours for each component (char,
scroll and object).

The .PAL file format used by the emulator is as follows:

Offset
0x0000-0x000f Char 16 colour palette colours
0x0010-0x001f Scroll 16 colour palette colours
0x0020-0x002f Object 16 colour palette colours
0x0030-0x00f0 Char colour schemes (3 bytes each). Values are 0-0xff
0x00f0-0x01ef Object colour scheme (16 bytes per scheme)
0x01f0-0x02ef Scroll colour scheme (8 bytes per scheme)

Note that only half the colour schemes are shown on the scroll palette
screen. The game uses colour values that are not shown. There must
be some mirroring here. Either that, or I have got the scheme wrong.



Interesting RAM locations:
--------------------------

0xE09B - Number of rolls



Graphics format:
----------------

Roberto Ventura has written a document, detailing the Ghosts 'n' Goblins
graphics layout. This can be found on the repository.


Schematics:
-----------

Schematics for Commando can be found at:

This game is fairly similar to 1942.



DIP Switch Settings
-------------------


WWW.SPIES.COM contains DIP switch settings.


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



extern void c1942_bankswitch_w(int offset,int data);
extern int c1942_init_machine(const char *gamename);
extern int c1942_interrupt(void);

extern unsigned char *c1942_backgroundram;
extern unsigned char *c1942_scroll;
extern void c1942_background_w(int offset,int data);
int c1942_vh_start(void);
void c1942_vh_stop(void);
extern void c1942_vh_screenrefresh(struct osd_bitmap *bitmap);

extern int c1942_sh_interrupt(void);
extern int c1942_sh_start(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0xe000, 0xefff, MRA_RAM },
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xd000, 0xdbff, MRA_RAM },
	{ 0xc000, 0xc000, input_port_0_r },
	{ 0xc001, 0xc001, input_port_1_r },
	{ 0xc002, 0xc002, input_port_2_r },
	{ 0xc003, 0xc003, input_port_3_r },
	{ 0xc004, 0xc004, input_port_4_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0xe000, 0xefff, MWA_RAM },
	{ 0xd000, 0xd3ff, videoram_w, &videoram },
	{ 0xd400, 0xd7ff, colorram_w, &colorram },
	{ 0xd800, 0xdbff, c1942_background_w, &c1942_backgroundram },
	{ 0xcc00, 0xcc7f, MWA_RAM, &spriteram },
	{ 0xc806, 0xc806, c1942_bankswitch_w },
	{ 0xc802, 0xc803, MWA_RAM, &c1942_scroll },
	{ 0xc800, 0xc800, sound_command_w },
	{ 0x0000, 0xbfff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x6000, 0x6000, sound_command_r },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x8000, 0x8000, AY8910_control_port_0_w },
	{ 0x8001, 0x8001, AY8910_write_port_0_w },
	{ 0xc000, 0xc000, AY8910_control_port_1_w },
	{ 0xc001, 0xc001, AY8910_write_port_1_w },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_1, OSD_KEY_2, 0, 0, OSD_KEY_3, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_DOWN, OSD_KEY_UP,
				OSD_KEY_CONTROL, OSD_KEY_ALT, 0, 0 },
		{ OSD_JOY_RIGHT, OSD_JOY_LEFT, OSD_JOY_DOWN, OSD_JOY_UP,
				OSD_JOY_FIRE1, OSD_JOY_FIRE2, 0, 0 }
	},
	{	/* IN2 */
		0xff,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW1 */
		0xf7,
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* DSW2 */
		0xff,
		{ 0, 0, 0, OSD_KEY_F2, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW dsw[] =
{
	{ 3, 0xc0, "LIVES", { "5", "2", "1", "3" }, 1 },
	{ 3, 0x30, "BONUS", { "30000 100000", "30000 80000", "20000 100000", "20000 80000" }, 1 },
	{ 4, 0x60, "DIFFICULTY", { "HARDEST", "HARD", "EASY", "NORMAL" }, 1 },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 2 bits per pixel */
	{ 0, 4 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	{ 8+3, 8+2, 8+1, 8+0, 3, 2, 1, 0 },
	16*8	/* every char takes 16 consecutive bytes */
};
static struct GfxLayout tilelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	3,	/* 3 bits per pixel */
	{ 0, 0x2000*8, 0x4000*8 },	/* the bitplanes are separated */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
		16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	32*8	/* every tile takes 32 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	4,	/* 4 bits per pixel */
	{ 0, 4, 0x4000*8, 0x4000*8+4 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	{ 33*8+3, 33*8+2, 33*8+1, 33*8+0, 32*8+3, 32*8+2, 32*8+1, 32*8+0,
			8+3, 8+2, 8+1, 8+0, 3, 2, 1, 0 },
	64*8	/* every sprite takes 64 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,            0, 32 },
	{ 1, 0x02000, &tilelayout,         32*4, 32 },
	{ 1, 0x08000, &tilelayout,         32*4, 32 },
	{ 1, 0x0e000, &spritelayout, 32*4+32*16, 16 },
	{ 1, 0x16000, &spritelayout, 32*4+32*16, 16 },
	{ 1, 0x10000, &spritelayout, 32*4+32*16, 16 },
	{ 1, 0x18000, &spritelayout, 32*4+32*16, 16 },
	{ -1 } /* end of array */
};



static unsigned char palette[] =
{
	0x00,0x00,0x00,	/* BLACK */
	25,34,195,	/* BLUE */
	25,161,153,	/* CYAN */
	68,68,68, /* DKGRAY */
        0xff,0x00,0x00, /* RED */
	153,153,153,	/* GRAY */
	229,229,229, /* LTGRAY */
	195,195,195, /* LTGRAY2 */
	238,153,195,	/* PINK */
	25,110,110,	/* DKCYAN */
	195,153,110,	/* BROWN1 */
	238,195,153,	/* BROWN2 */
	238,238,238, /* WHITE */
	238,110,195,	/* DKPINK */

        0x00,0xff,0x00, /* GREEN */
        0xff,0xff,0x00, /* YELLOW */
        0xff,0x00,0xff, /* MAGENTA */

	0xe0,0xb0,0x70,	/* BROWN */
	0xd0,0xa0,0x60,	/* BROWN0 */
	0x80,0x60,0x20,	/* BROWN3 */
	0x54,0x40,0x14,	/* BROWN4 */

  0x54,0xa8,0xff, /* LTBLUE */
  0x00,0xa0,0x00, /* DKGREEN */
  0x00,0xe0,0x00, /* GRASSGREEN */

	0xff,0xb6,0x49,	/* DKORANGE */
	0xff,0xb6,0x92,	/* LTORANGE */
};

enum {BLACK,BLUE,CYAN,DKGRAY,RED,GRAY,LTGRAY,LTGRAY2,PINK,DKCYAN,
		BROWN1,BROWN2,WHITE,DKPINK,

		GREEN,YELLOW,MAGENTA,
       BROWN,BROWN0,BROWN3,BROWN4,
			 LTBLUE,DKGREEN,GRASSGREEN,DKORANGE,LTORANGE
            };

static unsigned char colortable[] =
{
	/* characters */
	0,1,2,3,
	0,2,3,4,
	0,3,4,5,
	0,4,5,6,
	0,5,6,7,
	0,6,7,8,
	0,7,8,9,
	0,8,9,10,
	0,9,10,11,
	0,10,11,12,
	0,11,12,13,
	0,12,13,14,
	0,13,14,15,
	0,14,15,1,
	0,15,1,2,
	0,1,2,3,
	0,1,2,3,
	0,2,3,4,
	0,3,4,5,
	0,4,5,6,
	0,5,6,7,
	0,6,7,8,
	0,7,8,9,
	0,8,9,10,
	0,9,10,11,
	0,10,11,12,
	0,11,12,13,
	0,12,13,14,
	0,13,14,15,
	0,14,15,1,
	0,15,1,2,
	0,1,2,3,

	/* background tiles */
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
	0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,1,
	0,3,4,5,6,7,8,9,10,11,12,13,14,15,1,2,
	0,4,5,6,7,8,9,10,11,12,13,14,15,1,2,3,
	0,5,6,7,8,9,10,11,12,13,14,15,1,2,3,4,
	0,6,7,8,9,10,11,12,13,14,15,1,2,3,4,5,
	0,7,8,9,10,11,12,13,14,15,1,2,3,4,5,6,
	0,8,9,10,11,12,13,14,15,1,2,3,4,5,6,7,
	0,9,10,11,12,13,14,15,1,2,3,4,5,6,7,8,
	0,10,11,12,13,14,15,1,2,3,4,5,6,7,8,9,
	0,11,12,13,14,15,1,2,3,4,5,6,7,8,9,10,
	0,12,13,14,15,1,2,3,4,5,6,7,8,9,10,11,
	0,13,14,15,1,2,3,4,5,6,7,8,9,10,11,12,
	0,14,15,1,2,3,4,5,6,7,8,9,10,11,12,13,
	0,15,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
	0,6,7,8,9,10,11,12,13,14,15,1,2,3,4,5,
	0,7,8,9,10,11,12,13,14,15,1,2,3,4,5,6,
	0,8,9,10,11,12,13,14,15,1,2,3,4,5,6,7,
	0,9,10,11,12,13,14,15,1,2,3,4,5,6,7,8,
	0,10,11,12,13,14,15,1,2,3,4,5,6,7,8,9,
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
	0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,1,
	0,3,4,5,6,7,8,9,10,11,12,13,14,15,1,2,
	0,4,5,6,7,8,9,10,11,12,13,14,15,1,2,3,
	0,5,6,7,8,9,10,11,12,13,14,15,1,2,3,4,
	0,11,12,13,14,15,1,2,3,4,5,6,7,8,9,10,
	0,12,13,14,15,1,2,3,4,5,6,7,8,9,10,11,
	0,13,14,15,1,2,3,4,5,6,7,8,9,10,11,12,
	0,14,15,1,2,3,4,5,6,7,8,9,10,11,12,13,
	0,15,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,

	/* sprites */
	1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0,
	2,3,4,5,6,7,8,9,10,11,12,13,14,15,1,0,
	3,4,5,6,7,8,9,10,11,12,13,14,15,1,2,0,
	4,5,6,7,8,9,10,11,12,13,14,15,1,2,3,0,
	5,6,7,8,9,10,11,12,13,14,15,1,2,3,4,0,
	6,7,8,9,10,11,12,13,14,15,1,2,3,4,5,0,
	7,8,9,10,11,12,13,14,15,1,2,3,4,5,6,0,
	8,9,10,11,12,13,14,15,1,2,3,4,5,6,7,0,
	9,10,11,12,13,14,15,1,2,3,4,5,6,7,8,0,
	10,11,12,13,14,15,1,2,3,4,5,6,7,8,9,0,
	11,12,13,14,15,1,2,3,4,5,6,7,8,9,10,0,
	12,13,14,15,1,2,3,4,5,6,7,8,9,10,11,0,
	13,14,15,1,2,3,4,5,6,7,8,9,10,11,12,0,
	14,15,1,2,3,4,5,6,7,8,9,10,11,12,13,0,
	15,1,2,3,4,5,6,7,8,9,10,11,12,13,14,0,
	1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz (?) */
			0,
			readmem,writemem,0,0,
			c1942_interrupt,2
		},
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz ? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,0,0,
			c1942_sh_interrupt,1
		}
	},
	60,
	c1942_init_machine,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	sizeof(palette)/3,sizeof(colortable),
	0,

	0,
	c1942_vh_start,
	c1942_vh_stop,
	c1942_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	c1942_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( c1942_rom )
	ROM_REGION(0x1c000)	/* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "1-n3.bin", 0x0000, 0x4000 )
	ROM_LOAD( "1-n4.bin", 0x4000, 0x4000 )
	ROM_LOAD( "1-n5.bin", 0x10000, 0x4000 )
	ROM_LOAD( "1-n6.bin", 0x14000, 0x2000 )
	ROM_LOAD( "1-n7.bin", 0x18000, 0x4000 )

	ROM_REGION(0x1e000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "1-f2.bin", 0x00000, 0x2000 )	/* characters */
	ROM_LOAD( "2-a5.bin", 0x02000, 0x2000 )	/* tiles */
	ROM_LOAD( "2-a3.bin", 0x04000, 0x2000 )	/* tiles */
	ROM_LOAD( "2-a1.bin", 0x06000, 0x2000 )	/* tiles */
	ROM_LOAD( "2-a6.bin", 0x08000, 0x2000 )	/* tiles */
	ROM_LOAD( "2-a4.bin", 0x0a000, 0x2000 )	/* tiles */
	ROM_LOAD( "2-a2.bin", 0x0c000, 0x2000 )	/* tiles */
	ROM_LOAD( "2-l1.bin", 0x0e000, 0x4000 )	/* sprites */
	ROM_LOAD( "2-n1.bin", 0x12000, 0x4000 )	/* sprites */
	ROM_LOAD( "2-l2.bin", 0x16000, 0x4000 )	/* sprites */
	ROM_LOAD( "2-n2.bin", 0x1a000, 0x4000 )	/* sprites */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "1-c11.bin", 0x0000, 0x4000 )
ROM_END



struct GameDriver c1942_driver =
{
	"1942",
	&machine_driver,

	c1942_rom,
	0, 0,
	0,

	input_ports, dsw,

	0, palette, colortable,
	{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,	/* numbers */
		0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,	/* letters */
		0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23 },
	19, 0,
	8*13, 8*16, 20,

	0, 0
};
