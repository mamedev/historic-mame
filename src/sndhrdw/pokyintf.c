#include "driver.h"
#include "pokey.h"

#include "sndhrdw/pokyintf.h"

#define emulation_rate (FREQ_17_APPROX / (28*4))
#define buffer_len (emulation_rate/Machine->drv->frames_per_second)

static int numpokeys; /* Number of pokeys used by the driver, also used for clipping */
					  /* For best results, should be power of 2 */

int pokey1_sh_start (void)
{
	numpokeys = 1;

	Pokey_sound_init (FREQ_17_APPROX,emulation_rate,numpokeys);
	return 0;
}

int pokey2_sh_start (void)
{
	numpokeys = 2;

	Pokey_sound_init (FREQ_17_APPROX,emulation_rate,numpokeys);
	return 0;
}

int pokey4_sh_start (void)
{
	numpokeys = 4;

	Pokey_sound_init (FREQ_17_APPROX,emulation_rate,numpokeys);
	return 0;
}



void pokey_sh_stop (void)
{
}



int pokey1_r (int offset)
{
	return Read_pokey_regs (offset,0);
}

int pokey2_r (int offset)
{
	return Read_pokey_regs (offset,1);
}

int pokey3_r (int offset)
{
	return Read_pokey_regs (offset,2);
}

int pokey4_r (int offset)
{
	return Read_pokey_regs (offset,3);
}


void pokey1_w (int offset,int data)
{
	Update_pokey_sound (offset,data,0,16/numpokeys);
}

void pokey2_w (int offset,int data)
{
	Update_pokey_sound (offset,data,1,16/numpokeys);
}

void pokey3_w (int offset,int data)
{
	Update_pokey_sound (offset,data,2,16/numpokeys);
}

void pokey4_w (int offset,int data)
{
	Update_pokey_sound (offset,data,3,16/numpokeys);
}



void pokey_sh_update (void)
{
	unsigned char buffer[emulation_rate/30];	/* worst case */
	static int playing;


	if (play_sound == 0) return;

	Pokey_process (buffer,buffer_len);
	osd_play_streamed_sample (0,buffer,buffer_len,emulation_rate,255);
        if (!playing)
        {
	        osd_play_streamed_sample (0,buffer,buffer_len,emulation_rate,255);
	        osd_play_streamed_sample (0,buffer,buffer_len,emulation_rate,255);
	        playing = 1;
        }
}
