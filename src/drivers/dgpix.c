/********************************************************************
 Preliminary Hyperstone based dgPix games driver

 Games dumped
 - X-Files

 Known games not dumped
 - Elfin (c) 1999
 
 Notes:
 
 - T. Slanina 2005.02.06
   Preliminary emulation of X-Files.  VRender0- is probably just framebuffer.
   Patch in DRIVER_INIT removes call at RAM adr $8f30 - protection ?
   (without fix, game freezes int one of startup screens - like on real 
    board  with  protection PIC removed)

*********************************************************************/

#include "driver.h"
#include "machine/random.h"
#include "vidhrdw/generic.h"

static data32_t *vram;

static void plot_pixel_rgb(struct mame_bitmap *bitmap, int x, int y, int color)
{
	if(color&0x8000)return; //transparency bit.. or maybe alpha blending
	if (Machine->color_depth == 32)
	{
		UINT32 cb=(color&0x1f)<<3;	
		UINT32 cg=(color&0x3e0)>>2;	
		UINT32 cr=(color&0x7c00)>>7;	
		((UINT32 *)bitmap->line[y])[x] = cb | (cg<<8) | (cr<<16);
	}
	else
	{
		((UINT16 *)bitmap->line[y])[x] = ((color&0x7c00)>>10)|((color&0x1f)<<10)|(color&0x3e0);	
	}
}

static data32_t flash_cmd = 0;

static READ32_HANDLER( flash_r )
{
	data32_t *ROM = (data32_t *)memory_region(REGION_USER2);

	if((offset*4 >= 400000*0 && offset*4 < 0x400000*1) ||
	   (offset*4 >= 400000*7 && offset*4 < 0x400000*8))
	{
		if(flash_cmd == 0x90900000)
		{
			//read maker ID and chip ID
			return 0x00890014;
		}
		else if(flash_cmd == 0x00700000)
		{
			//read status
			return 0x80<<16;
		}
		else if(flash_cmd == 0x70700000)
		{
			//mode = 70700000 @ 13DB8
			return 0x82<<16;
		}
		else if(flash_cmd == 0xE8E80000)
		{
			//mode = E8E80000 @ 14252
			return 0x80<<16;
		}
	}
	return ROM[offset&0x1fffff];
}

static WRITE32_HANDLER( flash_w )
{
	flash_cmd = data;
}

static WRITE32_HANDLER( vram_w )
{
	COMBINE_DATA(&vram[offset]);
	if( (offset&0xff)<160 && (offset>>8)<240)
	{
		plot_pixel_rgb(tmpbitmap,(offset&0xff)*2,offset>>8,(vram[offset]>>16)&0xffff);
		plot_pixel_rgb(tmpbitmap,(offset&0xff)*2+1,offset>>8,vram[offset]&0xffff);
	}
}

static READ32_HANDLER( vram_r )
{
	return vram[offset];
}

static ADDRESS_MAP_START( dgpix_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM
	AM_RANGE(0x3ffff000, 0x3fffffff) AM_RAM
	AM_RANGE(0x40000000, 0x4003ffff) AM_WRITE(vram_w) AM_READ(vram_r) AM_BASE(&vram)
	AM_RANGE(0xe0000000, 0xe1ffffff) AM_READ(flash_r) AM_WRITE(flash_w)
	AM_RANGE(0xffc00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END

static READ32_HANDLER( io_200_r )
{
	return rand()&3;
}

static READ32_HANDLER( io_400_r )
{
	//vblank?
	return (rand()&(~1))| (readinputportbytag("VBL"));
}

static WRITE32_HANDLER( io_400_w )
{

}

static READ32_HANDLER( io_C80_r )
{
	return rand()&0x40;
}

static READ32_HANDLER( io_C84_r )
{
	return mame_rand();
}

static READ32_HANDLER( rand1_r )
{
	return  0xffffffff;
}

static ADDRESS_MAP_START( io_map, ADDRESS_SPACE_IO, 32 )
	AM_RANGE(0x0200, 0x0203) AM_READ(io_200_r)
	AM_RANGE(0x0400, 0x0403) AM_READ(io_400_r) AM_WRITE(io_400_w)
	AM_RANGE(0x0600, 0x0603) AM_READ(rand1_r)
	AM_RANGE(0x0800, 0x0803) AM_READ(rand1_r)
	AM_RANGE(0x0a10, 0x0a13) AM_READ(input_port_0_dword_r)
	AM_RANGE(0x0c80, 0x0c83) AM_READ(io_C80_r)
	AM_RANGE(0x0c84, 0x0c87) AM_READ(io_C84_r)
ADDRESS_MAP_END

INPUT_PORTS_START( dgpix )
PORT_START	
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)

	PORT_BIT( 0x400000, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x800000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_DIPNAME( 0x00010000, 0x00010000, "testmode" )
	PORT_DIPSETTING(      0x00010000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_DIPNAME( 0x0040, 0x0040, "0-06" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "0-07" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	
	PORT_DIPNAME( 0x4000, 0x4000, "0-0e" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "0-0f" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	
	PORT_DIPNAME( 0x00020000, 0x00020000, "0-11" )
	PORT_DIPSETTING(      0x00020000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00040000, 0x00040000, "0-12" )
	PORT_DIPSETTING(      0x00040000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00080000, 0x00080000, "0-13" )
	PORT_DIPSETTING(      0x00080000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00100000, 0x00100000, "0-14" )
	PORT_DIPSETTING(      0x00100000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x00200000, 0x00200000, "0-15" )
	PORT_DIPSETTING(      0x00200000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	
	PORT_DIPNAME( 0x01000000, 0x01000000, "0-18" )
	PORT_DIPSETTING(      0x01000000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x02000000, 0x02000000, "0-19" )
	PORT_DIPSETTING(      0x02000000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x04000000, 0x04000000, "0-1a" )
	PORT_DIPSETTING(      0x04000000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x08000000, 0x08000000, "0-1b" )
	PORT_DIPSETTING(      0x08000000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x10000000, 0x10000000, "0-1c" )
	PORT_DIPSETTING(      0x10000000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x20000000, 0x20000000, "0-1d" )
	PORT_DIPSETTING(      0x20000000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x40000000, 0x40000000, "0-1e" )
	PORT_DIPSETTING(      0x40000000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x80000000, 0x80000000, "0-1f" )
	PORT_DIPSETTING(      0x80000000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	
	PORT_START_TAG("VBL")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_VBLANK ) 

INPUT_PORTS_END

static MACHINE_DRIVER_START( dgpix )
	MDRV_CPU_ADD(E132XT, 14318180*3)		 /* ?? */
	MDRV_CPU_PROGRAM_MAP(dgpix_map,0)
	MDRV_CPU_IO_MAP(io_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(generic_bitmapped)

MACHINE_DRIVER_END


/*

X-Files
dgPIX Entertainment Inc. 1999

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
	ROM_LOAD16_WORD_SWAP( "u9.bin",  0x000000, 0x400000, CRC(ebdb75c0) SHA1(9aa5736bbf3215c35d62b424c2e5e40223227baf) )

	ROM_REGION32_BE( 0x400000*8, REGION_USER2, ROMREGION_ERASEFF )
	ROM_LOAD16_WORD_SWAP( "u8.bin",  0x400000*0, 0x400000, CRC(3b2c2bc1) SHA1(1c07fb5bd8a8c9b5fb169e6400fef845f3aee7aa) )
	ROM_LOAD16_WORD_SWAP( "u9.bin",  0x400000*1, 0x400000, CRC(ebdb75c0) SHA1(9aa5736bbf3215c35d62b424c2e5e40223227baf) )
	

	ROM_REGION32_BE( 0x400000, REGION_SOUND1, 0 ) /* samples ? */
	ROM_LOAD16_WORD_SWAP( "u10.bin", 0x000000, 0x400000, CRC(f2ef1eb9) SHA1(d033d140fce6716d7d78509aa5387829f0a1404c) )
ROM_END

static DRIVER_INIT(xfiles)
{
	memory_region(REGION_USER1)[0x3aa92c]=0;
	memory_region(REGION_USER1)[0x3aa92d]=3;
	memory_region(REGION_USER1)[0x3aa930]=0;
	memory_region(REGION_USER1)[0x3aa931]=3;
	memory_region(REGION_USER1)[0x3aa932]=0;
	memory_region(REGION_USER1)[0x3aa933]=3;
}

GAMEX( 1999, xfiles, 0, dgpix, dgpix, xfiles, ROT0, "dgPIX Entertainment Inc.", "X-Files", GAME_NO_SOUND | GAME_NOT_WORKING )
