/***************************************************************************

  cpuintrf.c

  Don't you love MS-DOS 8+3 names? That stands for CPU interface.
  Functions needed to interface the CPU emulator with the other parts of
  the emulation.

***************************************************************************/

#include <signal.h>
#include "driver.h"
#include "timer.h"
#include "state.h"
#include "mamedbg.h"

#if (HAS_Z80)
#include "cpu/z80/z80.h"
#endif
#if (HAS_Z80_VM)
#include "cpu/z80/z80_vm.h"
#endif
#if (HAS_8080 || HAS_8085A)
#include "cpu/i8085/i8085.h"
#endif
#if (HAS_M6502 || HAS_M65C02 || HAS_M6510 || HAS_N2A03)
#include "cpu/m6502/m6502.h"
#endif
#if (HAS_H6280)
#include "cpu/h6280/h6280.h"
#endif
#if (HAS_I86)
#include "cpu/i86/i86intrf.h"
#endif
#if (HAS_V20 || HAS_V30 || HAS_V33)
#include "cpu/nec/necintrf.h"
#endif
#if (HAS_I8035 || HAS_I8039 || HAS_I8048 || HAS_N7751)
#include "cpu/i8039/i8039.h"
#endif
#if (HAS_M6800 || HAS_M6801 || HAS_M6802 || HAS_M6803 || HAS_M6808 || HAS_HD63701)
#include "cpu/m6800/m6800.h"
#endif
#if (HAS_M6805 || HAS_M68705 || HAS_HD63705)
#include "cpu/m6805/m6805.h"
#endif
#if (HAS_M6309 || HAS_M6809)
#include "cpu/m6809/m6809.h"
#endif
#if (HAS_KONAMI)
#include "cpu/konami/konami.h"
#endif
#if (HAS_M68000 || defined HAS_M68010 || HAS_M68020)
#include "cpu/m68000/m68000.h"
#endif
#if (HAS_T11)
#include "cpu/t11/t11.h"
#endif
#if (HAS_S2650)
#include "cpu/s2650/s2650.h"
#endif
#if (HAS_TMS34010)
#include "cpu/tms34010/tms34010.h"
#endif
#if (HAS_TMS9900)
#include "cpu/tms9900/tms9900.h"
#endif
#if (HAS_Z8000)
#include "cpu/z8000/z8000.h"
#endif
#if (HAS_TMS320C10)
#include "cpu/tms32010/tms32010.h"
#endif
#if (HAS_CCPU)
#include "cpu/ccpu/ccpu.h"
#endif
#if (HAS_PDP1)
#include "cpu/pdp1/pdp1.h"
#endif

/* these are triggers sent to the timer system for various interrupt events */
#define TRIGGER_TIMESLICE       -1000
#define TRIGGER_INT             -2000
#define TRIGGER_YIELDTIME       -3000
#define TRIGGER_SUSPENDTIME     -4000

#define VERBOSE 0

#define SAVE_STATE_TEST 0

#if VERBOSE
#define LOG(x)	if( errorlog ) fprintf x
#else
#define LOG(x)
#endif

#define CPUINFO_SIZE	(5*sizeof(int)+4*sizeof(void*)+2*sizeof(double))
/* How do I calculate the next power of two from CPUINFO_SIZE using a macro? */
#ifdef __LP64__
#define CPUINFO_ALIGN   (128-CPUINFO_SIZE)
#else
#define CPUINFO_ALIGN   (64-CPUINFO_SIZE)
#endif

struct cpuinfo
{
	struct cpu_interface *intf; 	/* pointer to the interface functions */
	int iloops; 					/* number of interrupts remaining this frame */
	int totalcycles;				/* total CPU cycles executed */
	int vblankint_countdown;		/* number of vblank callbacks left until we interrupt */
	int vblankint_multiplier;		/* number of vblank callbacks per interrupt */
	void *vblankint_timer;			/* reference to elapsed time counter */
	double vblankint_period;		/* timing period of the VBLANK interrupt */
	void *timedint_timer;			/* reference to this CPU's timer */
	double timedint_period; 		/* timing period of the timed interrupt */
	void *context;					/* dynamically allocated context buffer */
	int save_context;				/* need to context switch this CPU? yes or no */
	UINT8 filler[CPUINFO_ALIGN];	/* make the array aligned to next power of 2 */
};

static struct cpuinfo cpu[MAX_CPU];

static int activecpu,totalcpu;
static int cpu_running[MAX_CPU];
static int cycles_running;	/* number of cycles that the CPU emulation was requested to run */
					/* (needed by cpu_getfcount) */
static int have_to_reset;
static int hiscoreloaded;

static int interrupt_enable[MAX_CPU];
static int interrupt_vector[MAX_CPU];

static int irq_line_state[MAX_CPU * MAX_IRQ_LINES];
static int irq_line_vector[MAX_CPU * MAX_IRQ_LINES];

static int watchdog_counter;

static void *vblank_timer;
static int vblank_countdown;
static int vblank_multiplier;
static double vblank_period;

static void *refresh_timer;
static double refresh_period;
static double refresh_period_inv;

static void *timeslice_timer;
static double timeslice_period;

static double scanline_period;
static double scanline_period_inv;

static int usres; /* removed from cpu_run and made global */
static int vblank;
static int current_frame;

static void cpu_generate_interrupt (int cpunum, int (*func)(void), int num);
static void cpu_vblankintcallback (int param);
static void cpu_timedintcallback (int param);
static void cpu_internal_interrupt(int cpunum, int type);
static void cpu_manualnmicallback(int param);
static void cpu_manualirqcallback(int param);
static void cpu_internalintcallback(int param);
static void cpu_manualintcallback (int param);
static void cpu_clearintcallback (int param);
static void cpu_resetcallback (int param);
static void cpu_timeslicecallback (int param);
static void cpu_vblankreset (void);
static void cpu_vblankcallback (int param);
static void cpu_updatecallback (int param);
static double cpu_computerate (int value);
static void cpu_inittimers (void);


/* default irq callback handlers */
static int cpu_0_irq_callback(int irqline);
static int cpu_1_irq_callback(int irqline);
static int cpu_2_irq_callback(int irqline);
static int cpu_3_irq_callback(int irqline);

/* and a list of them for indexed access */
static int (*cpu_irq_callbacks[MAX_CPU])(int) = {
    cpu_0_irq_callback,
    cpu_1_irq_callback,
    cpu_2_irq_callback,
    cpu_3_irq_callback
};

/* Default window layout for the debugger */
UINT8 default_win_layout[] = {
	 0, 0,80, 5,	/* register window (top rows) */
	 0, 5,24,17,	/* disassembler window (left, middle columns) */
	25, 5,55, 8,	/* memory #1 window (right, upper middle) */
	25,14,55, 8,	/* memory #2 window (right, lower middle) */
	 0,23,80, 1 	/* command line window (bottom row) */
};

/* Dummy interfaces for non-CPUs */
static void Dummy_reset(void *param);
static void Dummy_exit(void);
static int Dummy_execute(int cycles);
static unsigned Dummy_get_context(void *regs);
static void Dummy_set_context(void *regs);
static unsigned Dummy_get_pc(void);
static void Dummy_set_pc(unsigned val);
static unsigned Dummy_get_sp(void);
static void Dummy_set_sp(unsigned val);
static unsigned Dummy_get_reg(int regnum);
static void Dummy_set_reg(int regnum, unsigned val);
static void Dummy_set_nmi_line(int state);
static void Dummy_set_irq_line(int irqline, int state);
static void Dummy_set_irq_callback(int (*callback)(int irqline));
static int dummy_icount;
static const char *Dummy_info(void *context, int regnum);
static unsigned Dummy_dasm(char *buffer, unsigned pc);

/* Convenience macros - not in cpuintrf.h because they shouldn't be used by everyone */
#define RESET(index)                    ((*cpu[index].intf->reset)(Machine->drv->cpu[index].reset_param))
#define EXECUTE(index,cycles)           ((*cpu[index].intf->execute)(cycles))
#define GETCONTEXT(index,context)		((*cpu[index].intf->get_context)(context))
#define SETCONTEXT(index,context)		((*cpu[index].intf->set_context)(context))
#define GETPC(index)                    ((*cpu[index].intf->get_pc)())
#define SETPC(index,val)				((*cpu[index].intf->set_pc)(val))
#define GETSP(index)					((*cpu[index].intf->get_sp)())
#define SETSP(index,val)				((*cpu[index].intf->set_sp)(val))
#define GETREG(index,regnum)            ((*cpu[index].intf->get_reg)(regnum))
#define SETREG(index,regnum,value)		((*cpu[index].intf->set_reg)(regnum,value))
#define SETNMILINE(index,state) 		((*cpu[index].intf->set_nmi_line)(state))
#define SETIRQLINE(index,line,state)	((*cpu[index].intf->set_irq_line)(line,state))
#define SETIRQCALLBACK(index,callback)	((*cpu[index].intf->set_irq_callback)(callback))
#define INTERNAL_INTERRUPT(index,type)	if( cpu[index].intf->internal_interrupt ) ((*cpu[index].intf->internal_interrupt)(type))
#define CPUINFO(index,context,regnum)	((*cpu[index].intf->cpu_info)(context,regnum))
#define CPUDASM(index,buffer,pc)		((*cpu[index].intf->cpu_dasm)(buffer,pc))
#define ICOUNT(index)                   (*cpu[index].intf->icount)
#define INT_TYPE_NONE(index)            (cpu[index].intf->no_int)
#define INT_TYPE_IRQ(index) 			(cpu[index].intf->irq_int)
#define INT_TYPE_NMI(index)             (cpu[index].intf->nmi_int)
#define READMEM(index,offset)           ((*cpu[index].intf->memory_read)(offset))
#define WRITEMEM(index,offset,data)     ((*cpu[index].intf->memory_write)(offset,data))
#define SET_OP_BASE(index,pc)           ((*cpu[index].intf->set_op_base)(pc))

#define CPU_TYPE(index) 				(Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK)
#define CPU_AUDIO(index)				(Machine->drv->cpu[index].cpu_type & CPU_AUDIO_CPU)

#define IFC_INFO(cpu,context,regnum)	((cpuintf[cpu].cpu_info)(context,regnum))

/* warning these must match the defines in driver.h! */
struct cpu_interface cpuintf[] =
{
	/* Dummy CPU -- placeholder for type 0 */
	{
		CPU_DUMMY,							/* CPU number and family cores sharing resources */
		Dummy_reset,						/* Reset CPU */
		Dummy_exit, 						/* Shut down the CPU */
		Dummy_execute,						/* Execute a number of cycles */
        Dummy_get_context,                  /* Get the contents of the registers */
		Dummy_set_context,					/* Set the contents of the registers */
		Dummy_get_pc,						/* Return the current program counter */
		Dummy_set_pc,						/* Set the current program counter */
		Dummy_get_sp,						/* Return the current stack pointer */
		Dummy_set_sp,						/* Set the current stack pointer */
        Dummy_get_reg,                       /* Get a specific register value */
        Dummy_set_reg,                       /* Set a specific register value */
		Dummy_set_nmi_line, 				/* Set state of the NMI line */
		Dummy_set_irq_line, 				/* Set state of the IRQ line */
		Dummy_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        Dummy_info,                         /* Get formatted string for a specific register */
		Dummy_dasm, 						/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&dummy_icount,						/* Pointer to the instruction count */
		0,									/* Interrupt types: none, IRQ, NMI */
		-1,
		-1,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,16,CPU_IS_LE,1,1, 				/* CPU address shift, bits, endianess, align unit, max. instruction length */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
	},
#if (HAS_Z80)
    {
		CPU_Z80,							/* CPU number and family cores sharing resources */
        z80_reset,                          /* Reset CPU */
		z80_exit,							/* Shut down the CPU */
		z80_execute,						/* Execute a number of cycles */
		z80_get_context,					/* Get the contents of the registers */
		z80_set_context,					/* Set the contents of the registers */
		z80_get_pc,							/* Return the current program counter */
		z80_set_pc,							/* Set the current program counter */
		z80_get_sp,							/* Return the current stack pointer */
		z80_set_sp,							/* Set the current stack pointer */
		z80_get_reg, 						/* Get a specific register value */
		z80_set_reg, 						/* Set a specific register value */
        z80_set_nmi_line,                   /* Set state of the NMI line */
		z80_set_irq_line,					/* Set state of the IRQ line */
		z80_set_irq_callback,				/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		z80_state_save, 					/* Save CPU state */
		z80_state_load, 					/* Load CPU state */
        z80_info,                           /* Get formatted string for a specific register */
		z80_dasm,							/* Disassemble one instruction */
		1,0xff, 							/* Number of IRQ lines, default IRQ vector */
		&z80_ICount,						/* Pointer to the instruction count */
		Z80_IGNORE_INT, 					/* Interrupt types: none, IRQ, NMI */
		Z80_IRQ_INT,
		Z80_NMI_INT,
        cpu_readmem16,                      /* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,16,CPU_IS_LE,1,4, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
	},
#endif
#if (HAS_Z80_VM)
    {
		CPU_Z80_VM, 						/* CPU number and family cores sharing resources */
		z80_vm_reset,						/* Reset CPU */
		z80_vm_exit,						/* Shut down the CPU */
		z80_vm_execute, 					/* Execute a number of cycles */
		z80_vm_get_context, 				/* Get the contents of the registers */
		z80_vm_set_context, 				/* Set the contents of the registers */
		z80_vm_get_pc,						/* Return the current program counter */
		z80_vm_set_pc,						/* Set the current program counter */
		z80_vm_get_sp,						/* Return the current stack pointer */
		z80_vm_set_sp,						/* Set the current stack pointer */
		z80_vm_get_reg, 					/* Get a specific register value */
		z80_vm_set_reg, 					/* Set a specific register value */
		z80_vm_set_nmi_line,				/* Set state of the NMI line */
		z80_vm_set_irq_line,				/* Set state of the IRQ line */
		z80_vm_set_irq_callback,			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		z80_vm_state_save,					/* Save CPU state */
		z80_vm_state_load,					/* Load CPU state */
		z80_vm_info,						/* Get formatted string for a specific register */
		z80_vm_dasm,						/* Disassemble one instruction */
		1,0xff, 							/* Number of IRQ lines, default IRQ vector */
		&z80_ICount,						/* Pointer to the instruction count */
		Z80_IGNORE_INT, 					/* Interrupt types: none, IRQ, NMI */
		Z80_IRQ_INT,
		Z80_NMI_INT,
        cpu_readmem16,                      /* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,16,CPU_IS_LE,1,4, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
	},
#endif
#if (HAS_8080)
    {
		CPU_8080,							/* CPU number and family cores sharing resources */
        i8080_reset,                        /* Reset CPU */
		i8080_exit, 						/* Shut down the CPU */
		i8080_execute,						/* Execute a number of cycles */
		i8080_get_context,					/* Get the contents of the registers */
		i8080_set_context,					/* Set the contents of the registers */
		i8080_get_pc,						/* Return the current program counter */
		i8080_set_pc,						/* Set the current program counter */
		i8080_get_sp,						/* Return the current stack pointer */
		i8080_set_sp,						/* Set the current stack pointer */
		i8080_get_reg,						/* Get a specific register value */
		i8080_set_reg,						/* Set a specific register value */
        i8080_set_nmi_line,                 /* Set state of the NMI line */
		i8080_set_irq_line, 				/* Set state of the IRQ line */
		i8080_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		i8080_state_save,					/* Save CPU state */
		i8080_state_load,					/* Load CPU state */
        i8080_info,                         /* Get formatted string for a specific register */
		i8080_dasm, 						/* Disassemble one instruction */
		4,0xff, 							/* Number of IRQ lines, default IRQ vector */
		&i8080_ICount,						/* Pointer to the instruction count */
		I8080_NONE, 						/* Interrupt types: none, IRQ, NMI */
		I8080_INTR,
		I8080_TRAP,
		cpu_readmem16,                      /* Memory read */
		cpu_writemem16,                     /* Memory write */
		cpu_setOPbase16,                    /* Update CPU opcode base */
		0,16,CPU_IS_LE,1,3, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
#endif
#if (HAS_8085A)
    {
		CPU_8085A,							/* CPU number and family cores sharing resources */
        i8085_reset,                        /* Reset CPU */
		i8085_exit, 						/* Shut down the CPU */
		i8085_execute,						/* Execute a number of cycles */
		i8085_get_context,					/* Get the contents of the registers */
		i8085_set_context,					/* Set the contents of the registers */
		i8085_get_pc,						/* Return the current program counter */
		i8085_set_pc,						/* Set the current program counter */
		i8085_get_sp,						/* Return the current stack pointer */
		i8085_set_sp,						/* Set the current stack pointer */
		i8085_get_reg,						/* Get a specific register value */
		i8085_set_reg,						/* Set a specific register value */
        i8085_set_nmi_line,                 /* Set state of the NMI line */
		i8085_set_irq_line, 				/* Set state of the IRQ line */
		i8085_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		i8085_state_save,					/* Save CPU state */
		i8085_state_load,					/* Load CPU state */
        i8085_info,                         /* Get formatted string for a specific register */
		i8085_dasm, 						/* Disassemble one instruction */
		4,0xff, 							/* Number of IRQ lines, default IRQ vector */
		&i8085_ICount,						/* Pointer to the instruction count */
		I8085_NONE, 						/* Interrupt types: none, IRQ, NMI */
		I8085_INTR,
		I8085_TRAP,
		cpu_readmem16,                      /* Memory read */
		cpu_writemem16,                     /* Memory write */
		cpu_setOPbase16,                    /* Update CPU opcode base */
		0,16,CPU_IS_LE,1,3, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
	},
#endif
#if (HAS_M6502)
    {
		CPU_M6502,							/* CPU number and family cores sharing resources */
        m6502_reset,                        /* Reset CPU */
		m6502_exit, 						/* Shut down the CPU */
		m6502_execute,						/* Execute a number of cycles */
		m6502_get_context,					/* Get the contents of the registers */
		m6502_set_context,					/* Set the contents of the registers */
		m6502_get_pc,						/* Return the current program counter */
		m6502_set_pc,						/* Set the current program counter */
		m6502_get_sp,						/* Return the current stack pointer */
		m6502_set_sp,						/* Set the current stack pointer */
		m6502_get_reg,						/* Get a specific register value */
		m6502_set_reg,						/* Set a specific register value */
        m6502_set_nmi_line,                 /* Set state of the NMI line */
		m6502_set_irq_line, 				/* Set state of the IRQ line */
		m6502_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		m6502_state_save,					/* Save CPU state */
		m6502_state_load,					/* Load CPU state */
        m6502_info,                         /* Get formatted string for a specific register */
		m6502_dasm, 						/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&m6502_ICount,						/* Pointer to the instruction count */
		M6502_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		M6502_INT_IRQ,
		M6502_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,16,CPU_IS_LE,1,3, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
	},
#endif
#if (HAS_M65C02)
    {
		CPU_M65C02, 						/* CPU number and family cores sharing resources */
        m65c02_reset,                       /* Reset CPU */
		m65c02_exit,						/* Shut down the CPU */
		m65c02_execute, 					/* Execute a number of cycles */
		m65c02_get_context, 				/* Get the contents of the registers */
		m65c02_set_context, 				/* Set the contents of the registers */
		m65c02_get_pc,						/* Return the current program counter */
		m65c02_set_pc,						/* Set the current program counter */
		m65c02_get_sp,						/* Return the current stack pointer */
		m65c02_set_sp,						/* Set the current stack pointer */
		m65c02_get_reg,						/* Get a specific register value */
		m65c02_set_reg,						/* Set a specific register value */
        m65c02_set_nmi_line,                /* Set state of the NMI line */
		m65c02_set_irq_line,				/* Set state of the IRQ line */
		m65c02_set_irq_callback,			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		m65c02_state_save,					/* Save CPU state */
		m65c02_state_load,					/* Load CPU state */
        m65c02_info,                        /* Get formatted string for a specific register */
		m65c02_dasm,						/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&m65c02_ICount, 					/* Pointer to the instruction count */
		M65C02_INT_NONE,					/* Interrupt types: none, IRQ, NMI */
		M65C02_INT_IRQ,
		M65C02_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,16,CPU_IS_LE,1,3, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
#endif
#if (HAS_M6510)
    {
		CPU_M6510,							/* CPU number and family cores sharing resources */
        m6510_reset,                        /* Reset CPU */
		m6510_exit, 						/* Shut down the CPU */
		m6510_execute,						/* Execute a number of cycles */
		m6510_get_context,					/* Get the contents of the registers */
		m6510_set_context,					/* Set the contents of the registers */
		m6510_get_pc,						/* Return the current program counter */
		m6510_set_pc,						/* Set the current program counter */
		m6510_get_sp,						/* Return the current stack pointer */
		m6510_set_sp,						/* Set the current stack pointer */
		m6510_get_reg,						/* Get a specific register value */
		m6510_set_reg,						/* Set a specific register value */
        m6510_set_nmi_line,                 /* Set state of the NMI line */
		m6510_set_irq_line, 				/* Set state of the IRQ line */
		m6510_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		m6510_state_save,					/* Save CPU state */
		m6510_state_load,					/* Load CPU state */
        m6510_info,                         /* Get formatted string for a specific register */
		m6510_dasm, 						/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&m6510_ICount,						/* Pointer to the instruction count */
		M6510_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		M6510_INT_IRQ,
		M6510_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,16,CPU_IS_LE,1,3, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
#endif
#if (HAS_N2A03)
    {
		CPU_N2A03,							/* CPU number and family cores sharing resources */
        n2a03_reset,                        /* Reset CPU */
		n2a03_exit, 						/* Shut down the CPU */
		n2a03_execute,						/* Execute a number of cycles */
		n2a03_get_context,					/* Get the contents of the registers */
		n2a03_set_context,					/* Set the contents of the registers */
		n2a03_get_pc,						/* Return the current program counter */
		n2a03_set_pc,						/* Set the current program counter */
		n2a03_get_sp,						/* Return the current stack pointer */
		n2a03_set_sp,						/* Set the current stack pointer */
		n2a03_get_reg,						/* Get a specific register value */
		n2a03_set_reg,						/* Set a specific register value */
        n2a03_set_nmi_line,                 /* Set state of the NMI line */
		n2a03_set_irq_line, 				/* Set state of the IRQ line */
		n2a03_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		n2a03_state_save,					/* Save CPU state */
		n2a03_state_load,					/* Load CPU state */
        n2a03_info,                         /* Get formatted string for a specific register */
		n2a03_dasm, 						/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&n2a03_ICount,						/* Pointer to the instruction count */
		N2A03_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		N2A03_INT_IRQ,
		N2A03_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,16,CPU_IS_LE,1,3, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
#endif
#if (HAS_H6280)
    {
		CPU_H6280,							/* CPU number and family cores sharing resources */
        h6280_reset,                        /* Reset CPU */
		h6280_exit, 						/* Shut down the CPU */
		h6280_execute,						/* Execute a number of cycles */
		h6280_get_context,					/* Get the contents of the registers */
		h6280_set_context,					/* Set the contents of the registers */
		h6280_get_pc,						/* Return the current program counter */
		h6280_set_pc,						/* Set the current program counter */
		h6280_get_sp,						/* Return the current stack pointer */
		h6280_set_sp,						/* Set the current stack pointer */
		h6280_get_reg,						/* Get a specific register value */
		h6280_set_reg,						/* Set a specific register value */
        h6280_set_nmi_line,                 /* Set state of the NMI line */
		h6280_set_irq_line, 				/* Set state of the IRQ line */
		h6280_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        h6280_info,                         /* Get formatted string for a specific register */
		h6280_dasm, 						/* Disassemble one instruction */
		3,0,								/* Number of IRQ lines, default IRQ vector */
		&h6280_ICount,						/* Pointer to the instruction count */
		H6280_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		-1,
		H6280_INT_NMI,
		cpu_readmem21,						/* Memory read */
		cpu_writemem21, 					/* Memory write */
		cpu_setOPbase21,					/* Update CPU opcode base */
		0,21,CPU_IS_LE,1,3, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_21,ABITS2_21,ABITS_MIN_21	/* Address bits, for the memory system */
    },
#endif
#if (HAS_I86)
    {
		CPU_I86,							/* CPU number and family cores sharing resources */
        i86_reset,                          /* Reset CPU */
		i86_exit,							/* Shut down the CPU */
		i86_execute,						/* Execute a number of cycles */
		i86_get_context,					/* Get the contents of the registers */
		i86_set_context,					/* Set the contents of the registers */
		i86_get_pc,							/* Return the current program counter */
		i86_set_pc,							/* Set the current program counter */
		i86_get_sp,							/* Return the current stack pointer */
		i86_set_sp,							/* Set the current stack pointer */
		i86_get_reg, 						/* Get a specific register value */
		i86_set_reg, 						/* Set a specific register value */
        i86_set_nmi_line,                   /* Set state of the NMI line */
		i86_set_irq_line,					/* Set state of the IRQ line */
		i86_set_irq_callback,				/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        i86_info,                           /* Get formatted string for a specific register */
		i86_dasm,							/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&i86_ICount,						/* Pointer to the instruction count */
		I86_INT_NONE,						/* Interrupt types: none, IRQ, NMI */
		-1000,
		I86_NMI_INT,
		cpu_readmem20,						/* Memory read */
		cpu_writemem20, 					/* Memory write */
		cpu_setOPbase20,					/* Update CPU opcode base */
		0,20,CPU_IS_LE,1,5, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_20,ABITS2_20,ABITS_MIN_20	/* Address bits, for the memory system */
    },
#endif
#if (HAS_V20)
	{
		CPU_V20,							/* CPU number and family cores sharing resources */
		nec_reset,							/* Reset CPU */
		nec_exit,							/* Shut down the CPU */
		nec_execute,						/* Execute a number of cycles */
		nec_get_context,					/* Get the contents of the registers */
		nec_set_context,					/* Set the contents of the registers */
		nec_get_pc,							/* Return the current program counter */
		nec_set_pc,							/* Set the current program counter */
		nec_get_sp,							/* Return the current stack pointer */
		nec_set_sp,							/* Set the current stack pointer */
		nec_get_reg,						/* Get a specific register value */
		nec_set_reg,						/* Set a specific register value */
		nec_set_nmi_line,					/* Set state of the NMI line */
		nec_set_irq_line,					/* Set state of the IRQ line */
		nec_set_irq_callback,				/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
		nec_v20_info,						/* Get formatted string for a specific register */
		nec_dasm,							/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&nec_ICount,						/* Pointer to the instruction count */
		NEC_INT_NONE,						/* Interrupt types: none, IRQ, NMI */
		-1000,
		NEC_NMI_INT,
		cpu_readmem20,						/* Memory read */
		cpu_writemem20,						/* Memory write */
		cpu_setOPbase20,					/* Update CPU opcode base */
		0,20,CPU_IS_LE,1,5,					/* CPU address shift, bits, endianess, align unit, max. instruction length */
		ABITS1_20,ABITS2_20,ABITS_MIN_20	/* Address bits, for the memory system */
	},
#endif
#if (HAS_V30)
	{
		CPU_V30,							/* CPU number and family cores sharing resources */
		nec_reset,							/* Reset CPU */
		nec_exit,							/* Shut down the CPU */
		nec_execute,						/* Execute a number of cycles */
		nec_get_context,					/* Get the contents of the registers */
		nec_set_context,					/* Set the contents of the registers */
		nec_get_pc,							/* Return the current program counter */
		nec_set_pc,							/* Set the current program counter */
		nec_get_sp,							/* Return the current stack pointer */
		nec_set_sp,							/* Set the current stack pointer */
		nec_get_reg,						/* Get a specific register value */
		nec_set_reg,						/* Set a specific register value */
		nec_set_nmi_line,					/* Set state of the NMI line */
		nec_set_irq_line,					/* Set state of the IRQ line */
		nec_set_irq_callback,				/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
		nec_v30_info,						/* Get formatted string for a specific register */
		nec_dasm,							/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&nec_ICount,						/* Pointer to the instruction count */
		NEC_INT_NONE,						/* Interrupt types: none, IRQ, NMI */
		-1000,
		NEC_NMI_INT,
		cpu_readmem20,						/* Memory read */
		cpu_writemem20,						/* Memory write */
		cpu_setOPbase20,					/* Update CPU opcode base */
		0,20,CPU_IS_LE,1,5,					/* CPU address shift, bits, endianess, align unit, max. instruction length */
		ABITS1_20,ABITS2_20,ABITS_MIN_20	/* Address bits, for the memory system */
	},
#endif
#if (HAS_V33)
	{
		CPU_V33,							/* CPU number and family cores sharing resources */
		nec_reset,							/* Reset CPU */
		nec_exit,							/* Shut down the CPU */
		nec_execute,						/* Execute a number of cycles */
		nec_get_context,					/* Get the contents of the registers */
		nec_set_context,					/* Set the contents of the registers */
		nec_get_pc,							/* Return the current program counter */
		nec_set_pc,							/* Set the current program counter */
		nec_get_sp,							/* Return the current stack pointer */
		nec_set_sp,							/* Set the current stack pointer */
		nec_get_reg,						/* Get a specific register value */
		nec_set_reg,						/* Set a specific register value */
		nec_set_nmi_line,					/* Set state of the NMI line */
		nec_set_irq_line,					/* Set state of the IRQ line */
		nec_set_irq_callback,				/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
		nec_v33_info,						/* Get formatted string for a specific register */
		nec_dasm,							/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&nec_ICount,						/* Pointer to the instruction count */
		NEC_INT_NONE,						/* Interrupt types: none, IRQ, NMI */
		-1000,
		NEC_NMI_INT,
		cpu_readmem20,						/* Memory read */
		cpu_writemem20,						/* Memory write */
		cpu_setOPbase20,					/* Update CPU opcode base */
		0,20,CPU_IS_LE,1,5,					/* CPU address shift, bits, endianess, align unit, max. instruction length */
		ABITS1_20,ABITS2_20,ABITS_MIN_20	/* Address bits, for the memory system */
	},
#endif
#if (HAS_I8035)
    {
		CPU_I8035,							/* CPU number and family cores sharing resources */
        i8035_reset,                        /* Reset CPU */
		i8035_exit, 						/* Shut down the CPU */
		i8035_execute,						/* Execute a number of cycles */
		i8035_get_context,					/* Get the contents of the registers */
		i8035_set_context,					/* Set the contents of the registers */
		i8035_get_pc,						/* Return the current program counter */
		i8035_set_pc,						/* Set the current program counter */
		i8035_get_sp,						/* Return the current stack pointer */
		i8035_set_sp,						/* Set the current stack pointer */
		i8035_get_reg,						/* Get a specific register value */
		i8035_set_reg,						/* Set a specific register value */
        i8035_set_nmi_line,                 /* Set state of the NMI line */
		i8035_set_irq_line, 				/* Set state of the IRQ line */
		i8035_set_irq_callback, 			/* Set IRQ enable/vector callback */
        NULL,                               /* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        i8035_info,                         /* Get formatted string for a specific register */
		i8035_dasm, 						/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&i8035_ICount,						/* Pointer to the instruction count */
		I8035_IGNORE_INT,					/* Interrupt types: none, IRQ, NMI */
		I8035_EXT_INT,
        -1,
        cpu_readmem16,                      /* Memory read */
        cpu_writemem16,                     /* Memory write */
        cpu_setOPbase16,                    /* Update CPU opcode base */
		0,16,CPU_IS_LE,1,2, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
        ABITS1_16,ABITS2_16,ABITS_MIN_16    /* Address bits, for the memory system */
    },
#endif
#if (HAS_I8039)
    {
		CPU_I8039,							/* CPU number and family cores sharing resources */
        i8039_reset,                        /* Reset CPU */
		i8039_exit, 						/* Shut down the CPU */
		i8039_execute,						/* Execute a number of cycles */
		i8039_get_context,					/* Get the contents of the registers */
		i8039_set_context,					/* Set the contents of the registers */
		i8039_get_pc,						/* Return the current program counter */
		i8039_set_pc,						/* Set the current program counter */
		i8039_get_sp,						/* Return the current stack pointer */
		i8039_set_sp,						/* Set the current stack pointer */
		i8039_get_reg,						/* Get a specific register value */
		i8039_set_reg,						/* Set a specific register value */
        i8039_set_nmi_line,                 /* Set state of the NMI line */
		i8039_set_irq_line, 				/* Set state of the IRQ line */
		i8039_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        i8039_info,                         /* Get formatted string for a specific register */
		i8039_dasm, 						/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&i8039_ICount,						/* Pointer to the instruction count */
		I8039_IGNORE_INT,					/* Interrupt types: none, IRQ, NMI */
		I8039_EXT_INT,
		-1,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,16,CPU_IS_LE,1,2, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
#endif
#if (HAS_I8048)
    {
		CPU_I8048,							/* CPU number and family cores sharing resources */
        i8048_reset,                        /* Reset CPU */
		i8048_exit, 						/* Shut down the CPU */
		i8048_execute,						/* Execute a number of cycles */
		i8048_get_context,					/* Get the contents of the registers */
		i8048_set_context,					/* Set the contents of the registers */
		i8048_get_pc,						/* Return the current program counter */
		i8048_set_pc,						/* Set the current program counter */
		i8048_get_sp,						/* Return the current stack pointer */
		i8048_set_sp,						/* Set the current stack pointer */
		i8048_get_reg,						/* Get a specific register value */
		i8048_set_reg,						/* Set a specific register value */
        i8048_set_nmi_line,                 /* Set state of the NMI line */
		i8048_set_irq_line, 				/* Set state of the IRQ line */
		i8048_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        i8048_info,                         /* Get formatted string for a specific register */
		i8048_dasm, 						/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&i8048_ICount,						/* Pointer to the instruction count */
		I8048_IGNORE_INT,					/* Interrupt types: none, IRQ, NMI */
		I8048_EXT_INT,
		-1,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,16,CPU_IS_LE,1,2, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
#endif
#if (HAS_N7751)
    {
		CPU_N7751,							/* CPU number and family cores sharing resources */
        n7751_reset,                        /* Reset CPU */
		n7751_exit, 						/* Shut down the CPU */
		n7751_execute,						/* Execute a number of cycles */
		n7751_get_context,					/* Get the contents of the registers */
		n7751_set_context,					/* Set the contents of the registers */
		n7751_get_pc,						/* Return the current program counter */
		n7751_set_pc,						/* Set the current program counter */
		n7751_get_sp,						/* Return the current stack pointer */
		n7751_set_sp,						/* Set the current stack pointer */
		n7751_get_reg,						/* Get a specific register value */
		n7751_set_reg,						/* Set a specific register value */
        n7751_set_nmi_line,                 /* Set state of the NMI line */
		n7751_set_irq_line, 				/* Set state of the IRQ line */
		n7751_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        n7751_info,                         /* Get formatted string for a specific register */
		n7751_dasm, 						/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&n7751_ICount,						/* Pointer to the instruction count */
		N7751_IGNORE_INT,					/* Interrupt types: none, IRQ, NMI */
		N7751_EXT_INT,
		-1,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,16,CPU_IS_LE,1,2, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
#endif
#if (HAS_M6800)
    {
		CPU_M6800,							/* CPU number and family cores sharing resources */
        m6800_reset,                        /* Reset CPU */
		m6800_exit, 						/* Shut down the CPU */
		m6800_execute,						/* Execute a number of cycles */
		m6800_get_context,					/* Get the contents of the registers */
		m6800_set_context,					/* Set the contents of the registers */
		m6800_get_pc,						/* Return the current program counter */
		m6800_set_pc,						/* Set the current program counter */
		m6800_get_sp,						/* Return the current stack pointer */
		m6800_set_sp,						/* Set the current stack pointer */
		m6800_get_reg,						/* Get a specific register value */
		m6800_set_reg,						/* Set a specific register value */
        m6800_set_nmi_line,                 /* Set state of the NMI line */
		m6800_set_irq_line, 				/* Set state of the IRQ line */
		m6800_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		m6800_state_save,					/* Save CPU state */
		m6800_state_load,					/* Load CPU state */
		m6800_info, 						/* Get formatted string for a specific register */
		m6800_dasm, 						/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&m6800_ICount,						/* Pointer to the instruction count */
		M6800_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		M6800_INT_IRQ,
		M6800_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,16,CPU_IS_BE,1,4, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
#endif
#if (HAS_M6801)
    {
		CPU_M6801,							/* CPU number and family cores sharing resources */
        m6801_reset,                        /* Reset CPU */
		m6801_exit, 						/* Shut down the CPU */
		m6801_execute,						/* Execute a number of cycles */
		m6801_get_context,					/* Get the contents of the registers */
		m6801_set_context,					/* Set the contents of the registers */
		m6801_get_pc,						/* Return the current program counter */
		m6801_set_pc,						/* Set the current program counter */
		m6801_get_sp,						/* Return the current stack pointer */
		m6801_set_sp,						/* Set the current stack pointer */
		m6801_get_reg,						/* Get a specific register value */
		m6801_set_reg,						/* Set a specific register value */
		m6801_set_nmi_line, 				/* Set state of the NMI line */
		m6801_set_irq_line, 				/* Set state of the IRQ line */
		m6801_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		m6801_state_save,					/* Save CPU state */
		m6801_state_load,					/* Load CPU state */
		m6801_info, 						/* Get formatted string for a specific register */
		m6801_dasm, 						/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&m6801_ICount,						/* Pointer to the instruction count */
		M6801_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		M6801_INT_IRQ,
		M6801_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,16,CPU_IS_BE,1,4, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
#endif
#if (HAS_M6802)
    {
		CPU_M6802,							/* CPU number and family cores sharing resources */
        m6802_reset,                        /* Reset CPU */
		m6802_exit, 						/* Shut down the CPU */
		m6802_execute,						/* Execute a number of cycles */
		m6802_get_context,					/* Get the contents of the registers */
		m6802_set_context,					/* Set the contents of the registers */
		m6802_get_pc,						/* Return the current program counter */
		m6802_set_pc,						/* Set the current program counter */
		m6802_get_sp,						/* Return the current stack pointer */
		m6802_set_sp,						/* Set the current stack pointer */
		m6802_get_reg,						/* Get a specific register value */
		m6802_set_reg,						/* Set a specific register value */
        m6802_set_nmi_line,                 /* Set state of the NMI line */
		m6802_set_irq_line, 				/* Set state of the IRQ line */
		m6802_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		m6802_state_save,					/* Save CPU state */
		m6802_state_load,					/* Load CPU state */
        m6802_info,                         /* Get formatted string for a specific register */
		m6802_dasm, 						/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&m6802_ICount,						/* Pointer to the instruction count */
		M6802_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		M6802_INT_IRQ,
		M6802_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,16,CPU_IS_BE,1,4, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
#endif
#if (HAS_M6803)
    {
		CPU_M6803,							/* CPU number and family cores sharing resources */
        m6803_reset,                        /* Reset CPU */
		m6803_exit, 						/* Shut down the CPU */
		m6803_execute,						/* Execute a number of cycles */
		m6803_get_context,					/* Get the contents of the registers */
		m6803_set_context,					/* Set the contents of the registers */
		m6803_get_pc,						/* Return the current program counter */
		m6803_set_pc,						/* Set the current program counter */
		m6803_get_sp,						/* Return the current stack pointer */
		m6803_set_sp,						/* Set the current stack pointer */
		m6803_get_reg,						/* Get a specific register value */
		m6803_set_reg,						/* Set a specific register value */
        m6803_set_nmi_line,                 /* Set state of the NMI line */
		m6803_set_irq_line, 				/* Set state of the IRQ line */
		m6803_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		m6803_state_save,					/* Save CPU state */
		m6803_state_load,					/* Load CPU state */
        m6803_info,                         /* Get formatted string for a specific register */
		m6803_dasm, 						/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&m6803_ICount,						/* Pointer to the instruction count */
		M6803_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		M6803_INT_IRQ,
		M6803_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,16,CPU_IS_BE,1,4, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
#endif
#if (HAS_M6808)
    {
		CPU_M6808,							/* CPU number and family cores sharing resources */
        m6808_reset,                        /* Reset CPU */
		m6808_exit, 						/* Shut down the CPU */
        m6808_execute,                      /* Execute a number of cycles */
		m6808_get_context,					/* Get the contents of the registers */
		m6808_set_context,					/* Set the contents of the registers */
		m6808_get_pc,						/* Return the current program counter */
		m6808_set_pc,						/* Set the current program counter */
		m6808_get_sp,						/* Return the current stack pointer */
		m6808_set_sp,						/* Set the current stack pointer */
		m6808_get_reg,						/* Get a specific register value */
		m6808_set_reg,						/* Set a specific register value */
        m6808_set_nmi_line,                 /* Set state of the NMI line */
		m6808_set_irq_line, 				/* Set state of the IRQ line */
		m6808_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		m6808_state_save,					/* Save CPU state */
		m6808_state_load,					/* Load CPU state */
        m6808_info,                         /* Get formatted string for a specific register */
		m6808_dasm, 						/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&m6808_ICount,						/* Pointer to the instruction count */
		M6808_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		M6808_INT_IRQ,
		M6808_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,16,CPU_IS_BE,1,4, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
#endif
#if (HAS_HD63701)
    {
		CPU_HD63701,						/* CPU number and family cores sharing resources */
        hd63701_reset,                      /* Reset CPU */
		hd63701_exit,						/* Shut down the CPU */
		hd63701_execute,					/* Execute a number of cycles */
		hd63701_get_context,				/* Get the contents of the registers */
		hd63701_set_context,				/* Set the contents of the registers */
		hd63701_get_pc,						/* Return the current program counter */
		hd63701_set_pc,						/* Set the current program counter */
		hd63701_get_sp,						/* Return the current stack pointer */
		hd63701_set_sp,						/* Set the current stack pointer */
		hd63701_get_reg, 					/* Get a specific register value */
		hd63701_set_reg, 					/* Set a specific register value */
        hd63701_set_nmi_line,               /* Set state of the NMI line */
		hd63701_set_irq_line,				/* Set state of the IRQ line */
		hd63701_set_irq_callback,			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		hd63701_state_save, 				/* Save CPU state */
		hd63701_state_load, 				/* Load CPU state */
        hd63701_info,                       /* Get formatted string for a specific register */
		hd63701_dasm,						/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&hd63701_ICount,					/* Pointer to the instruction count */
		HD63701_INT_NONE,					/* Interrupt types: none, IRQ, NMI */
		HD63701_INT_IRQ,
		HD63701_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,16,CPU_IS_BE,1,4, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
#endif
#if (HAS_M6805)
    {
		CPU_M6805,							/* CPU number and family cores sharing resources */
        m6805_reset,                        /* Reset CPU */
		m6805_exit, 						/* Shut down the CPU */
        m6805_execute,                      /* Execute a number of cycles */
		m6805_get_context,					/* Get the contents of the registers */
		m6805_set_context,					/* Set the contents of the registers */
		m6805_get_pc,						/* Return the current program counter */
		m6805_set_pc,						/* Set the current program counter */
		m6805_get_sp,						/* Return the current stack pointer */
		m6805_set_sp,						/* Set the current stack pointer */
		m6805_get_reg,						/* Get a specific register value */
		m6805_set_reg,						/* Set a specific register value */
        m6805_set_nmi_line,                 /* Set state of the NMI line */
		m6805_set_irq_line, 				/* Set state of the IRQ line */
		m6805_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		m6805_state_save,					/* Save CPU state */
		m6805_state_load,					/* Load CPU state */
        m6805_info,                         /* Get formatted string for a specific register */
		m6805_dasm, 						/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&m6805_ICount,						/* Pointer to the instruction count */
		M6805_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		M6805_INT_IRQ,
		-1,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,11,CPU_IS_BE,1,3, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
#endif
#if (HAS_M68705)
    {
		CPU_M68705, 						/* CPU number and family cores sharing resources */
        m68705_reset,                       /* Reset CPU */
		m68705_exit,						/* Shut down the CPU */
		m68705_execute, 					/* Execute a number of cycles */
		m68705_get_context, 				/* Get the contents of the registers */
		m68705_set_context, 				/* Set the contents of the registers */
		m68705_get_pc,						/* Return the current program counter */
		m68705_set_pc,						/* Set the current program counter */
		m68705_get_sp,						/* Return the current stack pointer */
		m68705_set_sp,						/* Set the current stack pointer */
		m68705_get_reg,						/* Get a specific register value */
		m68705_set_reg,						/* Set a specific register value */
        m68705_set_nmi_line,                /* Set state of the NMI line */
		m68705_set_irq_line,				/* Set state of the IRQ line */
		m68705_set_irq_callback,			/* Set IRQ enable/vector callback */
        NULL,                               /* Cause internal interrupt */
		m68705_state_save,					/* Save CPU state */
		m68705_state_load,					/* Load CPU state */
        m68705_info,                        /* Get formatted string for a specific register */
		m68705_dasm,						/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&m68705_ICount, 					/* Pointer to the instruction count */
		M68705_INT_NONE,					/* Interrupt types: none, IRQ, NMI */
		M68705_INT_IRQ,
		-1,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,11,CPU_IS_BE,1,3, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
#endif
#if (HAS_HD63705)
    {
		CPU_HD63705,						/* CPU number and family cores sharing resources */
		hd63705_reset,						/* Reset CPU */
		hd63705_exit,						/* Shut down the CPU */
		hd63705_execute,					/* Execute a number of cycles */
		hd63705_get_context,				/* Get the contents of the registers */
		hd63705_set_context,				/* Set the contents of the registers */
		hd63705_get_pc, 					/* Return the current program counter */
		hd63705_set_pc, 					/* Set the current program counter */
		hd63705_get_sp, 					/* Return the current stack pointer */
		hd63705_set_sp, 					/* Set the current stack pointer */
		hd63705_get_reg,					/* Get a specific register value */
		hd63705_set_reg,					/* Set a specific register value */
		hd63705_set_nmi_line,				/* Set state of the NMI line */
		hd63705_set_irq_line,				/* Set state of the IRQ line */
		hd63705_set_irq_callback,			/* Set IRQ enable/vector callback */
        NULL,                               /* Cause internal interrupt */
		hd63705_state_save, 				/* Save CPU state */
		hd63705_state_load, 				/* Load CPU state */
		hd63705_info,						/* Get formatted string for a specific register */
		hd63705_dasm,						/* Disassemble one instruction */
		8,0,								/* Number of IRQ lines, default IRQ vector */
		&hd63705_ICount,					/* Pointer to the instruction count */
		HD63705_INT_NONE,					/* Interrupt types: none, IRQ, NMI */
		HD63705_INT_IRQ,
		-1,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,16,CPU_IS_BE,1,3, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
#endif
#if (HAS_M6309)
    {
		CPU_M6309,							/* CPU number and family cores sharing resources */
        m6309_reset,                        /* Reset CPU */
		m6309_exit, 						/* Shut down the CPU */
		m6309_execute,						/* Execute a number of cycles */
		m6309_get_context,					/* Get the contents of the registers */
		m6309_set_context,					/* Set the contents of the registers */
		m6309_get_pc,						/* Return the current program counter */
		m6309_set_pc,						/* Set the current program counter */
		m6309_get_sp,						/* Return the current stack pointer */
		m6309_set_sp,						/* Set the current stack pointer */
		m6309_get_reg,						/* Get a specific register value */
		m6309_set_reg,						/* Set a specific register value */
        m6309_set_nmi_line,                 /* Set state of the NMI line */
		m6309_set_irq_line, 				/* Set state of the IRQ line */
		m6309_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		m6309_state_save,					/* Save CPU state */
		m6309_state_load,					/* Load CPU state */
        m6309_info,                         /* Get formatted string for a specific register */
		m6309_dasm, 						/* Disassemble one instruction */
		2,0,								/* Number of IRQ lines, default IRQ vector */
		&m6309_ICount,						/* Pointer to the instruction count */
		M6309_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		M6309_INT_IRQ,
		M6309_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,16,CPU_IS_BE,1,4, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
	},
#endif
#if (HAS_M6809)
    {
		CPU_M6809,							/* CPU number and family cores sharing resources */
        m6809_reset,                        /* Reset CPU */
		m6809_exit, 						/* Shut down the CPU */
        m6809_execute,                      /* Execute a number of cycles */
		m6809_get_context,					/* Get the contents of the registers */
		m6809_set_context,					/* Set the contents of the registers */
		m6809_get_pc,						/* Return the current program counter */
		m6809_set_pc,						/* Set the current program counter */
		m6809_get_sp,						/* Return the current stack pointer */
		m6809_set_sp,						/* Set the current stack pointer */
		m6809_get_reg,						/* Get a specific register value */
		m6809_set_reg,						/* Set a specific register value */
        m6809_set_nmi_line,                 /* Set state of the NMI line */
		m6809_set_irq_line, 				/* Set state of the IRQ line */
		m6809_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		m6809_state_save,					/* Save CPU state */
		m6809_state_load,					/* Load CPU state */
        m6809_info,                         /* Get formatted string for a specific register */
		m6809_dasm, 						/* Disassemble one instruction */
		2,0,								/* Number of IRQ lines, default IRQ vector */
		&m6809_ICount,						/* Pointer to the instruction count */
		M6809_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		M6809_INT_IRQ,
		M6809_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,16,CPU_IS_BE,1,4, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
#endif
#if (HAS_M68000)
    {
		CPU_M68000, 						/* CPU number and family cores sharing resources */
        m68000_reset,                       /* Reset CPU */
		m68000_exit,						/* Shut down the CPU */
		m68000_execute, 					/* Execute a number of cycles */
		m68000_get_context, 				/* Get the contents of the registers */
		m68000_set_context, 				/* Set the contents of the registers */
		m68000_get_pc,						/* Return the current program counter */
		m68000_set_pc,						/* Set the current program counter */
		m68000_get_sp,						/* Return the current stack pointer */
		m68000_set_sp,						/* Set the current stack pointer */
		m68000_get_reg,						/* Get a specific register value */
		m68000_set_reg,						/* Set a specific register value */
		m68000_set_nmi_line,				/* Set state of the NMI line */
		m68000_set_irq_line,				/* Set state of the IRQ line */
		m68000_set_irq_callback,			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
        NULL,                               /* Save CPU state */
        NULL,                               /* Load CPU state */
        m68000_info,                        /* Get formatted string for a specific register */
		m68000_dasm,						/* Disassemble one instruction */
		8,MC68000_INT_ACK_AUTOVECTOR,		/* Number of IRQ lines, default IRQ vector */
		&m68000_ICount, 					/* Pointer to the instruction count */
		MC68000_INT_NONE,					/* Interrupt types: none, IRQ, NMI */
		-1,
		-1,
		cpu_readmem24,						/* Memory read */
		cpu_writemem24, 					/* Memory write */
		cpu_setOPbase24,					/* Update CPU opcode base */
		0,24,CPU_IS_BE,2,10,				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_24,ABITS2_24,ABITS_MIN_24	/* Address bits, for the memory system */
	},
#endif
#if (HAS_M68010)
    {
		CPU_M68010, 						/* CPU number and family cores sharing resources */
        m68010_reset,                       /* Reset CPU */
		m68010_exit,						/* Shut down the CPU */
		m68010_execute, 					/* Execute a number of cycles */
		m68010_get_context, 				/* Get the contents of the registers */
		m68010_set_context, 				/* Set the contents of the registers */
		m68010_get_pc,						/* Return the current program counter */
		m68010_set_pc,						/* Set the current program counter */
		m68010_get_sp,						/* Return the current stack pointer */
		m68010_set_sp,						/* Set the current stack pointer */
		m68010_get_reg,						/* Get a specific register value */
		m68010_set_reg,						/* Set a specific register value */
		m68010_set_nmi_line,				/* Set state of the NMI line */
		m68010_set_irq_line,				/* Set state of the IRQ line */
		m68010_set_irq_callback,			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
		m68010_info,						/* Get formatted string for a specific register */
		m68010_dasm,						/* Disassemble one instruction */
		8,MC68010_INT_ACK_AUTOVECTOR,		/* Number of IRQ lines, default IRQ vector */
		&m68010_ICount, 					/* Pointer to the instruction count */
		MC68010_INT_NONE,					/* Interrupt types: none, IRQ, NMI */
		-1,
		-1,
		cpu_readmem24,						/* Memory read */
		cpu_writemem24, 					/* Memory write */
		cpu_setOPbase24,					/* Update CPU opcode base */
		0,24,CPU_IS_BE,2,10,				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_24,ABITS2_24,ABITS_MIN_24	/* Address bits, for the memory system */
    },
#endif
#if (HAS_M68020)
    {
		CPU_M68020, 						/* CPU number and family cores sharing resources */
        m68020_reset,                       /* Reset CPU */
		m68020_exit,						/* Shut down the CPU */
		m68020_execute, 					/* Execute a number of cycles */
		m68020_get_context, 				/* Get the contents of the registers */
		m68020_set_context, 				/* Set the contents of the registers */
		m68020_get_pc,						/* Return the current program counter */
		m68020_set_pc,						/* Set the current program counter */
		m68020_get_sp,						/* Return the current stack pointer */
		m68020_set_sp,						/* Set the current stack pointer */
		m68020_get_reg,						/* Get a specific register value */
		m68020_set_reg,						/* Set a specific register value */
		m68020_set_nmi_line,				/* Set state of the NMI line */
		m68020_set_irq_line,				/* Set state of the IRQ line */
		m68020_set_irq_callback,			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
		m68020_info,						/* Get formatted string for a specific register */
		m68020_dasm,						/* Disassemble one instruction */
		8,MC68020_INT_ACK_AUTOVECTOR,		/* Number of IRQ lines, default IRQ vector */
        &m68020_ICount,                     /* Pointer to the instruction count */
		MC68020_INT_NONE,					/* Interrupt types: none, IRQ, NMI */
		-1,
		-1,
		cpu_readmem24,						/* Memory read */
		cpu_writemem24, 					/* Memory write */
		cpu_setOPbase24,					/* Update CPU opcode base */
		0,24,CPU_IS_BE,2,10,				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_24,ABITS2_24,ABITS_MIN_24	/* Address bits, for the memory system */
    },
#endif
#if (HAS_T11)
    {
		CPU_T11,							/* CPU number and family cores sharing resources */
        t11_reset,                          /* Reset CPU */
		t11_exit,							/* Shut down the CPU */
        t11_execute,                        /* Execute a number of cycles */
		t11_get_context,					/* Get the contents of the registers */
		t11_set_context,					/* Set the contents of the registers */
		t11_get_pc,							/* Return the current program counter */
		t11_set_pc,							/* Set the current program counter */
		t11_get_sp,							/* Return the current stack pointer */
		t11_set_sp,							/* Set the current stack pointer */
		t11_get_reg, 						/* Get a specific register value */
		t11_set_reg, 						/* Set a specific register value */
        t11_set_nmi_line,                   /* Set state of the NMI line */
		t11_set_irq_line,					/* Set state of the IRQ line */
		t11_set_irq_callback,				/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        t11_info,                           /* Get formatted string for a specific register */
		t11_dasm,							/* Disassemble one instruction */
		4,0,								/* Number of IRQ lines, default IRQ vector */
		&t11_ICount,						/* Pointer to the instruction count */
		T11_INT_NONE,						/* Interrupt types: none, IRQ, NMI */
		-1,
		-1,
		cpu_readmem16lew,					/* Memory read */
		cpu_writemem16lew,					/* Memory write */
		cpu_setOPbase16lew, 				/* Update CPU opcode base */
		0,16,CPU_IS_LE,2,6, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16LEW,ABITS2_16LEW,ABITS_MIN_16LEW  /* Address bits, for the memory system */
	},
#endif
#if (HAS_S2650)
    {
		CPU_S2650,							/* CPU number and family cores sharing resources */
        s2650_reset,                        /* Reset CPU */
		s2650_exit, 						/* Shut down the CPU */
		s2650_execute,						/* Execute a number of cycles */
		s2650_get_context,					/* Get the contents of the registers */
		s2650_set_context,					/* Set the contents of the registers */
		s2650_get_pc,						/* Return the current program counter */
		s2650_set_pc,						/* Set the current program counter */
		s2650_get_sp,						/* Return the current stack pointer */
		s2650_set_sp,						/* Set the current stack pointer */
		s2650_get_reg,						/* Get a specific register value */
		s2650_set_reg,						/* Set a specific register value */
        s2650_set_nmi_line,                 /* Set state of the NMI line */
		s2650_set_irq_line, 				/* Set state of the IRQ line */
		s2650_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		s2650_state_save,					/* Save CPU state */
		s2650_state_load,					/* Load CPU state */
        s2650_info,                         /* Get formatted string for a specific register */
		s2650_dasm, 						/* Disassemble one instruction */
		2,0,								/* Number of IRQ lines, default IRQ vector */
		&s2650_ICount,						/* Pointer to the instruction count */
		S2650_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		-1,
		-1,
		cpu_readmem16,                      /* Memory read */
		cpu_writemem16,                     /* Memory write */
		cpu_setOPbase16,                    /* Update CPU opcode base */
		0,15,CPU_IS_LE,1,3, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
	},
#endif
#if (HAS_TMS34010)
    {
		CPU_TMS34010,						/* CPU number and family cores sharing resources */
        tms34010_reset,                     /* Reset CPU */
		tms34010_exit,						/* Shut down the CPU */
		tms34010_execute,					/* Execute a number of cycles */
		tms34010_get_context,				/* Get the contents of the registers */
		tms34010_set_context,				/* Set the contents of the registers */
		tms34010_get_pc, 					/* Return the current program counter */
		tms34010_set_pc, 					/* Set the current program counter */
		tms34010_get_sp, 					/* Return the current stack pointer */
		tms34010_set_sp, 					/* Set the current stack pointer */
		tms34010_get_reg,					/* Get a specific register value */
		tms34010_set_reg,					/* Set a specific register value */
		tms34010_set_nmi_line,				/* Set state of the NMI line */
		tms34010_set_irq_line,				/* Set state of the IRQ line */
		tms34010_set_irq_callback,			/* Set IRQ enable/vector callback */
		tms34010_internal_interrupt,		/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
		tms34010_info,						/* Get formatted string for a specific register */
		tms34010_dasm,						/* Disassemble one instruction */
		2,0,								/* Number of IRQ lines, default IRQ vector */
		&tms34010_ICount,					/* Pointer to the instruction count */
		TMS34010_INT_NONE,					/* Interrupt types: none, IRQ, NMI */
		TMS34010_INT1,
		-1,
		cpu_readmem29,						/* Memory read */
		cpu_writemem29, 					/* Memory write */
		cpu_setOPbase29,					/* Update CPU opcode base */
		3,29,CPU_IS_LE,2,10,				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_29,ABITS2_29,ABITS_MIN_29	/* Address bits, for the memory system */
	},
#endif
#if (HAS_TMS9900)
    {
		CPU_TMS9900,						/* CPU number and family cores sharing resources */
        tms9900_reset,                      /* Reset CPU */
		tms9900_exit,						/* Shut down the CPU */
		tms9900_execute,					/* Execute a number of cycles */
		tms9900_get_context,				/* Get the contents of the registers */
		tms9900_set_context,				/* Set the contents of the registers */
		tms9900_get_pc,						/* Return the current program counter */
		tms9900_set_pc,						/* Set the current program counter */
		tms9900_get_sp,						/* Return the current stack pointer */
		tms9900_set_sp,						/* Set the current stack pointer */
		tms9900_get_reg, 					/* Get a specific register value */
		tms9900_set_reg, 					/* Set a specific register value */
		tms9900_set_nmi_line,				/* Set state of the NMI line */
		tms9900_set_irq_line,				/* Set state of the IRQ line */
		tms9900_set_irq_callback,			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
		tms9900_info,						/* Get formatted string for a specific register */
		tms9900_dasm,						/* Disassemble one instruction */
		1,0,								/* Number of IRQ lines, default IRQ vector */
		&tms9900_ICount,					/* Pointer to the instruction count */
		TMS9900_NONE,						/* Interrupt types: none, IRQ, NMI */
		-1,
		-1,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,16,CPU_IS_BE,2,6, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
        ABITS1_16,ABITS2_16,ABITS_MIN_16    /* Address bits, for the memory system */
	},
#endif
#if (HAS_Z8000)
    {
		CPU_Z8000,							/* CPU number and family cores sharing resources */
        z8000_reset,                        /* Reset CPU */
		z8000_exit, 						/* Shut down the CPU */
		z8000_execute,						/* Execute a number of cycles */
		z8000_get_context,					/* Get the contents of the registers */
		z8000_set_context,					/* Set the contents of the registers */
		z8000_get_pc,						/* Return the current program counter */
		z8000_set_pc,						/* Set the current program counter */
		z8000_get_sp,						/* Return the current stack pointer */
		z8000_set_sp,						/* Set the current stack pointer */
		z8000_get_reg,						/* Get a specific register value */
		z8000_set_reg,						/* Set a specific register value */
        z8000_set_nmi_line,                 /* Set state of the NMI line */
		z8000_set_irq_line, 				/* Set state of the IRQ line */
		z8000_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        z8000_info,                         /* Get formatted string for a specific register */
		z8000_dasm, 						/* Disassemble one instruction */
		2,0,								/* Number of IRQ lines, default IRQ vector */
		&z8000_ICount,						/* Pointer to the instruction count */
		Z8000_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		Z8000_NVI,
		Z8000_NMI,
		cpu_readmem16,                      /* Memory read */
		cpu_writemem16,                     /* Memory write */
		cpu_setOPbase16,                    /* Update CPU opcode base */
		0,16,CPU_IS_LE,2,6, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
#endif
#if (HAS_TMS320C10)
    {
		CPU_TMS320C10,						/* CPU number and family cores sharing resources */
        tms320c10_reset,                    /* Reset CPU */
		tms320c10_exit, 					/* Shut down the CPU */
		tms320c10_execute,					/* Execute a number of cycles */
		tms320c10_get_context,				/* Get the contents of the registers */
		tms320c10_set_context,				/* Set the contents of the registers */
		tms320c10_get_pc,					/* Return the current program counter */
		tms320c10_set_pc,					/* Set the current program counter */
		tms320c10_get_sp,					/* Return the current stack pointer */
		tms320c10_set_sp,					/* Set the current stack pointer */
		tms320c10_get_reg,					/* Get a specific register value */
		tms320c10_set_reg,					/* Set a specific register value */
		tms320c10_set_nmi_line, 			/* Set state of the NMI line */
		tms320c10_set_irq_line, 			/* Set state of the IRQ line */
		tms320c10_set_irq_callback, 		/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
		tms320c10_info, 					/* Get formatted string for a specific register */
		tms320c10_dasm, 					/* Disassemble one instruction */
		2,0,								/* Number of IRQ lines, default IRQ vector */
		&tms320c10_ICount,					/* Pointer to the instruction count */
		TMS320C10_INT_NONE, 				/* Interrupt types: none, IRQ, NMI */
		-1,
		-1,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		-1,16,CPU_IS_BE,2,4,				/* CPU address shift, bits, endianess, align unit, max. instruction length */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
#endif
#if (HAS_CCPU)
    {
		CPU_CCPU,							/* CPU number and family cores sharing resources */
        ccpu_reset,                         /* Reset CPU  */
		ccpu_exit,							/* Shut down CPU  */
		ccpu_execute,						/* Execute a number of cycles  */
		ccpu_get_context,					/* Get the contents of the registers */
		ccpu_set_context,					/* Set the contents of the registers */
		ccpu_get_pc, 						/* Return the current program counter */
		ccpu_set_pc, 						/* Set the current program counter */
		ccpu_get_sp, 						/* Return the current stack pointer */
		ccpu_set_sp, 						/* Set the current stack pointer */
		ccpu_get_reg,						/* Get a specific register value */
		ccpu_set_reg,						/* Set a specific register value */
        ccpu_set_nmi_line,                  /* Set state of the NMI line */
		ccpu_set_irq_line,					/* Set state of the IRQ line */
		ccpu_set_irq_callback,				/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
		ccpu_info,							/* Get formatted string for a specific register */
		ccpu_dasm,							/* Disassemble one instruction */
		2,0,								/* Number of IRQ lines, default IRQ vector */
		&ccpu_ICount,						/* Pointer to the instruction count  */
		0,									/* Interrupt types: none, IRQ, NMI	*/
		-1,
		-1,
		cpu_readmem16,						/* Memory read	*/
		cpu_writemem16, 					/* Memory write  */
		cpu_setOPbase16,					/* Update CPU opcode base  */
		0,15,CPU_IS_LE,1,3, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	 */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system	*/
	},
#endif
#if (HAS_PDP1)
    {
		CPU_PDP1,							/* CPU number and family cores sharing resources */
        pdp1_reset,                         /* Reset CPU  */
		pdp1_exit,							/* Shut down CPU  */
		pdp1_execute,						/* Execute a number of cycles  */
		pdp1_get_context,					/* Get the contents of the registers */
		pdp1_set_context,					/* Set the contents of the registers */
		pdp1_get_pc, 						/* Return the current program counter */
		pdp1_set_pc, 						/* Set the current program counter */
		pdp1_get_sp, 						/* Return the current stack pointer */
		pdp1_set_sp, 						/* Set the current stack pointer */
		pdp1_get_reg,						/* Get a specific register value */
		pdp1_set_reg,						/* Set a specific register value */
        pdp1_set_nmi_line,                  /* Set state of the NMI line */
		pdp1_set_irq_line,					/* Set state of the IRQ line */
		pdp1_set_irq_callback,				/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
		pdp1_info,							/* Get formatted string for a specific register */
		pdp1_dasm,							/* Disassemble one instruction */
		0,0,								/* Number of IRQ lines, default IRQ vector */
		&pdp1_ICount,						/* Pointer to the instruction count  */
		0,									/* Interrupt types: none, IRQ, NMI	*/
		-1,
		-1,
		cpu_readmem16,						/* Memory read	*/
		cpu_writemem16, 					/* Memory write  */
		cpu_setOPbase16,					/* Update CPU opcode base  */
		0,18,CPU_IS_LE,1,3, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	 */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system	*/
    },
#endif
#if (HAS_KONAMI)
    {
		CPU_KONAMI,							/* CPU number and family cores sharing resources */
        konami_reset,                       /* Reset CPU */
		konami_exit, 						/* Shut down the CPU */
        konami_execute,                     /* Execute a number of cycles */
		konami_get_context,					/* Get the contents of the registers */
		konami_set_context,					/* Set the contents of the registers */
		konami_get_pc,						/* Return the current program counter */
		konami_set_pc,						/* Set the current program counter */
		konami_get_sp,						/* Return the current stack pointer */
		konami_set_sp,						/* Set the current stack pointer */
		konami_get_reg,						/* Get a specific register value */
		konami_set_reg,						/* Set a specific register value */
        konami_set_nmi_line,                /* Set state of the NMI line */
		konami_set_irq_line, 				/* Set state of the IRQ line */
		konami_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		konami_state_save,					/* Save CPU state */
		konami_state_load,					/* Load CPU state */
        konami_info,                        /* Get formatted string for a specific register */
		konami_dasm, 						/* Disassemble one instruction */
		2,0,								/* Number of IRQ lines, default IRQ vector */
		&konami_ICount,						/* Pointer to the instruction count */
		KONAMI_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		KONAMI_INT_IRQ,
		KONAMI_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		0,16,CPU_IS_BE,1,4, 				/* CPU address shift, bits, endianess, align unit, max. instruction length	*/
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
#endif
};

void cpu_init(void)
{
	int i;

	/* Verify the order of entries in the cpuintf[] array */
	for( i = 0; i < CPU_COUNT; i++ )
	{
		if( cpuintf[i].cpu_num != i )
		{
			fprintf( stderr, "CPU #%d [%s] wrong ID %d: check enum CPU_... in src/driver.h!\n", i, cputype_name(i), cpuintf[i].cpu_num);
			exit(1);
		}
	}

    /* count how many CPUs we have to emulate */
	totalcpu = 0;

	while (totalcpu < MAX_CPU)
	{
		if( CPU_TYPE(totalcpu) == CPU_DUMMY ) break;
        totalcpu++;
	}

	/* zap the CPU data structure */
	memset (cpu, 0, sizeof (cpu));

	/* Set up the interface functions */
    for (i = 0; i < MAX_CPU; i++)
        cpu[i].intf = &cpuintf[CPU_TYPE(i)];

	/* reset the timer system */
	timer_init ();
	timeslice_timer = refresh_timer = vblank_timer = NULL;
}

void cpu_run(void)
{
	int i;

    /* determine which CPUs need a context switch */
	for (i = 0; i < totalcpu; i++)
	{
		int j, size;

        /* allocate a context buffer for the CPU */
		size = GETCONTEXT(i,NULL);
        if( size == 0 )
        {
            /* That can't really be true */
			fprintf( stderr, "CPU #%d claims to need no context buffer!\n", i);
            raise( SIGABRT );
        }

        cpu[i].context = malloc( size );
        if( cpu[i].context == NULL )
        {
            /* That's really bad :( */
			fprintf( stderr, "CPU #%d failed to allocate context buffer (%d bytes)!\n", i, size);
            raise( SIGABRT );
        }

		/* Zap the context buffer */
		memset(cpu[i].context, 0, size );


        /* Save if there is another CPU of the same type */
		cpu[i].save_context = 0;

		for (j = 0; j < totalcpu; j++)
			if ( i != j && !strcmp(cpunum_core_file(i),cpunum_core_file(j)) )
				cpu[i].save_context = 1;

		#ifdef MAME_DEBUG

		/* or if we're running with the debugger */
		{
			extern int mame_debug;
			cpu[i].save_context |= mame_debug;
		}

		#endif

        for( j = 0; j < MAX_IRQ_LINES; j++ )
		{
			irq_line_state[i * MAX_IRQ_LINES + j] = CLEAR_LINE;
            irq_line_vector[i * MAX_IRQ_LINES + j] = cpuintf[CPU_TYPE(i)].default_vector;
		}
    }

#ifdef	MAME_DEBUG
	/* Initialize the debugger */
	if( mame_debug )
		mame_debug_init();
#endif

reset:
	/* initialize the various timers (suspends all CPUs at startup) */
	cpu_inittimers ();
	watchdog_counter = -1;

	/* reset sound chips */
	sound_reset();

	/* enable all CPUs (except for audio CPUs if the sound is off) */
	for (i = 0; i < totalcpu; i++)
	{
        if (!CPU_AUDIO(i) || Machine->sample_rate != 0)
		{
			cpu_running[i] = 1;
			timer_suspendcpu (i, 0);
		}
		else
		{
			cpu_running[i] = 0;
			timer_suspendcpu (i, 1);
		}
	}

	have_to_reset = 0;
	hiscoreloaded = 0;
	vblank = 0;

if (errorlog) fprintf(errorlog,"Machine reset\n");

	/* start with interrupts enabled, so the generic routine will work even if */
	/* the machine doesn't have an interrupt enable port */
	for (i = 0;i < MAX_CPU;i++)
	{
		interrupt_enable[i] = 1;
		interrupt_vector[i] = 0xff;
	}

	/* do this AFTER the above so init_machine() can use cpu_halt() to hold the */
	/* execution of some CPUs, or disable interrupts */
	if (Machine->drv->init_machine) (*Machine->drv->init_machine)();

	/* reset each CPU */
	for (i = 0; i < totalcpu; i++)
	{
		/* swap memory contexts and reset */
		memorycontextswap (i);
		if (cpu[i].save_context) SETCONTEXT (i, cpu[i].context);
		activecpu = i;
		RESET (i);

        /* Set the irq callback for the cpu */
		SETIRQCALLBACK(i,cpu_irq_callbacks[i]);

        /* save the CPU context if necessary */
		if (cpu[i].save_context) GETCONTEXT (i, cpu[i].context);

        /* reset the total number of cycles */
		cpu[i].totalcycles = 0;
    }

	/* reset the globals */
	cpu_vblankreset ();
	current_frame = 0;

	/* loop until the user quits */
	usres = 0;
	while (usres == 0)
	{
		int cpunum;

		/* was machine_reset() called? */
		if (have_to_reset)
		{
#ifdef MESS
            if (Machine->drv->stop_machine) (*Machine->drv->stop_machine)();
#endif
            goto reset;
		}
		profiler_mark(PROFILER_EXTRA);

#if SAVE_STATE_TEST
        {
            if( keyboard_pressed_memory(KEYCODE_S) )
            {
                void *s = state_create(Machine->gamedrv->name);
                if( s )
                {
					for( cpunum = 0; cpunum < totalcpu; cpunum++ )
					{
						activecpu = cpunum;
						memorycontextswap (activecpu);
						if (cpu[activecpu].save_context) SETCONTEXT (activecpu, cpu[activecpu].context);
						/* make sure any bank switching is reset */
						SET_OP_BASE (activecpu, GETPC (activecpu));
						if( cpu[activecpu].intf->cpu_state_save )
							(*cpu[activecpu].intf->cpu_state_save)(s);
					}
                    state_close(s);
                }
            }

            if( keyboard_pressed_memory(KEYCODE_L) )
            {
                void *s = state_open(Machine->gamedrv->name);
                if( s )
                {
					for( cpunum = 0; cpunum < totalcpu; cpunum++ )
                    {
						activecpu = cpunum;
						memorycontextswap (activecpu);
						if (cpu[activecpu].save_context) SETCONTEXT (activecpu, cpu[activecpu].context);
						/* make sure any bank switching is reset */
						SET_OP_BASE (activecpu, GETPC (activecpu));
						if( cpu[activecpu].intf->cpu_state_load )
							(*cpu[activecpu].intf->cpu_state_load)(s);
						/* update the contexts */
						if (cpu[activecpu].save_context) GETCONTEXT (activecpu, cpu[activecpu].context);
						updatememorybase (activecpu);
                    }
                    state_close(s);
                }
            }
        }
#endif
        /* ask the timer system to schedule */
		if (timer_schedule_cpu (&cpunum, &cycles_running))
		{
			int ran;


			/* switch memory and CPU contexts */
			activecpu = cpunum;
			memorycontextswap (activecpu);
			if (cpu[activecpu].save_context) SETCONTEXT (activecpu, cpu[activecpu].context);

			/* make sure any bank switching is reset */
			SET_OP_BASE (activecpu, GETPC (activecpu));

            /* run for the requested number of cycles */
			profiler_mark(PROFILER_CPU1 + cpunum);
			ran = EXECUTE (activecpu, cycles_running);
			profiler_mark(PROFILER_END);

			/* update based on how many cycles we really ran */
			cpu[activecpu].totalcycles += ran;

			/* update the contexts */
			if (cpu[activecpu].save_context) GETCONTEXT (activecpu, cpu[activecpu].context);
			updatememorybase (activecpu);
			activecpu = -1;

			/* update the timer with how long we actually ran */
			timer_update_cpu (cpunum, ran);
		}

		profiler_mark(PROFILER_END);
	}

	/* write hi scores to disk - No scores saving if cheat */
	if (hiscoreloaded != 0 && Machine->gamedrv->hiscore_save)
		(*Machine->gamedrv->hiscore_save)();

#ifdef MESS
    if (Machine->drv->stop_machine) (*Machine->drv->stop_machine)();
#endif

#ifdef	MAME_DEBUG
	/* Shut down the debugger */
    if( mame_debug )
		mame_debug_exit();
#endif

    /* shut down the CPU cores */
	for (i = 0; i < totalcpu; i++)
	{
		/* if the CPU core defines an exit function, call it now */
        if( cpu[i].intf->exit )
			(*cpu[i].intf->exit)();

        /* free the context buffer for that CPU */
        if( cpu[i].context )
		{
			free( cpu[i].context );
			cpu[i].context = NULL;
		}
	}
	totalcpu = 0;
}




/***************************************************************************

  Use this function to initialize, and later maintain, the watchdog. For
  convenience, when the machine is reset, the watchdog is disabled. If you
  call this function, the watchdog is initialized, and from that point
  onwards, if you don't call it at least once every 10 video frames, the
  machine will be reset.

***************************************************************************/
void watchdog_reset_w(int offset,int data)
{
	watchdog_counter = Machine->drv->frames_per_second;
}

int watchdog_reset_r(int offset)
{
	watchdog_counter = Machine->drv->frames_per_second;
	return 0;
}



/***************************************************************************

  This function resets the machine (the reset will not take place
  immediately, it will be performed at the end of the active CPU's time
  slice)

***************************************************************************/
void machine_reset(void)
{
	/* write hi scores to disk - No scores saving if cheat */
	if (hiscoreloaded != 0 && Machine->gamedrv->hiscore_save)
		(*Machine->gamedrv->hiscore_save)();

	hiscoreloaded = 0;

	have_to_reset = 1;
}



/***************************************************************************

  Use this function to reset a specified CPU immediately

***************************************************************************/
void cpu_reset(int cpunum)
{
	timer_set (TIME_NOW, cpunum, cpu_resetcallback);
}


/***************************************************************************

  Use this function to stop and restart CPUs

***************************************************************************/
void cpu_halt(int cpunum,int running)
{
	if (cpunum >= MAX_CPU) return;

	/* don't resume audio CPUs if sound is disabled */
	if (!CPU_AUDIO(cpunum) || Machine->sample_rate != 0)
	{
		cpu_running[cpunum] = running;
		timer_suspendcpu (cpunum, !running);
	}
}



/***************************************************************************

  This function returns CPUNUM current status  (running or halted)

***************************************************************************/
int cpu_getstatus(int cpunum)
{
	if (cpunum >= MAX_CPU) return 0;

	return cpu_running[cpunum];
}



int cpu_getactivecpu(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return cpunum;
}

void cpu_setactivecpu(int cpunum)
{
	activecpu = cpunum;
}

int cpu_gettotalcpu(void)
{
	return totalcpu;
}



unsigned cpu_get_pc(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return GETPC (cpunum);
}

void cpu_set_pc(unsigned val)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	SETPC (cpunum,val);
}

unsigned cpu_get_sp(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return GETSP (cpunum);
}

void cpu_set_sp(unsigned val)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	SETSP (cpunum,val);
}

/* these are available externally, for the timer system */
int cycles_currently_ran(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return cycles_running - ICOUNT (cpunum);
}

int cycles_left_to_run(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return ICOUNT (cpunum);
}



/***************************************************************************

  Returns the number of CPU cycles since the last reset of the CPU

  IMPORTANT: this value wraps around in a relatively short time.
  For example, for a 6Mhz CPU, it will wrap around in
  2^32/6000000 = 716 seconds = 12 minutes.
  Make sure you don't do comparisons between values returned by this
  function, but only use the difference (which will be correct regardless
  of wraparound).

***************************************************************************/
int cpu_gettotalcycles(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return cpu[cpunum].totalcycles + cycles_currently_ran();
}



/***************************************************************************

  Returns the number of CPU cycles before the next interrupt handler call

***************************************************************************/
int cpu_geticount(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	int result = TIME_TO_CYCLES (cpunum, cpu[cpunum].vblankint_period - timer_timeelapsed (cpu[cpunum].vblankint_timer));
	return (result < 0) ? 0 : result;
}



/***************************************************************************

  Returns the number of CPU cycles before the end of the current video frame

***************************************************************************/
int cpu_getfcount(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	int result = TIME_TO_CYCLES (cpunum, refresh_period - timer_timeelapsed (refresh_timer));
	return (result < 0) ? 0 : result;
}



/***************************************************************************

  Returns the number of CPU cycles in one video frame

***************************************************************************/
int cpu_getfperiod(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return TIME_TO_CYCLES (cpunum, refresh_period);
}



/***************************************************************************

  Scales a given value by the ratio of fcount / fperiod

***************************************************************************/
int cpu_scalebyfcount(int value)
{
	int result = (int)((double)value * timer_timeelapsed (refresh_timer) * refresh_period_inv);
	if (value >= 0) return (result < value) ? result : value;
	else return (result > value) ? result : value;
}



/***************************************************************************

  Returns the current scanline, or the time until a specific scanline

  Note: cpu_getscanline() counts from 0, 0 being the first visible line. You
  might have to adjust this value to match the hardware, since in many cases
  the first visible line is >0.

***************************************************************************/
int cpu_getscanline(void)
{
	return (int)(timer_timeelapsed (refresh_timer) * scanline_period_inv);
}


double cpu_getscanlinetime(int scanline)
{
	double ret;
	double scantime = timer_starttime (refresh_timer) + (double)scanline * scanline_period;
	double abstime = timer_get_time ();
	if (abstime >= scantime) scantime += TIME_IN_HZ(Machine->drv->frames_per_second);
	ret = scantime - abstime;
	if (ret < TIME_IN_NSEC(1))
	{
		ret = TIME_IN_HZ (Machine->drv->frames_per_second);
	}

	return ret;
}


double cpu_getscanlineperiod(void)
{
	return scanline_period;
}


/***************************************************************************

  Returns the number of cycles in a scanline

 ***************************************************************************/
int cpu_getscanlinecycles(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return TIME_TO_CYCLES (cpunum, scanline_period);
}


/***************************************************************************

  Returns the number of cycles since the beginning of this frame

 ***************************************************************************/
int cpu_getcurrentcycles(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return TIME_TO_CYCLES (cpunum, timer_timeelapsed (refresh_timer));
}


/***************************************************************************

  Returns the current horizontal beam position in pixels

 ***************************************************************************/
int cpu_gethorzbeampos(void)
{
    double elapsed_time = timer_timeelapsed (refresh_timer);
    int scanline = (int)(elapsed_time * scanline_period_inv);
    double time_since_scanline = elapsed_time -
                         (double)scanline * scanline_period;
    return (int)(time_since_scanline * scanline_period_inv *
                         (double)Machine->drv->screen_width);
}


/***************************************************************************

  Returns the number of times the interrupt handler will be called before
  the end of the current video frame. This can be useful to interrupt
  handlers to synchronize their operation. If you call this from outside
  an interrupt handler, add 1 to the result, i.e. if it returns 0, it means
  that the interrupt handler will be called once.

***************************************************************************/
int cpu_getiloops(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return cpu[cpunum].iloops;
}



/***************************************************************************

  Interrupt handling

***************************************************************************/

/***************************************************************************

  These functions are called when a cpu calls the callback sent to it's
  set_irq_callback function. It clears the irq line if the current state
  is HOLD_LINE and returns the interrupt vector for that line.

***************************************************************************/
static int cpu_0_irq_callback(int irqline)
{
	if( irq_line_state[0 * MAX_IRQ_LINES + irqline] == HOLD_LINE )
	{
		SETIRQLINE(0, irqline, CLEAR_LINE);
		irq_line_state[0 * MAX_IRQ_LINES + irqline] = CLEAR_LINE;
    }
	LOG((errorlog, "cpu_0_irq_callback(%d) $%04x\n", irqline, irq_line_vector[0 * MAX_IRQ_LINES + irqline]));
	return irq_line_vector[0 * MAX_IRQ_LINES + irqline];
}

static int cpu_1_irq_callback(int irqline)
{
	if( irq_line_state[1 * MAX_IRQ_LINES + irqline] == HOLD_LINE )
	{
		SETIRQLINE(1, irqline, CLEAR_LINE);
		irq_line_state[1 * MAX_IRQ_LINES + irqline] = CLEAR_LINE;
    }
	LOG((errorlog, "cpu_1_irq_callback(%d) $%04x\n", irqline, irq_line_vector[1 * MAX_IRQ_LINES + irqline]));
	return irq_line_vector[1 * MAX_IRQ_LINES + irqline];
}

static int cpu_2_irq_callback(int irqline)
{
	if( irq_line_state[2 * MAX_IRQ_LINES + irqline] == HOLD_LINE )
	{
		SETIRQLINE(2, irqline, CLEAR_LINE);
		irq_line_state[2 * MAX_IRQ_LINES + irqline] = CLEAR_LINE;
	}
	LOG((errorlog, "cpu_2_irq_callback(%d) $%04x\n", irqline, irq_line_vector[2 * MAX_IRQ_LINES + irqline]));
	return irq_line_vector[2 * MAX_IRQ_LINES + irqline];
}

static int cpu_3_irq_callback(int irqline)
{
	if( irq_line_state[3 * MAX_IRQ_LINES + irqline] == HOLD_LINE )
	{
		SETIRQLINE(3, irqline, CLEAR_LINE);
		irq_line_state[3 * MAX_IRQ_LINES + irqline] = CLEAR_LINE;
	}
	LOG((errorlog, "cpu_3_irq_callback(%d) $%04x\n", irqline, irq_line_vector[2 * MAX_IRQ_LINES + irqline]));
	return irq_line_vector[3 * MAX_IRQ_LINES + irqline];
}

/***************************************************************************

  This function is used to generate internal interrupts (TMS34010)

***************************************************************************/
void cpu_generate_internal_interrupt(int cpunum, int type)
{
	timer_set (TIME_NOW, (cpunum & 7) | (type << 3), cpu_internalintcallback);
}


/***************************************************************************

  Use this functions to set the vector for a irq line of a CPU

***************************************************************************/
void cpu_irq_line_vector_w(int cpunum, int irqline, int vector)
{
	cpunum &= (MAX_CPU - 1);
	irqline &= (MAX_IRQ_LINES - 1);
	if( irqline < cpu[cpunum].intf->num_irqs )
	{
		LOG((errorlog,"cpu_irq_line_vector_w(%d,%d,$%04x)\n",cpunum,irqline,vector));
		irq_line_vector[cpunum * MAX_IRQ_LINES + irqline] = vector;
		return;
	}
	LOG((errorlog, "cpu_irq_line_vector_w CPU#%d irqline %d > max irq lines\n", cpunum, irqline));
}

/***************************************************************************

  Use these functions to set the vector (data) for a irq line (offset)
  of CPU #0 to #3

***************************************************************************/
void cpu_0_irq_line_vector_w(int offset, int data) { cpu_irq_line_vector_w(0, offset, data); }
void cpu_1_irq_line_vector_w(int offset, int data) { cpu_irq_line_vector_w(1, offset, data); }
void cpu_2_irq_line_vector_w(int offset, int data) { cpu_irq_line_vector_w(2, offset, data); }
void cpu_3_irq_line_vector_w(int offset, int data) { cpu_irq_line_vector_w(3, offset, data); }

/***************************************************************************

  Use this function to set the state the NMI line of a CPU

***************************************************************************/
void cpu_set_nmi_line(int cpunum, int state)
{
	/* don't trigger interrupts on suspended CPUs */
	if (cpu_getstatus(cpunum) == 0) return;

	LOG((errorlog,"cpu_set_nmi_line(%d,%d)\n",cpunum,state));
	timer_set (TIME_NOW, (cpunum & 7) | (state << 3), cpu_manualnmicallback);
}

/***************************************************************************

  Use this function to set the state of an IRQ line of a CPU
  The meaning of irqline varies between the different CPU types

***************************************************************************/
void cpu_set_irq_line(int cpunum, int irqline, int state)
{
	/* don't trigger interrupts on suspended CPUs */
	if (cpu_getstatus(cpunum) == 0) return;

	LOG((errorlog,"cpu_set_irq_line(%d,%d,%d)\n",cpunum,irqline,state));
	timer_set (TIME_NOW, (irqline & 7) | ((cpunum & 7) << 3) | (state << 6), cpu_manualirqcallback);
}

/***************************************************************************

  Use this function to cause an interrupt immediately (don't have to wait
  until the next call to the interrupt handler)

***************************************************************************/
void cpu_cause_interrupt(int cpunum,int type)
{
	/* don't trigger interrupts on suspended CPUs */
	if (cpu_getstatus(cpunum) == 0) return;

	timer_set (TIME_NOW, (cpunum & 7) | (type << 3), cpu_manualintcallback);
}



void cpu_clear_pending_interrupts(int cpunum)
{
	timer_set (TIME_NOW, cpunum, cpu_clearintcallback);
}



void interrupt_enable_w(int offset,int data)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	interrupt_enable[cpunum] = data;

	/* make sure there are no queued interrupts */
	if (data == 0) cpu_clear_pending_interrupts(cpunum);
}



void interrupt_vector_w(int offset,int data)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	if (interrupt_vector[cpunum] != data)
	{
		LOG((errorlog,"CPU#%d interrupt_vector_w $%02x\n", cpunum, data));
        interrupt_vector[cpunum] = data;

        /* make sure there are no queued interrupts */
		cpu_clear_pending_interrupts(cpunum);
	}
}



int interrupt(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	int val;

    if (interrupt_enable[cpunum] == 0)
		return INT_TYPE_NONE (cpunum);

    val = INT_TYPE_IRQ (cpunum);
    if (val == -1000)
		val = interrupt_vector[cpunum];

    return val;
}



int nmi_interrupt(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;

    if (interrupt_enable[cpunum] == 0)
		return INT_TYPE_NONE (cpunum);

    return INT_TYPE_NMI (cpunum);
}



int m68_level1_irq(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	if (interrupt_enable[cpunum] == 0) return MC68000_INT_NONE;
    return MC68000_IRQ_1;
}
int m68_level2_irq(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	if (interrupt_enable[cpunum] == 0) return MC68000_INT_NONE;
    return MC68000_IRQ_2;
}
int m68_level3_irq(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	if (interrupt_enable[cpunum] == 0) return MC68000_INT_NONE;
    return MC68000_IRQ_3;
}
int m68_level4_irq(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	if (interrupt_enable[cpunum] == 0) return MC68000_INT_NONE;
    return MC68000_IRQ_4;
}
int m68_level5_irq(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	if (interrupt_enable[cpunum] == 0) return MC68000_INT_NONE;
    return MC68000_IRQ_5;
}
int m68_level6_irq(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	if (interrupt_enable[cpunum] == 0) return MC68000_INT_NONE;
    return MC68000_IRQ_6;
}
int m68_level7_irq(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	if (interrupt_enable[cpunum] == 0) return MC68000_INT_NONE;
    return MC68000_IRQ_7;
}



int ignore_interrupt(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return INT_TYPE_NONE (cpunum);
}



/***************************************************************************

  CPU timing and synchronization functions.

***************************************************************************/

/* generate a trigger */
void cpu_trigger (int trigger)
{
	timer_trigger (trigger);
}

/* generate a trigger after a specific period of time */
void cpu_triggertime (double duration, int trigger)
{
	timer_set (duration, trigger, cpu_trigger);
}



/* burn CPU cycles until a timer trigger */
void cpu_spinuntil_trigger (int trigger)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	timer_suspendcpu_trigger (cpunum, trigger);
}

/* burn CPU cycles until the next interrupt */
void cpu_spinuntil_int (void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	cpu_spinuntil_trigger (TRIGGER_INT + cpunum);
}

/* burn CPU cycles until our timeslice is up */
void cpu_spin (void)
{
	cpu_spinuntil_trigger (TRIGGER_TIMESLICE);
}

/* burn CPU cycles for a specific period of time */
void cpu_spinuntil_time (double duration)
{
	static int timetrig = 0;

	cpu_spinuntil_trigger (TRIGGER_SUSPENDTIME + timetrig);
	cpu_triggertime (duration, TRIGGER_SUSPENDTIME + timetrig);
	timetrig = (timetrig + 1) & 255;
}



/* yield our timeslice for a specific period of time */
void cpu_yielduntil_trigger (int trigger)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	timer_holdcpu_trigger (cpunum, trigger);
}

/* yield our timeslice until the next interrupt */
void cpu_yielduntil_int (void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	cpu_yielduntil_trigger (TRIGGER_INT + cpunum);
}

/* yield our current timeslice */
void cpu_yield (void)
{
	cpu_yielduntil_trigger (TRIGGER_TIMESLICE);
}

/* yield our timeslice for a specific period of time */
void cpu_yielduntil_time (double duration)
{
	static int timetrig = 0;

	cpu_yielduntil_trigger (TRIGGER_YIELDTIME + timetrig);
	cpu_triggertime (duration, TRIGGER_YIELDTIME + timetrig);
	timetrig = (timetrig + 1) & 255;
}



int cpu_getvblank(void)
{
	return vblank;
}


int cpu_getcurrentframe(void)
{
	return current_frame;
}


/***************************************************************************

  Internal CPU event processors.

***************************************************************************/

static void cpu_manualnmicallback(int param)
{
	int cpunum, state, oldactive;
	cpunum = param & 7;
	state = param >> 3;

    /* swap to the CPU's context */
	oldactive = activecpu;
    activecpu = cpunum;
    memorycontextswap (activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT (activecpu, cpu[activecpu].context);

	LOG((errorlog,"cpu_manualnmicallback %d,%d\n",cpunum,state));

	switch (state)
	{
        case PULSE_LINE:
			SETNMILINE(cpunum,ASSERT_LINE);
			SETNMILINE(cpunum,CLEAR_LINE);
            break;
        case HOLD_LINE:
        case ASSERT_LINE:
			SETNMILINE(cpunum,ASSERT_LINE);
            break;
        case CLEAR_LINE:
			SETNMILINE(cpunum,CLEAR_LINE);
            break;
        default:
			if( errorlog ) fprintf( errorlog, "cpu_manualnmicallback cpu #%d unknown state %d\n", cpunum, state);
    }
    /* update the CPU's context */
	if (cpu[activecpu].save_context) GETCONTEXT (activecpu, cpu[activecpu].context);
	activecpu = oldactive;
	if (activecpu >= 0) memorycontextswap (activecpu);

	/* generate a trigger to unsuspend any CPUs waiting on the interrupt */
	if (state != CLEAR_LINE)
        timer_trigger (TRIGGER_INT + cpunum);
}

static void cpu_manualirqcallback(int param)
{
	int cpunum, irqline, state, oldactive;

    irqline = param & 7;
	cpunum = (param >> 3) & 7;
	state = param >> 6;

    /* swap to the CPU's context */
	oldactive = activecpu;
    activecpu = cpunum;
    memorycontextswap (activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT (activecpu, cpu[activecpu].context);

	LOG((errorlog,"cpu_manualirqcallback %d,%d,%d\n",cpunum,irqline,state));

	irq_line_state[cpunum * MAX_IRQ_LINES + irqline] = state;
	switch (state)
	{
        case PULSE_LINE:
			SETIRQLINE(cpunum,irqline,ASSERT_LINE);
			SETIRQLINE(cpunum,irqline,CLEAR_LINE);
            break;
        case HOLD_LINE:
        case ASSERT_LINE:
			SETIRQLINE(cpunum,irqline,ASSERT_LINE);
            break;
        case CLEAR_LINE:
			SETIRQLINE(cpunum,irqline,CLEAR_LINE);
            break;
        default:
			if( errorlog ) fprintf( errorlog, "cpu_manualirqcallback cpu #%d, line %d, unknown state %d\n", cpunum, irqline, state);
    }

    /* update the CPU's context */
	if (cpu[activecpu].save_context) GETCONTEXT (activecpu, cpu[activecpu].context);
	activecpu = oldactive;
	if (activecpu >= 0) memorycontextswap (activecpu);

	/* generate a trigger to unsuspend any CPUs waiting on the interrupt */
	if (state != CLEAR_LINE)
        timer_trigger (TRIGGER_INT + cpunum);
}

static void cpu_internal_interrupt (int cpunum, int type)
{
    int oldactive = activecpu;

    /* swap to the CPU's context */
    activecpu = cpunum;
    memorycontextswap (activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT (activecpu, cpu[activecpu].context);

	INTERNAL_INTERRUPT (cpunum, type);

    /* update the CPU's context */
	if (cpu[activecpu].save_context) GETCONTEXT (activecpu, cpu[activecpu].context);
    activecpu = oldactive;
    if (activecpu >= 0) memorycontextswap (activecpu);

    /* generate a trigger to unsuspend any CPUs waiting on the interrupt */
    timer_trigger (TRIGGER_INT + cpunum);
}

static void cpu_internalintcallback (int param)
{
	int type = param >> 3;
    int cpunum = param & 7;

	LOG((errorlog,"CPU#%d internal interrupt type $%04x\n", cpunum, type));
    /* generate the interrupt */
	cpu_internal_interrupt(cpunum, type);
}

static void cpu_generate_interrupt (int cpunum, int (*func)(void), int num)
{
	int oldactive = activecpu;

	if (!cpu_running[cpunum]) return;

	/* swap to the CPU's context */
    activecpu = cpunum;
	memorycontextswap (activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT (activecpu, cpu[activecpu].context);

	/* cause the interrupt, calling the function if it exists */
	if (func) num = (*func)();

	/* wrapper for the new interrupt system */
	if (num != INT_TYPE_NONE(cpunum))
	{
		LOG((errorlog,"CPU#%d interrupt type $%04x: ", cpunum, num));
		/* is it the NMI type interrupt of that CPU? */
		if (num == INT_TYPE_NMI(cpunum))
		{

            LOG((errorlog,"NMI\n"));
			cpu_manualnmicallback (cpunum | (PULSE_LINE << 3) );

		}
		else
		{
			int irq_line;

			switch (CPU_TYPE(cpunum))
			{
#if (HAS_Z80)
            case CPU_Z80:               irq_line = 0; LOG((errorlog,"Z80 IRQ\n")); break;
#endif
#if (HAS_8080)
            case CPU_8080:
				switch (num)
				{
				case I8080_INTR:		irq_line = 0; LOG((errorlog,"I8080 INTR\n")); break;
				default:				irq_line = 0; LOG((errorlog,"I8080 unknown\n"));
				}
                break;
#endif
#if (HAS_8085A)
            case CPU_8085A:
				switch (num)
				{
				case I8085_INTR:		irq_line = 0; LOG((errorlog,"I8085 INTR\n")); break;
				case I8085_RST55:		irq_line = 1; LOG((errorlog,"I8085 RST55\n")); break;
				case I8085_RST65:		irq_line = 2; LOG((errorlog,"I8085 RST65\n")); break;
				case I8085_RST75:		irq_line = 3; LOG((errorlog,"I8085 RST75\n")); break;
				default:				irq_line = 0; LOG((errorlog,"I8085 unknown\n"));
				}
				break;
#endif
#if (HAS_M6502)
            case CPU_M6502:             irq_line = 0; LOG((errorlog,"M6502 IRQ\n")); break;
#endif
#if (HAS_M65C02)
            case CPU_M65C02:            irq_line = 0; LOG((errorlog,"M65C02 IRQ\n")); break;
#endif
#if (HAS_M6510)
            case CPU_M6510:             irq_line = 0; LOG((errorlog,"M6510 IRQ\n")); break;
#endif
#if (HAS_N2A03)
            case CPU_N2A03:             irq_line = 0; LOG((errorlog,"N2A03 IRQ\n")); break;
#endif
#if (HAS_H6280)
            case CPU_H6280:
                switch (num)
                {
				case H6280_INT_IRQ1:	irq_line = 0; LOG((errorlog,"H6280 INT 1\n")); break;
				case H6280_INT_IRQ2:	irq_line = 1; LOG((errorlog,"H6280 INT 2\n")); break;
				case H6280_INT_TIMER:	irq_line = 2; LOG((errorlog,"H6280 TIMER INT\n")); break;
				default:				irq_line = 0; LOG((errorlog,"H6280 unknown\n"));
                }
                break;
#endif
#if (HAS_I86)
            case CPU_I86:               irq_line = 0; LOG((errorlog,"I86 IRQ\n")); break;
#endif
#if (HAS_V20)
            case CPU_V20:               irq_line = 0; LOG((errorlog,"V20 IRQ\n")); break;
#endif
#if (HAS_V30)
            case CPU_V30:               irq_line = 0; LOG((errorlog,"V30 IRQ\n")); break;
#endif
#if (HAS_V33)
            case CPU_V33:               irq_line = 0; LOG((errorlog,"V33 IRQ\n")); break;
#endif
#if (HAS_I8035)
            case CPU_I8035:             irq_line = 0; LOG((errorlog,"I8035 IRQ\n")); break;
#endif
#if (HAS_I8039)
            case CPU_I8039:             irq_line = 0; LOG((errorlog,"I8039 IRQ\n")); break;
#endif
#if (HAS_I8048)
            case CPU_I8048:             irq_line = 0; LOG((errorlog,"I8048 IRQ\n")); break;
#endif
#if (HAS_N7751)
            case CPU_N7751:             irq_line = 0; LOG((errorlog,"N7751 IRQ\n")); break;
#endif
#if (HAS_M6800)
			case CPU_M6800: 			irq_line = 0; LOG((errorlog,"M6800 IRQ\n")); break;
#endif
#if (HAS_M6801)
			case CPU_M6801:				irq_line = 0; LOG((errorlog,"M6801 IRQ\n")); break;
#endif
#if (HAS_M6802)
            case CPU_M6802:				irq_line = 0; LOG((errorlog,"M6802 IRQ\n")); break;
#endif
#if (HAS_M6803)
            case CPU_M6803:				irq_line = 0; LOG((errorlog,"M6803 IRQ\n")); break;
#endif
#if (HAS_M6808)
            case CPU_M6808:				irq_line = 0; LOG((errorlog,"M6808 IRQ\n")); break;
#endif
#if (HAS_HD63701)
            case CPU_HD63701:			irq_line = 0; LOG((errorlog,"HD63701 IRQ\n")); break;
#endif
#if (HAS_M6805)
            case CPU_M6805:             irq_line = 0; LOG((errorlog,"M6805 IRQ\n")); break;
#endif
#if (HAS_M68705)
            case CPU_M68705:            irq_line = 0; LOG((errorlog,"M68705 IRQ\n")); break;
#endif
#if (HAS_HD63705)
			case CPU_HD63705:			irq_line = 0; LOG((errorlog,"HD68705 IRQ\n")); break;
#endif
#if (HAS_M6309)
            case CPU_M6309:
				switch (num)
				{
				case M6309_INT_IRQ: 	irq_line = 0; LOG((errorlog,"M6309 IRQ\n")); break;
				case M6309_INT_FIRQ:	irq_line = 1; LOG((errorlog,"M6309 FIRQ\n")); break;
				default:				irq_line = 0; LOG((errorlog,"M6309 unknown\n"));
				}
                break;
#endif
#if (HAS_M6809)
            case CPU_M6809:
				switch (num)
				{
				case M6809_INT_IRQ: 	irq_line = 0; LOG((errorlog,"M6809 IRQ\n")); break;
				case M6809_INT_FIRQ:	irq_line = 1; LOG((errorlog,"M6809 FIRQ\n")); break;
				default:				irq_line = 0; LOG((errorlog,"M6809 unknown\n"));
				}
				break;
#endif
#if (HAS_KONAMI)
				case CPU_KONAMI:
				switch (num)
				{
				case KONAMI_INT_IRQ:	irq_line = 0; LOG((errorlog,"KONAMI IRQ\n")); break;
				case KONAMI_INT_FIRQ:	irq_line = 1; LOG((errorlog,"KONAMI FIRQ\n")); break;
				default:				irq_line = 0; LOG((errorlog,"KONAMI unknown\n"));
				}
				break;
#endif
#if (HAS_M68000)
            case CPU_M68000:
				switch (num)
                {
                case MC68000_IRQ_1:     irq_line = 1; LOG((errorlog,"M68K IRQ1\n")); break;
                case MC68000_IRQ_2:     irq_line = 2; LOG((errorlog,"M68K IRQ2\n")); break;
                case MC68000_IRQ_3:     irq_line = 3; LOG((errorlog,"M68K IRQ3\n")); break;
                case MC68000_IRQ_4:     irq_line = 4; LOG((errorlog,"M68K IRQ4\n")); break;
                case MC68000_IRQ_5:     irq_line = 5; LOG((errorlog,"M68K IRQ5\n")); break;
                case MC68000_IRQ_6:     irq_line = 6; LOG((errorlog,"M68K IRQ6\n")); break;
                case MC68000_IRQ_7:     irq_line = 7; LOG((errorlog,"M68K IRQ7\n")); break;
                default:                irq_line = 0; LOG((errorlog,"M68K unknown\n"));
                }
                /* until now only auto vector interrupts supported */
                num = MC68000_INT_ACK_AUTOVECTOR;
                break;
#endif
#if (HAS_M68010)
            case CPU_M68010:
                switch (num)
                {
                case MC68010_IRQ_1:     irq_line = 1; LOG((errorlog,"M68010 IRQ1\n")); break;
                case MC68010_IRQ_2:     irq_line = 2; LOG((errorlog,"M68010 IRQ2\n")); break;
                case MC68010_IRQ_3:     irq_line = 3; LOG((errorlog,"M68010 IRQ3\n")); break;
                case MC68010_IRQ_4:     irq_line = 4; LOG((errorlog,"M68010 IRQ4\n")); break;
                case MC68010_IRQ_5:     irq_line = 5; LOG((errorlog,"M68010 IRQ5\n")); break;
                case MC68010_IRQ_6:     irq_line = 6; LOG((errorlog,"M68010 IRQ6\n")); break;
                case MC68010_IRQ_7:     irq_line = 7; LOG((errorlog,"M68010 IRQ7\n")); break;
                default:                irq_line = 0; LOG((errorlog,"M68010 unknown\n"));
                }
                /* until now only auto vector interrupts supported */
                num = MC68000_INT_ACK_AUTOVECTOR;
                break;
#endif
#if (HAS_M68020)
			case CPU_M68020:
				switch (num)
                {
				case MC68020_IRQ_1: 	irq_line = 1; LOG((errorlog,"M68020 IRQ1\n")); break;
				case MC68020_IRQ_2: 	irq_line = 2; LOG((errorlog,"M68020 IRQ2\n")); break;
				case MC68020_IRQ_3: 	irq_line = 3; LOG((errorlog,"M68020 IRQ3\n")); break;
				case MC68020_IRQ_4: 	irq_line = 4; LOG((errorlog,"M68020 IRQ4\n")); break;
				case MC68020_IRQ_5: 	irq_line = 5; LOG((errorlog,"M68020 IRQ5\n")); break;
				case MC68020_IRQ_6: 	irq_line = 6; LOG((errorlog,"M68020 IRQ6\n")); break;
				case MC68020_IRQ_7: 	irq_line = 7; LOG((errorlog,"M68020 IRQ7\n")); break;
				default:				irq_line = 0; LOG((errorlog,"M68020 unknown\n"));
                }
                /* until now only auto vector interrupts supported */
                num = MC68000_INT_ACK_AUTOVECTOR;
                break;
#endif
#if HAS_T11
            case CPU_T11:
				switch (num)
				{
				case T11_IRQ0:			irq_line = 0; LOG((errorlog,"T11 IRQ0\n")); break;
				case T11_IRQ1:			irq_line = 1; LOG((errorlog,"T11 IRQ1\n")); break;
				case T11_IRQ2:			irq_line = 2; LOG((errorlog,"T11 IRQ2\n")); break;
				case T11_IRQ3:			irq_line = 3; LOG((errorlog,"T11 IRQ3\n")); break;
				default:				irq_line = 0; LOG((errorlog,"T11 unknown\n"));
                }
                break;
#endif
#if HAS_S2650
            case CPU_S2650:             irq_line = 0; LOG((errorlog,"S2650 IRQ\n")); break;
#endif
#if HAS_TMS34010
            case CPU_TMS34010:
				switch (num)
				{
				case TMS34010_INT1: 	irq_line = 0; LOG((errorlog,"TMS34010 INT1\n")); break;
				case TMS34010_INT2: 	irq_line = 1; LOG((errorlog,"TMS34010 INT2\n")); break;
				default:				irq_line = 0; LOG((errorlog,"TMS34010 unknown\n"));
                }
                break;
#endif
#if HAS_TMS9900
			case CPU_TMS9900:	irq_line = 0; LOG((errorlog,"TMS9900 IRQ\n")); break;
#endif
#if HAS_Z8000
            case CPU_Z8000:
				switch (num)
				{
				case Z8000_NVI: 		irq_line = 0; LOG((errorlog,"Z8000 NVI\n")); break;
				case Z8000_VI:			irq_line = 1; LOG((errorlog,"Z8000 VI\n")); break;
				default:				irq_line = 0; LOG((errorlog,"Z8000 unknown\n"));
                }
                break;
#endif
#if HAS_TMS320C10
            case CPU_TMS320C10:
				switch (num)
				{
				case TMS320C10_ACTIVE_INT:	irq_line = 0; LOG((errorlog,"TMS32010 INT\n")); break;
				case TMS320C10_ACTIVE_BIO:	irq_line = 1; LOG((errorlog,"TMS32010 BIO\n")); break;
				default:					irq_line = 0; LOG((errorlog,"TMS32010 unknown\n"));
                }
                break;
#endif
			default:
				irq_line = 0;
				/* else it should be an IRQ type; assume line 0 and store vector */
				LOG((errorlog,"unknown IRQ\n"));
            }
			cpu_irq_line_vector_w(cpunum, irq_line, num);
			cpu_manualirqcallback (irq_line | (cpunum << 3) | (HOLD_LINE << 6) );
        }
	}

    /* update the CPU's context */
	if (cpu[activecpu].save_context) GETCONTEXT (activecpu, cpu[activecpu].context);
    activecpu = oldactive;
    if (activecpu >= 0) memorycontextswap (activecpu);

	/* trigger already generated by cpu_manualirqcallback or cpu_manualnmicallback */
}

static void cpu_clear_interrupts (int cpunum)
{
	int oldactive = activecpu;
	int i;

	/* swap to the CPU's context */
	activecpu = cpunum;
	memorycontextswap (activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT (activecpu, cpu[activecpu].context);

	/* clear NMI line */
	SETNMILINE(activecpu,CLEAR_LINE);

    /* clear all IRQ lines */
    for (i = 0; i < cpu[activecpu].intf->num_irqs; i++)
		SETIRQLINE(activecpu,i,CLEAR_LINE);

	/* update the CPU's context */
	if (cpu[activecpu].save_context) GETCONTEXT (activecpu, cpu[activecpu].context);
	activecpu = oldactive;
	if (activecpu >= 0) memorycontextswap (activecpu);
}


static void cpu_reset_cpu (int cpunum)
{
	int oldactive = activecpu;

	/* swap to the CPU's context */
	activecpu = cpunum;
	memorycontextswap (activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT (activecpu, cpu[activecpu].context);

	/* reset the CPU */
	RESET (cpunum);

    /* Set the irq callback for the cpu */
	SETIRQCALLBACK(cpunum,cpu_irq_callbacks[cpunum]);

	/* update the CPU's context */
	if (cpu[activecpu].save_context) GETCONTEXT (activecpu, cpu[activecpu].context);
	activecpu = oldactive;
	if (activecpu >= 0) memorycontextswap (activecpu);
}

/***************************************************************************

  Interrupt callback. This is called once per CPU interrupt by either the
  VBLANK handler or by the CPU's own timer directly, depending on whether
  or not the CPU's interrupts are synced to VBLANK.

***************************************************************************/
static void cpu_vblankintcallback (int param)
{
	if (Machine->drv->cpu[param].vblank_interrupt)
		cpu_generate_interrupt (param, Machine->drv->cpu[param].vblank_interrupt, 0);

	/* update the counters */
	cpu[param].iloops--;
}


static void cpu_timedintcallback (int param)
{
	/* bail if there is no routine */
	if (!Machine->drv->cpu[param].timed_interrupt)
		return;

	/* generate the interrupt */
	cpu_generate_interrupt (param, Machine->drv->cpu[param].timed_interrupt, 0);
}


static void cpu_manualintcallback (int param)
{
	int intnum = param >> 3;
	int cpunum = param & 7;

	/* generate the interrupt */
	cpu_generate_interrupt (cpunum, 0, intnum);
}


static void cpu_clearintcallback (int param)
{
	/* clear the interrupts */
	cpu_clear_interrupts (param);
}


static void cpu_resetcallback (int param)
{
	/* reset the CPU */
	cpu_reset_cpu (param);
}



/***************************************************************************

  VBLANK reset. Called at the start of emulation and once per VBLANK in
  order to update the input ports and reset the interrupt counter.

***************************************************************************/
static void cpu_vblankreset (void)
{
	int i;

	/* read hi scores from disk */
	if (hiscoreloaded == 0 && Machine->gamedrv->hiscore_load)
		hiscoreloaded = (*Machine->gamedrv->hiscore_load)();

	/* read keyboard & update the status of the input ports */
	update_input_ports ();

	/* reset the cycle counters */
	for (i = 0; i < totalcpu; i++)
	{
		if (!timer_iscpususpended (i))
			cpu[i].iloops = Machine->drv->cpu[i].vblank_interrupts_per_frame - 1;
		else
			cpu[i].iloops = -1;
	}
}


/***************************************************************************

  VBLANK callback. This is called 'vblank_multipler' times per frame to
  service VBLANK-synced interrupts and to begin the screen update process.

***************************************************************************/
static void cpu_firstvblankcallback (int param)
{
	/* now that we're synced up, pulse from here on out */
	vblank_timer = timer_pulse (vblank_period, param, cpu_vblankcallback);

	/* but we need to call the standard routine as well */
	cpu_vblankcallback (param);
}

/* note that calling this with param == -1 means count everything, but call no subroutines */
static void cpu_vblankcallback (int param)
{
	int i;

	/* loop over CPUs */
	for (i = 0; i < totalcpu; i++)
	{
		/* if the interrupt multiplier is valid */
		if (cpu[i].vblankint_multiplier != -1)
		{
			/* decrement; if we hit zero, generate the interrupt and reset the countdown */
			if (!--cpu[i].vblankint_countdown)
			{
				if (param != -1)
					cpu_vblankintcallback (i);
				cpu[i].vblankint_countdown = cpu[i].vblankint_multiplier;
				timer_reset (cpu[i].vblankint_timer, TIME_NEVER);
			}
		}

		/* else reset the VBLANK timer if this is going to be a real VBLANK */
		else if (vblank_countdown == 1)
			timer_reset (cpu[i].vblankint_timer, TIME_NEVER);
	}

	/* is it a real VBLANK? */
	if (!--vblank_countdown)
	{
		/* do we update the screen now? */
		if (Machine->drv->video_attributes & VIDEO_UPDATE_BEFORE_VBLANK)
			usres = updatescreen();

		/* Set the timer to update the screen */
		timer_set (TIME_IN_USEC (Machine->drv->vblank_duration), 0, cpu_updatecallback);
		vblank = 1;

		/* reset the globals */
		cpu_vblankreset ();

		/* reset the counter */
		vblank_countdown = vblank_multiplier;
	}
}


/***************************************************************************

  Video update callback. This is called a game-dependent amount of time
  after the VBLANK in order to trigger a video update.

***************************************************************************/
static void cpu_updatecallback (int param)
{
	/* update the screen if we didn't before */
	if (!(Machine->drv->video_attributes & VIDEO_UPDATE_BEFORE_VBLANK))
		usres = updatescreen();
	vblank = 0;

	/* update IPT_VBLANK input ports */
	inputport_vblank_end();

	/* check the watchdog */
	if (watchdog_counter > 0)
	{
		if (--watchdog_counter == 0)
		{
if (errorlog) fprintf(errorlog,"reset caused by the watchdog\n");
			machine_reset ();
		}
	}

	current_frame++;

	/* reset the refresh timer */
	timer_reset (refresh_timer, TIME_NEVER);
}


/***************************************************************************

  Converts an integral timing rate into a period. Rates can be specified
  as follows:

        rate > 0       -> 'rate' cycles per frame
        rate == 0      -> 0
        rate >= -10000 -> 'rate' cycles per second
        rate < -10000  -> 'rate' nanoseconds

***************************************************************************/
static double cpu_computerate (int value)
{
	/* values equal to zero are zero */
	if (value <= 0)
		return 0.0;

	/* values above between 0 and 50000 are in Hz */
	if (value < 50000)
		return TIME_IN_HZ (value);

	/* values greater than 50000 are in nanoseconds */
	else
		return TIME_IN_NSEC (value);
}


static void cpu_timeslicecallback (int param)
{
	timer_trigger (TRIGGER_TIMESLICE);
}


/***************************************************************************

  Initializes all the timers used by the CPU system.

***************************************************************************/
static void cpu_inittimers (void)
{
	double first_time;
	int i, max, ipf;

	/* remove old timers */
	if (timeslice_timer)
		timer_remove (timeslice_timer);
	if (refresh_timer)
		timer_remove (refresh_timer);
	if (vblank_timer)
		timer_remove (vblank_timer);

	/* allocate a dummy timer at the minimum frequency to break things up */
	ipf = Machine->drv->cpu_slices_per_frame;
	if (ipf <= 0)
		ipf = 1;
	timeslice_period = TIME_IN_HZ (Machine->drv->frames_per_second * ipf);
	timeslice_timer = timer_pulse (timeslice_period, 0, cpu_timeslicecallback);

	/* allocate an infinite timer to track elapsed time since the last refresh */
	refresh_period = TIME_IN_HZ (Machine->drv->frames_per_second);
	refresh_period_inv = 1.0 / refresh_period;
	refresh_timer = timer_set (TIME_NEVER, 0, NULL);

	/* while we're at it, compute the scanline times */
	if (Machine->drv->vblank_duration)
		scanline_period = (refresh_period - TIME_IN_USEC (Machine->drv->vblank_duration)) /
				(double)(Machine->drv->visible_area.max_y - Machine->drv->visible_area.min_y + 1);
	else
		scanline_period = refresh_period / (double)Machine->drv->screen_height;
	scanline_period_inv = 1.0 / scanline_period;

	/*
	 *		The following code finds all the CPUs that are interrupting in sync with the VBLANK
	 *		and sets up the VBLANK timer to run at the minimum number of cycles per frame in
	 *		order to service all the synced interrupts
	 */

	/* find the CPU with the maximum interrupts per frame */
	max = 1;
	for (i = 0; i < totalcpu; i++)
	{
		ipf = Machine->drv->cpu[i].vblank_interrupts_per_frame;
		if (ipf > max)
			max = ipf;
	}

	/* now find the LCD with the rest of the CPUs (brute force - these numbers aren't huge) */
	vblank_multiplier = max;
	while (1)
	{
		for (i = 0; i < totalcpu; i++)
		{
			ipf = Machine->drv->cpu[i].vblank_interrupts_per_frame;
			if (ipf > 0 && (vblank_multiplier % ipf) != 0)
				break;
		}
		if (i == totalcpu)
			break;
		vblank_multiplier += max;
	}

	/* initialize the countdown timers and intervals */
	for (i = 0; i < totalcpu; i++)
	{
		ipf = Machine->drv->cpu[i].vblank_interrupts_per_frame;
		if (ipf > 0)
			cpu[i].vblankint_countdown = cpu[i].vblankint_multiplier = vblank_multiplier / ipf;
		else
			cpu[i].vblankint_countdown = cpu[i].vblankint_multiplier = -1;
	}

	/* allocate a vblank timer at the frame rate * the LCD number of interrupts per frame */
	vblank_period = TIME_IN_HZ (Machine->drv->frames_per_second * vblank_multiplier);
	vblank_timer = timer_pulse (vblank_period, 0, cpu_vblankcallback);
	vblank_countdown = vblank_multiplier;

	/*
	 *		The following code creates individual timers for each CPU whose interrupts are not
	 *		synced to the VBLANK, and computes the typical number of cycles per interrupt
	 */

	/* start the CPU interrupt timers */
	for (i = 0; i < totalcpu; i++)
	{
		ipf = Machine->drv->cpu[i].vblank_interrupts_per_frame;

		/* remove old timers */
		if (cpu[i].vblankint_timer)
			timer_remove (cpu[i].vblankint_timer);
		if (cpu[i].timedint_timer)
			timer_remove (cpu[i].timedint_timer);

		/* compute the average number of cycles per interrupt */
		if (ipf <= 0)
			ipf = 1;
		cpu[i].vblankint_period = TIME_IN_HZ (Machine->drv->frames_per_second * ipf);
		cpu[i].vblankint_timer = timer_set (TIME_NEVER, 0, NULL);

		/* see if we need to allocate a CPU timer */
		ipf = Machine->drv->cpu[i].timed_interrupts_per_second;
		if (ipf)
		{
			cpu[i].timedint_period = cpu_computerate (ipf);
			cpu[i].timedint_timer = timer_pulse (cpu[i].timedint_period, i, cpu_timedintcallback);
		}
	}

	/* note that since we start the first frame on the refresh, we can't pulse starting
	   immediately; instead, we back up one VBLANK period, and inch forward until we hit
	   positive time. That time will be the time of the first VBLANK timer callback */
	timer_remove (vblank_timer);

	first_time = -TIME_IN_USEC (Machine->drv->vblank_duration) + vblank_period;
	while (first_time < 0)
	{
		cpu_vblankcallback (-1);
		first_time += vblank_period;
	}
	vblank_timer = timer_set (first_time, 0, cpu_firstvblankcallback);
}


/* AJP 981016 */
int cpu_is_saving_context(int _activecpu)
{
	return (cpu[_activecpu].save_context);
}


/* JB 971019 */
void* cpu_getcontext (int _activecpu)
{
	return cpu[_activecpu].context;
}


/***************************************************************************
  Retrieve or set the entire context of the active CPU
***************************************************************************/

unsigned cpu_get_context(void *context)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return GETCONTEXT(cpunum,context);
}

void cpu_set_context(void *context)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	SETCONTEXT(cpunum,context);
}

/***************************************************************************
  Retrieve or set the value of a specific register of the active CPU
***************************************************************************/

unsigned cpu_get_reg(int regnum)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return GETREG(cpunum,regnum);
}

void cpu_set_reg(int regnum, unsigned val)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	SETREG(cpunum,regnum,val);
}

/***************************************************************************

  Get various CPU information

***************************************************************************/

/***************************************************************************
  Returns the number of address bits for the active CPU
***************************************************************************/
unsigned cpu_address_bits(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return cpuintf[CPU_TYPE(cpunum)].address_bits;
}

/***************************************************************************
  Returns the address bit mask for the active CPU
***************************************************************************/
unsigned cpu_address_mask(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return (1 << cpuintf[CPU_TYPE(cpunum)].address_bits) - 1;
}

/***************************************************************************
  Returns the address shift factor for the active CPU
***************************************************************************/
int cpu_address_shift(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return cpuintf[CPU_TYPE(cpunum)].address_shift;
}

/***************************************************************************
  Returns the endianess for the active CPU
***************************************************************************/
unsigned cpu_endianess(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return cpuintf[CPU_TYPE(cpunum)].endianess;
}

/***************************************************************************
  Returns the code align unit for the active CPU (1 byte, 2 word, ...)
***************************************************************************/
unsigned cpu_align_unit(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return cpuintf[CPU_TYPE(cpunum)].align_unit;
}

/***************************************************************************
  Returns the max. instruction length for the active CPU
***************************************************************************/
unsigned cpu_max_inst_len(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return cpuintf[CPU_TYPE(cpunum)].max_inst_len;
}

/***************************************************************************
  Returns the name for the active CPU
***************************************************************************/
const char *cpu_name(void)
{
	if( activecpu >= 0 )
		return CPUINFO(activecpu,NULL,CPU_INFO_NAME);
	return "";
}

/***************************************************************************
  Returns the family name for the active CPU
***************************************************************************/
const char *cpu_core_family(void)
{
	if( activecpu >= 0 )
		return CPUINFO(activecpu,NULL,CPU_INFO_FAMILY);
    return "";
}

/***************************************************************************
  Returns the version number for the active CPU
***************************************************************************/
const char *cpu_core_version(void)
{
	if( activecpu >= 0 )
		return CPUINFO(activecpu,NULL,CPU_INFO_VERSION);
    return "";
}

/***************************************************************************
  Returns the core filename for the active CPU
***************************************************************************/
const char *cpu_core_file(void)
{
	if( activecpu >= 0 )
		return CPUINFO(activecpu,NULL,CPU_INFO_FILE);
    return "";
}

/***************************************************************************
  Returns the credits for the active CPU
***************************************************************************/
const char *cpu_core_credits(void)
{
	if( activecpu >= 0 )
		return CPUINFO(activecpu,NULL,CPU_INFO_CREDITS);
    return "";
}

/***************************************************************************
  Returns the register layout for the active CPU (debugger)
***************************************************************************/
const char *cpu_reg_layout(void)
{
	if( activecpu >= 0 )
		return CPUINFO(activecpu,NULL,CPU_INFO_REG_LAYOUT);
    return "";
}

/***************************************************************************
  Returns the window layout for the active CPU (debugger)
***************************************************************************/
const char *cpu_win_layout(void)
{
	if( activecpu >= 0 )
		return CPUINFO(activecpu,NULL,CPU_INFO_WIN_LAYOUT);
    return "";
}

/***************************************************************************
  Returns a dissassembled instruction for the active CPU
***************************************************************************/
unsigned cpu_dasm(char *buffer, unsigned pc)
{
    if( activecpu >= 0 )
		return CPUDASM(activecpu,buffer,pc);
	return 0;
}

/***************************************************************************
  Returns a flags (state, condition codes) string for the active CPU
***************************************************************************/
const char *cpu_flags(void)
{
	if( activecpu >= 0 )
		return CPUINFO(activecpu,NULL,CPU_INFO_FLAGS);
	return "";
}

/***************************************************************************
  Returns a specific register string for the currently active CPU
***************************************************************************/
const char *cpu_dump_reg(int regnum)
{
	if( activecpu >= 0 )
		return CPUINFO(activecpu,NULL,CPU_INFO_REG+regnum);
	return "";
}

/***************************************************************************
  Returns a state dump for the currently active CPU
***************************************************************************/
const char *cpu_dump_state(void)
{
	static char buffer[1024+1];
	unsigned addr_width = (cpu_address_bits() + 3) / 4;
	char *dst = buffer;
	const char *src, *regs;
	int width;

	dst += sprintf(dst, "CPU #%d [%s]\n", activecpu, cputype_name(CPU_TYPE(activecpu)));
	width = 0;
	regs = cpu_reg_layout();
	while( *regs )
	{
		if( *regs == -1 )
		{
			dst += sprintf(dst, "\n");
			width = 0;
		}
		else
		{
			src = cpu_dump_reg( *regs );
			if( *src )
			{
				if( width + strlen(src) + 1 >= 80 )
				{
					dst += sprintf(dst, "\n");
					width = 0;
				}
				dst += sprintf(dst, "%s ", src);
				width += strlen(src) + 1;
			}
		}
		regs++;
	}
	dst += sprintf(dst, "\n%0*X: ", addr_width, cpu_get_pc());
	cpu_dasm( dst, cpu_get_pc() );
	strcat(dst, "\n\n");

    return buffer;
}

/***************************************************************************
  Returns the number of address bits for a specific CPU type
***************************************************************************/
unsigned cputype_address_bits(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return cpuintf[cpu_type].address_bits;
	return 0;
}

/***************************************************************************
  Returns the address bit mask for a specific CPU type
***************************************************************************/
unsigned cputype_address_mask(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return (1 << cpuintf[cpu_type].address_bits) - 1;
    return 0;
}

/***************************************************************************
  Returns the address shift factor for a specific CPU type
***************************************************************************/
int cputype_address_shift(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return cpuintf[cpu_type].address_shift;
    return 0;
}

/***************************************************************************
  Returns the endianess for a specific CPU type
***************************************************************************/
unsigned cputype_endianess(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return cpuintf[cpu_type].endianess;
    return 0;
}

/***************************************************************************
  Returns the code align unit for a speciific CPU type (1 byte, 2 word, ...)
***************************************************************************/
unsigned cputype_align_unit(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return cpuintf[cpu_type].align_unit;
    return 0;
}

/***************************************************************************
  Returns the max. instruction length for a specific CPU type
***************************************************************************/
unsigned cputype_max_inst_len(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return cpuintf[cpu_type].max_inst_len;
    return 0;
}

/***************************************************************************
  Returns the name for a specific CPU type
***************************************************************************/
const char *cputype_name(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return IFC_INFO(cpu_type,NULL,CPU_INFO_NAME);
	return "";
}

/***************************************************************************
  Returns the family name for a specific CPU type
***************************************************************************/
const char *cputype_core_family(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return IFC_INFO(cpu_type,NULL,CPU_INFO_FAMILY);
    return "";
}

/***************************************************************************
  Returns the version number for a specific CPU type
***************************************************************************/
const char *cputype_core_version(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return IFC_INFO(cpu_type,NULL,CPU_INFO_VERSION);
    return "";
}

/***************************************************************************
  Returns the core filename for a specific CPU type
***************************************************************************/
const char *cputype_core_file(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return IFC_INFO(cpu_type,NULL,CPU_INFO_FILE);
    return "";
}

/***************************************************************************
  Returns the credits for a specific CPU type
***************************************************************************/
const char *cputype_core_credits(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return IFC_INFO(cpu_type,NULL,CPU_INFO_CREDITS);
    return "";
}

/***************************************************************************
  Returns the register layout for a specific CPU type (debugger)
***************************************************************************/
const char *cputype_reg_layout(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return IFC_INFO(cpu_type,NULL,CPU_INFO_REG_LAYOUT);
    return "";
}

/***************************************************************************
  Returns the window layout for a specific CPU type (debugger)
***************************************************************************/
const char *cputype_win_layout(int cpu_type)
{
    cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < CPU_COUNT )
		return IFC_INFO(cpu_type,NULL,CPU_INFO_WIN_LAYOUT);

    /* just in case... */
	return (const char *)default_win_layout;
}

/***************************************************************************
  Returns the number of address bits for a specific CPU number
***************************************************************************/
unsigned cpunum_address_bits(int cpunum)
{
	if( cpunum < totalcpu )
		return cputype_address_bits(CPU_TYPE(cpunum));
	return 0;
}

/***************************************************************************
  Returns the address bit mask for a specific CPU number
***************************************************************************/
unsigned cpunum_address_mask(int cpunum)
{
    if( cpunum < totalcpu )
		return cputype_address_mask(CPU_TYPE(cpunum));
	return 0;
}

/***************************************************************************
  Returns the endianess for a specific CPU number
***************************************************************************/
unsigned cpunum_endianess(int cpunum)
{
    if( cpunum < totalcpu )
		return cputype_endianess(CPU_TYPE(cpunum));
	return 0;
}

/***************************************************************************
  Returns the code align unit for the active CPU (1 byte, 2 word, ...)
***************************************************************************/
unsigned cpunum_align_unit(int cpunum)
{
    if( cpunum < totalcpu )
		return cputype_align_unit(CPU_TYPE(cpunum));
	return 0;
}

/***************************************************************************
  Returns the max. instruction length for a specific CPU number
***************************************************************************/
unsigned cpunum_max_inst_len(int cpunum)
{
    if( cpunum < totalcpu )
		return cputype_max_inst_len(CPU_TYPE(cpunum));
	return 0;
}

/***************************************************************************
  Returns the name for a specific CPU number
***************************************************************************/
const char *cpunum_name(int cpunum)
{
    if( cpunum < totalcpu )
        return cputype_name(CPU_TYPE(cpunum));
    return "";
}

/***************************************************************************
  Returns the family name for a specific CPU number
***************************************************************************/
const char *cpunum_core_family(int cpunum)
{
    if( cpunum < totalcpu )
        return cputype_core_family(CPU_TYPE(cpunum));
    return "";
}

/***************************************************************************
  Returns the core version for a specific CPU number
***************************************************************************/
const char *cpunum_core_version(int cpunum)
{
    if( cpunum < totalcpu )
        return cputype_core_version(CPU_TYPE(cpunum));
    return "";
}

/***************************************************************************
  Returns the core filename for a specific CPU number
***************************************************************************/
const char *cpunum_core_file(int cpunum)
{
    if( cpunum < totalcpu )
		return cputype_core_file(CPU_TYPE(cpunum));
    return "";
}

/***************************************************************************
  Returns the credits for a specific CPU number
***************************************************************************/
const char *cpunum_core_credits(int cpunum)
{
    if( cpunum < totalcpu )
		return cputype_core_credits(CPU_TYPE(cpunum));
    return "";
}

/***************************************************************************
  Returns (debugger) register layout for a specific CPU number
***************************************************************************/
const char *cpunum_reg_layout(int cpunum)
{
    if( cpunum < totalcpu )
		return cputype_reg_layout(CPU_TYPE(cpunum));
	return "";
}

/***************************************************************************
  Returns (debugger) window layout for a specific CPU number
***************************************************************************/
const char *cpunum_win_layout(int cpunum)
{
    if( cpunum < totalcpu )
		return cputype_win_layout(CPU_TYPE(cpunum));
	return (const char *)default_win_layout;
}

/***************************************************************************
  Return a register value for a specific CPU number of the running machine
***************************************************************************/
unsigned cpunum_get_reg(int cpunum, int regnum)
{
    int oldactive;
    unsigned val = 0;

	if( cpunum == activecpu )
		return cpu_get_reg( regnum );

    /* swap to the CPU's context */
    oldactive = activecpu;
    activecpu = cpunum;
    memorycontextswap (activecpu);
    if (cpu[activecpu].save_context) SETCONTEXT (activecpu, cpu[activecpu].context);

    val = GETREG(activecpu,regnum);

    /* update the CPU's context */
    if (cpu[activecpu].save_context) GETCONTEXT (activecpu, cpu[activecpu].context);
    activecpu = oldactive;
    if (activecpu >= 0) memorycontextswap (activecpu);

    return val;
}

/***************************************************************************
  Set a register value for a specific CPU number of the running machine
***************************************************************************/
void cpunum_set_reg(int cpunum, int regnum, unsigned val)
{
    int oldactive;

	if( cpunum == activecpu )
	{
		cpu_set_reg( regnum, val );
		return;
	}

    /* swap to the CPU's context */
    oldactive = activecpu;
    activecpu = cpunum;
    memorycontextswap (activecpu);
    if (cpu[activecpu].save_context) SETCONTEXT (activecpu, cpu[activecpu].context);

    SETREG(activecpu,regnum,val);

    /* update the CPU's context */
    if (cpu[activecpu].save_context) GETCONTEXT (activecpu, cpu[activecpu].context);
    activecpu = oldactive;
    if (activecpu >= 0) memorycontextswap (activecpu);
}

/***************************************************************************
  Return a dissassembled instruction for a specific CPU
***************************************************************************/
unsigned cpunum_dasm(int cpunum,char *buffer,unsigned pc)
{
	unsigned result;
	int oldactive;

	if( cpunum == activecpu )
		return cpu_dasm(buffer,pc);

    /* swap to the CPU's context */
	oldactive = activecpu;
    activecpu = cpunum;
    memorycontextswap (activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT (activecpu, cpu[activecpu].context);

	result = CPUDASM(activecpu,buffer,pc);

	/* update the CPU's context */
    if (cpu[activecpu].save_context) GETCONTEXT (activecpu, cpu[activecpu].context);
	activecpu = oldactive;
    if (activecpu >= 0) memorycontextswap (activecpu);

    return result;
}

/***************************************************************************
  Return a flags (state, condition codes) string for a specific CPU
***************************************************************************/
const char *cpunum_flags(int cpunum)
{
	const char *result;
	int oldactive;

	if( cpunum == activecpu )
		return cpu_flags();

    /* swap to the CPU's context */
	oldactive = activecpu;
    activecpu = cpunum;
    memorycontextswap (activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT (activecpu, cpu[activecpu].context);

	result = CPUINFO(activecpu,NULL,CPU_INFO_FLAGS);

	/* update the CPU's context */
    if (cpu[activecpu].save_context) GETCONTEXT (activecpu, cpu[activecpu].context);
	activecpu = oldactive;
    if (activecpu >= 0) memorycontextswap (activecpu);

    return result;
}

/***************************************************************************
  Return a specific register string for a specific CPU
***************************************************************************/
const char *cpunum_dump_reg(int cpunum, int regnum)
{
	const char *result;
	int oldactive;

	if( cpunum == activecpu )
		return cpu_dump_reg(regnum);

    /* swap to the CPU's context */
	oldactive = activecpu;
    activecpu = cpunum;
    memorycontextswap (activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT (activecpu, cpu[activecpu].context);

	result = CPUINFO(activecpu,NULL,CPU_INFO_REG+regnum);

	/* update the CPU's context */
    if (cpu[activecpu].save_context) GETCONTEXT (activecpu, cpu[activecpu].context);
	activecpu = oldactive;
    if (activecpu >= 0) memorycontextswap (activecpu);

    return result;
}

/***************************************************************************
  Return a state dump for a specific CPU
***************************************************************************/
const char *cpunum_dump_state(int cpunum)
{
	static char buffer[1024+1];
	int oldactive;

	/* swap to the CPU's context */
	oldactive = activecpu;
    activecpu = cpunum;
    memorycontextswap (activecpu);
	if (cpu[activecpu].save_context) SETCONTEXT (activecpu, cpu[activecpu].context);

	strcpy( buffer, cpu_dump_state() );

	/* update the CPU's context */
    if (cpu[activecpu].save_context) GETCONTEXT (activecpu, cpu[activecpu].context);
	activecpu = oldactive;
    if (activecpu >= 0) memorycontextswap (activecpu);

    return buffer;
}

/***************************************************************************
  Dump all CPU's state to stderr
***************************************************************************/
void cpu_dump_states(void)
{
	int i;

	for( i = 0; i < totalcpu; i++ )
	{
		fputs( cpunum_dump_state(i), stderr );
	}
	fflush(stderr);
}

/***************************************************************************

  Dummy interfaces for non-CPUs

***************************************************************************/
static void Dummy_reset(void *param) { }
static void Dummy_exit(void) { }
static int Dummy_execute(int cycles) { return cycles; }
static unsigned Dummy_get_context(void *regs) { return 0; }
static void Dummy_set_context(void *regs) { }
static unsigned Dummy_get_pc(void) { return 0; }
static void Dummy_set_pc(unsigned val) { }
static unsigned Dummy_get_sp(void) { return 0; }
static void Dummy_set_sp(unsigned val) { }
static unsigned Dummy_get_reg(int regnum) { return 0; }
static void Dummy_set_reg(int regnum, unsigned val) { }
static void Dummy_set_nmi_line(int state) { }
static void Dummy_set_irq_line(int irqline, int state) { }
static void Dummy_set_irq_callback(int (*callback)(int irqline)) { }

/****************************************************************************
 * Return a formatted string for a register
 ****************************************************************************/
static const char *Dummy_info(void *context, int regnum)
{
	if( !context && regnum )
		return "";

    switch (regnum)
	{
		case CPU_INFO_NAME: return "Dummy";
		case CPU_INFO_FAMILY: return "no CPU";
		case CPU_INFO_VERSION: return "0.0";
		case CPU_INFO_FILE: return __FILE__;
		case CPU_INFO_CREDITS: return "The MAME team.";
	}
	return "";
}

static unsigned Dummy_dasm(char *buffer, unsigned pc)
{
	strcpy(buffer, "???");
	return 1;
}

