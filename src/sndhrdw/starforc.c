#include "driver.h"
#include "machine/Z80fmly.h"
#include "generic.h"
#include "sn76496.h"
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
static single_rate = 1000;
static single_volume = 0;

/* sound chips */
#define SND_CLOCK (Machine->drv->cpu[1].cpu_clock)
#define CHIPS 3
static struct SN76496 sn[CHIPS];
#define buffer_len 350
#define emulation_rate (buffer_len*Machine->drv->frames_per_second)
static char *sample;


void starforce_pio_w( int offset , int data )
{
	z80pio_w( &pio , (offset/2)&0x01 , offset&0x01 , data );
}

int starforce_pio_r( int offset )
{
	return z80pio_r( &pio , (offset/2)&0x01 , offset&0x01 );
}

int starforce_pio_p_r( int offset )
{
	return z80pio_p_r( &pio , 0 );
}

void  starforce_ctc_w( int offset , int data )
{
	z80ctc_w( &ctc , offset , data );
}

int starforce_ctc_r( int offset  )
{
	return z80ctc_r( &ctc , offset );
}

void  starforce_sh_0_w( int offset , int data )
{
	SN76496Write(&sn[1],data);
}
void  starforce_sh_1_w( int offset , int data )
{
	SN76496Write(&sn[2],data);
}
void  starforce_sh_2_w( int offset , int data )
{
	SN76496Write(&sn[0],data);
}

void starforce_volume_w( int offset , int data )
{
	single_volume = ((data & 0x0f)<<4)|(data & 0x0f);
}

int starforce_sh_interrupt(void)
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

int starforce_sh_start(void)
{
	int i,j;

	pending_commands = 0;

	z80ctc_reset( &ctc , Machine->drv->cpu[1].cpu_clock );
	z80pio_reset( &pio );

	if ((sample = malloc(buffer_len)) == 0)
		return 1;

	if ((single = malloc(SINGLE_LENGTH)) == 0)
	{
		free(sample);
		free(single);
		return 1;
	}
	for (i = 0;i < SINGLE_LENGTH;i++)		/* freq = ctc2 zco / 8 */
		single[i] = ((i/SINGLE_DIVIDER)&0x01)*(SINGLE_VOLUME/2);

	for (j = 0;j < CHIPS;j++)
	{
		sn[j].Clock = SND_CLOCK;
		SN76496Reset(&sn[j]);
	}
	/* CTC2 single tone generator */
	osd_play_sample(CHIPS ,single,SINGLE_LENGTH,single_rate,single_volume,1);
	return 0;
}

void starforce_sh_stop(void)
{
	free(sample);
	free(single);
}

void starforce_sh_update(void)
{
	int i;

	if (play_sound == 0) return;
	for (i = 0;i < CHIPS;i++)
	{
		SN76496UpdateB(&sn[i] , emulation_rate , sample , buffer_len );
		osd_play_streamed_sample(i,sample,buffer_len,emulation_rate,SSG_VOLUME );
	}

	/* CTC2 single tone generator */
	osd_adjust_sample(CHIPS,single_rate,single_volume );
}

