#include "driver.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"



static unsigned char samplenumber = 0;


void gyruss_sh_soundfx_on_w(int offset, int data)
{
        static unsigned char soundon = 0;

	if (Machine->samples == 0) return;
/*
        if (errorlog) fprintf(errorlog, "SoundON:  %d\n", data);
*/
        if (data) {
          if (Machine->samples->sample[data-1])
            osd_play_sample(7,Machine->samples->sample[data-1]->data,
                          Machine->samples->sample[data-1]->length,
                          Machine->samples->sample[data-1]->smpfreq,
                          Machine->samples->sample[data-1]->volume,0);
        }
        else osd_stop_sample(7);

        soundon = data;
}

void gyruss_sh_soundfx_data_w(int offset, int data)
{
/*
        if (errorlog) fprintf(errorlog, "SoundDATA:  %d\n", data);
*/
        samplenumber = data;
}



static int gyruss_portA_r(int offset)
{
	#define TIMER_RATE (570)

	return cpu_gettotalcycles() / TIMER_RATE;


#if 0	/* temporarily removed */
	/* to speed up the emulation, detect when the program is looping waiting */
	/* for the timer, and skip the necessary CPU cycles in that case */
	if (cpu_getreturnpc() == 0x0101)
	{
		/* wait until clock & 0x04 == 0 */
		if ((clock & 0x04) != 0)
		{
//			clock = clock + 0x04;
clock = (clock - 0x04) | 0x03;
			clockticks = clock * TIMER_RATE;
//			cpu_seticount(Z80_IPeriod - clockticks);
cpu_seticount(clockticks);
		}
	}
	else if (cpu_getreturnpc() == 0x0108)
	{
		/* wait until clock & 0x04 != 0 */
		if ((clock & 0x04) == 0)
		{
//			clock = (clock + 0x04) & ~0x03;
clock = (clock - 0x04) | 0x03;
			clockticks = clock * TIMER_RATE;
//			cpu_seticount(Z80_IPeriod - clockticks);
cpu_seticount(clockticks);
		}
	}
#endif
}



int gyruss_sh_interrupt(void)
{
	AY8910_update();

	if (pending_commands) return interrupt();
	else return ignore_interrupt();
}



static struct AY8910interface interface =
{
	5,	/* 5 chips */
	10,	/* 10 updates per video frame (good quality) */
	1789772727,	/* 1.789772727 MHZ */
	{ 255, 255, 255, 255, 255 },
	{ 0, 0, gyruss_portA_r },
	{ 0 },
	{ 0 },
	{ 0 }
};



int gyruss_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
