/** M6502: portable 6502 emulator ****************************/
/**                                                         **/
/**                         M6502.c                         **/
/**                                                         **/
/** This file contains implementation for 6502 CPU. Don't   **/
/** forget to provide Rd6502(), Wr6502(), Loop6502(), and   **/
/** possibly Op6502() functions to accomodate the emulated  **/
/** machine's architecture.                                 **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996                      **/
/**               Alex Krasivsky  1996                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

/* Changed ADC/SBC to handle Flags in BCD mode correctly  BW 260797 */

#include "M6502.h"
#include "Tables.h"
#include <stdio.h>
#include "osd_dbg.h"
#include "driver.h"	/* NS 980212 */

/** INLINE ***************************************************/
/** Different compilers inline C functions differently.     **/
/*************************************************************/
#ifndef INLINE	/* ASG 980216 */
#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE static
#endif
#endif

/** System-Dependent Stuff ***********************************/
/** This is system-dependent code put here to speed things  **/
/** up. It has to stay inlined to be fast.                  **/
/*************************************************************/
#ifdef INES
#define FAST_RDOP
extern byte *Page[];
INLINE byte Op6502(register word A) { return(Page[A>>13][A&0x1FFF]); }
#endif

/** FAST_RDOP ************************************************/
/** With this #define not present, Rd6502() should perform  **/
/** the functions of Rd6502().                              **/
/*************************************************************/
#ifndef FAST_RDOP
#define Op6502(A) Rd6502(A)
#endif

/** Addressing Methods ***************************************/
/** These macros calculate and return effective addresses.  **/
/*************************************************************/
#define MC_Ab(Rg)	M_LDWORD(Rg)
#define MC_Zp(Rg)	Rg.B.l=Op6502(R->PC.W++);Rg.B.h=0
#define MC_Zx(Rg)	Rg.B.l=Op6502(R->PC.W++)+R->X;Rg.B.h=0
#define MC_Zy(Rg)	Rg.B.l=Op6502(R->PC.W++)+R->Y;Rg.B.h=0
#define MC_Ax(Rg)	M_LDWORD(Rg);Rg.W+=R->X
#define MC_Ay(Rg)	M_LDWORD(Rg);Rg.W+=R->Y
#define MC_Ix(Rg)	K.B.l=Op6502(R->PC.W++)+R->X;K.B.h=0; \
			Rg.B.l=Zr6502(K.W++);Rg.B.h=Zr6502(K.W) /* ASG 971210 -- changed from Op6502 to Zr6502 */
#define MC_Iy(Rg)	K.B.l=Op6502(R->PC.W++);K.B.h=0; \
			Rg.B.l=Zr6502(K.W++);Rg.B.h=Zr6502(K.W); /* ASG 971210 -- changed from Op6502 to Zr6502 */\
			Rg.W+=R->Y

/** Reading From Memory **************************************/
/** These macros calculate address and read from it.        **/
/*************************************************************/
#define MR_Ab(Rg)	MC_Ab(J);Rg=Rd6502(J.W)
#define MR_Im(Rg)	Rg=Op6502(R->PC.W++)
#define	MR_Zp(Rg)	MC_Zp(J);Rg=Rd6502(J.W)
#define MR_Zx(Rg)	MC_Zx(J);Rg=Rd6502(J.W)
#define MR_Zy(Rg)	MC_Zy(J);Rg=Rd6502(J.W)
#define	MR_Ax(Rg)	MC_Ax(J);Rg=Rd6502(J.W)
#define MR_Ay(Rg)	MC_Ay(J);Rg=Rd6502(J.W)
#define MR_Ix(Rg)	MC_Ix(J);Rg=Rd6502(J.W)
#define MR_Iy(Rg)	MC_Iy(J);Rg=Rd6502(J.W)

/** Writing To Memory ****************************************/
/** These macros calculate address and write to it.         **/
/*************************************************************/
#define MW_Ab(Rg)	MC_Ab(J);Wr6502(J.W,Rg)
#define MW_Zp(Rg)	MC_Zp(J);Wr6502(J.W,Rg)
#define MW_Zx(Rg)	MC_Zx(J);Wr6502(J.W,Rg)
#define MW_Zy(Rg)	MC_Zy(J);Wr6502(J.W,Rg)
#define MW_Ax(Rg)	MC_Ax(J);Wr6502(J.W,Rg)
#define MW_Ay(Rg)	MC_Ay(J);Wr6502(J.W,Rg)
#define MW_Ix(Rg)	MC_Ix(J);Wr6502(J.W,Rg)
#define MW_Iy(Rg)	MC_Iy(J);Wr6502(J.W,Rg)

/** Modifying Memory *****************************************/
/** These macros calculate address and modify it.           **/
/*************************************************************/
#define MM_Ab(Cmd)	MC_Ab(J);I=Rd6502(J.W);Cmd(I);Wr6502(J.W,I)
#define MM_Zp(Cmd)	MC_Zp(J);I=Rd6502(J.W);Cmd(I);Wr6502(J.W,I)
#define MM_Zx(Cmd)	MC_Zx(J);I=Rd6502(J.W);Cmd(I);Wr6502(J.W,I)
#define MM_Ax(Cmd)	MC_Ax(J);I=Rd6502(J.W);Cmd(I);Wr6502(J.W,I)

/** Other Macros *********************************************/
/** Calculating flags, stack, jumps, arithmetics, etc.      **/
/*************************************************************/
#define M_FL(Rg)	R->P=(R->P&~(Z_FLAG|N_FLAG))|ZNTable[Rg]
#define M_LDWORD(Rg)	Rg.B.l=Op6502(R->PC.W++);Rg.B.h=Op6502(R->PC.W++)

#define M_PUSH(Rg)	Wr6502(0x0100|R->S,Rg);R->S--
#define M_POP(Rg)	R->S++;Rg=Zr6502(0x0100|R->S)  /* ASG 971210 -- changed from Op6502 to Zr6502 */
#define M_JR		R->PC.W+=(signed char)Op6502(R->PC.W)+1;R->ICount--

#define M_ADC(Rg) \
  if(R->P&D_FLAG) \
  { \
    K.W=R->A+Rg+(R->P&C_FLAG); \
    R->P=(R->P&~Z_FLAG)|((K.W&0xff)? 0:Z_FLAG); \
    K.B.l=(R->A&0x0F)+(Rg&0x0F)+(R->P&C_FLAG); \
    if(K.B.l>9) K.B.l+=6; \
    K.B.h=(R->A>>4)+(Rg>>4)+((K.B.l&0x10)>>4); \
    R->P&=~(N_FLAG|V_FLAG|C_FLAG); \
    R->P|=((K.B.h&0x08)? N_FLAG:0); \
    R->P|=(~(R->A^Rg)&(R->A^(K.B.h<<4))&0x80? V_FLAG:0); \
    if(K.B.h>9) K.B.h+=6; \
    R->A=(K.B.l&0x0F)|(K.B.h<<4); \
    R->P=(R->P&~C_FLAG)|(K.B.h&0x10? C_FLAG:0); \
  } \
  else \
  { \
    K.W=R->A+Rg+(R->P&C_FLAG); \
    R->P&=~(N_FLAG|V_FLAG|Z_FLAG|C_FLAG); \
    R->P|=(~(R->A^Rg)&(R->A^K.B.l)&0x80? V_FLAG:0)| \
          (K.B.h? C_FLAG:0)|ZNTable[K.B.l]; \
    R->A=K.B.l; \
  }

/* Warning! C_FLAG is inverted before SBC and after it */
#define M_SBC(Rg) \
  if(R->P&D_FLAG) \
  { \
    short tmp; \
    tmp=R->A-Rg-((R->P&C_FLAG)? 0:1); \
    R->P&=~(N_FLAG|V_FLAG|Z_FLAG); \
    R->P|=((R->A^Rg)&(R->A^tmp)&0x80? V_FLAG:0)|ZNTable[tmp&0xff]; \
    K.B.l=(R->A&0x0F)-(Rg&0x0F)-((R->P&C_FLAG)? 0:1); \
    if(K.B.l&0x10) K.B.l-=6; \
    K.B.h=(R->A>>4)-(Rg>>4)-((K.B.l&0x10)>>4); \
    if(K.B.h&0x10) K.B.h-=6; \
    R->P&=~(C_FLAG); \
    R->P|=(tmp&0x100)? 0:C_FLAG; \
    R->A=((K.B.l&0x0F)|(K.B.h<<4))&0xFF; \
  } \
  else \
  { \
    K.W=R->A-Rg-((R->P&C_FLAG)? 0:1); \
    R->P&=~(N_FLAG|V_FLAG|Z_FLAG|C_FLAG); \
    R->P|=((R->A^Rg)&(R->A^K.B.l)&0x80? V_FLAG:0)| \
          (K.B.h? 0:C_FLAG)|ZNTable[K.B.l]; \
    R->A=K.B.l; \
  }

#define M_CMP(Rg1,Rg2) \
  K.W=Rg1-Rg2; \
  R->P&=~(N_FLAG|Z_FLAG|C_FLAG); \
  R->P|=ZNTable[K.B.l]|(K.B.h? 0:C_FLAG)
#define M_BIT(Rg) \
  R->P&=~(N_FLAG|V_FLAG|Z_FLAG); \
  R->P|=(Rg&(N_FLAG|V_FLAG))|(Rg&R->A? 0:Z_FLAG)

#define M_AND(Rg)	R->A&=Rg;M_FL(R->A)
#define M_ORA(Rg)	R->A|=Rg;M_FL(R->A)
#define M_EOR(Rg)	R->A^=Rg;M_FL(R->A)
#define M_INC(Rg)	Rg++;M_FL(Rg)
#define M_DEC(Rg)	Rg--;M_FL(Rg)

#define M_ASL(Rg)	R->P&=~C_FLAG;R->P|=Rg>>7;Rg<<=1;M_FL(Rg)
#define M_LSR(Rg)	R->P&=~C_FLAG;R->P|=Rg&C_FLAG;Rg>>=1;M_FL(Rg)
#define M_ROL(Rg)	K.B.l=(Rg<<1)|(R->P&C_FLAG); \
			R->P&=~C_FLAG;R->P|=Rg>>7;Rg=K.B.l; \
			M_FL(Rg)
#define M_ROR(Rg)	K.B.l=(Rg>>1)|(R->P<<7); \
			R->P&=~C_FLAG;R->P|=Rg&C_FLAG;Rg=K.B.l; \
			M_FL(Rg)

/** Reset6502() **********************************************/
/** This function can be used to reset the registers before **/
/** starting execution with Run6502(). It sets registers to **/
/** their initial values.                                   **/
/*************************************************************/
void Reset6502(M6502 *R)
{
  R->A=R->X=R->Y=0x00;
  R->P=Z_FLAG|R_FLAG;
  R->S=0xFF;
  R->PC.B.l=Rd6502(0xFFFC);
  R->PC.B.h=Rd6502(0xFFFD);
  change_pc16(R->PC.W);/*ASG 971124*/
/*  R->ICount=R->IPeriod; */	/* NS 970908 */
/*  R->IRequest=INT_NONE; */	/* NS 970904 */
  M6502_Clear_Pending_Interrupts(R);	/* NS 970904 */
  R->AfterCLI=0;
}

/** Exec6502() ***********************************************/
/** This function will execute a single 6502 opcode. It     **/
/** will then return next PC, and current register values   **/
/** in R.                                                   **/
/*************************************************************/
#if 0	/* -NS- */
word Exec6502(M6502 *R)
{
  register pair J,K;
  register byte I;

  I=Op6502(R->PC.W++);
  R->ICount-=Cycles[I];
  switch(I)
  {
#include "Codes.h"
  }

  /* We are done */
  return(R->PC.W);
}
#endif

/** Int6502() ************************************************/
/** This function will generate interrupt of a given type.  **/
/** INT_NMI will cause a non-maskable interrupt. INT_IRQ    **/
/** will cause a normal interrupt, unless I_FLAG set in R.  **/
/*************************************************************/
static void Int6502(M6502 *R/*,byte Type*/)	/* NS 970904 */
{
  register pair J;

/*  if((Type==INT_NMI)||((Type==INT_IRQ)&&!(R->P&I_FLAG)))*/
  if ((R->pending_nmi != 0) || ((R->pending_irq != 0)&&!(R->P&I_FLAG)))	/* NS 970904 */
  {
    R->ICount-=7;
    M_PUSH(R->PC.B.h);
    M_PUSH(R->PC.B.l);
    M_PUSH(R->P&~B_FLAG);
    R->P&=~D_FLAG;
    R->P|=I_FLAG; /* LBO 971204 NMIs will also set the I flag so that IRQs won't interrupt them */
/*    if(Type==INT_NMI)*/ /* NS 970904 */
	if (R->pending_nmi != 0)	/* NS 970904 */
	{
		R->pending_nmi = 0;	/* NS 970904 */
		J.W=0xFFFA;
	}
	else
	{
		R->pending_irq = 0;	/* NS 970904 */
		/*R->P|=I_FLAG;J.W=0xFFFE;  LBO 971204 */
		J.W=0xFFFE;
	}
    R->PC.B.l=Rd6502(J.W++);
    R->PC.B.h=Rd6502(J.W);
    change_pc16(R->PC.W);/*ASG 971124*/
  }
#if 0	/* NS 970904 */
  else if ((Type==INT_IRQ)&&(R->P&I_FLAG))	/* -NS- */
       R->IRequest = Type;	/* -NS- */
#endif
}


void M6502_Cause_Interrupt(M6502 *R,int type)	/* NS 970904 */
{
	if (type == INT_NMI)
		R->pending_nmi = 1;
	else if (type == INT_IRQ)
		R->pending_irq = 1;
}
void M6502_Clear_Pending_Interrupts(M6502 *R)	/* NS 970904 */
{
	R->pending_irq = 0;
	R->pending_nmi = 0;
}


/** Run6502() ************************************************/
/** This function will run 6502 code until Loop6502() call  **/
/** returns INT_QUIT. It will return the PC at which        **/
/** emulation stopped, and current register values in R.    **/
/*************************************************************/
word Run6502(M6502 *R,int cycles)	/* NS 970904 */
{
  register pair J,K;
  register byte I;

 R->ICount=cycles;	/* NS 970904 */

  change_pc16 (R->PC.W);

  do
  {
#ifdef DEBUG
    /* Turn tracing on when reached trap address */
    if(R->PC.W==R->Trap) R->Trace=1;
    /* Call single-step debugger, exit if requested */
    if(R->Trace)
      if(!Debug6502(R)) return(R->PC.W);
#endif
#ifdef MAME_DEBUG
{
  extern int mame_debug;
  if (mame_debug) MAME_Debug();
}
#endif

{	/* NS 971024 */
	extern int previouspc;
	previouspc = R->PC.W;
}
    I=Op6502_1(R->PC.W++);	/* -NS- */
    R->ICount-=Cycles[I];

    switch(I)
    {
#include "Codes.h"
    }


/* NS 970904 - all new */
	if (R->AfterCLI) R->AfterCLI = 0;
	else
	{
		if (R->pending_irq != 0 || R->pending_nmi != 0)
			Int6502(R);
	}
 }
 while (R->ICount>0);

 return cycles - R->ICount;	/* NS 970904 */


#if 0	/* NS 970904 */
    /* If cycle counter expired... */
    if(R->ICount<=0)
    {
      /* If we have come after CLI, get INT_? from IRequest */
      /* Otherwise, get it from the loop handler            */
      if(R->AfterCLI)
      {
        I=R->IRequest;            /* Get pending interrupt     */
        R->IRequest = INT_NONE;	/* -FF- */
        R->ICount+=R->IBackup-1;  /* Restore the ICount        */
        R->AfterCLI=0;            /* Done with AfterCLI state  */
      }
      else
      {
        R->IRequest = INT_NONE;	/* -NS- */
        I=Loop6502(R);            /* Call the periodic handler */
        R->ICount=R->IPeriod;     /* Reset the cycle counter   */
      }

      if(I==INT_QUIT) return(R->PC.W); /* Exit if INT_QUIT     */
      if(I) Int6502(R,I);              /* Interrupt if needed  */
  return(R->PC.W);	/* -NS- */
    }

  /* Execution stopped */
  return(R->PC.W);
#endif
}
