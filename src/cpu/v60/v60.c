// V60.C
// Undiscover the beast!
// Main hacking and coding by Farfetch'd
// Portability fixes by Richter Belmont

#include "cpuintrf.h"
#include "osd_cpu.h"
#include "mamedbg.h"

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include "v60.h"

// macros stolen from MAME for flags calc
// note that these types are in x86 naming:
// byte = 8 bit, word = 16 bit, long = 32 bit

// parameter x = result, y = source 1, z = source 2

#define SetOFL_Add(x,y,z)	(_OV = (((x) ^ (y)) & ((x) ^ (z)) & 0x80000000) ? 1: 0)
#define SetOFW_Add(x,y,z)	(_OV = (((x) ^ (y)) & ((x) ^ (z)) & 0x8000) ? 1 : 0)
#define SetOFB_Add(x,y,z)	(_OV = (((x) ^ (y)) & ((x) ^ (z)) & 0x80) ? 1 : 0)

#define SetOFL_Sub(x,y,z)	(_OV = (((z) ^ (y)) & ((z) ^ (x)) & 0x80000000) ? 1 : 0)
#define SetOFW_Sub(x,y,z)	(_OV = (((z) ^ (y)) & ((z) ^ (x)) & 0x8000) ? 1 : 0)
#define SetOFB_Sub(x,y,z)	(_OV = (((z) ^ (y)) & ((z) ^ (x)) & 0x80) ? 1 : 0)

#define SetCFB(x)		{_CY = ((x) & 0x100) ? 1 : 0; }
#define SetCFW(x)		{_CY = ((x) & 0x10000) ? 1 : 0; }
#define SetCFL(x)		{_CY = ((x) & (((UINT64)1) << 32)) ? 1 : 0; }

#define SetSF(x)		(_S = (x))
#define SetZF(x)		(_Z = (x))

#define SetSZPF_Byte(x) 	{_Z = ((UINT8)(x)==0);  _S = ((x)&0x80) ? 1 : 0; }
#define SetSZPF_Word(x) 	{_Z = ((UINT16)(x)==0);  _S = ((x)&0x8000) ? 1 : 0; }
#define SetSZPF_Long(x) 	{_Z = ((UINT32)(x)==0);  _S = ((x)&0x80000000) ? 1 : 0; }

#define ORB(dst,src)		{ (dst) |= (src); _CY = _OV = 0; SetSZPF_Byte(dst); }
#define ORW(dst,src)		{ (dst) |= (src); _CY = _OV = 0; SetSZPF_Word(dst); }
#define ORL(dst,src)		{ (dst) |= (src); _CY = _OV = 0; SetSZPF_Long(dst); }

#define ANDB(dst,src)		{ (dst) &= (src); _CY = _OV = 0; SetSZPF_Byte(dst); }
#define ANDW(dst,src)		{ (dst) &= (src); _CY = _OV = 0; SetSZPF_Word(dst); }
#define ANDL(dst,src)		{ (dst) &= (src); _CY = _OV = 0; SetSZPF_Long(dst); }

#define XORB(dst,src)		{ (dst) ^= (src); _CY = _OV = 0; SetSZPF_Byte(dst); }
#define XORW(dst,src)		{ (dst) ^= (src); _CY = _OV = 0; SetSZPF_Word(dst); }
#define XORL(dst,src)		{ (dst) ^= (src); _CY = _OV = 0; SetSZPF_Long(dst); }

#define SUBB(dst, src)		{ unsigned res=(dst)-(src); SetCFB(res); SetOFB_Sub(res,src,dst); SetSZPF_Byte(res); dst=(UINT8)res; }
#define SUBW(dst, src)		{ unsigned res=(dst)-(src); SetCFW(res); SetOFW_Sub(res,src,dst); SetSZPF_Word(res); dst=(UINT16)res; }
#define SUBL(dst, src)		{ UINT64 res=(UINT64)(dst)-(INT64)(src); SetCFL(res); SetOFL_Sub(res,src,dst); SetSZPF_Long(res); dst=(UINT32)res; }

#define ADDB(dst, src)		{ unsigned res=(dst)+(src); SetCFB(res); SetOFB_Add(res,src,dst); SetSZPF_Byte(res); dst=(UINT8)res; }
#define ADDW(dst, src)		{ unsigned res=(dst)+(src); SetCFW(res); SetOFW_Add(res,src,dst); SetSZPF_Word(res); dst=(UINT16)res; }
#define ADDL(dst, src)		{ UINT64 res=(UINT64)(dst)+(UINT64)(src); SetCFL(res); SetOFL_Add(res,src,dst); SetSZPF_Long(res); dst=(UINT32)res; }

#if 1
// This version may have issues with strict aliasing
#ifdef LSB_FIRST
#define SETREG8(a, b)  *(UINT8 *)&(a) = (b)
#define SETREG16(a, b) *(UINT16 *)&(a) = (b)
#else
#define SETREG8(a, b)  ((UINT8 *)&(a))[3] = (b)
#define SETREG16(a, b) ((UINT16 *)&(a))[1] = (b)
#endif
#else
// This is portable, but beware the double evaluation of a
#define SETREG8(a, b)  (a) = ((a) & ~0xff) | ((b) & 0xff)
#define SETREG16(a, b) (a) = ((a) & ~0xffff) | ((b) & 0xffff)
#endif

// Ultra Function Tables
static UINT32 (*OpCodeTable[256])(void);

typedef struct
{
	UINT8 CY;
	UINT8 OV;
	UINT8 S;
	UINT8 Z;
} Flags;

/* Workaround for LinuxPPC. */
#ifdef PPC
#undef PPC
#endif

struct cpu_info {
	UINT8  (*mr8) (offs_t address);
	void   (*mw8) (offs_t address, UINT8  data);
	UINT16 (*mr16)(offs_t address);
	void   (*mw16)(offs_t address, UINT16 data);
	UINT32 (*mr32)(offs_t address);
	void   (*mw32)(offs_t address, UINT32 data);
	UINT8  (*pr8) (offs_t address);
	void   (*pw8) (offs_t address, UINT8  data);
	UINT16 (*pr16)(offs_t address);
	void   (*pw16)(offs_t address, UINT16 data);
	UINT32 (*pr32)(offs_t address);
	void   (*pw32)(offs_t address, UINT32 data);
	UINT32 start_pc;
};

// v60 Register Inside (Hm... It's not a pentium inside :-))) )
struct v60info {
	const struct cpu_info *info;
	UINT32 reg[58];
	Flags flags;
	int irq_line;
	int nmi_line;
	int (*irq_cb)(int irqline);
	UINT32 PPC;
} v60;

int v60_ICount;

#define _CY v60.flags.CY
#define _OV v60.flags.OV
#define _S v60.flags.S
#define _Z v60.flags.Z


// Defines of all v60 register...
#define R0 v60.reg[0]
#define R1 v60.reg[1]
#define R2 v60.reg[2]
#define R3 v60.reg[3]
#define R4 v60.reg[4]
#define R5 v60.reg[5]
#define R6 v60.reg[6]
#define R7 v60.reg[7]
#define R8 v60.reg[8]
#define R9 v60.reg[9]
#define R10 v60.reg[10]
#define R11 v60.reg[11]
#define R12 v60.reg[12]
#define R13 v60.reg[13]
#define R14 v60.reg[14]
#define R15 v60.reg[15]
#define R16 v60.reg[16]
#define R17 v60.reg[17]
#define R18 v60.reg[18]
#define R19 v60.reg[19]
#define R20 v60.reg[20]
#define R21 v60.reg[21]
#define R22 v60.reg[22]
#define R23 v60.reg[23]
#define R24 v60.reg[24]
#define R25 v60.reg[25]
#define R26 v60.reg[26]
#define R27 v60.reg[27]
#define R28 v60.reg[28]
#define AP v60.reg[29]
#define FP v60.reg[30]
#define SP v60.reg[31]

#define PC		v60.reg[32]
#define PSW		v60.reg[33]
#define TR		v60.reg[34]
#define PIR		v60.reg[35]
#define PPC		v60.PPC

// Privileged registers
#define ISP		v60.reg[36]
#define L0SP	v60.reg[37]
#define L1SP	v60.reg[38]
#define L2SP	v60.reg[39]
#define L3SP	v60.reg[40]
#define SBR		v60.reg[41]
#define SYCW	v60.reg[42]
#define TKCW	v60.reg[43]
#define PSW2	v60.reg[44]
#define ATBR0	v60.reg[45]
#define ATLR0	v60.reg[46]
#define ATBR1	v60.reg[47]
#define ATLR1	v60.reg[48]
#define ATBR2	v60.reg[49]
#define ATLR2	v60.reg[50]
#define ATBR3	v60.reg[51]
#define ATLR3	v60.reg[52]
#define TRMODE	v60.reg[53]
#define ADTR0	v60.reg[54]
#define ADTR1	v60.reg[55]
#define ADTMR0	v60.reg[56]
#define ADTMR1	v60.reg[57]

// Register names
const char *v60_reg_names[58] = {
	"R0", "R1", "R2", "R3",
	"R4", "R5", "R6", "R7",
	"R8", "R9", "R10", "R11",
	"R12", "R13", "R14", "R15",
	"R16", "R17", "R18", "R19",
	"R20", "R21", "R22", "R23",
	"R24", "R25", "R26", "R27",
	"R28", "AP", "FP", "SP",
	"PC", "PSW", "TR", "PIR",
	"ISP", "L0SP", "L1SP", "L2SP",
	"L3SP", "SBR", "SYCW", "TKCW",
	"PSW2", "ATBR0", "ATLR0", "ATBR1",
	"ATLR1", "ATBR2", "ATLR2", "ATBR3",
	"ATLR3", "TRMODE", "ADTR0", "ADTR1",
	"ADTMR0", "ADTMR1"
};

// Defines...
#define UPDATEPSW()	\
{ \
  PSW &= 0xfffffff0; \
  PSW |= (_Z?1:0) | (_S?2:0) | (_OV?4:0) | (_CY?8:0); \
}

#define UPDATECPUFLAGS() \
{ \
	_Z =  (UINT8)(PSW & 1); \
	_S =  (UINT8)(PSW & 2); \
	_OV = (UINT8)(PSW & 4); \
	_CY = (UINT8)(PSW & 8); \
}

#define UPDATEFPUFLAGS()	{ }

#define NORMALIZEFLAGS() \
{ \
	_S	= _S  ? 1 : 0; \
	_OV	= _OV ? 1 : 0; \
	_Z	= _Z  ? 1 : 0; \
	_CY	= _CY ? 1 : 0; \
}

#define MemRead8_16		cpu_readmem24lew
#define MemWrite8_16	cpu_writemem24lew

static UINT16 MemRead16_16(offs_t address)
{
	if (!(address & 1))
	{
		return cpu_readmem24lew_word(address);
	}
	else
	{
		UINT16 result = cpu_readmem24lew(address);
		return result | cpu_readmem24lew(address + 1) << 8;
	}
}

static void MemWrite16_16(offs_t address, UINT16 data)
{
	if (!(address & 1))
	{
		cpu_writemem24lew_word(address, data);
	}
	else
	{
		cpu_writemem24lew(address, data);
		cpu_writemem24lew(address + 1, data >> 8);
	}
}

static UINT32 MemRead32_16(offs_t address)
{
	if (!(address & 1))
	{
		UINT32 result = cpu_readmem24lew_word(address);
		return result | (cpu_readmem24lew_word(address + 2) << 16);
	}
	else
	{
		UINT32 result = cpu_readmem24lew(address);
		result |= cpu_readmem24lew_word(address + 1) << 8;
		return result | cpu_readmem24lew(address + 3) << 24;
	}
}

static void MemWrite32_16(offs_t address, UINT32 data)
{
	if (!(address & 1))
	{
		cpu_writemem24lew_word(address, data);
		cpu_writemem24lew_word(address + 2, data >> 16);
	}
	else
	{
		cpu_writemem24lew(address, data);
		cpu_writemem24lew_word(address + 1, data >> 8);
		cpu_writemem24lew(address + 3, data >> 24);
	}
}

#define PortRead8_16		cpu_readport24lew
#define PortWrite8_16		cpu_writeport24lew

static UINT16 PortRead16_16(offs_t address)
{
	if (!(address & 1))
	{
		return cpu_readport24lew_word(address);
	}
	else
	{
		UINT16 result = cpu_readport24lew(address);
		return result | cpu_readport24lew(address + 1) << 8;
	}
}

static void PortWrite16_16(offs_t address, UINT16 data)
{
	if (!(address & 1))
	{
		cpu_writeport24lew_word(address, data);
	}
	else
	{
		cpu_writeport24lew(address, data);
		cpu_writeport24lew(address + 1, data >> 8);
	}
}

static UINT32 PortRead32_16(offs_t address)
{
	if (!(address & 1))
	{
		UINT32 result = cpu_readport24lew_word(address);
		return result | (cpu_readport24lew_word(address + 2) << 16);
	}
	else
	{
		UINT32 result = cpu_readport24lew(address);
		result |= cpu_readport24lew_word(address + 1) << 8;
		return result | cpu_readport24lew(address + 3) << 24;
	}
}

static void PortWrite32_16(offs_t address, UINT32 data)
{
	if (!(address & 1))
	{
		cpu_writeport24lew_word(address, data);
		cpu_writeport24lew_word(address + 2, data >> 16);
	}
	else
	{
		cpu_writeport24lew(address, data);
		cpu_writeport24lew_word(address + 1, data >> 8);
		cpu_writeport24lew(address + 3, data >> 24);
	}
}
#define MemRead8_32		cpu_readmem32ledw
#define MemWrite8_32	cpu_writemem32ledw

static UINT16 MemRead16_32(offs_t address)
{
	if (!(address & 1))
	{
		return cpu_readmem32ledw_word(address);
	}
	else
	{
		UINT16 result = cpu_readmem32ledw(address);
		return result | cpu_readmem32ledw(address + 1) << 8;
	}
}

static void MemWrite16_32(offs_t address, UINT16 data)
{
	if (!(address & 1))
	{
		cpu_writemem32ledw_word(address, data);
	}
	else
	{
		cpu_writemem32ledw(address, data);
		cpu_writemem32ledw(address + 1, data >> 8);
	}
}

static UINT32 MemRead32_32(offs_t address)
{
	if (!(address & 3))
		return cpu_readmem32ledw_dword(address);
	else if (!(address & 1))
	{
		UINT32 result = cpu_readmem32ledw_word(address);
		return result | (cpu_readmem32ledw_word(address + 2) << 16);
	}
	else
	{
		UINT32 result = cpu_readmem32ledw(address);
		result |= cpu_readmem32ledw_word(address + 1) << 8;
		return result | cpu_readmem32ledw(address + 3) << 24;
	}
}

static void MemWrite32_32(offs_t address, UINT32 data)
{
	if (!(address & 3))
		cpu_writemem32ledw_dword(address, data);
	else if (!(address & 1))
	{
		cpu_writemem32ledw_word(address, data);
		cpu_writemem32ledw_word(address + 2, data >> 16);
	}
	else
	{
		cpu_writemem32ledw(address, data);
		cpu_writemem32ledw_word(address + 1, data >> 8);
		cpu_writemem32ledw(address + 3, data >> 24);
	}
}

#define PortRead8_32		cpu_readport32ledw
#define PortWrite8_32		cpu_writeport32ledw

static UINT16 PortRead16_32(offs_t address)
{
	if (!(address & 1))
	{
		return cpu_readport32ledw_word(address);
	}
	else
	{
		UINT16 result = cpu_readport32ledw(address);
		return result | cpu_readport32ledw(address + 1) << 8;
	}
}

static void PortWrite16_32(offs_t address, UINT16 data)
{
	if (!(address & 1))
	{
		cpu_writeport32ledw_word(address, data);
	}
	else
	{
		cpu_writeport32ledw(address, data);
		cpu_writeport32ledw(address + 1, data >> 8);
	}
}

static UINT32 PortRead32_32(offs_t address)
{
	if (!(address & 3))
		return cpu_readport32ledw_dword(address);
	else if (!(address & 1))
	{
		UINT32 result = cpu_readport32ledw_word(address);
		return result | (cpu_readport32ledw_word(address + 2) << 16);
	}
	else
	{
		UINT32 result = cpu_readport32ledw(address);
		result |= cpu_readport32ledw_word(address + 1) << 8;
		return result | cpu_readport32ledw(address + 3) << 24;
	}
}

static void PortWrite32_32(offs_t address, UINT32 data)
{
	if (!(address & 3))
		cpu_writeport32ledw_dword(address, data);
	else if (!(address & 1))
	{
		cpu_writeport32ledw_word(address, data);
		cpu_writeport32ledw_word(address + 2, data >> 16);
	}
	else
	{
		cpu_writeport32ledw(address, data);
		cpu_writeport32ledw_word(address + 1, data >> 8);
		cpu_writeport32ledw(address + 3, data >> 24);
	}
}

static struct cpu_info v60_i = {
	MemRead8_16,  MemWrite8_16,  MemRead16_16,  MemWrite16_16,  MemRead32_16,  MemWrite32_16,
	PortRead8_16, PortWrite8_16, PortRead16_16, PortWrite16_16, PortRead32_16, PortWrite32_16,
	0xfffff0
};

static struct cpu_info v70_i = {
	MemRead8_32,  MemWrite8_32,  MemRead16_32,  MemWrite16_32,  MemRead32_32,  MemWrite32_32,
	PortRead8_32, PortWrite8_32, PortRead16_32, PortWrite16_32, PortRead32_32, PortWrite32_32,
	0xfffffff0
};

#define MemRead8    v60.info->mr8
#define MemWrite8   v60.info->mw8
#define MemRead16   v60.info->mr16
#define MemWrite16  v60.info->mw16
#define MemRead32   v60.info->mr32
#define MemWrite32  v60.info->mw32
#define PortRead8   v60.info->pr8
#define PortWrite8  v60.info->pw8
#define PortRead16  v60.info->pr16
#define PortWrite16 v60.info->pw16
#define PortRead32  v60.info->pr32
#define PortWrite32 v60.info->pw32

static void messagebox(char *msg)
{
	logerror("%s", msg);
}

static void logWrite(int channel, char *format, ...)
{
	char msg[1024];
	va_list arg;
	va_start(arg, format);
	vsprintf(msg, format, arg);
	va_end(arg);
	logerror("%s", msg);
}

static void v60_try_irq(void);

#define STACK_REG(IS)	((IS)==0?37:36)

static UINT32 v60ReadPSW(void)
{
	v60.reg[STACK_REG((v60.reg[33]>>28)&1)] = SP;
	UPDATEPSW();
	return PSW;
}


static void v60WritePSW(UINT32 newval)
{
	UINT32 oldval = v60ReadPSW();
	int oldIS, newIS, oldEL, newEL;

	PSW = newval;
	UPDATECPUFLAGS();

	// Now check if we must swap SP
	oldIS = (oldval >> 28) & 1;
	newIS = (newval >> 28) & 1;

	oldEL = (oldval >> 24) & 3;
	newEL = (newval >> 24) & 3;

	if (oldIS != newIS)
	{
		v60.reg[STACK_REG(oldIS)] = SP;
		SP = v60.reg[STACK_REG(newIS)];
	}
}

#define GETINTVECT(nint)	MemRead32(SBR + (nint)*4)

// Addressing mode decoding functions
#include "am.c"

// Opcode functions
#include "op12.c"
#include "op2.c"
#include "op3.c"
#include "op4.c"
#include "op5.c"
#include "op6.c"
#include "op7a.c"

UINT32 opUNHANDLED(void)
{
	logerror("Unhandled OpCode found : %02x at %08x\n", MemRead16(PC), PC);
	return 1;
}

// Opcode jump table
#include "optable.c"

static int v60_default_irq_cb(int irqline)
{
	return 0;
}

void v60_init(void)
{
	static int opt_init = 0;
	if(!opt_init) {
		InitTables();	// set up opcode tables
#ifdef MAME_DEBUG
		v60_dasm_init();
#endif
		opt_init = 1;
	}

	// Set PIR (Processor ID) for NEC v60. LSB is reserved to NEC,
	// so I don't know what it contains.
	PIR = 0x00006000;
	v60.irq_cb = v60_default_irq_cb;
	v60.irq_line = CLEAR_LINE;
	v60.nmi_line = CLEAR_LINE;
	v60.info = &v60_i;
}

void v70_init(void)
{
	v60_init();
	v60.info = &v70_i;
}

void v60_reset(void *param)
{
	PSW		= 0x10000000;
	PC		= v60.info->start_pc;
	SBR		= 0x00000000;
	SYCW	= 0x00000070;
	TKCW	= 0x0000e000;
	PSW2	= 0x0000f002;

	_CY	= 0;
	_OV	= 0;
	_S	= 0;
	_Z	= 0;
}

void v60_exit(void)
{
}

static void v60_do_irq(int vector)
{
	UPDATEPSW();

	// Push PC and PSW onto the stack
	SP-=4;
	MemWrite32(SP, PSW);
	SP-=4;
	MemWrite32(SP, PC);

	// Change to interrupt context
	PSW &= ~(3 << 24);  // PSW.EL = 0
	PSW &= ~(1 << 18);  // PSW.IE = 0
	PSW &= ~(1 << 16);  // PSW.TE = 0
	PSW &= ~(1 << 27);  // PSW.TP = 0
	PSW &= ~(1 << 17);  // PSW.AE = 0
	PSW &= ~(1 << 29);  // PSW.EM = 0
	PSW |=  (1 << 31);  // PSW.ASA = 1

	// Jump to vector for user interrupt
	PC = GETINTVECT(vector);
}

static void v60_try_irq(void)
{
	if(v60.irq_line == CLEAR_LINE)
		return;
	if((PSW & (1<<18)) != 0) {
		int vector;
		if(v60.irq_line != ASSERT_LINE)
			v60.irq_line = CLEAR_LINE;

		vector = v60.irq_cb(0);

		v60_do_irq(vector + 0x40);
	} else if(v60.irq_line == PULSE_LINE)
		v60.irq_line = CLEAR_LINE;
}

void v60_set_irq_line(int irqline, int state)
{
	if(irqline == IRQ_LINE_NMI) {
		switch(state) {
		case ASSERT_LINE:
			if(v60.nmi_line == CLEAR_LINE) {
				v60.nmi_line = ASSERT_LINE;
				v60_do_irq(2);
			}
			break;
		case CLEAR_LINE:
			v60.nmi_line = CLEAR_LINE;
			break;
		case HOLD_LINE:
		case PULSE_LINE:
			v60.nmi_line = CLEAR_LINE;
			v60_do_irq(2);
			break;
		}
	} else {
		v60.irq_line = state;
		v60_try_irq();
	}
}

void v60_set_irq_callback(int (*irq_cb)(int irqline))
{
	v60.irq_cb = irq_cb;
}

// Actual cycles/instruction is unknown

int v60_execute(int cycles)
{
	v60_ICount = cycles;
	if(v60.irq_line != CLEAR_LINE)
		v60_try_irq();
	while(v60_ICount) {
		PPC = PC;
		CALL_MAME_DEBUG;
		v60_ICount--;
		PC += OpCodeTable[MemRead8(PC)]();
		if(v60.irq_line != CLEAR_LINE)
			v60_try_irq();
	}

	return cycles;
}

unsigned v60_get_context(void *dst)
{
	if(dst)
		*(struct v60info *)dst = v60;
	return sizeof(struct v60info);
}

void v60_set_context(void *src)
{
	if(src)
		v60 = *(struct v60info *)src;
}

static UINT8 v60_reg_layout[] = {
	V60_PC, V60_TR, -1,
	-1,
	V60_R0, V60_R1, -1,
	V60_R2, V60_R3, -1,
	V60_R4, V60_R5, -1,
	V60_R6, V60_R7, -1,
	V60_R8, V60_R9, -1,
	V60_R10, V60_R11, -1,
	V60_R12, V60_R13, -1,
	V60_R14, V60_R15, -1,
	V60_R16, V60_R17, -1,
	V60_R18, V60_R19, -1,
	V60_R20, V60_R21, -1,
	V60_R22, V60_R23, -1,
	V60_R24, V60_R25, -1,
	V60_R26, V60_R27, -1,
	V60_R28, V60_AP, -1,
	V60_FP, V60_SP, -1,
	-1,
	V60_SBR, V60_ISP, -1,
	V60_L0SP, V60_L1SP, -1,
	V60_L2SP, V60_L3SP, 0
};

static UINT8 v60_win_layout[] = {
	45, 0,35,13,	/* register window (top right) */
	 0, 0,44,13,	/* disassembler window (left, upper) */
	 0,14,44, 8,	/* memory #1 window (left, middle) */
	45,14,35, 8,	/* memory #2 window (lower) */
	 0,23,80, 1 	/* command line window (bottom rows) */
};


unsigned v60_get_reg(int regnum)
{
	switch(regnum) {
	case REG_PC:
		return PC;
	case REG_PREVIOUSPC:
		return PPC;
	case REG_SP:
		return SP;
	}
	if(regnum >= 1 && regnum < V60_REGMAX)
		return v60.reg[regnum - 1];
	return 0;
}

void v60_set_reg(int regnum, unsigned val)
{
	switch(regnum) {
	case REG_PC:
		PC = val;
		return;
	case REG_SP:
		SP = val;
		return;
	}
	if(regnum >= 1 && regnum < V60_REGMAX)
		v60.reg[regnum - 1] = val;
}

const char *v60_info(void *context, int regnum)
{
	struct v60info *r = context ? context : &v60;
	static char buffer[32][47+1];
	static int which = 0;

	switch(regnum) {
	case CPU_INFO_NAME:
		return "V60";
	case CPU_INFO_FAMILY:
		return "NEC V60";
	case CPU_INFO_VERSION:
		return "1.0";
	case CPU_INFO_CREDITS:
		return "Farfetch'd and R.Belmont";
	case CPU_INFO_REG_LAYOUT:
		return (const char *)v60_reg_layout;
	case CPU_INFO_WIN_LAYOUT:
		return (const char *)v60_win_layout;
	}

	which = (which+1) % 32;
	buffer[which][0] = '\0';

	if(regnum > CPU_INFO_REG && regnum < CPU_INFO_REG + V60_REGMAX) {
		int reg = regnum - CPU_INFO_REG - 1;
		sprintf(buffer[which], "%s:%08X", v60_reg_names[reg], r->reg[reg]);
	}

	return buffer[which];
}

const char *v70_info(void *context, int regnum)
{
	switch(regnum) {
	case CPU_INFO_NAME:
		return "V70";
	case CPU_INFO_FAMILY:
		return "NEC V70";
	default:
		return v60_info(context, regnum);
	}
}

#ifndef MAME_DEBUG
unsigned v60_dasm(char *buffer,  unsigned pc)
{
	sprintf(buffer, "$%02X", cpu_readmem24lew(pc));
	return 1;
}

unsigned v70_dasm(char *buffer,  unsigned pc)
{
	sprintf(buffer, "$%02X", cpu_readmem32ledw(pc));
	return 1;
}
#endif

