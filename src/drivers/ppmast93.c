/*

Ping Pong Masters '93
Electronic Devices, 1993

PCB Layout
----------

|----------------------------------------------|
|                                              |
|          2018                                |
|          1.UE7                 YM2413        |
|                    |----------|              |
|          Z80       |Unknown   | 24MHz   PROM1|
|                    |PLCC84    |              |
|J  DSW2             |          |         PROM2|
|A           2.UP7   |          |      PAL     |
|M                   |          | PAL     PROM3|
|M  DSW1      2018   |----------|              |
|A                                   2018      |
|             PAL                    2018      |
|                          5MHz                |
|                                              |
|                                              |
|                                              |
|          Z80             3.UG16     4.UG15   |
|----------------------------------------------|
Notes:
      Z80 clock    : 5.000MHz (both)
      YM2413 clock : 2.500MHz (5/2)
      VSync        : 55Hz


Dip Switch Settings
-------------------

SW1                1     2     3     4     5     6     7     8
|-------|-------|-----|-----|-----|-----|-----|-----|-----|-----|
|Coin1  | 1C 1P | OFF | OFF | OFF | OFF |     |     |     |     |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 1C 2P | ON  | OFF | OFF | OFF |     |     |     |     |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 1C 3P | OFF | ON  | OFF | OFF |     |     |     |     |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 1C 4P | ON  | ON  | OFF | OFF |     |     |     |     |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 1C 5P | OFF | OFF | ON  | OFF |     |     |     |     |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 2C 1P | ON  | OFF | ON  | OFF |     |     |     |     |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 2C 3P | OFF | ON  | ON  | OFF |     |     |     |     |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 2C 5P | ON  | ON  | ON  | OFF |     |     |     |     |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 3C 1P | OFF | OFF | OFF | ON  |     |     |     |     |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 3C 2P | ON  | OFF | OFF | ON  |     |     |     |     |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 3C 5P | OFF | ON  | OFF | ON  |     |     |     |     |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 4C 1P | ON  | ON  | OFF | ON  |     |     |     |     |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 4C 3P | OFF | OFF | ON  | ON  |     |     |     |     |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 4C 5P | ON  | OFF | ON  | ON  |     |     |     |     |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 5C 1P | OFF | ON  | ON  | ON  |     |     |     |     |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 5C 2P | ON  | ON  | ON  | ON  |     |     |     |     |
|-------|-------|-----|-----|-----|-----|-----|-----|-----|-----|
|Coin2  | 1C 1P |     |     |     |     | OFF | OFF | OFF | OFF |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 1C 2P |     |     |     |     | ON  | OFF | OFF | OFF |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 1C 3P |     |     |     |     | OFF | ON  | OFF | OFF |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 1C 4P |     |     |     |     | ON  | ON  | OFF | OFF |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 1C 5P |     |     |     |     | OFF | OFF | ON  | OFF |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 2C 1P |     |     |     |     | ON  | OFF | ON  | OFF |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 2C 3P |     |     |     |     | OFF | ON  | ON  | OFF |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 2C 5P |     |     |     |     | ON  | ON  | ON  | OFF |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 3C 1P |     |     |     |     | OFF | OFF | OFF | ON  |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 3C 2P |     |     |     |     | ON  | OFF | OFF | ON  |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 3C 5P |     |     |     |     | OFF | ON  | OFF | ON  |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 4C 1P |     |     |     |     | ON  | ON  | OFF | ON  |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 4C 3P |     |     |     |     | OFF | OFF | ON  | ON  |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 4C 5P |     |     |     |     | ON  | OFF | ON  | ON  |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 5C 1P |     |     |     |     | OFF | ON  | ON  | ON  |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | 5C 2P |     |     |     |     | ON  | ON  | ON  | ON  |
|-------|-------|-----|-----|-----|-----|-----|-----|-----|-----|

SW2                1     2     3     4     5     6     7     8
|-------|-------|-----|-----|-----|-----|-----|-----|-----|-----|
|Diffic-| Easy  | OFF | OFF |     |     |     |     |     |     |
|ulty   |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | Normal| ON  | OFF |     |     |     |     |     |     |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | Hard  | OFF | ON  |     |     |     |     |     |     |
|       |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | V.Hard| ON  | ON  |     |     |     |     |     |     |
|-------|-------|-----|-----|-----|-----|-----|-----|-----|-----|
|Demo   |Without|     |     | OFF |     |     |     |     |     |
|Sound  |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | With  |     |     | ON  |     |     |     |     |     |
|-------|-------|-----|-----|-----|-----|-----|-----|-----|-----|
|Test   | No    |     |     |     | OFF |     |     |     |     |
|Mode   |-------|-----|-----|-----|-----|-----|-----|-----|-----|
|       | Yes   |     |     |     | ON  |     |     |     |     |
|-------|-------|-----|-----|-----|-----|-----|-----|-----|-----|

The DIP sheet also seems to suggest the use of a 4-way joystick and 2 buttons,
one for shoot and one for select.

*/

#include "driver.h"

static struct tilemap *ppmast93_fg_tilemap, *ppmast93_bg_tilemap;
data8_t *ppmast93_fgram, *ppmast93_bgram;

WRITE8_HANDLER( ppmast93_fgram_w )
{
	ppmast93_fgram[offset] = data;
	tilemap_mark_tile_dirty(ppmast93_fg_tilemap,offset/2);
}

WRITE8_HANDLER( ppmast93_bgram_w )
{
	ppmast93_bgram[offset] = data;
	tilemap_mark_tile_dirty(ppmast93_bg_tilemap,offset/2);
}

READ8_HANDLER( ppmast93_rand )
{
	return rand();
}


static ADDRESS_MAP_START( ppmast93_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK1) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xd000, 0xd7ff) AM_RAM AM_WRITE( ppmast93_fgram_w ) AM_BASE( &ppmast93_fgram )
	AM_RANGE(0xf000, 0xf7ff) AM_RAM AM_WRITE( ppmast93_bgram_w ) AM_BASE( &ppmast93_bgram )
	AM_RANGE(0xf800, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( ppmast93_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0x00) AM_READ(input_port_0_r)
	AM_RANGE(0x02, 0x02) AM_READ(input_port_1_r)
	AM_RANGE(0x04, 0x04) AM_READ(input_port_2_r)
	AM_RANGE(0x06, 0x06) AM_READ(input_port_3_r)
	AM_RANGE(0x08, 0x08) AM_READ(input_port_4_r)

	AM_RANGE(0x0e, 0x0e) AM_READ(ppmast93_rand)
	AM_RANGE(0x0f, 0x0f) AM_READ(ppmast93_rand)
ADDRESS_MAP_END


static ADDRESS_MAP_START( ppmast93_map2, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
//	AM_RANGE(0xd000, 0xd7ff) AM_RAM

//	AM_RANGE(0xf000, 0xf7ff) AM_RAM
	AM_RANGE(0xf800, 0xffff) AM_RAM
ADDRESS_MAP_END

static void get_ppmast93_fg_tile_info(int tile_index)
{
	int code = (ppmast93_fgram[tile_index*2+1] << 8) | ppmast93_fgram[tile_index*2];
	SET_TILE_INFO(
			0,
			code & 0x0fff,
			(code & 0xf000) >> 12,
			0)
}

static void get_ppmast93_bg_tile_info(int tile_index)
{
	int code = (ppmast93_bgram[tile_index*2+1] << 8) | ppmast93_bgram[tile_index*2];
	SET_TILE_INFO(
			0,
			code & 0x0fff,
			(code & 0xf000) >> 12,
			0)
}

INPUT_PORTS_START( ppmast93 )
	PORT_START	/* 8bit */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) // nothing?
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* 8bit */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) // nothing?
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* 8bit */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* 8bit */
	PORT_DIPNAME( 0x01, 0x01, "DSW0" )
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

	PORT_START	/* 8bit */
	PORT_DIPNAME( 0x01, 0x01, "DSW1" )
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



static struct GfxLayout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0, 2, 4, 6 },
	{ 1, 0, 9, 8, 17, 16, 25, 24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout, 0, 16 },
	{ -1 }
};



VIDEO_START(ppmast93)
{
	int i;

	ppmast93_fg_tilemap = tilemap_create(get_ppmast93_fg_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32, 32);
	tilemap_set_transparent_pen(ppmast93_fg_tilemap,0);

	ppmast93_bg_tilemap = tilemap_create(get_ppmast93_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32, 32);

	/* palette init */
	for (i=0;i<0x100;i++)
	{
		int r,g,b;
		data8_t* rgn=memory_region(REGION_PROMS);

		b = rgn[i+0x000]&0x0f;
		g = rgn[i+0x100]&0x0f;
		r = rgn[i+0x200]&0x0f;

		r<<=4;
		g<<=4;
		b<<=4;

		palette_set_color(i,r,g,b);

	}

	return 0;
}

VIDEO_UPDATE(ppmast93)
{
	tilemap_draw(bitmap,cliprect,ppmast93_bg_tilemap,0,0);
	tilemap_draw(bitmap,cliprect,ppmast93_fg_tilemap,0,0);
}


static MACHINE_DRIVER_START( ppmast93 )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,5000000)		 /* 5 MHz */
	MDRV_CPU_PROGRAM_MAP(ppmast93_map,0)
	MDRV_CPU_IO_MAP(ppmast93_io,0)
//	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_CPU_ADD(Z80,5000000)		 /* 5 MHz */
	MDRV_CPU_PROGRAM_MAP(ppmast93_map2,0)


	MDRV_FRAMES_PER_SECOND(55)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0, 256-16-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(ppmast93)
	MDRV_VIDEO_UPDATE(ppmast93)

MACHINE_DRIVER_END

DRIVER_INIT( ppmast93 )
{
	unsigned char *rom = memory_region(REGION_CPU1);
	cpu_setbank(1,&rom[0x10000]);
}

ROM_START( ppmast93 )
	ROM_REGION( 0x30000, REGION_CPU1, 0 )
	ROM_LOAD( "2.up7", 0x10000, 0x20000, CRC(8854d8db) SHA1(9d93ddfb44d533772af6519747a6cb50b42065cd)  )
	ROM_COPY(REGION_CPU1,0x10000, 0x00000,0x08000)

	ROM_REGION( 0x30000, REGION_CPU2, 0 )
	ROM_LOAD( "1.ue7", 0x10000, 0x20000, CRC(8e26939e) SHA1(e62441e523f5be6a3889064cc5e0f44545260e93)  )
	ROM_COPY(REGION_CPU2,0x10000, 0x00000,0x08000)

	ROM_REGION( 0x40000, REGION_GFX1, 0 )
	ROM_LOAD( "3.ug16", 0x00000, 0x20000, CRC(8ab24641) SHA1(c0ebee90bf3fe208947ae5ea56f31469ed24d198) )
	ROM_LOAD( "4.ug15", 0x20000, 0x20000, CRC(b16e9fb6) SHA1(53aa962c63319cd649e0c8cf0c26e2308598e1aa) )

	ROM_REGION( 0x300, REGION_PROMS, 0 ) // colour proms?
	ROM_LOAD( "prom1.ug26", 0x000, 0x100, CRC(d979c64e) SHA1(172c9579013d58e35a5b4f732e360811ac36295e) )
	ROM_LOAD( "prom2.ug25", 0x100, 0x100, CRC(4b5055ba) SHA1(6213e79492d35593c643ef5c01ce6a58a77866aa) )
	ROM_LOAD( "prom3.ug24", 0x200, 0x100, CRC(b1a4415a) SHA1(1dd22260f7dbdc9c812a2349069ed5f3c9c92826) )

ROM_END

GAMEX( 1993, ppmast93, 0, ppmast93, ppmast93, ppmast93, ROT0, "Electronic Devices", "Ping Pong Masters '93",GAME_NOT_WORKING|GAME_NO_SOUND )
