/*========================================
  tms9900.c

	TMS9900 emulation

  Original emulator by Edward Swartz.
  Smoothed out by Raphael Nabet

  Converted for Mame by M.Coates
  Some additional work by R Nabet
========================================*/


#include "memory.h"
#include "mamedbg.h"
#include "tms9900.h"

INLINE void execute(UINT16 opcode);

static UINT16 fetch(void);
static UINT16 decipheraddr(UINT16 opcode);
static UINT16 decipheraddrbyte(UINT16 opcode);
static void contextswitch(UINT16 addr);

static void h0000(UINT16 opcode);
static void h0200(UINT16 opcode);
static void h0400(UINT16 opcode);
static void h0800(UINT16 opcode);
static void h1000(UINT16 opcode);
static void h2000(UINT16 opcode);
static void h4000(UINT16 opcode);

static void tms9900_assert_irq(int int_level);
static void tms9900_clear_irq(int int_level);
static void field_interrupt(void);

/***************************/
/* Mame Interface Routines */
/***************************/

extern  FILE *errorlog;

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
     0,23,80, 1,    /* command line window (bottom rows) */
};

int     tms9900_ICount = 0;

/* TMS9900 ST register bits. */

/* These bits are set by every compare, move and arithmetic or logicaloperation : */
/* (Well, COC, CZC and TB only set the E bit, but these are kind ofexceptions.) */
#define ST_L 0x8000 /* logically greater (strictly) */
#define ST_A 0x4000 /* arithmetically greater (strictly) */
#define ST_E 0x2000 /* equal */

/* These bits are set by arithmetic operations, when it makes sense toupdate them. */
#define ST_C 0x1000 /* Carry */
#define ST_O 0x0800 /* Overflow (overflow with operations on signedintegers, */
                    /* and when the result of a 32bits:16bits divisioncannot fit in a 16-bit word.) */

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
    UINT16  WP;
    UINT16  PC;
    UINT16  STATUS;
    UINT16  IR;
    int irq_state;
    int (*irq_callback)(int irq_line);
	int interrupts_pending;
#if MAME_DEBUG
	UINT16	FR[16];
#endif
}	tms9900_Regs;

static tms9900_Regs I;
UINT8 lastparity = 0;

#define readword(addr)			((cpu_readmem16((addr)) << 8) +cpu_readmem16((addr)+1))
#define writeword(addr,data)	{ cpu_writemem16((addr), ((data)>>8));cpu_writemem16(((addr)+1),((data) & 0xff));}

#define readbyte(addr)			(cpu_readmem16(addr))
#define writebyte(addr,data)	{ cpu_writemem16((addr),(data)) ; }

#define READREG(reg)			readword(I.WP+reg)
#define WRITEREG(reg,data)		writeword(I.WP+reg,data)

#define IMASK					(I.STATUS & 0x0F)

/************************************************************************
 * Status word functions
 ************************************************************************/
#include "9900stat.h"

/**************************************************************************/

void tms9900_reset(void *param)
{
	I.STATUS = 0;
	I.WP=readword(0);
	I.PC=readword(2);
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

        I.IR = fetch();
      	execute(I.IR);

        /* Fudge : I'll put timings in when it works! */

		tms9900_ICount -= 12 ;

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
		getstat();
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
	Nota : the WP register actually has nothing to do with a stack.

	To make this even weirder, some later versions of the TMS9900 dohave a processor stack.
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
 *  These are just hacked to compile and need properly sorting out!
 */

void tms9900_set_nmi_line(int state)
{
	/* TMS9900 has no dedicated NMI line? */

	/*R Nabet : it has : LOAD line -> interrupt vector -1 (same as LREXinstruction)*/
	/*HJB: and how do we let this interrupt happen? */
}

/*
 * R Nabet : I rewrote interrupt handling code to have it work with TI99.
 *
 * I am not sure whether it is correct or not (I do think it is mostly
 * correct, though).
 *
 * Credits to Bob Nestor and MAME 68k emulator (can't remember its name and
 * authors...).
 *
 * HJB 990430: changed to use irq_callback() to retrieve the vector
 * instead of using 16 irqlines. A driver using the TMS9900 should do
 *		cpu_0_irq_line_vector_w(0, vector);
 *		cpu_set_irq_line(0,0,HOLD_LINE);
 */
void tms9900_set_irq_line(int irqline, int state)
{
	int vector;

    if (state == I.irq_state) return;

    I.irq_state = state;
	vector = I.irq_callback(irqline);

    if (state == CLEAR_LINE)
		tms9900_clear_irq(vector);
	else
		tms9900_assert_irq(vector);

	field_interrupt();
}

void tms9900_set_irq_callback(int (*callback)(int irqline))
{
	I.irq_callback = callback;
}

/*
 *	tms9900_assert_irq - declares a 9900 machine interrupt.
 */
static void tms9900_assert_irq(int int_level)
{
	I.interrupts_pending |= 1 << int_level;
}

static void tms9900_clear_irq(int int_level)
{
	I.interrupts_pending &= ~(1 << int_level);
}

/*
 * field_interrupt
 *
 * Check whether some pending interrupts can be honored, and honor thehighest one if needed.
 *
 * Should be called after after instruction, but calling it when an irq israised or
 * the interrupt mask is modified seems to be enough.
 *
 * R Nabet, largely inspirated by code by Bob Nestor.
 */
static void field_interrupt(void)
{
	int highest_enabled, i;
	UINT16 old_pc, old_wp, old_st;

	setstat();
	highest_enabled = I.STATUS & 0x000F;
	for (i = 1; i <= highest_enabled; i++)
	{
		if ((1 << i) & I.interrupts_pending)
		{
			tms9900_clear_irq(i);
			old_pc = I.PC;
			old_wp = I.WP;
			old_st = I.STATUS;
			I.WP = readword(i*4);
			I.PC = readword(2+i*4);
			I.STATUS = (I.STATUS & 0xFFF0) | (i-1);
			getstat();
			WRITEREG(R13, old_wp);
			WRITEREG(R14, old_pc);
			WRITEREG(R15, old_st);
			break;
		}
	}
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
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Converted for Mame by M.Coates (C/PPC file for TMS9900 emulation by R NABET, based on work by Edward Swartz)";
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

static void writeCRU(int CRUAddr, int Number, UINT16 Value)
{
	int count;

	if (errorlog) fprintf(errorlog,"PC %4.4x Write CRU %x for %x =%x\n",I.PC,CRUAddr,Number,Value);

	/* Write Number bits from CRUAddr */

    for(count=0;count<Number;count++)
    {
    	cpu_writeport(CRUAddr+count,(Value & 0x01));
        Value >>= 1;
    }
}

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
	    Value = (cpu_readport(Location+1) << 8) + cpu_readport(Location);

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
	    Value = (cpu_readport(Location+2) << 16)
		      + (cpu_readport(Location+1) << 8)
              + cpu_readport(Location);

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

/* contextswitch : performs a BLWP, ie change PC, WP, and save PC, WP andST... */
static void contextswitch(UINT16 addr)
{
  UINT16 oldWP,oldpc;

  oldWP = I.WP;
  oldpc = I.PC;

  I.WP = readword(addr) & ~1;
  I.PC = readword(addr+2) & ~1;

  WRITEREG(R13, oldWP);
  WRITEREG(R14, oldpc);
  setstat();
  WRITEREG(R15, I.STATUS);
}

/*
 * decipheraddr : compute and return the effective adress in wordinstructions.
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
    return(reg + I.WP);
  else if (ts == 0x10)
    return(readword(reg + I.WP));
  else if (ts == 0x20)
  {
	register UINT16 imm;

    imm = fetch();
    if (reg)
    {
      return(readword(reg + I.WP) + imm);
    }
    else
      return(imm);
  }
  else /*if (ts == 0x30)*/
  {
	register UINT16 reponse;

	reg += I.WP;	  /* reg now contains effective address */

    reponse = readword(reg);
	writeword(reg, reponse+2); /* we increment register content */
    return(reponse);
  }
}

/* decipheraddrbyte : compute and return the effective adress in byteinstructions. */
static UINT16 decipheraddrbyte(UINT16 opcode)
{
  register UINT16 ts = opcode & 0x30;
  register UINT16 reg = opcode & 0xF;

  reg += reg;

  if (ts == 0)
    return(reg + I.WP);
  else if (ts == 0x10)
    return(readword(reg + I.WP));
  else if (ts == 0x20)
  {
	register UINT16 imm;

    imm = fetch();
    if (reg)
    {
      return(readword(reg + I.WP) + imm);
    }
    else
      return(imm);
  }
  else /*if (ts == 0x30)*/
  {
	register UINT16 reponse;

	reg += I.WP;	  /* reg now contains effective address */

    reponse = readword(reg);
	writeword(reg, reponse+1); /* we increment register content */
    return(reponse);
  }
}


/*************************************************************************/


/*==========================================================================
   Data instructions,                                          >0000->01FF
============================================================================*/
static void h0000(UINT16 opcode)
{
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
      break;
		case 1:   /* AI */
			/* AI ---- Add Immediate */
			/* *Reg += *PC+ */
      value = fetch();
      wadd(addr, value);
      break;
		case 2:   /* ANDI */
			/* ANDI -- AND Immediate */
			/* *Reg &= *PC+ */
      value = fetch();
      value = readword(addr) & value;
      writeword(addr, value);
      setst_lae(value);
      break;
		case 3:   /* ORI */
			/* ORI --- OR Immediate */
			/* *Reg |= *PC+ */
      value = fetch();
      value = readword(addr) | value;
      writeword(addr, value);
      setst_lae(value);
      break;
		case 4:   /* CI */
			/* CI ---- Compare Immediate */
			/* status = (*Reg-*PC+) */
      value = fetch();
      setst_c_lae(value, readword(addr));
      break;
		case 5:   /* STWP */
			/* STWP -- STore Workspace Pointer */
			/* *Reg = WP */
      writeword(addr, I.WP);
      break;
		case 6:   /* STST */
			/* STST -- STore STatus register */
			/* *Reg = ST */
      setstat();
      writeword(addr, I.STATUS);
      break;
		case 7:   /* LWPI */
			/* LWPI -- Load Workspace Pointer Immediate */
			/* WP = *PC+ */
      I.WP = fetch();
      break;
		case 8:   /* LIMI */
			/* LIMI -- Load Interrupt Mask Immediate */
			/* ST&15 |= (*PC+)&15 */
      value = fetch();
      I.STATUS = (I.STATUS & ~ 0xF) | (value & 0xF);
			field_interrupt();  /*IM has been modified.*/
			break;
		case 10:  /* IDLE */
			/* IDLE -- IDLE until a reset, interrupt, load */
			/* The TMS99000 locks until an interrupt happen(like with 68k STOP instruction). */


			break;
		case 11:  /* RSET */
			/* RSET -- ReSET */
			/* Resets the ST register. */
			I.STATUS &= 0xFFF0; /*clear IM.*/
			//getstat();
			field_interrupt();  /*IM has been modified.*/
			break;
		case 12:  /* RTWP */
			/* RTWP -- Return with Workspace Pointer */
			/* WP = R13, PC = R14, ST = R15 */
      I.STATUS = READREG(R15);
      getstat();
      I.PC = READREG(R14);
      I.WP = READREG(R13);
			field_interrupt();  /*IM has been modified.*/
			break;
		case 13:  /* CKON */
			/* CKON -- ClocK ON */
			/* Enable external clock. */


			break;
		case 14:  /* CKOF */
			/* CKOF -- ClocK OFf */
			/* Disable external clock. */


      /*To implement these two instructions :*/
      /* - callback procedures */
      /* - or CRU writes (???) */


			break;
		case 15:  /* LREX */
			/* LREX -- Load or REstart eXecution */
			/* Cause a LOAD interrupt (vector -1). */
      contextswitch(0xFFFC);
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
  register UINT16 value;	/* used for anything */

  switch ((opcode &0x3C0) >> 6)
  {
    case 0:   /* BLWP */
      /* BLWP -- Branch with Workspace Pointer */
      /* Result: WP = *S+, PC = *S */
      /*         New R13=old WP, New R14=Old PC, New R15=Old ST */
      contextswitch(addr);
      break;
    case 1:   /* B */
      /* B ----- Branch */
      /* PC = S */
      I.PC = addr;
      break;
    case 2:   /* X */
      /* X ----- eXecute */
      /* Executes instruction *S */
      execute(readword(addr));
      break;
    case 3:   /* CLR */
      /* CLR --- CLeaR */
	  /* *S = 0 */
      writeword(addr, 0);
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
      break;
    case 5:   /* INV */
      /* INV --- INVert */
	  /* *S = ~*S */
      value = ~ readword(addr);
      writeword(addr, value);
      setst_lae(value);
      break;
    case 6:   /* INC */
      /* INC --- INCrement */
	  /* (*S)++ */
      wadd(addr, 1);
      break;
    case 7:   /* INCT */
      /* INCT -- INCrement by Two */
	  /* (*S) +=2 */
      wadd(addr, 2);
      break;
    case 8:   /* DEC */
      /* DEC --- DECrement */
	  /* (*S)-- */
      wsub(addr, 1);
      break;
    case 9:   /* DECT */
      /* DECT -- DECrement by Two */
	  /* (*S) -= 2 */
      wsub(addr, 2);
      break;
    case 10:  /* BL */
      /* BL ---- Branch and Link */
	  /* IP=S, R11=old IP */
      WRITEREG(R11, I.PC);
      I.PC = addr;
      break;
    case 11:  /* SWPB */
      /* SWPB -- SWaP Bytes */
	  /* *S = swab(*S) */
      value = readword(addr);
      value = logical_right_shift(value, 8) | (value << 8);
      writeword(addr, value);
      break;
    case 12:  /* SETO */
      /* SETO -- SET Ones */
	  /* *S = #$FFFF */
      writeword(addr, 0xFFFF);
      break;
    case 13:  /* ABS */
      /* ABS --- ABSolute value */
	  /* *S = |*S| */
	  /* clearing ST_C seems to be necessary, although ABS will neverset it. */
      I.STATUS &= ~ (ST_L | ST_A | ST_E | ST_C | ST_O);
      value = readword(addr);

	  if (((INT16) value) > 0)
        I.STATUS |=  ST_L | ST_A;
	  else if (((INT16) value) < 0)
      {
        I.STATUS |= ST_L;
        if (value == 0x8000)
          I.STATUS |= ST_O;
		writeword(addr, - ((INT16) value));
      }
      else
        I.STATUS |= ST_E;

      break;
  }
}


/*==========================================================================
   Shift instructions,                                         >0800->0BFF
   (V9T9 instructions,                                         >0C00->0FFF)
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

  if (cnt == 0)
    cnt = READREG(0) & 0xF;

  /* ONLY shift now */
  switch ((opcode & 0x700) >> 8)
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
      break;
    case 1:   /* JLT */
      /* JLT --- Jump if Less Than (arithmetic) */
      /* if (A==0 && EQ==0), PC += offset */
      if (! (I.STATUS & (ST_A | ST_E)))
        I.PC += (offset + offset);
      break;
    case 2:   /* JLE */
      /* JLE --- Jump if Lower or Equal (logical) */
      /* if (L==0 || EQ==1), PC += offset */
      if ((! (I.STATUS & ST_L)) || (I.STATUS & ST_E))
        I.PC += (offset + offset);
      break;
    case 3:   /* JEQ */
      /* JEQ --- Jump if Equal */
      /* if (EQ==1), PC += offset */
      if (I.STATUS & ST_E)
        I.PC += (offset + offset);
      break;
    case 4:   /* JHE */
      /* JHE --- Jump if Higher or Equal (logical) */
      /* if (L==1 || EQ==1), PC += offset */
      if  (I.STATUS & (ST_L | ST_E))
        I.PC += (offset + offset);
      break;
    case 5:   /* JGT */
      /* JGT --- Jump if Greater Than (arithmetic) */
      /* if (A==1), PC += offset */
      if (I.STATUS & ST_A)
        I.PC += (offset + offset);
      break;
    case 6:   /* JNE */
      /* JNE --- Jump if Not Equal */
      /* if (EQ==0), PC += offset */
      if (! (I.STATUS & ST_E))
        I.PC += (offset + offset);
      break;
    case 7:   /* JNC */
      /* JNC --- Jump if No Carry */
      /* if (C==0), PC += offset */
      if (! (I.STATUS & ST_C))
        I.PC += (offset + offset);
      break;
    case 8:   /* JOC */
      /* JOC --- Jump On Carry */
      /* if (C==1), PC += offset */
      if (I.STATUS & ST_C)
        I.PC += (offset + offset);
      break;
    case 9:   /* JNO */
      /* JNO --- Jump if No Overflow */
      /* if (OV==0), PC += offset */
      if (! (I.STATUS & ST_O))
        I.PC += (offset + offset);
      break;
    case 10:  /* JL */
      /* JL ---- Jump if Lower (logical) */
      /* if (L==0 && EQ==0), PC += offset */
      if (! (I.STATUS & (ST_L | ST_E)))
        I.PC += (offset + offset);
      break;
    case 11:  /* JH */
      /* JH ---- Jump if Higher (logical) */
      /* if (L==1 && EQ==0), PC += offset */
      if ((I.STATUS & ST_L) && ! (I.STATUS & ST_E))
        I.PC += (offset + offset);
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
          I.PC += (offset + offset);
      }

      break;
    case 13:  /* SBO */
      /* SBO --- Set Bit to One */
      /* CRU Bit = 1 */
      writeCRU(((READREG(R12) & 0x1FFE) >> 1) + offset, 1, 1);
      break;
    case 14:  /* SBZ */
      /* SBZ --- Set Bit to Zero */
	  /* CRU Bit = 0 */
      writeCRU(((READREG(R12) & 0x1FFE) >> 1) + offset, 1, 0);
      break;
    case 15:  /* TB */
      /* TB ---- Test Bit */
	  /* EQ = (CRU Bit == 1) */
      setst_e(readCRU(((READREG(R12) & 0x1FFE) >> 1) + offset, 1) & 1, 1);
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

  if (opcode < 0x3000 || opcode >= 0x3800)	/* not LDCR and STCR */
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
        break;
      case 1:   /* CZC */
        /* CZC --- Compare Zeroes Corresponding */
		/* status E bit = (S&~D == S) */
        dest = ((dest+dest) + I.WP) & ~1;
        src &= ~1;
        value = readword(src);
        setst_e(value & (~ readword(dest)), value);
        break;
      case 2:   /* XOR */
        /* XOR --- eXclusive OR */
		/* D ^= S */
        dest = ((dest+dest) + I.WP) & ~1;
        src &= ~1;
        value = readword(dest) ^ readword(src);
        setst_lae(value);
        writeword(dest,value);
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
            I.STATUS |= ST_O;
          else
          {
            writeword(dest, divq/d);
            writeword(dest+2, divq%d);
            I.STATUS &= ~ST_O;
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
    {    /* LDCR */
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
      }
    }
    else
    {    /* STCR */
      /* STCR -- Store from CRu */
	  /* S = CRU R12--CRU R12+D-1 */
      if (dest<=8)
      {
        value = readCRU(((READREG(R12) & 0x1FFE) >> 1), dest);
        setst_byte_laep(value);
        writebyte(src, value);
      }
      else
      {
        value = readCRU(((READREG(R12) & 0x1FFE) >> 1), dest);
        setst_lae(value);
        writeword(src, value);
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

  switch ((opcode >> 12) &0x000F)    /* ((opcode & 0xF000) >> 12) */
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
      writeword(dest, value);
      break;
    case 13:  /* MOVB */
      /* MOVB -- MOVe Bytes */
      /* Results:  D = S */
      value = readbyte(src);
      setst_byte_laep(value);
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
}


INLINE void execute(UINT16 opcode)
{
  static void (* jumptable[128])(UINT16) =
  {
    &h0000,&h0200,&h0400,&h0400,&h0800,&h0800,&h0800,&h0800,
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
