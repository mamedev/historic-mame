/*** t11: Portable DEC T-11 emulator ******************************************

	Copyright (C) Aaron Giles 1998

	System dependencies:	long must be at least 32 bits
	                        word must be 16 bit unsigned int
							byte must be 8 bit unsigned int
							long must be more than 16 bits
							arrays up to 65536 bytes must be supported
							machine must be twos complement

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "osd_dbg.h"
#include "cpuintrf.h"
#include "t11.h"


/* T-11 Registers */
typedef struct
{
    PAIR    reg[8];
    PAIR    psw;
    int     op;
    int     pending_interrupts;
    UINT8   *bank[8];
#if NEW_INTERRUPT_SYSTEM
    INT8    irq_state[4];
    int     (*irq_callback)(int irqline);
#endif
} t11_Regs;

static t11_Regs t11;

/* public globals */
int	t11_ICount=50000;

/* bits for the pending interrupts */
#define T11_IRQ0_BIT    0x0001
#define T11_IRQ1_BIT    0x0002
#define T11_IRQ2_BIT    0x0004
#define T11_IRQ3_BIT    0x0008
#define T11_WAIT        0x8000      /* Wait is pending */

/* register definitions and shortcuts */
#define REGD(x) t11.reg[x].d
#define REGW(x) t11.reg[x].w.l
#define REGB(x) t11.reg[x].b.l
#define SP REGW(6)
#define PC REGW(7)
#define SPD REGD(6)
#define PCD REGD(7)
#define PSW t11.psw.b.l

/* shortcuts for reading opcodes */
INLINE int ROPCODE (void)
{
	int pc = PCD;
	PC += 2;
	return READ_WORD (&t11.bank[pc >> 13][pc & 0x1fff]);
}

/* shortcuts for reading/writing memory bytes */
#define RBYTE(addr)      T11_RDMEM (addr)
#define WBYTE(addr,data) T11_WRMEM ((addr), (data))

/* shortcuts for reading/writing memory words */
INLINE int RWORD (int addr)
{
	return T11_RDMEM_WORD (addr);
}

INLINE void WWORD (int addr, int data)
{
	T11_WRMEM_WORD (addr, data);
}

/* pushes/pops a value from the stack */
INLINE void PUSH (int val)
{
	SP -= 2;
	WWORD (SPD, val);
}

INLINE int POP (void)
{
	int result = RWORD (SPD);
	SP += 2;
	return result;
}

/* flag definitions */
#define CFLAG 1
#define VFLAG 2
#define ZFLAG 4
#define NFLAG 8

/* extracts flags */
#define GET_C (PSW & CFLAG)
#define GET_V (PSW & VFLAG)
#define GET_Z (PSW & ZFLAG)
#define GET_N (PSW & NFLAG)

/* clears flags */
#define CLR_C (PSW &= ~CFLAG)
#define CLR_V (PSW &= ~VFLAG)
#define CLR_Z (PSW &= ~ZFLAG)
#define CLR_N (PSW &= ~NFLAG)

/* sets flags */
#define SET_C (PSW |= CFLAG)
#define SET_V (PSW |= VFLAG)
#define SET_Z (PSW |= ZFLAG)
#define SET_N (PSW |= NFLAG)

/* includes the static function prototypes and the master opcode table */
#include "t11table.c"

/* includes the actual opcode implementations */
#include "t11ops.c"


/****************************************************************************
 * Get all registers in given buffer
 ****************************************************************************/
unsigned t11_get_context(void *dst)
{
	if( dst )
		*(t11_Regs*)dst = t11;
	return sizeof(t11_Regs);
}

/****************************************************************************
 * Set all registers to given values
 ****************************************************************************/
void t11_set_context(void *src)
{
	if( src )
		t11 = *(t11_Regs*)src;
}

/****************************************************************************
 * Return program counter
 ****************************************************************************/
unsigned t11_get_pc(void)
{
	return PCD;
}

/****************************************************************************
 * Set program counter
 ****************************************************************************/
void t11_set_pc(unsigned val)
{
	PC = val;
}

/****************************************************************************
 * Return stack pointer
 ****************************************************************************/
unsigned t11_get_sp(void)
{
	return SPD;
}

/****************************************************************************
 * Set stack pointer
 ****************************************************************************/
void t11_set_sp(unsigned val)
{
	SP = val;
}

/****************************************************************************
 * Return a specific register                                               
 ****************************************************************************/
unsigned t11_get_reg(int regnum)
{
	switch( regnum )
	{
		case T11_R0: return t11.reg[0].w.l;
		case T11_R1: return t11.reg[1].w.l;
		case T11_R2: return t11.reg[2].w.l;
		case T11_R3: return t11.reg[3].w.l;
		case T11_R4: return t11.reg[4].w.l;
		case T11_R5: return t11.reg[5].w.l;
		case T11_SP: return t11.reg[6].w.l;
		case T11_PC: return t11.reg[7].w.l;
		case T11_PSW: return t11.psw.b.l;
		case T11_IRQ0_STATE: return t11.irq_state[T11_IRQ0];
		case T11_IRQ1_STATE: return t11.irq_state[T11_IRQ1];
		case T11_IRQ2_STATE: return t11.irq_state[T11_IRQ2];
		case T11_IRQ3_STATE: return t11.irq_state[T11_IRQ3];
		case T11_BANK0: return (unsigned)(t11.bank[0] - ROM);
		case T11_BANK1: return (unsigned)(t11.bank[1] - ROM);
		case T11_BANK2: return (unsigned)(t11.bank[2] - ROM);
		case T11_BANK3: return (unsigned)(t11.bank[3] - ROM);
		case T11_BANK4: return (unsigned)(t11.bank[4] - ROM);
		case T11_BANK5: return (unsigned)(t11.bank[5] - ROM);
		case T11_BANK6: return (unsigned)(t11.bank[6] - ROM);
		case T11_BANK7: return (unsigned)(t11.bank[7] - ROM);
	}
	return 0;
}

/****************************************************************************
 * Set a specific register                                                  
 ****************************************************************************/
void t11_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case T11_R0: t11.reg[0].w.l = val; break;
		case T11_R1: t11.reg[1].w.l = val; break;
		case T11_R2: t11.reg[2].w.l = val; break;
		case T11_R3: t11.reg[3].w.l = val; break;
		case T11_R4: t11.reg[4].w.l = val; break;
		case T11_R5: t11.reg[5].w.l = val; break;
		case T11_SP: t11.reg[6].w.l = val; break;
		case T11_PC: t11.reg[7].w.l = val; break;
		case T11_PSW: t11.psw.b.l = val; break;
		case T11_IRQ0_STATE: t11.irq_state[T11_IRQ0] = val; break;
		case T11_IRQ1_STATE: t11.irq_state[T11_IRQ1] = val; break;
		case T11_IRQ2_STATE: t11.irq_state[T11_IRQ2] = val; break;
		case T11_IRQ3_STATE: t11.irq_state[T11_IRQ3] = val; break;
		case T11_BANK0: t11.bank[0] = &ROM[val]; break;
		case T11_BANK1: t11.bank[1] = &ROM[val]; break;
		case T11_BANK2: t11.bank[2] = &ROM[val]; break;
		case T11_BANK3: t11.bank[3] = &ROM[val]; break;
		case T11_BANK4: t11.bank[4] = &ROM[val]; break;
		case T11_BANK5: t11.bank[5] = &ROM[val]; break;
		case T11_BANK6: t11.bank[6] = &ROM[val]; break;
		case T11_BANK7: t11.bank[7] = &ROM[val]; break;
    }
}

/****************************************************************************
 * Sets the banking                                                         
 ****************************************************************************/
void t11_SetBank(int offset, unsigned char *base)
{
	t11.bank[offset >> 13] = base;
}


void t11_reset(void *param)
{
	int i;
	extern unsigned char *RAM;

	memset (&t11, 0, sizeof (t11));
	SP = 0x0400;
	PC = 0x8000;
	PSW = 0xe0;

	for (i = 0; i < 8; i++)
		t11.bank[i] = &RAM[i * 0x2000];
	for (i = 0; i < 4; i++)
		t11.irq_state[i] = CLEAR_LINE;
	t11.pending_interrupts = 0;
}

void t11_exit(void)
{
	/* nothing to do */
}

void t11_set_nmi_line(int state)
{
	/* T-11 has no dedicated NMI line */
}

void t11_set_irq_line(int irqline, int state)
{
    t11.irq_state[irqline] = state;
	if( state == CLEAR_LINE )
	{
		switch( irqline )
		{
			case T11_IRQ0: t11.pending_interrupts &= ~T11_IRQ0_BIT; break;
			case T11_IRQ1: t11.pending_interrupts &= ~T11_IRQ1_BIT; break;
			case T11_IRQ2: t11.pending_interrupts &= ~T11_IRQ2_BIT; break;
			case T11_IRQ3: t11.pending_interrupts &= ~T11_IRQ3_BIT; break;
		}
	}
	else
	{
        t11.pending_interrupts &= ~T11_WAIT;
		switch( irqline )
		{
			case T11_IRQ0: t11.pending_interrupts |= T11_IRQ0_BIT; break;
			case T11_IRQ1: t11.pending_interrupts |= T11_IRQ1_BIT; break;
			case T11_IRQ2: t11.pending_interrupts |= T11_IRQ2_BIT; break;
			case T11_IRQ3: t11.pending_interrupts |= T11_IRQ3_BIT; break;
        }
    }
}

void t11_set_irq_callback(int (*callback)(int irqline))
{
	t11.irq_callback = callback;
}

/* Generate interrupts - I don't really know how this works, but this is how Paperboy uses them */
static void Interrupt(void)
{
	int level = (PSW >> 5) & 3;

    if (t11.pending_interrupts & T11_IRQ3_BIT)
	{
		PUSH (PSW);
		PUSH (PC);
		PCD = RWORD (0x60);
		PSW = RWORD (0x62);
		if( t11.irq_callback )
			(*t11.irq_callback)(T11_IRQ3);
    }
	else if ((t11.pending_interrupts & T11_IRQ2_BIT) && level < 3)
	{
		PUSH (PSW);
		PUSH (PC);
		PCD = RWORD (0x50);
		PSW = RWORD (0x52);
		if( t11.irq_callback )
			(*t11.irq_callback)(T11_IRQ2);
    }
	else if ((t11.pending_interrupts & T11_IRQ1_BIT) && level < 2)
	{
		PUSH (PSW);
		PUSH (PC);
		PCD = RWORD (0x40);
		PSW = RWORD (0x42);
		if( t11.irq_callback )
			(*t11.irq_callback)(T11_IRQ1);
    }
	else if ((t11.pending_interrupts & T11_IRQ0_BIT) && level < 1)
	{
		PUSH (PSW);
		PUSH (PC);
		PCD = RWORD (0x38);
		PSW = RWORD (0x3a);
		if( t11.irq_callback )
			(*t11.irq_callback)(T11_IRQ0);
    }
}



/* execute instructions on this CPU until icount expires */
int t11_execute(int cycles)
{
	t11_ICount = cycles;

	if (t11.pending_interrupts & T11_WAIT)
	{
		t11_ICount = 0;
		goto getout;
	}

change_pc (0xffff);
	do
	{
		if (t11.pending_interrupts != 0)
			Interrupt();

#if 0
/* use this code to nail a bogus jump or opcode */
		{
			extern int DasmT11 (unsigned char *pBase, char *buffer, int pc);
			static unsigned int pclist[256];
			static unsigned char inst[256][8];
			static int pcindex = 0;
			int i;

			pclist[pcindex] = PCD;
			for (i = 0; i < 8; i++)
				inst[pcindex][i] = t11.bank[(PCD + i) >> 13][(PCD + i) & 0x1fff];
			pcindex = (pcindex + 1) & 255;

			if (PCD < 0x4000)
			{
				char buffer[200];
				int i;

				#undef printf
				printf ("Error!\n");
				for (i = 255; i >= 0; i--)
				{
					unsigned char *temp;
					int index = (pcindex - i) & 255;

					temp = OP_ROM;
					OP_ROM = &inst[index][0] - pclist[index];
					DasmT11 (&inst[index][0], buffer, pclist[index]);
					OP_ROM = temp;

					printf ("%04X: %s\n", pclist[index], buffer);
				}
				printf ("$38 = %04X\n", RAM[0x38] + (RAM[0x39] << 8));
				printf ("$40 = %04X\n", RAM[0x40] + (RAM[0x41] << 8));
				printf ("$50 = %04X\n", RAM[0x50] + (RAM[0x51] << 8));
				printf ("$60 = %04X\n", RAM[0x60] + (RAM[0x61] << 8));
				gets (buffer);
			}
		}
#endif


#ifdef	MAME_DEBUG
		if (mame_debug) MAME_Debug();
#endif

		t11.op = ROPCODE ();
		(*opcode_table[t11.op >> 3])();

		t11_ICount -= 22;

	} while (t11_ICount > 0);

getout:
	return cycles - t11_ICount;
}

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *t11_info( void *context, int regnum )
{
	static char buffer[16][47+1];
	static int which = 0;
	t11_Regs *r = context;

	which = ++which % 16;
    buffer[which][0] = '\0';

	if( !context )
		r = &t11;

    switch( regnum )
	{
		case CPU_INFO_NAME: return "T11";
		case CPU_INFO_FAMILY: return "DEC T-11";
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (C) Aaron Giles 1998";
		case CPU_INFO_PC: sprintf(buffer[which], "%04X:", r->reg[7].w.l); break;
		case CPU_INFO_SP: sprintf(buffer[which], "%04X", r->reg[6].w.l); break;
#ifdef MAME_DEBUG
		case CPU_INFO_DASM: r->reg[7].w.l += DasmT11(&ROM[r->reg[7].w.l], buffer[which], r->reg[7].w.l); break;
#else
		case CPU_INFO_DASM: sprintf(buffer[which], "%06o", READ_WORD(&ROM[r->reg[7].w.l])); r->reg[7].w.l += 2; break;
#endif
		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				r->psw.b.l & 0x80 ? '?':'.',
				r->psw.b.l & 0x40 ? 'I':'.',
				r->psw.b.l & 0x20 ? 'I':'.',
				r->psw.b.l & 0x10 ? 'T':'.',
				r->psw.b.l & 0x08 ? 'N':'.',
				r->psw.b.l & 0x04 ? 'Z':'.',
				r->psw.b.l & 0x02 ? 'V':'.',
				r->psw.b.l & 0x01 ? 'C':'.');
			break;
		case CPU_INFO_REG+T11_R0: sprintf(buffer[which], "R0:%04X", r->reg[0].w.l); break;
		case CPU_INFO_REG+T11_R1: sprintf(buffer[which], "R1:%04X", r->reg[1].w.l); break;
		case CPU_INFO_REG+T11_R2: sprintf(buffer[which], "R2:%04X", r->reg[2].w.l); break;
		case CPU_INFO_REG+T11_R3: sprintf(buffer[which], "R3:%04X", r->reg[3].w.l); break;
		case CPU_INFO_REG+T11_R4: sprintf(buffer[which], "R4:%04X", r->reg[4].w.l); break;
		case CPU_INFO_REG+T11_R5: sprintf(buffer[which], "R5:%04X", r->reg[5].w.l); break;
		case CPU_INFO_REG+T11_SP: sprintf(buffer[which], "SP:%04X", r->reg[6].w.l); break;
		case CPU_INFO_REG+T11_PC: sprintf(buffer[which], "PC:%04X", r->reg[7].w.l); break;
		case CPU_INFO_REG+T11_PSW: sprintf(buffer[which], "PSW:%02X", r->psw.b.l); break;
		case CPU_INFO_REG+T11_IRQ0_STATE: sprintf(buffer[which], "IRQ0:%X", r->irq_state[T11_IRQ0]); break;
		case CPU_INFO_REG+T11_IRQ1_STATE: sprintf(buffer[which], "IRQ1:%X", r->irq_state[T11_IRQ1]); break;
		case CPU_INFO_REG+T11_IRQ2_STATE: sprintf(buffer[which], "IRQ2:%X", r->irq_state[T11_IRQ2]); break;
		case CPU_INFO_REG+T11_IRQ3_STATE: sprintf(buffer[which], "IRQ3:%X", r->irq_state[T11_IRQ3]); break;
		case CPU_INFO_REG+T11_BANK0: sprintf(buffer[which], "BANK0:%06X", (unsigned)(r->bank[0] - ROM)); break;
		case CPU_INFO_REG+T11_BANK1: sprintf(buffer[which], "BANK1:%06X", (unsigned)(r->bank[1] - ROM)); break;
		case CPU_INFO_REG+T11_BANK2: sprintf(buffer[which], "BANK2:%06X", (unsigned)(r->bank[2] - ROM)); break;
		case CPU_INFO_REG+T11_BANK3: sprintf(buffer[which], "BANK3:%06X", (unsigned)(r->bank[3] - ROM)); break;
		case CPU_INFO_REG+T11_BANK4: sprintf(buffer[which], "BANK4:%06X", (unsigned)(r->bank[4] - ROM)); break;
		case CPU_INFO_REG+T11_BANK5: sprintf(buffer[which], "BANK5:%06X", (unsigned)(r->bank[5] - ROM)); break;
		case CPU_INFO_REG+T11_BANK6: sprintf(buffer[which], "BANK6:%06X", (unsigned)(r->bank[6] - ROM)); break;
		case CPU_INFO_REG+T11_BANK7: sprintf(buffer[which], "BANK7:%06X", (unsigned)(r->bank[7] - ROM)); break;
    }
	return buffer[which];
}
