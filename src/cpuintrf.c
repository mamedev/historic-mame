/***************************************************************************

  cpuintrf.c

  Don't you love MS-DOS 8+3 names? That stands for CPU interface.
  Functions needed to interface the CPU emulator with the other parts of
  the emulation.

***************************************************************************/

#include "driver.h"
#include "Z80.h"
#include "I8039.h"
#include "M6502.h"
#include "M6809.h"
#include "M6808.h"
#include "M68000.h"
#include "i86intrf.h"

static int activecpu,totalcpu;
static int iloops[MAX_CPU];
static int cpurunning[MAX_CPU];
static int totalcycles[MAX_CPU];
static int current_slice;
static int ran_this_frame[MAX_CPU];
static int save_context[MAX_CPU]; /* ASG 971220 */
static int next_interrupt;	/* cycle count (relative to start of frame) when next interrupt will happen */
static int running;	/* number of cycles that the CPU emulation was requested to run */
					/* (needed by cpu_getfcount) */
static int have_to_reset;
static int watchdog_counter;

static unsigned char cpucontext[MAX_CPU][CPU_CONTEXT_SIZE];	/* ASG 971105 */
int previouspc;


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



/* ASG 971222 -- warning these must match the defines in driver.h! */
struct cpu_interface cpuintf[] =
{
	/* Dummy CPU -- placeholder for type 0 */
	{
		Dummy_Reset,                       /* Reset CPU */
		Dummy_Execute,                     /* Execute a number of cycles */
		(void *)Dummy_SetRegs,             /* Set the contents of the registers */
		(void *)Dummy_GetRegs,             /* Get the contents of the registers */
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
		Z80_Reset,                         /* Reset CPU */
		Z80_Execute,                       /* Execute a number of cycles */
		(void *)Z80_SetRegs,               /* Set the contents of the registers */
		(void *)Z80_GetRegs,               /* Get the contents of the registers */
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
		(void *)m6502_SetRegs,             /* Set the contents of the registers */
		(void *)m6502_GetRegs,             /* Get the contents of the registers */
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
		(void *)i86_SetRegs,               /* Set the contents of the registers */
		(void *)i86_GetRegs,               /* Get the contents of the registers */
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
		(void *)I8039_SetRegs,             /* Set the contents of the registers */
		(void *)I8039_GetRegs,             /* Get the contents of the registers */
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
		(void *)m6808_SetRegs,             /* Set the contents of the registers */
		(void *)m6808_GetRegs,             /* Get the contents of the registers */
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
	/* #define CPU_M6809  6 */
	{
		m6809_reset,                       /* Reset CPU */
		m6809_execute,                     /* Execute a number of cycles */
		(void *)m6809_SetRegs,             /* Set the contents of the registers */
		(void *)m6809_GetRegs,             /* Get the contents of the registers */
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
	/* #define CPU_M68000 7 */
	{
		MC68000_Reset,                     /* Reset CPU */
		MC68000_Execute,                   /* Execute a number of cycles */
		(void *)MC68000_SetRegs,           /* Set the contents of the registers */
		(void *)MC68000_GetRegs,           /* Get the contents of the registers */
		(void *)MC68000_GetPC,             /* Return the current PC */
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

/* ASG 971222 -- Convenience macros - not in cpuintrf.h because they shouldn't be used by everyone */
#define RESET(index)                    ((*cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].reset)())
#define EXECUTE(index,cycles)           ((*cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].execute)(cycles))
#define SETREGS(index,regs)             ((*cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].set_regs)(regs))
#define GETREGS(index,regs)             ((*cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].get_regs)(regs))
#define GETPC(index)                    ((*cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].get_pc)())
#define CAUSE_INTERRUPT(index,type)     ((*cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].cause_interrupt)(type))
#define CLEAR_PENDING_INTERRUPTS(index) ((*cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].clear_pending_interrupts)())
#define ICOUNT(index)                   (*cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].icount)
#define INT_TYPE_NONE(index)            (cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].no_int)
#define INT_TYPE_IRQ(index)             (cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].irq_int)
#define INT_TYPE_NMI(index)             (cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].nmi_int)

#define SET_OP_BASE(index,pc)           ((*cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK].set_op_base)(pc))


void cpu_init(void)
{
	/* count how many CPUs we have to emulate */
	totalcpu = 0;

	while (totalcpu < MAX_CPU)
	{
		if (Machine->drv->cpu[totalcpu].cpu_type == 0) break;

		totalcpu++;
	}

/* ASG 971007 move for mame.c */
/*	initmemoryhandlers();    */
}



void cpu_run(void)
{
	int usres;

	/* ASG 971220 - determine which CPUs need a context switch */
	for (activecpu = 0;activecpu < totalcpu;activecpu++)
	{
		#ifdef MAME_DEBUG
		save_context[activecpu] = 1;
		#else
		{
			int i;

			save_context[activecpu] = 0;

			for (i = 0; i < totalcpu; i++)
				if (i != activecpu && (Machine->drv->cpu[activecpu].cpu_type & ~CPU_FLAGS_MASK) == (Machine->drv->cpu[i].cpu_type & ~CPU_FLAGS_MASK))
					save_context[activecpu] = 1;
		}
		#endif
	}

reset:
	have_to_reset = 0;
	watchdog_counter = -1;	/* disable watchdog */

	for (activecpu = 0;activecpu < totalcpu;activecpu++)
	{
		/* if sound is disabled, don't emulate the audio CPU */
		if (Machine->sample_rate == 0 && (Machine->drv->cpu[activecpu].cpu_type & CPU_AUDIO_CPU))
			cpurunning[activecpu] = 0;
		else
			cpurunning[activecpu] = 1;

		totalcycles[activecpu] = 0;
	}

	/* do this AFTER the above so init_machine() can use cpu_halt() to hold the */
	/* execution of some CPUs */
	if (Machine->drv->init_machine) (*Machine->drv->init_machine)();

	for (activecpu = 0;activecpu < totalcpu;activecpu++)
	{
		int cycles;


		cycles = (Machine->drv->cpu[activecpu].cpu_clock / Machine->drv->frames_per_second)
				/ Machine->drv->cpu[activecpu].interrupts_per_frame;

		memorycontextswap(activecpu);

		RESET (activecpu);
		if (save_context[activecpu]) GETREGS (activecpu, cpucontext[activecpu]);
	}


	do
	{
		update_input_ports();	/* read keyboard & update the status of the input ports */

		for (activecpu = 0;activecpu < totalcpu;activecpu++)
		{
			ran_this_frame[activecpu] = 0;

			if (cpurunning[activecpu])
				iloops[activecpu] = Machine->drv->cpu[activecpu].interrupts_per_frame - 1;
			else
				iloops[activecpu] = -1;
		}


		for (current_slice = 0;current_slice < Machine->drv->cpu_slices_per_frame;current_slice++)
		{
			for (activecpu = 0;activecpu < totalcpu;activecpu++)
			{
				if (have_to_reset) goto reset;	/* machine_reset() was called, have to reset */

				if (cpurunning[activecpu])
				{
					if (iloops[activecpu] >= 0)
					{
						int ran,target;

						memorycontextswap(activecpu);

						if (save_context[activecpu]) SETREGS (activecpu, cpucontext[activecpu]);

						/* ASG 971223 -- make sure any bank switching is reset */
						SET_OP_BASE (activecpu, GETPC (activecpu));

						target = (Machine->drv->cpu[activecpu].cpu_clock / Machine->drv->frames_per_second)
								* (current_slice + 1) / Machine->drv->cpu_slices_per_frame;

						next_interrupt = (Machine->drv->cpu[activecpu].cpu_clock / Machine->drv->frames_per_second)
								* (Machine->drv->cpu[activecpu].interrupts_per_frame - iloops[activecpu])
								/ Machine->drv->cpu[activecpu].interrupts_per_frame;


						while (ran_this_frame[activecpu] < target)
						{
							if (target <= next_interrupt)
								running = target - ran_this_frame[activecpu];
							else
								running = next_interrupt - ran_this_frame[activecpu];

							ran = EXECUTE (activecpu, running);

							ran_this_frame[activecpu] += ran;
							totalcycles[activecpu] += ran;
							if (ran_this_frame[activecpu] >= next_interrupt)
							{
								cpu_cause_interrupt(activecpu,cpu_interrupt());
								iloops[activecpu]--;

								next_interrupt = (Machine->drv->cpu[activecpu].cpu_clock / Machine->drv->frames_per_second)
										* (Machine->drv->cpu[activecpu].interrupts_per_frame - iloops[activecpu])
										/ Machine->drv->cpu[activecpu].interrupts_per_frame;
							}
						}

						if (save_context[activecpu]) GETREGS (activecpu, cpucontext[activecpu]);

						updatememorybase(activecpu);
					}
				}
			}
		}

		usres = updatescreen();

		if (watchdog_counter >= 0)
		{
			if (--watchdog_counter < 0)
			{
if (errorlog) fprintf(errorlog,"warning: reset caused by the watchdog\n");
				machine_reset();
			}
		}
	} while (usres == 0);
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
	watchdog_counter = 10;
}

int watchdog_reset_r(int offset)
{

	watchdog_counter = 10;
	return 0;
}



/***************************************************************************

  This function resets the machine (the reset will not take place
  immediately, it will be performed at the end of the active CPU's time
  slice)

***************************************************************************/
void machine_reset(void)
{
	extern int hiscoreloaded;


	/* write hi scores to disk */
	if (hiscoreloaded != 0 && Machine->gamedrv->hiscore_save)
		(*Machine->gamedrv->hiscore_save)();
	hiscoreloaded = 0;

	have_to_reset = 1;
}



/***************************************************************************

  Use this function to reset a specified CPU immediately

***************************************************************************/
/* ASG 971105 -- added this function */
void cpu_reset(int cpu)
{
	if (cpu == activecpu)
		RESET (cpu);
	else
	{
		unsigned char context[CPU_CONTEXT_SIZE];

		if (save_context[activecpu]) GETREGS (activecpu, context);
		if (save_context[cpu]) SETREGS (cpu, cpucontext[cpu]);
		memorycontextswap (cpu);
		RESET (cpu);
		memorycontextswap (activecpu);
		if (save_context[cpu]) GETREGS (cpu, cpucontext[cpu]);
		if (save_context[activecpu]) SETREGS (activecpu, context);
	}
}



/***************************************************************************

  Use this function to stop and restart CPUs

***************************************************************************/
void cpu_halt(int cpunum,int running)
{
	if (cpunum >= MAX_CPU) return;

	cpurunning[cpunum] = running;
}



/***************************************************************************

  This function returns CPUNUM current status  (running or halted)

***************************************************************************/
int cpu_getstatus(int cpunum)
{
	if (cpunum >= MAX_CPU) return 0;

	return cpurunning[cpunum];
}



int cpu_getactivecpu(void)
{
	return activecpu;
}

int cpu_gettotalcpu(void)
{
	return totalcpu;
}



int cpu_getpc(void)
{
	return GETPC (activecpu);
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
	switch(Machine->drv->cpu[activecpu].cpu_type & ~CPU_FLAGS_MASK)
	{
		case CPU_Z80:
		case CPU_I8039:
		case CPU_M6502:
			return previouspc;
			break;

		default:
	if (errorlog) fprintf(errorlog,"cpu_getpreviouspc: unsupported CPU type %02x\n",Machine->drv->cpu[activecpu].cpu_type);
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
	switch(Machine->drv->cpu[activecpu].cpu_type & ~CPU_FLAGS_MASK)
	{
		case CPU_Z80:
			{
				Z80_Regs regs;


				Z80_GetRegs(&regs);
				return RAM[regs.SP.D] + (RAM[regs.SP.D+1] << 8);
			}
			break;

		default:
	if (errorlog) fprintf(errorlog,"cpu_getreturnpc: unsupported CPU type %02x\n",Machine->drv->cpu[activecpu].cpu_type);
			return -1;
			break;
	}
}



static int cycles_currently_ran(void)
{
	return running - ICOUNT (activecpu);
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
	return totalcycles[activecpu] + cycles_currently_ran();
}



/***************************************************************************

  Returns the number of CPU cycles before the next interrupt handler call

***************************************************************************/
int cpu_geticount(void)
{
	return next_interrupt - ran_this_frame[activecpu] - cycles_currently_ran();
}



/***************************************************************************

  Returns the number of CPU cycles before the end of the current video frame

***************************************************************************/
int cpu_getfcount(void)
{
	int result = (Machine->drv->cpu[activecpu].cpu_clock / Machine->drv->frames_per_second)
			- ran_this_frame[activecpu] - cycles_currently_ran();
	return (result < 0) ? 0 : result;
}



/***************************************************************************

  Returns the number of CPU cycles in one video frame

***************************************************************************/
int cpu_getfperiod(void)
{
	return Machine->drv->cpu[activecpu].cpu_clock / Machine->drv->frames_per_second;
}



void cpu_seticount(int cycles)
{
	ICOUNT (activecpu) = cycles;
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
	return iloops[activecpu];
}



/***************************************************************************

  Interrupt handling

***************************************************************************/

/***************************************************************************

  Use this function to cause an interrupt immediately (don't have to wait
  until the next call to the interrupt handler)

***************************************************************************/
void cpu_cause_interrupt(int cpu,int type)
{
	if (cpu == activecpu)
		CAUSE_INTERRUPT (cpu, type);
	else
	{
		unsigned char context[CPU_CONTEXT_SIZE];

		if (save_context[activecpu]) GETREGS (activecpu, context);
		if (save_context[cpu]) SETREGS (cpu, cpucontext[cpu]);
		CAUSE_INTERRUPT (cpu, type);
		if (save_context[cpu]) GETREGS (cpu, cpucontext[cpu]);
		if (save_context[activecpu]) SETREGS (activecpu, context);
	}
}



void cpu_clear_pending_interrupts(int cpu)
{
	if (cpu == activecpu)
		CLEAR_PENDING_INTERRUPTS (cpu);
	else
	{
		unsigned char context[CPU_CONTEXT_SIZE];

		if (save_context[activecpu]) GETREGS (activecpu, context);
		if (save_context[cpu]) SETREGS (cpu, cpucontext[cpu]);
		CLEAR_PENDING_INTERRUPTS (cpu);
		if (save_context[cpu]) GETREGS (cpu, cpucontext[cpu]);
		if (save_context[activecpu]) SETREGS (activecpu, context);
	}
}



/* start with interrupts enabled, so the generic routine will work even if */
/* the machine doesn't have an interrupt enable port */
static int interrupt_enable[MAX_CPU] = { 1, 1, 1, 1 };
static int interrupt_vector[MAX_CPU] = { 0xff, 0xff, 0xff, 0xff };

void interrupt_enable_w(int offset,int data)
{
	interrupt_enable[activecpu] = data;

	/* make sure there are no queued interrupts */
	if (data == 0) cpu_clear_pending_interrupts(activecpu);
}



void interrupt_vector_w(int offset,int data)
{
	if (interrupt_vector[activecpu] != data)
	{
		interrupt_vector[activecpu] = data;

		/* make sure there are no queued interrupts */
		cpu_clear_pending_interrupts(activecpu);
	}
}



int interrupt(void)
{
	int val;

	if (interrupt_enable[activecpu] == 0)
		return INT_TYPE_NONE (activecpu);

	val = INT_TYPE_IRQ (activecpu);
	if (val == -1000)
		val = interrupt_vector[activecpu];
	return val;
}



int nmi_interrupt(void)
{
	if (interrupt_enable[activecpu] == 0)
		return INT_TYPE_NONE (activecpu);
	return INT_TYPE_NMI (activecpu);
}



int ignore_interrupt(void)
{
	return INT_TYPE_NONE (activecpu);
}



/***************************************************************************

  Interrupt handler. This function is called at regular intervals
  (determined by IPeriod) by the CPU emulation.

***************************************************************************/

int cpu_interrupt(void)
{
	return (*Machine->drv->cpu[activecpu].interrupt)();
}


#ifdef MAME_DEBUG
/* JB 971019 */
void cpu_getcpucontext (int activecpu, unsigned char *buf)
{
    memcpy (buf, cpucontext[activecpu], CPU_CONTEXT_SIZE);	/* ASG 971105 */
}

void cpu_setcpucontext (int activecpu, const unsigned char *buf)
{
    memcpy (cpucontext[activecpu], buf, CPU_CONTEXT_SIZE);	/* ASG 971105 */
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