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

extern void I86_Execute();
extern void I86_Reset(unsigned char *mem,int cycles);


static int activecpu;
static int cpurunning[MAX_CPU];

static const struct MemoryReadAddress *memoryread;
static const struct MemoryWriteAddress *memorywrite;


unsigned char cpucontext[MAX_CPU][100];	/* enough to accomodate the cpu status */


struct z80context
{
	Z80_Regs regs;
	int icount;
	int iperiod;
	int irq;
};

/* DS...*/
struct m6809context
{
	m6809_Regs	regs;
	int	icount;
	int iperiod;
	int	irq;
};
/* ...DS */



void cpu_run(void)
{
	int totalcpu,usres;
	unsigned char *ROM0;	/* opcode decryption is currently supported only for the first memory region */


	ROM0 = ROM;

	/* count how many CPUs we have to emulate */
	totalcpu = 0;
	while (totalcpu < MAX_CPU)
	{
		if (Machine->drv->cpu[totalcpu].cpu_type == 0) break;

		/* if sound is disabled, don't emulate the audio CPU */
		if (play_sound == 0 && (Machine->drv->cpu[totalcpu].cpu_type & CPU_AUDIO_CPU))
			cpurunning[totalcpu] = 0;
		else
			cpurunning[totalcpu] = 1;

		totalcpu++;
	}

reset:
	for (activecpu = 0;activecpu < totalcpu;activecpu++)
	{
		int cycles;


		cycles = Machine->drv->cpu[activecpu].cpu_clock /
				(Machine->drv->frames_per_second * Machine->drv->cpu[activecpu].interrupts_per_frame);

		switch(Machine->drv->cpu[activecpu].cpu_type & ~CPU_FLAGS_MASK)
		{
			case CPU_Z80:
				{
					struct z80context *ctxt;


					ctxt = (struct z80context *)cpucontext[activecpu];
					Z80_Reset();
					Z80_GetRegs(&ctxt->regs);
					ctxt->icount = cycles;
					ctxt->iperiod = cycles;
					ctxt->irq = Z80_IGNORE_INT;
				}
				break;
			case CPU_M6502:
				{
					M6502 *ctxt;


					ctxt = (M6502 *)cpucontext[activecpu];
					/* Reset6502() needs to read memory to get the PC start address */
					RAM = Machine->memory_region[Machine->drv->cpu[activecpu].memory_region];
					memoryread = Machine->drv->cpu[activecpu].memory_read;
					ctxt->IPeriod = cycles;	/* must be done before Reset6502() */
					Reset6502(ctxt);
				}
				break;
			case CPU_I86:
				RAM = Machine->memory_region[Machine->drv->cpu[activecpu].memory_region];
				I86_Reset(RAM,cycles);
				break;
			/* DS... */
			case CPU_M6809:
				{
					struct m6809context *ctxt;

					ctxt = (struct m6809context *)cpucontext[activecpu];
					/* m6809_reset() needs to read memory to get the PC start address */
					RAM = Machine->memory_region[Machine->drv->cpu[activecpu].memory_region];
					memoryread = Machine->drv->cpu[activecpu].memory_read;
					m6809_IPeriod = cycles;
					m6809_reset();
					m6809_GetRegs(&ctxt->regs);
					ctxt->icount = cycles;
					ctxt->iperiod = cycles;
					ctxt->irq = INT_NONE;
				}
				break;
			/* ...DS */
		}
	}


	do
	{
		for (activecpu = 0;activecpu < totalcpu;activecpu++)
		{
			if (cpurunning[activecpu])
			{
				int loops;


				memoryread = Machine->drv->cpu[activecpu].memory_read;
				memorywrite = Machine->drv->cpu[activecpu].memory_write;

				RAM = Machine->memory_region[Machine->drv->cpu[activecpu].memory_region];
				/* opcode decryption is currently supported only for the first memory region */
				if (activecpu == 0) ROM = ROM0;
				else ROM = RAM;


				switch(Machine->drv->cpu[activecpu].cpu_type & ~CPU_FLAGS_MASK)
				{
					case CPU_Z80:
						{
							struct z80context *ctxt;


							ctxt = (struct z80context *)cpucontext[activecpu];

							Z80_SetRegs(&ctxt->regs);
							Z80_ICount = ctxt->icount;
							Z80_IPeriod = ctxt->iperiod;
							Z80_IRQ = ctxt->irq;

							for (loops = 0;loops < Machine->drv->cpu[activecpu].interrupts_per_frame;loops++)
								Z80_Execute();

							Z80_GetRegs(&ctxt->regs);
							ctxt->icount = Z80_ICount;
							ctxt->iperiod = Z80_IPeriod;
							ctxt->irq = Z80_IRQ;
						}
						break;

					case CPU_M6502:
						for (loops = 0;loops < Machine->drv->cpu[activecpu].interrupts_per_frame;loops++)
							Run6502((M6502 *)cpucontext[activecpu]);
						break;

					case CPU_I86:
						for (loops = 0;loops < Machine->drv->cpu[activecpu].interrupts_per_frame;loops++)
							I86_Execute();
						break;
					/* DS... */
					case CPU_M6809:
						{
							struct m6809context *ctxt;

							ctxt = (struct m6809context *)cpucontext[activecpu];

							m6809_SetRegs(&ctxt->regs);
							m6809_ICount = ctxt->icount;
							m6809_IPeriod = ctxt->iperiod;
							m6809_IRequest = ctxt->irq;

							for (loops = 0;loops < Machine->drv->cpu[activecpu].interrupts_per_frame;loops++)
								m6809_execute();

							m6809_GetRegs(&ctxt->regs);
							ctxt->icount = m6809_ICount;
							ctxt->iperiod = m6809_IPeriod;
							ctxt->irq = m6809_IRequest;
						}
						break;
					/* ...DS */
				}
			}
		}

		usres = updatescreen();
		if (usres == 2)	/* user asked to reset the machine */
			goto reset;
	} while (usres == 0);
}



/***************************************************************************

  Use this function to stop and restart CPUs

***************************************************************************/
void cpu_halt(int cpunum,int running)
{
	if (cpunum >= MAX_CPU) return;

	cpurunning[cpunum] = running;
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



int cpu_geticount(void)
{
	switch(Machine->drv->cpu[activecpu].cpu_type & ~CPU_FLAGS_MASK)
	{
		case CPU_Z80:
			return Z80_ICount;
			break;

		case CPU_M6502:
			return ((M6502 *)cpucontext[activecpu])->ICount;
			break;

		/* DS... */
		case CPU_M6809:
			return m6809_ICount;
			break;
		/* ...DS */

		default:
	if (errorlog) fprintf(errorlog,"cpu_geticycles: unsupported CPU type %02x\n",Machine->drv->cpu[activecpu].cpu_type);
			return -1;
			break;
	}
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

  Some functions commonly used by drivers

***************************************************************************/
int readinputport(int port)
{
	int res,i;
	struct InputPort *in;


	in = &Machine->gamedrv->input_ports[port];

	res = in->default_value;

	for (i = 7;i >= 0;i--)
	{
		int c;


		c = in->keyboard[i];
		if (c && osd_key_pressed(c))
			res ^= (1 << i);
		else
		{
			c = in->joystick[i];
			if (c && osd_joy_pressed(c))
				res ^= (1 << i);
		}
	}

	return res;
}



int input_port_0_r(int offset)
{
	return readinputport(0);
}



int input_port_1_r(int offset)
{
	return readinputport(1);
}



int input_port_2_r(int offset)
{
	return readinputport(2);
}



int input_port_3_r(int offset)
{
	return readinputport(3);
}



int input_port_4_r(int offset)
{
	return readinputport(4);
}



int input_port_5_r(int offset)
{
	return readinputport(5);
}


int input_port_6_r(int offset)
{
	return readinputport(6);
}


int input_port_7_r(int offset)
{
	return readinputport(7);
}


/***************************************************************************

  Interrupt handling

***************************************************************************/

int Z80_IRQ;	/* needed by the CPU emulation */


/* start with interrupts enabled, so the generic routine will work even if */
/* the machine doesn't have an interrupt enable port */
static int interrupt_enable = 1;
static int interrupt_vector = 0xff;

void interrupt_enable_w(int offset,int data)
{
	interrupt_enable = data;

	if (data == 0) Z80_IRQ = Z80_IGNORE_INT;	/* make sure there are no queued interrupts */
}



void interrupt_vector_w(int offset,int data)
{
	interrupt_vector = data;

	Z80_IRQ = Z80_IGNORE_INT;	/* make sure there are no queued interrupts */
}



/* If the game you are emulating doesn't have vertical blank interrupts */
/* (like Lady Bug) you'll have to provide your own interrupt function (set */
/* a flag there, and return the appropriate value from the appropriate input */
/* port when the game polls it) */
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

		/* DS... */
		case CPU_M6809:
			if (interrupt_enable == 0) return INT_NONE;
			else return INT_IRQ;
			break;
		/* ...DS */

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

		default:
	if (errorlog) fprintf(errorlog,"nmi_interrupt: unsupported CPU type %02x\n",Machine->drv->cpu[activecpu].cpu_type);
			return -1;
			break;
	}
}



/***************************************************************************

  Perform a memory read. This function is called by the CPU emulation.

***************************************************************************/
int cpu_readmem(register int A)
{
	const struct MemoryReadAddress *mra;


	mra = memoryread;
	while (mra->start != -1)
	{
		if (A >= mra->start && A <= mra->end)
		{
			int (*handler)() = mra->handler;


			if (handler == MRA_NOP) return 0;
			else if (handler == MRA_RAM || handler == MRA_ROM) return RAM[A];
			else return (*handler)(A - mra->start);
		}

		mra++;
	}

	if (errorlog) fprintf(errorlog,"CPU #%d PC %04x: warning - read unmapped memory address %04x\n",activecpu,cpu_getpc(),A);
	return RAM[A];
}



/***************************************************************************

  Perform a memory write. This function is called by the CPU emulation.

***************************************************************************/
void cpu_writemem(register int A,register unsigned char V)
{
	const struct MemoryWriteAddress *mwa;


	mwa = memorywrite;
	while (mwa->start != -1)
	{
		if (A >= mwa->start && A <= mwa->end)
		{
			void (*handler)() = mwa->handler;


			if (handler == MWA_NOP) return;
			else if (handler == MWA_RAM) RAM[A] = V;
			else if (handler == MWA_ROM)
			{
				if (errorlog) fprintf(errorlog,"CPU #%d PC %04x: warning - write %02x to ROM address %04x\n",activecpu,cpu_getpc(),V,A);
			}
			else (*handler)(A - mwa->start,V);

			return;
		}

		mwa++;
	}

	if (errorlog) fprintf(errorlog,"CPU #%d PC %04x: warning - write %02x to unmapped memory address %04x\n",activecpu,cpu_getpc(),V,A);
	RAM[A] = V;
}



/***************************************************************************

  Perform an I/O port read. This function is called by the CPU emulation.

***************************************************************************/
int cpu_readport(int Port)
{
	const struct IOReadPort *iorp;


	iorp = Machine->drv->cpu[activecpu].port_read;
	if (iorp)
	{
		while (iorp->start != -1)
		{
			if (Port >= iorp->start && Port <= iorp->end)
			{
				int (*handler)() = iorp->handler;


				if (handler == IORP_NOP) return 0;
				else return (*handler)(Port - iorp->start);
			}

			iorp++;
		}
	}

	if (errorlog) fprintf(errorlog,"CPU #%d PC %04x: warning - read unmapped I/O port %02x\n",activecpu,cpu_getpc(),Port);
	return 0;
}



/***************************************************************************

  Perform an I/O port write. This function is called by the CPU emulation.

***************************************************************************/
void cpu_writeport(int Port,int Value)
{
	const struct IOWritePort *iowp;


	iowp = Machine->drv->cpu[activecpu].port_write;
	if (iowp)
	{
		while (iowp->start != -1)
		{
			if (Port >= iowp->start && Port <= iowp->end)
			{
				void (*handler)() = iowp->handler;


				if (handler == IOWP_NOP) return;
				else (*handler)(Port - iowp->start,Value);

				return;
			}

			iowp++;
		}
	}

	if (errorlog) fprintf(errorlog,"CPU #%d PC %04x: warning - write %02x to unmapped I/O port %02x\n",activecpu,cpu_getpc(),Value,Port);
}



/***************************************************************************

  Interrupt handler. This function is called at regular intervals
  (determined by IPeriod) by the CPU emulation.

***************************************************************************/

int cpu_interrupt(void)
{
	return (*Machine->drv->cpu[activecpu].interrupt)();
}



void Z80_Patch (Z80_Regs *Regs)
{
}



void Z80_Reti (void)
{
}



void Z80_Retn (void)
{
}
