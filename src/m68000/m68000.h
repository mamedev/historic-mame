#include "cpudefs.h"

/* ASG 971105 */
typedef struct
{
	regstruct regs;
	int pending_interrupts;
} MC68000_Regs;

extern void MC68000_Reset(void);                      /* ASG 971105 */
extern int  MC68000_Execute(int);                     /* ASG 971105 */
extern void MC68000_SetRegs(MC68000_Regs *);          /* ASG 971105 */
extern void MC68000_GetRegs(MC68000_Regs *);          /* ASG 971105 */
extern void MC68000_Cause_Interrupt(int);             /* ASG 971105 */
extern void MC68000_Clear_Pending_Interrupts(void);   /* ASG 971105 */
extern int  MC68000_GetPC(void);                      /* ASG 971105 */

extern void MC68000_disasm(CPTR, CPTR*, int);

extern int MC68000_ICount;                            /* ASG 971105 */

/* ASG 971105 */
#define MC68000_INT_NONE 0
#define MC68000_IRQ_1    1
#define MC68000_IRQ_2    2
#define MC68000_IRQ_3    3
#define MC68000_IRQ_4    4
#define MC68000_IRQ_5    5
#define MC68000_IRQ_6    6
#define MC68000_IRQ_7    7
