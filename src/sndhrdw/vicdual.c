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

    data = ~data;

 	if (errorlog) fprintf(errorlog,"port 0 : %02x and %02x\n",data,SoundFX);

	if (data & 0x01 && ~Sound & 0x01)
		sample_start(0,0,0);

	if (data & 0x02 && ~Sound & 0x02)
	{
        if (SoundFX & 0x04)
			sample_start(1,1,0);
        else
           	sample_start(1,9,0);
    }

	if (data & 0x04 && ~Sound & 0x04)
		sample_start(2,2,1);

  	if (~data & 0x04 && Sound & 0x04)
  		sample_stop(2);

	if (data & 0x08 && ~Sound & 0x08)
		sample_start(3,3,1);

  	if (~data & 0x08 && Sound & 0x08)
  		sample_stop (3);

	if (data & 0x10 && ~Sound & 0x10)
		sample_start(4,4,1);

  	if (~data & 0x10 && Sound & 0x10)
  		sample_stop(4);

	if (data & 0x20 && ~Sound & 0x20)
		sample_start(5,5,0);

	if (data & 0x40 && ~Sound & 0x40)
		sample_start(6,6,0);

	if (data & 0x80 && ~Sound & 0x80)
		sample_start(7,7,0);

	Sound = data;
}


void carnival_sh_port2_w(int offset, int data)
{
    SoundFX = data;
}
