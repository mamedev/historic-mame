/***************************************************************************

Wanted Memory Map (preliminary)


Memory Mapped:

0000-5fff   R	ROM
8000-87ff	R/W	RAM
8800-8bff	  W Video RAM (and sprite RAM)
9000-93ff	  W Color RAM (and sprite RAM)
a000		  W Interrupt enable (?)
a000        R   Input port for player 2
a001		  W Flip screen horizontally
a002		  W Flip screen vertically
a004		  W ??? Initialized to 0, never touched again
a006		  W ??? Initialized to 0, never touched again
a007		  W ??? Initialized to 0, never touched again
9a00		  W \ Palette select (?)  Round 0: 00  Round 1: 01
9c00		  W	/ 					  Round 2: 10  Round 3: 11
a800        R   Input port for player 1
b000		R   DIP 1
b800	      W Interrupt emable (?)
b800		R   DIP 2


I/O Ports:

00-01		R/W AY8910 #0
02-03		R/W AY8910 #1


TODO:

- Find color PROM
- Verify Z80 and AY8910 clock speeds

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


//void wanted_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void wanted_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);
void wanted_flipscreen_x_w(int offset, int data);
void wanted_flipscreen_y_w(int offset, int data);




static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x5fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xa000, input_port_0_r },
	{ 0xa800, 0xa800, input_port_1_r },
	{ 0xb000, 0xb000, input_port_2_r },
	{ 0xb800, 0xb800, input_port_3_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x5fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0x8800, 0x8bff, videoram_w, &videoram, &videoram_size },
	{ 0x9000, 0x93ff, colorram_w, &colorram },
	{ 0xa000, 0xa000, MWA_NOP },
	{ 0xa001, 0xa001, wanted_flipscreen_y_w },
	{ 0xa002, 0xa002, wanted_flipscreen_x_w },
	{ 0xb800, 0xb800, MWA_NOP },
	{ -1 }  /* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x00, AY8910_control_port_0_w },
	{ 0x01, 0x01, AY8910_write_port_0_w },
	{ 0x02, 0x02, AY8910_control_port_1_w },
	{ 0x03, 0x03, AY8910_write_port_1_w },
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_START1 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT  | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_COIN1 )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* DSW2 */
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x04, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0xf0, 0x10, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x40, " A 3C/1C  B 3C/1C" )
	PORT_DIPSETTING(    0xe0, " A 3C/1C  B 1C/2C" )
	PORT_DIPSETTING(    0xf0, " A 3C/1C  B 1C/4C" )
	PORT_DIPSETTING(    0x20, " A 2C/1C  B 2C/1C" )
	PORT_DIPSETTING(    0xd0, " A 2C/1C  B 1C/1C" )
	PORT_DIPSETTING(    0x70, " A 2C/1C  B 1C/3C" )
	PORT_DIPSETTING(    0xb0, " A 2C/1C  B 1C/5C" )
	PORT_DIPSETTING(    0xc0, " A 2C/1C  B 1C/6C" )
	PORT_DIPSETTING(    0x60, " A 1C/1C  B 4C/1C" )
	PORT_DIPSETTING(    0x50, " A 1C/1C  B 2C/1C" )
	PORT_DIPSETTING(    0x10, " A 1C/1C  B 1C/1C" )
	PORT_DIPSETTING(    0x30, " A 1C/2C  B 1C/2C" )
	PORT_DIPSETTING(    0xa0, " A 1C/1C  B 1C/3C" )
	PORT_DIPSETTING(    0x80, " A 1C/1C  B 1C/5C" )
	PORT_DIPSETTING(    0x90, " A 1C/1C  B 1C/6C" )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	1024,   /* 1024 characters */
	2,      /* 2 bits per pixel */
	{ 0, 4 },       /* the bitplanes are packed in one byte */
	{ 0, 1, 2, 3, 8*8+0, 8*8+1, 8*8+2, 8*8+3 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8    /* every char takes 8 consecutive bytes */
};

static struct GfxLayout small_spritelayout =
{
	16,16,  /* 16*16 sprites */
	64,     /* 64 sprites */
	2,      /* 2 bits per pixel */
	{ 0, 256*32*8 },        /* the two bitplanes are separated */
	{     0,     1,     2,     3,     4,     5,     6,     7,
	  8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8,
	  16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8    /* every sprite takes 32 consecutive bytes */
};

static struct GfxLayout big_spritelayout =
{
	32,32,  /* 32*32 sprites */
	64,     /* 64 sprites */
	2,      /* 2 bits per pixel */
	{ 0, 256*32*8 },        /* the two bitplanes are separated */
	{      0,      1,      2,      3,      4,      5,      6,      7,
	   8*8+0,  8*8+1,  8*8+2,  8*8+3,  8*8+4,  8*8+5,  8*8+6,  8*8+7,
	  32*8+0, 32*8+1, 32*8+2, 32*8+3, 32*8+4, 32*8+5, 32*8+6, 32*8+7,
	  40*8+0, 40*8+1, 40*8+2, 40*8+3, 40*8+4, 40*8+5, 40*8+6, 40*8+7 },
	{  0*8,  1*8,  2*8,  3*8,  4*8,  5*8,  6*8,  7*8,
	  16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8,
      64*8, 65*8, 66*8, 67*8, 68*8, 69*8, 70*8, 71*8,
      80*8, 81*8, 82*8, 83*8, 84*8, 85*8, 86*8, 87*8 },
	128*8    /* every sprite takes 128 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,         0, 64 },
	{ 1, 0x4000, &small_spritelayout, 0, 64 },
	{ 1, 0x4000, &big_spritelayout,   0, 64 },
	{ -1 } /* end of array */
};


static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1500000,	/* 1.5 MHz ? */
	{ 25, 25 },
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
			readmem,writemem,0,writeport,
			interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	64*4, 64*4,
	0,//wanted_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	wanted_vh_screenrefresh,

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
ROM_START( wanted_rom )
	ROM_REGION(0x10000)       /* 64k for code */
	ROM_LOAD( "prg-1",		0x0000, 0x2000, 0x2dd90aed )
	ROM_LOAD( "prg-2",		0x2000, 0x2000, 0x67ac0210 )
	ROM_LOAD( "prg-3",		0x4000, 0x2000, 0x373c7d82 )

	ROM_REGION_DISPOSE(0x8000)  /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "vram-1",		0x0000, 0x2000, 0xc4226e54 )
	ROM_LOAD( "vram-2",		0x2000, 0x2000, 0x2a9b1e36 )
	ROM_LOAD( "obj-a",		0x4000, 0x2000, 0x90b60771 )
	ROM_LOAD( "obj-b",		0x6000, 0x2000, 0xe14ee689 )

	ROM_REGION(0x0100)       /* color PROM - missing */
	ROM_LOAD( "wanted.clr",	0x0000, 0x0100, 0x00000000 )
ROM_END

/****  Wanted high score save routine - RJF (May 2, 1999)  ****/
static int wanted_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	/* check if the hi score table has already been initialized */
        if (memcmp(&RAM[0x81b4],"\x00\x03\x00",3) == 0)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
                        osd_fread(f,&RAM[0x81b4], 16*5);        /* HS table */

                        RAM[0x81b4] = RAM[0x81b4];      /* update high score */
                        RAM[0x81b5] = RAM[0x81b5];      /* on top of screen */
                        RAM[0x81b6] = RAM[0x81b6];
                        RAM[0x81b7] = RAM[0x81b7];
                        RAM[0x81b8] = RAM[0x81b8];
                        RAM[0x81b9] = RAM[0x81b9];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void wanted_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
                osd_fwrite(f,&RAM[0x81b4], 16*5);       /* HS table */
		osd_fclose(f);
	}
}

struct GameDriver wanted_driver =
{
	__FILE__,
	0,
	"wanted",
	"Wanted",
	"1984",
	"Sigma Ent. Inc.",
	"Zsolt Vasvari",
	GAME_WRONG_COLORS,
	&machine_driver,
	0,

	wanted_rom,
	0,
	0,
	0,
	0,      /* sound_prom */

	input_ports,

	0, 0, 0,   /* colors, palette, colortable */
	ORIENTATION_ROTATE_90,

        wanted_hiload, wanted_hisave
};
