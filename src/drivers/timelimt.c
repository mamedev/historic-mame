/***************************************************************************

Time Limit (c) 1983 Chuo

driver by Ernesto Corvi

Notes:
- Sprite colors are wrong (missing colortable?)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* from vidhrdw */
extern int timelimt_vh_start( void );
extern void timelimt_vh_stop( void );
extern void timelimt_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
extern void timelimt_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

extern WRITE_HANDLER( timelimt_videoram_w );
extern WRITE_HANDLER( timelimt_bg_videoram_w );
extern WRITE_HANDLER( timelimt_scroll_y_w );
extern WRITE_HANDLER( timelimt_scroll_x_msb_w );
extern WRITE_HANDLER( timelimt_scroll_x_lsb_w );

extern data8_t *timelimt_bg_videoram;
extern size_t timelimt_bg_videoram_size;

/***************************************************************************/

static int nmi_enabled = 0;

static void timelimt_init( void )
{
	soundlatch_setclearedvalue( 0 );
	nmi_enabled = 0;
}

static WRITE_HANDLER( nmi_enable_w )
{
	nmi_enabled = data & 1;	/* bit 0 = nmi enable */
}

static WRITE_HANDLER( sound_reset_w )
{
	if ( data & 1 )
		cpu_set_reset_line( 1, PULSE_LINE );
}


static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },		/* rom */
	{ 0x8000, 0x87ff, MRA_RAM },		/* ram */
	{ 0x8800, 0x8bff, MRA_RAM },		/* video ram */
	{ 0x9000, 0x97ff, MRA_RAM },		/* background ram */
	{ 0x9800, 0x98ff, MRA_RAM },		/* sprite ram */
	{ 0xa000, 0xa000, input_port_0_r }, /* input port */
	{ 0xa800, 0xa800, input_port_1_r },	/* input port */
	{ 0xb000, 0xb000, input_port_2_r },	/* DSW */
	{ 0xb800, 0xb800, MRA_NOP },		/* NMI ack? */
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0x7fff, MWA_ROM },		/* rom */
	{ 0x8000, 0x87ff, MWA_RAM },		/* ram */
	{ 0x8800, 0x8bff, timelimt_videoram_w, &videoram, &videoram_size },	/* video ram */
	{ 0x9000, 0x97ff, timelimt_bg_videoram_w, &timelimt_bg_videoram, &timelimt_bg_videoram_size },/* background ram */
	{ 0x9800, 0x98ff, MWA_RAM, &spriteram, &spriteram_size },	/* sprite ram */
	{ 0xb000, 0xb000, nmi_enable_w },	/* nmi enable */
	{ 0xb003, 0xb003, sound_reset_w },	/* sound reset ? */
	{ 0xb800, 0xb800, soundlatch_w }, 	/* sound write */
	{ 0xc800, 0xc800, timelimt_scroll_x_lsb_w },
	{ 0xc801, 0xc801, timelimt_scroll_x_msb_w },
	{ 0xc802, 0xc802, timelimt_scroll_y_w },
	{ 0xc803, 0xc803, MWA_NOP },		/* ???? bit 0 used only */
	{ 0xc804, 0xc804, MWA_NOP },		/* ???? not used */
MEMORY_END

static PORT_READ_START( readport )
	{ 0x00, 0x00, watchdog_reset_r },
PORT_END

static MEMORY_READ_START( readmem_sound )
	{ 0x0000, 0x1fff, MRA_ROM },	/* rom */
	{ 0x3800, 0x3bff, MRA_RAM },	/* ram */
MEMORY_END

static MEMORY_WRITE_START( writemem_sound )
	{ 0x0000, 0x1fff, MWA_ROM },	/* rom */
	{ 0x3800, 0x3bff, MWA_RAM },	/* ram */
MEMORY_END

static PORT_READ_START( readport_sound )
	{ 0x8c, 0x8d, AY8910_read_port_0_r },
	{ 0x8e, 0x8f, AY8910_read_port_1_r },
PORT_END

static PORT_WRITE_START( writeport_sound )
	{ 0x00, 0x00, soundlatch_clear_w },
	{ 0x8c, 0x8c, AY8910_control_port_0_w },
	{ 0x8d, 0x8d, AY8910_write_port_0_w },
	{ 0x8e, 0x8e, AY8910_control_port_1_w },
	{ 0x8f, 0x8f, AY8910_write_port_1_w },
PORT_END

/***************************************************************************/

INPUT_PORTS_START( timelimt )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused */

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x07, 0x01, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 1C_6C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_7C ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x20, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x40, DEF_STR( On ) )
	PORT_BITX(    0x80, 0x00, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invincibility", IP_KEY_NONE, IP_JOY_NONE )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END

/***************************************************************************/

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(0,2)+0, RGN_FRAC(0,2)+4, RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			16*8, 17*8, 18*8, 19*8, 20*8, 21*8, 22*8, 23*8 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   32, 1 },	/* seems correct */
	{ REGION_GFX2, 0, &charlayout,    0, 1 },	/* seems correct */
	{ REGION_GFX3, 0, &spritelayout,  0, 8 },	/* ?? */
	{ -1 } /* end of array */
};

/***************************************************************************/

static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	18432000/12,
	{ 25, 25 },
	{ 0, soundlatch_r },
	{ 0 },
	{ 0 },
	{ 0 }
};

static int timelimt_irq( void ) {
	if ( nmi_enabled )
		return nmi_interrupt();

	return ignore_interrupt();
}

/***************************************************************************/

static const struct MachineDriver machine_driver_timelimt =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			5000000,	/* 5.000 MHz */
			readmem,writemem,readport,0,
			timelimt_irq,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			18432000/6,	/* 3.072 MHz */
			readmem_sound,writemem_sound,readport_sound,writeport_sound,
			interrupt,1 /* ? */
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	50,
	timelimt_init,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 2*8, 30*8-1 },
	gfxdecodeinfo,
	64, 64,
	timelimt_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	timelimt_vh_start,
	generic_vh_stop,
	timelimt_vh_screenrefresh,

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

  Game ROM(s)

***************************************************************************/

ROM_START( timelimt )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* ROMs */
	ROM_LOAD( "t8",     0x0000, 0x2000, 0x006767ca )
	ROM_LOAD( "t7",     0x2000, 0x2000, 0xcbe7cd86 )
	ROM_LOAD( "t6",     0x4000, 0x2000, 0xf5f17e39 )
	ROM_LOAD( "t9",     0x6000, 0x2000, 0x2d72ab45 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* ROMs */
	ROM_LOAD( "tl5",    0x0000, 0x1000, 0x5b782e4a )
	ROM_LOAD( "tl4",    0x1000, 0x1000, 0xa32883a9 )

	ROM_REGION( 0x2000, REGION_GFX1, ROMREGION_DISPOSE )	/* tiles */
	ROM_LOAD( "tl11",   0x0000, 0x1000, 0x46676307 )
	ROM_LOAD( "tl10",   0x1000, 0x1000, 0x2336908a )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )	/* tiles */
	ROM_LOAD( "tl13",   0x0000, 0x1000, 0x072e4053 )
	ROM_LOAD( "tl12",   0x1000, 0x1000, 0xce960389 )

	ROM_REGION( 0x6000, REGION_GFX3, ROMREGION_DISPOSE )	/* sprites */
	ROM_LOAD( "tl3",    0x0000, 0x2000, 0x01a9fd95 )
	ROM_LOAD( "tl2",    0x2000, 0x2000, 0x4693b849 )
	ROM_LOAD( "tl1",    0x4000, 0x2000, 0xc4007caf )

	ROM_REGION( 0x0040, REGION_PROMS, 0 )
	ROM_LOAD( "clr.35", 0x0000, 0x0020, 0x9c9e6073 )
	ROM_LOAD( "clr.48", 0x0020, 0x0020, BADCRC( 0xa0bcac59 ) )	/* FIXED BITS (xxxxxx1x) */
ROM_END



GAMEX( 1983, timelimt, 0, timelimt, timelimt, 0, ROT90, "Chuo Co. Ltd", "Time Limit", GAME_IMPERFECT_COLORS )
