/***************************************************************************

Mahjong Kyou Jidai     (c)1986 Sanritsu

CPU	:Z80
Sound	:SN76489*2 CUSTOM
OSC	:10MHz ??MHz

driver by Nicola Salmoria

TODO:
- Dip switches.

- Several imperfections with sprites rendering:
  - some sprites are misplaced by 1pixel vertically
  - during the tile distribution at the beginning of a match, there's something
    wrong with the stacks moved around, they aremisaligned and something is
	missing.

- unknown reads from port 01. Only the top two bits seem to be used.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern data8_t *mjkjidai_videoram;

VIDEO_START( mjkjidai );
VIDEO_UPDATE( mjkjidai );
WRITE_HANDLER( mjkjidai_videoram_w );
WRITE_HANDLER( mjkjidai_ctrl_w );



static WRITE_HANDLER( adpcm_w )
{
	ADPCM_play(0,(data & 0x07) * 0x1000,0x1000*2);
}


static int keyb,nvram_init_count;

static READ_HANDLER( keyboard_r )
{
	int res = 0x3f,i;

//	logerror("%04x: keyboard_r\n",activecpu_get_pc());

	for (i = 0;i < 12;i++)
	{
		if (~keyb & (1 << i))
		{
			res = readinputport(4+i) & 0x3f;
			break;
		}
	}

	res |= (readinputport(3) & 0xc0);

	if (nvram_init_count)
	{
		nvram_init_count--;
		res &= 0xbf;
	}

	return res;
}

static WRITE_HANDLER( keyboard_select_w )
{
//	logerror("%04x: keyboard_select %d = %02x\n",activecpu_get_pc(),offset,data);

	switch (offset)
	{
		case 0: keyb = (keyb & 0xff00) | (data);      break;
		case 1: keyb = (keyb & 0x00ff) | (data << 8); break;
	}
}

static data8_t *nvram;
static size_t nvram_size;

void nvram_handler_mjkjidai(mame_file *file, int read_or_write)
{
	if (read_or_write)
		mame_fwrite(file, nvram, nvram_size);
	else if (file)
		mame_fread(file, nvram, nvram_size);
	else
	{
		nvram_init_count = 1;
	}
}



static MEMORY_READ_START( readmem )
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xdfff, MRA_RAM },
	{ 0xe000, 0xf7ff, MRA_RAM },
MEMORY_END

static MEMORY_WRITE_START( writemem )
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xdfff, MWA_RAM, &nvram, &nvram_size },	// cleared and initialized on startup if bit 6 if port 00 is 0
	{ 0xe000, 0xe01f, MWA_RAM, &spriteram },	// shared with tilemap ram
	{ 0xe800, 0xe81f, MWA_RAM, &spriteram_2 },	// shared with tilemap ram
	{ 0xf000, 0xf01f, MWA_RAM, &spriteram_3 },	// shared with tilemap ram
	{ 0xe000, 0xf7ff, mjkjidai_videoram_w, &mjkjidai_videoram },
MEMORY_END

static PORT_READ_START( readport )
	{ 0x00, 0x00, keyboard_r },
	{ 0x01, 0x01, IORP_NOP },	// ???
	{ 0x02, 0x02, input_port_2_r },
	{ 0x11, 0x11, input_port_0_r },
	{ 0x12, 0x12, input_port_1_r },
MEMORY_END

static PORT_WRITE_START( writeport )
	{ 0x01, 0x02, keyboard_select_w },
	{ 0x10, 0x10, mjkjidai_ctrl_w },	// rom bank, coin counter, flip screen etc
	{ 0x20, 0x20, SN76496_0_w },
	{ 0x30, 0x30, SN76496_1_w },
	{ 0x40, 0x40, adpcm_w },
MEMORY_END



INPUT_PORTS_START( mjkjidai )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_PLAYER2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_PLAYER2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_PLAYER2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON4 | IPF_PLAYER2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SERVICE )	// service mode
	PORT_DIPNAME( 0x20, 0x20, "Statistics" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START4 )

	PORT_START
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_TILT )	// reinitialize NVRAM and reset the game
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )

	/* player 2 inputs (same as player 1) */
	PORT_START
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0x3f, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x3e, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "Kan",   KEYCODE_LCONTROL, IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "Reach", KEYCODE_LSHIFT,   IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "Ron",   KEYCODE_Z,        IP_JOY_NONE )
	PORT_BIT( 0x38, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "M",   KEYCODE_M,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "N",   KEYCODE_N,        IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "Chi", KEYCODE_SPACE,    IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "Pon", KEYCODE_LALT,     IP_JOY_NONE )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "I",     KEYCODE_I,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "J",     KEYCODE_J,        IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "K",     KEYCODE_K,        IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "L",     KEYCODE_L,        IP_JOY_NONE )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "E",     KEYCODE_E,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "F",     KEYCODE_F,        IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "G",     KEYCODE_G,        IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "H",     KEYCODE_H,        IP_JOY_NONE )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BITX(0x01, IP_ACTIVE_LOW, 0, "A",     KEYCODE_A,        IP_JOY_NONE )
	PORT_BITX(0x02, IP_ACTIVE_LOW, 0, "B",     KEYCODE_B,        IP_JOY_NONE )
	PORT_BITX(0x04, IP_ACTIVE_LOW, 0, "C",     KEYCODE_C,        IP_JOY_NONE )
	PORT_BITX(0x08, IP_ACTIVE_LOW, 0, "D",     KEYCODE_D,        IP_JOY_NONE )
	PORT_BIT( 0x30, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
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
	{ REGION_GFX1, 0, &charlayout,   0, 32 },
	{ REGION_GFX1, 0, &spritelayout, 0, 16 },
	{ -1 } /* end of array */
};



static struct SN76496interface sn76496_interface =
{
	2,	/* 2 chips */
	{ 10000000/4, 10000000/4, 10000000/4 },	/* 2.5 MHz ??? */
	{ 50, 50 }
};

static struct ADPCMinterface adpcm_interface =
{
	1,          	/* 1 channel */
	6000,       	/* 6000Hz playback */
	REGION_SOUND1,	/* memory region */
	{ 100 }
};



static MACHINE_DRIVER_START( mjkjidai )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,10000000/2)	/* 5 MHz ??? */
	MDRV_CPU_MEMORY(readmem,writemem)
	MDRV_CPU_PORTS(readport,writeport)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(mjkjidai)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER|VIDEO_PIXEL_ASPECT_RATIO_1_2)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(3*8, 61*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)
	MDRV_VIDEO_START(mjkjidai)
	MDRV_VIDEO_UPDATE(mjkjidai)

	/* sound hardware */
	MDRV_SOUND_ADD(SN76496, sn76496_interface)
	MDRV_SOUND_ADD(ADPCM, adpcm_interface)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( mjkjidai )
	ROM_REGION( 0x1c000, REGION_CPU1, 0 )
	ROM_LOAD( "mkj-00.14g",   0x00000, 0x8000, 0x188a27e9 )
	ROM_LOAD( "mkj-01.15g",   0x08000, 0x4000, 0xa6a5e9c7 )	/* banked, there is code flowing from 7fff to this bank */
	ROM_CONTINUE(             0x10000, 0x4000 )
	ROM_LOAD( "mkj-02.16g",   0x14000, 0x8000, 0xfb312927 )

	ROM_REGION( 0x30000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mkj-20.4e",    0x00000, 0x8000, 0x8fc66bce )
	ROM_LOAD( "mkj-21.5e",    0x08000, 0x8000, 0x4dd41a9b )
	ROM_LOAD( "mkj-22.6e",    0x10000, 0x8000, 0x70ac2bd7 )
	ROM_LOAD( "mkj-23.7e",    0x18000, 0x8000, 0xf9313dde )
	ROM_LOAD( "mkj-24.8e",    0x20000, 0x8000, 0xaa5130d0 )
	ROM_LOAD( "mkj-25.9e",    0x28000, 0x8000, 0xc12c3fe0 )

	ROM_REGION( 0x0300, REGION_PROMS, ROMREGION_DISPOSE )
	ROM_LOAD( "mkj-60.13a",   0x0000, 0x0100, 0x5dfaba60 )
	ROM_LOAD( "mkj-61.14a",   0x0100, 0x0100, 0xe9e90d55 )
	ROM_LOAD( "mkj-62.15a",   0x0200, 0x0100, 0x934f1d53 )

	ROM_REGION( 0x8000, REGION_SOUND1, 0 )	/* ADPCM samples */
	ROM_LOAD( "mkj-40.14c",   0x00000, 0x8000, 0x4d8fcc4a )
ROM_END


GAMEX( 1986, mjkjidai, 0, mjkjidai, mjkjidai, 0, ROT0, "Sanritsu",  "Mahjong Kyou Jidai (Japan)", GAME_IMPERFECT_GRAPHICS )
