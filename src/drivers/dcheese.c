/*

Double Cheese (c) 1993 HAR

CPU: 68000
Sound: 6809, BSMt2000 D15505N
RAM: 84256 (x2), 5116
Other: TRW9312HH (x2), LSI L1A6017 (MAX1 EXIT)

Notes: PCB labeled "Exit Entertainment MADMAX version 5". Title screen reports
(c)1993 Midway Manufacturing. ROM labels (c) 1993 HAR

*/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "sound/bsmt2000.h"

/* main cpu */

// read only once
static READ16_HANDLER( blitter_ready_r )
{
	return 5;
}

static READ16_HANDLER( fake_r )
{
	static int ret = 0;
	ret ^= 0x80;
	return ret;
}

static ADDRESS_MAP_START( main_cpu_map, ADDRESS_SPACE_PROGRAM, 16 )
ADDRESS_MAP_FLAGS( AMEF_UNMAP(1) )

	AM_RANGE(0x000000, 0x03ffff) AM_ROM
	AM_RANGE(0x100000, 0x10ffff) AM_RAM

	AM_RANGE(0x200000, 0x200001) AM_READ(fake_r) // PC 0x16AC
	AM_RANGE(0x2a0036, 0x2a0037) AM_READ(blitter_ready_r) // PC 0x1F8C
	AM_RANGE(0x100000, 0x10ffff) AM_RAM
	AM_RANGE(0x200000, 0x200001) AM_RAM
	AM_RANGE(0x220000, 0x220001) AM_RAM
	AM_RANGE(0x240000, 0x240001) AM_RAM
	AM_RANGE(0x260000, 0x26001f) AM_RAM
	AM_RANGE(0x280000, 0x28001f) AM_RAM
	AM_RANGE(0x2a0000, 0x2a0001) AM_RAM
	AM_RANGE(0x2a0010, 0x2a001f) AM_RAM
	AM_RANGE(0x2a0022, 0x2a0023) AM_RAM
	AM_RANGE(0x2a002e, 0x2a002f) AM_RAM
	AM_RANGE(0x2a0032, 0x2a0033) AM_RAM
	AM_RANGE(0x2a0038, 0x2a0039) AM_RAM
	AM_RANGE(0x300000, 0x300001) AM_RAM
ADDRESS_MAP_END

/* sound cpu */

static ADDRESS_MAP_START( sound_cpu_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0100, 0x0100) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1002, 0x1002) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1003, 0x1003) AM_WRITE(MWA8_RAM)
	AM_RANGE(0x1800, 0x1fff) AM_RAM
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END


INPUT_PORTS_START( dcheese )
INPUT_PORTS_END


/* constants */
#define SRCBITMAP_WIDTH		4096

#define DSTBITMAP_WIDTH		256*2
#define DSTBITMAP_HEIGHT	256*2

static UINT8 *srcbitmap;
static UINT8 *dstbitmap;
static UINT32 srcbitmap_height_mask;

VIDEO_START( dcheese )
{
	/* the source bitmap is in ROM */
	srcbitmap = memory_region(REGION_GFX1);

	/* compute the height */
	srcbitmap_height_mask = (memory_region_length(REGION_GFX1) / SRCBITMAP_WIDTH) - 1;

	/* the destination bitmap is not directly accessible to the CPU */
	dstbitmap = auto_malloc(DSTBITMAP_WIDTH * DSTBITMAP_HEIGHT);
	if (!dstbitmap)
		return 1;

	return 0;
}

VIDEO_UPDATE( dcheese )
{
	int width = cliprect->max_x - cliprect->min_x + 1;
	int y;

	static int en = 1, off=0;
	int i;

	if(code_pressed_memory(KEYCODE_W))
	{
		en ^= 1;
	}

	if(code_pressed_memory(KEYCODE_Q))
	{
		off = (off + 1) & 0x3;
	}

	if(code_pressed_memory(KEYCODE_A))
	{
		off = (off - 1) & 0x3;
	}

	if(en)
	{
		en = 0;
		for(i=0;i<DSTBITMAP_WIDTH * DSTBITMAP_HEIGHT;i++)
		{
			dstbitmap[i] = srcbitmap[i + off * 0x40000];
		}
	}

	/* render all the scanlines from the dstbitmap to MAME's bitmap */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		draw_scanline8(bitmap, cliprect->min_x, y, width, &dstbitmap[DSTBITMAP_WIDTH * y + cliprect->min_x], NULL, -1);
}

static struct BSMT2000interface bsmt2000_interface =
{
	12,
	REGION_SOUND1
};

static MACHINE_DRIVER_START( dcheese )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(main_cpu_map,0)
	/* no irq ? */

	MDRV_CPU_ADD(M6809, 1250000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_cpu_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0, 64*8-1, 0, 64*8-1)

	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(dcheese)
	MDRV_VIDEO_UPDATE(dcheese)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(BSMT2000, 12000000/2)
	MDRV_SOUND_CONFIG(bsmt2000_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

ROM_START( dcheese )
	ROM_REGION( 0x40000, REGION_CPU1, 0 ) /* 68k */
	ROM_LOAD16_BYTE( "dchez.104", 0x000000, 0x20000, CRC(5b6233d8) SHA1(7fdb606b5780dd8f45db07d3ee50e14a27f39533) )
	ROM_LOAD16_BYTE( "dchez.103", 0x000001, 0x20000, CRC(599c73ff) SHA1(f33e617ab7e9489c52b2434cfc61a5e1696e9400) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 ) /* M6809 */
	ROM_LOAD( "dchez.102", 0x8000, 0x8000, CRC(5d110061) SHA1(10d852a408a75979b8e8843afc7b39737ca2c6c8) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD( "dchez.123", 0x00000, 0x40000, CRC(2293dd9a) SHA1(3f0550c2a6f59a233c5b1010cecdb19404170dc0) )
	ROM_LOAD( "dchez.125", 0x40000, 0x40000, CRC(ddf28bab) SHA1(0f3bc86d0db7afebf8c6094b8337e5f343a82f29) )
	ROM_LOAD( "dchez.127", 0x80000, 0x40000, CRC(372f9d67) SHA1(74f73f0344bfb890b5e457fcde3d82c9106e7edd) )

	ROM_REGION( 0xc0000, REGION_SOUND1, 0 )
	ROM_LOAD( "dchez.ar0", 0x00000, 0x40000, CRC(6a9e2b12) SHA1(f7cb4d6b4a459682a68f734b2b2e27e3639b9ed5) )
	ROM_LOAD( "dchez.ar1", 0x40000, 0x40000, CRC(5f3a5f41) SHA1(30e0c7b2ab43a3224432204a9388d509a6a06a11) )
	ROM_LOAD( "dchez.ar2", 0x80000, 0x20000, CRC(d79b0d41) SHA1(cc84ddf6635097ba0aad2f1838ad0606c5bb8166) )
	ROM_LOAD( "dchez.ar3", 0xa0000, 0x20000, CRC(2056c1fd) SHA1(4c44930fb87ea6ad71326cc29313f3b817919d08) )

	ROM_REGION( 0x20000, REGION_USER1, 0 ) /* code ? */
	ROM_LOAD( "dchez.144", 0x000000, 0x10000, CRC(52c96252) SHA1(46de465c25e4602aa360336315b3c8e1a9a0b5f3) )
	ROM_LOAD( "dchez.145", 0x010000, 0x10000, CRC(a11b92d0) SHA1(265f93cb3657910aabca21ed8afbb55bdc86a964) )
ROM_END

GAMEX( 1993, dcheese, 0, dcheese, dcheese, 0, ROT0, "HAR", "Double Cheese", GAME_NOT_WORKING )
