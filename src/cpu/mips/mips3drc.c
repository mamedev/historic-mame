/*###################################################################################################
**
**
**		mips3drc.c
**		x86 Dynamic recompiler for MIPS III/IV emulator.
**		Written by Aaron Giles
**
**
**		Philosophy: this is intended to be a very basic implementation of a dynamic compiler in
**		order to keep things simple. There are certainly more optimizations that could be added
**		but for now, we keep it strictly to NOP stripping and LUI optimizations.
**
**
**		Still not implemented:
**			* MMU (it is logged, however)
**
**
**#################################################################################################*/

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "driver.h"
#include "mamedbg.h"
#include "mips3.h"
#include "x86drc.h"


//In GCC, ECX/EDX are volatile
//EBX/EBP/ESI/EDI are non-volatile



/*###################################################################################################
**	CONFIGURATION
**#################################################################################################*/

#define LOG_CODE			(0)

#define CACHE_SIZE			(16 * 1024 * 1024)
#define MAX_INSTRUCTIONS	512



/*###################################################################################################
**	CONSTANTS
**#################################################################################################*/

/* COP0 registers */
#define COP0_Index			0
#define COP0_Random			1
#define COP0_EntryLo		2
#define COP0_EntryLo0		2
#define COP0_EntryLo1		3
#define COP0_Context		4
#define COP0_PageMask		5
#define COP0_Wired			6
#define COP0_BadVAddr		8
#define COP0_Count			9
#define COP0_EntryHi		10
#define COP0_Compare		11
#define COP0_Status			12
#define COP0_Cause			13
#define COP0_EPC			14
#define COP0_PRId			15
#define COP0_Config			16
#define COP0_XContext		20

/* Status register bits */
#define SR_IE				0x00000001
#define SR_EXL				0x00000002
#define SR_ERL				0x00000004
#define SR_KSU_MASK			0x00000018
#define SR_KSU_KERNEL		0x00000000
#define SR_KSU_SUPERVISOR	0x00000008
#define SR_KSU_USER			0x00000010
#define SR_IMSW0			0x00000100
#define SR_IMSW1			0x00000200
#define SR_IMEX0			0x00000400
#define SR_IMEX1			0x00000800
#define SR_IMEX2			0x00001000
#define SR_IMEX3			0x00002000
#define SR_IMEX4			0x00004000
#define SR_IMEX5			0x00008000
#define SR_DE				0x00010000
#define SR_CE				0x00020000
#define SR_CH				0x00040000
#define SR_SR				0x00100000
#define SR_TS				0x00200000
#define SR_BEV				0x00400000
#define SR_RE				0x02000000
#define SR_COP0				0x10000000
#define SR_COP1				0x20000000
#define SR_COP2				0x40000000
#define SR_COP3				0x80000000

/* exception types */
#define EXCEPTION_INTERRUPT	0
#define EXCEPTION_TLBMOD	1
#define EXCEPTION_TLBLOAD	2
#define EXCEPTION_TLBSTORE	3
#define EXCEPTION_ADDRLOAD	4
#define EXCEPTION_ADDRSTORE	5
#define EXCEPTION_BUSINST	6
#define EXCEPTION_BUSDATA	7
#define EXCEPTION_SYSCALL	8
#define EXCEPTION_BREAK		9
#define EXCEPTION_INVALIDOP	10
#define EXCEPTION_BADCOP	11
#define EXCEPTION_OVERFLOW	12
#define EXCEPTION_TRAP		13



/*###################################################################################################
**	HELPER MACROS
**#################################################################################################*/

#define RSREG		((op >> 21) & 31)
#define RTREG		((op >> 16) & 31)
#define RDREG		((op >> 11) & 31)
#define SHIFT		((op >> 6) & 31)

#define FRREG		((op >> 21) & 31)
#define FTREG		((op >> 16) & 31)
#define FSREG		((op >> 11) & 31)
#define FDREG		((op >> 6) & 31)

#define IS_SINGLE(o) (((o) & (1 << 21)) == 0)
#define IS_DOUBLE(o) (((o) & (1 << 21)) != 0)
#define IS_FLOAT(o) (((o) & (1 << 23)) == 0)
#define IS_INTEGRAL(o) (((o) & (1 << 23)) != 0)

#define SIMMVAL		((INT16)op)
#define UIMMVAL		((UINT16)op)
#define LIMMVAL		(op & 0x03ffffff)



/*###################################################################################################
**	STRUCTURES & TYPEDEFS
**#################################################################################################*/

/* memory access function table */
typedef struct
{
	data8_t		(*readbyte)(offs_t);
	data16_t	(*readword)(offs_t);
	data32_t	(*readlong)(offs_t);
	void		(*writebyte)(offs_t, data8_t);
	void		(*writeword)(offs_t, data16_t);
	void		(*writelong)(offs_t, data32_t);
} memory_handlers;


/* MIPS3 Registers */
typedef struct
{
	/* core registers */
	UINT32		pc;
	UINT64		hi;
	UINT64		lo;
	UINT64		r[32];

	/* COP registers */
	UINT64		cpr[4][32];
	UINT64		ccr[4][32];
	UINT8		cf[4][8];

	/* internal stuff */
	struct drccore *drc;
	UINT32		drcoptions;
	UINT32		nextpc;
	int 		(*irq_callback)(int irqline);
	UINT64		count_zero_time;
	void *		compare_int_timer;
	UINT8		is_mips4;

	/* memory accesses */
	UINT8		bigendian;
	memory_handlers memory;

	/* cache memory */
	data32_t *	icache;
	data32_t *	dcache;
	size_t		icache_size;
	size_t		dcache_size;
	
	/* callbacks */
	void *		generate_interrupt_exception;
	void *		generate_cop_exception;
	void *		generate_overflow_exception;
	void *		generate_invalidop_exception;
	void *		generate_syscall_exception;
	void *		generate_break_exception;
	void *		generate_trap_exception;
} mips3_regs;



/*###################################################################################################
**	PROTOTYPES
**#################################################################################################*/

static void mips3drc_init(void);
static void mips3drc_reset(struct drccore *drc);
static void mips3drc_recompile(struct drccore *drc);
static void mips3drc_entrygen(struct drccore *drc);

static void update_cycle_counting(void);

static offs_t mips3_dasm(char *buffer, offs_t pc);



/*###################################################################################################
**	PUBLIC GLOBAL VARIABLES
**#################################################################################################*/

static int	mips3_icount;



/*###################################################################################################
**	PRIVATE GLOBAL VARIABLES
**#################################################################################################*/

static mips3_regs mips3;

#if LOG_CODE
static FILE *symfile;
#endif

static const memory_handlers be_memory =
{
	program_read_byte_32be,  program_read_word_32be,  program_read_dword_32be,
	program_write_byte_32be, program_write_word_32be, program_write_dword_32be
};

static const memory_handlers le_memory =
{
	program_read_byte_32le,  program_read_word_32le,  program_read_dword_32le,
	program_write_byte_32le, program_write_word_32le, program_write_dword_32le
};



/*###################################################################################################
**	IRQ HANDLING
**#################################################################################################*/

static void set_irq_line(int irqline, int state)
{
	if (state != CLEAR_LINE)
		mips3.cpr[0][COP0_Cause] |= 0x400 << irqline;
	else
		mips3.cpr[0][COP0_Cause] &= ~(0x400 << irqline);
}



/*###################################################################################################
**	CONTEXT SWITCHING
**#################################################################################################*/

static void mips3_get_context(void *dst)
{
	/* copy the context */
	if (dst)
		*(mips3_regs *)dst = mips3;
}


static void mips3_set_context(void *src)
{
	/* copy the context */
	if (src)
		mips3 = *(mips3_regs *)src;
}



/*###################################################################################################
**	INITIALIZATION AND SHUTDOWN
**#################################################################################################*/

static void compare_int_callback(int cpu)
{
	cpu_set_irq_line(cpu, 5, ASSERT_LINE);
}


static void mips3_init(void)
{
	mips3drc_init();

	/* allocate a timer */
	mips3.compare_int_timer = timer_alloc(compare_int_callback);

#if LOG_CODE
{
	FILE *munge_file = fopen("codemunge.bat", "w");
	if (munge_file)
	{
		int regnum;
		for (regnum = 0; regnum < 32; regnum++)
		{
			fprintf(munge_file, "@gsar -s%08Xh -rr%d.lo -o code.asm\n", (UINT32)(LO(&mips3.r[regnum])), regnum);
			fprintf(munge_file, "@gsar -s%08Xh -rr%d.hi -o code.asm\n", (UINT32)(HI(&mips3.r[regnum])), regnum);
			fprintf(munge_file, "@gsar -s%08Xh -rcp0.%d -o code.asm\n", (UINT32)(&mips3.cpr[0][regnum]), regnum);
			fprintf(munge_file, "@gsar -s%08Xh -rcp1.%d -o code.asm\n", (UINT32)(&mips3.cpr[1][regnum]), regnum);
			fprintf(munge_file, "@gsar -s%08Xh -rcc1.%d -o code.asm\n", (UINT32)(&mips3.ccr[1][regnum]), regnum);
		}
		for (regnum = 0; regnum < 8; regnum++)
			fprintf(munge_file, "@gsar -s%08Xh -rcf1.%d -o code.asm\n", (UINT32)(&mips3.cf[1][regnum]), regnum);
		fprintf(munge_file, "@gsar -s%08Xh -rpc -o code.asm\n", (UINT32)&mips3.pc);
		fprintf(munge_file, "@gsar -s%08Xh -ricount -o code.asm\n", (UINT32)&mips3_icount);
		fclose(munge_file);
	}
}
#endif
}


static void mips3_reset(void *param, int bigendian, int mips4, UINT32 prid)
{
	struct mips3_config *config = param;
	UINT32 configreg;
	int divisor;

	/* allocate memory */
	mips3.icache = malloc(config->icache);
	mips3.dcache = malloc(config->dcache);
	if (!mips3.icache || !mips3.dcache)
	{
		fprintf(stderr, "error: couldn't allocate cache for mips3!\n");
		exit(1);
	}

	/* set up the endianness */
	mips3.bigendian = bigendian;
	if (mips3.bigendian)
		mips3.memory = be_memory;
	else
		mips3.memory = le_memory;

	/* initialize the rest of the config */
	mips3.icache_size = config->icache;
	mips3.dcache_size = config->dcache;

	/* initialize the state */
	mips3.pc = 0xbfc00000;
	mips3.nextpc = ~0;
	mips3.cpr[0][COP0_Status] = SR_BEV | SR_ERL;
	mips3.cpr[0][COP0_Compare] = 0xffffffff;
	mips3.cpr[0][COP0_Count] = 0;
	mips3.count_zero_time = activecpu_gettotalcycles64();
	
	/* reset the DRC */
	drc_cache_reset(mips3.drc);
	
	/* config register: set the cache line size to 32 bytes */
	configreg = 0x00026030;

	/* config register: set the data cache size */
	     if (config->icache <= 0x01000) configreg |= 0 << 6;
	else if (config->icache <= 0x02000) configreg |= 1 << 6;
	else if (config->icache <= 0x04000) configreg |= 2 << 6;
	else if (config->icache <= 0x08000) configreg |= 3 << 6;
	else if (config->icache <= 0x10000) configreg |= 4 << 6;
	else if (config->icache <= 0x20000) configreg |= 5 << 6;
	else if (config->icache <= 0x40000) configreg |= 6 << 6;
	else                                configreg |= 7 << 6;
	
	/* config register: set the instruction cache size */
	     if (config->icache <= 0x01000) configreg |= 0 << 9;
	else if (config->icache <= 0x02000) configreg |= 1 << 9;
	else if (config->icache <= 0x04000) configreg |= 2 << 9;
	else if (config->icache <= 0x08000) configreg |= 3 << 9;
	else if (config->icache <= 0x10000) configreg |= 4 << 9;
	else if (config->icache <= 0x20000) configreg |= 5 << 9;
	else if (config->icache <= 0x40000) configreg |= 6 << 9;
	else                                configreg |= 7 << 9;
	
	/* config register: set the endianness bit */
	if (bigendian) configreg |= 0x00008000;
	
	/* config register: set the system clock divider */
	divisor = 2;
	if (config->system_clock != 0)
		divisor = Machine->drv->cpu[cpu_getactivecpu()].cpu_clock / config->system_clock;
	configreg |= (((divisor < 2) ? 2 : (divisor > 8) ? 8 : divisor) - 2) << 28;
	
	/* set up the architecture */
	mips3.is_mips4 = mips4;
	mips3.cpr[0][COP0_PRId] = prid;
	mips3.cpr[0][COP0_Config] = configreg;
}


/*

	R6000A = 0x0600
	R10000 = 0x0900
	R4300  = 0x0b00
	VR41XX = 0x0c00
	R12000 = 0x0e00
	R8000  = 0x1000
	R4600  = 0x2000
	R4650  = 0x2000
	R5000  = 0x2300
	R5432  = 0x5400
	RM7000 = 0x2700
*/

#if (HAS_R4600)
static void r4600be_reset(void *param)
{
	mips3_reset(param, 1, 0, 0x2000);
}

static void r4600le_reset(void *param)
{
	mips3_reset(param, 0, 0, 0x2000);
}
#endif


#if (HAS_R4700)
static void r4700be_reset(void *param)
{
	mips3_reset(param, 1, 0, 0x2100);
}

static void r4700le_reset(void *param)
{
	mips3_reset(param, 0, 0, 0x2100);
}
#endif


#if (HAS_R5000)
static void r5000be_reset(void *param)
{
	mips3_reset(param, 1, 1, 0x2300);
}

static void r5000le_reset(void *param)
{
	mips3_reset(param, 0, 1, 0x2300);
}
#endif


#if (HAS_QED5271)
static void qed5271be_reset(void *param)
{
	mips3_reset(param, 1, 1, 0x2300);
}

static void qed5271le_reset(void *param)
{
	mips3_reset(param, 1, 0, 0x2300);
}
#endif


#if (HAS_RM7000)
static void rm7000be_reset(void *param)
{
	mips3_reset(param, 1, 1, 0x2700);
}

static void rm7000le_reset(void *param)
{
	mips3_reset(param, 0, 1, 0x2700);
}
#endif


static int mips3_execute(int cycles)
{
	/* update the cycle timing */
	update_cycle_counting();

	/* count cycles and interrupt cycles */
	mips3_icount = cycles;
	drc_execute(mips3.drc);
	return cycles - mips3_icount;
}


static void mips3_exit(void)
{
	/* free cache memory */
	if (mips3.icache)
		free(mips3.icache);
	mips3.icache = NULL;

	if (mips3.dcache)
		free(mips3.dcache);
	mips3.dcache = NULL;

#if LOG_CODE
	if (symfile) fclose(symfile);
#endif
	drc_exit(mips3.drc);
}



/*###################################################################################################
**	RECOMPILER CALLBACKS
**#################################################################################################*/

/*------------------------------------------------------------------
	update_cycle_counting
------------------------------------------------------------------*/

static void update_cycle_counting(void)
{
	/* modify the timer to go off */
	if ((mips3.cpr[0][COP0_Status] & 0x8000) && mips3.cpr[0][COP0_Compare] != 0xffffffff)
	{
		UINT32 count = (activecpu_gettotalcycles64() - mips3.count_zero_time) / 2;
		UINT32 compare = mips3.cpr[0][COP0_Compare];
		UINT32 cyclesleft = compare - count;
		double newtime = TIME_IN_CYCLES(((INT64)cyclesleft * 2), cpu_getactivecpu());
		
		/* due to accuracy issues, don't bother setting timers unless they're for less than 100msec */
		if (newtime < TIME_IN_MSEC(100))
			timer_adjust(mips3.compare_int_timer, newtime, cpu_getactivecpu(), 0);
	}
	else
		timer_adjust(mips3.compare_int_timer, TIME_NEVER, cpu_getactivecpu(), 0);
}



/*------------------------------------------------------------------
	logtlbentry
------------------------------------------------------------------*/

INLINE void logonetlbentry(int which)
{
	UINT64 hi = mips3.cpr[0][COP0_EntryHi];
	UINT64 lo = mips3.cpr[0][COP0_EntryLo0 + which];
	UINT32 vpn = (((hi >> 13) & 0x07ffffff) << 1) + which;
	UINT32 asid = hi & 0xff;
	UINT32 r = (hi >> 62) & 3;
	UINT32 pfn = (lo >> 6) & 0x00ffffff;
	UINT32 c = (lo >> 3) & 7;
	UINT32 pagesize = ((mips3.cpr[0][COP0_PageMask] >> 1) | 0xfff) + 1;
	UINT64 vaddr = (UINT64)vpn * (UINT64)pagesize;
	UINT64 paddr = (UINT64)pfn * (UINT64)pagesize;

	logerror("index = %08X  pagesize = %08X  vaddr = %08X%08X  paddr = %08X%08X  asid = %02X  r = %X  c = %X  dvg=%c%c%c\n",
			(UINT32)mips3.cpr[0][COP0_Index], pagesize, (UINT32)(vaddr >> 32), (UINT32)vaddr, (UINT32)(paddr >> 32), (UINT32)paddr,
			asid, r, c, (lo & 4) ? 'd' : '.', (lo & 2) ? 'v' : '.', (lo & 1) ? 'g' : '.');
}

static void logtlbentry(void)
{
	logonetlbentry(0);
	logonetlbentry(1);
}



/*------------------------------------------------------------------
	log_code
------------------------------------------------------------------*/

static void log_code(struct drccore *drc)
{
#if LOG_CODE
	FILE *temp;
	temp = fopen("code.bin", "wb");
	fwrite(drc->cache_base, 1, drc->cache_top - drc->cache_base, temp);
	fclose(temp);
#endif
}



/*------------------------------------------------------------------
	log_symbol
------------------------------------------------------------------*/

static void log_symbol(struct drccore *drc, UINT32 pc)
{
#if LOG_CODE
	static int first = 1;
	char temp[256];
	if (pc == ~0)
	{
		if (symfile) fclose(symfile);
		symfile = NULL;
		return;
	}
	if (!symfile) symfile = fopen("code.sym", first ? "w" : "a");
	first = 0;
	if (pc == ~1)
	{
		fprintf(symfile, "%08X   ===============================================================\n", drc->cache_top - drc->cache_base);
		return;
	}
	else if (pc == ~2)
	{
		fprintf(symfile, "%08X   ~--------------------------------------------\n", drc->cache_top - drc->cache_base);
		return;
	}
	else
	{
		mips3_dasm(temp, pc);
		fprintf(symfile, "%08X   %08X: %s\n", drc->cache_top - drc->cache_base, pc, temp);
	}
#endif
}



/*###################################################################################################
**	RECOMPILER CORE
**#################################################################################################*/

#include "mdrcold.c"
//#include "mdrc32.c"



/*###################################################################################################
**	DEBUGGER DEFINITIONS
**#################################################################################################*/

static UINT8 mips3_reg_layout[] =
{
	MIPS3_PC,		MIPS3_SR,		-1,
	MIPS3_EPC,		MIPS3_CAUSE,	-1,
	MIPS3_COUNT,	MIPS3_COMPARE,	-1,
	MIPS3_HI,		MIPS3_LO,		-1,
	MIPS3_R0,	 	MIPS3_R16,		-1,
	MIPS3_R1, 		MIPS3_R17,		-1,
	MIPS3_R2, 		MIPS3_R18,		-1,
	MIPS3_R3, 		MIPS3_R19,		-1,
	MIPS3_R4, 		MIPS3_R20,		-1,
	MIPS3_R5, 		MIPS3_R21,		-1,
	MIPS3_R6, 		MIPS3_R22,		-1,
	MIPS3_R7, 		MIPS3_R23,		-1,
	MIPS3_R8,		MIPS3_R24,		-1,
	MIPS3_R9,		MIPS3_R25,		-1,
	MIPS3_R10,		MIPS3_R26,		-1,
	MIPS3_R11,		MIPS3_R27,		-1,
	MIPS3_R12,		MIPS3_R28,		-1,
	MIPS3_R13,		MIPS3_R29,		-1,
	MIPS3_R14,		MIPS3_R30,		-1,
	MIPS3_R15,		MIPS3_R31,		0
};

static UINT8 mips3_win_layout[] =
{
	 0, 0,45,20,	/* register window (top rows) */
	46, 0,33,14,	/* disassembler window (left colums) */
	 0,21,45, 1,	/* memory #1 window (right, upper middle) */
	46,15,33, 7,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1,	/* command line window (bottom rows) */
};



/*###################################################################################################
**	DISASSEMBLY HOOK
**#################################################################################################*/

static offs_t mips3_dasm(char *buffer, offs_t pc)
{
#if defined(MAME_DEBUG) || (LOG_CODE)
	extern unsigned dasmmips3(char *, unsigned);
	unsigned result;
	change_pc(pc);
    result = dasmmips3(buffer, pc);
	change_pc(mips3.pc);
    return result;
#else
	sprintf(buffer, "$%04X", cpu_readop32(pc));
	return 4;
#endif
}


/**************************************************************************
 * Generic set_info
 **************************************************************************/

static void mips3_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_IRQ_STATE + MIPS3_IRQ0:		set_irq_line(MIPS3_IRQ0, info->i);		break;
		case CPUINFO_INT_IRQ_STATE + MIPS3_IRQ1:		set_irq_line(MIPS3_IRQ1, info->i);		break;
		case CPUINFO_INT_IRQ_STATE + MIPS3_IRQ2:		set_irq_line(MIPS3_IRQ2, info->i);		break;
		case CPUINFO_INT_IRQ_STATE + MIPS3_IRQ3:		set_irq_line(MIPS3_IRQ3, info->i);		break;
		case CPUINFO_INT_IRQ_STATE + MIPS3_IRQ4:		set_irq_line(MIPS3_IRQ4, info->i);		break;
		case CPUINFO_INT_IRQ_STATE + MIPS3_IRQ5:		set_irq_line(MIPS3_IRQ5, info->i);		break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + MIPS3_PC:			mips3.pc = info->i;						break;
		case CPUINFO_INT_REGISTER + MIPS3_SR:			mips3.cpr[0][COP0_Status] = info->i; 	break;
		case CPUINFO_INT_REGISTER + MIPS3_EPC:			mips3.cpr[0][COP0_EPC] = info->i; 		break;
		case CPUINFO_INT_REGISTER + MIPS3_CAUSE:		mips3.cpr[0][COP0_Cause] = info->i;		break;
		case CPUINFO_INT_REGISTER + MIPS3_COUNT:		mips3.cpr[0][COP0_Count] = info->i; 	break;
		case CPUINFO_INT_REGISTER + MIPS3_COMPARE:		mips3.cpr[0][COP0_Compare] = info->i; 	break;

		case CPUINFO_INT_REGISTER + MIPS3_R0:			mips3.r[0] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R1:			mips3.r[1] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R2:			mips3.r[2] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R3:			mips3.r[3] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R4:			mips3.r[4] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R5:			mips3.r[5] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R6:			mips3.r[6] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R7:			mips3.r[7] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R8:			mips3.r[8] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R9:			mips3.r[9] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R10:			mips3.r[10] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R11:			mips3.r[11] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R12:			mips3.r[12] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R13:			mips3.r[13] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R14:			mips3.r[14] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R15:			mips3.r[15] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R16:			mips3.r[16] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R17:			mips3.r[17] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R18:			mips3.r[18] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R19:			mips3.r[19] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R20:			mips3.r[20] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R21:			mips3.r[21] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R22:			mips3.r[22] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R23:			mips3.r[23] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R24:			mips3.r[24] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R25:			mips3.r[25] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R26:			mips3.r[26] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R27:			mips3.r[27] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R28:			mips3.r[28] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R29:			mips3.r[29] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_R30:			mips3.r[30] = info->i;					break;
		case CPUINFO_INT_SP:	
		case CPUINFO_INT_REGISTER + MIPS3_R31:			mips3.r[31] = info->i;					break;
		case CPUINFO_INT_REGISTER + MIPS3_HI:			mips3.hi = info->i;						break;
		case CPUINFO_INT_REGISTER + MIPS3_LO:			mips3.lo = info->i;						break;
		
		case CPUINFO_INT_MIPS3_DRC_OPTIONS:				mips3.drcoptions = info->i;				break;
		
		/* --- the following bits of info are set as pointers to data or functions --- */
		case CPUINFO_PTR_IRQ_CALLBACK:					mips3.irq_callback = info->irqcallback;	break;
	}
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

void mips3_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(mips3);				break;
		case CPUINFO_INT_IRQ_LINES:						info->i = 6;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = 0;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 4;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 4;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 1;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 40;							break;
		
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_IRQ_STATE + MIPS3_IRQ0:		info->i = (mips3.cpr[0][COP0_Cause] & 0x400) ? ASSERT_LINE : CLEAR_LINE;	break;
		case CPUINFO_INT_IRQ_STATE + MIPS3_IRQ1:		info->i = (mips3.cpr[0][COP0_Cause] & 0x800) ? ASSERT_LINE : CLEAR_LINE;	break;
		case CPUINFO_INT_IRQ_STATE + MIPS3_IRQ2:		info->i = (mips3.cpr[0][COP0_Cause] & 0x1000) ? ASSERT_LINE : CLEAR_LINE;	break;
		case CPUINFO_INT_IRQ_STATE + MIPS3_IRQ3:		info->i = (mips3.cpr[0][COP0_Cause] & 0x2000) ? ASSERT_LINE : CLEAR_LINE;	break;
		case CPUINFO_INT_IRQ_STATE + MIPS3_IRQ4:		info->i = (mips3.cpr[0][COP0_Cause] & 0x4000) ? ASSERT_LINE : CLEAR_LINE;	break;
		case CPUINFO_INT_IRQ_STATE + MIPS3_IRQ5:		info->i = (mips3.cpr[0][COP0_Cause] & 0x8000) ? ASSERT_LINE : CLEAR_LINE;	break;

		case CPUINFO_INT_PREVIOUSPC:					/* not implemented */					break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + MIPS3_PC:			info->i = mips3.pc;						break;
		case CPUINFO_INT_REGISTER + MIPS3_SR:			info->i = mips3.cpr[0][COP0_Status];	break;
		case CPUINFO_INT_REGISTER + MIPS3_EPC:			info->i = mips3.cpr[0][COP0_EPC];		break;
		case CPUINFO_INT_REGISTER + MIPS3_CAUSE:		info->i = mips3.cpr[0][COP0_Cause];		break;
		case CPUINFO_INT_REGISTER + MIPS3_COUNT:		info->i = ((activecpu_gettotalcycles64() - mips3.count_zero_time) / 2); break;
		case CPUINFO_INT_REGISTER + MIPS3_COMPARE:		info->i = mips3.cpr[0][COP0_Compare];	break;

		case CPUINFO_INT_REGISTER + MIPS3_R0:			info->i = mips3.r[0];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R1:			info->i = mips3.r[1];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R2:			info->i = mips3.r[2];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R3:			info->i = mips3.r[3];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R4:			info->i = mips3.r[4];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R5:			info->i = mips3.r[5];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R6:			info->i = mips3.r[6];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R7:			info->i = mips3.r[7];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R8:			info->i = mips3.r[8];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R9:			info->i = mips3.r[9];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R10:			info->i = mips3.r[10];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R11:			info->i = mips3.r[11];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R12:			info->i = mips3.r[12];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R13:			info->i = mips3.r[13];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R14:			info->i = mips3.r[14];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R15:			info->i = mips3.r[15];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R16:			info->i = mips3.r[16];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R17:			info->i = mips3.r[17];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R18:			info->i = mips3.r[18];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R19:			info->i = mips3.r[19];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R20:			info->i = mips3.r[20];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R21:			info->i = mips3.r[21];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R22:			info->i = mips3.r[22];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R23:			info->i = mips3.r[23];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R24:			info->i = mips3.r[24];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R25:			info->i = mips3.r[25];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R26:			info->i = mips3.r[26];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R27:			info->i = mips3.r[27];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R28:			info->i = mips3.r[28];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R29:			info->i = mips3.r[29];					break;
		case CPUINFO_INT_REGISTER + MIPS3_R30:			info->i = mips3.r[30];					break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + MIPS3_R31:			info->i = mips3.r[31];					break;
		case CPUINFO_INT_REGISTER + MIPS3_HI:			info->i = mips3.hi;						break;
		case CPUINFO_INT_REGISTER + MIPS3_LO:			info->i = mips3.lo;						break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = mips3_set_info;			break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = mips3_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = mips3_set_context;	break;
		case CPUINFO_PTR_INIT:							info->init = mips3_init;				break;
		case CPUINFO_PTR_RESET:							/* provided per-CPU */					break;
		case CPUINFO_PTR_EXIT:							info->exit = mips3_exit;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = mips3_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = mips3_dasm;			break;
		case CPUINFO_PTR_IRQ_CALLBACK:					info->irqcallback = mips3.irq_callback;	break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &mips3_icount;			break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = mips3_reg_layout;				break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = mips3_win_layout;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "MIPS III"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "MIPS III"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "1.0"); break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright (C) Aaron Giles 2000-2004"); break;

		case CPUINFO_STR_FLAGS:							strcpy(info->s = cpuintrf_temp_str(), " "); break;

		case CPUINFO_STR_REGISTER + MIPS3_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC: %08X", mips3.pc); break;
		case CPUINFO_STR_REGISTER + MIPS3_SR:			sprintf(info->s = cpuintrf_temp_str(), "SR: %08X", (UINT32)mips3.cpr[0][COP0_Status]); break;
		case CPUINFO_STR_REGISTER + MIPS3_EPC:			sprintf(info->s = cpuintrf_temp_str(), "EPC:%08X", (UINT32)mips3.cpr[0][COP0_EPC]); break;
		case CPUINFO_STR_REGISTER + MIPS3_CAUSE: 		sprintf(info->s = cpuintrf_temp_str(), "Cause:%08X", (UINT32)mips3.cpr[0][COP0_Cause]); break;
		case CPUINFO_STR_REGISTER + MIPS3_COUNT: 		sprintf(info->s = cpuintrf_temp_str(), "Count:%08X", (UINT32)((activecpu_gettotalcycles64() - mips3.count_zero_time) / 2)); break;
		case CPUINFO_STR_REGISTER + MIPS3_COMPARE:		sprintf(info->s = cpuintrf_temp_str(), "Compare:%08X", (UINT32)mips3.cpr[0][COP0_Compare]); break;

		case CPUINFO_STR_REGISTER + MIPS3_R0:			sprintf(info->s = cpuintrf_temp_str(), "R0: %08X%08X", (UINT32)(mips3.r[0] >> 32), (UINT32)mips3.r[0]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R1:			sprintf(info->s = cpuintrf_temp_str(), "R1: %08X%08X", (UINT32)(mips3.r[1] >> 32), (UINT32)mips3.r[1]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R2:			sprintf(info->s = cpuintrf_temp_str(), "R2: %08X%08X", (UINT32)(mips3.r[2] >> 32), (UINT32)mips3.r[2]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R3:			sprintf(info->s = cpuintrf_temp_str(), "R3: %08X%08X", (UINT32)(mips3.r[3] >> 32), (UINT32)mips3.r[3]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R4:			sprintf(info->s = cpuintrf_temp_str(), "R4: %08X%08X", (UINT32)(mips3.r[4] >> 32), (UINT32)mips3.r[4]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R5:			sprintf(info->s = cpuintrf_temp_str(), "R5: %08X%08X", (UINT32)(mips3.r[5] >> 32), (UINT32)mips3.r[5]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R6:			sprintf(info->s = cpuintrf_temp_str(), "R6: %08X%08X", (UINT32)(mips3.r[6] >> 32), (UINT32)mips3.r[6]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R7:			sprintf(info->s = cpuintrf_temp_str(), "R7: %08X%08X", (UINT32)(mips3.r[7] >> 32), (UINT32)mips3.r[7]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R8:			sprintf(info->s = cpuintrf_temp_str(), "R8: %08X%08X", (UINT32)(mips3.r[8] >> 32), (UINT32)mips3.r[8]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R9:			sprintf(info->s = cpuintrf_temp_str(), "R9: %08X%08X", (UINT32)(mips3.r[9] >> 32), (UINT32)mips3.r[9]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R10:			sprintf(info->s = cpuintrf_temp_str(), "R10:%08X%08X", (UINT32)(mips3.r[10] >> 32), (UINT32)mips3.r[10]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R11:			sprintf(info->s = cpuintrf_temp_str(), "R11:%08X%08X", (UINT32)(mips3.r[11] >> 32), (UINT32)mips3.r[11]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R12:			sprintf(info->s = cpuintrf_temp_str(), "R12:%08X%08X", (UINT32)(mips3.r[12] >> 32), (UINT32)mips3.r[12]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R13:			sprintf(info->s = cpuintrf_temp_str(), "R13:%08X%08X", (UINT32)(mips3.r[13] >> 32), (UINT32)mips3.r[13]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R14:			sprintf(info->s = cpuintrf_temp_str(), "R14:%08X%08X", (UINT32)(mips3.r[14] >> 32), (UINT32)mips3.r[14]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R15:			sprintf(info->s = cpuintrf_temp_str(), "R15:%08X%08X", (UINT32)(mips3.r[15] >> 32), (UINT32)mips3.r[15]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R16:			sprintf(info->s = cpuintrf_temp_str(), "R16:%08X%08X", (UINT32)(mips3.r[16] >> 32), (UINT32)mips3.r[16]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R17:			sprintf(info->s = cpuintrf_temp_str(), "R17:%08X%08X", (UINT32)(mips3.r[17] >> 32), (UINT32)mips3.r[17]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R18:			sprintf(info->s = cpuintrf_temp_str(), "R18:%08X%08X", (UINT32)(mips3.r[18] >> 32), (UINT32)mips3.r[18]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R19:			sprintf(info->s = cpuintrf_temp_str(), "R19:%08X%08X", (UINT32)(mips3.r[19] >> 32), (UINT32)mips3.r[19]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R20:			sprintf(info->s = cpuintrf_temp_str(), "R20:%08X%08X", (UINT32)(mips3.r[20] >> 32), (UINT32)mips3.r[20]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R21:			sprintf(info->s = cpuintrf_temp_str(), "R21:%08X%08X", (UINT32)(mips3.r[21] >> 32), (UINT32)mips3.r[21]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R22:			sprintf(info->s = cpuintrf_temp_str(), "R22:%08X%08X", (UINT32)(mips3.r[22] >> 32), (UINT32)mips3.r[22]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R23:			sprintf(info->s = cpuintrf_temp_str(), "R23:%08X%08X", (UINT32)(mips3.r[23] >> 32), (UINT32)mips3.r[23]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R24:			sprintf(info->s = cpuintrf_temp_str(), "R24:%08X%08X", (UINT32)(mips3.r[24] >> 32), (UINT32)mips3.r[24]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R25:			sprintf(info->s = cpuintrf_temp_str(), "R25:%08X%08X", (UINT32)(mips3.r[25] >> 32), (UINT32)mips3.r[25]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R26:			sprintf(info->s = cpuintrf_temp_str(), "R26:%08X%08X", (UINT32)(mips3.r[26] >> 32), (UINT32)mips3.r[26]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R27:			sprintf(info->s = cpuintrf_temp_str(), "R27:%08X%08X", (UINT32)(mips3.r[27] >> 32), (UINT32)mips3.r[27]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R28:			sprintf(info->s = cpuintrf_temp_str(), "R28:%08X%08X", (UINT32)(mips3.r[28] >> 32), (UINT32)mips3.r[28]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R29:			sprintf(info->s = cpuintrf_temp_str(), "R29:%08X%08X", (UINT32)(mips3.r[29] >> 32), (UINT32)mips3.r[29]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R30:			sprintf(info->s = cpuintrf_temp_str(), "R30:%08X%08X", (UINT32)(mips3.r[30] >> 32), (UINT32)mips3.r[30]); break;
		case CPUINFO_STR_REGISTER + MIPS3_R31:			sprintf(info->s = cpuintrf_temp_str(), "R31:%08X%08X", (UINT32)(mips3.r[31] >> 32), (UINT32)mips3.r[31]); break;
		case CPUINFO_STR_REGISTER + MIPS3_HI:			sprintf(info->s = cpuintrf_temp_str(), "HI: %08X%08X", (UINT32)(mips3.hi >> 32), (UINT32)mips3.hi); break;
		case CPUINFO_STR_REGISTER + MIPS3_LO:			sprintf(info->s = cpuintrf_temp_str(), "LO: %08X%08X", (UINT32)(mips3.lo >> 32), (UINT32)mips3.lo); break;
	}
}


#if (HAS_R4600)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

void r4600be_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = r4600be_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "R4600 (big)"); break;

		default:
			mips3_get_info(state, info);
			break;
	}
}


void r4600le_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = r4600le_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "R4600 (little)"); break;

		default:
			mips3_get_info(state, info);
			break;
	}
}
#endif


#if (HAS_R4700)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

void r4700be_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = r4700be_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "R4700 (big)"); break;

		default:
			mips3_get_info(state, info);
			break;
	}
}


void r4700le_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = r4700le_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "R4700 (little)"); break;

		default:
			mips3_get_info(state, info);
			break;
	}
}
#endif


#if (HAS_R5000)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

void r5000be_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = r5000be_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "R5000 (big)"); break;

		default:
			mips3_get_info(state, info);
			break;
	}
}


void r5000le_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = r5000le_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "R5000 (little)"); break;

		default:
			mips3_get_info(state, info);
			break;
	}
}
#endif


#if (HAS_QED5271)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

void qed5271be_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = qed5271be_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "QED5271 (big)"); break;

		default:
			mips3_get_info(state, info);
			break;
	}
}


void qed5271le_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = qed5271le_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "QED5271 (little)"); break;

		default:
			mips3_get_info(state, info);
			break;
	}
}
#endif


#if (HAS_RM7000)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

void rm7000be_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = rm7000be_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "RM7000 (big)"); break;

		default:
			mips3_get_info(state, info);
			break;
	}
}


void rm7000le_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_LE;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = rm7000le_reset;			break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "RM7000 (little)"); break;

		default:
			mips3_get_info(state, info);
			break;
	}
}
#endif




#if !defined(MAME_DEBUG) && (LOG_CODE)
#include "mips3dsm.c"
#endif
