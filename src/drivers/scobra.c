/***************************************************************************

Super Cobra memory map (preliminary)

0000-5fff ROM (Lost Tomb: 0000-6fff)
8000-87ff RAM
8800-8bff Video RAM
9000-90ff Object RAM
  9000-903f  screen attributes
  9040-905f  sprites
  9060-907f  bullets
  9080-90ff  unused?

read:
b000      Watchdog Reset
9800      IN0
9801      IN1
9802      IN2

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
 * bit 1 : nr of lives  0 = 3  1 = 5
 * bit 0 : allow continue 0 = NO  1 = YES
*
 * IN2 (all bits are inverted)
 * bit 7 : protection check?
 * bit 6 : DOWN player 1
 * bit 5 : protection check?
 * bit 4 : UP player 1
 * bit 3 : COCKTAIL or UPRIGHT cabinet (0 = UPRIGHT)
 * bit 2 :\ coins per play
 * bit 1 :/ (00 = 99 credits!)
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
a000      To AY-3-8910 port A (commands for the audio CPU)
a001      bit 3 = trigger interrupt on audio CPU
a002      protection check control?

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
	{ 0x8000, 0x8bff, MRA_RAM },	/* RAM and Video RAM */
	{ 0x0000, 0x6fff, MRA_ROM },
	{ 0xb000, 0xb000, MRA_NOP },
	{ 0x9800, 0x9800, input_port_0_r },	/* IN0 */
	{ 0x9801, 0x9801, input_port_1_r },	/* IN1 */
	{ 0x9802, 0x9802, scramble_IN2_r },	/* IN2 */
	{ 0x9000, 0x907f, MRA_RAM },	/* screen attributes, sprites, bullets */
	{ 0xa002, 0xa002, scramble_protection_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8bff, videoram_w, &videoram },
	{ 0x9000, 0x903f, scramble_attributes_w, &scramble_attributesram },
	{ 0x9040, 0x905f, MWA_RAM, &spriteram },
	{ 0x9060, 0x907f, MWA_RAM, &scramble_bulletsram },
	{ 0xa801, 0xa801, interrupt_enable_w },
	{ 0xa804, 0xa804, scramble_stars_w },
	{ 0xa000, 0xa000, scramble_soundcommand_w },
	{ 0x0000, 0x6fff, MWA_ROM },
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
		{ 0, OSD_KEY_ALT, OSD_KEY_3, OSD_KEY_CONTROL, OSD_KEY_RIGHT, OSD_KEY_LEFT, 0, 0 },
		{ 0, OSD_JOY_FIRE2, 0, OSD_JOY_FIRE1, OSD_JOY_RIGHT, OSD_JOY_LEFT, 0, 0 }
	},
	{	/* IN1 */
		0xfd,
		{ 0, 0, 0, 0, 0, 0, OSD_KEY_2, OSD_KEY_1 },
		{ 0, 0, 0, 0, 0, 0, 0, 0 }
	},
	{	/* IN2 */
		0xf3,
		{ 0, 0, 0, 0, OSD_KEY_UP, 0, OSD_KEY_DOWN, 0 },
		{ 0, 0, 0, 0, OSD_JOY_UP, 0, OSD_JOY_DOWN, 0 }
	},
	{ -1 }	/* end of table */
};



static struct DSW dsw[] =
{
	{ 1, 0x02, "LIVES", { "3", "5" } },
	{ 1, 0x01, "ALLOW CONTINUE", { "NO", "YES" } },
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



static unsigned char color_prom[] =
{
	/* palette */
	0x00,0x17,0xC7,0xF6,0x00,0x17,0xC0,0x3F,0x00,0x07,0xC0,0x3F,0x00,0xC0,0xC4,0x07,
	0x00,0xC7,0x31,0x17,0x00,0x31,0xC7,0x3F,0x00,0xF6,0x07,0xF0,0x00,0x3F,0x07,0xC4
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



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( scobra_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "scobra2c.bin", 0x0000, 0x1000 )
	ROM_LOAD( "scobra2e.bin", 0x1000, 0x1000 )
	ROM_LOAD( "scobra2f.bin", 0x2000, 0x1000 )
	ROM_LOAD( "scobra2h.bin", 0x3000, 0x1000 )
	ROM_LOAD( "scobra2j.bin", 0x4000, 0x1000 )
	ROM_LOAD( "scobra2l.bin", 0x5000, 0x1000 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "scobra5f.bin", 0x0000, 0x0800 )
	ROM_LOAD( "scobra5h.bin", 0x0800, 0x0800 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "scobra5c.bin", 0x0000, 0x0800 )
	ROM_LOAD( "scobra5d.bin", 0x0800, 0x0800 )
	ROM_LOAD( "scobra5e.bin", 0x1000, 0x0800 )
ROM_END

ROM_START( scobrak_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2c", 0x0000, 0x1000 )
	ROM_LOAD( "2e", 0x1000, 0x1000 )
	ROM_LOAD( "2f", 0x2000, 0x1000 )
	ROM_LOAD( "2h", 0x3000, 0x1000 )
	ROM_LOAD( "2j", 0x4000, 0x1000 )
	ROM_LOAD( "2l", 0x5000, 0x1000 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5f", 0x0000, 0x0800 )
	ROM_LOAD( "5h", 0x0800, 0x0800 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "5c",      0x0000, 0x0800 )
	ROM_LOAD( "5d",      0x0800, 0x0800 )
	ROM_LOAD( "5e",      0x1000, 0x0800 )
ROM_END

ROM_START( scobrab_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "vid_2c.bin",   0x0000, 0x0800 )
	ROM_LOAD( "vid_2e.bin",   0x0800, 0x0800 )
	ROM_LOAD( "vid_2f.bin",   0x1000, 0x0800 )
	ROM_LOAD( "vid_2h.bin",   0x1800, 0x0800 )
	ROM_LOAD( "vid_2j_l.bin", 0x2000, 0x0800 )
	ROM_LOAD( "vid_2l_l.bin", 0x2800, 0x0800 )
	ROM_LOAD( "vid_2m_l.bin", 0x3000, 0x0800 )
	ROM_LOAD( "vid_2p_l.bin", 0x3800, 0x0800 )
	ROM_LOAD( "vid_2j_u.bin", 0x4000, 0x0800 )
	ROM_LOAD( "vid_2l_u.bin", 0x4800, 0x0800 )
	ROM_LOAD( "vid_2m_u.bin", 0x5000, 0x0800 )
	ROM_LOAD( "vid_2p_u.bin", 0x5800, 0x0800 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "vid_5f.bin",   0x0000, 0x0800 )
	ROM_LOAD( "vid_5h.bin",   0x0800, 0x0800 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "snd_5c.bin",   0x0000, 0x0800 )
	ROM_LOAD( "snd_5d.bin",   0x0800, 0x0800 )
	ROM_LOAD( "snd_5e.bin",   0x1000, 0x0800 )
ROM_END

ROM_START( losttomb_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2c",      0x0000, 0x1000 )
	ROM_LOAD( "2e",      0x1000, 0x1000 )
	ROM_LOAD( "2f",      0x2000, 0x1000 )
	ROM_LOAD( "2h-easy", 0x3000, 0x1000 )
	ROM_LOAD( "2j",      0x4000, 0x1000 )
	ROM_LOAD( "2l",      0x5000, 0x1000 )
	ROM_LOAD( "2m",      0x6000, 0x1000 )

	ROM_REGION(0x1000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5f",      0x0000, 0x0800 )
	ROM_LOAD( "5h",      0x0800, 0x0800 )

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "5c",      0x0000, 0x0800 )
	ROM_LOAD( "5d",      0x0800, 0x0800 )
ROM_END



struct GameDriver scobra_driver =
{
	"scobra",
	&machine_driver,

	scobra_rom,
	0, 0,

	input_ports, dsw,

	color_prom, 0, 0,
	0, 17,
	0x00, 0x01,
	8*13, 8*16, 0x04,

	0, 0
};

struct GameDriver scobrak_driver =
{
	"scobrak",
	&machine_driver,

	scobrak_rom,
	0, 0,

	input_ports, dsw,

	color_prom, 0, 0,
	0, 17,
	0x00, 0x01,
	8*13, 8*16, 0x04,

	0, 0
};

struct GameDriver scobrab_driver =
{
	"scobrab",
	&machine_driver,

	scobrab_rom,
	0, 0,

	input_ports, dsw,

	color_prom, 0, 0,
	0, 17,
	0x00, 0x01,
	8*13, 8*16, 0x04,

	0, 0
};

struct GameDriver losttomb_driver =
{
	"losttomb",
	&machine_driver,

	losttomb_rom,
	0, 0,

	input_ports, dsw,

	color_prom, 0, 0,
	0, 17,
	0x00, 0x01,
	8*13, 8*16, 0x04,

	0, 0
};
