/*****************************************************************************

	h6280.h Portable Hu6280 emulator interface

	Copyright (c) 1999 Bryan McPhail, mish@tendril.force9.net

	This source code is based (with permission!) on the 6502 emulator by
	Juergen Buchmueller.  It is released as part of the Mame emulator project.
	Let me know if you intend to use this code in any other project.

******************************************************************************/

#ifndef _H6280_H
#define _H6280_H

/****************************************************************************
 * sizeof(byte)=1, sizeof(word)=2, sizeof(dword)>=4
 ****************************************************************************/
#include "types.h"

#ifndef INLINE
#define INLINE static inline
#endif

#define LAZY_FLAGS	1

/****************************************************************************
 * Define a 6280 word. Upper bytes are always zero
 ****************************************************************************/
typedef 	union {
 #ifdef LSB_FIRST
   struct { byte l,h,h2,h3; } B;
   struct { word l,h; } W;
   dword D;
 #else
   struct { byte h3,h2,h,l; } B;
   struct { word h,l; } W;
   dword D;
 #endif
}	h6280_pair;

/****************************************************************************
 *** End of machine dependent definitions								  ***
 ****************************************************************************/

/****************************************************************************
 * The 6280 registers.
 ****************************************************************************/
typedef struct
{
	h6280_pair PC;					/* program counter */
	h6280_pair SP;					/* stack pointer (always 100 - 1FF) */
	h6280_pair ZP;					/* zero page address */
	h6280_pair EA;					/* effective address */
    byte a;                         /* Accumulator */
	byte x;							/* X index register */
	byte y;							/* Y index register */
	byte p;							/* Processor status */
	byte mmr[8]; 					/* Hu6280 memory mapper registers */
	byte irq_mask;					/* interrupt enable/disable */
	byte timer_status;				/* timer status */
	int timer_value;				/* timer interrupt */
	byte timer_load;				/* reload value */

	int pending_interrupt;			/* nonzero if an interrupt is pending */

#if NEW_INTERRUPT_SYSTEM
	int nmi_state;
	int irq_state[3];
	int (*irq_callback)(int irqline);
#endif

#if LAZY_FLAGS
	int NZ; 						/* last value (lazy N and Z flag) */
#endif

}	H6280_Regs;

#define H6280_INT_NONE	0
#define H6280_INT_NMI	1
#define H6280_INT_TIMER	2
#define H6280_INT_IRQ1	3
#define H6280_INT_IRQ2	4

#define H6280_NMI_MASK		1
#define H6280_TIMER_MASK	2
#define H6280_IRQ1_MASK		4
#define H6280_IRQ2_MASK		8

#define H6280_RESET_VEC	0xfffe
#define H6280_NMI_VEC	0xfffc
#define H6280_TIMER_VEC	0xfffa
#define H6280_IRQ1_VEC	0xfff8
#define H6280_IRQ2_VEC	0xfff6			/* Aka BRK vector */

extern int H6280_ICount;				/* cycle count */

unsigned H6280_GetPC (void);			/* Get program counter */
void H6280_GetRegs (H6280_Regs *Regs);	/* Get registers */
void H6280_SetRegs (H6280_Regs *Regs);	/* Set registers */
void H6280_Reset (void);				/* Reset registers to the initial values */
int  H6280_Execute(int cycles); 		/* Execute cycles - returns number of cycles actually run */


#if NEW_INTERRUPT_SYSTEM
extern void H6280_set_nmi_line(int state);
extern void H6280_set_irq_line(int irqline, int state);
extern void H6280_set_irq_callback(int (*callback)(int irqline));
#else
void H6280_Cause_Interrupt(int type);
void H6280_Clear_Pending_Interrupts(void);
#endif

int H6280_irq_status_r(int offset);
void H6280_irq_status_w(int offset, int data);

int H6280_timer_r(int offset);
void H6280_timer_w(int offset, int data);

#endif /* _H6280_H */
