/*****************************************************************************
 *
 *	 m6502.c
 *	 Portable 6502/65c02/6510 emulator
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

#include <stdio.h>
#include "memory.h"
#include "cpuintrf.h"
#include "osd_dbg.h"
#include "m6502/m6502.h"
#include "m6502/m6502ops.h"

extern FILE * errorlog;

int m6502_ICount = 0;
int M6502_Type = M6502_PLAIN;
static void(**insn)(void);
static M6502_Regs m6502;

/***************************************************************
 * include the opcode macros, functions and tables
 ***************************************************************/
#include "M6502/tbl6502.c"
#include "M6502/tbl65c02.c"
#include "M6502/tbl6510.c"

/*****************************************************************************
 *
 *		6502 CPU interface functions
 *
 *****************************************************************************/

unsigned m6502_GetPC (void)
{
	return PCD;
}

void m6502_GetRegs (M6502_Regs *Regs)
{
	*Regs = m6502;
}

void m6502_SetRegs (M6502_Regs *Regs)
{
	m6502 = *Regs;
}

void m6502_Reset(void)
{
	switch (M6502_Type)
	{
#if SUPP65C02
        case M6502_65C02:
			insn = insn65c02;
            break;
#endif
#if SUPP6510
        case M6502_6510:
			insn = insn6510;
			break;
#endif
        default:
            insn = insn6502;
            break;
    }

    /* wipe out the m6502 structure */
	memset(&m6502, 0, sizeof(M6502_Regs));

	/* set T, I and Z flags */
	P = _fT | _fI | _fZ;

    /* stack starts at 0x01ff */
	m6502.sp.D = 0x1ff;

    /* read the reset vector into PC */
	PCL = RDMEM(M6502_RST_VEC);
	PCH = RDMEM(M6502_RST_VEC+1);
	change_pc16(PCD);

#if NEW_INTERRUPT_SYSTEM
    m6502.pending_interrupt = 0;
    m6502.nmi_state = 0;
    m6502.irq_state = 0;
#else
    /* clear pending interrupts */
	m6502_Clear_Pending_Interrupts();
#endif
}

INLINE void take_nmi(void)
{
	EAD = M6502_NMI_VEC;
	m6502_ICount -= 7;
	m6502.halt = 0;
	PUSH(PCH);
	PUSH(PCL);
	COMPOSE_P;
	PUSH(P & ~_fB);
	P = (P & ~_fD) | _fI;		/* knock out D and set I flag */
	PCL = RDMEM(EAD);
	PCH = RDMEM(EAD+1);
	change_pc16(PCD);
	m6502.pending_interrupt &= ~M6502_INT_NMI;
}

INLINE void take_irq(void)
{
	EAD = M6502_IRQ_VEC;
	m6502_ICount -= 7;
	m6502.halt = 0;
	PUSH(PCH);
	PUSH(PCL);
	COMPOSE_P;
	PUSH(P & ~_fB);
	P = (P & ~_fD) | _fI;		/* knock out D and set I flag */
	PCL = RDMEM(EAD);
	PCH = RDMEM(EAD+1);
#if NEW_INTERRRUPT_SYSTEM
    /* call back the cpuintrf to let it clear the line */
	(void)(*m6502.irq_callack)(0);
#else
	m6502.pending_interrupt &= ~M6502_INT_IRQ;
#endif
    change_pc16(PCD);
}

int m6502_Execute(int cycles)
{
	m6502_ICount = cycles;

	change_pc16(PCD);

	do {
#ifdef  MAME_DEBUG
        {
        extern int mame_debug;
			if (mame_debug) {
				COMPOSE_P;
                MAME_Debug();
            }
        }
#endif
		{
			extern int previouspc;
			previouspc = PCW;
		}

		if (m6502.halt)
            break;

        insn[RDOP()]();

		if (m6502.pending_interrupt & M6502_INT_NMI)
			take_nmi();
        /* check if the I flag was just reset (interrupts enabled) */
		if (m6502.after_cli) {
            m6502.after_cli = 0;
#if NEW_INTERRUPT_SYSTEM
			if (m6502.irq_state != CLEAR_LINE)
				m6502.pending_interrupt |= M6502_INT_IRQ;
#endif
		} else if (m6502.pending_interrupt && !(P & _fI))
			take_irq();

    } while (m6502_ICount > 0);

	return cycles - m6502_ICount;
}

#if NEW_INTERRUPT_SYSTEM

void m6502_set_nmi_line(int state)
{
	if (m6502.nmi_state == state) return;
	m6502.nmi_state = state;
	if (state != CLEAR_LINE)
		m6502.pending_interrupt |= M6502_INT_NMI;
}

void m6502_set_irq_line(int irqline, int state)
{
	m6502.irq_state = state;
	if (state == CLEAR_LINE) {
		m6502.pending_interrupt &= ~M6502_INT_IRQ;
	} else {
		m6502.pending_interrupt |= M6502_INT_IRQ;
	}
}

void m6502_set_irq_callback(int (*callback)(int))
{
	m6502.irq_callback = callback;
}

#else

void m6502_Cause_Interrupt(int type)
{
	m6502.pending_interrupt |= type;
}

void m6502_Clear_Pending_Interrupts(void)
{
	m6502.pending_interrupt = M6502_INT_NONE;
}

#endif
