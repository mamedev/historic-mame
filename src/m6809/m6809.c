/*** m6809: Portable 6809 emulator ******************************************

	Copyright (C) John Butler 1997

	References:

		6809 Simulator V09, By L.C. Benschop, Eidnhoven The Netherlands.

		m6809: Portable 6809 emulator, DS (6809 code in MAME, derived from
			the 6809 Simulator V09)

		6809 Microcomputer Programming & Interfacing with Experiments"
			by Andrew C. Staugaard, Jr.; Howard W. Sams & Co., Inc.

	System dependencies:	word must be 16 bit unsigned int
							byte must be 8 bit unsigned int
							long must be more than 16 bits
							arrays up to 65536 bytes must be supported
							machine must be twos complement

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "osd_dbg.h"
#include "M6809.h"
#include "driver.h"

INLINE byte fetch_effective_address( void );

/* 6809 registers */
static byte cc,dpreg;

static union {
#ifdef LSB_FIRST
	struct {byte l,h;} b;
#else
	struct {byte h,l;} b;
#endif
	word w;
}dbreg;
#define areg (dbreg.b.h)
#define breg (dbreg.b.l)

static word xreg,yreg,ureg,sreg,pcreg;

static word eaddr; /* effective address */

static int pending_interrupts;	/* NS 970908 */

/* public globals */
int	m6809_ICount=50000;
int m6809_Flags;	/* flags for speed optimization */

/* flag, handlers for speed optimization */
static int (*rd_u_handler)(int);
static int (*rd_u_handler_wd)(int);
static int (*rd_s_handler)(int);
static int (*rd_s_handler_wd)(int);
static void (*wr_u_handler)(int,int);
static void (*wr_u_handler_wd)(int,int);
static void (*wr_s_handler)(int,int);
static void (*wr_s_handler_wd)(int,int);

/* these are re-defined in m6809.h TO RAM, ROM or functions in cpuintrf.c */
#define M_RDMEM(A)      M6809_RDMEM(A)
#define M_WRMEM(A,V)    M6809_WRMEM(A,V)
#define M_RDOP(A)       M6809_RDOP(A)
#define M_RDOP_ARG(A)   M6809_RDOP_ARG(A)

/* macros to access memory */
#define IMMBYTE(b)	{b = M_RDOP_ARG(pcreg++);}
#define IMMWORD(w)	{w = (M_RDOP_ARG(pcreg)<<8) + M_RDOP_ARG(pcreg+1); pcreg+=2;}

#define PUSHBYTE(b) {--sreg;(*wr_s_handler)(sreg,(b));}
#define PUSHWORD(w) {sreg-=2;(*wr_s_handler_wd)(sreg,(w));}
#define PULLBYTE(b) {b=(*rd_s_handler)(sreg);sreg++;}
#define PULLWORD(w) {w=(*rd_s_handler_wd)(sreg);sreg+=2;}
#define PSHUBYTE(b) {--ureg;(*wr_u_handler)(ureg,(b));}
#define PSHUWORD(w) {ureg-=2;(*wr_u_handler_wd)(ureg,(w));}
#define PULUBYTE(b) {b=(*rd_u_handler)(ureg);ureg++;}
#define PULUWORD(w) {w=(*rd_u_handler_wd)(ureg);ureg+=2;}

/* CC masks						  H  NZVC
								7654 3210	*/
#define CLR_HNZVC	cc&=0xd0
#define CLR_NZV		cc&=0xf1
#define CLR_HNZC	cc&=0xd2
#define CLR_NZVC	cc&=0xf0
#define CLR_Z		cc&=0xfb
#define CLR_NZC		cc&=0xf2
#define CLR_ZC		cc&=0xfa
/* macros for CC -- CC bits affected should be reset before calling */
#define SET_Z(a)		if(!a)SEZ
#define SET_Z8(a)		SET_Z((byte)a)
#define SET_Z16(a)		SET_Z((word)a)
#define SET_N8(a)		cc|=((a&0x80)>>4)
#define SET_N16(a)		cc|=((a&0x8000)>>12)
#define SET_H(a,b,r)	cc|=(((a^b^r)&0x10)<<1)
#define SET_C8(a)		cc|=((a&0x100)>>8)
#define SET_C16(a)		cc|=((a&0x10000)>>16)
#define SET_V8(a,b,r)	cc|=(((a^b^r^(r>>1))&0x80)>>6)
#define SET_V16(a,b,r)	cc|=(((a^b^r^(r>>1))&0x8000)>>14)

static byte flags8i[256]=	/* increment */
{
0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x0a,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08
};
static byte flags8d[256]= /* decrement */
{
0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,
0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08,0x08
};
#define SET_FLAGS8I(a)		{cc|=flags8i[(a)&0xff];}
#define SET_FLAGS8D(a)		{cc|=flags8d[(a)&0xff];}

/* combos */
#define SET_NZ8(a)			{SET_N8(a);SET_Z(a);}
#define SET_NZ16(a)			{SET_N16(a);SET_Z(a);}
#define SET_FLAGS8(a,b,r)	{SET_N8(r);SET_Z8(r);SET_V8(a,b,r);SET_C8(r);}
#define SET_FLAGS16(a,b,r)	{SET_N16(r);SET_Z16(r);SET_V16(a,b,r);SET_C16(r);}

/* for treating an unsigned byte as a signed word */
#define SIGNED(b) ((word)(b&0x80?b|0xff00:b))

/* macros to access dreg */
#define GETDREG (dbreg.w)
#define SETDREG(n) {dbreg.w=n;}

/* macros for addressing modes (postbytes have their own code) */
#define DIRECT {IMMBYTE(eaddr);eaddr|=(dpreg<<8);}
#define IMM8 eaddr=pcreg++
#define IMM16 {eaddr=pcreg;pcreg+=2;}
#define EXTENDED IMMWORD(eaddr)

/* macros to set status flags */
#define SEC cc|=0x01
#define CLC cc&=0xfe
#define SEZ cc|=0x04
#define CLZ cc&=0xfb
#define SEN cc|=0x08
#define CLN cc&=0xf7
#define SEV cc|=0x02
#define CLV cc&=0xfd
#define SEH cc|=0x20
#define CLH cc&=0xdf

/* macros for convenience */
#define DIRBYTE(b) {DIRECT;b=M_RDMEM(eaddr);}
#define DIRWORD(w) {DIRECT;w=M_RDMEM_WORD(eaddr);}
#define EXTBYTE(b) {EXTENDED;b=M_RDMEM(eaddr);}
#define EXTWORD(w) {EXTENDED;w=M_RDMEM_WORD(eaddr);}

/* macros for branch instructions */
#define BRANCH(f) {IMMBYTE(t);if(f){pcreg+=SIGNED(t);change_pc(pcreg);}}	/* TS 971002 */
#define LBRANCH(f) {IMMWORD(t);if(f){pcreg+=t;change_pc(pcreg);}}	/* TS 971002 */
#define NXORV  ((cc&0x08)^((cc&0x02)<<2))

/* macros for setting/getting registers in TFR/EXG instructions */
#define GETREG(val,reg) switch(reg) {\
                         case 0: val=GETDREG;break;\
                         case 1: val=xreg;break;\
                         case 2: val=yreg;break;\
                    	 case 3: val=ureg;break;\
                    	 case 4: val=sreg;break;\
                    	 case 5: val=pcreg;break;\
                    	 case 8: val=areg;break;\
                    	 case 9: val=breg;break;\
                    	 case 10: val=cc;break;\
                    	 case 11: val=dpreg;break;}

#define SETREG(val,reg) switch(reg) {\
			 case 0: SETDREG(val); break;\
			 case 1: xreg=val;break;\
			 case 2: yreg=val;break;\
			 case 3: ureg=val;break;\
			 case 4: sreg=val;break;\
			 case 5: pcreg=val;change_pc(pcreg);break;	/* TS 971002 */ \
			 case 8: areg=val;break;\
			 case 9: breg=val;break;\
			 case 10: cc=val;break;\
			 case 11: dpreg=val;break;}

#define EOP 0xff			/* 0xff = extend op code          */
#define E   0x80			/* 0x80 = fetch effective address */
/* timings for 1-byte opcodes */
static unsigned char cycles1[] =
{
	/*    0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F */
  /*0*/	  6,  0,  0,  6,  6,  0,  6,  6,  6,  6,  6,  0,  6,  6,  3,  6,
  /*1*/	255,255,  2,  2,  0,  0,  5,  9,  0,  2,  3,  0,  3,  2,  8,  6,
  /*2*/	  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
  /*3*/	E+4,E+4,E+4,E+4,  5,  5,  5,  5,  0,  5,  3,  6,  0, 11,  0, 19,
  /*4*/	  2,  0,  0,  2,  2,  0,  2,  2,  2,  2,  2,  0,  2,  2,  0,  2,
  /*5*/	  2,  0,  0,  2,  2,  0,  2,  2,  2,  2,  2,  0,  2,  2,  0,  2,
  /*6*/	E+6,E+0,E+0,E+6,E+6,E+0,E+6,E+6,E+6,E+6,E+6,E+0,E+6,E+6,E+3,E+6,
  /*7*/	  7,  0,  0,  7,  7,  0,  7,  7,  7,  7,  7,  0,  7,  7,  4,  7,
  /*8*/	  2,  2,  2,  4,  2,  2,  2,  2,  2,  2,  2,  2,  4,  7,  3,  0,
  /*9*/	  4,  4,  4,  6,  4,  4,  4,  4,  4,  4,  4,  4,  6,  7,  5,  5,
  /*A*/	E+4,E+4,E+4,E+6,E+4,E+4,E+4,E+4,E+4,E+4,E+4,E+4,E+6,E+7,E+5,E+5,
  /*B*/	  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  5,  7,  8,  6,  6,
  /*C*/	  2,  2,  2,  4,  2,  2,  2,  2,  2,  2,  2,  2,  3,  0,  3,  3,
  /*D*/	  4,  4,  4,  6,  4,  4,  4,  4,  4,  4,  4,  4,  5,  5,  5,  5,
  /*E*/	E+4,E+4,E+4,E+6,E+4,E+4,E+4,E+4,E+4,E+4,E+4,E+4,E+5,E+5,E+5,E+5,
  /*F*/	  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  5,  6,  6,  6,  6
};

/* timings for 2-byte opcodes */
static unsigned char cycles2[] =
{
	/*    0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F */
  /*0*/	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  /*1*/	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  /*2*/	  0,  5,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,
  /*3*/	E+0,E+0,E+0,E+0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 20,
  /*4*/	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  /*5*/	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  /*6*/	E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,
  /*7*/	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  /*8*/	  0,  0,  0,  5,  0,  0,  0,  0,  0,  0,  0,  0,  5,  0,  4,  0,
  /*9*/	  0,  0,  0,  7,  0,  0,  0,  0,  0,  0,  0,  0,  7,  0,  6,  6,
  /*A*/	E+0,E+0,E+0,E+7,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+7,E+0,E+6,E+6,
  /*B*/	  0,  0,  0,  8,  0,  0,  0,  0,  0,  0,  0,  0,  8,  0,  7,  7,
  /*C*/	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,  0,
  /*D*/	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  6,
  /*E*/	E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+0,E+6,E+6,
  /*F*/	  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  7,  7
};
#undef EOP
#undef E


static int rd_slow( int addr )
{
    return M_RDMEM(addr);
}

static int rd_slow_wd( int addr )
{
    return( (M_RDMEM(addr)<<8) | (M_RDMEM((addr+1)&0xffff)) );
}

static int rd_fast( int addr )
{
	extern unsigned char *RAM;

	return RAM[addr];
}

static int rd_fast_wd( int addr )
{
	extern unsigned char *RAM;

	return( (RAM[addr]<<8) | (RAM[(addr+1)&0xffff]) );
}

static void wr_slow( int addr, int v )
{
	M_WRMEM(addr,v);
}

static void wr_slow_wd( int addr, int v )
{
	M_WRMEM(addr,v>>8);
	M_WRMEM(((addr)+1)&0xFFFF,v&255);
}

static void wr_fast( int addr, int v )
{
	extern unsigned char *RAM;

	RAM[addr] = v;
}

static void wr_fast_wd( int addr, int v )
{
	extern unsigned char *RAM;

	RAM[addr] = v>>8;
	RAM[(addr+1)&0xffff] = v&255;
}

INLINE unsigned M_RDMEM_WORD (dword A)
{
	int i;

    i = M_RDMEM(A)<<8;
    i |= M_RDMEM(((A)+1)&0xFFFF);
	return i;
}

INLINE void M_WRMEM_WORD (dword A,word V)
{
	M_WRMEM (A,V>>8);
	M_WRMEM (((A)+1)&0xFFFF,V&255);
}

/****************************************************************************/
/* Set all registers to given values                                        */
/****************************************************************************/
void m6809_SetRegs(m6809_Regs *Regs)
{
	pcreg = Regs->pc;change_pc(pcreg);	/* TS 971002 */
	ureg = Regs->u;
	sreg = Regs->s;
	xreg = Regs->x;
	yreg = Regs->y;
	dpreg = Regs->dp;
	areg = Regs->a;
	breg = Regs->b;
	cc = Regs->cc;

	pending_interrupts = Regs->pending_interrupts;	/* NS 970908 */
}

/****************************************************************************/
/* Get all registers in given buffer                                        */
/****************************************************************************/
void m6809_GetRegs(m6809_Regs *Regs)
{
	Regs->pc = pcreg;
	Regs->u = ureg;
	Regs->s = sreg;
	Regs->x = xreg;
	Regs->y = yreg;
	Regs->dp = dpreg;
	Regs->a = areg;
	Regs->b = breg;
	Regs->cc = cc;

	Regs->pending_interrupts = pending_interrupts;	/* NS 970908 */
}

/****************************************************************************/
/* Return program counter                                                   */
/****************************************************************************/
unsigned m6809_GetPC(void)
{
	return pcreg;
}


/* Generate interrupts */
static void Interrupt(void)	/* NS 970909 */
{
	if ((pending_interrupts & M6809_INT_NMI) != 0)
	{
		pending_interrupts &= ~M6809_INT_NMI;

		/* NMI */
		cc|=0x80;	/* ASG 971016 */
		PUSHWORD(pcreg);
		PUSHWORD(ureg);
		PUSHWORD(yreg);
		PUSHWORD(xreg);
		PUSHBYTE(dpreg);
		PUSHBYTE(breg);
		PUSHBYTE(areg);
		PUSHBYTE(cc);
		cc|=0xd0;
		pcreg=M_RDMEM_WORD(0xfffc);change_pc(pcreg);	/* TS 971002 */
		m6809_ICount -= 19;
	}
	else if ((pending_interrupts & M6809_INT_IRQ) != 0 && (cc & 0x10) == 0)
	{
		pending_interrupts &= ~M6809_INT_IRQ;

		/* standard IRQ */
		cc|=0x80;	/* ASG 971016 */
		PUSHWORD(pcreg);
		PUSHWORD(ureg);
		PUSHWORD(yreg);
		PUSHWORD(xreg);
		PUSHBYTE(dpreg);
		PUSHBYTE(breg);
		PUSHBYTE(areg);
		PUSHBYTE(cc);
		cc|=0x90;
		pcreg=M_RDMEM_WORD(0xfff8);change_pc(pcreg);	/* TS 971002 */
		m6809_ICount -= 19;
	}
	else if ((pending_interrupts & M6809_INT_FIRQ) != 0 && (cc & 0x40) == 0)
	{
		pending_interrupts &= ~M6809_INT_FIRQ;

		/* fast IRQ */
		PUSHWORD(pcreg);
		cc&=0x7f;	/* ASG 971016 */
		PUSHBYTE(cc);
		cc|=0x50;
		pcreg=M_RDMEM_WORD(0xfff6);change_pc(pcreg);	/* TS 971002 */
		m6809_ICount -= 10;
	}
}



void m6809_reset(void)
{
	pcreg = M_RDMEM_WORD(0xfffe);change_pc(pcreg);	/* TS 971002 */

	dpreg = 0x00;		/* Direct page register = 0x00 */
	cc = 0x00;			/* Clear all flags */
	cc |= 0x10;			/* IRQ disabled */
	cc |= 0x40;			/* FIRQ disabled */
	areg = 0x00;		/* clear accumulator a */
	breg = 0x00;		/* clear accumulator b */
	m6809_Clear_Pending_Interrupts();	/* NS 970908 */

	/* default to unoptimized memory access */
	rd_u_handler = rd_slow;
	rd_u_handler_wd = rd_slow_wd;
	rd_s_handler = rd_slow;
	rd_s_handler_wd = rd_slow_wd;
	wr_u_handler = wr_slow;
	wr_u_handler_wd = wr_slow_wd;
	wr_s_handler = wr_slow;
	wr_s_handler_wd = wr_slow_wd;

	/* optimize memory access according to flags */
	if( m6809_Flags & M6809_FAST_U )
	{
		rd_u_handler=rd_fast; rd_u_handler_wd=rd_fast_wd;
		wr_u_handler=wr_fast; wr_u_handler_wd=wr_fast_wd;
	}
	if( m6809_Flags & M6809_FAST_S )
	{
		rd_s_handler=rd_fast; rd_s_handler_wd=rd_fast_wd;
		wr_s_handler=wr_fast; wr_s_handler_wd=wr_fast_wd;
	}
}


void m6809_Cause_Interrupt(int type)	/* NS 970908 */
{
	pending_interrupts |= type;
	if (type & (M6809_INT_NMI | M6809_INT_IRQ | M6809_INT_FIRQ))
	{
		pending_interrupts &= ~M6809_SYNC;
		if (pending_interrupts & M6809_CWAI)
		{
			if ((pending_interrupts & M6809_INT_NMI) != 0)
				pending_interrupts &= ~M6809_CWAI;
			else if ((pending_interrupts & M6809_INT_IRQ) != 0 && (cc & 0x10) == 0)
				pending_interrupts &= ~M6809_CWAI;
			else if ((pending_interrupts & M6809_INT_FIRQ) != 0 && (cc & 0x40) == 0)
				pending_interrupts &= ~M6809_CWAI;
		}
	}
}
void m6809_Clear_Pending_Interrupts(void)	/* NS 970908 */
{
	pending_interrupts &= ~(M6809_INT_IRQ | M6809_INT_FIRQ | M6809_INT_NMI);
}


#include "6809ops.c"

/* execute instructions on this CPU until icount expires */
int m6809_execute(int cycles)	/* NS 970908 */
{
	byte op_count;	/* op code clock count */
	byte ireg;
	m6809_ICount = cycles;	/* NS 970908 */

	if (pending_interrupts & (M6809_CWAI | M6809_SYNC))
	{
		m6809_ICount = 0;
		goto getout;
	}

	do
	{
		if (pending_interrupts != 0)
			Interrupt();	/* NS 970908 */

#ifdef	MAME_DEBUG
{
  extern int mame_debug;
  if (mame_debug) MAME_Debug();
}
#endif
		ireg=M_RDOP(pcreg++);

		if( (op_count = cycles1[ireg])!=0xff ){
			if( op_count &0x80 ) op_count += fetch_effective_address();

			switch( ireg )
			{
				case 0x00: neg_di(); break;
				case 0x01: illegal(); break;
				case 0x02: illegal(); break;
				case 0x03: com_di(); break;
				case 0x04: lsr_di(); break;
				case 0x05: illegal(); break;
				case 0x06: ror_di(); break;
				case 0x07: asr_di(); break;
				case 0x08: asl_di(); break;
				case 0x09: rol_di(); break;
				case 0x0a: dec_di(); break;
				case 0x0b: illegal(); break;
				case 0x0c: inc_di(); break;
				case 0x0d: tst_di(); break;
				case 0x0e: jmp_di(); break;
				case 0x0f: clr_di(); break;
				case 0x10: illegal(); break;
				case 0x11: illegal(); break;
				case 0x12: nop(); break;
				case 0x13: sync(); break;
				case 0x14: illegal(); break;
				case 0x15: illegal(); break;
				case 0x16: lbra(); break;
				case 0x17: lbsr(); break;
				case 0x18: illegal(); break;
				case 0x19: daa(); break;
				case 0x1a: orcc(); break;
				case 0x1b: illegal(); break;
				case 0x1c: andcc(); break;
				case 0x1d: sex(); break;
				case 0x1e: exg(); break;
				case 0x1f: tfr(); break;
				case 0x20: bra(); break;
				case 0x21: brn(); break;
				case 0x22: bhi(); break;
				case 0x23: bls(); break;
				case 0x24: bcc(); break;
				case 0x25: bcs(); break;
				case 0x26: bne(); break;
				case 0x27: beq(); break;
				case 0x28: bvc(); break;
				case 0x29: bvs(); break;
				case 0x2a: bpl(); break;
				case 0x2b: bmi(); break;
				case 0x2c: bge(); break;
				case 0x2d: blt(); break;
				case 0x2e: bgt(); break;
				case 0x2f: ble(); break;
				case 0x30: leax(); break;
				case 0x31: leay(); break;
				case 0x32: leas(); break;
				case 0x33: leau(); break;
				case 0x34: pshs(); break;
				case 0x35: puls(); break;
				case 0x36: pshu(); break;
				case 0x37: pulu(); break;
				case 0x38: illegal(); break;
				case 0x39: rts(); break;
				case 0x3a: abx(); break;
				case 0x3b: rti(); break;
				case 0x3c: cwai(); break;
				case 0x3d: mul(); break;
				case 0x3e: illegal(); break;
				case 0x3f: swi(); break;
				case 0x40: nega(); break;
				case 0x41: illegal(); break;
				case 0x42: illegal(); break;
				case 0x43: coma(); break;
				case 0x44: lsra(); break;
				case 0x45: illegal(); break;
				case 0x46: rora(); break;
				case 0x47: asra(); break;
				case 0x48: asla(); break;
				case 0x49: rola(); break;
				case 0x4a: deca(); break;
				case 0x4b: illegal(); break;
				case 0x4c: inca(); break;
				case 0x4d: tsta(); break;
				case 0x4e: illegal(); break;
				case 0x4f: clra(); break;
				case 0x50: negb(); break;
				case 0x51: illegal(); break;
				case 0x52: illegal(); break;
				case 0x53: comb(); break;
				case 0x54: lsrb(); break;
				case 0x55: illegal(); break;
				case 0x56: rorb(); break;
				case 0x57: asrb(); break;
				case 0x58: aslb(); break;
				case 0x59: rolb(); break;
				case 0x5a: decb(); break;
				case 0x5b: illegal(); break;
				case 0x5c: incb(); break;
				case 0x5d: tstb(); break;
				case 0x5e: illegal(); break;
				case 0x5f: clrb(); break;
				case 0x60: neg_ix(); break;
				case 0x61: illegal(); break;
				case 0x62: illegal(); break;
				case 0x63: com_ix(); break;
				case 0x64: lsr_ix(); break;
				case 0x65: illegal(); break;
				case 0x66: ror_ix(); break;
				case 0x67: asr_ix(); break;
				case 0x68: asl_ix(); break;
				case 0x69: rol_ix(); break;
				case 0x6a: dec_ix(); break;
				case 0x6b: illegal(); break;
				case 0x6c: inc_ix(); break;
				case 0x6d: tst_ix(); break;
				case 0x6e: jmp_ix(); break;
				case 0x6f: clr_ix(); break;
				case 0x70: neg_ex(); break;
				case 0x71: illegal(); break;
				case 0x72: illegal(); break;
				case 0x73: com_ex(); break;
				case 0x74: lsr_ex(); break;
				case 0x75: illegal(); break;
				case 0x76: ror_ex(); break;
				case 0x77: asr_ex(); break;
				case 0x78: asl_ex(); break;
				case 0x79: rol_ex(); break;
				case 0x7a: dec_ex(); break;
				case 0x7b: illegal(); break;
				case 0x7c: inc_ex(); break;
				case 0x7d: tst_ex(); break;
				case 0x7e: jmp_ex(); break;
				case 0x7f: clr_ex(); break;
				case 0x80: suba_im(); break;
				case 0x81: cmpa_im(); break;
				case 0x82: sbca_im(); break;
				case 0x83: subd_im(); break;
				case 0x84: anda_im(); break;
				case 0x85: bita_im(); break;
				case 0x86: lda_im(); break;
				case 0x87: sta_im(); break; /* ILLEGAL? */
				case 0x88: eora_im(); break;
				case 0x89: adca_im(); break;
				case 0x8a: ora_im(); break;
				case 0x8b: adda_im(); break;
				case 0x8c: cmpx_im(); break;
				case 0x8d: bsr(); break;
				case 0x8e: ldx_im(); break;
				case 0x8f: stx_im(); break; /* ILLEGAL? */
				case 0x90: suba_di(); break;
				case 0x91: cmpa_di(); break;
				case 0x92: sbca_di(); break;
				case 0x93: subd_di(); break;
				case 0x94: anda_di(); break;
				case 0x95: bita_di(); break;
				case 0x96: lda_di(); break;
				case 0x97: sta_di(); break;
				case 0x98: eora_di(); break;
				case 0x99: adca_di(); break;
				case 0x9a: ora_di(); break;
				case 0x9b: adda_di(); break;
				case 0x9c: cmpx_di(); break;
				case 0x9d: jsr_di(); break;
				case 0x9e: ldx_di(); break;
				case 0x9f: stx_di(); break;
				case 0xa0: suba_ix(); break;
				case 0xa1: cmpa_ix(); break;
				case 0xa2: sbca_ix(); break;
				case 0xa3: subd_ix(); break;
				case 0xa4: anda_ix(); break;
				case 0xa5: bita_ix(); break;
				case 0xa6: lda_ix(); break;
				case 0xa7: sta_ix(); break;
				case 0xa8: eora_ix(); break;
				case 0xa9: adca_ix(); break;
				case 0xaa: ora_ix(); break;
				case 0xab: adda_ix(); break;
				case 0xac: cmpx_ix(); break;
				case 0xad: jsr_ix(); break;
				case 0xae: ldx_ix(); break;
				case 0xaf: stx_ix(); break;
				case 0xb0: suba_ex(); break;
				case 0xb1: cmpa_ex(); break;
				case 0xb2: sbca_ex(); break;
				case 0xb3: subd_ex(); break;
				case 0xb4: anda_ex(); break;
				case 0xb5: bita_ex(); break;
				case 0xb6: lda_ex(); break;
				case 0xb7: sta_ex(); break;
				case 0xb8: eora_ex(); break;
				case 0xb9: adca_ex(); break;
				case 0xba: ora_ex(); break;
				case 0xbb: adda_ex(); break;
				case 0xbc: cmpx_ex(); break;
				case 0xbd: jsr_ex(); break;
				case 0xbe: ldx_ex(); break;
				case 0xbf: stx_ex(); break;
				case 0xc0: subb_im(); break;
				case 0xc1: cmpb_im(); break;
				case 0xc2: sbcb_im(); break;
				case 0xc3: addd_im(); break;
				case 0xc4: andb_im(); break;
				case 0xc5: bitb_im(); break;
				case 0xc6: ldb_im(); break;
				case 0xc7: stb_im(); break; /* ILLEGAL? */
				case 0xc8: eorb_im(); break;
				case 0xc9: adcb_im(); break;
				case 0xca: orb_im(); break;
				case 0xcb: addb_im(); break;
				case 0xcc: ldd_im(); break;
				case 0xcd: std_im(); break; /* ILLEGAL? */
				case 0xce: ldu_im(); break;
				case 0xcf: stu_im(); break; /* ILLEGAL? */
				case 0xd0: subb_di(); break;
				case 0xd1: cmpb_di(); break;
				case 0xd2: sbcb_di(); break;
				case 0xd3: addd_di(); break;
				case 0xd4: andb_di(); break;
				case 0xd5: bitb_di(); break;
				case 0xd6: ldb_di(); break;
				case 0xd7: stb_di(); break;
				case 0xd8: eorb_di(); break;
				case 0xd9: adcb_di(); break;
				case 0xda: orb_di(); break;
				case 0xdb: addb_di(); break;
				case 0xdc: ldd_di(); break;
				case 0xdd: std_di(); break;
				case 0xde: ldu_di(); break;
				case 0xdf: stu_di(); break;
				case 0xe0: subb_ix(); break;
				case 0xe1: cmpb_ix(); break;
				case 0xe2: sbcb_ix(); break;
				case 0xe3: addd_ix(); break;
				case 0xe4: andb_ix(); break;
				case 0xe5: bitb_ix(); break;
				case 0xe6: ldb_ix(); break;
				case 0xe7: stb_ix(); break;
				case 0xe8: eorb_ix(); break;
				case 0xe9: adcb_ix(); break;
				case 0xea: orb_ix(); break;
				case 0xeb: addb_ix(); break;
				case 0xec: ldd_ix(); break;
				case 0xed: std_ix(); break;
				case 0xee: ldu_ix(); break;
				case 0xef: stu_ix(); break;
				case 0xf0: subb_ex(); break;
				case 0xf1: cmpb_ex(); break;
				case 0xf2: sbcb_ex(); break;
				case 0xf3: addd_ex(); break;
				case 0xf4: andb_ex(); break;
				case 0xf5: bitb_ex(); break;
				case 0xf6: ldb_ex(); break;
				case 0xf7: stb_ex(); break;
				case 0xf8: eorb_ex(); break;
				case 0xf9: adcb_ex(); break;
				case 0xfa: orb_ex(); break;
				case 0xfb: addb_ex(); break;
				case 0xfc: ldd_ex(); break;
				case 0xfd: std_ex(); break;
				case 0xfe: ldu_ex(); break;
				case 0xff: stu_ex(); break;
			}
		}
		else
		{
			byte ireg2;
			ireg2=M_RDOP(pcreg++);

			if( (op_count=cycles2[ireg2]) &0x80 ) fetch_effective_address();

			if( ireg == 0x10 ){
				switch( ireg2 )	/* 10xx */
				{
				case 0x21: lbrn(); break;
				case 0x22: lbhi(); break;
				case 0x23: lbls(); break;
				case 0x24: lbcc(); break;
				case 0x25: lbcs(); break;
				case 0x26: lbne(); break;
				case 0x27: lbeq(); break;
				case 0x28: lbvc(); break;
				case 0x29: lbvs(); break;
				case 0x2a: lbpl(); break;
				case 0x2b: lbmi(); break;
				case 0x2c: lbge(); break;
				case 0x2d: lblt(); break;
				case 0x2e: lbgt(); break;
				case 0x2f: lble(); break;
				case 0x3f: swi2(); break;
				case 0x83: cmpd_im(); break;
				case 0x8c: cmpy_im(); break;
				case 0x8e: ldy_im(); break;
				case 0x8f: sty_im(); break; /* ILLEGAL? */
				case 0x93: cmpd_di(); break;
				case 0x9c: cmpy_di(); break;
				case 0x9e: ldy_di(); break;
				case 0x9f: sty_di(); break;
				case 0xa3: cmpd_ix(); break;
				case 0xac: cmpy_ix(); break;
				case 0xae: ldy_ix(); break;
				case 0xaf: sty_ix(); break;
				case 0xb3: cmpd_ex(); break;
				case 0xbc: cmpy_ex(); break;
				case 0xbe: ldy_ex(); break;
				case 0xbf: sty_ex(); break;
				case 0xce: lds_im(); break;
				case 0xcf: sts_im(); break; /* ILLEGAL? */
				case 0xde: lds_di(); break;
				case 0xdf: sts_di(); break;
				case 0xee: lds_ix(); break;
				case 0xef: sts_ix(); break;
				case 0xfe: lds_ex(); break;
				case 0xff: sts_ex(); break;
				default: illegal(); break;
				}
			}else{
				switch( ireg2 )	/* 11xx */
				{
				case 0x3f: swi3(); break;
				case 0x83: cmpu_im(); break;
				case 0x8c: cmps_im(); break;
				case 0x93: cmpu_di(); break;
				case 0x9c: cmps_di(); break;
				case 0xa3: cmpu_ix(); break;
				case 0xac: cmps_ix(); break;
				case 0xb3: cmpu_ex(); break;
				case 0xbc: cmps_ex(); break;
				default: illegal(); break;
				}
			}
		}
		m6809_ICount -= op_count & 0x7f;

	} while( m6809_ICount>0 );

getout:
	return cycles - m6809_ICount;	/* NS 970908 */
}

INLINE byte fetch_effective_address( void )
{
	byte postbyte, ec=0;


	postbyte=M_RDOP_ARG(pcreg++);

	switch(postbyte)
	{
	    case 0x00: eaddr=xreg;ec=1;break;
	    case 0x01: eaddr=xreg+1;ec=1;break;
	    case 0x02: eaddr=xreg+2;ec=1;break;
	    case 0x03: eaddr=xreg+3;ec=1;break;
	    case 0x04: eaddr=xreg+4;ec=1;break;
	    case 0x05: eaddr=xreg+5;ec=1;break;
	    case 0x06: eaddr=xreg+6;ec=1;break;
	    case 0x07: eaddr=xreg+7;ec=1;break;
	    case 0x08: eaddr=xreg+8;ec=1;break;
	    case 0x09: eaddr=xreg+9;ec=1;break;
	    case 0x0A: eaddr=xreg+10;ec=1;break;
	    case 0x0B: eaddr=xreg+11;ec=1;break;
	    case 0x0C: eaddr=xreg+12;ec=1;break;
	    case 0x0D: eaddr=xreg+13;ec=1;break;
	    case 0x0E: eaddr=xreg+14;ec=1;break;
	    case 0x0F: eaddr=xreg+15;ec=1;break;
	    case 0x10: eaddr=xreg-16;ec=1;break;
	    case 0x11: eaddr=xreg-15;ec=1;break;
	    case 0x12: eaddr=xreg-14;ec=1;break;
	    case 0x13: eaddr=xreg-13;ec=1;break;
	    case 0x14: eaddr=xreg-12;ec=1;break;
	    case 0x15: eaddr=xreg-11;ec=1;break;
	    case 0x16: eaddr=xreg-10;ec=1;break;
	    case 0x17: eaddr=xreg-9;ec=1;break;
	    case 0x18: eaddr=xreg-8;ec=1;break;
	    case 0x19: eaddr=xreg-7;ec=1;break;
	    case 0x1A: eaddr=xreg-6;ec=1;break;
	    case 0x1B: eaddr=xreg-5;ec=1;break;
	    case 0x1C: eaddr=xreg-4;ec=1;break;
	    case 0x1D: eaddr=xreg-3;ec=1;break;
	    case 0x1E: eaddr=xreg-2;ec=1;break;
	    case 0x1F: eaddr=xreg-1;ec=1;break;
	    case 0x20: eaddr=yreg;ec=1;break;
	    case 0x21: eaddr=yreg+1;ec=1;break;
	    case 0x22: eaddr=yreg+2;ec=1;break;
	    case 0x23: eaddr=yreg+3;ec=1;break;
	    case 0x24: eaddr=yreg+4;ec=1;break;
	    case 0x25: eaddr=yreg+5;ec=1;break;
	    case 0x26: eaddr=yreg+6;ec=1;break;
	    case 0x27: eaddr=yreg+7;ec=1;break;
	    case 0x28: eaddr=yreg+8;ec=1;break;
	    case 0x29: eaddr=yreg+9;ec=1;break;
	    case 0x2A: eaddr=yreg+10;ec=1;break;
	    case 0x2B: eaddr=yreg+11;ec=1;break;
	    case 0x2C: eaddr=yreg+12;ec=1;break;
	    case 0x2D: eaddr=yreg+13;ec=1;break;
	    case 0x2E: eaddr=yreg+14;ec=1;break;
	    case 0x2F: eaddr=yreg+15;ec=1;break;
	    case 0x30: eaddr=yreg-16;ec=1;break;
	    case 0x31: eaddr=yreg-15;ec=1;break;
	    case 0x32: eaddr=yreg-14;ec=1;break;
	    case 0x33: eaddr=yreg-13;ec=1;break;
	    case 0x34: eaddr=yreg-12;ec=1;break;
	    case 0x35: eaddr=yreg-11;ec=1;break;
	    case 0x36: eaddr=yreg-10;ec=1;break;
	    case 0x37: eaddr=yreg-9;ec=1;break;
	    case 0x38: eaddr=yreg-8;ec=1;break;
	    case 0x39: eaddr=yreg-7;ec=1;break;
	    case 0x3A: eaddr=yreg-6;ec=1;break;
	    case 0x3B: eaddr=yreg-5;ec=1;break;
	    case 0x3C: eaddr=yreg-4;ec=1;break;
	    case 0x3D: eaddr=yreg-3;ec=1;break;
	    case 0x3E: eaddr=yreg-2;ec=1;break;
	    case 0x3F: eaddr=yreg-1;ec=1;break;
	    case 0x40: eaddr=ureg;ec=1;break;
	    case 0x41: eaddr=ureg+1;ec=1;break;
	    case 0x42: eaddr=ureg+2;ec=1;break;
	    case 0x43: eaddr=ureg+3;ec=1;break;
	    case 0x44: eaddr=ureg+4;ec=1;break;
	    case 0x45: eaddr=ureg+5;ec=1;break;
	    case 0x46: eaddr=ureg+6;ec=1;break;
	    case 0x47: eaddr=ureg+7;ec=1;break;
	    case 0x48: eaddr=ureg+8;ec=1;break;
	    case 0x49: eaddr=ureg+9;ec=1;break;
	    case 0x4A: eaddr=ureg+10;ec=1;break;
	    case 0x4B: eaddr=ureg+11;ec=1;break;
	    case 0x4C: eaddr=ureg+12;ec=1;break;
	    case 0x4D: eaddr=ureg+13;ec=1;break;
	    case 0x4E: eaddr=ureg+14;ec=1;break;
	    case 0x4F: eaddr=ureg+15;ec=1;break;
	    case 0x50: eaddr=ureg-16;ec=1;break;
	    case 0x51: eaddr=ureg-15;ec=1;break;
	    case 0x52: eaddr=ureg-14;ec=1;break;
	    case 0x53: eaddr=ureg-13;ec=1;break;
	    case 0x54: eaddr=ureg-12;ec=1;break;
	    case 0x55: eaddr=ureg-11;ec=1;break;
	    case 0x56: eaddr=ureg-10;ec=1;break;
	    case 0x57: eaddr=ureg-9;ec=1;break;
	    case 0x58: eaddr=ureg-8;ec=1;break;
	    case 0x59: eaddr=ureg-7;ec=1;break;
	    case 0x5A: eaddr=ureg-6;ec=1;break;
	    case 0x5B: eaddr=ureg-5;ec=1;break;
	    case 0x5C: eaddr=ureg-4;ec=1;break;
	    case 0x5D: eaddr=ureg-3;ec=1;break;
	    case 0x5E: eaddr=ureg-2;ec=1;break;
	    case 0x5F: eaddr=ureg-1;ec=1;break;
	    case 0x60: eaddr=sreg;ec=1;break;
	    case 0x61: eaddr=sreg+1;ec=1;break;
	    case 0x62: eaddr=sreg+2;ec=1;break;
	    case 0x63: eaddr=sreg+3;ec=1;break;
	    case 0x64: eaddr=sreg+4;ec=1;break;
	    case 0x65: eaddr=sreg+5;ec=1;break;
	    case 0x66: eaddr=sreg+6;ec=1;break;
	    case 0x67: eaddr=sreg+7;ec=1;break;
	    case 0x68: eaddr=sreg+8;ec=1;break;
	    case 0x69: eaddr=sreg+9;ec=1;break;
	    case 0x6A: eaddr=sreg+10;ec=1;break;
	    case 0x6B: eaddr=sreg+11;ec=1;break;
	    case 0x6C: eaddr=sreg+12;ec=1;break;
	    case 0x6D: eaddr=sreg+13;ec=1;break;
	    case 0x6E: eaddr=sreg+14;ec=1;break;
	    case 0x6F: eaddr=sreg+15;ec=1;break;
	    case 0x70: eaddr=sreg-16;ec=1;break;
	    case 0x71: eaddr=sreg-15;ec=1;break;
	    case 0x72: eaddr=sreg-14;ec=1;break;
	    case 0x73: eaddr=sreg-13;ec=1;break;
	    case 0x74: eaddr=sreg-12;ec=1;break;
	    case 0x75: eaddr=sreg-11;ec=1;break;
	    case 0x76: eaddr=sreg-10;ec=1;break;
	    case 0x77: eaddr=sreg-9;ec=1;break;
	    case 0x78: eaddr=sreg-8;ec=1;break;
	    case 0x79: eaddr=sreg-7;ec=1;break;
	    case 0x7A: eaddr=sreg-6;ec=1;break;
	    case 0x7B: eaddr=sreg-5;ec=1;break;
	    case 0x7C: eaddr=sreg-4;ec=1;break;
	    case 0x7D: eaddr=sreg-3;ec=1;break;
	    case 0x7E: eaddr=sreg-2;ec=1;break;
	    case 0x7F: eaddr=sreg-1;ec=1;break;
	    case 0x80: eaddr=xreg;xreg++;ec=2;break;
	    case 0x81: eaddr=xreg;xreg+=2;ec=3;break;
	    case 0x82: xreg--;eaddr=xreg;ec=2;break;
	    case 0x83: xreg-=2;eaddr=xreg;ec=3;break;
	    case 0x84: eaddr=xreg;break;
	    case 0x85: eaddr=xreg+SIGNED(breg);ec=1;break;
	    case 0x86: eaddr=xreg+SIGNED(areg);ec=1;break;
	    case 0x87: eaddr=0;break; /*ILLEGAL*/
	    case 0x88: IMMBYTE(eaddr);eaddr=xreg+SIGNED(eaddr);ec=1;break; /* this is a hack to make Vectrex work. It should be ec=1. Dunno where the cycle was lost :( */
	    case 0x89: IMMWORD(eaddr);eaddr+=xreg;ec=4;break;
	    case 0x8A: eaddr=0;break; /*ILLEGAL*/
	    case 0x8B: eaddr=xreg+GETDREG;ec=4;break;
	    case 0x8C: IMMBYTE(eaddr);eaddr=pcreg+SIGNED(eaddr);ec=1;break;
	    case 0x8D: IMMWORD(eaddr);eaddr+=pcreg;ec=5;break;
	    case 0x8E: eaddr=0;break; /*ILLEGAL*/
	    case 0x8F: IMMWORD(eaddr);ec=5;break;
	    case 0x90: eaddr=xreg;xreg++;eaddr=M_RDMEM_WORD(eaddr);ec=5;break; /* Indirect ,R+ not in my specs */
	    case 0x91: eaddr=xreg;xreg+=2;eaddr=M_RDMEM_WORD(eaddr);ec=6;break;
	    case 0x92: xreg--;eaddr=xreg;eaddr=M_RDMEM_WORD(eaddr);ec=5;break;
	    case 0x93: xreg-=2;eaddr=xreg;eaddr=M_RDMEM_WORD(eaddr);ec=6;break;
	    case 0x94: eaddr=xreg;eaddr=M_RDMEM_WORD(eaddr);ec=3;break;
	    case 0x95: eaddr=xreg+SIGNED(breg);eaddr=M_RDMEM_WORD(eaddr);ec=4;break;
	    case 0x96: eaddr=xreg+SIGNED(areg);eaddr=M_RDMEM_WORD(eaddr);ec=4;break;
	    case 0x97: eaddr=0;break; /*ILLEGAL*/
	    case 0x98: IMMBYTE(eaddr);eaddr=xreg+SIGNED(eaddr);
	               eaddr=M_RDMEM_WORD(eaddr);ec=4;break;
	    case 0x99: IMMWORD(eaddr);eaddr+=xreg;eaddr=M_RDMEM_WORD(eaddr);ec=7;break;
	    case 0x9A: eaddr=0;break; /*ILLEGAL*/
	    case 0x9B: eaddr=xreg+GETDREG;eaddr=M_RDMEM_WORD(eaddr);ec=7;break;
	    case 0x9C: IMMBYTE(eaddr);eaddr=pcreg+SIGNED(eaddr);
	               eaddr=M_RDMEM_WORD(eaddr);ec=4;break;
	    case 0x9D: IMMWORD(eaddr);eaddr+=pcreg;eaddr=M_RDMEM_WORD(eaddr);ec=8;break;
	    case 0x9E: eaddr=0;break; /*ILLEGAL*/
	    case 0x9F: IMMWORD(eaddr);eaddr=M_RDMEM_WORD(eaddr);ec=8;break;
	    case 0xA0: eaddr=yreg;yreg++;ec=2;break;
	    case 0xA1: eaddr=yreg;yreg+=2;ec=3;break;
	    case 0xA2: yreg--;eaddr=yreg;ec=2;break;
	    case 0xA3: yreg-=2;eaddr=yreg;ec=3;break;
	    case 0xA4: eaddr=yreg;break;
	    case 0xA5: eaddr=yreg+SIGNED(breg);ec=1;break;
	    case 0xA6: eaddr=yreg+SIGNED(areg);ec=1;break;
	    case 0xA7: eaddr=0;break; /*ILLEGAL*/
	    case 0xA8: IMMBYTE(eaddr);eaddr=yreg+SIGNED(eaddr);ec=1;break;
	    case 0xA9: IMMWORD(eaddr);eaddr+=yreg;ec=4;break;
	    case 0xAA: eaddr=0;break; /*ILLEGAL*/
	    case 0xAB: eaddr=yreg+GETDREG;ec=4;break;
	    case 0xAC: IMMBYTE(eaddr);eaddr=pcreg+SIGNED(eaddr);ec=1;break;
	    case 0xAD: IMMWORD(eaddr);eaddr+=pcreg;ec=5;break;
	    case 0xAE: eaddr=0;break; /*ILLEGAL*/
	    case 0xAF: IMMWORD(eaddr);ec=5;break;
	    case 0xB0: eaddr=yreg;yreg++;eaddr=M_RDMEM_WORD(eaddr);ec=5;break;
	    case 0xB1: eaddr=yreg;yreg+=2;eaddr=M_RDMEM_WORD(eaddr);ec=6;break;
	    case 0xB2: yreg--;eaddr=yreg;eaddr=M_RDMEM_WORD(eaddr);ec=5;break;
	    case 0xB3: yreg-=2;eaddr=yreg;eaddr=M_RDMEM_WORD(eaddr);ec=6;break;
	    case 0xB4: eaddr=yreg;eaddr=M_RDMEM_WORD(eaddr);ec=3;break;
	    case 0xB5: eaddr=yreg+SIGNED(breg);eaddr=M_RDMEM_WORD(eaddr);ec=4;break;
	    case 0xB6: eaddr=yreg+SIGNED(areg);eaddr=M_RDMEM_WORD(eaddr);ec=4;break;
	    case 0xB7: eaddr=0;break; /*ILLEGAL*/
	    case 0xB8: IMMBYTE(eaddr);eaddr=yreg+SIGNED(eaddr);
	               eaddr=M_RDMEM_WORD(eaddr);ec=4;break;
	    case 0xB9: IMMWORD(eaddr);eaddr+=yreg;eaddr=M_RDMEM_WORD(eaddr);ec=7;break;
	    case 0xBA: eaddr=0;break; /*ILLEGAL*/
	    case 0xBB: eaddr=yreg+GETDREG;eaddr=M_RDMEM_WORD(eaddr);ec=7;break;
	    case 0xBC: IMMBYTE(eaddr);eaddr=pcreg+SIGNED(eaddr);
	               eaddr=M_RDMEM_WORD(eaddr);ec=4;break;
	    case 0xBD: IMMWORD(eaddr);eaddr+=pcreg;eaddr=M_RDMEM_WORD(eaddr);ec=8;break;
	    case 0xBE: eaddr=0;break; /*ILLEGAL*/
	    case 0xBF: IMMWORD(eaddr);eaddr=M_RDMEM_WORD(eaddr);ec=8;break;
	    case 0xC0: eaddr=ureg;ureg++;ec=2;break;
	    case 0xC1: eaddr=ureg;ureg+=2;ec=3;break;
	    case 0xC2: ureg--;eaddr=ureg;ec=2;break;
	    case 0xC3: ureg-=2;eaddr=ureg;ec=3;break;
	    case 0xC4: eaddr=ureg;break;
	    case 0xC5: eaddr=ureg+SIGNED(breg);ec=1;break;
	    case 0xC6: eaddr=ureg+SIGNED(areg);ec=1;break;
	    case 0xC7: eaddr=0;break; /*ILLEGAL*/
	    case 0xC8: IMMBYTE(eaddr);eaddr=ureg+SIGNED(eaddr);ec=1;break;
	    case 0xC9: IMMWORD(eaddr);eaddr+=ureg;ec=4;break;
	    case 0xCA: eaddr=0;break; /*ILLEGAL*/
	    case 0xCB: eaddr=ureg+GETDREG;ec=4;break;
	    case 0xCC: IMMBYTE(eaddr);eaddr=pcreg+SIGNED(eaddr);ec=1;break;
	    case 0xCD: IMMWORD(eaddr);eaddr+=pcreg;ec=5;break;
	    case 0xCE: eaddr=0;break; /*ILLEGAL*/
	    case 0xCF: IMMWORD(eaddr);ec=5;break;
	    case 0xD0: eaddr=ureg;ureg++;eaddr=M_RDMEM_WORD(eaddr);ec=5;break;
	    case 0xD1: eaddr=ureg;ureg+=2;eaddr=M_RDMEM_WORD(eaddr);ec=6;break;
	    case 0xD2: ureg--;eaddr=ureg;eaddr=M_RDMEM_WORD(eaddr);ec=5;break;
	    case 0xD3: ureg-=2;eaddr=ureg;eaddr=M_RDMEM_WORD(eaddr);ec=6;break;
	    case 0xD4: eaddr=ureg;eaddr=M_RDMEM_WORD(eaddr);ec=3;break;
	    case 0xD5: eaddr=ureg+SIGNED(breg);eaddr=M_RDMEM_WORD(eaddr);ec=4;break;
	    case 0xD6: eaddr=ureg+SIGNED(areg);eaddr=M_RDMEM_WORD(eaddr);ec=4;break;
	    case 0xD7: eaddr=0;break; /*ILLEGAL*/
	    case 0xD8: IMMBYTE(eaddr);eaddr=ureg+SIGNED(eaddr);
	               eaddr=M_RDMEM_WORD(eaddr);ec=4;break;
	    case 0xD9: IMMWORD(eaddr);eaddr+=ureg;eaddr=M_RDMEM_WORD(eaddr);ec=7;break;
	    case 0xDA: eaddr=0;break; /*ILLEGAL*/
	    case 0xDB: eaddr=ureg+GETDREG;eaddr=M_RDMEM_WORD(eaddr);ec=7;break;
	    case 0xDC: IMMBYTE(eaddr);eaddr=pcreg+SIGNED(eaddr);
	               eaddr=M_RDMEM_WORD(eaddr);ec=4;break;
	    case 0xDD: IMMWORD(eaddr);eaddr+=pcreg;eaddr=M_RDMEM_WORD(eaddr);ec=8;break;
	    case 0xDE: eaddr=0;break; /*ILLEGAL*/
	    case 0xDF: IMMWORD(eaddr);eaddr=M_RDMEM_WORD(eaddr);ec=8;break;
	    case 0xE0: eaddr=sreg;sreg++;ec=2;break;
	    case 0xE1: eaddr=sreg;sreg+=2;ec=3;break;
	    case 0xE2: sreg--;eaddr=sreg;ec=2;break;
	    case 0xE3: sreg-=2;eaddr=sreg;ec=3;break;
	    case 0xE4: eaddr=sreg;break;
	    case 0xE5: eaddr=sreg+SIGNED(breg);ec=1;break;
	    case 0xE6: eaddr=sreg+SIGNED(areg);ec=1;break;
	    case 0xE7: eaddr=0;break; /*ILLEGAL*/
	    case 0xE8: IMMBYTE(eaddr);eaddr=sreg+SIGNED(eaddr);ec=1;break;
	    case 0xE9: IMMWORD(eaddr);eaddr+=sreg;ec=4;break;
	    case 0xEA: eaddr=0;break; /*ILLEGAL*/
	    case 0xEB: eaddr=sreg+GETDREG;ec=4;break;
	    case 0xEC: IMMBYTE(eaddr);eaddr=pcreg+SIGNED(eaddr);ec=1;break;
	    case 0xED: IMMWORD(eaddr);eaddr+=pcreg;ec=5;break;
	    case 0xEE: eaddr=0;break; /*ILLEGAL*/
	    case 0xEF: IMMWORD(eaddr);ec=5;break;
	    case 0xF0: eaddr=sreg;sreg++;eaddr=M_RDMEM_WORD(eaddr);ec=5;break;
	    case 0xF1: eaddr=sreg;sreg+=2;eaddr=M_RDMEM_WORD(eaddr);ec=6;break;
	    case 0xF2: sreg--;eaddr=sreg;eaddr=M_RDMEM_WORD(eaddr);ec=5;break;
	    case 0xF3: sreg-=2;eaddr=sreg;eaddr=M_RDMEM_WORD(eaddr);ec=6;break;
	    case 0xF4: eaddr=sreg;eaddr=M_RDMEM_WORD(eaddr);ec=3;break;
	    case 0xF5: eaddr=sreg+SIGNED(breg);eaddr=M_RDMEM_WORD(eaddr);ec=4;break;
	    case 0xF6: eaddr=sreg+SIGNED(areg);eaddr=M_RDMEM_WORD(eaddr);ec=4;break;
	    case 0xF7: eaddr=0;break; /*ILLEGAL*/
	    case 0xF8: IMMBYTE(eaddr);eaddr=sreg+SIGNED(eaddr);
	               eaddr=M_RDMEM_WORD(eaddr);ec=4;break;
	    case 0xF9: IMMWORD(eaddr);eaddr+=sreg;eaddr=M_RDMEM_WORD(eaddr);ec=7;break;
	    case 0xFA: eaddr=0;break; /*ILLEGAL*/
	    case 0xFB: eaddr=sreg+GETDREG;eaddr=M_RDMEM_WORD(eaddr);ec=7;break;
	    case 0xFC: IMMBYTE(eaddr);eaddr=pcreg+SIGNED(eaddr);
	               eaddr=M_RDMEM_WORD(eaddr);ec=4;break;
	    case 0xFD: IMMWORD(eaddr);eaddr+=pcreg;eaddr=M_RDMEM_WORD(eaddr);ec=8;break;
	    case 0xFE: eaddr=0;break; /*ILLEGAL*/
	    case 0xFF: IMMWORD(eaddr);eaddr=M_RDMEM_WORD(eaddr);ec=8;break;
	}
	return (ec);
}
