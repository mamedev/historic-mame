/***************************************************************************

Hole Land

driver by Mathis Rosenhauer

Bugs: Usage of the other sprite banks is unknown.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

int holeland_vh_start( void );
void holeland_vh_convert_color_prom(unsigned char *obsolete,unsigned short *colortable,const unsigned char *color_prom);
void holeland_vh_screenrefresh( struct osd_bitmap *bitmap,int full_refresh );

WRITE_HANDLER( holeland_videoram_w );
WRITE_HANDLER( holeland_colorram_w );
WRITE_HANDLER( holeland_flipscreen_w );
WRITE_HANDLER( holeland_pal_offs_w );

static int ff1, ff2;

static READ_HANDLER( holeland_reset_r )
{
	ff1 = 1;
	ff2 = 1;
	return 0;
}
static READ_HANDLER( holeland_controls_r )
{
	if (ff1)
	{
		ff1 = 0;
		return input_port_0_r(offset);
	}
	else
		return input_port_1_r(offset);

}
static READ_HANDLER( holeland_dsw_r )
{
	if (ff2)
	{
		ff2 = 0;
		return input_port_2_r(offset);
	}
	else
		return input_port_3_r(offset);
}

static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xa000, 0xbfff, MRA_ROM },
	{ 0xf000, 0xf3ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xc001, holeland_pal_offs_w },
//	{ 0xc002, 0xc003, MWA_NOP }, /* unknown */
//	{ 0xc004, 0xc005, MWA_NOP }, /* seems to be coin related */
	{ 0xc006, 0xc007, holeland_flipscreen_w },
	{ 0xe000, 0xe3ff, holeland_colorram_w, &colorram },
	{ 0xe400, 0xe7ff, holeland_videoram_w, &videoram, &videoram_size },
	{ 0xf000, 0xf3ff, MWA_RAM, &spriteram, &spriteram_size },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x01, 0x01, holeland_reset_r },
	{ 0x04, 0x04, holeland_controls_r },
	{ 0x06, 0x06, holeland_dsw_r },
PORT_END

static PORT_WRITE_START( writeport )
	{ 0x04, 0x04, AY8910_control_port_0_w },
	{ 0x05, 0x05, AY8910_write_port_0_w },
	{ 0x06, 0x06, AY8910_control_port_1_w },
	{ 0x07, 0x07, AY8910_write_port_1_w },
PORT_END

INPUT_PORTS_START( holeland )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_PLAYER2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_PLAYER2 )

	PORT_START
	PORT_SERVICE( 0x01, IP_ACTIVE_HIGH )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "Language" )
	PORT_DIPSETTING(    0x00, "English" )
	PORT_DIPSETTING(    0x04, "Nihongo" )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "60000" )
	PORT_DIPSETTING(    0x20, "90000" )
	PORT_DIPNAME( 0x40, 0x00, "Fase 3 Difficulty")
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, "Coin Mode" )
	PORT_DIPSETTING(    0x80, "A" )
	PORT_DIPSETTING(    0x00, "B" )

	PORT_START
	PORT_DIPNAME( 0x03, 0x00, "Coin Case" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPSETTING(    0x01, "2" )
	PORT_DIPSETTING(    0x02, "3" )
	PORT_DIPSETTING(    0x03, "4" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x0c, "5" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Very Easy" )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x20, "Hard" )
	PORT_DIPSETTING(    0x30, "Very Hard" )
	PORT_DIPNAME( 0x40, 0x40, "Monsters" )
	PORT_DIPSETTING(    0x40, "Min" )
	PORT_DIPSETTING(    0x00, "Max" )
	PORT_DIPNAME( 0x80, 0x80, "Mode" ) /* seems to have no effect */
	PORT_DIPSETTING(    0x00, "Stop" )
	PORT_DIPSETTING(    0x80, "Play" )
INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	2,	/* 2 bits per pixel */
	{ 4, 0},
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*8*2, 1*8*2, 2*8*2, 3*8*2, 4*8*2, 5*8*2, 6*8*2, 7*8*2 },
	8*8*2	/* every char takes 16 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	128,	/* 128 sprites */
	2,	/* 2 bits per pixel */
	{ 4, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
	  8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8},
	{ 0, 1, 2, 3, 16*8+0, 16*8+1, 16*8+2, 16*8+3,
	  32*8+0, 32*8+1, 32*8+2, 32*8+3, 48*8+0, 48*8+1, 48*8+2, 48*8+3 },
	64*8	/* every sprite takes 64 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout  , 0, 256 },
	{ REGION_GFX2, 0, &spritelayout  , 0, 256 },
	{ REGION_GFX3, 0, &spritelayout  , 0, 256 },
	{ REGION_GFX4, 0, &spritelayout  , 0, 256 },
	{ REGION_GFX5, 0, &spritelayout  , 0, 256 },
	{ -1 } /* end of array */
};

static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	1818182,	/* 1.82 MHz ? */
	{ 20, 20 }
};

static struct MachineDriver machine_driver_holeland =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,        /* 4 MHz ? */
			readmem, writemem,readport, writeport,
			interrupt, 1
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	256, 0,
	holeland_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	holeland_vh_start,
	0,
	holeland_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};

static void init_holeland(void)
{
	int i, j;
	UINT8 *src, *dst1, *dst2, *dst3, *dst4;

	src = memory_region(REGION_GFX1);
	for (i = 0; i < 0x4000; i++)
		src[i] ^= 0xff;

	src = memory_region(REGION_GFX6);
	dst1 = memory_region(REGION_GFX2);
	dst2 = memory_region(REGION_GFX3);
	dst3 = memory_region(REGION_GFX4);
	dst4 = memory_region(REGION_GFX5);

	/* The sprite roms are interleaved */
	for (i = 0; i < 128; i++)
	{
		for (j = 0; j < 32; j++)
		{
			*dst1++ = ((src[0] & 0x22) << 2) | ((src[0x2000] & 0x22) << 1)
				| ((src[0x4000] & 0x22)) | ((src[0x6000] & 0x22) >> 1);
			*dst1++ = ((src[0] & 0x11) << 3) | ((src[0x2000] & 0x11) << 2)
				| ((src[0x4000] & 0x11) << 1) | ((src[0x6000] & 0x11));

			*dst2++ = ((src[0x20] & 0x22) << 2) | ((src[0x2020] & 0x22) << 1)
				| ((src[0x4020] & 0x22)) | ((src[0x6020] & 0x22) >> 1);
			*dst2++ = ((src[0x20] & 0x11) << 3) | ((src[0x2020] & 0x11) << 2)
				| ((src[0x4020] & 0x11) << 1) | ((src[0x6020] & 0x11));

			*dst3++ = ((src[0] & 0x88)) | ((src[0x2000] & 0x88) >> 1)
				| ((src[0x4000] & 0x88) >> 2) | ((src[0x6000] & 0x88) >> 3);
			*dst3++ = ((src[0] & 0x44) << 1) | ((src[0x2000] & 0x44))
				| ((src[0x4000] & 0x44) >> 1) | ((src[0x6000] & 0x44) >> 2);

			*dst4++ = ((src[0x20] & 0x88)) | ((src[0x2020] & 0x88) >> 1)
				| ((src[0x4020] & 0x88) >> 2) | ((src[0x6020] & 0x88) >> 3);
			*dst4++ = ((src[0x20] & 0x44) << 1) | ((src[0x2020] & 0x44))
				| ((src[0x4020] & 0x44) >> 1) | ((src[0x6020] & 0x44) >> 2);

			src++;
		}
		src+=32;
	}
}

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( holeland )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )          /* 64k for code */
	ROM_LOAD( "holeland.0",     0x00000, 0x2000, 0xb640e12b )
	ROM_LOAD( "holeland.1",     0x02000, 0x2000, 0x2f180851 )
	ROM_LOAD( "holeland.2",     0x04000, 0x2000, 0x35cfde75 )
	ROM_LOAD( "holeland.3",     0x06000, 0x2000, 0x5537c22e )
	ROM_LOAD( "holeland.4",     0x0a000, 0x2000, 0xc95c355d )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "holeland.5",     0x00000, 0x2000, 0x7f19e1f9 )
	ROM_LOAD( "holeland.6",     0x02000, 0x2000, 0x844400e3 )

	ROM_REGION( 0x02000, REGION_GFX2, ROMREGION_DISPOSE )

	ROM_REGION( 0x02000, REGION_GFX3, ROMREGION_DISPOSE )

	ROM_REGION( 0x02000, REGION_GFX4, ROMREGION_DISPOSE )

	ROM_REGION( 0x02000, REGION_GFX5, ROMREGION_DISPOSE )

	ROM_REGION( 0x08000, REGION_GFX6, ROMREGION_DISPOSE )
	ROM_LOAD( "holeland.7",     0x00000, 0x2000, 0xd7feb25b )
	ROM_LOAD( "holeland.8",     0x02000, 0x2000, 0x4b6eec16 )
	ROM_LOAD( "holeland.9",     0x04000, 0x2000, 0x6fe7fcc0 )
	ROM_LOAD( "holeland.10",    0x06000, 0x2000, 0xe1e11e8f )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "3m",    0x0000, 0x0100, 0x9d6fef5a )  /* Red component */
	ROM_LOAD( "3l",    0x0100, 0x0100, 0xf6682705 )  /* Green component */
	ROM_LOAD( "3n",    0x0200, 0x0100, 0x3d7b3af6 )  /* Blue component */
ROM_END


GAME( 1984, holeland, 0, holeland, holeland, holeland, ROT0, "Tecfri", "Hole Land" )
