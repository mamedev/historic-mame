/*****************************************************************************
 *
 *	 m6502.h
 *	 Portable 6502/65c02/6510 emulator interface
 *
 *	 Copyright (c) 1998 Juergen Buchmueller, all rights reserved.
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
 *     terms of its usage and license at any time, including retroactively
 *   - This entire notice must remain in the source code.
 *
 *****************************************************************************/

#ifndef _M6502_H
#define _M6502_H

#include "osd_cpu.h"

#ifndef INLINE
#define INLINE static inline
#endif

#define SUPP65C02	1		/* set to 1 to support the 65C02 opcodes */
#define SUPP6510	1		/* set to 1 to support the 6510 opcodes */

#define M6502_PLAIN 0		/* set M6502_Type to this for a plain 6502 emulation */

#if SUPP65C02
#define M6502_65C02 1		/* set M6502_Type to this for a 65C02 emulation */
#endif

#if SUPP6510
#define M6502_6510	2		/* set M6502_Type to this for a 6510 emulation */
#endif

/* set to 1 to test cur_mrhard/cur_wmhard to avoid calls */
#define FAST_MEMORY 1

/****************************************************************************
 *** End of machine dependent definitions								  ***
 ****************************************************************************/

/****************************************************************************
 * The 6502 registers.
 ****************************************************************************/
typedef struct
{
	UINT8	cpu_type;		/* currently selected cpu sub type */
	void	(**insn)(void); /* pointer to the function pointer table */
	PAIR	pc; 			/* program counter */
	PAIR	sp; 			/* stack pointer (always 100 - 1FF) */
	PAIR	zp; 			/* zero page address */
	PAIR	ea; 			/* effective address */
	UINT8	a;				/* Accumulator */
	UINT8	x;				/* X index register */
	UINT8	y;				/* Y index register */
	UINT8	p;				/* Processor status */
	UINT8	pending_interrupt; /* nonzero if a NMI or IRQ is pending */
	UINT8	after_cli;		/* pending IRQ and last insn cleared I */
	INT8	nmi_state;
	INT8	irq_state;
	int 	(*irq_callback)(int irqline);	/* IRQ callback */
}	m6502_Regs;

#define M6502_INT_NONE	0
#define M6502_INT_IRQ	1
#define M6502_INT_NMI	2

#define M6502_NMI_VEC	0xfffa
#define M6502_RST_VEC	0xfffc
#define M6502_IRQ_VEC	0xfffe

extern int m6502_ICount;				/* cycle count */

extern void m6502_reset (void *param);			/* Reset registers to the initial values */
extern void m6502_exit	(void); 				/* Shut down CPU core */
extern int	m6502_execute(int cycles);			/* Execute cycles - returns number of cycles actually run */
extern void m6502_getregs (m6502_Regs *Regs);	/* Get registers */
extern void m6502_setregs (m6502_Regs *Regs);	/* Set registers */
extern unsigned m6502_getpc (void); 			/* Get program counter */
extern unsigned m6502_getreg (int regnum);
extern void m6502_setreg (int regnum, unsigned val);
extern void m6502_set_nmi_line(int state);
extern void m6502_set_irq_line(int irqline, int state);
extern void m6502_set_irq_callback(int (*callback)(int irqline));
extern void m6502_state_save(void *file);
extern void m6502_state_load(void *file);
extern const char *m6502_info(void *context, int regnum);

/****************************************************************************
 * The 65C02
 ****************************************************************************/

#define M65C02_INT_NONE 				M6502_INT_NONE
#define M65C02_INT_IRQ					M6502_INT_IRQ
#define M65C02_INT_NMI					M6502_INT_NMI

#define M65C02_NMI_VEC					M6502_NMI_VEC
#define M65C02_RST_VEC					M6502_RST_VEC
#define M65C02_IRQ_VEC					M6502_IRQ_VEC

#define m65c02_ICount					m6502_ICount

extern void m65c02_reset (void *param);
#define m65c02_exit                     m6502_exit
#define m65c02_execute					m6502_execute
#define m65c02_setregs                  m6502_setregs
#define m65c02_getregs                  m6502_getregs
#define m65c02_getpc                    m6502_getpc
#define m65c02_getreg					m6502_getreg
#define m65c02_setreg					m6502_setreg
#define m65c02_set_nmi_line 			m6502_set_nmi_line
#define m65c02_set_irq_line 			m6502_set_irq_line
#define m65c02_set_irq_callback 		m6502_set_irq_callback
#define m65c02_state_save				m6502_state_save
#define m65c02_state_load				m6502_state_load
extern const char *m65c02_info(void *context, int regnum);

/****************************************************************************
 * The 6510
 ****************************************************************************/

#define M6510_INT_NONE					M6502_INT_NONE
#define M6510_INT_IRQ					M6502_INT_IRQ
#define M6510_INT_NMI					M6502_INT_NMI

#define M6510_NMI_VEC					M6502_NMI_VEC
#define M6510_RST_VEC					M6502_RST_VEC
#define M6510_IRQ_VEC					M6502_IRQ_VEC

#define m6510_ICount					m6502_ICount

extern void m6510_reset (void *param);
#define m6510_exit						m6502_exit
#define m6510_execute					m6502_execute
#define m6510_setregs                   m6502_setregs
#define m6510_getregs                   m6502_getregs
#define m6510_getpc                     m6502_getpc
#define m6510_getreg					m6502_getreg
#define m6510_setreg					m6502_setreg
#define m6510_set_nmi_line              m6502_set_nmi_line
#define m6510_set_irq_line				m6502_set_irq_line
#define m6510_set_irq_callback			m6502_set_irq_callback
#define m6510_state_save				m6502_state_save
#define m6510_state_load				m6502_state_load
extern const char *m6510_info(void *context, int regnum);

#ifdef MAME_DEBUG
extern int mame_debug;
extern int Dasm6502(char *buffer, int pc);
#endif

#endif /* _M6502_H */


