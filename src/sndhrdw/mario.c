#include "driver.h"
#include "sndhrdw\generic.h"
#include "I8039.h"
#include "dac.h"

static unsigned char marioDAC;
unsigned char *mario_dac = &marioDAC;

static struct DACinterface DAinterface =
{
	1,
	441000,
	{255,255 },
	{  1,  1 }
};

/* #define HARDWARE_SOUND */

/*  This define is for the few that can plug a real mario sound
 *  board on the parallel port of their PC.
 *  You will have to recompile MAME as there is no command line option
 *  to enable that feature.
 *
 *
 *      You have to plug it correctly:
 *       Parallel  Sound board (J3)
 *     Pin  2           3
 *     Pin  3           2
 *     Pin  4           5
 *     Pin  5           4
 *     Pin  6           7
 *     Pin  7           6
 *
 *     Pin  18         GND (Any ground pin on the board)
 */



#ifdef HARDWARE_SOUND

/* Put the port of your parallel port here. */

#define Parallel 0x378
#endif


void mario_sh_w(int offset,int data)
{
#ifdef HARDWARE_SOUND
	outp(Parallel,data);
	return;
#endif

	soundlatch_w(0,data);
	cpu_cause_interrupt(1,I8039_EXT_INT);
}



void mario_digital_out_w(int offset,int data)
{
	*mario_dac = data;
	DAC_data_w(0,data);
}



int mario_sh_start(void)
{
	if( DAC_sh_start(&DAinterface) ) return 1;
	return 0;
}

void mario_sh_stop(void)
{
	DAC_sh_stop();
}

void mario_sh_update(void)
{
	DAC_sh_update();
}

#define M_osd_play_sample(chan,num,loop) { \
	if (Machine->samples->sample[num] != 0) \
		osd_play_sample(chan,Machine->samples->sample[num]->data, \
			Machine->samples->sample[num]->length, \
			Machine->samples->sample[num]->smpfreq, \
			Machine->samples->sample[num]->volume,loop); }\

void mario_sh1_w(int offset,int data)
{
	static state[8];


	if (Machine->samples == 0) return;

	if (state[offset] != data)
	{
		if ((Machine->samples->sample[offset] != 0) && (data))
			osd_play_sample(10,Machine->samples->sample[offset]->data,
					Machine->samples->sample[offset]->length,
                                        Machine->samples->sample[offset]->smpfreq,
                                        Machine->samples->sample[offset]->volume,0);

		state[offset] = data;
	}
}


void mario_sh2_w(int offset,int data)
{
	static last;


	if (Machine->samples == 0) return;

/*	if (last != data)*/
	{
		last = data;

		switch (data & 0xf0) {
			case 0x10:
				M_osd_play_sample(8,8,0);
				break;
			case 0x20:
				M_osd_play_sample(8,9,0);
				break;
			case 0x40:
				M_osd_play_sample(8,10,0);
				break;
			case 0x80:
				M_osd_play_sample(8,11,0);
				break;
			}

		data &= 0x0f;
		switch (data)
		{
			/* these samples are longer - play on diff channel */
			case 0x08:
			case 0x09:
			case 0x0a:
			case 0x0d:
				if (Machine->samples->sample[data+11] != 0)
				{
					osd_play_sample(9,Machine->samples->sample[data+11]->data,
						Machine->samples->sample[data+11]->length,
						Machine->samples->sample[data+11]->smpfreq,
						Machine->samples->sample[data+11]->volume,0);
				}
				break;

			default:
				if (data != 0)
					if (Machine->samples->sample[data+11] != 0)
					{
						osd_play_sample(10,Machine->samples->sample[data+11]->data,
							Machine->samples->sample[data+11]->length,
							Machine->samples->sample[data+11]->smpfreq,
							Machine->samples->sample[data+11]->volume,0);
					}
				break;
		}
	}
}



void mario_sh3_w(int offset,int data)
{
        static last;

	if (Machine->samples == 0) return;

        if (last!= data)
	{
                last = data;
		if (data)
		{
                        if (Machine->samples->sample[27])
              osd_play_sample(11,Machine->samples->sample[27]->data,
                                Machine->samples->sample[27]->length,
                                Machine->samples->sample[27]->smpfreq,
                                Machine->samples->sample[27]->volume,0);
		}

	}

}

