/*****************************************************************************
 *
 *	 m6502.c
 *	 Portable 6502/65c02/6510 emulator V1.1
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
#include "driver.h"
#include "osd_dbg.h"
#include "m6502.h"
#include "m6502ops.h"

extern FILE * errorlog;

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	if( errorlog ) fprintf x
#else
#define LOG(x)
#endif

extern int previouspc;

int m6502_type = 0;
int m6502_ICount = 0;
static m6502_Regs m6502;

/***************************************************************
 * include the opcode macros, functions and tables
 ***************************************************************/
#include "tbl6502.c"
#include "tbl65c02.c"
#include "tbl6510.c"

/*****************************************************************************
 *
 *		6502 CPU interface functions
 *
 *****************************************************************************/

void m6502_reset(void *param)
{
	/* wipe out the m6502 structure */
    memset(&m6502, 0, sizeof(m6502_Regs));

	m6502.cpu_type = m6502_type;

	switch (m6502.cpu_type)
	{
#if SUPP65C02
		case M6502_65C02:
			m6502.insn = insn65c02;
			break;
#endif
#if SUPP6510
		case M6502_6510:
			m6502.insn = insn6510;
			break;
#endif
		default:
			m6502.insn = insn6502;
			break;
	}

	/* set T, I and Z flags */
	P = F_T | F_I | F_Z;

    /* stack starts at 0x01ff */
	m6502.sp.d = 0x1ff;

    /* read the reset vector into PC */
	PCL = RDMEM(M6502_RST_VEC);
	PCH = RDMEM(M6502_RST_VEC+1);
	change_pc16(PCD);

    m6502.pending_interrupt = 0;
    m6502.nmi_state = 0;
    m6502.irq_state = 0;
}

void m65c02_reset(void *param)
{
	m6502_type = M6502_65C02;
	m6502_reset(param);
}

void m6510_reset(void *param)
{
	m6502_type = M6502_6510;
    m6502_reset(param);
}

void m6502_exit(void)
{
	/* nothing to do yet */
}

void m6502_getregs (m6502_Regs *regs)
{
	*regs = m6502;
}

void m6502_setregs (m6502_Regs *regs)
{
	m6502 = *regs;
}

unsigned m6502_getpc (void)
{
	return PCD;
}

unsigned m6502_getreg (int regnum)
{
	switch( regnum )
	{
		case 0: return m6502.a;
		case 1: return m6502.x;
		case 2: return m6502.y;
		case 3: return m6502.sp.b.l;
		case 4: return m6502.pc.w.l;
		case 5: return m6502.ea.w.l;
		case 6: return m6502.zp.w.l;
		case 7: return m6502.nmi_state;
		case 8: return m6502.irq_state;
	}
	return 0;
}

void m6502_setreg (int regnum, unsigned val)
{
	switch( regnum )
	{
		case 0: m6502.a = val; break;
		case 1: m6502.x = val; break;
		case 2: m6502.y = val; break;
		case 3: m6502.sp.b.l = val; break;
		case 4: m6502.pc.w.l = val; break;
		case 5: m6502.ea.w.l = val; break;
		case 6: m6502.zp.w.l = val; break;
		case 7: m6502.nmi_state = val; break;
		case 8: m6502.irq_state = val; break;
	}
}

INLINE void take_nmi(void)
{
	EAD = M6502_NMI_VEC;
	m6502_ICount -= 7;
	PUSH(PCH);
	PUSH(PCL);
	PUSH(P & ~F_B);
	P = (P & ~F_D) | F_I;		/* knock out D and set I flag */
	PCL = RDMEM(EAD);
	PCH = RDMEM(EAD+1);
	LOG((errorlog,"M6502#%d takes NMI ($%04x)\n", cpu_getactivecpu(), PCD));
    m6502.pending_interrupt &= ~M6502_INT_NMI;
    change_pc16(PCD);
}

INLINE void take_irq(void)
{
	if( !(P & F_I) )
	{
		EAD = M6502_IRQ_VEC;
		m6502_ICount -= 7;
		PUSH(PCH);
		PUSH(PCL);
		PUSH(P & ~F_B);
		P = (P & ~F_D) | F_I;		/* knock out D and set I flag */
		PCL = RDMEM(EAD);
		PCH = RDMEM(EAD+1);
		LOG((errorlog,"M6502#%d takes IRQ ($%04x)\n", cpu_getactivecpu(), PCD));
		/* call back the cpuintrf to let it clear the line */
		if (m6502.irq_callback) (*m6502.irq_callback)(0);
	    change_pc16(PCD);
	}

	m6502.pending_interrupt &= ~M6502_INT_IRQ;
}

int m6502_execute(int cycles)
{
	m6502_ICount = cycles;

	change_pc16(PCD);

	do
	{
#ifdef  MAME_DEBUG
		if (mame_debug) MAME_Debug();
#endif
		previouspc = PCW;

		(*m6502.insn[RDOP()])();

		if (m6502.pending_interrupt & M6502_INT_NMI)
            take_nmi();
        /* check if the I flag was just reset (interrupts enabled) */
		if (m6502.after_cli)
		{
			LOG((errorlog,"M6502#%d after_cli was >0", cpu_getactivecpu()));
            m6502.after_cli = 0;
			if (m6502.irq_state != CLEAR_LINE)
			{
				LOG((errorlog,": irq line is asserted: set pending IRQ\n"));
                m6502.pending_interrupt |= M6502_INT_IRQ;
			}
			else
			{
				LOG((errorlog,": irq line is clear\n"));
            }
		}
		else if (m6502.pending_interrupt)
			take_irq();

    } while (m6502_ICount > 0);

	return cycles - m6502_ICount;
}

void m6502_set_nmi_line(int state)
{
	if (m6502.nmi_state == state) return;
	m6502.nmi_state = state;
	if( state != CLEAR_LINE )
	{
		LOG((errorlog, "M6502#%d set_nmi_line(ASSERT)\n", cpu_getactivecpu()));
        m6502.pending_interrupt |= M6502_INT_NMI;
	}
}

void m6502_set_irq_line(int irqline, int state)
{
	m6502.irq_state = state;
	if( state != CLEAR_LINE )
	{
		LOG((errorlog, "M6502#%d set_irq_line(ASSERT)\n", cpu_getactivecpu()));
		m6502.pending_interrupt |= M6502_INT_IRQ;
	}
}

void m6502_set_irq_callback(int (*callback)(int))
{
	m6502.irq_callback = callback;
}

void m6502_state_save(void *file)
{
	osd_fwrite(file,&m6502.cpu_type,1);
	/* insn is set at restore since it's a pointer */
	osd_fwrite_lsbfirst(file,&m6502.pc.w.l,2);
	osd_fwrite_lsbfirst(file,&m6502.sp.w.l,2);
	osd_fwrite(file,&m6502.a,1);
	osd_fwrite(file,&m6502.x,1);
	osd_fwrite(file,&m6502.y,1);
	osd_fwrite(file,&m6502.p,1);
	osd_fwrite(file,&m6502.pending_interrupt,1);
	osd_fwrite(file,&m6502.after_cli,1);
	osd_fwrite(file,&m6502.nmi_state,1);
	osd_fwrite(file,&m6502.irq_state,1);
}

void m6502_state_load(void *file)
{
	osd_fread(file,&m6502.cpu_type,1);
	switch (m6502.cpu_type)
	{
#if SUPP65C02
		case M6502_65C02:
			m6502.insn = insn65c02;
			break;
#endif
#if SUPP6510
		case M6502_6510:
			m6502.insn = insn6510;
			break;
#endif
		default:
			m6502.insn = insn6502;
			break;
	}
	osd_fread_lsbfirst(file,&m6502.pc.w.l,2);
	osd_fread_lsbfirst(file,&m6502.sp.w.l,2);
	osd_fread(file,&m6502.a,1);
	osd_fread(file,&m6502.x,1);
	osd_fread(file,&m6502.y,1);
	osd_fread(file,&m6502.p,1);
	osd_fread(file,&m6502.pending_interrupt,1);
	osd_fread(file,&m6502.after_cli,1);
	osd_fread(file,&m6502.nmi_state,1);
	osd_fread(file,&m6502.irq_state,1);
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *m6502_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	m6502_Regs *r = (m6502_Regs *)context;

	which = ++which % 16;
	buffer[which][0] = '\0';
	if( !context && regnum >= CPU_INFO_PC )
		return buffer[which];

    switch( regnum )
	{
		case CPU_INFO_NAME: return "M6502";
		case CPU_INFO_FAMILY: return "Motorola 6502";
		case CPU_INFO_VERSION: return "1.1";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (c) 1998 Juergen Buchmueller, all rights reserved.";
		case CPU_INFO_PC: sprintf(buffer[which], "%04X:", r->pc.w.l); break;
		case CPU_INFO_SP: sprintf(buffer[which], "%03X", r->sp.w.l); break;
#ifdef MAME_DEBUG
		case CPU_INFO_DASM: r->pc.w.l += Dasm6502(buffer[which], r->pc.w.l); break;
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
		case CPU_INFO_REG+ 0: sprintf(buffer[which], "A:%02X", r->a); break;
		case CPU_INFO_REG+ 1: sprintf(buffer[which], "X:%02X", r->x); break;
		case CPU_INFO_REG+ 2: sprintf(buffer[which], "Y:%02X", r->y); break;
		case CPU_INFO_REG+ 3: sprintf(buffer[which], "S:%02X", r->sp.b.l); break;
		case CPU_INFO_REG+ 4: sprintf(buffer[which], "PC:%04X", r->pc.w.l); break;
		case CPU_INFO_REG+ 5: sprintf(buffer[which], "EA:%04X", r->ea.w.l); break;
		case CPU_INFO_REG+ 6: sprintf(buffer[which], "ZP:%03X", r->zp.w.l); break;
		case CPU_INFO_REG+ 7: sprintf(buffer[which], "NMI:%d", r->nmi_state); break;
		case CPU_INFO_REG+ 8: sprintf(buffer[which], "IRQ:%d", r->irq_state); break;
	}
	return buffer[which];
}

const char *m65c02_info(void *context, int regnum)
{
	switch( regnum )
    {
		case CPU_INFO_NAME: return "M65C02";
		case CPU_INFO_VERSION: return "1.1";
	}
	return m6502_info(context,regnum);
}

const char *m6510_info(void *context, int regnum)
{
	switch( regnum )
    {
		case CPU_INFO_NAME: return "M6510";
		case CPU_INFO_VERSION: return "1.1";
	}
	return m6502_info(context,regnum);
}


