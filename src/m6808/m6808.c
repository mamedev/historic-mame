/*** m6808: Portable 6808 emulator ******************************************/
/***                                                                      ***/
/***                                 m6808.c                              ***/
/***                                                                      ***/
/*** This file contains the emulation code                                ***/
/***                                                                      ***/
/*** Copyright (C) DS 1997                                                ***/
/*** Portions Copyright (C) L.C. Benschop 1994,1995                       ***/
/*** Portions Copyright (C) Marcel de Kogel 1996,1997                     ***/
/*** Portions Copyright (C) Nicola Salmoria 1997                          ***/
/***                                                                      ***/
/***     You are not allowed to distribute this software commercially     ***/
/***                                                                      ***/
/*** Where DS has modified L.C. Benschop's original source,               ***/
/*** the initials DS will be indicated with the following exceptions:     ***/
/*** - all references to mem[] array have been changed to M_RDMEM(a)      ***/
/*** - instruction processing has been removed from main function and     ***/
/***   split into 16 separate functions                                   ***/
/****************************************************************************/
/* 6808 Simulator V09.

   Copyright 1994,1995 L.C. Benschop, Eidnhoven The Netherlands.
   This version of the program is distributed under the terms and conditions
   of the GNU General Public License version 2. See the file COPYING.
   THERE IS NO WARRANTY ON THIS PROGRAM!!!

   This program simulates a 6808 processor.

   System dependencies: short must be 16 bits.
                        char  must be 8 bits.
                        long must be more than 16 bits.
                        arrays up to 65536 bytes must be supported.
                        machine must be twos complement.
   Most Unix machines will work. For MSODS you need long pointers
   and you may have to malloc() the mem array of 65536 bytes.

   Special instructions:
   SWI2 writes char to stdout from register B.
   SWI3 reads char from stdout to register B, sets carry at EOF.
               (or when no key available when using term control).
   SWI retains its normal function.
   CWAI and SYNC stop simulator.
   Note: special instructions are gone for now.

   ACIA emulation at port $E000

   Note: BIG_ENDIAN option is no longer needed.
*/


#include <stdio.h>
#include <stdlib.h> /* DS */
#include <osd_dbg.h>

#include "m6808.h" /* DS */

INLINE void fetch_effective_address( void );
INLINE void codes_XX( void );


/* 6808 registers */
static byte iccreg;	        /* DS */
static word ixreg,isreg,ipcreg;	/* DS */

static byte iareg,ibreg;	/* DS */

static int pending_interrupts;	/* MB */

static word eaddr; /* effective address    */	/* DS */
static byte ireg;  /* instruction register */   /* DS */
static byte iflag; /* flag to indicate $10 or $11 prebyte */	/* DS */

/* DS -- PUBLIC GLOBALS -- */
/* int	m6808_IPeriod=50000; */   /* MB */
int	m6808_ICount=50000;
/* int	m6808_IRequest=INT_NONE; */  /* MB */

/* DS -- THESE ARE RE-DEFINED IN m6808.h TO RAM, ROM or FUNCTIONS IN cpuintrf.c */
#define M_RDMEM(A)      M6808_RDMEM(A)
#define M_WRMEM(A,V)    M6808_WRMEM(A,V)
#define M_RDOP(A)       M6808_RDOP(A)
#define M_RDOP_ARG(A)   M6808_RDOP_ARG(A)


/* DS ... */
#define GETWORD(a) M_RDMEM_WORD(a);
#define SETBYTE(a,n) M_WRMEM(a,n);
/* Two bytes of a word are fetched separately because of
   the possible wrap-around at address $ffff and alignment
*/
#define SETWORD(a,n) M_WRMEM_WORD(a,n);

#define IMMBYTE(b) {b=M_RDMEM(ipcreg);ipcreg++;}
#define IMMWORD(w) {w=M_RDMEM_WORD(ipcreg);ipcreg+=2;}

#define PUSHBYTE(b) {--isreg;M_WRMEM(isreg,b);}
#define PUSHWORD(w) {isreg-=2;M_WRMEM_WORD(isreg,w);}
#define PULLBYTE(b) {b=M_RDMEM(isreg);isreg++;}
#define PULLWORD(w) {w=M_RDMEM_WORD(isreg);isreg+=2;}
/* ...DS */

#define SIGNED(b) ((word)(b&0x80?b|0xff00:b))

#define GETDREG ((iareg<<8)|ibreg)
#define SETDREG(v) (iareg=(v>>8),ibreg=(v&0xff))

/* Macros for addressing modes (postbytes have their own code) */
#define DIRECT {IMMBYTE(eaddr)}
#define IMM8 {eaddr=ipcreg++;}
#define IMM16 {eaddr=ipcreg;ipcreg+=2;}
#define INDEXED {fetch_effective_address();}
#define EXTENDED {IMMWORD(eaddr)}

/* macros to set status flags */
#define SEC iccreg|=0x01;
#define CLC iccreg&=0xfe;
#define SEZ iccreg|=0x04;
#define CLZ iccreg&=0xfb;
#define SEN iccreg|=0x08;
#define CLN iccreg&=0xf7;
#define SEV iccreg|=0x02;
#define CLV iccreg&=0xfd;
#define SEH iccreg|=0x20;
#define CLH iccreg&=0xdf;
#define STI iccreg|=0x10;
#define CLI iccreg&=~0x10;

/* set N and Z flags depending on 8 or 16 bit result */
#define SETZ(b) {if(b)CLZ else SEZ}
#define SETNZ8(b) {if(b)CLZ else SEZ if(b&0x80)SEN else CLN}
#define SETNZ16(b) {if(b)CLZ else SEZ if(b&0x8000)SEN else CLN}

#define SETSTATUS(a,b,res) if((a^b^res)&0x10) SEH else CLH \
                           if((a^b^res^(res>>1))&0x80)SEV else CLV \
                           if(res&0x100)SEC else CLC SETNZ8((byte)res)

#define SETSTATUSD(a,b,res) {if(res&0x10000) SEC else CLC \
                            if(((res>>1)^a^b^res)&0x8000) SEV else CLV \
                            SETNZ16((word)res)}

/* Macros for branch instructions */
#define BRANCH(f) if(!iflag){IMMBYTE(tb) if(f){ipcreg+=SIGNED(tb);if(tb==0xfe)m6808_ICount=0;}}\
                     else{IMMWORD(tw) if(f)ipcreg+=tw; m6808_ICount-=2;}
#define NXORV  ((iccreg&0x08)^((iccreg&0x02)<<2))

/* Macros for load and store of accumulators. Can be modified to check
   for port addresses */
#define LOADAC(reg) reg=M_RDMEM(eaddr);		/* DS */
#define STOREAC(reg) M_WRMEM(eaddr,reg);	/* DS */

#define LOADREGS	/* DS */
#define SAVEREGS	/* DS */


   /* what they say it is ... */
static unsigned char cycles1[] =
{
 0, 2, 0, 0, 0, 0, 2, 2, 4, 4, 2, 2, 2, 2, 2, 2,
 2, 2, 0, 0, 0, 0, 2, 2, 0, 2, 0, 2, 0, 0, 0, 0,
 4, 0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
 4, 4, 4, 4, 4, 4, 4, 4, 0, 5, 0,10, 0, 0, 9,12,
 2, 0, 0, 2, 2, 0, 2, 2, 2, 2, 2, 0, 2, 2, 0, 2,
 2, 0, 0, 2, 2, 0, 2, 2, 2, 2, 2, 0, 2, 2, 0, 2,
 7, 0, 0, 7, 7, 0, 7, 7, 7, 7, 7, 0, 7, 7, 4, 7,
 6, 0, 0, 6, 6, 0, 6, 6, 6, 6, 6, 0, 6, 6, 3, 6,
 2, 2, 2, 0, 2, 2, 2, 0, 2, 2, 2, 2, 3, 8, 3, 0,
 3, 3, 3, 0, 3, 3, 3, 4, 3, 3, 3, 3, 4, 0, 4, 5,
 5, 5, 5, 0, 5, 5, 5, 6, 5, 5, 5, 5, 6, 8, 6, 7,
 4, 4, 4, 0, 4, 4, 4, 5, 4, 4, 4, 4, 5, 9, 5, 6,
 2, 2, 2, 0, 2, 2, 2, 0, 2, 2, 2, 2, 0, 0, 3, 0,
 3, 3, 3, 0, 3, 3, 3, 4, 3, 3, 3, 3, 0, 0, 4, 5,
 5, 5, 5, 0, 5, 5, 5, 6, 5, 5, 5, 5, 4, 0, 6, 7,
 4, 4, 4, 0, 4, 4, 4, 5, 4, 4, 4, 4, 0, 0, 5, 6,
};


/* DS -- modified from Z80.c */
INLINE unsigned M_RDMEM_WORD (dword A)
{
 int i;

 i = M_RDMEM(A)<<8;
 i |= M_RDMEM(((A)+1)&0xFFFF);
 return i;
}

/* DS */
INLINE void M_WRMEM_WORD (dword A,word V)
{
 M_WRMEM (A,V>>8);
 M_WRMEM (((A)+1)&0xFFFF,V&255);
}

/* DS */
/* Get next opcode argument and increment program counter */
INLINE unsigned M_RDMEM_OPCODE (void)
{
 unsigned retval;
 retval=M_RDOP_ARG(ipcreg);
 ipcreg++;
 return retval;
}

/* DS -- modified from Z80.c */
INLINE unsigned M_RDMEM_OPCODE_WORD (void)
{
 int i;

 i = M_RDMEM_OPCODE()<<8;
 i |= M_RDMEM_OPCODE();
 return i;
}

/* DS */
/****************************************************************************/
/* Set all registers to given values                                        */
/****************************************************************************/
void m6808_SetRegs(m6808_Regs *Regs)
{
	ipcreg = Regs->pc;
	isreg = Regs->s;
	ixreg = Regs->x;
	iareg = Regs->a;
	ibreg = Regs->b;
	iccreg = Regs->cc;

	pending_interrupts = Regs->pending_interrupts;	/* MB */
}

/* DS */
/****************************************************************************/
/* Get all registers in given buffer                                        */
/****************************************************************************/
void m6808_GetRegs(m6808_Regs *Regs)
{
	Regs->pc = ipcreg;
	Regs->s = isreg;
	Regs->x = ixreg;
	Regs->a = iareg;
	Regs->b = ibreg;
	Regs->cc = iccreg;

	Regs->pending_interrupts = pending_interrupts;	/* MB */
}

/* DS */
/****************************************************************************/
/* Return program counter                                                   */
/****************************************************************************/
unsigned m6808_GetPC(void)
{
	return ipcreg;
}

extern FILE *errorlog;

/* DS */
static void Interrupt()
{
	if ((pending_interrupts & M6808_INT_NMI) != 0)
	{
		pending_interrupts &= ~M6808_INT_NMI;

		/* NMI */
		PUSHWORD(ipcreg);
		PUSHWORD(ixreg);
		PUSHBYTE(ibreg);
		PUSHBYTE(iareg);
		PUSHBYTE(iccreg);
		iccreg|=0xd0;
		ipcreg=M_RDMEM_WORD(0xfffc);
		m6808_ICount -= 19;
	}
	else if ((pending_interrupts & M6808_INT_IRQ) != 0 && (iccreg & 0x10) == 0)
	{
		pending_interrupts &= ~M6808_INT_IRQ;

		/* standard IRQ */
		PUSHWORD(ipcreg)
		PUSHWORD(ixreg)
		PUSHBYTE(ibreg)
		PUSHBYTE(iareg)
		PUSHBYTE(iccreg)
		iccreg|=0x90;
		ipcreg=GETWORD(0xfff8);
		m6808_ICount -= 19;
	}
}

/* DS */
void m6808_reset(void)
{
	ipcreg = M_RDMEM_WORD(0xfffe);

	iccreg = 0x00;		/* Clear all flags */
	iccreg |= 0x10;		/* IRQ disabled */
	iccreg |= 0x40;		/* FIRQ disabled */
	iareg = 0x00;		/* clear accumulator a */
	ibreg = 0x00;		/* clear accumulator b */
/*	m6808_ICount=m6808_IPeriod; */   /* MB */
/*	m6808_IRequest=INT_NONE;    */   /* MB */
}

void m6808_Cause_Interrupt(int type)	/* MB */
{
	pending_interrupts |= type;
}
void m6808_Clear_Pending_Interrupts(void)	/* MB */
{
	pending_interrupts = 0;
}


int m6808_execute(int cycles) /* MB */
{
	LOADREGS;

	m6808_ICount = cycles;	/* MB */

	do
	{
#ifdef	MAME_DEBUG
{
  extern int mame_debug;
  if (mame_debug) MAME_Debug();
}
#endif
		if (pending_interrupts != 0) Interrupt();	/* MB */

		ireg=M_RDOP(ipcreg++);
		m6808_ICount -= cycles1[ireg&0xFF];
		codes_XX();

        }
	while (m6808_ICount > 0);

	return cycles - m6808_ICount;	/* MB */
#if 0
	{
		byte I;

		m6808_IRequest = INT_NONE;
        I=cpu_interrupt();        /* Call the periodic handler */

		if(I==INT_IRQ)
		{
			m6808_Interrupt(); /* Interrupt if needed  */
		}
        }
#endif
}

/* DS */
INLINE void fetch_effective_address( void )
{
		byte postbyte=M_RDOP(ipcreg++);
		eaddr = ixreg+postbyte;
}

INLINE void codes_XX( void )
{
	int res;
	word tw;
	byte tb;

	switch( ireg )
	{
		case 0x04: /*LSRD - 6803 only*/  tw=GETDREG;if(tw&0x0001)SEC else CLC
		                         tw>>=1;SETNZ16(tw)
		                         SETDREG(tw);break;
		case 0x05: /*ASLD - 6803 only*/  res=GETDREG<<1;
		                         SETSTATUSD(GETDREG,GETDREG,res)
		                         SETDREG(res);break;
		case 0x06: /* TAP */ iccreg=iareg; break;
		case 0x07: /* TPA */ iareg=iccreg; break;
		case 0x08: /* INX */ ixreg++; SETZ(ixreg) break;
		case 0x09: /* DEX */ ixreg--; SETZ(ixreg) break;
		case 0x0A: /* CLV */ CLV break;
		case 0x0B: /* SEV */ SEV break;
		case 0x0C: /* CLC */ CLC break;
		case 0x0D: /* SEC */ SEC break;
		case 0x0E: /* CLI */ CLI break;
		case 0x0F: /* STI */ STI break;
		case 0x10: /* SBA */  tw=iareg-ibreg;
					 SETSTATUS(iareg,ibreg,tw)
					 iareg=tw; break;
		case 0x11: /* CBA */  tw=iareg-ibreg;
					 SETSTATUS(iareg,ibreg,tw)
					 break;
		case 0x16: /* TAB */ ibreg=iareg; SETNZ8(ibreg) CLV break;
		case 0x17: /* TBA */ iareg=ibreg; SETNZ8(iareg) CLV break;
		case 0x18: break; /*ILLEGAL*/
		case 0x19: /* DAA*/ 	tw=iareg;
				if(iccreg&0x20)tw+=6;
				if((tw&0x0f)>9)tw+=6;
				if(iccreg&0x01)tw+=0x60;
				if((tw&0xf0)>0x90)tw+=0x60;
				if(tw&0x100)SEC
				iareg=tw;break;
		case 0x1B: /* ABA */  tw=iareg+ibreg;
					 SETSTATUS(iareg,ibreg,tw)
					 iareg=tw; break;
		case 0x20: /* (L)BRA*/  BRANCH(1) break;
		case 0x21: /* (L)BRN*/  BRANCH(0) break;
		case 0x22: /* (L)BHI*/  BRANCH(!(iccreg&0x05)) break;
		case 0x23: /* (L)BLS*/  BRANCH(iccreg&0x05) break;
		case 0x24: /* (L)BCC*/  BRANCH(!(iccreg&0x01)) break;
		case 0x25: /* (L)BCS*/  BRANCH(iccreg&0x01) break;
		case 0x26: /* (L)BNE*/  BRANCH(!(iccreg&0x04)) break;
		case 0x27: /* (L)BEQ*/  BRANCH(iccreg&0x04) break;
		case 0x28: /* (L)BVC*/  BRANCH(!(iccreg&0x02)) break;
		case 0x29: /* (L)BVS*/  BRANCH(iccreg&0x02) break;
		case 0x2A: /* (L)BPL*/  BRANCH(!(iccreg&0x08)) break;
		case 0x2B: /* (L)BMI*/  BRANCH(iccreg&0x08) break;
		case 0x2C: /* (L)BGE*/  BRANCH(!NXORV) break;
		case 0x2D: /* (L)BLT*/  BRANCH(NXORV) break;
		case 0x2E: /* (L)BGT*/  BRANCH(!(NXORV||iccreg&0x04)) break;
		case 0x2F: /* (L)BLE*/  BRANCH(NXORV||iccreg&0x04) break;
		case 0x30: /* TSX */ ixreg=isreg; break;
		case 0x31: /* INS */ isreg++; break;
		case 0x32: /* PULA */ PULLBYTE(iareg) break;
		case 0x33: /* PULB */ PULLBYTE(ibreg) break;
		case 0x34: /* DES */ isreg--; break;
		case 0x35: /* TXS */ isreg=ixreg; break;
		case 0x36: /* PSHA */ PUSHBYTE(iareg) break;
		case 0x37: /* PSHB */ PUSHBYTE(ibreg) break;
		case 0x38: /* PULX - 6803 only */ PULLWORD(ixreg) break;
		case 0x39: /* RTS */ PULLWORD(ipcreg) break;
		case 0x3A: /* ABX - 6803 only */ ixreg+=ibreg; break;
		case 0x3B: /* RTI */  tb=iccreg&0x80;
				PULLBYTE(iccreg)
				if(tb)
				{
				 m6808_ICount -= 9;
				 PULLBYTE(iareg)
				 PULLBYTE(ibreg)
				 PULLWORD(ixreg)
				}
				PULLWORD(ipcreg) break;
		case 0x3C: /* PSHX - 6803 only */ PUSHWORD(ixreg) break;
		case 0x3D: /* MUL - 6803 only */
			tw=iareg*ibreg; if (tw&0x80) SEC; SETDREG(tw);
			break;
		case 0x3E:
				/* force IRQ -- inf loop */
				ipcreg--;
				break;
		case 0x3F: /* SWI (SWI2 SWI3)*/ {
				 PUSHWORD(ipcreg)
				 PUSHWORD(ixreg)
				 PUSHBYTE(ibreg)
				 PUSHBYTE(iareg)
				 PUSHBYTE(iccreg)
				 iccreg|=0x80;
				 if(!iflag)iccreg|=0x50;
				 switch(iflag) {
		                      case 0:ipcreg=GETWORD(0xfffa);break;
		                      case 1:ipcreg=GETWORD(0xfff4);break;
		                      case 2:ipcreg=GETWORD(0xfff2);break;
		                     }
			      }break;
		case 0x40: /*NEGA*/  tw=-iareg;SETSTATUS(0,iareg,tw)
		                         iareg=tw;break;
		case 0x41: break;/*ILLEGAL*/
		case 0x42: break;/*ILLEGAL*/
		case 0x43: /*COMA*/   tb=~iareg;SETNZ8(tb);SEC CLV
		                         iareg=tb;break;
		case 0x44: /*LSRA*/  tb=iareg;if(tb&0x01)SEC else CLC
		                         if(tb&0x10)SEH else CLH tb>>=1;SETNZ8(tb)
		                         iareg=tb;break;
		case 0x45: break;/* ILLEGAL*/
		case 0x46: /*RORA*/  tb=(iccreg&0x01)<<7;
		                         if(iareg&0x01)SEC else CLC
		                         iareg=(iareg>>1)+tb;SETNZ8(iareg)
		                   	     break;
		case 0x47: /*ASRA*/  tb=iareg;if(tb&0x01)SEC else CLC
		                         if(tb&0x10)SEH else CLH tb>>=1;
		                         if(tb&0x40)tb|=0x80;iareg=tb;SETNZ8(tb)
		                         break;
		case 0x48: /*ASLA*/  tw=iareg<<1;
		                         SETSTATUS(iareg,iareg,tw)
		                         iareg=tw;break;
		case 0x49: /*ROLA*/  tb=iareg;tw=iccreg&0x01;
		                         if(tb&0x80)SEC else CLC
		                         if((tb&0x80)^((tb<<1)&0x80))SEV else CLV
		                         tb=(tb<<1)+tw;SETNZ8(tb) iareg=tb;break;
		case 0x4A: /*DECA*/  tb=iareg-1;if(tb==0x7F)SEV else CLV
				     SETNZ8(tb) iareg=tb;break;
		case 0x4B: break; /*ILLEGAL*/
		case 0x4C: /*INCA*/  tb=iareg+1;if(tb==0x80)SEV else CLV
				     SETNZ8(tb) iareg=tb;break;
		case 0x4D: /*TSTA*/  SETNZ8(iareg) break;
		case 0x4E: break; /*ILLEGAL*/
		case 0x4F: /*CLRA*/  iareg=0;CLN CLV SEZ CLC break;
		case 0x50: /*NEGB*/  tw=-ibreg;SETSTATUS(0,ibreg,tw)
		                         ibreg=tw;break;
		case 0x51: break;/*ILLEGAL*/
		case 0x52: break;/*ILLEGAL*/
		case 0x53: /*COMB*/   tb=~ibreg;SETNZ8(tb);SEC CLV
		                         ibreg=tb;break;
		case 0x54: /*LSRB*/  tb=ibreg;if(tb&0x01)SEC else CLC
		                         if(tb&0x10)SEH else CLH tb>>=1;SETNZ8(tb)
		                         ibreg=tb;break;
		case 0x55: break;/* ILLEGAL*/
		case 0x56: /*RORB*/  tb=(iccreg&0x01)<<7;
		                         if(ibreg&0x01)SEC else CLC
		                         ibreg=(ibreg>>1)+tb;SETNZ8(ibreg)
		                   	     break;
		case 0x57: /*ASRB*/  tb=ibreg;if(tb&0x01)SEC else CLC
		                         if(tb&0x10)SEH else CLH tb>>=1;
		                         if(tb&0x40)tb|=0x80;ibreg=tb;SETNZ8(tb)
		                         break;
		case 0x58: /*ASLB*/  tw=ibreg<<1;
		                         SETSTATUS(ibreg,ibreg,tw)
		                         ibreg=tw;break;
		case 0x59: /*ROLB*/  tb=ibreg;tw=iccreg&0x01;
		                         if(tb&0x80)SEC else CLC
		                         if((tb&0x80)^((tb<<1)&0x80))SEV else CLV
		                         tb=(tb<<1)+tw;SETNZ8(tb) ibreg=tb;break;
		case 0x5A: /*DECB*/  tb=ibreg-1;if(tb==0x7F)SEV else CLV
				     SETNZ8(tb) ibreg=tb;break;
		case 0x5B: break; /*ILLEGAL*/
		case 0x5C: /*INCB*/  tb=ibreg+1;if(tb==0x80)SEV else CLV
				     SETNZ8(tb) ibreg=tb;break;
		case 0x5D: /*TSTB*/  SETNZ8(ibreg) break;
		case 0x5E: break; /*ILLEGAL*/
		case 0x5F: /*CLRB*/  ibreg=0;CLN CLV SEZ CLC break;
		case 0x60: /*NEG indexed*/  INDEXED tw=-M_RDMEM(eaddr);SETSTATUS(0,M_RDMEM(eaddr),tw)
		                         SETBYTE(eaddr,tw)break;
		case 0x61: break;/*ILLEGAL*/
		case 0x62: break;/*ILLEGAL*/
		case 0x63: /*COM indexed*/   INDEXED tb=~M_RDMEM(eaddr);SETNZ8(tb);SEC CLV
		                         SETBYTE(eaddr,tb)break;
		case 0x64: /*LSR indexed*/  INDEXED tb=M_RDMEM(eaddr);if(tb&0x01)SEC else CLC
		                         if(tb&0x10)SEH else CLH tb>>=1;SETNZ8(tb)
		                         SETBYTE(eaddr,tb)break;
		case 0x65: break;/* ILLEGAL*/
		case 0x66: /*ROR indexed*/  INDEXED tb=(iccreg&0x01)<<7;
		                         if(M_RDMEM(eaddr)&0x01)SEC else CLC
		                         tw=(M_RDMEM(eaddr)>>1)+tb;SETNZ8(tw)
		                         SETBYTE(eaddr,tw)
		                   	     break;
		case 0x67: /*ASR indexed*/  INDEXED tb=M_RDMEM(eaddr);if(tb&0x01)SEC else CLC
		                         if(tb&0x10)SEH else CLH tb>>=1;
		                         if(tb&0x40)tb|=0x80;SETBYTE(eaddr,tb)SETNZ8(tb)
		                         break;
		case 0x68: /*ASL indexed*/  INDEXED tw=M_RDMEM(eaddr)<<1;
		                         SETSTATUS(M_RDMEM(eaddr),M_RDMEM(eaddr),tw)
		                         SETBYTE(eaddr,tw)break;
		case 0x69: /*ROL indexed*/  INDEXED tb=M_RDMEM(eaddr);tw=iccreg&0x01;
		                         if(tb&0x80)SEC else CLC
		                         if((tb&0x80)^((tb<<1)&0x80))SEV else CLV
		                         tb=(tb<<1)+tw;SETNZ8(tb) SETBYTE(eaddr,tb)break;
		case 0x6A: /*DEC indexed*/  INDEXED tb=M_RDMEM(eaddr)-1;if(tb==0x7F)SEV else CLV
				     SETNZ8(tb) SETBYTE(eaddr,tb)break;
		case 0x6B: break; /*ILLEGAL*/
		case 0x6C: /*INC indexed*/  INDEXED tb=M_RDMEM(eaddr)+1;if(tb==0x80)SEV else CLV
				     SETNZ8(tb) SETBYTE(eaddr,tb)break;
		case 0x6D: /*TST indexed*/  INDEXED tb=M_RDMEM(eaddr);SETNZ8(tb) break;
		case 0x6E: /*JMP indexed*/  INDEXED ipcreg=eaddr;break;
		case 0x6F: /*CLR indexed*/  INDEXED SETBYTE(eaddr,0)CLN CLV SEZ CLC break;
		case 0x70: /*NEG ext*/ EXTENDED tw=-M_RDMEM(eaddr);SETSTATUS(0,M_RDMEM(eaddr),tw)
		                         SETBYTE(eaddr,tw)break;
		case 0x71: break;/*ILLEGAL*/
		case 0x72: break;/*ILLEGAL*/
		case 0x73: /*COM ext*/ EXTENDED  tb=~M_RDMEM(eaddr);SETNZ8(tb);SEC CLV
		                        SETBYTE(eaddr,tb)break;
		case 0x74: /*LSR ext*/ EXTENDED tb=M_RDMEM(eaddr);if(tb&0x01)SEC else CLC
		                         if(tb&0x10)SEH else CLH tb>>=1;SETNZ8(tb)
		                         SETBYTE(eaddr,tb)break;
		case 0x75: break;/* ILLEGAL*/
		case 0x76: /*ROR ext*/ EXTENDED tb=(iccreg&0x01)<<7;
		                         if(M_RDMEM(eaddr)&0x01)SEC else CLC
		                         tw=(M_RDMEM(eaddr)>>1)+tb;SETNZ8(tw)
		                         SETBYTE(eaddr,tw)
		                   	     break;
		case 0x77: /*ASR ext*/ EXTENDED tb=M_RDMEM(eaddr);if(tb&0x01)SEC else CLC
		                         if(tb&0x10)SEH else CLH tb>>=1;
		                         if(tb&0x40)tb|=0x80;SETBYTE(eaddr,tb)SETNZ8(tb)
		                         break;
		case 0x78: /*ASL ext*/ EXTENDED tw=M_RDMEM(eaddr)<<1;
		                         SETSTATUS(M_RDMEM(eaddr),M_RDMEM(eaddr),tw)
		                         SETBYTE(eaddr,tw)break;
		case 0x79: /*ROL ext*/ EXTENDED tb=M_RDMEM(eaddr);tw=iccreg&0x01;
		                         if(tb&0x80)SEC else CLC
		                         if((tb&0x80)^((tb<<1)&0x80))SEV else CLV
		                         tb=(tb<<1)+tw;SETNZ8(tb) SETBYTE(eaddr,tb)break;
		case 0x7A: /*DEC ext*/ EXTENDED tb=M_RDMEM(eaddr)-1;if(tb==0x7F)SEV else CLV
				     SETNZ8(tb) SETBYTE(eaddr,tb)break;
		case 0x7B: break; /*ILLEGAL*/
		case 0x7C: /*INC ext*/ EXTENDED tb=M_RDMEM(eaddr)+1;if(tb==0x80)SEV else CLV
				     SETNZ8(tb) SETBYTE(eaddr,tb)break;
		case 0x7D: /*TST ext*/ EXTENDED tb=M_RDMEM(eaddr);SETNZ8(tb) break;
		case 0x7E: /*JMP ext*/ EXTENDED ipcreg=eaddr;break;
		case 0x7F: /*CLR ext*/ EXTENDED SETBYTE(eaddr,0)CLN CLV SEZ CLC break;
		case 0x80: /*SUBA immediate*/ IMM8 tw=iareg-M_RDMEM(eaddr);
		                             SETSTATUS(iareg,M_RDMEM(eaddr),tw)
		                             iareg=tw;break;
		case 0x81: /*CMPA immediate*/ IMM8 tw=iareg-M_RDMEM(eaddr);
					 SETSTATUS(iareg,M_RDMEM(eaddr),tw) break;
		case 0x82: /*SBCA immediate*/ IMM8 tw=iareg-M_RDMEM(eaddr)-(iccreg&0x01);
					 SETSTATUS(iareg,M_RDMEM(eaddr),tw)
					 iareg=tw;break;
		case 0x83: /*SUBD immediate - 6803 only */ IMM16 tw=GETWORD(eaddr); res=GETDREG-tw;
		          SETSTATUSD(GETDREG,tw,res)
		          SETDREG(res);break;
		case 0x84: /*ANDA immediate*/ IMM8 iareg=iareg&M_RDMEM(eaddr);SETNZ8(iareg)
					 CLV break;
		case 0x85: /*BITA immediate*/ IMM8 tb=iareg&M_RDMEM(eaddr);SETNZ8(tb)
					 CLV break;
		case 0x86: /*LDA immediate*/ IMM8 LOADAC(iareg) CLV SETNZ8(iareg)
		                             break;
		case 0x87: /*STA immediate (for the sake of orthogonality) */ IMM8
		                             SETNZ8(iareg) CLV STOREAC(iareg) break;
		case 0x88: /*EORA immediate*/ IMM8 iareg=iareg^M_RDMEM(eaddr);SETNZ8(iareg)
					 CLV break;
		case 0x89: /*ADCA immediate*/ IMM8 tw=iareg+M_RDMEM(eaddr)+(iccreg&0x01);
		                             SETSTATUS(iareg,M_RDMEM(eaddr),tw)
		                             iareg=tw;break;
		case 0x8A: /*ORA immediate*/  IMM8 iareg=iareg|M_RDMEM(eaddr);SETNZ8(iareg)
					 CLV break;
		case 0x8B: /*ADDA immediate*/ IMM8 tw=iareg+M_RDMEM(eaddr);
					 SETSTATUS(iareg,M_RDMEM(eaddr),tw)
					 iareg=tw;break;
		case 0x8C: /*CMPX (CMPY CMPS) immediate */ IMM16
		                             {unsigned long dreg,breg,res;
					 dreg=ixreg;
					 breg=GETWORD(eaddr);
					 res=dreg-breg;
					 SETSTATUSD(dreg,breg,res)
					 }break;
		case 0x8D: /*BSR */   IMMBYTE(tb) PUSHWORD(ipcreg) ipcreg+=SIGNED(tb);
		                     break;
		case 0x8E: /* LDS immediate */ IMM16 tw=GETWORD(eaddr);
		                              CLV SETNZ16(tw) isreg=tw; break;
		case 0x8F:  /* SS immediate (orthogonality) */ IMM16
		                              tw=isreg;
		                              CLV SETNZ16(tw) SETWORD(eaddr,tw) break;
		case 0x90: /*SUBA direct*/ DIRECT tw=iareg-M_RDMEM(eaddr);
		                             SETSTATUS(iareg,M_RDMEM(eaddr),tw)
		                             iareg=tw;break;
		case 0x91: /*CMPA direct*/ DIRECT tw=iareg-M_RDMEM(eaddr);
					 SETSTATUS(iareg,M_RDMEM(eaddr),tw) break;
		case 0x92: /*SBCA direct*/ DIRECT tw=iareg-M_RDMEM(eaddr)-(iccreg&0x01);
					 SETSTATUS(iareg,M_RDMEM(eaddr),tw)
					 iareg=tw;break;
		case 0x93: /*SUBD direct - 6803 only */ DIRECT tw=M_RDMEM_WORD(eaddr); res=GETDREG-tw;
		          SETSTATUSD(GETDREG,tw,res)
		          SETDREG(res);break;
		case 0x94: /*ANDA direct*/ DIRECT iareg=iareg&M_RDMEM(eaddr);SETNZ8(iareg)
					 CLV break;
		case 0x95: /*BITA direct*/ DIRECT tb=iareg&M_RDMEM(eaddr);SETNZ8(tb)
					 CLV break;
		case 0x96: /*LDA direct*/ DIRECT LOADAC(iareg) CLV SETNZ8(iareg)
		                             break;
		case 0x97: /*STA direct */ DIRECT
		                             SETNZ8(iareg) CLV STOREAC(iareg) break;
		case 0x98: /*EORA direct*/ DIRECT iareg=iareg^M_RDMEM(eaddr);SETNZ8(iareg)
					 CLV break;
		case 0x99: /*ADCA direct*/ DIRECT tw=iareg+M_RDMEM(eaddr)+(iccreg&0x01);
		                             SETSTATUS(iareg,M_RDMEM(eaddr),tw)
		                             iareg=tw;break;
		case 0x9A: /*ORA direct*/  DIRECT iareg=iareg|M_RDMEM(eaddr);SETNZ8(iareg)
					 CLV break;
		case 0x9B: /*ADDA direct*/ DIRECT tw=iareg+M_RDMEM(eaddr);
					 SETSTATUS(iareg,M_RDMEM(eaddr),tw)
					 iareg=tw;break;
		case 0x9C: /*CMPX (CMPY CMPS) direct */ DIRECT
		                             {unsigned long dreg,breg,res;
					 dreg=ixreg;
					 breg=GETWORD(eaddr);
					 res=dreg-breg;
					 SETSTATUSD(dreg,breg,res)
					 }break;
		case 0x9D: /*JSR direct */  DIRECT  PUSHWORD(ipcreg) ipcreg=eaddr;
		                     break;
		case 0x9E: /* LDS direct */ DIRECT tw=GETWORD(eaddr);
		                              CLV SETNZ16(tw) isreg=tw; break;
		case 0x9F:  /* STS direct */ DIRECT
		                              tw=isreg;
		                              CLV SETNZ16(tw) SETWORD(eaddr,tw) break;
		case 0xA0: /*SUBA indexed*/  INDEXED tw=iareg-M_RDMEM(eaddr);
		                             SETSTATUS(iareg,M_RDMEM(eaddr),tw)
		                             iareg=tw;break;
		case 0xA1: /*CMPA indexed*/  INDEXED tw=iareg-M_RDMEM(eaddr);
					 SETSTATUS(iareg,M_RDMEM(eaddr),tw) break;
		case 0xA2: /*SBCA indexed*/  INDEXED tw=iareg-M_RDMEM(eaddr)-(iccreg&0x01);
					 SETSTATUS(iareg,M_RDMEM(eaddr),tw)
					 iareg=tw;break;
		case 0xA3: /*SUBD indexed - 6803 only */ INDEXED tw=M_RDMEM_WORD(eaddr); res=GETDREG-tw;
		          SETSTATUSD(GETDREG,tw,res)
		          SETDREG(res);break;
		case 0xA4: /*ANDA indexed*/  INDEXED iareg=iareg&M_RDMEM(eaddr);SETNZ8(iareg)
					 CLV break;
		case 0xA5: /*BITA indexed*/  INDEXED tb=iareg&M_RDMEM(eaddr);SETNZ8(tb)
					 CLV break;
		case 0xA6: /*LDA indexed*/  INDEXED LOADAC(iareg) CLV SETNZ8(iareg)
		                             break;
		case 0xA7: /*STA indexed */ INDEXED
		                             SETNZ8(iareg) CLV STOREAC(iareg) break;
		case 0xA8: /*EORA indexed*/  INDEXED iareg=iareg^M_RDMEM(eaddr);SETNZ8(iareg)
					 CLV break;
		case 0xA9: /*ADCA indexed*/  INDEXED tw=iareg+M_RDMEM(eaddr)+(iccreg&0x01);
		                             SETSTATUS(iareg,M_RDMEM(eaddr),tw)
		                             iareg=tw;break;
		case 0xAA: /*ORA indexed*/   INDEXED iareg=iareg|M_RDMEM(eaddr);SETNZ8(iareg)
					 CLV break;
		case 0xAB: /*ADDA indexed*/  INDEXED tw=iareg+M_RDMEM(eaddr);
					 SETSTATUS(iareg,M_RDMEM(eaddr),tw)
					 iareg=tw;break;
		case 0xAC: /*CMPX (CMPY CMPS) indexed */  INDEXED
		                             {unsigned long dreg,breg,res;
					 dreg=ixreg;
					 breg=GETWORD(eaddr);
					 res=dreg-breg;
					 SETSTATUSD(dreg,breg,res)
					 }break;
		case 0xAD: /*JSR indexed */    INDEXED PUSHWORD(ipcreg) ipcreg=eaddr;
		                     break;
		case 0xAE: /* LDS indexed */  INDEXED tw=GETWORD(eaddr);
		                              CLV SETNZ16(tw) isreg=tw; break;
		case 0xAF:  /* STS indexed */ INDEXED
		                              tw=isreg;
		                              CLV SETNZ16(tw) SETWORD(eaddr,tw) break;
		case 0xB0: /*SUBA ext*/ EXTENDED tw=iareg-M_RDMEM(eaddr);
		                             SETSTATUS(iareg,M_RDMEM(eaddr),tw)
		                             iareg=tw;break;
		case 0xB1: /*CMPA ext*/ EXTENDED tw=iareg-M_RDMEM(eaddr);
					 SETSTATUS(iareg,M_RDMEM(eaddr),tw) break;
		case 0xB2: /*SBCA ext*/ EXTENDED tw=iareg-M_RDMEM(eaddr)-(iccreg&0x01);
					 SETSTATUS(iareg,M_RDMEM(eaddr),tw)
					 iareg=tw;break;
		case 0xB3: /*SUBD ext - 6803 only */ EXTENDED tw=M_RDMEM_WORD(eaddr); res=GETDREG-tw;
		          SETSTATUSD(GETDREG,tw,res)
		          SETDREG(res);break;
		case 0xB4: /*ANDA ext*/ EXTENDED iareg=iareg&M_RDMEM(eaddr);SETNZ8(iareg)
					 CLV break;
		case 0xB5: /*BITA ext*/ EXTENDED tb=iareg&M_RDMEM(eaddr);SETNZ8(tb)
					 CLV break;
		case 0xB6: /*LDA ext*/ EXTENDED LOADAC(iareg) CLV SETNZ8(iareg)
		                             break;
		case 0xB7: /*STA ext */ EXTENDED
		                             SETNZ8(iareg) CLV STOREAC(iareg) break;
		case 0xB8: /*EORA ext*/ EXTENDED iareg=iareg^M_RDMEM(eaddr);SETNZ8(iareg)
					 CLV break;
		case 0xB9: /*ADCA ext*/ EXTENDED tw=iareg+M_RDMEM(eaddr)+(iccreg&0x01);
		                             SETSTATUS(iareg,M_RDMEM(eaddr),tw)
		                             iareg=tw;break;
		case 0xBA: /*ORA ext*/  EXTENDED iareg=iareg|M_RDMEM(eaddr);SETNZ8(iareg)
					 CLV break;
		case 0xBB: /*ADDA ext*/ EXTENDED tw=iareg+M_RDMEM(eaddr);
					 SETSTATUS(iareg,M_RDMEM(eaddr),tw)
					 iareg=tw;break;
		case 0xBC: /*CMPX (CMPY CMPS) ext */ EXTENDED
		                             {unsigned long dreg,breg,res;
					 dreg=ixreg;
					 breg=GETWORD(eaddr);
					 res=dreg-breg;
					 SETSTATUSD(dreg,breg,res)
					 }break;
		case 0xBD: /*JSR ext */  EXTENDED  PUSHWORD(ipcreg) ipcreg=eaddr;
		                     break;
		case 0xBE: /* LDS ext */ EXTENDED tw=GETWORD(eaddr);
		                              CLV SETNZ16(tw) isreg=tw; break;
		case 0xBF:  /* STS ext */ EXTENDED
		                              tw=isreg;
		                              CLV SETNZ16(tw) SETWORD(eaddr,tw) break;
		case 0xC0: /*SUBB immediate*/ IMM8 tw=ibreg-M_RDMEM(eaddr);
		                             SETSTATUS(ibreg,M_RDMEM(eaddr),tw)
		                             ibreg=tw;break;
		case 0xC1: /*CMPB immediate*/ IMM8 tw=ibreg-M_RDMEM(eaddr);
					 SETSTATUS(ibreg,M_RDMEM(eaddr),tw) break;
		case 0xC2: /*SBCB immediate*/ IMM8 tw=ibreg-M_RDMEM(eaddr)-(iccreg&0x01);
					 SETSTATUS(ibreg,M_RDMEM(eaddr),tw)
					 ibreg=tw;break;
		case 0xC3: /*ADDD immediate - 6803 only */ IMM16 tw=GETWORD(eaddr); res=GETDREG+tw;
		          SETSTATUSD(GETDREG,tw,res)
		          SETDREG(res);break;
		case 0xC4: /*ANDB immediate*/ IMM8 ibreg=ibreg&M_RDMEM(eaddr);SETNZ8(ibreg)
					 CLV break;
		case 0xC5: /*BITB immediate*/ IMM8 tb=ibreg&M_RDMEM(eaddr);SETNZ8(tb)
					 CLV break;
		case 0xC6: /*LDB immediate*/ IMM8 LOADAC(ibreg) CLV SETNZ8(ibreg)
		                             break;
		case 0xC7: /*STB immediate (for the sake of orthogonality) */ IMM8
		                             SETNZ8(ibreg) CLV STOREAC(ibreg) break;
		case 0xC8: /*EORB immediate*/ IMM8 ibreg=ibreg^M_RDMEM(eaddr);SETNZ8(ibreg)
					 CLV break;
		case 0xC9: /*ADCB immediate*/ IMM8 tw=ibreg+M_RDMEM(eaddr)+(iccreg&0x01);
		                             SETSTATUS(ibreg,M_RDMEM(eaddr),tw)
		                             ibreg=tw;break;
		case 0xCA: /*ORB immediate*/  IMM8 ibreg=ibreg|M_RDMEM(eaddr);SETNZ8(ibreg)
					 CLV break;
		case 0xCB: /*ADDB immediate*/ IMM8 tw=ibreg+M_RDMEM(eaddr);
					 SETSTATUS(ibreg,M_RDMEM(eaddr),tw)
					 ibreg=tw;break;
		case 0xCC: /* LDD immediate */ IMM16 tw=GETWORD(eaddr);
		                              CLV SETNZ16(tw) SETDREG(tw); break;
		case 0xCD:  /* STD immediate (orthogonality) */ IMM16
		                              tw=GETDREG;
		                              CLV SETNZ16(tw) SETWORD(eaddr,tw) break;
		case 0xCE: /* LDX immediate */ IMM16 tw=GETWORD(eaddr);
		                              CLV SETNZ16(tw) ixreg=tw; break;
		case 0xCF:  /* STX immediate (orthogonality) */ IMM16
		                              tw=ixreg;
		                              CLV SETNZ16(tw) SETWORD(eaddr,tw) break;
		case 0xD0: /*SUBB direct*/ DIRECT tw=ibreg-M_RDMEM(eaddr);
		                             SETSTATUS(ibreg,M_RDMEM(eaddr),tw)
		                             ibreg=tw;break;
		case 0xD1: /*CMPB direct*/ DIRECT tw=ibreg-M_RDMEM(eaddr);
					 SETSTATUS(ibreg,M_RDMEM(eaddr),tw) break;
		case 0xD2: /*SBCB direct*/ DIRECT tw=ibreg-M_RDMEM(eaddr)-(iccreg&0x01);
					 SETSTATUS(ibreg,M_RDMEM(eaddr),tw)
					 ibreg=tw;break;
		case 0xD3: /*ADDD direct - 6803 only */ DIRECT tw=M_RDMEM_WORD(eaddr); res=GETDREG+tw;
		          SETSTATUSD(GETDREG,tw,res)
		          SETDREG(res);break;
		case 0xD4: /*ANDB direct*/ DIRECT ibreg=ibreg&M_RDMEM(eaddr);SETNZ8(ibreg)
					 CLV break;
		case 0xD5: /*BITB direct*/ DIRECT tb=ibreg&M_RDMEM(eaddr);SETNZ8(tb)
					 CLV break;
		case 0xD6: /*LDB direct*/ DIRECT LOADAC(ibreg) CLV SETNZ8(ibreg)
		                             break;
		case 0xD7: /*STB direct  */ DIRECT
		                             SETNZ8(ibreg) CLV STOREAC(ibreg) break;
		case 0xD8: /*EORB direct*/ DIRECT ibreg=ibreg^M_RDMEM(eaddr);SETNZ8(ibreg)
					 CLV break;
		case 0xD9: /*ADCB direct*/ DIRECT tw=ibreg+M_RDMEM(eaddr)+(iccreg&0x01);
		                             SETSTATUS(ibreg,M_RDMEM(eaddr),tw)
		                             ibreg=tw;break;
		case 0xDA: /*ORB direct*/  DIRECT ibreg=ibreg|M_RDMEM(eaddr);SETNZ8(ibreg)
					 CLV break;
		case 0xDB: /*ADDB direct*/ DIRECT tw=ibreg+M_RDMEM(eaddr);
					 SETSTATUS(ibreg,M_RDMEM(eaddr),tw)
					 ibreg=tw;break;
		case 0xDC: /* LDD direct - 6803 only */ DIRECT tw=GETWORD(eaddr);
		                              CLV SETNZ16(tw) SETDREG(tw); break;
		case 0xDD:  /* STD direct - 6803 only */ DIRECT
		                              tw=GETDREG;
		                              CLV SETNZ16(tw) SETWORD(eaddr,tw) break;
		case 0xDE: /* LDX direct */ DIRECT tw=GETWORD(eaddr);
		                              CLV SETNZ16(tw) ixreg=tw; break;
		case 0xDF:  /* STX direct  */ DIRECT
		                              tw=ixreg;
		                              CLV SETNZ16(tw) SETWORD(eaddr,tw) break;
		case 0xE0: /*SUBB indexed*/  INDEXED tw=ibreg-M_RDMEM(eaddr);
		                             SETSTATUS(ibreg,M_RDMEM(eaddr),tw)
		                             ibreg=tw;break;
		case 0xE1: /*CMPB indexed*/  INDEXED tw=ibreg-M_RDMEM(eaddr);
					 SETSTATUS(ibreg,M_RDMEM(eaddr),tw) break;
		case 0xE2: /*SBCB indexed*/  INDEXED tw=ibreg-M_RDMEM(eaddr)-(iccreg&0x01);
					 SETSTATUS(ibreg,M_RDMEM(eaddr),tw)
					 ibreg=tw;break;
		case 0xE3: /*ADDD indexed - 6803 only */ INDEXED tw=M_RDMEM_WORD(eaddr); res=GETDREG+tw;
		          SETSTATUSD(GETDREG,tw,res)
		          SETDREG(res);break;
		case 0xE4: /*ANDB indexed*/  INDEXED ibreg=ibreg&M_RDMEM(eaddr);SETNZ8(ibreg)
					 CLV break;
		case 0xE5: /*BITB indexed*/  INDEXED tb=ibreg&M_RDMEM(eaddr);SETNZ8(tb)
					 CLV break;
		case 0xE6: /*LDB indexed*/  INDEXED LOADAC(ibreg) CLV SETNZ8(ibreg)
		                             break;
		case 0xE7: /*STB indexed  */ INDEXED
		                             SETNZ8(ibreg) CLV STOREAC(ibreg) break;
		case 0xE8: /*EORB indexed*/  INDEXED ibreg=ibreg^M_RDMEM(eaddr);SETNZ8(ibreg)
					 CLV break;
		case 0xE9: /*ADCB indexed*/  INDEXED tw=ibreg+M_RDMEM(eaddr)+(iccreg&0x01);
		                             SETSTATUS(ibreg,M_RDMEM(eaddr),tw)
		                             ibreg=tw;break;
		case 0xEA: /*ORB indexed*/   INDEXED ibreg=ibreg|M_RDMEM(eaddr);SETNZ8(ibreg)
					 CLV break;
		case 0xEB: /*ADDB indexed*/  INDEXED tw=ibreg+M_RDMEM(eaddr);
					 SETSTATUS(ibreg,M_RDMEM(eaddr),tw)
					 ibreg=tw;break;
		case 0xEC: /* LDD indexed - 6803 only */  INDEXED tw=GETWORD(eaddr);
		                              CLV SETNZ16(tw) SETDREG(tw); break;
		case 0xED:  /* STD indexed - 6803 only  */ INDEXED
		                              tw=GETDREG;
		                              CLV SETNZ16(tw) SETWORD(eaddr,tw) break;
		case 0xEE: /* LDX indexed */  INDEXED tw=GETWORD(eaddr);
		                              CLV SETNZ16(tw) ixreg=tw; break;
		case 0xEF:  /* STX indexed  */ INDEXED
		                              tw=ixreg;
		                              CLV SETNZ16(tw) SETWORD(eaddr,tw) break;
		case 0xF0: /*SUBB ext*/ EXTENDED tw=ibreg-M_RDMEM(eaddr);
		                             SETSTATUS(ibreg,M_RDMEM(eaddr),tw)
		                             ibreg=tw;break;
		case 0xF1: /*CMPB ext*/ EXTENDED tw=ibreg-M_RDMEM(eaddr);
					 SETSTATUS(ibreg,M_RDMEM(eaddr),tw) break;
		case 0xF2: /*SBCB ext*/ EXTENDED tw=ibreg-M_RDMEM(eaddr)-(iccreg&0x01);
					 SETSTATUS(ibreg,M_RDMEM(eaddr),tw)
					 ibreg=tw;break;
		case 0xF3: /*ADDD ext - 6803 only */ EXTENDED tw=M_RDMEM_WORD(eaddr); res=GETDREG+tw;
		          SETSTATUSD(GETDREG,tw,res)
		          SETDREG(res);break;
		case 0xF4: /*ANDB ext*/ EXTENDED ibreg=ibreg&M_RDMEM(eaddr);SETNZ8(ibreg)
					 CLV break;
		case 0xF5: /*BITB ext*/ EXTENDED tb=ibreg&M_RDMEM(eaddr);SETNZ8(tb)
					 CLV break;
		case 0xF6: /*LDB ext*/ EXTENDED LOADAC(ibreg) CLV SETNZ8(ibreg)
		                             break;
		case 0xF7: /*STB ext  */ EXTENDED
		                             SETNZ8(ibreg) CLV STOREAC(ibreg) break;
		case 0xF8: /*EORB ext*/ EXTENDED ibreg=ibreg^M_RDMEM(eaddr);SETNZ8(ibreg)
					 CLV break;
		case 0xF9: /*ADCB ext*/ EXTENDED tw=ibreg+M_RDMEM(eaddr)+(iccreg&0x01);
		                             SETSTATUS(ibreg,M_RDMEM(eaddr),tw)
		                             ibreg=tw;break;
		case 0xFA: /*ORB ext*/  EXTENDED ibreg=ibreg|M_RDMEM(eaddr);SETNZ8(ibreg)
					 CLV break;
		case 0xFB: /*ADDB ext*/ EXTENDED tw=ibreg+M_RDMEM(eaddr);
					 SETSTATUS(ibreg,M_RDMEM(eaddr),tw)
					 ibreg=tw;break;
		case 0xFC: /* LDD ext - 6803 only */ EXTENDED tw=GETWORD(eaddr);
		                              CLV SETNZ16(tw) SETDREG(tw); break;
		case 0xFD:  /* STD ext - 6803 only  */ EXTENDED	tw=GETDREG;
		                              CLV SETNZ16(tw) SETWORD(eaddr,tw) break;
		case 0xFE: /* LDX ext */ EXTENDED tw=GETWORD(eaddr);
		                              CLV SETNZ16(tw) ixreg=tw; break;
		case 0xFF:  /* STX ext  */ EXTENDED	tw=ixreg;
		                              CLV SETNZ16(tw) SETWORD(eaddr,tw) break;
	}
}
