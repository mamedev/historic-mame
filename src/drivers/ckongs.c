/***************************************************************************

Crazy Kong running on Scamble board memory map (preliminary)

MAIN BOARD:
0000-5fff ROM
6000-6bff RAM
9000-93ff Video RAM
9800-98ff Object RAM
  9800-983f  screen attributes
  9840-985f  sprites
  9860-987f  bullets
  9880-98ff  unused?

read:
b000      Watchdog Reset
7000      IN0
7001      IN1
7002      IN2

*
 * IN0 (all bits are inverted)
 * bit 7 : COIN 1
 * bit 6 : COIN 2
 * bit 5 : LEFT player 1
 * bit 4 : RIGHT player 1
 * bit 3 : SHOOT 1 player 1
 * bit 2 : CREDIT (SERVICE)
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
 * bit 0 :/ 00 = 3  01 = 4  10 = 5  11 = 256
*
 * IN2 (all bits are inverted)
 * bit 7 : protection check?
 * bit 6 : DOWN player 1
 * bit 5 : protection check?
 * bit 4 : UP player 1
 * bit 3 : COCKTAIL or UPRIGHT cabinet (0 = UPRIGHT)
 * bit 2 :\ coins per play
 * bit 1 :/
 * bit 0 : DOWN player 2 (TABLE only)
 *

write:
a801      interrupt enable
a802      coin counter
a803      ? (POUT1)
a804      stars on
a805      ? (POUT2)
a806      screen vertical flip
a807      screen horizontal flip
7800      To AY-3-8910 port A (commands for the audio CPU)
7801      bit 3 = interrupt trigger on audio CPU  bit 4 = AMPM (?)


SOUND BOARD:
0000-1fff ROM
8000-83ff RAM

I/0 ports:
read:
20      8910 #2  read
80      8910 #1  read

write
10      8910 #2  control
20      8910 #2  write
40      8910 #1  control
80      8910 #1  write

interrupts:
interrupt mode 1 triggered by the main CPU

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



extern unsigned char *scramble_attributesram;
extern unsigned char *scramble_bulletsram;
extern int scramble_bulletsram_size;
extern void scramble_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void scramble_attributes_w(int offset,int data);
extern void scramble_stars_w(int offset,int data);
extern int scramble_vh_start(void);
extern void scramble_vh_screenrefresh(struct osd_bitmap *bitmap);

extern int scramble_sh_interrupt(void);
extern int scramble_sh_start(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x6000, 0x6bff, MRA_RAM },	/* RAM */
	{ 0x9000, 0x93ff, MRA_RAM },	/* Video RAM */
	{ 0x9800, 0x987f, MRA_RAM },	/* screen attributes, sprites, bullets */
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0xb000, 0xb000, MRA_NOP },
	{ 0x7000, 0x7000, input_port_0_r },	/* IN0 */
	{ 0x7001, 0x7001, input_port_1_r },	/* IN1 */
	{ 0x7002, 0x7002, input_port_2_r },	/* IN2 */
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x6000, 0x6bff, MWA_RAM },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9800, 0x983f, scramble_attributes_w, &scramble_attributesram },
	{ 0x9840, 0x985f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9860, 0x987f, MWA_RAM, &scramble_bulletsram, &scramble_bulletsram_size },
	{ 0xa801, 0xa801, interrupt_enable_w },
	{ 0xa804, 0xa804, scramble_stars_w },
	{ 0xa802, 0xa802, MWA_NOP },
	{ 0xa806, 0xa807, MWA_NOP },
	{ 0x7800, 0x7800, sound_command_w },
	{ 0x0000, 0x5fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0x0000, 0x1fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct IOReadPort sound_readport[] =
{
	{ 0x80, 0x80, AY8910_read_port_0_r },
	{ 0x20, 0x20, AY8910_read_port_1_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x40, 0x40, AY8910_control_port_0_w },
	{ 0x80, 0x80, AY8910_write_port_0_w },
	{ 0x10, 0x10, AY8910_control_port_1_w },
	{ 0x20, 0x20, AY8910_write_port_1_w },
	{ -1 }	/* end of table */
};



static struct InputPort input_ports[] =
{
	{	/* IN0 */
		0xff,
		{ OSD_KEY_UP, OSD_KEY_ALT, 0, OSD_KEY_CONTROL,
				OSD_KEY_RIGHT, OSD_KEY_LEFT, 0, OSD_KEY_3 },
		{ OSD_JOY_UP, OSD_JOY_FIRE2, 0, OSD_JOY_FIRE1,
				OSD_JOY_RIGHT, OSD_JOY_LEFT, 0, 0 }
	},
	{	/* IN1 */
		0xff,
		{ 0, 0, OSD_KEY_ALT, OSD_KEY_CONTROL,
				OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_2, OSD_KEY_1 },
		{ 0, 0, OSD_JOY_FIRE2, OSD_JOY_FIRE1,
				OSD_JOY_RIGHT, OSD_JOY_LEFT, 0, 0 }
	},
	{	/* IN2 */
		0xff,
		{ OSD_KEY_DOWN, 0, 0, 0, OSD_KEY_UP, 0, OSD_KEY_DOWN, 0 },
		{ OSD_JOY_DOWN, 0, 0, 0, OSD_JOY_UP, 0, OSD_JOY_DOWN, 0 }
	},
	{ -1 }	/* end of table */
};

static struct TrakPort trak_ports[] =
{
        { -1 }
};


static struct KEYSet keys[] =
{
        { 0, 0, "PL1 MOVE UP" },
        { 0, 5, "PL1 MOVE LEFT"  },
        { 0, 4, "PL1 MOVE RIGHT" },
        { 2, 0, "PL1 MOVE DOWN" },
        { 0, 1, "PL1 FIRE1" },
        { 0, 3, "PL1 FIRE2" },
        { 2, 4, "PL1 MOVE UP" },
        { 1, 5, "PL1 MOVE LEFT"  },
        { 1, 4, "PL1 MOVE RIGHT" },
        { 2, 6, "PL1 MOVE DOWN" },
        { 1, 2, "PL1 FIRE1" },
        { 1, 3, "PL1 FIRE2" },
        { -1 }
};


static struct DSW dsw[] =
{
	{ 2, 0x04, "LIVES", { "4", "3" }, 1 },
	{ 2, 0x02, "SW3", { "OFF", "ON" } },
	{ -1 }
};



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 512*8*8 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 128*16*16 },	/* the two bitplanes are separated */
	{ 23*8, 22*8, 21*8, 20*8, 19*8, 18*8, 17*8, 16*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	32*8	/* every sprite takes 32 consecutive bytes */
};
/* there's nothing here, this is just a placeholder to let the video hardware */
/* pick the color table */
static struct GfxLayout starslayout =
{
	1,1,
	0,
	1,	/* 1 star = 1 color */
	{ 0 },
	{ 0 },
	{ 0 },
	0
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,     0, 8 },
	{ 1, 0x0000, &spritelayout,   0, 8 },
	{ 0, 0,      &starslayout,   32, 64 },
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* palette */
	0x00,0xFF,0xFF,0xFF,0x00,0x38,0xF8,0x07,0x00,0xC0,0x3F,0x38,0x00,0xF8,0xC7,0x3F,
	0x00,0x07,0x38,0xC0,0x00,0x38,0x3F,0xC7,0x00,0xC7,0xFF,0xF8,0x00,0xC0,0x07,0xFF,
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
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			1789750,	/* 1.78975 Mhz?????? */
			2,	/* memory region #2 */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
			scramble_sh_interrupt,1
		}
	},
	60,
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	32+64,32+64,	/* 32 for the characters, 64 for the stars */
	scramble_vh_convert_color_prom,

	0,
	scramble_vh_start,
	generic_vh_stop,
	scramble_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	scramble_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( ckongs_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "vid_2c.bin", 0x0000, 0x1000 )
	ROM_LOAD( "vid_2e.bin", 0x1000, 0x1000 )
	ROM_LOAD( "vid_2f.bin", 0x2000, 0x1000 )
	ROM_LOAD( "vid_2h.bin", 0x3000, 0x1000 )
	ROM_LOAD( "vid_2j.bin", 0x4000, 0x1000 )
	ROM_LOAD( "vid_2l.bin", 0x5000, 0x1000 )

	ROM_REGION(0x2000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "vid_5f.bin", 0x0000, 0x1000 )
	ROM_LOAD( "vid_5h.bin", 0x1000, 0x1000 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "snd_5c.bin", 0x0000, 0x1000 )
	ROM_LOAD( "snd_5d.bin", 0x1000, 0x1000 )
ROM_END



struct GameDriver ckongs_driver =
{
	"Crazy Kong (Scramble Hardware)",
	"ckongs",
	"NICOLA SALMORIA",
	&machine_driver,

	ckongs_rom,
	0, 0,
	0,

	input_ports, trak_ports, dsw, keys,

	color_prom, 0, 0,
	8*13, 8*16,

	0, 0
};
