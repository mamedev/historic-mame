#include "driver.h"
#include "machine/Z80fmly.h"
#include <math.h>

/* mixing level */
#define SINGLE_VOLUME 32

/* z80 pio */
static void pio_interrupt(int state)
{
	cpu_cause_interrupt (1, Z80_VECTOR(0,state) );
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
	cpu_cause_interrupt (1, Z80_VECTOR(1,state) );
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

static signed char *_single;
static int single_rate = 1000;
static int single_volume = 0;
static int channel;


void senjyo_volume_w( int offset , int data )
{
	single_volume = ((data & 0x0f)<<4)|(data & 0x0f);
}

int senjyo_sh_start(void)
{
    int i;


	channel = get_play_channels(1);

	/* z80 ctc init */
	ctc_intf.baseclock[0] = Machine->drv->cpu[1].cpu_clock;
	z80ctc_init (&ctc_intf);

	/* z80 pio init */
	z80pio_init (&pio_intf);

	if ((_single = (signed char *)malloc(SINGLE_LENGTH)) == 0)
	{
		free(_single);
		return 1;
	}
	for (i = 0;i < SINGLE_LENGTH;i++)		/* freq = ctc2 zco / 8 */
		_single[i] = ((i/SINGLE_DIVIDER)&0x01)*(SINGLE_VOLUME/2);

	/* CTC2 single tone generator */
	osd_play_sample(channel,_single,SINGLE_LENGTH,single_rate,single_volume,1);

	return 0;
}



void senjyo_sh_stop(void)
{
	free(_single);
}



void senjyo_sh_update(void)
{
	double period;

	if (Machine->sample_rate == 0) return;


	/* ctc2 timer single tone generator frequency */
	period = z80ctc_getperiod (0, 2);
	if( period != 0 ) single_rate = (int)(1.0 / period );
	else single_rate = 0;

	osd_adjust_sample(channel,single_rate,single_volume);
}
