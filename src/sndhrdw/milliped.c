#include "driver.h"
#include "pokey.h"



#define UPDATES_PER_SECOND 60
#define emulation_rate (FREQ_17_APPROX / (28*4))
#define buffer_len (emulation_rate/UPDATES_PER_SECOND)



int milliped_sh_start(void)
{
	Pokey_sound_init(FREQ_17_APPROX,emulation_rate,2);

	return 0;
}



void milliped_sh_stop(void)
{
}



void milliped_pokey1_w(int offset,int data)
{
	Update_pokey_sound(offset,data,0,16);
}



void milliped_pokey2_w(int offset,int data)
{
	Update_pokey_sound(offset,data,1,16);
}



void milliped_sh_update(void)
{
	unsigned char buffer[buffer_len];
	static int playing;


	if (play_sound == 0) return;

	Pokey_process(buffer,buffer_len);
	osd_play_streamed_sample(0,buffer,buffer_len,emulation_rate,255);
        if (!playing)
        {
	        osd_play_streamed_sample(0,buffer,buffer_len,emulation_rate,255);
	        osd_play_streamed_sample(0,buffer,buffer_len,emulation_rate,255);
	        playing = 1;
        }
}
