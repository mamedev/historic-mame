/*************************************************************************

  Gauntlet machine hardware

*************************************************************************/

#include "machine/atarigen.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"

void gauntlet_scanline_update(int scanline);



/*************************************
 *
 *		Globals we own
 *
 *************************************/

unsigned char *gauntlet_speed_check;



/*************************************
 *
 *		Statics
 *
 *************************************/

static int last_speed_check;

static int speech_val;
static int last_speech_write;



/*************************************
 *
 *		Initialization of globals.
 *
 *************************************/

static void gauntlet_update_interrupts(int vblank, int sound)
{
	int newstate = 0;

	if (vblank)
		newstate |= 4;
	if (sound)
		newstate |= 6;

	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


void gauntlet_init_machine(void)
{
	last_speed_check = 0;
	last_speech_write = 0x80;
	
	atarigen_eeprom_reset();
	atarigen_slapstic_reset();
	atarigen_interrupt_init(gauntlet_update_interrupts, gauntlet_scanline_update);
}



/*************************************
 *
 *		Controller read dispatch.
 *
 *************************************/

static int fake_inputs(int real_port, int fake_port)
{
	int result = readinputport(real_port);
	int fake = readinputport(fake_port);

	if (fake & 0x01)			/* up */
	{
		if (fake & 0x04)		/* up and left */
			result &= ~0x20;
		else if (fake & 0x08)	/* up and right */
			result &= ~0x10;
		else					/* up only */
			result &= ~0x30;
	}
	else if (fake & 0x02)		/* down */
	{
		if (fake & 0x04)		/* down and left */
			result &= ~0x80;
		else if (fake & 0x08)	/* down and right */
			result &= ~0x40;
		else					/* down only */
			result &= ~0xc0;
	}
	else if (fake & 0x04)		/* left only */
		result &= ~0x60;
	else if (fake & 0x08)		/* right only */
		result &= ~0x90;

	return result;
}


int gauntlet_control_r(int offset)
{
	/* differentiate Gauntlet input from Vindicators 2 inputs via the slapstic */
	if (atarigen_slapstic_num != 118)
	{
		/* Gauntlet case */
		int p1 = input_port_6_r(offset);
		switch (offset)
		{
			case 0:
				return readinputport(p1);
			case 2:
				return readinputport((p1 != 1) ? 1 : 0);
			case 4:
				return readinputport((p1 != 2) ? 2 : 0);
			case 6:
				return readinputport((p1 != 3) ? 3 : 0);
		}
	}
	else
	{
		/* Vindicators 2 case */
		switch (offset)
		{
			case 0:
				return fake_inputs(0, 6);
			case 2:
				return fake_inputs(1, 7);
			case 4:
			case 6:
				return readinputport(offset / 2);
		}
	}
	return 0xffff;
}



/*************************************
 *
 *		I/O read dispatch.
 *
 *************************************/

int gauntlet_io_r(int offset)
{
	int temp;

	switch (offset)
	{
		case 0:
			temp = input_port_5_r(offset);
			if (atarigen_cpu_to_sound_ready) temp ^= 0x0020;
			if (atarigen_sound_to_cpu_ready) temp ^= 0x0010;
			return temp;

		case 6:
			return atarigen_sound_r(0);
	}
	return 0xffff;
}


int gauntlet_6502_switch_r(int offset)
{
	int temp = 0x30;

	if (atarigen_cpu_to_sound_ready) temp ^= 0x80;
	if (atarigen_sound_to_cpu_ready) temp ^= 0x40;
	if (tms5220_ready_r()) temp ^= 0x20;
	if (!(input_port_5_r(offset) & 0x0008)) temp ^= 0x10;

	return temp;
}



/*************************************
 *
 *		Controller write dispatch.
 *
 *************************************/

void gauntlet_io_w(int offset, int data)
{
	switch (offset)
	{
		case 0x0e:		/* sound CPU reset */
			if (data & 1)
				atarigen_sound_reset_w(0, 0);
			else
				cpu_halt(1, 0);
			break;
	}
}



/*************************************
 *
 *		Sound TMS5220 write.
 *
 *************************************/

void gauntlet_tms_w(int offset, int data)
{
	speech_val = data;
}



/*************************************
 *
 *		Sound control write.
 *
 *************************************/

void gauntlet_sound_ctl_w(int offset, int data)
{
	switch (offset & 7)
	{
		case 0:	/* music reset, bit D7, low reset */
			break;

		case 1:	/* speech write, bit D7, active low */
			if (((data ^ last_speech_write) & 0x80) && (data & 0x80))
				tms5220_data_w(0, speech_val);
			last_speech_write = data;
			break;

		case 2:	/* speech reset, bit D7, active low */
			break;

		case 3:	/* speech squeak, bit D7, low = 650kHz clock */
			break;
	}
}



/*************************************
 *
 *		Sound mixer write.
 *
 *************************************/

void gauntlet_mixer_w(int offset, int data)
{
	atarigen_set_ym2151_vol((data & 7) * 100 / 7);
	atarigen_set_pokey_vol(((data >> 3) & 3) * 100 / 3);
	atarigen_set_tms5220_vol(((data >> 5) & 7) * 100 / 7);
}



/*************************************
 *
 *		Speed cheats
 *
 *************************************/

int gauntlet_68010_speedup_r(int offset)
{
	int result = READ_WORD(&gauntlet_speed_check[offset]);
	int time = cpu_gettotalcycles();
	int delta = time - last_speed_check;

	last_speed_check = time;
	if (delta <= 100 && result == 0 && delta >= 0)
		cpu_spin();

	return result;
}


void gauntlet_68010_speedup_w(int offset, int data)
{
	last_speed_check -= 1000;
	COMBINE_WORD_MEM(&gauntlet_speed_check[offset], data);
}
