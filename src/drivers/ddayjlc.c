/*
	D-DAY
	(c)Jaleco 1984

	CPU  : Z80
	Sound: Z80 AY-3-8910(x2)
	OSC  : 12.000MHz

	-------
	DD-8416
	-------
	ROMs:
	1  - (2764)
	2  |
	3  |
	4  |
	5  |
	6  |
	7  |
	8  |
	9  |
	10 |
	11 /

	-------
	DD-8417
	-------
	ROMs:
	12 - (2732)
	13 /

	14 - (2732)
	15 /

	16 - (2764)
	17 |
	18 |
	19 /


	preliminary driver by Pierpaolo Prazzoli

*/

#include "driver.h"
#include "vidhrdw/generic.h"

static int char_bank = 0;
static struct tilemap *bg_tilemap, *fg_tilemap;
static data8_t *bgram;

static WRITE8_HANDLER( char_bank_w )
{
	char_bank = data;
	tilemap_mark_all_tiles_dirty(fg_tilemap);
}

static WRITE8_HANDLER( ddayjlc_bgram_w )
{
	if( bgram[offset] != data )
	{
		bgram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap,offset);
	}
}

static WRITE8_HANDLER( ddayjlc_videoram_w )
{
	if( videoram[offset] != data )
	{
		videoram[offset] = data;
		tilemap_mark_tile_dirty(fg_tilemap,offset);
	}
}

static ADDRESS_MAP_START( main_cpu, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x8fff) AM_RAM
	AM_RANGE(0x9400, 0x97ff) AM_READWRITE(videoram_r, ddayjlc_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x9800, 0x9fff) AM_READWRITE(MRA8_RAM, ddayjlc_bgram_w) AM_BASE(&bgram)
	AM_RANGE(0xa000, 0xbfff) AM_ROM AM_REGION(REGION_USER1, 0x6000) //banked? -> it seems to read bgdata from here
	AM_RANGE(0xe000, 0xe003) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xe008, 0xe008) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xf000, 0xf000) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xf080, 0xf080) AM_WRITE(char_bank_w)
	AM_RANGE(0xf081, 0xf081) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xf083, 0xf083) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xf084, 0xf084) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xf085, 0xf085) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xf086, 0xf086) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xf100, 0xf100) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xf101, 0xf101) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xf102, 0xf102) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xf103, 0xf103) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xf104, 0xf104) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xf105, 0xf105) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xf000, 0xf000) AM_READ(input_port_0_r)
	AM_RANGE(0xf100, 0xf100) AM_READ(input_port_1_r)
	AM_RANGE(0xf180, 0xf180) AM_READ(input_port_2_r)
	AM_RANGE(0xf200, 0xf200) AM_READ(input_port_3_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_cpu, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0x23ff) AM_RAM
//	AM_RANGE(0x3000, 0x3000) AM_READ(soundlatch_r)
ADDRESS_MAP_END

INPUT_PORTS_START( ddayjlc )
	PORT_START
	PORT_DIPNAME( 0xff, 0x00, "0" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0xff, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START	/* DIPSW-1 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x01, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_DIPSETTING(    0x03, "6" )
	PORT_DIPNAME( 0x1c, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x20, 0x00, "Extend" )
	PORT_DIPSETTING(    0x00, "30K" )
	PORT_DIPSETTING(    0x20, "50K" )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Cocktail ) )

	PORT_START	/* DIPSW-2 */
	PORT_DIPNAME( 0x07, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xf8, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0xf8, DEF_STR( On ) )

INPUT_PORTS_END

static struct GfxLayout charlayout =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2), RGN_FRAC(1,2) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8
};

static struct GfxLayout spritelayout =
{
	16,16,	
	RGN_FRAC(1,4),
	4,	
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*16,8*16+1,8*16+2,8*16+3,8*16+4,8*16+5,8*16+6,8*16+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
            8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &spritelayout,   0, 16 }, /* wrong? */
	{ REGION_GFX2, 0, &charlayout,     0, 16 },
	{ REGION_GFX3, 0, &charlayout,     0, 16 },
	{ -1 }
};

static void get_tile_info_bg(int tile_index)
{
	int code = bgram[tile_index];

	SET_TILE_INFO(2, code, 0, 0)
}

static void get_tile_info_fg(int tile_index)
{
	int code = videoram[tile_index];

	SET_TILE_INFO(1, code + char_bank * 0x100, 0, 0)
}

VIDEO_START( ddayjlc )
{	
	bg_tilemap = tilemap_create(get_tile_info_bg,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32,64);
	fg_tilemap = tilemap_create(get_tile_info_fg,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);

	if( !bg_tilemap || !fg_tilemap )
		return 1;

	tilemap_set_transparent_pen(fg_tilemap,0);

	return 0;
}

VIDEO_UPDATE( ddayjlc )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,fg_tilemap,0,0);

}

static struct AY8910interface ay8910_interface =
{
	2,  /* 2 chips */
	12000000/2,
	{ 100, 100 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 }
};

static INTERRUPT_GEN( ddayjlc_interrupt )
{
//	maskable?
	cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}

static MACHINE_DRIVER_START( ddayjlc )
	MDRV_CPU_ADD(Z80,12000000/2)		 /* ?? */
	MDRV_CPU_PROGRAM_MAP(main_cpu,0)
	MDRV_CPU_VBLANK_INT(ddayjlc_interrupt,1)

//	MDRV_CPU_ADD(Z80, 12000000/2)     /* ? MHz */
//	MDRV_CPU_FLAGS(CPU_AUDIO_CPU)
//	MDRV_CPU_PROGRAM_MAP(sound_cpu,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)	
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)

	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)

	MDRV_VIDEO_START(ddayjlc)
	MDRV_VIDEO_UPDATE(ddayjlc)

	MDRV_SOUND_ADD(AY8910, ay8910_interface)
MACHINE_DRIVER_END


ROM_START( ddayjlc )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1",  0x0000, 0x2000, CRC(dbfb8772) SHA1(1fbc9726d0cd1f8781ced2f8233107b65b9bdb1a) )
	ROM_LOAD( "2",  0x2000, 0x2000, CRC(f40ea53e) SHA1(234ef686d3e9fe12aceada7098c4cc53e56eb1a3) )
	ROM_LOAD( "3",  0x4000, 0x2000, CRC(0780ef60) SHA1(9247af38acbaea0f78892fc50081b2400abbdc1f) )
	ROM_LOAD( "4",  0x6000, 0x2000, CRC(75991a24) SHA1(175f505da6eb80479a70181d6aed01130f6a64cc) )
		
	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "11", 0x0000, 0x2000, CRC(fe4de019) SHA1(16c5402e1a79756f8227d7e99dd94c5896c57444) )

	ROM_REGION( 0x8000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "16", 0x0000, 0x2000, CRC(a167fe9a) SHA1(f2770d93ee5ae4eb9b3bcb052e14e36f53eec707) )
	ROM_LOAD( "17", 0x2000, 0x2000, CRC(13ffe662) SHA1(2ea7855a14a4b8429751bae2e670e77608f93406) )
	ROM_LOAD( "18", 0x4000, 0x2000, CRC(debe6531) SHA1(34b3b70a1872527266c664b2a82014d028a4ff1e) )
	ROM_LOAD( "19", 0x6000, 0x2000, CRC(5816f947) SHA1(2236bed3e82980d3e7de3749aef0fbab042086e6) )

	ROM_REGION( 0x2000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "14", 0x0000, 0x1000, CRC(2c0e9bbe) SHA1(e34ab774d2eb17ddf51af513dbcaa0c51f8dcbf7) )
	ROM_LOAD( "15", 0x1000, 0x1000, CRC(a6eeaa50) SHA1(052cd3e906ca028e6f55d0caa1e1386482684cbf) )

	ROM_REGION( 0x2000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "12", 0x0000, 0x1000, CRC(7f7afe80) SHA1(e8a549b8a8985c61d3ba452e348414146f2bc77e) )
	ROM_LOAD( "13", 0x1000, 0x1000, CRC(f169b93f) SHA1(fb0617162542d688503fc6618dd430308e259455) )

	/* background data? */
	ROM_REGION( 0x0c000, REGION_USER1, 0 )
	ROM_LOAD( "5",  0x0000, 0x2000, CRC(299b05f2) SHA1(3c1804bccb514bada4bed68a6af08db63a8f1b19) )
	ROM_LOAD( "6",  0x2000, 0x2000, CRC(38ae2616) SHA1(62c96f32532f0d7e2cf1606a303d81ebb4aada7d) ) // 1xxxxxxxxxxxx = 0xFF
	ROM_LOAD( "7",  0x4000, 0x2000, CRC(4210f6ef) SHA1(525d8413afabf97cf1d04ee9a3c3d980b91bde65) )
	ROM_LOAD( "8",  0x6000, 0x2000, CRC(91a32130) SHA1(cbcd673b47b672b9ce78c7354dacb5964a81db6f) )
	ROM_LOAD( "9",  0x8000, 0x2000, CRC(ccb82f09) SHA1(37c23f13aa0728bae82dba9e2858a8d81fa8afa5) ) // FIXED BITS (xx00xxxx)
	ROM_LOAD( "10", 0xa000, 0x2000, CRC(5452aba1) SHA1(03ef47161d0ab047c8675d6ffd3b7acf81f74721) ) // FIXED BITS (1xxxxxxx)
ROM_END


static DRIVER_INIT( ddayjlc )
{
#if 0
	UINT8 *ROM = memory_region(REGION_CPU1);

	//one protection patch
	ROM[0x7e5e] = 0;
	ROM[0x7e5f] = 0;
	ROM[0x7e60] = 0;
#endif
}

GAMEX( 1984, ddayjlc, 0, ddayjlc, ddayjlc, ddayjlc, ROT90, "Jaleco", "D-Day (Jaleco)", GAME_NOT_WORKING | GAME_NO_SOUND | GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_GRAPHICS | GAME_NO_COCKTAIL )
