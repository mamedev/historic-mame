/***************************************************************************

  cpuintrf.c

  Don't you love MS-DOS 8+3 names? That stands for CPU interface.
  Functions needed to interface the CPU emulator with the other parts of
  the emulation.

***************************************************************************/

#include "driver.h"
#include "cpu/z80/z80.h"
#include "cpu/i8039/i8039.h"
#include "cpu/i8085/i8085.h"
#include "cpu/m6502/m6502.h"
#include "cpu/m6809/m6809.h"
#include "cpu/m6808/m6808.h"
#include "cpu/m6805/m6805.h"
#include "cpu/m68000/m68000.h"
#include "cpu/s2650/s2650.h"
#include "cpu/t11/t11.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/i86/i86intrf.h"
#include "cpu/tms9900/tms9900.h"
#include "cpu/z8000/z8000.h"
#include "cpu/tms32010/tms32010.h"
#include "cpu/h6280/h6280.h"
#include "timer.h"


/* these are triggers sent to the timer system for various interrupt events */
#define TRIGGER_TIMESLICE       -1000
#define TRIGGER_INT             -2000
#define TRIGGER_YIELDTIME       -3000
#define TRIGGER_SUSPENDTIME     -4000

#define VERBOSE 0

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

#if NEW_INTERRUPT_SYSTEM
static int irq_line_state[MAX_CPU * MAX_IRQ_LINES];
static int irq_line_vector[MAX_CPU * MAX_IRQ_LINES];
#endif

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
#if NEW_INTERRUPT_SYSTEM
static void cpu_internal_interrupt(int cpunum, int type);
static void cpu_manualnmicallback(int param);
static void cpu_manualirqcallback(int param);
static void cpu_internalintcallback(int param);
#endif
static void cpu_manualintcallback (int param);
static void cpu_clearintcallback (int param);
static void cpu_resetcallback (int param);
static void cpu_timeslicecallback (int param);
static void cpu_vblankreset (void);
static void cpu_vblankcallback (int param);
static void cpu_updatecallback (int param);
static double cpu_computerate (int value);
static void cpu_inittimers (void);


#if NEW_INTERRUPT_SYSTEM

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

#endif

/* dummy interfaces for non-CPUs */
static int dummy_icount;
static void Dummy_SetRegs(void *Regs);
static void Dummy_GetRegs(void *Regs);
static unsigned Dummy_GetPC(void);
static void Dummy_Reset(void);
static int Dummy_Execute(int cycles);
#if NEW_INTERRUPT_SYSTEM
static void Dummy_set_nmi_line(int state);
static void Dummy_set_irq_line(int irqline, int state);
static void Dummy_set_irq_callback(int (*callback)(int irqline));
#else
static void Dummy_Cause_Interrupt(int type);
static void Dummy_Clear_Pending_Interrupts(void);
#endif

/* reset function wrapper */
static void Z80_Reset_with_daisychain(void);
/* pointers of daisy chain link */
static Z80_DaisyChain *Z80_daisychain[MAX_CPU];

/* Convenience macros - not in cpuintrf.h because they shouldn't be used by everyone */
#define RESET(index)                    ((*cpu[index].intf->reset)())
#define EXECUTE(index,cycles)           ((*cpu[index].intf->execute)(cycles))
#define SETREGS(index,regs)             ((*cpu[index].intf->set_regs)(regs))
#define GETREGS(index,regs)             ((*cpu[index].intf->get_regs)(regs))
#define GETPC(index)                    ((*cpu[index].intf->get_pc)())
#if NEW_INTERRUPT_SYSTEM
#define SET_NMI_LINE(index,state)       ((*cpu[index].intf->set_nmi_line)(state))
#define SET_IRQ_LINE(index,line,state)	((*cpu[index].intf->set_irq_line)(line,state))
#define SET_IRQ_CALLBACK(index,callback) ((*cpu[index].intf->set_irq_callback)(callback))
#define INTERNAL_INTERRUPT(index,type)	if( cpu[index].intf->internal_interrupt ) ((*cpu[index].intf->internal_interrupt)(type))
#else
#define CAUSE_INTERRUPT(index,type)     ((*cpu[index].intf->cause_interrupt)(type))
#define CLEAR_PENDING_INTERRUPTS(index) ((*cpu[index].intf->clear_pending_interrupts)())
#endif
#define ICOUNT(index)                   (*cpu[index].intf->icount)
#define INT_TYPE_NONE(index)            (cpu[index].intf->no_int)
#define INT_TYPE_IRQ(index) 			(cpu[index].intf->irq_int)
#define INT_TYPE_NMI(index)             (cpu[index].intf->nmi_int)
#define SET_OP_BASE(index,pc)           ((*cpu[index].intf->set_op_base)(pc))

#define CPU_TYPE(index) 				(Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK)
#define CPU_AUDIO(index)				(Machine->drv->cpu[index].cpu_type & CPU_AUDIO_CPU)

/* warning these must match the defines in driver.h! */
struct cpu_interface cpuintf[] =
{
	/* Dummy CPU -- placeholder for type 0 */
	{
		Dummy_Reset,						/* Reset CPU */
		Dummy_Execute,						/* Execute a number of cycles */
		(void (*)(void *))Dummy_SetRegs,	/* Set the contents of the registers */
		(void (*)(void *))Dummy_GetRegs,	/* Get the contents of the registers */
		Dummy_GetPC,						/* Return the current PC */
#if NEW_INTERRUPT_SYSTEM
		Dummy_set_nmi_line, 				/* Set state of the NMI line */
		Dummy_set_irq_line, 				/* Set state of the IRQ line */
		Dummy_set_irq_callback, 			/* Set IRQ enable/vector callback */
		0,									/* Cause internal interrupt */
		1,									/* Number of IRQ lines */
#else
		Dummy_Cause_Interrupt,				/* Generate an interrupt */
		Dummy_Clear_Pending_Interrupts, 	/* Clear pending interrupts */
#endif
		&dummy_icount,						/* Pointer to the instruction count */
		0,-1,-1,							/* Interrupt types: none, IRQ, NMI */
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
	},
	/* #define CPU_Z80    1 */
	{
		Z80_Reset_with_daisychain,			/* Reset CPU */
		Z80_Execute,						/* Execute a number of cycles */
		(void (*)(void *))Z80_SetRegs,		/* Set the contents of the registers */
		(void (*)(void *))Z80_GetRegs,		/* Get the contents of the registers */
		Z80_GetPC,							/* Return the current PC */
#if NEW_INTERRUPT_SYSTEM
		Z80_set_nmi_line,					/* Set state of the NMI line */
		Z80_set_irq_line,					/* Set state of the IRQ line */
		Z80_set_irq_callback,				/* Set IRQ enable/vector callback */
		0,									/* Cause internal interrupt */
        1,                                  /* Number of IRQ lines */
#else
		Z80_Cause_Interrupt,				/* Generate an interrupt */
		Z80_Clear_Pending_Interrupts,		/* Clear pending interrupts */
#endif
		&Z80_ICount,						/* Pointer to the instruction count */
		Z80_IGNORE_INT,Z80_IRQ_INT,Z80_NMI_INT, /* Interrupt types: none, IRQ, NMI */
        cpu_readmem16,                      /* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
	},
	/* #define CPU_8085A 2 */
	{
		I8085_Reset,                        /* Reset CPU */
		I8085_Execute,                      /* Execute a number of cycles */
		(void (*)(void *))I8085_SetRegs,    /* Set the contents of the registers */
		(void (*)(void *))I8085_GetRegs,    /* Get the contents of the registers */
		(unsigned int (*)(void))I8085_GetPC,/* Return the current PC */
#if NEW_INTERRUPT_SYSTEM
		I8085_set_nmi_line, 				/* Set state of the NMI line */
		I8085_set_irq_line, 				/* Set state of the IRQ line */
		I8085_set_irq_callback, 			/* Set IRQ enable/vector callback */
		0,									/* Cause internal interrupt */
        4,                                  /* Number of IRQ lines */
#else
        I8085_Cause_Interrupt,              /* Generate an interrupt */
		I8085_Clear_Pending_Interrupts,     /* Clear pending interrupts */
#endif
        &I8085_ICount,                      /* Pointer to the instruction count */
		I8085_NONE,I8085_INTR,I8085_TRAP,   /* Interrupt types: none, IRQ, NMI */
		cpu_readmem16,                      /* Memory read */
		cpu_writemem16,                     /* Memory write */
		cpu_setOPbase16,                    /* Update CPU opcode base */
		16,                                 /* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16    /* Address bits, for the memory system */
	},
    /* #define CPU_M6502  3 */
	{
		m6502_Reset,						/* Reset CPU */
		m6502_Execute,						/* Execute a number of cycles */
		(void (*)(void *))m6502_SetRegs,	/* Set the contents of the registers */
		(void (*)(void *))m6502_GetRegs,	/* Get the contents of the registers */
		m6502_GetPC,						/* Return the current PC */
#if NEW_INTERRUPT_SYSTEM
		m6502_set_nmi_line, 				/* Set state of the NMI line */
		m6502_set_irq_line, 				/* Set state of the IRQ line */
		m6502_set_irq_callback, 			/* Set IRQ enable/vector callback */
		0,									/* Cause internal interrupt */
        1,                                  /* Number of IRQ lines */
#else
		m6502_Cause_Interrupt,				/* Generate an interrupt */
		m6502_Clear_Pending_Interrupts, 	/* Clear pending interrupts */
#endif
		&m6502_ICount,						/* Pointer to the instruction count */
		M6502_INT_NONE,M6502_INT_IRQ,M6502_INT_NMI, /* Interrupt types: none, IRQ, NMI */
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
	},
	/* #define CPU_H6280 4 */
	{
 		H6280_Reset,					   /* Reset CPU */
 		H6280_Execute,					   /* Execute a number of cycles */
 		(void (*)(void *))H6280_SetRegs,   /* Set the contents of the registers */
 		(void (*)(void *))H6280_GetRegs,   /* Get the contents of the registers */
 		H6280_GetPC,					   /* Return the current PC */
#if NEW_INTERRUPT_SYSTEM
		H6280_set_nmi_line, 			    /* Set state of the NMI line */
		H6280_set_irq_line, 			    /* Set state of the IRQ line */
		H6280_set_irq_callback, 		    /* Set IRQ enable/vector callback */
		0,									/* Cause internal interrupt */
		3,									/* Number of IRQ lines */
#else
 		H6280_Cause_Interrupt,			   /* Generate an interrupt */
 		H6280_Clear_Pending_Interrupts,    /* Clear pending interrupts */
#endif
 		&H6280_ICount,					   /* Pointer to the instruction count */
 		0,-1,H6280_INT_NMI,                /* Interrupt types: none, IRQ, NMI */
		cpu_readmem21,                     /* Memory read */
		cpu_writemem21,                    /* Memory write */
		cpu_setOPbase21,                   /* Update CPU opcode base */
		21,                                /* CPU address bits */
		ABITS1_21,ABITS2_21,ABITS_MIN_21   /* Address bits, for the memory system */
	},
    /* #define CPU_I86    5 */
	{
		i86_Reset,							/* Reset CPU */
		i86_Execute,						/* Execute a number of cycles */
		(void (*)(void *))i86_SetRegs,		/* Set the contents of the registers */
		(void (*)(void *))i86_GetRegs,		/* Get the contents of the registers */
		i86_GetPC,							/* Return the current PC */
#if NEW_INTERRUPT_SYSTEM
		i86_set_nmi_line,					/* Set state of the NMI line */
		i86_set_irq_line,					/* Set state of the IRQ line */
		i86_set_irq_callback,				/* Set IRQ enable/vector callback */
		0,									/* Cause internal interrupt */
        1,                                  /* Number of IRQ lines */
#else
		i86_Cause_Interrupt,				/* Generate an interrupt */
		i86_Clear_Pending_Interrupts,		/* Clear pending interrupts */
#endif
		&i86_ICount,						/* Pointer to the instruction count */
		I86_INT_NONE,-1000,I86_NMI_INT, 	/* Interrupt types: none, IRQ, NMI */
		cpu_readmem20,						/* Memory read */
		cpu_writemem20, 					/* Memory write */
		cpu_setOPbase20,					/* Update CPU opcode base */
		20, 								/* CPU address bits */
		ABITS1_20,ABITS2_20,ABITS_MIN_20	/* Address bits, for the memory system */
	},
	/* #define CPU_I8039  6 */
	{
		I8039_Reset,						/* Reset CPU */
		I8039_Execute,						/* Execute a number of cycles */
		(void (*)(void *))I8039_SetRegs,	/* Set the contents of the registers */
		(void (*)(void *))I8039_GetRegs,	/* Get the contents of the registers */
		I8039_GetPC,						/* Return the current PC */
#if NEW_INTERRUPT_SYSTEM
		I8039_set_nmi_line, 				/* Set state of the NMI line */
		I8039_set_irq_line, 				/* Set state of the IRQ line */
		I8039_set_irq_callback, 			/* Set IRQ enable/vector callback */
		0,									/* Cause internal interrupt */
        1,                                  /* Number of IRQ lines */
#else
		I8039_Cause_Interrupt,				/* Generate an interrupt */
		I8039_Clear_Pending_Interrupts, 	/* Clear pending interrupts */
#endif
		&I8039_ICount,						/* Pointer to the instruction count */
		I8039_IGNORE_INT,I8039_EXT_INT,-1,	/* Interrupt types: none, IRQ, NMI */
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
	},
	/* #define CPU_M6808  7 */
	{
		m6808_reset,						/* Reset CPU */
		m6808_execute,						/* Execute a number of cycles */
		(void (*)(void *))m6808_SetRegs,	/* Set the contents of the registers */
		(void (*)(void *))m6808_GetRegs,	/* Get the contents of the registers */
		m6808_GetPC,						/* Return the current PC */
#if NEW_INTERRUPT_SYSTEM
		m6808_set_nmi_line, 				/* Set state of the NMI line */
		m6808_set_irq_line, 				/* Set state of the IRQ line */
		m6808_set_irq_callback, 			/* Set IRQ enable/vector callback */
		0,									/* Cause internal interrupt */
        1,                                  /* Number of IRQ lines */
#else
		m6808_Cause_Interrupt,				/* Generate an interrupt */
		m6808_Clear_Pending_Interrupts, 	/* Clear pending interrupts */
#endif
		&m6808_ICount,						/* Pointer to the instruction count */
		M6808_INT_NONE,M6808_INT_IRQ,M6808_INT_NMI, /* Interrupt types: none, IRQ, NMI */
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
	},
	/* #define CPU_M6805  8 */
	{
		m6805_reset,						/* Reset CPU */
		m6805_execute,						/* Execute a number of cycles */
		(void (*)(void *))m6805_SetRegs,	/* Set the contents of the registers */
		(void (*)(void *))m6805_GetRegs,	/* Get the contents of the registers */
		m6805_GetPC,						/* Return the current PC */
#if NEW_INTERRUPT_SYSTEM
		m6805_set_nmi_line, 				/* Set state of the NMI line */
		m6805_set_irq_line, 				/* Set state of the IRQ line */
		m6805_set_irq_callback, 			/* Set IRQ enable/vector callback */
		0,									/* Cause internal interrupt */
        1,                                  /* Number of IRQ lines */
#else
		m6805_Cause_Interrupt,				/* Generate an interrupt */
		m6805_Clear_Pending_Interrupts, 	/* Clear pending interrupts */
#endif
		&m6805_ICount,						/* Pointer to the instruction count */
		M6805_INT_NONE,M6805_INT_IRQ,-1,	/* Interrupt types: none, IRQ, NMI */
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
	},
	/* #define CPU_M6809  9 */
	{
		m6809_reset,						/* Reset CPU */
		m6809_execute,						/* Execute a number of cycles */
		(void (*)(void *))m6809_SetRegs,	/* Set the contents of the registers */
		(void (*)(void *))m6809_GetRegs,	/* Get the contents of the registers */
		m6809_GetPC,						/* Return the current PC */
#if NEW_INTERRUPT_SYSTEM
		m6809_set_nmi_line, 				/* Set state of the NMI line */
		m6809_set_irq_line, 				/* Set state of the IRQ line */
		m6809_set_irq_callback, 			/* Set IRQ enable/vector callback */
		0,									/* Cause internal interrupt */
        2,                                  /* Number of IRQ lines */
#else
		m6809_Cause_Interrupt,				/* Generate an interrupt */
		m6809_Clear_Pending_Interrupts, 	/* Clear pending interrupts */
#endif
		&m6809_ICount,						/* Pointer to the instruction count */
		M6809_INT_NONE,M6809_INT_IRQ,M6809_INT_NMI, /* Interrupt types: none, IRQ, NMI */
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
	},
	/* #define CPU_M68000 10 */
	{
		MC68000_Reset,						/* Reset CPU */
		MC68000_Execute,					/* Execute a number of cycles */
		(void (*)(void *))MC68000_SetRegs,	/* Set the contents of the registers */
		(void (*)(void *))MC68000_GetRegs,	/* Get the contents of the registers */
		(unsigned int (*)(void))MC68000_GetPC,/* Return the current PC */
#if NEW_INTERRUPT_SYSTEM
		MC68000_set_nmi_line,				/* Set state of the NMI line */
		MC68000_set_irq_line,				/* Set state of the IRQ line */
		MC68000_set_irq_callback,			/* Set IRQ enable/vector callback */
		0,									/* Cause internal interrupt */
        8,                                  /* Number of IRQ lines */
#else
		MC68000_Cause_Interrupt,			/* Generate an interrupt */
		MC68000_Clear_Pending_Interrupts,	/* Clear pending interrupts */
#endif
		&MC68000_ICount,					/* Pointer to the instruction count */
		MC68000_INT_NONE,-1,-1, 			/* Interrupt types: none, IRQ, NMI */
		cpu_readmem24,						/* Memory read */
		cpu_writemem24, 					/* Memory write */
		cpu_setOPbase24,					/* Update CPU opcode base */
		24, 								/* CPU address bits */
		ABITS1_24,ABITS2_24,ABITS_MIN_24	/* Address bits, for the memory system */
        },
    /* #define CPU_T11  11 */
	{
		t11_reset,							/* Reset CPU */
		t11_execute,						/* Execute a number of cycles */
		(void (*)(void *))t11_SetRegs,		/* Set the contents of the registers */
		(void (*)(void *))t11_GetRegs,		/* Get the contents of the registers */
		t11_GetPC,							/* Return the current PC */
#if NEW_INTERRUPT_SYSTEM
		t11_set_nmi_line,					/* Set state of the NMI line */
		t11_set_irq_line,					/* Set state of the IRQ line */
		t11_set_irq_callback,				/* Set IRQ enable/vector callback */
		0,									/* Cause internal interrupt */
        4,                                  /* Number of IRQ lines */
#else
		t11_Cause_Interrupt,				/* Generate an interrupt */
		t11_Clear_Pending_Interrupts,		/* Clear pending interrupts */
#endif
		&t11_ICount,						/* Pointer to the instruction count */
		T11_INT_NONE,-1,-1, 				/* Interrupt types: none, IRQ, NMI */
		cpu_readmem16lew,					/* Memory read */
		cpu_writemem16lew,					/* Memory write */
		cpu_setOPbase16lew, 				/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16LEW,ABITS2_16LEW,ABITS_MIN_16LEW /* Address bits, for the memory system */
	},
	/* #define CPU_S2650 12 */
	{
		S2650_Reset,						/* Reset CPU */
		S2650_Execute,						/* Execute a number of cycles */
		(void (*)(void *))S2650_SetRegs,	/* Set the contents of the registers */
		(void (*)(void *))S2650_GetRegs,	/* Get the contents of the registers */
		(unsigned int (*)(void))S2650_GetPC,/* Return the current PC */
#if NEW_INTERRUPT_SYSTEM
		S2650_set_nmi_line, 				/* Set state of the NMI line */
		S2650_set_irq_line, 				/* Set state of the IRQ line */
		S2650_set_irq_callback, 			/* Set IRQ enable/vector callback */
		0,									/* Cause internal interrupt */
        2,                                  /* Number of IRQ lines */
#else
        S2650_Cause_Interrupt,              /* Generate an interrupt */
		S2650_Clear_Pending_Interrupts, 	/* Clear pending interrupts */
#endif
        &S2650_ICount,                      /* Pointer to the instruction count */
		S2650_INT_NONE,-1,-1,				/* Interrupt types: none, IRQ, NMI */
		cpu_readmem16,                      /* Memory read */
		cpu_writemem16,                     /* Memory write */
		cpu_setOPbase16,                    /* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
	},
	/* #define CPU_TMS34010 13 */
	{
		TMS34010_Reset,                     /* Reset CPU */
		TMS34010_Execute,                   /* Execute a number of cycles */
		(void (*)(void *))TMS34010_SetRegs, /* Set the contents of the registers */
		(void (*)(void *))TMS34010_GetRegs, /* Get the contents of the registers */
		(unsigned int (*)(void))TMS34010_GetPC, /* Return the current PC */
#if NEW_INTERRUPT_SYSTEM
		TMS34010_set_nmi_line,				/* Set state of the NMI line */
		TMS34010_set_irq_line,				/* Set state of the IRQ line */
		TMS34010_set_irq_callback,			/* Set IRQ enable/vector callback */
		TMS34010_internal_interrupt,		/* Cause internal interrupt */
        2,                                  /* Number of IRQ lines */
#else
        TMS34010_Cause_Interrupt,           /* Generate an interrupt */
		TMS34010_Clear_Pending_Interrupts,  /* Clear pending interrupts */
#endif
        &TMS34010_ICount,                   /* Pointer to the instruction count */
		TMS34010_INT_NONE,TMS34010_INT1,-1, /* Interrupt types: none, IRQ, NMI */
		cpu_readmem29,						/* Memory read */
		cpu_writemem29, 					/* Memory write */
		cpu_setOPbase29,					/* Update CPU opcode base */
		29, 								/* CPU address bits */
		ABITS1_29,ABITS2_29,ABITS_MIN_29	/* Address bits, for the memory system */
	},
	/* #define CPU_TMS9900 14 */
	{
		TMS9900_Reset,						/* Reset CPU */
		TMS9900_Execute,					/* Execute a number of cycles */
		(void (*)(void *))TMS9900_SetRegs,	/* Set the contents of the registers */
		(void (*)(void *))TMS9900_GetRegs,	/* Get the contents of the registers */
		(unsigned int (*)(void))TMS9900_GetPC,  /* Return the current PC */
#if NEW_INTERRUPT_SYSTEM
		TMS9900_set_nmi_line,				/* Set state of the NMI line */
		TMS9900_set_irq_line,				/* Set state of the IRQ line */
		TMS9900_set_irq_callback,			/* Set IRQ enable/vector callback */
		0,									/* Cause internal interrupt */
        1,                                  /* Number of IRQ lines */
#else
		TMS9900_Cause_Interrupt,			/* Generate an interrupt */
		TMS9900_Clear_Pending_Interrupts,	/* Clear pending interrupts */
#endif
		&TMS9900_ICount,					/* Pointer to the instruction count */
		TMS9900_NONE,-1,-1, 				/* Interrupt types: none, IRQ, NMI */
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
        /* Were all ...LEW */
	},
	/* #define CPU_Z8000 15 */
	{
		Z8000_Reset,						/* Reset CPU */
		Z8000_Execute,						/* Execute a number of cycles */
		(void (*)(void *))Z8000_SetRegs,	/* Set the contents of the registers */
		(void (*)(void *))Z8000_GetRegs,	/* Get the contents of the registers */
		(unsigned int (*)(void))Z8000_GetPC,/* Return the current PC */
#if NEW_INTERRUPT_SYSTEM
		Z8000_set_nmi_line, 				/* Set state of the NMI line */
		Z8000_set_irq_line, 				/* Set state of the IRQ line */
		Z8000_set_irq_callback, 			/* Set IRQ enable/vector callback */
		0,									/* Cause internal interrupt */
        2,                                  /* Number of IRQ lines */
#else
		Z8000_Cause_Interrupt,				/* Generate an interrupt */
		Z8000_Clear_Pending_Interrupts, 	/* Clear pending interrupts */
#endif
		&Z8000_ICount,						/* Pointer to the instruction count */
		Z8000_INT_NONE,Z8000_NVI,Z8000_NMI, /* Interrupt types: none, IRQ, NMI */
		cpu_readmem16,                      /* Memory read */
		cpu_writemem16,                     /* Memory write */
		cpu_setOPbase16,                    /* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
	/* #define CPU_TMS320C10 16 */
	{
		TMS320C10_Reset,					/* Reset CPU */
		TMS320C10_Execute,					/* Execute a number of cycles */
		(void (*)(void *))TMS320C10_SetRegs,/* Set the contents of the registers */
		(void (*)(void *))TMS320C10_GetRegs,/* Get the contents of the registers */
		TMS320C10_GetPC,					/* Return the current PC */
#if NEW_INTERRUPT_SYSTEM
		TMS320C10_set_nmi_line, 			/* Set state of the NMI line */
		TMS320C10_set_irq_line, 			/* Set state of the IRQ line */
		TMS320C10_set_irq_callback, 		/* Set IRQ enable/vector callback */
		0,									/* Cause internal interrupt */
		2,									/* Number of IRQ lines */
#else
		TMS320C10_Cause_Interrupts, 		/* Generate an interrupt */
		TMS320C10_Clear_Pending_Interrupts, /* Clear pending interrupts */
#endif
		&TMS320C10_ICount,					/* Pointer to the instruction count */
		TMS320C10_INT_NONE,-1,-1,			/* Interrupt types: none, IRQ, NMI */
		cpu_readmem16,						/* Memory read */
		cpu_writemem16, 					/* Memory write */
		cpu_setOPbase16,					/* Update CPU opcode base */
		16, 								/* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16	/* Address bits, for the memory system */
    },
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
			if ( i != j && CPU_TYPE(i) == CPU_TYPE(j) )
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
#if NEW_INTERRUPT_SYSTEM
		/* Set the irq callback for the cpu */
		SET_IRQ_CALLBACK(i,cpu_irq_callbacks[i]);
#endif
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
		if (have_to_reset) goto reset;

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



int cpu_getpc(void)
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

				Z80_GetRegs(&_regs);
				return RAM[_regs.SP.D] + (RAM[_regs.SP.D+1] << 8);
			}
			break;

		default:
	if (errorlog) fprintf(errorlog,"cpu_getreturnpc: unsupported CPU type %02x\n",CPU_TYPE(cpunum));
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

#if NEW_INTERRUPT_SYSTEM

/***************************************************************************

  These functions are called when a cpu calls the callback sent to it's
  set_irq_callback function. It clears the irq line if the current state
  is HOLD_LINE and returns the interrupt vector for that line.

***************************************************************************/
static int cpu_0_irq_callback(int irqline)
{
	if (irq_line_state[0 * MAX_IRQ_LINES + irqline] == HOLD_LINE) {
        SET_IRQ_LINE(0, irqline, CLEAR_LINE);
		irq_line_state[0 * MAX_IRQ_LINES + irqline] = CLEAR_LINE;
    }
	LOG((errorlog, "cpu_0_irq_callback(%d) $%04x\n", irqline, irq_line_vector[0 * MAX_IRQ_LINES + irqline]));
	return irq_line_vector[0 * MAX_IRQ_LINES + irqline];
}

static int cpu_1_irq_callback(int irqline)
{
	if (irq_line_state[1 * MAX_IRQ_LINES + irqline] == HOLD_LINE) {
        SET_IRQ_LINE(1, irqline, CLEAR_LINE);
		irq_line_state[1 * MAX_IRQ_LINES + irqline] = CLEAR_LINE;
    }
	LOG((errorlog, "cpu_1_irq_callback(%d) $%04x\n", irqline, irq_line_vector[1 * MAX_IRQ_LINES + irqline]));
	return irq_line_vector[1 * MAX_IRQ_LINES + irqline];
}

static int cpu_2_irq_callback(int irqline)
{
	if (irq_line_state[2 * MAX_IRQ_LINES + irqline] == HOLD_LINE) {
        SET_IRQ_LINE(2, irqline, CLEAR_LINE);
		irq_line_state[2 * MAX_IRQ_LINES + irqline] = CLEAR_LINE;
	}
	LOG((errorlog, "cpu_2_irq_callback(%d) $%04x\n", irqline, irq_line_vector[2 * MAX_IRQ_LINES + irqline]));
	return irq_line_vector[2 * MAX_IRQ_LINES + irqline];
}

static int cpu_3_irq_callback(int irqline)
{
	if (irq_line_state[3 * MAX_IRQ_LINES + irqline] == HOLD_LINE) {
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
	if (irqline < cpu[cpunum].intf->num_irqs) {
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

#endif

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

#if NEW_INTERRUPT_SYSTEM

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

    LOG((errorlog,"cpu_manunalnmicallback %d,%d\n",cpunum,state));

    switch (state) {
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

    LOG((errorlog,"cpu_manunalirqcallback %d,%d,%d\n",cpunum,irqline,state));

	irq_line_state[cpunum * MAX_IRQ_LINES + irqline] = state;
    switch (state) {
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

#endif

static void cpu_generate_interrupt (int cpunum, int (*func)(void), int num)
{
	int oldactive = activecpu;

	/* swap to the CPU's context */
    activecpu = cpunum;
	memorycontextswap (activecpu);
	if (cpu[activecpu].save_context) SETREGS (activecpu, cpu[activecpu].context);

	/* cause the interrupt, calling the function if it exists */
	if (func) num = (*func)();

#if NEW_INTERRUPT_SYSTEM
	/* wrapper for the new system */
	if (num != INT_TYPE_NONE(cpunum)) {
		LOG((errorlog,"CPU#%d interrupt type $%04x: ", cpunum, num));
		/* is it the NMI type interrupt of that CPU? */
		if (num == INT_TYPE_NMI(cpunum)) {

            LOG((errorlog,"NMI\n"));
			cpu_manualnmicallback (cpunum | (PULSE_LINE << 3) );

        } else {
			int irq_line;

			switch (CPU_TYPE(cpunum)) {

			case CPU_Z80: LOG((errorlog,"Z80 IRQ\n"));
				irq_line = 0;
				break;

            case CPU_8085A:
				switch (num) {
				case I8085_INTR:
					LOG((errorlog,"I8085 INTR\n"));
					irq_line = 0;
					break;
				case I8085_RST55:
					LOG((errorlog,"I8085 RST55\n"));
					irq_line = 1;
					break;
				case I8085_RST65:
					LOG((errorlog,"I8085 RST65\n"));
					irq_line = 2;
					break;
				case I8085_RST75:
					LOG((errorlog,"I8085 RST75\n"));
					irq_line = 3;
					break;
				default:
					LOG((errorlog,"I8085 unknown\n"));
					irq_line = 0;
				}
				break;

            case CPU_M6502:
				LOG((errorlog,"M6502 IRQ\n"));
				irq_line = 0;
				break;

            case CPU_I86:
				LOG((errorlog,"I86 IRQ\n"));
				irq_line = 0;
				break;

            case CPU_I8039:
				LOG((errorlog,"I8039 IRQ\n"));
				irq_line = 0;
				break;

			case CPU_M6808:
				switch (num) {
				case M6808_INT_IRQ:
					LOG((errorlog,"M6808 IRQ\n"));
					irq_line = 0;
					break;
				case M6808_INT_OCI:
					LOG((errorlog,"M6808 OCI\n"));
					irq_line = 1;
					break;
				default:
					LOG((errorlog,"M6808 unknown\n"));
					irq_line = 0;
                    break;
                }
				break;

            case CPU_M6805:
				LOG((errorlog,"M6805 IRQ\n"));
				irq_line = 0;
				break;

            case CPU_M6809:
				switch (num) {
				case M6809_INT_IRQ:
					LOG((errorlog,"M6809 IRQ\n"));
					irq_line = 0;
					break;
				case M6809_INT_FIRQ:
					LOG((errorlog,"M6809 FIRQ\n"));
					irq_line = 1;
					break;
				default:
					LOG((errorlog,"M6809 unknown\n"));
					irq_line = 0;
				}
				break;

            case CPU_M68000:
				switch (num) {
					case MC68000_IRQ_1:
						LOG((errorlog,"M68000 IRQ1\n"));
						irq_line = 1;
						break;
					case MC68000_IRQ_2:
						LOG((errorlog,"M68000 IRQ2\n"));
						irq_line = 2;
                        break;
					case MC68000_IRQ_3:
						LOG((errorlog,"M68000 IRQ3\n"));
						irq_line = 3;
                        break;
					case MC68000_IRQ_4:
						LOG((errorlog,"M68000 IRQ4\n"));
						irq_line = 4;
                        break;
					case MC68000_IRQ_5:
						LOG((errorlog,"M68000 IRQ5\n"));
						irq_line = 5;
                        break;
					case MC68000_IRQ_6:
						LOG((errorlog,"M68000 IRQ6\n"));
						irq_line = 6;
                        break;
					case MC68000_IRQ_7:
						LOG((errorlog,"M68000 IRQ7\n"));
						irq_line = 7;
                        break;
					default:
						LOG((errorlog,"M68000 unknown\n"));
						irq_line = 0;
                }
				/* until now only auto vector interrupts supported */
                num = MC68000_INT_ACK_AUTOVECTOR;
				break;

            case CPU_T11:
				switch (num) {
					case T11_IRQ0:
						LOG((errorlog,"T11 IRQ0\n"));
						irq_line = 0;
						break;
					case T11_IRQ1:
						LOG((errorlog,"T11 IRQ1\n"));
						irq_line = 1;
                        break;
					case T11_IRQ2:
						LOG((errorlog,"T11 IRQ2\n"));
						irq_line = 2;
                        break;
					case T11_IRQ3:
						LOG((errorlog,"T11 IRQ3\n"));
						irq_line = 3;
                        break;
					default:
						LOG((errorlog,"T11 unknown\n"));
						irq_line = 0;
                }
                break;

            case CPU_S2650:
				LOG((errorlog,"S2650 IRQ\n"));
				irq_line = 0;
                break;

            case CPU_TMS34010:
				switch (num) {
					case TMS34010_INT1:
						LOG((errorlog,"TMS34010 INT1\n"));
						irq_line = 0;
                        break;
					case TMS34010_INT2:
						LOG((errorlog,"TMS34010 INT2\n"));
						irq_line = 1;
                        break;
                    default:
						LOG((errorlog,"TMS34010 unknown\n"));
						irq_line = 0;
                }
                break;

            case CPU_TMS9900:
				LOG((errorlog,"TMS9900 IRQ\n"));
				irq_line = 0;
                break;

            case CPU_Z8000:
				switch (num) {
					case Z8000_NVI:
						LOG((errorlog,"Z8000 NVI\n"));
						irq_line = 0;
                        break;
					case Z8000_VI:
						LOG((errorlog,"Z8000 VI\n"));
						irq_line = 1;
                        break;
					default:
						LOG((errorlog,"Z8000 unknown\n"));
						irq_line = 0;
                }
                break;

            case CPU_TMS320C10:
				switch (num) {
					case TMS320C10_ACTIVE_INT:
						LOG((errorlog,"TMS32010 INT\n"));
						irq_line = 0;
                        break;
					case TMS320C10_ACTIVE_BIO:
						LOG((errorlog,"TMS32010 BIO\n"));
						irq_line = 1;
                        break;
                    default:
						LOG((errorlog,"TMS32010 unknown\n"));
						irq_line = 0;
                }
                break;

            case CPU_H6280:
				switch (num) {
					case H6280_INT_IRQ1:
						LOG((errorlog,"H6280 INT 1\n"));
						irq_line = 0;
                        break;
					case H6280_INT_IRQ2:
						LOG((errorlog,"H6280 INT 2\n"));
						irq_line = 1;
                        break;
					case H6280_INT_TIMER:
						LOG((errorlog,"H6280 TIMER INT\n"));
						irq_line = 2;
                        break;
                    default:
						LOG((errorlog,"H6280 unknown\n"));
						irq_line = 0;
                }
                break;

            default:
				/* else it should be an IRQ type; assume line 0 and store vector */
				LOG((errorlog,"unknown IRQ\n"));
				irq_line = 0;
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
#else

    CAUSE_INTERRUPT (cpunum, num);

    /* update the CPU's context */
    if (cpu[activecpu].save_context) GETREGS (activecpu, cpu[activecpu].context);
    activecpu = oldactive;
    if (activecpu >= 0) memorycontextswap (activecpu);

	/* generate a trigger to unsuspend any CPUs waiting on the interrupt */
    if (num != INT_TYPE_NONE (cpunum))
        timer_trigger (TRIGGER_INT + cpunum);
#endif
}

static void cpu_clear_interrupts (int cpunum)
{
	int oldactive = activecpu;
	int i;

	/* swap to the CPU's context */
	activecpu = cpunum;
	memorycontextswap (activecpu);
	if (cpu[activecpu].save_context) SETREGS (activecpu, cpu[activecpu].context);

#if NEW_INTERRUPT_SYSTEM
	SET_NMI_LINE(activecpu,CLEAR_LINE);
	for (i = 0; i < cpu[activecpu].intf->num_irqs; i++)
		SET_IRQ_LINE(activecpu,i,CLEAR_LINE);
#else
	/* clear pending interrupts, calling the function if it exists */
    CLEAR_PENDING_INTERRUPTS (cpunum);
#endif

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
#if NEW_INTERRUPT_SYSTEM
	/* Set the irq callback for the cpu */
	SET_IRQ_CALLBACK(cpunum,cpu_irq_callbacks[cpunum]);
#endif

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
if (errorlog) fprintf(errorlog,"Reset caused by the watchdog\n");
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


/* Reset Z80 with set daisychain link */
static void Z80_Reset_with_daisychain(void)
{
	Z80_Reset( Z80_daisychain[activecpu] );
}

/* Set Z80 daisy chain link (upload when after reset ) */
void cpu_setdaisychain (int cpunum, Z80_DaisyChain *daisy_chain )
{
	Z80_daisychain[cpunum] = daisy_chain;
}

/* dummy interfaces for non-CPUs */
static void Dummy_SetRegs(void *Regs) { }
static void Dummy_GetRegs(void *Regs) { }
static unsigned Dummy_GetPC(void) { return 0; }
static void Dummy_Reset(void) { }
static int Dummy_Execute(int cycles) { return cycles; }
#if NEW_INTERRUPT_SYSTEM
static void Dummy_set_nmi_line(int state) { }
static void Dummy_set_irq_line(int irqline, int state) { }
static void Dummy_set_irq_callback(int (*callback)(int irqline)) { }
#else
static void Dummy_Cause_Interrupt(int type) { }
static void Dummy_Clear_Pending_Interrupts(void) { }
#endif
