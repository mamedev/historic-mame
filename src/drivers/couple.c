/*******************************************************************************************

The Couples (c) 1988 ??

Preliminary driver by Angelo Salese

TODO:
-ROM banking
-Finish graphics
-Inputs
-Colors
-Sound

********************************************************************************************

Main CPU : Z80B CPU

Sound : AY3-8910 + 2 x Nec D8255AC-2

Other : MC6845P

Battery backup ram (6264)

Provided to you by Thierry (ShinobiZ) & Gerald (COY)

*******************************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static struct tilemap *bg_tilemap,*tx_tilemap;
static UINT8 *bg_vram,*tx_vram;

static void get_bg_tile_info(int tile_index)
{
	int code;
	code = bg_vram[tile_index];
//  color = colorram[tile_index] & 0x0f;
//  region = (colorram[tile_index] & 0x10) >> 4;

	SET_TILE_INFO(0, code, 0, 0)
}

static void get_tx_tile_info(int tile_index)
{
	int code;
	code = tx_vram[tile_index]*2;
//  color = colorram[tile_index] & 0x0f;
//  region = (colorram[tile_index] & 0x10) >> 4;

	SET_TILE_INFO(1, code, 0, 0)
}

VIDEO_START( couple )
{
	bg_tilemap = tilemap_create(get_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,64,32);
	tx_tilemap = tilemap_create(get_tx_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,64,32);

	tilemap_set_transparent_pen(tx_tilemap,0);

	return 0;
}

VIDEO_UPDATE( couple )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,tx_tilemap,0,0);
}

static WRITE8_HANDLER( couple_bgvram_w )
{
	bg_vram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap,offset);
}

static WRITE8_HANDLER( couple_txvram_w )
{
	tx_vram[offset] = data;
	tilemap_mark_tile_dirty(tx_tilemap,offset);
}


static ADDRESS_MAP_START( mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE( 0x0000, 0x7fff ) AM_ROM
	AM_RANGE( 0x8000, 0x9fff ) AM_READWRITE(MRA8_BANK1,MWA8_BANK1)
	AM_RANGE( 0xa000, 0xbfff ) AM_RAM
	AM_RANGE( 0xc008, 0xc008 ) AM_READ(input_port_0_r)
	AM_RANGE( 0xc00a, 0xc00a ) AM_READ(input_port_1_r)
	AM_RANGE( 0xe800, 0xefff ) AM_READWRITE(MRA8_RAM, couple_bgvram_w) AM_BASE(&bg_vram)
	AM_RANGE( 0xf000, 0xf7ff ) AM_READWRITE(MRA8_RAM, couple_txvram_w) AM_BASE(&tx_vram)
	AM_RANGE( 0xf800, 0xfbff ) AM_RAM
ADDRESS_MAP_END

INPUT_PORTS_START( couple )
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
INPUT_PORTS_END


/*These two might be wrong,untested...*/
static struct GfxLayout tilelayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(2,3), RGN_FRAC(1,3), RGN_FRAC(0,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4),RGN_FRAC(1,4),RGN_FRAC(2,4),RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout,   0, 16 },
	{ REGION_GFX2, 0, &charlayout,   0, 16 },
	{ -1 }
};

static MACHINE_DRIVER_START( couple )
	MDRV_CPU_ADD(Z80,18432000/3)		 /* ?? */
	MDRV_CPU_PROGRAM_MAP(mem,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 32*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(couple)
	MDRV_VIDEO_UPDATE(couple)
MACHINE_DRIVER_END

ROM_START( couple )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "thecoupl.001", 0x00000, 0x8000, CRC(bc70337a) SHA1(ffc484bc3965f0780d3fa5d8801af27a7164a417) )
	ROM_LOAD( "thecoupl.002", 0x10000, 0x8000, CRC(17372a93) SHA1(e0f0980003473555c2543d98d1494f82afa49f1a) )

	ROM_REGION( 0x18000, REGION_GFX1, 0 )
	ROM_LOAD( "thecoupl.003", 0x00000, 0x8000, CRC(f017399a) SHA1(baf4c1bea6a12b1d4c8838552503fbdb81378411) )
	ROM_LOAD( "thecoupl.004", 0x08000, 0x8000, CRC(66da76c1) SHA1(8cdcec008d0d51704544069246e9eabb5d5958ea) )
	ROM_LOAD( "thecoupl.005", 0x10000, 0x8000, CRC(fc22bcf4) SHA1(cf3f6872965cb264d56d3a0b5ab998541b9af4ef) )

	ROM_REGION( 0x08000, REGION_GFX2, 0 )
	ROM_LOAD( "thecoupl.006", 0x00000, 0x8000, CRC(a6a9a73d) SHA1(f3cb1d434d730f6e00f48079eaf8b88f57779fa0) )

	ROM_REGION( 0x0800, REGION_PROMS, 0 )
	ROM_LOAD( "thecoupl.007", 0x00000, 0x0800, CRC(6c36361e) SHA1(7a018eecf3d8b7cf8845dcfcf8067feb292933b2) )
ROM_END

READ8_HANDLER( dummy_r )
{
	return 0;
}

static DRIVER_INIT( couple )
{
	UINT8 *ROM = memory_region(REGION_CPU1);
	UINT8 data = 2;

	/*For now load the default ROM.*/
	cpu_setbank(1,ROM + 0x10000 + 0x2000 * data);
	/*Disable call $b011 for now...*/
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa095,0xa095,0,0, dummy_r );
	memory_install_write8_handler(0, ADDRESS_SPACE_PROGRAM, 0xa095, 0xa095,0,0, MWA8_NOP);
}

GAMEX( 1988?, couple, 0, couple, couple, couple, ROT0, "?", "The Couples", GAME_NO_SOUND | GAME_NOT_WORKING)
