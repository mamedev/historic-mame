/*========================================
	tms9900.c

	TMS9900 emulation

	Original emulator by Edward Swartz.
	Smoothed out by Raphael Nabet

	Converted for Mame by M.Coates
	Additional work (processor timing, interrupt support, bug fixes) by R Nabet
========================================*/

/* set this to 1 to use 16-bit handlers */
/* (requires some changes in cpuintrf.c and all drivers which use tms9900) */
#define USE_16BIT_MEMORY_HANDLER 1
/* Set this to 1 to support HOLD_LINE */
/* This is a weird HOLD_LINE, actually : we hold the interrupt line only until IAQ
  (instruction acquisition) is enabled.  Well, this scheme could possibly exist on
  a TMS9900-based system, unlike a real HOLD_LINE.  (OK, this is just a pretext, I was just too
  lazy to implement a true HOLD_LINE ;-) .) */
#define SILLY_INTERRUPT_HACK 0

#include "memory.h"
#include "mamedbg.h"
#include "tms9900.h"

INLINE void execute(UINT16 opcode);

static void extended_writeCRU(int CRUAddr);
static UINT16 fetch(void);
static UINT16 decipheraddr(UINT16 opcode);
static UINT16 decipheraddrbyte(UINT16 opcode);
static void contextswitch(UINT16 addr);

static void h0200(UINT16 opcode);
static void h0400(UINT16 opcode);
static void h0800(UINT16 opcode);
static void h1000(UINT16 opcode);
static void h2000(UINT16 opcode);
static void h4000(UINT16 opcode);

static void field_interrupt(void);

/***************************/
/* Mame Interface Routines */
/***************************/

extern FILE *errorlog;

static UINT8 tms9900_reg_layout[] = {
	TMS9900_PC, TMS9900_WP, TMS9900_STATUS, TMS9900_IR
#ifdef MAME_DEBUG
	, -1,
	TMS9900_R0, TMS9900_R1, TMS9900_R2, TMS9900_R3,
	TMS9900_R4, TMS9900_R5, TMS9900_R6, TMS9900_R7, -1,
	TMS9900_R8, TMS9900_R9, TMS9900_R10, TMS9900_R11,
	TMS9900_R12, TMS9900_R13, TMS9900_R14, TMS9900_R15, -1,
#endif
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 tms9900_win_layout[] = {
	 0, 0,80, 4,	/* register window (top rows) */
	 0, 5,31,17,	/* disassembler window (left colums) */
	32, 5,48, 8,	/* memory #1 window (right, upper middle) */
	32,14,48, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

int tms9900_ICount = 0;

/* TMS9900 ST register bits. */

/* These bits are set by every compare, move and arithmetic or logicaloperation : */
/* (Well, COC, CZC and TB only set the E bit, but these are kind ofexceptions.) */
#define ST_L 0x8000 /* logically greater (strictly) */
#define ST_A 0x4000 /* arithmetically greater (strictly) */
#define ST_E 0x2000 /* equal */

/* These bits are set by arithmetic operations, when it makes sense to update them. */
#define ST_C 0x1000 /* Carry */
#define ST_O 0x0800 /* Overflow (overflow with operations on signed integers, */
                    /* and when the result of a 32bits:16bits division cannot fit in a 16-bit word.) */

/* This bit is set by move and arithmetic operations WHEN THEY USE BYTEOPERANDS. */
#define ST_P 0x0400 /* Parity (set when odd parity) */

/* This bit is set by the XOP instruction. */
#define ST_X 0x0200 /* Xop */

/* Offsets for registers. */
#define R0   0
#define R1   2
#define R2   4
#define R3   6
#define R4   8
#define R5  10
#define R6  12
#define R7  14
#define R8  16
#define R9  18
#define R10 20
#define R11 22
#define R12 24
#define R13 26
#define R14 28
#define R15 30

typedef struct
{
/* "actual" TMS9900 registers : */
	UINT16 WP;  /* Workspace pointer */
	UINT16 PC;  /* Program counter */
	UINT16 STATUS;  /* STatus register */

/* Now, data used for emulation */
	UINT16 IR;  /* Instruction register, with the currently parsed opcode */

  int interrupt_pending;  /* true if an interrupt must be honored... */

	int irq_state;	/* nonzero if the INTREQ* line is active (low) */
	int irq_level;	/* when INTREQ* is active, interrupt level on IC0-IC3 ; else always 16 */
	int (*irq_callback)(int irq_line);  /* this callback is used by tms9900_set_irq_line() to
	                                       retreive the value on IC0-IC3 (not-so-standard behaviour) */
	int load_state; /* nonzero if the LOAD* line is active (low) */
	int IDLE;       /* nonzero if processor is IDLE - i.e waiting for interrupt while writing
	                    special data on CRU bus */

#if MAME_DEBUG
	UINT16 FR[16];  /* contains a copy of the workspace for the needs of the debugger */
#endif
}	tms9900_Regs;

static tms9900_Regs I =
{
	0,0,0,0,  /* don't care */
	0, 0, 16, /* INTREQ* inactive */
	NULL,     /* don't care */
	0         /* LOAD* inactive */
};
static UINT8 lastparity = 0;  /* rather than handling ST_P directly, we copy the last value that
                                  would set it here */

#define readword(addr)			cpu_readmem16bew_word(addr)
#define writeword(addr,data)	cpu_writemem16bew_word((addr), (data))

#define readbyte(addr)			cpu_readmem16bew(addr)
#define writebyte(addr,data)	cpu_writemem16bew((addr),(data))

#define READREG(reg)			readword(I.WP+reg)
#define WRITEREG(reg,data)		writeword(I.WP+reg,data)

#define IMASK					(I.STATUS & 0x0F)

/************************************************************************
 * Status register functions
 ************************************************************************/
#include "9900stat.h"

/**************************************************************************/

/*
	TMS9900 hard reset
*/
void tms9900_reset(void *param)
{
	I.STATUS = 0;
	setstat();
	I.WP = readword(0);
	I.PC = readword(2);
	I.IDLE = 0; /* clear IDLE condition */

  field_interrupt();  /* mmmh...  The ST register changed, didn't it ? */

	tms9900_ICount -= 26;
}

void tms9900_exit(void)
{
	/* nothing to do ? */
}

int tms9900_execute(int cycles)
{
	tms9900_ICount = cycles;

	do
	{

		#ifdef MAME_DEBUG
		{
			extern int mame_debug;

			if (mame_debug)
			{
				setstat();

				I.FR[ 0] = READREG(R0);
				I.FR[ 1] = READREG(R1);
				I.FR[ 2] = READREG(R2);
				I.FR[ 3] = READREG(R3);
				I.FR[ 4] = READREG(R4);
				I.FR[ 5] = READREG(R5);
				I.FR[ 6] = READREG(R6);
				I.FR[ 7] = READREG(R7);
				I.FR[ 8] = READREG(R8);
				I.FR[ 9] = READREG(R9);
				I.FR[10] = READREG(R10);
				I.FR[11] = READREG(R11);
				I.FR[12] = READREG(R12);
				I.FR[13] = READREG(R13);
				I.FR[14] = READREG(R14);
				I.FR[15] = READREG(R15);

				#if 0		/* Trace */
				if (errorlog)
				{
					fprintf(errorlog,"> PC %4.4x :%4.4x %4.4x : R=%4.4x %4.4x %4.4x %4.4x %4.4x %4.4x %4.4x %4.4x %4.4x %4.4x%4.4x %4.4x %4.4x %4.4x %4.4x %4.4x :T=%d\n",I.PC,I.STATUS,I.WP,I.FR0,I.FR1,I.FR2,I.FR3,I.FR4,I.FR5,I.FR6,I.FR7,I.FR8,I.FR9,I.FR10,I.FR11,I.FR12,I.FR13,I.FR14,I.FR15,tms9900_ICount);
				}
				#endif

				MAME_Debug();
			}
		}
		#endif

		if (I.IDLE)
		{	/* IDLE instruction has halted execution */
			extended_writeCRU(0x2000);
			tms9900_ICount -= 2;  /* 2 cycles per CRU write */
		}
		else
		{	/* we execute an instruction */
			I.IR = fetch();
			execute(I.IR);
		}

		/*
		  We handle interrupts here because :
		  a) LOAD and level-0 (reset) interrupts are non-maskable, so, AFAIK, if the LOAD* line or
		     INTREQ* line (with IC0-3 == 0) remain active, we will execute repeatedly the first
		     instruction of the interrupt routine.
		  b) if we did otherwise, we could have weird, buggy behavior if IC0-IC3 changed more than
		     once in too short a while (i.e. before tms9900 executes another instruction).  Yes, this
		     is rather pedantic, the probability is really small.
		*/
		if (I.interrupt_pending)
		{
			int level;

#if SILLY_INTERRUPT_HACK
			if (I.irq_level == -1)
			{
				level = (* I.irq_callback)(0);
				if (I.irq_state)
				{ /* if callback didn't clear the line */
					I.irq_level = level;
					if (level > IMASK)
						I.interrupt_pending = 0;
				}
			}
			else
#endif
			level = I.irq_level;

			/* TMS9900 does not honor interrupt after XOP, BLWP, and possibly RTWP (after any
			   interrupt-like instruction, actually) */
			if (((I.IR & 0xFC00) != 0x2C00) && ((I.IR & 0xFFC0) != 0x0400) /*&& ((I.IR & 0xFFE0) != 0x0380)*/)
			{
				if (I.load_state)
				{	/* LOAD has the highest priority */

					contextswitch(0xFFFC);  /* load vector, save PC, WP and ST */

					/* clear IDLE status if necessary */
					I.IDLE = 0;

					tms9900_ICount -= 22;
				}
				else if (level <= IMASK)
				{	/* a maskable interrupt is honored only if its level isn't greater than IMASK */

					contextswitch(level*4); /* load vector, save PC, WP and ST */

					/* change interrupt mask */
					if (level)
					{
						I.STATUS = (I.STATUS & 0xFFF0) | (level -1);  /* decrement mask */
						I.interrupt_pending = 0;  /* as a consequence, the interrupt request will be subsequently ignored */
					}
					else
						I.STATUS &= 0xFFF0; /* clear mask (is this correct ???) */

					/* clear IDLE status if necessary */
					I.IDLE = 0;

					tms9900_ICount -= 22;
				}
				else
#if SILLY_INTERRUPT_HACK
				if (I.interrupt_pending)  /* we may have just cleared this */
#endif
				{
					if (errorlog)
						fprintf(errorlog,"tms9900.c : the interrupt_pending flag was set incorrectly\n");
					I.interrupt_pending = 0;
				}
			}
		}

	} while (tms9900_ICount > 0);

	return cycles - tms9900_ICount;
}

unsigned tms9900_get_context(void *dst)
{
	setstat();

	if( dst )
		*(tms9900_Regs*)dst = I;

	return sizeof(tms9900_Regs);
}

void tms9900_set_context(void *src)
{
	if( src )
	{
		I = *(tms9900_Regs*)src;

    if (! I.irq_state)    /* We have to check this, because Mame debugger can foolishly initialize */
			I.irq_level = 16;   /* the context to all 0s */

		getstat();  /* set last_parity */
	}
}

unsigned tms9900_get_pc(void)
{
	return I.PC;
}

void tms9900_set_pc(unsigned val)
{
	I.PC = val;
}

/*
	Note : the WP register actually has nothing to do with a stack.

	To make this even weirder, some later versions of the TMS9900 do have a processor stack.
*/
unsigned tms9900_get_sp(void)
{
	return I.WP;
}

void tms9900_set_sp(unsigned val)
{
	I.WP = val;
}

unsigned tms9900_get_reg(int regnum)
{
	switch( regnum )
	{
		case TMS9900_PC: return I.PC;
		case TMS9900_IR: return I.IR;
		case TMS9900_WP: return I.WP;
		case TMS9900_STATUS: return I.STATUS;
#ifdef MAME_DEBUG
		case TMS9900_R0: return I.FR[0];
		case TMS9900_R1: return I.FR[1];
		case TMS9900_R2: return I.FR[2];
		case TMS9900_R3: return I.FR[3];
		case TMS9900_R4: return I.FR[4];
		case TMS9900_R5: return I.FR[5];
		case TMS9900_R6: return I.FR[6];
		case TMS9900_R7: return I.FR[7];
		case TMS9900_R8: return I.FR[8];
		case TMS9900_R9: return I.FR[9];
		case TMS9900_R10: return I.FR[10];
		case TMS9900_R11: return I.FR[11];
		case TMS9900_R12: return I.FR[12];
		case TMS9900_R13: return I.FR[13];
		case TMS9900_R14: return I.FR[14];
		case TMS9900_R15: return I.FR[15];
#endif
	}
	return 0;
}

void tms9900_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case TMS9900_PC: I.PC = val; break;
		case TMS9900_IR: I.IR = val; break;
		case TMS9900_WP: I.WP = val; break;
		case TMS9900_STATUS: I.STATUS = val; break;
#ifdef MAME_DEBUG
		case TMS9900_R0: I.FR[0]= val; break;
		case TMS9900_R1: I.FR[1]= val; break;
		case TMS9900_R2: I.FR[2]= val; break;
		case TMS9900_R3: I.FR[3]= val; break;
		case TMS9900_R4: I.FR[4]= val; break;
		case TMS9900_R5: I.FR[5]= val; break;
		case TMS9900_R6: I.FR[6]= val; break;
		case TMS9900_R7: I.FR[7]= val; break;
		case TMS9900_R8: I.FR[8]= val; break;
		case TMS9900_R9: I.FR[9]= val; break;
		case TMS9900_R10: I.FR[10]= val; break;
		case TMS9900_R11: I.FR[11]= val; break;
		case TMS9900_R12: I.FR[12]= val; break;
		case TMS9900_R13: I.FR[13]= val; break;
		case TMS9900_R14: I.FR[14]= val; break;
		case TMS9900_R15: I.FR[15]= val; break;
#endif
	}
}


/*
void tms9900_set_nmi_line(int state) : change the state of the LOAD* line

	state == 0 -> LOAD* goes high (inactive)
	state != 0 -> LOAD* goes low (active)

	While LOAD* is low, we keep triggering LOAD interrupts...

	A problem : some peripheral lower the LOAD* line for a fixed time interval (causing the 1st
	instruction of the LOAD interrupt routine to be repeated while the line is low), and will be
	perfectly happy with the current scheme, but others might be more clever and wait for the IAQ
	(Instruction acquisition) line to go high, and this needs a callback function to emulate
	(but does such a scheme really exist ? - I think it would fail if the current instruction is
	an XOP).
*/
void tms9900_set_nmi_line(int state)
{
	I.load_state = state;   /* save new state */

	field_interrupt();  /* interrupt status changed */
}

/*
void tms9900_set_irq_line(int irqline, int state) : sets the state of the interrupt line.

	irqline is ignored, and should always be 0.

	state == 0 -> INTREQ* goes high (inactive)
	state != 0 -> INTREQ* goes low (active)
*/
/*
  R Nabet 991020 :
  In short : interrupt code should call "cpu_set_irq_line(0, 0, ASSERT_LINE);" to set an
  interrupt request.  Also, there MUST be a call to "cpu_set_irq_line(0, 0, CLEAR_LINE);" in
  the machine code, when the interrupt line is released by the hardware (generally in response to
  an action performed by the interrupt routines).

  **Note** : HOLD_LINE *NEVER* makes sense on the TMS9900.  The reason is the TMS9900 does NOT
  tell the world it acknoledges an interrupt, so no matter how much hardware you use, you cannot
  know when the CPU takes the interrupt, hence you cannot release the line when the CPU takes
  the interrupt.  Generally, the interrupt condition is cleared by the interrupt routine
  (with some CRU or memory access).  Note that I still do something with HOLD_LINE : the line
  is held until next instruction.

  Note that cpu_generate_interrupt uses HOLD_LINE, so your driver interrupt code
  should always use the new style, i.e. return "ignore_interrupt()" and call
  "cpu_set_irq_line(0, 0, ASSERT_LINE);" explicitely.

  Last, many TMS9900-based hardware use a TMS9901 interrupt-handling chip.  If anybody wants
  to emulate some hardware which uses it, note that I am writing some emulation in the TI99 driver
  in MESS, so you should ask me.
*/
/*
 * HJB 990430: changed to use irq_callback() to retrieve the vector
 * instead of using 16 irqlines.
 *
 * R Nabet 990830 : My mistake, I rewrote all these once again ; I think it is now correct.
 * A driver using the TMS9900 should do :
 *		cpu_0_irq_line_vector_w(0, level);
 *		cpu_set_irq_line(0,0,ASSERT_LINE);
 *
 * R Nabet 991108 : revised once again, with advice from Juergen Buchmueller, after a discussion
 * with Nicola...
 * We use the callback to retreive the interrupt level as soon as INTREQ* is asserted.
 * As a consequence, I do not support HOLD_LINE normally...  However, we do not really have to
 * support HOLD_LINE, since no real world TMS9900-based system can support this.
 * FYI, there are two alternatives to retreiving the interrupt level with the callback :
 * a) using 16 pseudo-IRQ lines.  Mostly OK, though it would require a few core changes.
 *    However, this could cause some problems if someone tried to set two lines simulteanously...
 *    And TMS9900 did NOT have 16 lines ! This is why Juergen and I did not retain this solution.
 * b) modifying the interrupt system in order to provide an extra int to every xxx_set_irq_line
 *    function.  I think this solution would be fine, but it would require quite a number of
 *    changes in the MAME core.  (And I did not feel the courage to check out 4000 drivers and 25
 *    cpu cores ;-) .)
*/
void tms9900_set_irq_line(int irqline, int state)
{
  if (I.irq_state == state)
    return;

	I.irq_state = state;

	if (state == CLEAR_LINE)
		I.irq_level = 16;
		/* trick : 16 will always be bigger than the IM (0-15), so there will never be interrupts */
	else
	{
#if SILLY_INTERRUPT_HACK
		I.irq_level = -1;
#else
		I.irq_level = (* I.irq_callback)(0);
#endif
	}

	field_interrupt();  /* interrupt state is likely to have changed */
}

void tms9900_set_irq_callback(int (*callback)(int irqline))
{
	I.irq_callback = callback;
}


/*
 * field_interrupt
 *
 * Determines whether if an interrupt is pending, and sets the revelant flag.
 *
 * Called when an interrupt pin (LOAD*, INTREQ*, IC0-IC3) is changed, and when the interrupt mask
 * is modified.
 *
 * By using this flag, we save some compares in the execution loop.  Subtle, isn't it ;-) ?
 *
 * R Nabet.
 */
static void field_interrupt(void)
{
	I.interrupt_pending = ((I.irq_level <= IMASK) || (I.load_state));
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *tms9900_info(void *context, int regnum)
{
	static char buffer[32][47+1];
	static int which = 0;
	tms9900_Regs *r = context;

	which = ++which % 32;
	buffer[which][0] = '\0';

	if( !context )
		r = &I;

	switch( regnum )
	{
		case CPU_INFO_REG+TMS9900_PC: sprintf(buffer[which], "PC :%04X",  r->PC); break;
		case CPU_INFO_REG+TMS9900_IR: sprintf(buffer[which], "IR :%04X",  r->IR); break;
		case CPU_INFO_REG+TMS9900_WP: sprintf(buffer[which], "WP :%04X",  r->WP); break;
		case CPU_INFO_REG+TMS9900_STATUS: sprintf(buffer[which], "ST :%04X",  r->STATUS); break;
#ifdef MAME_DEBUG
		case CPU_INFO_REG+TMS9900_R0: sprintf(buffer[which], "R0 :%04X",  r->FR[0]); break;
		case CPU_INFO_REG+TMS9900_R1: sprintf(buffer[which], "R1 :%04X",  r->FR[1]); break;
		case CPU_INFO_REG+TMS9900_R2: sprintf(buffer[which], "R2 :%04X",  r->FR[2]); break;
		case CPU_INFO_REG+TMS9900_R3: sprintf(buffer[which], "R3 :%04X",  r->FR[3]); break;
		case CPU_INFO_REG+TMS9900_R4: sprintf(buffer[which], "R4 :%04X",  r->FR[4]); break;
		case CPU_INFO_REG+TMS9900_R5: sprintf(buffer[which], "R5 :%04X",  r->FR[5]); break;
		case CPU_INFO_REG+TMS9900_R6: sprintf(buffer[which], "R6 :%04X",  r->FR[6]); break;
		case CPU_INFO_REG+TMS9900_R7: sprintf(buffer[which], "R7 :%04X",  r->FR[7]); break;
		case CPU_INFO_REG+TMS9900_R8: sprintf(buffer[which], "R8 :%04X",  r->FR[8]); break;
		case CPU_INFO_REG+TMS9900_R9: sprintf(buffer[which], "R9 :%04X",  r->FR[9]); break;
		case CPU_INFO_REG+TMS9900_R10: sprintf(buffer[which], "R10:%04X",  r->FR[10]); break;
		case CPU_INFO_REG+TMS9900_R11: sprintf(buffer[which], "R11:%04X",  r->FR[11]); break;
		case CPU_INFO_REG+TMS9900_R12: sprintf(buffer[which], "R12:%04X",  r->FR[12]); break;
		case CPU_INFO_REG+TMS9900_R13: sprintf(buffer[which], "R13:%04X",  r->FR[13]); break;
		case CPU_INFO_REG+TMS9900_R14: sprintf(buffer[which], "R14:%04X",  r->FR[14]); break;
		case CPU_INFO_REG+TMS9900_R15: sprintf(buffer[which], "R15:%04X",  r->FR[15]); break;
#endif
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				r->WP & 0x8000 ? 'L':'.',
				r->WP & 0x4000 ? 'A':'.',
				r->WP & 0x2000 ? 'E':'.',
				r->WP & 0x1000 ? 'C':'.',
				r->WP & 0x0800 ? 'V':'.',
				r->WP & 0x0400 ? 'P':'.',
				r->WP & 0x0200 ? 'X':'.',
				r->WP & 0x0100 ? '?':'.',
				r->WP & 0x0080 ? '?':'.',
				r->WP & 0x0040 ? '?':'.',
				r->WP & 0x0020 ? '?':'.',
				r->WP & 0x0010 ? '?':'.',
				r->WP & 0x0008 ? 'I':'.',
				r->WP & 0x0004 ? 'I':'.',
				r->WP & 0x0002 ? 'I':'.',
				r->WP & 0x0001 ? 'I':'.');
			break;
		case CPU_INFO_NAME: return "TMS9900";
		case CPU_INFO_FAMILY: return "Texas Instruments 9900";
		case CPU_INFO_VERSION: return "2.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "C TMS9900 emulator by Edward Swartz, converted for Mame by M.Coates, updated by R. Nabet";
		case CPU_INFO_REG_LAYOUT: return (const char*)tms9900_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)tms9900_win_layout;
	}
	return buffer[which];
}

unsigned tms9900_dasm(char *buffer, unsigned pc)
{

#ifdef MAME_DEBUG
	return Dasm9900(buffer,pc);
#else
	sprintf( buffer, "$%04X", readword(pc) );
	return 2;
#endif
}

/**************************************************************************/

/*
	performs a normal write to CRU bus (used by SBZ, SBO, LDCR : address range 0 -> 0xFFF)
*/
static void writeCRU(int CRUAddr, int Number, UINT16 Value)
{
	int count;

	if (errorlog) fprintf(errorlog,"PC %4.4x Write CRU %x for %x =%x\n",I.PC,CRUAddr,Number,Value);

	/* Write Number bits from CRUAddr */

	for(count=0; count<Number; count++)
	{
		cpu_writeport(CRUAddr, (Value & 0x01));
		Value >>= 1;
		CRUAddr = (CRUAddr + 1) & 0xFFF;    /* 3 MSBs are always 0 */
	}
}

/*
	Some opcodes perform a dummy write to a special CRU address, so that an external function may be
	triggered.

	Only the first 3 MSBs of the address matter : other address bits and the written value itself
	are undefined.

	How should we support this ? With callback functions ? Actually, as long as we do not support
	hardware which makes use of this feature, it does not really matter :-) .
*/
static void extended_writeCRU(int CRUAddr)
{
#if 0
	switch (CRUAddr >> 12)
	{
		case 2: /* IDLE */

			break;
		case 3: /* RSET */

			break;
		case 5: /* CKON */

			break;
		case 6: /* CKOF */

			break;
		case 7: /* LREX */

			break;
		case 0:
			/* normal CRU write !!! */
			if (errorlog)
				fprintf(errorlog,"PC %4.4x : extended_writeCRU : wrong CRU address",I.PC);
			break;
		default:
			/* unknown address */
			if (errorlog)
				fprintf(errorlog,"PC %4.4x : extended_writeCRU : unknown CRU address",I.PC);
			break;
	}
#else
	/* I guess we can support this like normal CRU operations */
	cpu_writeport(CRUAddr, 0);  /* or is it 1 ??? */
#endif
}

/*
	performs a normal read to CRU bus (used by TB, STCR : address range 0->0xFFF)

	Note that on some (all ?) hardware, e.g. TI99, all normal memory operations cause unwanted CRU
	read at the same address.
	This seems to be impossible to emulate efficiently.
*/
static UINT16 readCRU(int CRUAddr, int Number)
{
	static int BitMask[] =
	{
		0, /* filler - saves a subtract to find mask */
		0x0100,0x0300,0x0700,0x0F00,0x1F00,0x3F00,0x7F00,0xFF00,
		0x01FF,0x03FF,0x07FF,0x0FFF,0x1FFF,0x3FFF,0x7FFF,0xFFFF
	};

	int Offset,Location,Value;

	if (errorlog) fprintf(errorlog,"Read CRU %x for %x\n",CRUAddr,Number);

	Location = CRUAddr >> 3;
	Offset   = CRUAddr & 07;

	if (Number <= 8)
	{
		/* Read 16 Bits */
		Value = (cpu_readport((Location + 1) & 0x1FF) << 8) | cpu_readport(Location & 0x1FF);

		/* Allow for Offset */
		Value >>= Offset;

		/* Mask out what we want */
		Value = (Value << 8) & BitMask[Number];

		/* And update */
		return (Value>>8);
	}
	else
	{
		/* Read 24 Bits */
		Value = (cpu_readport((Location + 2) & 0x1FF) << 16)
		                                | (cpu_readport((Location + 1) & 0x1FF) << 8)
		                                | cpu_readport(Location & 0x1FF);

		/* Allow for Offset */
		Value >>= Offset;

		/* Mask out what we want */
		Value &= BitMask[Number];

		/* and Update */
		return Value;
	}
}

/**************************************************************************/

/* fetch : read one word at * PC, and increment PC. */

static UINT16 fetch(void)
{
	register UINT16 value = readword(I.PC);
	I.PC += 2;
	return value;
}

/* contextswitch : performs a BLWP, ie change PC, WP, and save PC, WP and ST... */
static void contextswitch(UINT16 addr)
{
	UINT16 oldWP, oldpc;

	/* save old state */
	oldWP = I.WP;
	oldpc = I.PC;

	/* load vector */
	I.WP = readword(addr) & ~1;
	I.PC = readword(addr+2) & ~1;

	/* write old state to regs */
	WRITEREG(R13, oldWP);
	WRITEREG(R14, oldpc);
	setstat();
	WRITEREG(R15, I.STATUS);
}

/*
 * decipheraddr : compute and return the effective adress in word instructions.
 *
 * NOTA : the LSB is always ignored in word adresses,
 * but we do not set to 0 because of XOP...
 */
static UINT16 decipheraddr(UINT16 opcode)
{
	register UINT16 ts = opcode & 0x30;
	register UINT16 reg = opcode & 0xF;

	reg += reg;

	if (ts == 0)
		/* Rx */
		return(reg + I.WP);
	else if (ts == 0x10)
	{	/* *Rx */
		tms9900_ICount -= 4;
		return(readword(reg + I.WP));
	}
	else if (ts == 0x20)
	{
		register UINT16 imm;

		imm = fetch();

		tms9900_ICount -= 8;

		if (reg)
			/* @>xxxx(Rx) */
			return(readword(reg + I.WP) + imm);
		else
			/* @>xxxx */
			return(imm);
	}
	else /*if (ts == 0x30)*/
	{	/* *Rx+ */
		register UINT16 reponse;

		reg += I.WP;    /* reg now contains effective address */

		reponse = readword(reg);

		tms9900_ICount -= 8;

		writeword(reg, reponse+2); /* we increment register content */
		return(reponse);
	}
}

/* decipheraddrbyte : compute and return the effective adress in byte instructions. */
static UINT16 decipheraddrbyte(UINT16 opcode)
{
	register UINT16 ts = opcode & 0x30;
	register UINT16 reg = opcode & 0xF;

	reg += reg;

	if (ts == 0)
		/* Rx */
		return(reg + I.WP);
	else if (ts == 0x10)
	{	/* *Rx */
		tms9900_ICount -= 4;
		return(readword(reg + I.WP));
	}
	else if (ts == 0x20)
	{
		register UINT16 imm;

		imm = fetch();

		tms9900_ICount -= 8;

		if (reg)
			/* @>xxxx(Rx) */
			return(readword(reg + I.WP) + imm);
		else
			/* @>xxxx */
			return(imm);
	}
	else /*if (ts == 0x30)*/
	{	/* *Rx+ */
		register UINT16 reponse;

		reg += I.WP;    /* reg now contains effective address */
		tms9900_ICount -= 6;

		reponse = readword(reg);
		writeword(reg, reponse+1); /* we increment register content */
		return(reponse);
	}
}


/*************************************************************************/


/*==========================================================================
   Illegal instructions                                        >0000->01FF
                                                               >0C00->0FFF
============================================================================*/
static void illegal(UINT16 opcode)
{
	tms9900_ICount -= 6;
}


/*==========================================================================
   Immediate, Control instructions,                            >0200->03FF
 ---------------------------------------------------------------------------

     0 1 2 3-4 5 6 7+8 9 A B-C D E F
    ---------------------------------
    |     o p c o d e     |0| reg # |
    ---------------------------------

  LI, AI, ANDI, ORI, CI, STWP, STST, LIMI, LWPI, IDLE, RSET, RTWP, CKON,CKOF, LREX
============================================================================*/
static void h0200(UINT16 opcode)
{
	register UINT16 addr;
	register UINT16 value;	/* used for anything */

	addr = opcode & 0xF;
	addr = ((addr + addr) + I.WP) & ~1;

	switch ((opcode & 0x1e0) >> 5)
	{
		case 0:   /* LI */
			/* LI ---- Load Immediate */
			/* *Reg = *PC+ */
			value = fetch();
			writeword(addr, value);
			setst_lae(value);
			tms9900_ICount -= 12;
			break;
		case 1:   /* AI */
			/* AI ---- Add Immediate */
			/* *Reg += *PC+ */
			value = fetch();
			wadd(addr, value);
			tms9900_ICount -= 14;
			break;
		case 2:   /* ANDI */
			/* ANDI -- AND Immediate */
			/* *Reg &= *PC+ */
			value = fetch();
			value = readword(addr) & value;
			writeword(addr, value);
			setst_lae(value);
			tms9900_ICount -= 14;
			break;
		case 3:   /* ORI */
			/* ORI --- OR Immediate */
			/* *Reg |= *PC+ */
			value = fetch();
			value = readword(addr) | value;
			writeword(addr, value);
			setst_lae(value);
			tms9900_ICount -= 14;
			break;
		case 4:   /* CI */
			/* CI ---- Compare Immediate */
			/* status = (*Reg-*PC+) */
			value = fetch();
			setst_c_lae(value, readword(addr));
			tms9900_ICount -= 14;
			break;
		case 5:   /* STWP */
			/* STWP -- STore Workspace Pointer */
			/* *Reg = WP */
			writeword(addr, I.WP);
			tms9900_ICount -= 8;
			break;
		case 6:   /* STST */
			/* STST -- STore STatus register */
			/* *Reg = ST */
			setstat();
			writeword(addr, I.STATUS);
			tms9900_ICount -= 8;
			break;
		case 7:   /* LWPI */
			/* LWPI -- Load Workspace Pointer Immediate */
			/* WP = *PC+ */
			I.WP = fetch();
			tms9900_ICount -= 10;
			break;
		case 8:   /* LIMI */
			/* LIMI -- Load Interrupt Mask Immediate */
			/* ST&15 |= (*PC+)&15 */
			value = fetch();
			I.STATUS = (I.STATUS & ~ 0xF) | (value & 0xF);
			field_interrupt();  /*IM has been modified.*/
			tms9900_ICount -= 16;
			break;
		case 10:  /* IDLE */
			/* IDLE -- IDLE until a reset, interrupt, load */
			/* The TMS99000 locks until an interrupt happen (like with 68k STOP instruction),
			   and continuously performs a dummy write at CRU address $2xxx. */
			I.IDLE = 1;
			tms9900_ICount -= 12;
			/* we take care of extended_writeCRU(0x2000); in execute() */
			break;
		case 11:  /* RSET */
			/* RSET -- ReSET */
			/* Reset the Interrupt Mask, and perform a dummy write at CRU address $3xxx. */
			/* Does not actually cause a reset, but an external circuitery could trigger one. */
			I.STATUS &= 0xFFF0; /*clear IM.*/
			field_interrupt();  /*IM has been modified.*/
			extended_writeCRU(0x3000);
			tms9900_ICount -= 12;
			break;
		case 12:  /* RTWP */
			/* RTWP -- Return with Workspace Pointer */
			/* WP = R13, PC = R14, ST = R15 */
			I.STATUS = READREG(R15);
			getstat();  /* set last_parity */
			I.PC = READREG(R14);
			I.WP = READREG(R13);
			field_interrupt();  /*IM has been modified.*/
			tms9900_ICount -= 14;
			break;
		case 13:  /* CKON */
			/* CKON -- ClocK ON */
			/* Perform a dummy write at CRU address $5xxx. */
			/* An external circuitery could, for instance, enable a "decrement-and-interrupt" timer. */
			extended_writeCRU(0x5000);
			tms9900_ICount -= 12;
			break;
		case 14:  /* CKOF */
			/* CKOF -- ClocK OFf */
			/* Perform a dummy write at CRU address 0x6nnn. */
			/* An external circuitery could, for instance, disable a "decrement-and-interrupt" timer. */
			extended_writeCRU(0x6000);
			tms9900_ICount -= 12;
			break;
		case 15:  /* LREX */
			/* LREX -- Load or REstart eXecution */
			/* Perform a dummy write at CRU address 0x7nnn. */
			/* An external circuitery could, for instance, activate the LOAD* line,
			   causing a non-maskable LOAD interrupt (vector -1). */
			extended_writeCRU(0x7000);
			tms9900_ICount -= 12;
			break;
	}
}

/*==========================================================================
   Single-operand instructions,		                   >0400->07FF
 ---------------------------------------------------------------------------

     0 1 2 3-4 5 6 7+8 9 A B-C D E F
    ---------------------------------
    |    o p c o d e    |TS |   S   |
    ---------------------------------

  BLWP, B, X, CLR, NEG, INV, INC, INCT, DEC, DECT, BL, SWPB, SETO, ABS
============================================================================*/
static void h0400(UINT16 opcode)
{
	register UINT16 addr = decipheraddr(opcode) & ~1;
	register UINT16 value;  /* used for anything */

	switch ((opcode & 0x3C0) >> 6)
	{
		case 0:   /* BLWP */
			/* BLWP -- Branch with Workspace Pointer */
			/* Result: WP = *S+, PC = *S */
			/*         New R13=old WP, New R14=Old PC, New R15=Old ST */
			contextswitch(addr);
			tms9900_ICount -= 26;
			break;
		case 1:   /* B */
			/* B ----- Branch */
			/* PC = S */
			I.PC = addr;
			tms9900_ICount -= 8;
			break;
		case 2:   /* X */
			/* X ----- eXecute */
			/* Executes instruction *S */
			execute(readword(addr));
			tms9900_ICount -= 4;  /* The X instruction actually takes 8 cycles, but we gain 4
			                         cycles on the next instruction, as we don't need to fetch it. */
			break;
		case 3:   /* CLR */
			/* CLR --- CLeaR */
			/* *S = 0 */
			writeword(addr, 0);
			tms9900_ICount -= 10;
			break;
		case 4:   /* NEG */
			/* NEG --- NEGate */
			/* *S = -*S */
			value = - (INT16) readword(addr);
			if (value)
				I.STATUS &= ~ ST_C;
			else
				I.STATUS |= ST_C;
			setst_laeo(value);
			writeword(addr, value);
			tms9900_ICount -= 12;
			break;
		case 5:   /* INV */
			/* INV --- INVert */
			/* *S = ~*S */
			value = ~ readword(addr);
			writeword(addr, value);
			setst_lae(value);
			tms9900_ICount -= 10;
			break;
		case 6:   /* INC */
			/* INC --- INCrement */
			/* (*S)++ */
			wadd(addr, 1);
			tms9900_ICount -= 10;
			break;
		case 7:   /* INCT */
			/* INCT -- INCrement by Two */
			/* (*S) +=2 */
			wadd(addr, 2);
			tms9900_ICount -= 10;
			break;
		case 8:   /* DEC */
			/* DEC --- DECrement */
			/* (*S)-- */
			wsub(addr, 1);
			tms9900_ICount -= 10;
			break;
		case 9:   /* DECT */
			/* DECT -- DECrement by Two */
			/* (*S) -= 2 */
			wsub(addr, 2);
			tms9900_ICount -= 10;
			break;
		case 10:  /* BL */
			/* BL ---- Branch and Link */
			/* IP=S, R11=old IP */
			WRITEREG(R11, I.PC);
			I.PC = addr;
			tms9900_ICount -= 12;
			break;
		case 11:  /* SWPB */
			/* SWPB -- SWaP Bytes */
			/* *S = swab(*S) */
			value = readword(addr);
			value = logical_right_shift(value, 8) | (value << 8);
			writeword(addr, value);
			tms9900_ICount -= 10;
			break;
		case 12:  /* SETO */
			/* SETO -- SET Ones */
			/* *S = #$FFFF */
			writeword(addr, 0xFFFF);
			tms9900_ICount -= 10;
			break;
		case 13:  /* ABS */
			/* ABS --- ABSolute value */
			/* *S = |*S| */
			/* clearing ST_C seems to be necessary, although ABS will never set it. */
			I.STATUS &= ~ (ST_L | ST_A | ST_E | ST_C | ST_O);
			value = readword(addr);

			tms9900_ICount -= 12;

			if (((INT16) value) > 0)
				I.STATUS |= ST_L | ST_A;
			else if (((INT16) value) < 0)
			{
				I.STATUS |= ST_L;
				if (value == 0x8000)
					I.STATUS |= ST_O;
				writeword(addr, - ((INT16) value));
				tms9900_ICount -= 2;
			}
			else
				I.STATUS |= ST_E;

			break;
		default:
			/* illegal instructions */
			tms9900_ICount -= 6;
			break;
	}
}


/*==========================================================================
   Shift instructions,                                         >0800->0BFF
  --------------------------------------------------------------------------

     0 1 2 3-4 5 6 7+8 9 A B-C D E F
    ---------------------------------
    | o p c o d e   |   C   |   W   |
    ---------------------------------

  SRA, SRL, SLA, SRC
============================================================================*/
static void h0800(UINT16 opcode)
{
	register UINT16 addr;
	register UINT16 cnt = (opcode & 0xF0) >> 4;
	register UINT16 value;

	addr = (opcode & 0xF);
	addr = ((addr+addr) + I.WP) & ~1;

	tms9900_ICount -= 12;

	if (cnt == 0)
	{
		tms9900_ICount -= 8;

		cnt = READREG(0) & 0xF;

		if (cnt == 0)
			cnt = 16;
	}

	tms9900_ICount -= cnt+cnt;

	switch ((opcode & 0x300) >> 8)
	{
		case 0:   /* SRA */
			/* SRA --- Shift Right Arithmetic */
			/* *W >>= C   (*W is filled on the left with a copy of the sign bit) */
			value = setst_sra_laec(readword(addr), cnt);
			writeword(addr, value);
			break;
		case 1:   /* SRL */
			/* SRL --- Shift Right Logical */
			/* *W >>= C   (*W is filled on the left with 0) */
			value = setst_srl_laec(readword(addr), cnt);
			writeword(addr, value);
			break;
		case 2:   /* SLA */
			/* SLA --- Shift Left Arithmetic */
			/* *W <<= C */
			value = setst_sla_laeco(readword(addr), cnt);
			writeword(addr, value);
			break;
		case 3:   /* SRC */
			/* SRC --- Shift Right Circular */
			/* *W = rightcircularshift(*W, C) */
			value = setst_src_laec(readword(addr), cnt);
			writeword(addr, value);
			break;
	}
}


/*==========================================================================
   Jump, CRU bit instructions,                                 >1000->1FFF
 ---------------------------------------------------------------------------

     0 1 2 3-4 5 6 7+8 9 A B-C D E F
    ---------------------------------
    |  o p c o d e  | signed offset |
    ---------------------------------

  JMP, JLT, JLE, JEQ, JHE, JGT, JNE, JNC, JOC, JNO, JL, JH, JOP
  SBO, SBZ, TB
============================================================================*/
static void h1000(UINT16 opcode)
{
	/* we convert 8 bit signed word offset to a 16 bit effective word offset. */
	register INT16 offset = ((INT8) opcode);


	switch ((opcode & 0xF00) >> 8)
	{
		case 0:   /* JMP */
			/* JMP --- Unconditional jump */
			/* PC += offset */
			I.PC += (offset + offset);
			tms9900_ICount -= 10;
			break;
		case 1:   /* JLT */
			/* JLT --- Jump if Less Than (arithmetic) */
			/* if (A==0 && EQ==0), PC += offset */
			if (! (I.STATUS & (ST_A | ST_E)))
			{
				I.PC += (offset + offset);
				tms9900_ICount -= 10;
			}
			else
				tms9900_ICount -= 8;
			break;
		case 2:   /* JLE */
			/* JLE --- Jump if Lower or Equal (logical) */
			/* if (L==0 || EQ==1), PC += offset */
			if ((! (I.STATUS & ST_L)) || (I.STATUS & ST_E))
			{
				I.PC += (offset + offset);
				tms9900_ICount -= 10;
			}
			else
				tms9900_ICount -= 8;
			break;
		case 3:   /* JEQ */
			/* JEQ --- Jump if Equal */
			/* if (EQ==1), PC += offset */
			if (I.STATUS & ST_E)
			{
				I.PC += (offset + offset);
				tms9900_ICount -= 10;
			}
			else
				tms9900_ICount -= 8;
			break;
		case 4:   /* JHE */
			/* JHE --- Jump if Higher or Equal (logical) */
			/* if (L==1 || EQ==1), PC += offset */
			if (I.STATUS & (ST_L | ST_E))
			{
				I.PC += (offset + offset);
				tms9900_ICount -= 10;
			}
			else
				tms9900_ICount -= 8;
			break;
		case 5:   /* JGT */
			/* JGT --- Jump if Greater Than (arithmetic) */
			/* if (A==1), PC += offset */
			if (I.STATUS & ST_A)
			{
				I.PC += (offset + offset);
				tms9900_ICount -= 10;
			}
			else
				tms9900_ICount -= 8;
			break;
		case 6:   /* JNE */
			/* JNE --- Jump if Not Equal */
			/* if (EQ==0), PC += offset */
			if (! (I.STATUS & ST_E))
			{
				I.PC += (offset + offset);
				tms9900_ICount -= 10;
			}
			else
				tms9900_ICount -= 8;
			break;
		case 7:   /* JNC */
			/* JNC --- Jump if No Carry */
			/* if (C==0), PC += offset */
			if (! (I.STATUS & ST_C))
			{
				I.PC += (offset + offset);
				tms9900_ICount -= 10;
			}
			else
				tms9900_ICount -= 8;
			break;
		case 8:   /* JOC */
			/* JOC --- Jump On Carry */
			/* if (C==1), PC += offset */
			if (I.STATUS & ST_C)
			{
				I.PC += (offset + offset);
				tms9900_ICount -= 10;
			}
			else
				tms9900_ICount -= 8;
			break;
		case 9:   /* JNO */
			/* JNO --- Jump if No Overflow */
			/* if (OV==0), PC += offset */
			if (! (I.STATUS & ST_O))
			{
				I.PC += (offset + offset);
				tms9900_ICount -= 10;
			}
			else
				tms9900_ICount -= 8;
			break;
		case 10:  /* JL */
			/* JL ---- Jump if Lower (logical) */
			/* if (L==0 && EQ==0), PC += offset */
			if (! (I.STATUS & (ST_L | ST_E)))
			{
				I.PC += (offset + offset);
				tms9900_ICount -= 10;
			}
			else
				tms9900_ICount -= 8;
			break;
		case 11:  /* JH */
			/* JH ---- Jump if Higher (logical) */
			/* if (L==1 && EQ==0), PC += offset */
			if ((I.STATUS & ST_L) && ! (I.STATUS & ST_E))
			{
				I.PC += (offset + offset);
				tms9900_ICount -= 10;
			}
			else
				tms9900_ICount -= 8;
			break;
		case 12:  /* JOP */
			/* JOP --- Jump On (odd) Parity */
			/* if (P==1), PC += offset */
			{
				/* Let's set ST_P. */
				int i;
				UINT8 a;

				a = lastparity;
				i = 0;

				while (a != 0)
				{
					if (a & 1)  /* If current bit is set, */
						i++;      /* increment bit count. */
					a >>= 1U;   /* Next bit. */
				}

				/* Set ST_P bit. */
				/*if (i & 1)
					I.STATUS |= ST_P;
				else
					I.STATUS &= ~ ST_P;*/

				/* Jump accordingly. */
				if (i & 1)  /*(I.STATUS & ST_P)*/
				{
					I.PC += (offset + offset);
					tms9900_ICount -= 10;
				}
				else
					tms9900_ICount -= 8;
			}

			break;
		case 13:  /* SBO */
			/* SBO --- Set Bit to One */
			/* CRU Bit = 1 */
			writeCRU(((READREG(R12) & 0x1FFE) >> 1) + offset, 1, 1);
			tms9900_ICount -= 12;
			break;
		case 14:  /* SBZ */
			/* SBZ --- Set Bit to Zero */
			/* CRU Bit = 0 */
			writeCRU(((READREG(R12) & 0x1FFE) >> 1) + offset, 1, 0);
			tms9900_ICount -= 12;
			break;
		case 15:  /* TB */
			/* TB ---- Test Bit */
			/* EQ = (CRU Bit == 1) */
			setst_e(readCRU(((READREG(R12) & 0x1FFE) >> 1) + offset, 1) & 1, 1);
			tms9900_ICount -= 12;
			break;
	}
}


/*==========================================================================
   General and One-Register instructions                       >2000->3FFF
 ---------------------------------------------------------------------------

     0 1 2 3-4 5 6 7+8 9 A B-C D E F
    ---------------------------------
    |  opcode   |   D   |TS |   S   |
    ---------------------------------

  COC, CZC, XOR, LDCR, STCR, XOP, MPY, DIV
==========================================================================*/
static void h2000(UINT16 opcode)
{
	register UINT16 dest = (opcode & 0x3C0) >> 6;
	register UINT16 src;
	register UINT16 value;

	if (opcode < 0x3000 || opcode >= 0x3800)    /* not LDCR and STCR */
	{
		src = decipheraddr(opcode);

		switch ((opcode & 0x1C00) >> 10)
		{
			case 0:   /* COC */
				/* COC --- Compare Ones Corresponding */
				/* status E bit = (S&D == S) */
				dest = ((dest+dest) + I.WP) & ~1;
				src &= ~1;
				value = readword(src);
				setst_e(value & readword(dest), value);
				tms9900_ICount -= 14;
				break;
			case 1:   /* CZC */
				/* CZC --- Compare Zeroes Corresponding */
				/* status E bit = (S&~D == S) */
				dest = ((dest+dest) + I.WP) & ~1;
				src &= ~1;
				value = readword(src);
				setst_e(value & (~ readword(dest)), value);
				tms9900_ICount -= 14;
				break;
			case 2:   /* XOR */
				/* XOR --- eXclusive OR */
				/* D ^= S */
				dest = ((dest+dest) + I.WP) & ~1;
				src &= ~1;
				value = readword(dest) ^ readword(src);
				setst_lae(value);
				writeword(dest,value);
				tms9900_ICount -= 14;
				break;
			case 3:   /* XOP */
				/* XOP --- eXtended OPeration */
				/* WP = *(40h+D), PC = *(42h+D) */
				/* New R13=old WP, New R14=Old IP, New R15=Old ST */
				/* New R11=S */
				/* Xop bit set */
				contextswitch(0x40 + (dest << 2));
				I.STATUS |= ST_X;
				WRITEREG(R11, src);
				tms9900_ICount -= 36;
				break;
			/*case 4:*/   /* LDCR is implemented elsewhere */
			/*case 5:*/   /* STCR is implemented elsewhere */
			case 6:   /* MPY */
				/* MPY --- MultiPlY  (unsigned) */
				/* Results:  D:D+1 = D*S */
				dest = ((dest+dest) + I.WP) & ~1;
				src &= ~1;
				{
					UINT32 prod = ((UINT32) readword(dest)) * ((UINT32)readword(src)); /* fixed by R Nabet */
					writeword(dest, prod >> 16);
					writeword(dest+2, prod);
				}
				tms9900_ICount -= 52;
				break;
			case 7:   /* DIV */
				/* DIV --- DIVide    (unsigned) */
				/* D = D/S    D+1 = D%S */
				dest = ((dest+dest) + I.WP) & ~1;
				src &= ~1;
				{
					UINT16 d = readword(src);
					UINT16 hi = readword(dest);
					UINT32 divq = (((UINT32) hi) << 16) | readword(dest+2);

					if (d <= hi)
					{
						I.STATUS |= ST_O;
						tms9900_ICount -= 16;
					}
					else
					{
						writeword(dest, divq/d);
						writeword(dest+2, divq%d);
						I.STATUS &= ~ST_O;
						tms9900_ICount -= 92;   /* from 92 to 124, possibly 92 + 2*(number of bits to 1 (or 0?) in quotient) */
					}
				}
		}
	}
	else	/* LDCR and STCR */
	{
		if (dest == 0)
			dest = 16;

		if (dest <= 8)
			src = decipheraddrbyte(opcode);
		else
			src = decipheraddr(opcode) & ~1;

		if (opcode < 0x3400)
		{	/* LDCR */
			/* LDCR -- LoaD into CRu */
			/* CRU R12--CRU R12+D-1 set to S */
			if (dest <= 8)
			{
				value = readbyte(src);
				setst_byte_laep(value);
				writeCRU(((READREG(R12) & 0x1FFE) >> 1), dest, value);
			}
			else
			{
				value = readword(src);
				setst_lae(value);
				writeCRU(((READREG(R12) & 0x1FFE) >> 1), dest, value);
				tms9900_ICount -= 20 + dest + dest;
			}
		}
		else
		{	/* STCR */
			/* STCR -- Store from CRu */
			/* S = CRU R12--CRU R12+D-1 */
			value = readCRU(((READREG(R12) & 0x1FFE) >> 1), dest);
			if (dest<=8)
			{
				setst_byte_laep(value);
				writebyte(src, value);
				tms9900_ICount -= (dest != 8) ? 42 : 44;
			}
			else
			{
				setst_lae(value);
				writeword(src, value);
				tms9900_ICount -= (dest != 16) ? 58 : 60;
			}
		}
	}
}


/*==========================================================================
   Two-Operand instructions                                    >4000->FFFF
 ---------------------------------------------------------------------------

      0 1 2 3-4 5 6 7+8 9 A B-C D E F
    ----------------------------------
    |opcode|B|TD |   D   |TS |   S   |
    ----------------------------------

  SZC, SZCB, S, SB, C, CB, A, AB, MOV, MOVB, SOC, SOCB
============================================================================*/
static void h4000(UINT16 opcode)
{
  register UINT16 src;
  register UINT16 dest;
  register UINT16 value;

  if (opcode & 0x1000)
  { /* byte instruction */
    src = decipheraddrbyte(opcode);
    dest = decipheraddrbyte(opcode >> 6);
  }
  else
  { /* word instruction */
    src = decipheraddr(opcode) & ~1;
    dest = decipheraddr(opcode >> 6) & ~1;
  }

  switch ((opcode >> 12) & 0x000F)    /* ((opcode & 0xF000) >> 12) */
  {
    case 4:   /* SZC */
      /* SZC --- Set Zeros Corresponding */
      /* D &= ~S */
      value = readword(dest) & (~ readword(src));
      setst_lae(value);
      writeword(dest, value);
      break;
    case 5:   /* SZCB */
      /* SZCB -- Set Zeros Corresponding, Byte */
      /* D &= ~S */
      value = readbyte(dest) & (~ readbyte(src));
      setst_byte_laep(value);
      writebyte(dest, value);
      break;
    case 6:   /* S */
      /* S ----- Subtract */
      /* D -= S */
      value = setst_sub_laeco(readword(dest), readword(src));
      writeword(dest, value);
      break;
    case 7:   /* SB */
      /* SB ---- Subtract, Byte */
      /* D -= S */
      value = setst_subbyte_laecop(readbyte(dest), readbyte(src));
      writebyte(dest, value);
      break;
    case 8:   /* C */
      /* C ----- Compare */
      /* ST = (D - S) */
      setst_c_lae(readword(dest), readword(src));
      break;
    case 9:   /* CB */
      /* CB ---- Compare Bytes */
      /* ST = (D - S) */
      value = readbyte(src);
      setst_c_lae(readbyte(dest)<<8, value<<8);
      lastparity = value;
      break;
    case 10:  /* A */
      /* A ----- Add */
      /* D += S */
      value = setst_add_laeco(readword(dest), readword(src));
      writeword(dest, value);
      break;
    case 11:  /* AB */
      /* AB ---- Add, Byte */
      /* D += S */
      value = setst_addbyte_laecop(readbyte(dest), readbyte(src));
      writebyte(dest, value);
      break;
    case 12:  /* MOV */
      /* MOV --- MOVe */
      /* Results:  D = S */
      value = readword(src);
      setst_lae(value);
      (void)readword(dest); /* yes, MOV performs a dummy read... */
      writeword(dest, value);
      break;
    case 13:  /* MOVB */
      /* MOVB -- MOVe Bytes */
      /* Results:  D = S */
      value = readbyte(src);
      setst_byte_laep(value);
      (void)readbyte(dest); /* MOVB read destination,too (this read is NOT useless, but this is a long story :-) ) */
      writebyte(dest, value);
      break;
    case 14:  /* SOC */
      /* SOC --- Set Ones Corresponding */
      /* D |= S */
      value = readword(dest) | readword(src);
      setst_lae(value);
      writeword(dest, value);
      break;
    case 15:  /* SOCB */
      /* SOCB -- Set Ones Corresponding, Byte */
      /* D |= S */
      value = readbyte(dest) | readbyte(src);
      setst_byte_laep(value);
      writebyte(dest, value);
      break;
  }
  tms9900_ICount -= 14;
}


INLINE void execute(UINT16 opcode)
{
  static void (* jumptable[128])(UINT16) =
  {
    &illegal,&h0200,&h0400,&h0400,&h0800,&h0800,&illegal,&illegal,
    &h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,&h1000,
    &h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,
    &h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,&h2000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,
    &h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000,&h4000
  };

  (* jumptable[opcode >> 9])(opcode);
}
