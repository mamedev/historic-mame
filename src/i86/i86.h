#ifndef CPU_H
#define CPU_H

#include "mytypes.h"

typedef enum { ES, CS, SS, DS } SREGS;
typedef enum { AX, CX, DX, BX, SP, BP, SI, DI, NONE } WREGS;

#ifdef LSB_FIRST
typedef enum { AL,AH,CL,CH,DL,DH,BL,BH,SPL,SPH,BPL,BPH,SIL,SIH,DIL,DIH } BREGS;
#else
typedef enum { AH,AL,CH,CL,DH,DL,BH,BL,SPH,SPL,BPH,BPL,SIH,SIL,DIH,DIL } BREGS;
#endif

/* parameter x = result, y = source 1, z = source 2 */

#define SetCFB_Add(x,y) (CF = (BYTE)(x) < (BYTE)(y))
#define SetCFW_Add(x,y) (CF = (WORD)(x) < (WORD)(y))
#define SetCFB_Sub(y,z) (CF = (BYTE)(y) > (BYTE)(z))
#define SetCFW_Sub(y,z) (CF = (WORD)(y) > (WORD)(z))
#define SetZFB(x)	(ZF = !(BYTE)(x))
#define SetZFW(x)	(ZF = !(WORD)(x))
#define SetTF(x)	(TF = (x))
#define SetIF(x)	(IF = (x))
#define SetDF(x)	(DF = (x))
#define SetAF(x,y,z)	(AF = ((x) ^ ((y) ^ (z))) & 0x10)
#define SetPF(x)	    (PF = parity_table[(BYTE)(x)])
#define SetOFW_Add(x,y,z)   (OF = ((x) ^ (y)) & ((x) ^ (z)) & 0x8000)
#define SetOFB_Add(x,y,z)   (OF = ((x) ^ (y)) & ((x) ^ (z)) & 0x80)
#define SetOFW_Sub(x,y,z)   (OF = ((z) ^ (y)) & ((z) ^ (x)) & 0x8000)
#define SetOFB_Sub(x,y,z)   (OF = ((z) ^ (y)) & ((z) ^ (x)) & 0x80)
#define SetSFW(x)	   (SF = (x) & 0x8000)
#define SetSFB(x)	   (SF = (x) & 0x80)

/* ChangeE(x) changes x to little endian from the machine's natural endian
    format and back again. Obviously there is nothing to do for little-endian
    machines... */

#if defined(LITTLE_ENDIAN)
#   define ChangeE(x) (WORD)(x)
#else
#   define ChangeE(x) (WORD)(((x) << 8) | ((BYTE)((x) >> 8)))
#endif

/************************************************************************/
/* These macros modified to interface with mame's memory scheme (FF)    */
/* (really not efficient IMHO, the price to pay for portability ???)    */

/* drop lines A16-A19 for a 64KB memory (yes, I know this should be done after adding the offset 8-) */
#define SegToMemPtr(Seg) ((sregs[Seg] << 4) & 0xFFFF)

#define PutMemB(Seg,Off,x) (cpu_writemem((Seg)+(Off),(x)))
#define GetMemB(Seg,Off) ((BYTE)cpu_readmem((Seg)+(Off)))
#define GetMemW(Seg,Off) ((WORD)GetMemB(Seg,Off)+(WORD)(GetMemB(Seg,(Off)+1)<<8))
#define PutMemW(Seg,Off,x) (PutMemB(Seg,Off,(BYTE)(x)),PutMemB(Seg,(Off)+1,(BYTE)((x)>>8)))

#define read_port(port) cpu_readport(port)
#define write_port(port,val) cpu_writeport(port,val)
/* no need to go through cpu_readmem for this one... used by fetching */
#define GetMemInc(Seg,Off) ((BYTE)Memory[(Seg)+(Off)++])
/************************************************************************/

#if defined(LITTLE_ENDIAN) && !defined(ALIGNED_ACCESS)
#   define ReadWord(x) (*(x))
#   define WriteWord(x,y) (*(x) = (y))
#   define CopyWord(x,y) (*x = *y)
#else
#   define ReadWord(x) ((WORD)(*((BYTE *)(x))) + ((WORD)(*((BYTE *)(x)+1)) << 8))
#   define WriteWord(x,y) (*(BYTE *)(x) = (BYTE)(y), *((BYTE *)(x)+1) = (BYTE)((y) >> 8))
#   define CopyWord(x,y) (*(BYTE *)(x) = *(BYTE *)(y), *((BYTE *)(x)+1) = *((BYTE *)(y)+1))
#endif

#define CalcAll()

#define CompressFlags() (WORD)(CF | (PF << 2) | (!(!AF) << 4) | (ZF << 6) \
			    | (!(!SF) << 7) | (TF << 8) | (IF << 9) \
			    | (DF << 10) | (!(!OF) << 11))

#define ExpandFlags(f) \
{ \
      CF = (f) & 1; \
      PF = ((f) & 4) == 4; \
      AF = (f) & 16; \
      ZF = ((f) & 64) == 64; \
      SF = (f) & 128; \
      TF = ((f) & 256) == 256; \
      IF = ((f) & 512) == 512; \
      DF = ((f) & 1024) == 1024; \
      OF = (f) & 2048; \
}


extern WORD wregs[8];	    /* Always little-endian */
extern BYTE *bregs[16];     /* Points to bytes within wregs[] */
extern unsigned sregs[4];   /* Always native machine word order */

extern unsigned ip;	     /* Always native machine word order */

    /* All the byte flags will either be 1 or 0 */
extern BYTE CF, PF, ZF, TF, IF, DF;

    /* All the word flags may be either none-zero (true) or zero (false) */
extern unsigned AF, OF, SF;

extern unsigned c_cs,c_ds,c_es,c_ss,c_stack;

extern volatile int int_pending;
extern volatile int int_blocked;

#endif
