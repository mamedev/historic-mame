/*** konami: Portable Konami cpu emulator ******************************************

	Copyright (C) The MAME Team 1999

	Based on M6809 cpu core	copyright (C) John Butler 1997

	References:

		6809 Simulator V09, By L.C. Benschop, Eidnhoven The Netherlands.

		m6809: Portable 6809 emulator, DS (6809 code in MAME, derived from
			the 6809 Simulator V09)

		6809 Microcomputer Programming & Interfacing with Experiments"
			by Andrew C. Staugaard, Jr.; Howard W. Sams & Co., Inc.

	System dependencies:	UINT16 must be 16 bit unsigned int
							UINT8 must be 8 bit unsigned int
							UINT32 must be more than 16 bits
							arrays up to 65536 bytes must be supported
							machine must be twos complement

	History:
990720 EHC:
	Created this file

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "cpuintrf.h"
#include "state.h"
#include "mamedbg.h"
#include "konami.h"

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	if( errorlog )	fprintf x
#else
#define LOG(x)
#endif

extern FILE *errorlog;

static UINT8 konami_reg_layout[] = {
	KONAMI_PC, KONAMI_S, KONAMI_CC, KONAMI_A, KONAMI_B, KONAMI_X, -1,
	KONAMI_Y, KONAMI_U, KONAMI_DP, KONAMI_NMI_STATE, KONAMI_IRQ_STATE, KONAMI_FIRQ_STATE, 0
};

/* Layout of the debugger windows x,y,w,h */
static UINT8 konami_win_layout[] = {
	27, 0,53, 4,	/* register window (top, right rows) */
	 0, 0,26,22,	/* disassembler window (left colums) */
	27, 5,53, 8,	/* memory #1 window (right, upper middle) */
	27,14,53, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};

INLINE UINT8 fetch_effective_address( UINT8 );

/* Konami Registers */
typedef struct
{
	PAIR	ppc;	/* Previous program counter */
    PAIR    pc;     /* Program counter */
    PAIR    u, s;   /* Stack pointers */
    PAIR    x, y;   /* Index registers */
    PAIR    d;      /* Accumulatora a and b */
    UINT8   dp;     /* Direct Page register */
    UINT8   cc;
	UINT8	int_state;	/* SYNC and CWAI flags */
	UINT8	nmi_state;
	UINT8	irq_state[2];
    int     (*irq_callback)(int irqline);
    int     extra_cycles; /* cycles used up by interrupts */
} konami_Regs;

/* flag bits in the cc register */
#define CC_C    0x01        /* Carry */
#define CC_V    0x02        /* Overflow */
#define CC_Z    0x04        /* Zero */
#define CC_N    0x08        /* Negative */
#define CC_II   0x10        /* Inhibit IRQ */
#define CC_H    0x20        /* Half (auxiliary) carry */
#define CC_IF   0x40        /* Inhibit FIRQ */
#define CC_E    0x80        /* entire state pushed */

/* Konami registers */
static konami_Regs konami;

#define	pPPC    konami.ppc
#define pPC 	konami.pc
#define pU		konami.u
#define pS		konami.s
#define pX		konami.x
#define pY		konami.y
#define pD		konami.d

#define	PPC		konami.ppc.w.l
#define PC  	konami.pc.w.l
#define U		konami.u.w.l
#define S		konami.s.w.l
#define X		konami.x.w.l
#define Y		konami.y.w.l
#define D   	konami.d.w.l
#define A   	konami.d.b.h
#define B		konami.d.b.l
#define DP		konami.dp
#define CC  	konami.cc

static PAIR ea;         /* effective address */
#define EA	ea.w.l
#define EAD ea.d

/* NS 980101 */
#define KONAMI_CWAI		8	/* set when CWAI is waiting for an interrupt */
#define KONAMI_SYNC		16	/* set when SYNC is waiting for an interrupt */
#define KONAMI_LDS		32	/* set when LDS occured at least once */

#define CHECK_IRQ_LINES 												\
	if( konami.irq_state[KONAMI_IRQ_LINE] != CLEAR_LINE ||				\
		konami.irq_state[KONAMI_FIRQ_LINE] != CLEAR_LINE )				\
		konami.int_state &= ~KONAMI_SYNC; /* clear SYNC flag */			\
	if( konami.irq_state[KONAMI_FIRQ_LINE]!=CLEAR_LINE && !(CC & CC_IF) ) \
	{																	\
		/* fast IRQ */													\
		/* HJB 990225: state already saved by CWAI? */					\
		if( konami.int_state & KONAMI_CWAI )							\
		{																\
			konami.int_state &= ~KONAMI_CWAI;  /* clear CWAI */			\
			konami.extra_cycles += 7;		 /* subtract +7 cycles */	\
        }                                                               \
		else															\
		{																\
			CC &= ~CC_E;				/* save 'short' state */        \
			PUSHWORD(pPC);												\
			PUSHBYTE(CC);												\
			konami.extra_cycles += 10;	/* subtract +10 cycles */		\
		}																\
		CC |= CC_IF | CC_II;			/* inhibit FIRQ and IRQ */		\
		RM16(0xfff6,&konami.pc); 										\
		change_pc(PC);					/* TS 971002 */ 				\
		(void)(*konami.irq_callback)(KONAMI_FIRQ_LINE);					\
	}																	\
	else																\
	if( konami.irq_state[KONAMI_IRQ_LINE]!=CLEAR_LINE && !(CC & CC_II) )\
	{																	\
		/* standard IRQ */												\
		/* HJB 990225: state already saved by CWAI? */					\
		if( konami.int_state & KONAMI_CWAI )							\
		{																\
			konami.int_state &= ~KONAMI_CWAI;  /* clear CWAI flag */	\
			konami.extra_cycles += 7;		 /* subtract +7 cycles */	\
		}																\
		else															\
		{																\
			CC |= CC_E; 				/* save entire state */ 		\
			PUSHWORD(pPC);												\
			PUSHWORD(pU);												\
			PUSHWORD(pY);												\
			PUSHWORD(pX);												\
			PUSHBYTE(DP);												\
			PUSHBYTE(B);												\
			PUSHBYTE(A);												\
			PUSHBYTE(CC);												\
			konami.extra_cycles += 19;	 /* subtract +19 cycles */		\
		}																\
		CC |= CC_II;					/* inhibit IRQ */				\
		RM16(0xfff8,&konami.pc); 										\
		change_pc(PC);					/* TS 971002 */ 				\
		(void)(*konami.irq_callback)(KONAMI_IRQ_LINE);					\
	}

/* public globals */
int konami_ICount=50000;
int konami_Flags;	/* flags for speed optimization */
void (*konami_cpu_setlines_callback)( int lines ) = 0; /* callback called when A16-A23 are set */

/* flag, handlers for speed optimization */
static void (*rd_u_handler_b)(UINT8 *r);
static void (*rd_u_handler_w)(PAIR *r);
static void (*rd_s_handler_b)(UINT8 *r);
static void (*rd_s_handler_w)(PAIR *r);
static void (*wr_u_handler_b)(UINT8 *r);
static void (*wr_u_handler_w)(PAIR *r);
static void (*wr_s_handler_b)(UINT8 *r);
static void (*wr_s_handler_w)(PAIR *r);

/* these are re-defined in konami.h TO RAM, ROM or functions in cpuintrf.c */
#define RM(Addr)			KONAMI_RDMEM(Addr)
#define WM(Addr,Value)		KONAMI_WRMEM(Addr,Value)
#define M_RDOP(Addr)		KONAMI_RDOP(Addr)
#define M_RDOP_ARG(Addr)	KONAMI_RDOP_ARG(Addr)

/* macros to access memory */
#define IMMBYTE(b)	{b = M_RDOP_ARG(PC++);}
#define IMMWORD(w)	{w.d = 0; w.b.h = M_RDOP_ARG(PC); w.b.l = M_RDOP_ARG(PC+1); PC+=2;}

/* pre-clear a PAIR union; clearing h2 and h3 only might be faster? */
#define CLEAR_PAIR(p)   p->d = 0

#define PUSHBYTE(b) (*wr_s_handler_b)(&b)
#define PUSHWORD(w) (*wr_s_handler_w)(&w)
#define PULLBYTE(b) (*rd_s_handler_b)(&b)
#define PULLWORD(w) (*rd_s_handler_w)(&w)
#define PSHUBYTE(b) (*wr_u_handler_b)(&b)
#define PSHUWORD(w) (*wr_u_handler_w)(&w)
#define PULUBYTE(b) (*rd_u_handler_b)(&b)
#define PULUWORD(w) (*rd_u_handler_w)(&w)

#define CLR_HNZVC	CC&=~(CC_H|CC_N|CC_Z|CC_V|CC_C)
#define CLR_NZV 	CC&=~(CC_N|CC_Z|CC_V)
#define CLR_HNZC	CC&=~(CC_H|CC_N|CC_Z|CC_C)
#define CLR_NZVC	CC&=~(CC_N|CC_Z|CC_V|CC_C)
#define CLR_Z		CC&=~(CC_Z)
#define CLR_NZC 	CC&=~(CC_N|CC_Z|CC_C)
#define CLR_ZC		CC&=~(CC_Z|CC_C)

/* macros for CC -- CC bits affected should be reset before calling */
#define SET_Z(a)		if(!a)SEZ
#define SET_Z8(a)		SET_Z((UINT8)a)
#define SET_Z16(a)		SET_Z((UINT16)a)
#define SET_N8(a)		CC|=((a&0x80)>>4)
#define SET_N16(a)		CC|=((a&0x8000)>>12)
#define SET_H(a,b,r)	CC|=(((a^b^r)&0x10)<<1)
#define SET_C8(a)		CC|=((a&0x100)>>8)
#define SET_C16(a)		CC|=((a&0x10000)>>16)
#define SET_V8(a,b,r)	CC|=(((a^b^r^(r>>1))&0x80)>>6)
#define SET_V16(a,b,r)	CC|=(((a^b^r^(r>>1))&0x8000)>>14)

static UINT8 flags8i[256]=	 /* increment */
{
CC_Z,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
CC_N|CC_V,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N
};
static UINT8 flags8d[256]= /* decrement */
{
CC_Z,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,CC_V,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,
CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N,CC_N
};
#define SET_FLAGS8I(a)		{CC|=flags8i[(a)&0xff];}
#define SET_FLAGS8D(a)		{CC|=flags8d[(a)&0xff];}

/* combos */
#define SET_NZ8(a)			{SET_N8(a);SET_Z(a);}
#define SET_NZ16(a)			{SET_N16(a);SET_Z(a);}
#define SET_FLAGS8(a,b,r)	{SET_N8(r);SET_Z8(r);SET_V8(a,b,r);SET_C8(r);}
#define SET_FLAGS16(a,b,r)	{SET_N16(r);SET_Z16(r);SET_V16(a,b,r);SET_C16(r);}

/* for treating an unsigned byte as a signed word */
#define SIGNED(b) ((UINT16)(b&0x80?b|0xff00:b))

/* macros for addressing modes (postbytes have their own code) */
#define DIRECT { ea.d=0; IMMBYTE(ea.b.l); ea.b.h=DP; }
#define IMM8 EA=PC++
#define IMM16 { EA=PC; PC+=2; }
#define EXTENDED IMMWORD(ea)

/* macros to set status flags */
#define SEC CC|=CC_C
#define CLC CC&=~CC_C
#define SEZ CC|=CC_Z
#define CLZ CC&=~CC_Z
#define SEN CC|=CC_N
#define CLN CC&=~CC_N
#define SEV CC|=CC_V
#define CLV CC&=~CC_V
#define SEH CC|=CC_H
#define CLH CC&=~CC_H

/* macros for convenience */
#define DIRBYTE(b) {DIRECT;b=RM(EAD);}
#define DIRWORD(w) {DIRECT;RM16(EAD,&w);}
#define EXTBYTE(b) {EXTENDED;b=RM(EAD);}
#define EXTWORD(w) {EXTENDED;RM16(EAD,&w);}

/* macros for branch instructions */
#define BRANCH(f) { 					\
	UINT8 t;							\
	IMMBYTE(t); 						\
	if( f ) 							\
	{									\
		PC += SIGNED(t);				\
		change_pc(PC);	/* TS 971002 */ \
	}									\
}

#define LBRANCH(f) {                    \
	PAIR t; 							\
	IMMWORD(t); 						\
	if( f ) 							\
	{									\
		konami_ICount -= 1;				\
		PC += t.w.l;					\
		change_pc(PC);	/* TS 971002 */ \
	}									\
}

#define NXORV  ((CC&CC_N)^((CC&CC_V)<<2))

/* macros for setting/getting registers in TFR/EXG instructions */
#define GETREG(val,reg) 				\
	switch(reg) {						\
	case 0: val = A;	break;			\
	case 1: val = B; 	break; 			\
	case 2: val = X; 	break;			\
	case 3: val = Y;	break; 			\
	case 4: val = S; 	break; /* ? */	\
	case 5: val = U;	break;			\
	default: val = 0xff; if ( errorlog ) fprintf( errorlog, "Unknown TFR/EXG idx at PC:%04x\n", PC ); break; \
}

#define SETREG(val,reg) 				\
	switch(reg) {						\
	case 0: A = val;	break;			\
	case 1: B = val;	break;			\
	case 2: X = val; 	break;			\
	case 3: Y = val;	break;			\
	case 4: S = val;	break; /* ? */	\
	case 5: U = val; 	break;			\
	default: if ( errorlog ) fprintf( errorlog, "Unknown TFR/EXG idx at PC:%04x\n", PC ); break; \
}

#define E   0x80			/* 0x80 = need to process postbyte to figure out addressing */
/* opcode timings */
static UINT8 cycles1[] =
{
	/*    0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F */
  /*0*/	  1,  1,  1,  1,  1,  1,  1,  1,E+4,E+4,E+4,E+4,  5,  5,  5,  5,
  /*1*/	  2,  2,E+2,E+2,  2,  2,E+2,E+2,  2,  2,E+2,E+2,  2,  2,E+2,E+2,
  /*2*/	  2,  2,E+2,E+2,  2,  2,E+2,E+2,  2,  2,E+2,E+2,  2,  2,E+2,E+2,
  /*3*/   2,  2,E+2,E+2,  2,  2,E+2,E+2,  2,E+2,E+2,E+2,  3,  3,  7,  6,
  /*4*/	  3,E+3,  3,E+3,  3,E+3,  3,E+3,  3,E+3,  4,E+4,  3,E+3,  4,E+4,
  /*5*/	  4,E+4,  4,E+4,  4,E+4,  4,E+4,E+3,E+3,E+3,E+3,E+3,  1,  1,  1,
  /*6*/	  3,  3,  3,  3,  3,  3,  3,  3,  5,  5,  5,  5,  5,  5,  5,  5,
  /*7*/	  3,  3,  3,  3,  3,  3,  3,  3,  5,  5,  5,  5,  5,  5,  5,  5,
  /*8*/	  2,  2,E+2,  2,  2,E+2,  2,  2,E+2,  2,  2,E+2,  2,  2,E+2,  5,
  /*9*/	  2,  2,E+2,  2,  2,E+2,  2,  2,E+2,  2,  2,E+2,  2,  2,E+2,  6,
  /*A*/	  2,  2,E+2,E+4,E+4,E+4,E+4,E+4,E+2,E+2,  2,  2,  3,  3,  2,  1,
  /*B*/	  3,  2,  2, 11, 22, 11,  2,  4,  3,E+3,  3,E+3,  3,E+3,  3,E+3,
  /*C*/	  3,E+3,  3,E+3,  3,E+3,  3,E+3,  3,E+3,  3,E+3,  2,  2,  3,  2,
  /*D*/	  2,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
  /*E*/	  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
  /*F*/	  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1
};
#undef E


static void rd_s_slow_b( UINT8 *b )
{
	*b = RM( konami.s.d );
	konami.s.w.l++;
}

static void rd_s_slow_w( PAIR *p )
{
	CLEAR_PAIR(p);
    p->b.h = RM( konami.s.d );
    konami.s.w.l++;
	p->b.l = RM( konami.s.d );
    konami.s.w.l++;
}

static void rd_s_fast_b( UINT8 *b )
{
	extern UINT8 *RAM;

	*b = RAM[ konami.s.d ];
    konami.s.w.l++;
}

static void rd_s_fast_w( PAIR *p )
{
	extern UINT8 *RAM;

	CLEAR_PAIR(p);
    p->b.h = RAM[ konami.s.d ];
    konami.s.w.l++;
	p->b.l = RAM[ konami.s.d ];
    konami.s.w.l++;
}

static void wr_s_slow_b( UINT8 *b )
{
	--konami.s.w.l;
	WM( konami.s.d, *b );
}

static void wr_s_slow_w( PAIR *p )
{
	--konami.s.w.l;
	WM( konami.s.d, p->b.l );
	--konami.s.w.l;
	WM( konami.s.d, p->b.h );
}

static void wr_s_fast_b( UINT8 *b )
{
	extern UINT8 *RAM;

	--konami.s.w.l;
	RAM[ konami.s.d ] = *b;
}

static void wr_s_fast_w( PAIR *p )
{
	extern UINT8 *RAM;

	--konami.s.w.l;
	RAM[ konami.s.d ] = p->b.l;
	--konami.s.w.l;
	RAM[ konami.s.d ] = p->b.h;
}

static void rd_u_slow_b( UINT8 *b )
{
	*b = RM( konami.u.d );
	konami.u.w.l++;
}

static void rd_u_slow_w( PAIR *p )
{
	CLEAR_PAIR(p);
    p->b.h = RM( konami.u.d );
	konami.u.w.l++;
	p->b.l = RM( konami.u.d );
	konami.u.w.l++;
}

static void rd_u_fast_b( UINT8 *b )
{
	extern UINT8 *RAM;

	*b = RAM[ konami.u.d ];
	konami.u.w.l++;
}

static void rd_u_fast_w( PAIR *p )
{
	extern UINT8 *RAM;

	CLEAR_PAIR(p);
    p->b.h = RAM[ konami.u.d ];
	konami.u.w.l++;
	p->b.l = RAM[ konami.u.d ];
	konami.u.w.l++;
}

static void wr_u_slow_b( UINT8 *b )
{
	--konami.u.w.l;
	WM( konami.u.d, *b );
}

static void wr_u_slow_w( PAIR *p )
{
	--konami.u.w.l;
	WM( konami.u.d, p->b.l );
	--konami.u.w.l;
	WM( konami.u.d, p->b.h );
}

static void wr_u_fast_b( UINT8 *b )
{
	extern UINT8 *RAM;

	--konami.u.w.l;
	RAM[ konami.u.d ] = *b;
}

static void wr_u_fast_w( PAIR *p )
{
	extern UINT8 *RAM;

	--konami.u.w.l;
	RAM[ konami.u.d ] = p->b.l;
	--konami.u.w.l;
	RAM[ konami.u.d ] = p->b.h;
}

INLINE void RM16( UINT32 Addr, PAIR *p )
{
	CLEAR_PAIR(p);
    p->b.h = RM(Addr);
	if( ++Addr > 0xffff ) Addr = 0;
	p->b.l = RM(Addr);
}

INLINE void WM16( UINT32 Addr, PAIR *p )
{
	WM( Addr, p->b.h );
	if( ++Addr > 0xffff ) Addr = 0;
	WM( Addr, p->b.l );
}

/****************************************************************************
 * Get all registers in given buffer
 ****************************************************************************/
unsigned konami_get_context(void *dst)
{
	if( dst )
		*(konami_Regs*)dst = konami;
	return sizeof(konami_Regs);
}

/****************************************************************************
 * Set all registers to given values
 ****************************************************************************/
void konami_set_context(void *src)
{
	if( src )
		konami = *(konami_Regs*)src;
    change_pc(PC);    /* TS 971002 */

    CHECK_IRQ_LINES;
}

/****************************************************************************
 * Return program counter
 ****************************************************************************/
unsigned konami_get_pc(void)
{
	return PC;
}


/****************************************************************************
 * Set program counter
 ****************************************************************************/
void konami_set_pc(unsigned val)
{
	PC = val;
	change_pc(PC);
}


/****************************************************************************
 * Return stack pointer
 ****************************************************************************/
unsigned konami_get_sp(void)
{
	return S;
}


/****************************************************************************
 * Set stack pointer
 ****************************************************************************/
void konami_set_sp(unsigned val)
{
	S = val;
}


/****************************************************************************/
/* Return a specific register                                               */
/****************************************************************************/
unsigned konami_get_reg(int regnum)
{
	switch( regnum )
	{
		case KONAMI_PC: return PC;
		case KONAMI_S: return S;
		case KONAMI_CC: return CC;
		case KONAMI_U: return U;
		case KONAMI_A: return A;
		case KONAMI_B: return B;
		case KONAMI_X: return X;
		case KONAMI_Y: return Y;
		case KONAMI_DP: return DP;
		case KONAMI_NMI_STATE: return konami.nmi_state;
		case KONAMI_IRQ_STATE: return konami.irq_state[KONAMI_IRQ_LINE];
		case KONAMI_FIRQ_STATE: return konami.irq_state[KONAMI_FIRQ_LINE];
		case REG_PREVIOUSPC: return PPC;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = S + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xffff )
					return ( RM( offset ) << 8 ) | RM( offset + 1 );
			}
	}
	return 0;
}


/****************************************************************************/
/* Set a specific register                                                  */
/****************************************************************************/
void konami_set_reg(int regnum, unsigned val)
{
	switch( regnum )
	{
		case KONAMI_PC: PC = val; change_pc(PC); break;
		case KONAMI_S: S = val; break;
		case KONAMI_CC: CC = val; CHECK_IRQ_LINES; break;
		case KONAMI_U: U = val; break;
		case KONAMI_A: A = val; break;
		case KONAMI_B: B = val; break;
		case KONAMI_X: X = val; break;
		case KONAMI_Y: Y = val; break;
		case KONAMI_DP: DP = val; break;
		case KONAMI_NMI_STATE: konami.nmi_state = val; break;
		case KONAMI_IRQ_STATE: konami.irq_state[KONAMI_IRQ_LINE] = val; break;
		case KONAMI_FIRQ_STATE: konami.irq_state[KONAMI_FIRQ_LINE] = val; break;
		default:
			if( regnum <= REG_SP_CONTENTS )
			{
				unsigned offset = S + 2 * (REG_SP_CONTENTS - regnum);
				if( offset < 0xffff )
				{
					WM( offset, (val >> 8) & 0xff );
					WM( offset+1, val & 0xff );
				}
			}
    }
}


/****************************************************************************/
/* Reset registers to their initial values									*/
/****************************************************************************/
void konami_reset(void *param)
{
	/* default to unoptimized memory access */
	rd_u_handler_b = rd_u_slow_b;
	rd_u_handler_w = rd_u_slow_w;
	rd_s_handler_b = rd_s_slow_b;
	rd_s_handler_w = rd_s_slow_w;
	wr_u_handler_b = wr_u_slow_b;
	wr_u_handler_w = wr_u_slow_w;
	wr_s_handler_b = wr_s_slow_b;
	wr_s_handler_w = wr_s_slow_w;

	konami.int_state = 0;
	konami.nmi_state = CLEAR_LINE;
	konami.irq_state[0] = CLEAR_LINE;
	konami.irq_state[0] = CLEAR_LINE;

	DP = 0; 			/* Reset direct page register */

    CC |= CC_II;        /* IRQ disabled */
    CC |= CC_IF;        /* FIRQ disabled */

    RM16(0xfffe,&konami.pc);
    change_pc(PC);    /* TS 971002 */

	/* optimize memory access according to flags */
	if( konami_Flags & KONAMI_FAST_U )
	{
		rd_u_handler_b = rd_u_fast_b;
		rd_u_handler_w = rd_u_fast_w;
		wr_u_handler_b = wr_u_fast_b;
		wr_u_handler_w = wr_u_fast_w;
	}
	if( konami_Flags & KONAMI_FAST_S )
	{
		rd_s_handler_b = rd_s_fast_b;
		rd_s_handler_w = rd_s_fast_w;
		wr_s_handler_b = wr_s_fast_b;
		wr_s_handler_w = wr_s_fast_w;
	}
}

void konami_exit(void)
{
	/* just make sure we deinit this, so the next game set its own */
	konami_cpu_setlines_callback = 0;
}

/* Generate interrupts */
/****************************************************************************
 * Set NMI line state
 ****************************************************************************/
void konami_set_nmi_line(int state)
{
	if (konami.nmi_state == state) return;
	konami.nmi_state = state;
	LOG((errorlog, "KONAMI#%d set_nmi_line %d\n", cpu_getactivecpu(), state));
	if( state == CLEAR_LINE ) return;

	/* if the stack was not yet initialized */
    if( !(konami.int_state & KONAMI_LDS) ) return;

    konami.int_state &= ~KONAMI_SYNC;
	/* HJB 990225: state already saved by CWAI? */
	if( konami.int_state & KONAMI_CWAI )
	{
		konami.int_state &= ~KONAMI_CWAI;
		konami.extra_cycles += 7;	/* subtract +7 cycles next time */
    }
	else
	{
		CC |= CC_E; 				/* save entire state */
		PUSHWORD(pPC);
		PUSHWORD(pU);
		PUSHWORD(pY);
		PUSHWORD(pX);
		PUSHBYTE(DP);
		PUSHBYTE(B);
		PUSHBYTE(A);
		PUSHBYTE(CC);
		konami.extra_cycles += 19;	/* subtract +19 cycles next time */
	}
	CC |= CC_IF | CC_II;			/* inhibit FIRQ and IRQ */
	RM16(0xfffc,&konami.pc);
	change_pc(PC);					/* TS 971002 */
}

/****************************************************************************
 * Set IRQ line state
 ****************************************************************************/
void konami_set_irq_line(int irqline, int state)
{
    LOG((errorlog, "KONAMI#%d set_irq_line %d, %d\n", cpu_getactivecpu(), irqline, state));
	konami.irq_state[irqline] = state;
	if (state == CLEAR_LINE) return;
	CHECK_IRQ_LINES;
}

/****************************************************************************
 * Set IRQ vector callback
 ****************************************************************************/
void konami_set_irq_callback(int (*callback)(int irqline))
{
	konami.irq_callback = callback;
}

/****************************************************************************
 * Save CPU state
 ****************************************************************************/
static void state_save(void *file, const char *module)
{
	int cpu = cpu_getactivecpu();
	state_save_UINT16(file, module, cpu, "PC", &PC, 1);
	state_save_UINT16(file, module, cpu, "U", &U, 1);
	state_save_UINT16(file, module, cpu, "S", &S, 1);
	state_save_UINT16(file, module, cpu, "X", &X, 1);
	state_save_UINT16(file, module, cpu, "Y", &Y, 1);
	state_save_UINT8(file, module, cpu, "DP", &DP, 1);
	state_save_UINT8(file, module, cpu, "CC", &CC, 1);
	state_save_UINT8(file, module, cpu, "INT", &konami.int_state, 1);
	state_save_UINT8(file, module, cpu, "NMI", &konami.nmi_state, 1);
	state_save_UINT8(file, module, cpu, "IRQ", &konami.irq_state[0], 1);
	state_save_UINT8(file, module, cpu, "FIRQ", &konami.irq_state[1], 1);
}

/****************************************************************************
 * Load CPU state
 ****************************************************************************/
static void state_load(void *file, const char *module)
{
	int cpu = cpu_getactivecpu();
	state_load_UINT16(file, module, cpu, "PC", &PC, 1);
	state_load_UINT16(file, module, cpu, "U", &U, 1);
	state_load_UINT16(file, module, cpu, "S", &S, 1);
	state_load_UINT16(file, module, cpu, "X", &X, 1);
	state_load_UINT16(file, module, cpu, "Y", &Y, 1);
	state_load_UINT8(file, module, cpu, "DP", &DP, 1);
	state_load_UINT8(file, module, cpu, "CC", &CC, 1);
	state_load_UINT8(file, module, cpu, "INT", &konami.int_state, 1);
	state_load_UINT8(file, module, cpu, "NMI", &konami.nmi_state, 1);
	state_load_UINT8(file, module, cpu, "IRQ", &konami.irq_state[0], 1);
	state_load_UINT8(file, module, cpu, "FIRQ", &konami.irq_state[1], 1);
}

void konami_state_save(void *file) { state_save(file, "konami"); }
void konami_state_load(void *file) { state_load(file, "konami"); }

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
const char *konami_info(void *context, int regnum)
{
	static char buffer[16][47+1];
	static int which = 0;
	konami_Regs *r = context;

	which = ++which % 16;
    buffer[which][0] = '\0';
	if( !context )
		r = &konami;

	switch( regnum )
	{
		case CPU_INFO_NAME: return "KONAMI";
		case CPU_INFO_FAMILY: return "KONAMI 5000x";
		case CPU_INFO_VERSION: return "1.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "Copyright (C) The MAME Team 1999";
		case CPU_INFO_REG_LAYOUT: return (const char*)konami_reg_layout;
		case CPU_INFO_WIN_LAYOUT: return (const char*)konami_win_layout;

		case CPU_INFO_FLAGS:
			sprintf(buffer[which], "%c%c%c%c%c%c%c%c",
				r->cc & 0x80 ? 'E':'.',
				r->cc & 0x40 ? 'F':'.',
                r->cc & 0x20 ? 'H':'.',
                r->cc & 0x10 ? 'I':'.',
                r->cc & 0x08 ? 'N':'.',
                r->cc & 0x04 ? 'Z':'.',
                r->cc & 0x02 ? 'V':'.',
                r->cc & 0x01 ? 'C':'.');
            break;
		case CPU_INFO_REG+KONAMI_PC: sprintf(buffer[which], "PC:%04X", r->pc.w.l); break;
		case CPU_INFO_REG+KONAMI_S: sprintf(buffer[which], "S:%04X", r->s.w.l); break;
		case CPU_INFO_REG+KONAMI_CC: sprintf(buffer[which], "CC:%02X", r->cc); break;
		case CPU_INFO_REG+KONAMI_U: sprintf(buffer[which], "U:%04X", r->u.w.l); break;
		case CPU_INFO_REG+KONAMI_A: sprintf(buffer[which], "A:%02X", r->d.b.h); break;
		case CPU_INFO_REG+KONAMI_B: sprintf(buffer[which], "B:%02X", r->d.b.l); break;
		case CPU_INFO_REG+KONAMI_X: sprintf(buffer[which], "X:%04X", r->x.w.l); break;
		case CPU_INFO_REG+KONAMI_Y: sprintf(buffer[which], "Y:%04X", r->y.w.l); break;
		case CPU_INFO_REG+KONAMI_DP: sprintf(buffer[which], "DP:%02X", r->dp); break;
		case CPU_INFO_REG+KONAMI_NMI_STATE: sprintf(buffer[which], "NMI:%X", r->nmi_state); break;
		case CPU_INFO_REG+KONAMI_IRQ_STATE: sprintf(buffer[which], "IRQ:%X", r->irq_state[KONAMI_IRQ_LINE]); break;
		case CPU_INFO_REG+KONAMI_FIRQ_STATE: sprintf(buffer[which], "FIRQ:%X", r->irq_state[KONAMI_FIRQ_LINE]); break;
	}
	return buffer[which];
}

unsigned konami_dasm(char *buffer, unsigned pc)
{
#ifdef MAME_DEBUG
    return Dasmknmi(buffer,pc);
#else
	sprintf( buffer, "$%02X", cpu_readop(pc) );
	return 1;
#endif
}

/* includes the static function prototypes and the master opcode table */
#include "konamtbl.c"

/* includes the actual opcode implementations */
#include "konamops.c"

/* execute instructions on this CPU until icount expires */
int konami_execute(int cycles)	/* NS 970908 */
{
	UINT8 op_count;  /* op code clock count */
	UINT8 ireg;
	konami_ICount = cycles;	/* NS 970908 */

	/* HJB 990226: subtract additional cycles for interrupts */
	konami_ICount -= konami.extra_cycles;
	konami.extra_cycles = 0;

	if (konami.int_state & (KONAMI_CWAI | KONAMI_SYNC))
	{
		konami_ICount = 0;
		goto getout;
	}

	do
	{
		pPPC = pPC;

		CALL_MAME_DEBUG;

		ireg=M_RDOP(PC++);

		op_count = cycles1[ireg];

		if ( op_count & 0x80 ) {
			UINT8 ireg2;
			ireg2 = M_RDOP_ARG(PC++);

			switch ( ireg2 ) {
				case 0x07: /* extended */
					(*konami_extended[ireg])();
					op_count += 2;
				break;

				case 0xc4: /* direct page */
					(*konami_direct[ireg])();
					op_count++;
				break;

				default:
					op_count += fetch_effective_address( ireg2 );

					(*konami_indexed[ireg])();
				break;
			}
		} else
			(*konami_main[ireg])();

		konami_ICount -= op_count & 0x7f;

	} while( konami_ICount>0 );

getout:
	/* HJB 990226: subtract additional cycles for interrupts */
	konami_ICount -= konami.extra_cycles;
	konami.extra_cycles = 0;

    return cycles - konami_ICount;   /* NS 970908 */
}

INLINE UINT8 fetch_effective_address( UINT8 mode )
{
	UINT8 ec = 0;

	switch ( mode ) {
//	case 0x00: EA=0;												break; /* auto increment */
//	case 0x01: EA=0;												break; /* double auto increment */
//	case 0x02: EA=0;												break; /* auto decrement */
//	case 0x03: EA=0;												break; /* double auto decrement */
//	case 0x04: EA=0;												break; /* postbyte offs */
//	case 0x05: EA=0;												break; /* postword offs */
//	case 0x06: EA=0;												break; /* normal */
//	case 0x07: EA=0;												break; /* extended */
//	case 0x08: EA=0;												break; /* indirect - auto increment */
//	case 0x09: EA=0;												break; /* indirect - double auto increment */
//	case 0x0a: EA=0;												break; /* indirect - auto decrement */
//	case 0x0b: EA=0;												break; /* indirect - double auto decrement */
//	case 0x0c: EA=0;												break; /* indirect - postbyte offs */
//	case 0x0d: EA=0;												break; /* indirect - postword offs */
//	case 0x0e: EA=0;												break; /* indirect - normal */
	case 0x0f: IMMWORD(ea);					RM16(EAD,&ea);	ec=4;	break; /* indirect - extended */
//	case 0x10: EA=0;												break; /* auto increment */
//	case 0x11: EA=0;												break; /* double auto increment */
//	case 0x12: EA=0;												break; /* auto decrement */
//	case 0x13: EA=0;												break; /* double auto decrement */
//	case 0x14: EA=0;												break; /* postbyte offs */
//	case 0x15: EA=0;												break; /* postword offs */
//	case 0x16: EA=0;												break; /* normal */
//	case 0x17: EA=0;												break; /* extended */
//	case 0x18: EA=0;												break; /* indirect - auto increment */
//	case 0x19: EA=0;												break; /* indirect - double auto increment */
//	case 0x1a: EA=0;												break; /* indirect - auto decrement */
//	case 0x1b: EA=0;												break; /* indirect - double auto decrement */
//	case 0x1c: EA=0;												break; /* indirect - postbyte offs */
//	case 0x1d: EA=0;												break; /* indirect - postword offs */
//	case 0x1e: EA=0;												break; /* indirect - normal */
//	case 0x1f: EA=0;												break; /* indirect - extended */
	case 0x20: EA=X;	X++;								ec=2;	break; /* auto increment */
	case 0x21: EA=X;	X+=2;								ec=3;	break; /* double auto increment */
	case 0x22: X--;		EA=X;								ec=2;	break; /* auto decrement */
	case 0x23: X-=2;	EA=X;								ec=3;	break; /* double auto decrement */
	case 0x24: IMMBYTE(EA); 	EA=X+SIGNED(EA);			ec=2;	break; /* postbyte offs */
	case 0x25: IMMWORD(ea); 	EA+=X;						ec=4;	break; /* postword offs */
	case 0x26: EA=X;												break; /* normal */
//	case 0x27: EA=0;												break; /* extended */
	case 0x28: EA=X;	X++;				RM16(EAD,&ea);	ec=5;	break; /* indirect - auto increment */
	case 0x29: EA=X;	X+=2;				RM16(EAD,&ea);	ec=6;	break; /* indirect - double auto increment */
	case 0x2a: X--;		EA=X;				RM16(EAD,&ea);	ec=5;	break; /* indirect - auto decrement */
	case 0x2b: X-=2;	EA=X;				RM16(EAD,&ea);	ec=6;	break; /* indirect - double auto decrement */
	case 0x2c: IMMBYTE(EA); EA=X+SIGNED(EA);RM16(EAD,&ea);	ec=4;	break; /* indirect - postbyte offs */
	case 0x2d: IMMWORD(ea); EA+=X;			RM16(EAD,&ea);	ec=7;	break; /* indirect - postword offs */
	case 0x2e: EA=X;						RM16(EAD,&ea);	ec=3;	break; /* indirect - normal */
//	case 0x2f: EA=0;												break; /* indirect - extended */
	case 0x30: EA=Y;	Y++;								ec=2;	break; /* auto increment */
	case 0x31: EA=Y;	Y+=2;								ec=3;	break; /* double auto increment */
	case 0x32: Y--;		EA=Y;								ec=2;	break; /* auto decrement */
	case 0x33: Y-=2;	EA=Y;								ec=3;	break; /* double auto decrement */
	case 0x34: IMMBYTE(EA); 	EA=Y+SIGNED(EA);			ec=2;	break; /* postbyte offs */
	case 0x35: IMMWORD(ea); 	EA+=Y;						ec=4;	break; /* postword offs */
	case 0x36: EA=Y;												break; /* normal */
//	case 0x37: EA=0;												break; /* extended */
	case 0x38: EA=Y;	Y++;				RM16(EAD,&ea);	ec=5;	break; /* indirect - auto increment */
	case 0x39: EA=Y;	Y+=2;				RM16(EAD,&ea);	ec=6;	break; /* indirect - double auto increment */
	case 0x3a: Y--;		EA=Y;				RM16(EAD,&ea);	ec=5;	break; /* indirect - auto decrement */
	case 0x3b: Y-=2;	EA=Y;				RM16(EAD,&ea);	ec=6;	break; /* indirect - double auto decrement */
	case 0x3c: IMMBYTE(EA); EA=Y+SIGNED(EA);RM16(EAD,&ea);	ec=4;	break; /* indirect - postbyte offs */
	case 0x3d: IMMWORD(ea); EA+=Y;			RM16(EAD,&ea);	ec=7;	break; /* indirect - postword offs */
	case 0x3e: EA=Y;						RM16(EAD,&ea);	ec=3;	break; /* indirect - normal */
//	case 0x3f: EA=0;												break; /* indirect - extended */
//	case 0x40: EA=0;												break; /* auto increment */
//	case 0x41: EA=0;												break; /* double auto increment */
//	case 0x42: EA=0;												break; /* auto decrement */
//	case 0x43: EA=0;												break; /* double auto decrement */
//	case 0x44: EA=0;												break; /* postbyte offs */
//	case 0x45: EA=0;												break; /* postword offs */
//	case 0x46: EA=0;												break; /* normal */
//	case 0x47: EA=0;												break; /* extended */
//	case 0x48: EA=0;												break; /* indirect - auto increment */
//	case 0x49: EA=0;												break; /* indirect - double auto increment */
//	case 0x4a: EA=0;												break; /* indirect - auto decrement */
//	case 0x4b: EA=0;												break; /* indirect - double auto decrement */
//	case 0x4c: EA=0;												break; /* indirect - postbyte offs */
//	case 0x4d: EA=0;												break; /* indirect - postword offs */
//	case 0x4e: EA=0;												break; /* indirect - normal */
//	case 0x4f: EA=0;												break; /* indirect - extended */
	case 0x50: EA=U;	U++;								ec=2;	break; /* auto increment */
	case 0x51: EA=U;	U+=2;								ec=3;	break; /* double auto increment */
	case 0x52: U--;		EA=U;								ec=2;	break; /* auto decrement */
	case 0x53: U-=2;	EA=U;								ec=3;	break; /* double auto decrement */
	case 0x54: IMMBYTE(EA); 	EA=U+SIGNED(EA);			ec=2;	break; /* postbyte offs */
	case 0x55: IMMWORD(ea); 	EA+=U;						ec=4;	break; /* postword offs */
	case 0x56: EA=U;												break; /* normal */
//	case 0x57: EA=0;												break; /* extended */
	case 0x58: EA=U;	U++;				RM16(EAD,&ea);	ec=5;	break; /* indirect - auto increment */
	case 0x59: EA=U;	U+=2;				RM16(EAD,&ea);	ec=6;	break; /* indirect - double auto increment */
	case 0x5a: U--;		EA=U;				RM16(EAD,&ea);	ec=5;	break; /* indirect - auto decrement */
	case 0x5b: U-=2;	EA=U;				RM16(EAD,&ea);	ec=6;	break; /* indirect - double auto decrement */
	case 0x5c: IMMBYTE(EA); EA=U+SIGNED(EA);RM16(EAD,&ea);	ec=4;	break; /* indirect - postbyte offs */
	case 0x5d: IMMWORD(ea); EA+=U;			RM16(EAD,&ea);	ec=7;	break; /* indirect - postword offs */
	case 0x5e: EA=U;						RM16(EAD,&ea);	ec=3;	break; /* indirect - normal */
//	case 0x5f: EA=0;												break; /* indirect - extended */
	case 0x60: EA=S;	S++;								ec=2;	break; /* auto increment */
	case 0x61: EA=S;	S+=2;								ec=3;	break; /* double auto increment */
	case 0x62: S--;		EA=S;								ec=2;	break; /* auto decrement */
	case 0x63: S-=2;	EA=S;								ec=3;	break; /* double auto decrement */
	case 0x64: IMMBYTE(EA); 	EA=S+SIGNED(EA);			ec=2;	break; /* postbyte offs */
	case 0x65: IMMWORD(ea); 	EA+=S;						ec=4;	break; /* postword offs */
	case 0x66: EA=S;												break; /* normal */
//	case 0x67: EA=0;												break; /* extended */
	case 0x68: EA=S;	S++;				RM16(EAD,&ea);	ec=5;	break; /* indirect - auto increment */
	case 0x69: EA=S;	S+=2;				RM16(EAD,&ea);	ec=6;	break; /* indirect - double auto increment */
	case 0x6a: S--;		EA=S;				RM16(EAD,&ea);	ec=5;	break; /* indirect - auto decrement */
	case 0x6b: S-=2;	EA=S;				RM16(EAD,&ea);	ec=6;	break; /* indirect - double auto decrement */
	case 0x6c: IMMBYTE(EA); EA=S+SIGNED(EA);RM16(EAD,&ea);	ec=4;	break; /* indirect - postbyte offs */
	case 0x6d: IMMWORD(ea); EA+=S;			RM16(EAD,&ea);	ec=7;	break; /* indirect - postword offs */
	case 0x6e: EA=S;						RM16(EAD,&ea);	ec=3;	break; /* indirect - normal */
//	case 0x6f: EA=0;												break; /* indirect - extended */
	case 0x70: EA=PC;	PC++;								ec=2;	break; /* auto increment */
	case 0x71: EA=PC;	PC+=2;								ec=3;	break; /* double auto increment */
	case 0x72: PC--;	EA=PC;								ec=2;	break; /* auto decrement */
	case 0x73: PC-=2;	EA=PC;								ec=3;	break; /* double auto decrement */
	case 0x74: IMMBYTE(EA); 	EA=PC-1+SIGNED(EA);			ec=2;	break; /* postbyte offs */
	case 0x75: IMMWORD(ea); 	EA+=PC-2;					ec=4;	break; /* postword offs */
	case 0x76: EA=PC;												break; /* normal */
//	case 0x77: EA=0;												break; /* extended */
	case 0x78: EA=PC;	PC++;				RM16(EAD,&ea);	ec=5;	break; /* indirect - auto increment */
	case 0x79: EA=PC;	PC+=2;				RM16(EAD,&ea);	ec=6;	break; /* indirect - double auto increment */
	case 0x7a: PC--;	EA=PC;				RM16(EAD,&ea);	ec=5;	break; /* indirect - auto decrement */
	case 0x7b: PC-=2;	EA=PC;				RM16(EAD,&ea);	ec=6;	break; /* indirect - double auto decrement */
	case 0x7c: IMMBYTE(EA); EA=PC-1+SIGNED(EA);RM16(EAD,&ea);ec=4;	break; /* indirect - postbyte offs */
	case 0x7d: IMMWORD(ea); EA+=PC-2;		RM16(EAD,&ea);	ec=7;	break; /* indirect - postword offs */
	case 0x7e: EA=PC;						RM16(EAD,&ea);	ec=3;	break; /* indirect - normal */
//	case 0x7f: EA=0;												break; /* indirect - extended */
//	case 0x80: EA=0;												break; /* register a */
//	case 0x81: EA=0;												break; /* register b */
//	case 0x82: EA=0;												break; /* ???? */
//	case 0x83: EA=0;												break; /* ???? */
//	case 0x84: EA=0;												break; /* ???? */
//	case 0x85: EA=0;												break; /* ???? */
//	case 0x86: EA=0;												break; /* ???? */
//	case 0x87: EA=0;												break; /* register d */
//	case 0x88: EA=0;												break; /* indirect - register a */
//	case 0x89: EA=0;												break; /* indirect - register b */
//	case 0x8a: EA=0;												break; /* indirect - ???? */
//	case 0x8b: EA=0;												break; /* indirect - ???? */
//	case 0x8c: EA=0;												break; /* indirect - ???? */
//	case 0x8d: EA=0;												break; /* indirect - ???? */
//	case 0x8e: EA=0;												break; /* indirect - register d */
//	case 0x8f: EA=0;												break; /* indirect - ???? */
//	case 0x90: EA=0;												break; /* register a */
//	case 0x91: EA=0;												break; /* register b */
//	case 0x92: EA=0;												break; /* ???? */
//	case 0x93: EA=0;												break; /* ???? */
//	case 0x94: EA=0;												break; /* ???? */
//	case 0x95: EA=0;												break; /* ???? */
//	case 0x96: EA=0;												break; /* ???? */
//	case 0x97: EA=0;												break; /* register d */
//	case 0x98: EA=0;												break; /* indirect - register a */
//	case 0x99: EA=0;												break; /* indirect - register b */
//	case 0x9a: EA=0;												break; /* indirect - ???? */
//	case 0x9b: EA=0;												break; /* indirect - ???? */
//	case 0x9c: EA=0;												break; /* indirect - ???? */
//	case 0x9d: EA=0;												break; /* indirect - ???? */
//	case 0x9e: EA=0;												break; /* indirect - register d */
//	case 0x9f: EA=0;												break; /* indirect - ???? */
	case 0xa0: EA=X+SIGNED(A);								ec=1;	break; /* register a */
	case 0xa1: EA=X+SIGNED(B);								ec=1;	break; /* register b */
//	case 0xa2: EA=0;												break; /* ???? */
//	case 0xa3: EA=0;												break; /* ???? */
//	case 0xa4: EA=0;												break; /* ???? */
//	case 0xa5: EA=0;												break; /* ???? */
//	case 0xa6: EA=0;										ec=4;	break; /* ???? */
	case 0xa7: EA=X+D;										ec=4;	break; /* register d */
	case 0xa8: EA=X+SIGNED(A);				RM16(EAD,&ea);	ec=4;	break; /* indirect - register a */
	case 0xa9: EA=X+SIGNED(B);				RM16(EAD,&ea);	ec=4;	break; /* indirect - register b */
//	case 0xaa: EA=0;												break; /* indirect - ???? */
//	case 0xab: EA=0;												break; /* indirect - ???? */
//	case 0xac: EA=0;												break; /* indirect - ???? */
//	case 0xad: EA=0;												break; /* indirect - ???? */
//	case 0xae: EA=0;												break; /* indirect - ???? */
	case 0xaf: EA=X+D;						RM16(EAD,&ea);	ec=7;	break; /* indirect - register d */
	case 0xb0: EA=Y+SIGNED(A);								ec=1;	break; /* register a */
	case 0xb1: EA=Y+SIGNED(B);								ec=1;	break; /* register b */
//	case 0xb2: EA=0;												break; /* ???? */
//	case 0xb3: EA=0;												break; /* ???? */
//	case 0xb4: EA=0;												break; /* ???? */
//	case 0xb5: EA=0;												break; /* ???? */
//	case 0xb6: EA=0;												break; /* ???? */
	case 0xb7: EA=Y+D;										ec=4;	break; /* register d */
	case 0xb8: EA=Y+SIGNED(A);				RM16(EAD,&ea);	ec=4;	break; /* indirect - register a */
	case 0xb9: EA=Y+SIGNED(B);				RM16(EAD,&ea);	ec=4;	break; /* indirect - register b */
//	case 0xba: EA=0;												break; /* indirect - ???? */
//	case 0xbb: EA=0;												break; /* indirect - ???? */
//	case 0xbc: EA=0;												break; /* indirect - ???? */
//	case 0xbd: EA=0;												break; /* indirect - ???? */
//	case 0xbe: EA=0;												break; /* indirect - ???? */
	case 0xbf: EA=Y+D;						RM16(EAD,&ea);	ec=7;	break; /* indirect - register d */
//	case 0xc0: EA=0;												break; /* register a */
//	case 0xc1: EA=0;												break; /* register b */
//	case 0xc2: EA=0;												break; /* ???? */
//	case 0xc3: EA=0;												break; /* ???? */
//	case 0xc4: EA=0;												break; /* ???? */
//	case 0xc5: EA=0;												break; /* ???? */
//	case 0xc6: EA=0;												break; /* ???? */
//	case 0xc7: EA=0;												break; /* register d */
//	case 0xc8: EA=0;												break; /* indirect - register a */
//	case 0xc9: EA=0;												break; /* indirect - register b */
//	case 0xca: EA=0;												break; /* indirect - ???? */
//	case 0xcb: EA=0;												break; /* indirect - ???? */
	case 0xcc: DIRWORD(ea);									ec=4;	break; /* indirect - direct */
//	case 0xcd: EA=0;												break; /* indirect - ???? */
//	case 0xce: EA=0;												break; /* indirect - register d */
//	case 0xcf: EA=0;												break; /* indirect - ???? */
	case 0xd0: EA=U+SIGNED(A);								ec=1;	break; /* register a */
	case 0xd1: EA=U+SIGNED(B);								ec=1;	break; /* register b */
//	case 0xd2: EA=0;												break; /* ???? */
//	case 0xd3: EA=0;												break; /* ???? */
//	case 0xd4: EA=0;												break; /* ???? */
//	case 0xd5: EA=0;												break; /* ???? */
//	case 0xd6: EA=0;												break; /* ???? */
	case 0xd7: EA=U+D;										ec=4;	break; /* register d */
	case 0xd8: EA=U+SIGNED(A);				RM16(EAD,&ea);	ec=4;	break; /* indirect - register a */
	case 0xd9: EA=U+SIGNED(B);				RM16(EAD,&ea);	ec=4;	break; /* indirect - register b */
//	case 0xda: EA=0;												break; /* indirect - ???? */
//	case 0xdb: EA=0;												break; /* indirect - ???? */
//	case 0xdc: EA=0;												break; /* indirect - ???? */
//	case 0xdd: EA=0;												break; /* indirect - ???? */
//	case 0xde: EA=0;												break; /* indirect - ???? */
	case 0xdf: EA=U+D;						RM16(EAD,&ea);	ec=7;	break; /* indirect - register d */
	case 0xe0: EA=S+SIGNED(A);								ec=1;	break; /* register a */
	case 0xe1: EA=S+SIGNED(B);								ec=1;	break; /* register b */
//	case 0xe2: EA=0;												break; /* ???? */
//	case 0xe3: EA=0;												break; /* ???? */
//	case 0xe4: EA=0;												break; /* ???? */
//	case 0xe5: EA=0;												break; /* ???? */
//	case 0xe6: EA=0;												break; /* ???? */
	case 0xe7: EA=S+D;										ec=4;	break; /* register d */
	case 0xe8: EA=S+SIGNED(A);				RM16(EAD,&ea);	ec=4;	break; /* indirect - register a */
	case 0xe9: EA=S+SIGNED(B);				RM16(EAD,&ea);	ec=4;	break; /* indirect - register b */
//	case 0xea: EA=0;												break; /* indirect - ???? */
//	case 0xeb: EA=0;												break; /* indirect - ???? */
//	case 0xec: EA=0;												break; /* indirect - ???? */
//	case 0xed: EA=0;												break; /* indirect - ???? */
//	case 0xee: EA=0;												break; /* indirect - ???? */
	case 0xef: EA=S+D;						RM16(EAD,&ea);	ec=7;	break; /* indirect - register d */
	case 0xf0: EA=PC+SIGNED(A);								ec=1;	break; /* register a */
	case 0xf1: EA=PC+SIGNED(B);								ec=1;	break; /* register b */
//	case 0xf2: EA=0;												break; /* ???? */
//	case 0xf3: EA=0;												break; /* ???? */
//	case 0xf4: EA=0;												break; /* ???? */
//	case 0xf5: EA=0;												break; /* ???? */
//	case 0xf6: EA=0;												break; /* ???? */
	case 0xf7: EA=PC+D;										ec=4;	break; /* register d */
	case 0xf8: EA=PC+SIGNED(A);				RM16(EAD,&ea);	ec=4;	break; /* indirect - register a */
	case 0xf9: EA=PC+SIGNED(B);				RM16(EAD,&ea);	ec=4;	break; /* indirect - register b */
//	case 0xfa: EA=0;												break; /* indirect - ???? */
//	case 0xfb: EA=0;												break; /* indirect - ???? */
//	case 0xfc: EA=0;												break; /* indirect - ???? */
//	case 0xfd: EA=0;												break; /* indirect - ???? */
//	case 0xfe: EA=0;												break; /* indirect - ???? */
	case 0xff: EA=PC+D;						RM16(EAD,&ea);	ec=7;	break; /* indirect - register d */
	default:
		if ( errorlog )
			fprintf( errorlog, "KONAMI: Unknown/Invalid postbyte at PC = %04x\n", PC -1 );
		EA = 0;
	break;
	}

	return (ec);
}
