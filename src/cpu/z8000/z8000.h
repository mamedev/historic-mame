#ifndef Z8K_H

#include "osd_cpu.h"

typedef union {
	UINT8	B[16]; /* RL0,RH0,RL1,RH1...RL7,RH7 */
	UINT16	W[16]; /* R0,R1,R2...R15 */
	UINT32	L[8];  /* RR0,RR2,RR4..RR14 */
	UINT64	Q[4];  /* RQ0,RQ4,..RQ12 */
}	Z8000_R;

typedef struct {
	UINT16	op[4];		/* opcodes/data of current instruction */
    UINT16  pc;         /* program counter */
    UINT16  psap;       /* program status pointer */
    UINT16  fcw;        /* flags and control word */
    UINT16  refresh;    /* refresh timer/counter */
    UINT16  nsp;        /* system stack pointer */
    UINT16  irq_req;    /* CPU is halted, interrupt or trap request */
    UINT16  irq_srv;    /* serviced interrupt request */
    UINT16  irq_vec;    /* interrupt vector */
#if NEW_INTERRUPT_SYSTEM
	int nmi_state;		/* NMI line state */
	int irq_state[2];	/* IRQ line states (NVI, VI) */
	int (*irq_callback)(int irqline);
#endif
    Z8000_R regs;       /* registers */
}	Z8000_Regs;

/* Interrupt Types that can be generated by outside sources */
#define Z8000_TRAP		0x4000	/* internal trap */
#define Z8000_NMI		0x2000	/* non maskable interrupt */
#define Z8000_SEGTRAP	0x1000	/* segment trap (Z8001) */
#define Z8000_NVI		0x0800	/* non vectored interrupt */
#define Z8000_VI		0x0400	/* vectored interrupt (LSB is vector)  */
#define Z8000_SYSCALL	0x0200	/* system call (lsb is vector) */
#define Z8000_HALT		0x0100	/* halted flag	*/
#define Z8000_INT_NONE  0x0000

/* PUBLIC FUNCTIONS */
unsigned Z8000_GetPC(void);
void Z8000_SetRegs(Z8000_Regs *Regs);
void Z8000_GetRegs(Z8000_Regs *Regs);
void Z8000_Reset(void);
int  Z8000_Execute(int cycles);
#if NEW_INTERRUPT_SYSTEM
void Z8000_set_nmi_line(int state);
void Z8000_set_irq_line(int irqline, int state);
void Z8000_set_irq_callback(int (*callback)(int irqline));
#else
void Z8000_Cause_Interrupt(int type);
void Z8000_Clear_Pending_Interrupts(void);
#endif

void Z8000_State_Save(int cpunum, void *f);
void Z8000_State_Load(int cpunum, void *f);

/* PUBLIC GLOBALS */
extern int Z8000_ICount;

#endif /* Z8K_H */
