/*****************************************************************************
 *
 *	 sh2.c
 *	 Portable Hitachi SH-2 (SH7600 family) emulator
 *
 *	 Copyright (c) 2000 Juergen Buchmueller <pullmoll@t-online.de>,
 *	 all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   pullmoll@t-online.de
 *	 - The author of this copywritten work reserves the right to change the
 *	   terms of its usage and license at any time, including retroactively
 *	 - This entire notice must remain in the source code.
 *
 *	This work is based on <tiraniddo@hotmail.com> C/C++ implementation of
 *	the SH-2 CPU core and was adapted to the MAME CPU core requirements.
 *	Thanks also go to Chuck Mason <chukjr@sundail.net> and Olivier Galibert
 *	<galibert@pobox.com> for letting me peek into their SEMU code :-)
 *
 *****************************************************************************/

/*****************************************************************************
	Changes
	20010207 Sylvain Glaize (mokona@puupuu.org)

	- Bug fix in INLINE void MOVBM(UINT32 m, UINT32 n) (see comment)
	- Support of full 32 bit addressing (RB, RW, RL and WB, WW, WL functions)
		reason : when the two high bits of the address are set, access is
		done directly in the cache data array. The SUPER KANEKO NOVA SYSTEM
		sets the stack pointer here, using these addresses as usual RAM access.

		No real cache support has been added.
	- Read/Write memory format correction (_bew to _bedw) (see also SH2
		definition in cpuintrf.c and DasmSH2(..) in sh2dasm.c )

 *****************************************************************************/

#include <stdio.h>
#include <signal.h>
#include "driver.h"
#include "state.h"
#include "mamedbg.h"
#include "sh2.h"
#include "memory.h"

/* speed up delay loops, bail out of tight loops */
#define BUSY_LOOP_HACKS 	1

#define VERBOSE 1

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

#define SH2_CAS_LATENCY_1_16b	0xffff8426
#define SH2_CAS_LATENCY_2_16b	0xffff8446
#define SH2_CAS_LATENCY_3_16b	0xffff8466
#define SH2_CAS_LATENCY_1_32b	0xffff8826
#define SH2_CAS_LATENCY_2_32b	0xffff8846
#define SH2_CAS_LATENCY_3_32b	0xffff8866
#define SH2_TIER				0xfffffe10
#define SH2_IRPB				0xfffffe60
#define SH2_VCRA				0xfffffe62
#define SH2_VCRB				0xfffffe64
#define SH2_VCRC				0xfffffe66
#define SH2_VCRD				0xfffffe68
#define SH2_CCR 				0xfffffe92
#define SH2_ICR 				0xfffffee0
#define SH2_IRPA				0xfffffee2
#define SH2_VCRWDT				0xfffffee4
#define SH2_DVSR				0xffffff00
#define SH2_DVDNT				0xffffff04
#define SH2_DVCR				0xffffff08
#define SH2_VCRDIV				0xffffff0c
#define SH2_DVDNTH				0xffffff10
#define SH2_DVDNTL				0xffffff14
#define SH2_VCRDMA0 			0xffffffa0
#define SH2_VCRDMA1 			0xffffffa8
#define SH2_BCR1				0xffffffe0
#define SH2_BCR2				0xffffffe4
#define SH2_WCR 				0xffffffe8
#define SH2_MCR 				0xffffffec
#define SH2_RTCSR				0xfffffff0
#define SH2_RTCNT				0xfffffff4
#define SH2_RTCOR				0xfffffff8

typedef struct {
	UINT32	ppc;
	UINT32	pc;
	UINT32	pr;
	UINT32	sr;
	UINT32	gbr, vbr;
	UINT32	mach, macl;
	UINT32	r[16];
	UINT32	ea;
	UINT32	delay;
	UINT32	cpu_off;
	UINT32	dvsr, dvdnth, dvdntl, dvcr;
	UINT32	pending_irq;
	INT8	irq_line_state[16];
	int 	(*irq_callback)(int irqline);
	UINT8	*m;
}	SH2;

int sh2_icount;
static SH2 sh2;

#define T	0x00000001
#define S	0x00000002
#define I	0x000000f0
#define Q	0x00000100
#define M	0x00000200

#define AM	0x07ffffff

#define FLAGS	(M|Q|I|S|T)

#define Rn	((opcode>>8)&15)
#define Rm	((opcode>>4)&15)

INLINE data8_t RB(offs_t A)
{
	if (A >= 0xe0000000)
		return sh2_internal_r(A);

	if (A >= 0xc0000000)
		return cpu_readmem32bedw(A);

	if (A >= 0x40000000)
		return 0xa5;

	return cpu_readmem32bedw(A & AM);
}

INLINE data16_t RW(offs_t A)
{
	if (A >= 0xe0000000)
		return (sh2_internal_r(A) << 8) | sh2_internal_r(A+1);

	if (A >= 0xc0000000)
		return cpu_readmem32bedw_word(A);

	if (A >= 0x40000000)
		return 0xa5a5;

	return cpu_readmem32bedw_word(A & AM);
}

INLINE data32_t RL(offs_t A)
{
	if (A >= 0xe0000000)
		return (sh2_internal_r(A+0) << 24) |
			   (sh2_internal_r(A+1) << 16) |
			   (sh2_internal_r(A+2) <<	8) |
			   (sh2_internal_r(A+3) <<	0);

	if (A >= 0xc0000000)
		return cpu_readmem32bedw_dword(A);

	if (A >= 0x40000000)
		return 0xa5a5a5a5;

	return cpu_readmem32bedw_dword(A & AM);
}

INLINE void WB(offs_t A, data8_t V)
{
	if (A >= 0xe0000000)
	{
		sh2_internal_w(A,V);
		return;
	}

	if (A >= 0xc0000000)
	{
		cpu_writemem32bedw(A,V);
		return;
	}

	if (A >= 0x40000000)
		return;

	cpu_writemem32bedw(A & AM,V);
}

INLINE void WW(offs_t A, data16_t V)
{
	if (A >= 0xe0000000)
	{
		sh2_internal_w(A+0,(V >> 8) & 0xff);
		sh2_internal_w(A+1,(V >> 0) & 0xff);
		return;
	}

	if (A >= 0xc0000000)
	{
		cpu_writemem32bedw_word(A,V);
		return;
	}

	if (A >= 0x40000000)
		return;

	cpu_writemem32bedw_word(A & AM,V);
}

INLINE void WL(offs_t A, data32_t V)
{
	if (A >= 0xe0000000)
	{
		sh2_internal_w(A+0,(V >> 24) & 0xff);
		sh2_internal_w(A+1,(V >> 16) & 0xff);
		sh2_internal_w(A+2,(V >>  8) & 0xff);
		sh2_internal_w(A+3,(V >>  0) & 0xff);
		return;
	}

	if (A >= 0xc0000000)
	{
		cpu_writemem32bedw_dword(A,V);
		return;
	}

	if (A >= 0x40000000)
		return;

	cpu_writemem32bedw_dword(A & AM,V);
}

INLINE void sh2_exception(char *message, int irqline)
{
	int vector;

	if (irqline <= ((sh2.sr >> 4) & 15))
		return;

    vector = (*sh2.irq_callback)(irqline);
	if (sh2.m[(SH2_ICR+1) & 0x1ff] & 1)
	{
        vector = 0x40 + irqline / 2;
		LOG(("SH-2 #%d exception #%d (autovector: $%x) after [%s]\n", cpu_getactivecpu(), irqline, vector, message));
    }
	else
	{
		LOG(("SH-2 #%d exception #%d (scu vector: $%x) after [%s]\n", cpu_getactivecpu(), irqline, vector, message));
	}
	sh2.r[15] -= 4;
	WL( sh2.r[15], sh2.sr );		/* push SR onto stack */
	sh2.r[15] -= 4;
	WL( sh2.r[15], sh2.pc );		/* push PC onto stack */

    /* set I flags in SR */
	if (irqline > SH2_INT_15)
		sh2.sr = sh2.sr | I;
	else
		sh2.sr = (sh2.sr & ~I) | (irqline << 4);

    /* fetch PC */
	sh2.pc = RL( sh2.vbr + vector * 4 );
	change_pc32bew(sh2.pc & AM);
}

#define CHECK_PENDING_IRQ(message)				\
{												\
	int irq = -1;								\
	if (sh2.pending_irq & (1 <<  0)) irq =	0;	\
	if (sh2.pending_irq & (1 <<  1)) irq =	1;	\
	if (sh2.pending_irq & (1 <<  2)) irq =	2;	\
	if (sh2.pending_irq & (1 <<  3)) irq =	3;	\
	if (sh2.pending_irq & (1 <<  4)) irq =	4;	\
	if (sh2.pending_irq & (1 <<  5)) irq =	5;	\
	if (sh2.pending_irq & (1 <<  6)) irq =	6;	\
	if (sh2.pending_irq & (1 <<  7)) irq =	7;	\
	if (sh2.pending_irq & (1 <<  8)) irq =	8;	\
	if (sh2.pending_irq & (1 <<  9)) irq =	9;	\
	if (sh2.pending_irq & (1 << 10)) irq = 10;	\
	if (sh2.pending_irq & (1 << 11)) irq = 11;	\
	if (sh2.pending_irq & (1 << 12)) irq = 12;	\
	if (sh2.pending_irq & (1 << 13)) irq = 13;	\
	if (sh2.pending_irq & (1 << 14)) irq = 14;	\
	if (sh2.pending_irq & (1 << 15)) irq = 15;	\
	if (irq >= 0)								\
		sh2_exception(message,irq); 			\
}

/* Layout of the registers in the debugger */
static UINT8 sh2_reg_layout[] = {
	SH2_PC, 	SH2_SP, 	SH2_SR, 	SH2_PR,  -1,
	SH2_GBR,	SH2_VBR,	SH2_MACH,	SH2_MACL,-1,
	SH2_R0, 	SH2_R1, 	SH2_R2, 	SH2_R3,  -1,
	SH2_R4, 	SH2_R5, 	SH2_R6, 	SH2_R7,  -1,
	SH2_R8, 	SH2_R9, 	SH2_R10,	SH2_R11, -1,
	SH2_R12,	SH2_R13,	SH2_R14,	SH2_EA,   0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 sh2_win_layout[] = {
	 0, 0,80, 6,	/* register window (top rows) */
	 0, 7,39,15,	/* disassembler window	*/
	40, 7,39, 7,	/* memory #1 window (left) */
	40,15,39, 7,	/* memory #2 window (right) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

/*	code				 cycles  t-bit
 *	0011 nnnn mmmm 1100  1		 -
 *	ADD 	Rm,Rn
 */
INLINE void ADD(UINT32 m, UINT32 n)
{
	sh2.r[n] += sh2.r[m];
}

/*	code				 cycles  t-bit
 *	0111 nnnn iiii iiii  1		 -
 *	ADD 	#imm,Rn
 */
INLINE void ADDI(UINT32 i, UINT32 n)
{
	sh2.r[n] += (INT32)(INT16)(INT8)i;
}

/*	code				 cycles  t-bit
 *	0011 nnnn mmmm 1110  1		 carry
 *	ADDC	Rm,Rn
 */
INLINE void ADDC(UINT32 m, UINT32 n)
{
	UINT32 tmp0, tmp1;

	tmp1 = sh2.r[n] + sh2.r[m];
	tmp0 = sh2.r[n];
	sh2.r[n] = tmp1 + (sh2.sr & T);
	if (tmp0 > tmp1)
		sh2.sr |= T;
	else
		sh2.sr &= ~T;
	if (tmp1 > sh2.r[n])
		sh2.sr |= T;
}

/*	code				 cycles  t-bit
 *	0011 nnnn mmmm 1111  1		 overflow
 *	ADDV	Rm,Rn
 */
INLINE void ADDV(UINT32 m, UINT32 n)
{
	INT32 dest, src, ans;

	if ((INT32) sh2.r[n] >= 0)
		dest = 0;
	else
		dest = 1;
	if ((INT32) sh2.r[m] >= 0)
		src = 0;
	else
		src = 1;
	src += dest;
	sh2.r[n] += sh2.r[m];
	if ((INT32) sh2.r[n] >= 0)
		ans = 0;
	else
		ans = 1;
	ans += dest;
	if (src == 0 || src == 2)
	{
		if (ans == 1)
			sh2.sr |= T;
		else
			sh2.sr &= ~T;
	}
	else
		sh2.sr &= ~T;
}

/*	code				 cycles  t-bit
 *	0010 nnnn mmmm 1001  1		 -
 *	AND 	Rm,Rn
 */
INLINE void AND(UINT32 m, UINT32 n)
{
	sh2.r[n] &= sh2.r[m];
}


/*	code				 cycles  t-bit
 *	1100 1001 iiii iiii  1		 -
 *	AND 	#imm,R0
 */
INLINE void ANDI(UINT32 i)
{
	sh2.r[0] &= i;
}

/*	code				 cycles  t-bit
 *	1100 1101 iiii iiii  1		 -
 *	AND.B	#imm,@(R0,GBR)
 */
INLINE void ANDM(UINT32 i)
{
	UINT32 temp;

	sh2.ea = sh2.gbr + sh2.r[0];
	temp = i & RB( sh2.ea );
	WB( sh2.ea, temp );
	sh2_icount -= 2;
}

/*	code				 cycles  t-bit
 *	1000 1011 dddd dddd  3/1	 -
 *	BF		disp8
 */
INLINE void BF(UINT32 d)
{
	if ((sh2.sr & T) == 0)
	{
		INT32 disp = ((INT32)d << 24) >> 24;
		sh2.pc = sh2.ea = sh2.pc + disp * 2 + 2;
		change_pc32bew(sh2.pc & AM);
		sh2_icount -= 2;
	}
}

/*	code				 cycles  t-bit
 *	1000 1111 dddd dddd  3/1	 -
 *	BFS 	disp8
 */
INLINE void BFS(UINT32 d)
{
	UINT32 temp = sh2.pc;
	if ((sh2.sr & T) == 0)
	{
		INT32 disp = ((INT32)d << 24) >> 24;
		sh2.pc = sh2.ea = sh2.pc + disp * 2 + 2;
		change_pc32bew(sh2.pc & AM);
		sh2.delay = temp;
		sh2_icount--;
	}
}

/*	code				 cycles  t-bit
 *	1010 dddd dddd dddd  2		 -
 *	BRA 	disp12
 */
INLINE void BRA(UINT32 d)
{
	INT32 disp = ((INT32)d << 20) >> 20;

#if BUSY_LOOP_HACKS
	if (disp == -2)
	{
		UINT32 next_opcode = RW(sh2.ppc & AM);
		/* BRA	$
		 * NOP
		 */
		if (next_opcode == 0x0009)
			sh2_icount %= 3;	/* cycles for BRA $ and NOP taken (3) */
	}
#endif
	sh2.pc = sh2.ea = sh2.pc + disp * 2 + 2;
	change_pc32bew(sh2.pc & AM);
	sh2.delay = sh2.ppc;
	sh2_icount--;
}

/*	code				 cycles  t-bit
 *	0000 mmmm 0010 0011  2		 -
 *	BRAF	Rm
 */
INLINE void BRAF(UINT32 m)
{
	sh2.pc += sh2.r[m] + 2;
	change_pc32bew(sh2.pc & AM);
	sh2.delay = sh2.ppc;
	sh2_icount--;
}

/*	code				 cycles  t-bit
 *	1011 dddd dddd dddd  2		 -
 *	BSR 	disp12
 */
INLINE void BSR(UINT32 d)
{
	INT32 disp = ((INT32)d << 20) >> 20;

	sh2.pr = sh2.pc;
	sh2.pc = sh2.ea = sh2.pc + disp * 2 + 2;
	change_pc32bew(sh2.pc & AM);
	sh2.delay = sh2.pr;
	sh2.pr += 2;
	sh2_icount--;
}

/*	code				 cycles  t-bit
 *	0000 mmmm 0000 0011  2		 -
 *	BSRF	Rm
 */
INLINE void BSRF(UINT32 m)
{
	sh2.pr = sh2.pc;
	sh2.pc += sh2.r[m] + 2;
	change_pc32bew(sh2.pc & AM);
	sh2.delay = sh2.pr;
	sh2.pr += 2;
	sh2_icount--;
}

/*	code				 cycles  t-bit
 *	1000 1001 dddd dddd  3/1	 -
 *	BT		disp8
 */
INLINE void BT(UINT32 d)
{
	if ((sh2.sr & T) != 0)
	{
		INT32 disp = ((INT32)d << 24) >> 24;
		sh2.pc = sh2.ea = sh2.pc + disp * 2 + 2;
		change_pc32bew(sh2.pc & AM);
		sh2_icount -= 2;
	}
}

/*	code				 cycles  t-bit
 *	1000 1101 dddd dddd  2/1	 -
 *	BTS 	disp8
 */
INLINE void BTS(UINT32 d)
{
	if ((sh2.sr & T) != 0)
	{
		INT32 disp = ((INT32)d << 24) >> 24;
		sh2.pc = sh2.ea = sh2.pc + disp * 2 + 2;
		change_pc32bew(sh2.pc & AM);
		sh2.delay = sh2.ppc;
		sh2_icount--;
	}
}

/*	code				 cycles  t-bit
 *	0000 0000 0010 1000  1		 -
 *	CLRMAC
 */
INLINE void CLRMAC(void)
{
	sh2.mach = 0;
	sh2.macl = 0;
}

/*	code				 cycles  t-bit
 *	0000 0000 0000 1000  1		 -
 *	CLRT
 */
INLINE void CLRT(void)
{
	sh2.sr &= ~T;
}

/*	code				 cycles  t-bit
 *	0011 nnnn mmmm 0000  1		 comparison result
 *	CMP_EQ	Rm,Rn
 */
INLINE void CMPEQ(UINT32 m, UINT32 n)
{
	if (sh2.r[n] == sh2.r[m])
		sh2.sr |= T;
	else
		sh2.sr &= ~T;
}

/*	code				 cycles  t-bit
 *	0011 nnnn mmmm 0011  1		 comparison result
 *	CMP_GE	Rm,Rn
 */
INLINE void CMPGE(UINT32 m, UINT32 n)
{
	if ((INT32) sh2.r[n] >= (INT32) sh2.r[m])
		sh2.sr |= T;
	else
		sh2.sr &= ~T;
}

/*	code				 cycles  t-bit
 *	0011 nnnn mmmm 0111  1		 comparison result
 *	CMP_GT	Rm,Rn
 */
INLINE void CMPGT(UINT32 m, UINT32 n)
{
	if ((INT32) sh2.r[n] > (INT32) sh2.r[m])
		sh2.sr |= T;
	else
		sh2.sr &= ~T;
}

/*	code				 cycles  t-bit
 *	0011 nnnn mmmm 0110  1		 comparison result
 *	CMP_HI	Rm,Rn
 */
INLINE void CMPHI(UINT32 m, UINT32 n)
{
	if ((UINT32) sh2.r[n] > (UINT32) sh2.r[m])
		sh2.sr |= T;
	else
		sh2.sr &= ~T;
}

/*	code				 cycles  t-bit
 *	0011 nnnn mmmm 0010  1		 comparison result
 *	CMP_HS	Rm,Rn
 */
INLINE void CMPHS(UINT32 m, UINT32 n)
{
	if ((UINT32) sh2.r[n] >= (UINT32) sh2.r[m])
		sh2.sr |= T;
	else
		sh2.sr &= ~T;
}


/*	code				 cycles  t-bit
 *	0100 nnnn 0001 0101  1		 comparison result
 *	CMP_PL	Rn
 */
INLINE void CMPPL(UINT32 n)
{
	if ((INT32) sh2.r[n] > 0)
		sh2.sr |= T;
	else
		sh2.sr &= ~T;
}

/*	code				 cycles  t-bit
 *	0100 nnnn 0001 0001  1		 comparison result
 *	CMP_PZ	Rn
 */
INLINE void CMPPZ(UINT32 n)
{
	if ((INT32) sh2.r[n] >= 0)
		sh2.sr |= T;
	else
		sh2.sr &= ~T;
}

/*	code				 cycles  t-bit
 *	0010 nnnn mmmm 1100  1		 comparison result
 * CMP_STR	Rm,Rn
 */
INLINE void CMPSTR(UINT32 m, UINT32 n)
{
	UINT32 temp;
	INT32 HH, HL, LH, LL;

	temp = sh2.r[n] ^ sh2.r[m];
	HH = (temp >> 12) & 0xff;
	HL = (temp >> 8) & 0xff;
	LH = (temp >> 4) & 0xff;
	LL = temp & 0xff;
	if (HH && HL && LH && LL)
		sh2.sr &= ~T;
	else
		sh2.sr |= T;
}

/*	code				 cycles  t-bit
 *	1000 1000 iiii iiii  1		 comparison result
 *	CMP/EQ #imm,R0
 */
INLINE void CMPIM(UINT32 i)
{
	UINT32 imm = (UINT32)(INT32)(INT16)(INT8)i;

	if (sh2.r[0] == imm)
		sh2.sr |= T;
	else
		sh2.sr &= ~T;
}

/*	code				 cycles  t-bit
 *	0010 nnnn mmmm 0111  1		 calculation result
 *	DIV0S	Rm,Rn
 */
INLINE void DIV0S(UINT32 m, UINT32 n)
{
	if ((sh2.r[n] & 0x80000000) == 0)
		sh2.sr &= ~Q;
	else
		sh2.sr |= Q;
	if ((sh2.r[m] & 0x80000000) == 0)
		sh2.sr &= ~M;
	else
		sh2.sr |= M;
	if ((sh2.r[m] ^ sh2.r[n]) & 0x80000000)
		sh2.sr |= T;
	else
		sh2.sr &= ~T;
}

/*	code				 cycles  t-bit
 *	0000 0000 0001 1001  1		 0
 *	DIV0U
 */
INLINE void DIV0U(void)
{
	sh2.sr &= ~(M | Q | T);
}

/*	code				 cycles  t-bit
 *	0011 nnnn mmmm 0100  1		 calculation result
 *	DIV1 Rm,Rn
 */
INLINE void DIV1(UINT32 m, UINT32 n)
{
	UINT32 tmp0;
	UINT32 old_q;

	old_q = sh2.sr & Q;
	if (0x80000000 & sh2.r[n])
		sh2.sr |= Q;
	else
		sh2.sr &= ~Q;
	sh2.r[n] = (sh2.r[n] << 1) | (sh2.sr & T);
	if (old_q)
	{
		if (sh2.sr & M)
		{
			tmp0 = sh2.r[n];
			sh2.r[n] -= sh2.r[m];
			if (sh2.r[n] > tmp0)
				sh2.sr &= ~Q;
			else
				sh2.sr |= Q;
		}
		else
		{
			tmp0 = sh2.r[n];
			sh2.r[n] += sh2.r[m];
			if (sh2.r[n] < tmp0)
				sh2.sr &= ~Q;
			else
				sh2.sr |= Q;
		}
	}
	else
	{
		if (sh2.sr & M)
		{
			tmp0 = sh2.r[n];
			sh2.r[n] += sh2.r[m];
			if (sh2.r[n] < tmp0)
				sh2.sr &= ~Q;
			else
				sh2.sr |= Q;
		}
		else
		{
			tmp0 = sh2.r[n];
			sh2.r[n] -= sh2.r[m];
			if (sh2.r[n] > tmp0)
				sh2.sr &= ~Q;
			else
				sh2.sr |= Q;
		}
	}
	sh2.sr = (sh2.sr & ~T) | (((sh2.sr >> 8) ^ (sh2.sr >> 9) ^ T) & T);
}

/*	DMULS.L Rm,Rn */
INLINE void DMULS(UINT32 m, UINT32 n)
{
	UINT32 RnL, RnH, RmL, RmH, Res0, Res1, Res2;
	UINT32 temp0, temp1, temp2, temp3;
	INT32 tempm, tempn, fnLmL;

	tempn = (INT32) sh2.r[n];
	tempm = (INT32) sh2.r[m];
	if (tempn < 0)
		tempn = 0 - tempn;
	if (tempm < 0)
		tempm = 0 - tempm;
	if ((INT32) (sh2.r[n] ^ sh2.r[m]) < 0)
		fnLmL = -1;
	else
		fnLmL = 0;
	temp1 = (UINT32) tempn;
	temp2 = (UINT32) tempm;
	RnL = temp1 & 0x0000ffff;
	RnH = (temp1 >> 16) & 0x0000ffff;
	RmL = temp2 & 0x0000ffff;
	RmH = (temp2 >> 16) & 0x0000ffff;
	temp0 = RmL * RnL;
	temp1 = RmH * RnL;
	temp2 = RmL * RnH;
	temp3 = RmH * RnH;
	Res2 = 0;
	Res1 = temp1 + temp2;
	if (Res1 < temp1)
		Res2 += 0x00010000;
	temp1 = (Res1 << 16) & 0xffff0000;
	Res0 = temp0 + temp1;
	if (Res0 < temp0)
		Res2++;
	Res2 = Res2 + ((Res1 >> 16) & 0x0000ffff) + temp3;
	if (fnLmL < 0)
	{
		Res2 = ~Res2;
		if (Res0 == 0)
			Res2++;
		else
			Res0 = (~Res0) + 1;
	}
	sh2.mach = Res2;
	sh2.macl = Res0;
	sh2_icount--;
}

/*	DMULU.L Rm,Rn */
INLINE void DMULU(UINT32 m, UINT32 n)
{
	UINT32 RnL, RnH, RmL, RmH, Res0, Res1, Res2;
	UINT32 temp0, temp1, temp2, temp3;

	RnL = sh2.r[n] & 0x0000ffff;
	RnH = (sh2.r[n] >> 16) & 0x0000ffff;
	RmL = sh2.r[m] & 0x0000ffff;
	RmH = (sh2.r[m] >> 16) & 0x0000ffff;
	temp0 = RmL * RnL;
	temp1 = RmH * RnL;
	temp2 = RmL * RnH;
	temp3 = RmH * RnH;
	Res2 = 0;
	Res1 = temp1 + temp2;
	if (Res1 < temp1)
		Res2 += 0x00010000;
	temp1 = (Res1 << 16) & 0xffff0000;
	Res0 = temp0 + temp1;
	if (Res0 < temp0)
		Res2++;
	Res2 = Res2 + ((Res1 >> 16) & 0x0000ffff) + temp3;
	sh2.mach = Res2;
	sh2.macl = Res0;
	sh2_icount--;
}

/*	DT		Rn */
INLINE void DT(UINT32 n)
{
	sh2.r[n]--;
	if (sh2.r[n] == 0)
		sh2.sr |= T;
	else
		sh2.sr &= ~T;
#if BUSY_LOOP_HACKS
	{
		UINT32 next_opcode = RW(sh2.ppc & AM);
		/* DT	Rn
		 * BF	$-2
		 */
		if (next_opcode == 0x8bfd)
		{
			while (sh2.r[n] > 1 && sh2_icount > 4)
			{
				sh2.r[n]--;
				sh2_icount -= 4;	/* cycles for DT (1) and BF taken (3) */
			}
		}
	}
#endif
}

/*	EXTS.B	Rm,Rn */
INLINE void EXTSB(UINT32 m, UINT32 n)
{
	sh2.r[n] = ((INT32)sh2.r[m] << 24) >> 24;
}

/*	EXTS.W	Rm,Rn */
INLINE void EXTSW(UINT32 m, UINT32 n)
{
	sh2.r[n] = ((INT32)sh2.r[m] << 16) >> 16;
}

/*	EXTU.B	Rm,Rn */
INLINE void EXTUB(UINT32 m, UINT32 n)
{
	sh2.r[n] = sh2.r[m] & 0x000000ff;
}

/*	EXTU.W	Rm,Rn */
INLINE void EXTUW(UINT32 m, UINT32 n)
{
	sh2.r[n] = sh2.r[m] & 0x0000ffff;
}

/*	JMP 	@Rm */
INLINE void JMP(UINT32 m)
{
	sh2.pc = sh2.ea = sh2.r[m];
	change_pc32bew(sh2.pc & AM);
	sh2.delay = sh2.ppc;
}

/*	JSR 	@Rm */
INLINE void JSR(UINT32 m)
{
	sh2.pr = sh2.pc;
	sh2.pc = sh2.ea = sh2.r[m];
	change_pc32bew(sh2.pc & AM);
	sh2.delay = sh2.pr;
	sh2.pr += 2;
	sh2_icount--;
}


/*	LDC 	Rm,SR */
INLINE void LDCSR(UINT32 m)
{
	sh2.sr = sh2.r[m] & FLAGS;
	CHECK_PENDING_IRQ("LDC  Rm,SR")
}

/*	LDC 	Rm,GBR */
INLINE void LDCGBR(UINT32 m)
{
	sh2.gbr = sh2.r[m];
}

/*	LDC 	Rm,VBR */
INLINE void LDCVBR(UINT32 m)
{
	sh2.vbr = sh2.r[m];
}

/*	LDC.L	@Rm+,SR */
INLINE void LDCMSR(UINT32 m)
{
	sh2.ea = sh2.r[m];
	sh2.sr = RL( sh2.ea ) & FLAGS;
	sh2.r[m] += 4;
	sh2_icount -= 2;
	CHECK_PENDING_IRQ("LDC.L  @Rm+,SR")
}

/*	LDC.L	@Rm+,GBR */
INLINE void LDCMGBR(UINT32 m)
{
	sh2.ea = sh2.r[m];
	sh2.gbr = RL( sh2.ea );
	sh2.r[m] += 4;
	sh2_icount -= 2;
}

/*	LDC.L	@Rm+,VBR */
INLINE void LDCMVBR(UINT32 m)
{
	sh2.ea = sh2.r[m];
	sh2.vbr = RL( sh2.ea );
	sh2.r[m] += 4;
	sh2_icount -= 2;
}

/*	LDS 	Rm,MACH */
INLINE void LDSMACH(UINT32 m)
{
	sh2.mach = sh2.r[m];
}

/*	LDS 	Rm,MACL */
INLINE void LDSMACL(UINT32 m)
{
	sh2.macl = sh2.r[m];
}

/*	LDS 	Rm,PR */
INLINE void LDSPR(UINT32 m)
{
	sh2.pr = sh2.r[m];
}

/*	LDS.L	@Rm+,MACH */
INLINE void LDSMMACH(UINT32 m)
{
	sh2.ea = sh2.r[m];
	sh2.mach = RL( sh2.ea );
	sh2.r[m] += 4;
}

/*	LDS.L	@Rm+,MACL */
INLINE void LDSMMACL(UINT32 m)
{
	sh2.ea = sh2.r[m];
	sh2.macl = RL( sh2.ea );
	sh2.r[m] += 4;
}

/*	LDS.L	@Rm+,PR */
INLINE void LDSMPR(UINT32 m)
{
	sh2.ea = sh2.r[m];
	sh2.pr = RL( sh2.ea );
	sh2.r[m] += 4;
}

/*	MAC.L	@Rm+,@Rn+ */
INLINE void MAC_L(UINT32 m, UINT32 n)
{
	UINT32 RnL, RnH, RmL, RmH, Res0, Res1, Res2;
	UINT32 temp0, temp1, temp2, temp3;
	INT32 tempm, tempn, fnLmL;

	tempn = (INT32) RL( sh2.r[n] );
	sh2.r[n] += 4;
	tempm = (INT32) RL( sh2.r[m] );
	sh2.r[m] += 4;
	if ((INT32) (tempn ^ tempm) < 0)
		fnLmL = -1;
	else
		fnLmL = 0;
	if (tempn < 0)
		tempn = 0 - tempn;
	if (tempm < 0)
		tempm = 0 - tempm;
	temp1 = (UINT32) tempn;
	temp2 = (UINT32) tempm;
	RnL = temp1 & 0x0000ffff;
	RnH = (temp1 >> 16) & 0x0000ffff;
	RmL = temp2 & 0x0000ffff;
	RmH = (temp2 >> 16) & 0x0000ffff;
	temp0 = RmL * RnL;
	temp1 = RmH * RnL;
	temp2 = RmL * RnH;
	temp3 = RmH * RnH;
	Res2 = 0;
	Res1 = temp1 + temp2;
	if (Res1 < temp1)
		Res2 += 0x00010000;
	temp1 = (Res1 << 16) & 0xffff0000;
	Res0 = temp0 + temp1;
	if (Res0 < temp0)
		Res2++;
	Res2 = Res2 + ((Res1 >> 16) & 0x0000ffff) + temp3;
	if (fnLmL < 0)
	{
		Res2 = ~Res2;
		if (Res0 == 0)
			Res2++;
		else
			Res0 = (~Res0) + 1;
	}
	if (sh2.sr & S)
	{
		Res0 = sh2.macl + Res0;
		if (sh2.macl > Res0)
			Res2++;
		Res2 += (sh2.mach & 0x0000ffff);
		if (((INT32) Res2 < 0) && (Res2 < 0xffff8000))
		{
			Res2 = 0x00008000;
			Res0 = 0x00000000;
		}
		if (((INT32) Res2 > 0) && (Res2 > 0x00007fff))
		{
			Res2 = 0x00007fff;
			Res0 = 0xffffffff;
		}
		sh2.mach = Res2;
		sh2.macl = Res0;
	}
	else
	{
		Res0 = sh2.macl + Res0;
		if (sh2.macl > Res0)
			Res2++;
		Res2 += sh2.mach;
		sh2.mach = Res2;
		sh2.macl = Res0;
	}
	sh2_icount -= 2;
}

/*	MAC.W	@Rm+,@Rn+ */
INLINE void MAC_W(UINT32 m, UINT32 n)
{
	INT32 tempm, tempn, dest, src, ans;
	UINT32 templ;

	tempn = (INT32) RW( sh2.r[n] );
	sh2.r[n] += 2;
	tempm = (INT32) RW( sh2.r[m] );
	sh2.r[m] += 2;
	templ = sh2.macl;
	tempm = ((INT32) (short) tempn * (INT32) (short) tempm);
	if ((INT32) sh2.macl >= 0)
		dest = 0;
	else
		dest = 1;
	if ((INT32) tempm >= 0)
	{
		src = 0;
		tempn = 0;
	}
	else
	{
		src = 1;
		tempn = 0xffffffff;
	}
	src += dest;
	sh2.macl += tempm;
	if ((INT32) sh2.macl >= 0)
		ans = 0;
	else
		ans = 1;
	ans += dest;
	if (sh2.sr & S)
	{
		if (ans == 1)
		{
			if (src == 0)
				sh2.macl = 0x7fffffff;
			if (src == 2)
				sh2.macl = 0x80000000;
		}
	}
	else
	{
		sh2.mach += tempn;
		if (templ > sh2.macl)
			sh2.mach += 1;

	}
	sh2_icount -= 2;
}

/*	MOV 	Rm,Rn */
INLINE void MOV(UINT32 m, UINT32 n)
{
	sh2.r[n] = sh2.r[m];
}

/*	MOV.B	Rm,@Rn */
INLINE void MOVBS(UINT32 m, UINT32 n)
{
	sh2.ea = sh2.r[n];
	WB( sh2.ea, sh2.r[m] & 0x000000ff);
}

/*	MOV.W	Rm,@Rn */
INLINE void MOVWS(UINT32 m, UINT32 n)
{
	sh2.ea = sh2.r[n];
	WW( sh2.ea, sh2.r[m] & 0x0000ffff);
}

/*	MOV.L	Rm,@Rn */
INLINE void MOVLS(UINT32 m, UINT32 n)
{
	sh2.ea = sh2.r[n];
	WL( sh2.ea, sh2.r[m] );
}

/*	MOV.B	@Rm,Rn */
INLINE void MOVBL(UINT32 m, UINT32 n)
{
	sh2.ea = sh2.r[m];
	sh2.r[n] = (UINT32)(INT32)(INT16)(INT8) RB( sh2.ea );
}

/*	MOV.W	@Rm,Rn */
INLINE void MOVWL(UINT32 m, UINT32 n)
{
	sh2.ea = sh2.r[m];
	sh2.r[n] = (UINT32)(INT32)(INT16) RW( sh2.ea );
}

/*	MOV.L	@Rm,Rn */
INLINE void MOVLL(UINT32 m, UINT32 n)
{
	sh2.ea = sh2.r[m];
	sh2.r[n] = RL( sh2.ea );
}

/*	MOV.B	Rm,@-Rn */
INLINE void MOVBM(UINT32 m, UINT32 n)
{
	/* SMG : bug fix, was reading sh2.r[n] */
	UINT32 data = sh2.r[m] & 0x000000ff;

	sh2.r[n] -= 1;
	WB( sh2.r[n], data );
}

/*	MOV.W	Rm,@-Rn */
INLINE void MOVWM(UINT32 m, UINT32 n)
{
	UINT32 data = sh2.r[m] & 0x0000ffff;

	sh2.r[n] -= 2;
	WW( sh2.r[n], data );
}

/*	MOV.L	Rm,@-Rn */
INLINE void MOVLM(UINT32 m, UINT32 n)
{
	UINT32 data = sh2.r[m];

	sh2.r[n] -= 4;
	WL( sh2.r[n], data );
}

/*	MOV.B	@Rm+,Rn */
INLINE void MOVBP(UINT32 m, UINT32 n)
{
	sh2.r[n] = (UINT32)(INT32)(INT16)(INT8) RB( sh2.r[m] );
	if (n != m)
		sh2.r[m] += 1;
}

/*	MOV.W	@Rm+,Rn */
INLINE void MOVWP(UINT32 m, UINT32 n)
{
	sh2.r[n] = (UINT32)(INT32)(INT16) RW( sh2.r[m] );
	if (n != m)
		sh2.r[m] += 2;
}

/*	MOV.L	@Rm+,Rn */
INLINE void MOVLP(UINT32 m, UINT32 n)
{
	sh2.r[n] = RL( sh2.r[m] );
	if (n != m)
		sh2.r[m] += 4;
}

/*	MOV.B	Rm,@(R0,Rn) */
INLINE void MOVBS0(UINT32 m, UINT32 n)
{
	sh2.ea = sh2.r[n] + sh2.r[0];
	WB( sh2.ea, sh2.r[m] & 0x000000ff );
}

/*	MOV.W	Rm,@(R0,Rn) */
INLINE void MOVWS0(UINT32 m, UINT32 n)
{
	sh2.ea = sh2.r[n] + sh2.r[0];
	WW( sh2.ea, sh2.r[m] & 0x0000ffff );
}

/*	MOV.L	Rm,@(R0,Rn) */
INLINE void MOVLS0(UINT32 m, UINT32 n)
{
	sh2.ea = sh2.r[n] + sh2.r[0];
	WL( sh2.ea, sh2.r[m] );
}

/*	MOV.B	@(R0,Rm),Rn */
INLINE void MOVBL0(UINT32 m, UINT32 n)
{
	sh2.ea = sh2.r[m] + sh2.r[0];
	sh2.r[n] = (UINT32)(INT32)(INT16)(INT8) RB( sh2.ea );
}

/*	MOV.W	@(R0,Rm),Rn */
INLINE void MOVWL0(UINT32 m, UINT32 n)
{
	sh2.ea = sh2.r[m] + sh2.r[0];
	sh2.r[n] = (UINT32)(INT32)(INT16) RW( sh2.ea );
}

/*	MOV.L	@(R0,Rm),Rn */
INLINE void MOVLL0(UINT32 m, UINT32 n)
{
	sh2.ea = sh2.r[m] + sh2.r[0];
	sh2.r[n] = RL( sh2.ea );
}

/*	MOV 	#imm,Rn */
INLINE void MOVI(UINT32 i, UINT32 n)
{
	sh2.r[n] = (UINT32)(INT32)(INT16)(INT8) i;
}

/*	MOV.W	@(disp8,PC),Rn */
INLINE void MOVWI(UINT32 d, UINT32 n)
{
	UINT32 disp = d & 0xff;
	sh2.ea = sh2.pc + disp * 2 + 2;
	sh2.r[n] = (UINT32)(INT32)(INT16) RW( sh2.ea );
}

/*	MOV.L	@(disp8,PC),Rn */
INLINE void MOVLI(UINT32 d, UINT32 n)
{
	UINT32 disp = d & 0xff;
	sh2.ea = ((sh2.pc + 2) & ~3) + disp * 4;
	sh2.r[n] = RL( sh2.ea );
}

/*	MOV.B	@(disp8,GBR),R0 */
INLINE void MOVBLG(UINT32 d)
{
	UINT32 disp = d & 0xff;
	sh2.ea = sh2.gbr + disp;
	sh2.r[0] = (UINT32)(INT32)(INT16)(INT8) RB( sh2.ea );
}

/*	MOV.W	@(disp8,GBR),R0 */
INLINE void MOVWLG(UINT32 d)
{
	UINT32 disp = d & 0xff;
	sh2.ea = sh2.gbr + disp * 2;
	sh2.r[0] = (INT32)(INT16) RW( sh2.ea );
}

/*	MOV.L	@(disp8,GBR),R0 */
INLINE void MOVLLG(UINT32 d)
{
	UINT32 disp = d & 0xff;
	sh2.ea = sh2.gbr + disp * 4;
	sh2.r[0] = RL( sh2.ea );
}

/*	MOV.B	R0,@(disp8,GBR) */
INLINE void MOVBSG(UINT32 d)
{
	UINT32 disp = d & 0xff;
	sh2.ea = sh2.gbr + disp;
	WB( sh2.ea, sh2.r[0] & 0x000000ff );
}

/*	MOV.W	R0,@(disp8,GBR) */
INLINE void MOVWSG(UINT32 d)
{
	UINT32 disp = d & 0xff;
	sh2.ea = sh2.gbr + disp * 2;
	WW( sh2.ea, sh2.r[0] & 0x0000ffff );
}

/*	MOV.L	R0,@(disp8,GBR) */
INLINE void MOVLSG(UINT32 d)
{
	UINT32 disp = d & 0xff;
	sh2.ea = sh2.gbr + disp * 4;
	WL( sh2.ea, sh2.r[0] );
}

/*	MOV.B	R0,@(disp4,Rn) */
INLINE void MOVBS4(UINT32 d, UINT32 n)
{
	UINT32 disp = d & 0x0f;
	sh2.ea = sh2.r[n] + disp;
	WB( sh2.ea, sh2.r[0] & 0x000000ff );
}

/*	MOV.W	R0,@(disp4,Rn) */
INLINE void MOVWS4(UINT32 d, UINT32 n)
{
	UINT32 disp = d & 0x0f;
	sh2.ea = sh2.r[n] + disp * 2;
	WW( sh2.ea, sh2.r[0] & 0x0000ffff );
}

/* MOV.L Rm,@(disp4,Rn) */
INLINE void MOVLS4(UINT32 m, UINT32 d, UINT32 n)
{
	UINT32 disp = d & 0x0f;
	sh2.ea = sh2.r[n] + disp * 4;
	WL( sh2.ea, sh2.r[m] );
}

/*	MOV.B	@(disp4,Rm),R0 */
INLINE void MOVBL4(UINT32 m, UINT32 d)
{
	UINT32 disp = d & 0x0f;
	sh2.ea = sh2.r[m] + disp;
	sh2.r[0] = (UINT32)(INT32)(INT16)(INT8) RB( sh2.ea );
}

/*	MOV.W	@(disp4,Rm),R0 */
INLINE void MOVWL4(UINT32 m, UINT32 d)
{
	UINT32 disp = d & 0x0f;
	sh2.ea = sh2.r[m] + disp * 2;
	sh2.r[0] = (UINT32)(INT32)(INT16) RW( sh2.ea );
}

/*	MOV.L	@(disp4,Rm),Rn */
INLINE void MOVLL4(UINT32 m, UINT32 d, UINT32 n)
{
	UINT32 disp = d & 0x0f;
	sh2.ea = sh2.r[m] + disp * 4;
	sh2.r[n] = RL( sh2.ea );
}

/*	MOVA	@(disp8,PC),R0 */
INLINE void MOVA(UINT32 d)
{
	UINT32 disp = d & 0xff;
	sh2.ea = ((sh2.pc + 2) & ~3) + disp * 4;
	sh2.r[0] = sh2.ea;
}

/*	MOVT	Rn */
INLINE void MOVT(UINT32 n)
{
	sh2.r[n] = sh2.sr & T;
}

/*	MUL.L	Rm,Rn */
INLINE void MULL(UINT32 m, UINT32 n)
{
	sh2.macl = sh2.r[n] * sh2.r[m];
	sh2_icount--;
}

/*	MULS	Rm,Rn */
INLINE void MULS(UINT32 m, UINT32 n)
{
	sh2.macl = (INT16) sh2.r[n] * (INT16) sh2.r[m];
}

/*	MULU	Rm,Rn */
INLINE void MULU(UINT32 m, UINT32 n)
{
	sh2.macl = (UINT16) sh2.r[n] * (UINT16) sh2.r[n];
}

/*	NEG 	Rm,Rn */
INLINE void NEG(UINT32 m, UINT32 n)
{
	sh2.r[n] = 0 - sh2.r[m];
}

/*	NEGC	Rm,Rn */
INLINE void NEGC(UINT32 m, UINT32 n)
{
	UINT32 temp;

	temp = 0 - sh2.r[m];
	sh2.r[n] = temp - (sh2.sr & T);
	if (0 < temp)
		sh2.sr |= T;
	else
		sh2.sr &= ~T;
	if (temp < sh2.r[n])
		sh2.sr |= T;
}

/*	NOP */
INLINE void NOP(void)
{
}

/*	NOT 	Rm,Rn */
INLINE void NOT(UINT32 m, UINT32 n)
{
	sh2.r[n] = ~sh2.r[m];
}

/*	OR		Rm,Rn */
INLINE void OR(UINT32 m, UINT32 n)
{
	sh2.r[n] |= sh2.r[m];
}

/*	OR		#imm,R0 */
INLINE void ORI(UINT32 i)
{
	sh2.r[0] |= i;
	sh2_icount -= 2;
}

/*	OR.B	#imm,@(R0,GBR) */
INLINE void ORM(UINT32 i)
{
	UINT32 temp;

	sh2.ea = sh2.gbr + sh2.r[0];
	temp = RB( sh2.ea );
	temp |= i;
	WB( sh2.ea, temp );
}

/*	ROTCL	Rn */
INLINE void ROTCL(UINT32 n)
{
	UINT32 temp;

	temp = (sh2.r[n] >> 31) & T;
	sh2.r[n] = (sh2.r[n] << 1) | (sh2.sr & T);
	sh2.sr = (sh2.sr & ~T) | temp;
}

/*	ROTCR	Rn */
INLINE void ROTCR(UINT32 n)
{
	UINT32 temp;

	temp = (sh2.r[n] & T) << 31;
	sh2.r[n] = (sh2.r[n] >> 1) | temp;
	if (temp)
		sh2.sr |= T;
	else
		sh2.sr &= ~T;
}

/*	ROTL	Rn */
INLINE void ROTL(UINT32 n)
{
	sh2.sr = (sh2.sr & ~T) | ((sh2.r[n] >> 31) & T);
	sh2.r[n] = (sh2.r[n] << 1) | (sh2.r[n] >> 31);
}

/*	ROTR	Rn */
INLINE void ROTR(UINT32 n)
{
	sh2.sr = (sh2.sr & ~T) | (sh2.r[n] & T);
	sh2.r[n] = (sh2.r[n] >> 1) | (sh2.r[n] << 31);
}

/*	RTE */
INLINE void RTE(void)
{
	sh2.ea = sh2.r[15];
	sh2.pc = RL( sh2.ea );
	change_pc32bew(sh2.pc & AM);
	sh2.r[15] += 4;
	sh2.ea = sh2.r[15];
	sh2.sr = RL( sh2.ea ) & FLAGS;
	sh2.r[15] += 4;
	sh2.delay = sh2.ppc;
	sh2_icount -= 3;
	CHECK_PENDING_IRQ("RTE")
}

/*	RTS */
INLINE void RTS(void)
{
	sh2.pc = sh2.ea = sh2.pr;
	change_pc32bew(sh2.pc & AM);
	sh2.delay = sh2.ppc;
	sh2_icount--;
}

/*	SETT */
INLINE void SETT(void)
{
	sh2.sr |= T;
}

/*	SHAL	Rn		(same as SHLL) */
INLINE void SHAL(UINT32 n)
{
	sh2.sr = (sh2.sr & ~T) | ((sh2.r[n] >> 31) & T);
	sh2.r[n] <<= 1;
}

/*	SHAR	Rn */
INLINE void SHAR(UINT32 n)
{
	sh2.sr = (sh2.sr & ~T) | (sh2.r[n] & T);
	sh2.r[n] = (UINT32)((INT32)sh2.r[n] >> 1);
}

/*	SHLL	Rn		(same as SHAL) */
INLINE void SHLL(UINT32 n)
{
	sh2.sr = (sh2.sr & ~T) | ((sh2.r[n] >> 31) & T);
	sh2.r[n] <<= 1;
}

/*	SHLL2	Rn */
INLINE void SHLL2(UINT32 n)
{
	sh2.r[n] <<= 2;
}

/*	SHLL8	Rn */
INLINE void SHLL8(UINT32 n)
{
	sh2.r[n] <<= 8;
}

/*	SHLL16	Rn */
INLINE void SHLL16(UINT32 n)
{
	sh2.r[n] <<= 16;
}

/*	SHLR	Rn */
INLINE void SHLR(UINT32 n)
{
	sh2.sr = (sh2.sr & ~T) | (sh2.r[n] & T);
	sh2.r[n] >>= 1;
}

/*	SHLR2	Rn */
INLINE void SHLR2(UINT32 n)
{
	sh2.r[n] >>= 2;
}

/*	SHLR8	Rn */
INLINE void SHLR8(UINT32 n)
{
	sh2.r[n] >>= 8;
}

/*	SHLR16	Rn */
INLINE void SHLR16(UINT32 n)
{
	sh2.r[n] >>= 16;
}

/*	SLEEP */
INLINE void SLEEP(void)
{
	sh2.pc -= 2;
	sh2_icount -= 2;
	/* Wait_for_exception; */
}

/*	STC 	SR,Rn */
INLINE void STCSR(UINT32 n)
{
	sh2.r[n] = sh2.sr;
}

/*	STC 	GBR,Rn */
INLINE void STCGBR(UINT32 n)
{
	sh2.r[n] = sh2.gbr;
}

/*	STC 	VBR,Rn */
INLINE void STCVBR(UINT32 n)
{
	sh2.r[n] = sh2.vbr;
}

/*	STC.L	SR,@-Rn */
INLINE void STCMSR(UINT32 n)
{
	sh2.r[n] -= 4;
	sh2.ea = sh2.r[n];
	WL( sh2.ea, sh2.sr );
	sh2_icount--;
}

/*	STC.L	GBR,@-Rn */
INLINE void STCMGBR(UINT32 n)
{
	sh2.r[n] -= 4;
	sh2.ea = sh2.r[n];
	WL( sh2.ea, sh2.gbr );
	sh2_icount--;
}

/*	STC.L	VBR,@-Rn */
INLINE void STCMVBR(UINT32 n)
{
	sh2.r[n] -= 4;
	sh2.ea = sh2.r[n];
	WL( sh2.ea, sh2.vbr );
	sh2_icount--;
}

/*	STS 	MACH,Rn */
INLINE void STSMACH(UINT32 n)
{
	sh2.r[n] = sh2.mach;
}

/*	STS 	MACL,Rn */
INLINE void STSMACL(UINT32 n)
{
	sh2.r[n] = sh2.macl;
}

/*	STS 	PR,Rn */
INLINE void STSPR(UINT32 n)
{
	sh2.r[n] = sh2.pr;
}

/*	STS.L	MACH,@-Rn */
INLINE void STSMMACH(UINT32 n)
{
	sh2.r[n] -= 4;
	sh2.ea = sh2.r[n];
	WL( sh2.ea, sh2.mach );
}

/*	STS.L	MACL,@-Rn */
INLINE void STSMMACL(UINT32 n)
{
	sh2.r[n] -= 4;
	sh2.ea = sh2.r[n];
	WL( sh2.ea, sh2.macl );
}

/*	STS.L	PR,@-Rn */
INLINE void STSMPR(UINT32 n)
{
	sh2.r[n] -= 4;
	sh2.ea = sh2.r[n];
	WL( sh2.ea, sh2.pr );
}

/*	SUB 	Rm,Rn */
INLINE void SUB(UINT32 m, UINT32 n)
{
	sh2.r[n] -= sh2.r[m];
}

/*	SUBC	Rm,Rn */
INLINE void SUBC(UINT32 m, UINT32 n)
{
	UINT32 tmp0, tmp1;

	tmp1 = sh2.r[n] - sh2.r[m];
	tmp0 = sh2.r[n];
	sh2.r[n] = tmp1 - (sh2.sr & T);
	if (tmp0 < tmp1)
		sh2.sr |= T;
	else
		sh2.sr &= ~T;
	if (tmp1 < sh2.r[n])
		sh2.sr |= T;
}

/*	SUBV	Rm,Rn */
INLINE void SUBV(UINT32 m, UINT32 n)
{
	INT32 dest, src, ans;

	if ((INT32) sh2.r[n] >= 0)
		dest = 0;
	else
		dest = 1;
	if ((INT32) sh2.r[m] >= 0)
		src = 0;
	else
		src = 1;
	src += dest;
	sh2.r[n] -= sh2.r[m];
	if ((INT32) sh2.r[n] >= 0)
		ans = 0;
	else
		ans = 1;
	ans += dest;
	if (src == 1)
	{
		if (ans == 1)
			sh2.sr |= T;
		else
			sh2.sr &= ~T;
	}
	else
		sh2.sr &= ~T;
}

/*	SWAP.B	Rm,Rn */
INLINE void SWAPB(UINT32 m, UINT32 n)
{
	UINT32 temp0, temp1;

	temp0 = sh2.r[m] & 0xffff0000;
	temp1 = (sh2.r[m] & 0x000000ff) << 8;
	sh2.r[n] = (sh2.r[m] >> 8) & 0x000000ff;
	sh2.r[n] = sh2.r[n] | temp1 | temp0;
}

/*	SWAP.W	Rm,Rn */
INLINE void SWAPW(UINT32 m, UINT32 n)
{
	UINT32 temp;

	temp = (sh2.r[m] >> 16) & 0x0000ffff;
	sh2.r[n] = (sh2.r[m] << 16) | temp;
}

/*	TAS.B	@Rn */
INLINE void TAS(UINT32 n)
{
	UINT32 temp;
	sh2.ea = sh2.r[n];
	/* Bus Lock enable */
	temp = RB( sh2.ea );
	if (temp == 0)
		sh2.sr |= T;
	else
		sh2.sr &= ~T;
	temp |= 0x80;
	/* Bus Lock disable */
	WB( sh2.ea, temp );
	sh2_icount -= 3;
}

/*	TRAPA	#imm */
INLINE void TRAPA(UINT32 i)
{
	UINT32 imm = i & 0xff;

	sh2.ea = sh2.vbr + imm * 4;

	sh2.r[15] -= 4;
	WL( sh2.r[15], sh2.sr );
	sh2.r[15] -= 4;
	WL( sh2.r[15], sh2.pc );

	sh2.pc = RL( sh2.ea );
	change_pc32bew(sh2.pc & AM);

	sh2_icount -= 7;
}

/*	TST 	Rm,Rn */
INLINE void TST(UINT32 m, UINT32 n)
{
	if ((sh2.r[n] & sh2.r[m]) == 0)
		sh2.sr |= T;
	else
		sh2.sr &= ~T;
}

/*	TST 	#imm,R0 */
INLINE void TSTI(UINT32 i)
{
	UINT32 imm = i & 0xff;

	if ((imm & sh2.r[0]) == 0)
		sh2.sr |= T;
	else
		sh2.sr &= ~T;
}

/*	TST.B	#imm,@(R0,GBR) */
INLINE void TSTM(UINT32 i)
{
	UINT32 imm = i & 0xff;

	sh2.ea = sh2.gbr + sh2.r[0];
	if ((imm & RB( sh2.ea )) == 0)
		sh2.sr |= T;
	else
		sh2.sr &= ~T;
	sh2_icount -= 2;
}

/*	XOR 	Rm,Rn */
INLINE void XOR(UINT32 m, UINT32 n)
{
	sh2.r[n] ^= sh2.r[m];
}

/*	XOR 	#imm,R0 */
INLINE void XORI(UINT32 i)
{
	UINT32 imm = i & 0xff;
	sh2.r[0] ^= imm;
}

/*	XOR.B	#imm,@(R0,GBR) */
INLINE void XORM(UINT32 i)
{
	UINT32 imm = i & 0xff;
	UINT32 temp;

	sh2.ea = sh2.gbr + sh2.r[0];
	temp = RB( sh2.ea );
	temp ^= imm;
	WB( sh2.ea, temp );
	sh2_icount -= 2;
}

/*	XTRCT	Rm,Rn */
INLINE void XTRCT(UINT32 m, UINT32 n)
{
	UINT32 temp;

	temp = (sh2.r[m] << 16) & 0xffff0000;
	sh2.r[n] = (sh2.r[n] >> 16) & 0x0000ffff;
	sh2.r[n] |= temp;
}

/*****************************************************************************
 *	OPCODE DISPATCHERS
 *****************************************************************************/

INLINE void op0000(UINT16 opcode)
{
	switch (opcode & 0x3F)
	{
	case 0x00: NOP();						break;
	case 0x01: NOP();						break;
	case 0x02: STCSR(Rn);					break;
	case 0x03: BSRF(Rn);					break;
	case 0x04: MOVBS0(Rm, Rn);				break;
	case 0x05: MOVWS0(Rm, Rn);				break;
	case 0x06: MOVLS0(Rm, Rn);				break;
	case 0x07: MULL(Rm, Rn);				break;
	case 0x08: CLRT();						break;
	case 0x09: NOP();						break;
	case 0x0a: STSMACH(Rn); 				break;
	case 0x0b: RTS();						break;
	case 0x0c: MOVBL0(Rm, Rn);				break;
	case 0x0d: MOVWL0(Rm, Rn);				break;
	case 0x0e: MOVLL0(Rm, Rn);				break;
	case 0x0f: MAC_L(Rm, Rn);				break;

	case 0x10: NOP();						break;
	case 0x11: NOP();						break;
	case 0x12: STCGBR(Rn);					break;
	case 0x13: NOP();						break;
	case 0x14: MOVBS0(Rm, Rn);				break;
	case 0x15: MOVWS0(Rm, Rn);				break;
	case 0x16: MOVLS0(Rm, Rn);				break;
	case 0x17: MULL(Rm, Rn);				break;
	case 0x18: SETT();						break;
	case 0x19: DIV0U(); 					break;
	case 0x1a: STSMACL(Rn); 				break;
	case 0x1b: SLEEP(); 					break;
	case 0x1c: MOVBL0(Rm, Rn);				break;
	case 0x1d: MOVWL0(Rm, Rn);				break;
	case 0x1e: MOVLL0(Rm, Rn);				break;
	case 0x1f: MAC_L(Rm, Rn);				break;

	case 0x20: NOP();						break;
	case 0x21: NOP();						break;
	case 0x22: STCVBR(Rn);					break;
	case 0x23: BRAF(Rn);					break;
	case 0x24: MOVBS0(Rm, Rn);				break;
	case 0x25: MOVWS0(Rm, Rn);				break;
	case 0x26: MOVLS0(Rm, Rn);				break;
	case 0x27: MULL(Rm, Rn);				break;
	case 0x28: CLRMAC();					break;
	case 0x29: MOVT(Rn);					break;
	case 0x2a: STSPR(Rn);					break;
	case 0x2b: RTE();						break;
	case 0x2c: MOVBL0(Rm, Rn);				break;
	case 0x2d: MOVWL0(Rm, Rn);				break;
	case 0x2e: MOVLL0(Rm, Rn);				break;
	case 0x2f: MAC_L(Rm, Rn);				break;

	case 0x30: NOP();						break;
	case 0x31: NOP();						break;
	case 0x32: NOP();						break;
	case 0x33: NOP();						break;
	case 0x34: MOVBS0(Rm, Rn);				break;
	case 0x35: MOVWS0(Rm, Rn);				break;
	case 0x36: MOVLS0(Rm, Rn);				break;
	case 0x37: MULL(Rm, Rn);				break;
	case 0x38: NOP();						break;
	case 0x39: NOP();						break;
	case 0x3c: MOVBL0(Rm, Rn);				break;
	case 0x3d: MOVWL0(Rm, Rn);				break;
	case 0x3e: MOVLL0(Rm, Rn);				break;
	case 0x3f: MAC_L(Rm, Rn);				break;
	case 0x3a: NOP();						break;
	case 0x3b: NOP();						break;
	}
}

INLINE void op0001(UINT16 opcode)
{
	MOVLS4(Rm, opcode & 0x0f, Rn);
}

INLINE void op0010(UINT16 opcode)
{
	switch (opcode & 15)
	{
	case  0: MOVBS(Rm, Rn); 				break;
	case  1: MOVWS(Rm, Rn); 				break;
	case  2: MOVLS(Rm, Rn); 				break;
	case  3: NOP(); 						break;
	case  4: MOVBM(Rm, Rn); 				break;
	case  5: MOVWM(Rm, Rn); 				break;
	case  6: MOVLM(Rm, Rn); 				break;
	case  7: DIV0S(Rm, Rn); 				break;
	case  8: TST(Rm, Rn);					break;
	case  9: AND(Rm, Rn);					break;
	case 10: XOR(Rm, Rn);					break;
	case 11: OR(Rm, Rn);					break;
	case 12: CMPSTR(Rm, Rn);				break;
	case 13: XTRCT(Rm, Rn); 				break;
	case 14: MULU(Rm, Rn);					break;
	case 15: MULS(Rm, Rn);					break;
	}
}

INLINE void op0011(UINT16 opcode)
{
	switch (opcode & 15)
	{
	case  0: CMPEQ(Rm, Rn); 				break;
	case  1: NOP(); 						break;
	case  2: CMPHS(Rm, Rn); 				break;
	case  3: CMPGE(Rm, Rn); 				break;
	case  4: DIV1(Rm, Rn);					break;
	case  5: DMULU(Rm, Rn); 				break;
	case  6: CMPHI(Rm, Rn); 				break;
	case  7: CMPGT(Rm, Rn); 				break;
	case  8: SUB(Rm, Rn);					break;
	case  9: NOP(); 						break;
	case 10: SUBC(Rm, Rn);					break;
	case 11: SUBV(Rm, Rn);					break;
	case 12: ADD(Rm, Rn);					break;
	case 13: DMULS(Rm, Rn); 				break;
	case 14: ADDC(Rm, Rn);					break;
	case 15: ADDV(Rm, Rn);					break;
	}
}

INLINE void op0100(UINT16 opcode)
{
	switch (opcode & 0x3F)
	{
	case 0x00: SHLL(Rn);					break;
	case 0x01: SHLR(Rn);					break;
	case 0x02: STSMMACH(Rn);				break;
	case 0x03: STCMSR(Rn);					break;
	case 0x04: ROTL(Rn);					break;
	case 0x05: ROTR(Rn);					break;
	case 0x06: LDSMMACH(Rn);				break;
	case 0x07: LDCMSR(Rn);					break;
	case 0x08: SHLL2(Rn);					break;
	case 0x09: SHLR2(Rn);					break;
	case 0x0a: LDSMACH(Rn); 				break;
	case 0x0b: JSR(Rn); 					break;
	case 0x0c: NOP();						break;
	case 0x0d: NOP();						break;
	case 0x0e: LDCSR(Rn);					break;
	case 0x0f: MAC_W(Rm, Rn);				break;

	case 0x10: DT(Rn);						break;
	case 0x11: CMPPZ(Rn);					break;
	case 0x12: STSMMACL(Rn);				break;
	case 0x13: STCMGBR(Rn); 				break;
	case 0x14: NOP();						break;
	case 0x15: CMPPL(Rn);					break;
	case 0x16: LDSMMACL(Rn);				break;
	case 0x17: LDCMGBR(Rn); 				break;
	case 0x18: SHLL8(Rn);					break;
	case 0x19: SHLR8(Rn);					break;
	case 0x1a: LDSMACL(Rn); 				break;
	case 0x1b: TAS(Rn); 					break;
	case 0x1c: NOP();						break;
	case 0x1d: NOP();						break;
	case 0x1e: LDCGBR(Rn);					break;
	case 0x1f: MAC_W(Rm, Rn);				break;

	case 0x20: SHAL(Rn);					break;
	case 0x21: SHAR(Rn);					break;
	case 0x22: STSMPR(Rn);					break;
	case 0x23: STCMVBR(Rn); 				break;
	case 0x24: ROTCL(Rn);					break;
	case 0x25: ROTCR(Rn);					break;
	case 0x26: LDSMPR(Rn);					break;
	case 0x27: LDCMVBR(Rn); 				break;
	case 0x28: SHLL16(Rn);					break;
	case 0x29: SHLR16(Rn);					break;
	case 0x2a: LDSPR(Rn);					break;
	case 0x2b: JMP(Rn); 					break;
	case 0x2c: NOP();						break;
	case 0x2d: NOP();						break;
	case 0x2e: LDCVBR(Rn);					break;
	case 0x2f: MAC_W(Rm, Rn);				break;

	case 0x30: NOP();						break;
	case 0x31: NOP();						break;
	case 0x32: NOP();						break;
	case 0x33: NOP();						break;
	case 0x34: NOP();						break;
	case 0x35: NOP();						break;
	case 0x36: NOP();						break;
	case 0x37: NOP();						break;
	case 0x38: NOP();						break;
	case 0x39: NOP();						break;
	case 0x3a: NOP();						break;
	case 0x3b: NOP();						break;
	case 0x3c: NOP();						break;
	case 0x3d: NOP();						break;
	case 0x3e: NOP();						break;
	case 0x3f: MAC_W(Rm, Rn);				break;
	}
}

INLINE void op0101(UINT16 opcode)
{
	MOVLL4(Rm, opcode & 0x0f, Rn);
}

INLINE void op0110(UINT16 opcode)
{
	switch (opcode & 15)
	{
	case  0: MOVBL(Rm, Rn); 				break;
	case  1: MOVWL(Rm, Rn); 				break;
	case  2: MOVLL(Rm, Rn); 				break;
	case  3: MOV(Rm, Rn);					break;
	case  4: MOVBP(Rm, Rn); 				break;
	case  5: MOVWP(Rm, Rn); 				break;
	case  6: MOVLP(Rm, Rn); 				break;
	case  7: NOT(Rm, Rn);					break;
	case  8: SWAPB(Rm, Rn); 				break;
	case  9: SWAPW(Rm, Rn); 				break;
	case 10: NEGC(Rm, Rn);					break;
	case 11: NEG(Rm, Rn);					break;
	case 12: EXTUB(Rm, Rn); 				break;
	case 13: EXTUW(Rm, Rn); 				break;
	case 14: EXTSB(Rm, Rn); 				break;
	case 15: EXTSW(Rm, Rn); 				break;
	}
}

INLINE void op0111(UINT16 opcode)
{
	ADDI(opcode & 0xff, Rn);
}

INLINE void op1000(UINT16 opcode)
{
	switch ((opcode >> 8) & 15)
	{
	case  0: MOVBS4(opcode & 0x0f, Rm); 	break;
	case  1: MOVWS4(opcode & 0x0f, Rm); 	break;
	case  2: NOP(); 						break;
	case  3: NOP(); 						break;
	case  4: MOVBL4(Rm, opcode & 0x0f); 	break;
	case  5: MOVWL4(Rm, opcode & 0x0f); 	break;
	case  6: NOP(); 						break;
	case  7: NOP(); 						break;
	case  8: CMPIM(opcode & 0xff);			break;
	case  9: BT(opcode & 0xff); 			break;
	case 10: NOP(); 						break;
	case 11: BF(opcode & 0xff); 			break;
	case 12: NOP(); 						break;
	case 13: BTS(opcode & 0xff);			break;
	case 14: NOP(); 						break;
	case 15: BFS(opcode & 0xff);			break;
	}
}


INLINE void op1001(UINT16 opcode)
{
	MOVWI(opcode & 0xff, Rn);
}

INLINE void op1010(UINT16 opcode)
{
	BRA(opcode & 0xfff);
}

INLINE void op1011(UINT16 opcode)
{
	BSR(opcode & 0xfff);
}

INLINE void op1100(UINT16 opcode)
{
	switch ((opcode >> 8) & 15)
	{
	case  0: MOVBSG(opcode & 0xff); 		break;
	case  1: MOVWSG(opcode & 0xff); 		break;
	case  2: MOVLSG(opcode & 0xff); 		break;
	case  3: TRAPA(opcode & 0xff);			break;
	case  4: MOVBLG(opcode & 0xff); 		break;
	case  5: MOVWLG(opcode & 0xff); 		break;
	case  6: MOVLLG(opcode & 0xff); 		break;
	case  7: MOVA(opcode & 0xff);			break;
	case  8: TSTI(opcode & 0xff);			break;
	case  9: ANDI(opcode & 0xff);			break;
	case 10: XORI(opcode & 0xff);			break;
	case 11: ORI(opcode & 0xff);			break;
	case 12: TSTM(opcode & 0xff);			break;
	case 13: ANDM(opcode & 0xff);			break;
	case 14: XORM(opcode & 0xff);			break;
	case 15: ORM(opcode & 0xff);			break;
	}
}

INLINE void op1101(UINT16 opcode)
{
	MOVLI(opcode & 0xff, Rn);
}

INLINE void op1110(UINT16 opcode)
{
	MOVI(opcode & 0xff, Rn);
}

INLINE void op1111(UINT16 opcode)
{
	NOP();
}

/*****************************************************************************
 *	MAME CPU INTERFACE
 *****************************************************************************/

void sh2_reset(void *param)
{
	memset(&sh2, 0, sizeof(SH2));
	sh2.m = malloc(0x200);
	if (!sh2.m)
	{
		logerror("SH2 failed to malloc FREGS\n");
		raise( SIGABRT );
	}
	memset(sh2.m, 0, 0x200);

	sh2.pc = RL(0);
	sh2.r[15] = RL(4);
	sh2.sr = I;
	change_pc32bew(sh2.pc & AM);
}

/* Shut down CPU core */
void sh2_exit(void)
{
	if (sh2.m)
		free(sh2.m);
	sh2.m = NULL;
}

/* Execute cycles - returns number of cycles actually run */
int sh2_execute(int cycles)
{
	sh2_icount = cycles;

	if (sh2.cpu_off)
		return 0;

	do
	{
		UINT32 opcode;

		if (sh2.delay)
		{
			opcode = RW(sh2.delay & AM);
			sh2.pc -= 2;
		}
		else
		{
			opcode = RW(sh2.pc & AM);
		}

		CALL_MAME_DEBUG;

		sh2.delay = 0;
		sh2.pc += 2;
		sh2.ppc = sh2.pc;

		switch ((opcode >> 12) & 15)
		{
		case  0: op0000(opcode); break;
		case  1: op0001(opcode); break;
		case  2: op0010(opcode); break;
		case  3: op0011(opcode); break;
		case  4: op0100(opcode); break;
		case  5: op0101(opcode); break;
		case  6: op0110(opcode); break;
		case  7: op0111(opcode); break;
		case  8: op1000(opcode); break;
		case  9: op1001(opcode); break;
		case 10: op1010(opcode); break;
		case 11: op1011(opcode); break;
		case 12: op1100(opcode); break;
		case 13: op1101(opcode); break;
		case 14: op1110(opcode); break;
		default: op1111(opcode); break;
		}

		sh2_icount--;
	} while( sh2_icount > 0 );

	return cycles - sh2_icount;
}

/* Get registers, return context size */
unsigned sh2_get_context(void *dst)
{
	if( dst )
		memcpy(dst, &sh2, sizeof(SH2));
	return sizeof(SH2);
}

/* Set registers */
void sh2_set_context(void *src)
{
	if( src )
		memcpy(&sh2, src, sizeof(SH2));
}

/* Get program counter */
unsigned sh2_get_pc(void)
{
	if (sh2.delay)
		return sh2.delay & AM;
	return sh2.pc & AM;
}

/* Set program counter */
void sh2_set_pc(unsigned val)
{
	if (sh2.delay)
		sh2.delay = val;
	else
		sh2.pc = val;
}

/* Get stack pointer */
unsigned sh2_get_sp(void)
{
	return sh2.r[15];
}

/* Set stack pointer */
void sh2_set_sp(unsigned val)
{
	sh2.r[15] = val;
}


struct
{
	offs_t offs;
	const char *name;
} FREGS[] = {
	{SH2_CAS_LATENCY_1_16b, "CAS_LAT1_16"},
	{SH2_CAS_LATENCY_2_16b, "CAS_LAT2_16"},
	{SH2_CAS_LATENCY_3_16b, "CAS_LAT3_16"},
	{SH2_CAS_LATENCY_1_32b, "CAS_LAT1_32"},
	{SH2_CAS_LATENCY_2_32b, "CAS_LAT2_32"},
	{SH2_CAS_LATENCY_3_32b, "CAS_LAT3_32"},
	{SH2_TIER, "TIER"},
	{SH2_IRPB, "IRPB"},
	{SH2_VCRA, "VCRA"},
	{SH2_VCRB, "VCRB"},
	{SH2_VCRC, "VCRC"},
	{SH2_VCRD, "VCRD"},
	{SH2_CCR, "CCR"},
	{SH2_ICR, "ICR"},
	{SH2_IRPA, "IRPA"},
	{SH2_VCRWDT, "VCRWDT"},
	{SH2_DVSR, "DVSR"},
	{SH2_DVDNT, "DVDNT"},
	{SH2_DVCR, "DVCR"},
	{SH2_VCRDIV, "VCRDIV"},
	{SH2_DVDNTH, "DVDNTH"},
	{SH2_DVDNTL, "DVDNTL"},
	{SH2_VCRDMA0, "VCRDMA0"},
	{SH2_VCRDMA1, "VCRDMA1"},
	{SH2_BCR1, "BCR1"},
	{SH2_BCR2, "BCR2"},
	{SH2_WCR, "WCR"},
	{SH2_MCR, "MCR"},
	{SH2_RTCSR, "RTCSR"},
	{SH2_RTCNT, "RTCNT"},
	{SH2_RTCOR, "RTCOR"},
	{0, NULL}
};

WRITE_HANDLER( sh2_internal_w )
{
	sh2.m[offset & 0x1ff] = data;

	switch( offset )
	{
	case SH2_DVDNT+3:
		{
			INT32 a = 0, b = 0, q = 0, r = 0;
			a |= sh2.m[(SH2_DVDNT+0)&0x1ff] << 24;
			a |= sh2.m[(SH2_DVDNT+1)&0x1ff] << 16;
			a |= sh2.m[(SH2_DVDNT+2)&0x1ff] <<	8;
			a |= sh2.m[(SH2_DVDNT+3)&0x1ff] <<	0;
			b |= sh2.m[(SH2_DVSR +0)&0x1ff] << 24;
			b |= sh2.m[(SH2_DVSR +1)&0x1ff] << 16;
			b |= sh2.m[(SH2_DVSR +2)&0x1ff] <<	8;
			b |= sh2.m[(SH2_DVSR +3)&0x1ff] <<	0;
			LOG(("SH2 #%d div+mod %d/%d\n", a, b));
			if (b)
			{
				q = a / b;
				r = q % b;
			}
			else
			{
				sh2.m[SH2_DVCR&0x1ff] |= 1;
			}
			sh2.m[(SH2_DVDNTL+0)&0x1ff] = (q >> 24) & 0xff;
			sh2.m[(SH2_DVDNTL+1)&0x1ff] = (q >> 16) & 0xff;
			sh2.m[(SH2_DVDNTL+2)&0x1ff] = (q >>  8) & 0xff;
			sh2.m[(SH2_DVDNTL+3)&0x1ff] = (q >>  0) & 0xff;
			sh2.m[(SH2_DVDNTH+0)&0x1ff] = (r >> 24) & 0xff;
			sh2.m[(SH2_DVDNTH+1)&0x1ff] = (r >> 16) & 0xff;
			sh2.m[(SH2_DVDNTH+2)&0x1ff] = (r >>  8) & 0xff;
			sh2.m[(SH2_DVDNTH+3)&0x1ff] = (r >>  0) & 0xff;
		}
		break;

	case SH2_DVDNTL+3:
		{
			INT64 a = 0, b = 0, q = 0, r = 0;
			a |= sh2.m[(SH2_DVDNTH+0)&0x1ff] << 24;
			a |= sh2.m[(SH2_DVDNTH+1)&0x1ff] << 16;
			a |= sh2.m[(SH2_DVDNTH+2)&0x1ff] <<  8;
			a |= sh2.m[(SH2_DVDNTH+3)&0x1ff] <<  0;
			a <<= 32;
			a |= sh2.m[(SH2_DVDNTL+0)&0x1ff] << 24;
			a |= sh2.m[(SH2_DVDNTL+1)&0x1ff] << 16;
			a |= sh2.m[(SH2_DVDNTL+2)&0x1ff] <<  8;
			a |= sh2.m[(SH2_DVDNTL+3)&0x1ff] <<  0;
			b |= sh2.m[(SH2_DVSR  +0)&0x1ff] << 24;
			b |= sh2.m[(SH2_DVSR  +1)&0x1ff] << 16;
			b |= sh2.m[(SH2_DVSR  +2)&0x1ff] <<  8;
			b |= sh2.m[(SH2_DVSR  +3)&0x1ff] <<  0;
			LOG(("SH2 #%d div+mod %ld/%ld\n", a, b));
			if (b)
			{
				q = DIV_64_64_32(a,b);
				r = MOD_32_64_32(a,b);
				if (q != (INT32)q)
				{
					sh2.m[SH2_DVCR&0x1ff] |= 1;
				}
			}
			else
			{
				sh2.m[SH2_DVCR&0x1ff] |= 1;
			}
			sh2.m[(SH2_DVDNTL+0)&0x1ff] = (q >> 24) & 0xff;
			sh2.m[(SH2_DVDNTL+1)&0x1ff] = (q >> 16) & 0xff;
			sh2.m[(SH2_DVDNTL+2)&0x1ff] = (q >>  8) & 0xff;
			sh2.m[(SH2_DVDNTL+3)&0x1ff] = (q >>  0) & 0xff;
			sh2.m[(SH2_DVDNTH+0)&0x1ff] = (r >> 24) & 0xff;
			sh2.m[(SH2_DVDNTH+1)&0x1ff] = (r >> 16) & 0xff;
			sh2.m[(SH2_DVDNTH+2)&0x1ff] = (r >>  8) & 0xff;
			sh2.m[(SH2_DVDNTH+3)&0x1ff] = (r >>  0) & 0xff;
		}
		break;
	}
#if VERBOSE
	{
		int cpu = cpu_getactivecpu();
		int i;

		for (i = 0; FREGS[i].offs; i++)
		{
			if (offset >= FREGS[i].offs && (FREGS[i+1].offs == 0 || offset < FREGS[i+1].offs))
			{
				if (offset == FREGS[i].offs)
					LOG(("SH2 #%d wr %-16s $%02x\n", cpu, FREGS[i].name, data));
				else
					LOG(("SH2 #%d wr %s+%3d%*s $%02x\n", cpu, FREGS[i].name, offset - FREGS[i].offs, 12 - strlen(FREGS[i].name), "", data));
			}
		}
	}
#endif
}

READ_HANDLER( sh2_internal_r )
{
	UINT32 data;
	int cpu = cpu_getactivecpu();

	switch( offset )
	{
	case SH2_BCR1:
	case SH2_BCR1+1:
	case SH2_BCR1+2:
	case SH2_BCR1+3:
		sh2.m[(SH2_BCR1+0)&0x1ff] = 0x00;
		sh2.m[(SH2_BCR1+1)&0x1ff] = 0x00;
		sh2.m[(SH2_BCR1+2)&0x1ff] = cpu ? 0x80 : 0x00;
		sh2.m[(SH2_BCR1+3)&0x1ff] = 0x00;
		break;
	}
	data = sh2.m[ offset & 0x1ff ];

#if VERBOSE
	{
		int i;

		for (i = 0; FREGS[i].offs; i++)
		{
			if (offset >= FREGS[i].offs && (FREGS[i+1].offs == 0 || offset < FREGS[i+1].offs))
			{
				if (offset == FREGS[i].offs)
					LOG(("SH2 #%d rd %-16s $%02x\n", cpu, FREGS[i].name, data));
				else
					LOG(("SH2 #%d rd %s+%3d%*s $%02x\n", cpu, FREGS[i].name, offset - FREGS[i].offs, 12 - strlen(FREGS[i].name), "", data));
			}
		}
	}
#endif

	return data;
}

unsigned sh2_get_reg(int regnum)
{
	switch( regnum )
	{
	case SH2_PC:   return sh2.pc;
	case SH2_SP:   return sh2.r[15];
	case SH2_PR:   return sh2.pr;
	case SH2_SR:   return sh2.sr;
	case SH2_GBR:  return sh2.gbr;
	case SH2_VBR:  return sh2.vbr;
	case SH2_MACH: return sh2.mach;
	case SH2_MACL: return sh2.macl;
	case SH2_R0:   return sh2.r[ 0];
	case SH2_R1:   return sh2.r[ 1];
	case SH2_R2:   return sh2.r[ 2];
	case SH2_R3:   return sh2.r[ 3];
	case SH2_R4:   return sh2.r[ 4];
	case SH2_R5:   return sh2.r[ 5];
	case SH2_R6:   return sh2.r[ 6];
	case SH2_R7:   return sh2.r[ 7];
	case SH2_R8:   return sh2.r[ 8];
	case SH2_R9:   return sh2.r[ 9];
	case SH2_R10:  return sh2.r[10];
	case SH2_R11:  return sh2.r[11];
	case SH2_R12:  return sh2.r[12];
	case SH2_R13:  return sh2.r[13];
	case SH2_R14:  return sh2.r[14];
	case SH2_EA:   return sh2.ea;
	}
	return 0;
}

void sh2_set_reg (int regnum, unsigned val)
{
	switch( regnum )
	{
	case SH2_PC:   sh2.pc = val;	   break;
	case SH2_SP:   sh2.r[15] = val;    break;
	case SH2_PR:   sh2.pr = val;	   break;
	case SH2_SR:
		sh2.sr = val;
		CHECK_PENDING_IRQ("sh2_set_reg")
		break;
	case SH2_GBR:  sh2.gbr = val;	   break;
	case SH2_VBR:  sh2.vbr = val;	   break;
	case SH2_MACH: sh2.mach = val;	   break;
	case SH2_MACL: sh2.macl = val;	   break;
	case SH2_R0:   sh2.r[ 0] = val;    break;
	case SH2_R1:   sh2.r[ 1] = val;    break;
	case SH2_R2:   sh2.r[ 2] = val;    break;
	case SH2_R3:   sh2.r[ 3] = val;    break;
	case SH2_R4:   sh2.r[ 4] = val;    break;
	case SH2_R5:   sh2.r[ 5] = val;    break;
	case SH2_R6:   sh2.r[ 6] = val;    break;
	case SH2_R7:   sh2.r[ 7] = val;    break;
	case SH2_R8:   sh2.r[ 8] = val;    break;
	case SH2_R9:   sh2.r[ 9] = val;    break;
	case SH2_R10:  sh2.r[10] = val;    break;
	case SH2_R11:  sh2.r[11] = val;    break;
	case SH2_R12:  sh2.r[12] = val;    break;
	case SH2_R13:  sh2.r[13] = val;    break;
	case SH2_R14:  sh2.r[14] = val;    break;
	case SH2_EA:   sh2.ea = val;	   break;
	}
}

void sh2_set_nmi_line(int state)
{
	/* not applicable */
}

void sh2_set_irq_line(int irqline, int state)
{
	if (sh2.irq_line_state[irqline] == state)
		return;
	sh2.irq_line_state[irqline] = state;

	if( state == CLEAR_LINE )
	{
		LOG(("SH-2 #%d cleared irq #%d\n", cpu_getactivecpu(), irqline));
		sh2.pending_irq &= ~(1 << irqline);
	}
	else
	{
		LOG(("SH-2 #%d assert irq #%d\n", cpu_getactivecpu(), irqline));
		sh2.pending_irq |= 1 << irqline;
		CHECK_PENDING_IRQ("sh2_set_irq_line")
	}
}

void sh2_set_irq_callback(int (*callback)(int irqline))
{
	sh2.irq_callback = callback;
}

void sh2_state_save(void *file)
{
}

void sh2_state_load(void *file)
{
}

const char *sh2_info(void *context, int regnum)
{
	static char buffer[8][15+1];
	static int which = 0;
	SH2 *r = context;

	which = ++which % 8;
	buffer[which][0] = '\0';
	if( !context )
		r = &sh2;

	switch( regnum )
	{
		case CPU_INFO_REG+SH2_PC:  sprintf(buffer[which], "PC  :%08X", r->pc); break;
		case CPU_INFO_REG+SH2_SP:  sprintf(buffer[which], "SP  :%08X", r->r[15]); break;
		case CPU_INFO_REG+SH2_SR:  sprintf(buffer[which], "SR  :%08X", r->sr); break;
		case CPU_INFO_REG+SH2_PR:  sprintf(buffer[which], "PR  :%08X", r->pr); break;
		case CPU_INFO_REG+SH2_GBR: sprintf(buffer[which], "GBR :%08X", r->gbr); break;
		case CPU_INFO_REG+SH2_VBR: sprintf(buffer[which], "VBR :%08X", r->vbr); break;
		case CPU_INFO_REG+SH2_MACH:sprintf(buffer[which], "MACH:%08X", r->mach); break;
		case CPU_INFO_REG+SH2_MACL:sprintf(buffer[which], "MACL:%08X", r->macl); break;
		case CPU_INFO_REG+SH2_R0:  sprintf(buffer[which], "R0  :%08X", r->r[ 0]); break;
		case CPU_INFO_REG+SH2_R1:  sprintf(buffer[which], "R1  :%08X", r->r[ 1]); break;
		case CPU_INFO_REG+SH2_R2:  sprintf(buffer[which], "R2  :%08X", r->r[ 2]); break;
		case CPU_INFO_REG+SH2_R3:  sprintf(buffer[which], "R3  :%08X", r->r[ 3]); break;
		case CPU_INFO_REG+SH2_R4:  sprintf(buffer[which], "R4  :%08X", r->r[ 4]); break;
		case CPU_INFO_REG+SH2_R5:  sprintf(buffer[which], "R5  :%08X", r->r[ 5]); break;
		case CPU_INFO_REG+SH2_R6:  sprintf(buffer[which], "R6  :%08X", r->r[ 6]); break;
		case CPU_INFO_REG+SH2_R7:  sprintf(buffer[which], "R7  :%08X", r->r[ 7]); break;
		case CPU_INFO_REG+SH2_R8:  sprintf(buffer[which], "R8  :%08X", r->r[ 8]); break;
		case CPU_INFO_REG+SH2_R9:  sprintf(buffer[which], "R9  :%08X", r->r[ 9]); break;
		case CPU_INFO_REG+SH2_R10: sprintf(buffer[which], "R10 :%08X", r->r[10]); break;
		case CPU_INFO_REG+SH2_R11: sprintf(buffer[which], "R11 :%08X", r->r[11]); break;
		case CPU_INFO_REG+SH2_R12: sprintf(buffer[which], "R12 :%08X", r->r[12]); break;
		case CPU_INFO_REG+SH2_R13: sprintf(buffer[which], "R13 :%08X", r->r[13]); break;
		case CPU_INFO_REG+SH2_R14: sprintf(buffer[which], "R14 :%08X", r->r[14]); break;
		case CPU_INFO_REG+SH2_EA:  sprintf(buffer[which], "EA  :%08X", r->ea);    break;
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%d%c%c",
				r->sr & M ? 'M':'.',
				r->sr & Q ? 'Q':'.',
				(r->sr & I) >> 4,
				r->sr & S ? 'S':'.',
				r->sr & T ? 'T':'.');
			break;
		case CPU_INFO_NAME: return "SH-2";
		case CPU_INFO_FAMILY: return "Hitachi SH7600";
		case CPU_INFO_VERSION: return "1.01";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (c) 2000 Juergen Buchmueller, all rights reserved.";
		case CPU_INFO_REG_LAYOUT: return (const char*)sh2_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)sh2_win_layout;
	}
	return buffer[which];
}

unsigned sh2_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return DasmSH2( buffer, pc );
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}
