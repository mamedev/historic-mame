/********************************************************************
	Preliminary Hyperstone based dfPix games driver

 Games dumped
 - X-Files

 Known games not dumped
 - Elfin (c) 1999

*********************************************************************/

#include "driver.h"

static data32_t hyperstone_iram[0x1000];

static WRITE32_HANDLER( hyperstone_iram_w )
{
	COMBINE_DATA(&hyperstone_iram[offset&0xfff]);
}

static READ32_HANDLER( hyperstone_iram_r )
{
	return hyperstone_iram[offset&0xfff];
}

static ADDRESS_MAP_START( dfpix_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x00ffffff) AM_RAM
	AM_RANGE(0x40000000, 0x4007ffff) AM_RAM
	AM_RANGE(0x80000000, 0x800fffff) AM_RAM
	AM_RANGE(0xc0000000, 0xdfffffff) AM_READ(hyperstone_iram_r) AM_WRITE(hyperstone_iram_w)
	AM_RANGE(0xe0000000, 0xe1f00003) AM_RAM
	AM_RANGE(0xffc00000, 0xffffffff) AM_READ(MRA32_BANK1)
ADDRESS_MAP_END

INPUT_PORTS_START( dfpix )
INPUT_PORTS_END

VIDEO_UPDATE( dfpix )
{

}

static INTERRUPT_GEN( test_interrupts )
{
	cpunum_set_input_line(0, cpu_getiloops(), PULSE_LINE);
}


static MACHINE_DRIVER_START( dfpix )
	MDRV_CPU_ADD(E132XS, 100000000*5)		 /* ?? */
	MDRV_CPU_PROGRAM_MAP(dfpix_map,0)
	MDRV_CPU_VBLANK_INT(test_interrupts, 8)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)

	MDRV_PALETTE_LENGTH(0x8000)

	MDRV_VIDEO_UPDATE(dfpix)
MACHINE_DRIVER_END


/*

X-Files
dfPIX Entertainment Inc. 1999

Contrary to what you might think on first hearing the title, this game
is like Match It 2 etc. However, the quality of the graphics
is outstanding, perhaps the most high quality seen in this "type" of game.
At the end of the level, you are presented with a babe, where you can use
the joystick and buttons to scroll up and down and zoom in for erm...
a closer inspection of the 'merchandise' ;-))


PCB Layout
----------


VRenderOMinus Rev4
-------------------------------------------------------
|                                                     |
|   DA1545A             C-O-N-N-1                 C   |
|                                                 O   |
|  POT1    T2316162               SEC KS0164      N   |
|  POT2    T2316162                               N   |
|J                                    169NDK19:   3   |
|A     14.31818MHz                     CONN2          |
|M  KA4558                                            |
|M                                                    |
|A                                SEC KM6161002CJ-12  |
|          E1-32XT                                    |
|                                 SEC KM6161002CJ-12  |
|                                                     |
|       ST7705C                   SEC KM6161002CJ-12  |
| B1             XCS05                                |
| B2 B3          14.31818MHz      SEC KM6161002CJ-12  |
-------------------------------------------------------


Notes
-----
ST7705C          : EEPROM?
E1-32XT          : Hyperstone E1-32XT CPU
169NDK19         : Xtal, 16.9MHz
CONN1,CONN2,CONN3: Connectors for small daughterboard containing
                   3x DA28F320J5 (32M surface mounted SSOP56 Flash ROM)
XCS05            : XILINX XCS05 PLD
B1,B2,B3         : Push Buttons for TEST, SERVICE and RESET
SEC KS0164       : Manufactured by Samsung Electronics. Possibly sound
                   related or Sound CPU? (QFP100)
T2316162         : Main program RAM (SOJ44)
SEC KM6161002    : Graphics RAM (SOJ44)

*/

ROM_START( xfiles )
	ROM_REGION32_BE( 0x400000, REGION_USER1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD16_WORD_SWAP( "u9.bin", 0x000000, 0x400000, CRC(ebdb75c0) SHA1(9aa5736bbf3215c35d62b424c2e5e40223227baf) )

	/* the following probably aren't in the right regions etc. */

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROM_LOAD16_WORD_SWAP( "u8.bin", 0x000000, 0x400000, CRC(3b2c2bc1) SHA1(1c07fb5bd8a8c9b5fb169e6400fef845f3aee7aa) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD16_WORD_SWAP( "u10.bin", 0x000000, 0x400000, CRC(f2ef1eb9) SHA1(d033d140fce6716d7d78509aa5387829f0a1404c) )
ROM_END


DRIVER_INIT( dfpix )
{
	cpu_setbank(1, memory_region(REGION_USER1));
}

GAMEX( 199?, xfiles, 0, dfpix, dfpix, dfpix, ROT0, "dfPIX Entertainment Inc.", "X-Files", GAME_NO_SOUND | GAME_NOT_WORKING )
