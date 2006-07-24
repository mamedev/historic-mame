/* Limenko Hyperstone Based Hardware

  Games Supported (but NOT WORKING)

  Legend of Heroes
  Super Bubble 2003 (2 sets)

  driver by Pierpaolo Prazzoli
*/

#include "driver.h"
#include "machine/eeprom.h"

static UINT32 *hypr_tiles;
static int a=0;

/*****************************************************************************************************
  VIDEO HARDWARE EMULATION
*****************************************************************************************************/

VIDEO_START( sb2003 )
{
	return 0;
}

VIDEO_UPDATE( sb2003 )
{
/*  if(code_pressed_memory(KEYCODE_Q))
        printf("%d\n",++a);
    if(code_pressed_memory(KEYCODE_W))
        printf("%d\n",--a);
*/
	int x,y;
	int cnt;
	const gfx_element *gfx = Machine->gfx[0];
	fillbitmap(bitmap,Machine->pens[0],cliprect);

	cnt = 0;

	for (y=0; y<128;y++)
	{
		for (x=0;x<128;x++)
		{
			int dat;
			dat = hypr_tiles[cnt]& 0x1fff;
			drawgfx(bitmap,gfx,dat,0,0,0,x*8,y*8,cliprect,TRANSPARENCY_PEN,0);
			cnt++;
		}
	}

	return 0;
}


VIDEO_START( legendoh )
{
	return 0;
}

VIDEO_UPDATE( legendoh )
{
	if(code_pressed_memory(KEYCODE_Q))
		printf("%d\n",++a);
	if(code_pressed_memory(KEYCODE_W))
		printf("%d\n",--a);

	return 0;
}


/*****************************************************************************************************
  MISC FUNCTIONS
*****************************************************************************************************/

/*
sb2003a :
    024000E8 = 00050000  - eeprom ?
    8000e8 R = control word ?
*/



static READ32_HANDLER( r1 )
{
//  return 1 << a;
	return ~0;
}
static READ32_HANDLER( r2 )
{
	return ~0;
}

static READ32_HANDLER( eeprom_r )
{
/*  UINT32 pc = activecpu_get_pc();
    if(pc != 0x268D4 && pc!=0x272B6)
        logerror("0x1000 read @ %X\n",pc);
    return 0x400000;
    return ((EEPROM_read_bit() & 1)<<23) | (mame_rand() & ~0x800000);
    */

	UINT32 pc = activecpu_get_pc();
	if(/*pc != 0x268D4 && */pc!=0x272B6)
		logerror("0x1000 read @ %X\n",pc);

	return (EEPROM_read_bit() << 23);
}

static WRITE32_HANDLER( eeprom_w )
{
	// data & 0x80000 video buffer ?

	EEPROM_write_bit(data & 0x40000);
	EEPROM_set_cs_line((data & 0x10000) ? CLEAR_LINE : ASSERT_LINE );
	EEPROM_set_clock_line((data & 0x20000) ? ASSERT_LINE : CLEAR_LINE );

//  if(data & ~(0x80000+0x10000+0x20000))
//      printf("data = %X\n",data & ~(0x80000+0x10000+0x20000));
}


static NVRAM_HANDLER( limenko_93C46 )
{
	if( read_or_write )
	{
		EEPROM_save( file );
	}
	else
	{
		EEPROM_init( &eeprom_interface_93C46 );

		if( file )
		{
			EEPROM_load( file );
		}
		else
		{
			EEPROM_set_data( memory_region( REGION_USER3 ), memory_region_length( REGION_USER3 ) );
		}
	}
}


static INTERRUPT_GEN( common_interrupts )
{
	cpunum_set_input_line(0, 5, PULSE_LINE);
}




/*****************************************************************************************************
  INPUT PORTS
*****************************************************************************************************/


INPUT_PORTS_START( sb2003 )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP )    PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )  PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )  PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_SERVICE_NO_TOGGLE( 0x0010, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

/*****************************************************************************************************
  GRAPHICS DECODES
*****************************************************************************************************/


static gfx_layout bub_layout =
{
	8,8,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0,8,16,24,32,40, 48,56 },
	{ 64*0, 64*1,64*2,64*3,64*4,64*5,64*6,64*7 },
	64*8,
};


static gfx_layout sprites_layout =
{
	16,16,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{ 0,8,16,24, 32,40,48,56, 64,72,80,88 ,96,104,112,120 },
	{ 0*128, 1*128, 2*128, 3*128, 4*128, 5*128, 6*128, 7*128, 8*128,9*128,10*128,11*128,12*128,13*128,14*128,15*128 },
	16*128,
};

static gfx_decode sb2003_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &bub_layout, 0, 1  }, /* bg tiles */
	{ -1 } /* end of array */
};

static gfx_decode legendoh_gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &sprites_layout,   0x0, 32  }, /* bg tiles */
	{ REGION_GFX2, 0, &sprites_layout,   0x0, 32  }, /* bg tiles */
	{ -1 } /* end of array */
};



/*****************************************************************************************************
  MEMORY MAPS
*****************************************************************************************************/


//value at 8001FFEC?

static ADDRESS_MAP_START( sb2003_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_RAM	//AM_BASE(&wram)
	AM_RANGE(0x80000000, 0x8001ffff) AM_RAM AM_BASE(&hypr_tiles)
	AM_RANGE(0xffe00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1,0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sb2003_io_map, ADDRESS_SPACE_IO, 32 )
//  AM_RANGE(0x0000, 0x0003) AM_READ(r1)
//  AM_RANGE(0x0800, 0x0803) AM_READ(r2)
	AM_RANGE(0x1000, 0x1003) AM_READ(eeprom_r)
	AM_RANGE(0x4800, 0x4803) AM_WRITE(eeprom_w)
ADDRESS_MAP_END



/*
pc = 2728a, 26d1a

memory: 400011be -> eeprom read data
PC = 27340
mem: 135804-5 -> store eeprom read
PC = 2746a

*/

//pc = 1a70! -> 1654 = cl3 = 1
//pc = 231a0 instead of pc = 230f0!
//pc = 230e2 -> cmp L9 == FF !
//00022CFA: CMPI L52, $ff
//00022CFE: BNE $22d02
//00022D00: BR $22d3a <-- !!!

//resource.nr2 = 40001730
//pc=8728
//read 40385e3c
//PC = 284a8, b0de, 244e6, 28c80
//writes 2a38c
//pc: 244e6
//writes 2a3b8
//bdc4 (??)
//be28
//230ee
//232da (231B4 function)
/*
cpu #0 (PC=0002445C): unmapped program memory dword read from 40000000 & FF000000
cpu #0 (PC=000086DE): unmapped program memory dword read from 40000000 & FF000000
cpu #0 (PC=00008716): unmapped program memory dword read from 40000000 & FF000000

cpu #0 (PC=00008738): unmapped program memory dword read from 6503FFF8 & FFFFFFFF
cpu #0 (PC=00008738): unmapped program memory dword read from 6503FFFC & FFFFFFFF
cpu #0 (PC=000087A2): unmapped program memory dword read from 69022DF4 & 0000FF00

*/

static ADDRESS_MAP_START( legendoh_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_RAM
	AM_RANGE(0x40000000, 0x403fffff) AM_ROM AM_REGION(REGION_USER2,0)
	AM_RANGE(0x80000000, 0x8003ffff) AM_RAM AM_BASE(&hypr_tiles)
	AM_RANGE(0xfff80000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1,0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( limenko_io_map, ADDRESS_SPACE_IO, 32 )
	AM_RANGE(0x0000, 0x0003) AM_READ(r1)
	AM_RANGE(0x0800, 0x0803) AM_READ(r2)
	AM_RANGE(0x1000, 0x1003) AM_READ(eeprom_r)
	AM_RANGE(0x4800, 0x4803) AM_WRITE(eeprom_w)
ADDRESS_MAP_END


/*****************************************************************************************************
  MACHINE DRIVERS
*****************************************************************************************************/


static MACHINE_DRIVER_START( sb2003 )
	MDRV_CPU_ADD(E132XT, 80000000)		 /* ?? */ //XN!
	MDRV_CPU_PROGRAM_MAP(sb2003_map,0)
	MDRV_CPU_IO_MAP(sb2003_io_map,0)
//  MDRV_CPU_VBLANK_INT(common_interrupts, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(0, 511, 0, 511)

	MDRV_GFXDECODE(sb2003_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32768)
//  MDRV_PALETTE_INIT(sb2003a)

	MDRV_VIDEO_START(sb2003)
	MDRV_VIDEO_UPDATE(sb2003)

	/* sound hardware */
MACHINE_DRIVER_END



static MACHINE_DRIVER_START( legendoh )
	MDRV_CPU_ADD(E132XT, 80000000)		 /* ?? */ //XN!
	MDRV_CPU_PROGRAM_MAP(legendoh_map,0)
	MDRV_CPU_IO_MAP(limenko_io_map,0)
	MDRV_CPU_VBLANK_INT(common_interrupts, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(/*limenko_*/93C46)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 512)
	MDRV_VISIBLE_AREA(0, 511, 0, 511)

	MDRV_PALETTE_LENGTH(32768)
	MDRV_GFXDECODE(legendoh_gfxdecodeinfo)

	MDRV_VIDEO_START(legendoh)
	MDRV_VIDEO_UPDATE(legendoh)
MACHINE_DRIVER_END


/*****************************************************************************************************
  ROM LOADING
*****************************************************************************************************/

ROM_START( sb2003a )
	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD16_WORD_SWAP("sb2003a_05.u6", 0x00000000, 0x200000, CRC(8aec4554) SHA1(57a12b142eb7bf08dd1e78d3c79222001bbaa636) )

	ROM_REGION( 0x220000, REGION_CPU2, 0 ) /* sound cpu + data */
	ROM_LOAD("08", 0x000000,    0x080000,  CRC(85c52e1e) SHA1(08ab6fa200c3d42b1c16fd5c79fac2fd37f8592e) )
	ROM_LOAD("06.u18", 0x020000,    0x200000,  CRC(b6ad0d32) SHA1(33e73963ea25e131801dc11f25be6ab18bef03ed) )

	ROM_REGION( 0x000080, REGION_USER3, ROMREGION_ERASEFF ) /* eeprom */
	// not dumped

	ROM_REGION( 0x800000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE("01.u1", 0x000000,    0x200000,   CRC(d2c7091a) SHA1(deff050eb0aee89f60d5ad13053e4f1bd4d35961) )
	ROM_LOAD32_BYTE("02.u2", 0x000001,    0x200000,   CRC(a0734195) SHA1(8947f351434e2f750c4bdf936238815baaeb8402) )
	ROM_LOAD32_BYTE("03.u3", 0x000002,    0x200000,   CRC(0f020280) SHA1(2c10baec8dbb201ee5e1c4c9d6b962e2ed02df7d) )
	ROM_LOAD32_BYTE("04.u4", 0x000003,    0x200000,   CRC(fc2222b9) SHA1(c7ee8cffbbee1673a9f107f3f163d029c3900230) )
ROM_END


ROM_START( sb2003 )
	ROM_REGION32_BE( 0x200000, REGION_USER1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD16_WORD_SWAP( "sb2003_05.u6", 0x000000, 0x200000, CRC(265e45a7) SHA1(b9c8b63aa89c08f3d9d404621e301b122f85389a) )

	ROM_REGION( 0x220000, REGION_CPU2, 0 ) /* sound cpu + data */
	ROM_LOAD( "07.u16",       0x000000, 0x020000, CRC(78acc607) SHA1(30a1aed40d45233dce88c6114989c71aa0f99ff7) )
	ROM_LOAD( "06.u18",       0x020000, 0x200000, CRC(b6ad0d32) SHA1(33e73963ea25e131801dc11f25be6ab18bef03ed) )

	ROM_REGION( 0x800000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "01.u1", 0x000000, 0x200000, CRC(d2c7091a) SHA1(deff050eb0aee89f60d5ad13053e4f1bd4d35961) )
	ROM_LOAD32_BYTE( "02.u2", 0x000001, 0x200000, CRC(a0734195) SHA1(8947f351434e2f750c4bdf936238815baaeb8402) )
	ROM_LOAD32_BYTE( "03.u3", 0x000002, 0x200000, CRC(0f020280) SHA1(2c10baec8dbb201ee5e1c4c9d6b962e2ed02df7d) )
	ROM_LOAD32_BYTE( "04.u4", 0x000003, 0x200000, CRC(fc2222b9) SHA1(c7ee8cffbbee1673a9f107f3f163d029c3900230) )

	ROM_REGION( 0x000080, REGION_USER3, ROMREGION_ERASEFF ) /* eeprom */
	// not dumped
ROM_END


/*

Legend Of Heroes
Limenko, 2000

This game runs on a cartridge-based system with Hyperstone E1-32XN CPU and
QDSP QS1000 sound hardware.

PCB Layout
----------

LIMENKO MAIN BOARD SYSTEM
MODEL : LMSYS
REV : LM-003B
SEL : B3-06-00
|-----------------------------------------------------------|
|                                                           |
||-|                 IS61C256    |--------| IS41C16256   |-||
|| |                             |SYS     |              | ||
|| |           |------|          |L2D_HYP |              | ||
|| | QS1003    |QS1000|          |VER1.0  |              | ||
|| |           |      |24kHz     |--------| IC41C16256   | ||
|| |           |------|                                  | ||
|| |                             32MHz     20MHz         | ||
|| |                                                     | ||
|| |              PAL                                    | ||
|| |DA1311                       |--------| IS41C16105   | ||
|| |               IS61C6416     |E1-32XN |              | ||
||-|                             |        |              |-||
|  TL084                         |        |                 |
|                                |--------| IC41C16105      |
|                                                           |
|  TL082         93C46                               PWR_LED|
|                                                    RUN_LED|
|VOL                                                        |
| KIA6280                                           RESET_SW|
|                                                    TEST_SW|
|                                                           |
|---|          JAMMA            |------|    22-WAY      |---|
    |---------------------------|      |----------------|


ROM Board
---------

REV : LMSYS_D
SEL : D2-09-00
|-----------------------------------------------------------|
|   +&*SYS_ROM7              SOU_ROM2      SOU_PRG          |
||-|+&*SYS_ROM8                                          |-||
|| |  +SYS_ROM6              SOU_ROM1                    | ||
|| |  +SYS_ROM5                           +CG_ROM10      | ||
|| | +&SYS_ROM1              CG_ROM12    +*CG_ROM11      | ||
|| |                                                     | ||
|| |                                      +CG_ROM20      | ||
|| |  &SYS_ROM2              CG_ROM22    +*CG_ROM21      | ||
|| |                                                     | ||
|| |                                      +CG_ROM30      | ||
|| |  &SYS_ROM3              CG_ROM32    +*CG_ROM31      | ||
|| |                                                     | ||
||-|                                      +CG_ROM40      |-||
|      SYS_ROM4              CG_ROM42    +*CG_ROM41         |
|-----------------------------------------------------------|
Notes:
      * - These ROMs located on the other side of the PCB
      + - These ROMs surface mounted, type MX29F1610 16MBit SOP44
      & - These locations not populated
*/

ROM_START( legendoh )
	ROM_REGION32_BE( 0x80000, REGION_USER1, 0 ) /* Hyperstone CPU Code */
	ROM_LOAD16_WORD_SWAP( "01.sys_rom4", 0x000000, 0x80000, CRC(49b4a91f) SHA1(21619e8cd0b2fba8c2e08158497575a1760f52c5) )

	ROM_REGION32_BE( 0x400000, REGION_USER2, 0 )
	ROM_LOAD16_WORD_SWAP( "sys_rom6", 0x000000, 0x200000, CRC(5c13d467) SHA1(ed07b7e1b22293e256787ab079d00c2fb070bf4f) )
	ROM_LOAD16_WORD_SWAP( "sys_rom5", 0x200000, 0x200000, CRC(19dc8d23) SHA1(433687c6aa24b9456436eecb1dcb57814af3009d) )

	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "02.cg_rom12",  0x000000, 0x80000, CRC(8b2e8cbc) SHA1(6ed6db843e27d715e473752dd3853a28bb81a368) )
	ROM_LOAD( "03.cg_rom22",  0x080000, 0x80000, CRC(a35960c8) SHA1(86914701930512cae81d1ad892d482264f80f695) )
	ROM_LOAD( "04.cg_rom32",  0x100000, 0x80000, CRC(3f486cab) SHA1(6507d4bb9b4aa7d43f1026e932c82629d4fa44dd) )
	ROM_LOAD( "05.cg_rom42",  0x180000, 0x80000, CRC(5d807bec) SHA1(c72c77ed0478f705018519cf68a54d22524d05fd) )

	ROM_REGION( 0x1000000, REGION_GFX2, ROMREGION_DISPOSE )  /* 16x16x8 Sprites */
	ROM_LOAD( "cg_rom10", 0x000000, 0x200000, CRC(93a48489) SHA1(a14157d31b4e9c8eb7ebe1b2f1b707ec8c8561a0) )
	ROM_LOAD( "cg_rom11", 0x200000, 0x200000, CRC(a9fd5a50) SHA1(d15fc4d1697c1505aa98979af09bcfbbc2521145) )
	ROM_LOAD( "cg_rom20", 0x400000, 0x200000, CRC(1a6c0258) SHA1(ac7c3b8c2fdfb542103032144a30293d44759fd1) )
	ROM_LOAD( "cg_rom21", 0x600000, 0x200000, CRC(b05cdeb2) SHA1(43115146496ee3a820278ffc0b5f0325d6af6335) )
	ROM_LOAD( "cg_rom30", 0x800000, 0x200000, CRC(a0559ef4) SHA1(6622f7107b374c9da816b9814fe93347e7422190) )
	ROM_LOAD( "cg_rom31", 0xa00000, 0x200000, CRC(a9a0d386) SHA1(501af14ea1af70be4862172701af4850750d3f36) )
	ROM_LOAD( "cg_rom40", 0xc00000, 0x200000, CRC(a607b2b5) SHA1(9a6b867d6a777cbc910b98d505367819e0c20077) )
	ROM_LOAD( "cg_rom41", 0xe00000, 0x200000, CRC(1c014f45) SHA1(a76246e90b41cc892575f3a3dc26d8d674e3fc3a) )

	ROM_REGION( 0x200000, REGION_SOUND1, 0 ) /* sounds */
	ROM_LOAD( "sou_prg.06",   0x000000, 0x80000, CRC(bfafe7aa) SHA1(3e65869fe0970bafb59a0225642834042fdedfa6) )
	ROM_LOAD( "sou_rom.07",   0x000000, 0x80000, CRC(4c6eb6d2) SHA1(58bced7bd944e03b0e3dfe1107c01819a33b2b31) )
	ROM_LOAD( "sou_rom.08",   0x000000, 0x80000, CRC(42c32dd5) SHA1(4702771288ba40119de63feb67eed85667235d81) )

	ROM_REGION( 0x000080, REGION_USER3, 0 ) /* eeprom */
	ROM_LOAD( "eeprom.u54",   0x000000, 0x00080, CRC(d4d40394) SHA1(7826bb0dff25b1af4ebea5223d949a97cbb8edb8) )
//  ROM_LOAD16_WORD_SWAP( "eeprom.u54",   0x000000, 0x00080, CRC(d4d40394) SHA1(7826bb0dff25b1af4ebea5223d949a97cbb8edb8) )
ROM_END


DRIVER_INIT( sb2003 )
{

}

GAME( 2003, sb2003,  0,      sb2003,   sb2003, sb2003, ROT0, "Limenko", "Super Bubble 2003 (set 1)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2003, sb2003a, sb2003, sb2003,   sb2003, sb2003, ROT0, "Limenko", "Super Bubble 2003 (set 2)", GAME_NO_SOUND|GAME_NOT_WORKING )
GAME( 2000, legendoh,0,      legendoh, sb2003, 0,	   ROT0, "Limenko", "Legend of Heroes", GAME_NO_SOUND|GAME_NOT_WORKING )
