#ifndef TMS9900_H
#define TMS9900_H

#include "types.h"

#define TMS9900_NONE  -1

#ifndef FALSE
#define FALSE 0
#endif

typedef struct
{
	word	WP;
	word	PC;
	word	STATUS;
	word	IR;
#if NEW_INTERRUPT_SYSTEM
	int irq_state;
	int (*irq_callback)(int irq_line);
#endif
} TMS9900_Regs;

extern	int TMS9900_ICount;

void TMS9900_Reset(void);
int  TMS9900_Execute(int cycles);
void TMS9900_GetRegs(TMS9900_Regs *Regs);
void TMS9900_SetRegs(TMS9900_Regs *Regs);
int  TMS9900_GetPC(void);
#if NEW_INTERRUPT_SYSTEM
void TMS9900_set_nmi_line(int state);
void TMS9900_set_irq_line(int irqline, int state);
void TMS9900_set_irq_callback(int (*callback)(int irqline));
#else
void TMS9900_Cause_Interrupt(int type);
void TMS9900_Clear_Pending_Interrupts(void);
#endif

#endif

