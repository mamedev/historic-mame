/*************************************************************************

  Gauntlet machine hardware

*************************************************************************/

#include "sndhrdw/5220intf.h"
#include "vidhrdw/generic.h"
#include "m6502/m6502.h"

void slapstic_init (int chip);
int slapstic_bank (void);
int slapstic_tweak (int offset);



/*************************************
 *
 *		Globals we own
 *
 *************************************/

unsigned char *gauntlet_eeprom;
unsigned char *gauntlet_slapstic_base;
unsigned char *gauntlet_speed_check;



/*************************************
 *
 *		Statics
 *
 *************************************/

static int cpu_to_sound, cpu_to_sound_ready;
static int sound_to_cpu, sound_to_cpu_ready;
static int unlocked;
static int last_speed_check;

static void *comm_timer;
static void *stop_comm_timer;

static int speech_val;
static int last_speech_write;



/*************************************
 *
 *		Initialization of globals.
 *
 *************************************/

static void generic_init_machine (void)
{
	unlocked = 0;
	cpu_to_sound = cpu_to_sound_ready = 0;
	sound_to_cpu = sound_to_cpu_ready = 0;
	last_speed_check = 0;

	last_speech_write = 0x80;
	comm_timer = stop_comm_timer = NULL;
}


void gauntlet_init_machine (void)
{
	generic_init_machine ();
	slapstic_init (104);
}


void gaunt2p_init_machine (void)
{
	generic_init_machine ();
	slapstic_init (107);
}


void gauntlet2_init_machine (void)
{
	generic_init_machine ();
	slapstic_init (106);
}



/*************************************
 *
 *		EEPROM read/write/enable
 *
 *************************************/

void gauntlet_eeprom_enable_w (int offset, int data)
{
	unlocked = 1;
}


int gauntlet_eeprom_r (int offset)
{
	return READ_WORD (&gauntlet_eeprom[offset]) & 0xff;
}


void gauntlet_eeprom_w (int offset, int data)
{
	if (!unlocked)
		return;

	COMBINE_WORD_MEM (&gauntlet_eeprom[offset], data);
	unlocked = 0;
}



/*************************************
 *
 *		Slapstic ROM read/write.
 *
 *************************************/

int gauntlet_slapstic_r (int offset)
{
	int bank = slapstic_tweak (offset / 2) * 0x2000;
	return READ_WORD (&gauntlet_slapstic_base[bank + (offset & 0x1fff)]);
}


void gauntlet_slapstic_w (int offset, int data)
{
	slapstic_tweak (offset / 2);
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
		case 0:
			return readinputport (p1);
		case 2:
			return readinputport ((p1 != 1) ? 1 : 0);
		case 4:
			return readinputport ((p1 != 2) ? 2 : 0);
		case 6:
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
		case 0:
			temp = input_port_5_r (offset);
			if (cpu_to_sound_ready) temp |= 0x20;
			if (sound_to_cpu_ready) temp |= 0x10;
			return temp;

		case 6:
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
	if (input_port_5_r (offset) & 0x08) temp |= 0x10;

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
		case 0x0e:		/* sound CPU reset */
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
 *		Sound TMS5220 write.
 *
 *************************************/

void gauntlet_tms_w (int offset, int data)
{
	speech_val = data;
}



/*************************************
 *
 *		Sound control write.
 *
 *************************************/

void gauntlet_sound_ctl_w (int offset, int data)
{
	switch (offset & 7)
	{
		case 0:	/* music reset, bit D7, low reset */
			break;

		case 1:	/* speech write, bit D7, active low */
			if (((data ^ last_speech_write) & 0x80) && (data & 0x80))
				tms5220_data_w (0, speech_val);
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
 *		Main CPU to sound CPU communications
 *
 *************************************/

void gauntlet_stop_comm_timer (int param)
{
	if (comm_timer)
		timer_remove (comm_timer);
	comm_timer = stop_comm_timer = NULL;
}


void gauntlet_delayed_sound_w (int param)
{
	if (cpu_to_sound_ready)
		if (errorlog) fprintf (errorlog, "Missed command from 68010\n");

	cpu_to_sound = param;
	cpu_to_sound_ready = 1;
	cpu_cause_interrupt (1, INT_NMI);

	/* allocate a high frequency timer until a response is generated */
	/* the main CPU is *very* sensistive to the timing of the response */
	if (!comm_timer)
		comm_timer = timer_pulse (TIME_IN_USEC (50), 0, 0);
	if (stop_comm_timer)
		timer_remove (stop_comm_timer);
	stop_comm_timer = timer_set (TIME_IN_USEC (1000), 0, gauntlet_stop_comm_timer);
}


void gauntlet_sound_w (int offset, int data)
{
	/* use a timer to force a resynchronization */
	timer_set (TIME_NOW, data & 0xff, gauntlet_delayed_sound_w);
}


int gauntlet_6502_sound_r (int offset)
{
	cpu_to_sound_ready = 0;
	return cpu_to_sound;
}



/*************************************
 *
 *		Sound CPU to main CPU communications
 *
 *************************************/

void gauntlet_delayed_6502_sound_w (int param)
{
	if (sound_to_cpu_ready)
		if (errorlog) fprintf (errorlog, "Missed result from 6502\n");

	sound_to_cpu = param;
	sound_to_cpu_ready = 1;
	cpu_cause_interrupt (0, 6);

	/* remove the high frequency timer if there is one */
	if (comm_timer)
		timer_remove (comm_timer);
	comm_timer = NULL;
}


void gauntlet_6502_sound_w (int offset, int data)
{
	/* use a timer to force a resynchronization */
	timer_set (TIME_NOW, data, gauntlet_delayed_6502_sound_w);
}



/*************************************
 *
 *		Speed cheats
 *
 *************************************/

int gauntlet_68010_speedup_r (int offset)
{
	int result = READ_WORD (&gauntlet_speed_check[offset]);

	if (offset == 2)
	{
		int time = cpu_gettotalcycles ();
		int delta = time - last_speed_check;

		last_speed_check = time;
		if (delta <= 100 && result == 0 && delta >= 0)
			cpu_spin ();
	}

	return result;
}


void gauntlet_68010_speedup_w (int offset, int data)
{
	if (offset == 2)
		last_speed_check -= 1000;
	COMBINE_WORD_MEM (&gauntlet_speed_check[offset], data);
}


int gauntlet_6502_speedup_r (int offset)
{
	int result = RAM[0x0211];
	if (cpu_getpreviouspc() == 0x412a && RAM[0x0211] == RAM[0x0210] && RAM[0x0225] == RAM[0x0224])
		cpu_spin ();
	return result;
}
