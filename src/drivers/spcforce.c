/***************************************************************************

Space Force Memory Map

driver by Zsolt Vasvari


0000-3fff   R	ROM
4000-43ff	R/W	RAM
7000-7002	R   input ports 0-2
7000		  W sound command
7001	      W sound CPU IRQ trigger on bit 3 falling edge
7002		  W unknown
7008		  W unknown
7009		  W unknown
700a		  W unknown
700b		  W flip screen
700c		  W unknown
700d		  W unknown
700e		  W main CPU interrupt enable (it uses RST7.5)
700f		  W unknown
8000-83ff   R/W bit 0-7 of character code
9000-93ff   R/W attributes RAM
				bit 0   - bit 8 of character code
				bit 1-3 - unused
				bit 4-6 - color
				bit 7   - unused
a000-a3ff	R/W X/Y scroll position of each character (can be scrolled up
				to 7 pixels in each direction)


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/i8085/i8085.h"
#include "cpu/i8039/i8039.h"


extern unsigned char *spcforce_scrollram;

WRITE_HANDLER( spcforce_flip_screen_w );
void spcforce_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);


static int spcforce_interrupt(void)
{
	return I8085_RST75;
}


static int spcforce_SN76496_latch;
static int spcforce_SN76496_select;

static WRITE_HANDLER( spcforce_SN76496_latch_w )
{
	spcforce_SN76496_latch = data;
}

static READ_HANDLER( spcforce_SN76496_select_r )
{
	return spcforce_SN76496_select;
}

static WRITE_HANDLER( spcforce_SN76496_select_w )
{
    spcforce_SN76496_select = data;

	if (~data & 0x40)  SN76496_0_w(0, spcforce_SN76496_latch);
	if (~data & 0x20)  SN76496_1_w(0, spcforce_SN76496_latch);
	if (~data & 0x10)  SN76496_2_w(0, spcforce_SN76496_latch);
}

static READ_HANDLER( spcforce_t0_r )
{
	/* SN76496 status according to Al - not supported by MAME?? */
	return rand() & 1;
}


static WRITE_HANDLER( spcforce_soundtrigger_w )
{
	cpu_set_irq_line(1, I8039_EXT_INT, (~data & 0x08) ? ASSERT_LINE : CLEAR_LINE);
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x4000, 0x43ff, MRA_RAM },
	{ 0x7000, 0x7000, input_port_0_r },
	{ 0x7001, 0x7001, input_port_1_r },
	{ 0x7002, 0x7002, input_port_2_r },
	{ 0x8000, 0x83ff, MRA_RAM },
	{ 0x9000, 0x93ff, MRA_RAM },
	{ 0xa000, 0xa3ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x3fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
	{ 0x7000, 0x7000, soundlatch_w },
	{ 0x7001, 0x7001, spcforce_soundtrigger_w },
	{ 0x700b, 0x700b, spcforce_flip_screen_w },
	{ 0x700e, 0x700e, interrupt_enable_w },
	{ 0x700f, 0x700f, MWA_NOP },
	{ 0x8000, 0x83ff, MWA_RAM, &videoram, &videoram_size },
	{ 0x9000, 0x93ff, MWA_RAM, &colorram },
	{ 0xa000, 0xa3ff, MWA_RAM, &spcforce_scrollram },
MEMORY_END

static MEMORY_READ_START( sound_readmem )
	{ 0x0000, 0x07ff, MRA_ROM },
MEMORY_END

static MEMORY_WRITE_START( sound_writemem )
	{ 0x0000, 0x07ff, MWA_ROM },
MEMORY_END

static PORT_READ_START( sound_readport )
	{ I8039_bus, I8039_bus, soundlatch_r },
	{ I8039_p2,  I8039_p2,  spcforce_SN76496_select_r },
	{ I8039_t0,  I8039_t0,  spcforce_t0_r },
PORT_END

static PORT_WRITE_START( sound_writeport )
	{ I8039_p1,  I8039_p1,  spcforce_SN76496_latch_w },
	{ I8039_p2,  I8039_p2,  spcforce_SN76496_select_w },
PORT_END


INPUT_PORTS_START( spcforce )
	PORT_START      /* DSW */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x18, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x18, "5" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )

	PORT_START      /* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )

	PORT_START      /* IN1 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_COCKTAIL | IPF_2WAY )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_COCKTAIL | IPF_2WAY )
INPUT_PORTS_END

/* same as spcforce, but no cocktail mode */
INPUT_PORTS_START( spcforc2 )
	PORT_START      /* DSW */
	PORT_DIPNAME( 0x03, 0x02, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x18, 0x08, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x18, "5" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )	/* probably unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) )  /* probably unused */
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START      /* IN0 */
	PORT_BIT ( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT ( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT ( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT ( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT ( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT ( 0x20, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT ( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )

	PORT_START      /* IN1 */
	PORT_BITX( 0x08, IP_ACTIVE_LOW, IPT_SERVICE, DEF_STR( Service_Mode ), KEYCODE_F2, IP_JOY_NONE )
	PORT_BIT ( 0x40, IP_ACTIVE_LOW, IPT_VBLANK )
INPUT_PORTS_END


static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 chars */
	512,    /* 512 characters */
	3,      /* 3 bits per pixel */
	{ 2*512*8*8, 512*8*8, 0 },  /* The bitplanes are seperate */
	{ 0, 1, 2, 3, 4, 5, 6, 7},
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8},
	8*8     /* every char takes 8 consecutive bytes */
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 8 },
	{ -1 } /* end of array */
};


/* 1-bit RGB palette */
static unsigned char palette[] =
{
	0x00, 0x00, 0x00,
	0xff, 0x00, 0x00,
	0x00, 0xff, 0x00,
	0xff, 0xff, 0x00,
	0x00, 0x00, 0xff,
	0xff, 0x00, 0xff,
	0x00, 0xff, 0xff,
	0xff, 0xff, 0xff
};
static unsigned short colortable[] =
{
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 0, 1, 2, 3, 4, 5, 6,	 /* not sure about these, but they are only used */
	0, 7, 0, 1, 2, 3, 4, 5,  /* to change the text color. During the game,   */
	0, 6, 7, 0, 1, 2, 3, 4,  /* only color 0 is used, which is correct.      */
	0, 5, 6, 7, 0, 1, 2, 3,
	0, 4, 5, 6, 7, 0, 1, 2,
	0, 3, 4, 5, 6, 7, 0, 1,
	0, 2, 3, 4, 5, 6, 7, 0
};
static void init_palette(unsigned char *game_palette, unsigned short *game_colortable,const unsigned char *color_prom)
{
	memcpy(game_palette,palette,sizeof(palette));
	memcpy(game_colortable,colortable,sizeof(colortable));
}


static struct SN76496interface sn76496_interface =
{
	3,		/* 3 chips */
	{ 2000000, 2000000, 2000000 },	/* 8 MHz / 4 ?*/
	{ 100, 100, 100 }
};


static const struct MachineDriver machine_driver_spcforce =
{
	/* basic machine hardware */
	{
		{
			CPU_8085A,
			4000000,        /* 4.00 MHz??? */
			readmem,writemem,0,0,
			spcforce_interrupt,1
		},
		{
            CPU_I8035 | CPU_AUDIO_CPU,
            6144000/8,		/* divisor ??? */
			sound_readmem,sound_writemem,sound_readport,sound_writeport,
            ignore_interrupt,0  /* IRQ's are triggered by the main CPU */
        }
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	sizeof(palette) / sizeof(palette[0]) / 3, sizeof(colortable) / sizeof(colortable[0]),
	init_palette,

	VIDEO_TYPE_RASTER,
	0,
	generic_bitmapped_vh_start,
	generic_bitmapped_vh_stop,
	spcforce_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};


/***************************************************************************

  Game driver(s)

***************************************************************************/
ROM_START( spcforce )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for code */
	ROM_LOAD( "m1v4f.1a",  	  0x0000, 0x0800, 0x7da0d1ed )
	ROM_LOAD( "m2v4f.1c",  	  0x0800, 0x0800, 0x25605bff )
	ROM_LOAD( "m3v5f.2a",  	  0x1000, 0x0800, 0x6f879366 )
	ROM_LOAD( "m4v5f.2c",  	  0x1800, 0x0800, 0x7fbfabfa )
							/*0x2000 empty */
	ROM_LOAD( "m6v4f.3c",  	  0x2800, 0x0800, 0x12128e9e )
	ROM_LOAD( "m7v4f.4a",  	  0x3000, 0x0800, 0x978ad452 )
	ROM_LOAD( "m8v4f.4c",  	  0x3800, 0x0800, 0xf805c3cd )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )		/* sound MCU */
	ROM_LOAD( "spacefor.snd", 0x0000, 0x0800, 0x8820913c )

	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rm1v2.6s",     0x0000, 0x0800, 0x8e3490d7 )
	ROM_LOAD( "rm2v1.7s",     0x0800, 0x0800, 0xfbbfa05a )
	ROM_LOAD( "gm1v2.6p",     0x1000, 0x0800, 0x4f574920 )
	ROM_LOAD( "gm2v1.7p",     0x1800, 0x0800, 0x0cd89ce2 )
	ROM_LOAD( "bm1v2.6m",     0x2000, 0x0800, 0x130869ce )
	ROM_LOAD( "bm2v1.7m",     0x2800, 0x0800, 0x472f0a9b )
ROM_END

ROM_START( spcforc2 )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for code */
	ROM_LOAD( "spacefor.1a",  0x0000, 0x0800, 0xef6fdccb )
	ROM_LOAD( "spacefor.1c",  0x0800, 0x0800, 0x44bd1cdd )
	ROM_LOAD( "spacefor.2a",  0x1000, 0x0800, 0xfcbc7df7 )
	ROM_LOAD( "vm4", 	      0x1800, 0x0800, 0xc5b073b9 )
							/*0x2000 empty */
	ROM_LOAD( "spacefor.3c",  0x2800, 0x0800, 0x9fd52301 )
	ROM_LOAD( "spacefor.4a",  0x3000, 0x0800, 0x89aefc0a )
	ROM_LOAD( "m8v4f.4c",  	  0x3800, 0x0800, 0xf805c3cd )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )		/* sound MCU */
	ROM_LOAD( "spacefor.snd", 0x0000, 0x0800, 0x8820913c )

	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "spacefor.6s",  0x0000, 0x0800, 0x848ae522 )
	ROM_LOAD( "rm2v1.7s",     0x0800, 0x0800, 0xfbbfa05a )
	ROM_LOAD( "spacefor.6p",  0x1000, 0x0800, 0x95446911 )
	ROM_LOAD( "gm2v1.7p",     0x1800, 0x0800, 0x0cd89ce2 )
	ROM_LOAD( "bm1v2.6m",     0x2000, 0x0800, 0x130869ce )
	ROM_LOAD( "bm2v1.7m",     0x2800, 0x0800, 0x472f0a9b )
ROM_END

ROM_START( meteor )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )       /* 64k for code */
	ROM_LOAD( "vm1", 	      0x0000, 0x0800, 0x894fe9b1 )
	ROM_LOAD( "vm2", 	      0x0800, 0x0800, 0x28685a68 )
	ROM_LOAD( "vm3", 	      0x1000, 0x0800, 0xc88fb12a )
	ROM_LOAD( "vm4", 	      0x1800, 0x0800, 0xc5b073b9 )
							/*0x2000 empty */
	ROM_LOAD( "vm6", 	      0x2800, 0x0800, 0x9969ec43 )
	ROM_LOAD( "vm7", 	      0x3000, 0x0800, 0x39f43ac2 )
	ROM_LOAD( "vm8", 	      0x3800, 0x0800, 0xa0508de3 )

	ROM_REGION( 0x1000, REGION_CPU2, 0 )		/* sound MCU */
	ROM_LOAD( "vm5", 	      0x0000, 0x0800, 0xb14ccd57 )

	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "rm1v",         0x0000, 0x0800, 0xd621fe96 )
	ROM_LOAD( "rm2v",         0x0800, 0x0800, 0xb3981251 )
	ROM_LOAD( "gm1v",         0x1000, 0x0800, 0xd44617e8 )
	ROM_LOAD( "gm2v",         0x1800, 0x0800, 0x0997d945 )
	ROM_LOAD( "bm1v",         0x2000, 0x0800, 0xcc97c890 )
	ROM_LOAD( "bm2v",         0x2800, 0x0800, 0x2858cf5c )
ROM_END


GAMEX( 1980, spcforce, 0,        spcforce, spcforce, 0, ROT270, "Venture Line", "Space Force", GAME_IMPERFECT_COLORS )
GAMEX( 19??, spcforc2, spcforce, spcforce, spcforc2, 0, ROT270, "Elcon (bootleg?)", "Space Force (set 2)", GAME_IMPERFECT_COLORS )
GAMEX( 1981, meteor,   spcforce, spcforce, spcforc2, 0, ROT270, "Venture Line", "Meteoroids", GAME_IMPERFECT_COLORS )
