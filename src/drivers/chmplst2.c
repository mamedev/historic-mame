/*

Champion List II
IGS, 1996

PCB Layout
----------

IGS PCB NO-0115
|---------------------------------------------|
|                  M6295  IGSS0503.U38        |
|  UM3567  3.57945MHz                         |
|                          DSW3               |
|                          DSW2     PAL       |
| IGSM0502.U5              DSW1    6264       |
| IGSM0501.U7     PAL              6264       |
|                 PAL                         |
|                 PAL            IGS011       |
|                 PAL                         |
|                 PAL                         |
|                                             |
|   MC68HC000P10          22MHz  TC524258AZ-10|
|           6264         8255    TC524258AZ-10|
|    BATT   6264   MAJ2V185H.U29 TC524258AZ-10|
|                                TC524258AZ-10|
|---------------------------------------------|

Notes:
        68k clock: 7.3333MHz (i.e. 22/3)
      M6295 clock: 1.0476MHz (i.e. 22/21) \
         M6295 SS: HIGH                   / Therefore sampling freq = 7.936363636kHz (i.e. 1047600 / 132)
           UM3567: Compatible with YM2413, clock = 3.57945MHz
            HSync: 15.78kHz
            VSync: 60Hz

*/

#include "driver.h"

VIDEO_START(chmplst2)
{
	return 0;
}

VIDEO_UPDATE(chmplst2)
{

}

static ADDRESS_MAP_START( chmplst2_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x100000, 0x103fff) AM_RAM
ADDRESS_MAP_END

INPUT_PORTS_START( chmplst2 )
INPUT_PORTS_END

static INTERRUPT_GEN( chmplst2_interrupt )
{
	switch (cpu_getiloops())
	{
		case 0:
		cpunum_set_input_line(0, 5, HOLD_LINE);
		break;

		case 1:
		cpunum_set_input_line(0, 6, HOLD_LINE);
		break;
	}
}


static MACHINE_DRIVER_START( chmplst2 )
	MDRV_CPU_ADD(M68000, 22000000/3)
	MDRV_CPU_PROGRAM_MAP(chmplst2_map,0)
	MDRV_CPU_VBLANK_INT(chmplst2_interrupt,2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

//  MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 64*8-1)
	MDRV_PALETTE_LENGTH(0x800)

	MDRV_VIDEO_START(chmplst2)
	MDRV_VIDEO_UPDATE(chmplst2)
MACHINE_DRIVER_END

void decrypt_chmplst2(void)
{

	int i,j;
	data16_t *src = (data16_t *) (memory_region(REGION_CPU1));
	data16_t *result_data;

	int rom_size = 0x80000;

 	result_data = malloc(rom_size);

 	for(i=0; i<rom_size/2; i++) {
		unsigned short x = src[i];

		if((i & 0x0054) != 0x0000 && (i & 0x0056) != 0x0010)
			x ^= 0x0400;

		if((i & 0x0204) == 0x0000)
 			x ^= 0x0800;

		if((i & 0x3080) != 0x3080 && (i & 0x3090) != 0x3010)
			x ^= 0x2000;

		j = BITSWAP24(i,
			23,22,21,20,
			19,18,17,16,
			15,14,13, 8,
			11,10, 9, 2,
			7, 6, 5, 4,
			3,12, 1, 0);

    	result_data[j] = (x >> 8) | (x << 8);
	}

	memcpy(src,result_data,rom_size);

	free(result_data);
}

DRIVER_INIT( chmplst2 )
{
	decrypt_chmplst2();
}

ROM_START( chmplst2 )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD( "maj2v185h.u29",  0x00000, 0x80000,  CRC(2572d59a) SHA1(1d5362e209dadf8b21c10d1351d4bb038bfcaaef) )

	ROM_REGION( 0x200000, REGION_USER1, 0 ) /* other roms */
	ROM_LOAD( "igsm0501.u7",  0x00000, 0x200000,   CRC(1c952bd6) SHA1(a6b6f1cdfb29647e81c032ffe59c94f1a10ceaf8) )

	ROM_REGION( 0x80000, REGION_USER2, 0 ) /* other roms */
	/* these are identical ..seems ok as igs number is same, only ic changed */
	ROM_LOAD( "igsm0502.u4",  0x00000, 0x80000,  CRC(5d73ae99) SHA1(7283aa3d6b15ceb95db80756892be46eb997ef15) )
	ROM_LOAD( "igsm0502.u5",  0x00000, 0x80000,  CRC(5d73ae99) SHA1(7283aa3d6b15ceb95db80756892be46eb997ef15) )

	ROM_REGION( 0x300000, REGION_SOUND1, 0 ) /* OKI Samples */
	ROM_LOAD( "igss0503.u38",  0x00000, 0x80000,  CRC(c9609c9c) SHA1(f036e682b792033409966e84292a69275eaa05e5) )
ROM_END

GAMEX( 1996, chmplst2, 0, chmplst2, chmplst2, chmplst2, ROT0, "IGS", "Champion List II", GAME_NOT_WORKING | GAME_NO_SOUND  )
