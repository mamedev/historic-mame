/***************************************************************************
Cops 01
Nichibutsu - 1985

calb@gsyc.inf.uc3m.es

TODO:
----
- Fix colors. (it isn't using the lookup proms)
- Fix sprites bank. (ahhhghg!)
- Fix sprites clip.
- Add hi-score support.

MEMORY MAP
----------
0000-BFFF  ROM
C000-C7FF  RAM
D000-DFFF  VRAM (Background)
E000-E0FF  VRAM (Sprites)
F000-F3FF  VRAM (Foreground)

AUDIO MEMORY MAP (Advise: Real audio chip used, UNKNOWN)
----------------
0000-7FFF  ROM
8000-8000  UNKNOWN
C000-C700  RAM

Notes:
-----
- Sound chip unknow. (probably 3 ay8910)

***************************************************************************/
#include "driver.h"
#include "sndhrdw/psgintf.h"
#include "vidhrdw/generic.h"

void cop01_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void cop01_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void cop01_videoram_w(int offset,int data);
int cop01_vh_start(void);
void cop01_vh_stop(void);

extern unsigned char *cop01_vram;
extern int cop01_vram_size;
extern unsigned char cop01_scrollx[];
extern unsigned char *cop01_spram;
extern unsigned char cop01_gfxbank;



void port_w40(int offset,int data) { cop01_gfxbank = data; }
void port_w41(int offset,int data) { cop01_scrollx[0] = data; }
void port_w42(int offset,int data) { cop01_scrollx[1] = data; }
void port_w43(int offset,int data) { if( errorlog ) fprintf( errorlog, "[43] <- %x\n", data );}
void port_w44(int offset,int data) { if( errorlog ) fprintf( errorlog, "[44] <- %x\n", data );}
void port_w45(int offset,int data) { if( errorlog ) fprintf( errorlog, "[45] <- %x\n", data );}


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ 0xd000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xe0ff, MRA_RAM },
	{ -1 }
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ 0xd000, 0xdfff, cop01_videoram_w, &cop01_vram, &cop01_vram_size },
	{ 0xe000, 0xe0ff, MWA_RAM, &cop01_spram },
	{ 0xf000, 0xf3ff, MWA_RAM, &videoram, &videoram_size },
	{ -1 }
};

static struct IOReadPort readport[] =
{
	{ 0x00, 0x00, input_port_0_r },
	{ 0x01, 0x01, input_port_1_r },
	{ 0x02, 0x02, input_port_2_r },
	{ 0x03, 0x03, input_port_3_r },
	{ 0x04, 0x04, input_port_4_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x40, 0x40, port_w40 },    /* Video RAM bank register  */
	{ 0x41, 0x41, port_w41 },    /* Scroll Low  */
	{ 0x42, 0x42, port_w42 },    /* Scroll High */
	{ 0x43, 0x43, port_w43 },    /* Unknown */
	{ 0x44, 0x44, soundlatch_w}, /* Sound Command  */
	{ 0x45, 0x45, port_w45 },    /* Unknown */
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sndreadmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x8000, MRA_RAM },
	{ 0xc000, 0xc7ff, MRA_RAM },
	{ -1 }
};

static struct MemoryWriteAddress sndwritemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xc000, 0xc7ff, MWA_RAM },
	{ -1 }
};


static struct IOReadPort sndreadport[] =
{
	{ 0x06, 0x06, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort sndwriteport[] =
{
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
	{ 0x02, 0x02, AY8910_control_port_1_w },
	{ 0x03, 0x03, AY8910_write_port_1_w },
	{ 0x04, 0x04, AY8910_control_port_2_w },
	{ 0x05, 0x05, AY8910_write_port_2_w },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START  /* TEST, COIN, START */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )  /* Test Mode */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, "Coin 1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x02, "2 Coin/1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x00, "Free play" )
	PORT_DIPNAME( 0x0c, 0x0c, "Coin 2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x08, "2 Coin/3 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x00, "3 Coin/1 Credit" )
	PORT_DIPNAME( 0x10, 0x10, "Unused", IP_KEY_NONE )
	PORT_DIPNAME( 0x20, 0x20, "Demo Sound", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x00, "1 Coin/3 Credits" )
	PORT_DIPNAME( 0x40, 0x40, "Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Table/Upright", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "1" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x10, 0x10, "1st Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "20K" )
	PORT_DIPSETTING(    0x00, "30K" )
	PORT_DIPNAME( 0x60, 0x60, "Next Bonus", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "30K" )
	PORT_DIPSETTING(    0x40, "100K" )
	PORT_DIPSETTING(    0x20, "50K" )
	PORT_DIPSETTING(    0x00, "150K" )
	PORT_DIPNAME( 0x80, 0x80, "Unused", IP_KEY_NONE )
INPUT_PORTS_END



static struct GfxLayout tilelayout =
{
	8,8,    /* 8*8 characters */
	256,    /* 256 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24},
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8    /* every char takes 32 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 chars */
	128,	/* 128 characters */
	4,		/* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* plane offset */
	{ 4, 0, 4+0x2000*8, 0+0x2000*8, 12, 8, 12+0x2000*8, 8+0x2000*8,
		20, 16, 20+0x2000*8, 16+0x2000*8, 28, 24, 28+0x2000*8, 24+0x2000*8 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
		8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	64*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &tilelayout,     0, 16 },
	{ 1, 0x02000, &tilelayout,   192, 16 },
	{ 1, 0x04000, &tilelayout,   192, 16 },
	{ 1, 0x06000, &tilelayout,   192, 16 },
	{ 1, 0x08000, &tilelayout,   192, 16 },
	{ 1, 0x0A000, &spritelayout, 128, 16 },
	{ 1, 0x0E000, &spritelayout, 128, 16 },
	{ 1, 0x12000, &spritelayout, 128, 16 },
	{ 1, 0x16000, &spritelayout, 128, 16 },
	{ -1 }
};



static struct AY8910interface ay8910_interface =
{
	3,	/* 3 chips */
	1500000,	/* 1.5 MHz?????? */
	{ 255, 255, 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



static struct MachineDriver cop01_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3500000,        /* 3.5 Mhz (?) */
			0,
			readmem,writemem,readport,writeport,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,        /* 3.0 Mhz (?) */
			3,
			sndreadmem,sndwritemem,sndreadport,sndwriteport,
			interrupt,1
		},
	},
	60,DEFAULT_60HZ_VBLANK_DURATION,
	10,     /* 10 CPU slices per frame - enough for the sound CPU to read all commands */
	0, /* init machine */

	/* video hardware */
	32*8, 32*8, { 2, 32*8-3, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, /* number of colors */
	256, /* color table length */
	cop01_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	cop01_vh_start,
	cop01_vh_stop,
	cop01_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

/***************************************************************************

  Game driver(s)

***************************************************************************/


ROM_START( cop01_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "cop01.2b", 0x0000, 0x4000, 0x0dac94a2 )
	ROM_LOAD( "cop02.4b", 0x4000, 0x4000, 0xbc8ff33b )
	ROM_LOAD( "cop03.5b", 0x8000, 0x4000, 0x852b8129 )

	ROM_REGION(0x1A000)
	/* Tiles */
	ROM_LOAD( "cop14.15g", 0x0000, 0x2000, 0x801e53d0 )

	/* Background */
	ROM_LOAD( "cop04.15c", 0x2000, 0x4000, 0xed4b27db )
	ROM_LOAD( "cop05.16c", 0x6000, 0x4000, 0x2a25667d )

	/* Sprites */
	/*
	ROM_LOAD( "cop10.3e", 0x0A000, 0x2000, 0x619b703b )
	ROM_LOAD( "cop06.3g", 0x0C000, 0x2000, 0xd9c15c53 )

	ROM_LOAD( "cop13.8e", 0x0E000, 0x2000, 0x8fb89324 )
	ROM_LOAD( "cop09.8g", 0x10000, 0x2000, 0x02e2fd04 )

	ROM_LOAD( "cop11.5e", 0x12000, 0x2000, 0x4a5c0070 )
	ROM_LOAD( "cop07.5g", 0x14000, 0x2000, 0x7133594d )

	ROM_LOAD( "cop12.6e", 0x16000, 0x2000, 0x767747a7 )
	ROM_LOAD( "cop08.6g", 0x18000, 0x2000, 0x64822cb0 )
	*/

	ROM_LOAD( "cop10.3e", 0x0A000, 0x2000, 0x619b703b )
	ROM_LOAD( "cop06.3g", 0x0C000, 0x2000, 0xd9c15c53 )

	ROM_LOAD( "cop13.8e", 0x0E000, 0x2000, 0x8fb89324 )
	ROM_LOAD( "cop09.8g", 0x10000, 0x2000, 0x02e2fd04 )

	ROM_LOAD( "cop11.5e", 0x12000, 0x2000, 0x4a5c0070 )
	ROM_LOAD( "cop07.5g", 0x14000, 0x2000, 0x7133594d )

	ROM_LOAD( "cop12.6e", 0x16000, 0x2000, 0x767747a7 )
	ROM_LOAD( "cop08.6g", 0x18000, 0x2000, 0x64822cb0 )

	ROM_REGION(0x100*5)     /* PROM */
	ROM_LOAD( "copproma.13d", 0x000, 0x100, 0x26880006 )
	ROM_LOAD( "coppromb.14d", 0x100, 0x100, 0xe3bb0201 )
	ROM_LOAD( "coppromc.15d", 0x200, 0x100, 0xab010301 )
	ROM_LOAD( "coppromd.19d", 0x300, 0x100, 0x72770609 )
	ROM_LOAD( "copprome.2e",  0x400, 0x100, 0xb50e0c08 )

	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "cop15.17b", 0x0000, 0x4000, 0xa7e82b5a )
	ROM_LOAD( "cop16.18b", 0x4000, 0x4000, 0x9c039d7b )
ROM_END



struct GameDriver cop01_driver =
{
	__FILE__,
	0,
	"cop01",
	"COP 01",
	"1985",
	"Nichibutsu",
	"Carlos A. Lozano\n",
	0,
	&cop01_machine_driver,

	cop01_rom,
	0, 0,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};
