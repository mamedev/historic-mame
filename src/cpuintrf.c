/***************************************************************************

  cpuintrf.c

  Don't you love MS-DOS 8+3 names? That stands for CPU interface.
  Functions needed to interface the CPU emulator with the other parts of
  the emulation.

***************************************************************************/

#include "driver.h"
#include "Z80/Z80.h"
#include "I8039/I8039.h"
#include "M6502/M6502.h"
#include "M6809/M6809.h"
#include "M6808/M6808.h"
#include "M6805/M6805.h"
#include "M68000/M68000.h"
#include "I86/i86intrf.h"
#include "timer.h"


/* these are triggers sent to the timer system for various interrupt events */
#define TRIGGER_TIMESLICE       -1000
#define TRIGGER_INT             -2000
#define TRIGGER_YIELDTIME       -3000
#define TRIGGER_SUSPENDTIME     -4000


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
	unsigned char context[CPU_CONTEXT_SIZE];   /* this CPU's context */
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


static void *watchdog_timer;

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

static void cpu_generate_interrupt (int cpu, int (*func)(void), int num);
static void cpu_vblankintcallback (int param);
static void cpu_timedintcallback (int param);
static void cpu_manualintcallback (int param);
static void cpu_clearintcallback (int param);
static void cpu_resetcallback (int param);
static void cpu_timeslicecallback (int param);
static void cpu_vblankreset (void);
static void cpu_vblankcallback (int param);
static void cpu_updatecallback (int param);
static double cpu_computerate (int value);
static void cpu_inittimers (void);



/* interfaces to the 6502 so that it looks like the other CPUs */
static void m6502_SetRegs(M6502 *Regs);
static void m6502_GetRegs(M6502 *Regs);
static unsigned m6502_GetPC(void);
static void m6502_Reset(void);
static int m6502_Execute(int cycles);
static void m6502_Cause_Interrupt(int type);
static void m6502_Clear_Pending_Interrupts(void);
static int dummy_icount;

/* dummy interfaces for non-CPUs */
static M6502 Current6502;
static void Dummy_SetRegs(void *Regs);
static void Dummy_GetRegs(void *Regs);
static unsigned Dummy_GetPC(void);
static void Dummy_Reset(void);
static int Dummy_Execute(int cycles);
static void Dummy_Cause_Interrupt(int type);
static void Dummy_Clear_Pending_Interrupts(void);

#ifdef Z80_DAISYCHAIN
static void z80_Reset(void);
/* pointers of daisy chain link */
static Z80_DaisyChain *z80_daisychain[MAX_CPU];
#endif

/* warning these must match the defines in driver.h! */
struct cpu_interface cpuintf[] =
{
	/* Dummy CPU -- placeholder for type 0 */
	{
		Dummy_Reset,                       /* Reset CPU */
		Dummy_Execute,                     /* Execute a number of cycles */
		(void (*)(void *))Dummy_SetRegs,             /* Set the contents of the registers */
		(void (*)(void *))Dummy_GetRegs,             /* Get the contents of the registers */
		Dummy_GetPC,                       /* Return the current PC */
		Dummy_Cause_Interrupt,             /* Generate an interrupt */
		Dummy_Clear_Pending_Interrupts,    /* Clear pending interrupts */
		&dummy_icount,                     /* Pointer to the instruction count */
		0,-1,-1,                           /* Interrupt types: none, IRQ, NMI */
		cpu_readmem16,                     /* Memory read */
		cpu_writemem16,                    /* Memory write */
		cpu_setOPbase16,                   /* Update CPU opcode base */
		16,                                /* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16   /* Address bits, for the memory system */
	},
	/* #define CPU_Z80    1 */
	{
#ifdef Z80_DAISYCHAIN
		z80_Reset,                         /* Reset CPU */
#else
		Z80_Reset,                         /* Reset CPU */
#endif
		Z80_Execute,                       /* Execute a number of cycles */
		(void (*)(void *))Z80_SetRegs,               /* Set the contents of the registers */
		(void (*)(void *))Z80_GetRegs,               /* Get the contents of the registers */
		Z80_GetPC,                         /* Return the current PC */
		Z80_Cause_Interrupt,               /* Generate an interrupt */
		Z80_Clear_Pending_Interrupts,      /* Clear pending interrupts */
		&Z80_ICount,                       /* Pointer to the instruction count */
		Z80_IGNORE_INT,-1000,Z80_NMI_INT,  /* Interrupt types: none, IRQ, NMI */
		cpu_readmem16,                     /* Memory read */
		cpu_writemem16,                    /* Memory write */
		cpu_setOPbase16,                   /* Update CPU opcode base */
		16,                                /* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16   /* Address bits, for the memory system */
	},
	/* #define CPU_M6502  2 */
	{
		m6502_Reset,                       /* Reset CPU */
		m6502_Execute,                     /* Execute a number of cycles */
		(void (*)(void *))m6502_SetRegs,             /* Set the contents of the registers */
		(void (*)(void *))m6502_GetRegs,             /* Get the contents of the registers */
		m6502_GetPC,                       /* Return the current PC */
		m6502_Cause_Interrupt,             /* Generate an interrupt */
		m6502_Clear_Pending_Interrupts,    /* Clear pending interrupts */
		&Current6502.ICount,               /* Pointer to the instruction count */
		INT_NONE,INT_IRQ,INT_NMI,          /* Interrupt types: none, IRQ, NMI */
		cpu_readmem16,                     /* Memory read */
		cpu_writemem16,                    /* Memory write */
		cpu_setOPbase16,                   /* Update CPU opcode base */
		16,                                /* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16   /* Address bits, for the memory system */
	},
	/* #define CPU_I86    3 */
	{
		i86_Reset,                         /* Reset CPU */
		i86_Execute,                       /* Execute a number of cycles */
		(void (*)(void *))i86_SetRegs,               /* Set the contents of the registers */
		(void (*)(void *))i86_GetRegs,               /* Get the contents of the registers */
		i86_GetPC,                         /* Return the current PC */
		i86_Cause_Interrupt,               /* Generate an interrupt */
		i86_Clear_Pending_Interrupts,      /* Clear pending interrupts */
		&i86_ICount,                       /* Pointer to the instruction count */
		I86_INT_NONE,I86_NMI_INT,I86_NMI_INT,/* Interrupt types: none, IRQ, NMI */
		cpu_readmem20,                     /* Memory read */
		cpu_writemem20,                    /* Memory write */
		cpu_setOPbase20,                   /* Update CPU opcode base */
		20,                                /* CPU address bits */
		ABITS1_20,ABITS2_20,ABITS_MIN_20   /* Address bits, for the memory system */
	},
	/* #define CPU_I8039  4 */
	{
		I8039_Reset,                       /* Reset CPU */
		I8039_Execute,                     /* Execute a number of cycles */
		(void (*)(void *))I8039_SetRegs,             /* Set the contents of the registers */
		(void (*)(void *))I8039_GetRegs,             /* Get the contents of the registers */
		I8039_GetPC,                       /* Return the current PC */
		I8039_Cause_Interrupt,             /* Generate an interrupt */
		I8039_Clear_Pending_Interrupts,    /* Clear pending interrupts */
		&I8039_ICount,                     /* Pointer to the instruction count */
		I8039_IGNORE_INT,I8039_EXT_INT,-1, /* Interrupt types: none, IRQ, NMI */
		cpu_readmem16,                     /* Memory read */
		cpu_writemem16,                    /* Memory write */
		cpu_setOPbase16,                   /* Update CPU opcode base */
		16,                                /* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16   /* Address bits, for the memory system */
	},
	/* #define CPU_M6808  5 */
	{
		m6808_reset,                       /* Reset CPU */
		m6808_execute,                     /* Execute a number of cycles */
		(void (*)(void *))m6808_SetRegs,             /* Set the contents of the registers */
		(void (*)(void *))m6808_GetRegs,             /* Get the contents of the registers */
		m6808_GetPC,                       /* Return the current PC */
		m6808_Cause_Interrupt,             /* Generate an interrupt */
		m6808_Clear_Pending_Interrupts,    /* Clear pending interrupts */
		&m6808_ICount,                     /* Pointer to the instruction count */
		M6808_INT_NONE,M6808_INT_IRQ,M6808_INT_NMI, /* Interrupt types: none, IRQ, NMI */
		cpu_readmem16,                     /* Memory read */
		cpu_writemem16,                    /* Memory write */
		cpu_setOPbase16,                   /* Update CPU opcode base */
		16,                                /* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16   /* Address bits, for the memory system */
	},
	/* #define CPU_M6805  6 */
	{
		m6805_reset,                       /* Reset CPU */
		m6805_execute,                     /* Execute a number of cycles */
		(void (*)(void *))m6805_SetRegs,             /* Set the contents of the registers */
		(void (*)(void *))m6805_GetRegs,             /* Get the contents of the registers */
		m6805_GetPC,                       /* Return the current PC */
		m6805_Cause_Interrupt,             /* Generate an interrupt */
		m6805_Clear_Pending_Interrupts,    /* Clear pending interrupts */
		&m6805_ICount,                     /* Pointer to the instruction count */
		M6805_INT_NONE,M6805_INT_IRQ,-1,   /* Interrupt types: none, IRQ, NMI */
		cpu_readmem16,                     /* Memory read */
		cpu_writemem16,                    /* Memory write */
		cpu_setOPbase16,                   /* Update CPU opcode base */
		16,                                /* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16   /* Address bits, for the memory system */
	},
	/* #define CPU_M6809  7 */
	{
		m6809_reset,                       /* Reset CPU */
		m6809_execute,                     /* Execute a number of cycles */
		(void (*)(void *))m6809_SetRegs,             /* Set the contents of the registers */
		(void (*)(void *))m6809_GetRegs,             /* Get the contents of the registers */
		m6809_GetPC,                       /* Return the current PC */
		m6809_Cause_Interrupt,             /* Generate an interrupt */
		m6809_Clear_Pending_Interrupts,    /* Clear pending interrupts */
		&m6809_ICount,                     /* Pointer to the instruction count */
		M6809_INT_NONE,M6809_INT_IRQ,M6809_INT_NMI, /* Interrupt types: none, IRQ, NMI */
		cpu_readmem16,                     /* Memory read */
		cpu_writemem16,                    /* Memory write */
		cpu_setOPbase16,                   /* Update CPU opcode base */
		16,                                /* CPU address bits */
		ABITS1_16,ABITS2_16,ABITS_MIN_16   /* Address bits, for the memory system */
	},
	/* #define CPU_M68000 8 */
	{
		MC68000_Reset,                     /* Reset CPU */
		MC68000_Execute,                   /* Execute a number of cycles */
		(void (*)(void *))MC68000_SetRegs,           /* Set the contents of the registers */
		(void (*)(void *))MC68000_GetRegs,           /* Get the contents of the registers */
		(unsigned int (*)(void))MC68000_GetPC,             /* Return the current PC */
		MC68000_Cause_Interrupt,           /* Generate an interrupt */
		MC68000_Clear_Pending_Interrupts,  /* Clear pending interrupts */
		&MC68000_ICount,                   /* Pointer to the instruction count */
		MC68000_INT_NONE,-1,-1,            /* Interrupt types: none, IRQ, NMI */
		cpu_readmem24,                     /* Memory read */
		cpu_writemem24,                    /* Memory write */
		cpu_setOPbase24,                   /* Update CPU opcode base */
		24,                                /* CPU address bits */
		ABITS1_24,ABITS2_24,ABITS_MIN_24   /* Address bits, for the memory system */
	}
};

/* Convenience macros - not in cpuintrf.h because they shouldn't be used by everyone */
#define RESET(index)                    ((*cpu[index].intf->reset)())
#define EXECUTE(index,cycles)           ((*cpu[index].intf->execute)(cycles))
#define SETREGS(index,regs)             ((*cpu[index].intf->set_regs)(regs))
#define GETREGS(index,regs)             ((*cpu[index].intf->get_regs)(regs))
#define GETPC(index)                    ((*cpu[index].intf->get_pc)())
#define CAUSE_INTERRUPT(index,type)     ((*cpu[index].intf->cause_interrupt)(type))
#define CLEAR_PENDING_INTERRUPTS(index) ((*cpu[index].intf->clear_pending_interrupts)())
#define ICOUNT(index)                   (*cpu[index].intf->icount)
#define INT_TYPE_NONE(index)            (cpu[index].intf->no_int)
#define INT_TYPE_IRQ(index)             (cpu[index].intf->irq_int)
#define INT_TYPE_NMI(index)             (cpu[index].intf->nmi_int)

#define SET_OP_BASE(index,pc)           ((*cpu[index].intf->set_op_base)(pc))


void cpu_init(void)
{
	int i;

	/* count how many CPUs we have to emulate */
	totalcpu = 0;

	while (totalcpu < MAX_CPU)
	{
		if (Machine->drv->cpu[totalcpu].cpu_type == 0) break;
		totalcpu++;
	}

	/* zap the CPU data structure */
	memset (cpu, 0, sizeof (cpu));

	/* set up the interface functions */
	for (i = 0; i < MAX_CPU; i++)
		cpu[i].intf = &cpuintf[Machine->drv->cpu[i].cpu_type & ~CPU_FLAGS_MASK];
#ifdef Z80_DAISYCHAIN
	for (i = 0; i < MAX_CPU; i++)
		z80_daisychain[i] = 0;
#endif

	/* reset the timer system */
	timer_init ();
	timeslice_timer = refresh_timer = watchdog_timer = vblank_timer = NULL;
}



void cpu_run(void)
{
	int i;

	/* determine which CPUs need a context switch */
	for (i = 0; i < totalcpu; i++)
	{
		#ifdef MAME_DEBUG

			/* with the debugger, we need to save the contexts */
			cpu[i].save_context = 1;

		#else
			int j;


			/* otherwise, we only need to save if there is another CPU of the same type */
			cpu[i].save_context = 0;

			for (j = 0; j < totalcpu; j++)
				if (i != j && (Machine->drv->cpu[i].cpu_type & ~CPU_FLAGS_MASK) == (Machine->drv->cpu[j].cpu_type & ~CPU_FLAGS_MASK))
					cpu[i].save_context = 1;

		#endif
	}

reset:
	/* initialize the various timers (suspends all CPUs at startup) */
	cpu_inittimers ();

	/* enable all CPUs (except for audio CPUs if the sound is off) */
	for (i = 0; i < totalcpu; i++)
		if (!(Machine->drv->cpu[i].cpu_type & CPU_AUDIO_CPU) || Machine->sample_rate != 0)
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
#ifdef Z80_DAISYCHAIN
		if (cpu[i].save_context) SETREGS (i, cpu[i].context);
		activecpu = i;
#endif
		RESET (i);
		/* save the CPU context if necessary */
		if (cpu[i].save_context) GETREGS (i, cpu[i].context);

		/* reset the total number of cycles */
		cpu[i].totalcycles = 0;
	}

	/* reset the globals */
	cpu_vblankreset ();

	/* loop until the user quits */
	usres = 0;
	while (usres == 0)
	{
		int cpunum;

		/* was machine_reset() called? */
		if (have_to_reset) goto reset;

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
			ran = EXECUTE (activecpu, running);

			/* update based on how many cycles we really ran */
			cpu[activecpu].totalcycles += ran;

			/* update the contexts */
			if (cpu[activecpu].save_context) GETREGS (activecpu, cpu[activecpu].context);
			updatememorybase (activecpu);
			activecpu = -1;

			/* update the timer with how long we actually ran */
			timer_update_cpu (cpunum, ran);
		}
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
static void watchdog_callback (int param)
{
	watchdog_timer = 0;

	if (errorlog) fprintf(errorlog,"warning: reset caused by the watchdog\n");
	machine_reset ();
}

void watchdog_reset_w(int offset,int data)
{
	timer_reset (watchdog_timer, TIME_IN_SEC (1));
}

int watchdog_reset_r(int offset)
{
	timer_reset (watchdog_timer, TIME_IN_SEC (1));
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

	timer_suspendcpu (cpunum, !running);
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

	switch(Machine->drv->cpu[cpunum].cpu_type & ~CPU_FLAGS_MASK)
	{
		case CPU_Z80:
		case CPU_I8039:
		case CPU_M6502:
		case CPU_M68000:	/* ASG 980413 */
			return previouspc;
			break;

		default:
	if (errorlog) fprintf(errorlog,"cpu_getpreviouspc: unsupported CPU type %02x\n",Machine->drv->cpu[cpunum].cpu_type);
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

	switch(Machine->drv->cpu[cpunum].cpu_type & ~CPU_FLAGS_MASK)
	{
		case CPU_Z80:
			{
				Z80_Regs regs;


				Z80_GetRegs(&regs);
				return RAM[regs.SP.D] + (RAM[regs.SP.D+1] << 8);
			}
			break;

		default:
	if (errorlog) fprintf(errorlog,"cpu_getreturnpc: unsupported CPU type %02x\n",Machine->drv->cpu[cpunum].cpu_type);
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
	double scantime = timer_starttime (refresh_timer) + (double)scanline * scanline_period;
	double time = timer_gettime ();
	if (time >= scantime) scantime += TIME_IN_HZ (Machine->drv->frames_per_second);
	return scantime - time;
}


double cpu_getscanlineperiod(void)
{
	return scanline_period;
}



void cpu_seticount(int cycles)
{
	int cpunum = (activecpu < 0) ? 0 : activecpu;
	ICOUNT (cpunum) = cycles;
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


/***************************************************************************

  Internal CPU event processors.

***************************************************************************/
static void cpu_generate_interrupt (int cpunum, int (*func)(void), int num)
{
	int oldactive = activecpu;

	/* swap to the CPU's context */
	activecpu = cpunum;
	memorycontextswap (activecpu);
	if (cpu[activecpu].save_context) SETREGS (activecpu, cpu[activecpu].context);

	/* cause the interrupt, calling the function if it exists */
	if (func) num = (*func)();

	CAUSE_INTERRUPT (cpunum, num);

	/* update the CPU's context */
	if (cpu[activecpu].save_context) GETREGS (activecpu, cpu[activecpu].context);
	activecpu = oldactive;
	if (activecpu >= 0) memorycontextswap (activecpu);

	/* generate a trigger to unsuspend any CPUs waiting on the interrupt */
	if (num != INT_TYPE_NONE (cpunum))
		timer_trigger (TRIGGER_INT + cpunum);
}


static void cpu_clear_interrupts (int cpunum)
{
	int oldactive = activecpu;

	/* swap to the CPU's context */
	activecpu = cpunum;
	memorycontextswap (activecpu);
	if (cpu[activecpu].save_context) SETREGS (activecpu, cpu[activecpu].context);

	/* cause the interrupt, calling the function if it exists */
	CLEAR_PENDING_INTERRUPTS (cpunum);

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

		/* set the timer to update the screen */
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

	/* update the sound system */
	sound_update();

	/* update IPT_VBLANK input ports */
	inputport_vblank_end();

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
	if (watchdog_timer)
		timer_remove (watchdog_timer);
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
	scanline_period = (refresh_period - TIME_IN_USEC (Machine->drv->vblank_duration)) / (double)Machine->drv->screen_height;
	scanline_period_inv = 1.0 / scanline_period;

	/* allocate an infinite watchdog timer; it will be set to a sane value on a read/write */
	watchdog_timer = timer_set (TIME_NEVER, 0, watchdog_callback);

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



#ifdef MAME_DEBUG
/* JB 971019 */
void cpu_getcontext (int activecpu, unsigned char *buf)
{
    memcpy (buf, cpu[activecpu].context, CPU_CONTEXT_SIZE);
}

void cpu_setcontext (int activecpu, const unsigned char *buf)
{
    memcpy (cpu[activecpu].context, buf, CPU_CONTEXT_SIZE);
}
#endif


#ifdef Z80_DAISYCHAIN

/* reset Z80 with set daisychain link */
static void z80_Reset(void)
{
	Z80_Reset( z80_daisychain[activecpu] );
}
/* set z80 daisy chain link (upload when after reset ) */
void cpu_setdaisychain (int cpunum, Z80_DaisyChain *daisy_chain )
{
	z80_daisychain[cpunum] = daisy_chain;
}
#endif

/* some empty functions needed by the Z80 emulation */
void Z80_Patch (Z80_Regs *Regs)
{
}
void Z80_Reti (void)
{
}
void Z80_Retn (void)
{
}



/* interfaces to the 6502 so that it looks like the other CPUs */
static void m6502_SetRegs(M6502 *Regs)
{
	Current6502 = *Regs;
}
static void m6502_GetRegs(M6502 *Regs)
{
	*Regs = Current6502;
}
static unsigned m6502_GetPC(void)
{
	return Current6502.PC.W;
}
static void m6502_Reset(void)
{
	Reset6502 (&Current6502);
}
static int m6502_Execute(int cycles)
{
	return Run6502 (&Current6502, cycles);
}
static void m6502_Cause_Interrupt(int type)
{
	M6502_Cause_Interrupt (&Current6502, type);
}
static void m6502_Clear_Pending_Interrupts(void)
{
	M6502_Clear_Pending_Interrupts (&Current6502);
}


/* dummy interfaces for non-CPUs */
static void Dummy_SetRegs(void *Regs) { }
static void Dummy_GetRegs(void *Regs) { }
static unsigned Dummy_GetPC(void) { return 0; }
static void Dummy_Reset(void) { }
static int Dummy_Execute(int cycles) { return cycles; }
static void Dummy_Cause_Interrupt(int type) { }
static void Dummy_Clear_Pending_Interrupts(void) { }
