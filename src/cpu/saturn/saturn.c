/*****************************************************************************
 *
 *	 saturn.c
 *	 portable saturn emulator interface
 *   (hp calculators)
 *
 *	 Copyright (c) 2000 Peter Trauner, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   peter.trauner@jk.uni-linz.ac.at
 *	 - The author of this copywritten work reserves the right to change the
 *	   terms of its usage and license at any time, including retroactively
 *	 - This entire notice must remain in the source code.
 *
 *****************************************************************************/
#include <stdio.h>
#include "driver.h"
#include "state.h"
#include "mamedbg.h"

#include "saturn.h"
#include "sat.h"

typedef int bool;

#define R0 0
#define R1 1
#define R2 2
#define R3 3
#define R4 4
#define A 5
#define B 6
#define C 7
#define D 8

typedef int SaturnAdr; // 20 bit
typedef UINT8 SaturnNib; // 4 bit
typedef short SaturnX; // 12 bit
typedef INT64 SaturnM; // 48 bit

typedef union { 
	UINT8 b[8];
	UINT16 w[4];
	UINT32 d[2];
	UINT64 q; 
} Saturn64;

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)
#endif

#ifdef RUNTIME_LOADER
#define saturn_ICount saturn_icount
struct cpu_interface
saturn_interface=
CPU0(SATURN,  saturn,  1,  0,1.00,SATURN_INT_NONE,  SATURN_INT_IRQ,SATURN_INT_NMI, 8, 20,     0,20,LE,1, 21);
extern void saturn_runtime_loader_init(void)
{
	cpuintf[CPU_SATURN]=saturn_interface;
}
#endif

/* Layout of the registers in the debugger */
static UINT8 saturn_reg_layout[] = {
	SATURN_A, 
	SATURN_RSTK0,
    SATURN_PC,
	-1,

	SATURN_B, 
	SATURN_RSTK1,
	SATURN_D0, 
	-1,

	SATURN_C, 
	SATURN_RSTK2,
	SATURN_D1,
	-1,

	SATURN_D,
	SATURN_RSTK3,
	SATURN_P,
	-1,

	SATURN_R0, 
	SATURN_RSTK4,
	SATURN_HST,
	-1,

	SATURN_R1, 
	SATURN_RSTK5,
	SATURN_ST,
	-1,

	SATURN_R2, 
	SATURN_RSTK6,
	SATURN_OUT,
	-1,

	SATURN_R3, 
	SATURN_RSTK7,
	SATURN_IN,	
	-1,

	SATURN_R4,
	SATURN_IRQ_STATE,
	0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 saturn_win_layout[] = {
	 0, 0,80, 9,	/* register window (top, right rows) */
	 0,10,35,12,	/* disassembler window (left colums) */
	36,10,44, 6,	/* memory #1 window (right, upper middle) */
	36,17,44, 5,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

/****************************************************************************
 * The 6502 registers.
 ****************************************************************************/
typedef struct
{
	SATURN_CONFIG *config;
	Saturn64 reg[9]; //r0,r1,r2,r3,r4,a,b,c,d;

	SaturnAdr d[2], pc, oldpc, rstk[8]; // 20 bit addresses

	int stackpointer; // this is only for debugger stepover support!

	SaturnNib p; // 4 bit pointer

	UINT16 in;
	int out; // 12
	bool carry, decimal;
	UINT16 st; // status 16 bit
#define XM 1
#define SR 2
#define SB 4
#define MP 8
	int hst; // hardware status 4 bit
		/*  XM external Modules missing
			SR Service Request
			SB Sticky bit
			MP Module Pulled */

	int irq_state;

	UINT8	pending_irq;	/* nonzero if an IRQ is pending */
	UINT8	after_cli;		/* pending IRQ and last insn cleared I */
	UINT8	nmi_state;
	int 	(*irq_callback)(int irqline);	/* IRQ callback */
}	Saturn_Regs;

int saturn_icount = 0;

static Saturn_Regs saturn;

/***************************************************************
 * include the opcode macros, functions and tables
 ***************************************************************/
#include "satops.c"
#include "sattable.c"

/*****************************************************************************
 *
 *		6502 CPU interface functions
 *
 *****************************************************************************/

void saturn_reset(void *param)
{
	if (param) {
		saturn.config=(SATURN_CONFIG *)param;
	}
	saturn.stackpointer=0;
	saturn.pc=0;
	change_pc20(saturn.pc);
}

void saturn_exit(void)
{
	/* nothing to do yet */
}

unsigned saturn_get_context (void *dst)
{
	if( dst )
		*(Saturn_Regs*)dst = saturn;
	return sizeof(Saturn_Regs);
}

void saturn_set_context (void *src)
{
	if( src )
	{
		saturn = *(Saturn_Regs*)src;
		change_pc20(saturn.pc);
	}
}

unsigned saturn_get_pc (void)
{
	return saturn.pc;
}

void saturn_set_pc (unsigned val)
{
	saturn.pc = val;
	change_pc20(saturn.pc);
}

unsigned saturn_get_sp (void)
{
	return saturn.stackpointer;
}

void saturn_set_sp (unsigned val)
{
	saturn.stackpointer=val;
}

unsigned saturn_get_reg (int regnum)
{
	switch( regnum )
	{
	case SATURN_PC: return saturn.pc;
	case SATURN_D0: return saturn.d[0];
	case SATURN_D1: return saturn.d[1];
#if 0
	case SATURN_A: return saturn.reg[A];
	case SATURN_B: return saturn.reg[B];
	case SATURN_C: return saturn.reg[C];
	case SATURN_D: return saturn.reg[D];
	case SATURN_R0: return saturn.reg[R0];
	case SATURN_R1: return saturn.reg[R1];
	case SATURN_R2: return saturn.reg[R2];
	case SATURN_R3: return saturn.reg[R3];
	case SATURN_R4: return saturn.reg[R4];
#endif
	case SATURN_P: return saturn.p;
	case SATURN_IN: return saturn.in;
	case SATURN_OUT: return saturn.out;
	case SATURN_CARRY: return saturn.carry;
	case SATURN_ST: return saturn.st;
	case SATURN_HST: return saturn.hst;
	case SATURN_RSTK0: return saturn.rstk[0];
	case SATURN_RSTK1: return saturn.rstk[1];
	case SATURN_RSTK2: return saturn.rstk[2];
	case SATURN_RSTK3: return saturn.rstk[3];
	case SATURN_RSTK4: return saturn.rstk[4];
	case SATURN_RSTK5: return saturn.rstk[5];
	case SATURN_RSTK6: return saturn.rstk[6];
	case SATURN_RSTK7: return saturn.rstk[7];
	case SATURN_NMI_STATE: return saturn.nmi_state;
	case SATURN_IRQ_STATE: return saturn.irq_state;
	case REG_PREVIOUSPC: return saturn.oldpc;
#if 0
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = S + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0x1ff )
					return RDMEM( offset ) | ( RDMEM( offset + 1 ) << 8 );
			}
#endif
	}
	return 0;
}

void saturn_set_reg (int regnum, unsigned val)
{
	switch( regnum )
	{
	case SATURN_PC: saturn.pc=val;break;
	case SATURN_D0: saturn.d[0]=val;break;
	case SATURN_D1: saturn.d[1]=val;break;
#if 0
	case SATURN_A: saturn.reg[A]=val;break;
	case SATURN_B: saturn.reg[B]=val;break;
	case SATURN_C: saturn.reg[C]=val;break;
	case SATURN_D: saturn.reg[D]=val;break;
	case SATURN_R0: saturn.reg[R0]=val;break;
	case SATURN_R1: saturn.reg[R1]=val;break;
	case SATURN_R2: saturn.reg[R2]=val;break;
	case SATURN_R3: saturn.reg[R3]=val;break;
	case SATURN_R4: saturn.reg[R4]=val;break;
#endif
	case SATURN_P: saturn.p=val;break;
	case SATURN_IN: saturn.in=val;break;
	case SATURN_OUT: saturn.out=val;break;
	case SATURN_CARRY: saturn.carry=val;break;
	case SATURN_ST: saturn.st=val;break;
	case SATURN_HST: saturn.hst=val;break;
	case SATURN_RSTK0: saturn.rstk[0]=val;break;
	case SATURN_RSTK1: saturn.rstk[1]=val;break;
	case SATURN_RSTK2: saturn.rstk[2]=val;break;
	case SATURN_RSTK3: saturn.rstk[3]=val;break;
	case SATURN_RSTK4: saturn.rstk[4]=val;break;
	case SATURN_RSTK5: saturn.rstk[5]=val;break;
	case SATURN_RSTK6: saturn.rstk[6]=val;break;
	case SATURN_RSTK7: saturn.rstk[7]=val;break;
	case SATURN_NMI_STATE: saturn.nmi_state=val;break;
	case SATURN_IRQ_STATE: saturn.irq_state=val;break;
#if 0
	default:
		if( regnum <= REG_SP_CONTENTS )
		{
			unsigned offset = S + 2 * (REG_SP_CONTENTS - regnum);
			if( offset < 0x1ff )
			{
				WRMEM( offset, val & 0xfff );
				WRMEM( offset + 1, (val >> 8) & 0xff );
			}
		}
#endif
	}
}

INLINE void saturn_take_irq(void)
{
	{
		saturn_icount -= 7;
		
		saturn_push(saturn.pc);
		saturn.pc=IRQ_ADDRESS;

		LOG(("Saturn#%d takes IRQ ($%04x)\n", cpu_getactivecpu(), saturn.pc));
		/* call back the cpuintrf to let it clear the line */
		if (saturn.irq_callback) (*saturn.irq_callback)(0);
		change_pc20(saturn.pc);
	}
	saturn.pending_irq = 0;
}

int saturn_execute(int cycles)
{
	saturn_icount = cycles;

	change_pc20(saturn.pc);

	do
	{
		saturn.oldpc = saturn.pc;

		CALL_MAME_DEBUG;

		/* if an irq is pending, take it now */
		if( saturn.pending_irq )
			saturn_take_irq();

		saturn_instruction();

		/* check if the I flag was just reset (interrupts enabled) */
		if( saturn.after_cli )
		{
			LOG(("M6502#%d after_cli was >0", cpu_getactivecpu()));
			saturn.after_cli = 0;
			if (saturn.irq_state != CLEAR_LINE)
			{
				LOG((": irq line is asserted: set pending IRQ\n"));
				saturn.pending_irq = 1;
			}
			else
			{
				LOG((": irq line is clear\n"));
			}
		}
		else
		if( saturn.pending_irq )
			saturn_take_irq();

	} while (saturn_icount > 0);

	return cycles - saturn_icount;
}

void saturn_set_nmi_line(int state)
{
	if (saturn.nmi_state == state) return;
	saturn.nmi_state = state;
	if( state != CLEAR_LINE )
	{
		LOG(( "M6502#%d set_nmi_line(ASSERT)\n", cpu_getactivecpu()));
		saturn_icount -= 7;
		saturn_push(saturn.pc);
		saturn.pc=IRQ_ADDRESS;

		LOG(("M6502#%d takes NMI ($%04x)\n", cpu_getactivecpu(), PC));
		change_pc20(saturn.pc);
	}
}

void saturn_set_irq_line(int irqline, int state)
{
	saturn.irq_state = state;
	if( state != CLEAR_LINE )
	{
		LOG(( "M6502#%d set_irq_line(ASSERT)\n", cpu_getactivecpu()));
		saturn.pending_irq = 1;
	}
}

void saturn_set_irq_callback(int (*callback)(int))
{
	saturn.irq_callback = callback;
}

void saturn_state_save(void *file)
{
#if 0
	int cpu = cpu_getactivecpu();
	state_save_UINT16(file,"m6502",cpu,"PC",&m6502.pc.w.l,2);
	state_save_UINT16(file,"m6502",cpu,"SP",&m6502.sp.w.l,2);
	state_save_UINT8(file,"m6502",cpu,"P",&m6502.p,1);
	state_save_UINT8(file,"m6502",cpu,"A",&m6502.a,1);
	state_save_UINT8(file,"m6502",cpu,"X",&m6502.x,1);
	state_save_UINT8(file,"m6502",cpu,"Y",&m6502.y,1);
	state_save_UINT8(file,"m6502",cpu,"PENDING",&m6502.pending_irq,1);
	state_save_UINT8(file,"m6502",cpu,"AFTER_CLI",&m6502.after_cli,1);
	state_save_UINT8(file,"m6502",cpu,"NMI_STATE",&m6502.nmi_state,1);
	state_save_UINT8(file,"m6502",cpu,"IRQ_STATE",&m6502.irq_state,1);
	state_save_UINT8(file,"m6502",cpu,"SO_STATE",&m6502.so_state,1);
#endif
}

void saturn_state_load(void *file)
{
#if 0
	int cpu = cpu_getactivecpu();
	state_load_UINT16(file,"m6502",cpu,"PC",&m6502.pc.w.l,2);
	state_load_UINT16(file,"m6502",cpu,"SP",&m6502.sp.w.l,2);
	state_load_UINT8(file,"m6502",cpu,"P",&m6502.p,1);
	state_load_UINT8(file,"m6502",cpu,"A",&m6502.a,1);
	state_load_UINT8(file,"m6502",cpu,"X",&m6502.x,1);
	state_load_UINT8(file,"m6502",cpu,"Y",&m6502.y,1);
	state_load_UINT8(file,"m6502",cpu,"PENDING",&m6502.pending_irq,1);
	state_load_UINT8(file,"m6502",cpu,"AFTER_CLI",&m6502.after_cli,1);
	state_load_UINT8(file,"m6502",cpu,"NMI_STATE",&m6502.nmi_state,1);
	state_load_UINT8(file,"m6502",cpu,"IRQ_STATE",&m6502.irq_state,1);
	state_load_UINT8(file,"m6502",cpu,"SO_STATE",&m6502.so_state,1);
#endif
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *saturn_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	Saturn_Regs *r = context;

	which = ++which % 16;
	buffer[which][0] = '\0';
	if( !context )
		r = &saturn;

	switch( regnum )
	{
	case CPU_INFO_REG+SATURN_PC: sprintf(buffer[which],"PC:   %.5x",r->pc);break;
	case CPU_INFO_REG+SATURN_D0: sprintf(buffer[which],"D0:   %.5x",r->d[0]);break;
	case CPU_INFO_REG+SATURN_D1: sprintf(buffer[which],"D1:   %.5x",r->d[1]);break;
	case CPU_INFO_REG+SATURN_A: 
		sprintf(buffer[which],"A: %.8x %.8x",r->reg[A].d[1],r->reg[A].d[0]);break;
	case CPU_INFO_REG+SATURN_B: 
		sprintf(buffer[which],"B: %.8x %.8x",r->reg[B].d[1],r->reg[B].d[0]);break;
	case CPU_INFO_REG+SATURN_C: 
		sprintf(buffer[which],"C: %.8x %.8x",r->reg[C].d[1],r->reg[C].d[0]);break;
	case CPU_INFO_REG+SATURN_D: 
		sprintf(buffer[which],"D: %.8x %.8x",r->reg[D].d[1],r->reg[D].d[0]);break;
	case CPU_INFO_REG+SATURN_R0: 
		sprintf(buffer[which],"R0:%.8x %.8x",r->reg[R0].d[1],r->reg[R0].d[0]);break;
	case CPU_INFO_REG+SATURN_R1: 
		sprintf(buffer[which],"R1:%.8x %.8x",r->reg[R1].d[1],r->reg[R1].d[0]);break;
	case CPU_INFO_REG+SATURN_R2: 
		sprintf(buffer[which],"R2:%.8x %.8x",r->reg[R2].d[1],r->reg[R2].d[0]);break;
	case CPU_INFO_REG+SATURN_R3: 
		sprintf(buffer[which],"R3:%.8x %.8x",r->reg[R3].d[1],r->reg[R3].d[0]);break;
	case CPU_INFO_REG+SATURN_R4: 
		sprintf(buffer[which],"R4:%.8x %.8x",r->reg[R4].d[1],r->reg[R4].d[0]);break;
	case CPU_INFO_REG+SATURN_P: sprintf(buffer[which],"P:%x",r->p);break;
	case CPU_INFO_REG+SATURN_IN: sprintf(buffer[which],"IN:%.4x",r->in);break;
	case CPU_INFO_REG+SATURN_OUT: sprintf(buffer[which],"OUT:%.3x",r->out);break;
	case CPU_INFO_REG+SATURN_CARRY: sprintf(buffer[which],"Carry: %d",r->carry);break;
	case CPU_INFO_REG+SATURN_ST: sprintf(buffer[which],"ST:%.4x",r->st);break;
	case CPU_INFO_REG+SATURN_HST: sprintf(buffer[which],"HST:%x",r->hst);break;
	case CPU_INFO_REG+SATURN_RSTK0: sprintf(buffer[which],"RSTK0:%.5x",r->rstk[0]);break;
	case CPU_INFO_REG+SATURN_RSTK1: sprintf(buffer[which],"RSTK1:%.5x",r->rstk[1]);break;
	case CPU_INFO_REG+SATURN_RSTK2: sprintf(buffer[which],"RSTK2:%.5x",r->rstk[2]);break;
	case CPU_INFO_REG+SATURN_RSTK3: sprintf(buffer[which],"RSTK3:%.5x",r->rstk[3]);break;
	case CPU_INFO_REG+SATURN_RSTK4: sprintf(buffer[which],"RSTK4:%.5x",r->rstk[4]);break;
	case CPU_INFO_REG+SATURN_RSTK5: sprintf(buffer[which],"RSTK5:%.5x",r->rstk[5]);break;
	case CPU_INFO_REG+SATURN_RSTK6: sprintf(buffer[which],"RSTK6:%.5x",r->rstk[6]);break;
	case CPU_INFO_REG+SATURN_RSTK7: sprintf(buffer[which],"RSTK7:%.5x",r->rstk[7]);break;
	case CPU_INFO_REG+SATURN_NMI_STATE: sprintf(buffer[which],"NMI:%d",r->nmi_state);break;
	case CPU_INFO_REG+SATURN_IRQ_STATE: sprintf(buffer[which],"IRQ:%d",r->irq_state);break;
	case CPU_INFO_FLAGS: sprintf(buffer[which], "%c%c", r->decimal?'D':'.', r->carry ? 'C':'.'); break;
	case CPU_INFO_NAME: return "Saturn";
	case CPU_INFO_FAMILY: return "Saturn";
	case CPU_INFO_VERSION: return "1.0alpha";
	case CPU_INFO_FILE: return __FILE__;
	case CPU_INFO_CREDITS: return "Copyright (c) 2000 Peter Trauner, all rights reserved.";
	case CPU_INFO_REG_LAYOUT: return (const char*)saturn_reg_layout;
	case CPU_INFO_WIN_LAYOUT: return (const char*)saturn_win_layout;
	}
	return buffer[which];
}

#ifndef MAME_DEBUG
unsigned saturn_dasm(char *buffer, unsigned pc)
{
	sprintf( buffer, "$%X", cpu_readop(pc) );
	return 1;
}
#endif
