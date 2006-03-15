/***************************************************************************

    segag80.c

    Sound boards for early Sega G-80 based games.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "driver.h"
#include "streams.h"
#include "sega.h"
#include "cpu/i8039/i8039.h"
#include "sound/sp0250.h"
#include <math.h>



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define USB_2MHZ_CLOCK		2000000
#define USB_PCS_CLOCK		(USB_2MHZ_CLOCK/2)
#define USB_GOS_CLOCK		(USB_2MHZ_CLOCK/4)
#define MM5837_CLOCK		100000



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _timer8253_channel timer8253_channel;
struct _timer8253_channel
{
	UINT8				holding;			/* holding until counts written? */
	UINT8				latchmode;			/* latching mode */
	UINT8				latchtoggle;		/* latching state */
	UINT8				clockmode;			/* clocking mode */
	UINT8				bcdmode;			/* BCD mode? */
	UINT8				output;				/* current output value */
	UINT8				lastgate;			/* previous gate value */
	UINT8				gate;				/* current gate value */
	UINT8				env;				/* envelope value */
	UINT8				subcount;			/* subcount (2MHz clocks per input clock) */
	UINT16				count;				/* initial count */
	UINT16				remain;				/* current down counter value */
};

typedef struct _timer8253 timer8253;
struct _timer8253
{
	timer8253_channel	chan[3];
};



/***************************************************************************
    GLOBALS
***************************************************************************/

/* SP0250-based speech board */
static UINT8 speech_latch, speech_t0, speech_p2, speech_drq;

/* Universal sound board */
static sound_stream *usb_stream;
static UINT8 usb_cpunum;
static UINT8 usb_in_latch;
static UINT8 usb_out_latch;
static UINT8 usb_last_p2_value;
static UINT8 *usb_program_ram;
static UINT8 *usb_work_ram;
static UINT8 usb_work_ram_bank;
static UINT8 usb_t1_clock;
static UINT8 usb_t1_clock_mask;
static timer8253 usb_timer_group[3];
static UINT8 usb_timer_mode[3];
static UINT32 usb_noise_shift;
static UINT8 usb_noise_state;
static UINT8 usb_noise_subcount;
static double usb_cr_cap, usb_cr_exp;
static double usb_noise_rc1_cap, usb_noise_rc1_exp;
static double usb_noise_rc2_cap, usb_noise_rc2_exp;
static double usb_noise_rc3_cap, usb_noise_rc3_exp;
static double usb_noise_rc4_cap, usb_noise_rc4_exp;
static double usb_noise_cr_cap, usb_noise_cr_exp;



/***************************************************************************
    SPEECH BOARD
***************************************************************************/


/*************************************
 *
 *  i8035 port accessors
 *
 *************************************/

static READ8_HANDLER( speech_t0_r )
{
	return speech_t0;
}

static READ8_HANDLER( speech_t1_r )
{
	return speech_drq;
}

static READ8_HANDLER( speech_p1_r )
{
	return speech_latch & 0x7f;
}

static READ8_HANDLER( speech_rom_r )
{
	return memory_region(REGION_CPU2)[0x800 + 0x100 * (speech_p2 & 0x3f) + offset];
}

static WRITE8_HANDLER( speech_p1_w )
{
	if (!(data & 0x80))
		speech_t0 = 0;
}

static WRITE8_HANDLER( speech_p2_w )
{
	speech_p2 = data;
}



/*************************************
 *
 *  i8035 port accessors
 *
 *************************************/

static void speech_drq_w(int level)
{
	speech_drq = (level == ASSERT_LINE);
}



/*************************************
 *
 *  External access
 *
 *************************************/

static void delayed_speech_w(int data)
{
	UINT8 old = speech_latch;

	/* all 8 bits are latched */
	speech_latch = data;

	/* the high bit goes directly to the INT line */
	cpunum_set_input_line(1, 0, (data & 0x80) ? CLEAR_LINE : ASSERT_LINE);

	/* a clock on the high bit clocks a 1 into T0 */
	if (!(old & 0x80) && (data & 0x80))
		speech_t0 = 1;
}


WRITE8_HANDLER( sega_speech_data_w )
{
	timer_set(TIME_NOW, data, delayed_speech_w);
}


WRITE8_HANDLER( sega_speech_control_w )
{
	logerror("Speech control = %X\n", data);
}



/*************************************
 *
 *  Speech board address maps
 *
 *************************************/

static ADDRESS_MAP_START( speech_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x07ff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( speech_portmap, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0xff) AM_READ(speech_rom_r)
	AM_RANGE(0x00, 0xff) AM_WRITE(sp0250_w)
	AM_RANGE(I8039_p1, I8039_p1) AM_READWRITE(speech_p1_r, speech_p1_w)
	AM_RANGE(I8039_p2, I8039_p2) AM_WRITE(speech_p2_w)
	AM_RANGE(I8039_t0, I8039_t0) AM_READ(speech_t0_r)
	AM_RANGE(I8039_t1, I8039_t1) AM_READ(speech_t1_r)
ADDRESS_MAP_END



/*************************************
 *
 *  Speech board sound interfaces
 *
 *************************************/

static struct sp0250_interface sp0250_interface =
{
	speech_drq_w
};



/*************************************
 *
 *  Speech board machine drivers
 *
 *************************************/

MACHINE_DRIVER_START( sega_speech_board )

	/* CPU for the speech board */
	MDRV_CPU_ADD(I8035, 3120000/15)		/* divide by 15 in CPU */
	MDRV_CPU_PROGRAM_MAP(speech_map, 0)
	MDRV_CPU_IO_MAP(speech_portmap, 0)

	/* sound hardware */
	MDRV_SOUND_ADD(SP0250, 3120000)
	MDRV_SOUND_CONFIG(sp0250_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END



/***************************************************************************
    UNIVERSAL SOUND BOARD
***************************************************************************/

/*

ENV1.Y3 = 0 (X)


NOISE--+--



                              +-----+
                              | OUT2|---+--|\
                     100k     |     |   |  |+\       33k     100k
                  +--vvvv--+  |     |   v  |  >--+--vvvv--+--vvvv--+
       100k       |        |  |     |  gnd |-/   |        |        |
   +---vvvv--+----+--|\    |  | OUT1|---+--|/    |        +--|\    |
             |       |-\   |  |     |   |        |           |-\   |  100k
             |       |  >--+--|Vref |  ===100p   |           |  >--+--vvvv---> mix with output
       100k  |       |+/      |     |   |        |           |+/
NOISE--vvvv--+    +--|/       |  Rfb|---+--------+        +--|/
                  |           +-----+                     |
                  v                                       v
                 gnd                                     gnd
*/


/*************************************
 *
 *  Initialization/reset
 *
 *************************************/

static void increment_t1_clock(int param)
{
	/* only increment if it is not being forced clear */
	if (!(usb_last_p2_value & 0x80))
		usb_t1_clock++;
}


void sega_usb_reset(UINT8 t1_clock_mask)
{
	/* halt the USB CPU at reset time */
	cpunum_set_input_line(usb_cpunum, INPUT_LINE_RESET, ASSERT_LINE);

	/* start the clock timer */
	timer_pulse(TIME_IN_HZ(USB_2MHZ_CLOCK / 256), 0, increment_t1_clock);
	usb_t1_clock_mask = t1_clock_mask;
}



/*************************************
 *
 *  External access
 *
 *************************************/

READ8_HANDLER( sega_usb_status_r )
{
	logerror("%04X:usb_data_r = %02X\n", activecpu_get_pc(), usb_out_latch);

	/* only bits 0 and 7 are controlled by the I8035; the remaining */
	/* bits 1-6 reflect the current input latch values */
	return (usb_out_latch & 0x81) | (usb_in_latch & 0x7e);
}


static void delayed_usb_data_w(int data)
{
	/* look for rising/falling edges of bit 7 to control the RESET line */
	cpunum_set_input_line(usb_cpunum, INPUT_LINE_RESET, (data & 0x80) ? ASSERT_LINE : CLEAR_LINE);

	/* if the CLEAR line is set, the low 7 bits of the input are ignored */
	if ((usb_last_p2_value & 0x40) == 0)
		data &= ~0x7f;

	/* update the effective input latch */
	usb_in_latch = data;
}


WRITE8_HANDLER( sega_usb_data_w )
{
	logerror("%04X:usb_data_w = %02X\n", activecpu_get_pc(), data);
	timer_set(TIME_NOW, data, delayed_usb_data_w);
}


READ8_HANDLER( sega_usb_ram_r )
{
	return usb_program_ram[offset];
}


WRITE8_HANDLER( sega_usb_ram_w )
{
	if (usb_in_latch & 0x80)
		usb_program_ram[offset] = data;
	else
		logerror("%04X:sega_usb_ram_w(%03X) = %02X while /LOAD disabled\n", activecpu_get_pc(), offset, data);
}



/*************************************
 *
 *  I8035 port accesses
 *
 *************************************/

static READ8_HANDLER( usb_p1_r )
{
	/* bits 0-6 are inputs and map to bits 0-6 of the input latch */
	if ((usb_in_latch & 0x7f) != 0)
		logerror("%03X: P1 read = %02X\n", activecpu_get_pc(), usb_in_latch & 0x7f);
	return usb_in_latch & 0x7f;
}


static WRITE8_HANDLER( usb_p1_w )
{
	/* bit 7 maps to bit 0 on the output latch */
	usb_out_latch = (usb_out_latch & 0xfe) | (data >> 7);
	logerror("%03X: P1 write = %02X\n", activecpu_get_pc(), data);
}


static WRITE8_HANDLER( usb_p2_w )
{
	UINT8 old = usb_last_p2_value;
	usb_last_p2_value = data;

	/* low 2 bits control the bank of work RAM we are addressing */
	usb_work_ram_bank = data & 3;

	/* bit 6 controls the "ready" bit output to the host */
	/* it also clears the input latch from the host (active low) */
	usb_out_latch = ((data & 0x40) << 1) | (usb_out_latch & 0x7f);
	if ((data & 0x40) == 0)
		usb_in_latch = 0;

	/* bit 7 controls the reset on the upper counter at U33 */
	if ((old & 0x80) && !(data & 0x80))
		usb_t1_clock = 0;

	logerror("%03X: P2 write -> bank=%d ready=%d clock=%d\n", activecpu_get_pc(), data & 3, (data >> 6) & 1, (data >> 7) & 1);
}


static READ8_HANDLER( usb_t0_r )
{
	/* T0 returns the raw 2MHz clock */
	mame_time curtime = mame_timer_get_time();
	return (curtime.subseconds / (MAX_SUBSECONDS / (2 * USB_2MHZ_CLOCK))) & 1;
}


static READ8_HANDLER( usb_t1_r )
{
	/* T1 returns 1 based on the value of the T1 clock; the exact */
	/* pattern is determined by one or more jumpers on the board. */
	return (usb_t1_clock & usb_t1_clock_mask) != 0;
}



/*************************************
 *
 *  Sound generation
 *
 *************************************/

INLINE void clock_channel(timer8253_channel *ch)
{
	UINT8 lastgate = ch->lastgate;

	/* update the gate */
	ch->lastgate = ch->gate;

	/* if we're holding, skip */
	if (ch->holding)
		return;

	/* switch off the clock mode */
	switch (ch->clockmode)
	{
		/* oneshot; waits for trigger to restart */
		case 1:
			if (!lastgate && ch->gate)
			{
				ch->output = 0;
				ch->remain = ch->count;
			}
			else
			{
				if (ch->remain-- == 0)
					ch->output = 1;
			}
			break;

		/* square wave: counts down by 2 and toggles output */
		case 3:
			ch->remain = (ch->remain - 1) & ~1;
			if (ch->remain == 0)
			{
				ch->output ^= 1;
				ch->remain = ch->count;
			}
			break;
	}
}


static void usb_stream_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int length)
{
	stream_sample_t *dest = outputs[0];

	/* iterate over samples */
	while (length--)
	{
		int noiseval;
		int sample = 0;
		int group;

		/* update the noise source */
		if (usb_noise_subcount-- == 0)
		{
			usb_noise_shift = (usb_noise_shift << 1) | (((usb_noise_shift >> 13) ^ (usb_noise_shift >> 16)) & 1);
			usb_noise_state = (usb_noise_shift >> 16) & 1;
			usb_noise_subcount = USB_2MHZ_CLOCK / MM5837_CLOCK - 1;

#if 0
{
	static FILE *f;
	if (!f)
	{
		f = fopen("noise.log", "w");
		fprintf(f, "rc1_exp = %.10f\nrc2_exp = %.10f\nrc3_exp = %.10f\ncr_exp = %.10f\n",
			usb_noise_rc1_exp,
			usb_noise_rc2_exp,
			usb_noise_rc3_exp,
			usb_noise_cr_exp);
	}
	fprintf(f, "nv=%d  c1=%.10f  c2=%.10f  c3=%.10f  cr=%.10f  res=%d\n", usb_noise_state,
			usb_noise_rc1_cap,
			usb_noise_rc2_cap,
			usb_noise_rc3_cap,
			usb_noise_cr_cap,
			noiseval);
}
#endif
		}

		usb_noise_rc1_cap += ((double)usb_noise_state - usb_noise_rc1_cap) * usb_noise_rc1_exp;
		usb_noise_rc2_cap += (usb_noise_rc1_cap - usb_noise_rc2_cap) * usb_noise_rc2_exp;
		usb_noise_rc3_cap += (usb_noise_rc2_cap - usb_noise_rc3_cap) * usb_noise_rc3_exp;
		usb_noise_rc4_cap += (usb_noise_rc3_cap - usb_noise_rc4_cap) * usb_noise_rc4_exp;
		noiseval = (usb_noise_rc4_cap - usb_noise_cr_cap) * 30 * 3.2 * 150;
		usb_noise_cr_cap += (usb_noise_rc4_cap - usb_noise_cr_cap) * usb_noise_cr_exp;

		/* there are 3 nearly identical groups of circuits, each with its own 8253 */
		for (group = 0; group < 3; group++)
		{
			timer8253 *g = &usb_timer_group[group];
			stream_sample_t timerval = 0;

			/*
                8253        CR         UNITY      AD7524
                OUT0 ---> FILTER ---->  GAIN  -->  VRef  ---> 100k -> mix
                                      FOLLOWER
            */

			/* channel 0 clocks with the PCS clock */
			if (g->chan[0].subcount-- == 0)
			{
				g->chan[0].subcount = USB_2MHZ_CLOCK / USB_PCS_CLOCK - 1;
				g->chan[0].gate = 1;
				clock_channel(&g->chan[0]);
			}

			/* channel 0 is mixed in with a resistance of 100k */
			timerval += 50 * g->chan[0].output * g->chan[0].env;

			/* channel 1 clocks with the PCS clock */
			if (g->chan[1].subcount-- == 0)
			{
				g->chan[1].subcount = USB_2MHZ_CLOCK / USB_PCS_CLOCK - 1;
				g->chan[1].gate = 1;
				clock_channel(&g->chan[1]);
			}

			/* channel 1 is mixed in with a resistance of 100k */
			timerval += 50 * g->chan[1].output * g->chan[1].env;

			/* channel 2 clocks with the 2MHZ clock and triggers with the GOS clock */
			if (g->chan[2].subcount-- == 0)
			{
				g->chan[2].subcount = USB_2MHZ_CLOCK / USB_GOS_CLOCK / 2 - 1;
				g->chan[2].gate = !g->chan[2].gate;
			}
			clock_channel(&g->chan[2]);

			/* based on the envelope mode, we do one of two things with source 2 */
			if (usb_timer_mode[group] == 0)
			{
				/* the raw noise source is gated by the output of channel 2 and then */
				/* used as the Vref for the channel 2 envelope DAC; the output of */
				/* that is mixed in with a resistance of 33k */
				timerval += noiseval * g->chan[2].env;
			}
			else
			{
				/* the raw noise source is used as the Vref for the channel 2 envelope */
				/* DAC, which is mixed in with a resistance of 33k; the final mixed result */
				/* is then gated by the output of channel 2 */
				timerval += noiseval * g->chan[2].env;
			}

			/* accumulate the sample */
			sample += timerval;
		}

		*dest++ = sample - (stream_sample_t)usb_cr_cap;
		usb_cr_cap += ((double)sample - usb_cr_cap) * usb_cr_exp;
	}
}


void *usb_start(int clock, const struct CustomSound_interface *config)
{
	/* find the CPU we are associated with */
	usb_cpunum = mame_find_cpu_index("usb");
	assert(usb_cpunum != (UINT8)-1);

	/* allocate work RAM */
	usb_work_ram = auto_malloc(0x400);

	/* create a sound stream */
	usb_stream = stream_create(0, 1, USB_2MHZ_CLOCK, NULL, usb_stream_update);

	/* initialize state */
	usb_noise_shift = 0x15555;
	usb_cr_cap = 0;
	usb_cr_exp = 1.0 - exp(-1.0 / (100e3 * 4.7e-6 * USB_2MHZ_CLOCK));

	usb_noise_rc1_cap = 0;
	usb_noise_rc1_exp = 1.0 - exp(-1.0 / ((2.7e3 + 2.7e3) * 1.0e-6 * USB_2MHZ_CLOCK));
	usb_noise_rc2_cap = 0;
	usb_noise_rc2_exp = 1.0 - exp(-1.0 / ((2.7e3 + 1e3) * 0.30e-6 * USB_2MHZ_CLOCK));
	usb_noise_rc3_cap = 0;
	usb_noise_rc3_exp = 1.0 - exp(-1.0 / ((2.7e3 + 270) * 0.15e-6 * USB_2MHZ_CLOCK));
	usb_noise_rc4_cap = 0;
	usb_noise_rc4_exp = 1.0 - exp(-1.0 / ((2.7e3 + 0) * 0.082e-6 * USB_2MHZ_CLOCK));
	usb_noise_cr_cap = 0;
	usb_noise_cr_exp = 1.0 - exp(-1.0 / (33e3 * 0.1e-6 * USB_2MHZ_CLOCK));

	return usb_stream;
}



/*************************************
 *
 *  USB timer and envelope controls
 *
 *************************************/

static void timer_w(int which, UINT8 offset, UINT8 data)
{
	timer8253 *g = &usb_timer_group[which];
	timer8253_channel *ch;
	int was_holding;

	stream_update(usb_stream, 0);

	/* switch off the offset */
	switch (offset)
	{
		case 0:
		case 1:
		case 2:
			ch = &g->chan[offset];
			was_holding = ch->holding;

			/* based on the latching mode */
			switch (ch->latchmode)
			{
				case 1:	/* low word only */
					ch->count = data;
					ch->holding = FALSE;
					break;

				case 2:	/* high word only */
					ch->count = data << 8;
					ch->holding = FALSE;
					break;

				case 3:	/* low word followed by high word */
					if (ch->latchtoggle == 0)
					{
						ch->count = (ch->count & 0xff00) | (data & 0x00ff);
						ch->latchtoggle = 1;
					}
					else
					{
						ch->count = (ch->count & 0x00ff) | (data << 8);
						ch->holding = FALSE;
						ch->latchtoggle = 0;
					}
					break;
			}

			/* if we're not holding, load the initial count for some modes */
			if (was_holding && !ch->holding)
				ch->remain = 1;
			break;

		case 3:
			/* break out the components */
			if (((data & 0xc0) >> 6) < 3)
			{
				ch = &g->chan[(data & 0xc0) >> 6];

				/* extract the bits */
				ch->holding = TRUE;
				ch->latchmode = (data >> 4) & 3;
				ch->clockmode = (data >> 1) & 7;
				ch->bcdmode = (data >> 0) & 1;
				ch->latchtoggle = 0;
				ch->output = (ch->clockmode == 1);

				/* we don't support BCD mode */
//              assert(ch->bcdmode == 0);
//              assert(ch->latchmode != 0);
//              assert(ch->clockmode == 1 || ch->clockmode == 3);
			}
			break;
	}

	logerror("timer%d_w(%d) = %02X\n", which, offset, data);
}


static void env_w(int which, UINT8 offset, UINT8 data)
{
	timer8253 *g = &usb_timer_group[which];

	stream_update(usb_stream, 0);

	if (offset < 3)
		g->chan[offset].env = data;
	else
		usb_timer_mode[which] = data & 1;

	logerror("env%d_w(%d) = %02X\n", which, offset, data);
}



/*************************************
 *
 *  USB work RAM access
 *
 *************************************/

static READ8_HANDLER( usb_workram_r )
{
	offset += 256 * usb_work_ram_bank;
	return usb_work_ram[offset];
}


static WRITE8_HANDLER( usb_workram_w )
{
	offset += 256 * usb_work_ram_bank;
	usb_work_ram[offset] = data;

	/* writes to the low 32 bytes go to various controls */
	switch (offset & ~3)
	{
		case 0x00:	/* CTC0 */
			timer_w(0, offset & 3, data);
			break;

		case 0x04:	/* ENV0 */
			env_w(0, offset & 3, data);
			break;

		case 0x08:	/* CTC1 */
			timer_w(1, offset & 3, data);
			break;

		case 0x0c:	/* ENV1 */
			env_w(1, offset & 3, data);
			break;

		case 0x10:	/* CTC2 */
			timer_w(2, offset & 3, data);
			break;

		case 0x14:	/* ENV2 */
			env_w(2, offset & 3, data);
			break;
	}
}



/*************************************
 *
 *  USB address maps
 *
 *************************************/

static ADDRESS_MAP_START( usb_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x0fff) AM_RAM AM_BASE(&usb_program_ram)
ADDRESS_MAP_END


static ADDRESS_MAP_START( usb_portmap, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x00, 0xff) AM_READWRITE(usb_workram_r, usb_workram_w)
	AM_RANGE(I8039_p1, I8039_p1) AM_READWRITE(usb_p1_r, usb_p1_w)
	AM_RANGE(I8039_p2, I8039_p2) AM_WRITE(usb_p2_w)
	AM_RANGE(I8039_t0, I8039_t0) AM_READ(usb_t0_r)
	AM_RANGE(I8039_t1, I8039_t1) AM_READ(usb_t1_r)
ADDRESS_MAP_END



/*************************************
 *
 *  USB sound definitions
 *
 *************************************/

static struct CustomSound_interface usb_custom_interface =
{
    usb_start
};



/*************************************
 *
 *  USB machine drivers
 *
 *************************************/

MACHINE_DRIVER_START( sega_universal_sound_board )

	/* CPU for the usb board */
	MDRV_CPU_ADD_TAG("usb", I8035, 6000000/15)		/* divide by 15 in CPU */
	MDRV_CPU_PROGRAM_MAP(usb_map, 0)
	MDRV_CPU_IO_MAP(usb_portmap, 0)

	/* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, 0)
	MDRV_SOUND_CONFIG(usb_custom_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END
