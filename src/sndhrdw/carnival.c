/* carnival.c ********************************* updated: 1997-07-23
 *
 * Author      : Mike Coates (based on Invaders by Tormod Tjaberg)
 * Created     : 1997-07-23
 * Description : Sound routines for the carnival
 *
 * Note:
 * The samples on which these effects are based were made by Virtual-Al
 *
 * There is also a second gun sound, but although I assume that it is
 * used on the bear screen, nothing seems to be altered to select it!
 *
 * Port 1:
 * bit 0 = fire gun
 * bit 1 = hit object     	* See Port 2
 * bit 2 = duck 1
 * bit 3 = duck 2
 * bit 4 = duck 3
 * bit 5 = hit pipe
 * bit 6 = bonus
 * bit 7 = hit background
 *
 * Port 2:
 * bit 0 = ?
 * bit 1 = ?
 * bit 2 = Switch effect for hit object
 * bit 3 = Music On/Off
 * bit 4 = ?
 * bit 5 = ?
 */

#include "driver.h"

int SoundFX;

void carnival_sh_port1_w(int offset, int data)
{
	static unsigned char Sound = 0;

	if (Machine->samples == 0) return;

    data = ~data;

 	if (errorlog) fprintf(errorlog,"port 0 : %02x and %02x\n",data,SoundFX);

	if (data & 0x01 && ~Sound & 0x01 && Machine->samples->sample[0])
		osd_play_sample(0,Machine->samples->sample[0]->data,
				          Machine->samples->sample[0]->length,
                          Machine->samples->sample[0]->smpfreq,
                          Machine->samples->sample[0]->volume,0);

	if (data & 0x02 && ~Sound & 0x02)
	{
        if (SoundFX & 0x04 && Machine->samples->sample[1])
 		    osd_play_sample(1,Machine->samples->sample[1]->data,
				              Machine->samples->sample[1]->length,
                              Machine->samples->sample[1]->smpfreq,
                              Machine->samples->sample[1]->volume,0);
        else
            if (Machine->samples->sample[9])
		        osd_play_sample(1,Machine->samples->sample[9]->data,
				                  Machine->samples->sample[9]->length,
                                  Machine->samples->sample[9]->smpfreq,
                                  Machine->samples->sample[9]->volume,0);
    }

	if (data & 0x04 && ~Sound & 0x04 && Machine->samples->sample[2])
		osd_play_sample(2,Machine->samples->sample[2]->data,
				          Machine->samples->sample[2]->length,
                          Machine->samples->sample[2]->smpfreq,
                          Machine->samples->sample[2]->volume,1);

  	if (~data & 0x04 && Sound & 0x04)
  		osd_stop_sample(2);


	if (data & 0x08 && ~Sound & 0x08 && Machine->samples->sample[3])
		osd_play_sample(3,Machine->samples->sample[3]->data,
				          Machine->samples->sample[3]->length,
                          Machine->samples->sample[3]->smpfreq,
                          Machine->samples->sample[3]->volume,1);

  	if (~data & 0x08 && Sound & 0x08)
  		osd_stop_sample(3);


	if (data & 0x10 && ~Sound & 0x10 && Machine->samples->sample[4])
		osd_play_sample(4,Machine->samples->sample[4]->data,
				          Machine->samples->sample[4]->length,
                          Machine->samples->sample[4]->smpfreq,
                          Machine->samples->sample[4]->volume,1);

  	if (~data & 0x10 && Sound & 0x10)
  		osd_stop_sample(4);


	if (data & 0x20 && ~Sound & 0x20 && Machine->samples->sample[5])
		osd_play_sample(5,Machine->samples->sample[5]->data,
				          Machine->samples->sample[5]->length,
                          Machine->samples->sample[5]->smpfreq,
                          Machine->samples->sample[5]->volume,0);

	if (data & 0x40 && ~Sound & 0x40 && Machine->samples->sample[6])
		osd_play_sample(6,Machine->samples->sample[6]->data,
				          Machine->samples->sample[6]->length,
                          Machine->samples->sample[6]->smpfreq,
                          Machine->samples->sample[6]->volume,0);

	if (data & 0x80 && ~Sound & 0x80 && Machine->samples->sample[7])
		osd_play_sample(7,Machine->samples->sample[7]->data,
 						  Machine->samples->sample[7]->length,
                		  Machine->samples->sample[7]->smpfreq,
                		  Machine->samples->sample[7]->volume,0);

	Sound = data;
}


void carnival_sh_port2_w(int offset, int data)
{
    SoundFX = data;
}


void carnival_sh_update(void)
{
}
