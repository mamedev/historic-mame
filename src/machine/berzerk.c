/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "z80/z80.h"

static int irq_enabled;
static int nmi_enabled;
int berzerk_irq_end_of_screen;

static int_count;
static int led_count;
void berzerk_init_machine()
{
  int i;

  /* Machine expects locations 3800-3fff to be ff */
  for (i = 0x3800; i < 0x4000; i++)
     Machine->memory_region[0][i] = 0xff;

  irq_enabled = 0;
  nmi_enabled = 0;
  berzerk_irq_end_of_screen = 0;
  int_count = 0;
  led_count = 0;

}


void berzerk_interrupt_enable_w(int offset,int data)
{
  irq_enabled = data;
}

void berzerk_enable_nmi_w(int offset,int data)
{
  nmi_enabled = 1;
}

void berzerk_disable_nmi_w(int offset,int data)
{
  nmi_enabled = 0;
}

int berzerk_enable_nmi_r(int offset)
{
  nmi_enabled = 1;
  return 0;
}

int berzerk_disable_nmi_r(int offset)
{
  nmi_enabled = 0;
  return 0;
}

int berzerk_led_on_w(int offset)
{
   led_count++;
   if (errorlog)
      fprintf(errorlog,"LED On %i\n",led_count);

   return 0;
}

int berzerk_led_off_w(int offset)
{
   if (errorlog)
      fprintf(errorlog,"LED Off\n");

   return 0;
}

int berzerk_interrupt()
{
  int_count++;

  switch (int_count)
  {
  case 4:
  case 8:
    if (int_count == 8)
    {
      berzerk_irq_end_of_screen = 0;
      int_count = 0;
    }
    else
    {
      berzerk_irq_end_of_screen = 1;
    }
    return irq_enabled ? 0xfc : Z80_IGNORE_INT;

  case 1:
  case 2:
  case 3:
  case 5:
  case 6:
  case 7:
    if (int_count == 7)
    {
      berzerk_irq_end_of_screen = 1;
    }
    else
    {
      berzerk_irq_end_of_screen = 0;
    }
    return nmi_enabled ? Z80_NMI_INT : Z80_IGNORE_INT;
  }

  /* To satisfy the compiler */
  return Z80_IGNORE_INT;
}

