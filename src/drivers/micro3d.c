/* Microprose 3D Hardware */

/*

preliminary driver, doesn't really do much more than load the roms

*/

/*

I think all three games are the similar HW, readmes pasted anyway
super tank attack has no sound roms, maybe audio hw was changed

*/


/* BOTSS Readme


------------------------------
B.O.T.S.S by MicroProse (1992)
(Battle of the Solar System)
------------------------------
malcor




Location    Type      File ID     Part Number     Checksum
----------------------------------------------------------
HST U67    27C010     300.HST    110-00013-300      638A   [ Host CPU prog ]
HST U91    27C010     301.HST    110-00013-301      05BD   [ Host CPU prog ]
HST U68    27C010     302.HST    110-00013-302      70BB   [ Host CPU prog ]
HST U92    27C010     303.HST    110-00013-303      411F   [ Host CPU prog ]
HST U69    27C010     104.HST    110-00013-104      C800   [  object data  ]
HST U93    27C010     105.HST    110-00013-105      1900   [  object data  ]
HST U70    27C010     106.HST    110-00013-106      3100   [  object data  ]
HST U94    27C010     107.HST    110-00013-107      3C00   [  object data  ]
HST U71    27C010     108.HST    110-00013-108      309B   [  object data  ]
HST U95    27C010     109.HST    110-00013-109      EA60   [  object data  ]

DTH U153   27C010     014.DTH    110-00013-014      F400
DTH U154   27C010     015.DTH    110-00013-015      3300
DTH U167   27C010     016.DTH    110-00013-016      5900
DTH U160   27C010     017.DTH    110-00013-017      9F00
DTH U135   27C256     118.DTH    110-00013-018      6300
DTH U115   27C256     119.DTH    110-00013-119      BC00
DTH U108   27C256     120.DTH    110-00013-120      FF00
DTH U127   27C256     121.DTH    110-00013-121      7900
DTH U134   27C256     122.DTH    110-00013-122      5000
DTH U114   27C256     123.DTH    110-00013-123      7A00
DTH U107   27C256     124.DTH    110-00013-124      AE00
DTH U126   27C256     125.DTH    110-00013-125      5000

VGB U101   27C010     101.VGB    110-00023-101      90B5
VGB U108   27C010     102.VGB    110-00023-102      E85D
VGB U114   27C010     103.VGB    110-00023-103      47B5
VGB U97    27C010     104.VGB    110-00023-104      282D
VGB U124   27C010     105.VGB    110-00023-105      2986
VGB U121   27C010     106.VGB    110-00023-106      D98A
VGB U130   27C010     107.VGB    110-00023-107      6478
VGB U133   27C010     108.VGB    110-00023-108      8F03

SND U2     27C256    14-001.SND  110-00014-001      4C00   [  Sound prog  ]
SND U17    27C020    13-001.SND  110-00013-001      DF33   [  ADPCM data  ]


Note:  HST - Host PCB            (MPG DW-00011C-0011-02)
       DTH - Dr Math PCB         (MPG 010-00003-003)
       VGB - Video Graphics PCB  (MPG DW-010-00002-002)
       SND - Sound PCB           (MPG 010-00018-002)


       Host ROMs 300, 301, 302, and 303 are the Rev. 1.1
       Upgrade ROMs. They correct the following problems
       from the previous revisions:

FREEZE: Will prevent 3D hang errors from interupting the game.

SPURIOUS INTERUPT: A spurious interupt (error #96) which in most
cases is caused by an external problem; mechanical coin counter,
static, or loose wiring. The error will not interupt the operation
of the game.

GAME OPTIONS: Game options have been altered to improve earnings.

Volume Setting: Changed to enable full volume control without the
need to enter self-test.


ROM Label Revision Level Identification
---------------------------------------

Example: 110-00002-005
                   |
                   Revision Level




Brief hardware overview:
------------------------

Host PCB:

Main processor - 68000P12
               - MC68681P (UART)
               - MC68901P (peripheral)
               - Numerous PLDs (9 PAL/GALs)

The Host PCB is where the program CPU is located and where the game
I/O is connected to. The host PCB decompresses the game objects into
RAM on power-up after a self test. The compressed object information
is located in the ROMs at locations U69, U70, U71, U93, U94, and U95.
There is an RS232 port for debugging via an external computer, as well
as an RS232 port for communication to the Audio PCB.



Dr Math PCB:

Processor    - AM29000-16    (32bit RISC microprocessor, 4 stage pipeline)
Co-processor - AM29C323GC    (32x32 bit floating point multiplier, parallel)
         4 x - SDT7134       (44 pin PLCC, custom CPLDs ?)
MAC1 U173A   - SAM448-20     (UV erasable microprogram sequencer)
MAC2 U166A   - SAM448-20     (UV erasable microprogram sequencer)
             - Z85C3010VSC   (communications controller, Zilog)
             - DS1228        (RS232 port for debug)
             - Numerous PLDs (13 PAL/GALs)


Video Generator PCB:

processor    - TMS34010      (40MHz graphics system processor)
             - 9027EV        (VLSI MPG, VGT8003-2148)
             - 9027EV        (custom, VLSI MPG, VGT8007-2105)
             - BT101KC30     (8-bit digital to analog converter)
         6 x - TC110G38AF    (custom, 100MW002-320)
         3 x - SDT7134       (44 pin PLCC, custom CPLDs ?)
         4 x - AM2701-50JC   (?)
             - SCN2651CC1N28 (communications interface, for debug)
             - DS1228        (RS232 port for debug)
             - Number PLDs   (13 PAL/GALs)



Audio PCB:

processor    - SC80C31BCCN40 (microcontroller, 11.0592MHz, 8051 family)
             - upD7759C      (speech synthesizer, ADPCM)
         2 x - SSM2047       (sound generator circuit)
             - DS1267        (digital potentiometer)
             - SSM2300       (sample and hold amplifier)
             - LC7528CN
             - YM2151
             - YM3012
             - ICL232CPE     (RS232 port for communication to Host PCB)
             - Audio out is stereo

*/

/* F15SE readme


-------------------------------------
F-15 Strike Eagle by MicroProse (1990)
-------------------------------------
malcor




Location    Type      File ID     Part Number     Checksum
----------------------------------------------------------
HST U67    27C010     500.HST    110-00001-500      196A   [ Host CPU prog ]
HST U91    27C010     501.HST    110-00001-501      735A   [ Host CPU prog ]
HST U68    27C010     502.HST    110-00001-502      8937   [ Host CPU prog ]
HST U92    27C010     503.HST    110-00001-503      ABF6   [ Host CPU prog ]
HST U69    27C010     004.HST    110-00001-004      D400   [  object data  ]
HST U93    27C010     005.HST    110-00001-005      6E00   [  object data  ]
HST U70    27C010     006.HST    110-00001-006      8A06   [  object data  ]
HST U94    27C010     007.HST    110-00001-007      4F00   [  object data  ]
HST U71    27C010     008.HST    110-00001-008      3900   [  object data  ]
HST U95    27C010     009.HST    110-00001-009      B900   [  object data  ]

DTH U153   27C010     014.DTH    110-00001-014      C800
DTH U154   27C010     015.DTH    110-00001-015      B500
DTH U167   27C010     016.DTH    110-00001-016      DE00
DTH U160   27C010     017.DTH    110-00001-017      9800
DTH U135   27C256     118.DTH    110-00001-118      2900
DTH U115   27C256     119.DTH    110-00001-119      5B00
DTH U108   27C256     120.DTH    110-00001-120      8000
DTH U127   27C256     121.DTH    110-00001-121      3B00
DTH U134   27C256     122.DTH    110-00001-122      3700
DTH U114   27C256     123.DTH    110-00001-123      2500
DTH U107   27C256     124.DTH    110-00001-124      EC00
DTH U126   27C256     125.DTH    110-00001-125      2200

VGB U101   27C010     001.VGB    110-00002-001      0696
VGB U108   27C010     002.VGB    110-00002-002      C34B
VGB U114   27C010     003.VGB    110-00002-003      F9D5
VGB U97    27C010     004.VGB    110-00002-004      1B12
VGB U124   27C010     005.VGB    110-00002-005      C675
VGB U121   27C010     006.VGB    110-00002-006      302F
VGB U130   27C010     007.VGB    110-00002-007      B9E6
VGB U133   27C010     008.VGB    110-00002-008      CE6D

SND U2     27C256    4-001.SND   110-00004-001      BE00   [  Sound prog  ]
SND U17    27C020    3-001.SND   110-00003-001      4A11   [  ADPCM data  ]


Note:  HST - Host PCB            (MPG DW-00011C-0011-01)
       DTH - Dr Math PCB         (MPG 010-00002-001)
       VGB - Video Graphics PCB  (MPG DW-010-00003-001)
       SND - Sound PCB           (MPG 010-00018-002)



ROM Label Revision Level Identification
---------------------------------------

Example: 110-00002-005
                   |
                   Revision Level



Brief hardware overview:
------------------------

Host PCB:

Main processor - 68000P12
               - MC68681P (UART)
               - MC68901P (peripheral)
               - Numerous PLDs (9 PAL/GALs)

The Host PCB is where the program CPU is located and where the game
I/O is connected to. The host PCB decompresses the game objects into
RAM on power-up after a self test. The compressed object information
is located in the ROMs at locations U69, U70, U71, U93, U94, and U95.
There is an RS232 port for debugging via an external computer, as well
as an RS232 port for communication to the Audio PCB.



Dr Math PCB:

Processor    - AM29000-16    (32bit RISC microprocessor, 4 stage pipeline)
Co-processor - AM29C323GC    (32x32 bit floating point multiplier, parallel)
         4 x - SDT7134       (44 pin PLCC, custom CPLDs ?)
MAC1 U173A   - SAM448-20     (UV erasable microprogram sequencer)
MAC2 U166A   - SAM448-20     (UV erasable microprogram sequencer)
             - Z85C3010VSC   (communications controller, Zilog)
             - DS1228        (RS232 port for debug)
             - Numerous PLDs (13 PAL/GALs)


Video Generator PCB:

processor    - TMS34010      (40MHz graphics system processor)
             - 9027EV        (VLSI MPG, VGT8003-2148)
             - 9027EV        (custom, VLSI MPG, VGT8007-2105)
             - BT101KC30     (8-bit digital to analog converter)
         6 x - TC110G38AF    (custom, 100MW002-320)
         3 x - SDT7134       (44 pin PLCC, custom CPLDs ?)
         4 x - AM2701-50JC   (?)
             - SCN2651CC1N28 (communications interface, for debug)
             - DS1228        (RS232 port for debug)
             - Number PLDs   (13 PAL/GALs)



Audio PCB:

processor    - SC80C31BCCN40 (microcontroller, 11.0592MHz, 8051 family)
             - upD7759C      (speech synthesizer, ADPCM)
         2 x - SSM2047       (sound generator circuit)
             - DS1267        (digital potentiometer)
             - SSM2300       (sample and hold amplifier)
             - LC7528CN
             - YM2151
             - YM3012
             - ICL232CPE     (RS232 port for communication to Host PCB)
             - Audio out is stereo


*/

#include "driver.h"

VIDEO_START(micro3d)
{
	return 0;
}

VIDEO_UPDATE(micro3d)
{

}

READ16_HANDLER( micro3d_unk1_read )
{
	return 0xffff;
}

static ADDRESS_MAP_START( micro3d_readmem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x13ffff) AM_READ(MRA16_ROM)
	AM_RANGE(0x200000, 0x20ffff) AM_READ(MRA16_RAM)
	AM_RANGE(0xa00002, 0xa00003) AM_READ(micro3d_unk1_read)
ADDRESS_MAP_END

static ADDRESS_MAP_START( micro3d_writemem, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x13ffff) AM_WRITE(MWA16_ROM)
	AM_RANGE(0x200000, 0x20ffff) AM_WRITE(MWA16_RAM)
ADDRESS_MAP_END

INPUT_PORTS_START( micro3d )
INPUT_PORTS_END


static MACHINE_DRIVER_START( micro3d )
	MDRV_CPU_ADD(M68000, 12000000 )
	MDRV_CPU_PROGRAM_MAP(micro3d_readmem,micro3d_writemem)
//	MDRV_CPU_VBLANK_INT(irq1_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

//	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(8*8, 48*8-1, 2*8, 30*8-1)
	MDRV_PALETTE_LENGTH(0x200)

	MDRV_VIDEO_START(micro3d)
	MDRV_VIDEO_UPDATE(micro3d)
MACHINE_DRIVER_END

ROM_START( botss )
	/* HOST PCB (MPG DW-00011C-0011-02) */
	ROM_REGION( 0x140000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "300hst.67", 0x000001, 0x20000, CRC(7f74362a) SHA1(41611ba8e6eb5d6b3dfe88e1cede7d9fb5472e40) ) // rev 1.1
	ROM_LOAD16_BYTE( "301hst.91", 0x000000, 0x20000, CRC(a8100d1e) SHA1(69d3cac6f67563c0796560f7b874d7660720027d) ) // rev 1.1
	ROM_LOAD16_BYTE( "302hst.68", 0x040001, 0x20000, CRC(af865ee4) SHA1(f00bce49401431bc749208399329d9f92457186b) ) // rev 1.1
	ROM_LOAD16_BYTE( "303hst.92", 0x040000, 0x20000, CRC(15182619) SHA1(e95dcce11c0651c8e85fc0c658029f48eea35fb8) ) // rev 1.1
	ROM_LOAD16_BYTE( "104hst.69", 0x080001, 0x20000, CRC(72a607ca) SHA1(1afc85380be12c429808c48f1502736a4c8b98e5) )
	ROM_LOAD16_BYTE( "105hst.93", 0x080000, 0x20000, CRC(f37680ae) SHA1(51f1ee805b7d1b2b078c612c572e12846de623b9) )
	ROM_LOAD16_BYTE( "106hst.70", 0x0c0001, 0x20000, CRC(57a1c728) SHA1(2bdc831be739ada0f4f4adec7974da453878db0e) )
	ROM_LOAD16_BYTE( "107hst.94", 0x0c0000, 0x20000, CRC(4c9e16af) SHA1(1f8acc9bb85fe1bf459b4358b9bf9cf9847e6a36) )
	ROM_LOAD16_BYTE( "108hst.71", 0x100001, 0x20000, CRC(cfc0333e) SHA1(9f290769129a61189870faef45c3f061eb7b5c07) )
	ROM_LOAD16_BYTE( "109hst.95", 0x100000, 0x20000, CRC(6c595d1e) SHA1(89fdc30166ba1e9706798547195bdf6875a02e96) )

	/* DR MATH PCB (MPG 010-00003-003) */
	ROM_REGION( 0x40000, REGION_USER1, 0 )
	ROM_LOAD( "014dth.153", 0x000000, 0x20000, CRC(0eee0557) SHA1(8abe52cad31e59cf814fd9f64f4e42ddb4aa8c93) )
	ROM_LOAD( "015dth.154", 0x020000, 0x20000, CRC(68564122) SHA1(410d2db74e574774b2eadd7fdf891feef5d8a93f) )
	ROM_LOAD( "016dth.167", 0x000000, 0x20000, CRC(60c6cb26) SHA1(0e2bf65793715e12d8fd7f87fd3336a9d00ee7e6) )
	ROM_LOAD( "017dth.160", 0x020000, 0x20000, CRC(d8b89379) SHA1(aa08e111c1505a4ad55b14659f8e21fd39cfcb16) )

	ROM_LOAD( "118dth.135", 0x000000, 0x08000, CRC(2903e682) SHA1(027ed6524e9d4490632f10aeb22150c2fbc4eec2) )
	ROM_LOAD( "119dth.115", 0x020000, 0x08000, CRC(9c9dbac1) SHA1(4c66971884190598e128684ece2e15a1c80b94ed) )
	ROM_LOAD( "120dth.108", 0x000000, 0x08000, CRC(dafa173a) SHA1(a19980b92a5e74ebe395be36313701fdb527a46a) )
	ROM_LOAD( "121dth.127", 0x020000, 0x08000, CRC(198a636b) SHA1(356b8948aafb98cb5e6ee7b5ad6ea9e5998265e5) )
	ROM_LOAD( "122dth.134", 0x000000, 0x08000, CRC(bf60c487) SHA1(5ce80e89d9a24b627b0e97bf36a4e71c2eff4324) )
	ROM_LOAD( "123dth.114", 0x020000, 0x08000, CRC(04ba6ed1) SHA1(012be71c6b955beda2bd0ff376dcaab51b226723) )
	ROM_LOAD( "124dth.107", 0x000000, 0x08000, CRC(220db5d3) SHA1(3bfbe0eb97282c4ce449fd44e8e141de74f08eb0) )
	ROM_LOAD( "125dth.126", 0x020000, 0x08000, CRC(b0dccf4a) SHA1(e8bfd622c006985b724cdbd3ad14c33e9ed27c6c) )

	/* VGB - Video Graphics PCB  (MPG DW-010-00002-002) */
	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD( "101vgb.101", 0x000000, 0x20000, CRC(6aada23d) SHA1(85dbf9b20e4f17cb21922637763654d6cae80dfd) )
	ROM_LOAD( "102vgb.108", 0x020000, 0x20000, CRC(441e8490) SHA1(6cfe30cea3fa297b71e881fbddad6d65a96e4386) )
	ROM_LOAD( "103vgb.114", 0x040000, 0x20000, CRC(4e486e70) SHA1(04ee16cfadd43dbe9ed5bd8330c21a718d63a8f4) )
	ROM_LOAD( "104vgb.97",  0x060000, 0x20000, CRC(715cac9d) SHA1(2aa0c563dc1fe4d02fa1ecbaed16f720f899fdc4) )
	ROM_LOAD( "105vgb.124", 0x080000, 0x20000, CRC(5482e0c4) SHA1(492afac1862f2899cd734d1e57ca978ed6a906d5) )
	ROM_LOAD( "106vgb.121", 0x0a0000, 0x20000, CRC(a55e5d19) SHA1(86fbcb425103ae9fff381357339af349848fc3f2) )
	ROM_LOAD( "107vgb.130", 0x0c0000, 0x20000, CRC(006487b6) SHA1(f8bc6abad13df099da1708bd22f239703e407b21) )
	ROM_LOAD( "108vgb.133", 0x0e0000, 0x20000, CRC(e4587ba1) SHA1(1323b4be5a526ae182ee38e96fccd263a4cecc37) )

	/*SND - Sound PCB           (MPG 010-00018-002) */
	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "14-001snd.2", 0x000000, 0x08000, CRC(307fcb6d) SHA1(0cf63a39ac8920be6532974311804529d7218545) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "13-001snd.17", 0x000000, 0x40000, CRC(015a0b17) SHA1(f229c9aa59f0e6b25b818f9513997a8685e33982) )
ROM_END

ROM_START( stankatk )
	ROM_REGION( 0x140000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "lo_u67",    0x000001, 0x20000, CRC(97aabac0) SHA1(12a0719d3332a63e912161200b0a942c27c1f5da) )
	ROM_LOAD16_BYTE( "le_u91",    0x000000, 0x20000, CRC(977f90d9) SHA1(530fa5c32b1f28e2b90d20d98cc453cb290c0ad2) )
	ROM_LOAD16_BYTE( "ho_u68",    0x040001, 0x20000, CRC(8f76f4ac) SHA1(f6c1d4c933a373b153eee7d9f3016c985acaa281) )
	ROM_LOAD16_BYTE( "he_u92",    0x040000, 0x20000, CRC(1ea1db7c) SHA1(ecaa1bd3d70489a5ba0d96c6935c2959f57467b2) )
	ROM_LOAD16_BYTE( "b00_o.u69", 0x080001, 0x20000, CRC(393718e5) SHA1(f956f8bd946f53a032af16011dc69f66fb3f095c) )
	ROM_LOAD16_BYTE( "b00_e.u93", 0x080000, 0x20000, CRC(aedea0ef) SHA1(a81c3518c7a1e21f2fa2ad29c30346f727069257) )
	ROM_LOAD16_BYTE( "b01_o.u70", 0x0c0001, 0x20000, CRC(e895167d) SHA1(677cbf1be32c1f0c76a0e1527db66eb037d7e9df) )
	ROM_LOAD16_BYTE( "b01_e.u94", 0x0c0000, 0x20000, CRC(823bba4d) SHA1(6668e972b1435aac43f9b21cc40fc3adec0d285f) )
	ROM_LOAD16_BYTE( "host.71",   0x100001, 0x20000, CRC(cfc0333e) SHA1(9f290769129a61189870faef45c3f061eb7b5c07) ) // same as botss
	ROM_LOAD16_BYTE( "host.95",   0x100000, 0x20000, CRC(6c595d1e) SHA1(89fdc30166ba1e9706798547195bdf6875a02e96) ) // same as botss

	ROM_REGION( 0x40000, REGION_USER1, 0 )
	ROM_LOAD( "pb0o_u.153", 0x000000, 0x20000, CRC(bcd7ddad) SHA1(3982756b6f0821df77918dd0d00807a90dbfb595) )
	ROM_LOAD( "pb0e_u.154", 0x020000, 0x20000, CRC(d84e7c71) SHA1(2edb13c1f96f35c7934dad380e06035335ccbb48) )
	ROM_LOAD( "pb1o_u.167", 0x000000, 0x20000, CRC(e4a65313) SHA1(f2df5cc87aa388d3273705562ab2d7c937a0a866) )
	ROM_LOAD( "pb1e_u.160", 0x020000, 0x20000, CRC(9d9d1395) SHA1(9d937eac8d7e7bea40a69b596ba2c01753b97565) )

	ROM_LOAD( "s24o_u.135", 0x000000, 0x08000, CRC(f89bab5f) SHA1(e79e71d0a5e7ba933952c5d41f6afb633da06e8a) )
	ROM_LOAD( "s08o_u.115", 0x020000, 0x08000, CRC(af1eae4a) SHA1(44f272b472f546ffff7d8f82e29c5d80b472b1c3) )
	ROM_LOAD( "s00o_u.108", 0x000000, 0x08000, CRC(9cadc977) SHA1(e95f60d9df422511bae6a6c4a20f813d77a894a4) )
	ROM_LOAD( "s16o_u.127", 0x020000, 0x08000, CRC(53ba1a3f) SHA1(333734fff41b98abfa7b2904692cb128ab1f90a3) )
	ROM_LOAD( "s24e_u.134", 0x000000, 0x08000, CRC(0a41756b) SHA1(8681aaf8eeda7acdff967a773290c4b2c17cbe30) )
	ROM_LOAD( "s08e_u.114", 0x020000, 0x08000, CRC(765da5d7) SHA1(d489581bd12d7fca42570ee7a12d922be2528c1e) )
	ROM_LOAD( "s00e_u.107", 0x000000, 0x08000, CRC(558918cc) SHA1(7e61639ab4af88f888f4aa481dd01db7de3829da) )
	ROM_LOAD( "s16e_u.126", 0x020000, 0x08000, CRC(d24654cd) SHA1(88d3624f23c669dc902136c822b1f4732104c9c1) )

	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD( "3el_u101", 0x000000, 0x20000, CRC(130e1a18) SHA1(c31af5c5a403da588142ccbea79d3aa253ac6519) )
	ROM_LOAD( "3ch_u108", 0x020000, 0x20000, CRC(7cb688af) SHA1(6be495ae0ed74739f62de65386810864c9ffaaee) )
	ROM_LOAD( "3cl_u114", 0x040000, 0x20000, CRC(bc04b0e6) SHA1(d08fddd52f2c1a565a80f5d4ff8b07f1c5f01a01) )
	ROM_LOAD( "3eh_u97",  0x060000, 0x20000, CRC(0fdcab16) SHA1(afc21747e1624f3ab87b289b5f4a498141062445) )
	ROM_LOAD( "38l_u124", 0x080000, 0x20000, CRC(4e084daa) SHA1(f65f51d8d7c6b46aa844b37b212dab11c786d856) )
	ROM_LOAD( "38h_u121", 0x0a0000, 0x20000, CRC(3628c8c1) SHA1(760eda076ec46af5b954548036da5230a5c86371) )
	ROM_LOAD( "3al_u130", 0x0c0000, 0x20000, CRC(8a5386e3) SHA1(361f6abdb88cf51d5ec5ce6882986296dd274d3b) )
	ROM_LOAD( "3ah_u133", 0x0e0000, 0x20000, CRC(7e674ac1) SHA1(81f1d87e62faf94a44aca7e41a32edf5c7c145ec) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	/* nothing? */

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	/* nothing? */
ROM_END

ROM_START( f15se )
	/* HST - Host PCB            (MPG DW-00011C-0011-01)	*/
	ROM_REGION( 0x140000, REGION_CPU1, 0 ) /* 68000 Code */
	ROM_LOAD16_BYTE( "500.hst", 0x000001, 0x20000, CRC(6c26806d) SHA1(7cfd2b3b92b0fc6627c92a2013a317ca5abc66a0) )
	ROM_LOAD16_BYTE( "501.hst", 0x000000, 0x20000, CRC(81f02bf7) SHA1(09976746fe4d9c88bd8840f6e7addb09226aa54b) )
	ROM_LOAD16_BYTE( "502.hst", 0x040001, 0x20000, CRC(1eb945e5) SHA1(aba3ff038f2ca0f1200be5710073825ce80e3656) )
	ROM_LOAD16_BYTE( "503.hst", 0x040000, 0x20000, CRC(21fcb974) SHA1(56f78ce652e2bf432fbba8cda8c800f02dad84bb) )
	ROM_LOAD16_BYTE( "004.hst", 0x080001, 0x20000, CRC(81671ce1) SHA1(51ff641ccbc9dea640a62944910abe73d796b062) )
	ROM_LOAD16_BYTE( "005.hst", 0x080000, 0x20000, CRC(bdaa7db5) SHA1(52cd832cdd44e609e8cd269469b806e2cd27d63d) )
	ROM_LOAD16_BYTE( "006.hst", 0x0c0001, 0x20000, CRC(8eedef6d) SHA1(a1b5b53afc9ff092d86e7c7d4e357807fae3ad85) )
	ROM_LOAD16_BYTE( "007.hst", 0x0c0000, 0x20000, CRC(36e06cba) SHA1(5ffee5da6f475978be10fa5e1a2c24f00497ea5f) )
	ROM_LOAD16_BYTE( "008.hst", 0x100001, 0x20000, CRC(d96fd4e2) SHA1(001af758da437e955b4ee914eabeb9739ebc4454) )
	ROM_LOAD16_BYTE( "009.hst", 0x100000, 0x20000, CRC(33e3b473) SHA1(66deda79ba94f0ed722b399b3fc6062dcdd1a6c9) )

	/*  DTH - Dr Math PCB         (MPG 010-00002-001) */
	ROM_REGION( 0x40000, REGION_USER1, 0 )
	ROM_LOAD( "014.dth", 0x000000, 0x20000, CRC(5ca7713f) SHA1(ac7b9629684b99ecfb1945176b06eb6be284ba93) )
	ROM_LOAD( "015.dth", 0x020000, 0x20000, CRC(beae31bb) SHA1(1ab80a6b99eea6d5bf9b1bce58ecca13042c77a6) )
	ROM_LOAD( "016.dth", 0x000000, 0x20000, CRC(5db4f677) SHA1(25a6fe4c562e4fa4225aa4687dd41920b614e591) )
	ROM_LOAD( "017.dth", 0x020000, 0x20000, CRC(47f9a868) SHA1(7c8a9355893e4a3f3846fd05e0237ffd1404ffee) )

	ROM_LOAD( "118.dth", 0x000000, 0x08000, CRC(cc895c20) SHA1(140ef47536914fe1441778e759894c2cdd893276) )
	ROM_LOAD( "119.dth", 0x020000, 0x08000, CRC(b1c966e5) SHA1(9703bb1f9bdf6a779b59daebb39df2926727fa76) )
	ROM_LOAD( "120.dth", 0x000000, 0x08000, CRC(5fb9836d) SHA1(d511aa9f02972a7f475c82c6f57d1f3fd4f118fa) )
	ROM_LOAD( "121.dth", 0x020000, 0x08000, CRC(392e5c43) SHA1(455cf3bb3c16217e58d6eea51d8f49a5bed1955e) )
	ROM_LOAD( "122.dth", 0x000000, 0x08000, CRC(9d2032cf) SHA1(8430816756ea92bbe86b94eaa24a6071bf0ef879) )
	ROM_LOAD( "123.dth", 0x020000, 0x08000, CRC(54d5544f) SHA1(d039ee39991b947a7483111359ab245fc104e060) )
	ROM_LOAD( "124.dth", 0x000000, 0x08000, CRC(7be96646) SHA1(a6733f75c0404282d71e8c1a287546ef4d9d42ad) )
	ROM_LOAD( "125.dth", 0x020000, 0x08000, CRC(7718487c) SHA1(609106f55601f84095b64ce2484107779da89149) )

	/* Video Graphics PCB  (MPG DW-010-00003-001) */
	ROM_REGION( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD( "001.vgb", 0x000000, 0x20000, CRC(810c142d) SHA1(d37e5ecd716dda65d43cec7bca524c59d3dc9803) )
	ROM_LOAD( "002.vgb", 0x020000, 0x20000, CRC(f6488e31) SHA1(d2f9304cc59f5523007592ae76ddd56107cc29e8) )
	ROM_LOAD( "003.vgb", 0x040000, 0x20000, CRC(4d8e8f54) SHA1(d8a23b5fd00ab919dc6d63fc72824d1293073813) )
	ROM_LOAD( "004.vgb", 0x060000, 0x20000, CRC(b69e1260) SHA1(1a2b69ea7c96b0293b24d87ea46bd4b1d4c56a66) )
	ROM_LOAD( "005.vgb", 0x080000, 0x20000, CRC(7b1852f0) SHA1(d21525e59b3112313ea9783ac3dd988a4c1d5f87) )
	ROM_LOAD( "006.vgb", 0x0a0000, 0x20000, CRC(9d031636) SHA1(b7c7b57d547f2ce2eeb97126e961f3b5f35823f7) )
	ROM_LOAD( "007.vgb", 0x0c0000, 0x20000, CRC(15326070) SHA1(ec4484d4515694742d3fd3b944f342f052463988) )
	ROM_LOAD( "008.vgb", 0x0e0000, 0x20000, CRC(ca0e86d8) SHA1(a7b4b02d100a7875d5a184cdb76d507e926d1ca3) )

	/* SND - Sound PCB           (MPG 010-00018-002) */
	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "4-001.snd", 0x000000, 0x08000, CRC(705685a9) SHA1(311f7cac126a19e8bd555ebf31ff4ec4680ddfa4) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "3-001.snd", 0x000000, 0x40000, CRC(af84b635) SHA1(844e5987a66e9e3ab2d2fe05b93a4da3512776bb) )
ROM_END

GAMEX( 1990, f15se,    0, micro3d, micro3d, 0, ROT0, "Microprose", "F-15 Strike Eagle", GAME_NOT_WORKING | GAME_NO_SOUND )
GAMEX( 199?, botss,    0, micro3d, micro3d, 0, ROT0, "Microprose", "Battle of the Solar System", GAME_NOT_WORKING | GAME_NO_SOUND )
GAMEX( 199?, stankatk, 0, micro3d, micro3d, 0, ROT0, "Microprose", "Super Tank Attack (prototype?)", GAME_NOT_WORKING | GAME_NO_SOUND )
