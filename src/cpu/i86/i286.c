/****************************************************************************
 *			   real mode i286 emulator v1.4 by Fabrice Frances				*
 *				 (initial work based on David Hedley's pcemu)               *
 ****************************************************************************/

#include <stdio.h>
#include <string.h>
#include "host.h"
#include "cpuintrf.h"
#include "memory.h"
#include "mamedbg.h"
#include "mame.h"

#define i86_ICount i286_ICount

#include "i286.h"
#include "i286intf.h"


static UINT8 i286_reg_layout[] = {
	I286_AX, I286_BX, I286_DS, I286_ES, I286_SS, I286_FLAGS, I286_CS, I286_VECTOR, -1,
	I286_CX, I286_DX, I286_SI, I286_DI, I286_SP, I286_BP, I286_IP,
	I286_IRQ_STATE, I286_NMI_STATE, 0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 i286_win_layout[] = {
	 0, 0,80, 2,	/* register window (top rows) */
	 0, 3,34,19,	/* disassembler window (left colums) */
	35, 3,45, 9,	/* memory #1 window (right, upper middle) */
	35,13,45, 9,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};


/***************************************************************************/
/* cpu state															   */
/***************************************************************************/
/* I86 registers */
typedef union
{					/* eight general registers */
	UINT16 w[8];	/* viewed as 16 bits registers */
	UINT8  b[16];	/* or as 8 bit registers */
} i286basicregs;

typedef struct
{
	i286basicregs regs;
	int 	amask;			/* address mask */
	UINT16	ip;
	UINT16	flags;
	UINT32	base[4];
	UINT16	sregs[4];
	int 	(*irq_callback)(int irqline);
	int 	AuxVal, OverVal, SignVal, ZeroVal, CarryVal, ParityVal; /* 0 or non-0 valued flags */
	UINT8	TF, IF, DF; 	/* 0 or 1 valued flags */
	UINT8	int_vector;
	UINT8	pending_irq;
	INT8	nmi_state;
	INT8	irq_state;
} i286_Regs;

int i286_ICount;

static i286_Regs I;
static unsigned prefix_base;	/* base address of the latest prefix segment */
static char seg_prefix; 		/* prefix segment indicator */

#define INT_IRQ 0x01
#define NMI_IRQ 0x02

static UINT8 parity_table[256];
/***************************************************************************/

#define PREFIX(fname) i286##fname
#define PREFIX86(fname) i286##fname
#define PREFIX186(fname) i286##fname
#define PREFIX286(fname) i286##fname

#include "ea.h"
#include "modrm.h"
#include "instr86.h"
#include "instr186.h"
#include "table286.h"
#include "instr86.c"
#include "instr186.c"

void i286_reset (void *param)
{
	unsigned int i,j,c;
	BREGS reg_name[8]={ AL, CL, DL, BL, AH, CH, DH, BH };

	memset( &I, 0, sizeof(I) );

	/* If a reset parameter is given, take it as pointer to an address mask */
	if( param )
		I.amask = *(unsigned*)param;
	else
		I.amask = 0x00ffff;
	I.sregs[CS] = 0xffff;
	I.base[CS] = I.sregs[CS] << 4;

	CHANGE_PC( (I.base[CS] + I.ip) & I.amask);

	for (i = 0;i < 256; i++)
	{
		for (j = i, c = 0; j > 0; j >>= 1)
			if (j & 1) c++;

		parity_table[i] = !(c & 1);
	}

	I.ZeroVal = I.ParityVal = 1;

	for (i = 0; i < 256; i++)
	{
		Mod_RM.reg.b[i] = reg_name[(i & 0x38) >> 3];
		Mod_RM.reg.w[i] = (WREGS) ( (i & 0x38) >> 3) ;
	}

	for (i = 0xc0; i < 0x100; i++)
	{
		Mod_RM.RM.w[i] = (WREGS)( i & 7 );
		Mod_RM.RM.b[i] = (BREGS)reg_name[i & 7];
	}
}

void i286_exit (void)
{
	/* nothing to do ? */
}

/****************************************************************************/

/* ASG 971222 -- added these interface functions */

unsigned i286_get_context(void *dst)
{
	if( dst )
		*(i286_Regs*)dst = I;
	return sizeof(i286_Regs);
}

void i286_set_context(void *src)
{
	if( src )
	{
		I = *(i286_Regs*)src;
		I.base[CS] = SegBase(CS);
		I.base[DS] = SegBase(DS);
		I.base[ES] = SegBase(ES);
		I.base[SS] = SegBase(SS);
		CHANGE_PC((I.base[CS]+I.ip) & I.amask);
	}
}

unsigned i286_get_pc(void)
{
	return (I.base[CS] + (WORD)I.ip) & I.amask;
}

void i286_set_pc(unsigned val)
{
	if( val - I.base[CS] < 0x10000 )
	{
		I.ip = val - I.base[CS];
	}
	else
	{
		I.base[CS] = val & 0xffff0;
		I.sregs[CS] = I.base[CS] >> 4;
		I.ip = val & 0x0000f;
	}
}

unsigned i286_get_sp(void)
{
	return I.base[SS] + I.regs.w[SP];
}

void i286_set_sp(unsigned val)
{
	if( val - I.base[SS] < 0x10000 )
	{
		I.regs.w[SP] = val - I.base[SS];
	}
	else
	{
		I.base[SS] = val & 0xffff0;
		I.sregs[SS] = I.base[SS] >> 4;
		I.regs.w[SP] = val & 0x0000f;
	}
}

unsigned i286_get_reg(int regnum)
{
	switch( regnum )
	{
		case I286_IP: return I.ip;
		case I286_SP: return I.regs.w[SP];
		case I286_FLAGS: CompressFlags(); return I.flags;
		case I286_AX: return I.regs.w[AX];
		case I286_CX: return I.regs.w[CX];
		case I286_DX: return I.regs.w[DX];
		case I286_BX: return I.regs.w[BX];
		case I286_BP: return I.regs.w[BP];
		case I286_SI: return I.regs.w[SI];
		case I286_DI: return I.regs.w[DI];
		case I286_ES: return I.sregs[ES];
		case I286_CS: return I.sregs[CS];
		case I286_SS: return I.sregs[SS];
		case I286_DS: return I.sregs[DS];
		case I286_VECTOR: return I.int_vector;
		case I286_PENDING: return I.pending_irq;
		case I286_NMI_STATE: return I.nmi_state;
		case I286_IRQ_STATE: return I.irq_state;
		case REG_PREVIOUSPC: return 0;	/* not supported */
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = ((I.base[SS] + I.regs.w[SP]) & I.amask) + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < I.amask )
					return cpu_readmem24lew( offset ) | ( cpu_readmem24lew( offset + 1) << 8 );
			}
	}
	return 0;
}

void i286_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case I286_IP: I.ip = val; break;
		case I286_SP: I.regs.w[SP] = val; break;
		case I286_FLAGS: I.flags = val; ExpandFlags(val); break;
		case I286_AX: I.regs.w[AX] = val; break;
		case I286_CX: I.regs.w[CX] = val; break;
		case I286_DX: I.regs.w[DX] = val; break;
		case I286_BX: I.regs.w[BX] = val; break;
		case I286_BP: I.regs.w[BP] = val; break;
		case I286_SI: I.regs.w[SI] = val; break;
		case I286_DI: I.regs.w[DI] = val; break;
		case I286_ES: I.sregs[ES] = val; break;
		case I286_CS: I.sregs[CS] = val; break;
		case I286_SS: I.sregs[SS] = val; break;
		case I286_DS: I.sregs[DS] = val; break;
		case I286_VECTOR: I.int_vector = val; break;
		case I286_PENDING: I.pending_irq = val; break;
		case I286_NMI_STATE: i286_set_nmi_line(val); break;
		case I286_IRQ_STATE: i286_set_irq_line(0,val); break;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = ((I.base[SS] + I.regs.w[SP]) & I.amask) + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < I.amask - 1 )
				{
					cpu_writemem24lew( offset, val & 0xff );
					cpu_writemem24lew( offset+1, (val >> 8) & 0xff );
				}
			}
	}
}

void i286_set_nmi_line(int state)
{
	if( I.nmi_state == state ) return;
	I.nmi_state = state;
	if (state != CLEAR_LINE)
	{
		I.pending_irq |= NMI_IRQ;
	}
}

void i286_set_irq_line(int irqline, int state)
{
	I.irq_state = state;
	if (state == CLEAR_LINE)
	{
		//if (!I.IF)
			I.pending_irq &= ~INT_IRQ;
	}
	else
	{
		//if (I.IF)
			I.pending_irq |= INT_IRQ;
	}
}

void i286_set_irq_callback(int (*callback)(int))
{
	I.irq_callback = callback;
}

int i286_execute(int cycles)
{
	i286_ICount=cycles; /* ASG 971222 cycles_per_run;*/
	while(i286_ICount>0)
	{

//#define VERBOSE_DEBUG
#ifdef VERBOSE_DEBUG
		 logerror("[%04x:%04x]=%02x\tF:%04x\tAX=%04x\tBX=%04x\tCX=%04x\tDX=%04x %d%d%d%d%d%d%d%d%d\n",I.sregs[CS],I.ip,GetMemB(CS,I.ip),I.flags,I.regs.w[AX],I.regs.w[BX],I.regs.w[CX],I.regs.w[DX], I.AuxVal?1:0, I.OverVal?1:0, I.SignVal?1:0, I.ZeroVal?1:0, I.CarryVal?1:0, I.ParityVal?1:0,I.TF, I.IF, I.DF);
#endif

		if ((I.pending_irq && I.IF) || (I.pending_irq & NMI_IRQ))
			i286_external_int();	 /* HJB 12/15/98 */

		CALL_MAME_DEBUG;

		seg_prefix=FALSE;

		TABLE286 /* call instruction */

	}
	return cycles - i286_ICount;
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *i286_info(void *context, int regnum)
{
	static char buffer[32][63+1];
	static int which = 0;
	i286_Regs *r = context;

	which = ++which % 32;
	buffer[which][0] = '\0';
	if( !context )
		r = &I;

	switch( regnum )
	{
		case CPU_INFO_REG+I286_IP: sprintf(buffer[which], "IP:%04X", r->ip); break;
		case CPU_INFO_REG+I286_SP: sprintf(buffer[which], "SP:%04X", r->regs.w[SP]); break;
		case CPU_INFO_REG+I286_FLAGS: sprintf(buffer[which], "F:%04X", r->flags); break;
		case CPU_INFO_REG+I286_AX: sprintf(buffer[which], "AX:%04X", r->regs.w[AX]); break;
		case CPU_INFO_REG+I286_CX: sprintf(buffer[which], "CX:%04X", r->regs.w[CX]); break;
		case CPU_INFO_REG+I286_DX: sprintf(buffer[which], "DX:%04X", r->regs.w[DX]); break;
		case CPU_INFO_REG+I286_BX: sprintf(buffer[which], "BX:%04X", r->regs.w[BX]); break;
		case CPU_INFO_REG+I286_BP: sprintf(buffer[which], "BP:%04X", r->regs.w[BP]); break;
		case CPU_INFO_REG+I286_SI: sprintf(buffer[which], "SI:%04X", r->regs.w[SI]); break;
		case CPU_INFO_REG+I286_DI: sprintf(buffer[which], "DI:%04X", r->regs.w[DI]); break;
		case CPU_INFO_REG+I286_ES: sprintf(buffer[which], "ES:%04X", r->sregs[ES]); break;
		case CPU_INFO_REG+I286_CS: sprintf(buffer[which], "CS:%04X", r->sregs[CS]); break;
		case CPU_INFO_REG+I286_SS: sprintf(buffer[which], "SS:%04X", r->sregs[SS]); break;
		case CPU_INFO_REG+I286_DS: sprintf(buffer[which], "DS:%04X", r->sregs[DS]); break;
		case CPU_INFO_REG+I286_VECTOR: sprintf(buffer[which], "V:%02X", r->int_vector); break;
		case CPU_INFO_REG+I286_PENDING: sprintf(buffer[which], "P:%X", r->pending_irq); break;
		case CPU_INFO_REG+I286_NMI_STATE: sprintf(buffer[which], "NMI:%X", r->nmi_state); break;
		case CPU_INFO_REG+I286_IRQ_STATE: sprintf(buffer[which], "IRQ:%X", r->irq_state); break;
		case CPU_INFO_FLAGS:
			r->flags = CompressFlags();
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				r->flags & 0x8000 ? '?':'.',
				r->flags & 0x4000 ? '?':'.',
				r->flags & 0x2000 ? '?':'.',
				r->flags & 0x1000 ? '?':'.',
				r->flags & 0x0800 ? 'O':'.',
				r->flags & 0x0400 ? 'D':'.',
				r->flags & 0x0200 ? 'I':'.',
				r->flags & 0x0100 ? 'T':'.',
				r->flags & 0x0080 ? 'S':'.',
				r->flags & 0x0040 ? 'Z':'.',
				r->flags & 0x0020 ? '?':'.',
				r->flags & 0x0010 ? 'A':'.',
				r->flags & 0x0008 ? '?':'.',
				r->flags & 0x0004 ? 'P':'.',
				r->flags & 0x0002 ? 'N':'.',
				r->flags & 0x0001 ? 'C':'.');
			break;
		case CPU_INFO_NAME: return "I286";
		case CPU_INFO_FAMILY: return "Intel 80286";
		case CPU_INFO_VERSION: return "1.4";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Real mode i286 emulator v1.4 by Fabrice Frances\n(initial work I.based on David Hedley's pcemu)";
		case CPU_INFO_REG_LAYOUT: return (const char*)i286_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)i286_win_layout;
	}
	return buffer[which];
}

unsigned i286_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
	return DasmI286(buffer,pc);
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}

