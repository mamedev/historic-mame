/********************************************************************
	Hyperstone-based games
	 ***VERY PRELIMINARY***

*********************************************************************/
#include "driver.h"
#include "machine/eeprom.h"
#include "machine/random.h"

static data32_t hyperstone_iram[0x1000/*4000*/];
static data32_t *vamp_tiles;
static data32_t *eo_vram;

PALETTE_INIT( eo )
{
	int i;

	for (i = 0; i < 32768; i++)
	{
		int r,g,b;

		r = (i >>  5) & 0x1f;
		g = (i >> 10) & 0x1f;
		b = (i >>  0) & 0x1f;

		r = (r << 3) | (r >> 2);
		g = (g << 3) | (g >> 2);
		b = (b << 3) | (b >> 2);

		palette_set_color(i,r,g,b);
	}

}


static INTERRUPT_GEN(test_interrupt)
{
	if(cpu_getiloops())
	{
		cpunum_set_input_line(0, 4, PULSE_LINE);	//vamphlaf =int4 +something else
	}
	else
	{
		cpunum_set_input_line(0, 7, PULSE_LINE);	//vamphlaf =int4 +something else
	}
}


static WRITE32_HANDLER( hyperstone_iram_w )
{
// 	COMBINE_DATA(&hyperstone_iram[offset&0x3fff]);
	COMBINE_DATA(&hyperstone_iram[offset&0xfff]);
}

static READ32_HANDLER( hyperstone_iram_r )
{
// 	return hyperstone_iram[offset&0x3fff];
	return hyperstone_iram[offset&0xfff];
}


static READ32_HANDLER( iohack_r )
{
 	return mame_rand();
}
static data32_t iodata;

static WRITE32_HANDLER( io_w )
{
 	COMBINE_DATA(&iodata);
}

static READ32_HANDLER( io_r )
{
 	return iodata;
}


static ADDRESS_MAP_START( iomap, ADDRESS_SPACE_IO, 32 )
	AM_RANGE(0x00000, 0x00ffffff) AM_READ(iohack_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_READ(MRA32_RAM)
	AM_RANGE(0x40000000, 0x400fffff) AM_READ(MRA32_RAM)
	AM_RANGE(0x80000000, 0x800fffff) AM_READ(MRA32_RAM)
//	AM_RANGE(0x90000000, 0x9003ffff) AM_READ(MRA32_RAM)//racoon
	AM_RANGE(0xc0000000, 0xdfffffff) AM_READ(hyperstone_iram_r)
	//AM_RANGE(0xfcc00000, 0xfccfffff) AM_READ(MRA32_RAM)
	//AM_RANGE(0xfd000000, 0xfff7ffff) AM_READ(MRA32_RAM)
	AM_RANGE(0xfff80000, 0xffffffff) AM_READ(MRA32_BANK1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_WRITE(MWA32_RAM)
	AM_RANGE(0x40000000, 0x400fffff) AM_WRITE(MWA32_RAM)
	AM_RANGE(0x80000000, 0x800fffff) AM_WRITE(MWA32_RAM)
	//AM_RANGE(0x90000000, 0x9003ffff) AM_WRITE(MWA32_RAM)//racoon
	AM_RANGE(0xc0000000, 0xdfffffff) AM_WRITE(hyperstone_iram_w)
	//AM_RANGE(0xfcc00000, 0xfccfffff) AM_WRITE(MWA32_RAM)//racoon
	//AM_RANGE(0xfd000000, 0xfff7ffff) AM_WRITE(MWA32_RAM)//racoon
	AM_RANGE(0xfff80000, 0xffffffff) AM_WRITE(MWA32_ROM)
ADDRESS_MAP_END


static ADDRESS_MAP_START( xfiles_readmem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0007ffff) AM_READ(MRA32_RAM)
	AM_RANGE(0x40000000, 0x4007ffff) AM_READ(MRA32_RAM)
	AM_RANGE(0x80000000, 0x800fffff) AM_READ(MRA32_RAM)
	AM_RANGE(0xc0000000, 0xdfffffff) AM_READ(hyperstone_iram_r)
	AM_RANGE(0xe0000000, 0xe1f00003) AM_READ(MRA32_RAM)
	AM_RANGE(0xffc00000, 0xffffffff) AM_READ(MRA32_BANK1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( xfiles_writemem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0007ffff) AM_WRITE(MWA32_RAM)
	AM_RANGE(0x40000000, 0x4007ffff) AM_WRITE(MWA32_RAM)
	AM_RANGE(0x80000000, 0x800fffff) AM_WRITE(MWA32_RAM) AM_BASE( &vamp_tiles )
	AM_RANGE(0xc0000000, 0xdfffffff) AM_WRITE(hyperstone_iram_w)
	AM_RANGE(0xe0000000, 0xe1f00003) AM_WRITE(MWA32_RAM)
	AM_RANGE(0xffc00000, 0xffffffff) AM_WRITE(MWA32_ROM)
ADDRESS_MAP_END



/* vamphalf */

VIDEO_START( vamphalf )
{
	return 0;
}

static struct GfxLayout vamphalf_layout =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0,8,16,24, 32,40,48,56, 64,72,80,88 ,96,104,112,120 },
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128, 8*128,9*128,10*128,11*128,12*128,13*128,14*128,15*128 },
	16*128,
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &vamphalf_layout,   0x0, 1  }, /* bg tiles */
	{ -1 } /* end of array */
};

static WRITE32_HANDLER(vamp_eeprom_w )
{
	/* BAD */
	data^=0xf;
	EEPROM_write_bit(data & 0x01);
	EEPROM_set_cs_line((data & 0x04) ? CLEAR_LINE : ASSERT_LINE );
	EEPROM_set_clock_line((data & 0x02) ? ASSERT_LINE : CLEAR_LINE );
}


static ADDRESS_MAP_START( iomapvamp, ADDRESS_SPACE_IO, 32 )
//	AM_RANGE(0x180,0x183) 	AM_WRITE(vamp_eeprom_w)
	AM_RANGE(0x00000, 0x00ffffff) AM_READ(iohack_r)

//	AM_RANGE(0x00,0x03) AM_READWRITE(io_r,io_w)
//	AM_RANGE(0x04,0x07) AM_READWRITE(io_r,io_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START( vampreadmem, ADDRESS_SPACE_PROGRAM, 32 )
//	AM_RANGE(0x00000000, 0x000bffff) AM_READ(MRA32_RAM)
//	AM_RANGE(0x00000000, 0x001fffff) AM_READ(MRA32_RAM)
	AM_RANGE(0x00000000, 0x00ffffff) AM_READ(MRA32_RAM)
//	AM_RANGE(0x40000000, 0x40007fff) AM_READ(MRA32_RAM)
	AM_RANGE(0x40000000, 0x4003ffff) AM_READ(MRA32_RAM)
	AM_RANGE(0x80000000, 0x8000bfff) AM_READ(MRA32_RAM)
	AM_RANGE(0xc0000000, 0xdfffffff) AM_READ(hyperstone_iram_r)
//	AM_RANGE(0xe0000000, 0xfff7ffff) AM_READ(MRA32_RAM)
	AM_RANGE(0xfff80000, 0xffffffff) AM_READ(MRA32_BANK1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( vampwritemem, ADDRESS_SPACE_PROGRAM, 32 )
//	AM_RANGE(0x00000000, 0x000bffff) AM_WRITE(MWA32_RAM)
//	AM_RANGE(0x00000000, 0x001fffff) AM_WRITE(MWA32_RAM)
	AM_RANGE(0x00000000, 0x00ffffff) AM_WRITE(MWA32_RAM)
//	AM_RANGE(0x40000000, 0x40007fff) AM_WRITE(MWA32_RAM) AM_BASE( &vamp_tiles )
	AM_RANGE(0x40000000, 0x4003ffff) AM_WRITE(MWA32_RAM) AM_BASE( &vamp_tiles )
	AM_RANGE(0x80000000, 0x8000bfff) AM_WRITE(MWA32_RAM)
	AM_RANGE(0xc0000000, 0xdfffffff) AM_WRITE(hyperstone_iram_w)
//	AM_RANGE(0xe0000000, 0xfff7ffff) AM_WRITE(MWA32_RAM)
	AM_RANGE(0xfff80000, 0xffffffff) AM_WRITE(MWA32_ROM)
ADDRESS_MAP_END

VIDEO_UPDATE( vamphalf )
{
	int x,y;
	int cnt;
	const struct GfxElement *gfx = Machine->gfx[0];
	fillbitmap(bitmap,Machine->pens[0],cliprect);

	cnt = 0;
	for (y=0; y<128;y++)
	{
		for (x=0;x<128;x++)
		{
			int dat;
			dat = vamp_tiles[cnt]&0xff;
			drawgfx(bitmap,gfx,dat,0,0,0,x*8,y*8,cliprect,TRANSPARENCY_PEN,0);
			cnt+=2;
		}
	}
}

static MACHINE_DRIVER_START( vamphalf )
	MDRV_CPU_ADD(E132XS,10000000*5)		 /* ?? */
	MDRV_CPU_PROGRAM_MAP(vampreadmem,vampwritemem)
	MDRV_CPU_IO_MAP(iomapvamp,0)
//	MDRV_CPU_VBLANK_INT(test_interrupt, 8)
	MDRV_CPU_VBLANK_INT(test_interrupt, 2)
//	MDRV_CPU_VBLANK_INT(test_interrupt, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 0, 255)

	MDRV_GFXDECODE(gfxdecodeinfo)
//	MDRV_PALETTE_INIT( eo )
	MDRV_PALETTE_LENGTH(256)

	MDRV_NVRAM_HANDLER(93C46)

	MDRV_VIDEO_START(vamphalf)
	MDRV_VIDEO_UPDATE(vamphalf)
MACHINE_DRIVER_END


static ADDRESS_MAP_START( misnreadmem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_READ(MRA32_RAM)
//	AM_RANGE(0x00000000, 0x000cffff) AM_READ(MRA32_RAM)
//	AM_RANGE(0x40000000, 0x403fffff) AM_READ(MRA32_RAM)
	AM_RANGE(0x40000000, 0x40007fff) AM_READ(MRA32_RAM)
	AM_RANGE(0x80000000, 0x80001fff) AM_READ(MRA32_RAM)
	AM_RANGE(0xc0000000, 0xdfffffff) AM_READ(hyperstone_iram_r)
//	AM_RANGE(0xe0000000, 0xfff7ffff) AM_READ(MRA32_RAM)
	AM_RANGE(0xfff80000, 0xffffffff) AM_READ(MRA32_BANK1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( misnwritemem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_WRITE(MWA32_RAM)
//	AM_RANGE(0x00000000, 0x000cffff) AM_WRITE(MWA32_RAM)
//	AM_RANGE(0x40000000, 0x403fffff) AM_WRITE(MWA32_RAM) AM_BASE( &vamp_tiles )
	AM_RANGE(0x40000000, 0x40007fff) AM_WRITE(MWA32_RAM) AM_BASE( &vamp_tiles )
	AM_RANGE(0x80000000, 0x80001fff) AM_WRITE(MWA32_RAM)
	AM_RANGE(0xc0000000, 0xdfffffff) AM_WRITE(hyperstone_iram_w)
//	AM_RANGE(0xe0000000, 0xfff7ffff) AM_WRITE(MWA32_RAM)
	AM_RANGE(0xfff80000, 0xffffffff) AM_WRITE(MWA32_ROM)
ADDRESS_MAP_END

VIDEO_UPDATE( misncrft )
{
}

static MACHINE_DRIVER_START( misncrft )
	MDRV_CPU_ADD(E132XS,10000000*5)		 /* ?? */
	MDRV_CPU_PROGRAM_MAP(misnreadmem,misnwritemem)
	MDRV_CPU_IO_MAP(iomapvamp,0)
//	MDRV_CPU_VBLANK_INT(test_interrupt, 8)
	MDRV_CPU_VBLANK_INT(test_interrupt, 2)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 0, 255)

	MDRV_GFXDECODE(gfxdecodeinfo)
//	MDRV_PALETTE_INIT( eo )
	MDRV_PALETTE_LENGTH(256)

	MDRV_NVRAM_HANDLER(93C46)

	MDRV_VIDEO_START(vamphalf)
//	MDRV_VIDEO_UPDATE(misncrft)
	MDRV_VIDEO_UPDATE(vamphalf)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( xfiles )
	MDRV_CPU_ADD(E132XS,10000000*5)		 /* ?? */
	MDRV_CPU_PROGRAM_MAP(xfiles_readmem,xfiles_writemem)
	MDRV_CPU_IO_MAP(iomap,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 32*8-1)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(vamphalf)
	MDRV_VIDEO_UPDATE(vamphalf)
MACHINE_DRIVER_END

static ADDRESS_MAP_START( eo_readmem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_READ(MRA32_RAM)
	AM_RANGE(0x0c000000, 0x0c00ffff) AM_READ(MRA32_RAM)
	AM_RANGE(0x40000000, 0x400fffff) AM_READ(MRA32_RAM)
	AM_RANGE(0x80000000, 0x800fffff) AM_READ(MRA32_RAM)
	AM_RANGE(0x90000000, 0x9003ffff) AM_READ(MRA32_RAM)//racoon
	AM_RANGE(0xc0000000, 0xdfffffff) AM_READ(hyperstone_iram_r)
	AM_RANGE(0xfc000000, 0xfccfffff) AM_READ(MRA32_RAM)
	AM_RANGE(0xfd000000, 0xfff7ffff) AM_READ(MRA32_BANK2)
	AM_RANGE(0xfff80000, 0xffffffff) AM_READ(MRA32_BANK1)
ADDRESS_MAP_END

static ADDRESS_MAP_START( eo_writemem, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_WRITE(MWA32_RAM)
	AM_RANGE(0x40000000, 0x400fffff) AM_WRITE(MWA32_RAM)
	AM_RANGE(0x80000000, 0x800fffff) AM_WRITE(MWA32_RAM)
	AM_RANGE(0x90000000, 0x9003ffff) AM_WRITE(MWA32_RAM)	AM_BASE( &eo_vram)
	AM_RANGE(0xc0000000, 0xdfffffff) AM_WRITE(hyperstone_iram_w)
	AM_RANGE(0xfc000000, 0xfccfffff) AM_WRITE(MWA32_RAM)//racoon
	//AM_RANGE(0xfd000000, 0xfeffffff) AM_WRITE(MWA32_RAM)//racoon
	AM_RANGE(0xfff80000, 0xffffffff) AM_WRITE(MWA32_ROM)
ADDRESS_MAP_END

VIDEO_UPDATE( eolith )
{
#define XSIZE 320
	int x,y;
	for(y=0;y<200;y++)
		for(x=0;x<XSIZE;x+=2)
		{
			plot_pixel(bitmap,x,y,(eo_vram[y*XSIZE+x]>>16)&0x7fff);
			plot_pixel(bitmap,x+1,y,eo_vram[y*XSIZE+x]&0x7fff);
		}


}

INPUT_PORTS_START( vamphalf )
	PORT_START
	PORT_DIPNAME( 0x0001, 0x0001, "Test 0" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, "Test 1" )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, "Test 2" )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0008, "Test 3" )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, "Test 4" )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, "Test 5" )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "Test 6" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Test 7" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, "Test 8" )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, "Test 9" )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, "Test 10" )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, "Test 11" )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, "Test 12" )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, "Test 13" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Test 14" )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, "Test 15" )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

static MACHINE_DRIVER_START( eolith )
	MDRV_CPU_ADD(E132XS,10000000*5)		 /* ?? */
	MDRV_CPU_PROGRAM_MAP(eo_readmem,eo_writemem)
	MDRV_CPU_IO_MAP(iomap,0)
	MDRV_CPU_VBLANK_INT(test_interrupt, 8)


	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 0, 255)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_PALETTE_LENGTH(32768)
	MDRV_PALETTE_INIT(eo)

	MDRV_VIDEO_START(vamphalf)
	MDRV_VIDEO_UPDATE(eolith)
MACHINE_DRIVER_END

/*

Vamp 1/2 (Semi Vamp)
Danbi, 1999

Official page here...
http://f2.co.kr/eng/product/intro1-17.asp


PCB Layout
----------
             KA12    VROM1.

             BS901   AD-65    ROML01.   ROMU01.
                              ROML00.   ROMU00.
                 62256
                 62256

T2316162A  E1-16T  PROM1.          QL2003-XPL84C

                 62256
                 62256       62256
                             62256
    93C46.IC3                62256
                             62256
    50.000MHz  QL2003-XPL84C
B1 B2 B3                     28.000MHz



Notes
-----
B1 B2 B3:      Push buttons for SERV, RESET, TEST
T2316162A:     Main program RAM
E1-16T:        Hyperstone E1-16T CPU
QL2003-XPL84C: QuickLogic PLCC84 PLD
AD-65:         Compatible to OKI M6295
KA12:          Compatible to Y3012 or Y3014
BS901          Compatible to YM2151
PROM1:         Main program
VROM1:         OKI samples
ROML* / U*:    Graphics, device is MX29F1610ML (surface mounted SOP44 MASK ROM)

*/


ROM_START( vamphalf )
	ROM_REGION32_BE( 0x80000, REGION_USER1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD("prom1", 0x00000000,    0x00080000,   CRC(f05e8e96) SHA1(c860e65c811cbda2dc70300437430fb4239d3e2d))

	ROM_REGION( 0x800000, REGION_GFX1, 0 ) /* 16x16x8 Sprites? */
	ROM_LOAD32_WORD( "roml00",       0x000000, 0x200000, CRC(cc075484) SHA1(6496d94740457cbfdac3d918dce2e52957341616) )
	ROM_LOAD32_WORD( "roml01",       0x400000, 0x200000, CRC(626c9925) SHA1(c90c72372d145165a8d3588def12e15544c6223b) )
	ROM_LOAD32_WORD( "romu00",       0x000002, 0x200000, CRC(711c8e20) SHA1(1ef7f500d6f5790f5ae4a8b58f96ee9343ef8d92) )
	ROM_LOAD32_WORD( "romu01",       0x400002, 0x200000, CRC(d5be3363) SHA1(dbdd0586909064e015f190087f338f37bbf205d2) )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Oki Samples */
	ROM_LOAD( "vrom1",        0x000000, 0x040000, CRC(ee9e371e) SHA1(3ead5333121a77d76e4e40a0e0bf0dbc75f261eb) )
ROM_END

/* eolith hardware */

/*



Name         Size     CRC32       Chip Type
-------------------------------------------
hc0_u39.bin  4194304  0xeefb6add  C32000 dumped as SGS 27C322
hc1_u34.bin  4194304  0x482f3e52  C32000 dumped as SGS 27C322
hc2_u40.bin  4194304  0x914a1544  C32000 dumped as SGS 27C322
hc3_u35.bin  4194304  0x80c59133  C32000 dumped as SGS 27C322
hc4_u41.bin  4194304  0x9a9e2203  C32000 dumped as SGS 27C322
hc5_u36.bin  4194304  0x74b1719d  C32000 dumped as SGS 27C322
hc_u108.bin   524288  0x2bae46cb  27C040
hc_u43.bin    524288  0x635b4478  27C040
hc_u97.bin    524288  0xebf9f77b  27C040
hc_u107.bin    32768  0xafd5263d  AMIC 275308 dumped as 27256
hc_u111.bin    32768  0x79012474  AMIC 275308 dumped as 27256
*/

ROM_START( hidnctch )
	ROM_REGION32_BE( 0x80000, REGION_USER1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD("hc_u43.bin", 0x00000000,    0x080000,  CRC(635b4478) SHA1(31ea4a9725e0c329447c7d221c22494c905f6940) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 ) /* GFX (not tile based) */
	ROM_LOAD16_BYTE("hc0_u39.bin", 0x0000001,    0x0400000, CRC(eefb6add) SHA1(a0f6f2cf86699a666be0647274d8c9381782640d))
	ROM_LOAD16_BYTE("hc1_u34.bin", 0x0000000,    0x0400000, CRC(482f3e52) SHA1(7a527c6af4c80e10cc25219a04ccf7c7ea1b23af))
	ROM_LOAD16_BYTE("hc2_u40.bin", 0x0800001,    0x0400000, CRC(914a1544) SHA1(683cb007ace50d1ba88253da6ad71dc3a395299d))
	ROM_LOAD16_BYTE("hc3_u35.bin", 0x0800000,    0x0400000, CRC(80c59133) SHA1(66ca4c2c014c4a1c87c46a3971732f0a2be95408))
	ROM_LOAD16_BYTE("hc4_u41.bin", 0x1000001,    0x0400000, CRC(9a9e2203) SHA1(a90f5842b63696753e6c16114b1893bbeb91e45c))
	ROM_LOAD16_BYTE("hc5_u36.bin", 0x1000000,    0x0400000, CRC(74b1719d) SHA1(fe2325259117598ad7c23217426ac9c28440e3a0))

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* ? */
	ROM_LOAD("hc_u108.bin", 0x000000,    0x080000, CRC(2bae46cb) SHA1(7c43f1002dfc20b9c1bb1647f7261dfa7ed2b4f9))

	ROM_REGION( 0x080000, REGION_GFX3, 0 ) /* ? */
	ROM_LOAD("hc_u107.bin", 0x000000,    0x08000, CRC(afd5263d) SHA1(71ace1b749d8a6b84d08b97185e7e512d04e4b8d) ) // same in landbrk

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* ? */
	ROM_LOAD("hc_u111.bin", 0x000000,    0x08000, CRC(79012474) SHA1(09a2d5705d7bc52cc2d1644c87c1e31ee44813ef))

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) /* ? */
	ROM_LOAD("hc_u97.bin", 0x000000,    0x080000, CRC(ebf9f77b) SHA1(5d472aeb84fc011e19b9e61d34aeddfe7d6ac216) )
ROM_END

/*

Documentation
-------------------------------------------
lb_pcb.jpg    614606  0xf041e24c

Name         Size     CRC32       Chip Type
-------------------------------------------
lb.107         32768  0xafd5263d  AMIC 275308 dumped as 27256
lb2-000.u39  4194304  0xb37faf7a  C32000 dumped as SGS 27C322
lb2-001.u34  4194304  0x07e620c9  C32000 dumped as SGS 27C322
lb2-002.u40  4194304  0x3bb4bca6  C32000 dumped as SGS 27C322
lb2-003.u35  4194304  0x28ce863a  C32000 dumped as SGS 27C322
lb2-004.u41  4194304  0xcbe84b06  C32000 dumped as SGS 27C322
lb2-005.u36  4194304  0x350c77a3  C32000 dumped as SGS 27C322
lb2-006.u42  4194304  0x22c57cd8  C32000 dumped as SGS 27C322
lb2-007.u37  4194304  0x31f957b3  C32000 dumped as SGS 27C322
lb_1.u43      524288  0xf8bbcf44  27C040
lb_2.108      524288  0xa99182d7  27C040
lb_3.u97      524288  0x5b34dff0  27C040

*/

ROM_START( landbrk )
	ROM_REGION32_BE( 0x80000, REGION_USER1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD("lb_1.u43", 0x00000000,    0x080000,   CRC(f8bbcf44) SHA1(ad85a890ae2f921cd08c1897b4d9a230ccf9e072) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 ) /* GFX (not tile based) */
	ROM_LOAD16_BYTE("lb2-000.u39", 0x0000001,    0x0400000, CRC(b37faf7a) SHA1(30e9af3957ada7c72d85f55add221c2e9b3ea823) )
	ROM_LOAD16_BYTE("lb2-001.u34", 0x0000000,    0x0400000, CRC(07e620c9) SHA1(19f95316208fb4e52cef78f18c5d93460a644566) )
	ROM_LOAD16_BYTE("lb2-002.u40", 0x0800001,    0x0400000, CRC(3bb4bca6) SHA1(115029be4a4e322549a35f3ae5093ec161e9a421) )
	ROM_LOAD16_BYTE("lb2-003.u35", 0x0800000,    0x0400000, CRC(28ce863a) SHA1(1ba7d8be0ed4459dbdf99df18a2ad817904b9f04) )
	ROM_LOAD16_BYTE("lb2-004.u41", 0x1000001,    0x0400000, CRC(cbe84b06) SHA1(52505939fb88cd24f409c795fe5ceed5b41a52c2))
	ROM_LOAD16_BYTE("lb2-005.u36", 0x1000000,    0x0400000, CRC(350c77a3) SHA1(231e65ea7db19019615a8aa4444922bcd5cf9e5c) )
	ROM_LOAD16_BYTE("lb2-006.u42", 0x1800001,    0x0400000, CRC(22c57cd8) SHA1(c9eb745523005876395ff7f0b3e996994b3f1220))
	ROM_LOAD16_BYTE("lb2-007.u37", 0x1800000,    0x0400000, CRC(31f957b3) SHA1(ab1c4c50c2d5361ba8db047feb714423d84e6df4) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* ? */
	ROM_LOAD("lb_2.108", 0x000000,    0x080000,  CRC(a99182d7) SHA1(628c8d09efb3917a4e97d9e02b6b0ca1f339825d) )

	ROM_REGION( 0x080000, REGION_GFX3, 0 ) /* ? */
	ROM_LOAD("lb.107", 0x000000,    0x08000,    CRC(afd5263d) SHA1(71ace1b749d8a6b84d08b97185e7e512d04e4b8d) )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* ? */
	/* 111 isn't populated? */

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) /* ? */
	ROM_LOAD("lb_3.u97", 0x000000,    0x080000,  CRC(5b34dff0) SHA1(1668763e977e272781ddcc74beba97b53477cc9d) )
ROM_END

/*

Racoon World by Eolith

U43, u97, u108   are 27c040 devices

u111, u107   are 27c256 devices

On the ROM sub board:
u1, u2, u5, u10, u11, u14  are all 27c160 devices
--------------------------------------------------------------------------
Stereo sound?
24MHz crystal near the sound section

there is a 4 position DIP switch.

Hyperstone E1-32N    45.00000 MHz  near this chip
QDSP     QS1001A
QDSP     QS1000
EOLITH  EV0514-001  custom??   14.31818MHz  xtl near this chip
12MHz crystal is near the U111

U107 and U97 are mostlikely sound roms but not sure

*/

ROM_START( racoon )
ROM_REGION32_BE( 0x80000, REGION_USER1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD("racoon-u.43", 0x00000000,    0x080000,  CRC(711ee026) SHA1(c55dfaa24cbaa7a613657cfb25e7f0085f1e4cbf) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 ) /* GFX (not tile based) */
	ROM_LOAD16_BYTE("racoon.u1", 0x0000001,    0x0200000, CRC(49775125) SHA1(2b8ee9dd767465999c828d65bb02b8aaad94177c) )
	ROM_LOAD16_BYTE("racoon.u10",0x0000000,    0x0200000, CRC(f702390e) SHA1(47520ba0e6d3f044136a517ebbec7426a66ce33d) )
	ROM_LOAD16_BYTE("racoon.u2", 0x0800001,    0x0200000, CRC(1eb00529) SHA1(d9af75e116f5237a3c6812538b77155b9c08dd5c) )
	ROM_LOAD16_BYTE("racoon.u11",0x0800000,    0x0200000, CRC(3f23f368) SHA1(eb1ea51def2cde5e7e4f334888294b794aa03dfc) )
	ROM_LOAD16_BYTE("racoon.u5", 0x1000001,    0x0200000, CRC(5fbac174) SHA1(1d3e3f40a737d61ff688627891dec183af7fa19a) )
	ROM_LOAD16_BYTE("racoon.u14",0x1000000,    0x0200000, CRC(870fe45e) SHA1(f8d800b92eb1ee9ef4663319fd3cb1f5e52d0e72) )

	ROM_REGION( 0x080000, REGION_GFX2, 0 ) /* ? */
	ROM_LOAD("racoon-u.108", 0x000000,    0x080000,  CRC(fc4f30ee) SHA1(74b9e60cceb03ad572e0e080fbe1de5cffa1b2c3) )

	ROM_REGION( 0x080000, REGION_GFX3, 0 ) /* ? */
	ROM_LOAD("racoon-u.107", 0x000000,    0x08000,    CRC(89450a2f) SHA1(d58efa805f497bec179fdbfb8c5860ac5438b4ec) )

	ROM_REGION( 0x080000, REGION_GFX4, 0 ) /* ? */
	ROM_LOAD("racoon-u.111", 0x000000,    0x08000, CRC(52f419ea) SHA1(79c9f135b0cf8b1928411faed9b447cd98a83287))

	ROM_REGION( 0x080000, REGION_SOUND1, 0 ) /* ? */
	ROM_LOAD("racoon-u.97", 0x000000,    0x080000,  CRC(fef828b1) SHA1(38352b67d18300db40113df9426c2aceec12a29b))
ROM_END

/* ?? dfpix hardware */

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
	ROM_LOAD16_WORD_SWAP("u9.bin", 0x00000000,    0x400000,   CRC(ebdb75c0) SHA1(9aa5736bbf3215c35d62b424c2e5e40223227baf) )

	/* the following probably aren't in the right regions etc. */

	ROM_REGION( 0x400000, REGION_GFX1, 0 )
	ROM_LOAD16_WORD_SWAP("u8.bin", 0x00000000,    0x400000,   CRC(3b2c2bc1) SHA1(1c07fb5bd8a8c9b5fb169e6400fef845f3aee7aa) )

	ROM_REGION( 0x400000, REGION_SOUND1, 0 )
	ROM_LOAD16_WORD_SWAP("u10.bin", 0x00000000,    0x400000,   CRC(f2ef1eb9) SHA1(d033d140fce6716d7d78509aa5387829f0a1404c) )
ROM_END


/* Mission Craft ... different HW again */

/*

Mission Craft
Sun, 2000

PCB Layout
----------

SUN2000
|---------------------------------------------|
|       |------|  SND-ROM1     ROMH00  ROMH01 |
|       |QDSP  |                              |
|       |QS1001|                              |
|DA1311A|------|  SND-ROM2                    |
|       /------\                              |
|       |QDSP  |               ROML00  ROML01 |
|       |QS1000|                              |
|  24MHz\------/                              |
|                                 |---------| |
|                                 | ACTEL   | |
|J               62256            |A40MX04-F| |
|A  *  PRG-ROM2  62256            |PL84     | |
|M   PAL                          |         | |
|M                    62256 62256 |---------| |
|A                    62256 62256             |
|             |-------|           |---------| |
|             |GMS    |           | ACTEL   | |
|  93C46      |30C2116|           |A40MX04-F| |
|             |       | 62256     |PL84     | |
|  HY5118164C |-------| 62256     |         | |
|                                 |---------| |
|SW2                                          |
|SW1                                          |
|   50MHz                              28MHz  |
|---------------------------------------------|
Notes:
      GMS30C2116 - based on Hyperstone technology, clock running at 50.000MHz
      QS1001A    - Wavetable audio chip, 1M ROM, manufactured by AdMOS (Now LG Semi.), SOP32
      QS1000     - Wavetable audio chip manufactured by AdMOS (Now LG Semi.), QFP100
                   provides Creative Waveblaster functionality and General Midi functions
      SW1        - Used to enter test mode
      SW2        - PCB Reset
      *          - Empty socket for additional program ROM

*/

ROM_START( misncrft )
	ROM_REGION32_BE( 0x80000, REGION_USER1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD("prg-rom2.bin", 0x00000,    0x80000,   CRC(059ae8c1) SHA1(2c72fcf560166cb17cd8ad665beae302832d551c) )

	ROM_REGION( 0x800000, REGION_GFX1, 0 )
	ROM_LOAD32_WORD("roml00", 0x000000,    0x200000,   CRC(748c5ae5) SHA1(28005f655920e18c82eccf05c0c449dac16ee36e) )
	ROM_LOAD32_WORD("romh00", 0x000002,    0x200000,   CRC(f34ae697) SHA1(2282e3ef2d100f3eea0167b25b66b35a64ddb0f8) )
	ROM_LOAD32_WORD("roml01", 0x400000,    0x200000,   CRC(e37ece7b) SHA1(744361bb73905bc0184e6938be640d3eda4b758d) )
	ROM_LOAD32_WORD("romh01", 0x400002,    0x200000,   CRC(71fe4bc3) SHA1(08110b02707e835bf428d343d5112b153441e255) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD("snd-rom1.u15", 0x00000,    0x80000,   CRC(fb381da9) SHA1(2b1a5447ed856ab92e44d000f27a04d981e3ac52) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )
	ROM_LOAD("qs1001a.u17", 0x00000,    0x80000,   CRC(d13c6407) SHA1(57b14f97c7d4f9b5d9745d3571a0b7115fbe3176) )

	ROM_REGION( 0x400000, REGION_SOUND3, 0 )
	ROM_LOAD("snd-rom2.us1", 0x00000,    0x20000,   CRC(8821e5b9) SHA1(4b8df97bc61b48aa16ed411614fcd7ed939cac33) )
ROM_END


ROM_START( coolmini )
	ROM_REGION32_BE( 0x80000*2, REGION_USER1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD("cm-rom1.040", 0x00080000,    0x00080000,   CRC(9688fa98) SHA1(d5ebeb1407980072f689c3b3a5161263c7082e9a) )
	ROM_LOAD("cm-rom2.040", 0x00000000,    0x00080000,   CRC(9d588fef) SHA1(7b6b0ba074c7fa0aecda2b55f411557b015522b6) )

	ROM_REGION( 0x800000, REGION_GFX1, 0 ) /* 16x16x8 Sprites? */
	/* missing . these roms are from vamphalf */
//	ROM_LOAD32_WORD( "roml00",       0x000000, 0x200000, CRC(cc075484) SHA1(6496d94740457cbfdac3d918dce2e52957341616) )
//	ROM_LOAD32_WORD( "roml01",       0x400000, 0x200000, CRC(626c9925) SHA1(c90c72372d145165a8d3588def12e15544c6223b) )
//	ROM_LOAD32_WORD( "romu00",       0x000002, 0x200000, CRC(711c8e20) SHA1(1ef7f500d6f5790f5ae4a8b58f96ee9343ef8d92) )
//	ROM_LOAD32_WORD( "romu01",       0x400002, 0x200000, CRC(d5be3363) SHA1(dbdd0586909064e015f190087f338f37bbf205d2) )

	/* 8x flash roms? */
	ROM_LOAD( "coolmini.gfx",        0x000000, 0x800000, NO_DUMP )

	ROM_REGION( 0x040000, REGION_SOUND1, 0 ) /* Oki Samples */
	ROM_LOAD( "cm-vrom1.020",        0x000000, 0x040000, CRC(fcc28081) SHA1(44031df0ee28ca49df12bcb73c83299fac205e21)  )
ROM_END


DRIVER_INIT( vamphalf )
{
	cpu_setbank(1, memory_region(REGION_USER1));
}

DRIVER_INIT( coolmini )
{
	cpu_setbank(1, memory_region(REGION_USER1));

	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0xfff00000, 0xfff7ffff, 0, 0, MRA32_BANK2);

	cpu_setbank(2, memory_region(REGION_USER1)+0x80000);
}

DRIVER_INIT( eolith )
{
	cpu_setbank(1, memory_region(REGION_USER1));
	cpu_setbank(2, memory_region(REGION_GFX1));
}



/*           rom       parent    machine   inp       init */
GAMEX( 19??, vamphalf, 0,        vamphalf, vamphalf, vamphalf, ROT0, "Danbi", "Vamp 1/2", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 19??, hidnctch, 0,        eolith, vamphalf, eolith, ROT0, "Eolith", "Hidden Catch", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 19??, landbrk,  0,        eolith, vamphalf, eolith, ROT0, "Eolith", "Land Breaker", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 19??, racoon,   0,        eolith, vamphalf, eolith, ROT0, "Eolith", "Racoon World", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 19??, xfiles,   0,        xfiles,   vamphalf, vamphalf, ROT0, "dfPIX Entertainment Inc.", "X-Files", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 2000, misncrft, 0,        misncrft, vamphalf, vamphalf, ROT90, "Sun", "Mission Craft", GAME_NO_SOUND | GAME_NOT_WORKING )
GAMEX( 19??, coolmini, 0,        vamphalf, vamphalf, coolmini, ROT0, "Semicom", "Cool Mini", GAME_NO_SOUND | GAME_NOT_WORKING )
