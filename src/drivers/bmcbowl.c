/*

 BMC (BMT?) Bowling

 .. finish me ..

 not sure about much here .. seems it can read the gfx roms at least

 roms contain lots of strings relating to security error

 data gets copied from the roms to the other regions .. mapping could be wrong


BMC Bowling (c) 1994.05 BMC, Ltd

Chips:
MC68000P10
Goldstar GM68B45S (same as Hitachi HD6845 CTR Controller)*
Winbond WF19054 (same as AY3-8910)
MK28 (appears to be a AD-65, AKA OKI6295) next to rom 10
Synertek SY6522 VIA
9325-AG (Elliptical Filter)
KDA0476BCN-66 (unkown)
part # scratched 64 pin PLCC next to 7EX & 8 EX

Ram:
Goldstar GM76C28A (2Kx8 SRAM) x3
HM62256LP-12 x6

OSC:
GREAT1 21.47727
13.3M
3.579545

DIPS:
Place for 4 8 switch dips
dips 1 & 3 are all connected via resitors
dips 2 & 4 are standard 8 switch dips

EEPROM       Label         Use
----------------------------------------
ST M27C1001  bmc_8ex.bin - 68K code 0x00
ST M27C1001  bmc_7ex.bin - 68K code 0x01
ST M27C512   bmc_3.bin\
ST M27C512   bmc_4.bin | Graphics
ST M27C512   bmc_5.bin |
ST M27C512   bmc_6.bin/
HM27C101AG   bmc_10.bin - Sound samples

BrianT

* There is a MESS driver for this chip (gm68b45s CTR controller):
http://cvs.mess.org:6502/cgi-bin/viewcvs.cgi/mess/vidhrdw/m6845.c?rev=1.10


*/

#include "driver.h"
#include "machine/random.h"

data16_t *bmcbowl_vid1;
data16_t *bmcbowl_vid2;

//FILE *bmcbowl_log;


VIDEO_START( bmcbowl )
{
	return 0;
}

/* might not be a bitmap .. this is just to see whats going on */
VIDEO_UPDATE( bmcbowl )
{

	int x,y,z;

	z=0;
	for (y=0;y<512;y++)
	{
		for (x=0;x<40;x++)
		{
			int pixdat;

			pixdat = bmcbowl_vid1[z];
			z++;
			plot_pixel(bitmap, x,   y, pixdat&0xff);



		}

	}

}

static READ16_HANDLER( bmc_random_read )
{
	return mame_rand();
}

static WRITE16_HANDLER( bmc_unknown_write )
{
//	fwrite(&data, 1, 1, bmcbowl_log);
}

static ADDRESS_MAP_START( bmcbowl_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x092000, 0x092001) AM_READ(bmc_random_read)
	AM_RANGE(0x140000, 0x1bffff) AM_READ(MRA16_ROM) // reads gfx roms here?

	AM_RANGE(0x1c0000, 0x1dffff) AM_READ(MRA16_RAM)
	AM_RANGE(0x200000, 0x21ffff) AM_READ(MRA16_RAM)

	AM_RANGE(0x1f0000, 0x1f3fff) AM_READ(MRA16_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( bmcbowl_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_WRITE(MWA16_ROM)

	AM_RANGE(0x090002, 0x090003) AM_WRITE(bmc_unknown_write)

	AM_RANGE(0x1c0000, 0x1dffff) AM_WRITE(MWA16_RAM) AM_BASE(&bmcbowl_vid1) // 2 tilemaps? or sprites? or bitmaps?

	AM_RANGE(0x1f0000, 0x1f3fff) AM_WRITE(MWA16_RAM)

	AM_RANGE(0x200000, 0x21ffff) AM_WRITE(MWA16_RAM) AM_BASE(&bmcbowl_vid2) // ?? populated at startup
ADDRESS_MAP_END

INPUT_PORTS_START( bmcbowl )
INPUT_PORTS_END

static struct GfxLayout bmcbowl_layout =
{
	8,8,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 0,4,8,12,16,20,24,28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	8*32
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &bmcbowl_layout,   0x0, 2  }, /* bg tiles */
	{ -1 } /* end of array */
};


static struct OKIM6295interface okim6295_interface =
{
	1,				/* 1 chip */
	{ 8500 },		/* frequency (Hz) */
	{ REGION_SOUND1 },	/* memory region */
	{ 47 }
};

INTERRUPT_GEN ( bmc_interrupt )
{
	if (cpu_getiloops())
		cpunum_set_input_line(0, 4, HOLD_LINE);
//	else
//		cpunum_set_input_line(0, 4, HOLD_LINE);
}

static MACHINE_DRIVER_START( bmcbowl )
	MDRV_CPU_ADD_TAG("main", M68000, 10000000)	 // ?
	MDRV_CPU_PROGRAM_MAP(bmcbowl_readmem,bmcbowl_writemem)
	MDRV_CPU_VBLANK_INT(bmc_interrupt,1) // lev 2 / 4 look valid

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(128*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 64*8-1)
	MDRV_PALETTE_LENGTH(0x200)

	MDRV_VIDEO_START(bmcbowl)
	MDRV_VIDEO_UPDATE(bmcbowl)

	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(OKIM6295, okim6295_interface)
MACHINE_DRIVER_END

ROM_START( bmcbowl )
	ROM_REGION( 0x200000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "bmc_8ex.bin", 0x000000, 0x10000, CRC(8b1aa5db) SHA1(879df950bedf2c163ba89d983ca4a0691b01c46e) )
	ROM_LOAD16_BYTE( "bmc_7ex.bin", 0x000001, 0x10000, CRC(7726d47a) SHA1(8438c3345847c2913c640a29145ec8502f6b01e7) )
	ROM_LOAD16_BYTE( "bmc_6.bin", 0x180000, 0x20000, CRC(7b9e0d77) SHA1(1ec1c92c6d4c512f7292b77e9663518085684ba9) )
	ROM_LOAD16_BYTE( "bmc_5.bin", 0x180001, 0x20000, CRC(708b6f8b) SHA1(4a910126d87c11fed99f44b61d51849067eddc02) )
	ROM_LOAD16_BYTE( "bmc_4.bin", 0x140000, 0x10000, CRC(f43880d6) SHA1(9e73a29baa84d417ff88026896d852567a38e714) )
	ROM_LOAD16_BYTE( "bmc_3.bin", 0x140001, 0x10000, CRC(d1af9410) SHA1(e66b3ddd9d9e3c567fdb140c4c8972c766f2b975) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Samples */
	ROM_LOAD( "bmc_10.bin", 0x00000, 0x20000,  CRC(f840c17f) SHA1(82891a85c8dc60f727b5a8c8e8ab09e8e4bd8af4) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
//	ROM_LOAD( "moo", 0x00000, 0x20000, CRC(1) SHA1(1) )
ROM_END


DRIVER_INIT (bmcbowl)
{
//	bmcbowl_log=fopen("bmcbowl.log", "w+b");
}

GAMEX( 1994, bmcbowl,    0, bmcbowl,    bmcbowl,    bmcbowl, ROT0,  "BMC", "BMC Bowling", GAME_NOT_WORKING )
