#ifndef I8085_H
#define I8085_H

enum { I8085_PC, I8085_SP, I8085_BC, I8085_DE, I8085_HL, I8085_AF,
	I8085_HALT, I8085_IM, I8085_IREQ, I8085_ISRV, I8085_VECTOR,
	I8085_TRAP_STATE, I8085_INTR_STATE,
	I8085_RST55_STATE, I8085_RST65_STATE, I8085_RST75_STATE};

#define I8085_INTR_LINE 	0
#define I8085_RST55_LINE	1
#define I8085_RST65_LINE	2
#define I8085_RST75_LINE	3

#define I8085_NONE      0
#define I8085_TRAP      0x01
#define I8085_RST55     0x02
#define I8085_RST65     0x04
#define I8085_RST75     0x08
#define I8085_SID       0x10
#define I8085_INTR      0xff

extern int i8085_ICount;

extern void i8085_set_SID(int state);
extern void i8085_set_SOD_callback(void (*callback)(int state));
extern void i8085_reset(void *param);
extern void i8085_exit(void);
extern int i8085_execute(int cycles);
extern unsigned i8085_get_context(void *dst);
extern void i8085_set_context(void *src);
extern unsigned i8085_get_pc(void);
extern void i8085_set_pc(unsigned val);
extern unsigned i8085_get_sp(void);
extern void i8085_set_sp(unsigned val);
extern unsigned i8085_get_reg(int regnum);
extern void i8085_set_reg(int regnum, unsigned val);
extern void i8085_set_nmi_line(int state);
extern void i8085_set_irq_line(int irqline, int state);
extern void i8085_set_irq_callback(int (*callback)(int irqline));
extern void i8085_state_save(void *file);
extern void i8085_state_load(void *file);
extern const char *i8085_info(void *context, int regnum);

#define I8080_INTR_LINE 		I8085_INTR_LINE
#define I8080_TRAP				I8085_TRAP
#define I8080_INTR				I8085_INTR
#define I8080_TRAP				I8085_TRAP
#define I8080_NONE				I8085_NONE

#define     i8080_ICount            i8085_ICount
extern void i8080_reset(void *param);
extern void i8080_exit(void);
extern int i8080_execute(int cycles);
extern unsigned i8080_get_context(void *dst);
extern void i8080_set_context(void *src);
extern unsigned i8080_get_pc(void);
extern void i8080_set_pc(unsigned val);
extern unsigned i8080_get_sp(void);
extern void i8080_set_sp(unsigned val);
extern unsigned i8080_get_reg(int regnum);
extern void i8080_set_reg(int regnum, unsigned val);
extern void i8080_set_nmi_line(int state);
extern void i8080_set_irq_line(int irqline, int state);
extern void i8080_set_irq_callback(int (*callback)(int irqline));
extern void i8080_state_save(void *file);
extern void i8080_state_load(void *file);
extern const char *i8080_info(void *context, int regnum);

#ifdef	MAME_DEBUG
extern int mame_debug;
extern int Dasm8085(char * dst, int PC);
#endif

#endif
