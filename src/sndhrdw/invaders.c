/* invaders.c ********************************* updated: 1997-04-09 08:46 TT
 *
 * Author      : Tormod Tjaberg
 * Created     : 1997-04-09
 * Description : Sound routines for the 'invaders' games
 *
 * Note:
 * The samples were taken from Michael Strutt's (mstrutt@pixie.co.za)
 * excellent space invader emulator and converted to signed samples so
 * they would work under SEAL. The port info was also gleaned from
 * his emulator. These sounds should also work on all the invader games.
 *
 * The sounds are generated using output port 3 and 5
 *
 * Port 4:
 * bit 0=UFO  (repeats)       0.raw
 * bit 1=Shot                 1.raw
 * bit 2=Base hit             2.raw
 * bit 3=Invader hit          3.raw
 *
 * Port 5:
 * bit 0=Fleet movement 1     4.raw
 * bit 1=Fleet movement 2     5.raw
 * bit 2=Fleet movement 3     6.raw
 * bit 3=Fleet movement 4     7.raw
 * bit 4=UFO 2                8.raw
 */

#include "driver.h"

#define emulation_rate 11025



void invaders_sh_port3_w(int offset, int data)
{
	static unsigned char Sound = 0;


	if (Machine->samples == 0) return;

	if (data & 0x01 && ~Sound & 0x01 && Machine->samples->sample[0])
		osd_play_sample(0,Machine->samples->sample[0]->data,
				Machine->samples->sample[0]->length,
                                Machine->samples->sample[0]->smpfreq,
                                Machine->samples->sample[0]->volume,1);

	if (~data & 0x01 && Sound & 0x01)
		osd_stop_sample(0);

	if (data & 0x02 && ~Sound & 0x02 && Machine->samples->sample[1])
		osd_play_sample(1,Machine->samples->sample[1]->data,
				Machine->samples->sample[1]->length,
                                Machine->samples->sample[1]->smpfreq,
                                Machine->samples->sample[1]->volume,0);

	if (~data & 0x02 && Sound & 0x02)
		osd_stop_sample(1);

	if (data & 0x04 && ~Sound & 0x04 && Machine->samples->sample[2])
		osd_play_sample(2,Machine->samples->sample[2]->data,
				Machine->samples->sample[2]->length,
                                Machine->samples->sample[2]->smpfreq,
                                Machine->samples->sample[2]->volume,0);

	if (~data & 0x04 && Sound & 0x04)
		osd_stop_sample(2);

	if (data & 0x08 && ~Sound & 0x08 && Machine->samples->sample[3])
		osd_play_sample(3,Machine->samples->sample[3]->data,
				Machine->samples->sample[3]->length,
                                Machine->samples->sample[3]->smpfreq,
                                Machine->samples->sample[3]->volume,0);

	if (~data & 0x08 && Sound & 0x08)
		osd_stop_sample(3);

	Sound = data;
}


void invaders_sh_port5_w(int offset, int data)
{
	static unsigned char Sound = 0;


	if (Machine->samples == 0) return;

	if (data & 0x01 && ~Sound & 0x01 && Machine->samples->sample[4])
		osd_play_sample(4,Machine->samples->sample[4]->data,
				Machine->samples->sample[4]->length,
                                Machine->samples->sample[4]->smpfreq,
                                Machine->samples->sample[4]->volume,0);

	if (data & 0x02 && ~Sound & 0x02 && Machine->samples->sample[5])
		osd_play_sample(4,Machine->samples->sample[5]->data,
				Machine->samples->sample[5]->length,
                                Machine->samples->sample[5]->smpfreq,
                                Machine->samples->sample[5]->volume,0);

	if (data & 0x04 && ~Sound & 0x04 && Machine->samples->sample[6])
		osd_play_sample(4,Machine->samples->sample[6]->data,
				Machine->samples->sample[6]->length,
                                Machine->samples->sample[6]->smpfreq,
                                Machine->samples->sample[6]->volume,0);

	if (data & 0x08 && ~Sound & 0x08 && Machine->samples->sample[7])
		osd_play_sample(4,Machine->samples->sample[7]->data,
				Machine->samples->sample[7]->length,
                                Machine->samples->sample[7]->smpfreq,
                                Machine->samples->sample[7]->volume,0);

	if (data & 0x10 && ~Sound & 0x10 && Machine->samples->sample[8])
		osd_play_sample(5,Machine->samples->sample[8]->data,
				Machine->samples->sample[8]->length,
                                Machine->samples->sample[8]->smpfreq,
                                Machine->samples->sample[8]->volume,0);

	if (~data & 0x10 && Sound & 0x10)
		osd_stop_sample(5);

	Sound = data;
}



void invaders_sh_update(void)
{
}
