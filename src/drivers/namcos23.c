/*
	Namco (Super) System 23
	Stub driver

	Hardware: R4650 (MIPS III) main CPU @ 166 MHz
	          H8/3002 MCU for sound/inputs
		  Custom polygon hardware
		  Tilemaps?  Sprites?
*/

/*

PCB Layouts
-----------

SystemSuper23 MAIN(1) PCB 8672960904 8672960104 (8672970104)
|----------------------------------------------------------------------------|
|       J5                    3V_BATT                                        |
|                     LED1-8         *R4543     *MAX734          ADM485JR    |
|  |-------| LED10-11         LC35256                 CXA1779P   3414  3414  |
|  |H8/3002|          *2061ASC-1                                             |
|  |       |       SW4                                *LM358                 |
|  |-------|                    DS8921                *MB88347  *MB87078     |
|J18                                                  *LC78832  *LC78832     |
|               SW3  14.7456MHz                |----|    |----|  CXD1178Q    |
|              |------|  |----|   |---------|  |C435|    |C435|              |
|    N341256   | C416 |  |C422|   |         |  |----|    |----|              |
|              |      |  |----|   |   C444  |                                |
|    N341256   |      |           |         |                   |---------|  |
|              |------|           |         |  PAL(2) CY7C1399  |         |  |
|   |----| *PST575                |---------|                   |  C417   |  |
|   |C352|           CY7C182                                    |         |  |
|   |----| LED9                                       CY7C1399  |         |  |
|                                                               |---------|  |
|     KM416S1020           EPM7064      PAL(3)  KM416V2540                   |
|                                                                            |
|J16                                                                      J17|
|     KM416S1020                            |---------|  |---------| N341256 |
|                                CY7C1399   |         |  |         |         |
|                                           |   C421  |  |   C404  | N341256 |
|       |---------|              CY7C1399   |         |  |         |         |
|       |         |                         |         |  |         | N341256 |
|       |   C413  |                         |---------|  |---------|         |
|       |         |                                                          |
|       |         |                             KM416V2540                   |
|       |---------| SW2    LC321664                                          |
|               *PST575                                                      |
|                                                           *KM416S1020      |
|       |----------|    |---------|                        |-------------|   |
|       |NKK       |    |         |                        |             |   |
|       |NR4650-167|    |   C361  |           CY2291       |    C447     |   |
|J14    |          |    |         |                        |             |J15|
|       |          |    |         |           14.31818MHz  |             |   |
|       |----------|    |---------|  PAL(1)                |-------------|   |
|                                                           *KM416S1020      |
|                                                          71V124   71V124   |
|----------------------------------------------------------------------------|
Notes:
      * - These parts are underneath the PCB.

      Main Parts List:

      CPU
      ---
          NKK NR4650 - R4600-based 64bit RISC CPU (Main CPU, QFP208, clock input source = CY2291)
          H8/3002    - Hitachi H8/3002 HD6413002F17 (Sound CPU, QFP100, running at 14.7456MHz)

      RAM
      ---
          N341256    - NKK 32K x8 SRAM (x5, SOJ28)
          LC35256    - Sanyo 32K x8 SRAM (SOP28)
          KM416S1020 - Samsung 16M SDRAM (x4, TSOP48)
          KM416V2540 - Samsung 256K x16 EDO DRAM (x2, TSOP40/44)
          LC321664   - Sanyo 64K x16 EDO DRAM (SOJ40)
          71V124     - IDT 128K x8 SRAM (x2, SOJ32)
          CY7C1399   - Cypress 32K x8 SRAM (x4, SOJ28)
          CY7C182    - Cypress 8K x9 SRAM (SOJ28)

      Namco Customs
      -------------
                    C352 (QFP100)
                    C361 (QFP120)
                    C404 (QFP208)
                    C413 (QFP208)
                    C416 (QFP176)
                    C417 (QFP208)
                    C421 (QFP208)
                    C422 (QFP64)
                    C435 (x2, QFP144)
                    C444 (QFP136)
                    C447 (QFP256)

      Other ICs
      ---------
               EPM7064  - Altera MAX EPM7064STC100-10 CPLD (TQFP100, labelled 'SS23MA4A')
               DS8921   - National RS422/423 Differential Line Driver and Receiver Pair (SOIC8)
               CXD1178Q - SONY CXD1178Q  8-bit RGB 3-channel D/A converter (QFP48)
               PAL(1)   - PALCE16V8H (PLCC20, stamped 'SS23MA1B')
               PAL(2)   - PALCE22V10H (PLCC28, stamped 'SS23MA2A')
               PAL(3)   - PALCE22V10H (PLCC28, stamped 'SS23MA3A')
               MAX734   - MAX734 +12V 120mA Flash Memory Programming Supply Switching Regulator (SOIC8)
               PST575   - PST575 System Reset IC (SOIC4)
               3414     - NJM3414 70mA Dual Op Amp (x2, SOIC8)
               LM358    - National LM358 Low Power Dual Operational Amplifier (SOIC8)
               MB87078  - Fujitsu MB87078 Electronic Volume Control IC (SOIC24)
               MB88347  - Fujitsu MB88347 8bit 8 channel D/A converter with OP AMP output buffers (SOIC16)
               ADM485   - Low Power EIA RS485 transceiver (SOIC8)
               CXA1779P - SONY CXA1779P TV/Video circuit RGB Pre-Driver (DIP28)
               CY2291   - Cypress CY2291 Three-PLL General Purpose EPROM Programmable Clock Generator (SOIC20)
               2061ASC-1- IC Designs 2061ASC-1 clock? IC (SOIC16, also found on Namco System 11 PCBs)
               R4543    - EPSON Real Time Clock Module (SOIC14)

      Misc
      ----
          J18   - Connector for MSPM(FRA) PCB
          J5    - Connector for EMI PCB
          J14 \
          J15 \
          J16 \
          J17 \ - Connectors for MEM(M) PCB
          SW2   - 2 position DIP Switch
          SW3   - 2 position DIP Switch
          SW4   - 8 position DIP Switch


Program ROM PCB
---------------

MSPM(FRA) PCB 8699017501 (8699017401)
sticker on top says '5GP3 Ver.C'
|--------------------------|
|            J1            |
|                          |
|  IC2               IC3   |
|                          |
|                          |
|  IC1                     |
|--------------------------|
Notes:
      J1 -  Connector to plug into Main PCB
      IC1 \
      IC2 - Main Program  (Fujitsu 29F016 16MBit FlashROM, TSOP48)
      IC3 - Sound Program (ST M29F400T 4MBit FlashROM, TSOP48)


ROM PCB
-------

8660960601 (8660970601) SYSTEM23 MEM(M) PCB
sticker - 8672961100
|----------------------------------------------------------------------------|
| KEYCUS  5GP1MTBH.2M  5GP1CGLL.4M     5GP1CGLL.5M     5GP1CCRL.7M    PAL(3) |
|                                                                            |
|                                                                            |
|J1    5GP1MTAH.2J     5GP1CGLM.4K     5GP1CGLM.5K     5GP1CCRH.7K         J4|
|                                                                            |
|   *                                                 JP5                    |
|                      5GP1CGUM.4J     5GP1CGUM.5J    JP4                    |
|      5GP1MTAL.2H                                    JP3                    |
|                                                     JP2                    |
|                      5GP1CGUU.4F     5GP1CGUU.5F                           |
|                                                      5GP1CCRL.7F           |
|      5GP1MTBL.2F                                                           |
|                            PAL(1)    PAL(2)                                |
|                                                      5GP1CCRH.7E           |
|                                                                            |
|         JP1                                                                |
|                                                                            |
|     5GP1WAVEL.2C  5GP1PT3L.3C  5GP1PT2L.4C  5GP1PT1L.5C  5GP1PT0L.7C       |
|J2                                                                        J3|
|                                                                            |
|     5GP1WAVEH.2A  5GP1PT3H.3A  5GP1PT2H.4A  5GP1PT1H.5A  5GP1PT0H.7A       |
|                                                                            |
|                                                                            |
|----------------------------------------------------------------------------|
Notes:
      J1   \
      J2   \
      J3   \
      J4   \   - Connectors to main PCB
      JP1      - ROM size configuration jumper for WAVE ROMs. Set to 64M, alt. setting 32M
      JP2      - ROM size configuration jumper for CG ROMs. Set to 64M, alt. setting 32M
      JP3      - ROM size configuration jumper for CG ROMs. Set to 64M, alt. setting 32M
      JP4      - ROM size configuration jumper for CG ROMs. Set to 64M, alt. setting 32M
      JP5      - ROM size configuration jumper for CG ROMs. Set to 64M, alt. setting 32M
                 Other ROMs
                           CCRL - size fixed at 32M
                           CCRH - size fixed at 16M
                           PT*  - size fixed at 32M
                           MT*  - size fixed at 64M

      KEYCUS   - Mach211 CPLD (PLCC44, labelled 'KC029')
      PAL(1)   - PALCE20V8H  (PLCC28, stamped 'SS22M2')  \ Both identical
      PAL(2)   - PALCE20V8H  (PLCC28, stamped 'SS22M2')  /
      PAL(3)   - PALCE16V8H  (PLCC20, stamped 'SS22M1')
           *   - Unpopulated position for PAL (PLCC20, labelled 'SS23M1')

      All ROMs are SOP44 MaskROMs
      Note: ROMs at locations 7M, 7K, 5M, 5K, 5J & 5F are not included in the archive since they're copies of
            other ROMs which are included in the archive.
*/


#include "driver.h"
#include "cpu/mips/mips3.h"

static int ss23_vstat = 0;

/* no video yet */
VIDEO_START( ss23 )
{
	return 0;
}

static double
Normalize( UINT32 data )
{
	data &=  0xffffff;
	if( data&0x800000 )
	{
		data |= 0xff000000;
	}
	return (INT32)data;
}

static void
DrawLine( struct mame_bitmap *bitmap, int x0, int y0, int x1, int y1 )
{
	if( x0>=0 && x0<bitmap->width &&
		x1>=0 && x1<bitmap->width &&
		y0>=0 && y0<bitmap->height &&
		y1>=0 && y1<bitmap->height )
	{
		int sx,sy,dy;
		if( x0>x1 )
		{
			int temp = x0;
			x0 = x1;
			x1 = temp;

			temp = y0;
			y0 = y1;
			y1 = temp;
		}

		if( x1>x0 )
		{
			sy = y0<<16;
			dy = ((y1-y0)<<16)/(x1-x0);
			for( sx=x0; sx<x1; sx++ )
			{
				UINT16 *pDest = (UINT16 *)bitmap->line[sy>>16];
				pDest[sx] = 1;
				sy += dy;
			}
		}
	}
} /* DrawLine */

static void
DrawPoly( struct mame_bitmap *bitmap, const UINT32 *pSource, int n, int bNew )
{
	UINT32 flags = *pSource++;
	UINT32 unk = *pSource++;
	UINT32 intensity = *pSource++;
	double x[4],y[4],z[4];
	int i;
	if( bNew )
	{
		printf( "polydata: 0x%08x 0x%08x 0x%08x\n", flags, unk, intensity );
	}
	for( i=0; i<n; i++ )
	{
		x[i] = Normalize(*pSource++);
		y[i] = Normalize(*pSource++);
		z[i] = Normalize(*pSource++);

		if( bNew )
		{
			printf( "\t(%f,%f,%f)\n", x[i], y[i], z[i] );
		}
	}
	for( i=0; i<n; i++ )
	{
		#define KDIST 0x400
		int j = (i+1)%n;
		double z0 = z[i]+KDIST;
		double z1 = z[j]+KDIST;

		if( z0>0 && z1>0 )
		{
			int x0 = bitmap->width*x[i]/z0 + bitmap->width/2;
			int y0 = bitmap->width*y[i]/z0 + bitmap->height/2;
			int x1 = bitmap->width*x[j]/z1 + bitmap->width/2;
			int y1 = bitmap->width*y[j]/z1 + bitmap->height/2;
			if( bNew )
			{
				printf( "[%d,%d]..[%d,%d]\n", x0,y0,x1,y1 );
			}
			DrawLine( bitmap, x0,y0,x1,y1 );
		}
	}
}

VIDEO_UPDATE( ss23 )
{
#if 0
	static int bNew = 1;
	static int code = 0x80;
	const UINT32 *pSource = (UINT32 *)memory_region(REGION_GFX4);

	pSource = pSource + pSource[code];

	fillbitmap( bitmap, 0, 0 );
	for(;;)
	{
		UINT32 opcode = *pSource++;
		int bDone = (opcode&0x10000);

		switch( opcode&0x0f00 )
		{
		case 0x0300:
			DrawPoly( bitmap, pSource, 3, bNew );
			pSource += 3 + 3*3;
			break;

		case 0x0400:
			DrawPoly( bitmap, pSource, 4, bNew );
			pSource += 3 + 4*3;
			break;

		default:
			printf( "unk opcode: 0x%x\n", opcode );
			bDone = 1;
			break;
		}


		if( bDone )
		{
			break;
		}
	}

	bNew = 0;

	if( keyboard_pressed(KEYCODE_SPACE) )
	{
		while( keyboard_pressed(KEYCODE_SPACE) ){}
		code++;
		bNew = 1;
	}
#endif
}

static READ32_HANDLER( keycus_KC029_r ) /* KC029 for GP500 */
{
	logerror("Read keycus @ offset %x, PC=%x\n", offset, activecpu_get_pc());

	switch (offset)
	{
		case 0:
			return 0x7f454c46;
			break;

		case 1:
			return 0x01020000;
			break;

		case 4:
			return 8;
			break;

		case 8:
			return 1;
			break;

//		case 9:	// maybe not a good idea, makes program crash earlier
//			return 0x20000000;
//			break;

		case 10:
			return 0x20;
			break;

		case 11:
			if (activecpu_get_pc() == 0xbfc0215c)
				return 0;
			else
				return 0x00280028;
			break;

		default:
			break;
	}

	return 0;
}

static READ32_HANDLER( ss23_vstat_r )
{
	ss23_vstat ^= 0xffffffff;
	return ss23_vstat;
}

static ADDRESS_MAP_START( ss23_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_RAM AM_SHARE(1)
	AM_RANGE(0x80000000, 0x803fffff) AM_RAM AM_SHARE(1)
	AM_RANGE(0xa0000000, 0xa03fffff) AM_RAM AM_SHARE(1)
	AM_RANGE(0xa6800000, 0xa6803fff) AM_RAM 			/* palette RAM? */
	AM_RANGE(0xa6820008, 0xa682000b) AM_READ( ss23_vstat_r )
	AM_RANGE(0xa6810000, 0xa681ffff) AM_RAM				/* tilemap VRAM? */
	AM_RANGE(0xafc20000, 0xafc203ff) AM_READ( keycus_KC029_r )
	AM_RANGE(0xbfc00000, 0xbfc7ffff) AM_WRITENOP AM_ROM AM_SHARE(2) AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END

INPUT_PORTS_START( ss23 )
INPUT_PORTS_END


DRIVER_INIT(ss23)
{
/*
	data32_t * pSrc = (data32_t *)memory_region(REGION_GFX4);
	int i;
	for( i=0; i<0x200000; i++ )
	{
		if( (i&0xf)==0 ) logerror( "\n%08x:", i );
		logerror( " %08x", *pSrc++ );
	}
*/
}
#if 0
static struct GfxLayout sprite_layout =
{
	32,32,
	RGN_FRAC(1,1),
	8,
	{ 0,1,2,3,4,5,6,7 },
	{
		0*8,1*8,2*8,3*8,4*8,5*8,6*8,7*8,
		8*8,9*8,10*8,11*8,12*8,13*8,14*8,15*8,
		16*8,17*8,18*8,19*8,20*8,21*8,22*8,23*8,
		24*8,25*8,26*8,27*8,28*8,29*8,30*8,31*8 },
	{
		0*32*8,1*32*8,2*32*8,3*32*8,4*32*8,5*32*8,6*32*8,7*32*8,
		8*32*8,9*32*8,10*32*8,11*32*8,12*32*8,13*32*8,14*32*8,15*32*8,
		16*32*8,17*32*8,18*32*8,19*32*8,20*32*8,21*32*8,22*32*8,23*32*8,
		24*32*8,25*32*8,26*32*8,27*32*8,28*32*8,29*32*8,30*32*8,31*32*8 },
	32*32*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &sprite_layout,  0, 0x80 },
	{ -1 },
};
#endif

static struct mips3_config config =
{
	16384,				/* code cache size - guess */
	16384				/* data cache size - guess */
};

MACHINE_DRIVER_START( ss23 )

	/* basic machine hardware */
	MDRV_CPU_ADD(R4600BE, 166000000)  /* actually R4650 */
	MDRV_CPU_CONFIG(config)
	MDRV_CPU_PROGRAM_MAP(ss23_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256,256)
	MDRV_VISIBLE_AREA(0, 255, 0, 255)
//	MDRV_SCREEN_SIZE(768, 512)
//	MDRV_VISIBLE_AREA(0, 767, 0, 511)
	MDRV_PALETTE_LENGTH(0x2000)

//	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(ss23)
	MDRV_VIDEO_UPDATE(ss23)
MACHINE_DRIVER_END

ROM_START( gp500 )
	/* r4650-generic-xrom-generic: NMON 1.0.8-sys23-19990105 P for SYSTEM23 P1 */
	ROM_REGION( 0x080000, REGION_CPU1, 0 )	/* dummy region for R4650 */

	ROM_REGION32_BE( 0x400000, REGION_USER1, 0 ) /* 4 megs for main R4650 code */
        ROM_LOAD16_BYTE( "5gp3verc.2",   0x000000, 0x200000, CRC(e2d43468) SHA1(5e861dd223c7fa177febed9803ac353cba18e19d) )
        ROM_LOAD16_BYTE( "5gp3verc.1",   0x000001, 0x200000, CRC(f6efc94a) SHA1(785eee2bec5080d4e8ef836f28d446328c942b0e) )

	ROM_REGION( 0x80000, REGION_CPU2, 0 )	/* Hitachi H8/3002 MCU code */
        ROM_LOAD16_WORD_SWAP( "5gp3verc.3",   0x000000, 0x080000, CRC(b323abdf) SHA1(8962e39b48a7074a2d492afb5db3f5f3e5ae2389) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )	/* sprite? tilemap? tiles */
		ROM_LOAD16_BYTE( "5gp1mtal.2h",  0x0000000, 0x800000, CRC(1bb00c7b) SHA1(922be45d57330c31853b2dc1642c589952b09188) )
        ROM_LOAD16_BYTE( "5gp1mtah.2j",  0x0000001, 0x800000, CRC(246e4b7a) SHA1(75743294b8f48bffb84f062febfbc02230d49ce9) )

		/* COMMON FUJII YASUI WAKAO KURE INOUE
		 * 0x000000..0x57ffff: all 0xff
		 */
        ROM_LOAD16_BYTE( "5gp1mtbl.2f",  0x1000000, 0x800000, CRC(66640606) SHA1(c69a0219748241c49315d7464f8156f8068e9cf5) )
        ROM_LOAD16_BYTE( "5gp1mtbh.2m",  0x1000001, 0x800000, CRC(352360e8) SHA1(d621dfac3385059c52d215f6623901589a8658a3) )

	ROM_REGION( 0x2000000, REGION_GFX2, 0 )	/* texture tiles */
        ROM_LOAD( "5gp1cguu.4f",  0x0000000, 0x800000, CRC(c411163b) SHA1(ae644d62357b8b806b160774043e41908fba5d05) )
        ROM_LOAD( "5gp1cgum.4j",  0x0800000, 0x800000, CRC(0265b701) SHA1(497a4c33311d3bb315100a78400cf2fa726f1483) )
        ROM_LOAD( "5gp1cgll.4m",  0x1000000, 0x800000, CRC(0cc5bf35) SHA1(b75510a94fa6b6d2ed43566e6e84c7ae62f68194) )
        ROM_LOAD( "5gp1cglm.4k",  0x1800000, 0x800000, CRC(31557d48) SHA1(b85c3db20b101ba6bdd77487af67c3324bea29d5) )

	ROM_REGION( 0x600000, REGION_GFX3, 0 )	/* texture tilemap */
        ROM_LOAD( "5gp1ccrl.7f",  0x000000, 0x400000, CRC(e7c77e1f) SHA1(0231ddbe2afb880099dfe2657c41236c74c730bb) )
        ROM_LOAD( "5gp1ccrh.7e",  0x400000, 0x200000, CRC(b2eba764) SHA1(5e09d1171f0afdeb9ed7337df1dbc924f23d3a0b) )

	ROM_REGION32_LE( 0x2000000, REGION_GFX4, 0 )	/* 3D model data */
        ROM_LOAD32_WORD( "5gp1pt0l.7c",  0x0000000, 0x400000, CRC(a0ece0a1) SHA1(b7aab2d78e1525f865214c7de387ccd585de5d34) )
        ROM_LOAD32_WORD( "5gp1pt0h.7a",  0x0000002, 0x400000, CRC(5746a8cd) SHA1(e70fc596ab9360f474f716c73d76cb9851370c76) )

        ROM_LOAD32_WORD( "5gp1pt1l.5c",  0x0800000, 0x400000, CRC(80b25ad2) SHA1(e9a03fe5bb4ce925f7218ab426ed2a1ca1a26a62) )
        ROM_LOAD32_WORD( "5gp1pt1h.5a",  0x0800002, 0x400000, CRC(b1feb5df) SHA1(45db259215511ac3e472895956f70204d4575482) )

		ROM_LOAD32_WORD( "5gp1pt2l.4c",  0x1000000, 0x400000, CRC(9289dbeb) SHA1(ec546ad3b1c90609591e599c760c70049ba3b581) )
        ROM_LOAD32_WORD( "5gp1pt2h.4a",  0x1000002, 0x400000, CRC(9a693771) SHA1(c988e04cd91c3b7e75b91376fd73be4a7da543e7) )

		ROM_LOAD32_WORD( "5gp1pt3l.3c",  0x1800000, 0x400000, CRC(480b120d) SHA1(6c703550faa412095d9633cf508050614e15fbae) )
        ROM_LOAD32_WORD( "5gp1pt3h.3a",  0x1800002, 0x400000, CRC(26eaa400) SHA1(0157b76fffe81b40eb970e84c98398807ced92c4) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 ) /* C352 PCM samples */
        ROM_LOAD( "5gp1wavel.2c", 0x000000, 0x800000, CRC(aa634cc2) SHA1(e96f5c682039bc6ef22bf90e98f4da78486bd2b1) )
        ROM_LOAD( "5gp1waveh.2a", 0x800000, 0x800000, CRC(1e3523e8) SHA1(cb3d0d389fcbfb728fad29cfc36ef654d28d553a) )
ROM_END

/* Games */
GAMEX( 1999, gp500, 0,    ss23, ss23, ss23, ROT0, "Namco", "GP500", GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION | GAME_NO_SOUND )
