/*****************************************************************************
 *
 *	 i8x41.h
 *	 Portable UPI-41/8041/8741/8042/8742 emulator interface
 *
 *	 Copyright (c) 1999 Juergen Buchmueller, all rights reserved.
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   pullmoll@t-online.de
 *	 - The author of this copywritten work reserves the right to change the
 *	   terms of its usage and license at any time, including retroactively
 *	 - This entire notice must remain in the source code.
 *
 *****************************************************************************/

#ifndef _I8X41_H
#define _I8X41_H

#include "cpuintrf.h"
#include "osd_cpu.h"

/* Note:
 * I8X41_DATA is A0 = 0 and R/W
 * I8X41_CMND is A0 = 1 and W only
 * I8X41_STAT is A0 = 1 and R only
 */
enum {
	I8X41_PC=1, I8X41_SP, I8X41_PSW, I8X41_A, I8X41_T, I8X41_DATA, I8X41_CMND, I8X41_STAT,
	I8X41_R0, I8X41_R1, I8X41_R2, I8X41_R3, I8X41_R4, I8X41_R5, I8X41_R6, I8X41_R7
};

#define I8X41_INT_IBF	0	/* input buffer full interrupt */
#define I8X41_INT_TEST0 1	/* test0 line */
#define I8X41_INT_TEST1 2	/* test1 line (also counter interrupt; taken on cntr overflow)	*/

extern int i8x41_ICount;				/* cycle count */

extern void i8x41_init (void);					/* Initialize save states */
extern void i8x41_reset (void *param);			/* Reset registers to the initial values */
extern void i8x41_exit	(void); 				/* Shut down CPU core */
extern int	i8x41_execute(int cycles);			/* Execute cycles - returns number of cycles actually run */
extern unsigned i8x41_get_context (void *dst);	/* Get registers, return context size */
extern void i8x41_set_context (void *src);		/* Set registers */
extern unsigned i8x41_get_reg (int regnum);
extern void i8x41_set_reg (int regnum, unsigned val);
extern void i8x41_set_irq_line(int irqline, int state);
extern void i8x41_set_irq_callback(int (*callback)(int irqline));
extern void i8x41_state_save(void *file);
extern void i8x41_state_load(void *file);
extern const char *i8x41_info(void *context, int regnum);
extern unsigned i8x41_dasm(char *buffer, unsigned pc);

#ifdef MAME_DEBUG
extern unsigned Dasm8x41( char *dst, unsigned pc );
#endif

#endif /* _I8X41_H */


