/***************************************************************************

Scramble memory map (preliminary)

MAIN BOARD:
0000-3fff ROM
4000-47ff RAM
4800-4bff Video RAM
5000-50ff Object RAM
  5000-503f  screen attributes
  5040-505f  sprites
  5060-507f  bullets
  5080-50ff  unused?

read:
7000      Watchdog Reset
8100      IN0
8101      IN1
8102      IN2

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
6801      interrupt enable
6802      coin counter
6803      ? (POUT1)
6804      stars on
6805      ? (POUT2)
6806      screen vertical flip
6807      screen horizontal flip
8200      To AY-3-8910 port A (commands for the audio CPU)
8201      bit 3 = interrupt trigger on audio CPU  bit 4 = AMPM (?)
8202      protection check control?


SOUND BOARD:
0000-17ff ROM
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



extern int scramble_IN2_r(int offset);
extern int scramble_protection_r(int offset);

extern unsigned char *scramble_attributesram;
extern unsigned char *scramble_bulletsram;
extern void scramble_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
extern void scramble_attributes_w(int offset,int data);
extern void scramble_stars_w(int offset,int data);
extern int scramble_vh_start(void);
extern void scramble_vh_screenrefresh(struct osd_bitmap *bitmap);

extern void scramble_soundcommand_w(int offset,int data);
extern int scramble_sh_read_port1_r(int offset);
extern void scramble_sh_control_port1_w(int offset,int data);
extern void scramble_sh_write_port1_w(int offset,int data);
extern int scramble_sh_read_port2_r(int offset);
extern void scramble_sh_control_port2_w(int offset,int data);
extern void scramble_sh_write_port2_w(int offset,int data);
extern int scramble_sh_interrupt(void);
extern int scramble_sh_start(void);
extern void scramble_sh_stop(void);
extern void scramble_sh_update(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x4000, 0x4bff, MRA_RAM },	/* RAM and Video RAM */
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x7000, 0x7000, MRA_NOP },
	{ 0x8100, 0x8100, input_port_0_r },	/* IN0 */
	{ 0x8101, 0x8101, input_port_1_r },	/* IN1 */
	{ 0x8102, 0x8102, scramble_IN2_r },	/* IN2 */
	{ 0x5000, 0x507f, MRA_RAM },	/* screen attributes, sprites, bullets */
	{ 0x8202, 0x8202, scramble_protection_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x4800, 0x4bff, videoram_w, &videoram },
	{ 0x5000, 0x503f, scramble_attributes_w, &scramble_attributesram },
	{ 0x5040, 0x505f, MWA_RAM, &spriteram },
	{ 0x5060, 0x507f, MWA_RAM, &scramble_bulletsram },
	{ 0x6801, 0x6801, interrupt_enable_w },
	{ 0x6804, 0x6804, scramble_stars_w },
	{ 0x6802, 0x6802, MWA_NOP },
	{ 0x6806, 0x6807, MWA_NOP },
	{ 0x8200, 0x8200, scramble_soundcommand_w },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0x0000, 0x17ff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0x0000, 0x17ff, MWA_ROM },
	{ -1 }	/* end of table */
};



static struct IOReadPort sound_readport[] =
{
	{ 0x80, 0x80, scramble_sh_read_port1_r },
	{ 0x20, 0x20, scramble_sh_read_port2_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x40, 0x40, scramble_sh_control_port1_w },
	{ 0x80, 0x80, scramble_sh_write_port1_w },
	{ 0x10, 0x10, scramble_sh_control_port2_w },
	{ 0x20, 0x20, scramble_sh_write_port2_w },
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
		0xfc,
		{ 0, 0, OSD_KEY_ALT, OSD_KEY_CONTROL,
				OSD_KEY_RIGHT, OSD_KEY_LEFT, OSD_KEY_2, OSD_KEY_1 },
		{ 0, 0, OSD_JOY_FIRE2, OSD_JOY_FIRE1,
				OSD_JOY_RIGHT, OSD_JOY_LEFT, 0, 0 }
	},
	{	/* IN2 */
		0xf1,
		{ OSD_KEY_DOWN, 0, 0, 0, OSD_KEY_UP, 0, OSD_KEY_DOWN, 0 },
		{ OSD_JOY_DOWN, 0, 0, 0, OSD_JOY_UP, 0, OSD_JOY_DOWN, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW scramble_dsw[] =
{
	{ 1, 0x03, "LIVES", { "3", "4", "5", "256" } },
	{ -1 }
};
static struct DSW theend_dsw[] =
{
	{ 1, 0x03, "LIVES", { "3", "4", "5", "256" } },
	{ -1 }
};

static struct DSW atlantis_dsw[] =
{
	{ 1, 0x02, "LIVES", { "5", "3" }, 1 },
	{ 1, 0x01, "SW1", { "OFF", "ON" } },
	{ 2, 0x02, "SW3", { "OFF", "ON" } },
	{ 2, 0x04, "SW4", { "OFF", "ON" } },
	{ 2, 0x08, "SW5", { "OFF", "ON" } },
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



static unsigned char scramble_color_prom[] =
{
	/* palette */
	0x00,0x17,0xC7,0xF6,0x00,0x17,0xC0,0x3F,0x00,0x07,0xC0,0x3F,0x00,0xC0,0xC4,0x07,
	0x00,0xC7,0x31,0x17,0x00,0x31,0xC7,0x3F,0x00,0xF6,0x07,0xF0,0x00,0x3F,0x07,0xC4
};



static unsigned char froggers_color_prom[] =
{
	/* palette */
	0x00,0xF6,0x79,0x4F,0x00,0xC0,0x3F,0x17,0x00,0x87,0xF8,0x7F,0x00,0xC1,0x7F,0x38,
	0x00,0x7F,0xCF,0xF9,0x00,0x57,0xB7,0xC3,0x00,0xFF,0x7F,0x87,0x00,0x79,0x4F,0xFF
};



static struct MachineDriver scramble_machine_driver =
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
			CPU_Z80,
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
	scramble_sh_stop,
	scramble_sh_update
};



static struct MachineDriver scramble_nosound_machine_driver =
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
	0,
	0,
	0
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( scramble_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2d", 0x0000, 0x0800 )
	ROM_LOAD( "2e", 0x0800, 0x0800 )
	ROM_LOAD( "2f", 0x1000, 0x0800 )
	ROM_LOAD( "2h", 0x1800, 0x0800 )
	ROM_LOAD( "2j", 0x2000, 0x0800 )
	ROM_LOAD( "2l", 0x2800, 0x0800 )
	ROM_LOAD( "2m", 0x3000, 0x0800 )
	ROM_LOAD( "2p", 0x3800, 0x0800 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5f", 0x0000, 0x0800 )
	ROM_LOAD( "5h", 0x0800, 0x0800 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "5c", 0x0000, 0x0800 )
	ROM_LOAD( "5d", 0x0800, 0x0800 )
	ROM_LOAD( "5e", 0x1000, 0x0800 )
ROM_END

ROM_START( atlantis_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2c", 0x0000, 0x0800 )
	ROM_LOAD( "2e", 0x0800, 0x0800 )
	ROM_LOAD( "2f", 0x1000, 0x0800 )
	ROM_LOAD( "2h", 0x1800, 0x0800 )
	ROM_LOAD( "2j", 0x2000, 0x0800 )
	ROM_LOAD( "2l", 0x2800, 0x0800 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5f", 0x0000, 0x0800 )
	ROM_LOAD( "5h", 0x0800, 0x0800 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "5c", 0x0000, 0x0800 )
	ROM_LOAD( "5d", 0x0800, 0x0800 )
	ROM_LOAD( "5e", 0x1000, 0x0800 )
ROM_END

ROM_START( theend_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "IC13", 0x0000, 0x0800 )
	ROM_LOAD( "IC14", 0x0800, 0x0800 )
	ROM_LOAD( "IC15", 0x1000, 0x0800 )
	ROM_LOAD( "IC16", 0x1800, 0x0800 )
	ROM_LOAD( "IC17", 0x2000, 0x0800 )
	ROM_LOAD( "IC18", 0x2800, 0x0800 )
	ROM_LOAD( "IC56", 0x3000, 0x0800 )
	ROM_LOAD( "IC55", 0x3800, 0x0800 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "IC30", 0x0000, 0x0800 )
	ROM_LOAD( "IC31", 0x0800, 0x0800 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "IC56", 0x0000, 0x0800 )
	ROM_LOAD( "IC55", 0x0800, 0x0800 )
ROM_END

ROM_START( froggers_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "vid_d2.bin", 0x0000, 0x0800 )
	ROM_LOAD( "vid_e2.bin", 0x0800, 0x0800 )
	ROM_LOAD( "vid_f2.bin", 0x1000, 0x0800 )
	ROM_LOAD( "vid_h2.bin", 0x1800, 0x0800 )
	ROM_LOAD( "vid_j2.bin", 0x2000, 0x0800 )
	ROM_LOAD( "vid_l2.bin", 0x2800, 0x0800 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "vid_f5.bin", 0x0000, 0x0800 )
	ROM_LOAD( "vid_h5.bin", 0x0800, 0x0800 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "snd_c5.bin", 0x0000, 0x0800 )
	ROM_LOAD( "snd_d5.bin", 0x0800, 0x0800 )
	ROM_LOAD( "snd_e5.bin", 0x1000, 0x0800 )
ROM_END



struct GameDriver scramble_driver =
{
	"scramble",
	&scramble_machine_driver,

	scramble_rom,
	0, 0,

	input_ports, scramble_dsw,

	scramble_color_prom, 0, 0,
	0, 17,
	0x00, 0x01,
	8*13, 8*16, 0x04,

	0, 0
};

struct GameDriver atlantis_driver =
{
	"atlantis",
	&scramble_machine_driver,

	atlantis_rom,
	0, 0,

	input_ports, atlantis_dsw,

	scramble_color_prom, 0, 0,
	0, 17,
	0x00, 0x01,
	8*13, 8*16, 0x04,

	0, 0
};

struct GameDriver theend_driver =
{
	"theend",
	&scramble_machine_driver,

	theend_rom,
	0, 0,

	input_ports, theend_dsw,

	scramble_color_prom, 0, 0,
	0, 17,
	0x00, 0x01,
	8*13, 8*16, 0x04,

	0, 0
};

struct GameDriver froggers_driver =
{
	"froggers",
	&scramble_nosound_machine_driver,

	froggers_rom,
	0, 0,

	input_ports, scramble_dsw,

	froggers_color_prom, 0, 0,
	0, 17,
	0x07, 0x02,
	8*13, 8*16, 0x01,

	0, 0
};
