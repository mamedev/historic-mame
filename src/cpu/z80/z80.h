#ifndef Z80_H
#define Z80_H

#include "osd_cpu.h"
#include "cpuintrf.h"


/****************************************************************************/
/* Define a Z80 word. Upper bytes are always zero                           */
/****************************************************************************/
typedef union {
 #ifdef LSB_FIRST
   struct { UINT8 l,h,h2,h3; } B;
   struct { UINT16 l,h; } W;
   UINT32 D;
 #else
   struct { UINT8 h3,h2,h,l; } B;
   struct { UINT16 h,l; } W;
   UINT32 D;
 #endif
} Z80_pair; /* -NS- */

/****************************************************************************/
/* The Z80 registers. HALT is set to 1 when the CPU is halted, the refresh  */
/* register is calculated as follows: refresh=(Regs.R&127)|(Regs.R2&128)    */
/****************************************************************************/
typedef struct {
	Z80_pair	AF,BC,DE,HL,IX,IY,PC,SP;
	Z80_pair	AF2,BC2,DE2,HL2;
	unsigned	R;
	UINT8		IFF1,IFF2,HALT,IM,I,R2;

	int vector; 					/* vector for SINGLE INT mode			*/
	int pending_irq;
	int irq_max;					/* number of daisy chain devices		*/
	int request_irq;				/* daisy chain next request device		*/
	int service_irq;				/* daisy chain next reti handling devicve */
	int int_state[Z80_MAXDAISY];
	Z80_DaisyChain irq[Z80_MAXDAISY];
#if NEW_INTERRUPT_SYSTEM
	int nmi_state;					/* nmi line state */
	int irq_state;					/* irq line state */
	int (*irq_callback)(int irqline);	/* irq callback */
#endif
}   Z80_Regs;

extern int Z80_ICount;				/* T-state count						*/

#define Z80_IGNORE_INT	-1			/* Ignore interrupt 					*/
#define Z80_NMI_INT 	-2			/* Execute NMI							*/
#define Z80_IRQ_INT 	-1000		/* Execute IRQ							*/

extern unsigned Z80_GetPC (void);		   /* Get program counter				   */
extern void Z80_GetRegs (Z80_Regs *Regs);  /* Get registers 					   */
extern void Z80_SetRegs (Z80_Regs *Regs);  /* Set registers 					   */
extern void Z80_Reset (Z80_DaisyChain *daisy_chain);
extern int	Z80_Execute(int cycles);	   /* Execute cycles T-States - returns number of cycles actually run */
extern int	Z80_Interrupt(void);
#if NEW_INTERRUPT_SYSTEM
extern void Z80_set_nmi_line(int state);
extern void Z80_set_irq_line(int irqline, int state);
extern void Z80_set_irq_callback(int (*irq_callback)(int));
#else
extern void Z80_Cause_Interrupt(int type);
extern void Z80_Clear_Pending_Interrupts(void);
#endif

#ifdef MAME_DEBUG
extern int DasmZ80(char *buffer, int PC);
#endif

#endif

