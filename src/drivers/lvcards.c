/***************************************************************************

Pontoon Memory Map (preliminary)

driver by Zsolt Vasvari

Enter switch test mode by holding down the Hit key, and when the
crosshatch pattern comes up, release it and push it again.

After you get into Check Mode (F2), press the Hit key to switch pages.


Memory Mapped:

0000-5fff   R   ROM
6000-67ff   RW  Battery Backed RAM
8000-83ff   RW  Video RAM
8400-87ff   RW  Color RAM
                Bits 0-3 - color
                Bits 4-5 - character bank
                Bit  6   - flip x
                Bit  7   - Is it used?
a000        R   Input Port 0
a001        R   Input Port 1
a002        R   Input Port 2
a001         W  Control Port 0
a002         W  Control Port 1

I/O Ports:
00          RW  YM2149 Data Port
                Port A - DSW #1
                Port B - DSW #2
01           W  YM2149 Control Port


TODO:

- What do the control ports do?
- CPU speed/ YM2149 frequencies
- Input ports need to be cleaned up
- Work needs to be done to find the hopper overflow sensors, as fruit machine emulation precedent suggests
that these switches be set 'always on'

- NVRAM does not work for lvcards - I'm starting to think that this is FAO - for amusement only.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sound/ay8910.h"


extern WRITE8_HANDLER( lvcards_videoram_w );
extern WRITE8_HANDLER( lvcards_colorram_w );
extern PALETTE_INIT( lvcards );
extern VIDEO_START( lvcards );
extern VIDEO_UPDATE( lvcards );


static UINT8 *nvram;
static size_t nvram_size;

static NVRAM_HANDLER( lvcards )
{
	if (read_or_write)
		mame_fwrite(file, nvram, nvram_size);
	else
	{
		if (file)
        	mame_fread(file, nvram, nvram_size);
		else
			memset(nvram, 0xff, nvram_size);
	}
}


static ADDRESS_MAP_START( lvcards_map, ADDRESS_SPACE_PROGRAM, 8  )
	AM_RANGE(0x0000, 0x5fff) AM_ROM
	AM_RANGE(0x6000, 0x67ff) AM_RAM AM_BASE(&nvram) AM_SIZE(&nvram_size)
	AM_RANGE(0x9000, 0x93ff) AM_RAM AM_WRITE(lvcards_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x9400, 0x97ff) AM_RAM AM_WRITE(lvcards_colorram_w) AM_BASE(&colorram)
	AM_RANGE(0xa000, 0xa000) AM_READ(input_port_0_r)
	AM_RANGE(0xa001, 0xa001) AM_READ(input_port_1_r) AM_WRITENOP
	AM_RANGE(0xa002, 0xa002) AM_READ(input_port_2_r) AM_WRITENOP
	AM_RANGE(0xc000, 0xdfff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( lvcards_io_map, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(AY8910_read_port_0_r, AY8910_write_port_0_w)
	AM_RANGE(0x01, 0x01) AM_WRITE(AY8910_control_port_0_w)
ADDRESS_MAP_END


INPUT_PORTS_START( lvcards )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Analyzer") PORT_CODE(KEYCODE_0)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Red") PORT_CODE(KEYCODE_N)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Black") PORT_CODE(KEYCODE_M)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Hold 1") PORT_CODE(KEYCODE_Z)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Hold 2") PORT_CODE(KEYCODE_X)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Hold 3") PORT_CODE(KEYCODE_C)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Hold 4") PORT_CODE(KEYCODE_V)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Hold 5") PORT_CODE(KEYCODE_B)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Deal / Double") PORT_CODE(KEYCODE_LALT)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Bet") PORT_CODE(KEYCODE_LCONTROL)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_OTHER ) PORT_NAME("Cancel / Take Score") PORT_CODE(KEYCODE_SPACE)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(3)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW0")
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
	PORT_DIPNAME( 0x80, 0x80, "Reset?" )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x07, 0x07, "1 COIN =" )
	PORT_DIPSETTING(    0x06, "5$" )
	PORT_DIPSETTING(    0x05, "10$" )
	PORT_DIPSETTING(    0x04, "15$" )
	PORT_DIPSETTING(    0x03, "20$" )
	PORT_DIPSETTING(    0x07, "25$" )
	PORT_DIPSETTING(    0x02, "50$" )
	PORT_DIPSETTING(    0x01, "75$" )
	PORT_DIPSETTING(    0x00, "100$" )
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
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )
INPUT_PORTS_END


static gfx_layout charlayout =
{
	8,8,    /* 8*8 characters */
	2048,   /* 2048 characters */
	4,      /* 4 bits per pixel */
	{0,1,2,3},
	{4,0,12,8,20,16,28,24},
	{32*0, 32*1, 32*2, 32*3, 32*4, 32*5, 32*6, 32*7},
	32*8
};

static gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout, 0, 16 },
	{ -1 }
};


static struct AY8910interface lcay8910_interface =
{
	input_port_3_r,
	input_port_4_r
};


static MACHINE_DRIVER_START( lvcards )
	// basic machine hardware
 	MDRV_CPU_ADD(Z80, 18432000/6)	// 3.072 MHz

	MDRV_CPU_PROGRAM_MAP(lvcards_map, 0)
	MDRV_CPU_IO_MAP(lvcards_io_map, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	// video hardware
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(8*0, 32*8-1, 2*8, 30*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(256)
	MDRV_COLORTABLE_LENGTH(256)
	MDRV_NVRAM_HANDLER(lvcards)

	MDRV_PALETTE_INIT(lvcards)
	MDRV_VIDEO_START(lvcards)
	MDRV_VIDEO_UPDATE(lvcards)

	// sound hardware
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 18432000/12)
	MDRV_SOUND_CONFIG(lcay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END


ROM_START( lvcards )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "lc1.bin", 0x0000, 0x4000, CRC(0c5fbf05) SHA1(bf996bdccfc5748cee91d004f2b1da10bcd8e329) )
	ROM_LOAD( "lc2.bin", 0x4000, 0x2000, CRC(deb54548) SHA1(a245898635c5cd3c26989c2bba89bb71edacd906) )
	ROM_LOAD( "lc3.bin", 0xc000, 0x2000, CRC(45c2bea9) SHA1(3a33501824769656aa87649c3fd0a8b8a4d83f3c) )

	ROM_REGION( 0x10000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "lc4.bin", 0x0000, 0x2000, CRC(dd705389) SHA1(271c11c2bd9affd976d65e318fd9fb01dbdde040) )
	ROM_CONTINUE(		 0x8000, 0x2000 )
	ROM_LOAD( "lc5.bin", 0x2000, 0x2000, CRC(ddd1e3e5) SHA1(b7e8ccaab318b61b91eae4eee9e04606f9717037) )
	ROM_CONTINUE(		 0xa000, 0x2000 )
	ROM_LOAD( "lc6.bin", 0x4000, 0x2000, CRC(2991a6ec) SHA1(b2c32550884b7b708db48bb7f0854bbad504417d) )
	ROM_RELOAD(			 0xc000, 0x2000 )
	ROM_LOAD( "lc7.bin", 0x6000, 0x2000, CRC(f1b84c56) SHA1(6834139400bf8aa8db17f65bfdcbdcb2433d5fdc) )
	ROM_RELOAD(			 0xe000, 0x2000 )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
	ROM_LOAD( "3.7c", 0x0000,  0x0100, CRC(0c2ead4a) SHA1(e8e29e21366622c9bf3ee5e807c83b5cd7da8e3e) )
	ROM_LOAD( "2.7b", 0x0100,  0x0100, CRC(f4bc51e2) SHA1(38293c1117d6f3076081b6f2da3f42819558b04f) )
	ROM_LOAD( "1.7a", 0x0200,  0x0100, CRC(e40f2363) SHA1(cea598b6ed037dd3b4306c2ca3b0b4d5197d42a4) )
ROM_END


GAME( 1985, lvcards, 0, lvcards, lvcards, 0, ROT0, "Tehkan", "Lovely Cards" )
