/***************************************************************************

Championship Baseball memory map (preliminary)
the hardware is similar to Pengo

0000-5fff ROM
8000-83ff Video RAM
8400-87ff Color RAM
8800-8fff RAM

read:
a000      IN0
a040      IN1
a080      DSW
a0a0      ?
a0c0      COIN

write:
7000      8910 write
7001      8910 control
8ff0-8fff sprites
a000      ?
a060-a06f sprites
a080      command for the second CPU???
a0c0      watchdog reset???


There's a second CPU, but what does it do???

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



void champbas_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
void champbas_vh_screenrefresh(struct osd_bitmap *bitmap);

int champbas_sh_start(void);



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0xa000, 0xa000, input_port_0_r },
	{ 0xa040, 0xa040, input_port_1_r },
	{ 0xa080, 0xa080, input_port_2_r },
/*	{ 0xa0a0, 0xa0a0,  },	???? */
	{ 0xa0c0, 0xa0c0, input_port_3_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x8000, 0x83ff, videoram_w, &videoram, &videoram_size },
	{ 0x8400, 0x87ff, colorram_w, &colorram },
	{ 0x8800, 0x8fef, MWA_RAM },
	{ 0x8ff0, 0x8fff, MWA_RAM, &spriteram, &spriteram_size},
	{ 0x7000, 0x7000, AY8910_write_port_0_w },
	{ 0x7001, 0x7001, AY8910_control_port_0_w },
	{ 0xa000, 0xa000, MWA_NOP },	/* ??? */
	{ 0xa060, 0xa06f, MWA_RAM, &spriteram_2 },
	{ 0xa080, 0xa080, soundlatch_w },	/* actually this is not a sound command */
	{ 0xa0c0, 0xa0c0, MWA_NOP },	/* watchdog reset ??? */
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress readmem2[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x6000, 0x6000, soundlatch_r },
	{ 0xe000, 0xe3ff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem2[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0xe000, 0xe3ff, MWA_RAM },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* DSW */
	PORT_DIPNAME( 0x01, 0x00, "1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off")
	PORT_DIPSETTING(    0x01, "On" )
	PORT_DIPNAME( 0x02, 0x00, "2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off")
	PORT_DIPSETTING(    0x02, "On" )
	PORT_DIPNAME( 0x04, 0x00, "3", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off")
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x00, "4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off")
	PORT_DIPSETTING(    0x08, "On" )
	PORT_DIPNAME( 0x10, 0x00, "5", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off")
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x00, "6", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off")
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x00, "7", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off")
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "8", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off")
	PORT_DIPSETTING(    0x80, "On" )

	PORT_START	/* COIN */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 8*8+0, 8*8+1, 8*8+2, 8*8+3, 0, 1, 2, 3 },	/* bits are packed in groups of four */
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8	/* every char takes 16 bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	64,	/* 64 sprites */
	2,	/* 2 bits per pixel */
	{ 0, 4 },	/* the two bitplanes for 4 pixels are packed into one byte */
	{ 8*8, 8*8+1, 8*8+2, 8*8+3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 0, 1, 2, 3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8 },
	64*8	/* every sprite takes 64 bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 64 },
	{ 1, 0x1000, &spritelayout, 0, 64 },
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* champbb.pr2: palette */
	0x00,0x07,0x66,0xEF,0xF5,0xF8,0xEA,0x6F,0x86,0x3F,0x00,0xC9,0x38,0x63,0x28,0xF6,
	0x00,0x07,0x66,0xEF,0xF5,0xF8,0xEA,0x6F,0x86,0x3F,0x00,0xC9,0x38,0x63,0xAC,0xF6,
	/* champbb.pr1: lookup table */
	0x00,0x00,0x00,0x00,0x00,0x01,0x05,0x0F,0x00,0x07,0x09,0x01,0x00,0x00,0x07,0x05,
	0x00,0x0A,0x00,0x0A,0x00,0x00,0x0A,0x0A,0x00,0x09,0x00,0x09,0x00,0x00,0x09,0x09,
	0x02,0x00,0x05,0x0F,0x00,0x09,0x0F,0x0A,0x00,0x05,0x0B,0x0D,0x00,0x0F,0x04,0x01,
	0x00,0x09,0x0A,0x0F,0x00,0x07,0x08,0x06,0x00,0x09,0x02,0x0F,0x00,0x0F,0x06,0x05,
	0x00,0x0F,0x09,0x0A,0x00,0x0F,0x04,0x0B,0x00,0x0F,0x01,0x04,0x00,0x0F,0x04,0x01,
	0x00,0x0F,0x01,0x0B,0x00,0x0F,0x04,0x09,0x00,0x0F,0x04,0x0A,0x00,0x0F,0x04,0x06,
	0x00,0x0F,0x07,0x06,0x02,0x02,0x0F,0x0F,0x00,0x00,0x01,0x01,0x00,0x00,0x01,0x01,
	0x00,0x01,0x00,0x01,0x00,0x00,0x0F,0x0F,0x00,0x05,0x00,0x05,0x00,0x00,0x05,0x05,
	0x00,0x00,0x00,0x00,0x00,0x01,0x05,0x0F,0x00,0x07,0x09,0x01,0x00,0x0E,0x07,0x05,
	0x00,0x0B,0x0E,0x09,0x00,0x0F,0x0B,0x07,0x05,0x02,0x0E,0x09,0x05,0x0F,0x09,0x04,
	0x02,0x0E,0x05,0x0F,0x02,0x09,0x05,0x01,0x0E,0x05,0x0B,0x0D,0x0E,0x0F,0x04,0x01,
	0x0E,0x09,0x00,0x0F,0x0E,0x07,0x08,0x06,0x0E,0x09,0x02,0x0F,0x0E,0x0F,0x06,0x05,
	0x0E,0x0F,0x09,0x0A,0x0E,0x0F,0x04,0x0B,0x0E,0x0F,0x01,0x04,0x0E,0x0F,0x04,0x01,
	0x0E,0x0F,0x01,0x0B,0x0E,0x0F,0x04,0x09,0x0E,0x0F,0x04,0x0A,0x0E,0x0F,0x04,0x06,
	0x0E,0x0F,0x07,0x06,0x02,0x02,0x0F,0x0F,0x0E,0x0E,0x01,0x01,0x00,0x00,0x01,0x01,
	0x00,0x01,0x00,0x01,0x00,0x00,0x0F,0x0F,0x00,0x05,0x00,0x05,0x00,0x00,0x05,0x05
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz (?) */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz ? */
			2,	/* memory region #2 */
			readmem2,writemem2,0,0,
			ignore_interrupt,1
		}
	},
	60,
	100,	/* 100 CPU slices per frame - an high value to ensure proper */
			/* synchronization of the CPUs */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32,64*4,
	champbas_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	generic_vh_start,
	generic_vh_stop,
	champbas_vh_screenrefresh,

	/* sound hardware */
	0,
	0,
	champbas_sh_start,
	AY8910_sh_stop,
	AY8910_sh_update
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( champbas_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "champbb.1",  0x0000, 0x2000, 0x052682ae )
	ROM_LOAD( "champbb.2",  0x2000, 0x2000, 0x3a7cece2 )
	ROM_LOAD( "champbb.3",  0x4000, 0x2000, 0xd19d277d )

	ROM_REGION(0x4000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "champbb.4",  0x0000, 0x2000, 0xf7e29c04 )
	ROM_LOAD( "champbb.5",  0x2000, 0x2000, 0xf55fadd5 )

	ROM_REGION(0x10000)	/* 64k for the second CPU - what does it do??? */
	ROM_LOAD( "champbb.6",  0x0000, 0x2000, 0x167773f3 )
	ROM_LOAD( "champbb.7",  0x2000, 0x2000, 0xf75960a1 )
	ROM_LOAD( "champbb.8",  0x4000, 0x2000, 0x43e7a47b )
ROM_END



struct GameDriver champbas_driver =
{
	"Champion Baseball",
	"champbas",
	"Nicola Salmoria",
	&machine_driver,

	champbas_rom,
	0, 0,
	0,

	0/*TBR*/,input_ports,0/*TBR*/,0/*TBR*/,0/*TBR*/,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};
