/****************************************************************************
 *
 * geebee.c
 *
 * system driver
 * juergen buchmueller <pullmoll@t-online.de>, jan 2000
 *
 * memory map (preliminary)
 * 0000-0fff ROM1
 * 1000-1fff ROM2
 * 2000-2fff VRAM
 * 3000-3fff CGROM
 * 4000-4fff RAM
 * 5000-5fff IN
 *			 A1 A0
 *			  0  0	  SW0
 *					  D0 COIN1
 *					  D1 COIN2
 *					  D2 START1
 *					  D3 START2
 *					  D4 BUTTON1
 *					  D5 TEST MODE
 *			  0  1	  SW1
 *					  not used in GeeBee
 *			  1  0	  DSW2
 *					  D0	cabinet: 0= upright  1= table
 *					  D1	balls:	 0= 3		 1= 5
 *					  D2-D3 coinage: 0=1c/1c 1=1c/2c 2=2c/1c 3=free play
 *					  D4-D5 bonus:	 0=none, 1=40k	 2=70k	 3=100k
 *			  1  1	  VOLIN
 *					  D0-D7 vcount where paddle starts (note: rotated 90 deg!)
 * 6000-6fff OUT6
 *			 A1 A0
 *			  0  0	  BALL H
 *			  0  1	  BALL V
 *			  1  0	  n/c
 *			  1  1	  SOUND
 *					  D3 D2 D1 D0	   sound
 *					   x  0  0	0  PURE TONE 4V (4500Hz)
 *					   x  0  0	1  PURE TONE 8V (2250Hz)
 *					   x  0  1	0  PURE TONE 16V (1125Hz)
 *					   x  0  1	1  PURE TONE 32V (562.5Hz)
 *					   x  1  0	0  TONE1 (!1V && !16V)
 *					   x  1  0	1  TONE2 (!2V && !32V)
 *					   x  1  1	0  TONE3 (!4V && !64V)
 *					   x  1  1	1  NOISE
 *					   0  x  x	x  DECAY
 *					   1  x  x	x  FULL VOLUME
 * 7000-7fff OUT7
 *			 A2 A1 A0
 *			  0  0	0 LAMP 1
 *			  0  0	1 LAMP 2
 *			  0  1	0 LAMP 3
 *			  0  1	1 COUNTER
 *			  1  0	0 LOCK OUT COIL
 *			  1  0	1 BGW
 *			  1  1	0 BALL ON
 *			  1  1	1 INV
 * 8000-ffff INTA (read FF)
 *
 * TODO:
 *
 * add second controller for cocktail mode + two players?
 *
 ****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* from machine/geebee.c */
extern int geebee_interrupt(void);
extern int geebee_in_r(int offs);
extern void geebee_out6_w(int offs, int data);
extern void geebee_out7_w(int offs, int data);

/* from vidhrdw/geebee.c */
extern int geebee_vh_start(void);
extern void geebee_vh_stop(void);
extern void geebee_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);

/* from sndhrdw/geebee.c */
extern void geebee_sound_w(int offs, int data);
extern int geebee_sh_start(const struct MachineSound *msound);
extern void geebee_sh_stop(void);
extern void geebee_sh_update(void);

/*******************************************************
 *
 * memory regions
 *
 *******************************************************/

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x0fff, MRA_ROM },
	{ 0x2000, 0x23ff, MRA_RAM },
	{ 0x3000, 0x33ff, MRA_ROM },	/* CGROM fails checksum!? */
	{ 0x4000, 0x40ff, MRA_RAM },
	{ 0x5000, 0x5fff, geebee_in_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x0fff, MWA_ROM },
	{ 0x2000, 0x23ff, videoram_w, &videoram, &videoram_size },
	{ 0x4000, 0x40ff, MWA_RAM },
	{ 0x6000, 0x6fff, geebee_out6_w },
	{ 0x7000, 0x7fff, geebee_out7_w },
	{ -1 }  /* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x50, 0x5f, geebee_in_r },
	{ -1 }  /* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x60, 0x6f, geebee_out6_w },
	{ 0x70, 0x7f, geebee_out7_w },
	{ -1 }  /* end of table */
};

INPUT_PORTS_START( geebee )
	PORT_START		/* IN0 SW0 */
	PORT_BIT	( 0x01, IP_ACTIVE_LOW, IPT_COIN1   )
	PORT_BIT	( 0x02, IP_ACTIVE_LOW, IPT_COIN2   )
	PORT_BIT	( 0x04, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT	( 0x08, IP_ACTIVE_LOW, IPT_START2  )
	PORT_BIT	( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_BIT	( 0xc0, 0x00, IPT_UNUSED )

	PORT_START      /* IN1 SW1 */
	PORT_BIT	( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT	( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT	( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT	( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT	( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT	( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT	( 0xc0, 0x00, IPT_UNUSED )

	PORT_START      /* IN2 DSW2 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Free_Play ) )
	/* Bonus Life moved to two inputs to allow changing 3/5 lives mode separately */
	PORT_BIT	( 0x30, 0x00, IPT_UNUSED )
	PORT_BIT	( 0xc0, 0x00, IPT_UNUSED )

	PORT_START		/* IN3 VOLIN */
	PORT_ANALOG( 0xff, 0x80, IPT_AD_STICK_X | IPF_REVERSE, 100, 7, 0, 0x10, 0xd0 )

	PORT_START		/* IN4 FAKE for 3 lives */
	PORT_BIT	( 0x0f, 0x00, IPT_UNUSED )
	PORT_DIPNAME( 0x30, 0x00, "Bonus Life (3 lives)" )
	PORT_DIPSETTING(    0x10, "40k 80k" )
	PORT_DIPSETTING(    0x20, "70k 140k" )
	PORT_DIPSETTING(    0x30, "100k 200k" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_BIT	( 0xc0, 0x00, IPT_UNUSED )

	PORT_START		/* IN5 FAKE for 5 lives */
	PORT_BIT	( 0x0f, 0x00, IPT_UNUSED )
	PORT_DIPNAME( 0x30, 0x00, "Bonus Life (5 lives)" )
	PORT_DIPSETTING(    0x10, "60k 120k" )
	PORT_DIPSETTING(    0x20, "100k 200k" )
	PORT_DIPSETTING(    0x30, "150k 300k" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_BIT	( 0xc0, 0x00, IPT_UNUSED )

INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8, 8,							   /* 8x8 pixels */
	128,							   /* 128 codes */
	1,								   /* 1 bit per pixel */
	{0},							   /* no bitplanes */
	/* x offsets */
	{0,1,2,3,4,5,6,7},
	/* y offsets */
    {0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8},
	8 * 8							   /* eight bytes per code */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_CPU1, 0x3000, &charlayout, 0, 4 },
	{-1}							   /* end of array */
};

static unsigned char palette[] =
{
    0x00,0x00,0x00, /* BLACK */
    0xff,0xff,0xff, /* WHITE */
    0x7f,0x7f,0x7f, /* GREY  */
};

static unsigned short colortable[] = {
	0,1, 0,2, 1,0, 2,0,
};

/* Initialise the palette */
static void init_palette(unsigned char *sys_palette, unsigned short *sys_colortable, const unsigned char *color_prom)
{
    memcpy(sys_palette, palette, sizeof (palette));
    memcpy(sys_colortable, colortable, sizeof (colortable));
}

static struct CustomSound_interface custom_interface =
{
	geebee_sh_start,
	geebee_sh_stop,
	geebee_sh_update
};

static struct MachineDriver machine_driver_geebee =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			18432000/6, 		/* 18.432 Mhz / 6 */
			readmem,writemem,readport,writeport,
			geebee_interrupt,1	/* one interrupts per frame */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	34*8, 32*8, { 0*8, 34*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,	/* gfxdecodeinfo */
	sizeof(palette) / sizeof(palette[0]) / 3,
	sizeof(colortable) / sizeof(colortable[0]),
	init_palette,

	VIDEO_TYPE_RASTER | VIDEO_SUPPORTS_DIRTY,
	0,
	geebee_vh_start,
	geebee_vh_stop,
	geebee_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_CUSTOM,
			&custom_interface
		}
    }
};

ROM_START( geebee ) 																			   /* MJC */
	ROM_REGIONX( 0x10000, REGION_CPU1 )             /* 64k for code */
	ROM_LOAD( "geebee.1k",  0x0000, 0x1000, 0x8a5577e0 )
	ROM_LOAD( "geebee.3a",  0x3000, 0x0400, 0xa45932ba )
ROM_END


GAME( 1979, geebee, 0, geebee, geebee, 0, ROT90, "[Namco] (Gremlin license)", "Gee Bee" )
