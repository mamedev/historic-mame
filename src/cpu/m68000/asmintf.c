/*
	Interface routine for 68kem <-> Mame

*/

#include "driver.h"
#include "m68000.h"
#include "osd_dbg.h"

#ifdef WIN32
#define CONVENTION __cdecl
#else
#define CONVENTION
#endif

m68k_cpu_context regs;

extern void CONVENTION M68KRUN(void);
extern void CONVENTION M68KRESET(void);

int (*m68000_irq_callback)(int irqline);

/********************************************/
/* Interface routines to link Mame -> 68KEM */
/********************************************/

void MC68000_reset(void *param)
{
	memset(&regs,0,sizeof(regs));

    regs.a[7] = regs.isp = cpu_readmem24_dword(0);
    regs.pc   = cpu_readmem24_dword(4) & 0xffffff;
    regs.sr_high = 0x27;
   	regs.sr = 0x2700;

    M68KRESET();
}


void MC68000_exit(void)
{
	/* nothing to do ? */
}


int  MC68000_execute(int cycles)
{
	if (regs.IRQ_level == 0x80) return cycles;		/* STOP with no IRQs */

  	MC68000_ICount = cycles;

#ifdef MAME_DEBUG

    do
    {
        #if 0				/* Trace */

        if (errorlog)
        {
			int mycount, areg, dreg;

            areg = dreg = 0;
	        for (mycount=7;mycount>=0;mycount--)
            {
            	areg = areg + regs.a[mycount];
                dreg = dreg + regs.d[mycount];
            }

           	fprintf(errorlog,"=> %8x %8x ",areg,dreg);
            fprintf(errorlog,"%6x %4x %d\n",regs.pc,regs.sr,MC68000_ICount);
        }
        #endif

		{
			extern int mame_debug;

			if (mame_debug)
            {
				MAME_Debug();
            }
		}

		M68KRUN();
    }
	while (MC68000_ICount > 0);

#else

	M68KRUN();

#endif /* MAME_DEBUG */

    return (cycles - MC68000_ICount);
}


void MC68000_setregs(MC68000_Regs *src)
{
	regs = src->regs;
}

void MC68000_getregs(MC68000_Regs *dst)
{
  	dst->regs = regs;
}

unsigned MC68000_getpc(void)
{
    return regs.pc;
}

unsigned MC68000_getreg(int regnum)
{
    switch( regnum )
    {
        case  0: regs.pc; break;
        case  1: regs.vbr; break;
        case  2: regs.isp; break;
        case  3: regs.usp; break;
        case  4: regs.sfc; break;
        case  5: regs.dfc; break;
        case  6: regs.d[0]; break;
        case  7: regs.d[1]; break;
        case  8: regs.d[2]; break;
        case  9: regs.d[3]; break;
        case 10: regs.d[4]; break;
        case 11: regs.d[5]; break;
        case 12: regs.d[6]; break;
        case 13: regs.d[7]; break;
        case 14: regs.a[0]; break;
        case 15: regs.a[1]; break;
        case 16: regs.a[2]; break;
        case 17: regs.a[3]; break;
        case 18: regs.a[4]; break;
        case 19: regs.a[5]; break;
        case 20: regs.a[6]; break;
        case 21: regs.a[7]; break;
    }
    return 0;
}

void MC68000_setreg(int regnum, unsigned val)
{
    switch( regnum )
    {
        case  0: regs.pc = val; break;
        case  1: regs.vbr = val; break;
        case  2: regs.isp = val; break;
        case  3: regs.usp = val; break;
        case  4: regs.sfc = val; break;
        case  5: regs.dfc = val; break;
        case  6: regs.d[0] = val; break;
        case  7: regs.d[1] = val; break;
        case  8: regs.d[2] = val; break;
        case  9: regs.d[3] = val; break;
        case 10: regs.d[4] = val; break;
        case 11: regs.d[5] = val; break;
        case 12: regs.d[6] = val; break;
        case 13: regs.d[7] = val; break;
        case 14: regs.a[0] = val; break;
        case 15: regs.a[1] = val; break;
        case 16: regs.a[2] = val; break;
        case 17: regs.a[3] = val; break;
        case 18: regs.a[4] = val; break;
        case 19: regs.a[5] = val; break;
        case 20: regs.a[6] = val; break;
        case 21: regs.a[7] = val; break;
    }
}

void MC68000_set_nmi_line(int state)
{
	/* the 68K does not have a dedicated NMI line */
}

void MC68000_set_irq_line(int irqline, int state)
{
//	if (errorlog) fprintf(errorlog, "Set IRQ Line %x = %x\n",irqline,state);

	if (state == CLEAR_LINE)
	{
		regs.IRQ_level &= ~(1 << (irqline - 1));
	}
	else
	{
		/* the real IRQ level is retrieved inside interrupt handler */
        regs.IRQ_level |= (1 << (irqline - 1));
	}
}

void MC68000_set_irq_callback(int (*callback)(int irqline))
{
	regs.irq_callback = callback;
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *MC68000_info(void *context, int regnum)
{
#ifdef MAME_DEBUG
extern int m68k_disassemble(char* str_buff, int pc);
#endif
    static char buffer[32][47+1];
	static int which;
	MC68000_Regs *r = (MC68000_Regs *)context;

	which = ++which % 32;
	buffer[which][0] = '\0';
	if( !context && regnum >= CPU_INFO_PC )
		return buffer[which];

	switch( regnum )
	{
		case CPU_INFO_NAME: return "MC68000";
		case CPU_INFO_FAMILY: return "Motorola 68K";
		case CPU_INFO_VERSION: return "0.10";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright 1998,99 Mike Coates, Darren Olafson. All rights reserved";

		case CPU_INFO_PC: sprintf(buffer[which], "%06X:", r->regs.pc); break;
		case CPU_INFO_SP: sprintf(buffer[which], "%08X", r->regs.isp); break;
#ifdef MAME_DEBUG
		case CPU_INFO_DASM:
			change_pc24(r->regs.pc);
			r->regs.pc += m68k_disassemble(buffer[which], r->regs.pc);
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


