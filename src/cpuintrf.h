#ifndef CPUINTRF_H
#define CPUINTRF_H

/* Maximum size that a CPU structre can be:
 * HJB 01/02/98 changed to the next power of two
 * the real context size is evaluated by subtracting
 * the fixed part of struct cpuinfo
 */
#define CPU_CONTEXT_SIZE    2048

/* The old system is obsolete and no longer supported by the core */
#define NEW_INTERRUPT_SYSTEM    1

#define MAX_IRQ_LINES   8       /* maximum number of IRQ lines per CPU */

#define CLEAR_LINE		0		/* clear (a fired, held or pulsed) line */
#define ASSERT_LINE     1       /* assert an interrupt immediately */
#define HOLD_LINE       2       /* hold interrupt line until enable is true */
#define PULSE_LINE		3		/* pulse interrupt line for one instruction */

#include "timer.h"

/* ASG 971222 -- added this generic structure */
struct cpu_interface
{
	int cpu_num, cpu_family;
	void (*reset)(void *param);
	void (*exit)(void);
	int (*execute)(int cycles);
	unsigned (*get_context)(void *reg);
	void (*set_context)(void *reg);
    unsigned (*get_pc)(void);
	void (*set_pc)(unsigned val);
	unsigned (*get_sp)(void);
	void (*set_sp)(unsigned val);
    unsigned (*get_reg)(int regnum);
	void (*set_reg)(int regnum, unsigned val);
    void (*set_nmi_line)(int linestate);
	void (*set_irq_line)(int irqline, int linestate);
	void (*set_irq_callback)(int(*callback)(int irqline));
	void (*internal_interrupt)(int type);
	void (*cpu_state_save)(void *file);
	void (*cpu_state_load)(void *file);
	const char* (*cpu_info)(void *context,int regnum);

    int num_irqs;
    int *icount;
	int no_int, irq_int, nmi_int;

	int (*memory_read)(int offset);
	void (*memory_write)(int offset, int data);
	void (*set_op_base)(int pc);
	int address_bits;
	int abits1, abits2, abitsmin;
};

extern struct cpu_interface cpuintf[];



void cpu_init(void);
void cpu_run(void);

/* optional watchdog */
void watchdog_reset_w(int offset,int data);
int watchdog_reset_r(int offset);
/* Use this function to reset the machine */
void machine_reset(void);
/* Use this function to reset a single CPU */
void cpu_reset(int cpu);

/* Use this function to stop and restart CPUs */
void cpu_halt(int cpunum,int running);
/* This function returns CPUNUM current status (running or halted) */
int  cpu_getstatus(int cpunum);
int cpu_gettotalcpu(void);
int cpu_getactivecpu(void);
void cpu_setactivecpu(int cpunum);

/* Returns the current program counter */
unsigned cpu_get_pc(void);
/* Set the current program counter */
void cpu_set_pc(unsigned val);

/* Returns the current stack pointer */
unsigned cpu_get_sp(void);
/* Set the current stack pointer */
void cpu_set_sp(unsigned val);

/* Returns a specific register value (mamedbg) */
unsigned cpu_get_reg(int regnum);
/* Sets a specific register value (mamedbg) */
void cpu_set_reg(int regnum, unsigned val);

/* Returns a specific register value for the active CPU (mamedbg) */
unsigned cpunum_get_reg(int cpunum, int regnum);
/* Sets a specific register value for the active CPU (mamedbg) */
void cpunum_set_reg(int cpunum, int regnum, unsigned val);

/* Returns previous pc (start of opcode causing read/write) */
int cpu_getpreviouspc(void);  /* -RAY- */

/* Returns the return address from the top of the stack (Z80 only) */
int cpu_getreturnpc(void);
int cycles_currently_ran(void);
int cycles_left_to_run(void);

/* Returns the number of CPU cycles which take place in one video frame */
int cpu_gettotalcycles(void);
/* Returns the number of CPU cycles before the next interrupt handler call */
int cpu_geticount(void);
/* Returns the number of CPU cycles before the end of the current video frame */
int cpu_getfcount(void);
/* Returns the number of CPU cycles in one video frame */
int cpu_getfperiod(void);
/* Scales a given value by the ratio of fcount / fperiod */
int cpu_scalebyfcount(int value);
/* Returns the current scanline number */
int cpu_getscanline(void);
/* Returns the amount of time until a given scanline */
double cpu_getscanlinetime(int scanline);
/* Returns the duration of a single scanline */
double cpu_getscanlineperiod(void);
/* Returns the duration of a single scanline in cycles */
int cpu_getscanlinecycles(void);
/* Returns the number of cycles since the beginning of this frame */
int cpu_getcurrentcycles(void);
/* Returns the current horizontal beam position in pixels */
int cpu_gethorzbeampos(void);
/*
  Returns the number of times the interrupt handler will be called before
  the end of the current video frame. This is can be useful to interrupt
  handlers to synchronize their operation. If you call this from outside
  an interrupt handler, add 1 to the result, i.e. if it returns 0, it means
  that the interrupt handler will be called once.
*/
int cpu_getiloops(void);

/* Returns the current VBLANK state */
int cpu_getvblank(void);

/* Returns the number of the video frame we are currently playing */
int cpu_getcurrentframe(void);


/* generate a trigger after a specific period of time */
void cpu_triggertime (double duration, int trigger);
/* generate a trigger now */
void cpu_trigger (int trigger);

/* burn CPU cycles until a timer trigger */
void cpu_spinuntil_trigger (int trigger);
/* burn CPU cycles until the next interrupt */
void cpu_spinuntil_int (void);
/* burn CPU cycles until our timeslice is up */
void cpu_spin (void);
/* burn CPU cycles for a specific period of time */
void cpu_spinuntil_time (double duration);

/* yield our timeslice for a specific period of time */
void cpu_yielduntil_trigger (int trigger);
/* yield our timeslice until the next interrupt */
void cpu_yielduntil_int (void);
/* yield our current timeslice */
void cpu_yield (void);
/* yield our timeslice for a specific period of time */
void cpu_yielduntil_time (double duration);

/* set the NMI line state for a CPU, normally use PULSE_LINE */
void cpu_set_nmi_line(int cpunum, int state);
/* set the IRQ line state for a specific irq line of a CPU */
/* normally use state HOLD_LINE, irqline 0 for first IRQ type of a cpu */
void cpu_set_irq_line(int cpunum, int irqline, int state);
/* this is to be called by CPU cores only! */
void cpu_generate_internal_interrupt(int cpunum, int type);
/* set the vector to be returned during a CPU's interrupt acknowledge cycle */
void cpu_irq_line_vector_w(int cpunum, int irqline, int vector);

/* use these in your write memory/port handles to set an IRQ vector */
/* offset corresponds to the irq line number here */
void cpu_0_irq_line_vector_w(int offset, int data);
void cpu_1_irq_line_vector_w(int offset, int data);
void cpu_2_irq_line_vector_w(int offset, int data);
void cpu_3_irq_line_vector_w(int offset, int data);

/* Obsolete functions: avoid to use them for new drivers if possible! */

/* cause an interrupt on a CPU */
void cpu_cause_interrupt(int cpu,int type);
void cpu_clear_pending_interrupts(int cpu);
void interrupt_enable_w(int offset,int data);
void interrupt_vector_w(int offset,int data);
int interrupt(void);
int nmi_interrupt(void);
int m68_level1_irq(void);
int m68_level2_irq(void);
int m68_level3_irq(void);
int m68_level4_irq(void);
int m68_level5_irq(void);
int m68_level6_irq(void);
int m68_level7_irq(void);
int ignore_interrupt(void);

/* CPU context access */
void* cpu_getcontext (int _activecpu);
int cpu_is_saving_context(int _activecpu);

/* CPU info interface */

/* get information for the active CPU */
const char *cpu_name(void);
const char *cpu_core_family(void);
const char *cpu_core_version(void);
const char *cpu_core_file(void);
const char *cpu_core_credits(void);

const char *cpu_pc(void);
const char *cpu_sp(void);
const char *cpu_flags(void);
const char *cpu_dasm(void);
const char *cpu_dump_reg(int regnum);
const char *cpu_dump_state(void);

/* get information for a specific CPU */
const char *cputype_name(int cputype);
const char *cputype_core_family(int cputype);
const char *cputype_core_version(int cputype);
const char *cputype_core_file(int cputype);
const char *cputype_core_credits(int cputype);

const char *cpunum_pc(int cpunum);
const char *cpunum_sp(int cpunum);
const char *cpunum_dasm(int cpunum);
const char *cpunum_flags(int cpunum);
const char *cpunum_dump_reg(int cpunum, int regnum);
const char *cpunum_dump_state(int cpunum);

/* this is the 'low level' interface call for the active cpu */
#define CPU_INFO_NAME		0
#define CPU_INFO_FAMILY     1
#define CPU_INFO_VERSION    2
#define CPU_INFO_FILE       3
#define CPU_INFO_CREDITS    4
#define CPU_INFO_PC 		10
#define CPU_INFO_SP 		11
#define CPU_INFO_DASM		12
#define CPU_INFO_FLAGS		13
#define CPU_INFO_REG		14

void cpu_dump_states(void);

/* daisy-chain link */
typedef struct {
    void (*reset)(int);             /* reset callback     */
    int  (*interrupt_entry)(int);   /* entry callback     */
    void (*interrupt_reti)(int);    /* reti callback      */
    int irq_param;                  /* callback paramater */
}	Z80_DaisyChain;

#define Z80_MAXDAISY	4		/* maximum of daisy chan device */

#define Z80_INT_REQ     0x01    /* interrupt request mask       */
#define Z80_INT_IEO     0x02    /* interrupt disable mask(IEO)  */

#define Z80_VECTOR(device,state) (((device)<<8)|(state))

#endif	/* CPUINTRF_H */
