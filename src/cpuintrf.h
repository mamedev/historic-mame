#ifndef CPUINTRF_H
#define CPUINTRF_H

#define CPU_CONTEXT_SIZE 200		/* ASG 971105 */


/* ASG 971222 -- added this generic structure */
struct cpu_interface
{
	void (*reset)(void);
	int (*execute)(int cycles);
	void (*set_regs)(void *regs);
	void (*get_regs)(void *regs);
	unsigned int (*get_pc)(void);
	void (*cause_interrupt)(int type);
	void (*clear_pending_interrupts)(void);
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

int cpu_getpc(void);
int cpu_getpreviouspc(void);  /* -RAY- */
int cpu_getreturnpc(void);
/* Returns the number of CPU cycles which take place in one video frame */
int cpu_gettotalcycles(void);
/* Returns the number of CPU cycles before the next interrupt handler call */
int cpu_geticount(void);
/* Returns the number of CPU cycles before the end of the current video frame */
int cpu_getfcount(void);
/* Returns the number of CPU cycles in one video frame */
int cpu_getfperiod(void);
void cpu_seticount(int cycles);
/*
  Returns the number of times the interrupt handler will be called before
  the end of the current video frame. This is can be useful to interrupt
  handlers to synchronize their operation. If you call this from outside
  an interrupt handler, add 1 to the result, i.e. if it returns 0, it means
  that the interrupt handler will be called once.
*/
int cpu_getiloops(void);


/* cause an interrupt on a CPU */
void cpu_cause_interrupt(int cpu,int type);
void cpu_clear_pending_interrupts(int cpu);
void interrupt_enable_w(int offset,int data);
void interrupt_vector_w(int offset,int data);
int interrupt(void);
int nmi_interrupt(void);
int ignore_interrupt(void);

void cpu_getcpucontext (int, unsigned char *);
void cpu_setcpucontext (int, const unsigned char *);

#endif
