/*****************************************************************************

	h6280.c - Portable Hu6280 emulator

	Copyright (c) 1999 Bryan McPhail, mish@tendril.force9.net

	This source code is based (with permission!) on the 6502 emulator by
	Juergen Buchmueller.  It is released as part of the Mame emulator project.
	Let me know if you intend to use this code in any other project.


	NOTICE:

	This code is not currently complete!  Several things are unimplemented,
	some due to lack of time, some due to lack of documentation, mainly
	due to lack of programs using these features.

	csh, csl opcodes are not supported.
	set opcode and T flag behaviour are not supported.

	I am unsure if instructions like SBC take an extra cycle when used in
	decimal mode.  I am unsure if flag B is set upon execution of rti.

	Cycle counts should be quite accurate, illegal instructions are assumed
	to take two cycles.

	Internal timer (TIQ) not properly implemented.

******************************************************************************/

#include "memory.h"
#include "osd_dbg.h"
#include "cpuintrf.h"
#include "h6280.h"

#include <stdio.h>
extern FILE * errorlog;
extern int cpu_getpc(void);

int 	H6280_ICount = 0;
static	H6280_Regs	h6280;

#ifdef  MAME_DEBUG /* Need some public segmentation registers for debugger */
byte 	H6280_debug_mmr[8];
#endif

/* Hardwire zero page memory to Bank 8, only one Hu6280 is supported if this
is selected */
#define FAST_ZERO_PAGE

#ifdef FAST_ZERO_PAGE
unsigned char *ZEROPAGE;
#endif


/* include the macros */
#include "h6280ops.h"

/* include the opcode macros, functions and function pointer tables */
#include "tblh6280.c"

/*****************************************************************************/

unsigned H6280_GetPC (void)
{
	return PCD;
}

void H6280_GetRegs (H6280_Regs *Regs)
{
	*Regs = h6280;
}

void H6280_SetRegs (H6280_Regs *Regs)
{
	h6280 = *Regs;
}

void H6280_Reset(void)
{
	int i;

	/* wipe out the h6280 structure */
	memset(&h6280, 0, sizeof(H6280_Regs));

	/* set I and Z flags */
	P = _fI | _fZ;

    /* stack starts at 0x01ff */
	h6280.SP.D = 0x1ff;

    /* read the reset vector into PC */
	PCL = RDMEM(H6280_RESET_VEC);
	PCH = RDMEM((H6280_RESET_VEC+1));

    /* clear pending interrupts */
#if NEW_INTERRUPT_SYSTEM
	for (i = 0; i < 3; i++)
		h6280.irq_state[i] = CLEAR_LINE;
	h6280.pending_interrupt = 0;
#else
	H6280_Clear_Pending_Interrupts();
#endif

#ifdef FAST_ZERO_PAGE
{
	extern unsigned char *cpu_bankbase[];
	ZEROPAGE=cpu_bankbase[8];
}
#endif

}

int H6280_Execute(int cycles)
{
	extern int previouspc;
	int in;
	H6280_ICount = cycles;

#if 0
	/* Update timer - very wrong.. */
	if (h6280.timer_status) {
		h6280.timer_value-=cycles/2048;
		if (h6280.timer_value<0) {
			h6280.timer_value=h6280.timer_load;
//			H6280_Cause_Interrupt(H6280_INT_TIMER);
		}
	}
#endif

	/* Execute instructions */
	do
    {
		previouspc = PCD;

		#ifdef  MAME_DEBUG
	 	{
			extern int mame_debug;
			if (mame_debug) {
				/* Copy the segmentation registers for debugger to use */
				int i;
				for (i=0; i<8; i++)
					H6280_debug_mmr[i]=h6280.mmr[i];

				MAME_Debug();
			}
		}
		#endif

		/* Execute 1 instruction */
		in=RDOP();
		PCW++;
		insnh6280[in]();

		/* Check interrupts */
		if (h6280.pending_interrupt) {
			if (!(P & _fI)) {
				if ((h6280.pending_interrupt&H6280_IRQ2_MASK) && !(h6280.irq_mask&0x2))
				{
					DO_INTERRUPT(H6280_IRQ2_MASK,H6280_IRQ2_VEC);
				}

				if ((h6280.pending_interrupt&H6280_IRQ1_MASK) && !(h6280.irq_mask&0x1))
				{
					DO_INTERRUPT(H6280_IRQ1_MASK,H6280_IRQ1_VEC);
				}

				if ((h6280.pending_interrupt&H6280_TIMER_MASK) && !(h6280.irq_mask&0x4))
				{
					DO_INTERRUPT(H6280_TIMER_MASK,H6280_TIMER_VEC);
				}
			}

			if (h6280.pending_interrupt&H6280_NMI_MASK)
			{
				DO_INTERRUPT(H6280_NMI_MASK,H6280_NMI_VEC);
			}
		}

		/* If PC has not changed we are stuck in a tight loop, may as well finish */
		if (PCD==previouspc) {
//			if (errorlog && cpu_getpc()!=0xe0e6) fprintf(errorlog,"Spinning early %04x with %d cycles left\n",cpu_getpc(),H6280_ICount);
			if (H6280_ICount>0) H6280_ICount=0;
		}

	} while (H6280_ICount > 0);

	return cycles - H6280_ICount;
}

/*****************************************************************************/

#ifdef NEW_INTERRUPT_SYSTEM
void H6280_set_nmi_line(int state)
{
	if (h6280.nmi_state == state) return;
	h6280.nmi_state = state;
	if (state != CLEAR_LINE)
        h6280.pending_interrupt |= H6280_INT_NMI;
}

void H6280_set_irq_line(int irqline, int state)
{
    h6280.irq_state[irqline] = state;
	if (state == CLEAR_LINE) {
		switch (irqline) {
			case 0: h6280.pending_interrupt &= ~H6280_IRQ1_MASK; break;
			case 1: h6280.pending_interrupt &= ~H6280_IRQ2_MASK; break;
			case 2: h6280.pending_interrupt &= ~H6280_TIMER_MASK; break;
		}
	} else {
		switch (irqline) {
			case 0: h6280.pending_interrupt |= H6280_IRQ1_MASK; break;
			case 1: h6280.pending_interrupt |= H6280_IRQ2_MASK; break;
			case 2: h6280.pending_interrupt |= H6280_TIMER_MASK; break;
        }
    }
}

void H6280_set_irq_callback(int (*callback)(int irqline))
{
	h6280.irq_callback = callback;
}

#else

void H6280_Cause_Interrupt(int type)
{
    switch (type)
    {
		case H6280_INT_NONE:
            break;
		case H6280_INT_NMI:
			h6280.pending_interrupt |= H6280_NMI_MASK;
			break;
		case H6280_INT_TIMER:
			h6280.pending_interrupt |= H6280_TIMER_MASK;
			break;
		case H6280_INT_IRQ2:
			h6280.pending_interrupt |= H6280_IRQ2_MASK;
			break;
		case H6280_INT_IRQ1:
			h6280.pending_interrupt |= H6280_IRQ1_MASK;
			break;
		default:
            break;
    }
}

void H6280_Clear_Pending_Interrupts(void)
{
	h6280.pending_interrupt=H6280_INT_NONE;
}

#endif

/*****************************************************************************/

int H6280_irq_status_r(int offset)
{
	int status;

	switch (offset) {
		case 0: /* Read irq mask */
			return h6280.irq_mask;

		case 1: /* Read irq status */
//			if (errorlog) fprintf(errorlog,"Hu6280: %04x: Read irq status\n",cpu_getpc());
			status=0;
			/* Almost certainly wrong... */
//			if (h6280.pending_irq1) status^=1;
//			if (h6280.pending_irq2) status^=2;
//			if (h6280.pending_timer) status^=4;
			return status;
	}

	return 0;
}

void H6280_irq_status_w(int offset, int data)
{
	switch (offset) {
		case 0: /* Write irq mask */
//			if (errorlog) fprintf(errorlog,"Hu6280: %04x: write irq mask %04x\n",cpu_getpc(),data);
			h6280.irq_mask=data&0x7;
			return;

		case 1: /* Reset timer irq */

//aka TIQ acknowledge
// if not ack'd TIQ cant fire again?!
//
		//	if (errorlog) fprintf(errorlog,"Hu6280: %04x: Timer irq reset!\n",cpu_getpc());
			h6280.timer_value=h6280.timer_load; /* hmm */
			return;
	}
}

int H6280_timer_r(int offset)
{
	switch (offset) {
		case 0: /* Counter value */
//			if (errorlog) fprintf(errorlog,"Hu6280: %04x: Read counter value\n",cpu_getpc());
			return h6280.timer_value;

		case 1: /* Read counter status */
//			if (errorlog) fprintf(errorlog,"Hu6280: %04x: Read counter status\n",cpu_getpc());
			return h6280.timer_status;
	}

	return 0;
}

void H6280_timer_w(int offset, int data)
{
	switch (offset) {
		case 0: /* Counter preload */
			//if (errorlog) fprintf(errorlog,"Hu6280: %04x: Wrote counter preload %02x\n",cpu_getpc(),data);

//H6280_Cause_Interrupt(H6280_INT_TIMER);

			h6280.timer_load=h6280.timer_value=data;
			return;

		case 1: /* Counter enable */
			h6280.timer_status=data&1;
//			if (errorlog) fprintf(errorlog,"Hu6280: %04x: Timer status %02x\n",cpu_getpc(),data);
			return;
	}
}

/*****************************************************************************/
