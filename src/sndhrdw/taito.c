#include "driver.h"
#include "Z80.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"


#define AUDIO_CONV(A) (A+0x80)

#define TARGET_EMULATION_RATE 44100     /* will be adapted to be a multiple of buffer_len */
static int emulation_rate;
static int buffer_len;
static unsigned char *buffer;
static int sample_pos;

static unsigned char amplitude_DAC;

static int sndnmi_disable = 1;



static void taito_sndnmi_msk(int offset,int data)
{
	sndnmi_disable = data & 0x01;
}



static void taito_digital_out(int offset,int data)
{
	int totcycles,leftcycles,newpos;


	totcycles = Machine->drv->cpu[1].cpu_clock / Machine->drv->frames_per_second;
	leftcycles = cpu_getfcount();
	newpos = buffer_len * (totcycles-leftcycles) / totcycles;

	while (sample_pos < newpos-1)
	    buffer[sample_pos++] = amplitude_DAC;

    amplitude_DAC=AUDIO_CONV(data);

    buffer[sample_pos++] = amplitude_DAC;
}



int taito_sh_interrupt(void)
{
	static int count;


	AY8910_update();

	count++;
	if (count>=16){
		count = 0;
		return 0xff;
	}
	else
	{
		if (pending_commands && !sndnmi_disable) return Z80_NMI_INT;
		else return Z80_IGNORE_INT;
	}
}



static struct AY8910interface interface =
{
	4,	/* 4 chips */
	10,	/* 10 updates per video frame (good quality) */
	1500000000,	/* 1.5 MHz */
	{ 255, 255, 255, 255 },
	{ input_port_5_r, 0, 0, 0 },		/* port Aread */
	{ input_port_6_r, 0, 0, 0 },		/* port Bread */
	{ 0, taito_digital_out, 0, 0 },				/* port Awrite */
	{ 0, 0, 0, taito_sndnmi_msk }	/* port Bwrite */
};



int taito_sh_start(void)
{
	pending_commands = 0;

    buffer_len = TARGET_EMULATION_RATE / Machine->drv->frames_per_second;
    emulation_rate = buffer_len * Machine->drv->frames_per_second;

    if ((buffer = malloc(buffer_len)) == 0)
		return 1;
    memset(buffer,0x80,buffer_len);

    sample_pos = 0;

	return AY8910_sh_start(&interface);
}

void taito_sh_stop(void)
{
	AY8910_sh_stop();

    free(buffer);
}

void taito_sh_update(void)
{
	AY8910_sh_update();

	while (sample_pos < buffer_len)
		buffer[sample_pos++] = amplitude_DAC;

	osd_play_streamed_sample(4,buffer,buffer_len,emulation_rate,255);

	sample_pos=0;
}
