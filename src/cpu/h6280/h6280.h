/*****************************************************************************

    h6280.h Portable Hu6280 emulator interface

    Copyright (c) 1999 Bryan McPhail, mish@tendril.co.uk

    This source code is based (with permission!) on the 6502 emulator by
    Juergen Buchmueller.  It is released as part of the Mame emulator project.
    Let me know if you intend to use this code in any other project.

******************************************************************************/

#ifndef _H6280_H
#define _H6280_H

#include "osd_cpu.h"

enum {
	H6280_PC=1, H6280_S, H6280_P, H6280_A, H6280_X, H6280_Y,
	H6280_IRQ_MASK, H6280_TIMER_STATE,
	H6280_NMI_STATE, H6280_IRQ1_STATE, H6280_IRQ2_STATE, H6280_IRQT_STATE
#ifdef MAME_DEBUG
    ,
	H6280_M1, H6280_M2, H6280_M3, H6280_M4,
	H6280_M5, H6280_M6, H6280_M7, H6280_M8
#endif
};

#define LAZY_FLAGS  0

#define H6280_RESET_VEC	0xfffe
#define H6280_NMI_VEC	0xfffc
#define H6280_TIMER_VEC	0xfffa
#define H6280_IRQ1_VEC	0xfff8
#define H6280_IRQ2_VEC	0xfff6			/* Aka BRK vector */

void h6280_get_info(UINT32 state, union cpuinfo *info);


READ8_HANDLER( H6280_irq_status_r );
WRITE8_HANDLER( H6280_irq_status_w );

READ8_HANDLER( H6280_timer_r );
WRITE8_HANDLER( H6280_timer_w );

/* functions for use by the PSG and joypad port only! */
UINT8 get_h6280io_buffer(void);
void set_h6280io_buffer(UINT8);

#ifdef MAME_DEBUG
extern int Dasm6280(char *buffer, int pc);
#endif

#endif /* _H6280_H */
