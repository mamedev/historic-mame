/***************************************************************************

Frogger memory map (preliminary)

0000-3fff ROM
8000-87ff RAM
a800-abff Video RAM
b000-b0ff Object RAM
  b000-b03f  screen attributes
  b040-b05f  sprites
  b060-b0ff  unused?

read:
8800      Watchdog Reset
e000      IN0
e002      IN1
e004      IN2

*
 * IN0 (all bits are inverted)
 * bit 7 : COIN 1
 * bit 6 : COIN 2
 * bit 5 : LEFT player 1
 * bit 4 : RIGHT player 1
 * bit 3 : SHOOT 1 player 1
 * bit 2 : CREDIT
 * bit 1 : SHOOT 2 player 1
 * bit 0 : UP player 2 (TABLE only)
 *
*
 * IN1 (all bits are inverted)
 * bit 7 : START 1
 * bit 6 : START 2
 * bit 5 : LEFT player 2 (TABLE only)
 * bit 4 : RIGHT player 2 (TABLE only)
 * bit 3 : SHOOT 1 player 2 (TABLE only)
 * bit 2 : SHOOT 2 player 2 (TABLE only)
 * bit 1 :\ nr of lives
 * bit 0 :/ 00 = 3  01 = 5  10 = 7  11 = 256
*
 * IN2 (all bits are inverted)
 * bit 7 : unused
 * bit 6 : DOWN player 1
 * bit 5 : unused
 * bit 4 : UP player 1
 * bit 3 : COCKTAIL or UPRIGHT cabinet (0 = UPRIGHT)
 * bit 2 :\ coins per play
 * bit 1 :/
 * bit 0 : DOWN player 2 (TABLE only)
 *

write:
b808      interrupt enable
b80c      screen horizontal flip
b810      screen vertical flip
b818      coin counter 1
b81c      coin counter 2
d000      To AY-3-8910 port A (commands for the second Z80?)
d002      sound control?

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



extern unsigned char *frogger_attributesram;
extern void frogger_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void frogger_attributes_w(int offset,int data);
extern void frogger_vh_screenrefresh(struct osd_bitmap *bitmap);



static struct MemoryReadAddress readmem[] =
{
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa800, 0xabff, MRA_RAM },	/* video RAM */
	{ 0xb000, 0xb05f, MRA_RAM },	/* screen attributes, sprites */
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x8800, 0x8800, MRA_NOP },
	{ 0xe000, 0xe000, input_port_0_r },	/* IN0 */
	{ 0xe002, 0xe002, input_port_1_r },	/* IN1 */
	{ 0xe004, 0xe004, input_port_2_r },	/* IN2 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa800, 0xabff, videoram_w, &videoram },
	{ 0xb000, 0xb03f, frogger_attributes_w, &frogger_attributesram },
	{ 0xb040, 0xb05f, MWA_RAM, &spriteram },
	{ 0xb808, 0xb808, interrupt_enable_w },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ 0, 0, OSD_KEY_3, 0, OSD_KEY_RIGHT, OSD_KEY_LEFT, 0, 0 },
		{ 0, 0, 0, 0, OSD_JOY_RIGHT, OSD_JOY_LEFT, 0, 0 }
	},
	{	/* IN1 */
		0xfc,
		{ 0, 0, 0, 0, 0, 0, OSD_KEY_2, OSD_KEY_1 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0xf7,
		{ 0, 0, 0, 0, OSD_KEY_UP, 0, OSD_KEY_DOWN, 0 },
		{ 0, 0, 0, 0, OSD_JOY_UP, 0, OSD_JOY_DOWN, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW dsw[] =
{
	{ 1, 0x03, "LIVES", { "3", "5", "7", "256" } },
	{ -1 }
};



static struct GfxLayout charlayout =
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
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 64*16*16 },	/* the two bitplanes are separated */
	{ 23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 16 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ -1 } /* end of array */
};



#define BLACK 0x00
#define LTGREEN 0x3c
#define DKRED 0x17
#define DKBROWN 0x5c
#define DKPINK 0xd7
#define LTBROWN 0x5e
#define PURPLE 0xc4
#define BLUE 0xc0
#define RED 0x07
#define MAGENTA 0xc7
#define GREEN 0x39
#define CYAN 0xf8
#define YELLOW 0x3f
#define WHITE 0xf6

static unsigned char color_prom[] =
{
	/* palette */
	BLACK,BLUE,RED,WHITE,
	BLACK,DKRED,GREEN,CYAN,
	BLACK,LTBROWN,WHITE,DKBROWN,
	BLACK,MAGENTA,GREEN,YELLOW,
	BLACK,LTGREEN,CYAN,DKPINK,
	BLACK,RED,WHITE,GREEN,
	BLACK,PURPLE,BLUE,RED,
	BLACK,RED,YELLOW,PURPLE
};



const struct MachineDriver frogger_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		}
	},
	60,
	input_ports,dsw,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	32,64,
	color_prom,frogger_vh_convert_color_prom,0,0,
	0,17,
	0x00,0x03,
	8*13,8*16,0x06,
	0,
	generic_vh_start,
	generic_vh_stop,
	frogger_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	0,
	0,
	0
};
