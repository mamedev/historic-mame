#include "m68000.h"
#include "memory.h"
#include <stdio.h>


void MC68000_exit(void)
{
	/* nothing to do ? */
}

void MC68000_setregs(MC68000_Regs *regs)
{
	m68k_set_context(&regs->regs);
}

void MC68000_getregs(MC68000_Regs *regs)
{
	m68k_get_context(&regs->regs);
}


unsigned MC68000_getreg(int regnum)
{
    switch( regnum )
    {
        case  0: return m68k_peek_pc();
		case  1: return 0; /* missing m68k_peek_vbr(); */
        case  2: return m68k_peek_isp();
        case  3: return m68k_peek_usp();
		case  4: return 0; /* missing m68k_peek_sfc(); */
		case  5: return 0; /* missing m68k_peek_dfc(); */
        case  6: return m68k_peek_dr(0);
        case  7: return m68k_peek_dr(1);
        case  8: return m68k_peek_dr(2);
        case  9: return m68k_peek_dr(3);
        case 10: return m68k_peek_dr(4);
        case 11: return m68k_peek_dr(5);
        case 12: return m68k_peek_dr(6);
        case 13: return m68k_peek_dr(7);
        case 14: return m68k_peek_ar(0);
        case 15: return m68k_peek_ar(1);
        case 16: return m68k_peek_ar(2);
        case 17: return m68k_peek_ar(3);
        case 18: return m68k_peek_ar(4);
        case 19: return m68k_peek_ar(5);
        case 20: return m68k_peek_ar(6);
        case 21: return m68k_peek_ar(7);
    }
    return 0;
}

void MC68000_setreg(int regnum, unsigned val)
{
    switch( regnum )
    {
        case  0: m68k_poke_pc(val); break;
		case  1: /* missing m68k_poke_vbr(val); */ break;
        case  2: m68k_poke_isp(val); break;
        case  3: m68k_poke_usp(val); break;
		case  4: /* missing m68k_poke_sfc(val); */ break;
		case  5: /* missing m68k_poke_dfc(val); */ break;
        case  6: m68k_poke_dr(0,val); break;
        case  7: m68k_poke_dr(1,val); break;
        case  8: m68k_poke_dr(2,val); break;
        case  9: m68k_poke_dr(3,val); break;
        case 10: m68k_poke_dr(4,val); break;
        case 11: m68k_poke_dr(5,val); break;
        case 12: m68k_poke_dr(6,val); break;
        case 13: m68k_poke_dr(7,val); break;
        case 14: m68k_poke_ar(0,val); break;
        case 15: m68k_poke_ar(1,val); break;
        case 16: m68k_poke_ar(2,val); break;
        case 17: m68k_poke_ar(3,val); break;
        case 18: m68k_poke_ar(4,val); break;
        case 19: m68k_poke_ar(5,val); break;
        case 20: m68k_poke_ar(6,val); break;
        case 21: m68k_poke_ar(7,val); break;
    }
}

void MC68000_set_nmi_line(int state)
{
	switch(state)
	{
		case CLEAR_LINE:
			m68k_clear_irq(7);
			return;
		case ASSERT_LINE:
			m68k_assert_irq(7);
			return;
		default:
			m68k_assert_irq(7);
			return;
	}
}

void MC68000_set_irq_line(int irqline, int state)
{
	switch(state)
	{
		case CLEAR_LINE:
			m68k_clear_irq(irqline);
			return;
		case ASSERT_LINE:
			m68k_assert_irq(irqline);
			return;
		default:
			m68k_assert_irq(irqline);
			return;
	}
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *MC68000_info(void *context, int regnum)
{
	static char buffer[32][47+1];
	static int which = 0;
	MC68000_Regs *r = (MC68000_Regs *)context;

	which = ++which % 32;
	buffer[which][0] = '\0';
	if( !context && regnum >= CPU_INFO_PC )
		return buffer[which];

	switch( regnum )
	{
		case CPU_INFO_NAME: return "MC68000";
		case CPU_INFO_FAMILY: return "Motorola 68K";
		case CPU_INFO_VERSION: return "2.0a";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright 1999 Karl Stenerud.  All rights reserved.";
		case CPU_INFO_PC: sprintf(buffer[which], "%06X:", r->regs.pc); break;
		case CPU_INFO_SP: sprintf(buffer[which], "%08X", r->regs.isp); break;
#ifdef MAME_DEBUG
		case CPU_INFO_DASM:
			change_pc24(r->regs.pc);
			r->regs.pc += Dasm68000(buffer[which], r->regs.pc);
			break;
#else
		case CPU_INFO_DASM:
			change_pc24(r->regs.pc);
            sprintf(buffer[which],"$%02x", ROM[r->regs.pc]);
			r->regs.pc++;
			break;
#endif
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				r->regs.sr & 0x8000 ? 'T':'.',
				r->regs.sr & 0x4000 ? '?':'.',
				r->regs.sr & 0x2000 ? 'S':'.',
				r->regs.sr & 0x1000 ? '?':'.',
				r->regs.sr & 0x0800 ? '?':'.',
				r->regs.sr & 0x0400 ? 'I':'.',
				r->regs.sr & 0x0200 ? 'I':'.',
				r->regs.sr & 0x0100 ? 'I':'.',
				r->regs.sr & 0x0080 ? '?':'.',
				r->regs.sr & 0x0040 ? '?':'.',
				r->regs.sr & 0x0020 ? '?':'.',
				r->regs.sr & 0x0010 ? 'X':'.',
				r->regs.sr & 0x0008 ? 'N':'.',
				r->regs.sr & 0x0004 ? 'Z':'.',
				r->regs.sr & 0x0002 ? 'V':'.',
				r->regs.sr & 0x0001 ? 'C':'.');
            break;
		case CPU_INFO_REG+ 0: sprintf(buffer[which], "PC:%08X", r->regs.pc); break;
		case CPU_INFO_REG+ 1: sprintf(buffer[which], "VBR:%08X", r->regs.vbr); break;
		case CPU_INFO_REG+ 2: sprintf(buffer[which], "ISP:%08X", r->regs.isp); break;
		case CPU_INFO_REG+ 3: sprintf(buffer[which], "USP:%08X", r->regs.usp); break;
		case CPU_INFO_REG+ 4: sprintf(buffer[which], "SFC:%08X", r->regs.sfc); break;
		case CPU_INFO_REG+ 5: sprintf(buffer[which], "DFC:%08X", r->regs.dfc); break;
		case CPU_INFO_REG+ 6: sprintf(buffer[which], "D0:%08X", r->regs.d[0]); break;
		case CPU_INFO_REG+ 7: sprintf(buffer[which], "D1:%08X", r->regs.d[1]); break;
		case CPU_INFO_REG+ 8: sprintf(buffer[which], "D2:%08X", r->regs.d[2]); break;
		case CPU_INFO_REG+ 9: sprintf(buffer[which], "D3:%08X", r->regs.d[3]); break;
		case CPU_INFO_REG+10: sprintf(buffer[which], "D4:%08X", r->regs.d[4]); break;
		case CPU_INFO_REG+11: sprintf(buffer[which], "D5:%08X", r->regs.d[5]); break;
		case CPU_INFO_REG+12: sprintf(buffer[which], "D6:%08X", r->regs.d[6]); break;
		case CPU_INFO_REG+13: sprintf(buffer[which], "D7:%08X", r->regs.d[7]); break;
		case CPU_INFO_REG+14: sprintf(buffer[which], "A0:%08X", r->regs.a[0]); break;
		case CPU_INFO_REG+15: sprintf(buffer[which], "A1:%08X", r->regs.a[1]); break;
		case CPU_INFO_REG+16: sprintf(buffer[which], "A2:%08X", r->regs.a[2]); break;
		case CPU_INFO_REG+17: sprintf(buffer[which], "A3:%08X", r->regs.a[3]); break;
		case CPU_INFO_REG+18: sprintf(buffer[which], "A4:%08X", r->regs.a[4]); break;
		case CPU_INFO_REG+19: sprintf(buffer[which], "A5:%08X", r->regs.a[5]); break;
		case CPU_INFO_REG+20: sprintf(buffer[which], "A6:%08X", r->regs.a[6]); break;
        case CPU_INFO_REG+21: sprintf(buffer[which], "A7:%08X", r->regs.a[7]); break;
    }
	return buffer[which];
}

const char *MC68010_info(void *context, int regnum)
{
	switch( regnum )
	{
		case CPU_INFO_NAME: return "MC68010";
	}
	return MC68000_info(context,regnum);
}

const char *MC68020_info(void *context, int regnum)
{
	switch( regnum )
	{
		case CPU_INFO_NAME: return "MC68020";
	}
	return MC68000_info(context,regnum);
}


