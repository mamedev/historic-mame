/*************************************************************************

    Exidy Vertigo hardware

*************************************************************************/

#include "driver.h"
#include "vidhrdw/vector.h"
#include "vertigo.h"
#include "exidy440.h"
#include "machine/74148.h"

/*************************************
 *
 *  Prototypes
 *
 *************************************/

static void v_irq4_w(int level);
static void v_irq3_w(int level);
static void update_irq(void);


/*************************************
 *
 *  Statics
 *
 *************************************/

/* Timestamp of last INTL4 change. The vector CPU runs for
   the delta between this and now.
*/
static double irq4_time;

/* State of the priority encoder output */
static int irq_state;

/* Result of the last ADC channel sampled */
static int adc_result;

/* 8254 timer structs */

typedef struct _pit8254_config
{
	struct
	{
		double clock;
		void (*callback)(int state);
	} timer[3];
} pit8254_config;

typedef struct _pit8254
{
	UINT8 cw[3];
	UINT16 counter[3];
	int lc[3];
	mame_timer *pt[3];
	void (*callback[3])(int state);
	double clock[3];
} pit8254;

static const pit8254_config v_pit8254 =
{
	{
		{
			240000,
			v_irq4_w,
		},
		{
			240000,
			v_irq3_w,
		},
		{
			240000,
			NULL,
		}
	}
};

static pit8254 pit;

static struct TTL74148_interface irq_encoder =
{
	update_irq
};


/*************************************
 *
 *  IRQ handling. The priority encoder
 *  has to be emulated. Otherwise
 *  interrupts are lost.
 *
 *************************************/

static void update_irq(void)
{
	if (irq_state < 7)
		cpunum_set_input_line(0, irq_state ^ 7, CLEAR_LINE);

	irq_state = TTL74148_output_r(0);

	if (irq_state < 7)
		cpunum_set_input_line(0, irq_state ^ 7, ASSERT_LINE);
}

static void update_irq_encoder(int line, int state)
{
	TTL74148_input_line_w(0, line, !state);
	TTL74148_update(0);
}

static void v_irq4_w(int level)
{
	update_irq_encoder(INPUT_LINE_IRQ4, level);
	vertigo_vproc(TIME_TO_CYCLES(0, timer_get_time() - irq4_time), level);
	irq4_time = timer_get_time();
}

static void v_irq3_w(int level)
{
	if (level)
		cpunum_set_input_line(1, INPUT_LINE_IRQ0, ASSERT_LINE);

	update_irq_encoder(INPUT_LINE_IRQ3, level);
}


/*************************************
 *
 *  ADC and coin handlers
 *
 *************************************/

READ16_HANDLER(vertigo_io_convert)
{
	if (offset > 2)
		adc_result = 0;
	else
		adc_result = readinputport(offset);

	update_irq_encoder(INPUT_LINE_IRQ2, ASSERT_LINE);
	return 0;
}

READ16_HANDLER(vertigo_io_adc)
{
	update_irq_encoder(INPUT_LINE_IRQ2, CLEAR_LINE);
	return adc_result;
}

READ16_HANDLER(vertigo_coin_r)
{
	update_irq_encoder(INPUT_LINE_IRQ6, CLEAR_LINE);
	return (readinputportbytag("COIN"));
}

INTERRUPT_GEN(vertigo_interrupt)
{
	/* Coin inputs cause IRQ6 */
	if ((readinputportbytag("COIN") & 0x7) < 0x7)
		update_irq_encoder(INPUT_LINE_IRQ6, ASSERT_LINE);
}


/*************************************
 *
 *  Sound board interface
 *
 *************************************/

WRITE16_HANDLER( vertigo_wsot_w )
{
	/* Reset sound cpu */
	if ((data & 2) == 0)
		cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);
	else
		cpunum_set_input_line(1, INPUT_LINE_RESET, CLEAR_LINE);
}

static void sound_command_w(int data)
{
	exidy440_sound_command = data;
	exidy440_sound_command_ack = 0;
	cpunum_set_input_line(1, INPUT_LINE_IRQ1, ASSERT_LINE);

	/* It is important that the sound cpu ACKs the sound command
       quickly. Otherwise the main CPU gives up with sound. Boosting
       the interleave for a while helps. */

	cpu_boost_interleave(0, TIME_IN_USEC(100));
}

WRITE16_HANDLER( vertigo_audio_w )
{
	if (ACCESSING_LSB)
		timer_set(TIME_NOW, data & 0xff, sound_command_w);
}

READ16_HANDLER(vertigo_sio_r)
{
	if (exidy440_sound_command_ack)
		return 0xfc;
	else
		return 0xfd;
}


/*************************************
 *
 *  8254 timer
 *
 *************************************/

WRITE16_HANDLER( vertigo_8254_w )
{
	int sc, rw, m, bcd;
	double interval;

	offset &= 3;
	switch (offset)
	{
	case 0:
	case 1:
	case 2:
		if (pit.lc[offset])
		{
			pit.counter[offset] = data & 0xff;
			pit.lc[offset] = 0;
		}
		else
		{
			pit.counter[offset] |= (data << 8);
			pit.lc[offset] = 1;

			interval = (double)pit.counter[offset] / pit.clock[offset];
			m = (pit.cw[offset] >> 1) & 7;
			switch (m)
			{
			case 0:
				/* Mode 0 interrupt on terminal count */
				timer_adjust(pit.pt[offset], TIME_IN_SEC(interval), 1, TIME_NEVER);
				break;
			case 2:
			case 3:
				/* Mode 2,3 are periodic. Mode 3 should generate a
                   squarewave which is wrong here. Vertigo just uses
                   this mode as a PRNG. Output (OUT2) is not connected.*/
				timer_adjust(pit.pt[offset], TIME_IN_SEC(interval), 1, TIME_IN_SEC(interval));
				break;
			default:
				logerror("Vertigo 8354: mode %d not supported",m);
				break;
			}
		}
		break;
	case 3:
		sc = data >> 6;
		pit.cw[sc] = data;
		rw = (data >> 4) & 3;
		m  = (data >> 1) & 7;
		bcd = data & 1;
		if (sc < 3 && rw==3 && bcd==0)
		{
			pit.lc[sc] = 1;

			if (timer_timeleft(pit.pt[sc]))
				timer_adjust(pit.pt[sc], TIME_NEVER, 0, TIME_NEVER);

			if (pit.callback[sc])
				pit.callback[sc](0);
		}
		else
			logerror("Vertigo 8354: configuration %x not supported",data);
		break;
	default:
		break;
	}
}

READ16_HANDLER(vertigo_8254_r)
{
	int c;

	offset &= 3;
	/* Get current counter value */
	c = timer_timeleft(pit.pt[offset]) * (double)pit.clock[offset];

	/* If timer isn't running return initial value */
	c = (c > pit.counter[offset])? pit.counter[offset]: c;
	pit.lc[offset] = (pit.lc[offset] + 1) & 1;

	/* LSB first */
	if (pit.lc[offset])
		c >>= 8;
	return (c & 0xff);
}

/*************************************
 *
 *  Machine initialization
 *
 *************************************/

MACHINE_INIT(vertigo)
{
	int i;

	TTL74148_config(0, &irq_encoder);
	TTL74148_enable_input_w(0, 0);

	for (i = 0; i < 8; i++)
		TTL74148_input_line_w(0, i, 1);

	TTL74148_update(0);
	irq_state = 7;

	for (i = 0; i < 3; i++)
	{
		pit.callback[i]=v_pit8254.timer[i].callback;
		pit.clock[i]=v_pit8254.timer[i].clock;
		pit.pt[i] = mame_timer_alloc(pit.callback[i]);
	}

	vertigo_vproc_init();
	irq4_time = timer_get_time();
}


/*************************************
 *
 *  Motor controller interface
 *
 *************************************/

WRITE16_HANDLER( vertigo_motor_w )
{
	/* Motor controller interface. Not emulated. */
}

