//========================================
//  emulate.h
//
//  C Header file for TMS core
//========================================

#ifndef TMS9900_H
#define TMS9900_H

#include <stdio.h>
#include "driver.h"
#include "types.h"

/* Storage Classes */

#define u32 unsigned int
#define s32 int
#define u16 unsigned short
#define s16 signed short
#define u8  unsigned char
#define s8  signed char


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
	word	WP;
	word	PC;
	word	STATUS;
	word	IR;

#if NEW_INTERRUPT_SYSTEM
	int irq_state;
	int (*irq_callback)(int irq_line);
#endif

#if MAME_DEBUG
    word	FR0;
    word	FR1;
    word	FR2;
    word	FR3;
    word	FR4;
    word	FR5;
    word	FR6;
    word	FR7;
    word	FR8;
    word	FR9;
    word	FR10;
    word	FR11;
    word	FR12;
    word	FR13;
    word	FR14;
    word	FR15;
#endif
}
TMS9900_Regs ;

extern  TMS9900_Regs I;
extern  int TMS9900_ICount;
extern  u8  lastparity;

void    TMS9900_Reset(void);
int     TMS9900_Execute(int cycles);
void    TMS9900_GetRegs(TMS9900_Regs *Regs);
void    TMS9900_SetRegs(TMS9900_Regs *Regs);
int     TMS9900_GetPC(void);

#if NEW_INTERRUPT_SYSTEM
void TMS9900_set_nmi_line(int state);
void TMS9900_set_irq_line(int irqline, int state);
void TMS9900_set_irq_callback(int (*callback)(int irqline));
#else
void TMS9900_Cause_Interrupt(int type);
void TMS9900_Clear_Pending_Interrupts(void);
#endif

#endif





