/***************************************************************************

Zodiack/Dogfight Memory Map (preliminary)


Memory Mapped:


I/O Ports:

00-01		W   AY8910 #0


TODO:

- Verify Z80 and AY8910 clock speeds

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *zodiack_videoram2;
extern unsigned char *galaxian_attributesram;
extern unsigned char *galaxian_bulletsram;
extern int galaxian_bulletsram_size;

void zodiack_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void zodiack_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void galaxian_attributes_w(int offset,int data);
void zodiac_flipscreen_w(int offset,int data);
void zodiac_control_w(int offset,int data);

void espial_init_machine(void);
void zodiac_master_interrupt_enable_w(int offset, int data);
int  zodiac_master_interrupt(void);
void zodiac_master_soundlatch_w(int offset, int data);

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x4fff, MRA_ROM },
	{ 0x5800, 0x5fff, MRA_RAM },
	{ 0x6081, 0x6081, input_port_0_r },
	{ 0x6082, 0x6082, input_port_1_r },
	{ 0x6083, 0x6083, input_port_2_r },
	{ 0x6084, 0x6084, input_port_3_r },
	{ 0x6090, 0x6090, soundlatch_r },
	{ 0x7000, 0x7000, MRA_NOP },  /* ??? */
	{ 0x9000, 0x93ff, MRA_RAM },
	{ 0xa000, 0xa3ff, MRA_RAM },
	{ 0xb000, 0xb3ff, MRA_RAM },
	{ 0xc000, 0xcfff, MRA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x4fff, MWA_ROM },
	{ 0x5800, 0x5fff, MWA_RAM },
	{ 0x6081, 0x6081, zodiac_control_w },
	{ 0x6090, 0x6090, zodiac_master_soundlatch_w },
	{ 0x7000, 0x7000, watchdog_reset_w },
	{ 0x7100, 0x7100, zodiac_master_interrupt_enable_w },
	{ 0x7200, 0x7200, zodiac_flipscreen_w },
	{ 0x9000, 0x903f, galaxian_attributes_w, &galaxian_attributesram },
	{ 0x9040, 0x905f, MWA_RAM, &spriteram, &spriteram_size },
	{ 0x9060, 0x907f, MWA_RAM, &galaxian_bulletsram, &galaxian_bulletsram_size },
	{ 0x9080, 0x93ff, MWA_RAM },
	{ 0xa000, 0xa3ff, videoram_w, &videoram, &videoram_size },
	{ 0xb000, 0xb3ff, MWA_RAM, &zodiack_videoram2 },
	{ 0xc000, 0xcfff, MWA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x2000, 0x23ff, MRA_RAM },
	{ 0x6000, 0x6000, soundlatch_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x2000, 0x23ff, MWA_RAM },
	{ 0x4000, 0x4000, interrupt_enable_w },
	{ 0x6000, 0x6000, soundlatch_w },
	{ -1 }	/* end of table */
};

static struct IOWritePort sound_writeport[] =
{
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
	{ -1 }	/* end of table */
};


INPUT_PORTS_START( zodiac_input_ports )
	PORT_START      /* DSW0 */  /* never read in this game */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x1c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x18, "2 Coins/1 Credit  3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20000 50000" )
	PORT_DIPSETTING(    0x20, "40000 70000" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
INPUT_PORTS_END

INPUT_PORTS_START( dogfight_input_ports )
	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 3C_2C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x38, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x38, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x28, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 3C_4C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )  /* most likely unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )  /* most likely unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )  /* most likely unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )  /* most likely unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_BITX(    0x10, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x10, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )  /* bonus */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, "Freeze Screen" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
INPUT_PORTS_END

INPUT_PORTS_START( moguchan_input_ports )
	PORT_START      /* DSW0 */
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x1c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x18, "2 Coins/1 Credit  3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "20000 50000" )
	PORT_DIPSETTING(    0x20, "40000 70000" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL)	    /* these are read, but are they */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 )					/* ever used? */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 chars */
	256,    /* 256 characters */
	1,      /* 1 bit per pixel */
	{ 0 } , /* single bitplane */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout charlayout_2 =
{
	8,8,    /* 8*8 chars */
	256,    /* 256 characters */
	2,      /* 2 bits per pixel */
	{ 0, 512*8*8 },  /* The bitplanes are seperate */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	64,     /* 64 sprites */
	2,      /* 2 bits per pixel */
	{ 0, 128*32*8 },        /* the two bitplanes are separated */
	{     0,     1,     2,     3,     4,     5,     6,     7,
	  8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8,
	  16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout bulletlayout =
{
	/* there is no gfx ROM for this one, it is generated by the hardware */
	7,1,	/* it's just 1 pixel, but we use 7*1 to position it correctly */
	1,	/* just one */
	1,	/* 1 bit per pixel */
	{ 10*8*8 },	/* point to letter "A" */
	{ 3, 7, 7, 7, 7, 7, 7 },	/* I "know" that this bit of the */
	{ 1*8 },						/* graphics ROMs is 1 */
	0	/* no use */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,   0, 8 },
	{ 1, 0x1000, &spritelayout, 0, 8 },
	{ 1, 0x0000, &bulletlayout, 6, 1 },
	{ 1, 0x1800, &charlayout_2, 0, 8 },
	{ -1 } /* end of array */
};


static struct AY8910interface ay8910_interface =
{
	1,	/* 1 chip */
	1789750,	/* 1.78975 MHz? */
	{ 50 },
	AY8910_DEFAULT_GAIN,
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
			4000000,        /* 4.00 MHz??? */
			0,
			readmem,writemem,0,0,
			zodiac_master_interrupt,2
		},
		{
			CPU_Z80,
			14318000/8,	/* 1.78975 Mhz */
			3,	/* memory region #3 */
			sound_readmem,sound_writemem,0,sound_writeport,
			nmi_interrupt,8	/* IRQs are triggered by the main CPU */
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	espial_init_machine,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	32, 32,
	zodiack_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	zodiack_vh_screenrefresh,

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
ROM_START( zodiack_rom )
	ROM_REGION(0x10000)       /* 64k for code */
	ROM_LOAD( "ovg30c.2",     0x0000, 0x2000, 0xa2125e99 )
	ROM_LOAD( "ovg30c.3",     0x2000, 0x2000, 0xaee2b77f )
	ROM_LOAD( "ovg30c.6",     0x4000, 0x0800, 0x1debb278 )

	ROM_REGION_DISPOSE(0x3000)  /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "ovg40c.7",     0x0000, 0x1000, 0xa90e72a5 )
	ROM_LOAD( "orca40c.8",    0x1000, 0x1000, 0x88269c94 )
	ROM_LOAD( "orca40c.9",    0x2000, 0x1000, 0xa3bd40c9 )

	ROM_REGION(0x0040)	/* color prom */
	ROM_LOAD( "ovg40c.2a",    0x0000, 0x0020, 0x703821b8 )
	ROM_LOAD( "ovg40c.2b",    0x0020, 0x0020, 0x21f77ec7 )  /* I don't know what this is */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "ovg20c.1",     0x0000, 0x1000, 0x2d3c3baf )
ROM_END

ROM_START( dogfight_rom )
	ROM_REGION(0x10000)       /* 64k for code */
	ROM_LOAD( "df-2",         0x0000, 0x2000, 0xad24b28b )
	ROM_LOAD( "df-3",         0x2000, 0x2000, 0xcd172707 )
	ROM_LOAD( "df-5",         0x4000, 0x1000, 0x874dc6bf )
	ROM_LOAD( "df-4",         0xc000, 0x1000, 0xd8aa3d6d )

	ROM_REGION_DISPOSE(0x3000)  /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "df-6",         0x0000, 0x1000, 0xb216e664 )
	ROM_LOAD( "df-7",         0x1000, 0x1000, 0xffe05fee )
	ROM_LOAD( "df-8",         0x2000, 0x1000, 0x2cb51793 )

	ROM_REGION(0x0040)	/* color prom */
	ROM_LOAD( "1.bpr",        0x0000, 0x0020, 0x69a35aa5 )
	ROM_LOAD( "2.bpr",        0x0020, 0x0020, 0x596ae457 )  /* I don't know what this is */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "df-1",         0x0000, 0x1000, 0xdcbb1c5b )
ROM_END

ROM_START( moguchan_rom )
	ROM_REGION(0x10000)       /* 64k for code */
	ROM_LOAD( "2.5r",         0x0000, 0x1000, 0x85d0cb7e )
	ROM_LOAD( "4.5m",         0x1000, 0x1000, 0x359ef951 )
	ROM_LOAD( "3.5np",        0x2000, 0x1000, 0xc8776f77 )

	ROM_REGION_DISPOSE(0x3000)  /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5.4r",         0x0000, 0x1000, 0x7a0fd027 )
	ROM_LOAD( "6.7p",         0x1000, 0x1000, 0xc8060ffe )
	ROM_LOAD( "7.7m",         0x2000, 0x1000, 0xbfca00f4 )

	ROM_REGION(0x0040)	/* color prom - missing */
	ROM_LOAD( "moguchan.cl1", 0x0000, 0x0020, 0x00000000 )
	ROM_LOAD( "moguchan.cl2", 0x0020, 0x0020, 0x00000000 )  /* I don't know what this is */

	ROM_REGION(0x10000)	/* 64k for the audio CPU */
	ROM_LOAD( "1.7hj",        0x0000, 0x1000, 0x1a88d35f )
ROM_END

/****************************************************************************/

/****  Zodiack high score save routine - RJF (May 17, 1999)  ****/

static int zodiack_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0x5875],"\x14\x12\x1d",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0x5857], 5*6);        /* scores */
                        osd_fread(f,&RAM[0x5875], 5*5);        /* initials */

                        RAM[0x5847] = RAM[0x5859];      /* update high score */
                        RAM[0x5848] = RAM[0x585a];      /* on top of screen */
                        RAM[0x5849] = RAM[0x585b];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void zodiack_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0x5857], 5*6);
                osd_fwrite(f,&RAM[0x5875], 5*5);
		osd_fclose(f);
	}
}

/****  Dogfight high score save routine - RJF (May 19, 1999)  ****/

static int dogfight_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0x587e],"\x20\x35\x00",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0x587e], 10*3);        /* scores */
                        osd_fread(f,&RAM[0x589c], 10*10);       /* initials */

                        RAM[0x587b] = RAM[0x587e];      /* update high score */
                        RAM[0x587c] = RAM[0x587f];      /* on top of screen */
                        RAM[0x587d] = RAM[0x5880];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void dogfight_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0x587e], 10*3);
                osd_fwrite(f,&RAM[0x589c], 10*10);
		osd_fclose(f);
	}
}

/****  Moguchan high score save routine - RJF (May 19, 1999)  ****/

static int moguchan_hiload(void)
{
    unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	static int firsttime;

	/* check if the hi score table has already been initialized */
	/* the high score table is intialized to all 0, so first of all */
	/* we dirty it, then we wait for it to be cleared again */
	if (firsttime == 0)
	{
              memset(&RAM[0x5eda],0xff, 5);
              firsttime = 1;
	}

        if(memcmp(&RAM[0x5eda],"\x00\x00\x00",3) == 0)
	{
              void *f;
              if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
              {
                        osd_fread(f,&RAM[0x5eda], 5);	/* top score */
                        osd_fclose(f);
              }

              return 1;
    		  firsttime = 0;
	}
      else return 0;   /* we can't load the hi scores yet */
}

static void moguchan_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0x5eda], 5);
		osd_fclose(f);
	}
}

/***************************************************************************/



struct GameDriver zodiack_driver =
{
	__FILE__,
	0,
	"zodiack",
	"Zodiack",
	"1983",
	"Orca (Esco Trading Co, Inc)",
	"Zsolt Vasvari",
	GAME_IMPERFECT_COLORS,
	&machine_driver,
	0,

	zodiack_rom,
	0,
	0,
	0,
	0,      /* sound_prom */

	zodiac_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,   /* colors, palette, colortable */
	ORIENTATION_ROTATE_270,

        zodiack_hiload, zodiack_hisave
};

struct GameDriver dogfight_driver =
{
	__FILE__,
	0,
	"dogfight",
	"Dog Fight",
	"1983",
	"[Orca] Thunderbolt",
	"Zsolt Vasvari",
	GAME_IMPERFECT_COLORS,
	&machine_driver,
	0,

	dogfight_rom,
	0,
	0,
	0,
	0,      /* sound_prom */

	dogfight_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,   /* colors, palette, colortable */
	ORIENTATION_ROTATE_270,

        dogfight_hiload, dogfight_hisave
};

struct GameDriver moguchan_driver =
{
	__FILE__,
	0,
	"moguchan",
	"Moguchan",
	"1982",
	"Orca (Eastern Commerce Inc. license) (bootleg?)",  /* this is in the ROM at $0b5c */
	"Zsolt Vasvari",
	GAME_WRONG_COLORS,
	&machine_driver,
	0,

	moguchan_rom,
	0,
	0,
	0,
	0,      /* sound_prom */

	moguchan_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,   /* colors, palette, colortable */
	ORIENTATION_ROTATE_270,

	moguchan_hiload, moguchan_hisave
};

