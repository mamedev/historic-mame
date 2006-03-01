/*
Parallel Turn
(c) Jaleco, 1984
Preliminary driver by Tomasz Slanina

Custom Jaleco chip is some kind of state machine
used for calculate jump offsets.

Top PCB
-------

PCB No. PT-8418
CPU:  Z80A
SND:  AY-3-8010 x 2, Z80A
DIPS: 8 position x 2
RAM:  2114 x 2, MSM2128 x 1 (equivalent to 6116)

Other: Reset Switch near edge connector

       Custom JALECO chip (24 pin DIP) near RAM MSM2128, verified to be NOT 2128 ram.

       Pinouts are :
       Pin 1 hooked to pin 3 of ROM 7
       Pin 2 hooked to pin 4 of ROM 7
       Pin 3 hooked to pin 5 of ROM 7
       Pin 4 hooked to pin 6 of ROM 7
       Pin 5 hooked to pin 7 of ROM 7
       Pin 6 hooked to pin 8 of ROM 7
       Pin 7 hooked to pin 9 of ROM 7
       Pin 8 hooked to pin 10 of ROM 7
       Pin 9 hooked to pin 11 of ROM 7
       Pin 10 hooked to pin 12 of ROM 7
       Pin 11 hooked to pin 13 of ROM 7
       Pin 12 GND
       Pin 13 hooked to pin 13 of 2128 and pin 15 of ROM 7
       Pin 14 hooked to pin 14 of 2128 and pin 16 of ROM 7
       Pin 15 hooked to pin 17 of ROM 7
       Pin 16 hooked to pin 18 of ROM 7
       Pin 17 hooked to pin 19 of ROM 7
       Pin 18 NC
       Pin 19 NC
       Pin 20 hooked to pin 11 of 74LS32 at 4F
       Pin 21 hooked to pin 8 of 74LS32 at 4F
       Pin 22 hooked to pin 24 of ROM 7
       Pin 23 hooked to pin 25 of ROM 7
       Pin 24 +5

NOTE: The archive contains a different ROM7 that was in another archive.
      (I merged all archives since they are identical other than ROM 7 in one archive named 7.BIN)
      Perhaps this is from a bootleg PCB with a workaround for the custom JALECO chip?


ROMS: All ROM labels say only "PROM" and a number.
      1, 2, 3 and 9 near Z80 at 5K, all 2732's
      4, 5, 6 & 7 near custom JALECO chip and Z80 at 3D, all 2764's
      prom_red.3p type TBP24S10
      prom_grn.4p type N82S129
      prom_blu.4r type TBP24S10


LOWER PCB
---------

PCB:  PT-8419
XTAL: ? Stamped KSS5, connected to pins 8 & 13 of 74LS37 at 9P
      Also connected to 2 x 330 Ohm resistors which are in turn connected to pins
      9 & 11 of 9P. Pins 9 & 11 of 9P are joined with a 101 ceramic capacitor.
      UPDATE! When I substitute various speed xtals, a 8.000MHz xtal allows the PCB to work fine.

RAM:  AM93422DC x 2, MB7063 x 1, 2114 x 4

ROMS: All ROM labels say only "PROM" and a number.
      10, 14, 15 & 16 type 2764
      11, 12, 13 type 2732

*/
#include "driver.h"
#include "sound/ay8910.h"

tilemap *pturn_tilemap,*pturn_bgmap;
static void get_pturn_tile_info(int tile_index)
{
	int tileno,palno;
	tileno = videoram[tile_index]; /* wrong .. tile rom(s) adr lines should be swapped */
	if(tileno==0)
		tileno=16;
	palno=7;
	SET_TILE_INFO(0,tileno,palno,0)
}

static int bgbank=0;

static void get_pturn_bg_tile_info(int tile_index)
{
	int tileno,palno;
	tileno = memory_region(REGION_USER1)[tile_index];
	palno=1;
	SET_TILE_INFO(1,tileno+bgbank,palno,0)
}

VIDEO_START(pturn)
{
	pturn_tilemap = tilemap_create(get_pturn_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT, 8, 8,32,32);
	tilemap_set_transparent_pen(pturn_tilemap,0);
	pturn_bgmap = tilemap_create(get_pturn_bg_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE, 8, 8,32,32*8);
	return 0;
}

VIDEO_UPDATE(pturn)
{
	int offs;
	int sx, sy;

	for ( offs = 0x80-4 ; offs >=0 ; offs -= 4)
	{
		sy=spriteram[offs] - 16;
		sx=spriteram[offs+3] - 16;

		if(sx && sy)
		{
			drawgfx(bitmap, Machine->gfx[2],
			spriteram[offs+1] & 0x3f ,
			spriteram[offs+2] & 0x1f,
			0,0,
			sx,sy,
			cliprect,TRANSPARENCY_PEN,0);
		}
	}

	tilemap_draw(bitmap,cliprect,pturn_bgmap,0,0);
	tilemap_draw(bitmap,cliprect,pturn_tilemap,0,0);
}


WRITE8_HANDLER( pturn_videoram_w )
{
	videoram[offset]=data;
	tilemap_mark_tile_dirty(pturn_tilemap,offset);
}

static int nmi_main, nmi_sub;

static WRITE8_HANDLER( nmi_main_enable_w )
{
	nmi_main = data;
}

static WRITE8_HANDLER( nmi_sub_enable_w )
{
	nmi_sub = data;
}

static INTERRUPT_GEN( pturn_main_intgen )
{
	if (nmi_main)
		cpunum_set_input_line(0,INPUT_LINE_NMI,PULSE_LINE);
}

INPUT_PORTS_START( pturn )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH,IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH,IPT_COIN2 )
INPUT_PORTS_END


static WRITE8_HANDLER(sound_w)
{
	soundlatch_w(0,data);
	cpunum_set_input_line(1,INPUT_LINE_NMI,PULSE_LINE);
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0xc000, 0xc7ff) AM_RAM
	AM_RANGE(0xc800, 0xcfff) AM_NOP /* custom jaleco chip */
	AM_RANGE(0xe000, 0xe3ff) AM_WRITE(pturn_videoram_w) AM_READ(videoram_r) AM_BASE(&videoram)
	AM_RANGE(0xe400, 0xe400) AM_WRITENOP /* ?? */
	AM_RANGE(0xe800, 0xe800) AM_WRITE(sound_w)
	AM_RANGE(0xec00, 0xec0f) AM_NOP /* ?? */
	AM_RANGE(0xf000, 0xf0ff) AM_RAM AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xf400, 0xf400) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xf800, 0xf800) AM_NOP /* input ports */
	AM_RANGE(0xf801, 0xf801) AM_NOP
	AM_RANGE(0xf802, 0xf802) AM_READ(input_port_0_r)
	AM_RANGE(0xf803, 0xf803) AM_NOP
	AM_RANGE(0xf804, 0xf804) AM_NOP
	AM_RANGE(0xf805, 0xf805) AM_NOP
	AM_RANGE(0xf806, 0xf806) AM_NOP
	AM_RANGE(0xfc00, 0xfcff) AM_READ(MRA8_RAM)
	AM_RANGE(0xfc00, 0xfc00) AM_WRITENOP
	AM_RANGE(0xfc01, 0xfc01) AM_WRITE(nmi_main_enable_w)
	AM_RANGE(0xfc02, 0xfcff) AM_WRITENOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( sub_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_ROM
	AM_RANGE(0x2000, 0x23ff) AM_RAM
	AM_RANGE(0x3000, 0x3000) AM_READ(soundlatch_r) AM_WRITE(nmi_sub_enable_w)
	AM_RANGE(0x4000, 0x4000) AM_RAM
	AM_RANGE(0x5000, 0x5000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x5001, 0x5001) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x6000, 0x6000) AM_WRITE(AY8910_control_port_1_w)
	AM_RANGE(0x6001, 0x6001) AM_WRITE(AY8910_write_port_1_w)
	AM_RANGE(0xffe0, 0xffff) AM_RAM
ADDRESS_MAP_END

static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3), RGN_FRAC(1,3), RGN_FRAC(2,3) },
	{ 0,1,2,3, 4,5,6,7 },
	{ 0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8 },
	8*8
};

static const gfx_layout spritelayout =
{
	32,32,
	RGN_FRAC(1,3),
	3,
	{ RGN_FRAC(0,3),RGN_FRAC(1,3),RGN_FRAC(2,3) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 24*8+4, 24*8+5, 24*8+6, 24*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			32*8, 33*8, 34*8, 35*8, 36*8, 37*8, 38*8, 39*8,
			64*8, 65*8, 66*8, 67*8, 68*8, 69*8, 70*8, 71*8,
			96*8, 97*8, 98*8, 99*8, 100*8, 101*8, 102*8, 103*8 },
	128*8
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,   0x000, 32 },
	{ REGION_GFX2, 0, &charlayout,   0x000, 32 },
	{ REGION_GFX3, 0, &spritelayout, 0x000, 32 },
	{ -1 }
};


static MACHINE_DRIVER_START( pturn )
	MDRV_CPU_ADD(Z80, 12000000/3)
	MDRV_CPU_PROGRAM_MAP(main_map,0)
	MDRV_CPU_VBLANK_INT(pturn_main_intgen,1)

	MDRV_CPU_ADD(Z80, 12000000/3)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sub_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x100)
	MDRV_PALETTE_INIT(RRRR_GGGG_BBBB)

	MDRV_VIDEO_START(pturn)
	MDRV_VIDEO_UPDATE(pturn)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END


ROM_START( pturn )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "prom4.8d", 0x00000,0x2000, CRC(d3ae0840) SHA1(5ac5f2626de7865cdf379cf15ae3872e798b7e25))
	ROM_LOAD( "prom6.8b", 0x02000,0x2000, CRC(65f09c56) SHA1(c0a7a1bfaacfc4af14d8485e2b5f2c604937a1e4))
	ROM_LOAD( "prom5.7d", 0x04000,0x2000, CRC(de48afb4) SHA1(9412288b63cf3ae8c9522b1fcacc4aa36ac7a23c))
	ROM_LOAD( "prom7.7b", 0x06000,0x2000, CRC(bfaeff9f) SHA1(63972c311f28971e121fbccd4c0d78edbdb6bdb4))

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "prom9.5n", 0x00000,0x1000, CRC(8b4d944e) SHA1(6f956d972c2c2ef875378910b80ca59701710957))

	ROM_REGION( 0x3000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "prom1.8k", 0x00000,0x1000, CRC(10aba36d) SHA1(5f9ce00365b3be91f0942b282b3cfc0c791baf98))
	ROM_LOAD( "prom2.7k", 0x01000,0x1000, CRC(b8a4d94e) SHA1(78f9db58ceb4a87ab2744529b0e7ad3eb826e627))
	ROM_LOAD( "prom3.6k", 0x02000,0x1000, CRC(9f51185b) SHA1(84690556da013567133b7d8fcda25b9fb831e4b0))

	ROM_REGION( 0x3000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "prom11.16f", 0x000000, 0x01000, CRC(129122c6) SHA1(feb6d9abddb4d888b49861a32a063d009ca994aa) )
	ROM_LOAD( "prom12.16h", 0x001000, 0x01000, CRC(69b09323) SHA1(726749b625052984e1d8c71eb69511c35ca75f9c) )
	ROM_LOAD( "prom13.16k", 0x002000, 0x01000, CRC(e9f67599) SHA1(b2eb144c8ce9ff57bd66ba57705d5e242115ef41) )

	ROM_REGION( 0x6000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "prom14.16l", 0x000000, 0x02000, CRC(ffaa0b8a) SHA1(20b1acc2562e493539fe34d056e6254e4b2458be) )
	ROM_LOAD( "prom15.16m", 0x002000, 0x02000, CRC(41445155) SHA1(36d81b411729447ca7ff712ac27d8a0f2015bcac) )
	ROM_LOAD( "prom16.16p", 0x004000, 0x02000, CRC(94814c5d) SHA1(e4ab6c0ae94184d5270cadb887f56e3550b6d9f2) )

	ROM_REGION( 0x0300, REGION_PROMS, 0 )
  	ROM_LOAD( "prom_red.3p", 0x0000, 0x0100, CRC(505fd8c2) SHA1(f2660fe512c76412a7b9f4be21fe549dd59fbda0) )
	ROM_LOAD( "prom_grn.4p", 0x0100, 0x0100, CRC(6a00199d) SHA1(ff0ac7ae83d970778a756f7445afed3785fc1150) )
	ROM_LOAD( "prom_blu.4r", 0x0200, 0x0100, CRC(7b4c5788) SHA1(ca02b12c19be7981daa070533455bd4d227d56cd) )

	ROM_REGION( 0x2000, REGION_USER1, 0 )
	ROM_LOAD( "prom10.16d", 0x0000,0x2000, CRC(a96e3c95) SHA1(a3b1c1723fcda80c11d9858819659e5e9dfe5dd3))

ROM_END


READ8_HANDLER (pturn_hack_r)
{
	return 0x66;
}

static DRIVER_INIT(pturn)
{
	memory_install_read8_handler(0, ADDRESS_SPACE_PROGRAM, 0xc0dd, 0xc0dd, 0, 0, pturn_hack_r); /* initial protection check */
}

GAME( 1984, pturn,  0, pturn,  pturn,  pturn, ROT90,   "Jaleco", "Parallel Turn",GAME_NOT_WORKING )
