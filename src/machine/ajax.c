/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "cpu/konami/konami.h"
#include "vidhrdw/konamiic.h"

unsigned char *ajax_sharedram;
extern unsigned char ajax_priority;
static int firq_enable;

int ajax_sharedram_r( int offset )
{
	return ajax_sharedram[offset];
}

void ajax_sharedram_w( int offset, int data )
{
	ajax_sharedram[offset] = data;
}

void ajax_bankswitch_w( int offset, int data )
{
	/* b7 ROM N12/N11 */
	/* b6 coin counter 2 */
	/* b5 coin counter 1 */
	/* b4 reset slave cpus */
	/* b3 layer priority selector */
	/* b2-b0 bank # */
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];
	int bankaddress;

	bankaddress = 0;

	ajax_priority = data & 0x08;

	if (!(data & 0x80))	bankaddress += 0x8000;
	bankaddress += 0x10000 + (data & 0x07)*0x2000;
	cpu_setbank(2,&RAM[bankaddress]);

	coin_counter_w(0,data & 0x20);
	coin_counter_w(1,data & 0x40);
}

void ajax_bankswitch_w_2( int offset, int data )
{
	/* b6 enable char ROM reading through the video RAM */
	/* b4 FIRQ enable */
	/* b3-b0 bank # (ROMS G16 and I16) */
	/* other bits unknown */
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[1].memory_region];
	int bankaddress;

	firq_enable = (data & 0x10) >> 4;

	bankaddress = 0x10000 + (data & 0x0f)*0x2000;
	cpu_setbank(1,&RAM[bankaddress]);

	K052109_set_RMRD_line((data & 0x40) ? ASSERT_LINE : CLEAR_LINE);
}

void ajax_init_machine( void )
{
	firq_enable = 1;
}

int ajax_interrupt( void )
{
	if (K051960_is_IRQ_enabled())
		return KONAMI_INT_IRQ;
	else
		return ignore_interrupt();
}

int ajax_interrupt_2( void )
{
	if (firq_enable)
		return M6809_INT_FIRQ;
	else
		return ignore_interrupt();
}

void ajax_sh_irqtrigger_w(int offset, int data)
{
	soundlatch_w(offset,data);
	cpu_cause_interrupt(2,0xff);
}
