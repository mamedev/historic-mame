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


static int activecpu,totalcpu;
static int cpurunning[MAX_CPU];

static const struct MemoryReadAddress *memoryread;
static const struct MemoryWriteAddress *memorywrite;
#define MH_ENTRIES 64
#define MH_SHIFT 10	/* entry = address >> MH_SHIFT */
static int (*memoryreadhandler[MH_ENTRIES])(int address);
static int memoryreadoffset[MH_ENTRIES];
static void (*memorywritehandler[MH_ENTRIES])(int address,int data);
static int memorywriteoffset[MH_ENTRIES];


unsigned char cpucontext[MAX_CPU][100];	/* enough to accomodate the cpu status */


extern struct KEYSet *MM_DirectKEYSet;
extern unsigned char MM_PatchedPort;
extern unsigned char MM_OldPortValue;
extern unsigned char MM_LastChangedValue;

extern unsigned char MM_updir;
extern unsigned char MM_leftdir;
extern unsigned char MM_rightdir;
extern unsigned char MM_downdir;
extern unsigned char MM_orizdir;
extern unsigned char MM_lrdir;
extern unsigned char MM_totale;
extern unsigned char MM_inverted;
extern unsigned char MM_dir4;


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



int mrh_error(int address)
{
	if (errorlog) fprintf(errorlog,"CPU #%d PC %04x: warning - read unmapped memory address %04x\n",activecpu,cpu_getpc(),address);
	return RAM[address];
}
int mrh_ram(int address)
{
	return RAM[address];
}
int mrh_nop(int address)
{
	return 0;
}
int mrh_readmem(int address)
{
	const struct MemoryReadAddress *mra;


	mra = memoryread;
	while (mra->start != -1)
	{
		if (address >= mra->start && address <= mra->end)
		{
			int (*handler)() = mra->handler;


			if (handler == MRA_NOP) return 0;
			else if (handler == MRA_RAM || handler == MRA_ROM) return RAM[address];
			else return (*handler)(address - mra->start);
		}

		mra++;
	}

	if (errorlog) fprintf(errorlog,"CPU #%d PC %04x: warning - read unmapped memory address %04x\n",activecpu,cpu_getpc(),address);
	return RAM[address];
}


void mwh_error(int address,int data)
{
	if (errorlog) fprintf(errorlog,"CPU #%d PC %04x: warning - write %02x to unmapped memory address %04x\n",activecpu,cpu_getpc(),data,address);
	RAM[address] = data;
}
void mwh_ram(int address,int data)
{
	RAM[address] = data;
}
void mwh_nop(int address,int data)
{
}
void mwh_writemem(int address,int data)
{
	const struct MemoryWriteAddress *mwa;


	mwa = memorywrite;
	while (mwa->start != -1)
	{
		if (address >= mwa->start && address <= mwa->end)
		{
			void (*handler)() = mwa->handler;


			if (handler == MWA_NOP) return;
			else if (handler == MWA_RAM) RAM[address] = data;
			else if (handler == MWA_ROM)
			{
				if (errorlog) fprintf(errorlog,"CPU #%d PC %04x: warning - write %02x to ROM address %04x\n",activecpu,cpu_getpc(),data,address);
			}
			else (*handler)(address - mwa->start,data);

			return;
		}

		mwa++;
	}

	if (errorlog) fprintf(errorlog,"CPU #%d PC %04x: warning - write %02x to unmapped memory address %04x\n",activecpu,cpu_getpc(),data,address);
	RAM[address] = data;
}



static void initmemoryhandlers(void)
{
	int i,s,e,a,b;
	const struct MemoryReadAddress *mra;
	const struct MemoryWriteAddress *mwa;


	memoryread = Machine->drv->cpu[activecpu].memory_read;
	memorywrite = Machine->drv->cpu[activecpu].memory_write;

	for (i = 0;i < MH_ENTRIES;i++)
	{
		memoryreadhandler[i] = mrh_error;
		memoryreadoffset[i] = 0;

		memorywritehandler[i] = mwh_error;
		memorywriteoffset[i] = 0;
	}

	mra = memoryread;
	while (mra->start != -1) mra++;
	mra--;

	/* go backwards because entries up in the memory array have greater priority than */
	/* the following ones. If an entry is duplicated, going backwards we overwrite */
	/* the handler set by the lower priority one. */
	while (mra >= memoryread)
	{
		s = mra->start >> MH_SHIFT;
		a = mra->start ? ((mra->start-1) >> MH_SHIFT) + 1 : 0;
		b = ((mra->end+1) >> MH_SHIFT) - 1;
		e = mra->end >> MH_SHIFT;

		/* first of all make all the entries point to the general purpose handler... */
		for (i = s;i <= e;i++)
		{
			memoryreadhandler[i] = mrh_readmem;
			memoryreadoffset[i] = 0;
		}
		/* ... and now make black which contain only one handler point directly to the handler */
		for (i = a;i <= b;i++)
		{
			int (*handler)() = mra->handler;


			if (handler == MRA_NOP)
			{
				memoryreadhandler[i] = mrh_nop;
				memoryreadoffset[i] = 0;
			}
			else if (handler == MRA_RAM || handler == MRA_ROM)
			{
				memoryreadhandler[i] = mrh_ram;
				memoryreadoffset[i] = 0;
			}
			else
			{
				memoryreadhandler[i] = mra->handler;
				memoryreadoffset[i] = mra->start;
			}
		}

		mra--;
	}


	mwa = memorywrite;
	while (mwa->start != -1) mwa++;
	mwa--;

	/* go backwards because entries up in the memory array have greater priority than */
	/* the following ones. If an entry is duplicated, going backwards we overwrite */
	/* the handler set by the lower priority one. */
	while (mwa >= memorywrite)
	{
		s = mwa->start >> MH_SHIFT;
		a = mwa->start ? ((mwa->start-1) >> MH_SHIFT) + 1 : 0;
		b = ((mwa->end+1) >> MH_SHIFT) - 1;
		e = mwa->end >> MH_SHIFT;

		/* first of all make all the entries point to the general purpose handler... */
		for (i = s;i <= e;i++)
		{
			memorywritehandler[i] = mwh_writemem;
			memorywriteoffset[i] = 0;
		}
		/* ... and now make black which contain only one handler point directly to the handler */
		for (i = a;i <= b;i++)
		{
			void (*handler)() = mwa->handler;


			if (handler == MWA_NOP)
			{
				memorywritehandler[i] = mwh_nop;
				memorywriteoffset[i] = 0;
			}
			else if (handler == MWA_RAM)
			{
				memorywritehandler[i] = mwh_ram;
				memorywriteoffset[i] = 0;
			}
			else if (handler != MWA_ROM)
			{
				memorywritehandler[i] = mwa->handler;
				memorywriteoffset[i] = mwa->start;
			}
		}

		mwa--;
	}
}


void cpu_init(void)
{
	/* count how many CPUs we have to emulate */
	totalcpu = 0;
	while (totalcpu < MAX_CPU)
	{
		const struct MemoryReadAddress *mra;
		const struct MemoryWriteAddress *mwa;


		if (Machine->drv->cpu[totalcpu].cpu_type == 0) break;

		/* initialize the memory base pointers for memory hooks */
		mra = Machine->drv->cpu[totalcpu].memory_read;
		while (mra->start != -1)
		{
			if (mra->base) *mra->base =
					&Machine->memory_region[Machine->drv->cpu[totalcpu].memory_region][mra->start];
			if (mra->size) *mra->size = mra->end - mra->start + 1;
			mra++;
		}
		mwa = Machine->drv->cpu[totalcpu].memory_write;
		while (mwa->start != -1)
		{
			if (mwa->base) *mwa->base =
					&Machine->memory_region[Machine->drv->cpu[totalcpu].memory_region][mwa->start];
			if (mwa->size) *mwa->size = mwa->end - mwa->start + 1;
			mwa++;
		}


		totalcpu++;
	}
}



void cpu_run(void)
{
	int totalcpu,usres;
	unsigned char *ROM0;	/* opcode decryption is currently supported only for the first memory region */


	ROM0 = ROM;

	/* count how many CPUs we have to emulate */
	totalcpu = 0;
	while (totalcpu < MAX_CPU)
	{
		const struct MemoryReadAddress *mra;
		const struct MemoryWriteAddress *mwa;


		if (Machine->drv->cpu[totalcpu].cpu_type == 0) break;

		/* if sound is disabled, don't emulate the audio CPU */
		if (play_sound == 0 && (Machine->drv->cpu[totalcpu].cpu_type & CPU_AUDIO_CPU))
			cpurunning[totalcpu] = 0;
		else
			cpurunning[totalcpu] = 1;


		/* initialize the memory base pointers for memory hooks */
		RAM = Machine->memory_region[Machine->drv->cpu[totalcpu].memory_region];
		mra = Machine->drv->cpu[totalcpu].memory_read;
		while (mra->start != -1)
		{
			if (mra->base) *mra->base = &RAM[mra->start];
			mra++;
		}
		mwa = Machine->drv->cpu[totalcpu].memory_write;
		while (mwa->start != -1)
		{
			if (mwa->base) *mwa->base = &RAM[mwa->start];
			mwa++;
		}


		totalcpu++;
	}

reset:
	for (activecpu = 0;activecpu < totalcpu;activecpu++)
	{
		int cycles;


		cycles = Machine->drv->cpu[activecpu].cpu_clock /
				(Machine->drv->frames_per_second * Machine->drv->cpu[activecpu].interrupts_per_frame);

		initmemoryhandlers();
		RAM = Machine->memory_region[Machine->drv->cpu[activecpu].memory_region];

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
					ctxt->IPeriod = cycles;	/* must be done before Reset6502() */
					Reset6502(ctxt);
				}
				break;
			case CPU_I86:
				I86_Reset(RAM,cycles);
				break;
			/* DS... */
			case CPU_M6809:
				{
					struct m6809context *ctxt;

					ctxt = (struct m6809context *)cpucontext[activecpu];
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


				initmemoryhandlers();
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

  This function return CPUNUM current status  (running or halted)

***************************************************************************/
int cpu_getstatus(int cpunum)
{
	if (cpunum >= MAX_CPU) return 0;

        return cpurunning[cpunum];
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
	int temp_res, res, i;
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

        if (MM_dir4 && (port == MM_PatchedPort))
        {
            if (res != MM_LastChangedValue)
            {
                MM_LastChangedValue = res;
                if (MM_inverted)
                {
                    res = ~res;
                    MM_OldPortValue = ~MM_OldPortValue;
                };
                temp_res = res & MM_totale;

                if (MM_downdir & temp_res)                  /* DOWN control       */
                {
                   if ((MM_lrdir & temp_res) >= MM_orizdir) /* Left & Right?      */
                   {
                      if (MM_downdir & MM_OldPortValue)     /* changed direction? */
                         res=(~MM_downdir) & res;           /* DOWN off           */
                      else
                         res=(~MM_lrdir) & res;             /* Left & Right off   */
                   }
                }
                if (MM_updir & temp_res)                    /* UP control         */
                {
                   if ((MM_lrdir & temp_res) >= MM_orizdir)
                   {
                      if (MM_updir & MM_OldPortValue)
                         res=(~MM_updir) & res;
                      else
                         res=(~MM_lrdir) & res;
                   }
                }
                if (MM_inverted) res = ~res;

                MM_OldPortValue = res; //Valore a valle delle modifiche
            }
            else  res = MM_OldPortValue;
        };


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


int readtrakport(int port) {
  int axis;
  int read;
  struct TrakPort *in;

  in = &Machine->gamedrv->trak_ports[port];
  axis = in->axis;

  read = osd_trak_read(axis);

  if(read == NO_TRAK) {
    return(NO_TRAK);
  }

  if(in->centered) {
    switch(axis) {
    case X_AXIS:
      osd_trak_center_x();
      break;
    case Y_AXIS:
      osd_trak_center_y();
      break;
    }
  }

  if(in->conversion) {
    return((*in->conversion)(read*in->scale));
  }

  return(read*in->scale);
}

int input_trak_0_r(int offset) {
  return(readtrakport(0));
}

int input_trak_1_r(int offset) {
  return(readtrakport(1));
}

int input_trak_2_r(int offset) {
  return(readtrakport(2));
}

int input_trak_3_r(int offset) {
  return(readtrakport(3));
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
int cpu_readmem(int address)
{
	int i = address >> MH_SHIFT;

	return (memoryreadhandler[i])(address - memoryreadoffset[i]);
}



/***************************************************************************

  Perform a memory write. This function is called by the CPU emulation.

***************************************************************************/
void cpu_writemem(int address,int data)
{
	int i = address >> MH_SHIFT;

	(memorywritehandler[i])(address - memorywriteoffset[i],data);
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
