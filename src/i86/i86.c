/****************************************************************************
*                          8088/8086 emulator :                             *
	this is Fabrice Frances' version of David Hedley emulator (pcemu),
	- heavily modified in order to allow memory-mapped I/O
	- interfaced to MAME.
	- now faster too
****************************************************************************/

/****************************************************************************
*                                                                           *
*                            Third Year Project                             *
*                                                                           *
*                            An IBM PC Emulator                             *
*                          For Unix and X Windows                           *
*                                                                           *
*                             By David Hedley                               *
*                                                                           *
*                                                                           *
* This program is Copyrighted.  Consult the file COPYRIGHT for more details *
*                                                                           *
****************************************************************************/

#include "mytypes.h"
#include "global.h"

#include <stdio.h>

#include "I86.h"
#include "instr.h"

#ifdef DEBUGGER
#   include "debugger.h"
#endif

#ifdef DEBUG
#	include <stdlib.h>
#endif


static union {
	WORD w[9];	/* native machine word order ! 8 general registers + a NONE reg*/
	BYTE b[16];
} regs;

static unsigned sregs[4];   /* Always native machine word order */
static unsigned base[4]; /* 'Shadows' of the segment registers multiplied by 16 */

static unsigned ip;	     /* Always native machine word order */


    /* All the byte flags will either be 1 or 0 */

static BYTE CF, PF, ZF, TF, IF, DF;

    /* All the word flags may be either none-zero (true) or zero (false) */

static unsigned AF, OF, SF;


static UINT8 parity_table[256];
static BREGS ModRMRegB[256];
static WREGS ModRMRegW[256];


static volatile int int_pending;   /* Interrupt pending */
static volatile int int_blocked;   /* Blocked pending */
static int init=0;		/* has the cpu started ? */
unsigned char *Memory;	/* MAME kindly gives us the Memory address 8-) */

static struct
{
    SREGS segment;
    WREGS reg1;
    WREGS reg2;
    BOOLEAN offset;
    BOOLEAN offset16;
} ModRMRM[256];

static WREGS ModRMRMWRegs[256];
static BREGS ModRMRMBRegs[256];

static void trap(void);

#define PushSeg(Seg) \
{ \
      register unsigned tmp = (WORD)(regs.w[SP]-2); \
      regs.w[SP]=tmp; \
      PutMemW(base[SS],tmp,sregs[Seg]); \
}


#define PopSeg(seg, Seg) \
{ \
      register unsigned tmp = regs.w[SP]; \
      sregs[Seg] = GetMemW(base[SS],tmp); \
      seg = SegToMemPtr(Seg); \
      tmp += 2; \
      regs.w[SP]=tmp; \
}


#define PushWordReg(Reg) \
{ \
      register unsigned tmp1 = (WORD)(regs.w[SP]-2); \
      WORD tmp2; \
      regs.w[SP]=tmp1; \
      tmp2 = regs.w[Reg]; \
      PutMemW(base[SS],tmp1,tmp2); \
}


#define PopWordReg(Reg) \
{ \
      register unsigned tmp = regs.w[SP]; \
      WORD tmp2 = GetMemW(base[SS],tmp); \
      tmp += 2; \
      regs.w[SP]=tmp; \
      regs.w[Reg]=tmp2; \
}


#define XchgAXReg(Reg) \
{ \
      register unsigned tmp; \
      tmp = regs.w[Reg]; \
      regs.w[Reg] = regs.w[AX]; \
      regs.w[AX] = tmp; \
}


#define IncWordReg(Reg) \
{ \
      register unsigned tmp = (unsigned)regs.w[Reg]; \
      register unsigned tmp1 = tmp+1; \
      SetOFW_Add(tmp1,tmp,1); \
      SetAF(tmp1,tmp,1); \
      SetZFW(tmp1); \
      SetSFW(tmp1); \
      SetPF(tmp1); \
      regs.w[Reg]=tmp1; \
}


#define DecWordReg(Reg) \
{ \
      register unsigned tmp = (unsigned)regs.w[Reg]; \
      register unsigned tmp1 = tmp-1; \
      SetOFW_Sub(tmp1,1,tmp); \
      SetAF(tmp1,tmp,1); \
      SetZFW(tmp1); \
      SetSFW(tmp1); \
      SetPF(tmp1); \
      regs.w[Reg]=tmp1; \
}


#define Logical_br8(op) \
{ \
      unsigned ModRM = (unsigned)GetMemInc(base[CS],ip); \
      register unsigned src = (unsigned)GetModRMRegB(ModRM); \
      register unsigned tmp = (unsigned)GetModRMRMB(ModRM); \
      tmp op ## = src; \
      CF = OF = AF = 0; \
      SetZFB(tmp); \
      SetSFB(tmp); \
      SetPF(tmp); \
      PutbackModRMRMB(ModRM,(BYTE)tmp); \
}


#define Logical_r8b(op) \
{ \
      unsigned ModRM = (unsigned)GetMemInc(base[CS],ip); \
      register unsigned tmp = (unsigned)GetModRMRegB(ModRM); \
      register unsigned src = (unsigned)GetModRMRMB(ModRM); \
      tmp op ## = src; \
      CF = OF = AF = 0; \
      SetZFB(tmp); \
      SetSFB(tmp); \
      SetPF(tmp); \
      PutModRMRegB(ModRM,(BYTE)tmp); \
}


#define Logical_wr16(op) \
{ \
      unsigned ModRM = GetMemInc(base[CS],ip); \
      register unsigned tmp1 = (unsigned)GetModRMRegW(ModRM); \
      register unsigned tmp2 = (unsigned)GetModRMRMW(ModRM); \
      register unsigned tmp3 = tmp1 op tmp2; \
      CF = OF = AF = 0; \
      SetZFW(tmp3); \
      SetSFW(tmp3); \
      SetPF(tmp3); \
      PutbackModRMRMW(ModRM,tmp3); \
}


#define Logical_r16w(op) \
{ \
      unsigned ModRM = GetMemInc(base[CS],ip); \
      register unsigned tmp2 = (unsigned)GetModRMRegW(ModRM); \
      register unsigned tmp1 = (unsigned)GetModRMRMW(ModRM); \
      register unsigned tmp3 = tmp1 op tmp2; \
      CF = OF = AF = 0; \
      SetZFW(tmp3); \
      SetSFW(tmp3); \
      SetPF(tmp3); \
      PutModRMRegW(ModRM,tmp3); \
}


#define Logical_ald8(op) \
{ \
      register unsigned tmp = regs.b[AL]; \
      tmp op ## = (unsigned)GetMemInc(base[CS],ip); \
      CF = OF = AF = 0; \
      SetZFB(tmp); \
      SetSFB(tmp); \
      SetPF(tmp); \
      regs.b[AL] = (BYTE)tmp; \
}


#define Logical_axd16(op) \
{ \
      register unsigned src; \
      register unsigned tmp = regs.w[AX]; \
      src = GetMemInc(base[CS],ip); \
      src += GetMemInc(base[CS],ip) << 8; \
      tmp op ## = src; \
      CF = OF = AF = 0; \
      SetZFW(tmp); \
      SetSFW(tmp); \
      SetPF(tmp); \
      regs.w[AX]=tmp; \
}


#define LogicalOp(name,symbol) \
static INLINE2 void i_ ## name ## _br8(void) \
{ Logical_br8(symbol); } \
static INLINE2 void i_ ## name ## _wr16(void) \
{ Logical_wr16(symbol); } \
static INLINE2 void i_ ## name ## _r8b(void) \
{ Logical_r8b(symbol); } \
static INLINE2 void i_ ## name ## _r16w(void) \
{ Logical_r16w(symbol); } \
static INLINE2 void i_ ## name ## _ald8(void) \
{ Logical_ald8(symbol); } \
static INLINE2 void i_ ## name ## _axd16(void) \
{ Logical_axd16(symbol); }


#define JumpCond(name, cond) \
static INLINE2 void i_j ## name ## (void) \
{ \
      register int tmp = (int)((INT8)GetMemInc(base[CS],ip)); \
      if (cond) ip = (WORD)(ip+tmp); \
}


void I86_Reset(unsigned char *mem,int cycles)
{
    unsigned int i,j,c;
    BREGS reg_name[8]={ AL, CL, DL, BL, AH, CH, DH, BH };

    for (i = 0; i < 4; i++)
        sregs[i] = 0;
    for (i=0; i < 8; i++)
        regs.w[i] = 0;

    sregs[CS]=0xFFFF;
    ip = 0;
    init = 0;

    Memory=mem;

    for (i = 0;i < 256; i++)
    {
        for (j = i, c = 0; j > 0; j >>= 1)
            if (j & 1) c++;

        parity_table[i] = !(c & 1);
    }

    CF = PF = AF = ZF = SF = TF = IF = DF = OF = 0;

    for (i = 0; i < 256; i++)
    {
        ModRMRegB[i] = reg_name[(i & 0x38) >> 3];
        ModRMRegW[i] = (i & 0x38) >> 3;
    }

    for (i = 0; i < 0x40; i++)
    {
        switch (i & 7)
        {
        case 0:
            ModRMRM[i].segment = DS;
            ModRMRM[i].reg1 = BX;
            ModRMRM[i].reg2 = SI;
            ModRMRM[i].offset = ModRMRM[i].offset16 = FALSE;
            break;
        case 1:
            ModRMRM[i].segment = DS;
            ModRMRM[i].reg1 = BX;
            ModRMRM[i].reg2 = DI;
            ModRMRM[i].offset = ModRMRM[i].offset16 = FALSE;
            break;
        case 2:
            ModRMRM[i].segment = SS;
            ModRMRM[i].reg1 = BP;
            ModRMRM[i].reg2 = SI;
            ModRMRM[i].offset = ModRMRM[i].offset16 = FALSE;
            break;
        case 3:
            ModRMRM[i].segment = DS;
            ModRMRM[i].reg1 = BP;
            ModRMRM[i].reg2 = DI;
            ModRMRM[i].offset = ModRMRM[i].offset16 = FALSE;
            break;
        case 4:
            ModRMRM[i].segment = DS;
            ModRMRM[i].reg1 = NONE;
            ModRMRM[i].reg2 = SI;
            ModRMRM[i].offset = ModRMRM[i].offset16 = FALSE;
            break;
        case 5:
            ModRMRM[i].segment = DS;
            ModRMRM[i].reg1 = NONE;
            ModRMRM[i].reg2 = DI;
            ModRMRM[i].offset = ModRMRM[i].offset16 = FALSE;
            break;
        case 6:
            ModRMRM[i].segment = DS;
            ModRMRM[i].reg1 = NONE;
            ModRMRM[i].reg2 = NONE;
            ModRMRM[i].offset = TRUE;
            ModRMRM[i].offset16 = TRUE;
            break;
        default:
            ModRMRM[i].segment = DS;
            ModRMRM[i].reg1 = BX;
            ModRMRM[i].reg2 = NONE;
            ModRMRM[i].offset = ModRMRM[i].offset16 = FALSE;
            break;
        }
    }

    for (i = 0x40; i < 0x80; i++)
    {
        switch (i & 7)
        {
        case 0:
            ModRMRM[i].segment = DS;
            ModRMRM[i].reg1 = BX;
            ModRMRM[i].reg2 = SI;
            ModRMRM[i].offset = TRUE;
            ModRMRM[i].offset16 = FALSE;
            break;
        case 1:
            ModRMRM[i].segment = DS;
            ModRMRM[i].reg1 = BX;
            ModRMRM[i].reg2 = DI;
            ModRMRM[i].offset = TRUE;
            ModRMRM[i].offset16 = FALSE;
            break;
        case 2:
            ModRMRM[i].segment = SS;
            ModRMRM[i].reg1 = BP;
            ModRMRM[i].reg2 = SI;
            ModRMRM[i].offset = TRUE;
            ModRMRM[i].offset16 = FALSE;
            break;
        case 3:
            ModRMRM[i].segment = SS;
            ModRMRM[i].reg1 = BP;
            ModRMRM[i].reg2 = DI;
            ModRMRM[i].offset = TRUE;
            ModRMRM[i].offset16 = FALSE;
            break;
        case 4:
            ModRMRM[i].segment = DS;
            ModRMRM[i].reg1 = NONE;
            ModRMRM[i].reg2 = SI;
            ModRMRM[i].offset = TRUE;
            ModRMRM[i].offset16 = FALSE;
            break;
        case 5:
            ModRMRM[i].segment = DS;
            ModRMRM[i].reg1 = NONE;
            ModRMRM[i].reg2 = DI;
            ModRMRM[i].offset = TRUE;
            ModRMRM[i].offset16 = FALSE;
            break;
        case 6:
            ModRMRM[i].segment = SS;
            ModRMRM[i].reg1 = BP;
            ModRMRM[i].reg2 = NONE;
            ModRMRM[i].offset = TRUE;
            ModRMRM[i].offset16 = FALSE;
            break;
        default:
            ModRMRM[i].segment = DS;
            ModRMRM[i].reg1 = BX;
            ModRMRM[i].reg2 = NONE;
            ModRMRM[i].offset = TRUE;
            ModRMRM[i].offset16 = FALSE;
            break;
        }
    }

    for (i = 0x80; i < 0xc0; i++)
    {
        switch (i & 7)
        {
        case 0:
            ModRMRM[i].segment = DS;
            ModRMRM[i].reg1 = BX;
            ModRMRM[i].reg2 = SI;
            ModRMRM[i].offset = TRUE;
            ModRMRM[i].offset16 = TRUE;
            break;
        case 1:
            ModRMRM[i].segment = DS;
            ModRMRM[i].reg1 = BX;
            ModRMRM[i].reg2 = DI;
            ModRMRM[i].offset = TRUE;
            ModRMRM[i].offset16 = TRUE;
            break;
        case 2:
            ModRMRM[i].segment = DS;
            ModRMRM[i].reg1 = BP;
            ModRMRM[i].reg2 = SI;
            ModRMRM[i].offset = TRUE;
            ModRMRM[i].offset16 = TRUE;
            break;
        case 3:
            ModRMRM[i].segment = SS;
            ModRMRM[i].reg1 = BP;
            ModRMRM[i].reg2 = DI;
            ModRMRM[i].offset = TRUE;
            ModRMRM[i].offset16 = TRUE;
            break;
        case 4:
            ModRMRM[i].segment = DS;
            ModRMRM[i].reg1 = NONE;
            ModRMRM[i].reg2 = SI;
            ModRMRM[i].offset = TRUE;
            ModRMRM[i].offset16 = TRUE;
            break;
        case 5:
            ModRMRM[i].segment = DS;
            ModRMRM[i].reg1 = NONE;
            ModRMRM[i].reg2 = DI;
            ModRMRM[i].offset = TRUE;
            ModRMRM[i].offset16 = TRUE;
            break;
        case 6:
            ModRMRM[i].segment = SS;
            ModRMRM[i].reg1 = BP;
            ModRMRM[i].reg2 = NONE;
            ModRMRM[i].offset = TRUE;
            ModRMRM[i].offset16 = TRUE;
            break;
        default:
            ModRMRM[i].segment = DS;
            ModRMRM[i].reg1 = BX;
            ModRMRM[i].reg2 = NONE;
            ModRMRM[i].offset = TRUE;
            ModRMRM[i].offset16 = TRUE;
            break;
        }
    }

    for (i = 0xc0; i < 0x100; i++)
    {
        ModRMRMWRegs[i] = i & 7;
        ModRMRMBRegs[i] = reg_name[i & 7];
    }
}


#ifdef DEBUG2
void loc(void)
{
    printf("%04X:%04X ", sregs[CS], ip);
}
#endif

static void I86_interrupt(unsigned int_num)
{
    unsigned dest_seg, dest_off,tmp1;

    i_pushf();
    dest_off = GetMemW(0,int_num*4);
    dest_seg = GetMemW(0,int_num*4+2);

    tmp1 = (WORD)(regs.w[SP]-2);

    PutMemW(base[SS],tmp1,sregs[CS]);
    tmp1 = (WORD)(tmp1-2);
    PutMemW(base[SS],tmp1,ip);
    regs.w[SP]=tmp1;

    ip = (WORD)dest_off;
    sregs[CS] = (WORD)dest_seg;
    base[CS] = SegToMemPtr(CS);

    TF = IF = 0;    /* Turn of trap and interrupts... */

}


static void external_int(void)
{
/*
    disable();
*/

#ifdef DEBUGGER
    call_debugger(D_INT);
#endif
    D2(printf("Interrupt 0x%02X\n", int_pending););
    I86_interrupt(int_pending);
    int_pending = 0;

/*
    enable();
*/
}


#define GetModRMRegW(ModRM) regs.w[ModRMRegW[ModRM]]
#define GetModRMRegB(ModRM) regs.b[ModRMRegB[ModRM]]
#define PutModRMRegW(ModRM,val) regs.w[ModRMRegW[ModRM]]=(val)
#define PutModRMRegB(ModRM,val) regs.b[ModRMRegB[ModRM]]=(val)

static unsigned disp;

static INLINE WORD GetModRMRMW(unsigned ModRM)
{
    if (ModRM >= 0xc0)
        return regs.w[ModRMRMWRegs[ModRM]];

    disp = 0;

    if (ModRMRM[ModRM].offset)
    {
        disp = (WORD)((INT16)((INT8)GetMemInc(base[CS],ip)));
        if (ModRMRM[ModRM].offset16)
            disp = (GetMemInc(base[CS],ip) << 8) + (BYTE)disp;
    }

    return GetMemW(base[ModRMRM[ModRM].segment],
                  (WORD)(regs.w[ModRMRM[ModRM].reg1] +
                   regs.w[ModRMRM[ModRM].reg2] + disp));
}


static INLINE void PutModRMRMW(unsigned ModRM, WORD val)
{
    if (ModRM >= 0xc0) {
        regs.w[ModRMRMWRegs[ModRM]]=val;
        return;
    }

    disp = 0;

    if (ModRMRM[ModRM].offset)
    {
        disp = (WORD)((INT16)((INT8)GetMemInc(base[CS],ip)));
        if (ModRMRM[ModRM].offset16)
            disp = (GetMemInc(base[CS],ip) << 8) + (BYTE)disp;
    }

    PutMemW(base[ModRMRM[ModRM].segment],
            (WORD)(regs.w[ModRMRM[ModRM].reg1] +
                   regs.w[ModRMRM[ModRM].reg2] + disp),
            val);
}

static INLINE void PutImmModRMRMW(unsigned ModRM)
{
    WORD val;

    if (ModRM >= 0xc0) {
        val = (WORD)((INT16)((INT8)GetMemInc(base[CS],ip)));
        val = (GetMemInc(base[CS],ip) << 8) + (BYTE)val;
        regs.w[ModRMRMWRegs[ModRM]]=val;
        return;
    }

    disp = 0;

    if (ModRMRM[ModRM].offset)
    {
        disp = (WORD)((INT16)((INT8)GetMemInc(base[CS],ip)));
        if (ModRMRM[ModRM].offset16)
            disp = (GetMemInc(base[CS],ip) << 8) + (BYTE)disp;
    }

    val = (WORD)((INT16)((INT8)GetMemInc(base[CS],ip)));
    val = (GetMemInc(base[CS],ip) << 8) + (BYTE)val;

    PutMemW(base[ModRMRM[ModRM].segment],
            (WORD)(regs.w[ModRMRM[ModRM].reg1] +
                   regs.w[ModRMRM[ModRM].reg2] + disp),
            val);
}


static INLINE WORD GetnextModRMRMW(unsigned ModRM)
{
    return GetMemW(base[ModRMRM[ModRM].segment],
                  (WORD)(regs.w[ModRMRM[ModRM].reg1] +
                   regs.w[ModRMRM[ModRM].reg2] + disp + 2));
}

static INLINE void PutbackModRMRMW(unsigned ModRM, WORD val)
{
    if (ModRM >= 0xc0)
        regs.w[ModRMRMWRegs[ModRM]]=val;
    else
	PutMemW(base[ModRMRM[ModRM].segment],
            (WORD)(regs.w[ModRMRM[ModRM].reg1] +
                   regs.w[ModRMRM[ModRM].reg2] + disp),
            val);
}


static INLINE BYTE GetModRMRMB(unsigned ModRM)
{
    if (ModRM >= 0xc0)
        return regs.b[ModRMRMBRegs[ModRM]];

    disp = 0;

    if (ModRMRM[ModRM].offset)
    {
        disp = (WORD)((INT16)((INT8)GetMemInc(base[CS],ip)));
        if (ModRMRM[ModRM].offset16)
            disp = (GetMemInc(base[CS],ip) << 8) + (BYTE)disp;
    }

    return GetMemB(base[ModRMRM[ModRM].segment],
                  (WORD)(regs.w[ModRMRM[ModRM].reg1] +
                         regs.w[ModRMRM[ModRM].reg2] + disp));
}


static INLINE void PutModRMRMB(unsigned ModRM, BYTE val)
{
    if (ModRM >= 0xc0) {
        regs.b[ModRMRMBRegs[ModRM]]=val;
	return;
    }

    disp = 0;

    if (ModRMRM[ModRM].offset)
    {
        disp = (WORD)((INT16)((INT8)GetMemInc(base[CS],ip)));
        if (ModRMRM[ModRM].offset16)
            disp = (GetMemInc(base[CS],ip) << 8) + (BYTE)disp;
    }

    PutMemB(base[ModRMRM[ModRM].segment],
            (WORD)(regs.w[ModRMRM[ModRM].reg1] +
                   regs.w[ModRMRM[ModRM].reg2] + disp),
            val);
}


static INLINE void PutImmModRMRMB(unsigned ModRM)
{
    if (ModRM >= 0xc0) {
        regs.b[ModRMRMBRegs[ModRM]]=GetMemInc(base[CS],ip);
	return;
    }

    disp = 0;

    if (ModRMRM[ModRM].offset)
    {
        disp = (WORD)((INT16)((INT8)GetMemInc(base[CS],ip)));
        if (ModRMRM[ModRM].offset16)
            disp = (GetMemInc(base[CS],ip) << 8) + (BYTE)disp;
    }

    PutMemB(base[ModRMRM[ModRM].segment],
            (WORD)(regs.w[ModRMRM[ModRM].reg1] +
                   regs.w[ModRMRM[ModRM].reg2] + disp),
            GetMemInc(base[CS],ip));
}


static INLINE void PutbackModRMRMB(unsigned ModRM, BYTE val)
{
    if (ModRM >= 0xc0)
        regs.b[ModRMRMBRegs[ModRM]]=val;
    else
	PutMemB(base[ModRMRM[ModRM].segment],
            (WORD)(regs.w[ModRMRM[ModRM].reg1] +
                   regs.w[ModRMRM[ModRM].reg2] + disp),
            val);
}


static INLINE2 void i_add_br8(void)
{
    /* Opcode 0x00 */

    unsigned ModRM = (unsigned)GetMemInc(base[CS],ip);
    register unsigned src = (unsigned)GetModRMRegB(ModRM);
    register unsigned tmp = (unsigned)GetModRMRMB(ModRM);
    register unsigned tmp2 = tmp;

    tmp += src;

    SetCFB_Add(tmp,tmp2);
    SetOFB_Add(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);

    PutbackModRMRMB(ModRM,(BYTE)tmp);
}


static INLINE2 void i_add_wr16(void)
{
    /* Opcode 0x01 */

    unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned tmp2 = (unsigned)GetModRMRegW(ModRM);
    register unsigned tmp1 = (unsigned)GetModRMRMW(ModRM);
    register unsigned tmp3;

    tmp3 = tmp1+tmp2;

    SetCFW_Add(tmp3,tmp1);
    SetOFW_Add(tmp3,tmp2,tmp1);
    SetAF(tmp3,tmp2,tmp1);
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);

    PutbackModRMRMW(ModRM, tmp3);
}


static INLINE2 void i_add_r8b(void)
{
    /* Opcode 0x02 */

    unsigned ModRM = (unsigned)GetMemInc(base[CS],ip);
    register unsigned tmp = (unsigned)GetModRMRegB(ModRM);
    register unsigned src = (unsigned)GetModRMRMB(ModRM);
    register unsigned tmp2 = tmp;

    tmp += src;

    SetCFB_Add(tmp,tmp2);
    SetOFB_Add(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);

    PutModRMRegB(ModRM,(BYTE)tmp);

}


static INLINE2 void i_add_r16w(void)
{
    /* Opcode 0x03 */

    unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned tmp1 = (unsigned)GetModRMRegW(ModRM);
    register unsigned tmp2 = (unsigned)GetModRMRMW(ModRM);
    register unsigned tmp3;

    tmp3 = tmp1 + tmp2;

    SetCFW_Add(tmp3,tmp1);
    SetOFW_Add(tmp3,tmp2,tmp1);
    SetAF(tmp3,tmp2,tmp1);
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);

    PutModRMRegW(ModRM, tmp3);
}


static INLINE2 void i_add_ald8(void)
{
    /* Opcode 0x04 */

    register unsigned src = (unsigned)GetMemInc(base[CS],ip);
    register unsigned tmp = (unsigned)regs.b[AL];
    register unsigned tmp2 = tmp;

    tmp2 += src;

    SetCFB_Add(tmp2,tmp);
    SetOFB_Add(tmp2,src,tmp);
    SetAF(tmp2,src,tmp);
    SetZFB(tmp2);
    SetSFB(tmp2);
    SetPF(tmp2);

    regs.b[AL] = (BYTE)tmp2;
}


static INLINE2 void i_add_axd16(void)
{
    /* Opcode 0x05 */

    register unsigned src;
    register unsigned tmp = regs.w[AX];
    register unsigned tmp2 = tmp;

    src = GetMemInc(base[CS],ip);
    src += GetMemInc(base[CS],ip) << 8;

    tmp2 += src;

    SetCFW_Add(tmp2,tmp);
    SetOFW_Add(tmp2,tmp,src);
    SetAF(tmp2,tmp,src);
    SetZFW(tmp2);
    SetSFW(tmp2);
    SetPF(tmp2);

    regs.w[AX]=tmp2;
}


static INLINE2 void i_push_es(void)
{
    /* Opcode 0x06 */

    PushSeg(ES);
}


static INLINE2 void i_pop_es(void)
{
    /* Opcode 0x07 */
    PopSeg(base[ES],ES);
}

    /* most OR instructions go here... */

LogicalOp(or,|)


static INLINE2 void i_push_cs(void)
{
    /* Opcode 0x0e */

    PushSeg(CS);
}


static INLINE2 void i_adc_br8(void)
{
    /* Opcode 0x10 */

    unsigned ModRM = (unsigned)GetMemInc(base[CS],ip);
    register unsigned src = (unsigned)GetModRMRegB(ModRM)+CF;
    register unsigned tmp = (unsigned)GetModRMRMB(ModRM);
    register unsigned tmp2 = tmp;

    tmp += src;

    CF = tmp >> 8;

/*    SetCFB_Add(tmp,tmp2); */
    SetOFB_Add(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);

    PutbackModRMRMB(ModRM,(BYTE)tmp);
}


static INLINE2 void i_adc_wr16(void)
{
    /* Opcode 0x11 */

    unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned tmp2 = (unsigned)GetModRMRegW(ModRM)+CF;
    register unsigned tmp1 = (unsigned)GetModRMRMW(ModRM);
    register unsigned tmp3;

    tmp3 = tmp1+tmp2;

    CF = tmp3 >> 16;
/*    SetCFW_Add(tmp3,tmp1); */
    SetOFW_Add(tmp3,tmp2,tmp1);
    SetAF(tmp3,tmp2,tmp1);
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);

    PutbackModRMRMW(ModRM, tmp3);

}


static INLINE2 void i_adc_r8b(void)
{
    /* Opcode 0x12 */

    unsigned ModRM = (unsigned)GetMemInc(base[CS],ip);
    register unsigned tmp = (unsigned)GetModRMRegB(ModRM);
    register unsigned src = (unsigned)GetModRMRMB(ModRM)+CF;
    register unsigned tmp2 = tmp;

    tmp += src;

    CF = tmp >> 8;

/*    SetCFB_Add(tmp,tmp2); */
    SetOFB_Add(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);

    PutModRMRegB(ModRM,(BYTE)tmp);
}


static INLINE2 void i_adc_r16w(void)
{
    /* Opcode 0x13 */

    unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned tmp1 = (unsigned)GetModRMRegW(ModRM);
    register unsigned tmp2 = (unsigned)GetModRMRMW(ModRM)+CF;
    register unsigned tmp3;

    tmp3 = tmp1+tmp2;

    CF = tmp3 >> 16;

/*    SetCFW_Add(tmp3,tmp1); */
    SetOFW_Add(tmp3,tmp2,tmp1);
    SetAF(tmp3,tmp2,tmp1);
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);

    PutModRMRegW(ModRM, tmp3);

}


static INLINE2 void i_adc_ald8(void)
{
    /* Opcode 0x14 */

    register unsigned src = (unsigned)GetMemInc(base[CS],ip)+CF;
    register unsigned tmp = (unsigned)regs.b[AL];
    register unsigned tmp2 = tmp;

    tmp2 += src;

    CF = tmp2 >> 8;

/*    SetCFB_Add(tmp2,tmp); */
    SetOFB_Add(tmp2,src,tmp);
    SetAF(tmp2,src,tmp);
    SetZFB(tmp2);
    SetSFB(tmp2);
    SetPF(tmp2);

    regs.b[AL] = (BYTE)tmp2;
}


static INLINE2 void i_adc_axd16(void)
{
    /* Opcode 0x15 */

    register unsigned src;
    register unsigned tmp = regs.w[AX];
    register unsigned tmp2 = tmp;

    src = GetMemInc(base[CS],ip);
    src += (GetMemInc(base[CS],ip) << 8)+CF;

    tmp2 += src;

    CF = tmp2 >> 16;

/*    SetCFW_Add(tmp2,tmp); */
    SetOFW_Add(tmp2,tmp,src);
    SetAF(tmp2,tmp,src);
    SetZFW(tmp2);
    SetSFW(tmp2);
    SetPF(tmp2);

    regs.w[AX]=tmp2;
}


static INLINE2 void i_push_ss(void)
{
    /* Opcode 0x16 */

    PushSeg(SS);
}


static INLINE2 void i_pop_ss(void)
{
    /* Opcode 0x17 */

    static int multiple = 0;

    PopSeg(base[SS],SS);

    if (multiple == 0)	/* prevent unlimited recursion */
    {
        multiple = 1;
/*
#ifdef DEBUGGER
        call_debugger(D_TRACE);
#endif
*/
        instruction[GetMemInc(base[CS],ip)]();
        multiple = 0;
    }
}


static INLINE2 void i_sbb_br8(void)
{
    /* Opcode 0x18 */

    unsigned ModRM = (unsigned)GetMemInc(base[CS],ip);
    register unsigned src = (unsigned)GetModRMRegB(ModRM)+CF;
    register unsigned tmp = (unsigned)GetModRMRMB(ModRM);
    register unsigned tmp2 = tmp;

    tmp -= src;

    CF = (tmp & 0x100) == 0x100;

/*    SetCFB_Sub(tmp,tmp2); */
    SetOFB_Sub(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);

    PutbackModRMRMB(ModRM,(BYTE)tmp);
}


static INLINE2 void i_sbb_wr16(void)
{
    /* Opcode 0x19 */

    unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned tmp2 = GetModRMRegW(ModRM)+CF;
    register unsigned tmp1 = GetModRMRMW(ModRM);
    register unsigned tmp3;

    tmp3 = tmp1-tmp2;

    CF = (tmp3 & 0x10000) == 0x10000;

/*    SetCFW_Sub(tmp2,tmp1); */
    SetOFW_Sub(tmp3,tmp2,tmp1);
    SetAF(tmp3,tmp2,tmp1);
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);

    PutbackModRMRMW(ModRM, tmp3);
}


static INLINE2 void i_sbb_r8b(void)
{
    /* Opcode 0x1a */

    unsigned ModRM = (unsigned)GetMemInc(base[CS],ip);
    register unsigned tmp = (unsigned)GetModRMRegB(ModRM);
    register unsigned src = (unsigned)GetModRMRMB(ModRM)+CF;
    register unsigned tmp2 = tmp;

    tmp -= src;

    CF = (tmp & 0x100) == 0x100;

/*    SetCFB_Sub(tmp,tmp2); */
    SetOFB_Sub(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);

    PutModRMRegB(ModRM,(BYTE)tmp);

}


static INLINE2 void i_sbb_r16w(void)
{
    /* Opcode 0x1b */

    unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned tmp1 = GetModRMRegW(ModRM);
    register unsigned tmp2 = GetModRMRMW(ModRM)+CF;
    register unsigned tmp3;

    tmp3 = tmp1-tmp2;

    CF = (tmp3 & 0x10000) == 0x10000;

/*    SetCFW_Sub(tmp2,tmp1); */
    SetOFW_Sub(tmp3,tmp2,tmp1);
    SetAF(tmp3,tmp2,tmp1);
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);

    PutModRMRegW(ModRM, tmp3);
}


static INLINE2 void i_sbb_ald8(void)
{
    /* Opcode 0x1c */

    register unsigned src = GetMemInc(base[CS],ip)+CF;
    register unsigned tmp = regs.b[AL];
    register unsigned tmp1 = tmp;

    tmp1 -= src;

    CF = (tmp1 & 0x100) == 0x100;

/*    SetCFB_Sub(src,tmp); */
    SetOFB_Sub(tmp1,src,tmp);
    SetAF(tmp1,src,tmp);
    SetZFB(tmp1);
    SetSFB(tmp1);
    SetPF(tmp1);

    regs.b[AL] = (BYTE)tmp1;
}


static INLINE2 void i_sbb_axd16(void)
{
    /* Opcode 0x1d */

    register unsigned src;
    register unsigned tmp = regs.w[AX];
    register unsigned tmp2 = tmp;

    src = GetMemInc(base[CS],ip);
    src += (GetMemInc(base[CS],ip) << 8)+CF;

    tmp2 -= src;

    CF = (tmp2 & 0x10000) == 0x10000;

/*    SetCFW_Sub(src,tmp); */
    SetOFW_Sub(tmp2,src,tmp);
    SetAF(tmp2,tmp,src);
    SetZFW(tmp2);
    SetSFW(tmp2);
    SetPF(tmp2);

    regs.w[AX]=tmp2;
}


static INLINE2 void i_push_ds(void)
{
    /* Opcode 0x1e */

    PushSeg(DS);
}


static INLINE2 void i_pop_ds(void)
{
    /* Opcode 0x1f */
    PopSeg(base[DS],DS);
}

    /* most AND instructions go here... */

LogicalOp(and,&)


static INLINE2 void i_es(void)
{
    /* Opcode 0x26 */

    base[DS] = base[SS] = base[ES];

    instruction[GetMemInc(base[CS],ip)]();

    base[DS] = SegToMemPtr(DS);
    base[SS] = SegToMemPtr(SS);
}

static INLINE2 void i_daa(void)
{
    if (AF || ((regs.b[AL] & 0xf) > 9))
    {
        regs.b[AL] += 6;
        AF = 1;
    }
    else
        AF = 0;

    if (CF || (regs.b[AL] > 0x9f))
    {
        regs.b[AL] += 0x60;
        CF = 1;
    }
    else
        CF = 0;

    SetPF(regs.b[AL]);
    SetSFB(regs.b[AL]);
    SetZFB(regs.b[AL]);
}

static INLINE2 void i_sub_br8(void)
{
    /* Opcode 0x28 */

    unsigned ModRM = (unsigned)GetMemInc(base[CS],ip);
    register unsigned src = (unsigned)GetModRMRegB(ModRM);
    register unsigned tmp = (unsigned)GetModRMRMB(ModRM);
    register unsigned tmp2 = tmp;

    tmp -= src;

    SetCFB_Sub(tmp,tmp2);
    SetOFB_Sub(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);

    PutbackModRMRMB(ModRM,(BYTE)tmp);

}


static INLINE2 void i_sub_wr16(void)
{
    /* Opcode 0x29 */

    unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned tmp2 = GetModRMRegW(ModRM);
    register unsigned tmp1 = GetModRMRMW(ModRM);
    register unsigned tmp3;

    tmp3 = tmp1-tmp2;

    SetCFW_Sub(tmp2,tmp1);
    SetOFW_Sub(tmp3,tmp2,tmp1);
    SetAF(tmp3,tmp2,tmp1);
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);

    PutbackModRMRMW(ModRM, tmp3);
}


static INLINE2 void i_sub_r8b(void)
{
    /* Opcode 0x2a */

    unsigned ModRM = (unsigned)GetMemInc(base[CS],ip);
    register unsigned tmp = GetModRMRegB(ModRM);
    register unsigned src = GetModRMRMB(ModRM);
    register unsigned tmp2 = tmp;

    tmp -= src;

    SetCFB_Sub(tmp,tmp2);
    SetOFB_Sub(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);

    PutModRMRegB(ModRM,(BYTE)tmp);
}


static INLINE2 void i_sub_r16w(void)
{
    /* Opcode 0x2b */

    unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned tmp1 = GetModRMRegW(ModRM);
    register unsigned tmp2 = GetModRMRMW(ModRM);
    register unsigned tmp3;

    tmp3 = tmp1-tmp2;

    SetCFW_Sub(tmp2,tmp1);
    SetOFW_Sub(tmp3,tmp2,tmp1);
    SetAF(tmp3,tmp2,tmp1);
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);

    PutModRMRegW(ModRM, tmp3);
}


static INLINE2 void i_sub_ald8(void)
{
    /* Opcode 0x2c */

    register unsigned src = GetMemInc(base[CS],ip);
    register unsigned tmp = regs.b[AL];
    register unsigned tmp1 = tmp;

    tmp1 -= src;

    SetCFB_Sub(src,tmp);
    SetOFB_Sub(tmp1,src,tmp);
    SetAF(tmp1,src,tmp);
    SetZFB(tmp1);
    SetSFB(tmp1);
    SetPF(tmp1);

    regs.b[AL] = (unsigned) tmp1;
}


static INLINE2 void i_sub_axd16(void)
{
    /* Opcode 0x2d */

    register unsigned src;
    register unsigned tmp = regs.w[AX];
    register unsigned tmp2 = tmp;

    src = GetMemInc(base[CS],ip);
    src += (GetMemInc(base[CS],ip) << 8);

    tmp2 -= src;

    SetCFW_Sub(src,tmp);
    SetOFW_Sub(tmp2,src,tmp);
    SetAF(tmp2,tmp,src);
    SetZFW(tmp2);
    SetSFW(tmp2);
    SetPF(tmp2);

    regs.w[AX]=tmp2;
}


static INLINE2 void i_cs(void)
{
    /* Opcode 0x2e */

    base[DS] = base[SS] = base[CS];

    instruction[GetMemInc(base[CS],ip)]();

    base[DS] = SegToMemPtr(DS);
    base[SS] = SegToMemPtr(SS);
}

static INLINE2 void i_das(void)
{
    if (AF || ((regs.b[AL] & 0xf) > 9))
    {
        regs.b[AL] -= 6;
        AF = 1;
    }
    else
        AF = 0;

    if (CF || (regs.b[AL] > 0x9f))
    {
        regs.b[AL] -= 0x60;
        CF = 1;
    }
    else
        CF = 0;

    SetPF(regs.b[AL]);
    SetSFB(regs.b[AL]);
    SetZFB(regs.b[AL]);
}


    /* most XOR instructions go here */

LogicalOp(xor,^)


static INLINE2 void i_ss(void)
{
    /* Opcode 0x36 */

    base[DS] = base[SS];

    instruction[GetMemInc(base[CS],ip)]();

    base[DS] = SegToMemPtr(DS);
}

static INLINE2 void i_aaa(void)
{
    /* Opcode 0x37 */
    if (AF || ((regs.b[AL] & 0xf) > 9))
    {
        regs.b[AL] += 6;
        regs.b[AH] += 1;
        AF = 1;
        CF = 1;
    }
    else {
        AF = 0;
        CF = 0;
    }
    regs.b[AL] &= 0x0F;
}


static INLINE2 void i_cmp_br8(void)
{
    /* Opcode 0x38 */

    unsigned ModRM = (unsigned)GetMemInc(base[CS],ip);
    register unsigned src = (unsigned)GetModRMRegB(ModRM);
    register unsigned tmp = GetModRMRMB(ModRM);
    register unsigned tmp2 = tmp;

    tmp -= src;

    SetCFB_Sub(tmp,tmp2);
    SetOFB_Sub(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);
}


static INLINE2 void i_cmp_wr16(void)
{
    /* Opcode 0x39 */

    unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned tmp2 = GetModRMRegW(ModRM);
    register unsigned tmp1 = GetModRMRMW(ModRM);
    register unsigned tmp3;

    tmp3 = tmp1-tmp2;

    SetCFW_Sub(tmp2,tmp1);
    SetOFW_Sub(tmp3,tmp2,tmp1);
    SetAF(tmp3,tmp2,tmp1);
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);
}


static INLINE2 void i_cmp_r8b(void)
{
    /* Opcode 0x3a */

    unsigned ModRM = (unsigned)GetMemInc(base[CS],ip);
    register unsigned tmp = (unsigned)GetModRMRegB(ModRM);
    register unsigned src = GetModRMRMB(ModRM);
    register unsigned tmp2 = tmp;

    tmp -= src;

    SetCFB_Sub(tmp,tmp2);
    SetOFB_Sub(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);
}

static INLINE2 void i_cmp_r16w(void)
{
    /* Opcode 0x3b */

    unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned tmp1 = GetModRMRegW(ModRM);
    register unsigned tmp2 = GetModRMRMW(ModRM);
    register unsigned tmp3;

    tmp3 = tmp1-tmp2;

    SetCFW_Sub(tmp2,tmp1);
    SetOFW_Sub(tmp3,tmp2,tmp1);
    SetAF(tmp3,tmp2,tmp1);
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);
}


static INLINE2 void i_cmp_ald8(void)
{
    /* Opcode 0x3c */

    register unsigned src = GetMemInc(base[CS],ip);
    register unsigned tmp = regs.b[AL];
    register unsigned tmp1 = tmp;

    tmp1 -= src;

    SetCFB_Sub(src,tmp);
    SetOFB_Sub(tmp1,src,tmp);
    SetAF(tmp1,src,tmp);
    SetZFB(tmp1);
    SetSFB(tmp1);
    SetPF(tmp1);
}


static INLINE2 void i_cmp_axd16(void)
{
    /* Opcode 0x3d */

    register unsigned src;
    register unsigned tmp = regs.w[AX];
    register unsigned tmp2 = tmp;

    src = GetMemInc(base[CS],ip);
    src += (GetMemInc(base[CS],ip) << 8);

    tmp2 -= src;

    SetCFW_Sub(src,tmp);
    SetOFW_Sub(tmp2,src,tmp);
    SetAF(tmp2,tmp,src);
    SetZFW(tmp2);
    SetSFW(tmp2);
    SetPF(tmp2);
}


static INLINE2 void i_ds(void)
{
    /* Opcode 0x3e */

    base[SS] = base[DS];

    instruction[GetMemInc(base[CS],ip)]();

    base[SS] = SegToMemPtr(SS);
}

static INLINE2 void i_aas(void)
{
    /* Opcode 0x3f */
    if (AF || ((regs.b[AL] & 0xf) > 9))
    {
        regs.b[AL] -= 6;
        regs.b[AH] -= 1;
        AF = 1;
        CF = 1;
    }
    else {
        AF = 0;
        CF = 0;
    }
    regs.b[AL] &= 0x0F;
}

static INLINE2 void i_inc_ax(void)
{
    /* Opcode 0x40 */
    IncWordReg(AX);
}


static INLINE2 void i_inc_cx(void)
{
    /* Opcode 0x41 */
    IncWordReg(CX);
}


static INLINE2 void i_inc_dx(void)
{
    /* Opcode 0x42 */
    IncWordReg(DX);
}


static INLINE2 void i_inc_bx(void)
{
    /* Opcode 0x43 */
    IncWordReg(BX);
}


static INLINE2 void i_inc_sp(void)
{
    /* Opcode 0x44 */
    IncWordReg(SP);
}


static INLINE2 void i_inc_bp(void)
{
    /* Opcode 0x45 */
    IncWordReg(BP);
}


static INLINE2 void i_inc_si(void)
{
    /* Opcode 0x46 */
    IncWordReg(SI);
}


static INLINE2 void i_inc_di(void)
{
    /* Opcode 0x47 */
    IncWordReg(DI);
}


static INLINE2 void i_dec_ax(void)
{
    /* Opcode 0x48 */
    DecWordReg(AX);
}


static INLINE2 void i_dec_cx(void)
{
    /* Opcode 0x49 */
    DecWordReg(CX);
}


static INLINE2 void i_dec_dx(void)
{
    /* Opcode 0x4a */
    DecWordReg(DX);
}


static INLINE2 void i_dec_bx(void)
{
    /* Opcode 0x4b */
    DecWordReg(BX);
}


static INLINE2 void i_dec_sp(void)
{
    /* Opcode 0x4c */
    DecWordReg(SP);
}


static INLINE2 void i_dec_bp(void)
{
    /* Opcode 0x4d */
    DecWordReg(BP);
}


static INLINE2 void i_dec_si(void)
{
    /* Opcode 0x4e */
    DecWordReg(SI);
}


static INLINE2 void i_dec_di(void)
{
    /* Opcode 0x4f */
    DecWordReg(DI);
}


static INLINE2 void i_push_ax(void)
{
    /* Opcode 0x50 */
    PushWordReg(AX);
}


static INLINE2 void i_push_cx(void)
{
    /* Opcode 0x51 */
    PushWordReg(CX);
}


static INLINE2 void i_push_dx(void)
{
    /* Opcode 0x52 */
    PushWordReg(DX);
}


static INLINE2 void i_push_bx(void)
{
    /* Opcode 0x53 */
    PushWordReg(BX);
}


static INLINE2 void i_push_sp(void)
{
    /* Opcode 0x54 */
    PushWordReg(SP);
}


static INLINE2 void i_push_bp(void)
{
    /* Opcode 0x55 */
    PushWordReg(BP);
}



static INLINE2 void i_push_si(void)
{
    /* Opcode 0x56 */
    PushWordReg(SI);
}


static INLINE2 void i_push_di(void)
{
    /* Opcode 0x57 */
    PushWordReg(DI);
}


static INLINE2 void i_pop_ax(void)
{
    /* Opcode 0x58 */
    PopWordReg(AX);
}


static INLINE2 void i_pop_cx(void)
{
    /* Opcode 0x59 */
    PopWordReg(CX);
}


static INLINE2 void i_pop_dx(void)
{
    /* Opcode 0x5a */
    PopWordReg(DX);
}


static INLINE2 void i_pop_bx(void)
{
    /* Opcode 0x5b */
    PopWordReg(BX);
}


static INLINE2 void i_pop_sp(void)
{
    /* Opcode 0x5c */
    PopWordReg(SP);
}


static INLINE2 void i_pop_bp(void)
{
    /* Opcode 0x5d */
    PopWordReg(BP);
}


static INLINE2 void i_pop_si(void)
{
    /* Opcode 0x5e */
    PopWordReg(SI);
}


static INLINE2 void i_pop_di(void)
{
    /* Opcode 0x5f */
    PopWordReg(DI);
}

    /* Conditional jumps from 0x70 to 0x7f */

JumpCond(o, OF)                 /* 0x70 = Jump if overflow */
JumpCond(no, !OF)               /* 0x71 = Jump if no overflow */
JumpCond(b, CF)                 /* 0x72 = Jump if below */
JumpCond(nb, !CF)               /* 0x73 = Jump if not below */
JumpCond(z, ZF)                 /* 0x74 = Jump if zero */
JumpCond(nz, !ZF)               /* 0x75 = Jump if not zero */
JumpCond(be, CF || ZF)          /* 0x76 = Jump if below or equal */
JumpCond(nbe, !(CF || ZF))      /* 0x77 = Jump if not below or equal */
JumpCond(s, SF)                 /* 0x78 = Jump if sign */
JumpCond(ns, !SF)               /* 0x79 = Jump if no sign */
JumpCond(p, PF)                 /* 0x7a = Jump if parity */
JumpCond(np, !PF)               /* 0x7b = Jump if no parity */
JumpCond(l,(!(!SF)!=!(!OF))&&!ZF)    /* 0x7c = Jump if less */
JumpCond(nl, ZF||(!(!SF) == !(!OF))) /* 0x7d = Jump if not less */
JumpCond(le, ZF||(!(!SF) != !(!OF))) /* 0x7e = Jump if less than or equal */
JumpCond(nle,(!(!SF)==!(!OF))&&!ZF)  /* 0x7f = Jump if not less than or equal*/


static INLINE2 void i_80pre(void)
{
    /* Opcode 0x80 */
    unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned tmp = GetModRMRMB(ModRM);
    register unsigned src = GetMemInc(base[CS],ip);
    register unsigned tmp2;


    switch (ModRM & 0x38)
    {
    case 0x00:  /* ADD eb,d8 */
        tmp2 = src + tmp;

        SetCFB_Add(tmp2,tmp);
        SetOFB_Add(tmp2,src,tmp);
        SetAF(tmp2,src,tmp);
        SetZFB(tmp2);
        SetSFB(tmp2);
        SetPF(tmp2);

        PutbackModRMRMB(ModRM,(BYTE)tmp2);
        break;
    case 0x08:  /* OR eb,d8 */
        tmp |= src;

        CF = OF = AF = 0;

        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);

        PutbackModRMRMB(ModRM,(BYTE)tmp);
        break;
    case 0x10:  /* ADC eb,d8 */
        src += CF;
        tmp2 = src + tmp;

        CF = tmp2 >> 8;
/*        SetCFB_Add(tmp2,tmp); */
        SetOFB_Add(tmp2,src,tmp);
        SetAF(tmp2,src,tmp);
        SetZFB(tmp2);
        SetSFB(tmp2);
        SetPF(tmp2);

        PutbackModRMRMB(ModRM,(BYTE)tmp2);
        break;
    case 0x18:  /* SBB eb,b8 */
        src += CF;
        tmp2 = tmp;
        tmp -= src;

        CF = (tmp & 0x100) == 0x100;

/*        SetCFB_Sub(tmp,tmp2); */
        SetOFB_Sub(tmp,src,tmp2);
        SetAF(tmp,src,tmp2);
        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);

        PutbackModRMRMB(ModRM,(BYTE)tmp);
        break;
    case 0x20:  /* AND eb,d8 */
        tmp &= src;

        CF = OF = AF = 0;

        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);

        PutbackModRMRMB(ModRM,(BYTE)tmp);
        break;
    case 0x28:  /* SUB eb,d8 */
        tmp2 = tmp;
        tmp -= src;

        SetCFB_Sub(tmp,tmp2);
        SetOFB_Sub(tmp,src,tmp2);
        SetAF(tmp,src,tmp2);
        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);

        PutbackModRMRMB(ModRM,(BYTE)tmp);
        break;
    case 0x30:  /* XOR eb,d8 */
        tmp ^= src;

        CF = OF = AF = 0;

        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);

        PutbackModRMRMB(ModRM,(BYTE)tmp);
        break;
    case 0x38:  /* CMP eb,d8 */
        tmp2 = tmp;
        tmp -= src;

        SetCFB_Sub(tmp,tmp2);
        SetOFB_Sub(tmp,src,tmp2);
        SetAF(tmp,src,tmp2);
        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);

        break;
    }
}


static INLINE2 void i_81pre(void)
{
    /* Opcode 0x81 */

    unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned tmp = GetModRMRMW(ModRM);
    register unsigned src = GetMemInc(base[CS],ip);
    register unsigned tmp2;

    src += GetMemInc(base[CS],ip) << 8;

    switch (ModRM & 0x38)
    {
    case 0x00:  /* ADD ew,d16 */
        tmp2 = src + tmp;

        SetCFW_Add(tmp2,tmp);
        SetOFW_Add(tmp2,src,tmp);
        SetAF(tmp2,src,tmp);
        SetZFW(tmp2);
        SetSFW(tmp2);
        SetPF(tmp2);

	PutbackModRMRMW(ModRM,tmp2);
        break;
    case 0x08:  /* OR ew,d16 */
        tmp |= src;

        CF = OF = AF = 0;

        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);

	PutbackModRMRMW(ModRM,tmp);
        break;
    case 0x10:  /* ADC ew,d16 */
        src += CF;
        tmp2 = src + tmp;

        CF = tmp2 >> 16;
/*        SetCFW_Add(tmp2,tmp); */
        SetOFW_Add(tmp2,src,tmp);
        SetAF(tmp2,src,tmp);
        SetZFW(tmp2);
        SetSFW(tmp2);
        SetPF(tmp2);

	PutbackModRMRMW(ModRM,tmp2);
        break;
    case 0x18:  /* SBB ew,d16 */
        src += CF;
        tmp2 = tmp;
        tmp -= src;

        CF = (tmp & 0x10000) == 0x10000;

/*        SetCFW_Sub(tmp,tmp2); */
        SetOFW_Sub(tmp,src,tmp2);
        SetAF(tmp,src,tmp2);
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);

	PutbackModRMRMW(ModRM,tmp);
        break;
    case 0x20:  /* AND ew,d16 */
        tmp &= src;

        CF = OF = AF = 0;

        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);

	PutbackModRMRMW(ModRM,tmp);
        break;
    case 0x28:  /* SUB ew,d16 */
        tmp2 = tmp;
        tmp -= src;

        SetCFW_Sub(tmp,tmp2);
        SetOFW_Sub(tmp,src,tmp2);
        SetAF(tmp,src,tmp2);
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);

	PutbackModRMRMW(ModRM,tmp);
        break;
    case 0x30:  /* XOR ew,d16 */
        tmp ^= src;

        CF = OF = AF = 0;

        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);

	PutbackModRMRMW(ModRM,tmp);
        break;
    case 0x38:  /* CMP ew,d16 */
        tmp2 = tmp;
        tmp -= src;

        SetCFW_Sub(tmp,tmp2);
        SetOFW_Sub(tmp,src,tmp2);
        SetAF(tmp,src,tmp2);
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);

        break;
    }
}


static INLINE2 void i_83pre(void)
{
    /* Opcode 0x83 */

    unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned tmp = GetModRMRMW(ModRM);
    register unsigned src = (WORD)((INT16)((INT8)GetMemInc(base[CS],ip)));
    register unsigned tmp2;

    switch (ModRM & 0x38)
    {
    case 0x00:  /* ADD ew,d8 */
        tmp2 = src + tmp;

        SetCFW_Add(tmp2,tmp);
        SetOFW_Add(tmp2,src,tmp);
        SetAF(tmp2,src,tmp);
        SetZFW(tmp2);
        SetSFW(tmp2);
        SetPF(tmp2);

	PutbackModRMRMW(ModRM,tmp2);
        break;
    case 0x08:  /* OR ew,d8 */
        tmp |= src;

        CF = OF = AF = 0;

        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);

	PutbackModRMRMW(ModRM,tmp);
        break;
    case 0x10:  /* ADC ew,d8 */
        src += CF;
        tmp2 = src + tmp;

        CF = tmp2 >> 16;

/*        SetCFW_Add(tmp2,tmp); */
        SetOFW_Add(tmp2,src,tmp);
        SetAF(tmp2,src,tmp);
        SetZFW(tmp2);
        SetSFW(tmp2);
        SetPF(tmp2);

	PutbackModRMRMW(ModRM,tmp2);
        break;
    case 0x18:  /* SBB ew,d8 */
        src += CF;
        tmp2 = tmp;
        tmp -= src;

        CF = (tmp & 0x10000) == 0x10000;

/*        SetCFW_Sub(tmp,tmp2); */
        SetOFW_Sub(tmp,src,tmp2);
        SetAF(tmp,src,tmp2);
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);

	PutbackModRMRMW(ModRM,tmp);
        break;
    case 0x20:  /* AND ew,d8 */
        tmp &= src;

        CF = OF = AF = 0;

        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);

	PutbackModRMRMW(ModRM,tmp);
        break;
    case 0x28:  /* SUB ew,d8 */
        tmp2 = tmp;
        tmp -= src;

        SetCFW_Sub(tmp,tmp2);
        SetOFW_Sub(tmp,src,tmp2);
        SetAF(tmp,src,tmp2);
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);

	PutbackModRMRMW(ModRM,tmp);
        break;
    case 0x30:  /* XOR ew,d8 */
        tmp ^= src;

        CF = OF = AF = 0;

        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);

	PutbackModRMRMW(ModRM,tmp);
        break;

    case 0x38:  /* CMP ew,d8 */
        tmp2 = tmp;
        tmp -= src;

        SetCFW_Sub(tmp,tmp2);
        SetOFW_Sub(tmp,src,tmp2);
        SetAF(tmp,src,tmp2);
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);

        break;
    }
}


static INLINE2 void i_test_br8(void)
{
    /* Opcode 0x84 */

    unsigned ModRM = (unsigned)GetMemInc(base[CS],ip);
    register unsigned src = (unsigned)GetModRMRegB(ModRM);
    register unsigned tmp = (unsigned)GetModRMRMB(ModRM);
    tmp &= src;
    CF = OF = AF = 0;
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);
}


static INLINE2 void i_test_wr16(void)
{
    /* Opcode 0x85 */

    unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned tmp1 = (unsigned)GetModRMRegW(ModRM);
    register unsigned tmp2 = (unsigned)GetModRMRMW(ModRM);
    register unsigned tmp3 = tmp1 & tmp2;
    CF = OF = AF = 0;
    SetZFW(tmp3);
    SetSFW(tmp3);
    SetPF(tmp3);
}


static INLINE2 void i_xchg_br8(void)
{
    /* Opcode 0x86 */

    unsigned ModRM = (unsigned)GetMemInc(base[CS],ip);
    register BYTE src = GetModRMRegB(ModRM);
    register BYTE tmp = GetModRMRMB(ModRM);
    PutModRMRegB(ModRM,tmp);
    PutbackModRMRMB(ModRM,src);
}


static INLINE2 void i_xchg_wr16(void)
{
    /* Opcode 0x87 */

    unsigned ModRM = GetMemInc(base[CS],ip);
    register WORD src = GetModRMRegW(ModRM);
    register WORD dest = GetModRMRMW(ModRM);
    PutModRMRegW(ModRM,dest);
    PutbackModRMRMW(ModRM,src);
}


static INLINE2 void i_mov_br8(void)
{
    /* Opcode 0x88 */

    register unsigned ModRM = GetMemInc(base[CS],ip);
    register BYTE src = GetModRMRegB(ModRM);
    PutModRMRMB(ModRM,src);
}


static INLINE2 void i_mov_wr16(void)
{
    /* Opcode 0x89 */

    register unsigned ModRM = GetMemInc(base[CS],ip);
    register WORD src = GetModRMRegW(ModRM);
    PutModRMRMW(ModRM,src);
}


static INLINE2 void i_mov_r8b(void)
{
    /* Opcode 0x8a */

    register unsigned ModRM = GetMemInc(base[CS],ip);
    register BYTE src = GetModRMRMB(ModRM);
    PutModRMRegB(ModRM,src);
}


static INLINE2 void i_mov_r16w(void)
{
    /* Opcode 0x8b */

    register unsigned ModRM = GetMemInc(base[CS],ip);
    register WORD src = GetModRMRMW(ModRM);
    PutModRMRegW(ModRM,src);
}


static INLINE2 void i_mov_wsreg(void)
{
    /* Opcode 0x8c */

    register unsigned ModRM = GetMemInc(base[CS],ip);
    PutModRMRMW(ModRM,sregs[(ModRM & 0x38) >> 3]);
}


static INLINE2 void i_lea(void)
{
    /* Opcode 0x8d */

    register unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned src = 0;

    if (ModRMRM[ModRM].offset)
    {
        src = (WORD)((INT16)((INT8)GetMemInc(base[CS],ip)));
        if (ModRMRM[ModRM].offset16)
            src = (GetMemInc(base[CS],ip) << 8) + (BYTE)(src);
    }

    src += regs.w[ModRMRM[ModRM].reg1]+regs.w[ModRMRM[ModRM].reg2];
    PutModRMRegW(ModRM,src);
}


static INLINE2 void i_mov_sregw(void)
{
    /* Opcode 0x8e */

    static int multiple = 0;
    register unsigned ModRM = GetMemInc(base[CS],ip);
    register WORD src = GetModRMRMW(ModRM);

    switch (ModRM & 0x38)
    {
    case 0x00:	/* mov es,... */
        sregs[ES] = src;
        base[ES] = SegToMemPtr(ES);
        break;
    case 0x18:	/* mov ds,... */
        sregs[DS] = src;
        base[DS] = SegToMemPtr(DS);
        break;
    case 0x10:	/* mov ss,... */
        sregs[SS] = src;
        base[SS] = SegToMemPtr(SS);

        if (multiple == 0) /* Prevent unlimited recursion.. */
        {
            multiple = 1;
/*
#ifdef DEBUGGER
            call_debugger(D_TRACE);
#endif
*/
            instruction[GetMemInc(base[CS],ip)]();
            multiple = 0;
        }

        break;
    case 0x08:	/* mov cs,... (hangs 486, but we'll let it through) */
        break;

    }
}


static INLINE2 void i_popw(void)
{
    /* Opcode 0x8f */

    unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned tmp = regs.w[SP];
    WORD tmp2 = GetMemW(base[SS],tmp);
    tmp += 2;
    regs.w[SP]=tmp;
    PutModRMRMW(ModRM,tmp2);
}


static INLINE2 void i_nop(void)
{
    /* Opcode 0x90 */
}


static INLINE2 void i_xchg_axcx(void)
{
    /* Opcode 0x91 */
    XchgAXReg(CX);
}


static INLINE2 void i_xchg_axdx(void)
{
    /* Opcode 0x92 */
    XchgAXReg(DX);
}


static INLINE2 void i_xchg_axbx(void)
{
    /* Opcode 0x93 */
    XchgAXReg(BX);
}


static INLINE2 void i_xchg_axsp(void)
{
    /* Opcode 0x94 */
    XchgAXReg(SP);
}


static INLINE2 void i_xchg_axbp(void)
{
    /* Opcode 0x95 */
    XchgAXReg(BP);
}


static INLINE2 void i_xchg_axsi(void)
{
    /* Opcode 0x96 */
    XchgAXReg(SI);
}


static INLINE2 void i_xchg_axdi(void)
{
    /* Opcode 0x97 */
    XchgAXReg(DI);
}

static INLINE2 void i_cbw(void)
{
    /* Opcode 0x98 */

    regs.b[AH] = (regs.b[AL] & 0x80) ? 0xff : 0;
}

static INLINE2 void i_cwd(void)
{
    /* Opcode 0x99 */

    regs.w[DX] = (regs.b[AH] & 0x80) ? ChangeE(0xffff) : ChangeE(0);
}


static INLINE2 void i_call_far(void)
{
    register unsigned tmp, tmp1, tmp2;

    tmp = GetMemInc(base[CS],ip);
    tmp += GetMemInc(base[CS],ip) << 8;

    tmp2 = GetMemInc(base[CS],ip);
    tmp2 += GetMemInc(base[CS],ip) << 8;

    tmp1 = (WORD)(regs.w[SP]-2);

    PutMemW(base[SS],tmp1,sregs[CS]);
    tmp1 = (WORD)(tmp1-2);
    PutMemW(base[SS],tmp1,ip);

    regs.w[SP]=tmp1;

    ip = (WORD)tmp;
    sregs[CS] = (WORD)tmp2;
    base[CS] = SegToMemPtr(CS);
}


static INLINE2 void i_wait(void)
{
    /* Opcode 0x9b */

    return;
}


static INLINE2 void i_pushf(void)
{
    /* Opcode 0x9c */

    register unsigned tmp1 = (regs.w[SP]-2);
    WORD tmp2 = CompressFlags() | 0xf000;

    PutMemW(base[SS],tmp1,tmp2);
    regs.w[SP]=tmp1;
}


static INLINE2 void i_popf(void)
{
    /* Opcode 0x9d */

    register unsigned tmp = regs.w[SP];
    unsigned tmp2 = (unsigned)GetMemW(base[SS],tmp);

    ExpandFlags(tmp2);
    tmp += 2;
    regs.w[SP]=tmp;

    if (IF && int_blocked)
    {
        int_pending = int_blocked;
        int_blocked = 0;
        D2(printf("Unblocking interrupt\n"););
    }
    if (TF) trap();
}


static INLINE2 void i_sahf(void)
{
    /* Opcode 0x9e */

    unsigned tmp = (CompressFlags() & 0xff00) | (regs.b[AH] & 0xd5);

    ExpandFlags(tmp);
}


static INLINE2 void i_lahf(void)
{
    /* Opcode 0x9f */

    regs.b[AH] = CompressFlags() & 0xff;
}

static INLINE2 void i_mov_aldisp(void)
{
    /* Opcode 0xa0 */

    register unsigned addr;

    addr = GetMemInc(base[CS],ip);
    addr += GetMemInc(base[CS], ip) << 8;

    regs.b[AL] = GetMemB(base[DS], addr);
}


static INLINE2 void i_mov_axdisp(void)
{
    /* Opcode 0xa1 */

    register unsigned addr;

    addr = GetMemInc(base[CS], ip);
    addr += GetMemInc(base[CS], ip) << 8;

    regs.b[AL] = GetMemB(base[DS], addr);
    regs.b[AH] = GetMemB(base[DS], addr+1);
}


static INLINE2 void i_mov_dispal(void)
{
    /* Opcode 0xa2 */

    register unsigned addr;

    addr = GetMemInc(base[CS],ip);
    addr += GetMemInc(base[CS], ip) << 8;

    PutMemB(base[DS], addr, regs.b[AL]);
}


static INLINE2 void i_mov_dispax(void)
{
    /* Opcode 0xa3 */

    register unsigned addr;

    addr = GetMemInc(base[CS], ip);
    addr += GetMemInc(base[CS], ip) << 8;

    PutMemB(base[DS], addr, regs.b[AL]);
    PutMemB(base[DS], addr+1, regs.b[AH]);
}


static INLINE2 void i_movsb(void)
{
    /* Opcode 0xa4 */

    register unsigned di = regs.w[DI];
    register unsigned si = regs.w[SI];

    BYTE tmp = GetMemB(base[DS],si);

    PutMemB(base[ES],di, tmp);

    di += -2*DF +1;
    si += -2*DF +1;

    regs.w[DI]=di;
    regs.w[SI]=si;
}


static INLINE2 void i_movsw(void)
{
    /* Opcode 0xa5 */

    register unsigned di = regs.w[DI];
    register unsigned si = regs.w[SI];

    WORD tmp = GetMemW(base[DS],si);

    PutMemW(base[ES],di, tmp);

    di += -4*DF +2;
    si += -4*DF +2;

    regs.w[DI]=di;
    regs.w[SI]=si;
}


static INLINE2 void i_cmpsb(void)
{
    /* Opcode 0xa6 */

    unsigned di = regs.w[DI];
    unsigned si = regs.w[SI];
    unsigned src = GetMemB(base[ES], di);
    unsigned tmp = GetMemB(base[DS], si);
    unsigned tmp2 = tmp;

    tmp -= src;

    SetCFB_Sub(tmp,tmp2);
    SetOFB_Sub(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);

    di += -2*DF +1;
    si += -2*DF +1;

    regs.w[DI]=di;
    regs.w[SI]=si;
}


static INLINE2 void i_cmpsw(void)
{
    /* Opcode 0xa7 */

    unsigned di = regs.w[DI];
    unsigned si = regs.w[SI];
    unsigned src = GetMemW(base[ES], di);
    unsigned tmp = GetMemW(base[DS], si);
    unsigned tmp2 = tmp;

    tmp -= src;

    SetCFW_Sub(tmp,tmp2);
    SetOFW_Sub(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFW(tmp);
    SetSFW(tmp);
    SetPF(tmp);

    di += -4*DF +2;
    si += -4*DF +2;

    regs.w[DI]=di;
    regs.w[SI]=si;
}


static INLINE2 void i_test_ald8(void)
{
    /* Opcode 0xa8 */

    register unsigned tmp1 = (unsigned)regs.b[AL];
    register unsigned tmp2 = (unsigned) GetMemInc(base[CS],ip);

    tmp1 &= tmp2;
    CF = OF = AF = 0;
    SetZFB(tmp1);
    SetSFB(tmp1);
    SetPF(tmp1);
}


static INLINE2 void i_test_axd16(void)
{
    /* Opcode 0xa9 */

    register unsigned tmp1 = (unsigned)regs.w[AX];
    register unsigned tmp2;

    tmp2 = (unsigned) GetMemInc(base[CS],ip);
    tmp2 += GetMemInc(base[CS],ip) << 8;

    tmp1 &= tmp2;
    CF = OF = AF = 0;
    SetZFW(tmp1);
    SetSFW(tmp1);
    SetPF(tmp1);
}

static INLINE2 void i_stosb(void)
{
    /* Opcode 0xaa */

    register unsigned di = regs.w[DI];

    PutMemB(base[ES],di,regs.b[AL]);
    di += -2*DF +1;
    regs.w[DI]=di;
}

static INLINE2 void i_stosw(void)
{
    /* Opcode 0xab */

    register unsigned di = regs.w[DI];

    PutMemB(base[ES],di,regs.b[AL]);
    PutMemB(base[ES],di+1,regs.b[AH]);
    di += -4*DF +2;;
    regs.w[DI]=di;
}

static INLINE2 void i_lodsb(void)
{
    /* Opcode 0xac */

    register unsigned si = regs.w[SI];

    regs.b[AL] = GetMemB(base[DS],si);
    si += -2*DF +1;
    regs.w[SI]=si;
}

static INLINE2 void i_lodsw(void)
{
    /* Opcode 0xad */

    register unsigned si = regs.w[SI];
    register unsigned tmp = GetMemW(base[DS],si);

    si +=  -4*DF+2;
    regs.w[SI]=si;
    regs.w[AX]=tmp;
}

static INLINE2 void i_scasb(void)
{
    /* Opcode 0xae */

    unsigned di = regs.w[DI];
    unsigned src = GetMemB(base[ES], di);
    unsigned tmp = regs.b[AL];
    unsigned tmp2 = tmp;

    tmp -= src;

    SetCFB_Sub(tmp,tmp2);
    SetOFB_Sub(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFB(tmp);
    SetSFB(tmp);
    SetPF(tmp);

    di += -2*DF +1;

    regs.w[DI]=di;
}


static INLINE2 void i_scasw(void)
{
    /* Opcode 0xaf */

    unsigned di = regs.w[DI];
    unsigned src = GetMemW(base[ES], di);
    unsigned tmp = regs.w[AX];
    unsigned tmp2 = tmp;

    tmp -= src;

    SetCFW_Sub(tmp,tmp2);
    SetOFW_Sub(tmp,src,tmp2);
    SetAF(tmp,src,tmp2);
    SetZFW(tmp);
    SetSFW(tmp);
    SetPF(tmp);

    di += -4*DF +2;

    regs.w[DI]=di;
}

static INLINE2 void i_mov_ald8(void)
{
    /* Opcode 0xb0 */

    regs.b[AL] = GetMemInc(base[CS],ip);
}


static INLINE2 void i_mov_cld8(void)
{
    /* Opcode 0xb1 */

    regs.b[CL] = GetMemInc(base[CS],ip);
}


static INLINE2 void i_mov_dld8(void)
{
    /* Opcode 0xb2 */

    regs.b[DL] = GetMemInc(base[CS],ip);
}


static INLINE2 void i_mov_bld8(void)
{
    /* Opcode 0xb3 */

    regs.b[BL] = GetMemInc(base[CS],ip);
}


static INLINE2 void i_mov_ahd8(void)
{
    /* Opcode 0xb4 */

    regs.b[AH] = GetMemInc(base[CS],ip);
}


static INLINE2 void i_mov_chd8(void)
{
    /* Opcode 0xb5 */

    regs.b[CH] = GetMemInc(base[CS],ip);
}


static INLINE2 void i_mov_dhd8(void)
{
    /* Opcode 0xb6 */

    regs.b[DH] = GetMemInc(base[CS],ip);
}


static INLINE2 void i_mov_bhd8(void)
{
    /* Opcode 0xb7 */

    regs.b[BH] = GetMemInc(base[CS],ip);
}


static INLINE2 void i_mov_axd16(void)
{
    /* Opcode 0xb8 */

    regs.b[AL] = GetMemInc(base[CS],ip);
    regs.b[AH] = GetMemInc(base[CS],ip);
}


static INLINE2 void i_mov_cxd16(void)
{
    /* Opcode 0xb9 */

    regs.b[CL] = GetMemInc(base[CS],ip);
    regs.b[CH] = GetMemInc(base[CS],ip);
}


static INLINE2 void i_mov_dxd16(void)
{
    /* Opcode 0xba */

    regs.b[DL] = GetMemInc(base[CS],ip);
    regs.b[DH] = GetMemInc(base[CS],ip);
}


static INLINE2 void i_mov_bxd16(void)
{
    /* Opcode 0xbb */

    regs.b[BL] = GetMemInc(base[CS],ip);
    regs.b[BH] = GetMemInc(base[CS],ip);
}


static INLINE2 void i_mov_spd16(void)
{
    /* Opcode 0xbc */

    regs.b[SPL] = GetMemInc(base[CS],ip);
    regs.b[SPH] = GetMemInc(base[CS],ip);
}


static INLINE2 void i_mov_bpd16(void)
{
    /* Opcode 0xbd */

    regs.b[BPL] = GetMemInc(base[CS],ip);
    regs.b[BPH] = GetMemInc(base[CS],ip);
}


static INLINE2 void i_mov_sid16(void)
{
    /* Opcode 0xbe */

    regs.b[SIL] = GetMemInc(base[CS],ip);
    regs.b[SIH] = GetMemInc(base[CS],ip);
}


static INLINE2 void i_mov_did16(void)
{
    /* Opcode 0xbf */

    regs.b[DIL] = GetMemInc(base[CS],ip);
    regs.b[DIH] = GetMemInc(base[CS],ip);
}


static INLINE2 void i_ret_d16(void)
{
    /* Opcode 0xc2 */

    register unsigned tmp = regs.w[SP];
    register unsigned count;

    count = GetMemInc(base[CS],ip);
    count += GetMemInc(base[CS],ip) << 8;

    ip = GetMemW(base[SS],tmp);
    tmp += count+2;
    regs.w[SP]=tmp;
}


static INLINE2 void i_ret(void)
{
    /* Opcode 0xc3 */

    register unsigned tmp = regs.w[SP];
    ip = GetMemW(base[SS],tmp);
    tmp += 2;
    regs.w[SP]=tmp;
}


static INLINE2 void i_les_dw(void)
{
    /* Opcode 0xc4 */

    unsigned ModRM = GetMemInc(base[CS],ip);
    WORD tmp = GetModRMRMW(ModRM);

    PutModRMRegW(ModRM, tmp);
    sregs[ES] = GetnextModRMRMW(ModRM);
    base[ES] = SegToMemPtr(ES);
}


static INLINE2 void i_lds_dw(void)
{
    /* Opcode 0xc5 */

    unsigned ModRM = GetMemInc(base[CS],ip);
    WORD tmp = GetModRMRMW(ModRM);

    PutModRMRegW(ModRM,tmp);
    sregs[DS] = GetnextModRMRMW(ModRM);
    base[DS] = SegToMemPtr(DS);
}

static INLINE2 void i_mov_bd8(void)
{
    /* Opcode 0xc6 */

    unsigned ModRM = GetMemInc(base[CS],ip);
    PutImmModRMRMB(ModRM);
}


static INLINE2 void i_mov_wd16(void)
{
    /* Opcode 0xc7 */

    unsigned ModRM = GetMemInc(base[CS],ip);
    PutImmModRMRMW(ModRM);
}


static INLINE2 void i_retf_d16(void)
{
    /* Opcode 0xca */

    register unsigned tmp = regs.w[SP];
    register unsigned count;

    count = GetMemInc(base[CS],ip);
    count += GetMemInc(base[CS],ip) << 8;

    ip = GetMemW(base[SS],tmp);
    tmp = (WORD)(tmp+2);
    sregs[CS] = GetMemW(base[SS],tmp);
    base[CS] = SegToMemPtr(CS);
    tmp += count+2;
    regs.w[SP]=tmp;
}


static INLINE2 void i_retf(void)
{
    /* Opcode 0xcb */

    register unsigned tmp = regs.w[SP];
    ip = GetMemW(base[SS],tmp);
    tmp = (WORD)(tmp+2);
    sregs[CS] = GetMemW(base[SS],tmp);
    base[CS] = SegToMemPtr(CS);
    tmp += 2;
    regs.w[SP]=tmp;
}


static INLINE2 void i_int3(void)
{
    /* Opcode 0xcc */

    I86_interrupt(3);
}


static INLINE2 void i_int(void)
{
    /* Opcode 0xcd */

    unsigned int_num = GetMemInc(base[CS],ip);

    I86_interrupt(int_num);
}


static INLINE2 void i_into(void)
{
    /* Opcode 0xce */

    if (OF)
        I86_interrupt(4);
}


static INLINE2 void i_iret(void)
{
    /* Opcode 0xcf */

    register unsigned tmp = regs.w[SP];
    ip = GetMemW(base[SS],tmp);
    tmp = (WORD)(tmp+2);
    sregs[CS] = GetMemW(base[SS],tmp);
    base[CS] = SegToMemPtr(CS);
    tmp += 2;
    regs.w[SP]=tmp;

    i_popf();
#ifdef DEBUGGER
    call_debugger(D_TRACE);
#endif
}


static INLINE2 void i_d0pre(void)
{
    /* Opcode 0xd0 */

    unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned tmp = GetModRMRMB(ModRM);
    register unsigned tmp2 = tmp;

    switch (ModRM & 0x38)
    {
    case 0x00:  /* ROL eb,1 */
        CF = (tmp & 0x80) != 0;
	PutbackModRMRMB(ModRM,(tmp << 1) + CF);
        OF = !(!(tmp & 0x40)) != CF;
        break;
    case 0x08:  /* ROR eb,1 */
        CF = (tmp & 0x01) != 0;
	PutbackModRMRMB(ModRM, (tmp >> 1) + (CF << 7));
        OF = !(!(tmp & 0x80)) != CF;
        break;
    case 0x10:  /* RCL eb,1 */
        OF = (tmp ^ (tmp << 1)) & 0x80;
	PutbackModRMRMB(ModRM,(tmp << 1) + CF);
        CF = (tmp & 0x80) != 0;
        break;
    case 0x18:  /* RCR eb,1 */
	PutbackModRMRMB(ModRM,(tmp >> 1) + (CF << 7));
        OF = !(!(tmp & 0x80)) != CF;
        CF = (tmp & 0x01) != 0;
        break;
    case 0x20:  /* SHL eb,1 */
    case 0x30:
        tmp += tmp;

        SetCFB_Add(tmp,tmp2);
        SetOFB_Add(tmp,tmp2,tmp2);
        AF = 1;
        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);

	PutbackModRMRMB(ModRM, (BYTE)tmp);
        break;
    case 0x28:  /* SHR eb,1 */
        CF = (tmp & 0x01) != 0;
        OF = tmp & 0x80;

        tmp2 = tmp >> 1;

        SetSFB(tmp2);
        SetPF(tmp2);
        SetZFB(tmp2);
        AF = 1;
	PutbackModRMRMB(ModRM,(BYTE)tmp2);
        break;
    case 0x38:  /* SAR eb,1 */
        CF = (tmp & 0x01) != 0;
        OF = 0;

        tmp2 = (tmp >> 1) | (tmp & 0x80);

        SetSFB(tmp2);
        SetPF(tmp2);
        SetZFB(tmp2);
        AF = 1;
	PutbackModRMRMB(ModRM,(BYTE)tmp2);
        break;
    }
}


static INLINE2 void i_d1pre(void)
{
    /* Opcode 0xd1 */

    unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned tmp = GetModRMRMW(ModRM);
    register unsigned tmp2 = tmp;

    switch (ModRM & 0x38)
    {
    case 0x00:  /* ROL ew,1 */
        CF = (tmp & 0x8000) != 0;
        tmp2 = (tmp << 1) + CF;
        OF = !(!(tmp & 0x4000)) != CF;
	PutbackModRMRMW(ModRM,tmp2);
        break;
    case 0x08:  /* ROR ew,1 */
        CF = (tmp & 0x01) != 0;
        tmp2 = (tmp >> 1) + ((unsigned)CF << 15);
        OF = !(!(tmp & 0x8000)) != CF;
	PutbackModRMRMW(ModRM,tmp2);
        break;
    case 0x10:  /* RCL ew,1 */
        tmp2 = (tmp << 1) + CF;
        OF = (tmp ^ (tmp << 1)) & 0x8000;
        CF = (tmp & 0x8000) != 0;
	PutbackModRMRMW(ModRM,tmp2);
        break;
    case 0x18:  /* RCR ew,1 */
        tmp2 = (tmp >> 1) + ((unsigned)CF << 15);
        OF = !(!(tmp & 0x8000)) != CF;
        CF = (tmp & 0x01) != 0;
	PutbackModRMRMW(ModRM,tmp2);
        break;
    case 0x20:  /* SHL ew,1 */
    case 0x30:
        tmp += tmp;

        SetCFW_Add(tmp,tmp2);
        SetOFW_Add(tmp,tmp2,tmp2);
        AF = 1;
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);

	PutbackModRMRMW(ModRM,tmp);
        break;
    case 0x28:  /* SHR ew,1 */
        CF = (tmp & 0x01) != 0;
        OF = tmp & 0x8000;

        tmp2 = tmp >> 1;

        SetSFW(tmp2);
        SetPF(tmp2);
        SetZFW(tmp2);
        AF = 1;
	PutbackModRMRMW(ModRM,tmp2);
        break;
    case 0x38:  /* SAR ew,1 */
        CF = (tmp & 0x01) != 0;
        OF = 0;

        tmp2 = (tmp >> 1) | (tmp & 0x8000);

        SetSFW(tmp2);
        SetPF(tmp2);
        SetZFW(tmp2);
        AF = 1;
	PutbackModRMRMW(ModRM,tmp2);
        break;
    }
}


static INLINE2 void i_d2pre(void)
{
    /* Opcode 0xd2 */

    unsigned ModRM;
    register unsigned tmp;
    register unsigned tmp2;
    unsigned count;

    if (regs.b[CL] == 1)
    {
        i_d0pre();
        D(printf("Skipping CL processing\n"););
        return;
    }

    ModRM = GetMemInc(base[CS],ip);
    tmp = (unsigned)GetModRMRMB(ModRM);
    count = (unsigned)regs.b[CL];

    switch (ModRM & 0x38)
    {
    case 0x00:  /* ROL eb,CL */
        for (; count > 0; count--)
        {
            CF = (tmp & 0x80) != 0;
            tmp = (tmp << 1) + CF;
        }
	PutbackModRMRMB(ModRM,(BYTE)tmp);
        break;
    case 0x08:  /* ROR eb,CL */
        for (; count > 0; count--)
        {
            CF = (tmp & 0x01) != 0;
            tmp = (tmp >> 1) + (CF << 7);
        }
	PutbackModRMRMB(ModRM,(BYTE)tmp);
        break;
    case 0x10:  /* RCL eb,CL */
        for (; count > 0; count--)
        {
            tmp2 = CF;
            CF = (tmp & 0x80) != 0;
            tmp = (tmp << 1) + tmp2;
        }
	PutbackModRMRMB(ModRM,(BYTE)tmp);
        break;
    case 0x18:  /* RCR eb,CL */
        for (; count > 0; count--)
        {
            tmp2 = (tmp >> 1) + (CF << 7);
            CF = (tmp & 0x01) != 0;
            tmp = tmp2;
        }
	PutbackModRMRMB(ModRM,(BYTE)tmp);
        break;
    case 0x20:
    case 0x30:  /* SHL eb,CL */
        if (count >= 9)
        {
            CF = 0;             /* Not sure about this... */
            tmp = 0;
        }
        else
        {
            CF = ((tmp << (count-1)) & 0x80) != 0;
            tmp <<= count;
        }

        AF = 1;
        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);

	PutbackModRMRMB(ModRM,(BYTE)tmp);
        break;
    case 0x28:  /* SHR eb,CL */
        if (count >= 9)
        {
            CF = 0;             /* Not sure about this... */
            tmp = 0;
        }
        else
        {
            CF = ((tmp >> (count-1)) & 0x1) != 0;
            tmp >>= count;
        }

        SetSFB(tmp);
        SetPF(tmp);
        SetZFB(tmp);
        AF = 1;
	PutbackModRMRMB(ModRM,(BYTE)tmp);
        break;
    case 0x38:  /* SAR eb,CL */
        tmp2 = tmp & 0x80;
        CF = (((INT8)tmp >> (count-1)) & 0x01) != 0;
        for (; count > 0; count--)
            tmp = (tmp >> 1) | tmp2;

        SetSFB(tmp);
        SetPF(tmp);
        SetZFB(tmp);
        AF = 1;
	PutbackModRMRMB(ModRM,(BYTE)tmp);
        break;
    }
}


static INLINE2 void i_d3pre(void)
{
    /* Opcode 0xd3 */

    unsigned ModRM;
    register unsigned tmp;
    register unsigned tmp2;
    unsigned count;

    if (regs.b[CL] == 1)
    {
        i_d1pre();
        return;
    }

    ModRM = GetMemInc(base[CS],ip);
    tmp = GetModRMRMW(ModRM);
    count = (unsigned)regs.b[CL];

    switch (ModRM & 0x38)
    {
    case 0x00:  /* ROL ew,CL */
        for (; count > 0; count--)
        {
            CF = (tmp & 0x8000) != 0;
            tmp = (tmp << 1) + CF;
        }
	PutbackModRMRMW(ModRM,tmp);
        break;
    case 0x08:  /* ROR ew,CL */
        for (; count > 0; count--)
        {
            CF = (tmp & 0x01) != 0;
            tmp = (tmp >> 1) + (CF << 15);
        }
	PutbackModRMRMW(ModRM,tmp);
        break;
    case 0x10:  /* RCL ew,CL */
        for (; count > 0; count--)
        {
            tmp2 = CF;
            CF = (tmp & 0x8000) != 0;
            tmp = (tmp << 1) + tmp2;
        }
	PutbackModRMRMW(ModRM,tmp);
        break;
    case 0x18:  /* RCR ew,CL */
        for (; count > 0; count--)
        {
            tmp2 = (tmp >> 1) + (CF << 15);
            CF = (tmp & 0x01) != 0;
            tmp = tmp2;
        }
	PutbackModRMRMW(ModRM,tmp);
        break;
    case 0x20:
    case 0x30:  /* SHL ew,CL */
        if (count >= 17)
        {
            CF = 0;             /* Not sure about this... */
            tmp = 0;
        }
        else
        {
            CF = ((tmp << (count-1)) & 0x8000) != 0;
            tmp <<= count;
        }

        AF = 1;
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);

	PutbackModRMRMW(ModRM,tmp);
        break;
    case 0x28:  /* SHR ew,CL */
        if (count >= 17)
        {
            CF = 0;             /* Not sure about this... */
            tmp = 0;
        }
        else
        {
            CF = ((tmp >> (count-1)) & 0x1) != 0;
            tmp >>= count;
        }

        SetSFW(tmp);
        SetPF(tmp);
        SetZFW(tmp);
        AF = 1;
	PutbackModRMRMW(ModRM,tmp);
        break;
    case 0x38:  /* SAR ew,CL */
        tmp2 = tmp & 0x8000;
        CF = (((INT16)tmp >> (count-1)) & 0x01) != 0;
        for (; count > 0; count--)
            tmp = (tmp >> 1) | tmp2;

        SetSFW(tmp);
        SetPF(tmp);
        SetZFW(tmp);
        AF = 1;
	PutbackModRMRMW(ModRM,tmp);
        break;
    }
}

static INLINE2 void i_aam(void)
{
    /* Opcode 0xd4 */
    unsigned mult = GetMemInc(base[CS],ip);

	if (mult == 0)
        I86_interrupt(0);
    else
    {
        regs.b[AH] = regs.b[AL] / mult;
        regs.b[AL] %= mult;

        SetPF(regs.b[AL]);
        SetZFW(regs.w[AX]);
        SetSFB(regs.b[AH]);
    }
}


static INLINE2 void i_aad(void)
{
    /* Opcode 0xd5 */
    unsigned mult = GetMemInc(base[CS],ip);

    regs.b[AL] = regs.b[AH] * mult + regs.b[AL];
    regs.b[AH] = 0;

    SetPF(regs.b[AL]);
    SetZFB(regs.b[AL]);
    SF = 0;
}

static INLINE2 void i_xlat(void)
{
    /* Opcode 0xd7 */

    unsigned dest = regs.w[BX]+regs.b[AL];

    regs.b[AL] = GetMemB(base[DS], dest);
}

static INLINE2 void i_escape(void)
{
    /* Opcodes 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde and 0xdf */

    unsigned ModRM = GetMemInc(base[CS],ip);
    GetModRMRMB(ModRM);
}

static INLINE2 void i_loopne(void)
{
    /* Opcode 0xe0 */

    register int disp = (int)((INT8)GetMemInc(base[CS],ip));
    register unsigned tmp = regs.w[CX]-1;

    regs.w[CX]=tmp;

    if (!ZF && tmp) ip = (WORD)(ip+disp);
}

static INLINE2 void i_loope(void)
{
    /* Opcode 0xe1 */

    register int disp = (int)((INT8)GetMemInc(base[CS],ip));
    register unsigned tmp = regs.w[CX]-1;

    regs.w[CX]=tmp;

    if (ZF && tmp) ip = (WORD)(ip+disp);
}

static INLINE2 void i_loop(void)
{
    /* Opcode 0xe2 */

    register int disp = (int)((INT8)GetMemInc(base[CS],ip));
    register unsigned tmp = regs.w[CX]-1;

    regs.w[CX]=tmp;

    if (tmp) ip = (WORD)(ip+disp);
}

static INLINE2 void i_jcxz(void)
{
    /* Opcode 0xe3 */

    register int disp = (int)((INT8)GetMemInc(base[CS],ip));

    if (regs.w[CX] == 0)
        ip = (WORD)(ip+disp);
}

static INLINE2 void i_inal(void)
{
    /* Opcode 0xe4 */

    unsigned port = GetMemInc(base[CS],ip);

    regs.b[AL] = read_port(port);
}

static INLINE2 void i_inax(void)
{
    /* Opcode 0xe5 */

    unsigned port = GetMemInc(base[CS],ip);

    regs.b[AL] = read_port(port);
    regs.b[AH] = read_port(port+1);
}

static INLINE2 void i_outal(void)
{
    /* Opcode 0xe6 */

    unsigned port = GetMemInc(base[CS],ip);

    write_port(port, regs.b[AL]);
}

static INLINE2 void i_outax(void)
{
    /* Opcode 0xe7 */

    unsigned port = GetMemInc(base[CS],ip);

    write_port(port, regs.b[AL]);
    write_port(port+1, regs.b[AH]);
}

static INLINE2 void i_call_d16(void)
{
    /* Opcode 0xe8 */

    register unsigned tmp;
    register unsigned tmp1 = (WORD)(regs.w[SP]-2);

    tmp = GetMemInc(base[CS],ip);
    tmp += GetMemInc(base[CS],ip) << 8;

    PutMemW(base[SS],tmp1,ip);
    regs.w[SP]=tmp1;

    ip = (WORD)(ip+(INT16)tmp);
}


static INLINE2 void i_jmp_d16(void)
{
    /* Opcode 0xe9 */

    register int tmp = GetMemInc(base[CS],ip);
    tmp += GetMemInc(base[CS],ip) << 8;

    ip = (WORD)(ip+(INT16)tmp);
}


static INLINE2 void i_jmp_far(void)
{
    /* Opcode 0xea */

    register unsigned tmp,tmp1;

    tmp = GetMemInc(base[CS],ip);
    tmp += GetMemInc(base[CS],ip) << 8;

    tmp1 = GetMemInc(base[CS],ip);
    tmp1 += GetMemInc(base[CS],ip) << 8;

    sregs[CS] = (WORD)tmp1;
    base[CS] = SegToMemPtr(CS);
    ip = (WORD)tmp;
}


static INLINE2 void i_jmp_d8(void)
{
    /* Opcode 0xeb */
    register int tmp = (int)((INT8)GetMemInc(base[CS],ip));
    ip = (WORD)(ip+tmp);
}


static INLINE2 void i_inaldx(void)
{
    /* Opcode 0xec */

    regs.b[AL] = read_port(regs.w[DX]);
}

static INLINE2 void i_inaxdx(void)
{
    /* Opcode 0xed */

    unsigned port = regs.w[DX];

    regs.b[AL] = read_port(port);
    regs.b[AH] = read_port(port+1);
}

static INLINE2 void i_outdxal(void)
{
    /* Opcode 0xee */

    write_port(regs.w[DX], regs.b[AL]);
}

static INLINE2 void i_outdxax(void)
{
    /* Opcode 0xef */
    unsigned port = regs.w[DX];

    write_port(port, regs.b[AL]);
    write_port(port+1, regs.b[AH]);
}

static INLINE2 void i_lock(void)
{
    /* Opcode 0xf0 */
}

static INLINE2 void i_gobios(void)
{
    /* Opcode 0xf1 */

    void (*routine)(void);
    unsigned dest;

    if (GetMemInc(base[CS],ip) != 0xf1)
        i_notdone();

    dest = GetMemInc(base[CS],ip);	/* Must be re-coded sometime.... */
    dest += (GetMemInc(base[CS],ip) << 8);
    dest += (GetMemInc(base[CS],ip) << 16);
    dest += (GetMemInc(base[CS],ip) << 24);

    routine = (void (*)(void))dest;

    routine();
}


static void rep(int flagval)
{
    /* Handles rep- and repnz- prefixes. flagval is the value of ZF for the
       loop  to continue for CMPS and SCAS instructions. */

    unsigned next = GetMemInc(base[CS],ip);
    unsigned count = regs.w[CX];

    switch(next)
    {
    case 0x26:  /* ES: */
        base[SS] = base[DS] = base[ES];
        rep(flagval);
        base[DS] = SegToMemPtr(DS);
        base[SS] = SegToMemPtr(SS);
        break;
    case 0x2e:  /* CS: */
        base[SS] = base[DS] = base[CS];
        rep(flagval);
        base[DS] = SegToMemPtr(DS);
        base[SS] = SegToMemPtr(SS);
        break;
    case 0x36:  /* SS: */
        base[DS] = base[SS];
        rep(flagval);
        base[DS] = SegToMemPtr(DS);
        break;
    case 0x3e:  /* DS: */
        base[SS] = base[DS];
        rep(flagval);
        base[SS] = SegToMemPtr(SS);
        break;
    case 0xa4:  /* REP MOVSB */
        for (; count > 0; count--)
            i_movsb();
        regs.w[CX]=count;
        break;
    case 0xa5:  /* REP MOVSW */
        for (; count > 0; count--)
            i_movsw();
        regs.w[CX]=count;
        break;
    case 0xa6:  /* REP(N)E CMPSB */
        for (ZF = flagval; (ZF == flagval) && (count > 0); count--)
            i_cmpsb();
        regs.w[CX]=count;
        break;
    case 0xa7:  /* REP(N)E CMPSW */
        for (ZF = flagval; (ZF == flagval) && (count > 0); count--)
            i_cmpsw();
        regs.w[CX]=count;
        break;
    case 0xaa:  /* REP STOSB */
        for (; count > 0; count--)
            i_stosb();
        regs.w[CX]=count;
        break;
    case 0xab:  /* REP LODSW */
        for (; count > 0; count--)
            i_stosw();
        regs.w[CX]=count;
        break;
    case 0xac:  /* REP LODSB */
        for (; count > 0; count--)
            i_lodsb();
        regs.w[CX]=count;
        break;
    case 0xad:  /* REP LODSW */
        for (; count > 0; count--)
            i_lodsw();
        regs.w[CX]=count;
        break;
    case 0xae:  /* REP(N)E SCASB */
        for (ZF = flagval; (ZF == flagval) && (count > 0); count--)
            i_scasb();
        regs.w[CX]=count;
        break;
    case 0xaf:  /* REP(N)E SCASW */
        for (ZF = flagval; (ZF == flagval) && (count > 0); count--)
            i_scasw();
        regs.w[CX]=count;
        break;
    default:
        instruction[next]();
    }
}


static INLINE2 void i_repne(void)
{
    /* Opcode 0xf2 */

    rep(0);
}


static INLINE2 void i_repe(void)
{
    /* Opcode 0xf3 */

    rep(1);
}


static INLINE2 void i_cmc(void)
{
    /* Opcode 0xf5 */

    CF = !CF;
}


static INLINE2 void i_f6pre(void)
{
	/* Opcode 0xf6 */
    unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned tmp = (unsigned)GetModRMRMB(ModRM);
    register unsigned tmp2;


    switch (ModRM & 0x38)
    {
    case 0x00:	/* TEST Eb, data8 */
    case 0x08:  /* ??? */
        tmp &= GetMemInc(base[CS],ip);

        CF = OF = AF = 0;
        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);
        break;

    case 0x10:	/* NOT Eb */
	PutbackModRMRMB(ModRM,~tmp);
        break;

    case 0x18:	/* NEG Eb */
        tmp2 = tmp;
        tmp = -tmp;

        CF = (int)tmp2 > 0;

        SetAF(tmp,0,tmp2);
        SetZFB(tmp);
        SetSFB(tmp);
        SetPF(tmp);

	PutbackModRMRMB(ModRM,tmp);
        break;
    case 0x20:	/* MUL AL, Eb */
	{
	    UINT16 result;
	    tmp2 = regs.b[AL];

	    SetSFB(tmp2);
	    SetPF(tmp2);

	    result = (UINT16)tmp2*tmp;
	    regs.w[AX]=(WORD)result;

	    SetZFW(regs.w[AX]);
	    CF = OF = (regs.b[AH] != 0);
	}
        break;
    case 0x28:	/* IMUL AL, Eb */
	{
	    INT16 result;

	    tmp2 = (unsigned)regs.b[AL];

	    SetSFB(tmp2);
	    SetPF(tmp2);

	    result = (INT16)((INT8)tmp2)*(INT16)((INT8)tmp);
	    regs.w[AX]=(WORD)result;

	    SetZFW(regs.w[AX]);

	    OF = (result >> 7 != 0) && (result >> 7 != -1);
	    CF = !(!OF);
	}
        break;
    case 0x30:	/* DIV AL, Ew */
	{
	    UINT16 result;

	    result = regs.w[AX];

	    if (tmp)
	    {
            if ((result / tmp) > 0xff)
            {
                I86_interrupt(0);
                break;
            }
            else
            {
                regs.b[AH] = result % tmp;
                regs.b[AL] = result / tmp;
            }

	    }
	    else
	    {
            I86_interrupt(0);
            break;
	    }
	}
        break;
    case 0x38:	/* IDIV AL, Ew */
	{
	    INT16 result;

	    result = regs.w[AX];

	    if (tmp)
	    {
            tmp2 = result % (INT16)((INT8)tmp);

            if ((result /= (INT16)((INT8)tmp)) > 0xff)
            {
                I86_interrupt(0);
                break;
            }
            else
            {
                regs.b[AL] = result;
                regs.b[AH] = tmp2;
            }
	    }
	    else
	    {
            I86_interrupt(0);
            break;
	    }
	}
        break;
    }
}


static INLINE2 void i_f7pre(void)
{
	/* Opcode 0xf7 */
    unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned tmp = GetModRMRMW(ModRM);
    register unsigned tmp2;


    switch (ModRM & 0x38)
    {
    case 0x00:	/* TEST Ew, data16 */
    case 0x08:  /* ??? */
        tmp2 = GetMemInc(base[CS],ip);
        tmp2 += GetMemInc(base[CS],ip) << 8;

        tmp &= tmp2;

        CF = OF = AF = 0;
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);
        break;

    case 0x10:	/* NOT Ew */
        tmp = ~tmp;
	PutbackModRMRMW(ModRM,tmp);
        break;

    case 0x18:	/* NEG Ew */
        tmp2 = tmp;
        tmp = -tmp;

        CF = (int)tmp2 > 0;

        SetAF(tmp,0,tmp2);
        SetZFW(tmp);
        SetSFW(tmp);
        SetPF(tmp);

	PutbackModRMRMW(ModRM,tmp);
        break;
    case 0x20:	/* MUL AX, Ew */
	{
	    UINT32 result;
	    tmp2 = regs.w[AX];

	    SetSFW(tmp2);
	    SetPF(tmp2);

	    result = (UINT32)tmp2*tmp;
	    regs.w[AX]=(WORD)result;
        result >>= 16;
	    regs.w[DX]=result;

	    SetZFW(regs.w[AX] | regs.w[DX]);
	    CF = OF = (regs.w[DX] != ChangeE(0));
	}
        break;

    case 0x28:	/* IMUL AX, Ew */
	{
	    INT32 result;

	    tmp2 = regs.w[AX];

	    SetSFW(tmp2);
	    SetPF(tmp2);

	    result = (INT32)((INT16)tmp2)*(INT32)((INT16)tmp);
        OF = (result >> 15 != 0) && (result >> 15 != -1);

	    regs.w[AX]=(WORD)result;
        result = (WORD)(result >> 16);
	    regs.w[DX]=result;

	    SetZFW(regs.w[AX] | regs.w[DX]);

        CF = !(!OF);
	}
        break;
    case 0x30:	/* DIV AX, Ew */
	{
	    UINT32 result;

	    result = (regs.w[DX] << 16) + regs.w[AX];

	    if (tmp)
	    {
            tmp2 = result % tmp;
            if ((result / tmp) > 0xffff)
            {
                I86_interrupt(0);
                break;
            }
            else
            {
                regs.w[DX]=tmp2;
                result /= tmp;
                regs.w[AX]=result;
            }
	    }
	    else
	    {
            I86_interrupt(0);
            break;
	    }
	}
        break;
    case 0x38:	/* IDIV AX, Ew */
	{
	    INT32 result;

	    result = (regs.w[DX] << 16) + regs.w[AX];

	    if (tmp)
	    {
            tmp2 = result % (INT32)((INT16)tmp);

            if ((result /= (INT32)((INT16)tmp)) > 0xffff)
            {
                I86_interrupt(0);
                break;
            }
            else
            {
                regs.w[AX]=result;
                regs.w[DX]=tmp2;
            }
	    }
	    else
	    {
            I86_interrupt(0);
            break;
	    }
	}
        break;
    }
}


static INLINE2 void i_clc(void)
{
    /* Opcode 0xf8 */

    CF = 0;
}


static INLINE2 void i_stc(void)
{
	/* Opcode 0xf9 */

	CF = 1;
}


static INLINE2 void i_cli(void)
{
    /* Opcode 0xfa */

    IF = 0;
}


static INLINE2 void i_sti(void)
{
    /* Opcode 0xfb */

    IF = 1;

    if (int_blocked)
    {
        int_pending = int_blocked;
        int_blocked = 0;
        D2(printf("Unblocking interrupt\n"););
    }
}


static INLINE2 void i_cld(void)
{
    /* Opcode 0xfc */

    DF = 0;
}


static INLINE2 void i_std(void)
{
    /* Opcode 0xfd */

    DF = 1;
}


static INLINE2 void i_fepre(void)
{
    /* Opcode 0xfe */

    unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned tmp = GetModRMRMB(ModRM);
    register unsigned tmp1;

    if ((ModRM & 0x38) == 0)
    {
        tmp1 = tmp+1;
        SetOFB_Add(tmp1,tmp,1);
    }
    else
    {
        tmp1 = tmp-1;
        SetOFB_Sub(tmp1,1,tmp);
    }

    SetAF(tmp1,tmp,1);
    SetZFB(tmp1);
    SetSFB(tmp1);
    SetPF(tmp1);

    PutbackModRMRMB(ModRM,(BYTE)tmp1);
}


static INLINE2 void i_ffpre(void)
{
    /* Opcode 0xff */

    unsigned ModRM = GetMemInc(base[CS],ip);
    register unsigned tmp;
    register unsigned tmp1;

    switch(ModRM & 0x38)
    {
    case 0x00:  /* INC ew */
        tmp = GetModRMRMW(ModRM);
        tmp1 = tmp+1;

        SetOFW_Add(tmp1,tmp,1);
        SetAF(tmp1,tmp,1);
        SetZFW(tmp1);
        SetSFW(tmp1);
        SetPF(tmp1);

        PutbackModRMRMW(ModRM,(WORD)tmp1);
        break;

    case 0x08:  /* DEC ew */
        tmp = GetModRMRMW(ModRM);
        tmp1 = tmp-1;

        SetOFW_Sub(tmp1,1,tmp);
        SetAF(tmp1,tmp,1);
        SetZFW(tmp1);
        SetSFW(tmp1);
        SetPF(tmp1);

        PutbackModRMRMW(ModRM,(WORD)tmp1);
        break;

    case 0x10:  /* CALL ew */
        tmp = GetModRMRMW(ModRM);
        tmp1 = (WORD)(regs.w[SP]-2);

        PutMemW(base[SS],tmp1,ip);
        regs.w[SP]=tmp1;

        ip = (WORD)tmp;
        break;

    case 0x18:  /* CALL FAR ea */
        tmp1 = (WORD)(regs.w[SP]-2);

        PutMemW(base[SS],tmp1,sregs[CS]);
        tmp1 = (WORD)(tmp1-2);
        PutMemW(base[SS],tmp1,ip);
        regs.w[SP]=tmp1;

        ip = GetModRMRMW(ModRM);
        sregs[CS] = GetnextModRMRMW(ModRM);
        base[CS] = SegToMemPtr(CS);
        break;

    case 0x20:  /* JMP ea */
        ip = GetModRMRMW(ModRM);
        break;

    case 0x28:  /* JMP FAR ea */
        ip = GetModRMRMW(ModRM);
        sregs[CS] = GetnextModRMRMW(ModRM);
        base[CS] = SegToMemPtr(CS);
        break;

    case 0x30:  /* PUSH ea */
        tmp1 = (WORD)(regs.w[SP]-2);
        tmp = GetModRMRMW(ModRM);

        PutMemW(base[SS],tmp1,tmp);
        regs.w[SP]=tmp1;
        break;
    }
}


static INLINE2 void i_notdone(void)
{
    fprintf(stderr,"Error: Unimplemented opcode cs:ip = %04X:%04X\n",
		    sregs[CS],ip-1);
    exit(1);
}


void trap(void)
{
    instruction[GetMemInc(base[CS],ip)]();
    I86_interrupt(1);
}


void I86_Execute(void)
{
    int instr_count=5000;	/* I will soon change this and count cycles instead */

    base[CS] = SegToMemPtr(CS);
    base[DS] = SegToMemPtr(DS);
    base[ES] = SegToMemPtr(ES);
    base[SS] = SegToMemPtr(SS);

    if (init) I86_interrupt(2); /* NMI interrupt, but let the cpu start first */
    else init=1;

    while(instr_count--)
    {
        if (int_pending)
            external_int();

#ifdef DEBUGGER
        call_debugger(D_TRACE);
#endif
/*
printf("[%04x]=%02x\tAX=%04x\tBX=%04x\tCX=%04x\tDX=%04x\n",ip,GetMemB(base[CS],ip),regs.w[AX],regs.w[BX],regs.w[CX],regs.w[DX]);
*/

#if defined(BIGCASE) && !defined(RS6000)
  /* Some compilers cannot handle large case statements */
        switch(GetMemInc(base[CS],ip))
        {
        case 0x00:    i_add_br8(); break;
        case 0x01:    i_add_wr16(); break;
        case 0x02:    i_add_r8b(); break;
        case 0x03:    i_add_r16w(); break;
        case 0x04:    i_add_ald8(); break;
        case 0x05:    i_add_axd16(); break;
        case 0x06:    i_push_es(); break;
        case 0x07:    i_pop_es(); break;
        case 0x08:    i_or_br8(); break;
        case 0x09:    i_or_wr16(); break;
        case 0x0a:    i_or_r8b(); break;
        case 0x0b:    i_or_r16w(); break;
        case 0x0c:    i_or_ald8(); break;
        case 0x0d:    i_or_axd16(); break;
        case 0x0e:    i_push_cs(); break;
        case 0x0f:    i_notdone(); break;
        case 0x10:    i_adc_br8(); break;
        case 0x11:    i_adc_wr16(); break;
        case 0x12:    i_adc_r8b(); break;
        case 0x13:    i_adc_r16w(); break;
        case 0x14:    i_adc_ald8(); break;
        case 0x15:    i_adc_axd16(); break;
        case 0x16:    i_push_ss(); break;
        case 0x17:    i_pop_ss(); break;
        case 0x18:    i_sbb_br8(); break;
        case 0x19:    i_sbb_wr16(); break;
        case 0x1a:    i_sbb_r8b(); break;
        case 0x1b:    i_sbb_r16w(); break;
        case 0x1c:    i_sbb_ald8(); break;
        case 0x1d:    i_sbb_axd16(); break;
        case 0x1e:    i_push_ds(); break;
        case 0x1f:    i_pop_ds(); break;
        case 0x20:    i_and_br8(); break;
        case 0x21:    i_and_wr16(); break;
        case 0x22:    i_and_r8b(); break;
        case 0x23:    i_and_r16w(); break;
        case 0x24:    i_and_ald8(); break;
        case 0x25:    i_and_axd16(); break;
        case 0x26:    i_es(); break;
        case 0x27:    i_daa(); break;
        case 0x28:    i_sub_br8(); break;
        case 0x29:    i_sub_wr16(); break;
        case 0x2a:    i_sub_r8b(); break;
        case 0x2b:    i_sub_r16w(); break;
        case 0x2c:    i_sub_ald8(); break;
        case 0x2d:    i_sub_axd16(); break;
        case 0x2e:    i_cs(); break;
        case 0x2f:    i_das(); break;
        case 0x30:    i_xor_br8(); break;
        case 0x31:    i_xor_wr16(); break;
        case 0x32:    i_xor_r8b(); break;
        case 0x33:    i_xor_r16w(); break;
        case 0x34:    i_xor_ald8(); break;
        case 0x35:    i_xor_axd16(); break;
        case 0x36:    i_ss(); break;
        case 0x37:    i_aaa(); break;
        case 0x38:    i_cmp_br8(); break;
        case 0x39:    i_cmp_wr16(); break;
        case 0x3a:    i_cmp_r8b(); break;
        case 0x3b:    i_cmp_r16w(); break;
        case 0x3c:    i_cmp_ald8(); break;
        case 0x3d:    i_cmp_axd16(); break;
        case 0x3e:    i_ds(); break;
        case 0x3f:    i_aas(); break;
        case 0x40:    i_inc_ax(); break;
        case 0x41:    i_inc_cx(); break;
        case 0x42:    i_inc_dx(); break;
        case 0x43:    i_inc_bx(); break;
        case 0x44:    i_inc_sp(); break;
        case 0x45:    i_inc_bp(); break;
        case 0x46:    i_inc_si(); break;
        case 0x47:    i_inc_di(); break;
        case 0x48:    i_dec_ax(); break;
        case 0x49:    i_dec_cx(); break;
        case 0x4a:    i_dec_dx(); break;
        case 0x4b:    i_dec_bx(); break;
        case 0x4c:    i_dec_sp(); break;
        case 0x4d:    i_dec_bp(); break;
        case 0x4e:    i_dec_si(); break;
        case 0x4f:    i_dec_di(); break;
        case 0x50:    i_push_ax(); break;
        case 0x51:    i_push_cx(); break;
        case 0x52:    i_push_dx(); break;
        case 0x53:    i_push_bx(); break;
        case 0x54:    i_push_sp(); break;
        case 0x55:    i_push_bp(); break;
        case 0x56:    i_push_si(); break;
        case 0x57:    i_push_di(); break;
        case 0x58:    i_pop_ax(); break;
        case 0x59:    i_pop_cx(); break;
        case 0x5a:    i_pop_dx(); break;
        case 0x5b:    i_pop_bx(); break;
        case 0x5c:    i_pop_sp(); break;
        case 0x5d:    i_pop_bp(); break;
        case 0x5e:    i_pop_si(); break;
        case 0x5f:    i_pop_di(); break;
        case 0x60:    i_notdone(); break;
        case 0x61:    i_notdone(); break;
        case 0x62:    i_notdone(); break;
        case 0x63:    i_notdone(); break;
        case 0x64:    i_notdone(); break;
        case 0x65:    i_notdone(); break;
        case 0x66:    i_notdone(); break;
        case 0x67:    i_notdone(); break;
        case 0x68:    i_notdone(); break;
        case 0x69:    i_notdone(); break;
        case 0x6a:    i_notdone(); break;
        case 0x6b:    i_notdone(); break;
        case 0x6c:    i_notdone(); break;
        case 0x6d:    i_notdone(); break;
        case 0x6e:    i_notdone(); break;
        case 0x6f:    i_notdone(); break;
        case 0x70:    i_jo(); break;
        case 0x71:    i_jno(); break;
        case 0x72:    i_jb(); break;
        case 0x73:    i_jnb(); break;
        case 0x74:    i_jz(); break;
        case 0x75:    i_jnz(); break;
        case 0x76:    i_jbe(); break;
        case 0x77:    i_jnbe(); break;
        case 0x78:    i_js(); break;
        case 0x79:    i_jns(); break;
        case 0x7a:    i_jp(); break;
        case 0x7b:    i_jnp(); break;
        case 0x7c:    i_jl(); break;
        case 0x7d:    i_jnl(); break;
        case 0x7e:    i_jle(); break;
        case 0x7f:    i_jnle(); break;
        case 0x80:    i_80pre(); break;
        case 0x81:    i_81pre(); break;
        case 0x82:    i_notdone(); break;
        case 0x83:    i_83pre(); break;
        case 0x84:    i_test_br8(); break;
        case 0x85:    i_test_wr16(); break;
        case 0x86:    i_xchg_br8(); break;
        case 0x87:    i_xchg_wr16(); break;
        case 0x88:    i_mov_br8(); break;
        case 0x89:    i_mov_wr16(); break;
        case 0x8a:    i_mov_r8b(); break;
        case 0x8b:    i_mov_r16w(); break;
        case 0x8c:    i_mov_wsreg(); break;
        case 0x8d:    i_lea(); break;
        case 0x8e:    i_mov_sregw(); break;
        case 0x8f:    i_popw(); break;
        case 0x90:    i_nop(); break;
        case 0x91:    i_xchg_axcx(); break;
        case 0x92:    i_xchg_axdx(); break;
        case 0x93:    i_xchg_axbx(); break;
        case 0x94:    i_xchg_axsp(); break;
        case 0x95:    i_xchg_axbp(); break;
        case 0x96:    i_xchg_axsi(); break;
        case 0x97:    i_xchg_axdi(); break;
        case 0x98:    i_cbw(); break;
        case 0x99:    i_cwd(); break;
        case 0x9a:    i_call_far(); break;
        case 0x9b:    i_wait(); break;
        case 0x9c:    i_pushf(); break;
        case 0x9d:    i_popf(); break;
        case 0x9e:    i_sahf(); break;
        case 0x9f:    i_lahf(); break;
        case 0xa0:    i_mov_aldisp(); break;
        case 0xa1:    i_mov_axdisp(); break;
        case 0xa2:    i_mov_dispal(); break;
        case 0xa3:    i_mov_dispax(); break;
        case 0xa4:    i_movsb(); break;
        case 0xa5:    i_movsw(); break;
        case 0xa6:    i_cmpsb(); break;
        case 0xa7:    i_cmpsw(); break;
        case 0xa8:    i_test_ald8(); break;
        case 0xa9:    i_test_axd16(); break;
        case 0xaa:    i_stosb(); break;
        case 0xab:    i_stosw(); break;
        case 0xac:    i_lodsb(); break;
        case 0xad:    i_lodsw(); break;
        case 0xae:    i_scasb(); break;
        case 0xaf:    i_scasw(); break;
        case 0xb0:    i_mov_ald8(); break;
        case 0xb1:    i_mov_cld8(); break;
        case 0xb2:    i_mov_dld8(); break;
        case 0xb3:    i_mov_bld8(); break;
        case 0xb4:    i_mov_ahd8(); break;
        case 0xb5:    i_mov_chd8(); break;
        case 0xb6:    i_mov_dhd8(); break;
        case 0xb7:    i_mov_bhd8(); break;
        case 0xb8:    i_mov_axd16(); break;
        case 0xb9:    i_mov_cxd16(); break;
        case 0xba:    i_mov_dxd16(); break;
        case 0xbb:    i_mov_bxd16(); break;
        case 0xbc:    i_mov_spd16(); break;
        case 0xbd:    i_mov_bpd16(); break;
        case 0xbe:    i_mov_sid16(); break;
        case 0xbf:    i_mov_did16(); break;
        case 0xc0:    i_notdone(); break;
        case 0xc1:    i_notdone(); break;
        case 0xc2:    i_ret_d16(); break;
        case 0xc3:    i_ret(); break;
        case 0xc4:    i_les_dw(); break;
        case 0xc5:    i_lds_dw(); break;
        case 0xc6:    i_mov_bd8(); break;
        case 0xc7:    i_mov_wd16(); break;
        case 0xc8:    i_notdone(); break;
        case 0xc9:    i_notdone(); break;
        case 0xca:    i_retf_d16(); break;
        case 0xcb:    i_retf(); break;
        case 0xcc:    i_int3(); break;
        case 0xcd:    i_int(); break;
        case 0xce:    i_into(); break;
        case 0xcf:    i_iret(); break;
        case 0xd0:    i_d0pre(); break;
        case 0xd1:    i_d1pre(); break;
        case 0xd2:    i_d2pre(); break;
        case 0xd3:    i_d3pre(); break;
        case 0xd4:    i_aam(); break;
        case 0xd5:    i_aad(); break;
        case 0xd6:    i_notdone(); break;
        case 0xd7:    i_xlat(); break;
        case 0xd8:    i_escape(); break;
        case 0xd9:    i_escape(); break;
        case 0xda:    i_escape(); break;
        case 0xdb:    i_escape(); break;
        case 0xdc:    i_escape(); break;
        case 0xdd:    i_escape(); break;
        case 0xde:    i_escape(); break;
        case 0xdf:    i_escape(); break;
        case 0xe0:    i_loopne(); break;
        case 0xe1:    i_loope(); break;
        case 0xe2:    i_loop(); break;
        case 0xe3:    i_jcxz(); break;
        case 0xe4:    i_inal(); break;
        case 0xe5:    i_inax(); break;
        case 0xe6:    i_outal(); break;
        case 0xe7:    i_outax(); break;
        case 0xe8:    i_call_d16(); break;
        case 0xe9:    i_jmp_d16(); break;
        case 0xea:    i_jmp_far(); break;
        case 0xeb:    i_jmp_d8(); break;
        case 0xec:    i_inaldx(); break;
        case 0xed:    i_inaxdx(); break;
        case 0xee:    i_outdxal(); break;
        case 0xef:    i_outdxax(); break;
        case 0xf0:    i_lock(); break;
        case 0xf1:    i_gobios(); break;
        case 0xf2:    i_repne(); break;
        case 0xf3:    i_repe(); break;
        case 0xf4:    i_notdone(); break;
        case 0xf5:    i_cmc(); break;
        case 0xf6:    i_f6pre(); break;
        case 0xf7:    i_f7pre(); break;
        case 0xf8:    i_clc(); break;
        case 0xf9:    i_stc(); break;
        case 0xfa:    i_cli(); break;
        case 0xfb:    i_sti(); break;
        case 0xfc:    i_cld(); break;
        case 0xfd:    i_std(); break;
        case 0xfe:    i_fepre(); break;
        case 0xff:    i_ffpre(); break;
        };
#else
        instruction[GetMemInc(base[CS],ip)]();
#endif
    }
}

