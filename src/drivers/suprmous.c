/***************************************************************************

Super Mouse memory map (preliminary)

0000-04fff	ROM

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *galaxian_attributesram;
void galaxian_attributes_w(int offset,int data);

void suprmous_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void suprmous_flipx_w(int offset,int data);
void suprmous_flipy_w(int offset,int data);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x4fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0x9000, 0x9fff, MRA_RAM },
	{ 0xa000, 0xa000, input_port_0_r },
	{ 0xa800, 0xa800, input_port_1_r },
	{ 0xb000, 0xb000, input_port_2_r },
	{ 0xb800, 0xb800, watchdog_reset_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x4fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x9000, 0x93ff, videoram_w, &videoram, &videoram_size },
	{ 0x9400, 0x97ff, colorram_w, &colorram },
	{ 0x9800, 0x983f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x9840, 0x985f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9860, 0x98ff, MWA_RAM }, // Probably unused
	{ 0xb000, 0xb000, interrupt_enable_w },
	{ 0xb006, 0xb006, suprmous_flipx_w, &flip_screen_x },
	{ 0xb007, 0xb007, suprmous_flipy_w, &flip_screen_y },
	{ 0xb800, 0xb800, soundlatch_w },
	{ -1 }	/* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x3800, 0x3bff, MRA_RAM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x281c, 0x281d, MWA_NOP },  // ???
	{ 0x3800, 0x3bff, MWA_RAM },
	{ -1 }  /* end of table */
};

static struct IOReadPort sound_readport[] =
{
	{ 0x8f, 0x8f, AY8910_read_port_1_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, soundlatch_clear_w },
	{ 0x8c, 0x8c, AY8910_control_port_0_w },
	{ 0x8d, 0x8d, AY8910_write_port_0_w },
	{ 0x8e, 0x8e, AY8910_control_port_1_w },
	{ 0x8f, 0x8f, AY8910_write_port_1_w },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( input_ports )
	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x07, 0x01, "Coinage P1/P2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2 Coins 1 Credit" )
	PORT_DIPSETTING(    0x01, "1 Coin 1 Credit" )
	PORT_DIPSETTING(    0x02, "1 Coin 2 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin 3 Credits" )
	PORT_DIPSETTING(    0x04, "1 Coin 4 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin 5 Credits" )
	PORT_DIPSETTING(    0x06, "1 Coin 6 Credits" )
	PORT_DIPSETTING(    0x07, "1 Coin 7 Credits" )
	PORT_DIPNAME( 0x08, 0x00, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x10, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x20, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x40, "Cocktail" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x80, "On" )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	512,	/* 512 characters */
	2,	/* 3 bits per pixel */
	{ 0, 512*8*8, 512*8*8*2 },	/* the three bitplanes for 4 pixels are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8	/* every char takes 16 bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 3 bits per pixel */
	{ 0, 128*16*16, 128*16*16*2 },	/* the bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7,
	  8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,    0, 64 },
	{ 1, 0x0000, &spritelayout,  0, 64 },
	{ -1 } /* end of array */
};



void amidar_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
/* these are NOT the original color PROMs - they come from Amidar */
static unsigned char color_prom[] =
{
        /* palette */
       0x00,0x07,0xC0,0xB6,0x00,0x38,0xC5,0x67,0x00,0x30,0x07,0x3F,0x00,0x07,0x30,0x3F,
       0x00,0x3F,0x30,0x07,0x00,0x38,0x67,0x3F,0x00,0xFF,0x07,0xDF,0x00,0xF8,0x07,0xFF
};

static struct AY8910interface ay8910_interface =
{
	2,      /* 1 chip */
	14318000/8,     /* ? */
	{ 255, 255 },
	{ 0, soundlatch_r },
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
			18432000/6,     /* 3.072 Mhz ? */
			0,
			readmem,writemem,0,0,
			nmi_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			14318000/8,     /* ? */
			2,
			sound_readmem,sound_writemem,
			sound_readport,sound_writeport,
			interrupt,1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256,256,
	amidar_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	suprmous_vh_screenrefresh,

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

ROM_START( suprmous_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "sm.1", 0x0000, 0x1000, 0xe2160eb0 )
	ROM_LOAD( "sm.2", 0x1000, 0x1000, 0xad39e245 )
	ROM_LOAD( "sm.3", 0x2000, 0x1000, 0xe6851c6d )
	ROM_LOAD( "sm.4", 0x3000, 0x1000, 0xa571d335 )
	ROM_LOAD( "sm.5", 0x4000, 0x1000, 0x6571e1ed )

	ROM_REGION(0x3000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "sm.7",  0x0000, 0x1000, 0x3a4d8b6b )
	ROM_LOAD( "sm.8",  0x1000, 0x1000, 0x3b88e0ae )
	ROM_LOAD( "sm.9",  0x2000, 0x1000, 0xb4d332c3 )

	ROM_REGION(0x10000)	/* 64k for audio CPU */
	ROM_LOAD( "sm.6",  0x0000, 0x1000, 0xcba9ee97 )
ROM_END



struct GameDriver suprmous_driver =
{
	__FILE__,
	0,
	"suprmous",
	"Super Mouse",
	"1982",
	"Taito",
	"Brad Oliver",
	0,
	&machine_driver,

	suprmous_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	input_ports,

	color_prom, 0, 0,
	ORIENTATION_ROTATE_90,

	0, 0
};
