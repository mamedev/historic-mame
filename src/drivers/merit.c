/*

  Merit trivia games

  preliminary driver by Pierpaolo Prazzoli

  TODO:
  - find out address of the question rom they want to read
  - finish video emulation
  - inputs
  - sound

Note: it's important that REGION_USER1 is 0xa0000 bytes with empty space filled
      with 0xff, because the built-in roms test checks how many question roms
      the games has and the type of each one.
      The  type is stored in one byte in an offset which change for every game
      (it's the offset stored in rom_type variable).

      Rom type byte legend:
      0 -> 0x02000 bytes rom
      1 -> 0x04000 bytes rom
      2 -> 0x08000 bytes rom
      3 -> 0x10000 bytes rom

*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/random.h"
#include "sound/ay8910.h"

static struct tilemap *bg_tilemap;
static data8_t *phrcraze_attr;
static int rom_type;
static int byte_low = 0, byte_high = 0;

static READ8_HANDLER( questions_r )
{
	static int howmany = 0;
	static int prev = 0;

	data8_t *questions = memory_region(REGION_USER1);
	int address;

	/* the question rom read depends on the offset */
	switch(offset)
	{
		case 0x30:
			address = 0x00000;
			break;
		case 0x38:
			address = 0x10000;
			break;
		case 0x34:
			address = 0x20000;
			break;
		case 0x3c:
			address = 0x30000;
			break;
		case 0x32:
			address = 0x40000;
			break;
		case 0x3a:
			address = 0x50000;
			break;
		case 0x36:
			address = 0x60000;
			break;
		case 0x3e:
			address = 0x70000;
			break;
		case 0x21:
			address = 0x80000;
			break;
		case 0x11:
			address = 0x90000;
			break;
		default:
			address = 0;
			logerror("read unknown offset: %02X\n",offset);
			break;
	}

	if(prev == offset)
		howmany++;
	else
		howmany = 1;

	prev = offset;

	if(byte_low == 0x100)
		address = address | rom_type;
	else
		address = address | (byte_high << 8) | byte_low;

	logerror("read from %04X (%04X) addr = %X PC = %X\n",offset,howmany,address,activecpu_get_pc());

	//address is wrong!
	return questions[address];
}

static READ8_HANDLER( fake_r )
{
	return mame_rand() & 0xff;
}

WRITE8_HANDLER( phrcraze_attr_w )
{
	phrcraze_attr[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap,offset);
}

WRITE8_HANDLER( phrcraze_bg_w )
{
	videoram[offset] = data;
	tilemap_mark_tile_dirty(bg_tilemap,offset);
}

WRITE8_HANDLER( low_offset_w )
{
	if( offset == 0xff && data == 0x80 )
	{
		byte_low = 0x100;
	}
	else
//      byte_low = offset;
		byte_low = data;

	logerror("low:  offset = %02X data = %02X\n",offset,data);
}

WRITE8_HANDLER( high_offset_w )
{
	byte_high = offset;
//  byte_high = data;
	logerror("high: offset = %02X data = %02X\n",offset,data);
}

static ADDRESS_MAP_START( phrcraze_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xa000, 0xbfff) AM_RAM
	AM_RANGE(0xc008, 0xc008) AM_READ(input_port_0_r)
	AM_RANGE(0xc00c, 0xc00c) AM_READ(input_port_1_r)
	AM_RANGE(0xc00d, 0xc00d) AM_READ(input_port_2_r)
	AM_RANGE(0xc00e, 0xc00e) AM_READ(input_port_3_r)
	AM_RANGE(0xce00, 0xceff) AM_WRITENOP
	AM_RANGE(0xce00, 0xce3f) AM_READ(questions_r)
	AM_RANGE(0xd600, 0xd6ff) AM_WRITE(low_offset_w)
	AM_RANGE(0xda00, 0xdaff) AM_WRITE(high_offset_w)
	AM_RANGE(0xe000, 0xe001) AM_WRITENOP
	AM_RANGE(0xe800, 0xefff) AM_READWRITE(MRA8_RAM, phrcraze_attr_w) AM_BASE(&phrcraze_attr)
	AM_RANGE(0xf000, 0xf7ff) AM_READWRITE(MRA8_RAM, phrcraze_bg_w) AM_BASE(&videoram)
	AM_RANGE(0xf800, 0xfbff) AM_RAM // not read
ADDRESS_MAP_END

static ADDRESS_MAP_START( phrcraze_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x04, 0x04) AM_WRITENOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( tictac_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0x9fff) AM_RAM
	AM_RANGE(0xc004, 0xc004) AM_READ(fake_r)
	AM_RANGE(0xc005, 0xc005) AM_READ(fake_r)
	AM_RANGE(0xc006, 0xc006) AM_READ(fake_r)
	AM_RANGE(0xc008, 0xc008) AM_READ(fake_r)
	AM_RANGE(0xce00, 0xceff) AM_WRITENOP
	AM_RANGE(0xce00, 0xceff) AM_READ(questions_r)
	AM_RANGE(0xd600, 0xd6ff) AM_WRITE(low_offset_w)
	AM_RANGE(0xda00, 0xdaff) AM_WRITE(high_offset_w)
	AM_RANGE(0xe000, 0xe001) AM_WRITENOP
	AM_RANGE(0xe800, 0xefff) AM_READWRITE(MRA8_RAM, phrcraze_attr_w) AM_BASE(&phrcraze_attr)
	AM_RANGE(0xf000, 0xf7ff) AM_READWRITE(MRA8_RAM, phrcraze_bg_w) AM_BASE(&videoram)
	AM_RANGE(0xf800, 0xfbff) AM_RAM // not read
ADDRESS_MAP_END

static ADDRESS_MAP_START( tictac_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x0c, 0x0c) AM_WRITENOP
ADDRESS_MAP_END


static ADDRESS_MAP_START( trvwhiz_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x3fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x4c00, 0x4cff) AM_WRITENOP
	AM_RANGE(0x4c00, 0x4cff) AM_READ(questions_r)
	AM_RANGE(0x5400, 0x54ff) AM_WRITE(low_offset_w)
	AM_RANGE(0x5800, 0x58ff) AM_WRITE(high_offset_w)
	AM_RANGE(0x6000, 0x67ff) AM_RAM
	AM_RANGE(0xa000, 0xa000) AM_READ(fake_r)
	AM_RANGE(0xa001, 0xa001) AM_READ(fake_r)
	AM_RANGE(0xa002, 0xa002) AM_READ(fake_r)
	AM_RANGE(0xc000, 0xc000) AM_READ(fake_r)
	AM_RANGE(0xe000, 0xe001) AM_WRITENOP
	AM_RANGE(0xe800, 0xefff) AM_READWRITE(MRA8_RAM, phrcraze_attr_w) AM_BASE(&phrcraze_attr)
	AM_RANGE(0xf000, 0xf7ff) AM_READWRITE(MRA8_RAM, phrcraze_bg_w) AM_BASE(&videoram)
	AM_RANGE(0xf800, 0xfbff) AM_RAM // not read
ADDRESS_MAP_END

static ADDRESS_MAP_START( trvwhiz_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_WRITENOP
ADDRESS_MAP_END

//to enter test-mode: enable "Enable Service Mode", enable "display settings",
//enable "freeze?" and disable "display settings"

INPUT_PORTS_START( phrcraze )
	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "1" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPSETTING(    0x02, "6" )
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
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON3 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_BUTTON4 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON5 )
	PORT_DIPNAME( 0x20, 0x20, "2" )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Enable Service Mode" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_DIPNAME( 0x04, 0x04, "Show Settings" )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, "3-Freeze?" )
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
	PORT_DIPNAME( 0x01, 0x01, "4" )
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

static void get_tile_info_bg(int tile_index)
{
	int code = videoram[tile_index];
	int attr = phrcraze_attr[tile_index];
	int region;

	if( attr & 0x40 )
	{
		region = 1;
		code |= (attr & 0x80) << 1;
	}
	else
	{
		region = 0;
		//banking is wrong
		code |= ((attr & 0x80) << 1) | ((attr & 0x08) << 6);
	}


	SET_TILE_INFO(region, code, 0, 0)
}


VIDEO_START( merit )
{
	bg_tilemap = tilemap_create(get_tile_info_bg,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,64,32);

	if(!bg_tilemap)
		return 1;

	return 0;
}

VIDEO_UPDATE( merit )
{
	tilemap_draw(bitmap,cliprect,bg_tilemap,0,0);
}

static struct GfxLayout tiles8x8x4_layout =
{
	8,8,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static struct GfxLayout tiles8x8x3_layout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

static struct GfxLayout tiles8x8x1_layout =
{
	8,8,
	RGN_FRAC(1,1),
	1,
	{ 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	16*8
};

struct GfxDecodeInfo phrcraze_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8x3_layout, 0, 32 },
	{ REGION_GFX2, 0, &tiles8x8x4_layout, 0, 32 },
	{ REGION_GFX1, 8, &tiles8x8x3_layout, 0, 32 }, //flipped tiles
	{ REGION_GFX2, 8, &tiles8x8x4_layout, 0, 32 }, //flipped tiles
	{ -1 } /* end of array */
};

struct GfxDecodeInfo trvwhiz_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8x3_layout, 0, 32 },
	{ REGION_GFX2, 0, &tiles8x8x1_layout, 0, 32 },
	{ REGION_GFX1, 8, &tiles8x8x3_layout, 0, 32 }, //flipped tiles
	{ REGION_GFX2, 8, &tiles8x8x1_layout, 0, 32 }, //flipped tiles
	{ -1 } /* end of array */
};

static MACHINE_DRIVER_START( phrcraze )
	MDRV_CPU_ADD_TAG("main",Z80,2500000)		 /* ?? */
	MDRV_CPU_PROGRAM_MAP(phrcraze_map,0)
	MDRV_CPU_IO_MAP(phrcraze_io_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0, 64*8-1, 0*8, 32*8-1)

	MDRV_GFXDECODE(phrcraze_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)

	MDRV_VIDEO_START(merit)
	MDRV_VIDEO_UPDATE(merit)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 2500000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( tictac )
	MDRV_IMPORT_FROM(phrcraze)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(tictac_map,0)
	MDRV_CPU_IO_MAP(tictac_io_map,0)

	MDRV_GFXDECODE(trvwhiz_gfxdecodeinfo)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( trvwhiz )
	MDRV_IMPORT_FROM(phrcraze)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(trvwhiz_map,0)
	MDRV_CPU_IO_MAP(trvwhiz_io_map,0)

	MDRV_GFXDECODE(trvwhiz_gfxdecodeinfo)
MACHINE_DRIVER_END

ROM_START( phrcraze )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "phrz.11",      0x00000, 0x8000, CRC(ccd33a0c) SHA1(869b66af4369f3b4bc19336ca2b8104c7f652de7) )

	ROM_REGION( 0x18000, REGION_GFX1, 0 )
	ROM_LOAD( "phrz.10",      0x00000, 0x8000, CRC(bfa78b67) SHA1(1b51c0e00240f798fe717624e706cb15700bc2f9) )
	ROM_LOAD( "phrz.8",       0x08000, 0x8000, CRC(9ce22cb3) SHA1(b653afb8f13decd993e434aaad69a6e09ab65f83) )
	ROM_LOAD( "phrz.7",       0x10000, 0x8000, CRC(237e221a) SHA1(7aa69375c2b9a9e73e0e4ed207bf595368b2deb2) )

	ROM_REGION( 0x8000, REGION_GFX2, 0 )
	ROM_LOAD( "phrz.9",       0x00000, 0x8000, CRC(17dcddd4) SHA1(51682bdbfb67cd0ccf20b97e8fa12d72f0fe82ed) )

	ROM_REGION( 0xa0000, REGION_USER1, ROMREGION_ERASEFF ) // questions
	ROM_LOAD( "phrz.1",       0x00000, 0x8000, CRC(0a016c5e) SHA1(1a24ecd7fe59b08c75a1b4575c7fe467cc7f0cf8) )
	ROM_LOAD( "phrz.2",       0x10000, 0x8000, CRC(e67dc49e) SHA1(5265af228531dc16db7f7ee78da6e51ef9a1d772) )
	ROM_LOAD( "phrz.3",       0x20000, 0x8000, CRC(5c79a653) SHA1(85a904465b347564e937074e2b18159604c83e51) )
	ROM_LOAD( "phrz.4",       0x30000, 0x8000, CRC(9837f757) SHA1(01106114b6997fe6432e519101f95c83a1f7cc1e) )
	ROM_LOAD( "phrz.5",       0x40000, 0x8000, CRC(dc9d8682) SHA1(46973da4298d0ed149c651498527c91b8ba57e0a) )
	ROM_LOAD( "phrz.6",       0x50000, 0x8000, CRC(48e24f17) SHA1(f50c85505f6ab2360f0885494001f174224f8575) )
ROM_END

/*

crt200 rev e-1
1985 merit industries

u39   u37                      z80b
u40   u38
           6845                 u5

      2016                      pb
      2016                      6264

                        ay3-8912
                        8255
                        8255


crt205a

pba pb9 pb8 pb7 pb6 pb5 pb4 pb3 pb2 pb1

*/

ROM_START( tictac )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "merit.u5",     0x00000, 0x8000, CRC(f0dd73f5) SHA1(f2988b84255ce5f7ea6d25150cdbae88b98e1be3) )

	ROM_REGION( 0x6000, REGION_GFX1, 0 )
	ROM_LOAD( "merit.u39",    0x00000, 0x2000, CRC(dd79e824) SHA1(d65ee1c758293ddf8a5f4913878a2867ba526e68) )
	ROM_LOAD( "merit.u38",    0x02000, 0x2000, CRC(e1bf0fab) SHA1(291261ea817c42d6e8a19c17a2d3706fed7d78c4) )
	ROM_LOAD( "merit.u37",    0x04000, 0x2000, CRC(94f9c7f8) SHA1(494389983fb62fe2d772c276e659b6b20c531933) )

	ROM_REGION( 0x2000, REGION_GFX2, 0 )
	ROM_LOAD( "merit.u40",    0x00000, 0x2000, CRC(ab0088eb) SHA1(23a05a4dc11a8497f4fc7e4a76085af15ff89cea) )

	ROM_REGION( 0xa0000, REGION_USER1, ROMREGION_ERASEFF ) // questions
	ROM_LOAD( "merit.pb1",    0x00000, 0x8000, CRC(d1584173) SHA1(7a2190203f478f446cc70c473c345e7cc332e049) )
	ROM_LOAD( "merit.pb2",    0x10000, 0x8000, CRC(d00ab1fd) SHA1(c94269c8a478e88f71aeca94c6f20fc05a9c62bd) )
	ROM_LOAD( "merit.pb3",    0x20000, 0x8000, CRC(71b398a9) SHA1(5ea07c409afd52c7d08592b30ff0ff3b72c3f8c3) )
	ROM_LOAD( "merit.pb4",    0x30000, 0x8000, CRC(eb34672f) SHA1(c472fc4445fc434029a2740dfc1d9ab9b1ef9f87) )
	ROM_LOAD( "merit.pb5",    0x40000, 0x8000, CRC(8eea30b9) SHA1(fe1d0332106631f56bc6c57a888da9e4e63fa52f) )
	ROM_LOAD( "merit.pb6",    0x50000, 0x8000, CRC(3f45064d) SHA1(de109ac0b19fd1cd7f0020cc174c2da21708108c) )
	ROM_LOAD( "merit.pb7",    0x60000, 0x8000, CRC(f1c446cd) SHA1(9a6f18defbb64e202ae12e1a59502b8f2d6a58a6) )
	ROM_LOAD( "merit.pb8",    0x70000, 0x8000, CRC(206cfc0d) SHA1(78f6b684713459a617096aa3ffe6e9e62583938c) )
	ROM_LOAD( "merit.pb9",    0x80000, 0x8000, CRC(9333dbca) SHA1(dd87e6f69d60580fdb6f979398edbeb1a51be355) )
	ROM_LOAD( "merit.pba",    0x90000, 0x8000, CRC(6eda81f4) SHA1(6d64344691e3e52035a7d30fb3e762f0bd397db7) )
ROM_END

ROM_START( pitboss )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "u5-0c.rom",     0x00000, 0x2000, CRC(d8902656) SHA1(06da829201f6141a6b23afa0e277a3c7a122c26e) )
	ROM_LOAD( "u6-0.rom",      0x02000, 0x2000, CRC(bf903b01) SHA1(1f5f69cfd3eb105bd9bad071016931a79defa16b) )
	ROM_LOAD( "u7-0.rom",      0x04000, 0x2000, CRC(306351b9) SHA1(32cd243aa65571ee7fc72971b6a16beeb4ed9d85) )

	ROM_REGION( 0x6000, REGION_GFX1, 0 )
	ROM_LOAD( "u39.rom",    0x00000, 0x2000, CRC(6662f607) SHA1(6b423f8de011d196700839af0be37effbf87383f) )
	ROM_LOAD( "u38.rom",    0x02000, 0x2000, CRC(a014b44f) SHA1(906d426b1de75f26030c19dcd599b6570909f510) )
	ROM_LOAD( "u37.rom",    0x04000, 0x2000, CRC(cb12e139) SHA1(06fe91281faae5d0c0ae4b3cd8ad103bd3995c38) )

	ROM_REGION( 0x2000, REGION_GFX2, 0 )
	ROM_LOAD( "u40.rom",    0x00000, 0x2000, CRC(52298162) SHA1(79aa6c4ab6bec6450d882615e64f61cfef934153) )

	ROM_REGION( 0xa0000, REGION_USER1, ROMREGION_ERASEFF ) // questions
	/* no questions - missing or doesn't have them? */
ROM_END

/* super pit boss is not on this hardware? (roms for 2xZ80..) so not added here */

ROM_START( trvwhiz )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "u5_0e",        0x0000, 0x2000, CRC(97b8d320) SHA1(573945531113d8aae9418ba1e9a2063052227029) )
	ROM_LOAD( "u6_0e",        0x2000, 0x2000, CRC(2e86288d) SHA1(62c7024d8dfebed9bb05ea91302efe5d18cb7d2a) )

	ROM_REGION( 0x6000, REGION_GFX1, 0 )
	ROM_LOAD( "u39",          0x0000, 0x2000, CRC(b9d9a80e) SHA1(55b6a0d09f8619df93ba936e083835c859a557df) )
	ROM_LOAD( "u38",          0x2000, 0x2000, CRC(8348083e) SHA1(260a4c1ae043e7ceac65a8818c23940d32275879) )
	ROM_LOAD( "u37",          0x4000, 0x2000, CRC(b4d3c9f4) SHA1(dda99549306519c147d275d8c6af672e80a96b67) )

	ROM_REGION( 0x2000, REGION_GFX2, 0 )
	ROM_LOAD( "u40a",         0x0000, 0x2000, CRC(fbfae092) SHA1(b8569819952a5c805f11b6854d64b3ae9c857f97) )

	ROM_REGION( 0xa0000, REGION_USER1, ROMREGION_ERASEFF ) // questions
	ROM_LOAD( "ent_101.01a",  0x00000, 0x8000, CRC(3825ac47) SHA1(d0da047c4d30a26f496b3663cfda77c229279be8) )
	ROM_LOAD( "ent_101.02a",  0x10000, 0x8000, CRC(a0153407) SHA1(e669957a5d4775bfa2c16960a2a909a3505c078b) )
	ROM_LOAD( "ent_101.03a",  0x20000, 0x8000, CRC(755b16ab) SHA1(277ea4110479ecdb2c772299ea04f4918cf7f561) )
	ROM_LOAD( "gen_101.01a",  0x30000, 0x8000, CRC(74d14039) SHA1(54b85581d60fb535d37a051f375e687a933600ea) )
	ROM_LOAD( "gen_101.02a",  0x40000, 0x8000, CRC(b1b930d8) SHA1(57be3ee1c0adcb549088818dc7efda64508b5647) )
	ROM_LOAD( "spo_101.01a",  0x50000, 0x8000, CRC(9dc4ba98) SHA1(4ce2bbbd7436a0ba8140879d5d8614bddbd5a8ec) )
	ROM_LOAD( "spo_101.02a",  0x60000, 0x8000, CRC(9c106ad9) SHA1(1d1a5c91152283e3937a2df17cd57b8fe04072b7) )
	ROM_LOAD( "spo_101.03a",  0x70000, 0x8000, CRC(3d69c3a3) SHA1(9f16d45660f3cb15e44e9fc0d940a7b2b12819e8) )
	ROM_LOAD( "sex-101.01a",  0x80000, 0x8000, CRC(301d65c2) SHA1(48d260077e9c9ed82f6dfa176b1103723dc9e19a) )
	ROM_LOAD( "sex-101.02a",  0x90000, 0x8000, CRC(2596091b) SHA1(7fbb9c2c3f74e12714513928c8cf3769bf29fc8b) )
ROM_END

ROM_START( trvwhzii )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "trivia.u5",    0x0000, 0x2000, CRC(731fd5b1) SHA1(1074780321029446da0e6765b9e036b06b067a48) )
	ROM_LOAD( "trivia.u6",    0x2000, 0x2000, CRC(af6886c0) SHA1(48005b921d7ce33ffc0ba160be82053a26382a9d) )

	ROM_REGION( 0x6000, REGION_GFX1, 0 )
	ROM_LOAD( "trivia.u39",   0x0000, 0x2000, CRC(f8a5f5fb) SHA1(a511e1a2b5e887ef00dc919e9e664ccec2d36cfa) )
	ROM_LOAD( "trivia.u38",   0x2000, 0x2000, CRC(27621e52) SHA1(a7e88d329e2e774fef9bd8c5cefb4d8f1cfcba4c) )
	ROM_LOAD( "trivia.u37",   0x4000, 0x2000, CRC(f739b5dc) SHA1(fbf469b7f4cab50e06ec2def9344e3b9801a275e) )

	ROM_REGION( 0x2000, REGION_GFX2, 0 )
	ROM_LOAD( "trivia.u40",   0x0000, 0x2000, CRC(cea7319f) SHA1(663cd18a4699dfd5ad1d3357094eff247e9b4a47) )

	ROM_REGION( 0xa0000, REGION_USER1, ROMREGION_ERASEFF ) // questions
	ROM_LOAD( "trivia.ent",   0x00000, 0x8000, CRC(ff45d92b) SHA1(10356bc6a04b2c53ecaf76cb0cba3ec70b4ba612) )
	ROM_LOAD( "trivia.gen",   0x10000, 0x8000, CRC(1d8d353f) SHA1(6bd0cc5c67da81a48737f32bc49cbf235648c4c6) )
	ROM_LOAD( "trivia.sex",   0x20000, 0x8000, CRC(0be4ef9a) SHA1(c80080f1c853e1043bf7e47bea322540a8ac9195) )
	ROM_LOAD( "trivia.spo",   0x30000, 0x8000, CRC(64181d34) SHA1(f84e28fc589b86ca6a596815871ed26602bcc095) )
	ROM_LOAD( "trivia.sx2",   0x40000, 0x8000, CRC(32519098) SHA1(d070e02bb10e04964893903599a69a8943f9ac8a) )
	ROM_LOAD( "trivia1.ent",  0x50000, 0x8000, CRC(902e26f7) SHA1(f13b816bfc507fb429fb3f44531de346a82c780d) )
	ROM_LOAD( "trivia1.spo",  0x60000, 0x8000, CRC(ae111429) SHA1(ff551d7ac7ad367338e908805aeb78c59a747919) )
	ROM_LOAD( "trivia2.spo",  0x70000, 0x8000, CRC(ee9263b3) SHA1(1644ab01f17e3af1e193e509d64dcbb243d3eb80) )
	ROM_LOAD( "trivia.02a",   0x80000, 0x4000, CRC(2000e3c3) SHA1(21737fde3d1a1b22da4590476e4e52ee1bab026f) )
ROM_END

DRIVER_INIT(phrcraze)
{
	rom_type = 0x71f1;
}

DRIVER_INIT(tictac)
{
	rom_type = 0x7df3;
}

DRIVER_INIT(trvwhiz)
{
	rom_type = 0x7bfb;
}

DRIVER_INIT(trvwhzii)
{
	rom_type = 0x7fff; //and 0x3fff; for the last rom
}

GAMEX( 1986, phrcraze, 0, phrcraze, phrcraze, phrcraze, ROT0,  "Merit", "Phraze Craze",		GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_WRONG_COLORS | GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 198?, tictac,   0, tictac,   phrcraze, tictac,   ROT0,  "Merit", "Tic Tac Trivia",	GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_WRONG_COLORS | GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 198?, pitboss,  0, trvwhiz,  phrcraze, trvwhiz,  ROT0,  "Merit", "Pit Boss",			GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_WRONG_COLORS | GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 198?, trvwhiz,  0, trvwhiz,  phrcraze, trvwhiz,  ROT90, "Merit", "Trivia Whiz",		GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_WRONG_COLORS | GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
GAMEX( 198?, trvwhzii, 0, trvwhiz,  phrcraze, trvwhzii, ROT90, "Merit", "Trivia Whiz II ?", GAME_NO_SOUND | GAME_IMPERFECT_GRAPHICS | GAME_WRONG_COLORS | GAME_UNEMULATED_PROTECTION | GAME_NOT_WORKING )
