/***************************************************************************

	cpuexec.h

	Core multi-CPU execution engine.

***************************************************************************/

#ifndef CPUEXEC_H
#define CPUEXEC_H

#include "osd_cpu.h"
#include "memory.h"
#include "timer.h"

#ifdef __cplusplus
extern "C" {
#endif


/*************************************
 *
 *	CPU description for drivers
 *
 *************************************/

struct MachineCPU
{
	int			cpu_type;					/* see #defines below. */
	int			cpu_clock;					/* in Hertz */
	const void *memory_read;				/* struct Memory_ReadAddress */
	const void *memory_write;				/* struct Memory_WriteAddress */
	const void *port_read;
	const void *port_write;
	int 		(*vblank_interrupt)(void);	/* for interrupts tied to VBLANK */
	int 		vblank_interrupts_per_frame;/* usually 1 */
	int 		(*timed_interrupt)(void);	/* for interrupts not tied to VBLANK */
	int 		timed_interrupts_per_second;
	void *		reset_param;				/* parameter for cpu_reset */
};



/*************************************
 *
 *	CPU flag constants
 *
 *************************************/

enum
{
	/* flags for CPU go into upper byte */
	CPU_FLAGS_MASK = 0xff00,

	/* set this if the CPU is used as a slave for audio. It will not be emulated if */
	/* sound is disabled, therefore speeding up a lot the emulation. */
	CPU_AUDIO_CPU = 0x8000,

	/* the Z80 can be wired to use 16 bit addressing for I/O ports */
	CPU_16BIT_PORT = 0x4000
};



/*************************************
 *
 *	Interrupt constants
 *
 *************************************/

enum
{
	/* generic "none" vector */
	INTERRUPT_NONE = -126,

	/* generic NMI vector */
	INTERRUPT_NMI = -127
};



/*************************************
 *
 *	Core CPU execution
 *
 *************************************/

/* Prepare CPUs for execution */
int cpu_init(void);

/* Run CPUs until the user quits */
void cpu_run(void);

/* Clean up after quitting */
void cpu_exit(void);

/* Force a reset after the current timeslice */
void machine_reset(void);



/*************************************
 *
 *	Save/restore
 *
 *************************************/

/* Load or save the game state */
enum
{
	LOADSAVE_NONE,
	LOADSAVE_SAVE,
	LOADSAVE_LOAD
};
void cpu_loadsave_schedule(int type, char id);
void cpu_loadsave_reset(void);



/*************************************
 *
 *	Optional watchdog
 *
 *************************************/

/* 8-bit watchdog read/write handlers */
WRITE_HANDLER( watchdog_reset_w );
READ_HANDLER( watchdog_reset_r );

/* 16-bit watchdog read/write handlers */
WRITE16_HANDLER( watchdog_reset16_w );
READ16_HANDLER( watchdog_reset16_r );

/* 32-bit watchdog read/write handlers */
WRITE32_HANDLER( watchdog_reset32_w );
READ32_HANDLER( watchdog_reset32_r );



/*************************************
 *
 *	CPU halt/reset lines
 *
 *************************************/

/* Set the logical state (ASSERT_LINE/CLEAR_LINE) of the RESET line on a CPU */
void cpu_set_reset_line(int cpu,int state);

/* Set the logical state (ASSERT_LINE/CLEAR_LINE) of the HALT line on a CPU */
void cpu_set_halt_line(int cpu,int state);

/* Returns status (1=running, 0=suspended for some reason) of a CPU */
int cpu_getstatus(int cpunum);



/*************************************
 *
 *	Timing helpers
 *
 *************************************/

/* Returns the number of cycles run so far this timeslice */
int cycles_currently_ran(void);

/* Returns the number of cycles left to run in this timeslice */
int cycles_left_to_run(void);

/* Returns the number of CPU cycles which take place in one video frame */
int cpu_gettotalcycles(void);

/* Returns the number of CPU cycles before the next interrupt handler call */
int cpu_geticount(void);

/* Scales a given value by the ratio of fcount / fperiod */
int cpu_scalebyfcount(int value);



/*************************************
 *
 *	Video timing
 *
 *************************************/

/* Returns the current scanline number */
int cpu_getscanline(void);

/* Returns the amount of time until a given scanline */
double cpu_getscanlinetime(int scanline);

/* Returns the duration of a single scanline */
double cpu_getscanlineperiod(void);

/* Returns the current horizontal beam position in pixels */
int cpu_gethorzbeampos(void);

/* Returns the current VBLANK state */
int cpu_getvblank(void);

/* Returns the number of the video frame we are currently playing */
int cpu_getcurrentframe(void);



/*************************************
 *
 *	Interrupt handling
 *
 *************************************/

/* Install a driver callback for IRQ acknowledge */
void cpu_set_irq_callback(int cpunum, int (*callback)(int irqline));

/* Set the vector to be returned during a CPU's interrupt acknowledge cycle */
void cpu_irq_line_vector_w(int cpunum, int irqline, int vector);

/* set the NMI line state for a CPU, normally use PULSE_LINE */
void cpu_set_nmi_line(int cpunum, int state);

/* set the IRQ line state for a specific irq line of a CPU */
/* normally use state HOLD_LINE, irqline 0 for first IRQ type of a cpu */
void cpu_set_irq_line(int cpunum, int irqline, int state);



/*************************************
 *
 *	Obsolete interrupt handling
 *
 *************************************/

/* OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE */
/* OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE */
/* OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE */

/* Obsolete functions: avoid to use them in new drivers if possible. */

void cpu_cause_interrupt(int cpu,int type);
void cpu_interrupt_enable(int cpu,int enabled);
WRITE_HANDLER( interrupt_enable_w );
WRITE_HANDLER( interrupt_vector_w );
int interrupt(void);
int nmi_interrupt(void);
int ignore_interrupt(void);
#if (HAS_M68000 || HAS_M68010 || HAS_M68020 || HAS_M68EC020)
int m68_level1_irq(void);
int m68_level2_irq(void);
int m68_level3_irq(void);
int m68_level4_irq(void);
int m68_level5_irq(void);
int m68_level6_irq(void);
int m68_level7_irq(void);
#endif

/* defines for backward compatibility */
#define Z80_NMI_INT 		INTERRUPT_NMI
#define Z80_IRQ_INT 		-1000

#define H6280_INT_IRQ1		0
#define H6280_INT_IRQ2		1
#define H6280_INT_NMI		INTERRUPT_NMI

#define M6502_INT_IRQ		0
#define M6502_INT_NMI		INTERRUPT_NMI
#define M6510_INT_IRQ		M6502_INT_IRQ
#define M6510_INT_NMI		M6502_INT_NMI
#define M6510T_INT_IRQ		M6502_INT_IRQ
#define M6510T_INT_NMI		M6502_INT_NMI
#define M7501_INT_IRQ		M6502_INT_IRQ
#define M7501_INT_NMI		M6502_INT_NMI
#define M8502_INT_IRQ		M6502_INT_IRQ
#define M8502_INT_NMI		M6502_INT_NMI
#define N2A03_INT_IRQ		M6502_INT_IRQ
#define N2A03_INT_NMI		M6502_INT_NMI
#define M65C02_INT_IRQ		M6502_INT_IRQ
#define M65C02_INT_NMI		M6502_INT_NMI
#define M65SC02_INT_IRQ 	M6502_INT_IRQ
#define M65SC02_INT_NMI 	M6502_INT_NMI

#define M6800_INT_NMI		INTERRUPT_NMI
#define M6800_INT_IRQ		0
#define M6801_INT_IRQ		M6800_INT_IRQ
#define M6801_INT_NMI		M6800_INT_NMI
#define M6802_INT_IRQ		M6800_INT_IRQ
#define M6802_INT_NMI		M6800_INT_NMI
#define M6803_INT_IRQ		M6800_INT_IRQ
#define M6803_INT_NMI		M6800_INT_NMI
#define M6808_INT_IRQ       M6800_INT_IRQ
#define M6808_INT_NMI       M6800_INT_NMI
#define HD63701_INT_IRQ 	M6800_INT_IRQ
#define HD63701_INT_NMI 	M6800_INT_NMI
#define NSC8105_INT_IRQ 	M6800_INT_IRQ
#define NSC8105_INT_NMI 	M6800_INT_NMI

#define M6809_INT_NMI		INTERRUPT_NMI
#define M6809_INT_IRQ		0
#define M6809_INT_FIRQ		1

/* OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE */
/* OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE */
/* OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE */



/*************************************
 *
 *	Synchronization
 *
 *************************************/

/* generate a trigger now */
void cpu_trigger(int trigger);

/* generate a trigger after a specific period of time */
void cpu_triggertime(double duration, int trigger);

/* generate a trigger corresponding to an interrupt on the given CPU */
void cpu_triggerint(int cpunum);

/* burn CPU cycles until a timer trigger */
void cpu_spinuntil_trigger(int trigger);

/* yield our timeslice until a timer trigger */
void cpu_yielduntil_trigger(int trigger);

/* burn CPU cycles until the next interrupt */
void cpu_spinuntil_int(void);

/* yield our timeslice until the next interrupt */
void cpu_yielduntil_int(void);

/* burn CPU cycles until our timeslice is up */
void cpu_spin(void);

/* yield our current timeslice */
void cpu_yield(void);

/* burn CPU cycles for a specific period of time */
void cpu_spinuntil_time(double duration);

/* yield our timeslice for a specific period of time */
void cpu_yielduntil_time(double duration);



/*************************************
 *
 *	Core timing
 *
 *************************************/

/* Returns the number of times the interrupt handler will be called before
   the end of the current video frame. This is can be useful to interrupt
   handlers to synchronize their operation. If you call this from outside
   an interrupt handler, add 1 to the result, i.e. if it returns 0, it means
   that the interrupt handler will be called once. */
int cpu_getiloops(void);



/*************************************
 *
 *	Z80 daisy chain
 *
 *************************************/

/* fix me - where should this stuff go? */

/* daisy-chain link */
typedef struct
{
	void (*reset)(int); 			/* reset callback	  */
	int  (*interrupt_entry)(int);	/* entry callback	  */
	void (*interrupt_reti)(int);	/* reti callback	  */
	int irq_param;					/* callback paramater */
} Z80_DaisyChain;

#define Z80_MAXDAISY	4		/* maximum of daisy chan device */

#define Z80_INT_REQ 	0x01	/* interrupt request mask		*/
#define Z80_INT_IEO 	0x02	/* interrupt disable mask(IEO)	*/

#define Z80_VECTOR(device,state) (((device)<<8)|(state))


#ifdef __cplusplus
}
#endif

#endif	/* CPUEXEC_H */
