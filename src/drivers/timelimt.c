/***************************************************************************

Time Limit (c) 1983 Chuo

driver by Ernesto Corvi

Notes:
- Sprite colors are wrong (missing colortable?)

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/* from vidhrdw */
extern VIDEO_START( timelimt );
extern PALETTE_INIT( timelimt );
extern VIDEO_UPDATE( timelimt );

extern WRITE_HANDLER( timelimt_videoram_w );
extern WRITE_HANDLER( timelimt_bg_videoram_w );
extern WRITE_HANDLER( timelimt_scroll_y_w );
extern WRITE_HANDLER( timelimt_scroll_x_msb_w );
extern WRITE_HANDLER( timelimt_scroll_x_lsb_w );

extern data8_t *timelimt_bg_videoram;
extern size_t timelimt_bg_videoram_size;

/***************************************************************************/

static int nmi_enabled = 0;

static MACHINE_INIT( timelimt )
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

static INTERRUPT_GEN( timelimt_irq ) {
	if ( nmi_enabled )
		cpu_set_irq_line(0, IRQ_LINE_NMI, PULSE_LINE);
}

/***************************************************************************/

static MACHINE_DRIVER_START( timelimt )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 5000000)	/* 5.000 MHz */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,0)
	MDRV_CPU_VBLANK_INT(timelimt_irq,1)

	MDRV_CPU_ADD(Z80,18432000/6)
	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)	/* 3.072 MHz */
	MDRV_CPU_MEMORY(readmem_sound,writemem_sound)
	MDRV_CPU_PORTS(readport_sound,writeport_sound)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1) /* ? */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(50)

	MDRV_MACHINE_INIT(timelimt)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(64)
	MDRV_COLORTABLE_LENGTH(64)

	MDRV_PALETTE_INIT(timelimt)
	MDRV_VIDEO_START(timelimt)
	MDRV_VIDEO_UPDATE(timelimt)

	/* sound hardware */
	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END


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
