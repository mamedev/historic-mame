/*** m6809: Portable 6809 emulator ******************************************/

#ifndef _M6809_H
#define _M6809_H


/****************************************************************************/
/* sizeof(byte)=1, sizeof(word)=2, sizeof(dword)>=4                         */
/****************************************************************************/
#include "types.h"	/* -NS- */

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


#define INT_NONE  0			/* No interrupt required */
#define INT_IRQ	  1 		/* Standard IRQ interrupt */
#define INT_FIRQ  2			/* Fast IRQ */

/* PUBLIC FUNCTIONS */
void m6809_SetRegs(m6809_Regs *Regs);
void m6809_GetRegs(m6809_Regs *Regs);
unsigned m6809_GetPC(void);
void m6809_reset(void);
void m6809_execute(void);

/* PUBLIC GLOBALS */
extern int	m6809_IPeriod;
extern int	m6809_ICount;
extern int	m6809_IRequest;


/****************************************************************************/
/* Read a byte from given memory location                                   */
/****************************************************************************/
int cpu_readmem(int address);
#define M6809_RDMEM(A) ((unsigned)cpu_readmem(A))

/****************************************************************************/
/* Write a byte to given memory location                                    */
/****************************************************************************/
void cpu_writemem(int address, int data);
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

/****************************************************************************/
/* Flags for optimizing memory access. Game drivers should set m6809_Flags  */
/* to a combination of these flags depending on what can be safely          */
/* optimized. For example, if M6809_FAST_OP is set, opcodes are fetched     */
/* directly from the ROM array, and cpu_readmem() is not called.            */
/* The flags affect reads and writes.                                       */
/****************************************************************************/
extern int m6809_Flags;
#define M6809_FAST_NONE	0x00	/* no memory optimizations */
#define M6809_FAST_OP	0x01	/* opcode fetching */
#define M6809_FAST_S	0x02	/* stack */
#define M6809_FAST_U	0x04	/* user stack */

#ifndef FALSE
#    define FALSE 0
#endif
#ifndef TRUE
#    define TRUE (!FALSE)
#endif

#endif /* _M6809_H */
