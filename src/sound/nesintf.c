/***************************************************************************

  nesintf.c


  Support interface for NES PSG

***************************************************************************/

#include "driver.h"


#define MIN_SLICE 10

static int emulation_rate;
static int buffer_len;
static int sample_bits;

static const struct NESinterface *intf;


static void *buffer[MAX_NESPSG];
static int sample_pos[MAX_NESPSG];

static int channel;

int NESPSG_sh_start(const struct MachineSound *msound)
{
	int i;


	intf = msound->sound_interface;

	buffer_len = Machine->sample_rate / Machine->drv->frames_per_second;
	emulation_rate = buffer_len * Machine->drv->frames_per_second;

	sample_bits = Machine->sample_bits;

	for (i = 0;i < MAX_NESPSG;i++)
	{
		sample_pos[i] = 0;

		buffer[i] = 0;
	}
	for (i = 0;i < intf->num;i++)
	{
		if ((buffer[i] = malloc((sample_bits/8)*buffer_len)) == 0)
		{
			while (--i >= 0) free(buffer[i]);
			return 1;
		}
	}
	if (NESInit(intf->num,intf->baseclock,emulation_rate,sample_bits,buffer_len,buffer) == 0)
	{
		channel = mixer_allocate_channels(intf->num,intf->volume);

		return 0;
	}
	for (i = 0;i < intf->num;i++) free(buffer[i]);
	return 1;
}

void NESPSG_sh_stop(void)
{
	int i;

	NESShutdown();
	for (i = 0;i < intf->num;i++){
		free(buffer[i]);
	}
}

static void update(int chip)
{
	int newpos;


	newpos = sound_scalebufferpos(buffer_len);	/* get current position based on the timer */

	if( newpos - sample_pos[chip] < MIN_SLICE ) return;
	NESUpdateOne(chip, newpos );

	sample_pos[chip] = newpos;
}


int NESPSG_0_r(int offset)
{
	return NESReadReg(0,offset);
}
int NESPSG_1_r(int offset)
{
	return NESReadReg(1,offset);
}

void NESPSG_0_w(int offset,int data)
{
	update(0);
	NESWriteReg(0,offset,data);
}
void NESPSG_1_w(int offset,int data)
{
	update(1);
	NESWriteReg(1,offset,data);
}


void NESPSG_sh_update(void)
{
	int i;

	if (Machine->sample_rate == 0 ) return;
	NESUpdate();

	for (i = 0;i < intf->num;i++)
	{
		sample_pos[i] = 0;
		if( sample_bits == 16 )
			mixer_play_streamed_sample_16(channel+i,buffer[i],2*buffer_len,emulation_rate);
		else
			mixer_play_streamed_sample(channel+i,buffer[i],buffer_len,emulation_rate);
	}
}
