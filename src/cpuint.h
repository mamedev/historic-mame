/***************************************************************************

    cpuint.h

    Core multi-CPU interrupt engine.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __CPUINT_H__
#define __CPUINT_H__

#include "memory.h"


/*************************************
 *
 *  Startup/shutdown
 *
 *************************************/

int cpuint_init(void);

void cpuint_reset_cpu(int cpunum);

extern int (*cpu_irq_callbacks[])(int);



/*************************************
 *
 *  CPU lines
 *
 *************************************/

// fix me:
// cpu_set_reset_line/cpunum_set_reset_line
// cpu_set_halt_line/cpunum_set_halt_line
// cpunum_set_input_line/cpunum_set_input_line_and_vector
// cpu_set_nmi_line

/* Set the logical state (ASSERT_LINE/CLEAR_LINE) of the an input line on a CPU */
void cpunum_set_input_line(int cpunum, int line, int state);

/* Set the vector to be returned during a CPU's interrupt acknowledge cycle */
void cpunum_set_input_line_vector(int cpunum, int irqline, int vector);

/* Set the logical state (ASSERT_LINE/CLEAR_LINE) of the an input line on a CPU and its associated vector */
void cpunum_set_input_line_and_vector(int cpunum, int line, int state, int vector);

/* Install a driver callback for IRQ acknowledge */
void cpu_set_irq_callback(int cpunum, int (*callback)(int irqline));



/*************************************
 *
 *  Preferred interrupt callbacks
 *
 *************************************/

INTERRUPT_GEN( nmi_line_pulse );
INTERRUPT_GEN( nmi_line_assert );

INTERRUPT_GEN( irq0_line_hold );
INTERRUPT_GEN( irq0_line_pulse );
INTERRUPT_GEN( irq0_line_assert );

INTERRUPT_GEN( irq1_line_hold );
INTERRUPT_GEN( irq1_line_pulse );
INTERRUPT_GEN( irq1_line_assert );

INTERRUPT_GEN( irq2_line_hold );
INTERRUPT_GEN( irq2_line_pulse );
INTERRUPT_GEN( irq2_line_assert );

INTERRUPT_GEN( irq3_line_hold );
INTERRUPT_GEN( irq3_line_pulse );
INTERRUPT_GEN( irq3_line_assert );

INTERRUPT_GEN( irq4_line_hold );
INTERRUPT_GEN( irq4_line_pulse );
INTERRUPT_GEN( irq4_line_assert );

INTERRUPT_GEN( irq5_line_hold );
INTERRUPT_GEN( irq5_line_pulse );
INTERRUPT_GEN( irq5_line_assert );

INTERRUPT_GEN( irq6_line_hold );
INTERRUPT_GEN( irq6_line_pulse );
INTERRUPT_GEN( irq6_line_assert );

INTERRUPT_GEN( irq7_line_hold );
INTERRUPT_GEN( irq7_line_pulse );
INTERRUPT_GEN( irq7_line_assert );



/*************************************
 *
 *  Obsolete interrupt handling
 *
 *************************************/

/* OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE */
/* OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE */
/* OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE */

/* Obsolete functions: avoid using them in new drivers, as many of them will
   go away in the future! */

void cpu_interrupt_enable(int cpu,int enabled);
WRITE8_HANDLER( interrupt_enable_w );
WRITE8_HANDLER( interrupt_vector_w );
READ8_HANDLER( interrupt_enable_r );

/* OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE */
/* OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE */
/* OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE OBSOLETE */



#endif	/* __CPUINT_H__ */
