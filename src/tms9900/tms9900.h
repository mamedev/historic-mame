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
}
TMS9900_Regs ;

extern  int     TMS9900_ICount;

void    TMS9900_Reset(void);
int     TMS9900_Execute(int cycles);
void    TMS9900_GetRegs(TMS9900_Regs *Regs);
void    TMS9900_SetRegs(TMS9900_Regs *Regs);
int     TMS9900_GetPC(void);
void    TMS9900_Cause_Interrupt(int type);
void    TMS9900_Clear_Pending_Interrupts(void);

#endif

