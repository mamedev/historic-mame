/*
 *   audio Distribution circuit with volume controller for stream system
 *
 *   by Tatsuyuki Satoh
 *
 *  input stream -----+----- volume controll1 ----- output stream ch1
 *                    +----- vokume controll2 ----- otuput stream ch2
 *
 * used by TAITO games with YM2610
 */

#include <stdio.h>
#include <string.h>

#include "driver.h"
#include "exvolume.h"

struct ex_volume
{
	int basevol[2];
	int pan[2];
	int volume[2];
	int stream[2];
	void *src_buffer;
	int sample_size;
};

/* global pointer to the current interface */
static const struct EXVOLUMEinterface *intf;

static struct ex_volume controller[MAX_EXVOLUME];

static void volumectrlUpdateOne(int param,void *buffer,int length)
{
	memcpy(buffer,controller[param].src_buffer,length*controller[param].sample_size);
}

int EXVOLUME_sh_start(struct MachineSound *msound)
{
	int i,j;
	int vol1,pan1,vol2,pan2;

	if(errorlog) fprintf(errorlog,"taito controller start");

	intf = msound->sound_interface;
	if( intf->num > MAX_EXVOLUME ) return 1;

	/* stream system initialize */
	for (i = 0;i < intf->num;i++)
	{
		char *sname  = intf->name[i];
		/* search source stream channel */
		for (j = 0 ; j<MAX_STREAM_CHANNELS;j++)
		{
			/* source name found ? */
			if(strncmp(stream_get_name(j),sname,strlen(sname))==0 ) break;
		}
		if( j<MAX_STREAM_CHANNELS )
		{
			char new_name[40];
			int sample_bits,sample_rate;

			struct ex_volume *vctrl = &controller[i];
			if(errorlog) fprintf(errorlog,"taito controller %d source =%d\n",i,j);

			/* hook source sound channel */
			vctrl->stream[0] = j;
			sprintf(new_name,"Mixer #%d Ch1(%s)",i,sname);
			stream_set_name(j,new_name);
			/* stream setup for new one */
			sprintf(new_name,"Mixer #%d Ch2(%s)",i,sname);
			sample_bits       = stream_get_sample_bits(j);
			sample_rate       = stream_get_sample_rate(j);
			vctrl->sample_size = sample_bits/8;
			vctrl->src_buffer  = stream_get_buffer(j);
			vctrl->stream[1] =
				stream_init(msound,new_name,sample_rate,sample_bits,i,volumectrlUpdateOne);
			/* volume regiater initialize */
			vctrl->volume[0] = 255;
			vctrl->volume[1] = 255;
			/* volume setup */
			EXVOLUME_set_base_volume(i,0,intf->volume1[i]);
			EXVOLUME_set_base_volume(i,1,intf->volume2[i]);
		}
		else
		{
			if(errorlog) fprintf(errorlog,"taito controller source stream'%s' was not found",sname);
			return 1;
		}
	}
	return 0;
}

void EXVOLUME_sh_stop(void)
{
}

void EXVOLUME_set_base_volume(int num, int channel,int volume)
{
	struct ex_volume *vctrl = &controller[num];

	vctrl->pan[channel]     = (volume>>8)&0xff;
	vctrl->basevol[channel] = (volume)&0xff;
	stream_set_volume(vctrl->stream[channel],vctrl->basevol[channel]*vctrl->volume[channel]/255);
	stream_set_pan(vctrl->stream[channel],vctrl->pan[channel]);
}

#define EXVOLUME_WRITE(NUM,CHAN) \
{ \
	struct ex_volume *vctrl = &controller[(NUM)]; \
	vctrl->volume[(CHAN)] = data; \
	stream_set_volume(vctrl->stream[(CHAN)],vctrl->basevol[(CHAN)]*vctrl->volume[(CHAN)]/255); \
}

/* TAITO controller interface */
void EXVOLUME_write_CH0_0(int offset,int data) EXVOLUME_WRITE(0,0)
void EXVOLUME_write_CH1_0(int offset,int data) EXVOLUME_WRITE(0,1)
void EXVOLUME_write_CH0_1(int offset,int data) EXVOLUME_WRITE(1,0)
void EXVOLUME_write_CH1_1(int offset,int data) EXVOLUME_WRITE(1,1)
void EXVOLUME_write_CH0_2(int offset,int data) EXVOLUME_WRITE(2,0)
void EXVOLUME_write_CH1_2(int offset,int data) EXVOLUME_WRITE(2,1)
void EXVOLUME_write_CH0_3(int offset,int data) EXVOLUME_WRITE(3,0)
void EXVOLUME_write_CH1_3(int offset,int data) EXVOLUME_WRITE(3,1)

