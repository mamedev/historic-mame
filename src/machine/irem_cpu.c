/*****************************************************************************

	Irem Custom V30 CPU:

	It uses a simple opcode lookup encryption, the painful part is that it's
	preprogrammed into the cpu and isn't a algorithmic based one.

	Ken-Go							?		?

	Hasamu							Nanao   08J27261A1 011 9102KK700
	Gunforce						Nanao 	08J27261A  011 9106KK701

	Bomberman						Nanao   08J27261A1 012 9123KK200
	Blade Master					?		? (Same as Bomberman)

	Quiz F-1 1,2 Finish          	Nanao	08J27291A4 014 9147KK700
	Lethal Thunder					?		? (Same as Quiz F1)

	Bomberman World/Atomic Punk				?
	Undercover Cops							? (Same as BMan World)

	Skins Game						Nanao 	08J27291A7
	R-Type Leo						Irem 	D800001A1
	In The Hunt						Irem 	D8000011A1 020
	Gun Hohki									  16?
	Risky Challenge/Gussun Oyoyo 			D8000019A1
	Shisensho II                 			D8000020A1 023 9320NK700
	Perfect Soldiers						D8000022A1

	Emulation by Bryan McPhail, mish@tendril.co.uk, thanks to Chris Hardy too!

*****************************************************************************/

#include "driver.h"

#define xxxx 0xf1 /* Unknown */

unsigned char gussun_decryption_table[256] = {
	xxxx,xxxx,xxxx,0x36,xxxx,0x52,0xb1,0x5b, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 00 */
	xxxx,xxxx,0x75,xxxx,xxxx,0x83,0x33,0xe9, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0xc5,xxxx, /* 10 */
	0x5d,0xa4,xxxx,0x51,xxxx,xxxx,xxxx,xxxx ,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0x03,0x5f, /* 20 */
	0x26,xxxx,xxxx,0x8b,xxxx,0x02,xxxx,xxxx, 0x8e,0xab,xxxx,xxxx,0xbc,0x90,0xb3,xxxx, /* 30 */
	xxxx,xxxx,0xc6,xxxx,xxxx,0x3a,xxxx,xxxx, xxxx,0x74,xxxx,xxxx,0x33,xxxx,xxxx,xxxx, /* 40 */
	xxxx,0x53,xxxx,xxxx,0xc3,xxxx,0xfc,0xe7, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0xba,xxxx, /* 50 */
	0xb0,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0x07, 0xb9,xxxx,xxxx,0x46,xxxx,xxxx,xxxx,xxxx, /* 60 */
	xxxx,0xea,0x72,0x73,xxxx,0xd1,0x3b,0x5e, 0xe5,0x57,xxxx,0x0d,xxxx,xxxx,xxxx,0x3c, /* 70 */
	xxxx,0x86,xxxx,xxxx,xxxx,0x25,0x2d,xxxx, 0x9a,0xeb,xxxx,0x0b,xxxx,0xb8,0x81,xxxx, /* 80 */
	xxxx,xxxx,xxxx,xxxx,0xbb,xxxx,xxxx,xxxx, 0xa8,xxxx,xxxx,xxxx,0x43,0x56,xxxx,xxxx, /* 90 */
	xxxx,0xa3,xxxx,xxxx,xxxx,xxxx,0xfa,xxxx, xxxx, 0x81 ,0xe6,xxxx,0x80,xxxx,xxxx,xxxx, /* a0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,0x3d,0x3e,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* b0 */
	xxxx,0xff,xxxx,xxxx,0x55,0x1e,xxxx,0x59, 0x40,xxxx,xxxx,xxxx,0x88, 0xbd ,xxxx,0xb2, /* c0 */
	xxxx,xxxx,0x06,0xc7,0x05,xxxx,0x8a,0x5a, 0x58,0xbe,xxxx,xxxx,xxxx,0x1f,0x23,xxxx, /* d0 */
	0xe8,xxxx,0x89,0xa1,0xd0,xxxx,xxxx,0xe2, 0x38,0xfe,0x50,xxxx,xxxx,xxxx,xxxx,xxxx, /* e0 */
	xxxx,xxxx,0xf3,xxxx,xxxx,0x0f,xxxx,xxxx, xxxx,xxxx,0xf7,xxxx,0x39,xxxx,0xbf,xxxx, /* f0 */
};
//above - c8 (inc aw) guess from stos code
//c5 (push ds) guess (pop ds soon after) right?
//0xa9 (not 0x82 PRE) guess from 237df
//cd total guess (wrong but 3 bytes)

//double check 0x00/0xa0 AND.
//double check 0x8c (0x7d jg)
//double check 0xfd (0x20 AND) - 9d2 in code
//double check 0xd1 (0x41 INC cw) used in uccops and dynablaster (LOOKS GOOD)

//AND fd (0x20)
//0x37 (91) guess from dynablaster title screen
unsigned char dynablaster_decryption_table[256] = {
	0x1f,0x51,0x84,xxxx,0x3d,0x09,0x0d,xxxx, xxxx,0x57,xxxx,xxxx,xxxx,0x32,0x11,xxxx, /* 00 */
	xxxx,0x9c,xxxx,xxxx,0x4b,xxxx,xxxx,0x03, xxxx,xxxx,xxxx,0x89,0xb0,xxxx,xxxx,xxxx, /* 10 */
	xxxx,0xbb,xxxx,0xbe,0x53,0x21,0x55,0x7c, xxxx,xxxx,0x47,0x58,0xf6,xxxx,xxxx,0xb2, /* 20 */
	0x06,xxxx,0x2b,xxxx,0x2f,0x0b,0xfc, 0x91 , xxxx,xxxx,0xfa,0x81,0x83,0x40,0x38,xxxx, /* 30 */
	xxxx,xxxx,0x49,0x85,0xd1,0xf5,0x07,0xe2, 0x5e,0x1e,xxxx,0x04,xxxx,xxxx,xxxx,0xb1, /* 40 */
	0xc7,xxxx,xxxx, 0xf2 /*0xaf*/,0xb6,0xd2,0xc3,xxxx, 0x87,0xba, 0xcb/*0x36*/,0x88,xxxx,0xb9,0xd0,0xb5, /* 50 */
	0x9a,0x82,0xa2,0x72,xxxx,0xb4,xxxx,0xaa, 0x26,0x7d,0x52,0x33,0x2e,0xbc,0x08,0x79, /* 60 */
	0x48,xxxx,0x76,0x36,0x02,xxxx,0x5b,0x12, 0x8b,0xe7,xxxx,xxxx,xxxx,0xab,xxxx,0x4f, /* 70 */
	xxxx,xxxx,0xa8,0xe5,0x39,0x0e,0xa9,xxxx, xxxx,0x14,xxxx,0xff, 0x7f/*0x75*/ ,xxxx,xxxx,0x27, /* 80 */
	xxxx,0x01,xxxx,xxxx,0xe6,0x8a,0xd3,xxxx, xxxx,0x8e,0x56,0xa5,0x92,xxxx,xxxx,0xf9, /* 90 */
	0x00,xxxx,0x5f,xxxx,xxxx,0xa1,xxxx,0x74, 0xb8,xxxx,0x46,0x05,0xeb,0xcf,0xbf,0x5d, /* a0 */
	0x24,xxxx,0x9d,xxxx,xxxx,xxxx,xxxx,xxxx, 0x59,0x8d,0x3c,0xf8,0xc5,xxxx,0xf3,0x4e, /* b0 */
	xxxx,xxxx,0x50,0xc6,0xe9,0xfe,0x0a,xxxx, 0x99,0x86,xxxx,xxxx,0xaf ,0x8c/*0x8e*/,0x42,0xf7, /* c0 */
	xxxx,0x41,xxxx,0xa3,xxxx,0x3a,0x2a,0x43, xxxx,0xb3,0xe8,xxxx,0xc4,0x35,0x78,0x25, /* d0 */
	0x75,xxxx,0xb7,xxxx,0x23,xxxx, xxxx/*0xe2*/,0x8f, xxxx,xxxx,0x2c,xxxx,0x77,0x7e,xxxx,0x0f, /* e0 */
	0x0c,0xa0,0xbd,xxxx,xxxx,0x2d,0x29,0xea, xxxx,0x3b,0x73,xxxx,0xfb,0x30 /*20*/,xxxx,0x5a /* f0 */
};

unsigned char leagueman_decryption_table[256] = {
	xxxx,xxxx,xxxx,0x57,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 00 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,0xba,xxxx,0x1e,xxxx, /* 10 */
	0x2c,xxxx,xxxx,0xb5,xxxx,xxxx,xxxx,0xfe, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0x36,0x04, /* 20 */
	0xcf,xxxx,0xf3,0x5a,0x8a,0x0c,xxxx,xxxx, xxxx,xxxx,0xb2,0x50,xxxx,xxxx,xxxx,xxxx, /* 30 */
	xxxx,xxxx,0x24,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 40 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0x00,xxxx, xxxx,0x3a,0x5b,0xa2,xxxx,0x82,xxxx,xxxx, /* 50 */
	xxxx,xxxx,xxxx,xxxx,0x59,xxxx,0x88,xxxx, xxxx,0xbf,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 60 */
	0x72,xxxx,0x73,xxxx,xxxx,xxxx,xxxx,0x0f, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0xc6, /* 70 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0x8e,xxxx, 0x81,xxxx,0x58,xxxx,0xaa,xxxx,0x89,xxxx, /* 80 */
	xxxx,xxxx,xxxx,xxxx,xxxx,0x2f,0xbf,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0x07,xxxx, /* 90 */
	0xa0,xxxx,xxxx,xxxx,0xb0,xxxx,xxxx,xxxx, xxxx,xxxx,0x32,xxxx,xxxx,xxxx,0x74,0x0a, /* A0 */
	xxxx,xxxx,xxxx,xxxx,0x75,0x03,xxxx,xxxx, 0xb6,0x02,xxxx,xxxx,xxxx,xxxx,0xb8,xxxx, /* B0 */
	0xe8,xxxx,0xfc,xxxx,xxxx,0xc3,xxxx,0x06, xxxx,0x1f,xxxx,xxxx,xxxx,xxxx,xxxx,0xd0, /* C0 */
	xxxx,xxxx,0x87,xxxx,xxxx,xxxx,0x3c,0xc7, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* D0 */
	xxxx,xxxx,xxxx,0x8b,xxxx,xxxx,0x33,xxxx, xxxx,xxxx,xxxx,xxxx,0xfa,xxxx,xxxx,xxxx, /* E0 */
	xxxx,xxxx,xxxx,0xea,xxxx,xxxx,xxxx,0x5f, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* F0 */
};
//33 64 5a 8a 30 ec guess (above)
//3b guess
//af 2f guess
//cf guess

//96 guess
//a4



unsigned char shisen2_decryption_table[256] = {
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, 0xea,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 00 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 10 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 20 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0xb8, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 30 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 40 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 50 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 60 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,0xbc,xxxx,xxxx,xxxx, /* 70 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 80 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 90 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* A0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* B0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0x0f,xxxx, /* C0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* D0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* E0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* F0 */
};

//double check 0x00 0x22 0x28 0x4a 0x34 in these tables

//0x1ceb - 0x76 xx JNC
//therefore 0x76 cant be AND (0x22) as carry flag is unaffect

unsigned char lethalth_decryption_table[256] = {
	xxxx,0x26,xxxx,xxxx,0xba,xxxx,xxxx,0x5e, 0xb8,xxxx,0xbc,0xe8,xxxx,xxxx, 0x4f, xxxx, /* 00 */
	xxxx,xxxx,xxxx,0x00,xxxx,xxxx,0x02,0x57, xxxx,xxxx,xxxx,xxxx,0xe7,0x52,xxxx,xxxx, /* 10 */
	xxxx,xxxx,0xc6,0x06,0xa0,0xfe,0xcf,0x8e, 0x43,xxxx,xxxx,xxxx,xxxx,0x1f,0x75,0xa2, /* 20 */
	xxxx,xxxx,xxxx,xxxx,xxxx,0x89,xxxx,0x82, xxxx,0x72,0x07,xxxx,0x42,xxxx,0x0a,xxxx, /* 30 */
	0x88,0xb4,xxxx,0x8b,0xb9,0x9c,xxxx,xxxx, xxxx,0x4a,0xbf,xxxx,xxxx,xxxx,0x56,0xb0, /* 40 */
	xxxx,0x34,xxxx,0xeb,xxxx,0x50,xxxx,xxxx, 0x47,xxxx,xxxx,xxxx,xxxx,0xab,xxxx,xxxx, /* 50 */
	0xc3,0xe2,0xd0,0xb2,xxxx,0x79,xxxx,xxxx, xxxx,0xfb,xxxx,0x2c,xxxx,xxxx,xxxx,xxxx, /* 60 */
	xxxx,xxxx,0x88,0x83,0x3c,xxxx, 0x29/*0x22*/, 0x34, 0x5b,xxxx,xxxx,xxxx,xxxx,0x04,0xfc,xxxx, /* 70 */
	0xb1,0xf3,0x8a,xxxx,xxxx,0x87,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,0xbe,xxxx,xxxx,xxxx, /* 80 */
	0xff,xxxx,xxxx,xxxx,0xb5,0x36,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 90 */
	xxxx,xxxx,xxxx,0xc7,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* A0 */
	xxxx,xxxx,xxxx,0x0f,xxxx,0xb7,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* B0 */
	xxxx,0x3a,xxxx,xxxx,xxxx,xxxx,xxxx,0x03, xxxx,0xf8,xxxx,0x59,0xa8,0x5f,xxxx,0xbb, /* C0 */
	0x81,0xfa,0x9d,0xe9,0x2e,xxxx,xxxx,0x33, xxxx,0x78,xxxx,0x0c,xxxx,0x24,0xaa,xxxx, /* D0 */
	xxxx,0xb6,xxxx,0xea,xxxx,0x73,0xe5,0x58, xxxx,0xf7,xxxx,0x74,0x77,xxxx,xxxx,0xa3, /* E0 */
	xxxx,0x5a,0xf6,0x32,0x46,0x2a,xxxx,xxxx, 0x53,0x4b,0x90,xxxx,0x51,xxxx,xxxx,xxxx, /* F0 */
};

//double check 22 (boot bomb at 2a000)
//47a7 (46e0 in boot) - hmm

// 0x00 is NOT 0x20 (no context in bomberman)

unsigned char bomberman_decryption_table[256] = {
	xxxx,xxxx,0x79,xxxx,0x9d,0x48,xxxx,xxxx, xxxx,xxxx,0x2e,xxxx,xxxx,0xa5,0x72,xxxx, /* 00 */
	0x46,0x5b,0xb1,0x3a,0xc3,xxxx,0x35,xxxx, xxxx,0x23,xxxx,0x99,xxxx,0x05,xxxx,0x3c, /* 10 */
	0x3b,0x76,0x11,xxxx,xxxx,0x4b,xxxx,0x92, xxxx,0x32,0x5d,xxxx,0xf7,0x5a,0x9c,xxxx, /* 20 */
	0x26,0x40,0x89,xxxx,xxxx,xxxx,xxxx,0x57, xxxx,xxxx,xxxx,xxxx,xxxx,0xba,0x53,0xbb, /* 30 */
	0x42,0x59,0x2f,xxxx,0x77,xxxx,xxxx,0x4f, 0xbf, 0x4a/*0x41*/, 0xcb,0x86,0x62,0x7d,xxxx,0xb8, /* 40 */
	xxxx, xxxx/*0x34*/, xxxx,0x5f,xxxx,0x7f,0xf8,0x82, 0xa0,0x84,0x12,0x52,xxxx,xxxx,xxxx,0x47, /* 50 */
	xxxx,0x2b,0x88,0xf9,xxxx,0xa3,0x83,xxxx, 0x75,0x87,xxxx,0xab,0xeb,xxxx,0xfe,xxxx, /* 60 */
	xxxx,0xaf,0xd0,0x2c,0xd1,0xe6,0x90,0x43, 0xa2,0xe7,0x85,0xe2,0x49, 0x00, 0x29,xxxx, /* 70 */
	0x7c,xxxx,xxxx,0x9a,xxxx,xxxx,0xb9,xxxx, 0x14,0xcf,0x33,0x02,xxxx,xxxx,xxxx,0x73, /* 80 */
	xxxx,0xc5,xxxx,xxxx,xxxx,0xf3,0xf6,0x24, xxxx,0x56,0xd3,xxxx,0x09,0x01,xxxx,xxxx, /* 90 */
	0x03,0x2d, 0x22, xxxx,0xf5,0xbe,xxxx,xxxx, 0xfb,0x8e,0x21,0x8d,0x0b,xxxx,xxxx,0xb2, /* A0 */
	0xfc,0xfa,0xc6,xxxx,0xe8,0xd2,xxxx,0x08, 0x0a,0xa8,0x78,0xff,xxxx,0xb5,xxxx,xxxx, /* B0 */
	0xc7,0x06,xxxx,xxxx,xxxx,0x1e,0x7e,0xb0, 0x0e,0x0f,xxxx,xxxx,0x0c, 0xaa, 0x55,xxxx, /* C0 */
	xxxx,0x74,0x3d,xxxx,xxxx,0x38,0x27,0x50, xxxx,0xb6,0x5e,0x8b,0x07,0xe5,0x39,0xea, /* D0 */
	0xbd,xxxx,0x81,0xb7,xxxx,0x8a,0x0d,xxxx, 0x58,0xa1,0xa9,0x36,xxxx,0xc4,xxxx,0x8f, /* E0 */
	0x8c,0x1f,0x51,0x04,0xf2,xxxx,0xb3,0xb4, 0xe9,0x2a,xxxx,xxxx,xxxx,0x25,xxxx,0xbc, /* F0 */
};

// a2 is NOT 0x22 (AND) as a jnc follows it (and doesnt affect carry)
// 18b7 - 0x51 0x10 (Not 0x28,
// 1cc0 - 0x49


//1ca6
unsigned char gunforce_decryption_table[256] = {
	xxxx,xxxx,xxxx,0x2c,xxxx,xxxx,0x43,0x88, xxxx,xxxx,0x0a,xxxx,0xba,xxxx,0xea,xxxx, /* 00 */
	xxxx,xxxx,xxxx,xxxx,xxxx,0x00,xxxx,0x0c, xxxx,0x5f,0x9d,xxxx,xxxx,xxxx,xxxx,0xbb, /* 10 */
	0x8a,xxxx,xxxx,xxxx,0x3a,0x3c,0x5a,xxxx, xxxx,xxxx,0xf8,0x89,xxxx,xxxx,xxxx,xxxx, /* 20 */
	xxxx,xxxx,0x73,xxxx,0x59,xxxx,xxxx,xxxx, 0x50,0xfa,xxxx,xxxx,xxxx,xxxx,0x47,0xb7, /* 30 */
	xxxx,xxxx,xxxx,xxxx,xxxx,0x0f,0x8b,xxxx, 0xc3,xxxx,0xbf,xxxx,xxxx,xxxx,xxxx,xxxx, /* 40 */
	xxxx,xxxx,0x24,0xb4,xxxx,xxxx,xxxx,xxxx, 0x5e,0xb6,0x82,xxxx,0x2e,0xab,0xe7,xxxx, /* 50 */
	xxxx,xxxx,xxxx,xxxx,xxxx,0x22,0xc6,xxxx, 0x04,xxxx,xxxx,xxxx,xxxx,0xb0,xxxx,xxxx, /* 60 */
	xxxx,0x46,xxxx,xxxx,xxxx,0xfe,xxxx,0xcf, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0x51, /* 70 */
	xxxx,xxxx,0x36,0x52,0xfb,xxxx,0xb9,xxxx, xxxx,0xb1,xxxx,xxxx,xxxx,0xb5,xxxx,xxxx, /* 80 */
	xxxx,xxxx,0xbe,xxxx,xxxx, 0x4a ,xxxx,0xe5, xxxx,0x58,xxxx,xxxx,0xf3,0x02,xxxx,0xe8, /* 90 */
	xxxx,xxxx,xxxx,0x83,0x56,xxxx,xxxx,0xbc, xxxx,xxxx,xxxx,0x79,0xd0,xxxx,0x2a,xxxx, /* A0 */
	xxxx,0xb8,xxxx,xxxx,xxxx,0x90,xxxx,xxxx, xxxx,0x33,0xf6,xxxx,xxxx,xxxx,xxxx,0x32, /* B0 */
	xxxx,0xa8,0x53,0x26,xxxx,xxxx,0x81,0x75, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0xb2, /* C0 */
	0x57,xxxx,0xa0,xxxx,xxxx,xxxx,xxxx,0x72, xxxx,xxxx,0x42,0x74,0x9c,xxxx,xxxx,0x5b, /* D0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,0xeb,0xa2, xxxx,0xe2,xxxx,xxxx,0x4b,xxxx,xxxx,0x78, /* E0 */
	xxxx,xxxx,xxxx,xxxx,0x03,xxxx,xxxx,xxxx, 0x8e,0xe9,xxxx,xxxx,xxxx,xxxx,0xc7,0x7f, /* F0 */
};
//above - 0xa3 (0x83) total guess
//above - 0x97 (0xe5 IN aw) total guess (5304)
//above - 0x00 (0xc6) guess from hasamu  (5304) - WRONG!
//above - 0x00 (0xa3) guess from hasamu  (5304) - WRONG!
//above - 0x00 (0x8c) guess from hasamu  (5304) - WRONG!

unsigned char rtypeleo_decryption_table[256] = {
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 00 */
	xxxx,xxxx,xxxx,xxxx,0xea,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 10 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 20 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 30 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 40 */
	xxxx,xxxx,xxxx,xxxx,0xcf,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 50 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,0xfa,xxxx,xxxx,xxxx,xxxx,xxxx, /* 60 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 70 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 80 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 90 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* A0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* B0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* C0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* D0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* E0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* F0 */
};

unsigned char dsoccr94_decryption_table[256] = {
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 00 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 10 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 20 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 30 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 40 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 50 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 60 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 70 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 80 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 90 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* A0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* B0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* C0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* D0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* E0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* F0 */
};

unsigned char test_decryption_table[256] = {
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 00 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 10 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 20 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 30 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 40 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 50 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 60 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 70 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 80 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* 90 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* A0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* B0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* C0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* D0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* E0 */
	xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx,xxxx, /* F0 */
};

static unsigned char byte_count_table[256] = {
	2,2,2,2,2,3,1,1, 2,2,2,2,2,3,1,0, /* 00 */
	2,2,2,2,2,3,1,1, 2,2,2,2,2,3,1,1, /* 10 */
	2,2,2,2,2,3,1,1, 2,2,2,2,2,3,1,1, /* 20 */
	2,2,2,2,2,3,1,1, 2,2,2,2,2,3,1,1, /* 30 */
	1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, /* 40 */
	1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, /* 50 */
	1,1,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, /* 60 */
	1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, /* 70 */
	3,3,3,3,2,2,2,2, 2,2,2,2,0,0,0,0, /* 80 */
	1,1,1,1,1,1,1,1, 0,0,0,0,0,0,0,0, /* 90 */
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, /* A0 */
	2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3, /* B0 */
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, /* C0 */
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, /* D0 */
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, /* E0 */
	0,0,0,0,0,0,0,0, 1,1,1,1,1,1,0,0, /* F0 */
};

void irem_cpu_decrypt(int cpu,unsigned char *decryption_table)
{
	int A,diff;
	unsigned char *rom;
	int t[256];
#ifdef MAME_DEBUG
    extern char *opmap1[];
#endif

	rom = memory_region(cpu+REGION_CPU1);
	diff = memory_region_length(cpu+REGION_CPU1) / 2;

	memory_set_opcode_base(cpu,rom+diff);
	for (A = 0;A < 0x100000;A++)
		rom[A + diff] = decryption_table[rom[A]];

	for (A=0; A<256; A++) {
		t[A]=0;
		for (diff=0; diff<256; diff++)
			if (decryption_table[diff]==A) {
				t[A]++;
			}
#ifdef MAME_DEBUG
        if (t[A]==0) logerror("Unused: [%d] %02x\t%s\n",byte_count_table[A],A,opmap1[A]);
        if (t[A]>1) logerror("DUPLICATE: %02x\t%s\n",A,opmap1[A]);
#else
        if (t[A]==0) logerror("Unused: [%d] %02x\n",byte_count_table[A],A);
        if (t[A]>1) logerror("DUPLICATE: %02x\n",A);
#endif
    }
}
