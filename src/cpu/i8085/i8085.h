#ifndef I8085_H
#define I8085_H

typedef struct {
	PAIR	PC,SP,BC,DE,HL,AF,XX;
	UINT8	HALT;
	UINT8	IM; 	/* interrupt mask */
	UINT8	IREQ;	/* requested interrupts */
	UINT8	ISRV;	/* serviced interrupt */
	UINT32	INTR;	/* vector for INTR */
	UINT32	IRQ2;	/* scheduled interrupt address */
	UINT32	IRQ1;	/* executed interrupt address */
	INT8	nmi_state;
	INT8	irq_state[4];
	INT8	filler; /* align on dword boundary */
	int 	(*irq_callback)(int);
	void	(*sod_callback)(int state);
}	i8085_Regs;

#define I8085_INTR      0xff
#define I8085_SID       0x10
#define I8085_RST75     0x08
#define I8085_RST65     0x04
#define I8085_RST55     0x02
#define I8085_TRAP      0x01
#define I8085_NONE      0

extern int i8085_ICount;

extern void i8085_set_SID(int state);
extern void i8085_set_SOD_callback(void (*callback)(int state));
extern void i8085_reset(void *param);
extern void i8085_exit(void);
extern int i8085_execute(int cycles);
extern void i8085_setregs(i8085_Regs *regs);
extern void i8085_getregs(i8085_Regs *regs);
extern unsigned i8085_getpc(void);
extern unsigned i8085_getreg(int regnum);
extern void i8085_setreg(int regnum, unsigned val);
extern void i8085_set_nmi_line(int state);
extern void i8085_set_irq_line(int irqline, int state);
extern void i8085_set_irq_callback(int (*callback)(int irqline));
extern void i8085_state_save(void *file);
extern void i8085_state_load(void *file);
extern const char *i8085_info(void *context, int regnum);

/* for now let the 8080 use the 8085 functions */
#define 	I8080_INTR				I8085_INTR
#define     I8080_SID               I8085_SID
#define     I8080_RST75             I8085_RST75
#define     I8080_RST65             I8085_RST65
#define     I8080_RST55             I8085_RST55
#define     I8080_TRAP              I8085_TRAP
#define     I8080_NONE              I8085_NONE

#define     i8080_ICount            i8085_ICount
#define 	i8080_set_SID			i8085_set_SID
#define 	i8080_set_SOD_callback	i8085_set_SOD_callback
#define 	i8080_reset 			i8085_reset
#define 	i8080_exit				i8085_exit
#define 	i8080_execute			i8085_execute
#define 	i8080_setregs			i8085_setregs
#define 	i8080_getregs			i8085_getregs
#define 	i8080_getpc 			i8085_getpc
#define 	i8080_getreg 			i8085_getreg
#define 	i8080_setreg 			i8085_setreg
#define 	i8080_set_nmi_line		i8085_set_nmi_line
#define 	i8080_set_irq_line		i8085_set_irq_line
#define 	i8080_set_irq_callback	i8085_set_irq_callback
#define 	i8080_state_save		i8085_state_save
#define 	i8080_state_load		i8085_state_load
extern const char *i8080_info(void *context, int regnum);

#ifdef	MAME_DEBUG
extern int mame_debug;
extern int Dasm8085(char * dst, int PC);
#endif

#endif
