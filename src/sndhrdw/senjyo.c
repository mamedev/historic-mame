#include "driver.h"
#include "machine/z80fmly.h"
#include "sound/samples.h"
#include <math.h>


/* z80 pio */
static void pio_interrupt(int state)
{
	cpunum_set_input_line_and_vector (1, 0, HOLD_LINE, Z80_VECTOR(0,state) );
}

static z80pio_interface pio_intf =
{
	1,
	{pio_interrupt},
	{0},
	{0}
};

/* z80 ctc */
static void ctc_interrupt (int state)
{
	cpunum_set_input_line_and_vector (1, 0, HOLD_LINE, Z80_VECTOR(1,state) );
}

static z80ctc_interface ctc_intf =
{
	1,                   /* 1 chip */
	{ 0 },               /* clock (filled in from the CPU 0 clock */
	{ NOTIMER_2 },       /* timer disables */
	{ ctc_interrupt },   /* interrupt handler */
	{ z80ctc_0_trg1_w }, /* ZC/TO0 callback */
	{ 0 },               /* ZC/TO1 callback */
	{ 0 }                /* ZC/TO2 callback */
};


/* single tone generator */
#define SINGLE_LENGTH 10000
#define SINGLE_DIVIDER 8

static INT16 *_single;
static int single_rate = 1000;
static int single_volume = 0;


WRITE8_HANDLER( senjyo_volume_w )
{
	single_volume = data & 0x0f;
	sample_set_volume(0,single_volume / 15.0);
}

void senjyo_sh_start(void)
{
    int i;

	/* z80 ctc init */
	ctc_intf.baseclock[0] = Machine->drv->cpu[1].cpu_clock;
	z80ctc_init (&ctc_intf);

	/* z80 pio init */
	z80pio_init (&pio_intf);

	_single = (INT16 *)auto_malloc(SINGLE_LENGTH);

	for (i = 0;i < SINGLE_LENGTH;i++)		/* freq = ctc2 zco / 8 */
		_single[i] = ((i/SINGLE_DIVIDER)&0x01)*127*256;

	/* CTC2 single tone generator */
	sample_set_volume(0,0);
	sample_start_raw(0,_single,SINGLE_LENGTH,single_rate,1);
}


void senjyo_sh_update(void)
{
	double period;

	if (Machine->sample_rate == 0) return;


	/* ctc2 timer single tone generator frequency */
	period = z80ctc_getperiod (0, 2);
	if( period != 0 ) single_rate = (int)(1.0 / period );
	else single_rate = 0;

	sample_set_freq(0,single_rate);
}
