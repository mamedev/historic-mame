#ifndef M68000_H
#define M68000_H
#include "cpudefs.h"


/* Assembler core - regs in Assembler routine */

#ifdef A68KEM

extern regstruct regs;

#endif

/* ASG 971105 */
typedef struct
{
	regstruct regs;
	int pending_interrupts;
#if NEW_INTERRUPT_SYSTEM
    int irq_state;
    int (*irq_callback)(int irqline);
#endif
} MC68000_Regs;

extern void MC68000_Reset(void);                      /* ASG 971105 */
extern int  MC68000_Execute(int);                     /* ASG 971105 */
extern void MC68000_SetRegs(MC68000_Regs *);          /* ASG 971105 */
extern void MC68000_GetRegs(MC68000_Regs *);          /* ASG 971105 */
#if NEW_INTERRUPT_SYSTEM
extern void MC68000_set_nmi_line(int);
extern void MC68000_set_irq_line(int, int);
extern void MC68000_set_irq_callback(int (*callback)(int));
#else
extern void MC68000_Cause_Interrupt(int);             /* ASG 971105 */
extern void MC68000_Clear_Pending_Interrupts(void);   /* ASG 971105 */
#endif
extern int  MC68000_GetPC(void);                      /* ASG 971105 */

extern void MC68000_disasm(CPTR, CPTR*, int);


extern int MC68000_ICount;                            /* ASG 971105 */


#define MC68000_INT_NONE 0							  /* ASG 971105 */
#define MC68000_IRQ_1    1
#define MC68000_IRQ_2    2
#define MC68000_IRQ_3    3
#define MC68000_IRQ_4    4
#define MC68000_IRQ_5    5
#define MC68000_IRQ_6    6
#define MC68000_IRQ_7    7

#define MC68000_STOP     0x10


#endif
