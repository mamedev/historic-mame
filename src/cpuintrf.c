/***************************************************************************

  cpuintrf.c

  Don't you love MS-DOS 8+3 names? That stands for CPU interface.
  Functions needed to interface the CPU emulator with the other parts of
  the emulation.

***************************************************************************/

#include "driver.h"
#include "Z80.h"
#include "M6502.h"
#include "M6809.h"
#include "M6808.h"
#include "M68000.h"

void I86_Execute();
void I86_Reset(unsigned char *mem,int cycles);

static int activecpu,totalcpu;
static int iloops[MAX_CPU];
static int cpurunning[MAX_CPU];
static int totalcycles[MAX_CPU];
static int current_slice;
static int ran_this_frame[MAX_CPU];
static int next_interrupt;	/* cycle count (relative to start of frame) when next interrupt will happen */
static int running;	/* number of cycles that the CPU emulation was requested to run */
					/* (needed by cpu_getfcount) */
static int have_to_reset;

static unsigned char cpucontext[MAX_CPU][100];	/* enough to accomodate the cpu status */



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


reset:
	have_to_reset = 0;

	for (activecpu = 0;activecpu < totalcpu;activecpu++)
	{
		/* if sound is disabled, don't emulate the audio CPU */
		if (play_sound == 0 && (Machine->drv->cpu[activecpu].cpu_type & CPU_AUDIO_CPU))
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

		switch(Machine->drv->cpu[activecpu].cpu_type & ~CPU_FLAGS_MASK)
		{
			case CPU_Z80:
				Z80_Reset();
				Z80_GetRegs((Z80_Regs *)cpucontext[activecpu]);
				break;
			case CPU_M6502:
				Reset6502((M6502 *)cpucontext[activecpu]);
				break;
			case CPU_I86:
				I86_Reset(RAM,cycles);
				break;
                        /* MB */
			case CPU_M6808:
				m6808_reset();
				m6808_GetRegs((m6808_Regs *)cpucontext[activecpu]);
				break;
			/* DS... */
			case CPU_M6809:
				m6809_reset();
				m6809_GetRegs((m6809_Regs *)cpucontext[activecpu]);
				break;
			/* ...DS */
			case CPU_M68000:
				MC68000_reset(cycles);
				break;
		}
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
					memorycontextswap(activecpu);

					switch(Machine->drv->cpu[activecpu].cpu_type & ~CPU_FLAGS_MASK)
					{
						case CPU_Z80:
							Z80_SetRegs((Z80_Regs *)cpucontext[activecpu]);

							if (iloops[activecpu] >= 0)
							{
								int ran,target;


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

									ran = Z80_Execute(running);

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
							}

							Z80_GetRegs((Z80_Regs *)cpucontext[activecpu]);
							break;

						case CPU_M6502:
							if (iloops[activecpu] >= 0)
							{
								int ran,target;


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

									ran = Run6502((M6502 *)cpucontext[activecpu],running);

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
							}
							break;

						case CPU_I86:
								if (iloops[activecpu] >= 0)
								{
									running = Machine->drv->cpu[activecpu].cpu_clock / Machine->drv->frames_per_second;
									I86_Execute();
									totalcycles[activecpu] += running;
									iloops[activecpu]--;
								}
							break;

						case CPU_M6808:
							m6808_SetRegs((m6808_Regs *)cpucontext[activecpu]);

							if (iloops[activecpu] >= 0)
							{
								int ran,target;


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

									ran = m6808_execute(running);

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
							}

							m6808_GetRegs((m6808_Regs *)cpucontext[activecpu]);
							break;

						case CPU_M6809:
							m6809_SetRegs((m6809_Regs *)cpucontext[activecpu]);

							if (iloops[activecpu] >= 0)
							{
								int ran,target;


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

									ran = m6809_execute(running);

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
							}

							m6809_GetRegs((m6809_Regs *)cpucontext[activecpu]);
							break;

						case CPU_M68000:
							{
								if (iloops[activecpu] >= 0)
								{
									running = Machine->drv->cpu[activecpu].cpu_clock / Machine->drv->frames_per_second;
									MC68000_Execute();
									totalcycles[activecpu] += running;
									iloops[activecpu]--;
								}
							}
							break;
					}

					updatememorybase(activecpu);
				}
			}
		}

		usres = updatescreen();
	} while (usres == 0);
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
	switch(Machine->drv->cpu[activecpu].cpu_type & ~CPU_FLAGS_MASK)
	{
		case CPU_Z80:
			return Z80_GetPC();
			break;

		case CPU_M6502:
			return ((M6502 *)cpucontext[activecpu])->PC.W;
			break;

                /* MB */
		case CPU_M6808:
			return m6808_GetPC();
			break;

		/* DS... */
		case CPU_M6809:
			return m6809_GetPC();
			break;
		/* ...DS */

		default:
	if (errorlog) fprintf(errorlog,"cpu_getpc: unsupported CPU type %02x\n",Machine->drv->cpu[activecpu].cpu_type);
			return -1;
			break;
	}
}


/***************************************************************************

  This is similar to cpu_getpc(), but instead of returning the current PC,
  it returns the address of the opcode that is doing the read/write. The PC
  has already been incremented by some unknown amount by the time the actual
  read or write is being executed. This helps to figure out what opcode is
  actually doing the reading or writing, and therefore the amount of cycles
  it's taking. The Missile Command driver needs to know this.

***************************************************************************/
int cpu_getpreviouspc(void)  /* -RAY- */
{
	switch(Machine->drv->cpu[activecpu].cpu_type & ~CPU_FLAGS_MASK)
	{
		case CPU_M6502:
			return ((M6502 *)cpucontext[activecpu])->previousPC.W;
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
	switch(Machine->drv->cpu[activecpu].cpu_type & ~CPU_FLAGS_MASK)
	{
		case CPU_Z80:
			return running - Z80_ICount;
			break;

		case CPU_M6502:
			return running - ((M6502 *)cpucontext[activecpu])->ICount;
			break;

		/* MB */
		case CPU_M6808:
			return running - m6808_ICount;
			break;

		/* DS... */
		case CPU_M6809:
			return running - m6809_ICount;
			break;
		/* ...DS */

		default:
	if (errorlog) fprintf(errorlog,"cycles_currently_ran: unsupported CPU type %02x\n",Machine->drv->cpu[activecpu].cpu_type);
			return 0;
			break;
	}
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
	return (Machine->drv->cpu[activecpu].cpu_clock / Machine->drv->frames_per_second)
			- ran_this_frame[activecpu] - cycles_currently_ran();
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
	switch(Machine->drv->cpu[activecpu].cpu_type & ~CPU_FLAGS_MASK)
	{
		case CPU_Z80:
			Z80_ICount = cycles;
			break;

		case CPU_M6502:
			((M6502 *)cpucontext[activecpu])->ICount = cycles;
			break;

                /* MB */
		case CPU_M6808:
			m6808_ICount = cycles;
			break;

		/* DS... */
		case CPU_M6809:
			m6809_ICount = cycles;
			break;
		/* ...DS */

		default:
	if (errorlog) fprintf(errorlog,"cpu_seticycles: unsupported CPU type %02x\n",Machine->drv->cpu[activecpu].cpu_type);
			break;
	}
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
	switch(Machine->drv->cpu[cpu].cpu_type & ~CPU_FLAGS_MASK)
	{
		case CPU_Z80:
			if (cpu == activecpu)
				Z80_Cause_Interrupt(type);
			else
			{
				Z80_Regs regs;


				Z80_GetRegs(&regs);
				Z80_SetRegs((Z80_Regs *)cpucontext[cpu]);
				Z80_Cause_Interrupt(type);
				Z80_GetRegs((Z80_Regs *)cpucontext[cpu]);
				Z80_SetRegs(&regs);
			}
			break;

		case CPU_M6502:
			M6502_Cause_Interrupt((M6502 *)cpucontext[cpu],type);
			break;

                /* MB */
		case CPU_M6808:
			if (cpu == activecpu)
				m6808_Cause_Interrupt(type);
			else
			{
				m6808_Regs regs;


				m6808_GetRegs(&regs);
				m6808_SetRegs((m6808_Regs *)cpucontext[cpu]);
				m6808_Cause_Interrupt(type);
				m6808_GetRegs((m6808_Regs *)cpucontext[cpu]);
				m6808_SetRegs(&regs);
			}
			break;

		case CPU_M6809:
			if (cpu == activecpu)
				m6809_Cause_Interrupt(type);
			else
			{
				m6809_Regs regs;


				m6809_GetRegs(&regs);
				m6809_SetRegs((m6809_Regs *)cpucontext[cpu]);
				m6809_Cause_Interrupt(type);
				m6809_GetRegs((m6809_Regs *)cpucontext[cpu]);
				m6809_SetRegs(&regs);
			}
			break;

		default:
if (errorlog) fprintf(errorlog,"cpu_cause_interrupt: unsupported CPU type %02x\n",Machine->drv->cpu[activecpu].cpu_type);
			break;
	}
}



void cpu_clear_pending_interrupts(int cpu)
{
	switch(Machine->drv->cpu[activecpu].cpu_type & ~CPU_FLAGS_MASK)
	{
		case CPU_Z80:
			if (cpu == activecpu)
				Z80_Clear_Pending_Interrupts();
			else
			{
				Z80_Regs regs;


				Z80_GetRegs(&regs);
				Z80_SetRegs((Z80_Regs *)cpucontext[cpu]);
				Z80_Clear_Pending_Interrupts();
				Z80_GetRegs((Z80_Regs *)cpucontext[cpu]);
				Z80_SetRegs(&regs);
			}
			break;

		case CPU_M6502:
			M6502_Clear_Pending_Interrupts((M6502 *)cpucontext[cpu]);
			break;

                /* MB */
		case CPU_M6808:
			if (cpu == activecpu)
				m6808_Clear_Pending_Interrupts();
			else
			{
				m6808_Regs regs;


				m6808_GetRegs(&regs);
				m6808_SetRegs((m6808_Regs *)cpucontext[cpu]);
				m6808_Clear_Pending_Interrupts();
				m6808_GetRegs((m6808_Regs *)cpucontext[cpu]);
				m6808_SetRegs(&regs);
			}
			break;

		case CPU_M6809:
			if (cpu == activecpu)
				m6809_Clear_Pending_Interrupts();
			else
			{
				m6809_Regs regs;


				m6809_GetRegs(&regs);
				m6809_SetRegs((m6809_Regs *)cpucontext[cpu]);
				m6809_Clear_Pending_Interrupts();
				m6809_GetRegs((m6809_Regs *)cpucontext[cpu]);
				m6809_SetRegs(&regs);
			}
			break;

		default:
if (errorlog) fprintf(errorlog,"clear_pending_interrupts: unsupported CPU type %02x\n",Machine->drv->cpu[activecpu].cpu_type);
			break;
	}
}



/* start with interrupts enabled, so the generic routine will work even if */
/* the machine doesn't have an interrupt enable port */
static int interrupt_enable = 1;
static int interrupt_vector = 0xff;

void interrupt_enable_w(int offset,int data)
{
	interrupt_enable = data;

	/* make sure there are no queued interrupts */
	if (data == 0) cpu_clear_pending_interrupts(activecpu);
}



void interrupt_vector_w(int offset,int data)
{
	if (interrupt_vector != data)
	{
		interrupt_vector = data;

		/* make sure there are no queued interrupts */
		cpu_clear_pending_interrupts(activecpu);
	}
}



int interrupt(void)
{
	switch(Machine->drv->cpu[activecpu].cpu_type & ~CPU_FLAGS_MASK)
	{
		case CPU_Z80:
			if (interrupt_enable == 0) return Z80_IGNORE_INT;
			else return interrupt_vector;
			break;

		case CPU_M6502:
			if (interrupt_enable == 0) return INT_NONE;
			else return INT_IRQ;
			break;

		case CPU_M6808:
			if (interrupt_enable == 0) return M6808_INT_NONE;
			else return M6808_INT_IRQ;
			break;

		case CPU_M6809:
			if (interrupt_enable == 0) return M6809_INT_NONE;
			else return M6809_INT_IRQ;
			break;

		default:
if (errorlog) fprintf(errorlog,"interrupt: unsupported CPU type %02x\n",Machine->drv->cpu[activecpu].cpu_type);
			return -1;
			break;
	}
}



int nmi_interrupt(void)
{
	switch(Machine->drv->cpu[activecpu].cpu_type & ~CPU_FLAGS_MASK)
	{
		case CPU_Z80:
			if (interrupt_enable == 0) return Z80_IGNORE_INT;
			else return Z80_NMI_INT;
			break;

		case CPU_M6502:
			if (interrupt_enable == 0) return INT_NONE;
			else return INT_NMI;
			break;

		case CPU_M6808:
			if (interrupt_enable == 0) return M6808_INT_NONE;
			else return M6808_INT_NMI;
			break;

		case CPU_M6809:
			if (interrupt_enable == 0) return M6809_INT_NONE;
			else return M6809_INT_NMI;
			break;

		default:
if (errorlog) fprintf(errorlog,"nmi_interrupt: unsupported CPU type %02x\n",Machine->drv->cpu[activecpu].cpu_type);
			return -1;
			break;
	}
}



int ignore_interrupt(void)
{
	switch(Machine->drv->cpu[activecpu].cpu_type & ~CPU_FLAGS_MASK)
	{
		case CPU_Z80:
			return Z80_IGNORE_INT;
			break;

		case CPU_M6502:
			return INT_NONE;
			break;

                /* MB */
		case CPU_M6808:
			return M6808_INT_NONE;
			break;

		/* DS... */
		case CPU_M6809:
			return M6809_INT_NONE;
			break;
		/* ...DS */

		default:
if (errorlog) fprintf(errorlog,"interrupt: unsupported CPU type %02x\n",Machine->drv->cpu[activecpu].cpu_type);
			return -1;
			break;
	}
}



/***************************************************************************

  Interrupt handler. This function is called at regular intervals
  (determined by IPeriod) by the CPU emulation.

***************************************************************************/

int cpu_interrupt(void)
{
	return (*Machine->drv->cpu[activecpu].interrupt)();
}
