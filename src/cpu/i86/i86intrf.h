/* ASG 971222 -- rewrote this interface */
#ifndef __I86_H_
#define __I86_H_

#include "memory.h"
#include "osd_cpu.h"

/* I86 registers */
typedef union
{					/* eight general registers */
	UINT16 w[8];	/* viewed as 16 bits registers */
	UINT8  b[16];	/* or as 8 bit registers */
} i86basicregs;

typedef struct
{
	i86basicregs regs;
	int ip;
	UINT16 flags;
	UINT16 sregs[4];
	int int_vector;
	int pending_irq;
#if NEW_INTERRUPT_SYSTEM
	int nmi_state;
	int irq_state;
	int (*irq_callback)(int irqline);
#endif
} i86_Regs;



#define I86_INT_NONE 0
#define I86_NMI_INT 2


/* Public variables */
extern int i86_ICount;

/* Public functions */
extern void i86_reset(void *param);
extern void i86_exit(void);
extern int i86_execute(int cycles);
extern void i86_setregs(i86_Regs *Regs);
extern void i86_getregs(i86_Regs *Regs);
extern unsigned i86_getpc(void);
extern unsigned i86_getreg(int regnum);
extern void i86_setreg(int regnum, unsigned val);
extern void i86_set_nmi_line(int state);
extern void i86_set_irq_line(int irqline, int state);
extern void i86_set_irq_callback(int (*callback)(int irqline));
extern const char *i86_info(void *context, int regnum);

#ifdef MAME_DEBUG
extern int mame_debug;
extern unsigned DasmI86(unsigned char* data, char* buffer, unsigned pc);
#endif

#endif
