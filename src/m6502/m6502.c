/** M6502: portable 6502 emulator ****************************/
/**                                                         **/
/**                         M6502.c                         **/
/**                                                         **/
/** This file contains implementation for 6502 CPU. Don't   **/
/** forget to provide Rd6502(), Wr6502(), Interrupt6502(),  **/
/** and possibly Op6502() functions to accomodate the       **/
/** emulated machine's architecture.                        **/
/**                                                         **/
/** Copyright (C) Marat Fayzullin 1996                      **/
/**               Alex Krasivsky  1996                      **/
/**     You are not allowed to distribute this software     **/
/**     commercially. Please, notify me, if you make any    **/
/**     changes to this file.                               **/
/*************************************************************/

#include "M6502.h"
#include "Tables.h"
#include <stdio.h>

/** INLINE ***************************************************/
/** Different compilers inline C functions differently.     **/
/*************************************************************/
#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE static
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
/** With this #define not present, Rd6502() should perform **/
/** the functions of Rd6502().                              **/
/*************************************************************/
#ifndef FAST_RDOP
#define Op6502(A) Rd6502(A)
#endif

/** Store/Load Registers *************************************/
/** Store/load local copies of registers from a scturcture. **/
/*************************************************************/
#define M_STORE(R) \
  R->A=A;R->P=P;R->X=X;R->Y=Y;R->S=S;R->PC=PC;R->ICount=ICount
#define M_LOAD(R) \
  A=R->A;P=R->P;X=R->X;Y=R->Y;S=R->S;PC=R->PC;ICount=R->ICount

/** Addressing Methods ***************************************/
/** These macros calculate and return effective addresses.  **/
/*************************************************************/
#define MC_Ab(Rg)	M_LDWORD(Rg)
#define MC_Zp(Rg)	Rg.B.l=Op6502(PC.W++);Rg.B.h=0
#define MC_Zx(Rg)	Rg.B.l=Op6502(PC.W++)+X;Rg.B.h=0
#define MC_Zy(Rg)	Rg.B.l=Op6502(PC.W++)+Y;Rg.B.h=0
#define MC_Ax(Rg)	M_LDWORD(Rg);Rg.W+=X
#define MC_Ay(Rg)	M_LDWORD(Rg);Rg.W+=Y
#define MC_Ix(Rg)	K.B.l=Op6502(PC.W++)+X;K.B.h=0; \
			Rg.B.l=Op6502(K.W++);Rg.B.h=Op6502(K.W)
#define MC_Iy(Rg)	K.B.l=Op6502(PC.W++);K.B.h=0; \
			Rg.B.l=Op6502(K.W++);Rg.B.h=Op6502(K.W); \
			Rg.W+=Y

/** Reading From Memory **************************************/
/** These macros calculate address and read from it.        **/
/*************************************************************/
#define MR_Ab(Rg)	MC_Ab(J);Rg=Rd6502(J.W)
#define MR_Im(Rg)	Rg=Op6502(PC.W++)
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
#define M_FL(Rg)	P=(P&~(Z_FLAG|N_FLAG))|ZNTable[Rg]
#define M_LDWORD(Rg)	Rg.B.l=Op6502(PC.W++);Rg.B.h=Op6502(PC.W++)

#define M_PUSH(Rg)	Wr6502(0x0100|S,Rg);S--
#define M_POP(Rg)	S++;Rg=Op6502(0x0100|S)
#define M_JR		PC.W+=(offset)Op6502(PC.W)+1;ICount--

#define M_ADC(Rg) \
  if(P&D_FLAG) \
  { \
    K.B.l=(A&0x0F)+(Rg&0x0F)+(P&C_FLAG); \
    K.B.h=(A>>4)+(Rg>>4)+(K.B.l>15? 1:0); \
    if(K.B.l>9) {K.B.l+=6;K.B.h++;} \
    if(K.B.h>9) K.B.h+=6; \
    A=(K.B.l&0x0F)|(K.B.h<<4); \
    P=(P&~C_FLAG)|(K.B.h>15? C_FLAG:0); \
  } \
  else \
  { \
    K.W=A+Rg+(P&C_FLAG); \
    P&=~(N_FLAG|V_FLAG|Z_FLAG|C_FLAG); \
    P|=(~(A^Rg)&(A^K.B.l)&0x80? V_FLAG:0)| \
         (K.B.h? C_FLAG:0)|ZNTable[K.B.l]; \
    A=K.B.l; \
  }

/* Warning! C_FLAG is inverted before SBC and after it */
#define M_SBC(Rg) \
  if(P&D_FLAG) \
  { \
    K.B.l=(A&0x0F)-(Rg&0x0F)-(~P&C_FLAG); \
    if(K.B.l&0x10) K.B.l-=6; \
    K.B.h=(A>>4)-(Rg>>4)-(K.B.l&0x10); \
    if(K.B.h&0x10) K.B.h-=6; \
    A=(K.B.l&0x0F)|(K.B.h<<4); \
    P=(P&~C_FLAG)|(K.B.h>15? 0:C_FLAG); \
  } \
  else \
  { \
    K.W=A-Rg-(~P&C_FLAG); \
    P&=~(N_FLAG|V_FLAG|Z_FLAG|C_FLAG); \
    P|=((A^Rg)&(A^K.B.l)&0x80? V_FLAG:0)| \
         (K.B.h? 0:C_FLAG)|ZNTable[K.B.l]; \
    A=K.B.l; \
  }

#define M_CMP(Rg1,Rg2) \
  K.W=Rg1-Rg2; \
  P&=~(N_FLAG|Z_FLAG|C_FLAG); \
  P|=ZNTable[K.B.l]|(K.B.h? 0:C_FLAG)
#define M_BIT(Rg) \
  P&=~(N_FLAG|V_FLAG|Z_FLAG); \
  P|=(Rg&(N_FLAG|V_FLAG))|(Rg&A? 0:Z_FLAG)

#define M_AND(Rg)	A&=Rg;M_FL(A)
#define M_ORA(Rg)	A|=Rg;M_FL(A)
#define M_EOR(Rg)	A^=Rg;M_FL(A)
#define M_INC(Rg)	Rg++;M_FL(Rg)
#define M_DEC(Rg)	Rg--;M_FL(Rg)

#define M_ASL(Rg)	P&=~C_FLAG;P|=Rg>>7;Rg<<=1;M_FL(Rg)
#define M_LSR(Rg)	P&=~C_FLAG;P|=Rg&C_FLAG;Rg>>=1;M_FL(Rg)
#define M_ROL(Rg)	K.B.l=(Rg<<1)|(P&C_FLAG); \
			P&=~C_FLAG;P|=Rg>>7;Rg=K.B.l; \
			M_FL(Rg)
#define M_ROR(Rg)	K.B.l=(Rg>>1)|(P<<7); \
			P&=~C_FLAG;P|=Rg&C_FLAG;Rg=K.B.l; \
			M_FL(Rg)

/** Reset6502() **********************************************/
/** This function can be used to reset the registers before **/
/** starting execution with Run6502(). It sets registers to **/
/** their initial values.                                   **/
/*************************************************************/
void Reset6502(M6502 *R,int IPeriod)
{
  R->A=R->X=R->Y=0x00;
  R->P=Z_FLAG|R_FLAG;
  R->S=0xFF;
  R->PC.B.l=Rd6502(0xFFFC);
  R->PC.B.h=Rd6502(0xFFFD);
R->IPeriod = IPeriod;
  R->ICount=R->IPeriod;
}

/** Run6502() ************************************************/
/** This function will run 6502 code until Interrupt6502()  **/
/** call returns INT_QUIT. It will return the PC at which   **/
/** emulation stopped, and current register values in R.    **/
/*************************************************************/
word Run6502(M6502 *R)
{
  register pair J,K,PC;
  register byte I,P,A,X,Y,S;
  register int ICount;

  /* Load local copies of registers */
  M_LOAD(R);

  for(;;)
  {
#ifdef DEBUG
    /* Turn tracing on when reached trap address */
    if(PC.W==R->Trap) R->Trace=1;
    /* Call single-step debugger, exit if requested */
    if(R->Trace)
    {
      M_STORE(R);
      if(!Debug6502(R)) return(PC.W);
      M_LOAD(R);
    }
#endif

    I=Op6502(PC.W++);
    ICount-=Cycles[I];
    switch(I)
    {
#include "Codes.h"
      default:
        if(R->TrapBadOps)
          printf
          (
            "[M6502 %lX] Unrecognized instruction: $%02X at PC=$%04X\n",
            R->User,Op6502(PC.W-1),(word)(PC.W-1)
          );
    }

    /* If cycle counter expired... */
    if(ICount<=0)
    {
      M_STORE(R);                      /* Store local registers      */
      I=Interrupt6502(R);              /* Call the interrupt handler */
      if(I==INT_QUIT) return(PC.W);    /* If INT_QUIT returned, exit */
      M_LOAD(R);                       /* Load local registers       */
      ICount=R->IPeriod;               /* Reset the cycle counter    */

      if((I==INT_NMI)||((I==INT_IRQ)&&!(P&I_FLAG)))
      {
        ICount-=7;
        M_PUSH(PC.B.h);
        M_PUSH(PC.B.l);
        M_PUSH(P&~B_FLAG);
        P&=~D_FLAG;
        if(I==INT_IRQ) { P|=I_FLAG;J.W=0xFFFE; } else J.W=0xFFFA;
        PC.B.l=Rd6502(J.W++);
        PC.B.h=Rd6502(J.W);
      }
M_STORE(R);
return(PC.W);
    }
  }

  /* Execution stopped */
  M_STORE(R);
  return(PC.W);
}
