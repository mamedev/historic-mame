/* CPS 3 */

/*

CPS3 Games are supplied with a CD and Cartridge.
CD contains the game data, Cartridge appears to contain a chip for the encryption and a Bios rom

the CPU is thought to be SH-2 based

the cartridges are backed up by a battery and are highly sensitive.

this driver does nothing, it is for reference purposes only for now.

Very few games were produced for this system (6 in total)

JoJo no Kimyouna Bouken / JoJo's Venture
JoJo no Kimyouna Bouken  Miraie no Isan / JoJo's Bizarre Adventure  Heritage for the Future
Street Fighter III  New Generation
Street Fighter III, 2nd Impact  Giant Attack
Street Fighter III, 3rd Strike  Fight for the Future
Warzard / Red Earth

Notes:

JoJo Venture  -  bios dumped
JoJo Biz Adv  -  bios not dumped
SF3 - New Gen -  bios dumped
SF3 - Sec Imp -  bios not dumped
SF3 - 3rd Str -  bios not dumped
Warzard       -  bios dumped


Bios Rom in cart = region?

CD image also contains bmp format files of the title logos

2nd impact is interesting as the data isn't encrypted on it

CPS3 can read CD-R images, security is provided by the cartridge.  rom data from cd is checksumed.

*/

/* load extracted cd content? */
#define LOAD_CD_CONTENT 1

#include "driver.h"

VIDEO_START(cps3)
{
	return 0;
}

VIDEO_UPDATE(cps3)
{

}

static ADDRESS_MAP_START( cps3_map, ADDRESS_SPACE_PROGRAM, 32 )
ADDRESS_MAP_END

INPUT_PORTS_START( cps3 )
INPUT_PORTS_END

static MACHINE_DRIVER_START( cps3 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", SH2, 20000000) // ??
	MDRV_CPU_PROGRAM_MAP(cps3_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 64*8)
	MDRV_VISIBLE_AREA(0, 64*8-1, 0, 64*8-1)
	MDRV_PALETTE_LENGTH(0x1000)

	MDRV_VIDEO_START(cps3)
	MDRV_VIDEO_UPDATE(cps3)
MACHINE_DRIVER_END

static DRIVER_INIT (nocpu)
{
	cpunum_set_input_line(0, INPUT_LINE_RESET, ASSERT_LINE);
}


ROM_START( sfiii )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* dummy cpu region */

	ROM_REGION( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "sf3bios",  0x000000, 0x080000, CRC(74205250) SHA1(c3e83ace7121d32da729162662ec6b5285a31211) )

	/* Convert this to CHD? */
	ROM_REGION( 0x0000800, REGION_USER2, 0 ) /* cd cue image */
	ROM_LOAD( "sf3.cue",  0x0000000, 0x0000046, CRC(54c89005) SHA1(601d2aaac1298aa5d2eddb52e2a0a5194d790cd7) )
	ROM_REGION( 0x331f980, REGION_USER3, 0 ) /* cd bin image */
	ROM_LOAD( "sf3.bin",  0x0000000, 0x331f980, CRC(6e72edcc) SHA1(dc46e22e9f04d1a5940dfdd81d011a2d7e5aa571) )

	#ifdef LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
	         This is done only to make analysis easier. Once correct
	         emulation is possible this region will be removed and the
	         roms will be loaded into flashram from the CD by the system */
	ROM_REGION( 0x800000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x0000000, 0x800000, CRC(e896dc27) SHA1(47623820c64b72e69417afcafaacdd2c318cde1c) )
	ROM_REGION( 0x2400000, REGION_USER5, 0 ) /* cd content region */
	ROM_LOAD( "30",  0x0000000, 0x800000, CRC(98c2d07c) SHA1(604ce4a16170847c10bc233a47a47a119ce170f7) )
	ROM_LOAD( "31",  0x0800000, 0x800000, CRC(7115a396) SHA1(b60a74259e3c223138e66e68a3f6457694a0c639) )
	ROM_LOAD( "40",  0x1000000, 0x800000, CRC(839f0972) SHA1(844e43fcc157b7c774044408bfe918c49e174cdb) )
	ROM_LOAD( "41",  0x1800000, 0x800000, CRC(8a8b252c) SHA1(9ead4028a212c689d7a25746fbd656dca6f938e8) )
	ROM_LOAD( "50",  0x2000000, 0x400000, CRC(58933dc2) SHA1(1f1723be676a817237e96b6e20263b935c59daae) )
	/* 90,91,92,93 are bmp images, not extracted */
	#endif
ROM_END

ROM_START( sfiii2 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* dummy cpu region */

	ROM_REGION( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "sf32ibios.rom",  0x000000, 0x080000, CRC(faea0a3e) SHA1(a03cd63bcf52e4d57f7a598c8bc8e243694624ec) )

	/* Convert this to CHD? */
	ROM_REGION( 0x0000800, REGION_USER2, 0 ) /* cd cue image */
	ROM_LOAD( "cap-3ga-1.cue",  0x0000000, 0x000004b, CRC(2ba55809) SHA1(6e32186345eca6ab49e01dcf41ad92e22b21409d) )
	ROM_REGION( 0x4a17980, REGION_USER3, 0 ) /* cd bin image */
	ROM_LOAD( "cap-3ga-1.bin",  0x0000000, 0x4a17980, CRC(1c95fdd3) SHA1(f5b13ef2eacabf4aea9beb8244b75e9e01868dbc) )

	#ifdef LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
	         This is done only to make analysis easier. Once correct
	         emulation is possible this region will be removed and the
	         roms will be loaded into flashram from the CD by the system */
	ROM_REGION( 0x1000000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x0000000, 0x800000, CRC(682b014a) SHA1(abd5785f4b7c89584d6d1cf6fb61a77d7224f81f) )
	ROM_LOAD( "20",  0x0800000, 0x800000, CRC(38090460) SHA1(aaade89b8ccdc9154f97442ca35703ec538fe8be) )
	ROM_REGION( 0x3000000, REGION_USER5, 0 ) /* cd content region */
	ROM_LOAD( "30",  0x0000000, 0x800000, CRC(77c197c0) SHA1(944381161462e65de7ae63a656658f3fbe44727a) )
	ROM_LOAD( "31",  0x0800000, 0x800000, CRC(7470a6f2) SHA1(850b2e20afe8a5a1f0d212d9abe002cb5cf14d22) )
	ROM_LOAD( "40",  0x1000000, 0x800000, CRC(01a85ced) SHA1(802df3274d5f767636b2785606e0558f6d3b9f13) )
	ROM_LOAD( "41",  0x1800000, 0x800000, CRC(fb346d74) SHA1(57570f101a170aa7e83e84e4b7cbdc63a11a486a) )
	ROM_LOAD( "50",  0x2000000, 0x800000, CRC(32f79449) SHA1(44b1f2a640ab4abc23ff47e0edd87fbd0b345c06) )
	ROM_LOAD( "51",  0x2800000, 0x800000, CRC(1102b8eb) SHA1(c7dd2ee3a3214c6ec47a03fe3e8c941775d57f76) )
	/* 90,91,92,93 are bmp images, not extracted */
	#endif
ROM_END

ROM_START( sfiii3 )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* dummy cpu region */

	ROM_REGION( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "sf33rdbios.rom",  0x000000, 0x080000, BAD_DUMP CRC(9fa37a05) SHA1(22829c03d5109c451fc677a21592407cc09bcaa1) ) // fixed bits

	/* Convert this to CHD? */
	ROM_REGION( 0x0000800, REGION_USER2, 0 ) /* cd cue image */
	ROM_LOAD( "cap-33s-1.cue",  0x0000000, 0x000004b, CRC(bd084ff0) SHA1(cff0ea9fafc287d8cb70f1bf86366717e90453d0) )
	ROM_REGION( 0x5c77980, REGION_USER3, 0 ) /* cd bin image */
	ROM_LOAD( "cap-33s-1.bin",  0x0000000, 0x5c77980, CRC(bc8845b9) SHA1(36445f7bc80371bfba23ab5917c0477204d5a04d) )

	#ifdef LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
	         This is done only to make analysis easier. Once correct
	         emulation is possible this region will be removed and the
	         roms will be loaded into flashram from the CD by the system */
	ROM_REGION( 0x1000000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x0000000, 0x800000, CRC(77233d39) SHA1(59c3f890fdc33a7d8dc91e5f9c4e7b7019acfb00) )
	ROM_LOAD( "20",  0x0800000, 0x800000, CRC(5ca8faba) SHA1(71c12638ae7fa38b362d68c3ccb4bb3ccd67f0e9) )
	ROM_REGION( 0x4000000, REGION_USER5, 0 ) /* cd content region */
	ROM_LOAD( "30",  0x0000000, 0x800000, CRC(b37cf960) SHA1(60310f95e4ecedee85846c08ccba71e286cda73b) )
	ROM_LOAD( "31",  0x0800000, 0x800000, CRC(450ec982) SHA1(1cb3626b8479997c4f1b29c41c81cac038fac31b) )
	ROM_LOAD( "40",  0x1000000, 0x800000, CRC(632c965f) SHA1(9a46b759f5dee35411fd6446c2457c084a6dfcd8) )
	ROM_LOAD( "41",  0x1800000, 0x800000, CRC(7a4c5f33) SHA1(f33cdfe247c7caf9d3d394499712f72ca930705e) )
	ROM_LOAD( "50",  0x2000000, 0x800000, CRC(8562358e) SHA1(8ed78f6b106659a3e4d94f38f8a354efcbdf3aa7) )
	ROM_LOAD( "51",  0x2800000, 0x800000, CRC(7baf234b) SHA1(38feb45d6315d771de5f9ae965119cb25bae2a1e) )
	ROM_LOAD( "60",  0x3000000, 0x800000, CRC(bc9487b7) SHA1(bc2cd2d3551cc20aa231bba425ff721570735eba) )
	ROM_LOAD( "61",  0x3800000, 0x800000, CRC(b813a1b1) SHA1(16de0ee3dfd6bf33d07b0ff2e797ebe2cfe6589e) )
	/* 92,93 are bmp images, not extracted */
	#endif
ROM_END

ROM_START( warzard )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* dummy cpu region */

	ROM_REGION( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "warzard_cart_flashrom.bin",  0x000000, 0x080000, CRC(f8e2f0c6) SHA1(93d6a986f44c211fff014e55681eca4d2a2774d6) )

	/* Convert this to CHD? */
	ROM_REGION( 0x0000800, REGION_USER2, 0 ) /* cd cue image */
	ROM_LOAD( "wz.cue",  0x0000000, 0x0000045,  CRC(46dfcbee) SHA1(94841bd478383727377de41e77114a0517c0a379) )
	ROM_REGION( 0x331f980, REGION_USER3, 0 ) /* cd bin image */
	ROM_LOAD( "wz.bin",  0x0000000, 0x331f980, CRC(0d93d45a) SHA1(d121936c99d9fc3a9649a4b2534764d5a083e963) )

	#ifdef LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
	         This is done only to make analysis easier. Once correct
	         emulation is possible this region will be removed and the
	         roms will be loaded into flashram from the CD by the system */
	ROM_REGION( 0x800000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x000000, 0x800000, CRC(68188016) SHA1(93aaac08cb5566c33aabc16457085b0a36048019) )
	ROM_REGION( 0x2400000, REGION_USER5, 0 ) /* cd content region */
	ROM_LOAD( "30",  0x0000000, 0x800000, CRC(074cab4d) SHA1(4cb6cc9cce3b1a932b07058a5d723b3effa23fcf) )
	ROM_LOAD( "31",  0x0800000, 0x800000, CRC(14e2cad4) SHA1(9958a4e315e4476e4791a6219b93495413c7b751) )
	ROM_LOAD( "40",  0x1000000, 0x800000, CRC(72d98890) SHA1(f40926c52cb7a71b0ef0027a0ea38bbc7e8b31b0) )
	ROM_LOAD( "41",  0x1800000, 0x800000, CRC(88ccb33c) SHA1(1e7af35d186d0b4e45b6c27458ddb9cfddd7c9bc) )
	ROM_LOAD( "50",  0x2000000, 0x400000, CRC(2f5b44bd) SHA1(7ffdbed5b6899b7e31414a0828e04543d07435e4) )
	/* 90,91,92,93 are bmp images, not extracted */
	#endif
ROM_END

ROM_START( jojo )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* dummy cpu region */

	ROM_REGION( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "jojocart.bin",  0x000000, 0x080000, CRC(02778f60) SHA1(a167f9ebe030592a0cdb0c6a3c75835c6a43be4c) )

	/* Convert this to CHD? */
	ROM_REGION( 0x0000800, REGION_USER2, 0 ) /* cd cue image */
	ROM_LOAD( "cap-jjk-140.cue",  0x0000000, 0x000004d, CRC(7eae4b4d) SHA1(2ded408229fc8919d61404e556b264cac34a5919) )
	ROM_REGION( 0x3c4f980, REGION_USER3, 0 ) /* cd bin image */
	ROM_LOAD( "cap-jjk-140.bin",  0x0000000, 0x3c4f980, CRC(beb952ae) SHA1(fbe80bcf3521259be6dbc6890780a419d87ccdbb) )

	#ifdef LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
	         This is done only to make analysis easier. Once correct
	         emulation is possible this region will be removed and the
	         roms will be loaded into flashram from the CD by the system */
	ROM_REGION( 0x1000000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x0000000, 0x800000, CRC(e40dc123) SHA1(517e7006349b5a8fd6c30910362583f48d009355) )
	ROM_LOAD( "20",  0x0800000, 0x800000, CRC(0571e37c) SHA1(1aa28ef6ea1b606a55d0766480b3ee156f0bca5a) )
	ROM_REGION( 0x2400000, REGION_USER5, 0 ) /* cd content region */
	ROM_LOAD( "30",  0x0000000, 0x800000, CRC(1d99181b) SHA1(25c216de16cefac2d5892039ad23d07848a457e6) )
	ROM_LOAD( "31",  0x0800000, 0x800000, CRC(6889fbda) SHA1(53a51b993d319d81a604cdf70b224955eacb617e) )
	ROM_LOAD( "40",  0x1000000, 0x800000, CRC(8069f9de) SHA1(7862ee104a2e9034910dd592687b40ebe75fa9ce) )
	ROM_LOAD( "41",  0x1800000, 0x800000, CRC(9c426823) SHA1(1839dccc7943a44063e8cb2376cd566b24e8b797) )
	ROM_LOAD( "50",  0x2000000, 0x400000, CRC(1c749cc7) SHA1(23df741360476d8035c68247e645278fbab53b59) )
	/* 92,93 are bmp images, not extracted */
	#endif
ROM_END


ROM_START( jojoba )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* dummy cpu region */

	ROM_REGION( 0x080000, REGION_USER1, 0 ) /* bios region */
	// this was from a version which doesn't require the cd to run
	ROM_LOAD( "jojoba.rom",  0x000000, 0x080000, CRC(4dab19f5) SHA1(ba07190e7662937fc267f07285c51e99a45c061e) )

	/* Convert this to CHD? */
	ROM_REGION( 0x0000800, REGION_USER2, 0 ) /* cd cue image */
	ROM_LOAD( "cap-jjm-110.cue",  0x0000000, 0x000004d, CRC(c9d6c2e8) SHA1(40d72a034b9d1ffa432d73cd7771de9abd0cf613) )
	ROM_REGION( 0x4a17980, REGION_USER3, 0 ) /* cd bin image */
	ROM_LOAD( "cap-jjm-110.bin",  0x0000000, 0x4a17980, CRC(6b02c63a) SHA1(66b9a20560bc7d033420adf73e34f26c49893d80) )

	#ifdef LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
	         This is done only to make analysis easier. Once correct
	         emulation is possible this region will be removed and the
	         roms will be loaded into flashram from the CD by the system */
	ROM_REGION( 0x1000000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x000000, 0x800000, CRC(6e2490f6) SHA1(75cbf1e39ad6362a21c937c827e492d927b7cf39) )
	ROM_LOAD( "20",  0x800000, 0x800000, CRC(1293892b) SHA1(b1beafac1a9c4b6d0640658af8a3eb359e76eb25) )
	ROM_REGION( 0x3000000, REGION_USER5, 0 ) /* cd content region */
	ROM_LOAD( "30",  0x0000000, 0x800000, CRC(d25c5005) SHA1(93a19a14783d604bb42feffbe23eb370d11281e8) )
	ROM_LOAD( "31",  0x0800000, 0x800000, CRC(51bb3dba) SHA1(39e95a05882909820b3efa6a3b457b8574012638) )
	ROM_LOAD( "40",  0x1000000, 0x800000, CRC(94dc26d4) SHA1(5ae2815142972f322886eea4885baf2b82563ab1) )
	ROM_LOAD( "41",  0x1800000, 0x800000, CRC(1c53ee62) SHA1(e096bf3cb6fbc3d45955787b8f3213abcd76d120) )
	ROM_LOAD( "50",  0x2000000, 0x800000, CRC(36e416ed) SHA1(58d0e95cc13f39bc171165468ce72f4f17b8d8d6) )
	ROM_LOAD( "51",  0x2800000, 0x800000, CRC(eedf19ca) SHA1(a7660bf9ff87911afb4f83b64456245059986830) )
	/* 92,93 are bmp images, not extracted */
	#endif
ROM_END

ROM_START( jojobaa )
	ROM_REGION( 0x100000, REGION_CPU1, 0 ) /* dummy cpu region */

	ROM_REGION( 0x080000, REGION_USER1, 0 ) /* bios region */
	ROM_LOAD( "jojobacart.bin",  0x000000, 0x080000,  CRC(3085478c) SHA1(055eab1fc42816f370a44b17fd7e87ffcb10e8b7) )

	/* Convert this to CHD? */
	ROM_REGION( 0x0000800, REGION_USER2, 0 ) /* cd cue image */
	ROM_LOAD( "cap-jjm-110.cue",  0x0000000, 0x000004d, CRC(c9d6c2e8) SHA1(40d72a034b9d1ffa432d73cd7771de9abd0cf613) )
	ROM_REGION( 0x4a17980, REGION_USER3, 0 ) /* cd bin image */
	ROM_LOAD( "cap-jjm-110.bin",  0x0000000, 0x4a17980, CRC(6b02c63a) SHA1(66b9a20560bc7d033420adf73e34f26c49893d80) )

	#ifdef LOAD_CD_CONTENT
	/* Note: These regions contains the rom data extracted from the cd.
	         This is done only to make analysis easier. Once correct
	         emulation is possible this region will be removed and the
	         roms will be loaded into flashram from the CD by the system */
	ROM_REGION( 0x1000000, REGION_USER4, 0 ) /* cd content region */
	ROM_LOAD( "10",  0x000000, 0x800000, CRC(6e2490f6) SHA1(75cbf1e39ad6362a21c937c827e492d927b7cf39) )
	ROM_LOAD( "20",  0x800000, 0x800000, CRC(1293892b) SHA1(b1beafac1a9c4b6d0640658af8a3eb359e76eb25) )
	ROM_REGION( 0x3000000, REGION_USER5, 0 ) /* cd content region */
	ROM_LOAD( "30",  0x0000000, 0x800000, CRC(d25c5005) SHA1(93a19a14783d604bb42feffbe23eb370d11281e8) )
	ROM_LOAD( "31",  0x0800000, 0x800000, CRC(51bb3dba) SHA1(39e95a05882909820b3efa6a3b457b8574012638) )
	ROM_LOAD( "40",  0x1000000, 0x800000, CRC(94dc26d4) SHA1(5ae2815142972f322886eea4885baf2b82563ab1) )
	ROM_LOAD( "41",  0x1800000, 0x800000, CRC(1c53ee62) SHA1(e096bf3cb6fbc3d45955787b8f3213abcd76d120) )
	ROM_LOAD( "50",  0x2000000, 0x800000, CRC(36e416ed) SHA1(58d0e95cc13f39bc171165468ce72f4f17b8d8d6) )
	ROM_LOAD( "51",  0x2800000, 0x800000, CRC(eedf19ca) SHA1(a7660bf9ff87911afb4f83b64456245059986830) )
	/* 92,93 are bmp images, not extracted */
	#endif
ROM_END




GAMEX( 1997, sfiii,   0,        cps3, cps3, nocpu, ROT0,   "Capcom", "Street Fighter III - New Generation", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEX( 1998, sfiii2,  0,        cps3, cps3, nocpu, ROT0,   "Capcom", "Street Fighter III 2nd Impact - Giant Attack", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEX( 1999, sfiii3,  0,        cps3, cps3, nocpu, ROT0,   "Capcom", "Street Fighter III 3rd Strike -  Fight for the Future", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEX( 1996, warzard, 0,        cps3, cps3, nocpu, ROT0,   "Capcom", "Warzard", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEX( 1998, jojo,    0,        cps3, cps3, nocpu, ROT0,   "Capcom", "JoJo's Venture", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEX( 1999, jojoba,  0,        cps3, cps3, nocpu, ROT0,   "Capcom", "JoJo's Bizarre Adventure", GAME_NOT_WORKING|GAME_NO_SOUND )
GAMEX( 1999, jojobaa, jojoba,   cps3, cps3, nocpu, ROT0,   "Capcom", "JoJo's Bizarre Adventure (alt)", GAME_NOT_WORKING|GAME_NO_SOUND )
