#include "m68000.h"

/* The new interrupt system in MAME doesn't really apply to the 68000, so I've placed them in here */
#if M68000_BUILDING_MAME

#if NEW_INTERRUPT_SYSTEM

static int (*m68000_irq_callback)(int irqline);

int m68000_interrupt_acknowledge(int int_level)
{
	return (*m68000_irq_callback)(int_level);
}

void MC68000_SetRegs(MC68000_Regs *regs)
{
   m68000_set_context(&regs->regs);
   m68000_irq_callback = regs->irq_callback;
}

void MC68000_GetRegs(MC68000_Regs *regs)
{
   m68000_get_context(&regs->regs);
   regs->irq_callback = m68000_irq_callback;
}

void MC68000_set_nmi_line(int state)
{
   /* does not apply */
}

void MC68000_set_irq_line(int irqline, int state)
{
   if (state != CLEAR_LINE)
	  m68000_pulse_irq(irqline);
}

void MC68000_set_irq_callback(int (*callback)(int irqline))
{
   m68000_irq_callback = callback;
}

#else

void MC68000_ClearPendingInterrupts()
{
   /* does not apply */
}

#endif /* NEW_INTERRUPT_SYSTEM */


#endif /* M68000_BUILDING_MAME */
