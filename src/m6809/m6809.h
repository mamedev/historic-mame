/*** m6809: Portable 6809 emulator ******************************************/
/***                                                                      ***/
/***                                 m6809.h                              ***/
/***                                                                      ***/
/*** This file contains the function prototypes and variable declarations ***/
/***                                                                      ***/
/*** Copyright (C) DS 1997                                                ***/
/*** Portions Copyright (C) Marcel de Kogel 1996,1997                     ***/
/*** Portions Copyright (C) Nicola Salmoria 1997                          ***/
/***                                                                      ***/
/***     You are not allowed to distribute this software commercially     ***/
/****************************************************************************/

#ifndef _M6809_H
#define _M6809_H


/****************************************************************************/
/* sizeof(byte)=1, sizeof(word)=2, sizeof(dword)>=4                         */
/****************************************************************************/
#include "types.h"	/* -NS- */

typedef byte Byte;
typedef word Word;

/* 6809 Registers */
typedef struct
{
	word		pc;		/* Program counter */
	word		u, s;	/* Stack pointers */
	word		x, y;	/* Index registers */
	byte		dp;		/* Direct Page register */
	byte		a, b;	/* Accumulator */
	byte		cc;
} m6809_Regs;

#ifndef INLINE
#define INLINE static inline
#endif


#define INT_NONE  0            /* No interrupt required */
#define INT_IRQ	  1            /* Standard IRQ interrupt */

/* PUBLIC FUNCTIONS */
extern void m6809_SetRegs(m6809_Regs *Regs);
extern void m6809_GetRegs(m6809_Regs *Regs);
extern unsigned m6809_GetPC(void);
extern void m6809_reset(void);
extern void m6809_execute(void);

/* PUBLIC GLOBALS */
extern int	m6809_IPeriod;
extern int	m6809_ICount;
extern int	m6809_IRequest;

/****************************************************************************/
/* Read a byte from given memory location                                   */
/****************************************************************************/
extern int cpu_readmem(register int A);
#define M6809_RDMEM(A) ((unsigned)cpu_readmem(A))

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
extern void cpu_writemem(register int A,register unsigned char V);
#define M6809_WRMEM(A,V) (cpu_writemem(A,V))

/****************************************************************************/
/* Z80_RDOP() is identical to Z80_RDMEM() except it is used for reading     */
/* opcodes. In case of system with memory mapped I/O, this function can be  */
/* used to greatly speed up emulation                                       */
/****************************************************************************/
extern byte *RAM;
extern byte *ROM;
#define M6809_RDOP(A) (ROM[A])

/****************************************************************************/
/* Z80_RDOP_ARG() is identical to Z80_RDOP() except it is used for reading  */
/* opcode arguments. This difference can be used to support systems that    */
/* use different encoding mechanisms for opcodes and opcode arguments       */
/****************************************************************************/
/*#define Z80_RDOP_ARG(A)		Z80_RDOP(A)*/
#define M6809_RDOP_ARG(A) (RAM[A])


#endif /* _M6809_H */

