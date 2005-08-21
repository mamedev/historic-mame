/***************************************************************************

Gotcha  (c) 1997 Dongsung

driver by Nicola Salmoria

TODO:
- Find out what the "Explane Type" dip switch actually does.
- Should use the artwork system to show the lamp state: there are 12 lamps, one
  for every button, and they are used a lot during the game (see gotcha_lamps_w).
- Unknown writes to 0x30000c. It changes for some levels, it's probably
  gfx related but since everything seems fine I've no idea what it might do.
- Unknown sound writes at C00F; also, there's an NMI handler that would
  read from C00F.
- Sound samples were getting chopped; I fixed this by changing sound/adpcm.c to
  disregard requests to play new samples until the previous one is finished.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sound/2151intf.h"
#include "sound/okim6295.h"


VIDEO_START( gotcha );
VIDEO_UPDATE( gotcha );
WRITE16_HANDLER( gotcha_fgvideoram_w );
WRITE16_HANDLER( gotcha_bgvideoram_w );
WRITE16_HANDLER( gotcha_gfxbank_select_w );
WRITE16_HANDLER( gotcha_gfxbank_w );
WRITE16_HANDLER( gotcha_scroll_w );

extern data16_t *gotcha_fgvideoram,*gotcha_bgvideoram;



static WRITE16_HANDLER( gotcha_lamps_w )
{
#if 0
	ui_popup("%c%c%c%c %c%c%c%c %c%c%c%c",
			(data & 0x001) ? 'R' : '-',
			(data & 0x002) ? 'G' : '-',
			(data & 0x004) ? 'B' : '-',
			(data & 0x008) ? 'S' : '-',
			(data & 0x010) ? 'R' : '-',
			(data & 0x020) ? 'G' : '-',
			(data & 0x040) ? 'B' : '-',
			(data & 0x080) ? 'S' : '-',
			(data & 0x100) ? 'R' : '-',
			(data & 0x200) ? 'G' : '-',
			(data & 0x400) ? 'B' : '-',
			(data & 0x800) ? 'S' : '-'
			);
#endif
}

static WRITE16_HANDLER( gotcha_oki_bank_w )
{
	if (ACCESSING_MSB)
	{
		OKIM6295_set_bank_base(0,(((~data & 0x0100) >> 8) * 0x40000));
	}
}



static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x120000, 0x12ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x140000, 0x1405ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x160000, 0x1607ff) AM_READ(MRA16_RAM)
	AM_RANGE(0x180000, 0x180001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x180002, 0x180003) AM_READ(input_port_1_word_r)
	AM_RANGE(0x180004, 0x180005) AM_READ(input_port_2_word_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x100000, 0x100001) AM_WRITE(soundlatch_word_w)
	AM_RANGE(0x100002, 0x100003) AM_WRITE(gotcha_lamps_w)
	AM_RANGE(0x100004, 0x100005) AM_WRITE(gotcha_oki_bank_w)
	AM_RANGE(0x120000, 0x12ffff) AM_WRITE(MWA16_RAM)
	AM_RANGE(0x140000, 0x1405ff) AM_WRITE(paletteram16_xRRRRRGGGGGBBBBB_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x160000, 0x1607ff) AM_WRITE(MWA16_RAM) AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0x300000, 0x300001) AM_WRITE(gotcha_gfxbank_select_w)
	AM_RANGE(0x300002, 0x300009) AM_WRITE(gotcha_scroll_w)
//  { 0x30000c, 0x30000d,
	AM_RANGE(0x30000e, 0x30000f) AM_WRITE(gotcha_gfxbank_w)
	AM_RANGE(0x320000, 0x320fff) AM_WRITE(gotcha_fgvideoram_w) AM_BASE(&gotcha_fgvideoram)
	AM_RANGE(0x322000, 0x322fff) AM_WRITE(gotcha_bgvideoram_w) AM_BASE(&gotcha_bgvideoram)
ADDRESS_MAP_END


static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xc001, 0xc001) AM_READ(YM2151_status_port_0_r)
	AM_RANGE(0xc006, 0xc006) AM_READ(soundlatch_r)
	AM_RANGE(0xd000, 0xd7ff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xc000) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0xc001, 0xc001) AM_WRITE(YM2151_data_port_0_w)
	AM_RANGE(0xc002, 0xc003) AM_WRITE(OKIM6295_data_0_w)	// TWO addresses!
	AM_RANGE(0xd000, 0xd7ff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END



INPUT_PORTS_START( gotcha )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x0007, "1" )
	PORT_DIPSETTING(      0x0006, "2" )
	PORT_DIPSETTING(      0x0005, "3" )
	PORT_DIPSETTING(      0x0004, "4" )
	PORT_DIPSETTING(      0x0003, "5" )
	PORT_DIPSETTING(      0x0002, "6" )
	PORT_DIPSETTING(      0x0001, "7" )
	PORT_DIPSETTING(      0x0000, "8" )
	PORT_DIPNAME( 0x0008, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0030, 0x0010, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0030, "1" )
	PORT_DIPSETTING(      0x0020, "2" )
	PORT_DIPSETTING(      0x0010, "3" )
	PORT_DIPSETTING(      0x0000, "4" )
	PORT_DIPNAME( 0x00c0, 0x0080, DEF_STR( Coinage ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x00c0, "1 Coin/99 Credits" )
	PORT_DIPNAME( 0x0100, 0x0100, "Info" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, "Explane Type" )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, "Game Selection" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( On ) )
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
	PORT_SERVICE( 0x8000, IP_ACTIVE_LOW )
INPUT_PORTS_END



static struct GfxLayout tilelayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*16
};

static struct GfxLayout spritelayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(0,4), RGN_FRAC(1,4), RGN_FRAC(2,4), RGN_FRAC(3,4) },
	{ 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*16
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tilelayout,   0x100, 32 },
	{ REGION_GFX2, 0, &spritelayout, 0x000, 16 },
	{ -1 } /* end of array */
};



static void irqhandler(int linestate)
{
	cpunum_set_input_line(1,0,linestate);
}

static struct YM2151interface ym2151_interface =
{
	irqhandler
};



static MACHINE_DRIVER_START( gotcha )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000,12000000)	/* 12 MHz ? */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(Z80,6000000)	/* 6 MHz ? */
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
//  MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(40*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(768)

	MDRV_VIDEO_START(gotcha)
	MDRV_VIDEO_UPDATE(gotcha)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2151, 14318180/4)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.80)
	MDRV_SOUND_ROUTE(1, "mono", 0.80)

	MDRV_SOUND_ADD(OKIM6295, 1056000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.60)
MACHINE_DRIVER_END



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( gotcha )
	ROM_REGION( 0x80000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "gotcha.u3",    0x00000, 0x40000, CRC(5e5d52e0) SHA1(c3e9375350b7931e3c9874a045d7a9d8df5ea691) )
	ROM_LOAD16_BYTE( "gotcha.u2",    0x00001, 0x40000, CRC(3aa8eaff) SHA1(348f2ab43101d51c553ff10f9d18cc499006c965) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )	/* 64k for the audio CPU */
	ROM_LOAD( "gotcha_u.z02", 0x0000, 0x10000, CRC(f4f6e16b) SHA1(a360c571bee7391c66e98e5e111e78ac9732390e) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "gotcha-u.42a", 0x000000, 0x20000, CRC(4ea822f0) SHA1(5b25d4c80138d9a0f3d481fa0c2f3665772bc0c8) )
	ROM_CONTINUE(             0x100000, 0x20000 )
	ROM_CONTINUE(             0x020000, 0x20000 )
	ROM_CONTINUE(             0x120000, 0x20000 )
	ROM_LOAD( "gotcha-u.42b", 0x040000, 0x20000, CRC(6bb529ac) SHA1(d872ec3d13d2bef4f8e0d0a8e72827b5ca87e193) )
	ROM_CONTINUE(             0x140000, 0x20000 )
	ROM_CONTINUE(             0x060000, 0x20000 )
	ROM_CONTINUE(             0x160000, 0x20000 )
	ROM_LOAD( "gotcha-u.41a", 0x080000, 0x20000, CRC(49299b7b) SHA1(85276453b6fce925c7b10c713e35284066df6ebf) )
	ROM_CONTINUE(             0x180000, 0x20000 )
	ROM_CONTINUE(             0x0a0000, 0x20000 )
	ROM_CONTINUE(             0x1a0000, 0x20000 )
	ROM_LOAD( "gotcha-u.41b", 0x0c0000, 0x20000, CRC(c093f04e) SHA1(e731714c9fe9b583a23e162a5513574e63d0f454) )
	ROM_CONTINUE(             0x1c0000, 0x20000 )
	ROM_CONTINUE(             0x0e0000, 0x20000 )
	ROM_CONTINUE(             0x1e0000, 0x20000 )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "gotcha.u56",   0x000000, 0x80000, CRC(85f6a062) SHA1(77d1c9c8394af0c487fa6d657ae740eae940682a) )
	ROM_LOAD( "gotcha.u55",   0x080000, 0x80000, CRC(426b4e48) SHA1(91e79c9fd1f9cf84df8e1d6b67780d1cacd4a0f2) )
	ROM_LOAD( "gotcha.u54",   0x100000, 0x80000, CRC(903e05a4) SHA1(4fb675958f4dc057f8da7edff1f6680482bdc5dd) )
	ROM_LOAD( "gotcha.u53",   0x180000, 0x80000, CRC(3c24d51e) SHA1(8b987db14a56950cc0f77e232e20fcdd89f98f2b) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "gotcha-u.z11", 0x000000, 0x80000, CRC(6111c6ae) SHA1(9170a37eaca56586da2f5e4894816640193c8802) )
ROM_END



GAME( 1997, gotcha, 0, gotcha, gotcha, 0, ROT0, "Dongsung", "Got-cha" )
