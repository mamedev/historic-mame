//========================================
//  emulate.h
//
//  C Header file for TMS core
//========================================

#ifndef TMS9900_H
#define TMS9900_H

#include <stdio.h>
#include "driver.h"
#include "osd_cpu.h"

enum {
	TMS9900_PC=1, TMS9900_WP, TMS9900_STATUS, TMS9900_IR
#ifdef MAME_DEBUG
	,
	TMS9900_R0, TMS9900_R1, TMS9900_R2, TMS9900_R3,
	TMS9900_R4, TMS9900_R5, TMS9900_R6, TMS9900_R7,
	TMS9900_R8, TMS9900_R9, TMS9900_R10, TMS9900_R11,
	TMS9900_R12, TMS9900_R13, TMS9900_R14, TMS9900_R15
#endif
};

#define TMS9900_NONE  -1

extern	int tms9900_ICount;
extern	UINT8 lastparity;

extern void tms9900_reset(void *param);
extern int tms9900_execute(int cycles);
extern void tms9900_exit(void);
extern unsigned tms9900_get_context(void *dst);
extern void tms9900_set_context(void *src);
extern unsigned tms9900_get_pc(void);
extern void tms9900_set_pc(unsigned val);
extern unsigned tms9900_get_sp(void);
extern void tms9900_set_sp(unsigned val);
extern unsigned tms9900_get_reg(int regnum);
extern void tms9900_set_reg(int regnum, unsigned val);
extern void tms9900_set_nmi_line(int state);
extern void tms9900_set_irq_line(int irqline, int state);
extern void tms9900_set_irq_callback(int (*callback)(int irqline));
extern const char *tms9900_info(void *context, int regnum);
extern unsigned tms9900_dasm(char *buffer, unsigned pc);

#ifdef MAME_DEBUG
extern unsigned Dasm9900 (char *buffer, unsigned pc);
#endif

#endif





