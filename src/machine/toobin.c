/*************************************************************************

  Toobin' machine hardware

*************************************************************************/

#include "sndhrdw/5220intf.h"
#include "vidhrdw/generic.h"
#include "m6502/m6502.h"
#include "m68000/m68000.h"


/* approximately 440 scan lines: 384 visible plus the VBLANK time */
#define TOOBIN_VISIBLE_SCAN_LINES 384
#define TOOBIN_VBLANK_SCAN_LINES 56
#define TOOBIN_SCAN_LINES (TOOBIN_VISIBLE_SCAN_LINES + TOOBIN_VBLANK_SCAN_LINES)

#define TOOBIN_SCAN_LINE_TIME (TIME_IN_HZ (Machine->drv->frames_per_second * TOOBIN_SCAN_LINES))


extern void toobin_begin_frame (int param);



/*************************************
 *
 *		Globals we own
 *
 *************************************/

unsigned char *toobin_eeprom;
unsigned char *toobin_interrupt_scan;



/*************************************
 *
 *		Statics
 *
 *************************************/

static int cpu_to_sound, cpu_to_sound_ready;
static int sound_to_cpu, sound_to_cpu_ready;
static int unlocked;
static int interrupt_acked;

static void *interrupt_timer;
static void *comm_timer;
static void *stop_comm_timer;



/*************************************
 *
 *		Initialization of globals.
 *
 *************************************/

void toobin_init_machine(void)
{
	unlocked = 0;
	cpu_to_sound = cpu_to_sound_ready = 0;
	sound_to_cpu = sound_to_cpu_ready = 0;
	interrupt_acked = 1;
	interrupt_timer = 0;
	comm_timer = stop_comm_timer = NULL;
}



/*************************************
 *
 *		EEPROM read/write/enable.
 *
 *************************************/

void toobin_eeprom_enable_w (int offset, int data)
{
	unlocked = 1;
}


int toobin_eeprom_r (int offset)
{
	return READ_WORD (&toobin_eeprom[offset]) & 0xff;
}


void toobin_eeprom_w (int offset, int data)
{
	if (!unlocked)
		return;

	COMBINE_WORD_MEM (&toobin_eeprom[offset], data);
	unlocked = 0;
}



/*************************************
 *
 *		Scanline computation
 *
 *************************************/

int toobin_scanline (void)
{
	return cpu_scalebyfcount (TOOBIN_SCAN_LINES);
}



/*************************************
 *
 *		Interrupt handlers.
 *
 *************************************/

void toobin_interrupt (void)
{
	/* set a timer to go off around scanline 0 so that we have an appropriate update */
	timer_set (TOOBIN_SCAN_LINE_TIME * (double)(TOOBIN_VBLANK_SCAN_LINES - 1), 0, toobin_begin_frame);
}


void toobin_interrupt_callback (int param)
{
	/* generate the interrupt */
	if (interrupt_acked)
		cpu_cause_interrupt (0, 1);
	interrupt_acked = 0;

	/* set a new timer to go off at the same scan line next frame */
	interrupt_timer = timer_set (TIME_IN_HZ (Machine->drv->frames_per_second), 0, toobin_interrupt_callback);
}


void toobin_interrupt_scan_w (int offset, int data)
{
	int oldword = READ_WORD (&toobin_interrupt_scan[offset]);
	int newword = COMBINE_WORD (oldword, data);

	/* if something changed, update the word in memory */
	if (oldword != newword)
	{
		WRITE_WORD (&toobin_interrupt_scan[offset], newword);

		/* if this is offset 0, modify the timer */
		if (!offset)
		{
			int delta = (newword & 0x3ff) - toobin_scanline ();

			/* compute the delta between the new scanline and the current one */
			if (delta < 0)
				delta += TOOBIN_SCAN_LINES;

			/* remove any previous timer and set a new one */
			if (interrupt_timer)
				timer_remove (interrupt_timer);
			interrupt_timer = timer_set (TOOBIN_SCAN_LINE_TIME * (double)delta, 0, toobin_interrupt_callback);
		}
	}
}


void toobin_interrupt_ack_w (int offset, int data)
{
	interrupt_acked = 1;
}


int toobin_sound_interrupt (void)
{
	return INT_IRQ;
}



/*************************************
 *
 *		I/O read dispatch.
 *
 *************************************/

int toobin_io_r (int offset)
{
	int result = input_port_3_r (offset) << 8;

	if (cpu_to_sound_ready) result ^= 0x2000;

	return result | 0xff;
}


int toobin_6502_switch_r (int offset)
{
	int result = input_port_2_r (offset);

	if (!(input_port_3_r (offset) & 0x10)) result ^= 0x80;
	if (sound_to_cpu_ready) result ^= 0x20;
/*	if (cpu_to_sound_ready) result ^= 0x10; -- a guess for now, best not to mess with it */

	return result;
}



/*************************************
 *
 *		Controls read
 *
 *************************************/

int toobin_controls_r (int offset)
{
	return (input_port_0_r (offset) << 8) | input_port_1_r (offset);
}



/*************************************
 *
 *		Sound chip reset & bankswitching
 *
 *************************************/

void toobin_sound_reset_w (int offset, int data)
{
	cpu_reset (1);
	cpu_halt (1, 1);
	cpu_to_sound_ready = sound_to_cpu_ready = 0;
}


void toobin_6502_bank_w (int offset, int data)
{
	cpu_setbank (8, &RAM[0x10000 + 0x1000 * ((data >> 6) & 3)]);
}



/*************************************
 *
 *		Main CPU to sound CPU communications
 *
 *************************************/

void toobin_stop_comm_timer (int param)
{
	if (comm_timer)
		timer_remove (comm_timer);
	comm_timer = stop_comm_timer = NULL;
}


void toobin_delayed_sound_w (int param)
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
	stop_comm_timer = timer_set (TIME_IN_USEC (1000), 0, toobin_stop_comm_timer);
}


void toobin_sound_w (int offset, int data)
{
	/* use a timer to force a resynchronization */
	timer_set (TIME_NOW, data & 0xff, toobin_delayed_sound_w);
}


int toobin_6502_sound_r (int offset)
{
	cpu_to_sound_ready = 0;
	return cpu_to_sound;
}



/*************************************
 *
 *		Sound CPU to main CPU communications
 *
 *************************************/

void toobin_delayed_6502_sound_w (int param)
{
	if (sound_to_cpu_ready)
		if (errorlog) fprintf (errorlog, "Missed result from 6502\n");

	sound_to_cpu = param;
	sound_to_cpu_ready = 1;
	cpu_cause_interrupt (0, 2);

	/* remove the high frequency timer if there is one */
	if (comm_timer)
		timer_remove (comm_timer);
	comm_timer = NULL;
}


void toobin_6502_sound_w (int offset, int data)
{
	/* use a timer to force a resynchronization */
	timer_set (TIME_NOW, data, toobin_delayed_6502_sound_w);
}


int toobin_sound_r (int offset)
{
	sound_to_cpu_ready = 0;
	return 0xff00 | sound_to_cpu;
}



/*************************************
 *
 *		Speed cheats
 *
 *************************************/



