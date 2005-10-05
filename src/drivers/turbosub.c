/*
Turbo Sub (Unreleased prototype)
(C) 1986 Entertainment Sciences

Hardware
--------
* 6809
 - i8251A (USART)
* 6809
* 6809
 - TMS5220?
 - DAC?
* AM29116DC (16-bit bipolar microprocessor)

Useful information
------------------
http://www.ionpool.net/arcade/es/turbo_sub.html  (includes PCB pictures)
http://www.bitsavers.org/pdf/amd/_dataSheets/29116_dataSheet_Mar86.pdf

Main PCB RAM
------------
8xAM29128 (2k ea = 16k)
4xAM29128 (2k ea = 8k)
8xS2016C  (2k ea = 16k)

Video PCB RAM
---------------
20 x AM9122 (256x4 SRAM)
3 x AM93422 (256x2 SRAM)
*/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "vidhrdw/generic.h"

/* Set to 1 to display test results and skip on errors */
#define ROM_PATCHES 1

static UINT8 INTER_CPU_REG;
static int gfx_w=0x1d8;
static int gfx_s=0x80;

VIDEO_START( turbosub )
{
	return 0;
}

VIDEO_UPDATE( turbosub )
{
	int x,y;
	fillbitmap(bitmap,0,cliprect);
	if(code_pressed(KEYCODE_Z))
	{
		gfx_w=(gfx_w-1)&2047;
		printf("W: %x\n",gfx_w);
	}

	if(code_pressed(KEYCODE_X))
	{
		gfx_w=(gfx_w+1)&2047;
		printf("W: %x\n",gfx_w);
	}

	if(code_pressed_memory(KEYCODE_C))
	{
		gfx_w=(gfx_w-1)&511;
		printf("W: %x\n",gfx_w);
	}

	if(code_pressed_memory(KEYCODE_V))
	{
		gfx_w=(gfx_w+1)&2047;
		printf("W: %x\n",gfx_w);
	}

	if(code_pressed(KEYCODE_A))
	{
		gfx_s++;
		printf("S: %x\n",gfx_s);
	}

	if(code_pressed(KEYCODE_S))
	{
		gfx_s--;
		printf("S: %x\n",gfx_s);
	}


	if(code_pressed_memory(KEYCODE_D))
	{
		gfx_s++;
		printf("S: %x\n",gfx_s);
	}

	if(code_pressed_memory(KEYCODE_F))
	{
		gfx_s--;
		printf("S: %x\n",gfx_s);
	}

	for(y=0;y<256;y++)
		for(x=0;x<gfx_w;x++)
		{
			if(x<256)
				plot_pixel(bitmap,x,y,memory_region(REGION_GFX1)[(gfx_s*gfx_w+y*gfx_w+x)&0x7ffff]);
//              plot_pixel(bitmap,x,y,memory_region(REGION_GFX2)[(gfx_s*gfx_w+y*gfx_w+x)&0x3ffff]);

		}
}

/* i8251A UART */
static WRITE8_HANDLER( UART_W )
{
        if (offset==0) printf("%c",data);
}

static READ8_HANDLER( UART_R )
{
        return 0;
}

/* Corresponds to CPU 1 [6000] and something else (AM29116?) */
static READ8_HANDLER( A4800_R )
{
        return INTER_CPU_REG;
}

/* Unknown - CPU1<->AM29116 interface? */
static READ8_HANDLER( B6000_R )
{
        return 0;
}

/* Read by CPU0 at 4800 */
static WRITE8_HANDLER( B6000_W )
{
        INTER_CPU_REG = data;
}

/* Control register */
static WRITE8_HANDLER( A4800_W )
{
        /*
        Bit 0 6809 #0 ROM bank bit 0
        Bit 1 6809 #0 ROM bank bit 1
        Bit 2 ?
        Bit 3 ?
        Bit 4 ?
        Bit 5 ?
        Bit 6 ?
        Bit 7 6809 #1 NMI
        */

	int bankaddress;
	unsigned char *ROM = memory_region(REGION_CPU1);
	bankaddress = 0x10000 + (data & 0x03) * 0x10000;
	memory_set_bankptr(1,&ROM[bankaddress]);

        cpunum_set_input_line(1, INPUT_LINE_NMI, (data&0x80) ? ASSERT_LINE : CLEAR_LINE);
}



/*
   Look out for:
   o  Analog controls
   o  Digital controls
   o  Numeric keypad
*/

INPUT_PORTS_START( turbosub )
PORT_START
INPUT_PORTS_END


/*
TURBOSUB.U85
============

Vectors
=======

Reset  = E002
/NMI   = E3D0   (Not used = RTI)
SWI    = E8D0
IRQ    = E133
FIRQ   = E319
Others = E3D0   (Not used = RTI)


Self-tests (initiated by CPU 0)
===============================

       f561: LDY, #$0001             Change to 0 to print out test results.
       f737: JMP [A,X]               A = test number/2

       Test   Loc.                  Desc.                                                             Status
       ====   ====                  =====                                                             ======
       0-16                         CPU 0 ROM checksums                                               OK
       17                           CPU 0 RAM 0000-07ff                                               OK
       18                           CPU 0 RAM 0800-0fff                                               OK
       19                           CPU 0 RAM 1000-17ff                                               OK
       20                           CPU 0 RAM 1800-1fff                                               OK
       21                           CPU 0 RAM 2000-27ff                                               OK
       22                           CPU 0 RAM 2800-2fff                                               OK
       23                           CPU 0 RAM 3000-37ff                                               OK
       24     0xf90d                ?                                                                 FAIL
                                    [4800]<-90 then [4800]<-B0
                                    Expect negative number from [4800] before time out.

       25     0xf966                CPU 0<->1 communication test                                      OK
       26     0xf98b                CPU 1 ROM e000-ffff checksum                                      OK                                     OK
       27     0xf990                CPU 1 RAM 3000-37ff                                               OK
       28     0xf995                CPU 1 RAM 0000-0fff                                               OK
       29     0xf99a                CPU 1 RAM 1000-17ff                                               OK
       30     0xf99f                CPU 1 RAM 1800-1fff                                               OK
       31     0xf9a4                CPU 1 RAM 2000-27ff                                               OK
       32     0xf9a9                CPU 1 RAM 2800-2fff                                               OK
       33     0xf9ae                CPU 1 RAM 3000-37ff                                               OK
       34     0xf9b3                CPU 1 RAM 3800-3fff                                               OK

       35     0xf9dd                CPU 1: Read [6000]                                                FAIL
       36     0xf9e3                                                                                  FAIL
       37     0xf9e3                                                                                  FAIL
       38     0xf9e3                CPU 1 RAM 4000-4ffe (even bytes)                                  OK
       39     0xf9e3                CPU 1 RAM 4001-4fff (odd bytes)                                   OK

       40     0xfa06                                                                                  FAIL
       41     0xfafd                                                                                  OK
       42     0xfb0e                                                                                  FAIL
       43     0xfb2a                                                                                  FAIL
       44     0xfbc9                                                                                  FAIL
       45     0xfd61                                                                                  FAIL
       46     0xfd70                                                                                  FAIL
       47     0xfd97                                                                                  FAIL
       48     0xfdc9                                                                                  FAIL
       49     0xfdee                                                                                  FAIL
       50     0xfe05                                                                                  FAIL
       51     0xfe3d                                                                                  FAIL
       52     0xfe54                                                                                  FAIL
       53     0xfe6b                                                                                  FAIL
       54     0xfe82                                                                                  FAIL
       55     0xfe99                                                                                  FAIL
*/

static ADDRESS_MAP_START( cpu0_map, ADDRESS_SPACE_PROGRAM, 8 )
        AM_RANGE(0x0000, 0x36ff) AM_RAM
        AM_RANGE(0x3700, 0x3fff) AM_RAM AM_SHARE(1)
        AM_RANGE(0x40ff, 0x40ff) AM_RAM                                    /* W */
        AM_RANGE(0x41ff, 0x41ff) AM_RAM                                    /* W */
        AM_RANGE(0x42ff, 0x42ff) AM_RAM                                    /* W */
        AM_RANGE(0x4C00, 0x4C00) AM_RAM                                    /* R/W - An input device? (doesn't enter auto mode depending on read) */
        AM_RANGE(0x4800, 0x4800) AM_READWRITE(A4800_R, A4800_W)            /* Control Register */
        AM_RANGE(0x5000, 0x5000) AM_RAM                                    /* Write - related to 4C00 */
        AM_RANGE(0x5400, 0x54ff) AM_RAM                                    /* UART buffer? */
        AM_RANGE(0x5c00, 0x5c01) AM_READWRITE(UART_R, UART_W)              /* i8251A USART */
        AM_RANGE(0x6000, 0xdfff) AM_READWRITE(MRA8_BANK1, MWA8_ROM)        /* Bank switched ROMs */
        AM_RANGE(0xe000, 0xffff) AM_ROM
ADDRESS_MAP_END


/*
TURBOSUB.U63
============

Vectors
=======

Reset:  EF2A
/NMI:   EF30
Others: E064
*/


static ADDRESS_MAP_START( cpu1_map, ADDRESS_SPACE_PROGRAM, 8 )
        AM_RANGE(0x0000, 0x36ff) AM_RAM
        AM_RANGE(0x3700, 0x3fff) AM_RAM AM_SHARE(1)
        AM_RANGE(0x4000, 0x4fff) AM_RAM                            /* 4kB  RAM */
        AM_RANGE(0x6000, 0x6000) AM_READWRITE(B6000_R, B6000_W)    /* Read: AM29116? */
        AM_RANGE(0x8000, 0x8000) AM_RAM                            /* Write */
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END


/*
TURBOSUB.U66
============

Vectors
=======

Reset: ecb5               `
FIRQ:  e114
IRQ:   e0d3
/NMI:  ecb5
SWI:   e13d
SWI2:  e13d
SWI3:  e13d
*/

static ADDRESS_MAP_START( cpu2_map, ADDRESS_SPACE_PROGRAM, 8 )
        AM_RANGE(0x0000, 0x07ff) AM_RAM
        AM_RANGE(0x200c, 0x200d) AM_RAM                    /* R/W - probably shared with another CPU */
        AM_RANGE(0x200e, 0x200f) AM_RAM                    /* R/W */
        AM_RANGE(0x2020, 0x2024) AM_RAM                    /* R/W */
	AM_RANGE(0xc000, 0xffff) AM_ROM
ADDRESS_MAP_END


static MACHINE_INIT( turbosub )
{
#if ROM_PATCHES
       UINT8 *rom = (UINT8 *)memory_region(REGION_CPU1);

       rom[0xf564]=0;              /* Display test status */
       rom[0xf60a]=0x20;           /* Skip on error */
#endif

}

static MACHINE_DRIVER_START( turbosub )

	MDRV_CPU_ADD(M6809,4000000)
	MDRV_CPU_PROGRAM_MAP(cpu0_map,0)

	MDRV_CPU_ADD(M6809,4000000)
	MDRV_CPU_PROGRAM_MAP(cpu1_map,0)

	MDRV_CPU_ADD(M6809,4000000)
	MDRV_CPU_PROGRAM_MAP(cpu2_map,0)

	/* Don't forget the AM29116! */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(34*8, 34*8)
	MDRV_VISIBLE_AREA(0*8, 34*8-1, 0, 34*8-1)
	//MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(512)
        MDRV_MACHINE_INIT( turbosub )
        MDRV_INTERLEAVE(100)

	MDRV_VIDEO_START(turbosub)
	MDRV_VIDEO_UPDATE(turbosub)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( turbosub )
	ROM_REGION( 0xc0000, REGION_USER1, 0) /* Non-bankswitched, 6809 #0 code */
	ROM_LOAD( "turbosub.u85",    0x18000, 0x4000, CRC(eabb9509) SHA1(cbfb6c5becb3fe1b4ed729e92a0f4029a5df7d67) )

	ROM_REGION( 0x48000, REGION_CPU1, 0 )
	/* Bankswitched 6809 code */
	ROM_LOAD( "turbosub.u82",    0x10000, 0x2000, CRC(de32eb6f) SHA1(90bf31a5adf261d47b4f52e93b5e97f343b7ebf0) )
        ROM_CONTINUE(                0x20000, 0x2000 )
	ROM_LOAD( "turbosub.u81",    0x12000, 0x2000, CRC(9ae09613) SHA1(9b5ada4a21473b30be98bcc461129b6ed4e0bb11) )
        ROM_CONTINUE(                0x22000, 0x2000 )
	ROM_LOAD( "turbosub.u87",    0x14000, 0x2000, CRC(ad2284f7) SHA1(8e11b8ad0a98dd1fe6ec8f7ea9e6e4f4a45d8a1b) )
        ROM_CONTINUE(                0x24000, 0x2000 )
	ROM_LOAD( "turbosub.u86",    0x16000, 0x2000, CRC(4f51e6fd) SHA1(8f51ac6412aace29279ce7b02cad45ed681c2065) )
        ROM_CONTINUE(                0x26000, 0x2000 )

	ROM_LOAD( "turbosub.u80",    0x30000, 0x2000, CRC(ff2e2870) SHA1(45f91d63ad91585482c9dd05290b204b007e3f44) )
        ROM_CONTINUE(                0x40000, 0x2000 )
        ROM_LOAD( "turbosub.u79",    0x32000, 0x2000, CRC(13680923) SHA1(14e3daa2178853cef1fd96a68305420c11fceb96) )
        ROM_CONTINUE(                0x42000, 0x2000 )
	ROM_LOAD( "turbosub.u84",    0x34000, 0x2000, CRC(7059842d) SHA1(c20a8accd3fc23bc4476e1d08798d7a80915d37c) )
        ROM_CONTINUE(                0x44000, 0x2000 )
	ROM_LOAD( "turbosub.u83",    0x36000, 0x2000, CRC(31b86fc6) SHA1(8e56e8a75f653c3c4da2c9f31f739894beb194db) )
        ROM_CONTINUE(                0x46000, 0x2000 )

        /* e000 - ffff = Upper half of U85 (lower half is blank) */
	ROM_COPY( REGION_USER1, 0x18000+0x2000, 0xe000, 0x2000 )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "turbosub.u63",    0xc000, 0x4000, CRC(35701532) SHA1(77d957682aab10ee902c1e47c468b9ab8fe6a512) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )
	ROM_LOAD( "turbosub.u66",   0xc000, 0x4000, CRC(5091bf3d) SHA1(7ab872cef1562a45f7533c16bbbae8772673465b) )

       	ROM_REGION( 0xc0000, REGION_USER2, 0)       /* Unknown */
	ROM_LOAD( "turbosub.u67",    0x00000, 0x4000, CRC(f8ae82e9) SHA1(fd27b9fe7872c3c680a1f71a4a5d5eeaa12e4a19) )
	ROM_LOAD( "turbosub.u68",    0x04000, 0x4000, CRC(72e3d09b) SHA1(eefdfcd0c4c32e465f18d40f46cb5bc022c22bfd) )
	ROM_LOAD( "turbosub.u69",    0x00000, 0x4000, CRC(ad04193b) SHA1(2f660302e60a7e68e079a8dd13266a77c077f939) )

	ROM_REGION( 0x80000, REGION_GFX1, 0 )
	ROMX_LOAD( "turbosub.u4",    0x00000, 0x4000, CRC(08303604) SHA1(f075b645d89a2d91bd9b621748906a9f9890ee60), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u14",   0x00001, 0x4000, CRC(83b26c8d) SHA1(2dfa3b45c44652d255c402511bb3810fffb0731d), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u24",   0x00002, 0x4000, CRC(6bbb6cb3) SHA1(d513e547a05b34076bb8261abd51301ac5f3f5d4), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u34",   0x00003, 0x4000, CRC(7b844f4a) SHA1(82467eb7e116f9f225711a1698c151945e1de6e4), ROM_SKIP(3) )

	ROMX_LOAD( "turbosub.u9",    0x10000, 0x4000, CRC(9a03eadf) SHA1(25ee1ebe52f030b2fa09d76161e46540c91cbc4c), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u19",   0x10001, 0x4000, CRC(498253b8) SHA1(dd74d4f9f19d8a746415baea604116faedb4fb31), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u29",   0x10002, 0x4000, CRC(809c374f) SHA1(d3849eed8441e4641ffcbca7c83ee3bb16681a0b), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u39",   0x10003, 0x4000, CRC(3e4e0681) SHA1(ac834f6823ffe835d6f149e79c1d31ae2b89e85d), ROM_SKIP(3) )

	ROMX_LOAD( "turbosub.u3",    0x20000, 0x4000, CRC(825ef29c) SHA1(affadd0976f793b8bdbcbc4768b7de27121e7b11), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u13",   0x20001, 0x4000, CRC(350cc17a) SHA1(b98d16be997fc0576d3206f51f29ce3e257492d3), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u23",   0x20002, 0x4000, CRC(b1531916) SHA1(805a23f40aa875f431e835fdaceba87261c14155), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u33",   0x20003, 0x4000, CRC(0d5130cb) SHA1(7e4e4e5ea50c581a60d15964571464029515c720), ROM_SKIP(3) )

	ROMX_LOAD( "turbosub.u8",    0x30000, 0x4000, CRC(01118737) SHA1(3a8e998b80dffe82296170273dcbbe9870c5b695), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u18",   0x30001, 0x4000, CRC(39fd8e57) SHA1(392f8a8cf58fc4813de840775d9c53561488152d), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u28",   0x30002, 0x4000, CRC(0628586d) SHA1(e37508c2812e1c98659aaba9c495e7396842614e), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u38",   0x30003, 0x4000, CRC(7d597a7e) SHA1(2f48faf75406ab3ff0b954040b74e68b7ca6f7a5), ROM_SKIP(3) )

	ROMX_LOAD( "turbosub.u2",    0x40000, 0x4000, CRC(a8b8c032) SHA1(20512a3a1f8b9c0361e6f5a7e9a50605be3ae650), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u12",   0x40001, 0x4000, CRC(a2c4badf) SHA1(267af1be6261833211270af25045e306efffee80), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u22",   0x40002, 0x4000, CRC(97b7cf0e) SHA1(888fb2f384a5cba8a6f7569886eb6dc27e2b024f), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u32",   0x40003, 0x4000, CRC(b286710e) SHA1(5082db13630ba0967006619027c39ee3607b838d), ROM_SKIP(3) )

	ROMX_LOAD( "turbosub.u7",    0x50000, 0x4000, CRC(50eea315) SHA1(567dbb3cb3a75a7507f4cb4748c7dd878e69d6b7), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u17",   0x50001, 0x4000, CRC(8a9e19e6) SHA1(19067e153c0002edfd4a756f92ad75d9a0cbc3dd), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u27",   0x50002, 0x4000, CRC(1c81a8d9) SHA1(3d13d1ccd7ec3dddf2a27600eb64b5be386e868c), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u37",   0x50003, 0x4000, CRC(59f978cb) SHA1(e99d6378de941cad92e9702fcb18aea87acd371f), ROM_SKIP(3) )

	ROMX_LOAD( "turbosub.u1",    0x60000, 0x4000, CRC(88b0a7a9) SHA1(9012c8059cf60131efa6a0432accd87813187206), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u11",   0x60001, 0x4000, CRC(9f0ff723) SHA1(54b52b4ebc32f10aa32c799ac819928290e70455), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u21",   0x60002, 0x4000, CRC(b4122fe2) SHA1(50e8b488a7b7f739336b60a3fd8a5b14f5010b75), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u31",   0x60003, 0x4000, CRC(3fa15c78) SHA1(bf5cb85fc26b5045ad5acc944c917b068ace2c49), ROM_SKIP(3) )

	ROMX_LOAD( "turbosub.u6",    0x70000, 0x4000, CRC(841e00bd) SHA1(f777cc8dd8dd7c8baa2007355a76db782a218efc), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u16",   0x70001, 0x4000, CRC(d3b63d81) SHA1(e86dd64825f6d9e7bebc26413f524a8962f68f2d), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u26",   0x70002, 0x4000, CRC(867cfe32) SHA1(549e4e557d63dfab8e8c463916512a1b422ce425), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u36",   0x70003, 0x4000, CRC(0d8ebc21) SHA1(7ae65edae05869376caa975ff2c778a08e8ad8a2), ROM_SKIP(3) )

	ROM_REGION( 0x40000, REGION_GFX2, 0)  //gfx ?
	ROMX_LOAD( "turbosub.u44",   0x00000, 0x4000, CRC(eaa05860) SHA1(f649891dae9354b7f2e46e6a380b52a569229d64), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u54",   0x00001, 0x4000, CRC(bebf98d8) SHA1(170502bb44fc6d6bf14d8dac4778b37888c14a7b), ROM_SKIP(3) )

	ROMX_LOAD( "turbosub.u49",   0x00002, 0x4000, CRC(b4170ac2) SHA1(bdbfc43c891c8d525dcc46fb9d05602263ab69cd), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u59",   0x00003, 0x4000, CRC(9c1f4397) SHA1(94335f2db2650f8b7e24fc3f92a04b73325ab164), ROM_SKIP(3) )

	ROMX_LOAD( "turbosub.u43",   0x10000, 0x4000, CRC(5d76237c) SHA1(3d50347856039e43290497348447b1c4581f3a33), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u53",   0x10001, 0x4000, CRC(1352d58a) SHA1(76ae86c365dd4c9e1a6c5af91c01d31e7ee35f0f), ROM_SKIP(3) )

	ROMX_LOAD( "turbosub.u48",   0x10002, 0x4000, CRC(cea4e036) SHA1(4afce4f2a09adf9c83ab7188c05cd7236dea16a3), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u58",   0x10003, 0x4000, CRC(5024d83f) SHA1(a293d92a0ae01901b5618b0250d48e3ba631dfcb), ROM_SKIP(3) )

	ROMX_LOAD( "turbosub.u42",   0x20000, 0x4000, CRC(057a1c72) SHA1(5af89b128b7818550572d02e5ff724c415fa8b8b), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u52",   0x20001, 0x4000, CRC(070d07d6) SHA1(4c81310cd646641a380817fedffab66e76529c97), ROM_SKIP(3) )

	ROMX_LOAD( "turbosub.u47",   0x20002, 0x4000, CRC(10def494) SHA1(a3ba691eb2b0d782162ffc6c081761965844a3a9), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u57",   0x20003, 0x4000, CRC(5ddb0458) SHA1(d1169882397f364ca38fbd563250b33d13b1a7c6), ROM_SKIP(3) )

	ROMX_LOAD( "turbosub.u41",   0x30000, 0x4000, CRC(014bb06b) SHA1(97276ba26b60c2907e59b92cc9de5251298579cf), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u51",   0x30001, 0x4000, CRC(43cdcb5c) SHA1(3dd966daa904d3be7be63c584ba033c0e7904d5c), ROM_SKIP(3) )

	ROMX_LOAD( "turbosub.u46",   0x30002, 0x4000, CRC(3b866e2c) SHA1(c0dd4827a18eb9f4b1055d92544beed10f01fd86), ROM_SKIP(3) )
	ROMX_LOAD( "turbosub.u56",   0x30003, 0x4000, CRC(6d116adf) SHA1(f808e28cef41dc86e43d8c12966037213da87c87), ROM_SKIP(3) )
ROM_END

GAME( 1986, turbosub,  0,       turbosub,  turbosub,  0, ROT0, "Entertainment Sciences", "Turbo Sub",GAME_NO_SOUND|GAME_IMPERFECT_GRAPHICS|GAME_NOT_WORKING )
