/***************************************************************************

Shark Attack memory map (preliminary)

0000-67ff ROM
6800-bfff RAM
co00-dbff Video RAM
dc00-ffff RAM

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *sharkatt_videoram;

void sharkatt_videoram_w(int offset,int data);

int  sharkatt_vh_start(void);
void sharkatt_vh_stop(void);
void sharkatt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x67ff, MRA_ROM },
	{ 0x6800, 0xffff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x67ff, MWA_ROM },
	{ 0x6800, 0xbfff, MWA_RAM },
	{ 0xc000, 0xdbff, sharkatt_videoram_w, &sharkatt_videoram },
	{ 0xdc00, 0xffff, MWA_RAM },

	{ -1 }  /* end of table */
};


static struct IOReadPort readport[] =
{
	{ 0x31, 0x31, input_port_0_r },
	{ 0x41, 0x41, AY8910_read_port_0_r },
	{ 0x43, 0x43, AY8910_read_port_1_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x40, 0x40, AY8910_control_port_0_w },
	{ 0x41, 0x41, AY8910_write_port_0_w },
	{ 0x42, 0x42, AY8910_control_port_1_w },
	{ 0x43, 0x43, AY8910_write_port_1_w },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( sharkatt_input_ports )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )  // Test Mode

INPUT_PORTS_END

static unsigned char palette[] =
{
	0x00,0x00,0x00, /* BLACK */
	0xff,0x20,0x20, /* RED */
	0x20,0xff,0x20, /* GREEN */
	0xff,0xff,0x20, /* YELLOW */
	0xff,0xff,0xff, /* WHITE */
	0x20,0xff,0xff, /* CYAN */
	0xff,0x20,0xff  /* PURPLE */
};

enum { BLACK,RED,GREEN,YELLOW,WHITE,CYAN,PURPLE }; /* V.V */



static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	14318000/8,	/* 1.78975 MHz ????????? */
	{ 255, 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};

static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2000000,        /* 1 Mhz? */
			0,
			readmem,writemem,readport,writeport,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,       /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	0,      /* no gfxdecodeinfo - bitmapped display */
	sizeof(palette)/3, 0,
	0,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	sharkatt_vh_start,
	sharkatt_vh_stop,
	sharkatt_vh_screenrefresh,

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

ROM_START( sharkatt_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "sharkatt.0",   0x0000, 0x0800, 0xf5e073e2 )
	ROM_LOAD( "sharkatt.1",   0x0800, 0x0800, 0x530ecada )
	ROM_LOAD( "sharkatt.2",   0x1000, 0x0800, 0x75669bb8 )
	ROM_LOAD( "sharkatt.3",   0x1800, 0x0800, 0x97c93c3b )
	ROM_LOAD( "sharkatt.4a",  0x2000, 0x0800, 0x3249d0b5 )
	ROM_LOAD( "sharkatt.5",   0x2800, 0x0800, 0xa12e27fa )
	ROM_LOAD( "sharkatt.6",   0x3000, 0x0800, 0xc2f475d0 )
	ROM_LOAD( "sharkatt.7",   0x3800, 0x0800, 0x81742618 )
	ROM_LOAD( "sharkatt.8",   0x4000, 0x0800, 0x751f1f01 )
	ROM_LOAD( "sharkatt.9",   0x4800, 0x0800, 0x4bddae47 )
	ROM_LOAD( "sharkatt.10",  0x5000, 0x0800, 0x1b8365bb )
	ROM_LOAD( "sharkatt.11",  0x5800, 0x0800, 0xcaed67f9 )
	ROM_LOAD( "sharkatt.12a", 0x6000, 0x0800, 0x0efb8901 )

ROM_END

struct GameDriver sharkatt_driver =
{
	__FILE__,
	0,
	"sharkatt",
	"Shark Attack",
	"1980",
	"Pacific Novelty",
	"Victor Trucco",
	0,
	&machine_driver,

	sharkatt_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	sharkatt_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0, 0
};
