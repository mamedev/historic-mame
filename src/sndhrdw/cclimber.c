#include "driver.h"


#define AY8910_CLOCK (1536000000)       /* 1.536000000 MHZ */
#include "psg.c"

#define SND_CLOCK 3072000	/* 3.072 Mhz */


#define UPDATES_PER_SECOND 60
#define emulation_rate (400*UPDATES_PER_SECOND)
#define buffer_len (emulation_rate/UPDATES_PER_SECOND)


unsigned char samples[0x4000];	/* 16k for samples */
int sample_freq,sample_volume;
int porta;



int lastreg;	/* AY-3-8910 register currently selected */

int cclimber_sh_read_port_r(int offset)
{
	return AYReadReg(0,lastreg);
}



void cclimber_sh_control_port_w(int offset,int data)
{
	lastreg = data;
}



void cclimber_sh_write_port_w(int offset,int data)
{
	AYWriteReg(0,lastreg,data);
}



byte porthandler(AY8910 *chip, int port, int iswrite, byte val)
{
	if (iswrite)	/* Crazy Climber and Crazy Kong */
	{
		if (port == 0x0e) porta = val;
	}
	else		/* Bagman */
	{
		chip->Regs[port] = readinputport(port - 0x0e);
	}

	return 0;
}



int cclimber_sh_start(void)
{
	int i;
	unsigned char bits;


	/* decode the rom samples */
	for (i = 0;i < 0x2000;i++)
	{
		bits = RAM[0x18000 + i] & 0xf0;
		samples[2 * i] = (bits | (bits >> 4)) + 0x80;

		bits = RAM[0x18000 + i] & 0x0f;
		samples[2 * i + 1] = ((bits << 4) | bits) + 0x80;
	}

	if (AYInit(1,emulation_rate,buffer_len,0) == 0)
	{
		AYSetPortHandler(0,AY_PORTA,porthandler);
		AYSetPortHandler(0,AY_PORTB,porthandler);

		return 0;
	}
	else return 1;
}



void cclimber_sh_stop(void)
{
	AYShutdown();
}



void cclimber_sample_rate_w(int offset,int data)
{
	/* calculate the sampling frequency */
	sample_freq = SND_CLOCK / 4 / (256 - data);
}



void cclimber_sample_volume_w(int offset,int data)
{
	sample_volume = data & 0x1f;
	sample_volume = (sample_volume << 3) | (sample_volume >> 2);
}



void cclimber_sample_trigger_w(int offset,int data)
{
	int start,end;


	if (data == 0 || play_sound == 0)
		return;

	start = 64 * porta;
	end = start;

	/* find end of sample */
	while (end < 0x4000 && (samples[end] != 0xf7 || samples[end+1] != 0x80))
		end += 2;

	osd_play_sample(1,samples + start,end - start,sample_freq,sample_volume,0);
}



#define BUFFERS 2
void cclimber_sh_update(void)
{
	static int c;
	static unsigned char buffer[BUFFERS * buffer_len];
	static int playing;


	if (play_sound == 0) return;

	AYSetBuffer(0,&buffer[c * buffer_len]);
	AYUpdate();
	osd_play_streamed_sample(0,&buffer[c * buffer_len],buffer_len,emulation_rate,255);
if (!playing)
{
	osd_play_streamed_sample(0,&buffer[c * buffer_len],buffer_len,emulation_rate,255);
	osd_play_streamed_sample(0,&buffer[c * buffer_len],buffer_len,emulation_rate,255);
	playing = 1;
}
	c ^= 1;
}
