/* ASG 971222 -- rewrote this interface */
#ifndef __NEC_H_
#define __NEC_H_

#include "memory.h"
#include "osd_cpu.h"

enum {
	NEC_IP=1, NEC_AW, NEC_CW, NEC_DW, NEC_BW, NEC_SP, NEC_BP, NEC_IX, NEC_IY,
	NEC_FLAGS, NEC_ES, NEC_CS, NEC_SS, NEC_DS,
	NEC_VECTOR, NEC_PENDING, NEC_NMI_STATE, NEC_IRQ_STATE};

#define NEC_INT_NONE 0
#define NEC_NMI_INT 2

/* Public variables */
extern int nec_ICount;

/* Public functions */
extern void nec_reset(void *param);
extern void nec_exit(void);
extern int nec_execute(int cycles);
extern unsigned nec_get_context(void *dst);
extern void nec_set_context(void *src);
extern unsigned nec_get_pc(void);
extern void nec_set_pc(unsigned val);
extern unsigned nec_get_sp(void);
extern void nec_set_sp(unsigned val);
extern unsigned nec_get_reg(int regnum);
extern void nec_set_reg(int regnum, unsigned val);
extern void nec_set_nmi_line(int state);
extern void nec_set_irq_line(int irqline, int state);
extern void nec_set_irq_callback(int (*callback)(int irqline));
extern const char *nec_v20_info(void *context, int regnum);
extern const char *nec_v30_info(void *context, int regnum);
extern const char *nec_v33_info(void *context, int regnum);
extern unsigned nec_dasm(char *buffer, unsigned pc);

#ifdef MAME_DEBUG
extern unsigned Dasmnec(char* buffer, unsigned pc);
#endif

#endif
