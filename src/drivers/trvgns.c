/*
	
  Trivia Genius

	driver by Pierpaolo Prazzoli

  TODO:
  - colors
  - sound
	
*/

#include "driver.h"
#include "vidhrdw/generic.h"

static UINT8 *bg_videoram , *fg_videoram;
static UINT8 low_offset, mid_offset, high_offset;
static struct tilemap *bg_tilemap, *fg_tilemap;

static WRITE8_HANDLER( trvgns_fg_w )
{
	fg_videoram[offset] = data;
	tilemap_mark_tile_dirty(fg_tilemap,offset);
}

static WRITE8_HANDLER( trvgns_bg_w )
{
	bg_videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap,offset);
}

static READ8_HANDLER( trvgns_questions_r )
{
	UINT8 *questions = memory_region(REGION_USER1);
	return questions[(high_offset << 16) | (mid_offset << 8) | low_offset];
}

static WRITE8_HANDLER( trvgns_questions_w )
{
	switch( offset )
	{
		case 0:
			low_offset = data;
			break;

		case 1:
			mid_offset = data;
			break;

		case 2:
			high_offset = data;
			break;
	}
}

static ADDRESS_MAP_START( trvgns_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0x4fff) AM_RAM
	AM_RANGE(0x5000, 0x7fff) AM_WRITENOP //always 0
	AM_RANGE(0x8000, 0x87ff) AM_READWRITE(MRA8_RAM,trvgns_fg_w) AM_BASE(&fg_videoram)
	AM_RANGE(0x8800, 0x8fff) AM_READWRITE(MRA8_RAM,trvgns_bg_w) AM_BASE(&bg_videoram)
	AM_RANGE(0x9000, 0x9fff) AM_WRITENOP //$9000 always 1 - $9800 always 0
	AM_RANGE(0xa000, 0xa000) AM_WRITENOP //0x0e or 0x0f
	AM_RANGE(0xa800, 0xa800) AM_WRITENOP //sound (probably some kind of DAC)
	AM_RANGE(0xb000, 0xb000) AM_READ(input_port_0_r)
	AM_RANGE(0xb800, 0xb800) AM_READ(input_port_1_r)
	AM_RANGE(0xc000, 0xc000) AM_READ(trvgns_questions_r)
	AM_RANGE(0xc000, 0xc002) AM_WRITE(trvgns_questions_w)
ADDRESS_MAP_END

INPUT_PORTS_START( trvgns )
	PORT_START
	PORT_SERVICE(0x0f, IP_ACTIVE_LOW )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Screen orientation" )
	PORT_DIPSETTING(    0x01, "Horizontal" )
	PORT_DIPSETTING(    0x00, "Vertical" )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "Show Correct Answer" )
	PORT_DIPSETTING(    0x08, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_VBLANK )
INPUT_PORTS_END

static struct GfxLayout charlayout8x8x2 =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout charlayout8x8x3 =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ 7, 6, 5, 4, 3, 2, 1, 0 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout8x8x3, 0, 8 },
	{ REGION_GFX2, 0, &charlayout8x8x2, 0, 4 },
	{ -1 }
};

static void get_tile_info_bg(int tile_index)
{
	SET_TILE_INFO(0,bg_videoram[tile_index],0,0)
}

static void get_tile_info_fg(int tile_index)
{
	SET_TILE_INFO(1,fg_videoram[tile_index],0,0)
}

VIDEO_START( trvgns )
{
	bg_tilemap = tilemap_create(get_tile_info_bg,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,64,32);
	fg_tilemap = tilemap_create(get_tile_info_fg,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);

	if( !bg_tilemap || !fg_tilemap )
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);

	return 0;
}

VIDEO_UPDATE( trvgns )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);
}

INTERRUPT_GEN( trvgns_interrupt )
{
	if( ~readinputport(1) & 0x10 || ~readinputport(1) & 0x20 )
	{
		cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
	}
}

static MACHINE_DRIVER_START( trvgns )
	MDRV_CPU_ADD(Z80, 6000000) /* ? */
	MDRV_CPU_PROGRAM_MAP(trvgns_map,0)
	MDRV_CPU_VBLANK_INT(trvgns_interrupt,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(8)

	MDRV_VIDEO_START(trvgns)
	MDRV_VIDEO_UPDATE(trvgns)

	/* sound hardware */
	/* DAC ? */
MACHINE_DRIVER_END

ROM_START( trvgns )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	
	ROM_LOAD( "trvgns.30",   0x0000, 0x1000, CRC(a17f172c) SHA1(b831673f860f6b7566e248b13b349d82379b5e72) )
	ROM_LOAD( "trvgns.28",   0x1000, 0x1000, CRC(681a1bff) SHA1(53da179185ae3bfb30502706cc623c2f4cc57128) )
	ROM_LOAD( "trvgns.26",   0x2000, 0x1000, CRC(5b4068b8) SHA1(3b424dd8e2a6fa1e4628790f60c51d44f9a535a1) )

	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "trvgns.44",   0x0000, 0x1000, CRC(cd67f2cb) SHA1(22d9d8509fd44fbeb313f5120e692d7a30e3ca54) )
	ROM_LOAD( "trvgns.46",   0x1000, 0x1000, CRC(f4021941) SHA1(81a93b5b2bf46e2f5254a86b14e31b31b7821d4f) )
	ROM_LOAD( "trvgns.48",   0x2000, 0x1000, NO_DUMP )

	ROM_REGION( 0x1000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "trvgns.50",   0x0000, 0x1000, CRC(ac292be8) SHA1(41f95273907b27158af0631c716fdb9301852e27) )

	ROM_REGION( 0x20000, REGION_USER1, 0 ) /* Question roms */
	ROM_LOAD( "trvgns.u2",   0x00000, 0x4000, CRC(109bd359) SHA1(ea8cb4b0a14a3ef4932947afdfa773ecc34c2b9b) )
	ROM_LOAD( "trvgns.u1",   0x04000, 0x4000, CRC(8e8b5f71) SHA1(71514af2af2468a13cf5cc4237fa2590d7a16b27) )
	ROM_LOAD( "trvgns.u4",   0x08000, 0x4000, CRC(b73f2e31) SHA1(4390152e053118c31ed74fe850ea7124c0e7b731) )
	ROM_LOAD( "trvgns.u3",   0x0c000, 0x4000, CRC(bf654110) SHA1(5229f5e6973a04c53572ea94c14d79a238c0e90f) )
	ROM_LOAD( "trvgns.u6",   0x10000, 0x4000, CRC(4a2263a7) SHA1(63f2f79261d508c9bba3d73d78f7dce5d348b6d4) )
	ROM_LOAD( "trvgns.u5",   0x14000, 0x4000, CRC(bd31f382) SHA1(ec04a5d4a5fc8be059abf3c21c65cd970e569d44) )
	ROM_LOAD( "trvgns.u8",   0x18000, 0x4000, CRC(dbfce45f) SHA1(5d96186c96dee810b0ef63964cb3614fd486aefa) )
	ROM_LOAD( "trvgns.u7",   0x1c000, 0x4000, CRC(c8f5a02d) SHA1(8a566f83f9bd39ab508085af942957a7ed941813) )
ROM_END

GAMEX( 19??, trvgns, 0, trvgns, trvgns, 0, ROT0, "<unknown>", "Trivia Genius", GAME_NO_SOUND | GAME_WRONG_COLORS )
