/********************************************************************
 Preliminary Hyperstone based dgPix games driver

 Games dumped
 - X-Files

 Known games not dumped
 - Elfin (c) 1999

*********************************************************************/

#include "driver.h"
#include "machine/random.h"
#include "vidhrdw/generic.h"

static data32_t *vram;

static ADDRESS_MAP_START( dgpix_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x00ffffff) AM_RAM
	AM_RANGE(0x40000000, 0x400fffff) AM_RAM AM_BASE(&vram)
	AM_RANGE(0x80000000, 0x801fffff) AM_RAM
	AM_RANGE(0xe0000000, 0xe1f00003) AM_RAM
	AM_RANGE(0xe1f80000, 0xe1f8ffff) AM_RAM
	AM_RANGE(0xffc00000, 0xffffffff) AM_READ(MRA32_BANK1)
ADDRESS_MAP_END

static UINT32 io2;
static int frame_hack=0;

static READ32_HANDLER( dgio_r )
{
		

	switch(offset)
	{
	//	case 0x1c/4:	return io2;
		case 0x400/4: return (((++frame_hack)>>14)&1)|mame_rand(); //vblank flag ?
		default: return 0xffffffff;
	}
}

static WRITE32_HANDLER( dgio_w )
{
	switch(offset)
	{
		case 0x1c/4:	COMBINE_DATA(&io2);break;
		
	}
}

static ADDRESS_MAP_START( io_map, ADDRESS_SPACE_IO, 32 )
		AM_RANGE(0x00000000, 0x00ffffff) AM_READ(dgio_r) AM_WRITE(dgio_w)
ADDRESS_MAP_END

INPUT_PORTS_START( dgpix )
INPUT_PORTS_END


static void plot_pixel_rgb(struct mame_bitmap *bitmap, int x, int y , int color)
{
	if (Machine->color_depth == 32)
	{
		UINT32 cb=(color&0x1f)<<3;	
		UINT32 cg=(color&0x3e0)>>2;	
		UINT32 cr=(color&0x7c00)>>7;	
		((UINT32 *)bitmap->line[y])[x] = cb | (cg<<8) | (cr<<16);
	}
	else
	{
		((UINT16 *)bitmap->line[y])[x] = color;	
	}
}



VIDEO_UPDATE( dgpix )
{
	int x,y;	
	for(y=0;y<240;y++)
		for(x=0;x<160;x++)
		{
			plot_pixel_rgb(bitmap,x*2,y,(vram[y*256+x]>>16)&0x7fff);
			plot_pixel_rgb(bitmap,x*2+1,y,vram[y*256+x]&0x7fff);
		}
}

static INTERRUPT_GEN( test_interrupts )
{
	if(cpu_getiloops())
	{
		cpunum_set_input_line(0, 0, PULSE_LINE);	
	}
	else
	{
		cpunum_set_input_line(0, 7, PULSE_LINE);	
	}
}


static MACHINE_DRIVER_START( dgpix )
	MDRV_CPU_ADD(E132XT, 100000000)		 /* ?? */
	MDRV_CPU_PROGRAM_MAP(dgpix_map,0)
	MDRV_CPU_IO_MAP(io_map,0)
	MDRV_CPU_VBLANK_INT(test_interrupts, 2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER| VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)

	MDRV_PALETTE_LENGTH(0x8000)
	
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(dgpix)
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


DRIVER_INIT( dgpix )
{
	cpu_setbank(1, memory_region(REGION_USER1));
}

GAMEX( 199?, xfiles, 0, dgpix, dgpix, dgpix, ROT0, "dgPIX Entertainment Inc.", "X-Files", GAME_NO_SOUND | GAME_NOT_WORKING )
