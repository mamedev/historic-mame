#include "driver.h"


/*

  The sound CPU run in interrup mode 0. IRQ is shared by two sources: the
  YM2151 (bit 4 of the vector), and the main CPU (bit 5).
  Since the vector can be changed from different contexts (the YM2151 timer
  callback, the main CPU context, and the sound CPU context), it's important
  to accurately arbitrate the changes to avoid out-of-order execution. We do
  that by handling all vector changes in a single timer callback.

*/


enum
{
	VECTOR_INIT,
	YM2151_ASSERT,
	YM2151_CLEAR,
	Z80_ASSERT,
	Z80_CLEAR
};

static void setvector_callback(int param)
{
	static int irqvector;

	switch(param)
	{
		case VECTOR_INIT:
			irqvector = 0xff;
			break;

		case YM2151_ASSERT:
			irqvector &= 0xef;
			break;

		case YM2151_CLEAR:
			irqvector |= 0x10;
			break;

		case Z80_ASSERT:
			irqvector &= 0xdf;
			break;

		case Z80_CLEAR:
			irqvector |= 0x20;
			break;
	}

	cpu_irq_line_vector_w(1,0,irqvector);
	if (irqvector == 0xff)	/* no IRQs pending */
		cpu_set_irq_line(1,0,CLEAR_LINE);
	else	/* IRQ pending */
		cpu_set_irq_line(1,0,ASSERT_LINE);
}

void sichuan2_init_machine(void)
{
	setvector_callback(VECTOR_INIT);
}

void sichuan2_ym2151_irq_handler(int irq)
{
	if (irq)
		timer_set(TIME_NOW,YM2151_ASSERT,setvector_callback);
	else
		timer_set(TIME_NOW,YM2151_CLEAR,setvector_callback);
}

void sichuan2_sound_command_w(int offset,int data)
{
	soundlatch_w(offset,data);
	timer_set(TIME_NOW,Z80_ASSERT,setvector_callback);
}

void sichuan2_sound_irq_ack_w(int offset,int data)
{
	timer_set(TIME_NOW,Z80_CLEAR,setvector_callback);
}



static int sample_addr;

void sichuan2_sample_addr_w(int offset,int data)
{
	sample_addr >>= 2;

	if (offset == 1)
		sample_addr = (sample_addr & 0x00ff) | ((data << 8) & 0xff00);
	else
		sample_addr = (sample_addr & 0xff00) | ((data << 0) & 0x00ff);

	sample_addr <<= 2;
}

int sichuan2_sample_r(int offset)
{
	return Machine->memory_region[3][sample_addr];
}

void sichuan2_sample_w(int offset,int data)
{
	DAC_signed_data_w(0,data);
	sample_addr = (sample_addr + 1) & 0x3ffff;
}
