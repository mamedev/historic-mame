/*** Z80Em: Portable Z80 emulator *******************************************/
/***                                                                      ***/
/***                              Z80CDx86.c                              ***/
/***                                                                      ***/
/*** This file contains various macros used by the emulation engine,      ***/
/*** optimised for GCC/x86                                                ***/
/***                                                                      ***/
/*** Copyright (C) Marcel de Kogel 1996,1997                              ***/
/***     You are not allowed to distribute this software commercially     ***/
/***     Please, notify me, if you make any changes to this file          ***/
/****************************************************************************/

#define _INLINE         extern __inline__

#define M_POP(Rg)       R.Rg.D=M_RDMEM_WORD(R.SP.D); R.SP.W.l+=2
#define M_PUSH(Rg)      R.SP.W.l-=2; M_WRMEM_WORD(R.SP.D,R.Rg.D)
#define M_CALL              \
{                           \
 int q;                     \
 q=M_RDMEM_OPCODE_WORD();   \
 M_PUSH(PC);                \
 R.PC.D=q;                  \
 Z80_ICount-=7;             \
}
#define M_JP            R.PC.D=M_RDMEM_OPCODE_WORD()
#define M_JR            R.PC.W.l+=(offset)M_RDMEM_OPCODE(); Z80_ICount-=5
#define M_RET           M_POP(PC); Z80_ICount-=6
#define M_RST(Addr)     M_PUSH(PC); R.PC.D=Addr
#define M_SET(Bit,Reg)  Reg|=1<<Bit
#define M_RES(Bit,Reg)  Reg&=~(1<<Bit)
#define M_BIT(Bit,Reg)  \
        R.AF.B.l=(R.AF.B.l&C_FLAG)|H_FLAG|((Reg&(1<<Bit))?0:Z_FLAG)
#define M_IN(Reg)       \
        Reg=Z80_In(R.BC.B.l); R.AF.B.l=(R.AF.B.l&C_FLAG)|ZSPTable[Reg]

#define M_RLC(Reg)         \
{                          \
 int q;                    \
 q=Reg>>7;                 \
 Reg=(Reg<<1)|q;           \
 R.AF.B.l=ZSPTable[Reg]|q; \
}
#define M_RRC(Reg)         \
{                          \
 int q;                    \
 q=Reg&1;                  \
 Reg=(Reg>>1)|(q<<7);      \
 R.AF.B.l=ZSPTable[Reg]|q; \
}
#define M_RL(Reg)            \
{                            \
 int q;                      \
 q=Reg>>7;                   \
 Reg=(Reg<<1)|(R.AF.B.l&1);  \
 R.AF.B.l=ZSPTable[Reg]|q;   \
}
#define M_RR(Reg)            \
{                            \
 int q;                      \
 q=Reg&1;                    \
 Reg=(Reg>>1)|(R.AF.B.l<<7); \
 R.AF.B.l=ZSPTable[Reg]|q;   \
}
#define M_SLL(Reg)           \
{                            \
 int q;                      \
 q=Reg>>7;                   \
 Reg=(Reg<<1)|1;             \
 R.AF.B.l=ZSPTable[Reg]|q;   \
}
#define M_SLA(Reg)           \
{                            \
 int q;                      \
 q=Reg>>7;                   \
 Reg<<=1;                    \
 R.AF.B.l=ZSPTable[Reg]|q;   \
}
#define M_SRL(Reg)           \
{                            \
 int q;                      \
 q=Reg&1;                    \
 Reg>>=1;                    \
 R.AF.B.l=ZSPTable[Reg]|q;   \
}
#define M_SRA(Reg)           \
{                            \
 int q;                      \
 q=Reg&1;                    \
 Reg=(Reg>>1)|(Reg&0x80);    \
 R.AF.B.l=ZSPTable[Reg]|q;   \
}

_INLINE void M_AND(byte Reg)
{
 asm (
 " andb %2,%0           \n"
 " lahf                 \n"
 " andb $0xC4,%%ah      \n"
 " orb $0x10,%%ah       \n"
 " movb %%ah,%1         \n"
 :"=g" (R.AF.B.h),
  "=g" (R.AF.B.l)
 :"g" (Reg)
 :"eax"
 );
}

_INLINE void M_OR(byte Reg)
{
 asm (
 " orb %2,%0            \n"
 " lahf                 \n"
 " andb $0xC4,%%ah      \n"
 " movb %%ah,%1         \n"
 :"=g" (R.AF.B.h),
  "=g" (R.AF.B.l)
 :"g" (Reg)
 :"eax"
 );
}

_INLINE void M_XOR(byte Reg)
{
 asm (
 " xorb %2,%0           \n"
 " lahf                 \n"
 " andb $0xC4,%%ah      \n"
 " movb %%ah,%1         \n"
 :"=g" (R.AF.B.h),
  "=g" (R.AF.B.l)
 :"g" (Reg)
 :"eax"
 );
}

#define M_INC(Reg)              \
 asm (                          \
 " movb %3,%%al         \n"     \
 " shrb $1,%%al         \n"     \
 " incb %0              \n"     \
 " lahf                 \n"     \
 " seto %%al            \n"     \
 " shlb $2,%%al         \n"     \
 " andb $0xD1,%%ah      \n"     \
 " orb %%ah,%%al        \n"     \
 " movb %%al,%1         \n"     \
 :"=g" (Reg),                   \
  "=g" (R.AF.B.l)               \
 :"g" (Reg),                    \
  "g" (R.AF.B.l)                \
 :"eax"                         \
 );

#define M_DEC(Reg)              \
 asm (                          \
 " movb %3,%%al         \n"     \
 " shrb $1,%%al         \n"     \
 " decb %0              \n"     \
 " lahf                 \n"     \
 " seto %%al            \n"     \
 " shlb $2,%%al         \n"     \
 " andb $0xD1,%%ah      \n"     \
 " orb %%ah,%%al        \n"     \
 " orb $2,%%al          \n"     \
 " movb %%al,%1         \n"     \
 :"=g" (Reg),                   \
  "=g" (R.AF.B.l)               \
 :"g" (Reg),                    \
  "g" (R.AF.B.l)                \
 :"eax"                         \
 );

_INLINE void M_ADD (byte Reg)
{
 asm (
 " addb %2,%0           \n"
 " lahf                 \n"
 " seto %%al            \n"
 " shlb $2,%%al         \n"
 " andb $0xD1,%%ah      \n"
 " orb %%ah,%%al        \n"
 " movb %%al,%1         \n"
 :"=g" (R.AF.B.h),
  "=g" (R.AF.B.l)
 :"g" (Reg),
  "g" (R.AF.B.h)
 :"eax"
 );
}

_INLINE void M_ADC (byte Reg)
{
 asm (
 " movb %3,%%al         \n"
 " shrb $1,%%al         \n"
 " adcb %2,%0           \n"
 " lahf                 \n"
 " seto %%al            \n"
 " shlb $2,%%al         \n"
 " andb $0xD1,%%ah      \n"
 " orb %%ah,%%al        \n"
 " movb %%al,%1         \n"
 :"=g" (R.AF.B.h),
  "=g" (R.AF.B.l)
 :"g" (Reg),
  "g" (R.AF.B.l),
  "g" (R.AF.B.h)
 :"eax"
 );
}

_INLINE void M_SUB (byte Reg)
{
 asm (
 " subb %2,%0           \n"
 " lahf                 \n"
 " seto %%al            \n"
 " shlb $2,%%al         \n"
 " andb $0xD1,%%ah      \n"
 " orb %%ah,%%al        \n"
 " orb $2,%%al          \n"
 " movb %%al,%1         \n"
 :"=g" (R.AF.B.h),
  "=g" (R.AF.B.l)
 :"g" (Reg),
  "g" (R.AF.B.h)
 :"eax"
 );
}

_INLINE void M_SBC (byte Reg)
{
 asm (
 " movb %3,%%al         \n"
 " shrb $1,%%al         \n"
 " sbbb %2,%0           \n"
 " lahf                 \n"
 " seto %%al            \n"
 " shlb $2,%%al         \n"
 " andb $0xD1,%%ah      \n"
 " orb %%ah,%%al        \n"
 " orb $2,%%al          \n"
 " movb %%al,%1         \n"
 :"=g" (R.AF.B.h),
  "=g" (R.AF.B.l)
 :"g" (Reg),
  "g" (R.AF.B.l),
  "g" (R.AF.B.h)
 :"eax"
 );
}

_INLINE void M_CP (byte Reg)
{
 asm (
 " cmpb %2,%0          \n"
 " lahf                \n"
 " seto %%al           \n"
 " shlb $2,%%al        \n"
 " andb $0xD1,%%ah     \n"
 " orb %%ah,%%al       \n"
 " orb $2,%%al         \n"
 " movb %%al,%1        \n"
 :"=g" (R.AF.B.h),
  "=g" (R.AF.B.l)
 :"g" (Reg),
  "g" (R.AF.B.l),
  "g" (R.AF.B.h)
 :"eax"
 );
}

#define M_ADDW(Reg1,Reg2)                              \
{                                                      \
 int q;                                                \
 q=R.Reg1.D+R.Reg2.D;                                  \
 R.AF.B.l=(R.AF.B.l&(S_FLAG|Z_FLAG|V_FLAG))|           \
          (((R.Reg1.D^q^R.Reg2.D)&0x1000)>>8)|         \
          ((q>>16)&1);                                 \
 R.Reg1.W.l=q;                                         \
}

#define M_ADCW(Reg)                                            \
{                                                              \
 int q;                                                        \
 q=R.HL.D+R.Reg.D+(R.AF.D&1);                                  \
 R.AF.B.l=(((R.HL.D^q^R.Reg.D)&0x1000)>>8)|                    \
          ((q>>16)&1)|                                         \
          ((q&0x8000)>>8)|                                     \
          ((q&65535)?0:Z_FLAG)|                                \
          (((R.Reg.D^R.HL.D^0x8000)&(R.Reg.D^q)&0x8000)>>13);  \
 R.HL.W.l=q;                                                   \
}

#define M_SBCW(Reg)                                    \
{                                                      \
 int q;                                                \
 q=R.HL.D-R.Reg.D-(R.AF.D&1);                          \
 R.AF.B.l=(((R.HL.D^q^R.Reg.D)&0x1000)>>8)|            \
          ((q>>16)&1)|                                 \
          ((q&0x8000)>>8)|                             \
          ((q&65535)?0:Z_FLAG)|                        \
          (((R.Reg.D^R.HL.D)&(R.Reg.D^q)&0x8000)>>13)| \
          N_FLAG;                                      \
 R.HL.W.l=q;                                           \
}

