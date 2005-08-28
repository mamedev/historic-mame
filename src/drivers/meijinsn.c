/*

Meijinsen (snk/alpha?)
bit weird this
is the set complete?
maybe somebody can finish the driver..

                        p8      p7
   16mhz                p6      p5
                5816    p4      p3
                5816    p2      p1
                 ?
                          68000-8

        4416 4416 4416 4416
             clr             8910
     z80 p9 p10 2016



5816 = Sony CXK5816-10L (Ram for video)
2016 = Toshiba TMM2016AP-10 (SRAM for sound)
4416 = TI TMS4416-15NL (DRAM for MC68000)
clr  = TI TBP18S030N (32*8 Bipolar PROM)
Z80  = Sharp LH0080A Z80A-CPU-D
8910 = GI AY-3-8910A (Sound chip)
?    = Chip with Surface Scratched ....

"0" "1" MC68000 Program ROMs:
p1  p2
p3  p4
p5  p6
p7  p8

P9  = Z80 Program
P10 = AY-3-8910A Sounds

Text inside P9:

ALPHA DENSHI CO.,LTD  JUNE / 24 / 1986  FOR
* SHOUGI * GAME USED  SOUND BOARD CONTROL
SOFT  PSG & VOICE  BY M.C & S.H


zip also contains 4 jpg images (1.jpg is the board)

*/


#include "driver.h"
#include "machine/random.h"

static data16_t *meijinsn_fg_videoram;

READ16_HANDLER( meijinsn_read_random )
{
	return mame_rand();
//  return 0xffff;
}

static ADDRESS_MAP_START( meijinsn_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_READ(MRA16_ROM)

	AM_RANGE(0x100000, 0x107fff) AM_READ(MRA16_RAM)

	AM_RANGE(0x180000, 0x180fff) AM_READ(MRA16_RAM)

	AM_RANGE(0x1c0000, 0x1c0001) AM_READ(meijinsn_read_random)
ADDRESS_MAP_END

static ADDRESS_MAP_START( meijinsn_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x03ffff) AM_WRITE(MWA16_ROM)

	AM_RANGE(0x100000, 0x107fff) AM_WRITE(MWA16_RAM) AM_BASE(&meijinsn_fg_videoram) // a tilemap?

	AM_RANGE(0x180000, 0x180fff) AM_WRITE(MWA16_RAM)

ADDRESS_MAP_END

INPUT_PORTS_START( meijinsn )
	PORT_START	/* DSW */
INPUT_PORTS_END

static gfx_layout meijinsn_layout =
{
	4,8,
	RGN_FRAC(1,1),
	4,
	{ 0,4,8,12 },
	{ 0,1,2,3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};

static gfx_decode gfxdecodeinfo[] =
{
	{ REGION_CPU1, 0, &meijinsn_layout,   0x0, 2  }, /* bg tiles */

	{ -1 } /* end of array */
};

VIDEO_START(meijinsn)
{
	return 0;
}

VIDEO_UPDATE(meijinsn)
{
	int y,x,z;
	z = 0;
	fillbitmap(bitmap, get_black_pen(), cliprect);

	for (y = 0 ; y < 128; y++)
	{
		for (x= 0 ; x < 256; x++)
		{
			int tildata = 	meijinsn_fg_videoram[z]>>4;
			drawgfx(bitmap,Machine->gfx[0],tildata,0,0,0,x*4,y*8,cliprect,TRANSPARENCY_PEN,0);
			z++;

		}

	}
}

static MACHINE_DRIVER_START( meijinsn )
	MDRV_CPU_ADD_TAG("main", M68000, 8000000 )	 //  ?
	MDRV_CPU_PROGRAM_MAP(meijinsn_readmem,meijinsn_writemem)
	MDRV_CPU_VBLANK_INT(irq2_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(128*8, 128*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 2*8, 64*8-1)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(meijinsn)
	MDRV_VIDEO_UPDATE(meijinsn)

MACHINE_DRIVER_END


ROM_START( meijinsn )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68000 Code + gfx? */
	ROM_LOAD16_BYTE( "p1", 0x00000, 0x08000, CRC(8c9697a3) SHA1(19258e20a6aaadd6ba3469079fef85bc6dba548c) )
	ROM_LOAD16_BYTE( "p2", 0x00001, 0x08000, CRC(f7da3535) SHA1(fdbacd075d45abda782966b16b3ea1ad68d60f91) )
	ROM_LOAD16_BYTE( "p3", 0x10000, 0x08000, CRC(0af0b266) SHA1(d68ed31bc932bc5e9c554b2c0df06a751dc8eb96) )
	ROM_LOAD16_BYTE( "p4", 0x10001, 0x08000, CRC(aab159c5) SHA1(0c9cad8f9893f4080b498433068e9324c7f2e13c) )
	ROM_LOAD16_BYTE( "p5", 0x20000, 0x08000, CRC(0ed10a47) SHA1(9e89ec69f1f4e1ffa712f2e0c590d067c8c63026) )
	ROM_LOAD16_BYTE( "p6", 0x20001, 0x08000, CRC(60b58755) SHA1(1786fc1b4c6d1793fb8e9311356fa4119611cfae) )
	ROM_LOAD16_BYTE( "p7", 0x30000, 0x08000, CRC(604c76f1) SHA1(37fdf904f5e4d69dc8cb711cf3dece8f3075254a) )
	ROM_LOAD16_BYTE( "p8", 0x30001, 0x08000, CRC(e3eaef19) SHA1(b290922f252a790443109e5023c3c35b133275cc) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* Z80 Code + data? */
	ROM_LOAD( "p9", 0x00000, 0x04000, CRC(aedfefdf) SHA1(f9d35737a0e942fe7d483f87c52efa92a1bbb3e5) )
	ROM_LOAD( "p10",0x04000, 0x04000, CRC(93b4d764) SHA1(4fedd3fd1f3ef6c5f60ca86219f877df68d3027d) )

	ROM_REGION( 0x20, REGION_PROMS, 0 ) /* Colour PROM? */
	ROM_LOAD( "clr", 0x00, 0x20, CRC(7b95b5a7) SHA1(c15be28bcd6f5ffdde659f2d352ae409f04b2557) )
ROM_END


GAMEX( 1986, meijinsn, 0, meijinsn, meijinsn, 0, ROT0, "SNK / Alpha Denshi", "Meijinsen", GAME_NOT_WORKING|GAME_NO_SOUND )
