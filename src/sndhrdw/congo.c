#include "driver.h"
#include "generic.h"
#include "sn76496.h"



static int mute;



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



static struct SN76496interface interface =
{
	2,	/* 2 chips */
	4000000,	/* 4 Mhz? */
	{ 255, 255 }
};



int congo_sh_start(void)
{
	mute = 0;
	pending_commands = 0;

	return SN76496_sh_start(&interface);
}
