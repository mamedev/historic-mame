/*************************************************************************

  Atari System 1 machine hardware

*************************************************************************/

#include "atarigen.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"
#include "cpu/m68000/m68000.h"


int slapstic_tweak(int offset);

void atarisys1_scanline_update(int scanline);



/*************************************
 *
 *		Globals we own
 *
 *************************************/

unsigned char *marble_speedcheck;

int atarisys1_int3_state;

int atarisys1_joystick_type;
int atarisys1_trackball_type;



/*************************************
 *
 *		Statics
 *
 *************************************/

static void *joystick_timer;
static int joystick_int;
static int joystick_int_enable;
static int joystick_value;

static int pedal_value;

static int m6522_ddra, m6522_ddrb;
static int m6522_dra, m6522_drb;
static unsigned char m6522_regs[16];

static unsigned long speedcheck_time1, speedcheck_time2;

static int indytemp_setopbase(int pc);


/*************************************
 *
 *		Initialization of globals.
 *
 *************************************/

static int enable;
static void update_interrupts(int vblank, int sound)
{
	int newstate = 0;

if (vblank) enable = osd_key_pressed(OSD_KEY_CAPSLOCK);

	/* all interrupts go through an LS148, which gives priority to the highest */
	if (joystick_int && joystick_int_enable)
		newstate = 2;
	if (atarisys1_int3_state)
		newstate = 3;
	if (vblank)
		newstate = 4;
	if (sound)
		newstate = 6;

	/* set the new state of the IRQ lines */
	if (newstate)
		cpu_set_irq_line(0, newstate, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 7, CLEAR_LINE);
}


void atarisys1_init_machine(void)
{
	/* initialize the system */
	atarigen_eeprom_reset();
	atarigen_slapstic_reset();
	atarigen_interrupt_init(update_interrupts, atarisys1_scanline_update);

	/* special case for the Indiana Jones slapstic */
	if (atarigen_slapstic_num == 105)
		cpu_setOPbaseoverride(indytemp_setopbase);

	atarisys1_int3_state = 0;

	/* reset the joystick parameters */
	joystick_value = 0;
	joystick_timer = NULL;
	joystick_int = 0;
	joystick_int_enable = 0;

	/* reset the 6522 controller */
	m6522_ddra = m6522_ddrb = 0xff;
	m6522_dra = m6522_drb = 0xff;
	memset(m6522_regs, 0xff, sizeof(m6522_regs));

	/* reset the Marble Madness speedup checks */
	speedcheck_time1 = speedcheck_time2 = 0;
}



/*************************************
 *
 *		LED handlers.
 *
 *************************************/

void atarisys1_led_w(int offset, int data)
{
	osd_led_w(offset, ~data & 1);
}



/*************************************
 *
 *		Opcode memory catcher.
 *
 *************************************/

static int indytemp_setopbase(int pc)
{
	int prevpc = cpu_getpreviouspc();

	/*
	 *	This is a slightly ugly kludge for Indiana Jones & the Temple of Doom because it jumps
	 *	directly to code in the slapstic.  The general order of things is this:
	 *
	 *		jump to $3A, which turns off interrupts and jumps to $00 (the reset address)
	 *		look up the request in a table and jump there
	 *		(under some circumstances, tweak the special addresses)
	 *		return via an RTS at the real bankswitch address
	 *
	 *	To simulate this, we tweak the slapstic reset address on entry into slapstic code; then
	 *	we let the system tweak whatever other addresses it wishes.  On exit, we tweak the
	 *	address of the previous PC, which is the RTS instruction, thereby completing the
	 *	bankswitch sequence.
	 *
	 *	Fortunately for us, all 4 banks have exactly the same code at this point in their
	 *	ROM, so it doesn't matter which version we're actually executing.
	 */

	if (pc & 0x80000)
		slapstic_tweak(0);
	else if (prevpc & 0x80000)
		slapstic_tweak((prevpc / 2) & 0x3fff);

	return pc;
}



/*************************************
 *
 *		Interrupt handlers.
 *
 *************************************/

int atarisys1_interrupt(void)
{
	/* update the gas pedal for RoadBlasters */
	if (atarisys1_joystick_type == 3)
	{
		if (input_port_1_r(0) & 0x80)
		{
			pedal_value += 64;
			if (pedal_value > 0xff) pedal_value = 0xff;
		}
		else
		{
			pedal_value -= 64;
			if (pedal_value < 0) pedal_value = 0;
		}
	}

	return atarigen_vblank_gen();
}


void atarisys1_sound_interrupt(int irq)
{
	if (irq)
		atarigen_6502_irq_gen();
	else
		atarigen_6502_irq_ack_w(0, 0);
}



/*************************************
 *
 *		Joystick read.
 *
 *************************************/

static void delayed_joystick_int(int param)
{
	joystick_timer = NULL;
	joystick_value = param;
	joystick_int = 1;
	atarigen_update_interrupts();
}


int atarisys1_joystick_r(int offset)
{
	int newval = 0xff;

	/* digital joystick type */
	if (atarisys1_joystick_type == 1)
		newval = (input_port_0_r(offset) & (0x80 >> (offset / 2))) ? 0xf0 : 0x00;

	/* Hall-effect analog joystick */
	else if (atarisys1_joystick_type == 2)
		newval = (offset & 2) ? input_port_0_r(offset) : input_port_1_r(offset);

	/* Road Blasters gas pedal */
	else if (atarisys1_joystick_type == 3)
		newval = pedal_value;

	/* set a timer on the joystick interrupt */
	if (joystick_timer)
		timer_remove(joystick_timer);
	joystick_timer = NULL;

	/* the A4 bit enables/disables joystick IRQs */
	joystick_int_enable = ((offset >> 4) & 1) ^ 1;

	/* clear any existing interrupt and set a timer for a new one */
	joystick_int = 0;
	joystick_timer = timer_set(TIME_IN_USEC(50), newval, delayed_joystick_int);
	atarigen_update_interrupts();

	return joystick_value;
}


void atarisys1_joystick_w(int offset, int data)
{
	/* the A4 bit enables/disables joystick IRQs */
	joystick_int_enable = ((offset >> 4) & 1) ^ 1;
}



/*************************************
 *
 *		Trackball read.
 *
 *************************************/

int atarisys1_trakball_r(int offset)
{
	int result = 0xff;

	/* Marble Madness trackball type -- rotated 45 degrees! */
	if (atarisys1_trackball_type == 1)
	{
		static int old[2][2], cur[2][2];
		int player = (offset >> 2) & 1;
		int which = (offset >> 1) & 1;
		int diff;

		/* when reading the even ports, do a real analog port update */
		if (which == 0)
		{
			int dx,dy;

			if (player == 0)
			{
				dx = (signed char)input_port_0_r(offset);
				dy = (signed char)input_port_1_r(offset);
			}
			else
			{
				dx = (signed char)input_port_2_r(offset);
				dy = (signed char)input_port_3_r(offset);
			}

			cur[player][0] += dx + dy;
			cur[player][1] += dx - dy;
		}

		/* clip the result to -0x3f to +0x3f to remove directional ambiguities */
		diff = cur[player][which] - old[player][which];
		if (diff < -0x3f) diff = -0x3f;
		if (diff >  0x3f) diff =  0x3f;
		result = old[player][which] += diff;
	}

	/* Road Blasters steering wheel */
	else if (atarisys1_trackball_type == 2)
		result = input_port_0_r(offset);

	return result;
}



/*************************************
 *
 *		I/O read dispatch.
 *
 *************************************/

int atarisys1_io_r(int offset)
{
	int temp = input_port_5_r(offset);
	if (atarigen_cpu_to_sound_ready) temp ^= 0x0080;
	return temp;
}


int atarisys1_6502_switch_r(int offset)
{
	int temp = input_port_4_r(offset);

	if (atarigen_cpu_to_sound_ready) temp ^= 0x08;
	if (atarigen_sound_to_cpu_ready) temp ^= 0x10;
	if (!(input_port_5_r(offset) & 0x0040)) temp ^= 0x80;

	return temp;
}



/*************************************
 *
 *		TMS5220 communications
 *
 *************************************/

/*
 *	All communication to the 5220 goes through an SY6522A, which is an overpowered chip
 *	for the job.  Here is a listing of the I/O addresses:
 *
 *		$00	DRB		Data register B
 *		$01	DRA		Data register A
 *		$02	DDRB	Data direction register B (0=input, 1=output)
 *		$03	DDRA	Data direction register A (0=input, 1=output)
 *		$04	T1CL	T1 low counter
 *		$05	T1CH	T1 high counter
 *		$06	T1LL	T1 low latches
 *		$07	T1LH	T1 high latches
 *		$08	T2CL	T2 low counter
 *		$09	T2CH	T2 high counter
 *		$0A	SR		Shift register
 *		$0B	ACR		Auxiliary control register
 *		$0C	PCR		Peripheral control register
 *		$0D	IFR		Interrupt flag register
 *		$0E	IER		Interrupt enable register
 *		$0F	NHDRA	No handshake DRA
 *
 *	Fortunately, only addresses $00,$01,$0B,$0C, and $0F are accessed in the code, and
 *	$0B and $0C are merely set up once.
 *
 *	The ports are hooked in like follows:
 *
 *	Port A, D0-D7 = TMS5220 data lines (i/o)
 *
 *	Port B, D0 = 	Write strobe (out)
 *	        D1 = 	Read strobe (out)
 *	        D2 = 	Ready signal (in)
 *	        D3 = 	Interrupt signal (in)
 *	        D4 = 	LED (out)
 *	        D5 = 	??? (out)
 */

int atarisys1_6522_r(int offset)
{
	switch (offset)
	{
		case 0x00:	/* DRB */
			return (m6522_drb & m6522_ddrb) | (!tms5220_ready_r() << 2) | (!tms5220_int_r() << 3);

		case 0x01:	/* DRA */
		case 0x0f:	/* NHDRA */
			return (m6522_dra & m6522_ddra);

		case 0x02:	/* DDRB */
			return m6522_ddrb;

		case 0x03:	/* DDRA */
			return m6522_ddra;

		default:
			return m6522_regs[offset & 15];
	}
}


void atarisys1_6522_w(int offset, int data)
{
	int old;

	switch (offset)
	{
		case 0x00:	/* DRB */
			old = m6522_drb;
			m6522_drb = (m6522_drb & ~m6522_ddrb) | (data & m6522_ddrb);
			if (!(old & 1) && (m6522_drb & 1))
				tms5220_data_w(0, m6522_dra);
			if (!(old & 2) && (m6522_drb & 2))
				m6522_dra = (m6522_dra & m6522_ddra) | (tms5220_status_r(0) & ~m6522_ddra);
			break;

		case 0x01:	/* DRA */
		case 0x0f:	/* NHDRA */
			m6522_dra = (m6522_dra & ~m6522_ddra) | (data & m6522_ddra);
			break;

		case 0x02:	/* DDRB */
			m6522_ddrb = data;
			break;

		case 0x03:	/* DDRA */
			m6522_ddra = data;
			break;

		default:
			m6522_regs[offset & 15] = data;
			break;
	}
}



/*************************************
 *
 *		Speed cheats
 *
 *************************************/

int marble_speedcheck_r(int offset)
{
	int result = READ_WORD(&marble_speedcheck[offset]);

	if (offset == 2 && result == 0)
	{
		int time = cpu_gettotalcycles();
		if (time - speedcheck_time1 < 100 && speedcheck_time1 - speedcheck_time2 < 100)
			cpu_spinuntil_int();

		speedcheck_time2 = speedcheck_time1;
		speedcheck_time1 = time;
	}

	return result;
}


void marble_speedcheck_w(int offset, int data)
{
	COMBINE_WORD_MEM(&marble_speedcheck[offset], data);
	speedcheck_time1 = cpu_gettotalcycles() - 1000;
	speedcheck_time2 = speedcheck_time1 - 1000;
}
