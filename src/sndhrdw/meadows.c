/***************************************************************************
	meadows.c
	Sound handler
	Dead Eye, Gypsy Juggler

	J. Buchmueller, June '98
****************************************************************************/

#include "driver.h"
#include "S2650/s2650.h"

byte meadows_0c00 = 0;
byte meadows_0c01 = 0;
byte meadows_0c02 = 0;
byte meadows_0c03 = 0;
byte meadows_dac  = 0;

#define BASE_CLOCK		5000000
#define BASE_CTR1       (BASE_CLOCK / 256)
#define BASE_CTR2		(BASE_CLOCK / 32 / 2)

#define DIVIDE_CTR2     0x01
#define ENABLE_CTR2     0x02
#define ENABLE_DAC      0x04
#define ENABLE_CTR1     0x08

static	int wavelen;
static	int channel[2];
static	signed char * waveform[2];

/************************************/
/* Sound handler init				*/
/************************************/
int meadows_sh_init(const char *gamename)
{
	channel[0] = get_play_channels(1);
	channel[1] = get_play_channels(1);
    return 0;
}

/************************************/
/* Sound handler start				*/
/************************************/
int meadows_sh_start(void)
{
int max_length;

    /* build the wave form length based on the sample rate */
	wavelen = Machine->sample_rate / Machine->drv->frames_per_second;
    if (errorlog) fprintf(errorlog, "meadows wavelen %d\n", wavelen);

	waveform[0] = malloc(wavelen);
	if (!waveform[0])
		return 1;

	waveform[1] = malloc(wavelen);
	if (!waveform[1])
        return 1;
	if (errorlog) fprintf(errorlog, "meadows wavelen %d\n", wavelen);

    return 0;
}

/************************************/
/* Sound handler stop				*/
/************************************/
void meadows_sh_stop(void)
{
	free(waveform[1]);
	free(waveform[0]);
    osd_stop_sample(channel[1]);
	osd_stop_sample(channel[0]);
}

/************************************/
/* Sound handler update 			*/
/************************************/
void meadows_sh_update(void)
{
static	byte latched_dac  = 0;
static  byte latched_0c01 = 0;
static	byte latched_0c02 = 0;
static	byte latched_0c03 = 0;
int freq, amp;

    if (latched_0c01 != meadows_0c01 || latched_0c03 != meadows_0c03)
	{
		if ((meadows_0c03 & ENABLE_CTR1) != 0)
		{
		int i, n, data, preset;
			/* amplitude is a combination of the upper 4 bits of 0c01 */
			/* and bit 4 merged from S2650's flag output */
			amp = (meadows_0c01 & 0xf0) >> 1;
			if (S2650_get_flag())
				amp += 128;
            /* calculate frequency for counter #1 */
			/* bit 0..3 of 0c01 are ctr preset */
			preset = (meadows_0c01 & 0x0f) + 1;
			/* build the waveform based on sample rate */
			for (i = 0, n = BASE_CTR1 / preset; i < wavelen; i++)
			{
				waveform[0][i] = i;
				if ((n -= Machine->sample_rate) < 0)
				{
					n += BASE_CTR1 / preset;
					data = -data;
				}
			}
			osd_stop_sample(channel[0]);
            osd_play_sample(channel[0], waveform[0], wavelen, Machine->sample_rate, amp, 0);
		}
    }

	if (latched_0c02 != meadows_0c02 || latched_0c03 != meadows_0c03)
	{
		if ((meadows_0c03 & ENABLE_CTR2) == 0)
		{
		int i, n, data, preset;
            /* calculate frequency for counter #2 */
			/* 0c02 is ctr preset, 0c03 bit 0 enables division by 2 */
			preset = meadows_0c02 + 1;
            amp = 255;
			if (meadows_0c03 & DIVIDE_CTR2)
				preset *= 2;
			/* build the waveform based on sample rate */
			for (i = 0, n = BASE_CTR2 / preset; i < wavelen; i++)
			{
				waveform[1][i] = i;
				if ((n -= Machine->sample_rate) < 0)
				{
					n += BASE_CTR2 / preset;
					data = -data;
				}
			}
			osd_stop_sample(channel[1]);
			osd_play_sample(channel[1], waveform[1], wavelen, Machine->sample_rate, amp, 0);
        }
    }

	if (latched_dac != meadows_dac || latched_0c03 != meadows_0c03)
    {
		amp = meadows_dac;
		if ((meadows_0c03 & ENABLE_DAC) == 0)
			amp = 0;
        DAC_data_w(0, amp);
    }

	latched_dac  = meadows_dac;
    latched_0c01 = meadows_0c01;
	latched_0c02 = meadows_0c02;
	latched_0c03 = meadows_0c03;
}

/************************************/
/* Write DAC value					*/
/************************************/
void meadows_sh_dac_w(int data)
{
	meadows_dac = data + 0x80;
    meadows_sh_update();
}


