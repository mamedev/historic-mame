/*

Virtua Bowling by IGS

PCB # 0101

U45  (27c240) is probably program
next to 68000 processor
U68,U69 probably images   (27c800 - mask)
U67, U66 sound and ????  (27c040)



ASIC chip used

SMD - custom chip IGS 011      F5XD  174
SMD - custom --near sound section - unknown -- i.d. rubbed off
SMD - custom  -- near inputs and 68000  IGS 012    9441EK001

XTL near sound 33.868mhz
XTL near 68000  22.0000mhz

there are 4 banks of 8 dip switches

--
the dump looks like crap ;-)

*/

#include "driver.h"

VIDEO_START(vbowl)
{
	return 0;
}

VIDEO_UPDATE(vbowl)
{

}

static ADDRESS_MAP_START( vbowl_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x100000, 0x103fff) AM_RAM
ADDRESS_MAP_END

INPUT_PORTS_START( vbowl )
INPUT_PORTS_END

static INTERRUPT_GEN( vbowl_interrupt )
{
	switch (cpu_getiloops())
	{
		case 0:
		cpunum_set_input_line(0, 1, HOLD_LINE);
		break;

		case 1:
		cpunum_set_input_line(0, 2, HOLD_LINE);
		break;
	}
}


static MACHINE_DRIVER_START( vbowl )
	MDRV_CPU_ADD(M68000, 11000000)
	MDRV_CPU_PROGRAM_MAP(vbowl_map,0)
	MDRV_CPU_VBLANK_INT(vbowl_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

//  MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 64*8-1)
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START(vbowl)
	MDRV_VIDEO_UPDATE(vbowl)
MACHINE_DRIVER_END

void decrypt_vbowl(void)
{

	int i;
	UINT16 *src = (UINT16 *) (memory_region(REGION_CPU1));

	int rom_size = 0x80000;

	for(i=0; i<rom_size/2; i++) {
		UINT16 x = src[i];

		if((i & 0x4100) == 0x0100)
			x ^= 0x0002;

		if((i & 0x4000) == 0x4000 && (i & 0x0300) != 0x0100)
			x ^= 0x0002;

		if((i & 0x5700) == 0x5100)
			x ^= 0x0002;

		if((i & 0x5500) == 0x1000)
			x ^= 0x0002;

		if((i & 0x0140) != 0x0000 || (i & 0x0012) == 0x0012)
			x ^= 0x0400;

		if((i & 0x2004) != 0x2004 || (i & 0x0090) == 0x0000)
			x ^= 0x2000;

	    src[i] = (x << 8) | (x >> 8);
	  }
}

DRIVER_INIT( vbowl )
{
	decrypt_vbowl();
}

ROM_START( vbowl )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD( "vrbowlng.u45",  0x00000, 0x80000, CRC(091c19c1) SHA1(5a7bfbee357122e9061b38dfe988c3853b0984b0) ) // second half all 00

	ROM_REGION( 0x300000, REGION_USER1, 0 ) /* other roms, i haven't sorted them, u69 looks like it might be bad */
	ROM_LOAD( "vrbowlng.u66",  0x000000, 0x080000, CRC(f62cf8ed) SHA1(c53e47e2c619ed974ad40ee4aaa4a35147ea8311) )
	ROM_LOAD( "vrbowlng.u67",  0x080000, 0x080000, CRC(53000936) SHA1(e50c6216f559a9248c095bdfae05c3be4be79ff3) )
	ROM_LOAD( "vrbowlng.u68",  0x100000, 0x100000, CRC(b0ce27e7) SHA1(6d3ef97edd606f384b1e05b152fbea12714887b7) )
	ROM_LOAD( "vrbowlng.u69",  0x200000, 0x100000, BAD_DUMP CRC(19172e3b) SHA1(da9297621a98835c0d27b4d4a90d295a284c7ef6) ) // FIXED BITS (xxxxxxxx11111111)  maybe bad
ROM_END

GAME( 1996, vbowl, 0, vbowl, vbowl, vbowl, ROT0, "Alta / IGS", "Virtua Bowling", GAME_NOT_WORKING | GAME_NO_SOUND  )
