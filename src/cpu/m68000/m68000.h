#ifndef M68000__HEADER
#define M68000__HEADER

/* ======================================================================== */
/* ========================= LICENSING & COPYRIGHT ======================== */
/* ======================================================================== */
/*
 *                                  MUSASHI
 *                                Version 1.1
 *
 * A portable Motorola M680x0 processor emulation engine.
 * Copyright 1999 Karl Stenerud.  All rights reserved.
 *
 * This code may be freely used for non-commercial purpooses as long as this
 * copyright notice remains unaltered in the source code and any binary files
 * containing this code in compiled form.
 *
 * Any commercial ventures wishing to use this code must contact the author
 * (Karl Stenerud) to negotiate commercial licensing terms.
 *
 * The latest version of this code can be obtained at:
 * http://milliways.scas.bcit.bc.ca/~karl/musashi
 */


/* ======================================================================== */
/* ============================= INSTRUCTIONS ============================= */
/* ======================================================================== */
/* 1. edit m68kconf.h and modify according to your needs.
 * 2. Implement in your host program the functions defined in
 *    "FUNCTIONS CALLED BY THE CPU" located later in this file.
 * 3. You must call m68000_pulse_reset() first to initialize the emulation.
 *
 * Notes:
 *
 * You must at least implement the m68000_read_memory_xx() and
 * m68000_write_memory_xx() functions in order to use the emulation core.
 * Unless you plan to implement more direct access to memory for immediate
 * reads and instruction fetches, you can just #define the
 * m68000_read_immediate_xx() and m68000_read_instruction() functions to
 * the m68000_read_memory_xx() functions.
 *
 * In order to use the emulation, you will need to call m68000_inout_reset()
 * and m68000_execute().  All the other functions are just to let you poke
 * about with the internals of the CPU.
 */


/* ======================================================================== */
/* ============================ GENERAL DEFINES =========================== */
/* ======================================================================== */

/* There are 7 levels of interrupt to the M68000.  Level 7 cannot me masked */
#define MC68000_INT_NONE 0
#define MC68000_IRQ_1    1
#define MC68000_IRQ_2    2
#define MC68000_IRQ_3    3
#define MC68000_IRQ_4    4
#define MC68000_IRQ_5    5
#define MC68000_IRQ_6    6
#define MC68000_IRQ_7    7

/* Special interrupt acknowledge values.
 * Use these as special returns from m68000_interrupt_acknowledge
 */
/* Causes an interrupt autovector (0x18 + interrupt level) to be taken.
 * This happens in a real 68000 if VPA or AVEC is asserted during an interrupt
 * acknowledge cycle instead of DTACK.
 */
#define M68000_INT_ACK_AUTOVECTOR    -1
/* this is the name that cpuintrf.c expected last time :-/ */
#define MC68000_INT_ACK_AUTOVECTOR M68000_INT_ACK_AUTOVECTOR
/* Causes the spurious interrupt vector (0x18) to be taken
 * This happens in a real 68000 if BERR is asserted during the interrupt
 * acknowledge cycle (i.e. no devices responded to the acknowledge).
 */
#define M68000_INT_ACK_SPURIOUS      -2


/* CPU types for use in m68000_set_cpu_type() */
#define M68000_CPU_000 1
#define M68000_CPU_010 2


/* ======================================================================== */
/* ============================== STRUCTURES ============================== */
/* ======================================================================== */

#ifndef A68KEM

typedef struct                 /* CPU Context */
{
   unsigned int  cpu_type;     /* CPU Type being emulated */
   unsigned int  dr[8];        /* Data Registers */
   unsigned int  ar[8];        /* Address Registers */
   unsigned int  pc;           /* Program Counter */
   unsigned int  usp;          /* User Stack Pointer */
   unsigned int  isp;          /* Interrupt Stack Pointer */
   unsigned int  vbr;          /* Vector Base Register.  Used in 68010+ */
   unsigned int  sfc;          /* Source Function Code.  Used in 68010+ */
   unsigned int  dfc;          /* Destination Function Code.  Used in 68010+ */
   unsigned int  sr;           /* Status Register */
   unsigned int  stopped;      /* Stopped state: only interrupt can restart */
   unsigned int  halted;       /* Halted state: only reset can restart */
   unsigned int  ints_pending; /* Interrupt levels pending */
} m68000_cpu_context;

#else

typedef struct
{
	int   d[8]; 			/* 0x0004 8 Data registers */
	int   a[8]; 			/* 0x0024 8 Address registers */

	int   usp;				/* 0x0044 Stack registers (They will overlap over reg_a7) */
	int   isp;				/* 0x0048 */

	int   sr_high;			/* 0x004C System registers */
	int   ccr;				/* 0x0050 CCR in Intel Format */
	int   x_carry;			/* 0x0054 Extended Carry */

	int   pc;				/* 0x0058 Program Counter */

	int   IRQ_level;		/* 0x005C IRQ level you want the MC68K process (0=None)  */

	/* Backward compatible with C emulator - Only set in Debug compile */

	int   sr;

#if NEW_INTERRUPT_SYSTEM
	int (*irq_callback)(int irqline);
#endif /* NEW_INTERRUPT_SYSTEM */

	int vbr;				/* Vector Base Register.  Will be used in 68010 */
	int sfc;				/* Source Function Code.  Will be used in 68010 */
	int dfc;				/* Destination Function Code.  Will be used in 68010 */

} m68000_cpu_context;

#endif

/* ======================================================================== */
/* ====================== FUNCTIONS CALLED BY THE CPU ===================== */
/* ======================================================================== */

/* You will have to implement these functions */

/* read/write functions called by the CPU to access memory.
 * while values used are 32 bits, only the appropriate number
 * of bits are relevant (i.e. in write_memory_8, only the lower 8 bits
 * of value should be written to memory).
 * address will be a 24-bit value.
 */

/* Read from anywhere */
int  m68000_read_memory_8(int address);
int  m68000_read_memory_16(int address);
int  m68000_read_memory_32(int address);

/* Read data immediately following the PC */
int  m68000_read_immediate_8(int address);
int  m68000_read_immediate_16(int address);
int  m68000_read_immediate_32(int address);

/* Read an instruction (immeditate after PC) */
int  m68000_read_instruction(int address);

/* Write to anywhere */
void m68000_write_memory_8(int address, int value);
void m68000_write_memory_16(int address, int value);
void m68000_write_memory_32(int address, int value);



/* ======================================================================== */
/* ==== FUNCTIONS CALLED BY THE CPU THAT CAN BE DISABLED IN M68KCONF.H ==== */
/* ======================================================================== */

/* Simulates acknowledgement of an interrupt.
 * int_level is the interrupt level being acknowledged.
 * The host program should return a vector from 0x02-0xff (0 and 1 are
 * ignored), or one of the special interrupt acknowledge values specified
 * above.
 */
int m68000_interrupt_acknowledge(int int_level);

/* Called when A 68010+ cpu is being emulated and it encounters a BKPT
 * instruction.  The 68010 will always have data = 0.
 */
void m68000_breakpoint_acknowledge(int data);

/* These functions can be disabled in m68kconf.h */

/* Called if the cpu encounters a reset instruction. */
void m68000_output_reset(void);

/* Inform the host we are changing the PC by a large value */
void m68000_setting_pc(unsigned int address);

/* Inform the host that the CPU function code is changing */
void m68000_changing_fc(int function_code);

/* Allow a hook into the instruction cycle of the cpu */
void m68000_debug_hook(void);

/* Allow a hook to peek at the PC during the instruction cycle of the cpu */
void m68000_peek_pc_hook(void);



/* ======================================================================== */
/* ====================== FUNCTIONS TO ACCESS THE CPU ===================== */
/* ======================================================================== */

/* Use this function to set the CPU type ypu want to emulate.
 * Currently supported types are: M68000_CPU_000 and M68000_CPU_010.
 */
void m68000_set_cpu_type(int cpu_type);

/* Reset the CPU as if you asserted RESET */
/* You *MUST* call this at least once to initialize the emulation */
void m68000_pulse_reset(void);

/* execute num_clks worth of instructions.  returns number of clks used */
int m68000_execute(int num_clks);

/* Interrupt the CPU as if you asserted the INT pins */
void m68000_pulse_irq(int int_level);

void m68000_clear_irq(int int_level);

/* Halt the CPU as if you asserted the HALT pin */
void m68000_pulse_halt(void);

/* look at the internals of the CPU */
int m68000_peek_dr(int reg_num);
int m68000_peek_ar(int reg_num);
unsigned int m68000_peek_pc(void);
int m68000_peek_sr(void);
int m68000_peek_ir(void);
int m68000_peek_t_flag(void);
int m68000_peek_s_flag(void);
int m68000_peek_int_mask(void);
int m68000_peek_x_flag(void);
int m68000_peek_n_flag(void);
int m68000_peek_z_flag(void);
int m68000_peek_v_flag(void);
int m68000_peek_c_flag(void);
int m68000_peek_usp(void);
int m68000_peek_isp(void);

/* poke values into the internals of the CPU */
void m68000_poke_dr(int reg_num, int value);
void m68000_poke_ar(int reg_num, int value);
void m68000_poke_pc(unsigned int value);
void m68000_poke_sr(int value);
void m68000_poke_ir(int value);
void m68000_poke_t_flag(int value);
void m68000_poke_s_flag(int value);
void m68000_poke_int_mask(int value);
void m68000_poke_x_flag(int value);
void m68000_poke_n_flag(int value);
void m68000_poke_z_flag(int value);
void m68000_poke_v_flag(int value);
void m68000_poke_c_flag(int value);
void m68000_poke_usp(int value);
void m68000_poke_isp(int value);

/* context switching to allow multiple CPUs */
void m68000_get_context(m68000_cpu_context* cpu_context);
void m68000_set_context(m68000_cpu_context* cpu_context);

/* check if an instruction is valid for the M68000 */
int m68000_is_valid_instruction(int instruction);



/* ======================================================================== */
/* ============================= CONFIGURATION ============================ */
/* ======================================================================== */

/* Import the configuration for this build */
#include "m68kconf.h"


/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */

#endif /* M68000__HEADER */
