#include "m68000.h"
#include "memory.h"
#include <stdio.h>


void MC68000_SetRegs(MC68000_Regs *regs)
{
   m68k_set_context(&regs->regs);
}

void MC68000_GetRegs(MC68000_Regs *regs)
{
   m68k_get_context(&regs->regs);
}



#if NEW_INTERRUPT_SYSTEM

void MC68000_set_nmi_line(int state)
{
   switch(state)
   {
      case CLEAR_LINE:
         m68k_clear_irq(7);
         return;
      case ASSERT_LINE:
         m68k_assert_irq(7);
         return;
      default:
         m68k_assert_irq(7);
         return;
   }
}

void MC68000_set_irq_line(int irqline, int state)
{
   switch(state)
   {
      case CLEAR_LINE:
         m68k_clear_irq(irqline);
         return;
      case ASSERT_LINE:
         m68k_assert_irq(irqline);
         return;
      default:
         m68k_assert_irq(irqline);
         return;
   }
}

#else

void MC68000_ClearPendingInterrupts()
{
   m68k_clear_irq(7);
   m68k_clear_irq(6);
   m68k_clear_irq(5);
   m68k_clear_irq(4);
   m68k_clear_irq(3);
   m68k_clear_irq(2);
   m68k_clear_irq(1);
}

#endif /* NEW_INTERRUPT_SYSTEM */
