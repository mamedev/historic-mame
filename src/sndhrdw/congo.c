#include "driver.h"
#include "sn76496.h"
#include <math.h>


#define SND_CLOCK1 4000000       /* 4 Mhz */
#define SND_CLOCK2 4000000       /* 4 Mhz */
#define CHIPS 2


#define buffer_len 350
#define emulation_rate (buffer_len*Machine->drv->frames_per_second)


static char *tone;
static unsigned char mute ;
static struct SN76496 sn[CHIPS];


void congo_sound1_w(int offset,int data)
{
        SN76496Write(&sn[0],data);
}

void congo_sound2_w(int offset,int data)
{
        SN76496Write(&sn[1],data);
}

void congo_daio(int offset, int data)
{
        if((offset == 1))
        {
        if(data&2)
         if (Machine->samples->sample[0] != 0){
                osd_play_sample(12,Machine->samples->sample[0]->data,
                Machine->samples->sample[0]->length,
                Machine->samples->sample[0]->smpfreq,
                Machine->samples->sample[0]->volume,0);
                }

        }
        else
        {


        if(offset==2)
        {
                mute = data&0x80;
                data ^= 0xff;
                if(!mute)
                {
                if(data&8)
                if (Machine->samples->sample[1] != 0){
                osd_play_sample(12,Machine->samples->sample[1]->data,
                Machine->samples->sample[1]->length,
                Machine->samples->sample[1]->smpfreq,
                Machine->samples->sample[1]->volume,0);
                }
                if(data&4)
                if (Machine->samples->sample[2] != 0){
                osd_play_sample(13,Machine->samples->sample[2]->data,
                Machine->samples->sample[2]->length,
                Machine->samples->sample[2]->smpfreq,
                Machine->samples->sample[2]->volume,0);
                }
                if(data&2)
                if (Machine->samples->sample[3] != 0){
                osd_play_sample(14,Machine->samples->sample[3]->data,
                Machine->samples->sample[3]->length,
                Machine->samples->sample[3]->smpfreq,
                Machine->samples->sample[3]->volume,0);
                }
                if(data&1)
                if (Machine->samples->sample[4] != 0){
                osd_play_sample(15,Machine->samples->sample[4]->data,
                Machine->samples->sample[4]->length,
                Machine->samples->sample[4]->smpfreq,
                Machine->samples->sample[4]->volume,0);
                }
                }
       }
       }
}

int congo_sh_start(void)
{
        int j;


        if ((tone = malloc(buffer_len)) == 0)
                return 1;

        for (j = 0;j < CHIPS;j++)
	{
                sn[j].Clock = SND_CLOCK1;
                SN76496Reset(&sn[j]);

	}
        mute = 0;
	return 0;
}



void congo_sh_stop(void)
{
	free(tone);
}



void congo_sh_update(void)
{
        int j;


	if (play_sound == 0) return;

	for (j = 0;j < CHIPS;j++)
	{

                SN76496UpdateB(&sn[j] , emulation_rate , tone , buffer_len );
                osd_play_streamed_sample(j,tone,buffer_len,emulation_rate,255 );

        }
}
