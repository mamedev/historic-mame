/*
	Interface routine for 68kem <-> Mame
*/

#include "driver.h"
#include "mamedbg.h"
#include "m68000.h"
#include "state.h"

struct m68k_memory_interface a68k_memory_intf;

// If we are only using assembler cores, we need to define these
// otherwise they are declared by the C core.

#ifdef A68K0
#ifdef A68K2
int m68k_ICount;
struct m68k_memory_interface m68k_memory_intf;
offs_t m68k_encrypted_opcode_start[MAX_CPU];
offs_t m68k_encrypted_opcode_end[MAX_CPU];

void m68k_set_encrypted_opcode_range(int cpunum, offs_t start, offs_t end)
{
	m68k_encrypted_opcode_start[cpunum] = start;
	m68k_encrypted_opcode_end[cpunum] = end;
}
#endif
#endif

enum
{
	M68K_CPU_TYPE_INVALID,
	M68K_CPU_TYPE_68000,
	M68K_CPU_TYPE_68010,
	M68K_CPU_TYPE_68EC020,
	M68K_CPU_TYPE_68020,
	M68K_CPU_TYPE_68030,	/* Supported by disassembler ONLY */
	M68K_CPU_TYPE_68040		/* Supported by disassembler ONLY */
};

#define A68K_SET_PC_CALLBACK(A)     change_pc(A)

int illegal_op = 0 ;
int illegal_pc = 0 ;

UINT32 mem_amask;

#ifdef MAME_DEBUG
void m68k_illegal_opcode(void)
{
	logerror("Illegal Opcode %4x at %8x\n",illegal_op,illegal_pc);
}
#endif

unsigned int m68k_disassemble(char* str_buff, unsigned int pc, unsigned int cpu_type);

#ifdef _WIN32
#define CONVENTION __cdecl
#else
#define CONVENTION
#endif

/* Use the x86 assembly core */
typedef struct
{
    UINT32 d[8];             /* 0x0004 8 Data registers */
    UINT32 a[8];             /* 0x0024 8 Address registers */

    UINT32 isp;              /* 0x0048 */

    UINT32 sr_high;          /* 0x004C System registers */
    UINT32 ccr;              /* 0x0050 CCR in Intel Format */
    UINT32 x_carry;          /* 0x0054 Extended Carry */

    UINT32 pc;               /* 0x0058 Program Counter */

    UINT32 IRQ_level;        /* 0x005C IRQ level you want the MC68K process (0=None)  */

    /* Backward compatible with C emulator - Only set in Debug compile */

    UINT16 sr;
    UINT16 filler;

    int (*irq_callback)(int irqline);

    UINT32 previous_pc;      /* last PC used */

    int (*reset_callback)(void);

    UINT32 sfc;              /* Source Function Code. (68010) */
    UINT32 dfc;              /* Destination Function Code. (68010) */
    UINT32 usp;              /* User Stack (All) */
    UINT32 vbr;              /* Vector Base Register. (68010) */

    UINT32 BankID;			 /* Memory bank in use */
    UINT32 CPUtype;		  	 /* CPU Type 0=68000,1=68010,2=68020 */
	UINT32 FullPC;

	struct m68k_memory_interface Memory_Interface;

} a68k_cpu_context;


static UINT8 M68K_layout[] = {
	M68K_PC, M68K_ISP, -1,
	M68K_SR, M68K_USP, -1,
	M68K_D0, M68K_A0, -1,
	M68K_D1, M68K_A1, -1,
	M68K_D2, M68K_A2, -1,
	M68K_D3, M68K_A3, -1,
	M68K_D4, M68K_A4, -1,
	M68K_D5, M68K_A5, -1,
	M68K_D6, M68K_A6, -1,
	M68K_D7, M68K_A7, 0
};

static UINT8 m68k_win_layout[] = {
	48, 0,32,13,	/* register window (top right) */
	 0, 0,47,13,	/* disassembler window (top left) */
	 0,14,47, 8,	/* memory #1 window (left, middle) */
	48,14,32, 8,	/* memory #2 window (right, middle) */
	 0,23,80, 1 	/* command line window (bottom rows) */
};

#ifdef A68K0
extern a68k_cpu_context M68000_regs;

extern void CONVENTION M68000_RUN(void);
extern void CONVENTION M68000_RESET(void);

#endif

#ifdef A68K2
extern a68k_cpu_context M68020_regs;

extern void CONVENTION M68020_RUN(void);
extern void CONVENTION M68020_RESET(void);
#endif

/***************************************************************************/
/* Save State stuff                                                        */
/***************************************************************************/

static int IntelFlag[32] = {
	0x0000,0x0001,0x0800,0x0801,0x0040,0x0041,0x0840,0x0841,
    0x0080,0x0081,0x0880,0x0881,0x00C0,0x00C1,0x08C0,0x08C1,
    0x0100,0x0101,0x0900,0x0901,0x0140,0x0141,0x0940,0x0941,
    0x0180,0x0181,0x0980,0x0981,0x01C0,0x01C1,0x09C0,0x09C1
};


// The assembler engine only keeps flags in intel format, so ...

static UINT32 zero = 0;
static int stopped = 0;

static void a68k_prepare_substate(void)
{
	stopped = ((M68000_regs.IRQ_level & 0x80) != 0);

	M68000_regs.sr = ((M68000_regs.ccr >> 4) & 0x1C)
                   | (M68000_regs.ccr & 0x01)
                   | ((M68000_regs.ccr >> 10) & 0x02)
                   | (M68000_regs.sr_high << 8);
}

static void a68k_post_load(void)
{
	int intel = M68000_regs.sr & 0x1f;

    M68000_regs.sr_high = M68000_regs.sr >> 8;
    M68000_regs.x_carry = (IntelFlag[intel] >> 8) & 0x01;
    M68000_regs.ccr     = IntelFlag[intel] & 0x0EFF;
}

void a68k_state_register(const char *type)
{
	int cpu = cpu_getactivecpu();

	state_save_register_UINT32(type, cpu, "D"         , &M68000_regs.d[0], 8);
	state_save_register_UINT32(type, cpu, "A"         , &M68000_regs.a[0], 8);
	state_save_register_UINT32(type, cpu, "PPC"       , &M68000_regs.previous_pc, 1);
	state_save_register_UINT32(type, cpu, "PC"        , &M68000_regs.pc, 1);
	state_save_register_UINT32(type, cpu, "USP"       , &M68000_regs.usp, 1);
	state_save_register_UINT32(type, cpu, "ISP"       , &M68000_regs.isp, 1);
	state_save_register_UINT32(type, cpu, "MSP"       , &zero, 1);
	state_save_register_UINT32(type, cpu, "VBR"       , &M68000_regs.vbr, 1);
	state_save_register_UINT32(type, cpu, "SFC"       , &M68000_regs.sfc, 1);
	state_save_register_UINT32(type, cpu, "DFC"       , &M68000_regs.dfc, 1);
	state_save_register_UINT32(type, cpu, "CACR"      , &zero, 1);
	state_save_register_UINT32(type, cpu, "CAAR"      , &zero, 1);
	state_save_register_UINT16(type, cpu, "SR"        , &M68000_regs.sr, 1);
	state_save_register_UINT32(type, cpu, "INT_LEVEL" , &M68000_regs.IRQ_level, 1);
	state_save_register_UINT32(type, cpu, "INT_CYCLES", (UINT32 *)&m68k_ICount, 1);
	state_save_register_int   (type, cpu, "STOPPED"   , &stopped);
	state_save_register_int   (type, cpu, "HALTED"    , (int *)&zero);
	state_save_register_UINT32(type, cpu, "PREF_ADDR" , &zero, 1);
	state_save_register_UINT32(type, cpu, "PREF_DATA" , &zero, 1);
  	state_save_register_func_presave(a68k_prepare_substate);
  	state_save_register_func_postload(a68k_post_load);
}


static void change_pc_m68k(offs_t pc)
{
	change_pc(pc);
}


#ifdef A68K0

/****************************************************************************
 * 16-bit data memory interface
 ****************************************************************************/

static data32_t readlong_d16(offs_t address)
{
	data32_t result = program_read_word_16be(address) << 16;
	return result | program_read_word_16be(address + 2);
}

static void writelong_d16(offs_t address, data32_t data)
{
	program_write_word_16be(address, data >> 16);
	program_write_word_16be(address + 2, data);
}

/* interface for 24-bit address bus, 16-bit data bus (68000, 68010) */
static const struct m68k_memory_interface interface_d16 =
{
	0,
	program_read_byte_16be,
	program_read_word_16be,
	readlong_d16,
	program_write_byte_16be,
	program_write_word_16be,
	writelong_d16,
	change_pc_m68k,
	program_read_byte_16be,				// Encrypted Versions
	program_read_word_16be,
	readlong_d16,
	program_read_word_16be,
	readlong_d16
};

#endif // A68K0

/****************************************************************************
 * 32-bit data memory interface
 ****************************************************************************/

#ifdef A68K2

/* potentially misaligned 16-bit reads with a 32-bit data bus (and 24-bit address bus) */
static data16_t readword_d32(offs_t address)
{
	data16_t result;

	if (!(address & 1))
		return program_read_word_32be(address);
	result = program_read_byte_32be(address) << 8;
	return result | program_read_byte_32be(address + 1);
}

/* potentially misaligned 16-bit writes with a 32-bit data bus (and 24-bit address bus) */
static void writeword_d32(offs_t address, data16_t data)
{
	if (!(address & 1))
	{
		program_write_word_32be(address, data);
		return;
	}
	program_write_byte_32be(address, data >> 8);
	program_write_byte_32be(address + 1, data);
}

/* potentially misaligned 32-bit reads with a 32-bit data bus (and 24-bit address bus) */
static data32_t readlong_d32(offs_t address)
{
	data32_t result;

	if (!(address & 3))
		return program_read_dword_32be(address);
	else if (!(address & 1))
	{
		result = program_read_word_32be(address) << 16;
		return result | program_read_word_32be(address + 2);
	}
	result = program_read_byte_32be(address) << 24;
	result |= program_read_word_32be(address + 1) << 8;
	return result | program_read_byte_32be(address + 3);
}

/* potentially misaligned 32-bit writes with a 32-bit data bus (and 24-bit address bus) */
static void writelong_d32(offs_t address, data32_t data)
{
	if (!(address & 3))
	{
		program_write_dword_32be(address, data);
		return;
	}
	else if (!(address & 1))
	{
		program_write_word_32be(address, data >> 16);
		program_write_word_32be(address + 2, data);
		return;
	}
	program_write_byte_32be(address, data >> 24);
	program_write_word_32be(address + 1, data >> 8);
	program_write_byte_32be(address + 3, data);
}

/* interface for 32-bit data bus (68EC020, 68020) */
static const struct m68k_memory_interface interface_d32 =
{
	WORD_XOR_BE(0),
	program_read_byte_32be,
	readword_d32,
	readlong_d32,
	program_write_byte_32be,
	writeword_d32,
	writelong_d32,
	change_pc_m68k,
	program_read_byte_32be,				// Encrypted Versions
	program_read_word_32be,
	readlong_d32,
	program_read_word_32be,
	readlong_d32
};


/* global access */
struct m68k_memory_interface m68k_memory_intf;

#endif // A68K2


/********************************************/
/* Interface routines to link Mame -> 68KEM */
/********************************************/

#define READOP(a)	(cpu_readop16((a) ^ a68k_memory_intf.opcode_xor))

#ifdef A68K0

static void m68000_init(void)
{
	a68k_state_register("m68000");
	M68000_regs.reset_callback = 0;
}

static void m68k16_reset_common(void)
{
	int (*rc)(void);

	rc = M68000_regs.reset_callback;
	memset(&M68000_regs,0,sizeof(M68000_regs));
	M68000_regs.reset_callback = rc;

    M68000_regs.a[7] = M68000_regs.isp = (( READOP(0) << 16 ) | READOP(2));
    M68000_regs.pc   = (( READOP(4) << 16 ) | READOP(6)) & 0xffffff;
    M68000_regs.sr_high = 0x27;

	#ifdef MAME_DEBUG
		M68000_regs.sr = 0x2700;
	#endif

    M68000_RESET();
}

static void m68000_reset(void *param)
{
	struct m68k_encryption_interface *interface = param;

    // Default Memory Routines
	a68k_memory_intf = interface_d16;
	mem_amask = address_space[ADDRESS_SPACE_PROGRAM].addrmask;

	// Import encryption routines if present
	if (param)
	{
		a68k_memory_intf.read8pc = interface->read8pc;
		a68k_memory_intf.read16pc = interface->read16pc;
		a68k_memory_intf.read32pc = interface->read32pc;
		a68k_memory_intf.read16d = interface->read16d;
		a68k_memory_intf.read32d = interface->read32d;
	}

	m68k16_reset_common();
    M68000_regs.Memory_Interface = a68k_memory_intf;
}

static void m68000_exit(void)
{
	/* nothing to do ? */
}


#ifdef TRACE68K 							/* Trace */
	static int skiptrace=0;
#endif

static int m68000_execute(int cycles)
{
	if (M68000_regs.IRQ_level == 0x80) return cycles;		/* STOP with no IRQs */

	m68k_ICount = cycles;

#ifdef MAME_DEBUG
    do
    {
		if (mame_debug)
        {
			#ifdef TRACE68K

			int StartCycle = m68k_ICount;

            skiptrace++;

            if (skiptrace > 0)
            {
			    int mycount, areg, dreg;

                areg = dreg = 0;
	            for (mycount=7;mycount>=0;mycount--)
                {
            	    areg = areg + M68000_regs.a[mycount];
                    dreg = dreg + M68000_regs.d[mycount];
                }

           	    logerror("=> %8x %8x ",areg,dreg);
			    logerror("%6x %4x %d\n",M68000_regs.pc,M68000_regs.sr & 0x271F,m68k_ICount);
            }
            #endif

//	        m68k_memory_intf = a68k_memory_intf;
			MAME_Debug();
            M68000_RUN();

            #ifdef TRACE68K
            if ((M68000_regs.IRQ_level & 0x80) || (cpunum_is_suspended(cpunum, SUSPEND_REASON_HALT | SUSPEND_REASON_RESET | SUSPEND_REASON_DISABLE)))
    			m68k_ICount = 0;
            else
				m68k_ICount = StartCycle - 12;
            #endif
        }
        else
			M68000_RUN();

    } while (m68k_ICount > 0);

#else

	M68000_RUN();

#endif /* MAME_DEBUG */

	return (cycles - m68k_ICount);
}


static void m68000_get_context(void *dst)
{
	if( dst )
		*(a68k_cpu_context*)dst = M68000_regs;
}

static void m68000_set_context(void *src)
{
	if( src )
	{
		M68000_regs = *(a68k_cpu_context*)src;
        a68k_memory_intf = M68000_regs.Memory_Interface;
        mem_amask = address_space[ADDRESS_SPACE_PROGRAM].addrmask;
    }
}

static void m68k_assert_irq(int int_line)
{
	/* Save icount */
	int StartCount = m68k_ICount;

	M68000_regs.IRQ_level = int_line;

    /* Now check for Interrupt */

	m68k_ICount = -1;
    M68000_RUN();

    /* Restore Count */
	m68k_ICount = StartCount;
}

static void m68k_clear_irq(int int_line)
{
	M68000_regs.IRQ_level = 0;
}

static void m68000_set_irq_line(int irqline, int state)
{
	if (irqline == INPUT_LINE_NMI)
		irqline = 7;
	switch(state)
	{
		case CLEAR_LINE:
			m68k_clear_irq(irqline);
			return;
		case ASSERT_LINE:
			m68k_assert_irq(irqline);
			return;
		default:
			m68k_assert_irq(irqline);
			return;
	}
}

void m68000_set_reset_callback(int (*callback)(void))
{
	M68000_regs.reset_callback = callback;
}

static offs_t m68000_dasm(char *buffer, offs_t pc)
{
	A68K_SET_PC_CALLBACK(pc);

#ifdef MAME_DEBUG
	m68k_memory_intf = a68k_memory_intf;
	return m68k_disassemble(buffer, pc, M68K_CPU_TYPE_68000);
#else
	sprintf(buffer, "$%04X", cpu_readop16(pc) );
	return 2;
#endif
}

/****************************************************************************
 * M68010 section
 ****************************************************************************/

#if (HAS_M68010)

void m68010_reset(void *param)
{
	a68k_memory_intf = interface_d16;
	mem_amask = address_space[ADDRESS_SPACE_PROGRAM].addrmask;

	m68k16_reset_common();

    M68000_regs.CPUtype=1;
    M68000_regs.Memory_Interface = a68k_memory_intf;
}

static offs_t m68010_dasm(char *buffer, offs_t pc)
{
	A68K_SET_PC_CALLBACK(pc);

#ifdef MAME_DEBUG
	m68k_memory_intf = a68k_memory_intf;
	return m68k_disassemble(buffer, pc, M68K_CPU_TYPE_68010);
#else
	sprintf(buffer, "$%04X", cpu_readop16(pc) );
	return 2;
#endif
}
#endif


#endif // A68K0


/****************************************************************************
 * M68020 section
 ****************************************************************************/

#ifdef A68K2

static void m68020_init(void)
{
	a68k_state_register("m68020");
	M68020_regs.reset_callback = 0;
}

static void m68k32_reset_common(void)
{
	int (*rc)(void);

	rc = M68020_regs.reset_callback;
	memset(&M68020_regs,0,sizeof(M68020_regs));
	M68020_regs.reset_callback = rc;

    M68020_regs.a[7] = M68020_regs.isp = (( READOP(0) << 16 ) | READOP(2));
    M68020_regs.pc   = (( READOP(4) << 16 ) | READOP(6)) & 0xffffff;
    M68020_regs.sr_high = 0x27;

	#ifdef MAME_DEBUG
		M68020_regs.sr = 0x2700;
	#endif

    M68020_RESET();
}

#if (HAS_M68020)

static void m68020_reset(void *param)
{
	a68k_memory_intf = interface_d32;
	mem_amask = address_space[ADDRESS_SPACE_PROGRAM].addrmask;

	m68k32_reset_common();

    M68020_regs.CPUtype=2;
    M68020_regs.Memory_Interface = a68k_memory_intf;
}

static void m68020_exit(void)
{
	/* nothing to do ? */
}

static int m68020_execute(int cycles)
{
	if (M68020_regs.IRQ_level == 0x80) return cycles;		/* STOP with no IRQs */

	m68k_ICount = cycles;

#ifdef MAME_DEBUG
    do
    {
		if (mame_debug)
        {
			#ifdef TRACE68K

			int StartCycle = m68k_ICount;

            skiptrace++;

            if (skiptrace > 0)
            {
			    int mycount, areg, dreg;

                areg = dreg = 0;
	            for (mycount=7;mycount>=0;mycount--)
                {
            	    areg = areg + M68020_regs.a[mycount];
                    dreg = dreg + M68020_regs.d[mycount];
                }

           	    logerror("=> %8x %8x ",areg,dreg);
			    logerror("%6x %4x %d\n",M68020_regs.pc,M68020_regs.sr & 0x271F,m68k_ICount);
            }
            #endif

//	        m68k_memory_intf = a68k_memory_intf;
			MAME_Debug();
            M68020_RUN();

            #ifdef TRACE68K
            if ((M68020_regs.IRQ_level & 0x80) || (cpunum_is_suspended(cpunum, SUSPEND_REASON_HALT | SUSPEND_REASON_RESET | SUSPEND_REASON_DISABLE)))
    			m68k_ICount = 0;
            else
				m68k_ICount = StartCycle - 12;
            #endif
        }
        else
			M68020_RUN();

    } while (m68k_ICount > 0);

#else

	M68020_RUN();

#endif /* MAME_DEBUG */

	return (cycles - m68k_ICount);
}

static void m68020_get_context(void *dst)
{
	if( dst )
		*(a68k_cpu_context*)dst = M68020_regs;
}

static void m68020_set_context(void *src)
{
	if( src )
    {
		M68020_regs = *(a68k_cpu_context*)src;
        a68k_memory_intf = M68020_regs.Memory_Interface;
		mem_amask = address_space[ADDRESS_SPACE_PROGRAM].addrmask;
    }
}

static void m68020_assert_irq(int int_line)
{
	/* Save icount */
	int StartCount = m68k_ICount;

	M68020_regs.IRQ_level = int_line;

    /* Now check for Interrupt */

	m68k_ICount = -1;
    M68020_RUN();

    /* Restore Count */
	m68k_ICount = StartCount;
}

static void m68020_clear_irq(int int_line)
{
	M68020_regs.IRQ_level = 0;
}

static void m68020_set_irq_line(int irqline, int state)
{
	if (irqline == INPUT_LINE_NMI)
		irqline = 7;
	switch(state)
	{
		case CLEAR_LINE:
			m68020_clear_irq(irqline);
			return;
		case ASSERT_LINE:
			m68020_assert_irq(irqline);
			return;
		default:
			m68020_assert_irq(irqline);
			return;
	}
}

void m68020_set_reset_callback(int (*callback)(void))
{
	M68020_regs.reset_callback = callback;
}

static offs_t m68020_dasm(char *buffer, offs_t pc)
{
	A68K_SET_PC_CALLBACK(pc);

#ifdef MAME_DEBUG
	m68k_memory_intf = a68k_memory_intf;
	return m68k_disassemble(buffer, pc, M68K_CPU_TYPE_68020);
#else
	sprintf(buffer, "$%04X", cpu_readop16(pc) );
	return 2;
#endif
}
#endif

#if (HAS_M68EC020)

static void m68ec020_reset(void *param)
{
	a68k_memory_intf = interface_d32;
	mem_amask = address_space[ADDRESS_SPACE_PROGRAM].addrmask;

	m68k32_reset_common();

    M68020_regs.CPUtype=2;
    M68020_regs.Memory_Interface = a68k_memory_intf;
}

static offs_t m68ec020_dasm(char *buffer, unsigned pc)
{
	A68K_SET_PC_CALLBACK(pc);

#ifdef MAME_DEBUG
	m68k_memory_intf = a68k_memory_intf;
	return m68k_disassemble(buffer, pc, M68K_CPU_TYPE_68EC020);
#else
	sprintf(buffer, "$%04X", cpu_readop16(pc) );
	return 2;
#endif
}

#endif

#endif // A68K2



/**************************************************************************
 * Generic set_info
 **************************************************************************/

static void m68000_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_STATE + 0:			m68000_set_irq_line(0, info->i);			break;
		case CPUINFO_INT_INPUT_STATE + 1:			m68000_set_irq_line(1, info->i);			break;
		case CPUINFO_INT_INPUT_STATE + 2:			m68000_set_irq_line(2, info->i);			break;
		case CPUINFO_INT_INPUT_STATE + 3:			m68000_set_irq_line(3, info->i);			break;
		case CPUINFO_INT_INPUT_STATE + 4:			m68000_set_irq_line(4, info->i);			break;
		case CPUINFO_INT_INPUT_STATE + 5:			m68000_set_irq_line(5, info->i);			break;
		case CPUINFO_INT_INPUT_STATE + 6:			m68000_set_irq_line(6, info->i);			break;
		case CPUINFO_INT_INPUT_STATE + 7:			m68000_set_irq_line(7, info->i);			break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + M68K_PC:  		M68000_regs.pc = info->i;					break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + M68K_ISP:		M68000_regs.isp = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_USP:		M68000_regs.usp = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_SR:		M68000_regs.sr = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_VBR:		M68000_regs.vbr = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_SFC:		M68000_regs.sfc = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_DFC:		M68000_regs.dfc = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_D0:		M68000_regs.d[0] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_D1:		M68000_regs.d[1] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_D2:		M68000_regs.d[2] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_D3:		M68000_regs.d[3] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_D4:		M68000_regs.d[4] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_D5:		M68000_regs.d[5] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_D6:		M68000_regs.d[6] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_D7:		M68000_regs.d[7] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_A0:		M68000_regs.a[0] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_A1:		M68000_regs.a[1] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_A2:		M68000_regs.a[2] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_A3:		M68000_regs.a[3] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_A4:		M68000_regs.a[4] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_A5:		M68000_regs.a[5] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_A6:		M68000_regs.a[6] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_A7:		M68000_regs.a[7] = info->i;					break;
		
		/* --- the following bits of info are set as pointers to data or functions --- */
		case CPUINFO_PTR_IRQ_CALLBACK:				M68000_regs.irq_callback = info->irqcallback; break;
	}
}



#ifdef A68K0

/**************************************************************************
 * Generic get_info
 **************************************************************************/

void m68000_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(M68000_regs);			break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 8;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = -1;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 2;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 10;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 4;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 158;							break;
		
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 16;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 24;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE + 0:				info->i = 0;	/* fix me */			break;
		case CPUINFO_INT_INPUT_STATE + 1:				info->i = 0;	/* fix me */			break;
		case CPUINFO_INT_INPUT_STATE + 2:				info->i = 0;	/* fix me */			break;
		case CPUINFO_INT_INPUT_STATE + 3:				info->i = 0;	/* fix me */			break;
		case CPUINFO_INT_INPUT_STATE + 4:				info->i = 0;	/* fix me */			break;
		case CPUINFO_INT_INPUT_STATE + 5:				info->i = 0;	/* fix me */			break;
		case CPUINFO_INT_INPUT_STATE + 6:				info->i = 0;	/* fix me */			break;
		case CPUINFO_INT_INPUT_STATE + 7:				info->i = 0;	/* fix me */			break;

		case CPUINFO_INT_PREVIOUSPC:					info->i = M68000_regs.previous_pc;		break;

    	case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + M68K_PC:			info->i = M68000_regs.pc;				break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + M68K_ISP:			info->i = M68000_regs.isp;				break;
		case CPUINFO_INT_REGISTER + M68K_USP:			info->i = M68000_regs.usp;				break;
		case CPUINFO_INT_REGISTER + M68K_SR:			info->i = M68000_regs.sr;				break;
		case CPUINFO_INT_REGISTER + M68K_VBR:			info->i = M68000_regs.vbr;				break;
		case CPUINFO_INT_REGISTER + M68K_SFC:			info->i = M68000_regs.sfc;				break;
		case CPUINFO_INT_REGISTER + M68K_DFC:			info->i = M68000_regs.dfc;				break;
		case CPUINFO_INT_REGISTER + M68K_D0:			info->i = M68000_regs.d[0];				break;
		case CPUINFO_INT_REGISTER + M68K_D1:			info->i = M68000_regs.d[1];				break;
		case CPUINFO_INT_REGISTER + M68K_D2:			info->i = M68000_regs.d[2];				break;
		case CPUINFO_INT_REGISTER + M68K_D3:			info->i = M68000_regs.d[3];				break;
		case CPUINFO_INT_REGISTER + M68K_D4:			info->i = M68000_regs.d[4];				break;
		case CPUINFO_INT_REGISTER + M68K_D5:			info->i = M68000_regs.d[5];				break;
		case CPUINFO_INT_REGISTER + M68K_D6:			info->i = M68000_regs.d[6];				break;
		case CPUINFO_INT_REGISTER + M68K_D7:			info->i = M68000_regs.d[7];				break;
		case CPUINFO_INT_REGISTER + M68K_A0:			info->i = M68000_regs.a[0];				break;
		case CPUINFO_INT_REGISTER + M68K_A1:			info->i = M68000_regs.a[1];				break;
		case CPUINFO_INT_REGISTER + M68K_A2:			info->i = M68000_regs.a[2];				break;
		case CPUINFO_INT_REGISTER + M68K_A3:			info->i = M68000_regs.a[3];				break;
		case CPUINFO_INT_REGISTER + M68K_A4:			info->i = M68000_regs.a[4];				break;
		case CPUINFO_INT_REGISTER + M68K_A5:			info->i = M68000_regs.a[5];				break;
		case CPUINFO_INT_REGISTER + M68K_A6:			info->i = M68000_regs.a[6];				break;
		case CPUINFO_INT_REGISTER + M68K_A7:			info->i = M68000_regs.a[7];				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = m68000_set_info;		break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = m68000_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = m68000_set_context;	break;
		case CPUINFO_PTR_INIT:							info->init = m68000_init;				break;
		case CPUINFO_PTR_RESET:							info->reset = m68000_reset;				break;
		case CPUINFO_PTR_EXIT:							info->exit = m68000_exit;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = m68000_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = m68000_dasm;		break;
		case CPUINFO_PTR_IRQ_CALLBACK:					/* fix me */							break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &m68k_ICount;			break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = M68K_layout;					break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = m68k_win_layout;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "68000"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Motorola 68K"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "0.16"); break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright 1998,99 Mike Coates, Darren Olafson. All rights reserved"); break;

		case CPUINFO_STR_FLAGS:
			sprintf(info->s = cpuintrf_temp_str(), "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				M68000_regs.sr & 0x8000 ? 'T':'.',
				M68000_regs.sr & 0x4000 ? '?':'.',
				M68000_regs.sr & 0x2000 ? 'S':'.',
				M68000_regs.sr & 0x1000 ? '?':'.',
				M68000_regs.sr & 0x0800 ? '?':'.',
				M68000_regs.sr & 0x0400 ? 'I':'.',
				M68000_regs.sr & 0x0200 ? 'I':'.',
				M68000_regs.sr & 0x0100 ? 'I':'.',
				M68000_regs.sr & 0x0080 ? '?':'.',
				M68000_regs.sr & 0x0040 ? '?':'.',
				M68000_regs.sr & 0x0020 ? '?':'.',
				M68000_regs.sr & 0x0010 ? 'X':'.',
				M68000_regs.sr & 0x0008 ? 'N':'.',
				M68000_regs.sr & 0x0004 ? 'Z':'.',
				M68000_regs.sr & 0x0002 ? 'V':'.',
				M68000_regs.sr & 0x0001 ? 'C':'.');
			break;

		case CPUINFO_STR_REGISTER + M68K_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC:%06X", M68000_regs.pc); break;
		case CPUINFO_STR_REGISTER + M68K_ISP:			sprintf(info->s = cpuintrf_temp_str(), "ISP:%08X", M68000_regs.isp); break;
		case CPUINFO_STR_REGISTER + M68K_USP:			sprintf(info->s = cpuintrf_temp_str(), "USP:%08X", M68000_regs.usp); break;
		case CPUINFO_STR_REGISTER + M68K_SR:			sprintf(info->s = cpuintrf_temp_str(), "SR:%08X", M68000_regs.sr); break;
		case CPUINFO_STR_REGISTER + M68K_VBR:			sprintf(info->s = cpuintrf_temp_str(), "VBR:%08X", M68000_regs.vbr); break;
		case CPUINFO_STR_REGISTER + M68K_SFC:			sprintf(info->s = cpuintrf_temp_str(), "SFC:%08X", M68000_regs.sfc); break;
		case CPUINFO_STR_REGISTER + M68K_DFC:			sprintf(info->s = cpuintrf_temp_str(), "DFC:%08X", M68000_regs.dfc); break;
		case CPUINFO_STR_REGISTER + M68K_D0:			sprintf(info->s = cpuintrf_temp_str(), "D0:%08X", M68000_regs.d[0]); break;
		case CPUINFO_STR_REGISTER + M68K_D1:			sprintf(info->s = cpuintrf_temp_str(), "D1:%08X", M68000_regs.d[1]); break;
		case CPUINFO_STR_REGISTER + M68K_D2:			sprintf(info->s = cpuintrf_temp_str(), "D2:%08X", M68000_regs.d[2]); break;
		case CPUINFO_STR_REGISTER + M68K_D3:			sprintf(info->s = cpuintrf_temp_str(), "D3:%08X", M68000_regs.d[3]); break;
		case CPUINFO_STR_REGISTER + M68K_D4:			sprintf(info->s = cpuintrf_temp_str(), "D4:%08X", M68000_regs.d[4]); break;
		case CPUINFO_STR_REGISTER + M68K_D5:			sprintf(info->s = cpuintrf_temp_str(), "D5:%08X", M68000_regs.d[5]); break;
		case CPUINFO_STR_REGISTER + M68K_D6:			sprintf(info->s = cpuintrf_temp_str(), "D6:%08X", M68000_regs.d[6]); break;
		case CPUINFO_STR_REGISTER + M68K_D7:			sprintf(info->s = cpuintrf_temp_str(), "D7:%08X", M68000_regs.d[7]); break;
		case CPUINFO_STR_REGISTER + M68K_A0:			sprintf(info->s = cpuintrf_temp_str(), "A0:%08X", M68000_regs.a[0]); break;
		case CPUINFO_STR_REGISTER + M68K_A1:			sprintf(info->s = cpuintrf_temp_str(), "A1:%08X", M68000_regs.a[1]); break;
		case CPUINFO_STR_REGISTER + M68K_A2:			sprintf(info->s = cpuintrf_temp_str(), "A2:%08X", M68000_regs.a[2]); break;
		case CPUINFO_STR_REGISTER + M68K_A3:			sprintf(info->s = cpuintrf_temp_str(), "A3:%08X", M68000_regs.a[3]); break;
		case CPUINFO_STR_REGISTER + M68K_A4:			sprintf(info->s = cpuintrf_temp_str(), "A4:%08X", M68000_regs.a[4]); break;
		case CPUINFO_STR_REGISTER + M68K_A5:			sprintf(info->s = cpuintrf_temp_str(), "A5:%08X", M68000_regs.a[5]); break;
		case CPUINFO_STR_REGISTER + M68K_A6:			sprintf(info->s = cpuintrf_temp_str(), "A6:%08X", M68000_regs.a[6]); break;
		case CPUINFO_STR_REGISTER + M68K_A7:			sprintf(info->s = cpuintrf_temp_str(), "A7:%08X", M68000_regs.a[7]); break;
	}
}


#if (HAS_M68010)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

void m68010_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = m68010_reset;				break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = m68010_dasm;		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "68010"); break;

		default:
			m68000_get_info(state, info);
			break;
	}
}
#endif

#endif


#ifdef A68K2

/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

static void m68020_set_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are set as 64-bit signed integers --- */
		case CPUINFO_INT_INPUT_STATE + 0:			m68020_set_irq_line(0, info->i);			break;
		case CPUINFO_INT_INPUT_STATE + 1:			m68020_set_irq_line(1, info->i);			break;
		case CPUINFO_INT_INPUT_STATE + 2:			m68020_set_irq_line(2, info->i);			break;
		case CPUINFO_INT_INPUT_STATE + 3:			m68020_set_irq_line(3, info->i);			break;
		case CPUINFO_INT_INPUT_STATE + 4:			m68020_set_irq_line(4, info->i);			break;
		case CPUINFO_INT_INPUT_STATE + 5:			m68020_set_irq_line(5, info->i);			break;
		case CPUINFO_INT_INPUT_STATE + 6:			m68020_set_irq_line(6, info->i);			break;
		case CPUINFO_INT_INPUT_STATE + 7:			m68020_set_irq_line(7, info->i);			break;

		case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + M68K_PC:  		M68020_regs.pc = info->i;					break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + M68K_ISP:		M68020_regs.isp = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_USP:		M68020_regs.usp = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_SR:		M68020_regs.sr = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_VBR:		M68020_regs.vbr = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_SFC:		M68020_regs.sfc = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_DFC:		M68020_regs.dfc = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_D0:		M68020_regs.d[0] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_D1:		M68020_regs.d[1] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_D2:		M68020_regs.d[2] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_D3:		M68020_regs.d[3] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_D4:		M68020_regs.d[4] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_D5:		M68020_regs.d[5] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_D6:		M68020_regs.d[6] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_D7:		M68020_regs.d[7] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_A0:		M68020_regs.a[0] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_A1:		M68020_regs.a[1] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_A2:		M68020_regs.a[2] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_A3:		M68020_regs.a[3] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_A4:		M68020_regs.a[4] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_A5:		M68020_regs.a[5] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_A6:		M68020_regs.a[6] = info->i;					break;
		case CPUINFO_INT_REGISTER + M68K_A7:		M68020_regs.a[7] = info->i;					break;
		
		/* --- the following bits of info are set as pointers to data or functions --- */
		case CPUINFO_PTR_IRQ_CALLBACK:				M68020_regs.irq_callback = info->irqcallback; break;
	}
}

void m68020_get_info(UINT32 state, union cpuinfo *info)
{
	int sr;

	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_CONTEXT_SIZE:					info->i = sizeof(M68020_regs);			break;
		case CPUINFO_INT_INPUT_LINES:					info->i = 8;							break;
		case CPUINFO_INT_DEFAULT_IRQ_VECTOR:			info->i = -1;							break;
		case CPUINFO_INT_ENDIANNESS:					info->i = CPU_IS_BE;					break;
		case CPUINFO_INT_CLOCK_DIVIDER:					info->i = 1;							break;
		case CPUINFO_INT_MIN_INSTRUCTION_BYTES:			info->i = 2;							break;
		case CPUINFO_INT_MAX_INSTRUCTION_BYTES:			info->i = 10;							break;
		case CPUINFO_INT_MIN_CYCLES:					info->i = 4;							break;
		case CPUINFO_INT_MAX_CYCLES:					info->i = 158;							break;
		
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_PROGRAM:	info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 32;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_PROGRAM: info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_DATA:	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_DATA: 	info->i = 0;					break;
		case CPUINFO_INT_DATABUS_WIDTH + ADDRESS_SPACE_IO:		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_IO: 		info->i = 0;					break;
		case CPUINFO_INT_ADDRBUS_SHIFT + ADDRESS_SPACE_IO: 		info->i = 0;					break;

		case CPUINFO_INT_INPUT_STATE + 0:				info->i = 0;	/* fix me */			break;
		case CPUINFO_INT_INPUT_STATE + 1:				info->i = 1;	/* fix me */			break;
		case CPUINFO_INT_INPUT_STATE + 2:				info->i = 2;	/* fix me */			break;
		case CPUINFO_INT_INPUT_STATE + 3:				info->i = 3;	/* fix me */			break;
		case CPUINFO_INT_INPUT_STATE + 4:				info->i = 4;	/* fix me */			break;
		case CPUINFO_INT_INPUT_STATE + 5:				info->i = 5;	/* fix me */			break;
		case CPUINFO_INT_INPUT_STATE + 6:				info->i = 6;	/* fix me */			break;
		case CPUINFO_INT_INPUT_STATE + 7:				info->i = 7;	/* fix me */			break;

		case CPUINFO_INT_PREVIOUSPC:					info->i = M68000_regs.previous_pc;		break;

    	case CPUINFO_INT_PC:
		case CPUINFO_INT_REGISTER + M68K_PC:			info->i = M68020_regs.pc;				break;
		case CPUINFO_INT_SP:
		case CPUINFO_INT_REGISTER + M68K_ISP:			info->i = M68020_regs.isp;				break;
		case CPUINFO_INT_REGISTER + M68K_USP:			info->i = M68020_regs.usp;				break;
		case CPUINFO_INT_REGISTER + M68K_SR:			info->i = M68020_regs.sr;				break;
		case CPUINFO_INT_REGISTER + M68K_VBR:			info->i = M68020_regs.vbr;				break;
		case CPUINFO_INT_REGISTER + M68K_SFC:			info->i = M68020_regs.sfc;				break;
		case CPUINFO_INT_REGISTER + M68K_DFC:			info->i = M68020_regs.dfc;				break;
		case CPUINFO_INT_REGISTER + M68K_D0:			info->i = M68020_regs.d[0];				break;
		case CPUINFO_INT_REGISTER + M68K_D1:			info->i = M68020_regs.d[1];				break;
		case CPUINFO_INT_REGISTER + M68K_D2:			info->i = M68020_regs.d[2];				break;
		case CPUINFO_INT_REGISTER + M68K_D3:			info->i = M68020_regs.d[3];				break;
		case CPUINFO_INT_REGISTER + M68K_D4:			info->i = M68020_regs.d[4];				break;
		case CPUINFO_INT_REGISTER + M68K_D5:			info->i = M68020_regs.d[5];				break;
		case CPUINFO_INT_REGISTER + M68K_D6:			info->i = M68020_regs.d[6];				break;
		case CPUINFO_INT_REGISTER + M68K_D7:			info->i = M68020_regs.d[7];				break;
		case CPUINFO_INT_REGISTER + M68K_A0:			info->i = M68020_regs.a[0];				break;
		case CPUINFO_INT_REGISTER + M68K_A1:			info->i = M68020_regs.a[1];				break;
		case CPUINFO_INT_REGISTER + M68K_A2:			info->i = M68020_regs.a[2];				break;
		case CPUINFO_INT_REGISTER + M68K_A3:			info->i = M68020_regs.a[3];				break;
		case CPUINFO_INT_REGISTER + M68K_A4:			info->i = M68020_regs.a[4];				break;
		case CPUINFO_INT_REGISTER + M68K_A5:			info->i = M68020_regs.a[5];				break;
		case CPUINFO_INT_REGISTER + M68K_A6:			info->i = M68020_regs.a[6];				break;
		case CPUINFO_INT_REGISTER + M68K_A7:			info->i = M68020_regs.a[7];				break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_SET_INFO:						info->setinfo = m68020_set_info;		break;
		case CPUINFO_PTR_GET_CONTEXT:					info->getcontext = m68020_get_context;	break;
		case CPUINFO_PTR_SET_CONTEXT:					info->setcontext = m68020_set_context;	break;
		case CPUINFO_PTR_INIT:							info->init = m68020_init;				break;
		case CPUINFO_PTR_RESET:							info->reset = m68020_reset;				break;
		case CPUINFO_PTR_EXIT:							info->exit = m68020_exit;				break;
		case CPUINFO_PTR_EXECUTE:						info->execute = m68020_execute;			break;
		case CPUINFO_PTR_BURN:							info->burn = NULL;						break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = m68020_dasm;		break;
		case CPUINFO_PTR_IRQ_CALLBACK:					/* fix me */							break;
		case CPUINFO_PTR_INSTRUCTION_COUNTER:			info->icount = &m68k_ICount;			break;
		case CPUINFO_PTR_REGISTER_LAYOUT:				info->p = M68K_layout;					break;
		case CPUINFO_PTR_WINDOW_LAYOUT:					info->p = m68k_win_layout;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "68020"); break;
		case CPUINFO_STR_CORE_FAMILY:					strcpy(info->s = cpuintrf_temp_str(), "Motorola 68K"); break;
		case CPUINFO_STR_CORE_VERSION:					strcpy(info->s = cpuintrf_temp_str(), "0.16"); break;
		case CPUINFO_STR_CORE_FILE:						strcpy(info->s = cpuintrf_temp_str(), __FILE__); break;
		case CPUINFO_STR_CORE_CREDITS:					strcpy(info->s = cpuintrf_temp_str(), "Copyright 1998,99 Mike Coates, Darren Olafson. All rights reserved"); break;

		case CPUINFO_STR_FLAGS:
			sprintf(info->s = cpuintrf_temp_str(), "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
				M68020_regs.sr & 0x8000 ? 'T':'.',
				M68020_regs.sr & 0x4000 ? '?':'.',
				M68020_regs.sr & 0x2000 ? 'S':'.',
				M68020_regs.sr & 0x1000 ? '?':'.',
				M68020_regs.sr & 0x0800 ? '?':'.',
				M68020_regs.sr & 0x0400 ? 'I':'.',
				M68020_regs.sr & 0x0200 ? 'I':'.',
				M68020_regs.sr & 0x0100 ? 'I':'.',
				M68020_regs.sr & 0x0080 ? '?':'.',
				M68020_regs.sr & 0x0040 ? '?':'.',
				M68020_regs.sr & 0x0020 ? '?':'.',
				M68020_regs.sr & 0x0010 ? 'X':'.',
				M68020_regs.sr & 0x0008 ? 'N':'.',
				M68020_regs.sr & 0x0004 ? 'Z':'.',
				M68020_regs.sr & 0x0002 ? 'V':'.',
				M68020_regs.sr & 0x0001 ? 'C':'.');
			break;

		case CPUINFO_STR_REGISTER + M68K_PC:			sprintf(info->s = cpuintrf_temp_str(), "PC:%06X", M68020_regs.pc); break;
		case CPUINFO_STR_REGISTER + M68K_ISP:			sprintf(info->s = cpuintrf_temp_str(), "ISP:%08X", M68020_regs.isp); break;
		case CPUINFO_STR_REGISTER + M68K_USP:			sprintf(info->s = cpuintrf_temp_str(), "USP:%08X", M68020_regs.usp); break;
		case CPUINFO_STR_REGISTER + M68K_SR:			sprintf(info->s = cpuintrf_temp_str(), "SR:%08X", M68020_regs.sr); break;
		case CPUINFO_STR_REGISTER + M68K_VBR:			sprintf(info->s = cpuintrf_temp_str(), "VBR:%08X", M68020_regs.vbr); break;
		case CPUINFO_STR_REGISTER + M68K_SFC:			sprintf(info->s = cpuintrf_temp_str(), "SFC:%08X", M68020_regs.sfc); break;
		case CPUINFO_STR_REGISTER + M68K_DFC:			sprintf(info->s = cpuintrf_temp_str(), "DFC:%08X", M68020_regs.dfc); break;
		case CPUINFO_STR_REGISTER + M68K_D0:			sprintf(info->s = cpuintrf_temp_str(), "D0:%08X", M68020_regs.d[0]); break;
		case CPUINFO_STR_REGISTER + M68K_D1:			sprintf(info->s = cpuintrf_temp_str(), "D1:%08X", M68020_regs.d[1]); break;
		case CPUINFO_STR_REGISTER + M68K_D2:			sprintf(info->s = cpuintrf_temp_str(), "D2:%08X", M68020_regs.d[2]); break;
		case CPUINFO_STR_REGISTER + M68K_D3:			sprintf(info->s = cpuintrf_temp_str(), "D3:%08X", M68020_regs.d[3]); break;
		case CPUINFO_STR_REGISTER + M68K_D4:			sprintf(info->s = cpuintrf_temp_str(), "D4:%08X", M68020_regs.d[4]); break;
		case CPUINFO_STR_REGISTER + M68K_D5:			sprintf(info->s = cpuintrf_temp_str(), "D5:%08X", M68020_regs.d[5]); break;
		case CPUINFO_STR_REGISTER + M68K_D6:			sprintf(info->s = cpuintrf_temp_str(), "D6:%08X", M68020_regs.d[6]); break;
		case CPUINFO_STR_REGISTER + M68K_D7:			sprintf(info->s = cpuintrf_temp_str(), "D7:%08X", M68020_regs.d[7]); break;
		case CPUINFO_STR_REGISTER + M68K_A0:			sprintf(info->s = cpuintrf_temp_str(), "A0:%08X", M68020_regs.a[0]); break;
		case CPUINFO_STR_REGISTER + M68K_A1:			sprintf(info->s = cpuintrf_temp_str(), "A1:%08X", M68020_regs.a[1]); break;
		case CPUINFO_STR_REGISTER + M68K_A2:			sprintf(info->s = cpuintrf_temp_str(), "A2:%08X", M68020_regs.a[2]); break;
		case CPUINFO_STR_REGISTER + M68K_A3:			sprintf(info->s = cpuintrf_temp_str(), "A3:%08X", M68020_regs.a[3]); break;
		case CPUINFO_STR_REGISTER + M68K_A4:			sprintf(info->s = cpuintrf_temp_str(), "A4:%08X", M68020_regs.a[4]); break;
		case CPUINFO_STR_REGISTER + M68K_A5:			sprintf(info->s = cpuintrf_temp_str(), "A5:%08X", M68020_regs.a[5]); break;
		case CPUINFO_STR_REGISTER + M68K_A6:			sprintf(info->s = cpuintrf_temp_str(), "A6:%08X", M68020_regs.a[6]); break;
		case CPUINFO_STR_REGISTER + M68K_A7:			sprintf(info->s = cpuintrf_temp_str(), "A7:%08X", M68020_regs.a[7]); break;
	}
}


#if (HAS_M68EC020)
/**************************************************************************
 * CPU-specific set_info
 **************************************************************************/

void m68ec020_get_info(UINT32 state, union cpuinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case CPUINFO_INT_ADDRBUS_WIDTH + ADDRESS_SPACE_PROGRAM: info->i = 24;					break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case CPUINFO_PTR_RESET:							info->reset = m68ec020_reset;			break;
		case CPUINFO_PTR_DISASSEMBLE:					info->disassemble = m68ec020_dasm;		break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case CPUINFO_STR_NAME:							strcpy(info->s = cpuintrf_temp_str(), "68EC020"); break;

		default:
			m68020_get_info(state, info);
			break;
	}
}
#endif

#endif
