/*****************************************************************************
 *
 *	 saturn.h
 *	 portable saturn emulator interface
 *   (hp calculators)
 *
 *	 Copyright (c) 2000 Peter Trauner, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   peter.trauner@jk.uni-linz.ac.at
 *	 - The author of this copywritten work reserves the right to change the
 *	   terms of its usage and license at any time, including retroactively
 *	 - This entire notice must remain in the source code.
 *
 *****************************************************************************/
/*
Calculator        Release Date          Chip Version     Analog/Digital IC
HP71B (early)     02/01/84              1LF2              -
HP71B (later)     ??/??/??              1LK7              -
HP18C             06/01/86              1LK7              -
HP28C             01/05/87              1LK7              -
HP17B             01/04/88              1LT8             Lewis
HP19B             01/04/88              1LT8             Lewis
HP27S             01/04/88              1LT8             Lewis
HP28S             01/04/88              1LT8             Lewis
HP48SX            03/16/91              1LT8             Clarke
HP48S             04/02/91              1LT8             Clarke
HP48GX            06/01/93              1LT8             Yorke
HP48G             06/01/93              1LT8             Yorke
HP38G             09/??/95              1LT8             Yorke
*/
/* 4 bit processor
   20 address lines */

#ifndef _SATURN_H
#define _SATURN_H

#include "cpuintrf.h"
#include "osd_cpu.h"


#ifdef __cplusplus
extern "C" {
#endif

#ifdef RUNTIME_LOADER
	extern void saturn_runtime_loader_init(void);
#endif

#define SATURN_INT_NONE	0
#define SATURN_INT_IRQ	1
#define SATURN_INT_NMI	2

typedef struct {
	void (*out)(int);
	int (*in)(void);
	void (*reset)(void);
	void (*config)(int v);
	void (*unconfig)(int v);
	int (*id)(void);
	void (*crc)(int addr, int data);
} SATURN_CONFIG;

extern int saturn_icount;				/* cycle count */

extern void saturn_reset(void *param);
extern void saturn_exit(void);
extern int	saturn_execute(int cycles);
extern unsigned saturn_get_context(void *dst);
extern void saturn_set_context(void *src);
extern unsigned saturn_get_pc(void);
extern void saturn_set_pc(unsigned val);
extern unsigned saturn_get_sp(void);
extern void saturn_set_sp(unsigned val);
extern unsigned saturn_get_reg(int regnum);
extern void saturn_set_reg(int regnum, unsigned val);
extern void saturn_set_nmi_line(int state);
extern void saturn_set_irq_line(int irqline, int state);
extern void saturn_set_irq_callback(int (*callback)(int irqline));
extern void saturn_state_save(void *file);
extern void saturn_state_load(void *file);
extern const char *saturn_info(void *context, int regnum);
extern unsigned saturn_dasm(char *buffer, unsigned pc);

#ifdef __cplusplus
	}
#endif
	
#endif

