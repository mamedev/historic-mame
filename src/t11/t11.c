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
#include "osd_dbg.h"
#include "cpuintrf.h"
#include "t11.h"


static t11_Regs t11;

/* public globals */
int	t11_ICount=50000;

/* bits for the pending interrupts */
#define T11_IRQ0_BIT    0x0001
#define T11_IRQ1_BIT    0x0002
#define T11_IRQ2_BIT    0x0004
#define T11_IRQ3_BIT    0x0008
#define T11_WAIT        0x8000      /* Wait is pending */

#if NEW_INTERRUPT_SYSTEM
#define T11_PENDING 	0x4000		/* IRQ is pending */
#endif

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


/****************************************************************************/
/* Set all registers to given values                                        */
/****************************************************************************/
void t11_SetRegs(t11_Regs *Regs)
{
	t11 = *Regs;
}

/****************************************************************************/
/* Get all registers in given buffer                                        */
/****************************************************************************/
void t11_GetRegs(t11_Regs *Regs)
{
	*Regs = t11;
}

/****************************************************************************/
/* Return program counter                                                   */
/****************************************************************************/
unsigned t11_GetPC(void)
{
	return PCD;
}

/****************************************************************************/
/* Sets the banking                                                         */
/****************************************************************************/
void t11_SetBank(int offset, unsigned char *base)
{
	t11.bank[offset >> 13] = base;
}


void t11_reset(void)
{
	int i;
	extern unsigned char *RAM;

	memset (&t11, 0, sizeof (t11));
	SP = 0x0400;
	PC = 0x8000;
	PSW = 0xe0;

	for (i = 0; i < 8; i++)
		t11.bank[i] = &RAM[i * 0x2000];
#if NEW_INTERRUPT_SYSTEM
	for (i = 0; i < 4; i++)
		t11.irq_state[i] = CLEAR_LINE;
	t11.pending_interrupts = 0;
#else
    t11_Clear_Pending_Interrupts();
#endif
}


#if NEW_INTERRUPT_SYSTEM

void t11_set_nmi_line(int state)
{
	/* T-11 has no dedicated NMI line */
}

void t11_set_irq_line(int irqline, int state)
{
	t11.irq_state[irqline] = state;
	if (state == CLEAR_LINE) {
		t11.pending_interrupts &= ~T11_PENDING;
	} else {
        t11.pending_interrupts &= ~T11_WAIT;
		t11.pending_interrupts |= T11_PENDING;
    }
}

void t11_set_irq_callback(int (*callback)(int irqline))
{
	t11.irq_callback = callback;
}

#else

void t11_Cause_Interrupt(int type)
{
	if (type >= 0 && type <= 3)
	{
		t11.pending_interrupts |= 1 << type;
		t11.pending_interrupts &= ~T11_WAIT;
	}
}

void t11_Clear_Pending_Interrupts(void)
{
	t11.pending_interrupts &= ~(T11_IRQ3_BIT | T11_IRQ2_BIT | T11_IRQ1_BIT | T11_IRQ0_BIT);
}

#endif

/* Generate interrupts - I don't really know how this works, but this is how Paperboy uses them */
static void Interrupt(void)
{
	int level = (PSW >> 5) & 3;

#if NEW_INTERRUPT_SYSTEM
	if (t11.pending_interrupts & T11_PENDING) {
		int type;
		if (*t11.irq_callback) {
			/* get the type (vector) */
			type = (*t11.irq_callback)(0);
			/* and set the corresponding bit */
			t11.pending_interrupts |= 1 << type;
		}
    }
#endif

    if (t11.pending_interrupts & T11_IRQ3_BIT)
	{
		t11.pending_interrupts &= ~T11_IRQ3_BIT;
		PUSH (PSW);
		PUSH (PC);
		PCD = RWORD (0x60);
		PSW = RWORD (0x62);
	}
	else if ((t11.pending_interrupts & T11_IRQ2_BIT) && level < 3)
	{
		t11.pending_interrupts &= ~T11_IRQ2_BIT;
		PUSH (PSW);
		PUSH (PC);
		PCD = RWORD (0x50);
		PSW = RWORD (0x52);
	}
	else if ((t11.pending_interrupts & T11_IRQ1_BIT) && level < 2)
	{
		t11.pending_interrupts &= ~T11_IRQ1_BIT;
		PUSH (PSW);
		PUSH (PC);
		PCD = RWORD (0x40);
		PSW = RWORD (0x42);
	}
	else if ((t11.pending_interrupts & T11_IRQ0_BIT) && level < 1)
	{
		t11.pending_interrupts &= ~T11_IRQ0_BIT;
		PUSH (PSW);
		PUSH (PC);
		PCD = RWORD (0x38);
		PSW = RWORD (0x3a);
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
{
  extern int mame_debug;
  if (mame_debug) MAME_Debug();
}
#endif

		t11.op = ROPCODE ();
		(*opcode_table[t11.op >> 3])();

		t11_ICount -= 22;

	} while (t11_ICount > 0);

getout:
	return cycles - t11_ICount;
}
