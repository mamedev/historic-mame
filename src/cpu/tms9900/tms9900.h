//========================================
//  emulate.h
//
//  C Header file for TMS core
//========================================

#ifndef TMS9900_H
#define TMS9900_H

#include <stdio.h>
#include "driver.h"
#include "osd_cpu.h"

#define TMS9900_NONE  -1

// 9900's STATUS register bits.

// These bits are set by every compare, move and arithmetic or logical operation :
// (Well, COC, CZC and TB only set the E bit, but these are kind of exceptions.)
#define ST_L 0x8000 // logically greater (strictly)
#define ST_A 0x4000 // arithmetically greater (strictly)
#define ST_E 0x2000 // equal

// these bits are set by arithmetic operations, when it makes sense to update them.
#define ST_C 0x1000 // Carry
#define ST_O 0x0800 // Overflow (overflow with operations on signed integers,
                    // and when the result of a 32bits:16bits division cannot fit in a 16-bit word.)

// this bit is set by move and arithmetic operations WHEN THEY USE BYTE OPERANDS.
#define ST_P 0x0400 // Parity (set when odd parity)

// this bit is set by the XOP instruction.
#define ST_X 0x0200 // Xop

// Offsets for registers.
#define R0 	 0
#define R1 	 2
#define R2 	 4
#define R3 	 6
#define R4 	 8
#define R5 	10
#define R6 	12
#define R7 	14
#define R8 	16
#define R9 	18
#define R10 20
#define R11 22
#define R12 24
#define R13 26
#define R14 28
#define R15 30

typedef struct
{
	UINT16	WP;
	UINT16	PC;
	UINT16	STATUS;
	UINT16	IR;
	int irq_state;
	int (*irq_callback)(int irq_line);

#if MAME_DEBUG
	UINT16	FR0;
	UINT16	FR1;
	UINT16	FR2;
	UINT16	FR3;
	UINT16	FR4;
	UINT16	FR5;
	UINT16	FR6;
	UINT16	FR7;
	UINT16	FR8;
	UINT16	FR9;
	UINT16	FR10;
	UINT16	FR11;
	UINT16	FR12;
	UINT16	FR13;
	UINT16	FR14;
	UINT16	FR15;
#endif
}
TMS9900_Regs ;

extern  TMS9900_Regs I;
extern  int TMS9900_ICount;
extern	UINT8 lastparity;

extern void TMS9900_reset(void *param);
extern int TMS9900_execute(int cycles);
extern void TMS9900_exit(void);
extern void TMS9900_getregs(TMS9900_Regs *Regs);
extern void TMS9900_setregs(TMS9900_Regs *Regs);
extern unsigned TMS9900_getpc(void);
extern unsigned TMS9900_getreg(int regnum);
extern void TMS9900_setreg(int regnum, unsigned val);
extern void TMS9900_set_nmi_line(int state);
extern void TMS9900_set_irq_line(int irqline, int state);
extern void TMS9900_set_irq_callback(int (*callback)(int irqline));
extern const char *TMS9900_info(void *context, int regnum);

#ifdef MAME_DEBUG
extern int mame_debug;
extern int Dasm9900 (char *buffer, int pc);
#endif

#endif





