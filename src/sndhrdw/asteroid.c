#include <math.h>
#include "driver.h"

#define VMAX	32767
#define VMIN	0

#define SAUCEREN    0
#define SAUCRFIREEN 1
#define SAUCERSEL   2
#define THRUSTEN    3
#define SHIPFIREEN	4
#define LIFEEN		5

#define EXPITCH0	(1<<6)
#define EXPITCH1	(1<<7)
#define EXPAUDSHIFT 2
#define EXPAUDMASK	(0x0f<<EXPAUDSHIFT)

static int channel;
static int explosion_latch;
static int thump_latch;
static int sound_latch[8];

static int poly_counter;
static int poly_shifter;
static int poly_sample_count;

static int explosion_out;

static int thump_counter;
static int thump_frequency;
static int thump_out;

static int saucer_vco;
static int saucer_vco_counter;
static int saucer_vco_charge;
static int saucer_counter;
static int saucer_out;

static int thrust_counter;
static int thrust_amp;
static int thrust_out;

static int shipfire_amp;
static int shipfire_amp_counter;
static int shipfire_vco;
static int shipfire_vco_counter;
static int shipfire_counter;
static int shipfire_out;

static int saucerfire_amp;
static int saucerfire_amp_counter;
static int saucerfire_vco;
static int saucerfire_vco_counter;
static int saucerfire_counter;
static int saucerfire_out;

static int lifesound_counter;
static int lifesound_out;

static INT16 *discharge;
static INT16 vol_explosion[16];
#define EXP(charge,n) (charge ? 0x7fff - discharge[0x7fff-n] : discharge[n])

static int astdelux_thrusten;

static void asteroid_sound_update(int param, INT16 *buffer, int length)
{
	int samplerate = Machine->sample_rate;

	while( length-- > 0 )
	{
		int sum = 0;

		poly_counter -= 12000;
		while( poly_counter <= 0 )
		{
			poly_counter += samplerate;
			if( ((poly_shifter & 0x4000) == 0) == ((poly_shifter & 0x0040) == 0) )
				poly_shifter = (poly_shifter << 1) | 1;
			else
				poly_shifter <<= 1;
			if( ++poly_sample_count == 16 )
			{
				poly_sample_count = 0;
				if( explosion_latch & EXPITCH0 )
					poly_sample_count |= 2 + 8;
                else
					poly_sample_count |= 4;
				if( explosion_latch & EXPITCH1 )
					poly_sample_count |= 1 + 8;
			}
			/* ripple count output goes high? */
			if( poly_sample_count == 15 )
				explosion_out = poly_shifter & 1;
        }
		/* mixer 4.7K */
		if( explosion_out )
			sum += vol_explosion[(explosion_latch & EXPAUDMASK) >> EXPAUDSHIFT] / 7;

		if( sound_latch[THRUSTEN] )
		{
			/* SHPSND filter
			 * rumbling low noise... well, the filter parameters
			 * are beyond me. I implement a 110Hz digital filter
			 * on the poly noise and sweep the amplitude of the
			 * signal to the new level.
			 */
			thrust_counter -= 110;
			while( thrust_counter <= 0 )
			{
				thrust_counter += samplerate;
				thrust_out = poly_shifter & 1;
			}
			if( thrust_out )
			{
				if( thrust_amp < VMAX )
				{
					thrust_amp += (VMAX - thrust_amp) * samplerate / 32768 / 32;
					if( thrust_amp > VMAX )
						thrust_amp = VMAX;
				}
			}
			else
			{
				if( thrust_amp > VMIN )
				{
					thrust_amp -= thrust_amp * samplerate / 32768 / 32;
					if( thrust_amp < VMIN)
						thrust_amp = VMIN;
                }
            }
			sum += thrust_amp / 7;
        }

        if( thump_latch & 0x10 )
		{
			thump_counter -= thump_frequency;
			while( thump_counter <= 0 )
			{
				thump_counter += samplerate;
				thump_out ^= 1;
			}
			if( thump_out )
				sum += VMAX / 7;
		}

        /* saucer sound enabled ? */
		if( sound_latch[SAUCEREN] )
		{
			/* NE555 setup as astable multivibrator
			 * C = 10u, Ra = 5.6k, Rb = 10k
             * charge time = 0.693 * (5.6k + 10k) * 10u = 0.108108s
			 * discharge time = 0.693 * 10k * 10u = 0.0693s
			 * ---------
			 * C = 10u, Ra = 5.6k, Rb = 6k [1 / (1/10k + 1/15k)]
			 * charge time = 0.693 * (5.6k + 6k) * 10u = 0.0.080388s
			 * discharge time = 0.693 * 6k * 10u = 0.04158s
             */
			if( saucer_vco_charge )
			{
				if( sound_latch[SAUCERSEL] )
					saucer_vco_counter -= (int)(VMAX / 0.108108);
				else
					saucer_vco_counter -= (int)(VMAX / 0.080388);
				if( saucer_vco_counter < 0 )
				{
					int n = (-saucer_vco_counter / samplerate) + 1;
					saucer_vco_counter += n * samplerate;
					if( (saucer_vco += n) >= VMAX*2/3 )
					{
						saucer_vco = VMAX*2/3;
                        saucer_vco_charge = 0;
						break;
					}
				}
			}
			else
			{
				if( sound_latch[SAUCERSEL] )
					saucer_vco_counter -= (int)(VMAX / 0.0693);
				else
					saucer_vco_counter -= (int)(VMAX / 0.04158);
				if( saucer_vco_counter < 0 )
				{
					int n = (-saucer_vco_counter / samplerate) + 1;
					saucer_vco_counter += n * samplerate;
					if( (saucer_vco -= n) <= VMAX*1/3 )
					{
						saucer_vco = VMIN*1/3;
                        saucer_vco_charge = 1;
						break;
					}
                }
            }
			/*
			 * NE566 setup as voltage controlled astable multivibrator
			 * C = 0.001u, Ra = 5.6k, Rb = 3.9k
			 * frequency = 1.44 / ((5600 + 2*3900) * 0.047e-6) = 2286Hz
             */
			if( sound_latch[SAUCERSEL] )
				saucer_counter -= 2286*1/3 + (2286 * EXP(saucer_vco_charge,saucer_vco) / 32768);
			else
				saucer_counter -= 2286*1/3 + 800 /*???*/ + ((2286*1/3 + 800) * EXP(saucer_vco_charge,saucer_vco) / 32768);
			while( saucer_counter <= 0 )
			{
				saucer_counter += samplerate;
				saucer_out ^= 1;
			}
			if( saucer_out )
				sum += VMAX / 7;
        }

		if( sound_latch[SAUCRFIREEN] )
		{
			if( saucerfire_vco > VMIN )
			{
				/* charge C38 (10u) through R54 (10K) */
				#define C38_CHARGE_TIME (int)(VMAX / 6.93);
				saucerfire_vco_counter -= C38_CHARGE_TIME;
				while( saucerfire_vco_counter <= 0 )
				{
					saucerfire_vco_counter += samplerate;
					if( --saucerfire_vco == VMIN )
						break;
				}
			}
			if( saucerfire_amp > VMIN )
			{
				/* discharge C39 (10u) through R58 (10K) and CR6 */
				#define C39_DISCHARGE_TIME (int)(VMAX / 6.93);
				saucerfire_amp_counter -= C39_DISCHARGE_TIME;
				while( saucerfire_amp_counter <= 0 )
				{
					saucerfire_amp_counter += samplerate;
					if( --saucerfire_amp == VMIN )
						saucerfire_amp = VMIN;
						break;
                }
			}
			if( saucerfire_out )
			{
				/* C35 = 1u, Rb = 680 -> f = 8571Hz */
				saucerfire_counter -= 8571;
				while( saucerfire_counter <= 0 )
				{
					saucerfire_counter += samplerate;
					saucerfire_out = 0;
				}
			}
			else
			{
				if( saucerfire_vco > VMAX*1/3 )
				{
					/* C35 = 1u, Ra = 3.3k, Rb = 680 -> f = 522Hz */
					saucerfire_counter -= 522*1/3 + (522 * EXP(0,saucerfire_vco) / 32768);
					while( saucerfire_counter <= 0 )
					{
						saucerfire_counter += samplerate;
						saucerfire_out = 1;
					}
				}
			}
            if( saucerfire_out )
				sum += saucerfire_amp / 7;
		}
		else
		{
			/* charge C38 and C39 */
			saucerfire_amp = VMAX;
			saucerfire_vco = VMAX;
		}

		if( sound_latch[SHIPFIREEN] )
		{
			if( shipfire_vco > VMIN )
			{
#if 0	/* A typo in the schematics? 1u is too low */
               /* charge C47 (1u) through R52 (33K) */
				#define C47_CHARGE_TIME (int)(VMAX / 0.033);
#else
			   /* charge C47 (10u) through R52 (33K) */
				#define C47_CHARGE_TIME (int)(VMAX / 0.33);
#endif
                shipfire_vco_counter -= C47_CHARGE_TIME;
				while( shipfire_vco_counter <= 0 )
				{
					shipfire_vco_counter += samplerate;
					if( --shipfire_vco == VMIN )
						break;
				}
			}
			if( shipfire_amp > VMIN )
			{
#if 0
				/* A typo in the schematics? 1u is too low */
                /* discharge C48 (10u) through R66 (2.7K) and CR8 */
				#define C48_DISCHARGE_TIME (int)(VMAX / 0.027);
#else
				/* discharge C48 (10u) through R66 (2.7K) and CR8 */
				#define C48_DISCHARGE_TIME (int)(VMAX / 0.27);
#endif
                shipfire_amp_counter -= C48_DISCHARGE_TIME;
				while( shipfire_amp_counter <= 0 )
				{
					shipfire_amp_counter += samplerate;
					if( --shipfire_amp == VMIN )
						break;
                }
			}
			if( shipfire_out )
			{
				/* C50 = 1u, Rb = 680 -> f = 8571Hz */
				shipfire_counter -= 8571;
				while( shipfire_counter <= 0 )
				{
					shipfire_counter += samplerate;
					shipfire_out = 0;
				}
			}
			else
			{
				/* C50 = 1u, Ra = 3.3k, Rb = 680 -> f = 522Hz */
				if( shipfire_vco > VMAX*1/3 )
				{
					shipfire_counter -= 522*1/3 + (522 * EXP(0,shipfire_vco) / 32768);
					while( shipfire_counter <= 0 )
					{
						shipfire_counter += samplerate;
						shipfire_out = 1;
					}
				}
			}
			if( shipfire_out )
				sum += shipfire_amp / 7;
		}
		else
		{
			/* charge C47 and C48 */
			shipfire_amp = VMAX;
			shipfire_vco = VMAX;
		}

		if( sound_latch[LIFEEN] )
		{
			lifesound_counter -= 3000;
			while( lifesound_counter <= 0 )
			{
				lifesound_counter += samplerate;
				lifesound_out ^= 1;
			}
			if( lifesound_out )
				sum += VMAX / 7;
		}

        *buffer++ = sum;
	}
}

static void explosion_init(void)
{
	int i;

    for( i = 0; i < 16; i++ )
    {
        /* r0 = open, r1 = open */
        double r0 = 1.0/1e12, r1 = 1.0/1e12;

        /* R14 */
        if( i & 1 )
            r1 += 1.0/47000;
        else
            r0 += 1.0/47000;
        /* R15 */
        if( i & 2 )
            r1 += 1.0/22000;
        else
            r0 += 1.0/22000;
        /* R16 */
        if( i & 4 )
            r1 += 1.0/12000;
        else
            r0 += 1.0/12000;
        /* R17 */
        if( i & 8 )
            r1 += 1.0/5600;
        else
            r0 += 1.0/5600;
        r0 = 1.0/r0;
        r1 = 1.0/r1;
        vol_explosion[i] = VMAX * r0 / (r0 + r1);
    }

}

int asteroid_sh_start(const struct MachineSound *msound)
{
    int i;

	discharge = (INT16 *)malloc(32768 * sizeof(INT16));
	if( !discharge )
        return 1;

    for( i = 0; i < 0x8000; i++ )
		discharge[0x7fff-i] = (INT16) (0x7fff/exp(1.0*i/4096));

	/* initialize explosion volume lookup table */
	explosion_init();

    channel = stream_init("Custom", 100, Machine->sample_rate, 0, asteroid_sound_update);
    if( channel == -1 )
        return 1;

    return 0;
}

void asteroid_sh_stop(void)
{
	if( discharge )
		free(discharge);
	discharge = NULL;
}

void asteroid_sh_update(void)
{
	stream_update(channel, 0);
}


void asteroid_explode_w (int offset,int data)
{
	if( data == explosion_latch )
		return;

    stream_update(channel, 0);
	explosion_latch = data;
}



void asteroid_thump_w (int offset,int data)
{
	double r0 = 1/47000, r1 = 1/1e12;

    if( data == thump_latch )
		return;

    stream_update(channel, 0);
	thump_latch = data;

	if( thump_latch & 1 )
		r1 += 1.0/220000;
	else
		r0 += 1.0/220000;
	if( thump_latch & 2 )
		r1 += 1.0/100000;
	else
		r0 += 1.0/100000;
	if( thump_latch & 4 )
		r1 += 1.0/47000;
	else
		r0 += 1.0/47000;
	if( thump_latch & 8 )
		r1 += 1.0/22000;
	else
		r0 += 1.0/22000;

	/* NE555 setup as voltage controlled astable multivibrator
	 * C = 0.22u, Ra = 22k...???, Rb = 18k
	 * frequency = 1.44 / ((22k + 2*18k) * 0.22n) = 56Hz .. huh?
	 */
	thump_frequency = 56 + 56 * r0 / (r0 + r1);
}


void asteroid_sounds_w (int offset,int data)
{
	data &= 0x80;
    if( data == sound_latch[offset] )
		return;

    stream_update(channel, 0);
	sound_latch[offset] = data;
}



static void astdelux_sound_update(int param, INT16 *buffer, int length)
{
	int samplerate = Machine->sample_rate;

    while( length-- > 0)
	{
		int sum = 0;

        poly_counter -= 12000;
		while( poly_counter <= 0 )
		{
			poly_counter += samplerate;
			if( ((poly_shifter & 0x4000) == 0) == ((poly_shifter & 0x0040) == 0) )
				poly_shifter = (poly_shifter << 1) | 1;
			else
				poly_shifter <<= 1;
			if( ++poly_sample_count == 16 )
			{
				poly_sample_count = 0;
				if( explosion_latch & EXPITCH0 )
					poly_sample_count |= 2 + 8;
				else
					poly_sample_count |= 4;
				if( explosion_latch & EXPITCH1 )
					poly_sample_count |= 1 + 8;
			}
			/* ripple count output goes high? */
			if( poly_sample_count == 15 )
				explosion_out = poly_shifter & 1;
        }
		/* mixer 4.7K */
		if( explosion_out )
			sum += vol_explosion[(explosion_latch & EXPAUDMASK) >> EXPAUDSHIFT] / 2;


        if( astdelux_thrusten )
        {
            /* SHPSND filter
             * rumbling low noise... well, the filter parameters
			 * are beyond me. I implement a 110Hz digital filter
             * on the poly noise and sweep the amplitude of the
			 * signal to the new level.
             */
			thrust_counter -= 110;
            while( thrust_counter < 0 )
            {
                thrust_counter += samplerate;
                thrust_out = poly_shifter & 1;
            }
			if( thrust_out )
			{
				if( thrust_amp < VMAX )
				{
					thrust_amp += (VMAX - thrust_amp) * samplerate / 32768 / 32;
					if( thrust_amp > VMAX )
						thrust_amp = VMAX;
				}
			}
			else
			{
				if( thrust_amp > VMIN )
				{
					thrust_amp -= thrust_amp * samplerate / 32768 / 32;
					if( thrust_amp < VMIN )
						thrust_amp = VMIN;
                }
            }
			sum += thrust_amp / 2;
        }
		*buffer++ = sum;
	}
}

int astdelux_sh_start(const struct MachineSound *msound)
{
	/* initialize explosion volume lookup table */
	explosion_init();

    channel = stream_init("Custom", 50, Machine->sample_rate, 0, astdelux_sound_update);
    if( channel == -1 )
        return 1;

    return 0;
}

void astdelux_sh_stop(void)
{
}

void astdelux_sh_update(void)
{
	stream_update(channel, 0);
}


void astdelux_sounds_w (int offset,int data)
{
	data = (data & 0x80) ^ 0x80;
	if( data == astdelux_thrusten )
		return;
    stream_update(channel, 0);
	astdelux_thrusten = data;
}


