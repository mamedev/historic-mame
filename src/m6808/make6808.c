/* Multi-6808 32 Bit emulator */

/* Copyright 1998, Neil Bradley, All rights reserved
 *
 * (This version has been modified by Alex Pasadyn for use with MAME)
 *
 * License agreement:
 *
 * (M6808 Refers to both the assembly code emitted by make6808.c and make6808.c
 * itself)
 *
 * M6808 May be distributed in unmodified form to any medium.
 *
 * M6808 May not be sold, or sold as a part of a commercial package without
 * the express written permission of Neil Bradley (neil@synthcom.com). This
 * includes shareware.
 *
 * Modified versions of M6808 may not be publicly redistributed without author
 * approval (neil@synthcom.com). This includes distributing via a publicly
 * accessible LAN. You may make your own source modifications and distribute
 * M6808 in source or object form, but if you make modifications to M6808
 * then it should be noted in the top as a comment in make6808.c.
 *
 * M6808 Licensing for commercial applications is available. Please email
 * neil@synthcom.com for details.
 *
 * Synthcom Systems, Inc, and Neil Bradley will not be held responsible for
 * any damage done by the use of M6808. It is purely "as-is".
 *
 * If you use M6808 in a freeware application, credit in the following text:
 *
 * "Multi-6808 CPU emulator by Neil Bradley (neil@synthcom.com)"
 *
 * must accompany the freeware application within the application itself or
 * in the documentation.
 *
 * Legal stuff aside:
 *
 * If you find problems with M6808, please email the author so they can get
 * resolved. If you find a bug and fix it, please also email the author so
 * that those bug fixes can be propogated to the installed base of M6808
 * users. If you find performance improvements or problems with M6808, please
 * email the author with your changes/suggestions and they will be rolled in
 * with subsequent releases of M6808.
 *
 * The whole idea of this emulator is to have the fastest available 32 bit
 * Multi-6808 emulator for the PC, giving maximum performance.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define	VERSION 	"1.0"

#define TRUE            0xffff
#define FALSE           0x0
#define	INVALID	0xff

#define UINT32          unsigned long int
#define UINT16          unsigned short int
#define UINT8                   unsigned char

#define CPU_M6808	1
#define CPU_M6803	2|CPU_M6808
#define CPU_HD63701	4|CPU_M6803|CPU_M6808


FILE *fp = NULL;
char string[150];
char basename[150];
char procname[150];
char vcyclesRemaining[150];
char fGetContext[150];
char fSetContext[150];
char fReset[150];
char fExec[150];
UINT32 dwGlobalLabel = 0;
UINT32 dwAnotherLabel = 0;
UINT8 bUseStack = FALSE;			// Use stack calling conventions
UINT8 bZeroDirect = FALSE;			// Zero page direct
UINT8 bSingleStep = FALSE;			// Here if we want to single step the CPU
UINT8 bMameFormat = FALSE;			// Here if we want to support MAME interface
UINT8 bCPUFlag = CPU_M6808;				// If we want to support extra instructions
UINT32 dwLoop = 0;

void ReadMemoryByte(UINT8 *pszTarget, UINT8 *pszAddress);
void ReadMemoryWord(UINT8 *pszTarget, UINT8 *pszAddress);
void WriteMemoryByte(UINT8 *pszTarget, UINT8 *pszAddress);


struct sOp
{
	UINT16 bOpCode;
	void (*Emitter)(UINT16);
        UINT8 bCPUFlag;
};

void ReadMemoryByte(UINT8 *pszTarget, UINT8 *pszAddress);

void SecSeiSevClcCliClvHandler(UINT16 dwOpcode);
void LdsLdx(UINT16 dwOpcode);
void LddHandler(UINT16 dwOpcode);
void ClrHandler(UINT16 dwOpcode);
void ComHandler(UINT16 dwOpcode);
void AimHandler(UINT16 dwOpcode);
void LdaHandler(UINT16 dwOpcode);
void LdbHandler(UINT16 dwOpcode);
void StaHandler(UINT16 dwOpcode);
void StbHandler(UINT16 dwOpcode);
void StxHandler(UINT16 dwOpcode);
void StdHandler(UINT16 dwOpcode);
void BraHandler(UINT16 dwOpcode);
void InxDexHandler(UINT16 dwOpcode);
void AndBitHandler(UINT16 dwOpcode);
void CmpHandler(UINT16 dwOpcode);
void TstHandler(UINT16 dwOpcode);
void SWIHandler(UINT16 dwOpcode);
void DesHandler(UINT16 dwOpcode);
void DecHandler(UINT16 dwOpcode);
void InsHandler(UINT16 dwOpcode);
void IncHandler(UINT16 dwOpcode);
void JsrBsrHandler(UINT16 dwOpcode);
void RtsHandler(UINT16 dwOpcode);
void TbaTabTpaTapHandler(UINT16 dwOpcode);
void AsxLsrRoxHandler(UINT16 dwOpcode);
void AsldLsrdHandler(UINT16 dwOpcode);
void AbaHandler(UINT16 dwOpcode);
void AddHandler(UINT16 dwOpcode);
void AdddHandler(UINT16 dwOpcode);
void SubdHandler(UINT16 dwOpcode);
void PulPshHandler(UINT16 dwOpcode);
void SbaCbaHandler(UINT16 dwOpcode);
void CmpxHandler(UINT16 dwOpcode);
void SubHandler(UINT16 dwOpcode);
void JmpHandler(UINT16 dwOpcode);
void EorHandler(UINT16 dwOpcode);
void SbcHandler(UINT16 dwOpcode);
void AdcHandler(UINT16 dwOpcode);
void OrHandler(UINT16 dwOpcode);
void NegHandler(UINT16 dwOpcode);
void TsxTxsHandler(UINT16 dwOpcode);
void NopHandler(UINT16 dwOpcode);
void DaaHandler(UINT16 dwOpcode);
void AbxHandler(UINT16 dwOpcode);
void WaiHandler(UINT16 dwOpcode);
void RtiHandler(UINT16 dwOpcode);
void MulHandler(UINT16 dwOpcode);
void XgdxHandler(UINT16 dwOpcode);

struct sOp StandardOps[] =
{
	{0x01,	NopHandler},							// VF
	{0x5e,	NopHandler},							// VF
	{0x04,	AsldLsrdHandler, CPU_M6803},
	{0x05,	AsldLsrdHandler, CPU_M6803},
	{0x06,	TbaTabTpaTapHandler},				// VF
	{0x07,	TbaTabTpaTapHandler},				// VF
	{0x08,	InxDexHandler},						// VF
	{0x09,	InxDexHandler},						// VF
	{0x0a,	SecSeiSevClcCliClvHandler},		// VF
	{0x0b,	SecSeiSevClcCliClvHandler},		// VF
	{0x0c,	SecSeiSevClcCliClvHandler},		// VF
	{0x0d,	SecSeiSevClcCliClvHandler},		// VF
	{0x0e,	SecSeiSevClcCliClvHandler},		// VF
	{0x0f,	SecSeiSevClcCliClvHandler},		// VF

	{0x10,	SbaCbaHandler},						// VF
	{0x11,	SbaCbaHandler},						// VF
	{0x16,	TbaTabTpaTapHandler},				// VF
	{0x17,	TbaTabTpaTapHandler},				// VF
	{0x18,	XgdxHandler, CPU_HD63701},
	{0x19,	DaaHandler},							// VF
	{0x1b,	AbaHandler},							// VF

	{0x20,	BraHandler},							// VF
	{0x22,	BraHandler},							// VF
	{0x23,	BraHandler},							// VF
	{0x24,	BraHandler},							// VF
	{0x25,	BraHandler},							// VF
	{0x26,	BraHandler},							// VF
	{0x27,	BraHandler},							// VF
	{0x28,	BraHandler},							// VF
	{0x29,	BraHandler},							// VF
	{0x2a,	BraHandler},							// VF
	{0x2b,	BraHandler},							// VF
	{0x2c,	BraHandler},							// VF
	{0x2d,	BraHandler},							// VF
	{0x2e,	BraHandler},							// VF
	{0x2f,	BraHandler},							// VF

	{0x30,	TsxTxsHandler},						//	VF
	{0x31,	InsHandler},
	{0x32,	PulPshHandler},						// VF
	{0x33,	PulPshHandler},						// VF
	{0x34,	DesHandler},							// VF
	{0x35,	TsxTxsHandler},						// VF
	{0x36,	PulPshHandler},						// VF
	{0x37,	PulPshHandler},						// VF
	{0x38,	PulPshHandler, CPU_M6803},
	{0x39,	RtsHandler},							// VF
	{0x3a,	AbxHandler},							// VF
	{0x3b,	RtiHandler},		// Todo
	{0x3c,	PulPshHandler, CPU_M6803},
	{0x3d,	MulHandler, CPU_M6803},
	{0x3e,	WaiHandler},							// VF
	{0x3f,	SWIHandler},							// VF

	{0x40,  NegHandler},								// VF
	{0x43,  ComHandler},								// VF
	{0x44,	AsxLsrRoxHandler},					// VF
	{0x46,	AsxLsrRoxHandler},					// VF
	{0x47,	AsxLsrRoxHandler},					// VF
	{0x48,	AsxLsrRoxHandler},					// VF
	{0x49,	AsxLsrRoxHandler},					// VF
	{0x4a,	DecHandler},							// VF
	{0x4c,	IncHandler},							// VF
	{0x4d,	TstHandler},							// VF
	{0x4f,	ClrHandler},							// VF

	{0x50,  NegHandler},								// VF
	{0x53,  ComHandler},								// VF
	{0x54,	AsxLsrRoxHandler},					// VF
	{0x56,	AsxLsrRoxHandler},					// VF
	{0x57,	AsxLsrRoxHandler},					// VF
	{0x58,	AsxLsrRoxHandler},					// VF
	{0x59,	AsxLsrRoxHandler},					// VF
	{0x5a,	DecHandler},							// VF
	{0x5c,	IncHandler},							// VF
	{0x5d,	TstHandler},							// VF
	{0x5f,	ClrHandler},							// VF

	{0x60,  NegHandler},								// VF
	{0x61,	AimHandler,  CPU_HD63701},
	{0x62,	AimHandler,  CPU_HD63701},
	{0x63,  ComHandler},								// VF
	{0x64,	AsxLsrRoxHandler},					// VF
	{0x66,	AsxLsrRoxHandler},					// VF
	{0x67,	AsxLsrRoxHandler},					// VF
	{0x68,	AsxLsrRoxHandler},					// VF
	{0x69,	AsxLsrRoxHandler},					// VF
	{0x6a,	DecHandler},							// VF
	{0x6c,	IncHandler},							// VF
	{0x6d,	TstHandler},							// VF
	{0x6e,	JmpHandler},							// VF
	{0x6f,	ClrHandler},							// VF

	{0x70,  NegHandler},								// VF
	{0x71,	AimHandler,  CPU_HD63701},
	{0x72,	AimHandler,  CPU_HD63701},
	{0x73,  ComHandler},								// VF
	{0x74,	AsxLsrRoxHandler},					// VF
	{0x76,	AsxLsrRoxHandler},					// VF
	{0x77,	AsxLsrRoxHandler},					// VF
	{0x78,	AsxLsrRoxHandler},					// VF
	{0x79,	AsxLsrRoxHandler},					// VF
	{0x7a,	DecHandler},							// VF
	{0x7c,	IncHandler},							// VF
	{0x7d,	TstHandler},							// VF
	{0x7e,	JmpHandler},							// VF
	{0x7f,	ClrHandler},							// VF

	{0x80,	SubHandler},							// VF
	{0x81,	CmpHandler},							// VF
	{0x82,	SbcHandler},							// VF
	{0x84,	AndBitHandler},						// VF
	{0x83,	SubdHandler, CPU_M6803},
	{0x85,	AndBitHandler},						// VF
	{0x86,	LdaHandler},							// VF
	{0x88,	EorHandler},							// VF
	{0x89,	AdcHandler},							// VF
	{0x8a,	OrHandler},								// VF
	{0x8b,	AddHandler},							// VF
	{0x8c,	CmpxHandler},							// VF
	{0x8d,	JsrBsrHandler},						// VF
	{0x8e,	LdsLdx},									// VF

	{0x90,	SubHandler},							// VF
	{0x91,	CmpHandler},							// VF
	{0x92,	SbcHandler},							// VF
	{0x93,	SubdHandler, CPU_M6803},
	{0x94,	AndBitHandler},						// VF
	{0x95,	AndBitHandler},						// VF
	{0x96,	LdaHandler},							// VF
	{0x97,	StaHandler},							// VF
	{0x98,	EorHandler},							// VF
	{0x99,	AdcHandler},							// VF
	{0x9a,	OrHandler},								// VF
	{0x9b,	AddHandler},							// VF
	{0x9c,	CmpxHandler},							// VF
//	{0x9d,	JsrBsrHandler},		// Vaild instruction???
	{0x9e,	LdsLdx},									// VF

	{0xa0,	SubHandler},							// VF
	{0xa1,	CmpHandler},							// VF
	{0xa2,	SbcHandler},							// VF
	{0xa3,	SubdHandler, CPU_M6803},
	{0xa4,	AndBitHandler},						// VF
	{0xa5,	AndBitHandler},						// VF
	{0xa6,	LdaHandler},							// VF
	{0xa7,	StaHandler},							// VF
	{0xa8,	EorHandler},							// VF
	{0xa9,	AdcHandler},							// VF
	{0xaa,	OrHandler},								// VF
	{0xab,	AddHandler},							// VF
	{0xac,	CmpxHandler},							// VF
	{0xad,	JsrBsrHandler},						// VF
	{0xae,	LdsLdx},									// VF

	{0xb0,	SubHandler},							// VF
	{0xb1,	CmpHandler},							// VF
	{0xb2,	SbcHandler},							// VF
	{0xb3,	SubdHandler, CPU_M6803},
	{0xb4,	AndBitHandler},						// VF
	{0xb5,	AndBitHandler},						// VF
	{0xb6,	LdaHandler},							// VF
	{0xb7,	StaHandler},							// VF
	{0xb8,	EorHandler},							// VF
	{0xb9,	AdcHandler},							// VF
	{0xba,	OrHandler},								// VF
	{0xbb,	AddHandler},							// VF
	{0xbc,	CmpxHandler},							// VF
	{0xbd,	JsrBsrHandler},						// VF
	{0xbe,	LdsLdx},									// VF

	{0xc0,	SubHandler},							// VF
	{0xc1,	CmpHandler},							// VF
	{0xc2,	SbcHandler},							// VF
	{0xc3,	AdddHandler, CPU_M6803},
	{0xc4,	AndBitHandler},						// VF
	{0xc5,	AndBitHandler},						// VF
	{0xc6,	LdbHandler},							// VF
	{0xc8,	EorHandler},							// VF
	{0xc9,	AdcHandler},							// VF
	{0xca,	OrHandler},								// VF
	{0xcb,	AddHandler},							// VF
	{0xcc,	LddHandler, CPU_M6803},
	{0xce,	LdsLdx},									// VF

	{0xd0,	SubHandler},							// VF
	{0xd1,	CmpHandler},							// VF
	{0xd2,	SbcHandler},							// VF
	{0xd3,	AdddHandler, CPU_M6803},
	{0xd4,	AndBitHandler},						// VF
	{0xd5,	AndBitHandler},						// VF
	{0xd6,	LdbHandler},							// VF
	{0xd7,	StbHandler},							// VF
	{0xd8,	EorHandler},							// VF
	{0xd9,	AdcHandler},							// VF
	{0xda,	OrHandler},								// VF
	{0xdb,	AddHandler},							// VF
	{0xdc,	LddHandler, CPU_M6803},
	{0xdd,	StdHandler, CPU_M6803},
	{0xde,	LdsLdx},									// VF
	{0xdf,	StxHandler},							// VF

	{0xe0,	SubHandler},							// VF
	{0xe1,	CmpHandler},							// VF
	{0xe2,	SbcHandler},							// VF
	{0xe3,	AdddHandler, CPU_M6803},
	{0xe4,	AndBitHandler},						// VF
	{0xe5,	AndBitHandler},						// VF
	{0xe6,	LdbHandler},							// VF
	{0xe7,	StbHandler},							// VF
	{0xe8,	EorHandler},							// VF
	{0xe9,	AdcHandler},							// VF
	{0xea,	OrHandler},								// VF
	{0xeb,	AddHandler},							// VF
	{0xec,	LddHandler, CPU_M6803},
	{0xed,	StdHandler, CPU_M6803},
	{0xee,	LdsLdx},									// VF
	{0xef,	StxHandler},							// VF

	{0xf0,	SubHandler},							// VF
	{0xf1,	CmpHandler},							// VF
	{0xf2,	SbcHandler},							// VF
	{0xf3,	AdddHandler, CPU_M6803},
	{0xf4,	AndBitHandler},						// VF
	{0xf5,	AndBitHandler},						// VF
	{0xf6,	LdbHandler},							// VF
	{0xf7,	StbHandler},							// VF
	{0xf8,	EorHandler},							// VF
	{0xf9,	AdcHandler},							// VF
	{0xfa,	OrHandler},								// VF
	{0xfb,	AddHandler},							// VF
	{0xfc,	LddHandler, CPU_M6803},
	{0xfd,	StdHandler, CPU_M6803},
	{0xfe,	LdsLdx},									// VF
	{0xff,	StxHandler},							// VF

	{0xffff, NULL}
};

//changed 0x61, 0x62, 0x71, 0x72, 0xdd, 0xed, 0xfd, 0xcc, 0xdc, 0xfc, 0x83, 0x93, 0xa3, 0xb3, 0xc3, 0xd3, 0xe3, 0xf3, 0x38, 0x3c, 0x04, 0x05, 0x3d to have timing 1

UINT8 bTimingTable[256] =
{
 0x0, 0x2, 0x0, 0x0, 0x1, 0x1, 0x2, 0x2, 0x4, 0x4, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2,
 0x2, 0x2, 0x0, 0x0, 0x0, 0x0, 0x2, 0x2, 0x1, 0x2, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0,
 0x4, 0x0, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4,
 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x1, 0x5, 0x5, 0xa, 0x1, 0x1, 0x9, 0xc,
 0x2, 0x0, 0x0, 0x2, 0x2, 0x0, 0x2, 0x2, 0x2, 0x2, 0x2, 0x0, 0x2, 0x2, 0x0, 0x2,
 0x2, 0x0, 0x0, 0x2, 0x2, 0x0, 0x2, 0x2, 0x2, 0x2, 0x2, 0x0, 0x2, 0x2, 0x2, 0x2,
 0x7, 0x1, 0x1, 0x7, 0x7, 0x0, 0x7, 0x7, 0x7, 0x7, 0x7, 0x0, 0x7, 0x7, 0x4, 0x7,
 0x6, 0x1, 0x1, 0x6, 0x6, 0x0, 0x6, 0x6, 0x6, 0x6, 0x6, 0x0, 0x6, 0x6, 0x3, 0x6,
 0x2, 0x2, 0x2, 0x1, 0x2, 0x2, 0x2, 0x0, 0x2, 0x2, 0x2, 0x2, 0x3, 0x8, 0x3, 0x0,
 0x3, 0x3, 0x3, 0x1, 0x3, 0x3, 0x3, 0x4, 0x3, 0x3, 0x3, 0x3, 0x4, 0x0, 0x4, 0x5,
 0x5, 0x5, 0x5, 0x1, 0x5, 0x5, 0x5, 0x6, 0x5, 0x5, 0x5, 0x5, 0x6, 0x8, 0x6, 0x7,
 0x4, 0x4, 0x4, 0x1, 0x4, 0x4, 0x4, 0x5, 0x4, 0x4, 0x4, 0x4, 0x5, 0x9, 0x5, 0x6,
 0x2, 0x2, 0x2, 0x1, 0x2, 0x2, 0x2, 0x0, 0x2, 0x2, 0x2, 0x2, 0x1, 0x0, 0x3, 0x0,
 0x3, 0x3, 0x3, 0x1, 0x3, 0x3, 0x3, 0x4, 0x3, 0x3, 0x3, 0x3, 0x1, 0x1, 0x4, 0x5,
 0x5, 0x5, 0x5, 0x1, 0x5, 0x5, 0x5, 0x6, 0x5, 0x5, 0x5, 0x5, 0x4, 0x1, 0x6, 0x7,
 0x4, 0x4, 0x4, 0x1, 0x4, 0x4, 0x4, 0x5, 0x4, 0x4, 0x4, 0x4, 0x1, 0x1, 0x5, 0x6
};

UINT8 bBit6808tox86[8] = {0, 0xff, 6, 7, 0xff, 4, 0xff, 0xff};
UINT8 bBitx86to6808[8] = {0, 0xff, 0xff, 0xff, 5, 0xff, 2, 3};

Flags6808toX86()
{
	fprintf(fp, "		xor	edx, edx\n");
	fprintf(fp, "		mov	dl, ah\n");
	fprintf(fp, "		mov	[ds:_altFlags], dl\n");
	fprintf(fp, "		and	[ds:_altFlags], byte 12h;\n");
	fprintf(fp, "		mov	ah, [ds:bit6808tox86+edx]\n");
}

FlagsX86to6808()
{
	fprintf(fp, "		xor	edx, edx\n");
	fprintf(fp, "		mov	dl, ah\n");
	fprintf(fp, "		mov	ah, [ds:bitx86to6808+edx]\n");
	fprintf(fp, "		or		ah, [ds:_altFlags]\n");
}

SetZeroSign(UINT8 *pszRegister)
{
	if(strcmp(pszRegister,"dx") && strcmp(pszRegister,"dl"))
	{
		fprintf(fp, "		mov	dl, ah		; Save flags\n");
		fprintf(fp, "		or	%s, %s		; OR our value\n", pszRegister, pszRegister);
		fprintf(fp, "		lahf\n");
		fprintf(fp, "		and	dl, 03fh	; Knock out sign and zero\n");
		fprintf(fp, "		and	ah, 0c0h	; Only sign & zero now\n");
		fprintf(fp, "		or	ah, dl	; OR In our new flags with the old\n");
	}
	else
	{
		fprintf(fp, "		mov	bh, ah		; Save flags\n");
		fprintf(fp, "		or	%s, %s		; OR our value\n", pszRegister, pszRegister);
		fprintf(fp, "		lahf\n");
		fprintf(fp, "		and	bh, 03fh	; Knock out sign and zero\n");
		fprintf(fp, "		and	ah, 0c0h	; Only sign & zero now\n");
		fprintf(fp, "		or	ah, bh		; OR In our new flags with the old\n");
		fprintf(fp, "		xor	bh, bh		; Clear out BH\n");
	}
}

// Word must be in DX because of the endian swap...
MamePushWord()
{
	fprintf(fp, "		xor	edi, edi		; Clear edi\n");
	fprintf(fp, "		mov	di, dx		; Load data to write\n", basename);
	fprintf(fp, "		xor	edx, edx		; Clear edx\n");
	fprintf(fp, "		mov	dx, [ds:_%ssp]		; Load SP into edx\n", basename);

	fprintf(fp, "		push	edx\n");

	fprintf(fp, "		xchg	dx, di\n");
	fprintf(fp, "		mov	bh, dl\n");
	fprintf(fp, "		xchg	dx, di\n");
	WriteMemoryByte("bh", "dx");

	fprintf(fp, "		pop	edx\n");

	fprintf(fp, "		dec	word [ds:_%ssp]\n", basename);
	fprintf(fp, "		dec	dx\n");

	fprintf(fp, "		xchg	dx, di\n");
	fprintf(fp, "		mov	bh, dh\n");
	fprintf(fp, "		xchg	dx, di\n");
	WriteMemoryByte("bh", "dx");

	fprintf(fp, "		dec	word [ds:_%ssp]\n", basename);
	fprintf(fp, "		xor	edx, edx\n");

}

// Word must be in DX because of the endian swap...
StdPushWord()
{
	fprintf(fp, "		xor	edi, edi		; Clear edi\n");
	fprintf(fp, "		mov	di, [ds:_%ssp]		; Load SP into edi\n", basename);

	fprintf(fp, "		dec	edi\n");
	fprintf(fp, "		xchg dh, dl			; Swap bytes\n");
	fprintf(fp, "		mov	[ds:edi+ebp], dx	; Write it out\n");
	fprintf(fp, "		dec	edi\n");

	fprintf(fp, "		mov	[ds:_%ssp], di \n", basename);
}

PushWord()
{
	if (FALSE != bMameFormat) MamePushWord();
	else StdPushWord();
}

// Pops the word into DX only because of the endian swap...
MamePopWord()
{
	fprintf(fp, "		xor	edx, edx		; Clear edx\n");
	fprintf(fp, "		inc	word [ds:_%ssp]\n", basename);
	fprintf(fp, "		mov	dx, [ds:_%ssp]		; Load SP into edx\n", basename);

	ReadMemoryWord("dx", "dx");
	fprintf(fp, "		inc	word [ds:_%ssp]\n", basename);
}

StdPopWord()
{
	fprintf(fp, "		xor	edi, edi		; Clear edi\n");
	fprintf(fp, "		mov	di, [ds:_%ssp]		; Load SP into edi\n", basename);

	fprintf(fp, "		inc	edi\n");
	fprintf(fp, "		mov	dx, [ds:edi+ebp]	; Read it in\n");
	fprintf(fp, "		xchg	dh, dl		; Swap bytes\n");
	fprintf(fp, "		inc	edi\n");

	fprintf(fp, "		mov	[ds:_%ssp], di \n", basename);
}

PopWord()
{
	if (FALSE != bMameFormat) MamePopWord();
	else StdPopWord();
}

MamePushByte(UINT8 *pszRegister)
{
	fprintf(fp, "		xor	edx, edx		; Clear edx\n");
	fprintf(fp, "		mov	dx, [ds:_%ssp]		; Load SP into edx\n", basename);
	WriteMemoryByte(pszRegister, "dx");
	fprintf(fp, "		dec	word [ds:_%ssp]\n", basename);
	fprintf(fp, "		xor	edx, edx		; Clear edx\n");
}

StdPushByte(UINT8 *pszRegister)
{
	fprintf(fp, "		xor	edi, edi		; Clear edi\n");
	fprintf(fp, "		mov	di, [ds:_%ssp]		; Load SP into edi\n", basename);
	fprintf(fp, "		mov	[ds:edi+ebp], %s	; Write it out\n",pszRegister);
	fprintf(fp, "		dec	edi\n");
	fprintf(fp, "		mov	[ds:_%ssp], di \n", basename);
}

PushByte(UINT8 *pszRegister)
{
	if (FALSE != bMameFormat) MamePushByte(pszRegister);
	else StdPushByte(pszRegister);
}

MamePopByte(UINT8 *pszRegister)
{
	fprintf(fp, "		xor	edx, edx		; Clear edx\n");
	fprintf(fp, "		inc	word [ds:_%ssp]\n", basename);
	fprintf(fp, "		mov	dx, [ds:_%ssp]		; Load SP into edx\n", basename);
	ReadMemoryByte(pszRegister, "dx");
	fprintf(fp, "		xor	edx, edx		; Clear edx\n");
}

StdPopByte(UINT8 *pszRegister)
{
	fprintf(fp, "		xor	edi, edi		; Clear edi\n");
	fprintf(fp, "		mov	di, [ds:_%ssp]		; Load SP into edi\n", basename);
	fprintf(fp, "		inc	edi\n");
	fprintf(fp, "		mov	%s, [ds:edi+ebp]	; Write it in to our register\n",pszRegister);
	fprintf(fp, "		mov	[ds:_%ssp], di \n", basename);
}

PopByte(UINT8 *pszRegister)
{
	if (FALSE != bMameFormat) MamePopByte(pszRegister);
	else StdPopByte(pszRegister);
}

SetOverflow()
{
	fprintf(fp, "		xchg	di, dx	; Save DX for later\n");
	fprintf(fp, "		o16		pushf\n");
	fprintf(fp, "		pop		dx\n");
	fprintf(fp, "		and		byte [ds:_altFlags], byte 0fdh	; Knock out overflow\n");
	fprintf(fp, "		and		dh, 08h ; Just the overflow\n");
	fprintf(fp, "		shr		dh, 2	; Shift it into position\n");
	fprintf(fp, "		or		byte [ds:_altFlags], dh	; OR It in with the real flags\n");
	fprintf(fp, "		xchg	di, dx	; Restore DX now\n");
}

SetOverflowCarry()
{
	fprintf(fp, "		xchg	di, dx\n");
	fprintf(fp, "		o16		pushf\n");
	fprintf(fp, "		and		[ds:_altFlags], byte 0fdh	; Knock out overflow\n");
	fprintf(fp, "		pop		dx\n");
	fprintf(fp, "		and		ah, 0feh			; Knock out carry\n");
	fprintf(fp, "		and		dh, 08h ; Just the overflow\n");
	fprintf(fp, "		shr		dh, 2	; Shift it into position\n");
	fprintf(fp, "		or		byte [ds:_altFlags], dh	; OR It in with the real flags\n");
	fprintf(fp, "		and		dl, 01h				; Just the carry flag\n");
	fprintf(fp, "		or		ah, dl				; OR it in with the real flags\n");
	fprintf(fp, "		xchg	di, dx\n");
}

SetOverflowCarryZeroSign()
{
	fprintf(fp, "		xchg	di, dx\n");
	fprintf(fp, "		o16		pushf\n");
	fprintf(fp, "		and		[ds:_altFlags], byte 0fdh	; Knock out overflow\n");
	fprintf(fp, "		and		ah, 03eh			; Knock out carry, zero and sign\n");
	fprintf(fp, "		pop		dx\n");
	fprintf(fp, "		and		dh, 08h ; Just the overflow\n");
	fprintf(fp, "		shr		dh, 2	; Shift it into position\n");
	fprintf(fp, "		or		byte [ds:_altFlags], dh	; OR It in with the real flags\n");
	fprintf(fp, "		and		dl, 0c1h				; Just the sign, zero and carry flag\n");
	fprintf(fp, "		or		ah, dl				; OR it in with the real flags\n");
	fprintf(fp, "		xchg	di, dx\n");
}

SetCarry()
{
	fprintf(fp, "		xchg	di, dx\n");
	fprintf(fp, "		o16		pushf\n");
	fprintf(fp, "		and		ah, 0feh			; Knock out carry\n");
	fprintf(fp, "		pop		dx\n");
	fprintf(fp, "		and		dl, 01h				; Just the carry flag\n");
	fprintf(fp, "		or		ah, dl				; OR it in with the real flags\n");
	fprintf(fp, "		xchg	di, dx\n");
}

SetOverflowCarryHalf()			// This is getting ridiculous!!! :-)
{
	fprintf(fp, "		xchg	di, dx\n");
	fprintf(fp, "		o16		pushf\n");
	fprintf(fp, "		and		[ds:_altFlags], byte 0fdh	; Knock out overflow\n");
	fprintf(fp, "		and		ah, 0eeh			; Knock out both carrys\n");
	fprintf(fp, "		pop		dx\n");
	fprintf(fp, "		push	dx\n");
	fprintf(fp, "		and		dh, 08h ; Just the overflow\n");
	fprintf(fp, "		shr		dh, 2	; Shift it into position\n");
	fprintf(fp, "		or		byte [ds:_altFlags], dh	; OR It in with the real flags\n");
	fprintf(fp, "		and		dl, 01h				; Just the carry flag\n");
	fprintf(fp, "		or		ah, dl				; OR it in with the real flags\n");
	fprintf(fp, "		pop		dx\n");
	fprintf(fp, "		and		dl, 10h				; Just the aux-carry flag\n");
	fprintf(fp, "		or		ah, dl				; OR it in with the real flags\n");
	fprintf(fp, "		xchg	di, dx\n");
}

Immediate16()
{
	fprintf(fp, "		mov	dx, [esi]	; Get our word\n");
	fprintf(fp, "		add	esi, 2		; Increment our pointer past the word\n");
	fprintf(fp, "		xchg	dh, dl	; Swap 'em\n");
}

Immediate8()
{
	fprintf(fp, "		mov	dl, [esi]	; Get our word\n");
	fprintf(fp, "		inc	esi\n");
}

Indexed()
{
	fprintf(fp, "		mov	dl, [esi]	; Get our byte\n");
	fprintf(fp, "		inc	esi	; Next opcode please\n");
	fprintf(fp, "		add	dx, cx	; Add our index\n");
}

Direct()
{
	fprintf(fp, "		xor	edx, edx\n");
	fprintf(fp, "		mov	dl, [esi]	; Get our direct address\n");
	fprintf(fp, "		inc	esi	; Next opcode\n");
}

Extended()
{
	fprintf(fp, "		xor	edx, edx\n");
	fprintf(fp, "		mov	dx, [esi]	; Get our extended address\n");
	fprintf(fp, "		xchg	dh, dl\n");
	fprintf(fp, "		add	esi, 2	; Skip past our address\n");
}

// THESE ARE THE VARIOUS DRIVING AGENTS FOR THE ASSEMBLER SPECIFIC AREAS

StandardHeader()
{
	fprintf(fp,"; For assembly by NASM only\n");
	fprintf(fp,"bits 32\n");

	fprintf(fp, ";\n; m6808 - V%s - Copyright 1998, Neil Bradley (neil@synthcom.com)\n", VERSION);
	fprintf(fp, ";\n; Modified by Alex Pasadyn for use with MAME\n;\n\n");

	if (bUseStack)
		fprintf(fp, "; Using stack calling conventions\n");
	else
		fprintf(fp, "; Using register calling conventions\n");

	if (bSingleStep)
		fprintf(fp, "; Single step version (debug)\n");

	fprintf(fp, "\n\n");
}

Alignment()
{
	fprintf(fp, "\ntimes ($$-$) & 3 nop	; pad with NOPs to 4-byte boundary\n\n");
}

ProcBegin(UINT32 dwOpcode)
{
	Alignment();
	fprintf(fp, "%s:\n", procname);

#if 0

	fprintf(fp, "		push	eax\n");
	fprintf(fp, "		push	edx\n");
	fprintf(fp, "		push	ebx\n");

	fprintf(fp, "		mov	ebx, 0b0000h\n");
	fprintf(fp, "		mov	al, dl\n");
	fprintf(fp, "		and	al, 0f0h\n");
	fprintf(fp, "		shr	al, 4\n");

	fprintf(fp, "		cmp	al, 9\n");
	fprintf(fp, "		jbe	noHose%ld\n", dwGlobalLabel);
	fprintf(fp, "		add	al, 7\n");
	fprintf(fp, "noHose%ld:\n", dwGlobalLabel++);
	fprintf(fp, "		add	al, '0'\n");
	fprintf(fp, "		mov	[ds:ebx], al\n");

	fprintf(fp, "		mov	al, dl\n");
	fprintf(fp, "		and	al, 0fh\n");
	fprintf(fp, "		cmp	al, 9\n");
	fprintf(fp, "		jbe	noHose%ld\n", dwGlobalLabel);
	fprintf(fp, "		add	al, 7\n");
	fprintf(fp, "noHose%ld:\n", dwGlobalLabel++);
	fprintf(fp, "		add	al, '0'\n");
	fprintf(fp, "		mov	[ds:ebx+2], al\n");

	fprintf(fp, "		pop	ebx\n");
	fprintf(fp, "		pop	edx\n");
	fprintf(fp, "		pop	eax\n");

#endif

	if (0xffffffff != dwOpcode)
	{
		// The timing on this instruction had better not be 0!
		assert(dwOpcode < 0x100);
		if(!bTimingTable[dwOpcode])
		{
			printf("%2x\n",dwOpcode);
		}
		assert(bTimingTable[dwOpcode]);


		if (FALSE == bMameFormat)
		{

        		if (FALSE == bSingleStep)
			{
			        fprintf(fp, "		sub	dword [%s], %ld\n", vcyclesRemaining, (UINT32) bTimingTable[dwOpcode]);
				fprintf(fp, "		jc	near noMoreExec	; Can't execute anymore!\n");
			}
			else
			{
			        fprintf(fp, "		dec	dword [%s]	; Single stepping (debugger)\n", vcyclesRemaining);
				fprintf(fp, "		jz	near noMoreExec ; No more code!\n", procname);
			}

			fprintf(fp, "		add	dword [dwElapsedTicks], %ld\n", bTimingTable[dwOpcode]);
		}
	}
}

ProcEnd(char *procname)
{
}

void ReadMemoryByteHandler()
{
	Alignment();
	fprintf(fp, "; This is a generic read memory byte handler when a foreign\n");
	fprintf(fp, "; handler is to be called\n\n");
	fprintf(fp, "ReadMemoryByte:\n");
	fprintf(fp, "		mov	[_%saf], ax	; Save Accumulator & flags\n", basename);
	fprintf(fp, "		mov	[_%sb], bl	; And B\n", basename);
	fprintf(fp, "		mov	[_%sx], cx	; And our index register\n", basename);
	fprintf(fp, "		sub	esi, ebp	; Our program counter\n", basename);
	fprintf(fp, "		mov	[_%spc], si	; Save our program counter\n", basename);

	if (bUseStack)
	{
		fprintf(fp, "		push	edi	; Save our structure address\n");
		fprintf(fp, "		and	edx, 0ffffh	; Only the lower 16 bits\n");
		fprintf(fp, "		push	edx	; And our desired address\n");
	}
	else
	{
		fprintf(fp, "		mov	eax, edx	; Get our desired address reg\n");
		fprintf(fp, "		mov	edx, edi	; Pointer to the structure\n");
	}

	if (FALSE == bMameFormat)
	fprintf(fp, "		call	dword [edi + 8]	; Go call our handler\n");
	else
	fprintf(fp, "		call	_cpu_readmem16	; Go call our handler\n");

	if (bUseStack)
		fprintf(fp, "		add	esp, 8	; Get the junk off the stack\n");

	fprintf(fp, "		xor	ebx, ebx	; Zero B\n");
	fprintf(fp, "		xor	ecx, ecx	; Zero index\n");
	fprintf(fp, "		xor	edx, edx	; Zero scratch register\n");
	fprintf(fp, "		xor	esi, esi	; Zero it!\n");
	fprintf(fp, "		mov	si, [ds:_%spc]	; Get our program counter back\n", basename);
	fprintf(fp, "		mov	ebp, [ds:_%sBase] ; Base pointer comes back\n", basename);
	fprintf(fp, "		add	esi, ebp	; Rebase it properly\n");

	fprintf(fp, "		ret\n\n");
}

void ReadMemoryByte(UINT8 *pszTarget, UINT8 *pszAddress)
{
        if (FALSE == bMameFormat)
	{
	        fprintf(fp, "		mov	edi, [_%sMemRead]	; Point to the read array\n\n", basename);
		fprintf(fp, "checkLoop%ld:\n", dwGlobalLabel);
		fprintf(fp, "		cmp	[edi], word 0ffffh ; End of the list?\n");
		fprintf(fp, "		je		memoryRead%ld\n", dwGlobalLabel);
		fprintf(fp, "		cmp	%s, [edi]	; Are we smaller?\n", pszAddress);
		fprintf(fp, "		jb		nextAddr%ld		; Yes, go to the next address\n", dwGlobalLabel);
		fprintf(fp, "		cmp	%s, [edi+4]	; Are we bigger?\n", pszAddress);
		fprintf(fp, "		jbe	callRoutine%ld\n\n", dwGlobalLabel);
		fprintf(fp, "nextAddr%ld:\n", dwGlobalLabel);
		if (FALSE == bMameFormat) fprintf(fp, "		add	edi, 10h		; Next structure!\n");
		else fprintf(fp, "		add	edi, 14h		; Next structure!\n");
		fprintf(fp, "		jmp	short checkLoop%ld\n\n", dwGlobalLabel);
	}
	fprintf(fp, "callRoutine%ld:\n", dwGlobalLabel);

	if (strcmp(pszAddress, "dx") != 0)
		fprintf(fp, "		mov	dx, %s	; Get our address\n", pszAddress);

	fprintf(fp, "		call	ReadMemoryByte	; Standard read routine\n");

	if ( (strcmp(pszTarget, "dl") == 0) ||
		  (strcmp(pszTarget, "dh") == 0) ||
		  (strcmp(pszTarget, "bh") == 0) )
		fprintf(fp, "		mov	%s, al\n", pszTarget);

	// Yes, these are intentionally reversed!

	if (strcmp(pszTarget, "al") == 0)
		fprintf(fp, "		mov	[_%saf], al	; Save our new accumulator\n", basename);
	else
	if (strcmp(pszTarget, "ah") == 0)
		fprintf(fp, "		mov	[_%saf + 1], al	; Save our new flags\n", basename);

	if (strcmp(pszTarget, "bl") == 0)
		fprintf(fp, "		mov	bl, al	; Get B back\n");
	else
		fprintf(fp, "		mov	bl, [_%sb]	; Get B's original value back\n", basename);

	// How about part of X?

	if (strcmp(pszTarget, "cl") == 0)
		fprintf(fp, "		mov	[_%sx], al	; Store it so it gets restored properly\n", basename);
	if (strcmp(pszTarget, "ch") == 0)
		fprintf(fp, "		mov	[_%sx + 1], al	; Store it so it gets restored properly\n", basename);

	// And are properly restored HERE:

	fprintf(fp, "		mov	cx, [_%sx]	; Get our index register back\n", basename);
	fprintf(fp, "		mov	ax, [_%saf]	; Get our AF back\n", basename);

	// Restore registers here...

	if (FALSE == bMameFormat)
	{
	        fprintf(fp, "		jmp	short readExit%ld\n\n", dwGlobalLabel);
		fprintf(fp, "memoryRead%ld:\n", dwGlobalLabel);
		fprintf(fp, "		mov	%s, [ds:ebp + e%s]	; Get our data\n\n", pszTarget, pszAddress);

		fprintf(fp, "readExit%ld:\n", dwGlobalLabel);
	}
	dwGlobalLabel++;
}

// Reads into DX from the address in DX
void ReadMemoryWord(UINT8 *pszTarget, UINT8 *pszAddress)
{
	if (strcmp(pszTarget, "cx") != 0)
			fprintf(fp, "		push	ecx		; Save X\n");

	if (strcmp(pszAddress, "dx") != 0)
		fprintf(fp, "		mov	dx, %s	; Get our address\n", pszAddress);

	fprintf(fp, "		push	edx\n");

	ReadMemoryByte("ch", "dx");

	fprintf(fp, "		pop	edx\n");

	fprintf(fp, "		inc	dx\n");
	ReadMemoryByte("cl", "dx");

	fprintf(fp, "		xor	edx, edx\n");

	if(strcmp(pszTarget, "cx") != 0)
	{
		fprintf(fp, "		mov	%s, cx\n",pszTarget);
		fprintf(fp, "		pop	ecx\n");
	}
}

void WriteMemoryByteHandler()
{
	Alignment();
	fprintf(fp, "; This is a generic read memory byte handler when a foreign\n");
	fprintf(fp, "; handler is to be called.\n");
	fprintf(fp, "; EDI=Handler address, AL=Byte to write, EDX=Address\n");
	fprintf(fp, "; EDI and EDX Are undisturbed on exit\n\n");
	fprintf(fp, "WriteMemoryByte:\n");

	fprintf(fp, "		mov	[_%sb], bl	; Save B\n", basename);
	fprintf(fp, "		mov	[_%sx], cx	; Save X\n", basename);

	fprintf(fp, "		sub	esi, ebp	; Our program counter\n", basename);
	fprintf(fp, "		mov	[_%spc], si	; Save our program counter\n", basename);

	fprintf(fp, "		push	ebx	; Save our bx register\n");  //new AJP 98/5/14
	fprintf(fp, "		push	edi	; Save our structure address\n");

	if (FALSE != bMameFormat) fprintf(fp, "		and	eax, dword 0ffh\n");
	if (bUseStack)
		fprintf(fp, "		push	eax	; Data to write\n");

	fprintf(fp, "		push	edx	; And our desired address\n");

	if (FALSE == bUseStack)
	{
		fprintf(fp, "		xchg	eax, edx ; Swap address/data around\n");
		fprintf(fp, "		mov	ebx, edi	; Our MemoryWriteByte structure address\n");
	}
	if (FALSE == bMameFormat)
	fprintf(fp, "		call	dword [edi + 8]	; Go call our handler\n");
	else
	fprintf(fp, "		call	_cpu_writemem16	; Go call our handler\n");
	fprintf(fp, "		pop	edx	; Restore our address\n");

	if (bUseStack)
		fprintf(fp, "		pop	eax	; Restore our data written\n");

	fprintf(fp, "		pop	edi	; Save our structure address\n");

	fprintf(fp, "		pop	ebx	; Save our bx register\n");  //new AJP 98/5/14
//	fprintf(fp, "		xor	ebx, ebx	; Zero our future B\n");
	fprintf(fp, "		xor	ecx, ecx	; Zero our future index\n");
	fprintf(fp, "		mov	bl, [_%sb]	; Get B back\n", basename);
	fprintf(fp, "		mov	cx, [_%sx]	; Get index back\n", basename);
	fprintf(fp, "		mov	ax, [_%saf]	; Get AF back\n", basename);
	fprintf(fp, "		xor	esi, esi	; Zero it!\n");
	fprintf(fp, "		mov	si, [_%spc]	; Get our program counter back\n", basename);
	fprintf(fp, "		mov	ebp, [ds:_%sBase] ; Base pointer comes back\n", basename);
	fprintf(fp, "		add	esi, ebp	; Rebase it properly\n");

	fprintf(fp, "		ret\n\n");
}

void WriteMemoryByte(UINT8 *pszValue, UINT8 *pszAddress)
{
        if (FALSE == bMameFormat)
	{
	        fprintf(fp, "		mov	edi, [_%sMemWrite]	; Point to the write array\n\n", basename);
		fprintf(fp, "checkLoop%ld:\n", dwGlobalLabel);
		fprintf(fp, "		cmp	[edi], word 0ffffh ; End of our list?\n");
		fprintf(fp, "		je	memoryWrite%ld	; Yes - go write it!\n", dwGlobalLabel);
		fprintf(fp, "		cmp	%s, [edi]	; Are we smaller?\n", pszAddress);
		fprintf(fp, "		jb	nextAddr%ld	; Yes... go to the next addr\n", dwGlobalLabel);
		fprintf(fp, "		cmp	%s, [edi+4]	; Are we bigger?\n", pszAddress);
		fprintf(fp, "		jbe	callRoutine%ld	; If not, go call it!\n\n", dwGlobalLabel);
		fprintf(fp, "nextAddr%ld:\n", dwGlobalLabel);
		if (FALSE == bMameFormat) fprintf(fp, "		add	edi, 10h		; Next structure, please\n");
		else fprintf(fp, "		add	edi, 14h		; Next structure, please\n");
		fprintf(fp, "		jmp	short checkLoop%ld\n\n", dwGlobalLabel);
	}
	fprintf(fp, "callRoutine%ld:\n", dwGlobalLabel);

	fprintf(fp, "		mov	[_%saf], ax	 ; Store accumulator & flags\n", basename);

	if (strcmp(pszValue, "al") != 0)
		fprintf(fp, "		mov	al, %s	; And our data to write\n", pszValue);

	if (strcmp(pszAddress, "dx") != 0)
		fprintf(fp, "		mov	dx, %s	; Get our address in DX\n", pszAddress);

	fprintf(fp, "		call	WriteMemoryByte	; Go write the data!\n");

	if (FALSE == bMameFormat)
	{
	        fprintf(fp, "		jmp	short writeExit%ld\n", dwGlobalLabel);
		fprintf(fp, "memoryWrite%ld:\n", dwGlobalLabel);
		fprintf(fp, "		mov	[ds:e%s + ebp], %s\n", pszAddress, pszValue);
	}
	fprintf(fp, "writeExit%ld\n", dwGlobalLabel);

	dwGlobalLabel++;
}

FetchInstructionByte()
{
	fprintf(fp,	"		mov	dl, [esi]		; Get the next instruction\n");
	fprintf(fp, "		inc	esi		; Advance PC!\n");
}

CallMameDebugCode()
{
        fprintf(fp, "%%macro CallMameDebug 1\n");
	fprintf(fp, "		test	[_mame_debug], dword 1\n");
	fprintf(fp, "		jz near noMameDBGNow%%1\n");
	FlagsX86to6808();
	fprintf(fp, "		mov	[ds:_%sb], bl	; Store B\n", basename);
	fprintf(fp, "		mov	[ds:_%sx], cx	; Store X\n", basename);
        fprintf(fp, "		mov	[ds:_%scc], ah	; Store flags\n", basename);
	fprintf( fp, "		mov	[ds:_%sa], al	; Store A\n", basename);
	fprintf(fp, "		sub	esi, ebp	; Get our PC back\n");
	fprintf(fp, "		mov	[ds:_%spc], si	; Store PC\n", basename);
	fprintf(fp, "		call	_MAME_Debug\n");
	fprintf(fp, "		xor	eax, eax	; Zero EAX 'cause we use it!\n");
	fprintf(fp, "		xor	ebx, ebx	; Zero EBX, too\n");
	fprintf(fp, "		xor	ecx, ecx	; Zero ECX\n");
	fprintf(fp, "		xor	edx, edx	; And EDX\n");
	fprintf(fp, "		xor	edi, edi	; Zero EDI as well\n");
	fprintf(fp, "		xor	esi, esi	; Zero our source address\n");
	fprintf(fp, "\n");
	fprintf(fp, "		mov	bl, [ds:_%sb]	; Get B\n", basename);
	fprintf(fp, "		mov	cx, [ds:_%sx]	; Get our index register\n", basename);
        fprintf(fp, "		mov	ah, [ds:_%scc]	; Get our flags\n", basename);
        fprintf(fp, "		mov	al, [ds:_%sa]	; Get our accumulator\n", basename);
	fprintf(fp, "		mov	si, [ds:_%spc]	; Get our program counter\n", basename);
	fprintf(fp, "		mov	ebp, [ds:_%sBase]	; Get our base address register\n", basename);
	fprintf(fp, "		add	esi, ebp		; Add in our base address\n");
	Flags6808toX86();
	fprintf(fp, "noMameDBGNow%%1:\n");
	fprintf(fp, "%%endmacro\n");
}

FetchNextInstruction()
{
	if (FALSE != bMameFormat)
	{
                // See if interrupt is waiting
		fprintf(fp, "		test	[ds:_%sirqPending], dword 0ffh\n", basename);
		fprintf(fp, "		jz	noIntNow	; Nope - none waiting\n");
		fprintf(fp, "		call	%sint_\n", basename);
		fprintf(fp, "noIntNow:\n");
		fprintf(fp, "%%ifdef MAME_DEBUG\n");
		fprintf(fp, "		CallMameDebug 1000\n");
		fprintf(fp, "%%endif\n");
	}
	fprintf(fp, "		mov	dl, [esi]\n");

#if 0
	fprintf(fp, "		test	edx, 0ffffff00h\n");
	fprintf(fp, "		jz	skipIt%ld\n", dwGlobalLabel);
	fprintf(fp, "		hlt\n");

	fprintf(fp, "skipIt%ld:\n", dwGlobalLabel++);
#endif

	fprintf(fp, "		inc	esi\n");
	fprintf(fp, "		jmp	dword [ds:%sregular+edx*4]\n\n", basename);
}

MameFetchNextInstructionEnd(UINT32 dwOpcode)
{
	if (!(FALSE == bMameFormat))
	{
	        if (FALSE == bSingleStep)
		{
		        fprintf(fp, "		sub	dword [%s], %ld\n", vcyclesRemaining, (UINT32) bTimingTable[dwOpcode]);
			fprintf(fp, "		jle	near noMoreExec	; Can't execute anymore!\n");
		}
		else
		{
		        fprintf(fp, "		dec	dword [%s]	; Single stepping (debugger)\n", vcyclesRemaining);
			fprintf(fp, "		jz	near noMoreExec ; No more code!\n", procname);
		}
	}

        // See if interrupt is waiting
	fprintf(fp, "		test	[ds:_%sirqPending], dword 0ffh\n", basename);
	fprintf(fp, "		jz	noIntNow0%02x	; Nope - none waiting\n", dwLoop);
	fprintf(fp, "		call	%sint_\n", basename);
	fprintf(fp, "noIntNow0%02x:\n", dwLoop);
	fprintf(fp, "%%ifdef MAME_DEBUG\n");
	fprintf(fp, "		CallMameDebug %x\n", dwLoop);
	fprintf(fp, "%%endif\n");

	fprintf(fp, "		mov	dl, [esi]\n");
	fprintf(fp, "		inc	esi\n");
	fprintf(fp, "		jmp	dword [ds:%sregular+edx*4]\n\n", basename);
}

FetchNextInstructionEnd(UINT32 dwOpcode)
{
	if (FALSE == bMameFormat) FetchNextInstruction();
	else MameFetchNextInstructionEnd(dwOpcode);
}

// Emulated instructions go here

void RtiHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	PopByte("ah");
	PopByte("al");
	Flags6808toX86();
	PopByte("bl");
	PopWord();
	fprintf(fp,"		mov	cx, dx		; Copy DX to X\n");
	PopWord();
	fprintf(fp,"		xor	esi, esi	; Clear ESI\n");
	fprintf(fp, "		mov	si, dx		; Get our program counter\n");
	fprintf(fp, "		mov	ebp, [ds:_%sBase]	; Get our base address register\n", basename);
	fprintf(fp, "		add	esi, ebp		; Add in our base address\n");
	fprintf(fp,"		xor	dx, dx		; Clear out DX\n");

	FetchNextInstructionEnd(dwOpcode);
}

void MulHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	fprintf(fp,"		mov	dl, ah		; save our flags\n");
	fprintf(fp,"		mul	bl	; Multiply!\n");
	fprintf(fp,"		mov	bx, ax		; D in BX\n");
	fprintf(fp,"		mov	ah, dl		; restore our flags\n");
	SetCarry();
	SetZeroSign("bx");
	fprintf(fp,"		mov	al, bh		; A in AL\n");

	FetchNextInstructionEnd(dwOpcode);
}

void XgdxHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	fprintf(fp,"		mov	dx, cx		; save D in DX\n");
	fprintf(fp,"		mov	cl, bl		; save new X\n");
	fprintf(fp,"		mov	ch, al		; \n");
	fprintf(fp,"		mov	al, dh		; save new D\n");
	fprintf(fp,"		mov	bl, dl		; \n");
	fprintf(fp,"		xor	dx, dx		; clear it\n");

	FetchNextInstructionEnd(dwOpcode);
}

// WAI basically halts the CPU until an interrupt happens.  We don't have that
// luxury. :-)  So what this instruction does is moves the PC back so that WAI
// keeps executing over and over and over...
void WaiHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	fprintf(fp, "       and byte [_altFlags], 0efh  ; Interrupt enable\n");
	fprintf(fp, "		dec	esi						; Move back an instruction\n");

	fprintf(fp, "		mov	edi, [%s]	; Get our cycles remaining\n", vcyclesRemaining);
	if (FALSE == bMameFormat) fprintf(fp, "		add	[dwElapsedTicks], edi	; Add in our remaining count\n");
	fprintf(fp, "		mov dword [%s], 00h	; Don't execute any more instructions\n", vcyclesRemaining);

	FetchNextInstructionEnd(dwOpcode);
}

void AbxHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	/*
	old code -- didn't seem to work properly
	fprintf(fp, "		movsx	di, bl		; Put B into DI\n");
	fprintf(fp, "		add	cx, di		; Add X and B\n");
	*/

        fprintf(fp, "		mov	bh, 0		; \n");
	fprintf(fp, "		add	cx, bx		; Add X and B\n");

	FetchNextInstructionEnd(dwOpcode);
}

void DaaHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	fprintf(fp, "		sahf\n");
	fprintf(fp, "		mov	dl, ah\n");
	fprintf(fp, "		daa					; Do DAA and store result in AL\n");

	// Set the flags

	SetCarry();
	SetZeroSign("al");

	FetchNextInstructionEnd(dwOpcode);
}

void NopHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	FetchNextInstructionEnd(dwOpcode);
}

void TsxTxsHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	if(0x30 == dwOpcode)
		fprintf(fp, "		mov	cx, [_%ssp]		; Move SP to X\n", basename);
	else
	if(0x35 == dwOpcode)
		fprintf(fp, "		mov	[_%ssp], cx		; Move X to SP\n", basename);
	else
		assert(0);

	FetchNextInstructionEnd(dwOpcode);
}

void NegHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	if (0x60 == dwOpcode)
	{
		Indexed();

		fprintf(fp, "		push dx			; Save DX\n");

		ReadMemoryByte("bh", "dx");
		fprintf(fp, "		neg	bh			; NEG what we read indexed!\n");
	}
	else
	if (0x70 == dwOpcode)
	{
		Extended();

		fprintf(fp, "		push dx			; Save DX\n");

		ReadMemoryByte("bh", "dx");
		fprintf(fp, "		neg	bh			; NEG what we read extended!\n");
	}
	else
	if (0x40 == dwOpcode)	// NEG A
	{
		fprintf(fp, "		neg	al			; NEG A!\n");
	}
	else
	if (0x50 == dwOpcode)	// NEG B
	{
		fprintf(fp, "		neg	bl			; NEG B!\n");
	}
	else
		assert(0);

	// Set flags for all instructions

	SetOverflowCarry();

	if(dwOpcode >= 0x60)
	{
		fprintf(fp, "		pop dx			; Restore DX\n");

		WriteMemoryByte("bh", "dx");
		SetZeroSign("bh");
	}
	else
	if (0x40 == dwOpcode)
	{
		SetZeroSign("al");
	}
	else
	if (0x50 == dwOpcode)
	{
		SetZeroSign("bl");
	}

	fprintf(fp, "		xor	dh, dh		; Zero this part\n");
	FetchNextInstructionEnd(dwOpcode);
}

void AndBitHandler(UINT16 dwOpcode)
{
	UINT16	dwOpcode2 = dwOpcode & 0x30;
	UINT16	dwOpcode3 = dwOpcode & 0x0f;

	ProcBegin(dwOpcode);

	if(0x00 == dwOpcode2)
	{
		Immediate8();
	}
	else
	if(0x10 == dwOpcode2)
	{
		Direct();
		ReadMemoryByte("dl", "dx");
	}
	else
	if(0x20 == dwOpcode2)
	{
		Indexed();
		ReadMemoryByte("dl", "dx");
	}
	else
	if(0x30 == dwOpcode2)
	{
		Extended();
		ReadMemoryByte("dl", "dx");
	}
	else
		assert(0);			// We're really screwed if we get here...

	if(0x04 == dwOpcode3)	// Check to see if it's a BIT or an AND...
	{
		if(dwOpcode < 0xc0)
		{
			fprintf(fp, "		and	al, dl		; AND A with the immediate byte\n");
			SetZeroSign("al");
		}
		else
		{
			fprintf(fp, "		and	bl, dl		; AND B with the immediate byte\n");
			SetZeroSign("bl");
		}
	}
	else
	{
		if(0xc0 > dwOpcode)
			fprintf(fp, "		and	dl, al		; AND A with the immediate byte\n");
		else
			fprintf(fp, "		and	dl, bl		; AND B with the immediate byte\n");

		SetZeroSign("dl");
	}

	// Set remaining flags for all instructions

	fprintf(fp, "		and	[_altFlags], byte 0fdh	; We've got no overflow\n");
	fprintf(fp, "		xor	dh, dh	; Zero this part\n");

	FetchNextInstructionEnd(dwOpcode);
}

void OrHandler(UINT16 dwOpcode)
{
	UINT16	dwOpcode2 = dwOpcode & 0x30;

	ProcBegin(dwOpcode);

	if(0x00 == dwOpcode2)
	{
		Immediate8();
	}
	else
	if(0x10 == dwOpcode2)
	{
		Direct();
		ReadMemoryByte("dl", "dx");
	}
	else
	if(0x20 == dwOpcode2)
	{
		Indexed();
		ReadMemoryByte("dl", "dx");
	}
	else
	if(0x30 == dwOpcode2)
	{
		Extended();
		ReadMemoryByte("dl", "dx");
	}
	else
		assert(0);			// We're really screwed if we get here...

	if(0xc0 > dwOpcode)
	{
		fprintf(fp, "		or	al, dl		; OR A with the immediate byte\n");
		SetZeroSign("al");
	}
	else
	{
		fprintf(fp, "		or	bl, dl		; OR B with the immediate byte\n");
		SetZeroSign("bl");
	}

	// Set remaining flags for all instructions

	fprintf(fp, "		and	[_altFlags], byte 0fdh	; We've got no overflow\n");
	fprintf(fp, "		xor	dh, dh	; Zero this part\n");

	FetchNextInstructionEnd(dwOpcode);
}

void AdcHandler(UINT16 dwOpcode)
{
	UINT16	dwOpcode2 = dwOpcode & 0x30;

	ProcBegin(dwOpcode);

	if(0x00 == dwOpcode2)
	{
		Immediate8();
	}
	else
	if(0x10 == dwOpcode2)
	{
		Direct();
		ReadMemoryByte("dl", "dx");
	}
	else
	if(0x20 == dwOpcode2)
	{
		Indexed();
		ReadMemoryByte("dl", "dx");
	}
	else
	if(0x30 == dwOpcode2)
	{
		Extended();
		ReadMemoryByte("dl", "dx");
	}
	else
		assert(0);			// We're really screwed if we get here...

	fprintf(fp, "		sahf			; Store our flags in the X86 flag reg.\n");

	if(0xc0 > dwOpcode)
	{
		fprintf(fp,"		adc al, dl		; Add DL+Carry to A\n");
	}
	else
	{
		fprintf(fp,"		adc bl, dl		; Add DL+Carry to A\n");
	}

	// Set flags for all instructions

	SetOverflowCarryHalf();

	if(0xc0 > dwOpcode)
		SetZeroSign("al");
	else
		SetZeroSign("bl");

	fprintf(fp, "		xor	dh, dh	; Zero this part\n");

	FetchNextInstructionEnd(dwOpcode);
}

void SbcHandler(UINT16 dwOpcode)
{
	UINT16	dwOpcode2 = dwOpcode & 0x30;

	ProcBegin(dwOpcode);

	if(0x00 == dwOpcode2)
	{
		Immediate8();
	}
	else
	if(0x10 == dwOpcode2)
	{
		Direct();
		ReadMemoryByte("dl", "dx");
	}
	else
	if(0x20 == dwOpcode2)
	{
		Indexed();
		ReadMemoryByte("dl", "dx");
	}
	else
	if(0x30 == dwOpcode2)
	{
		Extended();
		ReadMemoryByte("dl", "dx");
	}
	else
		assert(0);			// We're really screwed if we get here...

	fprintf(fp, "		sahf			; Store our flags in the X86 flag reg.\n");

	if(0xc0 > dwOpcode)
	{
		fprintf(fp,"		sbb al, dl		; Subtract DL+Carry from A\n");
	}
	else
	{
		fprintf(fp,"		sbb bl, dl		; Subtract DL+Carry from A\n");
	}

	// Set flags for all instructions

	SetOverflowCarry();

	if(0xc0 > dwOpcode)
		SetZeroSign("al");
	else
		SetZeroSign("bl");

	fprintf(fp, "		xor	dh, dh	; Zero this part\n");

	FetchNextInstructionEnd(dwOpcode);
}

void EorHandler(UINT16 dwOpcode)
{
	UINT16	dwOpcode2 = dwOpcode & 0x30;

	ProcBegin(dwOpcode);

	if(0x00 == dwOpcode2)
	{
		Immediate8();
	}
	else
	if(0x10 == dwOpcode2)
	{
		Direct();
		ReadMemoryByte("dl", "dx");
	}
	else
	if(0x20 == dwOpcode2)
	{
		Indexed();
		ReadMemoryByte("dl", "dx");
	}
	else
	if(0x30 == dwOpcode2)
	{
		Extended();
		ReadMemoryByte("dl", "dx");
	}
	else
		assert(0);			// We're really screwed if we get here...

	if(0xc0 > dwOpcode)
	{
		fprintf(fp, "		xor	al, dl		; XOR A with the immediate byte\n");
		SetZeroSign("al");
	}
	else
	{
		fprintf(fp, "		xor	bl, dl		; XOR B with the immediate byte\n");
		SetZeroSign("bl");
	}

	// Set remaining flags for all instructions

	fprintf(fp, "		and	[_altFlags], byte 0fdh	; We've got no overflow\n");
	fprintf(fp, "		xor	dh, dh	; Zero this part\n");

	FetchNextInstructionEnd(dwOpcode);
}

void JmpHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	if (0x6e == dwOpcode)	// JMP indexed
	{
		Indexed();
	}
	else
	if (0x7e == dwOpcode)	// JMP extended
	{
		Extended();
	}
	else
		assert(0);

	fprintf(fp, "		mov	esi, ebp			; Set PC to $0000\n");
	fprintf(fp, "		add	esi, edx			; Add in the direct offset\n");

	fprintf(fp, "		xor	dh, dh				; Clear out DH\n");

	FetchNextInstructionEnd(dwOpcode);
}

void SubHandler(UINT16 dwOpcode)
{
	UINT16	dwOpcode2 = dwOpcode & 0x30;

	ProcBegin(dwOpcode);

	if(0x00 == dwOpcode2)
	{
		Immediate8();
	}
	else
	if(0x10 == dwOpcode2)
	{
		Direct();
		ReadMemoryByte("dl","dx");
	}
	else
	if(0x20 == dwOpcode2)
	{
		Indexed();
		ReadMemoryByte("dl","dx");
	}
	else
	if(0x30 == dwOpcode2)
	{
		Extended();
		ReadMemoryByte("dl","dx");
	}
	else
		assert(0);		// If we get here we're really hosed...

	if(dwOpcode < 0xc0)
	{
		fprintf(fp, "		sub	al, dl				; Sub from A\n");
	}
	else
	{
		fprintf(fp, "		sub	bl, dl				; Sub from B\n");
	}

	SetOverflowCarry();

	if(dwOpcode < 0xc0)
	{
		SetZeroSign("al");
	}
	else
	{
		SetZeroSign("bl");
	}

	fprintf(fp, "		xor	dh, dh	\n");

	FetchNextInstructionEnd(dwOpcode);
}

void CmpxHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	if (0x8c == dwOpcode)	// CMPX	immediate
	{
		Immediate16();
	}
	else
	if (0x9c == dwOpcode)	// CMPX direct
	{
		Direct();
		ReadMemoryWord("dx", "dx");
	}
	else
	if (0xac == dwOpcode)	// CMPX indexed
	{
		Indexed();
		ReadMemoryWord("dx", "dx");
	}
	else
	if (0xbc == dwOpcode)	// CMPX extended
	{
		Extended();
		ReadMemoryWord("dx", "dx");
	}
	else
		assert(0);

	fprintf(fp, "		cmp	cx, dx		; CMP X with our word\n");

	// Set flags for all instructions

	SetOverflowCarryZeroSign();

	fprintf(fp, "		xor	dh, dh	; Zero this part\n");

	FetchNextInstructionEnd(dwOpcode);
}

void SbaCbaHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	fprintf(fp,"		mov dl, al			; Copy A to DL\n");
	fprintf(fp,"		sub	dl, bl			; Subtract B from A\n");

	SetOverflowCarry();
	SetZeroSign("dl");

	if(0x10 == dwOpcode)
		fprintf(fp,"		mov al, dl			; Save the result to A\n");

	FetchNextInstructionEnd(dwOpcode);
}

void PulPshHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	if(0x32 == dwOpcode)
	{
		PopByte("al");
	}
	else
	if(0x33 == dwOpcode)
	{
		PopByte("bl");
	}
	else
	if(0x38 == dwOpcode)
	{
		PopWord();
		fprintf(fp,"		mov	cx, dx		; Copy DX to X\n");
		fprintf(fp,"		xor	dx, dx		; Clear out DX\n");
	}
	else
	if(0x36 == dwOpcode)
	{
		PushByte("al");
	}
	else
	if(0x37 == dwOpcode)
	{
		PushByte("bl");
	}
	else
	if(0x3c == dwOpcode)
	{
		fprintf(fp,"		mov	dx, cx		; Copy X to DX\n");
		PushWord();
		fprintf(fp,"		xor	dx, dx		; Clear out DX\n");
	}
	else
		assert(0);

	FetchNextInstructionEnd(dwOpcode);
}

void AddHandler(UINT16 dwOpcode)
{
	UINT16	dwOpcode2 = dwOpcode & 0x30;

	ProcBegin(dwOpcode);

	if(0x00 == dwOpcode2)
	{
		Immediate8();
	}
	else
	if(0x10 == dwOpcode2)
	{
		Direct();
		ReadMemoryByte("dl","dx");
	}
	else
	if(0x20 == dwOpcode2)
	{
		Indexed();
		ReadMemoryByte("dl","dx");
	}
	else
	if(0x30 == dwOpcode2)
	{
		Extended();
		ReadMemoryByte("dl","dx");
	}
	else
		assert(0);		// If we get here we're really hosed...

	if(dwOpcode < 0xc0)
	{
		fprintf(fp, "		add	al, dl				; Add to A\n");
	}
	else
	{
		fprintf(fp, "		add	bl, dl				; Add to B\n");
	}

	SetOverflowCarryHalf();

	if(dwOpcode < 0xc0)
	{
		SetZeroSign("al");
	}
	else
	{
		SetZeroSign("bl");
	}

	fprintf(fp, "		xor	dh, dh	; Here so we don't hose things\n");

	FetchNextInstructionEnd(dwOpcode);
}

void AdddHandler(UINT16 dwOpcode)
{
        ProcBegin(dwOpcode);

	if(0xc3 == dwOpcode)
	{
		Immediate16();
	}
	else
	if(0xd3 == dwOpcode)
	{
		Direct();
		ReadMemoryWord("dx","dx");
	}
	else
	if(0xe3 == dwOpcode)
	{
		Indexed();
		ReadMemoryWord("dx","dx");
	}
	else
	if(0xf3 == dwOpcode)
	{
		Extended();
		ReadMemoryWord("dx","dx");
	}
	else
		assert(0);		// If we get here we're really hosed...

	fprintf(fp, "		mov	bh, al		; D in BX\n");
	fprintf(fp, "		add	bx, dx		; Add to D\n");

	SetOverflowCarryZeroSign();

	fprintf(fp, "		mov	al, bh		; A in AL\n");

	fprintf(fp, "		xor	dh, dh	; Here so we don't hose things\n");

	FetchNextInstructionEnd(dwOpcode);
}

void SubdHandler(UINT16 dwOpcode)
{
        ProcBegin(dwOpcode);

	if(0x83 == dwOpcode)
	{
		Immediate16();
	}
	else
	if(0x93 == dwOpcode)
	{
		Direct();
		ReadMemoryWord("dx","dx");
	}
	else
	if(0xa3 == dwOpcode)
	{
		Indexed();
		ReadMemoryWord("dx","dx");
	}
	else
	if(0xb3 == dwOpcode)
	{
		Extended();
		ReadMemoryWord("dx","dx");
	}
	else
		assert(0);		// If we get here we're really hosed...

	fprintf(fp, "		mov	bh, al		; D in BX\n");
	fprintf(fp, "		sub	bx, dx		; Add to D\n");

	SetOverflowCarryZeroSign();

	fprintf(fp, "		mov	al, bh		; A in AL\n");

	fprintf(fp, "		xor	dh, dh	; Here so we don't hose things\n");

	FetchNextInstructionEnd(dwOpcode);
}

void AbaHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	fprintf(fp, "		add	al, bl				; Add B to A\n");

	SetOverflowCarryHalf();
	SetZeroSign("al");

	FetchNextInstructionEnd(dwOpcode);
}

void AsxLsrRoxHandler(UINT16 dwOpcode)
{
	UINT16	dwOpcode2 = dwOpcode & 0xf0;

	ProcBegin(dwOpcode);

	if(0x60 == dwOpcode2 || 0x70 == dwOpcode2)
	{
		if(0x60 == dwOpcode2)
			Indexed();
		else
			Extended();

		fprintf(fp, "		push dx			; Save DX\n");

		dwOpcode2 = dwOpcode & 0x0f;

		ReadMemoryByte("bh","dx");

		if(0x04 == dwOpcode2)
		{
			fprintf(fp, "		shr	bh, 1				; Shift BH to the right\n");
			SetCarry();
		}
		else
		if(0x06 == dwOpcode2)
		{
			fprintf(fp, "		sahf\n");
			fprintf(fp, "		rcr	bh, 1				; Rotate BH to the right\n");
			SetCarry();
		}
		else
		if(0x07 == dwOpcode2)
		{
			fprintf(fp, "		sar	bh, 1				; Shift BH to the right\n");
			SetCarry();
		}
		else
		if(0x08 == dwOpcode2)
		{
			fprintf(fp, "		shl	bh, 1				; Shift BH to the left\n");
			SetOverflowCarry();
		}
		else
		if(0x09 == dwOpcode2)
		{
			fprintf(fp, "		sahf\n");
			fprintf(fp, "		rcl	bh, 1				; Rotate BH to the left\n");
			SetOverflowCarry();
		}
		else
			assert(0);

		SetZeroSign("bh");

		fprintf(fp, "		pop dx			; Restore DX\n");

		WriteMemoryByte("bh","dx");

		fprintf(fp, "		xor	bh, bh				; Clear out BH\n");
		fprintf(fp, "		xor	dh, dh				; Clear out DH\n");
	}
	else
	if(0x44 == dwOpcode)
	{
		fprintf(fp, "		shr	al, 1				; Shift A to the right\n");
		SetCarry();
	}
	else
	if(0x46 == dwOpcode)
	{
		fprintf(fp, "		sahf\n");
		fprintf(fp, "		rcr	al, 1				; Rotate A to the right\n");
		SetCarry();
	}
	else
	if(0x47 == dwOpcode)
	{
		fprintf(fp, "		sar	al, 1				; Shift A to the right\n");
		SetCarry();
	}
	else
	if(0x48 == dwOpcode)
	{
		fprintf(fp, "		xor	dl, dl\n");
		fprintf(fp, "		shl	al, 1				; Shift A to the left\n");
		SetCarry();
		fprintf(fp, "		mov	dl, al\n");
		fprintf(fp, "		shr	dl, 1\n");
		fprintf(fp, "		mov	dh, al\n");
		fprintf(fp, "		xor	dh, dl\n");
		fprintf(fp, "		and	dh, 080h\n");
		fprintf(fp, "		shr	dh, 6\n");
		fprintf(fp, "		and	byte [_altFlags], 0fdh	; Clear overflow\n");
		fprintf(fp, "		or	byte [_altFlags], dh\n");
		fprintf(fp, "		xor	dh, dh\n");
	}
	else
	if(0x49 == dwOpcode)
	{
		fprintf(fp, "		sahf\n");
		fprintf(fp, "		rcl	al, 1				; Rotate A to the left\n");
		SetOverflowCarry();
	}
	else
	if(0x54 == dwOpcode)
	{
		fprintf(fp, "		shr	bl, 1				; Shift B to the right\n");
		SetCarry();
	}
	else
	if(0x56 == dwOpcode)
	{
		fprintf(fp, "		sahf\n");
		fprintf(fp, "		rcr	bl, 1				; Rotate B to the right\n");
		SetCarry();
	}
	else
	if(0x57 == dwOpcode)
	{
		fprintf(fp, "		sar	bl, 1				; Shift B to the right\n");
		SetCarry();
	}
	else
	if(0x58 == dwOpcode)
	{
		fprintf(fp, "		shl	bl, 1				; Shift B to the left\n");
		SetCarry();
		fprintf(fp, "		mov	dl, bl\n");
		fprintf(fp, "		shr	dl, 1\n");
		fprintf(fp, "		mov	dh, bl\n");
		fprintf(fp, "		xor	dh, dl\n");
		fprintf(fp, "		and	dh, 080h\n");
		fprintf(fp, "		shr	dh, 6\n");
		fprintf(fp, "		and	byte [_altFlags], 0fdh	; Clear overflow\n");
		fprintf(fp, "		or	byte [_altFlags], dh\n");
		fprintf(fp, "		xor	dh, dh\n");
	}
	else
	if(0x59 == dwOpcode)
	{
		fprintf(fp, "		sahf\n");
		fprintf(fp, "		rcl	bl, 1				; Rotate B to the left\n");
		SetOverflowCarry();
	}
	else
		assert(0);

	if(0x40 == (dwOpcode & 0xf0))
	{
		SetZeroSign("al");
	}
	else
	if(0x50 == (dwOpcode & 0xf0))
	{
		SetZeroSign("bl");
	}

	FetchNextInstructionEnd(dwOpcode);
}

void AsldLsrdHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	fprintf(fp, "		mov	bh, al		; D in BX\n");

	if(0x04 == dwOpcode)
	{
		fprintf(fp, "		shr	bx, 1		; shift BX\n");
		SetCarry();
		SetZeroSign("bx");
	}
	else
	if(0x05 == dwOpcode)
	{
		fprintf(fp, "		shl	bx, 1		; shift BX\n");
		SetOverflowCarryZeroSign();
	}
	else
		assert(0);

	fprintf(fp, "		mov	al, bh		; A in AL\n");

	FetchNextInstructionEnd(dwOpcode);
}

void TbaTabTpaTapHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	if (0x06 == dwOpcode)	// TAP
	{
		fprintf(fp, "		mov	ah, al				; Copy A to CC\n");
		Flags6808toX86();
	}
	else
	if (0x07 == dwOpcode)	// TPA
	{
		FlagsX86to6808();
		fprintf(fp, "		mov	al, ah				; Copy CC to A\n");
		Flags6808toX86();
	}
	else
	if (0x16 == dwOpcode)	// TAB
		fprintf(fp, "		mov	bl, al				; Transfer A to B\n");
	else
	if (0x17 == dwOpcode)	// TBA
		fprintf(fp, "		mov	al, bl				; Transfer B to A\n");
	else
		assert(0);

	if (dwOpcode & 0x10)
	{
		fprintf(fp, "		and	byte [_altFlags], 0fdh	; Clear overflow\n");
		SetZeroSign("al");
	}

	FetchNextInstructionEnd(dwOpcode);
}

void JsrBsrHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	fprintf(fp,"		mov	edx, esi			; Copy the PC to EDX\n");
	fprintf(fp,"		sub	edx, ebp			; Subtract out the base\n");

	if(0xb0 > dwOpcode)
		fprintf(fp, "		inc dx					; Skip our offset we haven't read yet\n");
	else
		fprintf(fp, "		add dx, 2				; Skip our offset we haven't read yet\n");

	PushWord();

	fprintf(fp, "		xor	dh, dh				; Clear out DH\n");

	if(0x8d == dwOpcode)	// BSR -- put the dest address into DX
	{
		Immediate8();
		fprintf(fp, "		xor		edi, edi		; Clear out Edi\n");
		fprintf(fp,"		movsx	di, dl			; Make our offset a word\n");
		fprintf(fp,"		mov		edx, esi		; Copy ESI to EDX\n");
		fprintf(fp,"		sub		edx, ebp		; Subtract out the base\n");
		fprintf(fp,"		add		dx, di			; Add in our branch\n");
	}
	else
	if (0x9d == dwOpcode)	// JSR direct
	{
		Direct();
	}
	else
	if (0xad == dwOpcode)	// JSR indexed
	{
		Indexed();
	}
	else
	if (0xbd == dwOpcode)	// JSR extended
	{
		Extended();
	}
	else
		assert(0);

	fprintf(fp, "		mov	esi, ebp			; Set PC to $0000\n");
	fprintf(fp, "		add	esi, edx			; Add in the direct offset\n");

	fprintf(fp, "		xor	dh, dh				; Clear out DH\n");

	FetchNextInstructionEnd(dwOpcode);
}

void RtsHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	fprintf(fp, "		xor	edx, edx			; Clear out EDX\n");

	PopWord();

	fprintf(fp, "		mov	esi, ebp			; Set PC to $0000\n");
	fprintf(fp, "		add	esi, edx			; Add in the direct offset\n");

	fprintf(fp, "		xor	dh, dh				; Clear out DH\n");

	FetchNextInstructionEnd(dwOpcode);
}

void IncHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	if (0x4c == dwOpcode)	// INCA inherent
	{
			fprintf(fp, "		inc	al			; Increment A\n");
			SetOverflow();
			SetZeroSign("al");
	}
	else
	if (0x5c == dwOpcode)	// INCB inherent
	{
			fprintf(fp, "		inc	bl			; Increment B\n");
			SetOverflow();
			SetZeroSign("bl");
	}
	else
	if (0x6c == dwOpcode || 0x7c == dwOpcode)	// INC indexed or extended
	{
			if(0x6c == dwOpcode)
				Indexed();
			else
				Extended();

			fprintf(fp, "		push dx			; Save our address for later\n");

			ReadMemoryByte("dl", "dx");

			fprintf(fp, "		inc	dl			; Increment the byte we just read\n");
			SetOverflow();
			SetZeroSign("dl");

			fprintf(fp, "		mov	bh, dl		; Save the byte read to BH\n");
			fprintf(fp, "		pop dx			; Get our address back\n");

			WriteMemoryByte("bh", "dx");

			fprintf(fp, "		xor	bh, bh		; Clear BH\n");
			fprintf(fp, "		xor	dh, dh\n");
	}
	else
		assert(0);

	FetchNextInstructionEnd(dwOpcode);
}

void DecHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	if (0x4a == dwOpcode)	// DECA inherent
	{
			fprintf(fp, "		dec	al			; Decrement A\n");
			SetOverflow();
			SetZeroSign("al");
	}
	else
	if (0x5a == dwOpcode)	// DECB inherent
	{
			fprintf(fp, "		dec	bl			; Decrement B\n");
			SetOverflow();
			SetZeroSign("bl");
	}
	else
	if (0x6a == dwOpcode || 0x7a == dwOpcode)	// DEC indexed or extended
	{
			if(0x6a == dwOpcode)
				Indexed();
			else
				Extended();

			fprintf(fp, "		push dx			; Save our address for later\n");

			ReadMemoryByte("dl", "dx");

			fprintf(fp, "		dec	dl			; Decrement the byte we just read\n");
			SetOverflow();
			SetZeroSign("dl");

			fprintf(fp, "		mov	bh, dl		; Save the byte read to BH\n");
			fprintf(fp, "		pop dx			; Get our address back\n");

			WriteMemoryByte("bh", "dx");

			fprintf(fp, "		xor	bh, bh		; Clear BH\n");
			fprintf(fp, "		xor	dh, dh		; Clear DH\n");
	}
	else
		assert(0);

	FetchNextInstructionEnd(dwOpcode);
}

void InsHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	fprintf(fp, "		inc	word [_%ssp]	; Read in our current stack pointer\n", basename);
	FetchNextInstructionEnd(dwOpcode);
}

void DesHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	fprintf(fp, "		dec	word [_%ssp]	; Read in our current stack pointer\n", basename);
	FetchNextInstructionEnd(dwOpcode);
}

void SWIHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	// It can be taken. Save off the registers

	fprintf(fp, "		call	%spushall_\n", basename);
	fprintf(fp, "		or	[ds:_altFlags], byte 10h\n");
	fprintf(fp, "		push	eax\n");
	fprintf(fp, "		xor		eax, eax		; Clear out EAX\n");
	fprintf(fp, "		mov	ax, [ds:ebp+0fffah]	; Get SWI vector\n");
	fprintf(fp, "		xchg	ah, al\n");
	fprintf(fp, "		mov	esi, ebp\n");
	fprintf(fp, "		add esi, eax\n");
	fprintf(fp, "		pop	eax\n");

	FetchNextInstructionEnd(dwOpcode);
}

void TstHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	if (0x6d == dwOpcode)	// TST	indexed
	{
		Indexed();
		ReadMemoryByte("dl", "dx");

		fprintf(fp, "		xor	dh, dh		; Zero this part\n");
	}
	else
	if (0x7d == dwOpcode)	// TST extended
	{
		Extended();
		ReadMemoryByte("dl", "dx");

		fprintf(fp, "		xor	dh, dh		; Zero this part\n");
	}
	else
	if (0x4d == dwOpcode)	// TSTA inherent
	{
		fprintf(fp, "		mov	dl, al		; Move A into DL\n");
	}
	else
	if (0x5d == dwOpcode)	// TSTB inherent
	{
		fprintf(fp, "		mov	dl, bl		; Move B into DL\n");
	}
	else
		assert(0);

	// Set flags for all instructions

	SetZeroSign("dl");
	fprintf(fp, "		and	byte [_altFlags], 0fdh	; Clear overflow\n");
	fprintf(fp, "		xor	dh, dh		; Zero this part\n");

	FetchNextInstructionEnd(dwOpcode);
}

void CmpHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	if (0x81 == dwOpcode)	// CMPA	immediate
	{
		Immediate8();
		fprintf(fp, "		cmp	al, dl		; CMP A with the immediate byte\n");
	}
	else
	if (0x91 == dwOpcode)	// CMPA direct
	{
		Direct();
		ReadMemoryByte("dl", "dx");
		fprintf(fp, "		cmp	al, dl		; CMP A with the direct byte\n");
	}
	else
	if (0xa1 == dwOpcode)	// CMPA indexed
	{
		Indexed();
		ReadMemoryByte("dl", "dx");
		fprintf(fp, "		cmp	al, dl		; CMP A with the indexed byte\n");
	}
	else
	if (0xb1 == dwOpcode)	// CMPA extended
	{
		Extended();
		ReadMemoryByte("dl", "dx");
		fprintf(fp, "		cmp	al, dl		; CMP A with the extended byte\n");
	}
	else
	if (0xc1 == dwOpcode)	// CMPB	immediate
	{
		Immediate8();
		fprintf(fp, "		cmp	bl, dl		; CMP B with the immediate byte\n");
	}
	else
	if (0xd1 == dwOpcode)	// CMPB direct
	{
		Direct();
		ReadMemoryByte("dl", "dx");
		fprintf(fp, "		cmp	bl, dl		; CMP B with the direct byte\n");
	}
	else
	if (0xe1 == dwOpcode)	// CMPB indexed
	{
		Indexed();
		ReadMemoryByte("dl", "dx");
		fprintf(fp, "		cmp	bl, dl		; CMP B with the indexed byte\n");
	}
	else
	if (0xf1 == dwOpcode)	// CMPB extended
	{
		Extended();
		ReadMemoryByte("dl", "dx");
		fprintf(fp, "		cmp	bl, dl		; CMP B with the extended byte\n");
	}
	else
		assert(0);

	// Set flags for all instructions

	SetOverflowCarryZeroSign();
	fprintf(fp, "		xor	dh, dh		; Zero this part\n");
	FetchNextInstructionEnd(dwOpcode);
}

void SecSeiSevClcCliClvHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	// Clear operators

	if (0x0c == dwOpcode)
		fprintf(fp, "		and	ah, 0feh	; Clear carry\n");
	else
	if (0x0e == dwOpcode)
		fprintf(fp, "		and	byte [_altFlags], 0efh	; Interrupt enable\n");
	else
	if (0x0a == dwOpcode)
		fprintf(fp, "		and	byte [_altFlags], 0fdh	; Clear overflow\n");
	else

	// Set operators

	if (0x0d == dwOpcode)
		fprintf(fp, "		or	ah, 01h	; Set carry\n");
	else
	if (0x0b == dwOpcode)
		fprintf(fp, "		or	byte [_altFlags], 02h ; Set overflow\n");
	else
	if (0x0f == dwOpcode)
		fprintf(fp, "		or	byte [_altFlags], 10h ; Interrupt disable\n");
	else
		assert(0);

	FetchNextInstructionEnd(dwOpcode);
}

void LdsLdx(UINT16 dwOpcode)
{
	UINT16 dwOpcode1;

	ProcBegin(dwOpcode);
	dwOpcode1 = dwOpcode & 0x30;

	fprintf(fp, "		xor	edx, edx	; Make sure EDX is clear\n");

	// Whatever addressing mode we need

	if (0x00 == dwOpcode1)
		Immediate16();
	else
	if (0x10 == dwOpcode1)
		Direct();
	else
	if (0x20 == dwOpcode1)
		Indexed();
	else
	if (0x30 == dwOpcode1)
		Extended();

	// Now figure out the correct opcode

	if(0xc0 >= dwOpcode)	// It's LDS
	{
		if(0 == dwOpcode1)		// Check for immediate or not
		{
			fprintf(fp, "		mov [_%ssp], dx	; Save our new SP\n", basename);
		}
		else
		{
			ReadMemoryWord("dx","dx");
			fprintf(fp, "		mov	[_%ssp], dx	; Save our new SP\n", basename);
		}

		SetZeroSign("dx");
	}
	else
	{
		if(0 == dwOpcode1)		// Check for immediate or not
		{
			fprintf(fp, "		mov	cx, dx		; Save it to X\n");
			SetZeroSign("dx");
		}
		else
		{
			ReadMemoryWord("cx","dx");
			SetZeroSign("cx");		// Set our sign and zero based upon its results
		}

	}

	// Set flags appropriately

	fprintf(fp, "		and	byte [_altFlags], 0fdh	; Clear overflow\n");
	fprintf(fp, "		xor	dh, dh	; Knock out the MSB of DX\n");

	FetchNextInstructionEnd(dwOpcode);
}

void LddHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);
	fprintf(fp, "		xor	edx, edx	; Make sure EDX is clear\n");

	// Whatever addressing mode we need

	if (0xcc == dwOpcode)
		Immediate16();
	else
	if (0xdc == dwOpcode)
		Direct();
	else
	if (0xec == dwOpcode)
		Indexed();
	else
	if (0xfc == dwOpcode)
		Extended();

	// Now figure out the correct opcode

	if(0xcc == dwOpcode)		// Check for immediate or not
	{
		fprintf(fp, "		mov	al, dh		; Save it to A\n");
		fprintf(fp, "		mov	bl, dl		; Save it to B\n");
		SetZeroSign("dx");
	}
	else
	{
		ReadMemoryWord("bx","dx");
		SetZeroSign("bx");		// Set our sign and zero based upon its results
		fprintf(fp, "		mov	al, bh		; Save it to A\n");
	fprintf(fp, "		xor	bh, bh	; Knock out the MSB of BX\n");

	}

	// Set flags appropriately

	fprintf(fp, "		and	byte [_altFlags], 0fdh	; Clear overflow\n");
	fprintf(fp, "		xor	dh, dh	; Knock out the MSB of DX\n");

	FetchNextInstructionEnd(dwOpcode);
}

void ClrHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	if (0x7f == dwOpcode)
	{
		Immediate16();	// Word fetch
		WriteMemoryByte("bh", "dx");	// Write a zero!
	}
	else
	if (0x6f == dwOpcode)
	{
		Indexed();
		WriteMemoryByte("bh", "dx");	// Write a zero!
	}
	else
	if (0x4f == dwOpcode)	// CLR A
		fprintf(fp, "		xor	al, al	; Accumulator is toast!\n");
	else
	if (0x5f == dwOpcode)	// CLR B
		fprintf(fp, "		xor	bl, bl	; B Is toast!\n");
	else
		assert(0);

	// Set flags for all instructions

	fprintf(fp, "		and	ah, 03eh	; No carry, sign, or zero\n");
	fprintf(fp, "		and	[_altFlags], byte 0fdh	; No overflow\n");
	fprintf(fp, "		or	ah, 040h	; We've got zero!\n");
	fprintf(fp, "		xor	dh, dh	; Zero this part\n");

	FetchNextInstructionEnd(dwOpcode);
}

void ComHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	if (0x63 == dwOpcode)
	{
		Indexed();

		fprintf(fp, "		push dx			; Save DX\n");

		ReadMemoryByte("bh", "dx");
		fprintf(fp, "		xor	bh, 0ffh	; XOR what we read indexed!\n");
	}
	else
	if (0x73 == dwOpcode)
	{
		Extended();

		fprintf(fp, "		push dx			; Save DX\n");

		ReadMemoryByte("bh", "dx");
		fprintf(fp, "		xor	bh, 0ffh	; XOR what we read extended!\n");
	}
	else
	if (0x43 == dwOpcode)	// COM A
	{
		fprintf(fp, "		xor	al, 0ffh	; XOR A!\n");
	}
	else
	if (0x53 == dwOpcode)	// COM B
	{
		fprintf(fp, "		xor	bl, 0ffh	; XOR B!\n");
	}
	else
		assert(0);

	// Set flags for all instructions

	if(0x60 <= dwOpcode)
	{
		fprintf(fp, "		pop dx			; Restore DX\n");

		WriteMemoryByte("bh", "dx");
		SetZeroSign("bh");
	}
	else
	if (0x43 == dwOpcode)
	{
		SetZeroSign("al");
	}
	else
	if (0x53 == dwOpcode)
	{
		SetZeroSign("bl");
	}

	fprintf(fp, "		and	[_altFlags], byte 0fdh	; We've got no overflow\n");
	fprintf(fp, "		or	ah, 001h	; We've got carry!\n");
	fprintf(fp, "		xor	dh, dh	; Zero this part\n");
	fprintf(fp, "		xor	bh, bh	; Zero BH\n");

	FetchNextInstructionEnd(dwOpcode);
}

void AimHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	if (0x61 == dwOpcode)
	{
	        Immediate8();			// Fetch data
		fprintf(fp, "		push dx			; Save DX\n");

		Indexed();

		fprintf(fp, "		push dx			; Save DX\n");
		ReadMemoryByte("bh", "dx");
		fprintf(fp, "		pop dx			; Restore DX\n");
		fprintf(fp, "		pop cx			; Put it in CX\n");
		fprintf(fp, "		and	bh, cl	; AND what we read indexed!\n");
		fprintf(fp, "		mov	cx, [_%sx]	; Get our index register back\n", basename);
	}
	else
	if (0x62 == dwOpcode)
	{
	        Immediate8();			// Fetch data
		fprintf(fp, "		push dx			; Save DX\n");

		Indexed();

		fprintf(fp, "		push dx			; Save DX\n");
		ReadMemoryByte("bh", "dx");
		fprintf(fp, "		pop dx			; Restore DX\n");
		fprintf(fp, "		pop cx			; Put it in CX\n");
		fprintf(fp, "		or	bh, cl	; OR what we read indexed!\n");
		fprintf(fp, "		mov	cx, [_%sx]	; Get our index register back\n", basename);
	}
	else
	if (0x71 == dwOpcode)
	{
	        Immediate8();			// Fetch data
		fprintf(fp, "		push dx			; Save DX\n");

		Direct();

		fprintf(fp, "		push dx			; Save DX\n");
		ReadMemoryByte("bh", "dx");
		fprintf(fp, "		pop dx			; Restore DX\n");
		fprintf(fp, "		pop cx			; Put it in CX\n");
		fprintf(fp, "		and	bh, cl	; AND what we read indexed!\n");
		fprintf(fp, "		mov	cx, [_%sx]	; Get our index register back\n", basename);
	}
	else
	if (0x72 == dwOpcode)
	{
	        Immediate8();			// Fetch data
		fprintf(fp, "		push dx			; Save DX\n");

		Direct();

		fprintf(fp, "		push dx			; Save DX\n");
		ReadMemoryByte("bh", "dx");
		fprintf(fp, "		pop dx			; Restore DX\n");
		fprintf(fp, "		pop cx			; Put it in CX\n");
		fprintf(fp, "		or	bh, cl	; OR what we read indexed!\n");
		fprintf(fp, "		mov	cx, [_%sx]	; Get our index register back\n", basename);
	}
	else
		assert(0);

	// Set flags for all instructions

	if(0x60 <= dwOpcode)
	{
		WriteMemoryByte("bh", "dx");
		SetZeroSign("bh");
	}

	fprintf(fp, "		and	[_altFlags], byte 0fdh	; We've got no overflow\n");
	fprintf(fp, "		xor	dh, dh	; Zero this part\n");
	fprintf(fp, "		xor	bh, bh	; Zero BH\n");

	FetchNextInstructionEnd(dwOpcode);
}

void LdaHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	if (0x86 == dwOpcode)	// LDA immediate 8
	{
		Immediate8();
		fprintf(fp, "		mov	al, dl\n");
	}
	else
	if (0x96 == dwOpcode)	// LDA Direct
	{
		Direct();
		ReadMemoryByte("al", "dx");
	}
	else
	if (0xa6 == dwOpcode)	// LDA Indexed
	{
		Indexed();
		ReadMemoryByte("al", "dx");
	}
	else
	if (0xb6 == dwOpcode)	// LDA Extended
	{
		Extended();
		ReadMemoryByte("al", "dx");
	}
	else
		assert(0);

	// Set flags for all instructions

	SetZeroSign("al");
	fprintf(fp, "		and	byte [_altFlags], 0fdh	; Clear overflow\n");
	fprintf(fp, "		xor	dh, dh	; Knock out the MSB of DX\n");
	FetchNextInstructionEnd(dwOpcode);
}

void LdbHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	if (0xc6 == dwOpcode)	// LDB immediate 8
		Immediate8();
	else
	if (0xd6 == dwOpcode)	// LDB Direct
	{
		Direct();
		ReadMemoryByte("dl", "dx");
	}
	else
	if (0xe6 == dwOpcode)	// LDB Indexed
	{
		Indexed();
		ReadMemoryByte("dl", "dx");
	}
	else
	if (0xf6 == dwOpcode)	// LDB Extended
	{
		Extended();
		ReadMemoryByte("dl", "dx");
	}
	else
		assert(0);

	// B register now gets it

	fprintf(fp, "		mov	bl, dl\n");

	// Set flags for all instructions

	SetZeroSign("bl");
	fprintf(fp, "		and	byte [_altFlags], 0fdh	; Clear overflow\n");
	fprintf(fp, "		xor	dh, dh	; Knock out the MSB of DX\n");
	FetchNextInstructionEnd(dwOpcode);
}

void StaHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

// 0x87 - STA #xxh doesn't make any sense

//	if (0x87 == dwOpcode)	// STA immediate 8
//		Immediate8();
//	else
	if (0x97 == dwOpcode)	// STA Direct
	{
		Direct();
		WriteMemoryByte("al", "dx");
	}
	else
	if (0xa7 == dwOpcode)	// STA Indexed
	{
		Indexed();
		WriteMemoryByte("al", "dx");
	}
	else
	if (0xb7 == dwOpcode)	// STA Extended
	{
		Extended();
		WriteMemoryByte("al", "dx");
	}
	else
		assert(0);

	// Set flags for all instructions

	SetZeroSign("al");
	fprintf(fp, "		and	byte [_altFlags], 0fdh	; Clear overflow\n");
	fprintf(fp, "		xor	dh, dh	; Knock out the MSB of DX\n");
	FetchNextInstructionEnd(dwOpcode);
}

void StbHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

// 0xc7 - STB #xxh doesn't make any sense

//	if (0xc7 == dwOpcode)	// STB immediate 8
//		Immediate8();
//	else
	if (0xd7 == dwOpcode)	// STB Direct
	{
		Direct();
		WriteMemoryByte("bl", "dx");
	}
	else
	if (0xe7 == dwOpcode)	// STB Indexed
	{
		Indexed();
		WriteMemoryByte("bl", "dx");
	}
	else
	if (0xf7 == dwOpcode)	// STB Extended
	{
		Extended();
		WriteMemoryByte("bl", "dx");
	}
	else
		assert(0);

	// Set flags for all instructions

	SetZeroSign("bl");
	fprintf(fp, "		and	byte [_altFlags], 0fdh	; Clear overflow\n");
	fprintf(fp, "		xor	dh, dh	; Knock out the MSB of DX\n");
	FetchNextInstructionEnd(dwOpcode);
}

void StxHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

// 0xcf - STX #xxh doesn't make any sense

//	if (0xcf == dwOpcode)	// STX immediate 8
//		Immediate8();
//	else
	if (0xdf == dwOpcode)	// STX Direct
		Direct();
	else
	if (0xef == dwOpcode)	// STX Indexed
		Indexed();
	else
	if (0xff == dwOpcode)	// STX Extended
		Extended();
	else
		assert(0);

	WriteMemoryByte("ch", "dx");
	fprintf(fp, "		inc	dx\n");
	WriteMemoryByte("cl", "dx");

	// Set flags for all instructions

	SetZeroSign("cx");	// Set flags for X
	fprintf(fp, "		and	byte [_altFlags], 0fdh	; Clear overflow\n");
	fprintf(fp, "		xor	dh, dh	; Knock out the MSB of DX\n");
	FetchNextInstructionEnd(dwOpcode);
}

void StdHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

// 0xcd - STD #xxh doesn't make any sense

//	if (0xcd == dwOpcode)	// STD immediate 8
//		Immediate8();
//	else
	if (0xdd == dwOpcode)	// STD Direct
		Direct();
	else
	if (0xed == dwOpcode)	// STD Indexed
		Indexed();
	else
	if (0xfd == dwOpcode)	// STD Extended
		Extended();
	else
		assert(0);

	WriteMemoryByte("al", "dx");
	fprintf(fp, "		inc	dx\n");
	WriteMemoryByte("bl", "dx");

	// Set flags for all instructions

	fprintf(fp, "           mov	dh, al ; high byte\n");
	fprintf(fp, "           mov	dl, bl ; low byte\n");

	SetZeroSign("dx");	// Set flags for X
	fprintf(fp, "		and	byte [_altFlags], 0fdh	; Clear overflow\n");
	fprintf(fp, "		xor	dh, dh	; Knock out the MSB of DX\n");
	FetchNextInstructionEnd(dwOpcode);
}

void BraHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	Immediate8();			// Fetch our offset (just in case)

	if (0x20 == dwOpcode)	// BRA xx	- Providing support
	{
                // We just jump
                if (FALSE != bMameFormat)
                {
                        fprintf(fp, "		cmp	dl,0feh ; Check for infinite loop\n");
                        fprintf(fp, "		jnz	takeJump%ld\n", dwGlobalLabel);
                        fprintf(fp, "		push	eax\n");
                        fprintf(fp, "		mov	eax, dword[%s]\n", vcyclesRemaining);
                        fprintf(fp, "		mov	dword [%s],0\n", vcyclesRemaining);
                        fprintf(fp, "		pop	eax\n");
                }
        }
	else
	if (0x22 == dwOpcode)	// BHI
	{
                fprintf(fp, "           test            ah, 41h ; Carry & zero?\n");
                fprintf(fp, "           jnz             noJump%ld\n", dwGlobalLabel);
	}
	else
	if (0x23 == dwOpcode)	// BLS
	{
                fprintf(fp, "           test            ah, 41h ; Carry & zero?\n");
                fprintf(fp, "           jz              noJump%ld\n", dwGlobalLabel);
	}
	else
	if (0x24 == dwOpcode)	// BCC
	{
		fprintf(fp, "		test	ah, 01h	; Carry clear?\n");
		fprintf(fp, "		jnz		noJump%ld\n", dwGlobalLabel);
	}
	else
	if (0x25 == dwOpcode)	// BCS
	{
		fprintf(fp, "		test	ah, 01h	; Carry clear?\n");
		fprintf(fp, "		jz		noJump%ld\n", dwGlobalLabel);
	}
	else
	if (0x26 == dwOpcode)	// BNE
	{
		fprintf(fp, "		test	ah, 40h	; Not equal?\n");
		fprintf(fp, "		jnz		noJump%ld\n", dwGlobalLabel);
	}
	else
	if (0x27 == dwOpcode)	// BEQ
	{
		fprintf(fp, "		test	ah, 40h	; Equal?\n");
		fprintf(fp, "		jz		noJump%ld\n", dwGlobalLabel);
	}
	else
	if (0x28 == dwOpcode)	// BVC
	{
		fprintf(fp, "		test	byte [_altFlags], 02h ; Overflow set?\n");
		fprintf(fp, "		jnz		noJump%ld\n", dwGlobalLabel);
	}
	else
	if (0x29 == dwOpcode)	// BVS	- Beavis?
	{
		fprintf(fp, "		test	byte [_altFlags], 02h ; Overflow set?\n");
		fprintf(fp, "		jz		noJump%ld\n", dwGlobalLabel);
	}
	else
	if (0x2a == dwOpcode)	// BPL
	{
                fprintf(fp, "           test    ah, 80h ; Sign set?\n");
                fprintf(fp, "           jnz             noJump%ld\n", dwGlobalLabel);
	}
	else
	if (0x2b == dwOpcode)	// BMI
	{
                fprintf(fp, "           test    ah, 80h ; Sign set?\n");
                fprintf(fp, "           jz              noJump%ld\n", dwGlobalLabel);
	}
	else
	if (0x2c == dwOpcode)	// BGE
	{
		fprintf(fp, "		mov		dh, [_altFlags]	; Get our alt flags\n");
		fprintf(fp, "		shl		dh, 6	; Make Overflow match sign\n");
		fprintf(fp, "		xor		dh, ah	; See what we get out of sign/overflow\n");
		fprintf(fp, "		test	dh, 80h\n");
		fprintf(fp, "		jnz		noJumpClr%ld\n", dwGlobalLabel);
	}
	else
	if (0x2d == dwOpcode)	// BLT (Bacon, lettuce, and tomato)
	{
		fprintf(fp, "		mov		dh, [_altFlags]	; Get our alt flags\n");
		fprintf(fp, "		shl		dh, 6	; Make Overflow match sign\n");
		fprintf(fp, "		xor		dh, ah	; See what we get out of sign/overflow\n");
		fprintf(fp, "		test	dh, 80h\n");
		fprintf(fp, "		jz		noJumpClr%ld\n", dwGlobalLabel);
	}
	else
	if (0x2e == dwOpcode)	// BGT
	{
                fprintf(fp, "           test    ah, 40h ; Is zero set?\n");
                fprintf(fp, "           jnz     noJump%ld       ; Don't take the jump if so!\n", dwGlobalLabel);
                fprintf(fp, "           mov             dh, [_altFlags] ; Get our alt flags\n");
                fprintf(fp, "           shl             dh, 6   ; Make Overflow match sign\n");
		fprintf(fp, "		xor		dh, ah	; See what we get out of sign/overflow\n");
		fprintf(fp, "		test	dh, 80h\n");
		fprintf(fp, "		jnz		noJumpClr%ld\n", dwGlobalLabel);
	}
	else
	if (0x2f == dwOpcode)	// BLE
	{
                fprintf(fp, "           test    ah, 40h ; Is zero set?\n");
		fprintf(fp, "		jnz		takeJump%ld ; Yup! Jump!\n", dwGlobalLabel);
                fprintf(fp, "           mov             dh, [_altFlags] ; Get our alt flags\n");
                fprintf(fp, "           shl             dh, 6   ; Make Overflow match sign\n");
		fprintf(fp, "		xor		dh, ah	; See what we get out of sign/overflow\n");
		fprintf(fp, "		test	dh, 80h\n");
		fprintf(fp, "		jz		noJumpClr%ld\n", dwGlobalLabel);
	}
	else
		assert(0);

	fprintf(fp, "takeJump%ld:\n", dwGlobalLabel);
	fprintf(fp, "			movsx	dx, dl\n");
	fprintf(fp, "			sub		esi, ebp	; Get rid of the base\n");
	fprintf(fp, "			add		si, dx	; Add in our offset\n");
	fprintf(fp, "			add		esi, ebp	; Back up to the base\n");
	fprintf(fp, "noJumpClr%ld:\n", dwGlobalLabel);
	fprintf(fp, "			xor		dh, dh	; Get rid of this\n");
	fprintf(fp, "noJump%ld:\n", dwGlobalLabel++);

	FetchNextInstructionEnd(dwOpcode);
}

void InxDexHandler(UINT16 dwOpcode)
{
	ProcBegin(dwOpcode);

	fprintf(fp, "		and	ah, 0bfh	; Knock out zero flag\n");
	fprintf(fp, "		mov	dh, ah	; Store it for later\n");

	if (0x09 == dwOpcode)
		fprintf(fp, "	dec	cx	; Decrement our index counter\n");
	else
	if (0x08 == dwOpcode)
		fprintf(fp, "	inc	cx	; Increment our index counter\n");
	else
		assert(0);

	fprintf(fp, "		lahf\n");
	fprintf(fp, "		and	ah, 40h	; Zero flag only\n");
	fprintf(fp, "		or	ah, dh	; OR In our old flags\n");
	fprintf(fp, "		xor	dh, dh	; Zero this for later\n");

	FetchNextInstructionEnd(dwOpcode);
}

DataSegment()
{
	UINT32 dwLoop = 0;
	UINT32 dwBit = 0;
	UINT8 bValue = 0;
	UINT8 bUsed[256];
	fprintf(fp, "		section	.data\n");
	Alignment();
	fprintf(fp, "		global	_%spc\n\n",basename);
	if (FALSE != bMameFormat)
	{
	        fprintf(fp, "\n");
	        fprintf(fp, "%%ifdef MAME_DEBUG\n");
		fprintf(fp, "		extern	_mame_debug\n");
		fprintf(fp, "		extern	_MAME_Debug\n");
	        fprintf(fp, "%%endif\n");
	}
	if (FALSE == bMameFormat)
	{
	        fprintf(fp, "_%scontextBegin:\n", basename);
		fprintf(fp, "\n");
		fprintf(fp, "; DO NOT CHANGE THE ORDER OF THE FOLLOWING REGISTERS!\n");
		fprintf(fp, "\n");
		fprintf(fp, "_%sBase	dd	0	; Base address for 6808 stuff\n", basename);
		fprintf(fp, "_%sMemRead	dd	0	; Offset of memory read structure array\n", basename);
		fprintf(fp, "_%sMemWrite	dd	0	; Offset of memory write structure array\n", basename);
		fprintf(fp, "_%saf	dw	0	; A register and flags\n", basename);
		fprintf(fp, "_%spc	dw	0	; 6808 Program counter\n", basename);
		fprintf(fp, "_%sx		dw	0	; Index register\n", basename);
		fprintf(fp, "_%ssp	dw	0	; Stack pointer\n", basename);
		fprintf(fp, "_%sb		db	0	; B register\n", basename);
		fprintf(fp, "_irqPending	db	0	; Non-zero if an IRQ is pending\n");
		fprintf(fp, "\n");
		fprintf(fp, "_%scontextEnd:\n", basename);
	}
	else   /* MAME 6808 Context */
	{
	        fprintf(fp, "_%scontextBegin:\n", basename);
		fprintf(fp, "\n");
		fprintf(fp, "; DO NOT CHANGE THE ORDER OF THE FOLLOWING REGISTERS!\n");
		fprintf(fp, "\n");
		fprintf(fp, "_%spc	dw	0	; 6808 Program counter\n", basename);
		fprintf(fp, "_%ssp	dw	0	; Stack pointer\n", basename);
		fprintf(fp, "_%sx		dw	0	; Index register\n", basename);
		fprintf(fp, "_%sa		db	0	; A register\n", basename);
		fprintf(fp, "_%sb		db	0	; B register\n", basename);
		fprintf(fp, "_%scc		db	0	; CC register\n", basename);
		fprintf(fp, "			db	0,0,0\n");
		fprintf(fp, "_%sirqPending	dd	0	; Non-zero if an IRQ is pending\n", basename);

		fprintf(fp, "\n");
		fprintf(fp, "_%scontextEnd:\n", basename);
	        fprintf(fp, "\n");
		fprintf(fp, "_%sBase	dd	0	; Base address for 6808 stuff\n", basename);
		fprintf(fp, "_%sMemRead	dd	0	; Offset of memory read structure array\n", basename);
		fprintf(fp, "_%sMemWrite	dd	0	; Offset of memory write structure array\n", basename);
		fprintf(fp, "_%saf	dw	0	; A register and flags\n", basename);
		fprintf(fp, "wNewPC	dw	0	; temporary PC for interrupts\n", basename);
	        fprintf(fp, "extern		_OP_RAM\n");
		fprintf(fp, "extern		_cpu_readmem16\n");
		fprintf(fp, "extern		_cpu_writemem16\n");
	}
	Alignment();

        if (FALSE != bMameFormat) fprintf(fp, "		global	%s\n\n", vcyclesRemaining);
	if (FALSE == bMameFormat) fprintf(fp, "dwElapsedTicks	dd 0	; Elapsed ticks!\n");
	else fprintf(fp, "dwRunCycles	dd 0	; cycles to execute\n");
	fprintf(fp, "%s	dd	0	; # Of cycles remaining\n", vcyclesRemaining);
	fprintf(fp, "_altFlags	db	0	; temporary flag storage\n");
	fprintf(fp, "\n");

	// Now we write out our tables

	Alignment();

	memset(&bUsed[0], 0, 0x100);

	// Create flag translation tables

	fprintf(fp,	"bit6808tox86:\n");

	for (dwLoop = 0; dwLoop < 256; dwLoop++)
	{
		bValue = 0;

		for (dwBit = 0; dwBit < 8; dwBit++)
		{
			if (bBit6808tox86[dwBit] != 0xff)
			{
				if ((1 << dwBit) & dwLoop)
					bValue |= (1 << bBit6808tox86[dwBit]);
			}
		}

		if ((dwLoop & 0x0f) == 0)
			fprintf(fp, "		db	");

		if (bValue < 0xa0)
			fprintf(fp, "%.2xh", bValue);
		else
			fprintf(fp, "0%.2xh", bValue);

		if ((dwLoop & 0x0f) != 0xf)
			fprintf(fp, ", ");
		else
			fprintf(fp, "\n");
	}

	// Create flag translation tables

	fprintf(fp,	"\nbitx86to6808:\n");

	for (dwLoop = 0; dwLoop < 256; dwLoop++)
	{
		bValue = 0;

		for (dwBit = 0; dwBit < 8; dwBit++)
		{
			if (bBitx86to6808[dwBit] != 0xff)
			{
				if ((1 << dwBit) & dwLoop)
					bValue |= (1 << bBitx86to6808[dwBit]);
			}
		}

		if ((dwLoop & 0x0f) == 0)
			fprintf(fp, "		db	");

		if (bValue < 0xa0)
			fprintf(fp, "%.2xh", bValue);
		else
			fprintf(fp, "0%.2xh", bValue);

		if ((dwLoop & 0x0f) != 0xf)
			fprintf(fp, ", ");
		else
			fprintf(fp, "\n");
	}
	fprintf(fp, "\n");

	// Now rip through and find out what is and isn't used

	dwLoop = 0;

	while (StandardOps[dwLoop].Emitter)
	{
		assert(StandardOps[dwLoop].bOpCode < 0x100);
		if (bUsed[StandardOps[dwLoop].bOpCode])
		{
			fprintf(stderr, "Oops! %.2x\n", StandardOps[dwLoop].bOpCode);
			exit(1);
		}
		if ((StandardOps[dwLoop].bCPUFlag&bCPUFlag)==StandardOps[dwLoop].bCPUFlag) bUsed[StandardOps[dwLoop].bOpCode] = 1;
		dwLoop++;
	}

	fprintf(stderr, "%ld Unique opcodes\n", dwLoop);

	// Now that that's taken care of, emit the table

	fprintf(fp, "%sregular:\n", basename);

	dwLoop = 0;


	while (dwLoop < 0x100)
	{
		fprintf(fp, "		dd	");
		if (bUsed[dwLoop])
			fprintf(fp, "RegInst%.2x", dwLoop);
		else
			fprintf(fp, "invalidInsByte");
		fprintf(fp, "\n");
		dwLoop++;
	}
	fprintf(fp, "\n");

}

CodeSegmentBegin()
{
        fprintf(fp, "		section	.text\n");
}

CodeSegmentEnd()
{
}

ProgramEnd()
{
	fprintf(fp, "		end\n");
}

EmitRegularInstructions()
{
	UINT32 dwLoop2 = 0;

	while (dwLoop < 0x100)
	{
		dwLoop2 = 0;

		while (StandardOps[dwLoop2].bOpCode != dwLoop && StandardOps[dwLoop2].bOpCode != 0xffff)
			dwLoop2++;

		assert(dwLoop2 < 0x100);
		sprintf(procname, "RegInst%.2x", dwLoop);
		if (StandardOps[dwLoop2].Emitter
			&& StandardOps[dwLoop2].bOpCode != 0xffff)
			StandardOps[dwLoop2].Emitter((UINT16) dwLoop);

		dwLoop++;
	}
}

/* These are the meta routines */

ExecCode()
{
	fprintf(fp, "		global	_%s%s\n", basename, fExec);
	fprintf(fp, "		global	%s%s_\n", basename, fExec);

	sprintf(procname, "%s%s_", basename, fExec);
	ProcBegin(0xffffffff);

	fprintf(fp, "_%s%s:\n", basename, fExec);

	if (bUseStack)
		fprintf(fp, "		mov	eax, [esp+4]	; Get our execution cycle count\n");

	fprintf(fp, "		push	ebx			; Save all registers we use\n");
	fprintf(fp, "		push	ecx\n");
	fprintf(fp, "		push	edx\n");
	fprintf(fp, "		push	ebp\n");
	fprintf(fp, "		push	esi\n");
	fprintf(fp, "		push	edi\n");
	fprintf(fp, "\n");

	if (bSingleStep)
		fprintf(fp, "		mov	eax, 2	; No longer cycles - now instructions!\n");

	fprintf(fp, "		mov	dword [%s], eax	; Store # of instructions to\n", vcyclesRemaining);
	if (FALSE == bMameFormat) fprintf(fp, "		mov	dword [dwElapsedTicks], 0\n");
	else fprintf(fp, "		mov	dword [dwRunCycles], eax	;\n");
	fprintf(fp, "		cld				; Go forward!\n");
	fprintf(fp, "\n");
	fprintf(fp, "		xor	eax, eax		; Zero EAX 'cause we use it!\n");
	fprintf(fp, "		xor	ebx, ebx		; Zero EBX, too\n");
	fprintf(fp, "		xor	ecx, ecx		; Zero ECX\n");
	fprintf(fp, "		xor	edx, edx		; And EDX\n");
	fprintf(fp, "		xor	edi, edi		; Zero EDI as well\n");
	fprintf(fp, "		xor	esi, esi		; Zero our source address\n");
	fprintf(fp, "\n");

	fprintf(fp, "		mov	bl, [ds:_%sb]	; Get B\n", basename);
	fprintf(fp, "		mov	cx, [ds:_%sx]	; Get our index register\n", basename);
	if (FALSE == bMameFormat)
        {
	        fprintf(fp, "		mov	ax, [ds:_%saf]	; Get our flags and accumulator\n", basename);
	fprintf(fp, "		xchg	ah, al\n");
	}
	else
	{
	        fprintf(fp, "		mov	ah, [ds:_%scc]	; Get our flags\n", basename);
	        fprintf(fp, "		mov	al, [ds:_%sa]	; Get our accumulator\n", basename);
	}
	fprintf(fp, "		mov	si, [ds:_%spc]	; Get our program counter\n", basename);
	fprintf(fp, "		mov	ebp, [ds:_%sBase]	; Get our base address register\n", basename);
	fprintf(fp, "		add	esi, ebp		; Add in our base address\n");

	Flags6808toX86();

	FetchNextInstruction();

	fprintf(fp, "invalidInsWord:\n");
	fprintf(fp, "		dec	esi\n");
	fprintf(fp, "\n");
	fprintf(fp, "; We get to invalidInsByte if it's a single byte invalid opcode\n");
	fprintf(fp, "\n");

	fprintf(fp, "invalidInsByte:\n");
	if (FALSE == bMameFormat) fprintf(fp, "		dec	esi			; Back up one instruction...\n");
	else
	{
	        fprintf(fp, "		sub	dword [%s], 1\n", vcyclesRemaining);
	}
	fprintf(fp, "		mov	edx, esi		; Get our address in EAX\n");
	fprintf(fp, "		sub	edx, ebp		; And subtract our base for\n");
	fprintf(fp, "						; an invalid instruction\n");
	fprintf(fp, "		jmp	short emulateEnd\n");
	fprintf(fp, "\n");
	fprintf(fp, "noMoreExec:\n");
	if (FALSE == bMameFormat) fprintf(fp, "		dec	esi\n");

	// Put any register storing here

	fprintf(fp, "		mov	edx, 80000000h		; Indicate successful exec\n");

	// Store all the registers here

	fprintf(fp, "emulateEnd:\n");

	if (FALSE != bMameFormat)
	{
	        fprintf(fp, "		mov	edx, dword [dwRunCycles]\n");
	        fprintf(fp, "		sub	edx, dword [%s]\n", vcyclesRemaining);
	}
	fprintf(fp, "		push	edx		; Save this for the return\n");
	FlagsX86to6808();
	fprintf(fp, "		mov	[ds:_%sb], bl	; Store B\n", basename);
	fprintf(fp, "		mov	[ds:_%sx], cx	; Store X\n", basename);
	if (FALSE == bMameFormat)
	{
	        fprintf(fp, "		xchg	ah, al\n");
		fprintf(fp, "		mov	[ds:_%saf], ax	; Store A & flags\n", basename);
	}
	else
	{
	        fprintf(fp, "		mov	[ds:_%scc], ah	; Store flags\n", basename);
		fprintf( fp, "		mov	[ds:_%sa], al	; Store A\n", basename);
	}
	fprintf(fp, "		sub	esi, ebp	; Get our PC back\n");
	fprintf(fp, "		mov	[ds:_%spc], si	; Store PC\n", basename);
	fprintf(fp, "		pop	edx		; Restore EDX for later\n");

	fprintf(fp, "\n");
	fprintf(fp, "		mov	eax, edx	; Get our result code\n");
	fprintf(fp, "		pop	edi			; Restore registers\n");
	fprintf(fp, "		pop	esi\n");
	fprintf(fp, "		pop	ebp\n");
	fprintf(fp, "		pop	edx\n");
	fprintf(fp, "		pop	ecx\n");
	fprintf(fp, "		pop	ebx\n");
	fprintf(fp, "\n");
	fprintf(fp, "		ret\n");
	fprintf(fp, "\n");
	ProcEnd(procname);
}

CpuInit()
{
	fprintf(fp, "		global	_%sinit\n", basename);
	fprintf(fp, "		global	%sinit_\n", basename);

	sprintf(procname, "%sinit_", basename);
	ProcBegin(0xffffffff);
	fprintf(fp, "_%sinit:\n", basename);
	fprintf(fp, "		ret\n");

	ProcEnd(procname);
}

MamePushAll()
{
	sprintf(procname, "%spushall_", basename);
	ProcBegin(0xffffffff);

	fprintf(fp, "		push	esi\n");
	fprintf(fp, "		push	edx\n");
	fprintf(fp, "		push	ebp\n");

	fprintf(fp, "		mov	cx, [_%sx]\n", basename);
	fprintf(fp, "		mov	bl, [_%sb]\n", basename);
	fprintf(fp, "		mov	ax, [_%saf]\n", basename);

	// Store our program counter

	fprintf(fp, "		mov	dx, [_%spc]\n", basename);
	PushWord();

	// Now our index register

	fprintf(fp, "		mov	dx, [_%sx]\n", basename);
	PushWord();

	// Now the B register

	PushByte("bl");

	// Now the A register

	PushByte("ah");

	// Now the flags

	PushByte("al");

	fprintf(fp, "		pop	ebp\n");
	fprintf(fp, "		pop	edx\n");
	fprintf(fp, "		pop	esi\n");
	fprintf(fp, "		ret\n");
	ProcEnd(procname);
}

StdPushAll()
{
	sprintf(procname, "%spushall_", basename);
	ProcBegin(0xffffffff);

	fprintf(fp, "		push	esi\n");
	fprintf(fp, "		push	edx\n");
	fprintf(fp, "		push	ebp\n");

	fprintf(fp, "		xor	esi, esi\n");
	fprintf(fp, "		mov	si, [_%ssp]\n", basename);
	fprintf(fp, "		mov	ebp, [ds:_%sBase]\n", basename);

	// Store our program counter

	fprintf(fp, "		dec	esi\n");
	fprintf(fp, "		mov	dx, [_%spc]\n", basename);
	fprintf(fp, "		xchg	dh, dl\n");
	fprintf(fp, "		mov	[ds:esi+ebp], dx\n");
	fprintf(fp, "		dec	esi\n");

	// Now our index register

	fprintf(fp, "		dec	esi\n");
	fprintf(fp, "		mov	dx, [_%sx]\n", basename);
	fprintf(fp, "		xchg	dh, dl\n");
	fprintf(fp, "		mov	[ds:esi+ebp], dx\n");
	fprintf(fp, "		dec	esi\n");

	// Now the B register

	fprintf(fp, "		mov	dl, [_%sb]\n", basename);
	fprintf(fp, "		mov	[ds:esi+ebp], dl\n");
	fprintf(fp, "		dec	esi\n");

	// Now the A register

	fprintf(fp, "		mov	dl, byte [_%saf + 1]\n", basename);
	fprintf(fp, "		mov	[ds:esi+ebp], dl\n");
	fprintf(fp, "		dec	esi\n");

	// Now the flags

	fprintf(fp, "		mov	dl, byte [_%saf]\n", basename);
	fprintf(fp, "		mov	[ds:esi+ebp], dl\n");
	fprintf(fp, "		dec	esi\n");

	// Put our stack pointer back now

	fprintf(fp, "		mov	[_%ssp], si	; Store it\n", basename);

	fprintf(fp, "		pop	ebp\n");
	fprintf(fp, "		pop	edx\n");
	fprintf(fp, "		pop	esi\n");
	fprintf(fp, "		ret\n");
	ProcEnd(procname);
}

PushAll()
{
	if (FALSE != bMameFormat) MamePushAll();
	else StdPushAll();
}

CauseIntCode()
{
	fprintf(fp, "		global	_%s_Cause_Interrupt\n", basename);
	fprintf(fp, "		global	%s_Cause_Interrupt_\n", basename);

	sprintf(procname, "%s_Cause_Interrupt_", basename);
	ProcBegin(0xffffffff);
	fprintf(fp, "_%s_Cause_Interrupt:\n", basename);

	fprintf(fp, "		mov	eax, [esp+4]	; Get our interrupt type mask\n");
        fprintf(fp, "		or	[ds:_%sirqPending], eax\n", basename);
	fprintf(fp, "		test	[ds:_%sirqPending], dword M6808_WAI	; are we waiting?\n", basename);
        fprintf(fp, "		jz	LeaveCauseInt	; Nope - skip out\n");
	fprintf(fp, "		test	[ds:_%sirqPending], dword M6808_INT_NMI	; NMI?\n", basename);
        fprintf(fp, "		jz	noNMINow	; Nope - skip out\n");
        fprintf(fp, "		and	[ds:_%sirqPending], dword ~M6808_WAI\n", basename);
	fprintf(fp, "		ret\n");
	fprintf(fp, "noNMINow:\n");
	fprintf(fp, "		test	[ds:_%scc], byte 0x10	; \n", basename);
        fprintf(fp, "		jz	LeaveCauseInt	; skip out\n");
        fprintf(fp, "		and	[ds:_%sirqPending], dword ~M6808_WAI\n", basename);
	fprintf(fp, "LeaveCauseInt:\n");
	fprintf(fp, "		ret\n");
}

ClearIntsCode()
{
	fprintf(fp, "		global	_%s_Clear_Pending_Interrupts\n", basename);
	fprintf(fp, "		global	%s_Clear_Pending_Interrupts_\n", basename);

	sprintf(procname, "%s_Clear_Pending_Interrupts_", basename);
	ProcBegin(0xffffffff);
	fprintf(fp, "_%s_Clear_Pending_Interrupts:\n", basename);

        fprintf(fp, "		and	[ds:_%sirqPending], dword ~( M6808_INT_IRQ | M6808_INT_NMI | M6808_INT_OCI )\n", basename);
	fprintf(fp, "		ret\n");
}

NmiCode()
{
	fprintf(fp, "		global	_%snmi\n", basename);
	fprintf(fp, "		global	%snmi_\n", basename);

	sprintf(procname, "%snmi_", basename);
	ProcBegin(0xffffffff);
	fprintf(fp, "_%snmi:\n", basename);

	fprintf(fp, "		xor	eax, eax		; Indicate that we took the NMI\n");
	fprintf(fp, "		ret\n");

	ProcEnd(procname);
}

IntCode()
{
	fprintf(fp, "		global	_%sint\n", basename);
	fprintf(fp, "		global	%sint_\n", basename);

	sprintf(procname, "%sint_", basename);
	ProcBegin(0xffffffff);
	fprintf(fp, "_%sint:\n", basename);

	// See if the interrupt can be taken

        fprintf(fp, "		test	[ds:_%saf], byte 10h	; Can we take interrupts?\n", basename);
        fprintf(fp, "		jnz	setPending	; Nope - make it pending\n");

	// It can be taken. Save off the registers

	fprintf(fp, "		call	%spushall_\n", basename);
	fprintf(fp, "		push	ebp\n");
	fprintf(fp, "		push	eax\n");
	fprintf(fp, "		mov	ebp, [ds:_%sBase]\n", basename);
	fprintf(fp, "		mov	ax, [ds:ebp+0fff8h]	; Get interrupt vector\n");
	fprintf(fp, "		xchg	ah, al\n");
	fprintf(fp, "		mov	[ds:_%spc], ax\n", basename);
	fprintf(fp, "		pop	eax\n");
	fprintf(fp, "		pop	ebp\n");
	fprintf(fp, "		add	[ds:dwElapsedTicks], dword 17\n");
        fprintf(fp, "		mov	[ds:_irqPending], byte 0\n");
        fprintf(fp, "		xor	eax, eax	; Indicate we took it\n");
	fprintf(fp, "		jmp	short intExit\n");

	// Set the pending byte

	fprintf(fp, "setPending:\n");
	fprintf(fp, "		mov	[ds:_irqPending], byte 1 ; We have a pending IRQ\n");
	fprintf(fp, "		mov	eax, -1	; We didn't take it!\n");
	fprintf(fp, "intExit:\n");
	fprintf(fp, "		ret\n");

	ProcEnd(procname);
}

MameIntCode()
{
	fprintf(fp, "		global	_%sint\n", basename);
	fprintf(fp, "		global	%sint_\n", basename);

	sprintf(procname, "%sint_", basename);
	ProcBegin(0xffffffff);
	fprintf(fp, "_%sint:\n", basename);

	// check for NMI

	fprintf(fp, "		test	[ds:_%sirqPending], dword M6808_INT_NMI\n", basename);
	fprintf(fp, "		jz	IntNotNMI	; Nope - not NMI\n");
	fprintf(fp, "		mov	[ds:wNewPC], word 0fffch\n");
	fprintf(fp, "		and	[ds:_%sirqPending], dword ~M6808_INT_NMI\n", basename);
        fprintf(fp, "		jmp	short TakeInt	; yes, do it\n");

	fprintf(fp, "IntNotNMI:	\n");

	// See if the interrupt can be taken

	fprintf(fp, "		test	[ds:_altFlags], byte 10h ; Can we take interrupts?\n", basename);
        fprintf(fp, "		jz	doIRQ	; yes, do it\n");
        fprintf(fp, "		ret\n");
	fprintf(fp, "doIRQ:	\n");

	// check for IRQ

	fprintf(fp, "		test	[ds:_%sirqPending], dword M6808_INT_IRQ\n", basename);
	fprintf(fp, "		jz	IntNotIRQ	; Nope - not IRQ\n");
	fprintf(fp, "		mov	[ds:wNewPC], word 0fff8h\n");
	fprintf(fp, "		and	[ds:_%sirqPending], dword ~M6808_INT_IRQ\n", basename);
	fprintf(fp, "		jmp	short TakeInt	; yes, do it\n");

	fprintf(fp, "IntNotIRQ:	\n");

	// check for OCI

	fprintf(fp, "		test	[ds:_%sirqPending], dword M6808_INT_OCI\n", basename);
	fprintf(fp, "		jz	IntNotOCI	; Nope - not OCI\n");
	fprintf(fp, "		mov	[ds:wNewPC], word 0fff4h\n");
	fprintf(fp, "		and	[ds:_%sirqPending], dword ~M6808_INT_OCI\n", basename);
        fprintf(fp, "		jmp	short TakeInt	; yes, do it\n");

	fprintf(fp, "IntNotOCI:	\n");

	fprintf(fp, "		ret\n");

	fprintf(fp, "TakeInt:	\n");
	fprintf(fp, "		sub	esi, ebp	; Get our PC back\n");
	fprintf(fp, "		mov	[ds:_%spc], si	; Store PC\n", basename);
	fprintf(fp, "		xor	edx, edx\n");
	fprintf(fp, "		mov	dl, ah\n");
	fprintf(fp, "		mov	ah, [ds:bitx86to6808+edx]\n");
	fprintf(fp, "		or		ah, [ds:_altFlags]\n");
        fprintf(fp, "		xchg	ah, al\n");
        fprintf(fp, "		mov	[ds:_%saf], ax	; Save Accumulator & flags\n", basename);
        fprintf(fp, "		mov	[ds:_%sb], bl	; And B\n", basename);
        fprintf(fp, "		mov	[ds:_%sx], cx	; And our index register\n", basename);

	// It can be taken. Save off the registers

	fprintf(fp, "		call	%spushall_\n", basename);
	fprintf(fp, "		push	ebp\n");
	fprintf(fp, "		push	edx\n");
	fprintf(fp, "		push	eax\n");
	fprintf(fp, "		mov	ebp, [ds:_%sBase]\n", basename);
	fprintf(fp, "		xor	edx, edx\n");
	fprintf(fp, "		mov	dx, [ds:wNewPC]	; Get interrupt vector\n");
	fprintf(fp, "		mov	ax, [ds:ebp+edx]	; Get interrupt vector\n");
	fprintf(fp, "		xchg	ah, al\n");
	fprintf(fp, "		mov	[ds:_%spc], ax\n", basename);
	fprintf(fp, "		pop	eax\n");
	fprintf(fp, "		pop	edx\n");
	fprintf(fp, "		pop	ebp\n");
	fprintf(fp, "		sub	dword [%s], 19\n", vcyclesRemaining);  //17?
//	fprintf(fp, "		mov	[ds:_%sirqPending], dword 0\n", basename);
        fprintf(fp, "		mov	ax, [ds:_%saf]	; Get our flags and accumulator\n", basename);
	fprintf(fp, "		xchg	ah, al\n");
	fprintf(fp, "		xor	esi, esi\n");
	fprintf(fp, "		mov	si, [ds:_%spc]	; Get our program counter\n", basename);
	fprintf(fp, "		mov	ebp, [ds:_%sBase]	; Get our base address register\n", basename);
	fprintf(fp, "		add	esi, ebp		; Add in our base address\n");
	Flags6808toX86();
	fprintf(fp, "		or	[ds:_altFlags], byte 10h\n");
	fprintf(fp, "		ret\n");

	ProcEnd(procname);
}

ResetCode()
{
	fprintf(fp, "		global	_%s%s\n", basename, fReset);
	fprintf(fp, "		global	%s%s_\n", basename, fReset);

	sprintf(procname, "%s%s_", basename, fReset);
	ProcBegin(0xffffffff);
	fprintf(fp, "_%s%s:\n", basename, fReset);

	fprintf(fp, "		push	ebp	; Save our important register\n");
	if (FALSE != bMameFormat)
	{
		fprintf(fp, "		mov	eax, [_OP_RAM]\n");
		fprintf(fp, "		mov	[ds:_%sBase], eax\n", basename);
	}
	fprintf(fp, "		xor	eax, eax\n");
	fprintf(fp, "		mov	ebp, [ds:_%sBase]\n", basename);
	fprintf(fp, "		mov	[ds:_%sx], ax\n", basename);
	fprintf(fp, "		mov	[ds:_%sb], al\n", basename);
	if (FALSE == bMameFormat) fprintf(fp, "		mov	[ds:_irqPending], al\n");
	else fprintf(fp, "		mov	[ds:_%sirqPending], eax\n", basename);
	fprintf(fp, "		mov	[ds:_%ssp], ax\n", basename);
	if (FALSE == bMameFormat) fprintf(fp, "		mov	[ds:_%saf], word 0014h\n", basename);
	else
	{
	        fprintf(fp, "		mov	[ds:_%sa], byte 00h\n", basename);
	        fprintf(fp, "		mov	[ds:_%scc], byte 14h\n", basename);
	}
	fprintf(fp, "		mov	ax, [ds:ebp + 0fffeh] ; Get reset address\n");
	fprintf(fp, "		xchg	ah, al\n");
	fprintf(fp, "		mov	[ds:_%spc], ax	; Our new program counter\n", basename);
	fprintf(fp,	"		pop	ebp\n");
	fprintf(fp, "		ret\n");
	fprintf(fp, "\n");
	ProcEnd(procname);
}

SetContextCode()
{
	fprintf(fp, "		global	_%s%s\n", basename, fSetContext);
	fprintf(fp, "		global	%s%s_\n", basename, fSetContext);

	sprintf(procname, "%s%s_", basename, fSetContext);
	ProcBegin(0xffffffff);
	fprintf(fp, "_%s%s:\n", basename, fSetContext);

	if (bUseStack)
		fprintf(fp, "		mov	eax, [esp+4]	; Get our context address\n");

	fprintf(fp, "		push	esi		; Save registers we use\n");
	fprintf(fp, "		push	edi\n");
	fprintf(fp, "		push	ecx\n");
	fprintf(fp, "		mov     ecx, _%scontextEnd - _%scontextBegin\n", basename, basename);
	fprintf(fp, "		mov	edi, _%scontextBegin\n", basename);
	fprintf(fp, "		mov	esi, eax	; Source address in ESI\n");
	fprintf(fp, "		rep	movsb		; Move it as fast as we can!\n");
	fprintf(fp, "		pop	ecx\n");
	fprintf(fp, "		pop	edi\n");
	fprintf(fp, "		pop	esi\n");
	fprintf(fp, "		ret			; No return code\n");
	ProcEnd(procname);
}

GetTicksCode()
{
	fprintf(fp, "		global	_%sGetElapsedTicks\n", basename);
	fprintf(fp, "		global	%sGetElapsedTicks_\n", basename);

	Alignment();
	sprintf(procname, "%sGetElapsedTicks_", basename);
	ProcBegin(0xffffffff);
	fprintf(fp, "_%sGetElapsedTicks:\n", basename);

	if (bUseStack)
		fprintf(fp, "		mov	eax, [esp+4]	; Get our context address\n");

	fprintf(fp, "		or	eax, eax	; Should we clear it?\n");
	fprintf(fp, "		jz	getTicks\n");
	fprintf(fp, "		xor	eax, eax\n");
	fprintf(fp, "		xchg	eax, [ds:dwElapsedTicks]\n");
	fprintf(fp, "		ret\n");
	fprintf(fp, "getTicks:\n");
	fprintf(fp, "		mov	eax, [ds:dwElapsedTicks]\n");
	fprintf(fp, "		ret\n");
}

ReleaseTimesliceCode()
{
	fprintf(fp, "		global	_%sReleaseTimeslice\n", basename);
	fprintf(fp, "		global	%sReleaseTimeslice_\n", basename);

	Alignment();
	sprintf(procname, "%sReleaseTimeslice_", basename);
	ProcBegin(0xffffffff);
	fprintf(fp, "_%sReleaseTimeslice:\n", basename);

	fprintf(fp, "		mov	[ds:%s], dword 1\n", vcyclesRemaining);
	fprintf(fp, "		ret\n\n");
}


GetContextCode()
{
	fprintf(fp, "		global	_%s%s\n", basename, fGetContext);
	fprintf(fp, "		global	%s%s_\n", basename, fGetContext);

	sprintf(procname, "%s%s_", basename, fGetContext);
	ProcBegin(0xffffffff);
	fprintf(fp, "_%s%s:\n", basename, fGetContext);

	if (bUseStack)
		fprintf(fp, "		mov	eax, [esp+4]	; Get our context address\n");

	fprintf(fp, "		push	esi		; Save registers we use\n");
	fprintf(fp, "		push	edi\n");
	fprintf(fp, "		push	ecx\n");
	fprintf(fp, "		mov     ecx, _%scontextEnd - _%scontextBegin\n", basename, basename);
	fprintf(fp, "		mov	esi, _%scontextBegin\n", basename);
	fprintf(fp, "		mov	edi, eax	; Source address in ESI\n");
	fprintf(fp, "		rep	movsb		; Move it as fast as we can!\n");
	fprintf(fp, "		pop	ecx\n");
	fprintf(fp, "		pop	edi\n");
	fprintf(fp, "		pop	esi\n");
	fprintf(fp, "		ret			; No return code\n");
	ProcEnd(procname);
}

GetContextSizeCode()
{
	fprintf(fp, "		global	_%sGetContextSize\n", basename);
	fprintf(fp, "		global	%sGetContextSize_\n", basename);

	sprintf(procname, "%sGetContextSize_", basename);
	ProcBegin(0xffffffff);
	fprintf(fp, "_%sGetContextSize:\n", basename);
	fprintf(fp, "		mov     eax, _%scontextEnd - _%scontextBegin\n", basename, basename);
	fprintf(fp, "		ret\n");
	ProcEnd(procname);
}

GetPCCode()
{
	fprintf(fp, "		global	_%s_GetPC\n", basename);
	fprintf(fp, "		global	%s_GetPC_\n", basename);

	sprintf(procname, "%s_GetPC_", basename);
	ProcBegin(0xffffffff);
	fprintf(fp, "_%s_GetPC:\n", basename);
	fprintf(fp, "		xor     eax, eax\n");
	fprintf(fp, "		mov     ax, [ds:_%spc]\n", basename);
	fprintf(fp, "		ret\n");
}

MameDefines()
{
	fprintf(fp, "		%%define M6808_INT_NONE  0\n");
	fprintf(fp, "		%%define M6808_INT_IRQ   1\n");
	fprintf(fp, "		%%define M6808_INT_NMI   2\n");
	fprintf(fp, "		%%define M6808_INT_OCI   4\n");
	fprintf(fp, "		%%define M6808_WAI       8\n");
	CallMameDebugCode();
}

EmitCode()
{
	CodeSegmentBegin();
	ReadMemoryByteHandler();
	WriteMemoryByteHandler();
	EmitRegularInstructions();
	if (FALSE != bMameFormat) GetPCCode();
	GetContextCode();
	SetContextCode();
	if (FALSE == bMameFormat) GetContextSizeCode();
	if (FALSE == bMameFormat) GetTicksCode();
	if (FALSE == bMameFormat) ReleaseTimesliceCode();
	CpuInit();
	ResetCode();
	PushAll();
	if (FALSE == bMameFormat) IntCode(); else MameIntCode();
	if (FALSE != bMameFormat) CauseIntCode();
	if (FALSE != bMameFormat) ClearIntsCode();
	if (FALSE == bMameFormat) NmiCode();
	ExecCode();
	CodeSegmentEnd();
}

main(int argc, char **argv)
{
	UINT32 dwLoop = 0;

	printf("Make6808 - V%s - Copyright 1998, Neil Bradley (neil@synthcom.com)\n", VERSION);
	printf("         - modified for MAME by Alex Pasadyn (ajp@mail.utexas.edu)\n");

	if (argc < 2)
	{
		printf("Usage: %s outfile [option1] [option2] ....\n", argv[0]);
		printf("\n  -ss - Create m6808 to execute one instruction per m6808exec()\n");
		printf("  -s  - Stack calling conventions (DJGPP, MSVC, Borland)\n");
		printf("  -m  - Make adjustments for MAME\n");
		printf("  -3  - Support M6803 instructions\n");
		printf("  -h  - Support HD63701 instructions\n");
		exit(1);
	}

        strcpy(vcyclesRemaining, "cyclesRemaining");
	strcpy(fGetContext, "GetContext");
	strcpy(fSetContext, "SetContext");
	strcpy(fReset, "reset");
	strcpy(fExec, "exec");
	bCPUFlag = CPU_M6808;
	dwLoop = 1;

	while (dwLoop < argc)
	{
		if (strcmp("-ss", argv[dwLoop]) == 0 || strcmp("-SS", argv[dwLoop]) == 0)
			bSingleStep = 1;
		if (strcmp("-s", argv[dwLoop]) == 0 || strcmp("-S", argv[dwLoop]) == 0)
			bUseStack = 1;
		if (strcmp("-m", argv[dwLoop]) == 0 || strcmp("-M", argv[dwLoop]) == 0)
		{
		        bMameFormat = 1;
			bUseStack = 1;
			strcpy(vcyclesRemaining, "_m6808_ICount");
			strcpy(fGetContext, "_GetRegs");
			strcpy(fSetContext, "_SetRegs");
			strcpy(fReset, "_reset");
			strcpy(fExec, "_execute");
		}
		if (strcmp("-3", argv[dwLoop]) == 0)
		        bCPUFlag |= CPU_M6803;
		if (strcmp("-h", argv[dwLoop]) == 0 || strcmp("-H", argv[dwLoop]) == 0)
		        bCPUFlag |= CPU_HD63701;
		dwLoop++;
	}

	strcpy(basename, "m6808");

	for (dwLoop = 1; dwLoop < argc; dwLoop++)
		if (argv[dwLoop][0] != '-')
		{
			fp = fopen(argv[dwLoop], "w");
			break;
		}

	if ((FALSE != bMameFormat) && (FALSE != bSingleStep))
	{
	        fprintf(stderr, "Error: -m and -ss are not supported at the same time\n");
	}
	if (NULL == fp)
	{
		fprintf(stderr, "Can't open %s for writing\n", argv[1]);
		exit(1);
	}

	StandardHeader();
	if (FALSE != bMameFormat) MameDefines();
	DataSegment();
	EmitCode();
	ProgramEnd();

	fclose(fp);
}
