#ifndef I8085_H
#define I8085_H

typedef union {
#ifdef  LSB_FIRST
		struct { UINT8 l,h,h2,h3; } B;
		struct { UINT16 l,h; } W;
		UINT32 D;
#else
		struct { UINT8 h3,h2,h,l; } B;
		struct { UINT16 h,l; } W;
		UINT32 D;
#endif
}       I8085_Pair;

typedef struct {
	I8085_Pair	PC, SP, BC, DE, HL, AF, XX;
	int HALT;
	int IM; 	/* interrupt mask */
	int IREQ;	/* requested interrupts */
	int ISRV;	/* serviced interrupt */
	int INTR;	/* vector for INTR */
	int IRQ2;	/* scheduled interrupt address */
	int IRQ1;	/* executed interrupt address */
#if NEW_INTERRUPT_SYSTEM
	int nmi_state;
	int irq_state[4];
	int (*irq_callback)(int);
#endif
    void (*SOD_callback)(int state);
}	I8085_Regs;

#define I8085_INTR      0xff
#define I8085_SID       0x10
#define I8085_RST75     0x08
#define I8085_RST65     0x04
#define I8085_RST55     0x02
#define I8085_TRAP      0x01
#define I8085_NONE      0

extern  int     I8085_ICount;

extern void I8085_SetRegs(I8085_Regs * regs);
extern void I8085_GetRegs(I8085_Regs * regs);
extern unsigned I8085_GetPC(void);
extern void I8085_SetSID(int state);
extern void I8085_SetSOD_callback(void (*callback)(int state));
extern void I8085_Reset(void);
int I8085_Execute(int cycles);
#if NEW_INTERRUPT_SYSTEM
extern void I8085_set_nmi_line(int state);
extern void I8085_set_irq_line(int irqline, int state);
extern void I8085_set_irq_callback(int (*callback)(int irqline));
#else
extern void I8085_Cause_Interrupt(int type);
extern void I8085_Clear_Pending_Interrupts(void);
#endif

#ifdef  MAME_DEBUG
int     Dasm8085(char * dst, int PC);
#endif

#endif
