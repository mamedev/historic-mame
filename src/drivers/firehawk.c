/*

Fire Hawk - ESD
---------------

i think this hardware is more powerful than this game would suggest



PCB Layout
----------

ESD-PROT-002
|------------------------------------------------|
|      FHAWK_S1.U40   FHAWK_S2.U36               |
|      6116     6295  FHAWK_S3.U41               |
|               6295                 FHAWK_G1.UC6|
|    PAL   Z80                       FHAWK_G2.UC5|
|    4MHz                             |--------| |
|                                     | ACTEL  | |
|J   6116             62256           |A54SX16A| |
|A   6116             62256           |        | |
|M                                    |(QFP208)| |
|M                                    |--------| |
|A     DSW1              FHAWK_G3.UC2            |
|      DSW2                           |--------| |
|      DSW3                           | ACTEL  | |
|                     6116            |A54SX16A| |
|                     6116            |        | |
|      62256                          |(QFP208)| |
| FHAWK_P1.U59                        |--------| |
| FHAWK_P2.U60  PAL                  62256  62256|
|                                                |
|12MHz 62256   68000                 62256  62256|
|------------------------------------------------|
Notes:
      68000 clock: 12.000MHz
        Z80 clock: 4.000MHz
      6295 clocks: 1.000MHz (both), sample rate = 1000000 / 132 (both)
            VSync: 56Hz

*/

#include "driver.h"

static data16_t* firehawk_bgram;
static struct tilemap *firehawk_bg_tilemap;
static data16_t* firehawk_sprram;

static WRITE16_HANDLER( firehawk_bgram_w )
{
	if (firehawk_bgram[offset] != data)
	{
		COMBINE_DATA(&firehawk_bgram[offset]);
		tilemap_mark_tile_dirty(firehawk_bg_tilemap,offset);
	}
}

static void get_firehawk_bg_tile_info(int tile_index)
{
	int tileno;
	tileno = firehawk_bgram[tile_index];
	SET_TILE_INFO(0,tileno,0,0)
}

static void firehawk_draw_sprites ( struct mame_bitmap *bitmap, const struct rectangle *cliprect )
{
	const struct GfxElement *gfx = Machine->gfx[1];
	data16_t *source = firehawk_sprram;
	data16_t *finish = source + 0x8000/2;

	while( source<finish )
	{
		int xpos, ypos;
		int tileno;

		xpos = (source[4] & 0x00ff) >> 0;
		ypos = (source[6] & 0x00ff) >> 0;

		tileno = source[3];

		drawgfx(bitmap,gfx, tileno,1,0,0,xpos,ypos,cliprect,TRANSPARENCY_PEN,0);

		source++;
	}
}


VIDEO_START(firehawk)
{
	firehawk_bg_tilemap = tilemap_create(get_firehawk_bg_tile_info,tilemap_scan_cols,TILEMAP_OPAQUE, 16, 16, 256,16);
	return 0;
}

VIDEO_UPDATE(firehawk)
{
	tilemap_draw(bitmap,cliprect,firehawk_bg_tilemap,0,0);
	firehawk_draw_sprites(bitmap,cliprect);
}

READ16_HANDLER( fh_unk_r )
{
	return 0xffff;

	//return mame_rand();
}

static ADDRESS_MAP_START( firehawk_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)

	AM_RANGE(0x280000, 0x280001) AM_READ(fh_unk_r)
	AM_RANGE(0x280002, 0x280003) AM_READ(fh_unk_r)
	AM_RANGE(0x280004, 0x280005) AM_READ(fh_unk_r)

//	AM_RANGE(0x280006, 0x29ffff) AM_READ(MRA16_RAM)

	AM_RANGE(0x3c0000, 0x3c7fff) AM_READ(MRA16_RAM)
	AM_RANGE(0x3c8000, 0x3cffff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( firehawk_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)

	AM_RANGE(0x280000, 0x287fff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x288000, 0x28ffff) AM_WRITE(paletteram16_IIIIRRRRGGGGBBBB_word_w) AM_BASE (&paletteram16) // wrong format
	AM_RANGE(0x290000, 0x291fff) AM_WRITE(firehawk_bgram_w)  AM_BASE( &firehawk_bgram )
	AM_RANGE(0x292000, 0x29ffff) AM_WRITE(MWA16_RAM) // bg seems to be mirrored here?

	AM_RANGE(0x3c0000, 0x3c7fff) AM_WRITE(MWA16_RAM) // ram?
	AM_RANGE(0x3c8000, 0x3cffff) AM_WRITE(MWA16_RAM) AM_BASE( &firehawk_sprram ) // sprram?


ADDRESS_MAP_END

INPUT_PORTS_START( firehawk )
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "0" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END


static struct GfxLayout firehawk_tiles_layout =
{
	16,16,
	RGN_FRAC(1,2),
	8,
	{ 0,1,2,3,RGN_FRAC(1,2)+0,RGN_FRAC(1,2)+1,RGN_FRAC(1,2)+2,RGN_FRAC(1,2)+3 },
	{ 0,4,8,12,16,20,24,28, 512+0, 512+4, 512+8, 512+12, 512+16, 512+20, 512+24, 512+28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  8*32, 9*32, 10*32, 11*32, 12*32, 13*32, 14*32, 15*32 },
	32*32
};

static struct GfxLayout firehawk_sprites_layout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 0,4,8,12,16,20,24,28, 512+0, 512+4, 512+8, 512+12, 512+16, 512+20, 512+24, 512+28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  8*32, 9*32, 10*32,11*32,12*32,13*32,14*32,15*32 },
	32*32
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &firehawk_tiles_layout,   0x0, 0x80  }, /* bg tiles */
	{ REGION_GFX2, 0, &firehawk_sprites_layout, 0x0, 0x80 }, /* sprite tiles */
	{ -1 } /* end of array */
};


static struct OKIM6295interface okim6295_interface =
{
	2,				/* 2 chip */
	{ 1000000/132,1000000/132 },		/* frequency (Hz) */
	{ REGION_SOUND1,REGION_SOUND2 },	/* memory region */
	{ 47,47 }
};

static INTERRUPT_GEN( fh_interrupt )
{
	if (cpu_getiloops() == 0) cpu_set_irq_line(0, 2, HOLD_LINE);
	else cpu_set_irq_line(0, 4, HOLD_LINE);
}

static MACHINE_DRIVER_START( firehawk )
	MDRV_CPU_ADD(M68000, 12000000 )
	MDRV_CPU_PROGRAM_MAP(firehawk_readmem,firehawk_writemem)
	MDRV_CPU_VBLANK_INT(fh_interrupt,2)

	/* z80 @ 4mhz */

	MDRV_FRAMES_PER_SECOND(56)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(128*8, 128*8)
	MDRV_VISIBLE_AREA(8*8, 64*8-1, 0*8, 16*16-1)
	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_START(firehawk)
	MDRV_VIDEO_UPDATE(firehawk)


	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END



ROM_START( firehawk )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "fhawk_p1.u59", 0x00001, 0x80000, CRC(d6d71a50) SHA1(e947720a0600d049b7ea9486442e1ba5582536c2) )
	ROM_LOAD16_BYTE( "fhawk_p2.u60", 0x00000, 0x80000, CRC(9f35d245) SHA1(5a22146f16bff7db924550970ed2a3048bc3edab) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 ) /* Z80 Code */
	ROM_LOAD( "fhawk_s1.u40", 0x00000, 0x20000, CRC(c6609c39) SHA1(fe9b5f6c3ab42c48cb493fecb1181901efabdb58) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "fhawk_s2.u36", 0x00000, 0x40000, CRC(d16aaaad) SHA1(96ca173ca433164ed0ae51b41b42343bd3cfb5fe) )

	ROM_REGION( 0x040000, REGION_SOUND2, 0 ) /* Samples */
	ROM_LOAD( "fhawk_s3.u41", 0x00000, 0x40000, CRC(3fdcfac2) SHA1(c331f2ea6fd682cfb00f73f9a5b995408eaab5cf) )

	ROM_REGION( 0x400000, REGION_GFX1, 0 ) /* bg tiles */
	ROM_LOAD( "fhawk_g1.uc6", 0x000000, 0x200000, CRC(2ab0b06b) SHA1(25362f6a517f188c62bac28b1a7b7b49622b1518) )
	ROM_LOAD( "fhawk_g2.uc5", 0x200000, 0x200000, CRC(d11bfa20) SHA1(15142004ab49f7f1e666098211dff0835c61df8d) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_INVERT ) /* sprites */
	ROM_LOAD( "fhawk_g3.uc2", 0x00000, 0x200000,  CRC(cae72ff4) SHA1(7dca7164015228ea039deffd234778d0133971ab) )
ROM_END

GAMEX( 2001, firehawk, 0, firehawk, firehawk, 0, ORIENTATION_FLIP_Y, "ESD", "Firehawk", GAME_NOT_WORKING )
