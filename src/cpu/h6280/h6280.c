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
#include <string.h>
extern FILE * errorlog;
extern unsigned cpu_get_pc(void);

int 	h6280_ICount = 0;

/****************************************************************************
 * The 6280 registers.
 ****************************************************************************/
typedef struct
{
    PAIR  pc;                       /* program counter */
    PAIR  sp;                       /* stack pointer (always 100 - 1FF) */
    PAIR  zp;                       /* zero page address */
    PAIR  ea;                       /* effective address */
    UINT8 a;                        /* Accumulator */
    UINT8 x;                        /* X index register */
    UINT8 y;                        /* Y index register */
    UINT8 p;                        /* Processor status */
    UINT8 mmr[8];                   /* Hu6280 memory mapper registers */
    UINT8 irq_mask;                 /* interrupt enable/disable */
    UINT8 timer_status;             /* timer status */
    int timer_value;                /* timer interrupt */
    UINT8 timer_load;               /* reload value */
    int pending_interrupt;          /* nonzero if an interrupt is pending */
    int nmi_state;
    int irq_state[3];
    int (*irq_callback)(int irqline);

#if LAZY_FLAGS
    int NZ;                         /* last value (lazy N and Z flag) */
#endif

}   h6280_Regs;

static  h6280_Regs  h6280;

#ifdef  MAME_DEBUG /* Need some public segmentation registers for debugger */
UINT8	H6280_debug_mmr[8];
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

void h6280_reset(void *param)
{
	int i;

	/* wipe out the h6280 structure */
	memset(&h6280, 0, sizeof(h6280_Regs));

	/* set I and Z flags */
	P = _fI | _fZ;

    /* stack starts at 0x01ff */
	h6280.sp.d = 0x1ff;

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

void h6280_exit(void)
{
	/* nothing to do ? */
}

int h6280_execute(int cycles)
{
	extern int previouspc;
	int in;
	h6280_ICount = cycles;

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
//			if (errorlog && cpu_get_pc()!=0xe0e6) fprintf(errorlog,"Spinning early %04x with %d cycles left\n",cpu_get_pc(),h6280_ICount);
			if (h6280_ICount>0) h6280_ICount=0;
		}

	} while (h6280_ICount > 0);

	return cycles - h6280_ICount;
}

unsigned h6280_get_context (void *dst)
{
	if( dst )
		*(h6280_Regs*)dst = h6280;
	return sizeof(h6280_Regs);
}

void h6280_set_context (void *src)
{
	if( src )
		h6280 = *(h6280_Regs*)src;
}

unsigned h6280_get_pc (void)
{
    return PCD;
}

void h6280_set_pc (unsigned val)
{
	PCW = val;
	change_pc(PCD);
}

unsigned h6280_get_sp (void)
{
	return S;
}

void h6280_set_sp (unsigned val)
{
	S = val;
}

unsigned h6280_get_reg (int regnum)
{
	switch( regnum )
	{
		case H6280_A: return h6280.a;
		case H6280_X: return h6280.x;
		case H6280_Y: return h6280.y;
		case H6280_S: return h6280.sp.b.l;
		case H6280_PC: return h6280.pc.w.l;
		case H6280_P: return h6280.p;
		case H6280_IRQ_MASK: return h6280.irq_mask;
		case H6280_TIMER_STATE: return h6280.timer_status;
		case H6280_NMI_STATE: return h6280.nmi_state;
		case H6280_IRQ1_STATE: return h6280.irq_state[0];
		case H6280_IRQ2_STATE: return h6280.irq_state[1];
		case H6280_IRQT_STATE: return h6280.irq_state[2];
#ifdef MAME_DEBUG
		case H6280_M1: return h6280.mmr[0];
		case H6280_M2: return h6280.mmr[1];
		case H6280_M3: return h6280.mmr[2];
		case H6280_M4: return h6280.mmr[3];
		case H6280_M5: return h6280.mmr[4];
		case H6280_M6: return h6280.mmr[5];
		case H6280_M7: return h6280.mmr[6];
		case H6280_M8: return h6280.mmr[7];
#endif
	}
	return 0;
}

void h6280_set_reg (int regnum, unsigned val)
{
	switch( regnum )
	{
		case H6280_A: h6280.a = val; break;
		case H6280_X: h6280.x = val; break;
		case H6280_Y: h6280.y = val; break;
		case H6280_S: h6280.sp.b.l = val; break;
		case H6280_PC: h6280.pc.w.l = val; break;
		case H6280_P: h6280.p = val; break;
		case H6280_IRQ_MASK: h6280.irq_mask = val; break;
		case H6280_TIMER_STATE: h6280.timer_status = val; break;
		case H6280_NMI_STATE: h6280.nmi_state = val; break;
		case H6280_IRQ1_STATE: h6280.irq_state[0] = val; break;
		case H6280_IRQ2_STATE: h6280.irq_state[1] = val; break;
		case H6280_IRQT_STATE: h6280.irq_state[2] = val; break;
#ifdef MAME_DEBUG
		case H6280_M1: h6280.mmr[0] = val; break;
		case H6280_M2: h6280.mmr[1] = val; break;
		case H6280_M3: h6280.mmr[2] = val; break;
		case H6280_M4: h6280.mmr[3] = val; break;
		case H6280_M5: h6280.mmr[4] = val; break;
		case H6280_M6: h6280.mmr[5] = val; break;
		case H6280_M7: h6280.mmr[6] = val; break;
		case H6280_M8: h6280.mmr[7] = val; break;
#endif
    }
}

/*****************************************************************************/

void h6280_set_nmi_line(int state)
{
	if (h6280.nmi_state == state) return;
	h6280.nmi_state = state;
	if (state != CLEAR_LINE)
        h6280.pending_interrupt |= H6280_INT_NMI;
}

void h6280_set_irq_line(int irqline, int state)
{
    h6280.irq_state[irqline] = state;
	if (state == CLEAR_LINE)
	{
		switch (irqline)
		{
			case 0: h6280.pending_interrupt &= ~H6280_IRQ1_MASK; break;
			case 1: h6280.pending_interrupt &= ~H6280_IRQ2_MASK; break;
			case 2: h6280.pending_interrupt &= ~H6280_TIMER_MASK; break;
		}
	}
	else
	{
		switch (irqline)
		{
			case 0: h6280.pending_interrupt |= H6280_IRQ1_MASK; break;
			case 1: h6280.pending_interrupt |= H6280_IRQ2_MASK; break;
			case 2: h6280.pending_interrupt |= H6280_TIMER_MASK; break;
        }
    }
}

void h6280_set_irq_callback(int (*callback)(int irqline))
{
	h6280.irq_callback = callback;
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *h6280_info(void *context, int regnum)
{
	static char buffer[32][47+1];
	static int which = 0;
	h6280_Regs *r = context;

	which = ++which % 32;
	buffer[which][0] = '\0';
	if( !context )
		r = &h6280;

	switch( regnum )
	{
		case CPU_INFO_NAME: return "H6280";
		case CPU_INFO_FAMILY: return "Hitachi 6280";
		case CPU_INFO_VERSION: return "1.01";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (c) 1999 Bryan McPhail, mish@tendril.force9.net";

		case CPU_INFO_PC: sprintf(buffer[which], "%04X:", r->pc.w.l); break;
		case CPU_INFO_SP: sprintf(buffer[which], "%02X", r->sp.b.l); break;
#ifdef MAME_DEBUG
		case CPU_INFO_DASM: r->pc.w.l += Dasm6280(buffer[which], r->pc.w.l); break;
#else
		case CPU_INFO_DASM: sprintf(buffer[which], "$%02x", ROM[r->pc.w.l]); r->pc.w.l++; break;
#endif
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				r->p & 0x80 ? 'N':'.',
				r->p & 0x40 ? 'V':'.',
				r->p & 0x20 ? 'R':'.',
				r->p & 0x10 ? 'B':'.',
				r->p & 0x08 ? 'D':'.',
				r->p & 0x04 ? 'I':'.',
				r->p & 0x02 ? 'Z':'.',
				r->p & 0x01 ? 'C':'.');
			break;
		case CPU_INFO_REG+H6280_A: sprintf(buffer[which], "A:%02X", r->a); break;
		case CPU_INFO_REG+H6280_X: sprintf(buffer[which], "X:%02X", r->x); break;
		case CPU_INFO_REG+H6280_Y: sprintf(buffer[which], "Y:%02X", r->y); break;
		case CPU_INFO_REG+H6280_S: sprintf(buffer[which], "S:%02X", r->sp.b.l); break;
		case CPU_INFO_REG+H6280_PC: sprintf(buffer[which], "PC:%04X", r->pc.w.l); break;
		case CPU_INFO_REG+H6280_P: sprintf(buffer[which], "P:%02X", r->p); break;
		case CPU_INFO_REG+H6280_IRQ_MASK: sprintf(buffer[which], "IRQ_MASK:%02X", r->irq_mask); break;
		case CPU_INFO_REG+H6280_TIMER_STATE: sprintf(buffer[which], "TIMER:%02X", r->timer_status); break;
		case CPU_INFO_REG+H6280_NMI_STATE: sprintf(buffer[which], "NMI:%X", r->nmi_state); break;
		case CPU_INFO_REG+H6280_IRQ1_STATE: sprintf(buffer[which], "IRQ1:%X", r->irq_state[0]); break;
		case CPU_INFO_REG+H6280_IRQ2_STATE: sprintf(buffer[which], "IRQ2:%X", r->irq_state[1]); break;
		case CPU_INFO_REG+H6280_IRQT_STATE: sprintf(buffer[which], "IRQT:%X", r->irq_state[2]); break;
#ifdef MAME_DEBUG
		case CPU_INFO_REG+H6280_M1: sprintf(buffer[which], "M1:%02X", r->mmr[0]); break;
		case CPU_INFO_REG+H6280_M2: sprintf(buffer[which], "M2:%02X", r->mmr[1]); break;
		case CPU_INFO_REG+H6280_M3: sprintf(buffer[which], "M3:%02X", r->mmr[2]); break;
		case CPU_INFO_REG+H6280_M4: sprintf(buffer[which], "M4:%02X", r->mmr[3]); break;
		case CPU_INFO_REG+H6280_M5: sprintf(buffer[which], "M5:%02X", r->mmr[4]); break;
		case CPU_INFO_REG+H6280_M6: sprintf(buffer[which], "M6:%02X", r->mmr[5]); break;
		case CPU_INFO_REG+H6280_M7: sprintf(buffer[which], "M7:%02X", r->mmr[6]); break;
		case CPU_INFO_REG+H6280_M8: sprintf(buffer[which], "M8:%02X", r->mmr[7]); break;
#endif
    }
	return buffer[which];
}

/*****************************************************************************/

int H6280_irq_status_r(int offset)
{
	int status;

	switch (offset) {
		case 0: /* Read irq mask */
			return h6280.irq_mask;

		case 1: /* Read irq status */
//			if (errorlog) fprintf(errorlog,"Hu6280: %04x: Read irq status\n",cpu_get_pc());
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
//			if (errorlog) fprintf(errorlog,"Hu6280: %04x: write irq mask %04x\n",cpu_get_pc(),data);
			h6280.irq_mask=data&0x7;
			return;

		case 1: /* Reset timer irq */

//aka TIQ acknowledge
// if not ack'd TIQ cant fire again?!
//
		//	if (errorlog) fprintf(errorlog,"Hu6280: %04x: Timer irq reset!\n",cpu_get_pc());
			h6280.timer_value=h6280.timer_load; /* hmm */
			return;
	}
}

int H6280_timer_r(int offset)
{
	switch (offset) {
		case 0: /* Counter value */
//			if (errorlog) fprintf(errorlog,"Hu6280: %04x: Read counter value\n",cpu_get_pc());
			return h6280.timer_value;

		case 1: /* Read counter status */
//			if (errorlog) fprintf(errorlog,"Hu6280: %04x: Read counter status\n",cpu_get_pc());
			return h6280.timer_status;
	}

	return 0;
}

void H6280_timer_w(int offset, int data)
{
	switch (offset) {
		case 0: /* Counter preload */
			//if (errorlog) fprintf(errorlog,"Hu6280: %04x: Wrote counter preload %02x\n",cpu_get_pc(),data);

//H6280_Cause_Interrupt(H6280_INT_TIMER);

			h6280.timer_load=h6280.timer_value=data;
			return;

		case 1: /* Counter enable */
			h6280.timer_status=data&1;
//			if (errorlog) fprintf(errorlog,"Hu6280: %04x: Timer status %02x\n",cpu_get_pc(),data);
			return;
	}
}

/*****************************************************************************/
