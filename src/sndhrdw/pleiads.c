/* Sound hardware for Pleiades, Naughty Boy and Pop Flamer. Note that it's very */
/* similar to Phoenix. */

/* some of the missing sounds are now audible,
   but there is no modulation of any kind yet.
   Andrew Scott (ascott@utkux.utcc.utk.edu) */

#include "driver.h"

#define VMIN	0
#define VMAX	32767

static int channel;

static int sound_latch_a;
static int sound_latch_b;

static int tone_level;

static UINT32 *poly18 = NULL;

INLINE int tone(int samplerate)
{
	static int counter, divisor, output;
	int frequency = 11075;

	if( (sound_latch_a & 15) != 15 )
	{
		counter -= frequency;
		while( counter <= 0 )
		{
			counter += samplerate;
			if( ++divisor == 16 )
			{
				divisor = sound_latch_a & 15;
				output ^= 1;
			}
		}
	}
	return output ? tone_level / 2 : -tone_level / 2;
}

INLINE int update_c24(int samplerate)
{
	static int counter, level;
	/*
	 * Noise frequency control (Port B):
	 * Bit 6 lo charges C24 (6.8u) via R51 (330) and when
	 * bit 6 is hi, C24 is discharged through R52 (20k)
	 * in approx. 20000 * 6.8e-6 = 0.136 seconds
	 */
	#define C24 6.8e-6
	#define R49 1000
    #define R51 330
	#define R52 20000
	if( sound_latch_a & 0x40 )
	{
		counter -= (int)((VMAX - level) / ((R51+R49) * C24));
		if( counter <= 0 )
        {
            int n = -counter / samplerate + 1;
            counter += n * samplerate;
			if( (level += n) > VMAX)
				level = VMAX;
        }
    }
	else
	{
		counter -= (int)(level / (R52 * C24));
		if( counter <= 0 )
        {
            int n = -counter / samplerate + 1;
            counter += n * samplerate;
			if( (level -= n) < VMIN)
				level = VMIN;
        }
    }
	return VMAX - level;
}

INLINE int update_c25(int samplerate)
{
	static int counter, level;
	/*
	 * Bit 7 hi charges C25 (6.8u) over a R53 (330) and when
	 * bit 7 is lo, C25 is discharged through R54 (47k)
	 * in about 47000 * 6.8e-6 = 0.3196 seconds
	 */
	#define C25 6.8e-6
	#define R50 1000
    #define R53 330
	#define R54 47000

	if( sound_latch_a & 0x80 )
	{
		counter -= (int)((VMAX - level) / ((R50+R53) * C25));
		if( counter <= 0 )
		{
			int n = -counter / samplerate + 1;
			counter += n * samplerate;
			if( (level += n) > VMAX )
				level = VMAX;
		}
	}
	else
	{
        counter -= (int)(level / (R54 * C25));
		if( counter <= 0 )
		{
			int n = -counter / samplerate + 1;
			counter += n * samplerate;
			if( (level -= n) < VMIN )
				level = VMIN;
		}
	}
	return level;
}


INLINE int noise(int samplerate)
{
	static int counter, polyoffs, polybit, lowpass_counter, lowpass_polybit;
	int vc24 = update_c24(samplerate);
	int vc25 = update_c25(samplerate);
	int sum = 0, level, frequency;

	/*
	 * The voltage levels are added and control I(CE) of transistor TR1
	 * (NPN) which then controls the noise clock frequency (linearily?).
	 * level = voltage at the output of the op-amp controlling the noise rate.
	 */
	if( vc24 < vc25 )
		level = vc24 + (vc25 - vc24);
	else
		level = vc25 + (vc24 - vc25);

	frequency = 6325 - (6325-588) * level / 32768;

    /*
	 * NE555: Ra=47k, Rb=1k, C=0.05uF
	 * minfreq = 1.44 / ((47000+2*1000) * 0.05e-6) = approx. 588 Hz
	 * R71 (2700 Ohms) parallel to R73 (47k Ohms) = approx. 2553 Ohms
	 * maxfreq = 1.44 / ((2553+2*1000) * 0.05e-6) = approx. 6325 Hz
	 */
	counter -= frequency;
	if( counter <= 0 )
	{
		int n = (-counter / samplerate) + 1;
		counter += n * samplerate;
		polyoffs = (polyoffs + n) & 0x3ffff;
		polybit = (poly18[polyoffs>>5] >> (polyoffs & 31)) & 1;
	}
	if (!polybit)
		sum += VMAX - vc24;

	/* 400Hz crude low pass filter: only a guess */
	lowpass_counter -= 400;
	if( lowpass_counter <= 0 )
	{
		lowpass_counter += samplerate;
		lowpass_polybit = polybit;
	}
	if (!lowpass_polybit)
		sum += vc25;

	return sum;
}

static void pleiads_sound_update(int param, INT16 *buffer, int length)
{
	int samplerate = Machine->sample_rate;

	while( length-- > 0 )
	{
		int sum = tone(samplerate) + noise(samplerate);
		sum /= 2;
		*buffer++ = sum < 32768 ? sum > -32768 ? sum : -32768 : 32767;
	}
}

void pleiads_sound_control_a_w (int offset,int data)
{
    /* (data & 0x10), (data & 0x20), other analog stuff which I don't understand */

	stream_update(channel,0);
	sound_latch_a = data;

	/* (data & 0x10), (data & 0x20), other analog stuff which I don't understand */

    /* maybe bits 4 + 5 are volume control? */
	tone_level = VMAX / 2 + VMAX / 2 * ((data >> 4) & 3) / 4;
}

void pleiads_sound_control_b_w (int offset,int data)
{
	static int tms3617_chip;

	/*
     * pitch selects one of 4 possible clock inputs
     * (actually 3, because IC2 and IC3 are tied together)
     * write note value to TMS3615; voice b1 & b2
     */
	tms3617_note_w(tms3617_chip, (data >> 6) & 3, data & 15);

    /* next time next chip (I guess there's a flip-flop?) */
	tms3617_chip ^= 1;

    if (data == sound_latch_b)
		return;

    /* (data & 0x30) goes to a 556 */

    stream_update(channel,0);
    sound_latch_b = data;
}

int pleiads_sh_start(const struct MachineSound *msound)
{
	int i, j;
	UINT32 shiftreg;

	poly18 = (UINT32 *)malloc((1ul << (18-5)) * sizeof(UINT32));

	if( !poly18 )
		return 1;

    shiftreg = 0;
	for( i = 0; i < (1ul << (18-5)); i++ )
	{
		UINT32 bits = 0;
		for( j = 0; j < 32; j++ )
		{
			bits = (bits >> 1) | (shiftreg << 31);
			if( ((shiftreg >> 16) & 1) == ((shiftreg >> 17) & 1) )
				shiftreg = (shiftreg << 1) | 1;
			else
				shiftreg <<= 1;
		}
		poly18[i] = bits;
	}

	channel = stream_init("Custom", 50, Machine->sample_rate, 0, pleiads_sound_update);
	if( channel == -1 )
		return 1;

    return 0;
}

void pleiads_sh_stop(void)
{
	if( poly18 )
		free(poly18);
	poly18 = NULL;
}

void pleiads_sh_update(void)
{
	stream_update(channel, 0);
}


