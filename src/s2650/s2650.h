#ifndef S2650_H
#define S2650_H

#include "osd_cpu.h"

#define S2650_INT_NONE	0
#define S2650_INT_IRQ	1

/* fake control port   M/~IO=0 D/~C=0 E/~NE=0 */
#define S2650_CTRL_PORT 0x100

/* fake data port      M/~IO=0 D/~C=1 E/~NE=0 */
#define S2650_DATA_PORT 0x101

/* extended i/o ports  M/~IO=0 D/~C=x E/~NE=1 */
#define S2650_EXT_PORT	0xff

typedef struct {
	UINT16	page;	/* 8K page select register (A14..A13) */
	UINT16	iar;	/* instruction address register (A12..A0) */
	UINT16	ea; 	/* effective address (A14..A0) */
	UINT8	psl;	/* processor status lower */
	UINT8	psu;	/* processor status upper */
	UINT8	r;		/* absolute addressing dst/src register */
	UINT8	reg[7]; /* 7 general purpose registers */
	UINT8	halt;	/* 1 if cpu is halted */
	UINT8	ir; 	/* instruction register */
	int 	irq;	/* interrupt request vector */
	UINT16 ras[8];	/* 8 return address stack entries */
#if NEW_INTERRUPT_SYSTEM
	int 	irq_state;
	int 	(*irq_callback)(int irqline);
#endif
}	S2650_Regs;

extern void S2650_SetRegs(S2650_Regs * rgs);
extern void S2650_GetRegs(S2650_Regs * rgs);
extern int	S2650_GetPC(void);
extern void S2650_set_flag(int state);
extern int	S2650_get_flag(void);
extern void S2650_set_sense(int state);
extern int	S2650_get_sense(void);
extern void S2650_Reset(void);
extern int	S2650_Execute(int cycles);
#if NEW_INTERRUPT_SYSTEM
extern void S2650_set_nmi_line(int state);
extern void S2650_set_irq_line(int irqline, int state);
extern void S2650_set_irq_callback(int (*callback)(int irqline));
#else
extern void S2650_Cause_Interrupt(int type);
extern void S2650_Clear_Pending_Interrupts(void);
#endif

extern int S2650_ICount;

#ifdef  MAME_DEBUG
extern  int     Dasm2650(char * buff, int PC);
#endif

#endif
