#ifndef Z80_H
#define Z80_H

#include "osd_cpu.h"
#include "cpuintrf.h"

/****************************************************************************/
/* The Z80 registers. HALT is set to 1 when the CPU is halted, the refresh  */
/* register is calculated as follows: refresh=(Regs.R&127)|(Regs.R2&128)    */
/****************************************************************************/
typedef struct {
/* 00 */	PAIR	AF,BC,DE,HL,IX,IY,PC,SP;
/* 20 */	PAIR	AF2,BC2,DE2,HL2;
/* 30 */	UINT8	R,R2,IFF1,IFF2,HALT,IM,I;
/* 37 */	UINT8	irq_max;			/* number of daisy chain devices		*/
/* 38 */	INT8	request_irq;		/* daisy chain next request device		*/
/* 39 */	INT8	service_irq;		/* daisy chain next reti handling devicve */
/* 3a */	UINT8	int_state[Z80_MAXDAISY];
/* 3e */	INT8	nmi_state;			/* nmi line state */
/* 3f */	INT8	irq_state;			/* irq line state */
/* 40 */	Z80_DaisyChain irq[Z80_MAXDAISY];
/* 80 */	int 	(*irq_callback)(int irqline);
/* 84 */	int 	extra_cycles;		/* extra cycles for interrupts */
/* 88 */	int 	nmi_nesting;		/* nested NMI depth */
}   Z80_Regs;

extern int z80_ICount;				/* T-state count						*/

#define Z80_IGNORE_INT  -1          /* Ignore interrupt                     */
#define Z80_NMI_INT 	-2			/* Execute NMI							*/
#define Z80_IRQ_INT 	-1000		/* Execute IRQ							*/

/* Port number written to when entering/leaving HALT state */
#define Z80_HALT_PORT   0x10000     

extern void z80_reset (void *param);
extern void z80_exit (void);
extern int	z80_execute(int cycles);	   /* Execute cycles T-States - returns number of cycles actually run */
extern void z80_getregs (Z80_Regs *Regs);  /* Get registers 				*/
extern void z80_setregs (Z80_Regs *Regs);  /* Set registers 				*/
extern unsigned z80_getpc (void);		   /* Get program counter			*/
extern unsigned z80_getreg (int regnum);
extern void z80_setreg (int regnum, unsigned val);
extern void z80_set_nmi_line(int state);
extern void z80_set_irq_line(int irqline, int state);
extern void z80_set_irq_callback(int (*irq_callback)(int));
extern void z80_state_save(void *file);
extern void z80_state_load(void *file);
extern const char *z80_info(void *context, int regnum);

#ifdef MAME_DEBUG
extern int DasmZ80(char *buffer, int PC);
#endif

#endif

