/* TODO:  This file is ALL WRONG - it is just a quick cut and paste from StarForce */

#include "driver.h"
#include "machine/Z80fmly.h"
#include "generic.h"
#include "8910intf.h"
#include <math.h>

/* mixing level */
#define SINGLE_VOLUME 32
#define SSG_VOLUME 255

/* z80 pio , ctc */
#define CPU_CLOCK 2000000
static Z80PIO pio;
static Z80CTC ctc;

/* single tone generator */
#define SINGLE_LENGTH 10000
#define SINGLE_DIVIDER 8

static char *single;
static int single_rate = 1000;
static int single_volume = 0;



void pbaction_pio_w( int offset , int data )
{
	z80pio_w( &pio , (offset/2)&0x01 , offset&0x01 , data );
}

int pbaction_pio_r( int offset )
{
	return z80pio_r( &pio , (offset/2)&0x01 , offset&0x01 );
}

int pbaction_pio_p_r( int offset )
{
	return z80pio_p_r( &pio , 0 );
}

void  pbaction_ctc_w( int offset , int data )
{
	z80ctc_w( &ctc , offset , data );
}

int pbaction_ctc_r( int offset  )
{
	return z80ctc_r( &ctc , offset );
}



void pbaction_volume_w( int offset , int data )
{
	single_volume = ((data & 0x0f)<<4)|(data & 0x0f);
}

int pbaction_sh_interrupt(void)
{
	int irq = 0;

	/* ctc2 timer single tone generator frequency */
	single_rate = Machine->drv->cpu[1].cpu_clock / ctc.tconst[2] * ((ctc.mode[2]&0x20)? 1:16);
#if 0
	z80ctc_update( &ctc,2, 1,0);	/* tone freq. */
	ctc_update(&ctc,3,0,0);			/* not use    */
#endif
	/* ctc_0 cascade to ctc_1 , interval interrupt */
	if( z80ctc_update(&ctc,1,z80ctc_update(&ctc,0,1,0),0 ) ){
		/* interrupt check */
		if( (irq = z80ctc_irq_r(&ctc)) != Z80_IGNORE_INT ) return irq;
	}
	/* pio interrupt check */
	if (pending_commands){
		z80pio_p_w( &pio , 0 , sound_command_r(0) );
		if( (irq = z80pio_irq_r(&pio)) != Z80_IGNORE_INT ) return irq;
	}
	return Z80_IGNORE_INT;
}



static struct AY8910interface interface =
{
	3,	/* 3 chips */
	1832727,	/* 1.832727040 MHZ?????? */
	{ 255, 255, 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};



int pbaction_sh_start(void)
{
	int i;


	pending_commands = 0;

	if (AY8910_sh_start(&interface) != 0)
		return 1;

	z80ctc_reset( &ctc , Machine->drv->cpu[1].cpu_clock );
	z80pio_reset( &pio );

	if ((single = malloc(SINGLE_LENGTH)) == 0)
	{
		AY8910_sh_stop();
		free(single);
		return 1;
	}
	for (i = 0;i < SINGLE_LENGTH;i++)		/* freq = ctc2 zco / 8 */
		single[i] = ((i/SINGLE_DIVIDER)&0x01)*(SINGLE_VOLUME/2);

	/* CTC2 single tone generator */
	osd_play_sample(4,single,SINGLE_LENGTH,single_rate,single_volume,1);

	return 0;
}



void pbaction_sh_stop(void)
{
	AY8910_sh_stop();
	free(single);
}



void pbaction_sh_update(void)
{
	if (Machine->sample_rate == 0) return;

	AY8910_sh_update();

	/* CTC2 single tone generator */
	osd_adjust_sample(4,single_rate,single_volume );
}
