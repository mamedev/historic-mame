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
#include "t11.h"
#include "driver.h"


static t11_Regs state;

/* public globals */
int	t11_ICount=50000;

/* bits for the pending interrupts */
#define T11_IRQ0_BIT    0x0001
#define T11_IRQ1_BIT    0x0002
#define T11_IRQ2_BIT    0x0004
#define T11_IRQ3_BIT    0x0008
#define T11_WAIT        0x8000      /* Wait is pending */

/* register definitions and shortcuts */
#define REGD(x) state.reg[x].d
#define REGW(x) state.reg[x].w.l
#define REGB(x) state.reg[x].b.l
#define SP REGW(6)
#define PC REGW(7)
#define SPD REGD(6)
#define PCD REGD(7)
#define PSW state.psw.b.l

/* shortcuts for reading opcodes */
INLINE int ROPCODE (void)
{
	int pc = PCD;
	PC += 2;
	return READ_WORD (&state.bank[pc >> 13][pc & 0x1fff]);
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
	state = *Regs;
}

/****************************************************************************/
/* Get all registers in given buffer                                        */
/****************************************************************************/
void t11_GetRegs(t11_Regs *Regs)
{
	*Regs = state;
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
	state.bank[offset >> 13] = base;
}


void t11_reset(void)
{
	int i;
	extern unsigned char *RAM;

	memset (&state, 0, sizeof (state));
	SP = 0x0400;
	PC = 0x8000;
	PSW = 0xe0;

	for (i = 0; i < 8; i++)
		state.bank[i] = &RAM[i * 0x2000];

	t11_Clear_Pending_Interrupts();
}


void t11_Cause_Interrupt(int type)
{
	if (type >= 0 && type <= 3)
	{
		state.pending_interrupts |= 1 << type;
		state.pending_interrupts &= ~T11_WAIT;
	}
}


void t11_Clear_Pending_Interrupts(void)
{
	state.pending_interrupts &= ~(T11_IRQ3_BIT | T11_IRQ2_BIT | T11_IRQ1_BIT | T11_IRQ0_BIT);
}


/* Generate interrupts - I don't really know how this works, but this is how Paperboy uses them */
static void Interrupt(void)
{
	int level = (PSW >> 5) & 3;

	if (state.pending_interrupts & T11_IRQ3_BIT)
	{
		state.pending_interrupts &= ~T11_IRQ3_BIT;
		PUSH (PSW);
		PUSH (PC);
		PCD = RWORD (0x60);
		PSW = RWORD (0x62);
	}
	else if ((state.pending_interrupts & T11_IRQ2_BIT) && level < 3)
	{
		state.pending_interrupts &= ~T11_IRQ2_BIT;
		PUSH (PSW);
		PUSH (PC);
		PCD = RWORD (0x50);
		PSW = RWORD (0x52);
	}
	else if ((state.pending_interrupts & T11_IRQ1_BIT) && level < 2)
	{
		state.pending_interrupts &= ~T11_IRQ1_BIT;
		PUSH (PSW);
		PUSH (PC);
		PCD = RWORD (0x40);
		PSW = RWORD (0x42);
	}
	else if ((state.pending_interrupts & T11_IRQ0_BIT) && level < 1)
	{
		state.pending_interrupts &= ~T11_IRQ0_BIT;
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

	if (state.pending_interrupts & T11_WAIT)
	{
		t11_ICount = 0;
		goto getout;
	}

change_pc (0xffff);
	do
	{
		if (state.pending_interrupts != 0)
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
				inst[pcindex][i] = state.bank[(PCD + i) >> 13][(PCD + i) & 0x1fff];
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

		state.op = ROPCODE ();
		(*opcode_table[state.op >> 3])();

		t11_ICount -= 22;

	} while (t11_ICount > 0);

getout:
	return cycles - t11_ICount;
}
