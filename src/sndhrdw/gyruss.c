#include "driver.h"
#include "Z80.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"


#define emulation_rate 22050

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
	int clockticks,clock;
	Z80_Regs regs;

#define TIMER_RATE (570)

	clockticks = (Z80_IPeriod - cpu_geticount());
	clock = clockticks / TIMER_RATE;

	/* to speed up the emulation, detect when the program is looping waiting */
	/* for the timer, and skip the necessary CPU cycles in that case */
	Z80_GetRegs(&regs);
	if (RAM[regs.SP.D] == 0x01 && RAM[regs.SP.D+1] == 0x01)
	{
		/* wait until clock & 0x04 == 0 */
		if ((clock & 0x04) != 0)
		{
			clock = clock + 4;
			clockticks = clock * TIMER_RATE;
			cpu_seticount(Z80_IPeriod - clockticks);
		}
	}
	else if (RAM[regs.SP.D] == 0x08 && RAM[regs.SP.D+1] == 0x01)
	{
		/* wait until clock & 0x04 != 0 */
		if ((clock & 0x04) == 0)
		{
			clock = (clock + 4) & ~3;
			clockticks = clock * TIMER_RATE;
			cpu_seticount(Z80_IPeriod - clockticks);
		}
	}

	return clock;
}



int gyruss_sh_interrupt(void)
{
	if (pending_commands) return 0xff;
	else return Z80_IGNORE_INT;
}



static struct AY8910interface interface =
{
	5,	/* 5 chips */
	1789772727,	/* 1.789772727 MHZ */
	{ 255, 255, 255, 255, 255 },
	{ 0, 0, gyruss_portA_r },
	{ },
	{ },
	{ }
};



int gyruss_sh_start(void)
{
	pending_commands = 0;

	return AY8910_sh_start(&interface);
}
