/*************************************************************************
 Preliminary driver for Williams/Midway games using the TMS34010 processor
 This is very much a work in progress.

 sound hardware

**************************************************************************/
#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/m6809/m6809.h"
#include "vidhrdw/generic.h"
#include "machine/6821pia.h"

void smashtv_ym2151_int (int irq)
{
//	if (errorlog) fprintf(errorlog, "ym2151_int\n");
	pia_0_ca1_w (0, irq==0);
	/* pia_0_ca1_w (0, (0)); */
	/* pia_0_ca1_w (0, (1)); */
}
void narc_ym2151_int (int irq)
{
	cpu_set_irq_line(1,1,irq ? ASSERT_LINE : CLEAR_LINE);

	//cpu_cause_interrupt(1,M6809_INT_FIRQ);
//	if (errorlog) fprintf(errorlog, "ym2151_int: ");
//	if (errorlog) fprintf(errorlog, "sound FIRQ\n");
}

void smashtv_snd_cmd_real_w (int param)
{
	pia_0_portb_w (0, param&0xff);
	pia_0_cb1_w (0, (param&0x200?1:0));
	if (!(param&0x100))
	{
		cpu_reset(1);
		if (errorlog) fprintf(errorlog, "sound reset\n");
	}
}
void narc_snd_cmd_real_w (int param)
{
	soundlatch_w (0, param&0xff);
	if (!(param&0x200))
	{
		if (errorlog) fprintf(errorlog, "sound IRQ\n");
		cpu_cause_interrupt(1, M6809_INT_IRQ);
	}
	if (!(param&0x100))
	{
		if (errorlog) fprintf(errorlog, "sound NMI\n");
		cpu_cause_interrupt(1, M6809_INT_NMI);
	}
}
void mk_snd_cmd_real_w (int param)
{
	soundlatch_w (0, param&0xff);
	if (!(param&0x200))
	{
		if (errorlog) fprintf(errorlog, "sound IRQ\n");
		cpu_cause_interrupt(1, M6809_INT_IRQ);
	}
}
void smashtv_sound_w(int offset, int data)
{
	if (errorlog) fprintf(errorlog, "CPU #0 PC %08x: ", cpu_get_pc());
	if (errorlog) fprintf(errorlog, "sound write %x\n", data);
	timer_set (TIME_NOW, data, smashtv_snd_cmd_real_w);
}
void trog_sound_w(int offset, int data)
{
	if (errorlog) fprintf(errorlog, "CPU #0 PC %08x: ", cpu_get_pc());
	if (errorlog) fprintf(errorlog, "sound write %x\n", data|0x100);
	timer_set (TIME_NOW, data|0x0100, smashtv_snd_cmd_real_w);
}
void narc_sound_w(int offset, int data)
{
//	if (errorlog) fprintf(errorlog, "CPU #0 PC %08x: ", cpu_get_pc());
//	if (errorlog) fprintf(errorlog, "sound write %x\n", data);
	timer_set (TIME_NOW, data, narc_snd_cmd_real_w);
}
void mk_sound_w(int offset, int data)
{
//	if (errorlog) fprintf(errorlog, "CPU #0 PC %08x: ", cpu_get_pc());
//	if (errorlog) fprintf(errorlog, "sound write %x\n", data);
	timer_set (TIME_NOW, data, mk_snd_cmd_real_w);
}
void nbajam_sound_w(int offset, int data)
{
	if (errorlog) fprintf(errorlog, "CPU #0 PC %08x: ", cpu_get_pc());
	if (errorlog) fprintf(errorlog, "sound write %x\n", data);
//	timer_set (TIME_NOW, data, mk_snd_cmd_real_w);
}
void mk_sound_talkback_w(int offset, int data)
{
//	cpu_cause_interrupt(0,TMS34010_INT2);
}
int narc_DAC_r(int offset)
{
	return 0;
}
void narc_slave_cmd_real_w (int param)
{
	soundlatch2_w (0, param&0xff);
	cpu_cause_interrupt(2, M6809_INT_FIRQ);
}
void narc_slave_cmd_w(int offset, int data)
{
	timer_set (TIME_NOW, data, narc_slave_cmd_real_w);
}
void narc_slave_DAC_w(int offset, int data)
{
	DAC_data_w(1,data);
}
