/*****************************************************************************

	h6280.h Portable Hu6280 emulator interface

	Copyright (c) 1999 Bryan McPhail, mish@tendril.force9.net

	This source code is based (with permission!) on the 6502 emulator by
	Juergen Buchmueller.  It is released as part of the Mame emulator project.
	Let me know if you intend to use this code in any other project.

******************************************************************************/

#ifndef _H6280_H
#define _H6280_H

#include "osd_cpu.h"

#ifndef INLINE
#define INLINE static inline
#endif

#define LAZY_FLAGS	1

/****************************************************************************
 * The 6280 registers.
 ****************************************************************************/
typedef struct
{
	PAIR  pc;						/* program counter */
	PAIR  sp;						/* stack pointer (always 100 - 1FF) */
	PAIR  zp;						/* zero page address */
	PAIR  ea;						/* effective address */
	UINT8 a;						/* Accumulator */
	UINT8 x;						/* X index register */
	UINT8 y;						/* Y index register */
	UINT8 p;						/* Processor status */
	UINT8 mmr[8];					/* Hu6280 memory mapper registers */
	UINT8 irq_mask; 				/* interrupt enable/disable */
	UINT8 timer_status; 			/* timer status */
	int timer_value;				/* timer interrupt */
	UINT8 timer_load;				/* reload value */
	int pending_interrupt;			/* nonzero if an interrupt is pending */
	int nmi_state;
	int irq_state[3];
	int (*irq_callback)(int irqline);

#if LAZY_FLAGS
	int NZ; 						/* last value (lazy N and Z flag) */
#endif

}	h6280_Regs;

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

extern int h6280_ICount;				/* cycle count */

extern void h6280_reset (void *param);		   /* Reset registers to the initial values */
extern void h6280_exit	(void); 			   /* Shut down CPU */
extern int	h6280_execute(int cycles);		   /* Execute cycles - returns number of cycles actually run */
extern void h6280_getregs (h6280_Regs *Regs);  /* Get registers */
extern void h6280_setregs (h6280_Regs *Regs);  /* Set registers */
extern unsigned h6280_getpc (void); 		   /* Get program counter */
extern unsigned h6280_getreg (int regnum);
extern void h6280_setreg (int regnum, unsigned val);
extern void h6280_set_nmi_line(int state);
extern void h6280_set_irq_line(int irqline, int state);
extern void h6280_set_irq_callback(int (*callback)(int irqline));
extern const char *h6280_info(void *context, int regnum);

extern int H6280_irq_status_r(int offset);
extern void H6280_irq_status_w(int offset, int data);

extern int H6280_timer_r(int offset);
extern void H6280_timer_w(int offset, int data);

#ifdef MAME_DEBUG
extern int mame_debug;
extern int Dasm6280(char *buffer, int pc);
#endif

#endif /* _H6280_H */
