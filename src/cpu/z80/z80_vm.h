#ifndef Z80_VM_H
#define Z80_VM_H

#include "cpuintrf.h"
#include "osd_cpu.h"
#include "z80.h"

#define Z80_BANK0 Z80_NMI_NESTING+1
#define Z80_BANK1 Z80_NMI_NESTING+2
#define Z80_BANK2 Z80_NMI_NESTING+3
#define Z80_BANK3 Z80_NMI_NESTING+4
#define Z80_BANK4 Z80_NMI_NESTING+5
#define Z80_BANK5 Z80_NMI_NESTING+6
#define Z80_BANK6 Z80_NMI_NESTING+7
#define Z80_BANK7 Z80_NMI_NESTING+8

extern void z80_vm_reset (void *param);
extern void z80_vm_exit (void);
extern int z80_vm_execute(int cycles);
extern void z80_vm_burn(int cycles);
extern unsigned z80_vm_get_context (void *dst);
extern void z80_vm_set_context (void *src);
extern unsigned z80_vm_get_pc (void);
extern void z80_vm_set_pc (unsigned val);
extern unsigned z80_vm_get_sp (void);
extern void z80_vm_set_sp (unsigned val);
extern unsigned z80_vm_get_reg (int regnum);
extern void z80_vm_set_reg (int regnum, unsigned val);
extern void z80_vm_set_nmi_line(int state);
extern void z80_vm_set_irq_line(int irqline, int state);
extern void z80_vm_set_irq_callback(int (*irq_callback)(int));
extern void z80_vm_state_save(void *file);
extern void z80_vm_state_load(void *file);
extern const char *z80_vm_info(void *context, int regnum);
extern unsigned z80_vm_dasm(char *buffer, unsigned pc);

#ifdef MAME_DEBUG
extern unsigned DasmZ80(char *buffer, unsigned pc);
#endif

#endif	/* Z80_VM_H */


