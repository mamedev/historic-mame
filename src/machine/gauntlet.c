/*************************************************************************

  Gauntlet machine hardware

*************************************************************************/

#include "sndhrdw/5220intf.h"
#include "vidhrdw/generic.h"
#include "m6502/m6502.h"


/*************************************
 *
 *		Globals we own
 *
 *************************************/

int gauntlet_bank1_size;
int gauntlet_bank2_size;
int gauntlet_bank3_size;
int gauntlet_eeprom_size;

unsigned char *gauntlet_bank1;
unsigned char *gauntlet_bank2;
unsigned char *gauntlet_bank3;
unsigned char *gauntlet_eeprom;
unsigned char *gauntlet_slapstic_base;


/*************************************
 *
 *		Statics
 *
 *************************************/

static int cpu_to_sound, cpu_to_sound_ready;
static int sound_to_cpu, sound_to_cpu_ready;
static int unlocked;


/*************************************
 *
 *		Actually called by the video system to initialize memory regions.
 *
 *************************************/

int gauntlet_system_start (void)
{
	unsigned long *p1, *p2, temp;
	int i;

	/* swap the top and bottom halves of the main CPU ROM images */
	p1 = (unsigned long *)&Machine->memory_region[0][0x000000];
	p2 = (unsigned long *)&Machine->memory_region[0][0x008000];
	for (i = 0; i < 0x8000 / 4; i++)
		temp = *p1, *p1++ = *p2, *p2++ = temp;
	p1 = (unsigned long *)&Machine->memory_region[0][0x040000];
	p2 = (unsigned long *)&Machine->memory_region[0][0x048000];
	for (i = 0; i < 0x8000 / 4; i++)
		temp = *p1, *p1++ = *p2, *p2++ = temp;
	p1 = (unsigned long *)&Machine->memory_region[0][0x050000];
	p2 = (unsigned long *)&Machine->memory_region[0][0x058000];
	for (i = 0; i < 0x8000 / 4; i++)
		temp = *p1, *p1++ = *p2, *p2++ = temp;
	p1 = (unsigned long *)&Machine->memory_region[0][0x060000];
	p2 = (unsigned long *)&Machine->memory_region[0][0x068000];
	for (i = 0; i < 0x8000 / 4; i++)
		temp = *p1, *p1++ = *p2, *p2++ = temp;
	p1 = (unsigned long *)&Machine->memory_region[0][0x070000];
	p2 = (unsigned long *)&Machine->memory_region[0][0x078000];
	for (i = 0; i < 0x8000 / 4; i++)
		temp = *p1, *p1++ = *p2, *p2++ = temp;

	/* allocate the RAM banks */
	if (!gauntlet_bank1)
		gauntlet_bank1 = calloc (gauntlet_bank1_size + gauntlet_bank2_size +
		                         gauntlet_bank3_size + gauntlet_eeprom_size, 1);
	if (!gauntlet_bank1)
		return 1;
	gauntlet_bank2 = gauntlet_bank1 + gauntlet_bank1_size;
	gauntlet_bank3 = gauntlet_bank2 + gauntlet_bank2_size;
	gauntlet_eeprom = gauntlet_bank3 + gauntlet_bank3_size;

	/* point to the generic RAM banks */
	cpu_setbank (1, gauntlet_bank1);
	cpu_setbank (2, gauntlet_bank2);
	cpu_setbank (3, gauntlet_bank3);

	unlocked = 0;

	return 0;
}


/*************************************
 *
 *		Actually called by the video system to free memory regions.
 *
 *************************************/

int gauntlet_system_stop (void)
{
	/* free the banks we allocated */
	if (gauntlet_bank1)
		free (gauntlet_bank1);
	gauntlet_bank1 = gauntlet_bank2 = gauntlet_bank3 = gauntlet_eeprom = 0;

	return 0;
}


/*************************************
 *
 *		EEPROM read/write.
 *
 *************************************/

int gauntlet_eeprom_r (int offset)
{
	if (!(offset & 1))
		return 0;
	return gauntlet_eeprom[offset];
}

void gauntlet_eeprom_w (int offset, int data)
{
	if (!(offset & 1))
		return;
	if (!unlocked)
		return;
	gauntlet_eeprom[offset] = data;
	unlocked = 0;
}


/*************************************
 *
 *		Slapstic ROM read/write.
 *
 *************************************/

int gauntlet_slapstic_r (int offset)
{
	if (errorlog) fprintf (errorlog, "Slapstic read offset %04X\n", offset);

	/* Warning: this is not a real slapstic decode function; it just emulates the effects of the
	   hacked ROMs so that we can run with the standard set */

/*
	if (slapstic_active && offset < 0x160 && !(offset & 1))
	{
		if ((offset >= 0x054 && offset <= 0x056) ||
		    (offset >= 0x062 && offset <= 0x07c) ||
		    (offset >= 0x08a && offset <= 0x0a6))
			return gauntlet_slapstic_base[offset] | 0x20;
		if ((offset >= 0x058 && offset <= 0x060) ||
		    (offset == 0x0a8) ||
		    (offset >= 0x0b4 && offset <= 0x0d2) ||
		    (offset >= 0x0e0 && offset <= 0x0fc))
			return gauntlet_slapstic_base[offset] | 0x40;
		if ((offset >= 0x07e && offset <= 0x088) ||
		    (offset >= 0x0aa && offset <= 0x0b2) ||
		    (offset >= 0x0d4 && offset <= 0x0de) ||
		    (offset >= 0x146 && offset <= 0x15e))
			return gauntlet_slapstic_base[offset] | 0x60;
	}
*/

	return gauntlet_slapstic_base[offset];
}

void gauntlet_slapstic_w (int offset, int data)
{
	if (errorlog) fprintf (errorlog, "Slapstic write offset %04X\n", offset);
}

int gauntlet2_slapstic_r (int offset)
{
	if (errorlog) fprintf (errorlog, "Slapstic read offset %04X\n", offset);

	/* Warning: this is not a real slapstic decode function; it just emulates the effects of the
	   hacked ROMs so that we can run with the standard set */

	return gauntlet_slapstic_base[offset];
}

void gauntlet2_slapstic_w (int offset, int data)
{
	if (errorlog) fprintf (errorlog, "Slapstic write offset %04X\n", offset);
}


/*************************************
 *
 *		Interrupt handlers.
 *
 *************************************/

int gauntlet_interrupt(void)
{
	return 4;       /* Interrupt vector 4, used by VBlank */
}


int gauntlet_sound_interrupt(void)
{
	return INT_IRQ;
}


/*************************************
 *
 *		Controller read dispatch.
 *
 *************************************/

int gauntlet_control_r (int offset)
{
	int p1 = input_port_6_r (offset);
	switch (offset)
	{
		case 1:
			return readinputport (p1);
		case 3:
			return readinputport ((p1 != 1) ? 1 : 0);
		case 5:
			return readinputport ((p1 != 2) ? 2 : 0);
		case 7:
			return readinputport ((p1 != 3) ? 3 : 0);
	}
	return 0;
}


/*************************************
 *
 *		I/O read dispatch.
 *
 *************************************/

int gauntlet_io_r (int offset)
{
	int temp;

	switch (offset)
	{
		case 1:
			temp = input_port_5_r (offset);
			if (cpu_to_sound_ready) temp |= 0x20;
			if (sound_to_cpu_ready) temp |= 0x10;
			return temp;

		case 7:
			sound_to_cpu_ready = 0;
			return sound_to_cpu;
	}
	return 0;
}


int gauntlet_6502_switch_r (int offset)
{
	int temp = 0;

	if (cpu_to_sound_ready) temp |= 0x80;
	if (sound_to_cpu_ready) temp |= 0x40;
	if (!tms5220_ready_r ()) temp |= 0x20;
	temp |= 0x10;

	return temp;
}


/*************************************
 *
 *		Controller write dispatch.
 *
 *************************************/

void gauntlet_io_w (int offset, int data)
{
	switch (offset)
	{
		case 0x0f:		/* sound CPU reset */
			if (data & 1)
				cpu_halt (1, 1);
			else
			{
				cpu_halt (1, 0);
				cpu_reset (1);
			}
			break;
	}
}


/*************************************
 *
 *		EEPROM enable.
 *
 *************************************/

void gauntlet_eeprom_enable_w (int offset, int data)
{
	unlocked = 1;
}


/*************************************
 *
 *		I/O between main CPU and sound CPU.
 *
 *************************************/

void gauntlet_sound_w (int offset, int data)
{
	if (offset == 1)
	{
		cpu_to_sound = data;
		cpu_to_sound_ready = 1;
		cpu_cause_interrupt (1, INT_NMI);
	}
}


void gauntlet_6502_sound_w (int offset, int data)
{
	sound_to_cpu = data;
	sound_to_cpu_ready = 1;
	cpu_cause_interrupt (0, 6);
}


int gauntlet_6502_sound_r (int offset)
{
	cpu_to_sound_ready = 0;
	return cpu_to_sound;
}


/*************************************
 *
 *		Speed cheats
 *
 *************************************/

static int last_speed_check;
static unsigned char speed_check_ram[4];

int gauntlet_68010_speedup_r (int offset)
{
	int result = speed_check_ram[offset];

	if (offset == 3)
	{
		int time = cpu_getfcount ();
		int delta = last_speed_check - time;

		last_speed_check = time;
		if (delta >= 0 && delta <= 100 && result == 0 && speed_check_ram[2] == 0)
			cpu_seticount (0);
	}

	return result;
}


void gauntlet_68010_speedup_w (int offset, int data)
{
	if (offset & 2)
		last_speed_check = -10000;
	speed_check_ram[offset] = data;
}


int gauntlet_6502_speedup_r (int offset)
{
	int result = RAM[0x0211];
	if (cpu_getpreviouspc() == 0x412a && RAM[0x0211] == RAM[0x0210] && RAM[0x0225] == RAM[0x0224])
		cpu_seticount (0);
	return result;
}
