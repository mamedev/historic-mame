#ifndef __I386INTF_H
#define __I386INTF_H

#include "memory.h"
#include "osd_cpu.h"

extern int i386_ICount;

extern void i386_init(void);
extern void i386_reset(void* param);
extern void i386_exit(void);
extern int i386_execute(int cycles);
extern unsigned i386_get_context(void *dst);
extern void i386_set_context(void *src);
extern unsigned i386_get_reg(int regnum);
extern void i386_set_reg(int regnum, unsigned val);
extern void i386_set_irq_line(int irqline, int state);
extern void i386_set_irq_callback(int (*callback)(int irqline));
extern const char *i386_info(void *context, int regnum);
extern unsigned i386_dasm(char *buffer, unsigned pc);

void i386_get_info(UINT32, union cpuinfo*);

#ifdef MAME_DEBUG
extern unsigned DasmI386(char* buffer, unsigned pc, unsigned int, unsigned int);
#endif

#endif /* __I386INTF_H */
