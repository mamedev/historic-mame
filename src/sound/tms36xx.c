#include "driver.h"

#define VERBOSE 1

#if VERBOSE
#define LOG(x) if( errorlog ) fprintf x
#else
#define LOG(x)
#endif

#define VMIN	0x0000
#define VMAX	0x7fff

/* the frequencies are later adjusted by "* clock / FSCALE" */
#define FSCALE	1024

struct TMS36XX {
	int channel;					/* returned by stream_init() */

	int samplerate; 				/* from Machine->sample_rate */

	int basefreq;					/* chip's base frequency */
	int octave; 					/* octave select of the TMS3615 */

    int speed;                      /* speed of the tune */
	int tune_counter;				/* tune counter */
	int note_counter;				/* note counter */

	int voices; 					/* active voices */
    int vol[6];                     /* (decaying) volume of harmonics notes */
	int vol_counter[6]; 			/* volume adjustment counter */
	int decay[6];					/* volume adjustment rate - dervied from decay */

	int counter[6]; 				/* tone frequency counter */
	int frequency[6];				/* tone frequency */
	int output; 					/* output signal bits */
	int enable; 					/* mask which harmoics */

    int tune_num;                   /* tune currently playing */
	int tune_ofs;					/* note currently playing */
	int tune_max;					/* end of tune */
};

struct TMS36XXinterface *intf;
struct TMS36XX *tms36xx[MAX_TMS36XX];

#define C(n)	(int)((FSCALE<<(n-1))*1.18921)	/* 2^(3/12) */
#define Cx(n)	(int)((FSCALE<<(n-1))*1.25992)	/* 2^(4/12) */
#define D(n)	(int)((FSCALE<<(n-1))*1.33484)	/* 2^(5/12) */
#define Dx(n)	(int)((FSCALE<<(n-1))*1.41421)	/* 2^(6/12) */
#define E(n)	(int)((FSCALE<<(n-1))*1.49831)	/* 2^(7/12) */
#define F(n)	(int)((FSCALE<<(n-1))*1.58740)	/* 2^(8/12) */
#define Fx(n)	(int)((FSCALE<<(n-1))*1.68179)	/* 2^(9/12) */
#define G(n)	(int)((FSCALE<<(n-1))*1.78180)	/* 2^(10/12) */
#define Gx(n)	(int)((FSCALE<<(n-1))*1.88775)	/* 2^(11/12) */
#define A(n)	(int)((FSCALE<<n))				/* A */
#define Ax(n)	(int)((FSCALE<<n)*1.05946)		/* 2^(1/12) */
#define B(n)	(int)((FSCALE<<n)*1.12246)		/* 2^(2/12) */

/*
 * Alarm sound?
 * It is unknown what this sound is like. Until somebody manages
 * trigger sound #1 of the Phoenix PCB sound chip I put just something
 * 'alarming' in here.
 */
static int tune1[96*6] = {
	C(3),	0,		0,		C(2),	0,		0,
	G(3),	0,		0,		0,		0,		0,
	C(3),	0,		0,		0,		0,		0,
	G(3),	0,		0,		0,		0,		0,
	C(3),	0,		0,		0,		0,		0,
	G(3),	0,		0,		0,		0,		0,
	C(3),	0,		0,		0,		0,		0,
	G(3),	0,		0,		0,		0,		0,
	C(3),	0,		0,		C(4),	0,		0,
	G(3),	0,		0,		0,		0,		0,
	C(3),	0,		0,		0,		0,		0,
	G(3),	0,		0,		0,		0,		0,
	C(3),	0,		0,		0,		0,		0,
	G(3),	0,		0,		0,		0,		0,
	C(3),	0,		0,		0,		0,		0,
	G(3),	0,		0,		0,		0,		0,
	C(3),	0,		0,		C(2),	0,		0,
	G(3),	0,		0,		0,		0,		0,
	C(3),	0,		0,		0,		0,		0,
	G(3),	0,		0,		0,		0,		0,
	C(3),	0,		0,		0,		0,		0,
	G(3),	0,		0,		0,		0,		0,
	C(3),	0,		0,		0,		0,		0,
	G(3),	0,		0,		0,		0,		0,
	C(3),	0,		0,		C(4),	0,		0,
	G(3),	0,		0,		0,		0,		0,
	C(3),	0,		0,		0,		0,		0,
	G(3),	0,		0,		0,		0,		0,
	C(3),	0,		0,		0,		0,		0,
	G(3),	0,		0,		0,		0,		0,
	C(3),	0,		0,		0,		0,		0,
	G(3),	0,		0,		0,		0,		0,
};

/*
 * Fuer Elise, Beethoven
 * (Excuse my non-existent musical skill, Mr. B ;-)
 */
static int tune2[96*6] = {
	D(3),	D(4),	D(5),	0,		0,		0,
	Cx(3),	Cx(4),	Cx(5),	0,		0,		0,
	D(3),	D(4),	D(5),	0,		0,		0,
	Cx(3),	Cx(4),	Cx(5),	0,		0,		0,
	D(3),	D(4),	D(5),	0,		0,		0,
	A(2),	A(3),	A(4),	0,		0,		0,
	C(3),	C(4),	C(5),	0,		0,		0,
	Ax(2),	Ax(3),	Ax(4),	0,		0,		0,
	G(2),	G(3),	G(4),	0,		0,		0,
	D(1),	D(2),	D(3),	0,		0,		0,
	G(1),	G(2),	G(3),	0,		0,		0,
	Ax(1),	Ax(2),	Ax(3),	0,		0,		0,

	D(2),	D(3),	D(4),	0,		0,		0,
	G(2),	G(3),	G(4),	0,		0,		0,
	A(2),	A(3),	A(4),	0,		0,		0,
	D(1),	D(2),	D(3),	0,		0,		0,
	A(1),	A(2),	A(3),	0,		0,		0,
	D(2),	D(3),	D(4),	0,		0,		0,
	Fx(2),	Fx(3),	Fx(4),	0,		0,		0,
	A(2),	A(3),	A(4),	0,		0,		0,
	Ax(2),	Ax(3),	Ax(4),	0,		0,		0,
	D(1),	D(2),	D(3),	0,		0,		0,
	G(1),	G(2),	G(3),	0,		0,		0,
	Ax(1),	Ax(2),	Ax(3),	0,		0,		0,

	D(3),	D(4),	D(5),	0,		0,		0,
	Cx(3),	Cx(4),	Cx(5),	0,		0,		0,
	D(3),	D(4),	D(5),	0,		0,		0,
	Cx(3),	Cx(4),	Cx(5),	0,		0,		0,
	D(3),	D(4),	D(5),	0,		0,		0,
	A(2),	A(3),	A(4),	0,		0,		0,
	C(3),	C(4),	C(5),	0,		0,		0,
	Ax(2),	Ax(3),	Ax(4),	0,		0,		0,
	G(2),	G(3),	G(4),	0,		0,		0,
	D(1),	D(2),	D(3),	0,		0,		0,
	G(1),	G(2),	G(3),	0,		0,		0,
	Ax(1),	Ax(2),	Ax(3),	0,		0,		0,

	D(2),	D(3),	D(4),	0,		0,		0,
	G(2),	G(3),	G(4),	0,		0,		0,
	A(2),	A(3),	A(4),	0,		0,		0,
	D(1),	D(2),	D(3),	0,		0,		0,
	A(1),	A(2),	A(3),	0,		0,		0,
	D(2),	D(3),	D(4),	0,		0,		0,
	Ax(2),	Ax(3),	Ax(4),	0,		0,		0,
	A(2),	A(3),	A(4),	0,		0,		0,
	0,		0,		0,		G(2),	0,		0,
	D(1),	D(2),	D(3),	0,		0,		0,
	G(1),	G(2),	G(3),	0,		0,		0,
	0,		0,		0,		0,		0,		0
};

/*
 * The theme from Phoenix, a sad little tune.
 * Gerald Coy:
 *	 The starting song from Phoenix is coming from a old french movie and
 *	 it's called : "Jeux interdits" which means "unallowed games"  ;-)
 * Mirko Buffoni:
 *	 It's called "Sogni proibiti" in italian, by Anonymous.
 * Magic*:
 *	 This song is a classical piece called "ESTUDIO" from M.A.Robira.
 */
static int tune3[96*6] = {
	A(2),	A(3),	A(4),	D(1),	 D(2),	  D(3),
	0,		0,		0,		0,		 0, 	  0,
	A(2),	A(3),	A(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,
	A(2),	A(3),	A(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,

	A(2),	A(3),	A(4),	A(1),	 A(2),	  A(3),
	0,		0,		0,		0,		 0, 	  0,
	G(2),	G(3),	G(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,
	F(2),	F(3),	F(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,

	F(2),	F(3),	F(4),	F(1),	 F(2),	  F(3),
	0,		0,		0,		0,		 0, 	  0,
	E(2),	E(3),	E(4),	F(1),	 F(2),	  F(3),
	0,		0,		0,		0,		 0, 	  0,
	D(2),	D(3),	D(4),	F(1),	 F(2),	  F(3),
	0,		0,		0,		0,		 0, 	  0,

	D(2),	D(3),	D(4),	A(1),	 A(2),	  A(3),
	0,		0,		0,		0,		 0, 	  0,
	F(2),	F(3),	F(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,
	A(2),	A(3),	A(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,

	D(3),	D(4),	D(5),	D(1),	 D(2),	  D(3),
	0,		0,		0,		0,		 0, 	  0,
	0,		0,		0,		D(1),	 D(2),	  D(3),
	0,		0,		0,		F(1),	 F(2),	  F(3),
	0,		0,		0,		A(1),	 A(2),	  A(3),
	0,		0,		0,		D(2),	 D(2),	  D(2),

	D(3),	D(4),	D(5),	D(1),	 D(2),	  D(3),
	0,		0,		0,		0,		 0, 	  0,
	C(3),	C(4),	C(5),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,
	Ax(2),	Ax(3),	Ax(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,

	Ax(2),	Ax(3),	Ax(4),	Ax(1),	 Ax(2),   Ax(3),
	0,		0,		0,		0,		 0, 	  0,
	A(2),	A(3),	A(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,
	G(2),	G(3),	G(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,

	G(2),	G(3),	G(4),	G(1),	 G(2),	  G(3),
	0,		0,		0,		0,		 0, 	  0,
	A(2),	A(3),	A(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,
	Ax(2),	Ax(3),	Ax(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,

	A(2),	A(3),	A(4),	A(1),	 A(2),	  A(3),
	0,		0,		0,		0,		 0, 	  0,
	Ax(2),	Ax(3),	Ax(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,
	A(2),	A(3),	A(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,

	Cx(3),	Cx(4),	Cx(5),	A(1),	 A(2),	  A(3),
	0,		0,		0,		0,		 0, 	  0,
	Ax(2),	Ax(3),	Ax(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,
	A(2),	A(3),	A(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,

	A(2),	A(3),	A(4),	F(1),	 F(2),	  F(3),
	0,		0,		0,		0,		 0, 	  0,
	G(2),	G(3),	G(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,
	F(2),	F(3),	F(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,

	F(2),	F(3),	F(4),	D(1),	 D(2),	  D(3),
	0,		0,		0,		0,		 0, 	  0,
	E(2),	E(3),	E(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,
	D(2),	D(3),	D(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,

	E(2),	E(3),	E(4),	E(1),	 E(2),	  E(3),
	0,		0,		0,		0,		 0, 	  0,
	E(2),	E(3),	E(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,
	E(2),	E(3),	E(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,

	E(2),	E(3),	E(4),	Ax(1),	 Ax(2),   Ax(3),
	0,		0,		0,		0,		 0, 	  0,
	F(2),	F(3),	F(4),	0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,
	E(2),	E(3),	E(4),	F(1),	 F(2),	  F(3),
	0,		0,		0,		0,		 0, 	  0,

	D(2),	D(3),	D(4),	D(1),	 D(2),	  D(3),
	0,		0,		0,		0,		 0, 	  0,
	F(2),	F(3),	F(4),	A(1),	 A(2),	  A(3),
	0,		0,		0,		0,		 0, 	  0,
	A(2),	A(3),	A(4),	F(1),	 F(2),	  F(3),
	0,		0,		0,		0,		 0, 	  0,

	D(3),	D(4),	D(5),	D(1),	 D(2),	  D(3),
	0,		0,		0,		0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,
	0,		0,		0,		0,		 0, 	  0,
};

/* This is used to play single notes for the TMS3617 */
static int tune4[13*6] = {
/*	16'     8'      5 1/3'  4'      2 2/3'  2'      */
	B(0),	B(1),	Dx(2),	B(2),	Dx(3),	B(3),
	C(1),	C(2),	E(2),	C(3),	E(3),	C(4),
	Cx(1),	Cx(2),	F(2),	Cx(3),	F(3),	Cx(4),
	D(1),	D(2),	Fx(2),	D(3),	Fx(3),	D(4),
	Dx(1),	Dx(2),	G(2),	Dx(3),	G(3),	Dx(4),
	E(1),	E(2),	Gx(2),	E(3),	Gx(3),	E(4),
	F(1),	F(2),	A(2),	F(3),	A(3),	F(4),
	Fx(1),	Fx(2),	Ax(2),	Fx(3),	Ax(3),	Fx(4),
	G(1),	G(2),	B(2),	G(3),	B(3),	G(4),
	Gx(1),	Gx(2),	C(3),	Gx(3),	C(4),	Gx(4),
	A(1),	A(2),	Cx(3),	A(3),	Cx(4),	A(4),
	Ax(1),	Ax(2),	D(3),	Ax(3),	D(4),	Ax(4),
	B(1),	B(2),	Dx(3),	B(3),	Dx(4),	B(4)
};

static int *tunes[] = {NULL,tune1,tune2,tune3,tune4};

#define DECAY(voice)											\
	if( tms->vol[voice] > VMIN )								\
	{															\
		/* decay of first voice */								\
		tms->vol_counter[voice] -= tms->decay[voice];			\
		while( tms->vol_counter[voice] <= 0 )					\
		{														\
			tms->vol_counter[voice] += samplerate;				\
			if( tms->vol[voice]-- <= VMIN ) 					\
			{													\
				tms->frequency[voice] = 0;						\
				tms->vol[voice] = VMIN; 						\
				break;											\
			}													\
		}														\
	}

#define RESTART(voice)											\
	if( tunes[tms->tune_num][tms->tune_ofs*6+voice] )			\
	{															\
		tms->frequency[voice] = 								\
			tunes[tms->tune_num][tms->tune_ofs*6+voice] *		\
			(tms->basefreq << tms->octave) / FSCALE;			\
		tms->vol[voice] = VMAX; 								\
	}

#define TONE(voice)                                             \
	if( (tms->enable & (1<<voice)) && tms->frequency[voice] )	\
	{															\
		/* first note */										\
		tms->counter[voice] -= tms->frequency[voice];			\
		while( tms->counter[voice] <= 0 )						\
		{														\
			tms->counter[voice] += samplerate;					\
			tms->output ^= 1 << voice;							\
		}														\
		if (tms->output & tms->enable & (1 << voice))			\
			sum += tms->vol[voice]; 							\
	}



static void tms36xx_sound_update(int param, INT16 *buffer, int length)
{
	struct TMS36XX *tms = tms36xx[param];
	int samplerate = tms->samplerate;

    /* no tune played? */
	if( !tunes[tms->tune_num] || tms->voices == 0 )
	{
		while (--length >= 0)
			buffer[length] = 0;
		return;
	}

	while( length-- > 0 )
	{
		int sum = 0;

		/* decay the six voices */
        DECAY(0) DECAY(1) DECAY(2)
		DECAY(3) DECAY(4) DECAY(5)

		/* musical note timing */
		tms->tune_counter -= tms->speed;
		if( tms->tune_counter <= 0 )
		{
			int n = (-tms->tune_counter / samplerate) + 1;
			tms->tune_counter += n * samplerate;

			if( (tms->note_counter -= n) <= 0 )
			{
				tms->note_counter += VMAX;
				if (tms->tune_ofs < tms->tune_max)
				{
					RESTART(0) RESTART(1) RESTART(2)
					RESTART(3) RESTART(4) RESTART(5)
					tms->tune_ofs++;
				}
			}
		}

		/* update the six harmonic voices */
        TONE(0) TONE(1) TONE(2)
		TONE(3) TONE(4) TONE(5)

		*buffer++ = sum / tms->voices;
	}
}

static void tms36xx_reset_counters(int chip)
{
    struct TMS36XX *tms = tms36xx[chip];
    tms->tune_counter = 0;
    tms->note_counter = 0;
	memset(tms->vol_counter, 0, sizeof(tms->vol_counter));
	memset(tms->counter, 0, sizeof(tms->counter));
}

void tms3615_tune_w(int chip, int tune)
{
    struct TMS36XX *tms = tms36xx[chip];

    /* which tune? */
    tune &= 3;
    if( tune == tms->tune_num )
        return;

    /* update the stream before changing the tune */
    stream_update(tms->channel,0);

    tms->tune_num = tune;
    tms->tune_ofs = 0;
    tms->tune_max = 96; /* fixed for now */
}

void tms3617_note_w(int chip, int octave, int note)
{
	struct TMS36XX *tms = tms36xx[chip];

	octave &= 3;
	note &= 15;

	if (note > 12)
        return;

    LOG((errorlog,"TMS3615 #%d octave:%X note:%X\n", chip, octave, note));

	/* update the stream before changing the tune */
    stream_update(tms->channel,0);

	/* play a single note from 'tune 4', a list of the 13 tones */
	tms36xx_reset_counters(chip);
	tms->octave = octave;
    tms->tune_num = 4;
	tms->tune_ofs = note;
	tms->tune_max = note + 1;
}

void tms3617_enable_w(int chip, int enable)
{
	struct TMS36XX *tms = tms36xx[chip];
	int i, bits = 0;

	enable &= 0x3f;
	if (enable == tms->enable)
		return;

    /* update the stream before changing the tune */
    stream_update(tms->channel,0);

	for (i = 0; i < 6; i++)
		bits += (enable & (1 << i)) ? 1 : 0;
	/* set the enable mask and number of active voices */
	tms->enable = enable;
    tms->voices = bits;
}

int tms36xx_sh_start(const struct MachineSound *msound)
{
	int i, j;
	intf = msound->sound_interface;

	for( i = 0; i < intf->num; i++ )
	{
		int enable;
		struct TMS36XX *tms;
		char name[16];

		sprintf(name, "TMS36%02d #%d", intf->subtype[i], i);
		tms36xx[i] = malloc(sizeof(struct TMS36XX));
		if( !tms36xx[i] )
		{
			if( errorlog ) fprintf(errorlog, "%s failed to malloc struct TMS36XX\n", name);
            return 1;
        }
		tms = tms36xx[i];
		memset(tms, 0, sizeof(struct TMS36XX));

		tms->channel = stream_init(name, intf->mixing_level[i], Machine->sample_rate, i, tms36xx_sound_update);

        if( tms->channel == -1 )
		{
			if( errorlog ) fprintf(errorlog, "%s stream_init failed\n", name);
			return 1;
		}
		tms->samplerate = Machine->sample_rate ? Machine->sample_rate : 1;
		tms->basefreq = intf->basefreq[i];
		enable = 0;
        for (j = 0; j < 6; j++)
		{
			if( intf->decay[i][j] > 0 )
			{
				tms->decay[j] = VMAX / intf->decay[i][j];
				enable |= 1 << j;
			}
		}
		tms->speed = (intf->speed[i] > 0) ? VMAX / intf->speed[i] : VMAX;
		tms3617_enable_w(i,enable);

        LOG((errorlog, "%s samplerate    %d\n", name, tms->samplerate));
		LOG((errorlog, "%s basefreq      %d\n", name, tms->basefreq));
		LOG((errorlog, "%s speed         %d\n", name, tms->speed));
		LOG((errorlog, "%s decay         %d,%d,%d,%d,%d,%d\n", name,
			tms->decay[0], tms->decay[1], tms->decay[2],
			tms->decay[3], tms->decay[4], tms->decay[5]));
    }
    return 0;
}

void tms36xx_sh_stop(void)
{
	int i;
	for( i = 0; i < intf->num; i++ )
	{
		if( tms36xx[i] )
			free(tms36xx[i]);
		tms36xx[i] = NULL;
	}
}

void tms36xx_sh_update(void)
{
	int i;
    for( i = 0; i < intf->num; i++ )
		stream_update(i,0);
}

