/***************************************************************************

  cpuintrf.c

  Don't you love MS-DOS 8+3 names? That stands for CPU interface.
  Functions needed to interface the CPU emulator with the other parts of
  the emulation.

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/i8085/i8085.h"
#include "cpu/m6502/m6502.h"
#include "cpu/h6280/h6280.h"
#include "cpu/i86/i86intrf.h"
#include "cpu/i8039/i8039.h"
#include "cpu/m6808/m6808.h"
#include "cpu/m6805/m6805.h"
#include "cpu/m6809/m6809.h"
#include "cpu/m68000/m68000.h"
#include "cpu/t11/t11.h"
#include "cpu/s2650/s2650.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/tms9900/tms9900.h"
#include "cpu/z8000/z8000.h"
#include "cpu/tms32010/tms32010.h"
#include "cpu/ccpu/ccpu.h"
#ifdef MESS
#include "cpu/pdp1/pdp1.h"
#endif
#include "timer.h"


/* these are triggers sent to the timer system for various interrupt events */
#define TRIGGER_TIMESLICE       -1000
#define TRIGGER_INT             -2000
#define TRIGGER_YIELDTIME       -3000
#define TRIGGER_SUSPENDTIME     -4000

#define VERBOSE 1

#if VERBOSE
#define LOG(x)	if( errorlog ) fprintf x
#else
#define LOG(x)
#endif

#define CPUINFO_FIXED   (3*sizeof(void*)+5*sizeof(int)+2*sizeof(double))
#define CPUINFO_SIZE	(CPU_CONTEXT_SIZE-CPUINFO_FIXED)

struct cpuinfo
{
	struct cpu_interface *intf;                /* pointer to the interface functions */
	int iloops;                                /* number of interrupts remaining this frame */
	int totalcycles;                           /* total CPU cycles executed */
	int vblankint_countdown;                   /* number of vblank callbacks left until we interrupt */
	int vblankint_multiplier;                  /* number of vblank callbacks per interrupt */
	void *vblankint_timer;                     /* reference to elapsed time counter */
	double vblankint_period;                   /* timing period of the VBLANK interrupt */
	void *timedint_timer;                      /* reference to this CPU's timer */
	double timedint_period;                    /* timing period of the timed interrupt */
	int save_context;                          /* need to context switch this CPU? yes or no */
#ifdef __LP64__
	unsigned char context[CPUINFO_SIZE] __attribute__ ((__aligned__ (8)));
#else
	unsigned char context[CPUINFO_SIZE];	   /* this CPU's context */
#endif
};

static struct cpuinfo cpu[MAX_CPU];


static int activecpu,totalcpu;
static int running;	/* number of cycles that the CPU emulation was requested to run */
					/* (needed by cpu_getfcount) */
static int have_to_reset;
static int hiscoreloaded;

int previouspc;

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

/* Dummy interfaces for non-CPUs */
static void Dummy_reset(void *param);
static void Dummy_exit(void);
static int Dummy_execute(int cycles);
static void Dummy_setregs(void *Regs);
static void Dummy_getregs(void *Regs);
static unsigned Dummy_getpc(void);
static unsigned Dummy_getreg(int regnum);
static void Dummy_setreg(int regnum, unsigned val);
static void Dummy_set_nmi_line(int state);
static void Dummy_set_irq_line(int irqline, int state);
static void Dummy_set_irq_callback(int (*callback)(int irqline));
static int dummy_icount;
static const char *Dummy_info(void *context, int regnum);

/* pointers of daisy chain link */
static Z80_DaisyChain *Z80_daisychain[MAX_CPU];

/* Convenience macros - not in cpuintrf.h because they shouldn't be used by everyone */
#define RESET(index)					((*cpu[index].intf->reset)(Machine->drv->cpu[index].reset_param))
#define EXECUTE(index,cycles)           ((*cpu[index].intf->execute)(cycles))
#define SETREGS(index,regs)             ((*cpu[index].intf->set_regs)(regs))
#define GETREGS(index,regs)             ((*cpu[index].intf->get_regs)(regs))
#define GETPC(index)                    ((*cpu[index].intf->get_pc)())
#define GETREG(index,regnum)			((*cpu[index].intf->get_reg)(regnum))
#define SETREG(index,regnum,value)		((*cpu[index].intf->get_reg)(regnum,value))
#define SET_NMI_LINE(index,state)       ((*cpu[index].intf->set_nmi_line)(state))
#define SET_IRQ_LINE(index,line,state)	((*cpu[index].intf->set_irq_line)(line,state))
#define SET_IRQ_CALLBACK(index,callback) ((*cpu[index].intf->set_irq_callback)(callback))
#define INTERNAL_INTERRUPT(index,type)	if( cpu[index].intf->internal_interrupt ) ((*cpu[index].intf->internal_interrupt)(type))
#define CPU_INFO(index,context,regnum)	((*cpu[index].intf->cpu_info)(context,regnum))
#define ICOUNT(index)                   (*cpu[index].intf->icount)
#define INT_TYPE_NONE(index)            (cpu[index].intf->no_int)
#define INT_TYPE_IRQ(index) 			(cpu[index].intf->irq_int)
#define INT_TYPE_NMI(index)             (cpu[index].intf->nmi_int)
#define SET_OP_BASE(index,pc)           ((*cpu[index].intf->set_op_base)(pc))

#define CPU_TYPE(index) 				(Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK)
#define CPU_AUDIO(index)				(Machine->drv->cpu[index].cpu_type & CPU_AUDIO_CPU)

#define IFC_INFO(cpu,context,regnum)	((cpuintf[cpu].cpu_info)(context,regnum))

#define MAX_CPU_TYPE	(sizeof(cpuintf)/sizeof(cpuintf[0]))

/* warning these must match the defines in driver.h! */
struct cpu_interface cpuintf[] =
{
	/* Dummy CPU -- placeholder for type 0 */
	{
		Dummy_reset,						/* Reset CPU */
		Dummy_exit, 						/* Shut down the CPU */
		Dummy_execute,						/* Execute a number of cycles */
		(void (*)(void *))Dummy_setregs,	/* Set the contents of the registers */
		(void (*)(void *))Dummy_getregs,	/* Get the contents of the registers */
		Dummy_getpc,						/* Return the current PC */
        Dummy_getreg,                       /* Get a specific register value */
        Dummy_setreg,                       /* Set a specific register value */
		Dummy_set_nmi_line, 				/* Set state of the NMI line */
		Dummy_set_irq_line, 				/* Set state of the IRQ line */
		Dummy_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        Dummy_info,                         /* Get formatted string for a specific register */
		1,									/* Number of IRQ lines */
		&dummy_icount,						/* Pointer to the instruction count */
		0,									/* Interrupt types: none, IRQ, NMI */
		-1,
		-1,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
	},
	/* 1 CPU_Z80 */
	{
		z80_reset,							/* Reset CPU */
		z80_exit,							/* Shut down the CPU */
		z80_execute,						/* Execute a number of cycles */
		(void (*)(void *))z80_setregs,		/* Set the contents of the registers */
		(void (*)(void *))z80_getregs,		/* Get the contents of the registers */
		z80_getpc,							/* Return the current PC */
        z80_getreg,                         /* Get a specific register value */
		z80_setreg, 						/* Set a specific register value */
        z80_set_nmi_line,                   /* Set state of the NMI line */
		z80_set_irq_line,					/* Set state of the IRQ line */
		z80_set_irq_callback,				/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		z80_state_save, 					/* Save CPU state */
		z80_state_load, 					/* Load CPU state */
        z80_info,                           /* Get formatted string for a specific register */
        1,                                  /* Number of IRQ lines */
		&z80_ICount,						/* Pointer to the instruction count */
		Z80_IGNORE_INT, 					/* Interrupt types: none, IRQ, NMI */
		Z80_IRQ_INT,
		Z80_NMI_INT,
        cpu_readmem16,                      /* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
	},
	/* 2 CPU_8080 */
	{
		i8080_reset,						/* Reset CPU */
		i8080_exit, 						/* Shut down the CPU */
		i8080_execute,						/* Execute a number of cycles */
		(void (*)(void *))i8080_setregs,	/* Set the contents of the registers */
		(void (*)(void *))i8080_getregs,	/* Get the contents of the registers */
		i8080_getpc,						/* Return the current PC */
        i8080_getreg,                       /* Get a specific register value */
		i8080_setreg,						/* Set a specific register value */
        i8080_set_nmi_line,                 /* Set state of the NMI line */
		i8080_set_irq_line, 				/* Set state of the IRQ line */
		i8080_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		i8080_state_save,					/* Save CPU state */
		i8080_state_load,					/* Load CPU state */
        i8080_info,                         /* Get formatted string for a specific register */
        4,                                  /* Number of IRQ lines */
		&i8080_ICount,						/* Pointer to the instruction count */
		I8080_NONE, 						/* Interrupt types: none, IRQ, NMI */
		I8080_INTR,
		I8080_TRAP,
		cpu_readmem16,                      /* Memory read */
		cpu_writemem16,                     /* Memory write */
		cpu_setOPbase16,                    /* Update CPU opcode base */
		16,                                 /* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
	/* 3 CPU_8085A */
	{
		i8085_reset,						/* Reset CPU */
		i8085_exit, 						/* Shut down the CPU */
		i8085_execute,						/* Execute a number of cycles */
		(void (*)(void *))i8085_setregs,	/* Set the contents of the registers */
		(void (*)(void *))i8085_getregs,	/* Get the contents of the registers */
		i8085_getpc,						/* Return the current PC */
        i8085_getreg,                       /* Get a specific register value */
		i8085_setreg,						/* Set a specific register value */
        i8085_set_nmi_line,                 /* Set state of the NMI line */
		i8085_set_irq_line, 				/* Set state of the IRQ line */
		i8085_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		i8085_state_save,					/* Save CPU state */
		i8085_state_load,					/* Load CPU state */
        i8085_info,                         /* Get formatted string for a specific register */
        4,                                  /* Number of IRQ lines */
		&i8085_ICount,						/* Pointer to the instruction count */
		I8085_NONE, 						/* Interrupt types: none, IRQ, NMI */
		I8085_INTR,
		I8085_TRAP,
		cpu_readmem16,                      /* Memory read */
		cpu_writemem16,                     /* Memory write */
		cpu_setOPbase16,                    /* Update CPU opcode base */
		16,                                 /* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
	},
	/* 4 CPU_M6502 */
	{
		m6502_reset,						/* Reset CPU */
		m6502_exit, 						/* Shut down the CPU */
		m6502_execute,						/* Execute a number of cycles */
		(void (*)(void *))m6502_setregs,	/* Set the contents of the registers */
		(void (*)(void *))m6502_getregs,	/* Get the contents of the registers */
		m6502_getpc,						/* Return the current PC */
        m6502_getreg,                       /* Get a specific register value */
		m6502_setreg,						/* Set a specific register value */
        m6502_set_nmi_line,                 /* Set state of the NMI line */
		m6502_set_irq_line, 				/* Set state of the IRQ line */
		m6502_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		m6502_state_save,					/* Save CPU state */
		m6502_state_load,					/* Load CPU state */
        m6502_info,                         /* Get formatted string for a specific register */
        1,                                  /* Number of IRQ lines */
		&m6502_ICount,						/* Pointer to the instruction count */
		M6502_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		M6502_INT_IRQ,
		M6502_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
	},
	/* 5 CPU_M65C02 */
	{
		m65c02_reset,						/* Reset CPU */
		m65c02_exit,						/* Shut down the CPU */
		m65c02_execute, 					/* Execute a number of cycles */
		(void (*)(void *))m6502_setregs,	/* Set the contents of the registers */
		(void (*)(void *))m6502_getregs,	/* Get the contents of the registers */
		m65c02_getpc,						/* Return the current PC */
        m65c02_getreg,                      /* Get a specific register value */
		m65c02_setreg,						/* Set a specific register value */
        m65c02_set_nmi_line,                /* Set state of the NMI line */
		m65c02_set_irq_line,				/* Set state of the IRQ line */
		m65c02_set_irq_callback,			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		m65c02_state_save,					/* Save CPU state */
		m65c02_state_load,					/* Load CPU state */
        m65c02_info,                        /* Get formatted string for a specific register */
        1,                                  /* Number of IRQ lines */
		&m65c02_ICount, 					/* Pointer to the instruction count */
		M65C02_INT_NONE,					/* Interrupt types: none, IRQ, NMI */
		M65C02_INT_IRQ,
		M65C02_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
	/* 6 CPU_M6510 */
	{
		m6510_reset,						/* Reset CPU */
		m6510_exit, 						/* Shut down the CPU */
		m6510_execute,						/* Execute a number of cycles */
		(void (*)(void *))m6510_setregs,	/* Set the contents of the registers */
		(void (*)(void *))m6510_getregs,	/* Get the contents of the registers */
		m6510_getpc,						/* Return the current PC */
        m6510_getreg,                       /* Get a specific register value */
		m6510_setreg,						/* Set a specific register value */
        m6510_set_nmi_line,                 /* Set state of the NMI line */
		m6510_set_irq_line, 				/* Set state of the IRQ line */
		m6510_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		m6510_state_save,					/* Save CPU state */
		m6510_state_load,					/* Load CPU state */
        m6510_info,                         /* Get formatted string for a specific register */
        1,                                  /* Number of IRQ lines */
		&m6510_ICount,						/* Pointer to the instruction count */
		M6510_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		M6510_INT_IRQ,
		M6510_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
	/* 7 CPU_H6280 */
	{
		h6280_reset,						/* Reset CPU */
		h6280_exit, 						/* Shut down the CPU */
		h6280_execute,						/* Execute a number of cycles */
		(void (*)(void *))h6280_setregs,	/* Set the contents of the registers */
		(void (*)(void *))h6280_getregs,	/* Get the contents of the registers */
		h6280_getpc,						/* Return the current PC */
        h6280_getreg,                       /* Get a specific register value */
		h6280_setreg,						/* Set a specific register value */
        h6280_set_nmi_line,                 /* Set state of the NMI line */
		h6280_set_irq_line, 				/* Set state of the IRQ line */
		h6280_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        h6280_info,                         /* Get formatted string for a specific register */
		3,									/* Number of IRQ lines */
		&h6280_ICount,						/* Pointer to the instruction count */
		H6280_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		-1,
		H6280_INT_NMI,
		cpu_readmem21,						/* Memory read */
		cpu_writemem21, 					/* Memory write */
		cpu_setOPbase21,					/* Update CPU opcode base */
		21, 								/* CPU address bits */
		ABITS1_21,ABITS2_21,ABITS_MIN_21	/* Address bits, for the memory system */
    },
	/* 8 CPU_I86 */
	{
		i86_reset,							/* Reset CPU */
		i86_exit,							/* Shut down the CPU */
		i86_execute,						/* Execute a number of cycles */
		(void (*)(void *))i86_setregs,		/* Set the contents of the registers */
		(void (*)(void *))i86_getregs,		/* Get the contents of the registers */
		i86_getpc,							/* Return the current PC */
        i86_getreg,                         /* Get a specific register value */
		i86_setreg, 						/* Set a specific register value */
        i86_set_nmi_line,                   /* Set state of the NMI line */
		i86_set_irq_line,					/* Set state of the IRQ line */
		i86_set_irq_callback,				/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        i86_info,                           /* Get formatted string for a specific register */
        1,                                  /* Number of IRQ lines */
		&i86_ICount,						/* Pointer to the instruction count */
		I86_INT_NONE,						/* Interrupt types: none, IRQ, NMI */
		-1000,
		I86_NMI_INT,
		cpu_readmem20,						/* Memory read */
		cpu_writemem20, 					/* Memory write */
		cpu_setOPbase20,					/* Update CPU opcode base */
		20, 								/* CPU address bits */
		ABITS1_20,ABITS2_20,ABITS_MIN_20	/* Address bits, for the memory system */
    },
	/*	9 CPU_I8035 */
    {
		i8035_reset,						/* Reset CPU */
		i8035_exit, 						/* Shut down the CPU */
		i8035_execute,						/* Execute a number of cycles */
		(void (*)(void *))i8035_setregs,	/* Set the contents of the registers */
		(void (*)(void *))i8035_getregs,	/* Get the contents of the registers */
		i8035_getpc,						/* Return the current PC */
        i8035_getreg,                       /* Get a specific register value */
		i8035_setreg,						/* Set a specific register value */
        i8035_set_nmi_line,                 /* Set state of the NMI line */
		i8035_set_irq_line, 				/* Set state of the IRQ line */
		i8035_set_irq_callback, 			/* Set IRQ enable/vector callback */
        NULL,                               /* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        i8035_info,                         /* Get formatted string for a specific register */
        1,                                  /* Number of IRQ lines */
		&i8035_ICount,						/* Pointer to the instruction count */
		I8035_IGNORE_INT,					/* Interrupt types: none, IRQ, NMI */
		I8035_EXT_INT,
        -1,
        cpu_readmem16,                      /* Memory read */
        cpu_writemem16,                     /* Memory write */
        cpu_setOPbase16,                    /* Update CPU opcode base */
        16,                                 /* CPU address bits */
        ABITS1_16,ABITS2_16,ABITS_MIN_16    /* Address bits, for the memory system */
    },
	/* 10 CPU_I8039 */
	{
		i8039_reset,						/* Reset CPU */
		i8039_exit, 						/* Shut down the CPU */
		i8039_execute,						/* Execute a number of cycles */
		(void (*)(void *))i8039_setregs,	/* Set the contents of the registers */
		(void (*)(void *))i8039_getregs,	/* Get the contents of the registers */
		i8039_getpc,						/* Return the current PC */
        i8039_getreg,                       /* Get a specific register value */
		i8039_setreg,						/* Set a specific register value */
        i8039_set_nmi_line,                 /* Set state of the NMI line */
		i8039_set_irq_line, 				/* Set state of the IRQ line */
		i8039_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        i8039_info,                         /* Get formatted string for a specific register */
        1,                                  /* Number of IRQ lines */
		&i8039_ICount,						/* Pointer to the instruction count */
		I8039_IGNORE_INT,					/* Interrupt types: none, IRQ, NMI */
		I8039_EXT_INT,
		-1,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
	/* 11 CPU_I8048 */
	{
		i8048_reset,						/* Reset CPU */
		i8048_exit, 						/* Shut down the CPU */
		i8048_execute,						/* Execute a number of cycles */
		(void (*)(void *))i8048_setregs,	/* Set the contents of the registers */
		(void (*)(void *))i8048_getregs,	/* Get the contents of the registers */
		i8048_getpc,						/* Return the current PC */
        i8048_getreg,                       /* Get a specific register value */
		i8048_setreg,						/* Set a specific register value */
        i8048_set_nmi_line,                 /* Set state of the NMI line */
		i8048_set_irq_line, 				/* Set state of the IRQ line */
		i8048_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        i8048_info,                         /* Get formatted string for a specific register */
        1,                                  /* Number of IRQ lines */
		&i8048_ICount,						/* Pointer to the instruction count */
		I8048_IGNORE_INT,					/* Interrupt types: none, IRQ, NMI */
		I8048_EXT_INT,
		-1,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
	/* 12 CPU_N7751 */
	{
		n7751_reset,						/* Reset CPU */
		n7751_exit, 						/* Shut down the CPU */
		n7751_execute,						/* Execute a number of cycles */
		(void (*)(void *))n7751_setregs,	/* Set the contents of the registers */
		(void (*)(void *))n7751_getregs,	/* Get the contents of the registers */
		n7751_getpc,						/* Return the current PC */
        n7751_getreg,                       /* Get a specific register value */
		n7751_setreg,						/* Set a specific register value */
        n7751_set_nmi_line,                 /* Set state of the NMI line */
		n7751_set_irq_line, 				/* Set state of the IRQ line */
		n7751_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        n7751_info,                         /* Get formatted string for a specific register */
        1,                                  /* Number of IRQ lines */
		&n7751_ICount,						/* Pointer to the instruction count */
		N7751_IGNORE_INT,					/* Interrupt types: none, IRQ, NMI */
		N7751_EXT_INT,
		-1,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
	/* 13 CPU_M6800 */
	{
		m6800_reset,						/* Reset CPU */
		m6800_exit, 						/* Shut down the CPU */
		m6800_execute,						/* Execute a number of cycles */
		(void (*)(void *))m6800_setregs,	/* Set the contents of the registers */
		(void (*)(void *))m6800_getregs,	/* Get the contents of the registers */
		m6800_getpc,						/* Return the current PC */
		m6800_getreg,						/* Get a specific register value */
		m6800_setreg,						/* Set a specific register value */
		m6800_set_nmi_line, 				/* Set state of the NMI line */
		m6800_set_irq_line, 				/* Set state of the IRQ line */
		m6800_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
		m6800_info, 						/* Get formatted string for a specific register */
		2,									/* Number of IRQ lines */
		&m6800_ICount,						/* Pointer to the instruction count */
		M6800_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		M6800_INT_IRQ,
		M6800_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
	/* 14 CPU_M6802 */
	{
		m6802_reset,						/* Reset CPU */
		m6802_exit, 						/* Shut down the CPU */
		m6802_execute,						/* Execute a number of cycles */
		(void (*)(void *))m6802_setregs,	/* Set the contents of the registers */
		(void (*)(void *))m6802_getregs,	/* Get the contents of the registers */
		m6802_getpc,						/* Return the current PC */
        m6802_getreg,                       /* Get a specific register value */
		m6802_setreg,						/* Set a specific register value */
        m6802_set_nmi_line,                 /* Set state of the NMI line */
		m6802_set_irq_line, 				/* Set state of the IRQ line */
		m6802_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        m6802_info,                         /* Get formatted string for a specific register */
		2,									/* Number of IRQ lines */
		&m6802_ICount,						/* Pointer to the instruction count */
		M6802_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		M6802_INT_IRQ,
		M6802_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
	/* 15 CPU_M6803 */
	{
		m6803_reset,						/* Reset CPU */
		m6803_exit, 						/* Shut down the CPU */
		m6803_execute,						/* Execute a number of cycles */
		(void (*)(void *))m6803_setregs,	/* Set the contents of the registers */
		(void (*)(void *))m6803_getregs,	/* Get the contents of the registers */
		m6803_getpc,						/* Return the current PC */
        m6803_getreg,                       /* Get a specific register value */
		m6803_setreg,						/* Set a specific register value */
        m6803_set_nmi_line,                 /* Set state of the NMI line */
		m6803_set_irq_line, 				/* Set state of the IRQ line */
		m6803_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        m6803_info,                         /* Get formatted string for a specific register */
		2,									/* Number of IRQ lines */
		&m6803_ICount,						/* Pointer to the instruction count */
		M6803_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		M6803_INT_IRQ,
		M6803_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
	/* 16 CPU_M6808 */
	{
		m6808_reset,						/* Reset CPU */
		m6808_exit, 						/* Shut down the CPU */
        m6808_execute,                      /* Execute a number of cycles */
		(void (*)(void *))m6808_setregs,	/* Set the contents of the registers */
		(void (*)(void *))m6808_getregs,	/* Get the contents of the registers */
		m6808_getpc,						/* Return the current PC */
        m6808_getreg,                       /* Get a specific register value */
		m6808_setreg,						/* Set a specific register value */
        m6808_set_nmi_line,                 /* Set state of the NMI line */
		m6808_set_irq_line, 				/* Set state of the IRQ line */
		m6808_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        m6808_info,                         /* Get formatted string for a specific register */
		2,									/* Number of IRQ lines */
		&m6808_ICount,						/* Pointer to the instruction count */
		M6808_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		M6808_INT_IRQ,
		M6808_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
	/* 17 CPU_HD63701 */
	{
		hd63701_reset,						/* Reset CPU */
		hd63701_exit,						/* Shut down the CPU */
		hd63701_execute,					/* Execute a number of cycles */
		(void (*)(void *))hd63701_setregs,	/* Set the contents of the registers */
		(void (*)(void *))hd63701_getregs,	/* Get the contents of the registers */
		hd63701_getpc,						/* Return the current PC */
        hd63701_getreg,                     /* Get a specific register value */
		hd63701_setreg, 					/* Set a specific register value */
        hd63701_set_nmi_line,               /* Set state of the NMI line */
		hd63701_set_irq_line,				/* Set state of the IRQ line */
		hd63701_set_irq_callback,			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        hd63701_info,                       /* Get formatted string for a specific register */
		2,									/* Number of IRQ lines */
		&hd63701_ICount,					/* Pointer to the instruction count */
		HD63701_INT_NONE,					/* Interrupt types: none, IRQ, NMI */
		HD63701_INT_IRQ,
		HD63701_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
	/* 18 CPU_M6805 */
	{
		m6805_reset,						/* Reset CPU */
		m6805_exit, 						/* Shut down the CPU */
        m6805_execute,                      /* Execute a number of cycles */
		(void (*)(void *))m6805_setregs,	/* Set the contents of the registers */
		(void (*)(void *))m6805_getregs,	/* Get the contents of the registers */
		m6805_getpc,						/* Return the current PC */
        m6805_getreg,                       /* Get a specific register value */
		m6805_setreg,						/* Set a specific register value */
        m6805_set_nmi_line,                 /* Set state of the NMI line */
		m6805_set_irq_line, 				/* Set state of the IRQ line */
		m6805_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        m6805_info,                         /* Get formatted string for a specific register */
        1,                                  /* Number of IRQ lines */
		&m6805_ICount,						/* Pointer to the instruction count */
		M6805_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		M6805_INT_IRQ,
		-1,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
	/* 19 CPU_M68705 */
	{
		m68705_reset,						/* Reset CPU */
		m68705_exit,						/* Shut down the CPU */
		m68705_execute, 					/* Execute a number of cycles */
		(void (*)(void *))m68705_setregs,	/* Set the contents of the registers */
		(void (*)(void *))m68705_getregs,	/* Get the contents of the registers */
		m68705_getpc,						/* Return the current PC */
        m68705_getreg,                      /* Get a specific register value */
		m68705_setreg,						/* Set a specific register value */
        m68705_set_nmi_line,                /* Set state of the NMI line */
		m68705_set_irq_line,				/* Set state of the IRQ line */
		m68705_set_irq_callback,			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        m68705_info,                        /* Get formatted string for a specific register */
        1,                                  /* Number of IRQ lines */
		&m68705_ICount, 					/* Pointer to the instruction count */
		M68705_INT_NONE,					/* Interrupt types: none, IRQ, NMI */
		M68705_INT_IRQ,
		-1,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
	/* 20 CPU_M6309 */
	{
		m6309_reset,						/* Reset CPU */
		m6309_exit, 						/* Shut down the CPU */
		m6309_execute,						/* Execute a number of cycles */
		(void (*)(void *))m6309_setregs,	/* Set the contents of the registers */
		(void (*)(void *))m6309_getregs,	/* Get the contents of the registers */
		m6309_getpc,						/* Return the current PC */
        m6309_getreg,                       /* Get a specific register value */
		m6309_setreg,						/* Set a specific register value */
        m6309_set_nmi_line,                 /* Set state of the NMI line */
		m6309_set_irq_line, 				/* Set state of the IRQ line */
		m6309_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        m6309_info,                         /* Get formatted string for a specific register */
        2,                                  /* Number of IRQ lines */
		&m6309_ICount,						/* Pointer to the instruction count */
		M6309_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		M6309_INT_IRQ,
		M6309_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
	},
	/* 21 CPU_M6809 */
	{
		m6809_reset,						/* Reset CPU */
		m6809_exit, 						/* Shut down the CPU */
        m6809_execute,                      /* Execute a number of cycles */
		(void (*)(void *))m6809_setregs,	/* Set the contents of the registers */
		(void (*)(void *))m6809_getregs,	/* Get the contents of the registers */
		m6809_getpc,						/* Return the current PC */
        m6809_getreg,                       /* Get a specific register value */
		m6809_setreg,						/* Set a specific register value */
        m6809_set_nmi_line,                 /* Set state of the NMI line */
		m6809_set_irq_line, 				/* Set state of the IRQ line */
		m6809_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        m6809_info,                         /* Get formatted string for a specific register */
        2,                                  /* Number of IRQ lines */
		&m6809_ICount,						/* Pointer to the instruction count */
		M6809_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		M6809_INT_IRQ,
		M6809_INT_NMI,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
	/* 22 CPU_M68000 */
	{
		MC68000_reset,						/* Reset CPU */
		MC68000_exit,						/* Shut down the CPU */
		MC68000_execute,					/* Execute a number of cycles */
		(void (*)(void *))MC68000_setregs,	/* Set the contents of the registers */
		(void (*)(void *))MC68000_getregs,	/* Get the contents of the registers */
		MC68000_getpc,						/* Return the current PC */
        MC68000_getreg,                     /* Get a specific register value */
		MC68000_setreg, 					/* Set a specific register value */
        MC68000_set_nmi_line,               /* Set state of the NMI line */
		MC68000_set_irq_line,				/* Set state of the IRQ line */
		MC68000_set_irq_callback,			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        MC68000_info,                       /* Get formatted string for a specific register */
        8,                                  /* Number of IRQ lines */
		&MC68000_ICount,					/* Pointer to the instruction count */
		MC68000_INT_NONE,					/* Interrupt types: none, IRQ, NMI */
		-1,
		-1,
		cpu_readmem24,						/* Memory read */
		cpu_writemem24, 					/* Memory write */
		cpu_setOPbase24,					/* Update CPU opcode base */
		24, 								/* CPU address bits */
		ABITS1_24,ABITS2_24,ABITS_MIN_24	/* Address bits, for the memory system */
	},
	/* 23 CPU_M68010 */
	{
		MC68010_reset,						/* Reset CPU */
		MC68010_exit,						/* Shut down the CPU */
		MC68010_execute,					/* Execute a number of cycles */
		(void (*)(void *))MC68010_setregs,	/* Set the contents of the registers */
		(void (*)(void *))MC68010_getregs,	/* Get the contents of the registers */
		MC68010_getpc,						/* Return the current PC */
        MC68010_getreg,                     /* Get a specific register value */
		MC68010_setreg, 					/* Set a specific register value */
        MC68010_set_nmi_line,               /* Set state of the NMI line */
		MC68010_set_irq_line,				/* Set state of the IRQ line */
		MC68010_set_irq_callback,			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        MC68010_info,                       /* Get formatted string for a specific register */
        8,                                  /* Number of IRQ lines */
		&MC68010_ICount,					/* Pointer to the instruction count */
		MC68010_INT_NONE,					/* Interrupt types: none, IRQ, NMI */
		-1,
		-1,
		cpu_readmem24,						/* Memory read */
		cpu_writemem24, 					/* Memory write */
		cpu_setOPbase24,					/* Update CPU opcode base */
		24, 								/* CPU address bits */
		ABITS1_24,ABITS2_24,ABITS_MIN_24	/* Address bits, for the memory system */
    },
	/* 24 CPU_M68020 */
	{
		MC68020_reset,						/* Reset CPU */
		MC68020_exit,						/* Shut down the CPU */
		MC68020_execute,					/* Execute a number of cycles */
		(void (*)(void *))MC68020_setregs,	/* Set the contents of the registers */
		(void (*)(void *))MC68020_getregs,	/* Get the contents of the registers */
		MC68020_getpc,						/* Return the current PC */
        MC68020_getreg,                     /* Get a specific register value */
		MC68020_setreg, 					/* Set a specific register value */
        MC68020_set_nmi_line,               /* Set state of the NMI line */
		MC68020_set_irq_line,				/* Set state of the IRQ line */
		MC68020_set_irq_callback,			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        MC68020_info,                       /* Get formatted string for a specific register */
        8,                                  /* Number of IRQ lines */
		&MC68020_ICount,					/* Pointer to the instruction count */
		MC68020_INT_NONE,					/* Interrupt types: none, IRQ, NMI */
		-1,
		-1,
		cpu_readmem24,						/* Memory read */
		cpu_writemem24, 					/* Memory write */
		cpu_setOPbase24,					/* Update CPU opcode base */
		24, 								/* CPU address bits */
		ABITS1_24,ABITS2_24,ABITS_MIN_24	/* Address bits, for the memory system */
    },
	/* 25 CPU_T11 */
	{
		t11_reset,							/* Reset CPU */
		t11_exit,							/* Shut down the CPU */
        t11_execute,                        /* Execute a number of cycles */
		(void (*)(void *))t11_setregs,		/* Set the contents of the registers */
		(void (*)(void *))t11_getregs,		/* Get the contents of the registers */
		t11_getpc,							/* Return the current PC */
        t11_getreg,                         /* Get a specific register value */
		t11_setreg, 						/* Set a specific register value */
        t11_set_nmi_line,                   /* Set state of the NMI line */
		t11_set_irq_line,					/* Set state of the IRQ line */
		t11_set_irq_callback,				/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        t11_info,                           /* Get formatted string for a specific register */
        4,                                  /* Number of IRQ lines */
		&t11_ICount,						/* Pointer to the instruction count */
		T11_INT_NONE,						/* Interrupt types: none, IRQ, NMI */
		-1,
		-1,
		cpu_readmem16lew,					/* Memory read */
		cpu_writemem16lew,					/* Memory write */
		cpu_setOPbase16lew, 				/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16LEW,ABITS2_16LEW,ABITS_MIN_16LEW  /* Address bits, for the memory system */
	},
	/* 26 CPU_S2650 */
	{
		s2650_reset,						/* Reset CPU */
		s2650_exit, 						/* Shut down the CPU */
		s2650_execute,						/* Execute a number of cycles */
		(void (*)(void *))s2650_setregs,	/* Set the contents of the registers */
		(void (*)(void *))s2650_getregs,	/* Get the contents of the registers */
		s2650_getpc,						/* Return the current PC */
        s2650_getreg,                       /* Get a specific register value */
		s2650_setreg,						/* Set a specific register value */
        s2650_set_nmi_line,                 /* Set state of the NMI line */
		s2650_set_irq_line, 				/* Set state of the IRQ line */
		s2650_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        s2650_info,                         /* Get formatted string for a specific register */
        2,                                  /* Number of IRQ lines */
		&s2650_ICount,						/* Pointer to the instruction count */
		S2650_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		-1,
		-1,
		cpu_readmem16,                      /* Memory read */
		cpu_writemem16,                     /* Memory write */
		cpu_setOPbase16,                    /* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
	},
	/* 27 CPU_TMS34010 */
	{
		TMS34010_reset, 					/* Reset CPU */
		TMS34010_exit,						/* Shut down the CPU */
		TMS34010_execute,					/* Execute a number of cycles */
		(void (*)(void *))TMS34010_setregs, /* Set the contents of the registers */
		(void (*)(void *))TMS34010_getregs, /* Get the contents of the registers */
		TMS34010_getpc, 					/* Return the current PC */
        TMS34010_getreg,                    /* Get a specific register value */
		TMS34010_setreg,					/* Set a specific register value */
        TMS34010_set_nmi_line,              /* Set state of the NMI line */
		TMS34010_set_irq_line,				/* Set state of the IRQ line */
		TMS34010_set_irq_callback,			/* Set IRQ enable/vector callback */
		TMS34010_internal_interrupt,		/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        TMS34010_info,                      /* Get formatted string for a specific register */
        2,                                  /* Number of IRQ lines */
        &TMS34010_ICount,                   /* Pointer to the instruction count */
		TMS34010_INT_NONE,					/* Interrupt types: none, IRQ, NMI */
		TMS34010_INT1,
		-1,
		cpu_readmem29,						/* Memory read */
		cpu_writemem29, 					/* Memory write */
		cpu_setOPbase29,					/* Update CPU opcode base */
		29, 								/* CPU address bits */
		ABITS1_29,ABITS2_29,ABITS_MIN_29	/* Address bits, for the memory system */
	},
	/* 28 CPU_TMS9900 */
	{
		TMS9900_reset,						/* Reset CPU */
		TMS9900_exit,						/* Shut down the CPU */
		TMS9900_execute,					/* Execute a number of cycles */
		(void (*)(void *))TMS9900_setregs,	/* Set the contents of the registers */
		(void (*)(void *))TMS9900_getregs,	/* Get the contents of the registers */
		TMS9900_getpc,						/* Return the current PC */
        TMS9900_getreg,                     /* Get a specific register value */
		TMS9900_setreg, 					/* Set a specific register value */
        TMS9900_set_nmi_line,               /* Set state of the NMI line */
		TMS9900_set_irq_line,				/* Set state of the IRQ line */
		TMS9900_set_irq_callback,			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        TMS9900_info,                       /* Get formatted string for a specific register */
        1,                                  /* Number of IRQ lines */
		&TMS9900_ICount,					/* Pointer to the instruction count */
		TMS9900_NONE,						/* Interrupt types: none, IRQ, NMI */
		-1,
		-1,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
        /* Were all ...LEW */
	},
	/* 29 CPU_Z8000 */
	{
		z8000_reset,						/* Reset CPU */
		z8000_exit, 						/* Shut down the CPU */
		z8000_execute,						/* Execute a number of cycles */
		(void (*)(void *))z8000_setregs,	/* Set the contents of the registers */
		(void (*)(void *))z8000_getregs,	/* Get the contents of the registers */
		z8000_getpc,						/* Return the current PC */
        z8000_getreg,                       /* Get a specific register value */
		z8000_setreg,						/* Set a specific register value */
        z8000_set_nmi_line,                 /* Set state of the NMI line */
		z8000_set_irq_line, 				/* Set state of the IRQ line */
		z8000_set_irq_callback, 			/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        z8000_info,                         /* Get formatted string for a specific register */
        2,                                  /* Number of IRQ lines */
		&z8000_ICount,						/* Pointer to the instruction count */
		Z8000_INT_NONE, 					/* Interrupt types: none, IRQ, NMI */
		Z8000_NVI,
		Z8000_NMI,
		cpu_readmem16,                      /* Memory read */
		cpu_writemem16,                     /* Memory write */
		cpu_setOPbase16,                    /* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
	/* 30 CPU_TMS320C10 */
	{
		TMS320C10_reset,					/* Reset CPU */
		TMS320C10_exit, 					/* Shut down the CPU */
		TMS320C10_execute,					/* Execute a number of cycles */
		(void (*)(void *))TMS320C10_setregs,/* Set the contents of the registers */
		(void (*)(void *))TMS320C10_getregs,/* Get the contents of the registers */
		TMS320C10_getpc,					/* Return the current PC */
        TMS320C10_getreg,                   /* Get a specific register value */
		TMS320C10_setreg,					/* Set a specific register value */
        TMS320C10_set_nmi_line,             /* Set state of the NMI line */
		TMS320C10_set_irq_line, 			/* Set state of the IRQ line */
		TMS320C10_set_irq_callback, 		/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
        TMS320C10_info,                     /* Get formatted string for a specific register */
		2,									/* Number of IRQ lines */
		&TMS320C10_ICount,					/* Pointer to the instruction count */
		TMS320C10_INT_NONE, 				/* Interrupt types: none, IRQ, NMI */
		-1,
		-1,
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
	/* 31 CPU_CCPU */
    {
		ccpu_reset, 						/* Reset CPU  */
		ccpu_exit,							/* Shut down CPU  */
		ccpu_execute,						/* Execute a number of cycles  */
		(void (*)(void *))ccpu_setregs, 	/* Set the contents of the registers  */
		(void (*)(void *))ccpu_getregs, 	/* Get the contents of the registers  */
		ccpu_getpc, 						/* Return the current PC  */
        ccpu_getreg,                        /* Get a specific register value */
		ccpu_setreg,						/* Set a specific register value */
        ccpu_set_nmi_line,                  /* Set state of the NMI line */
		ccpu_set_irq_line,					/* Set state of the IRQ line */
		ccpu_set_irq_callback,				/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
		ccpu_info,							/* Get formatted string for a specific register */
        2,                                  /* Number of IRQ lines */
		&ccpu_ICount,						/* Pointer to the instruction count  */
		0,-1,-1,							/* Interrupt types: none, IRQ, NMI	*/
		cpu_readmem16,						/* Memory read	*/
		cpu_writemem16, 					/* Memory write  */
		cpu_setOPbase16,					/* Update CPU opcode base  */
		16, 								/* CPU address bits  */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system	*/
	},
#ifdef MESS
	/* 32 CPU_PDP1 */
    {
		pdp1_reset, 						/* Reset CPU  */
		pdp1_exit,							/* Shut down CPU  */
		pdp1_execute,						/* Execute a number of cycles  */
		(void (*)(void *))pdp1_setregs, 	/* Set the contents of the registers  */
		(void (*)(void *))pdp1_getregs, 	/* Get the contents of the registers  */
		pdp1_getpc, 						/* Return the current PC  */
        pdp1_getreg,                        /* Get a specific register value */
		pdp1_setreg,						/* Set a specific register value */
        pdp1_set_nmi_line,                  /* Set state of the NMI line */
		pdp1_set_irq_line,					/* Set state of the IRQ line */
		pdp1_set_irq_callback,				/* Set IRQ enable/vector callback */
		NULL,								/* Cause internal interrupt */
		NULL,								/* Save CPU state */
		NULL,								/* Load CPU state */
		pdp1_info,							/* Get formatted string for a specific register */
        2,                                  /* Number of IRQ lines */
		&pdp1_ICount,						/* Pointer to the instruction count  */
		0,-1,-1,							/* Interrupt types: none, IRQ, NMI	*/
		cpu_readmem16,						/* Memory read	*/
		cpu_writemem16, 					/* Memory write  */
		cpu_setOPbase16,					/* Update CPU opcode base  */
		16, 								/* CPU address bits  */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system	*/
    },
#endif
};

void cpu_init(void)
{
	int i;

	/* count how many CPUs we have to emulate */
	totalcpu = 0;

	while (totalcpu < MAX_CPU)
	{
		if (CPU_TYPE(totalcpu) == 0) break;
		totalcpu++;
	}

	/* zap the CPU data structure */
	memset (cpu, 0, sizeof (cpu));

	/* Set up the interface functions */
	for (i = 0; i < MAX_CPU; i++)
		cpu[i].intf = &cpuintf[CPU_TYPE(i)];
	for (i = 0; i < MAX_CPU; i++)
		Z80_daisychain[i] = 0;

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
		int j;

		/* Save if there is another CPU of the same type */
		cpu[i].save_context = 0;

		for (j = 0; j < totalcpu; j++)
			if ( i != j && !strcmp(cpu_core_file(CPU_TYPE(i)),cpu_core_file(CPU_TYPE(j))) )
				cpu[i].save_context = 1;

		#ifdef MAME_DEBUG

		/* or if we're running with the debugger */
		{
			extern int mame_debug;
			cpu[i].save_context |= mame_debug;
		}

		#endif
	}

reset:
	/* initialize the various timers (suspends all CPUs at startup) */
	cpu_inittimers ();
	watchdog_counter = -1;

	/* enable all CPUs (except for audio CPUs if the sound is off) */
	for (i = 0; i < totalcpu; i++)
		if (!CPU_AUDIO(i) || Machine->sample_rate != 0)
			timer_suspendcpu (i, 0);

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
		if (cpu[i].save_context) SETREGS (i, cpu[i].context);
		activecpu = i;
		RESET (i);

        /* Set the irq callback for the cpu */
		SET_IRQ_CALLBACK(i,cpu_irq_callbacks[i]);

        /* save the CPU context if necessary */
		if (cpu[i].save_context) GETREGS (i, cpu[i].context);

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
        osd_profiler(OSD_PROFILE_EXTRA);

		/* ask the timer system to schedule */
		if (timer_schedule_cpu (&cpunum, &running))
		{
			int ran;


			/* switch memory and CPU contexts */
			activecpu = cpunum;
			memorycontextswap (activecpu);
			if (cpu[activecpu].save_context) SETREGS (activecpu, cpu[activecpu].context);

			/* make sure any bank switching is reset */
			SET_OP_BASE (activecpu, GETPC (activecpu));

			/* run for the requested number of cycles */
			osd_profiler(OSD_PROFILE_CPU1 + cpunum);
			ran = EXECUTE (activecpu, running);
			osd_profiler(OSD_PROFILE_END);

			/* update based on how many cycles we really ran */
			cpu[activecpu].totalcycles += ran;

			/* update the contexts */
			if (cpu[activecpu].save_context) GETREGS (activecpu, cpu[activecpu].context);
			updatememorybase (activecpu);
			activecpu = -1;

			/* update the timer with how long we actually ran */
			timer_update_cpu (cpunum, ran);
		}

		osd_profiler(OSD_PROFILE_END);
	}

	/* write hi scores to disk - No scores saving if cheat */
	if (hiscoreloaded != 0 && Machine->gamedrv->hiscore_save)
		(*Machine->gamedrv->hiscore_save)();

#ifdef MESS
    if (Machine->drv->stop_machine) (*Machine->drv->stop_machine)();
#endif

	/* shut down the CPU cores */
	for (i = 0; i < totalcpu; i++)
	{
		if( cpu[i].intf->exit )
			(*cpu[i].intf->exit)();
	}
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
void cpu_halt(int cpunum,int _running)
{
	if (cpunum >= MAX_CPU) return;

	/* don't resume audio CPUs if sound is disabled */
	if (!CPU_AUDIO(cpunum) || Machine->sample_rate != 0)
		timer_suspendcpu (cpunum, !_running);
}



/***************************************************************************

  This function returns CPUNUM current status  (running or halted)

***************************************************************************/
int cpu_getstatus(int cpunum)
{
	if (cpunum >= MAX_CPU) return 0;

	return !timer_iscpususpended (cpunum);
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



unsigned cpu_getpc(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return GETPC (cpunum);
}


/***************************************************************************

  This is similar to cpu_getpc(), but instead of returning the current PC,
  it returns the address of the opcode that is doing the read/write. The PC
  has already been incremented by some unknown amount by the time the actual
  read or write is being executed. This helps to figure out what opcode is
  actually doing the reading or writing, and therefore the amount of cycles
  it's taking. The Missile Command driver needs to know this.

  WARNING: this function might return -1, meaning that there isn't a valid
  previouspc (e.g. a memory push caused by an interrupt).

***************************************************************************/
int cpu_getpreviouspc(void)  /* -RAY- */
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;

	switch(CPU_TYPE(cpunum))
	{
		case CPU_Z80:
		case CPU_I8039:
		case CPU_M6502:
		case CPU_M6809:
		case CPU_M68000:	/* ASG 980413 */
		case CPU_TMS320C10:
			return previouspc;
			break;

		default:
	if (errorlog) fprintf(errorlog,"cpu_getpreviouspc: unsupported CPU type %02x\n",CPU_TYPE(cpunum));
			return -1;
			break;
	}
}


/***************************************************************************

  This is similar to cpu_getpc(), but instead of returning the current PC,
  it returns the address stored on the top of the stack, which usually is
  the address where execution will resume after the current subroutine.
  Note that the returned value will be wrong if the program has PUSHed
  registers on the stack.

***************************************************************************/
int cpu_getreturnpc(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;

	switch(CPU_TYPE(cpunum))
    {
		case CPU_Z80:
			{
				Z80_Regs _regs;
				extern unsigned char *RAM;

				GETREGS(cpunum,&_regs);
				return RAM[_regs.SP.d] + (RAM[_regs.SP.d+1] << 8);
			}
			break;

		default:
			if (errorlog)
				fprintf(errorlog,"cpu_getreturnpc: unsupported CPU type %02x [%s]\n",CPU_TYPE(cpunum),CPU_INFO(CPU_TYPE(cpunum),0,0));
			return -1;
			break;
	}
}


/* these are available externally, for the timer system */
int cycles_currently_ran(void)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	return running - ICOUNT (cpunum);
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
	double time = timer_get_time ();
	if (time >= scantime) scantime += TIME_IN_HZ(Machine->drv->frames_per_second);
	ret = scantime - time;
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
        SET_IRQ_LINE(0, irqline, CLEAR_LINE);
		irq_line_state[0 * MAX_IRQ_LINES + irqline] = CLEAR_LINE;
    }
	LOG((errorlog, "cpu_0_irq_callback(%d) $%04x\n", irqline, irq_line_vector[0 * MAX_IRQ_LINES + irqline]));
	return irq_line_vector[0 * MAX_IRQ_LINES + irqline];
}

static int cpu_1_irq_callback(int irqline)
{
	if( irq_line_state[1 * MAX_IRQ_LINES + irqline] == HOLD_LINE )
	{
        SET_IRQ_LINE(1, irqline, CLEAR_LINE);
		irq_line_state[1 * MAX_IRQ_LINES + irqline] = CLEAR_LINE;
    }
	LOG((errorlog, "cpu_1_irq_callback(%d) $%04x\n", irqline, irq_line_vector[1 * MAX_IRQ_LINES + irqline]));
	return irq_line_vector[1 * MAX_IRQ_LINES + irqline];
}

static int cpu_2_irq_callback(int irqline)
{
	if( irq_line_state[2 * MAX_IRQ_LINES + irqline] == HOLD_LINE )
	{
        SET_IRQ_LINE(2, irqline, CLEAR_LINE);
		irq_line_state[2 * MAX_IRQ_LINES + irqline] = CLEAR_LINE;
	}
	LOG((errorlog, "cpu_2_irq_callback(%d) $%04x\n", irqline, irq_line_vector[2 * MAX_IRQ_LINES + irqline]));
	return irq_line_vector[2 * MAX_IRQ_LINES + irqline];
}

static int cpu_3_irq_callback(int irqline)
{
	if( irq_line_state[3 * MAX_IRQ_LINES + irqline] == HOLD_LINE )
	{
        SET_IRQ_LINE(3, irqline, CLEAR_LINE);
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
	LOG((errorlog,"cpu_set_nmi_line(%d,%d)\n",cpunum,state));
	timer_set (TIME_NOW, (cpunum & 7) | (state << 3), cpu_manualnmicallback);
}

/***************************************************************************

  Use this function to set the state of an IRQ line of a CPU
  The meaning of irqline varies between the different CPU types

***************************************************************************/
void cpu_set_irq_line(int cpunum, int irqline, int state)
{
	LOG((errorlog,"cpu_set_irq_line(%d,%d,%d)\n",cpunum,irqline,state));
	timer_set (TIME_NOW, (irqline & 7) | ((cpunum & 7) << 3) | (state << 6), cpu_manualirqcallback);
}

/***************************************************************************

  Use this function to cause an interrupt immediately (don't have to wait
  until the next call to the interrupt handler)

***************************************************************************/
void cpu_cause_interrupt(int cpunum,int type)
{
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
    if (cpu[activecpu].save_context) SETREGS (activecpu, cpu[activecpu].context);

	LOG((errorlog,"cpu_manualnmicallback %d,%d\n",cpunum,state));

	switch (state)
	{
        case PULSE_LINE:
			SET_NMI_LINE(cpunum,ASSERT_LINE);
			SET_NMI_LINE(cpunum,CLEAR_LINE);
            break;
        case HOLD_LINE:
        case ASSERT_LINE:
			SET_NMI_LINE(cpunum,ASSERT_LINE);
            break;
        case CLEAR_LINE:
			SET_NMI_LINE(cpunum,CLEAR_LINE);
            break;
        default:
			if( errorlog ) fprintf( errorlog, "cpu_manualnmicallback cpu #%d unknown state %d\n", cpunum, state);
    }
    /* update the CPU's context */
	if (cpu[activecpu].save_context) GETREGS (activecpu, cpu[activecpu].context);
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
    if (cpu[activecpu].save_context) SETREGS (activecpu, cpu[activecpu].context);

	LOG((errorlog,"cpu_manualirqcallback %d,%d,%d\n",cpunum,irqline,state));

	irq_line_state[cpunum * MAX_IRQ_LINES + irqline] = state;
	switch (state)
	{
        case PULSE_LINE:
			SET_IRQ_LINE(cpunum,irqline,ASSERT_LINE);
			SET_IRQ_LINE(cpunum,irqline,CLEAR_LINE);
            break;
        case HOLD_LINE:
        case ASSERT_LINE:
			SET_IRQ_LINE(cpunum,irqline,ASSERT_LINE);
            break;
        case CLEAR_LINE:
			SET_IRQ_LINE(cpunum,irqline,CLEAR_LINE);
            break;
        default:
			if( errorlog ) fprintf( errorlog, "cpu_manualirqcallback cpu #%d, line %d, unknown state %d\n", cpunum, irqline, state);
    }

    /* update the CPU's context */
	if (cpu[activecpu].save_context) GETREGS (activecpu, cpu[activecpu].context);
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
	if (cpu[activecpu].save_context) SETREGS (activecpu, cpu[activecpu].context);

	INTERNAL_INTERRUPT (cpunum, type);

    /* update the CPU's context */
	if (cpu[activecpu].save_context) GETREGS (activecpu, cpu[activecpu].context);
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

	/* swap to the CPU's context */
    activecpu = cpunum;
	memorycontextswap (activecpu);
	if (cpu[activecpu].save_context) SETREGS (activecpu, cpu[activecpu].context);

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
			case CPU_Z80:				irq_line = 0; LOG((errorlog,"Z80 IRQ\n")); break;

			case CPU_8080:
				switch (num)
				{
				case I8080_INTR:		irq_line = 0; LOG((errorlog,"I8080 INTR\n")); break;
				case I8080_RST55:		irq_line = 1; LOG((errorlog,"I8080 RST55\n")); break;
				case I8080_RST65:		irq_line = 2; LOG((errorlog,"I8080 RST65\n")); break;
				case I8080_RST75:		irq_line = 3; LOG((errorlog,"I8080 RST75\n")); break;
				default:				irq_line = 0; LOG((errorlog,"I8080 unknown\n"));
				}
                break;
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

			case CPU_M6502: 			irq_line = 0; LOG((errorlog,"M6502 IRQ\n")); break;
			case CPU_M65C02:			irq_line = 0; LOG((errorlog,"M65C02 IRQ\n")); break;
			case CPU_M6510: 			irq_line = 0; LOG((errorlog,"M6510 IRQ\n")); break;

			case CPU_H6280:
                switch (num)
                {
				case H6280_INT_IRQ1:	irq_line = 0; LOG((errorlog,"H6280 INT 1\n")); break;
				case H6280_INT_IRQ2:	irq_line = 1; LOG((errorlog,"H6280 INT 2\n")); break;
				case H6280_INT_TIMER:	irq_line = 2; LOG((errorlog,"H6280 TIMER INT\n")); break;
				default:				irq_line = 0; LOG((errorlog,"H6280 unknown\n"));
                }
                break;

			case CPU_I86:				irq_line = 0; LOG((errorlog,"I86 IRQ\n")); break;

			case CPU_I8035: 			irq_line = 0; LOG((errorlog,"I8035 IRQ\n")); break;
            case CPU_I8039:             irq_line = 0; LOG((errorlog,"I8039 IRQ\n")); break;
			case CPU_I8048: 			irq_line = 0; LOG((errorlog,"I8048 IRQ\n")); break;
			case CPU_N7751: 			irq_line = 0; LOG((errorlog,"N7751 IRQ\n")); break;

			case CPU_M6802:
				switch (num)
				{
				case M6802_INT_IRQ: 	irq_line = 0; LOG((errorlog,"M6802 IRQ\n")); break;
				case M6802_INT_OCI: 	irq_line = 1; LOG((errorlog,"M6802 OCI\n")); break;
				default:				irq_line = 0; LOG((errorlog,"M6802 unknown\n"));
                }
                break;
			case CPU_M6803:
				switch (num)
				{
				case M6803_INT_IRQ: 	irq_line = 0; LOG((errorlog,"M6803 IRQ\n")); break;
				case M6803_INT_OCI: 	irq_line = 1; LOG((errorlog,"M6803 OCI\n")); break;
				default:				irq_line = 0; LOG((errorlog,"M6803 unknown\n"));
                }
                break;
            case CPU_M6808:
				switch (num)
				{
				case M6808_INT_IRQ: 	irq_line = 0; LOG((errorlog,"M6808 IRQ\n")); break;
				case M6808_INT_OCI: 	irq_line = 1; LOG((errorlog,"M6808 OCI\n")); break;
				default:				irq_line = 0; LOG((errorlog,"M6808 unknown\n"));
                }
				break;
			case CPU_HD63701:
				switch (num)
				{
				case HD63701_INT_IRQ:	irq_line = 0; LOG((errorlog,"HD63701 IRQ\n")); break;
				case HD63701_INT_OCI:	irq_line = 1; LOG((errorlog,"HD63701 OCI\n")); break;
				default:				irq_line = 0; LOG((errorlog,"HD63701 unknown\n"));
                }
                break;

			case CPU_M6805: 			irq_line = 0; LOG((errorlog,"M6805 IRQ\n")); break;
			case CPU_M68705:			irq_line = 0; LOG((errorlog,"M68705 IRQ\n")); break;

			case CPU_M6309:
				switch (num)
				{
				case M6309_INT_IRQ: 	irq_line = 0; LOG((errorlog,"M6309 IRQ\n")); break;
				case M6309_INT_FIRQ:	irq_line = 1; LOG((errorlog,"M6309 FIRQ\n")); break;
				default:				irq_line = 0; LOG((errorlog,"M6309 unknown\n"));
				}
                break;
            case CPU_M6809:
				switch (num)
				{
				case M6809_INT_IRQ: 	irq_line = 0; LOG((errorlog,"M6809 IRQ\n")); break;
				case M6809_INT_FIRQ:	irq_line = 1; LOG((errorlog,"M6809 FIRQ\n")); break;
				default:				irq_line = 0; LOG((errorlog,"M6809 unknown\n"));
				}
				break;

            case CPU_M68000:
			case CPU_M68010:
			case CPU_M68020:
				switch (num)
				{
				case MC68000_IRQ_1: 	irq_line = 1; LOG((errorlog,"M68K IRQ1\n")); break;
				case MC68000_IRQ_2: 	irq_line = 2; LOG((errorlog,"M68K IRQ2\n")); break;
				case MC68000_IRQ_3: 	irq_line = 3; LOG((errorlog,"M68K IRQ3\n")); break;
				case MC68000_IRQ_4: 	irq_line = 4; LOG((errorlog,"M68K IRQ4\n")); break;
				case MC68000_IRQ_5: 	irq_line = 5; LOG((errorlog,"M68K IRQ5\n")); break;
				case MC68000_IRQ_6: 	irq_line = 6; LOG((errorlog,"M68K IRQ6\n")); break;
				case MC68000_IRQ_7: 	irq_line = 7; LOG((errorlog,"M68K IRQ7\n")); break;
				default:				irq_line = 0; LOG((errorlog,"M68K unknown\n"));
                }
				/* until now only auto vector interrupts supported */
                num = MC68000_INT_ACK_AUTOVECTOR;
				break;

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

			case CPU_S2650: 			irq_line = 0; LOG((errorlog,"S2650 IRQ\n")); break;

            case CPU_TMS34010:
				switch (num)
				{
				case TMS34010_INT1: 	irq_line = 0; LOG((errorlog,"TMS34010 INT1\n")); break;
				case TMS34010_INT2: 	irq_line = 1; LOG((errorlog,"TMS34010 INT2\n")); break;
				default:				irq_line = 0; LOG((errorlog,"TMS34010 unknown\n"));
                }
                break;

			case CPU_TMS9900:			irq_line = 0; LOG((errorlog,"TMS9900 IRQ\n")); break;

            case CPU_Z8000:
				switch (num)
				{
				case Z8000_NVI: 		irq_line = 0; LOG((errorlog,"Z8000 NVI\n")); break;
				case Z8000_VI:			irq_line = 1; LOG((errorlog,"Z8000 VI\n")); break;
				default:				irq_line = 0; LOG((errorlog,"Z8000 unknown\n"));
                }
                break;

            case CPU_TMS320C10:
				switch (num)
				{
				case TMS320C10_ACTIVE_INT:	irq_line = 0; LOG((errorlog,"TMS32010 INT\n")); break;
				case TMS320C10_ACTIVE_BIO:	irq_line = 1; LOG((errorlog,"TMS32010 BIO\n")); break;
				default:					irq_line = 0; LOG((errorlog,"TMS32010 unknown\n"));
                }
                break;

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
    if (cpu[activecpu].save_context) GETREGS (activecpu, cpu[activecpu].context);
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
	if (cpu[activecpu].save_context) SETREGS (activecpu, cpu[activecpu].context);

	/* clear NMI line */
    SET_NMI_LINE(activecpu,CLEAR_LINE);

    /* clear all IRQ lines */
    for (i = 0; i < cpu[activecpu].intf->num_irqs; i++)
		SET_IRQ_LINE(activecpu,i,CLEAR_LINE);

	/* update the CPU's context */
	if (cpu[activecpu].save_context) GETREGS (activecpu, cpu[activecpu].context);
	activecpu = oldactive;
	if (activecpu >= 0) memorycontextswap (activecpu);
}


static void cpu_reset_cpu (int cpunum)
{
	int oldactive = activecpu;

	/* swap to the CPU's context */
	activecpu = cpunum;
	memorycontextswap (activecpu);
	if (cpu[activecpu].save_context) SETREGS (activecpu, cpu[activecpu].context);

	/* reset the CPU */
	RESET (cpunum);

    /* Set the irq callback for the cpu */
	SET_IRQ_CALLBACK(cpunum,cpu_irq_callbacks[cpunum]);

	/* update the CPU's context */
	if (cpu[activecpu].save_context) GETREGS (activecpu, cpu[activecpu].context);
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


/*
 * This function is obsolete. Put the daisy_chain pointer into the
 * reset_param of your machine driver instead.
 */
#if 0
void cpu_setdaisychain(int cpunum, Z80_DaisyChain *daisy_chain)
{
	if( CPU_TYPE(cpunum) != CPU_Z80 )
	{
		if( errorlog ) fprintf(errorlog,"cpu_setdaisychain called for CPU #%d [%s]!\n", cpunum, CPU_INFO(CPU_TYPE(cpunum),0,0));
		return;
	}
	z80_reset(daisy_chain);
}
#endif

/***************************************************************************

  Retrieve or set the value of a specific register

***************************************************************************/

unsigned cur_cpu_getreg(int regnum)
{
	return (*cpu[activecpu].intf->get_reg)(regnum);
}

void cur_cpu_setreg(int regnum, unsigned val)
{
	(*cpu[activecpu].intf->set_reg)(regnum,val);
}

unsigned cpu_getreg(int cpunum, int regnum)
{
	int oldactive;
	unsigned val = 0;
    /* swap to the CPU's context */
	oldactive = activecpu;
    activecpu = cpunum;
    memorycontextswap (activecpu);
    if (cpu[activecpu].save_context) SETREGS (activecpu, cpu[activecpu].context);

	val = (*cpu[activecpu].intf->get_reg)(regnum);

	/* update the CPU's context */
	if (cpu[activecpu].save_context) GETREGS (activecpu, cpu[activecpu].context);
	activecpu = oldactive;
    if (activecpu >= 0) memorycontextswap (activecpu);

	return val;
}

void cpu_setreg(int cpunum, int regnum, unsigned val)
{
	int oldactive;
    /* swap to the CPU's context */
	oldactive = activecpu;
    activecpu = cpunum;
    memorycontextswap (activecpu);
    if (cpu[activecpu].save_context) SETREGS (activecpu, cpu[activecpu].context);

	(*cpu[activecpu].intf->set_reg)(regnum,val);

	/* update the CPU's context */
	if (cpu[activecpu].save_context) GETREGS (activecpu, cpu[activecpu].context);
	activecpu = oldactive;
    if (activecpu >= 0) memorycontextswap (activecpu);
}

/***************************************************************************

  Get various CPU information

***************************************************************************/

/***************************************************************************
  Return the name for the current CPU
***************************************************************************/
const char *cur_cpu_name(void)
{
	if( activecpu >= 0 )
		return CPU_INFO(activecpu,NULL,CPU_INFO_NAME);
	return "";
}

/***************************************************************************
  Return the family name for the current CPU
***************************************************************************/
const char *cur_cpu_core_family(void)
{
	if( activecpu >= 0 )
		return CPU_INFO(activecpu,NULL,CPU_INFO_FAMILY);
    return "";
}

/***************************************************************************
  Return the version number for the current CPU
***************************************************************************/
const char *cur_cpu_core_version(void)
{
	if( activecpu >= 0 )
		return CPU_INFO(activecpu,NULL,CPU_INFO_VERSION);
    return "";
}

/***************************************************************************
  Return the core filename for the current CPU
***************************************************************************/
const char *cur_cpu_core_file(void)
{
	if( activecpu >= 0 )
		return CPU_INFO(activecpu,NULL,CPU_INFO_FILE);
    return "";
}

/***************************************************************************
  Return the credits for the current CPU
***************************************************************************/
const char *cur_cpu_core_credits(void)
{
	if( activecpu >= 0 )
		return CPU_INFO(activecpu,NULL,CPU_INFO_CREDITS);
    return "";
}

/***************************************************************************
  Return the current program counter for the current CPU
***************************************************************************/
const char *cur_cpu_pc(void)
{
	UINT8 temp_context[CPU_CONTEXT_SIZE];

	if( activecpu >= 0 )
	{
		GETREGS (activecpu, temp_context);
		return CPU_INFO(activecpu,temp_context,CPU_INFO_PC);
	}
	return "";
}

/***************************************************************************
  Return the current stack pointer for the current CPU
***************************************************************************/
const char *cur_cpu_sp(void)
{
	UINT8 temp_context[CPU_CONTEXT_SIZE];

	if( activecpu >= 0 )
	{
		GETREGS (activecpu, temp_context);
		return CPU_INFO(activecpu,temp_context,CPU_INFO_SP);
	}
	return "";
}

/***************************************************************************
  Return a dissassembled instruction for the current CPU
***************************************************************************/
const char *cur_cpu_dasm(void)
{
	UINT8 temp_context[CPU_CONTEXT_SIZE];

	if( activecpu >= 0 )
	{
		GETREGS (activecpu, temp_context);
		return CPU_INFO(activecpu,temp_context,CPU_INFO_DASM);
	}
	return "";
}

/***************************************************************************
  Return a flags (state, condition codes) string for the current CPU
***************************************************************************/
const char *cur_cpu_flags(void)
{
	UINT8 temp_context[CPU_CONTEXT_SIZE];

	if( activecpu >= 0 )
	{
		GETREGS (activecpu, temp_context);
		return CPU_INFO(activecpu,temp_context,CPU_INFO_FLAGS);
	}
	return "";
}

/***************************************************************************
  Return a specific register string for a specific CPU
***************************************************************************/
const char *cur_cpu_register(int regnum)
{
	UINT8 temp_context[CPU_CONTEXT_SIZE];

	if( activecpu >= 0 )
	{
		GETREGS (activecpu, temp_context);
		return CPU_INFO(activecpu,temp_context,CPU_INFO_REG+regnum);
	}
	return "";
}

/***************************************************************************
  Return a state dump for the current CPU
***************************************************************************/
const char *cur_cpu_dump_state(void)
{
	static char buffer[1024+1];
	char *dst = buffer;
	const char *src;
	int regnum, width;

	dst += sprintf(dst, "CPU #%d [%s]", activecpu, cpu_name(CPU_TYPE(activecpu)));
	dst += sprintf(dst, " program counter %s %s\n", cpu_pc(activecpu), cpu_dasm(activecpu));
	regnum = 0;
	width = 0;
	src = cpu_register(activecpu,regnum);
	while (*src)
	{
		if( width + strlen(src) + 1 >= 80 )
		{
			dst += sprintf(dst, "\n");
			width = 0;
		}
		dst += sprintf(dst, "%s ", src);
		width += strlen(src) + 1;
		regnum++;
		src = cpu_register(activecpu,regnum);
	}

    return buffer;
}

/***************************************************************************
  Return the name for a CPU
***************************************************************************/
const char *cpu_name(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < MAX_CPU_TYPE )
		return IFC_INFO(cpu_type,NULL,CPU_INFO_NAME);
	return "";
}

/***************************************************************************
  Return the family name for a CPU
***************************************************************************/
const char *cpu_core_family(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < MAX_CPU_TYPE )
		return IFC_INFO(cpu_type,NULL,CPU_INFO_FAMILY);
    return "";
}

/***************************************************************************
  Return the version number for a CPU
***************************************************************************/
const char *cpu_core_version(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < MAX_CPU_TYPE )
		return IFC_INFO(cpu_type,NULL,CPU_INFO_VERSION);
    return "";
}

/***************************************************************************
  Return the core filename for a CPU
***************************************************************************/
const char *cpu_core_file(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < MAX_CPU_TYPE )
		return IFC_INFO(cpu_type,NULL,CPU_INFO_FILE);
    return "";
}

/***************************************************************************
  Return the credits for a CPU
***************************************************************************/
const char *cpu_core_credits(int cpu_type)
{
	cpu_type &= ~CPU_FLAGS_MASK;
	if( cpu_type < MAX_CPU_TYPE )
		return IFC_INFO(cpu_type,NULL,CPU_INFO_CREDITS);
    return "";
}

/***************************************************************************
  Return the current program counter for a specific CPU
***************************************************************************/
const char *cpu_pc(int cpunum)
{
	UINT8 temp_context[CPU_CONTEXT_SIZE];

	if( cpunum < MAX_CPU )
    {
		GETREGS (cpunum, temp_context);
		return CPU_INFO(cpunum,temp_context,CPU_INFO_PC);
	}
	return "";
}

/***************************************************************************
  Return the current stack pointer for a specific CPU
***************************************************************************/
const char *cpu_sp(int cpunum)
{
	UINT8 temp_context[CPU_CONTEXT_SIZE];

	if( cpunum < MAX_CPU )
	{
		GETREGS (cpunum, temp_context);
		return CPU_INFO(cpunum,temp_context,CPU_INFO_SP);
	}
	return "";
}

/***************************************************************************
  Return a dissassembled instruction for a specific CPU
***************************************************************************/
const char *cpu_dasm(int cpunum)
{
	UINT8 temp_context[CPU_CONTEXT_SIZE];

	if( cpunum < MAX_CPU )
	{
		GETREGS (cpunum, temp_context);
		return CPU_INFO(cpunum,temp_context,CPU_INFO_DASM);
	}
	return "";
}

/***************************************************************************
  Return a flags (state, condition codes) string for a specific CPU
***************************************************************************/
const char *cpu_flags(int cpunum)
{
	UINT8 temp_context[CPU_CONTEXT_SIZE];

	if( cpunum < MAX_CPU )
	{
		GETREGS (cpunum, temp_context);
		return CPU_INFO(cpunum,temp_context,CPU_INFO_FLAGS);
	}
	return "";
}

/***************************************************************************
  Return a specific register string for a specific CPU
***************************************************************************/
const char *cpu_register(int cpunum, int regnum)
{
	UINT8 temp_context[CPU_CONTEXT_SIZE];

	if( cpunum < MAX_CPU )
	{
		GETREGS (cpunum, temp_context);
		return CPU_INFO(cpunum,temp_context,CPU_INFO_REG+regnum);
	}
	return "";
}

/***************************************************************************
  Return a state dump for a specific CPU
***************************************************************************/
const char *cpu_dump_state(int cpunum)
{
	static char buffer[1024+1];
	char *dst = buffer;
	const char *src;
	int regnum, width;

	dst += sprintf(dst, "CPU #%d [%s]", cpunum, cpu_name(CPU_TYPE(cpunum)));
	dst += sprintf(dst, " program counter %s %s\n", cpu_pc(cpunum), cpu_dasm(cpunum));
	regnum = 0;
	width = 0;
	src = cpu_register(cpunum,regnum);
	while (*src)
	{
		if( width + strlen(src) + 1 >= 80 )
		{
			dst += sprintf(dst, "\n");
			width = 0;
		}
		dst += sprintf(dst, "%s ", src);
        width += strlen(src) + 1;
        width += strlen(src);
		regnum++;
		src = cpu_register(cpunum,regnum);
	}

    return buffer;
}

/***************************************************************************
  Dump all CPU's state to stderr
***************************************************************************/
void cpu_dump_states(void)
{
	int i;
	for( i = 0; i < MAX_CPU; i++ )
	{
		if( CPU_TYPE(i) != 0)
			fprintf(stderr, "%s\n\n", cpu_dump_state(i));
	}
}

/***************************************************************************

  Dummy interfaces for non-CPUs

***************************************************************************/
static void Dummy_reset(void *param) { }
static void Dummy_exit(void) { }
static int Dummy_execute(int cycles) { return cycles; }
static void Dummy_setregs(void *Regs) { }
static void Dummy_getregs(void *Regs) { }
static unsigned Dummy_getpc(void) { return 0; }
static unsigned Dummy_getreg(int regnum) { return 0; }
static void Dummy_setreg(int regnum, unsigned val) { }
static void Dummy_set_nmi_line(int state) { }
static void Dummy_set_irq_line(int irqline, int state) { }
static void Dummy_set_irq_callback(int (*callback)(int irqline)) { }

/****************************************************************************
 * Return a formatted string for a register
 * regnum	meaning
 * 0		CPU name
 * 1		Program counter
 * 2		Disassemble
 * 3		Flags
 * 14...n	Registers in the order you wish them to be displayed
 ****************************************************************************/
static const char *Dummy_info(void *context, int regnum)
{
	static char buffer[32+1];

	buffer[0] = '\0';
	if( !context && regnum )
		return buffer;

    switch (regnum)
	{
		case 0: sprintf(buffer,"Dummy"); break;
		case 1: sprintf(buffer,"%X", *(int*)context); break;
		case 2: sprintf(buffer,"???"); break;
		case 3: sprintf(buffer,"--------"); break;
		default: buffer[0] = '\0';
	}
	return buffer;
}

