#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "driver.h"
#include "ym2151.h"

/*
**  some globals ...
*/

/*
**  Shifts below are subject to change if sampling frequency changes...
*/
#define FREQ_SH 16  /* 16.16 fixed point for frequency calculations     */
#define LFO_SH 24   /*  8.24 fixed point for LFO frequency calculations */
#define ENV_SH 16   /* 16.16 fixed point for envelope calculations      */
#define TIMER_SH 16 /* 16.16 fixed point for timers calculations        */

#define ENV_BITS 10
#define ENV_RES ((int)1<<ENV_BITS)
#define ENV_STEP (96.0/ENV_RES)
#define MAX_VOLUME_INDEX ((ENV_RES-1)<<ENV_SH)
#define MIN_VOLUME_INDEX (0)
#define VOLUME_OFF (ENV_RES<<ENV_SH)

#define SIN_BITS 10
#define SIN_LEN ((int)1<<SIN_BITS)
#define SIN_MASK (SIN_LEN-1)

#define FINAL_SH8 7   /*this shift is applied to final output of all channels to get 8-bit sample */
#define FINAL_SH16 0  /*this shift is applied to final output of all channels to get 16-bit sample*/

static unsigned char FEED[8]={0,7,6,5,4,3,2,1}; /*shifts (divider) for output of op.0 which feeds into itself*/

#define TL_TAB_LEN (2*(ENV_RES + ENV_RES + ENV_RES + SIN_LEN))
static signed int * TL_TAB = NULL;
/*
 *  Offset in this table is calculated as:
 *
 *  1st ENV_RES:
 *     TL- main offset (Total attenuation Level), range 0 to 1023 (0-96 dB)
 *  2nd ENV_RES:
 *     current envelope value of the operator, range 0 to 1023 (0-96 dB)
 *  3rd ENV_RES:
 *     Amplitude Modulation from LFO, range 0 to 1023 (0-96dB)
 *  4th SIN_LEN:
 *     Sin Wave Offset from sin_tab, range 0 to about 56 dB only, but lets
 *     simplify things and assume sin could be 96 dB, range 0 to 1023
 *
 *  Length of this table is doubled because we have two separate parts
 *  for positive and negative halves of sin wave (above and below X axis).
 */

static signed int * sin_tab[SIN_LEN];  /* sin waveform table in decibel scale */



#if 0
/*tables below are defined for usage by LFO */
signed int PMsaw     [SIN_LEN];   /*saw waveform table PM      */
signed int PMsquare  [SIN_LEN];   /*square waveform table PM   */
signed int PMtriangle[SIN_LEN];   /*triangle waveform table PM */
signed int PMnoise   [SIN_LEN];   /*noise waveform table PM    */

unsigned short AMsaw     [SIN_LEN];     /*saw waveform table AM      */
unsigned short AMsquare  [SIN_LEN];     /*square waveform table AM   */
unsigned short AMtriangle[SIN_LEN];     /*triangle waveform table AM */
unsigned short AMnoise   [SIN_LEN];     /*noise waveform table AM    */
#endif




static int YM2151_CLOCK = 1;		/* this will be passed from 2151intf.c */
static int YM2151_SAMPFREQ = 1;		/* this will be passed from 2151intf.c */
static unsigned char sample_16bit;	/* 1 -> 16 bit sample, 0 -> 8 bit */

static int YMBufSize;		/* size of sound buffer, in samples */
static int YMNumChips;		/* total # of YM's emulated */

static int TimerA[1024];
static int TimerB[256];

/* ASG 980324: added */
static double TimerATime[1024];
static double TimerBTime[256];


static YM2151 * YMPSG = NULL;	/* array of YM's */

signed int * BuffTemp = NULL;	/*temporary buffer for speedup purposes*/



static void (*envelope_calc[5])(OscilRec *);
static void (*register_writes[256])(unsigned char , unsigned char, unsigned char);


/*save output as raw 16-bit sample - just in case you would like to listen to it offline ;-)*/
/*#define SAVE_SAMPLE*/
/*#define SAVE_SEPARATE_CHANNELS*/

#ifdef SAVE_SAMPLE
#ifdef SAVE_SEPARATE_CHANNELS
FILE *sample1;
FILE *sample2;
FILE *sample3;
FILE *sample4;
FILE *sample5;
FILE *sample6;
#endif
FILE *samplesum;
#endif


int PMTab[8]; /*8 channels */
/*  this table is used for PM setup of LFO */

int AMTab[8]; /*8 channels */
/*  this table is used for AM setup of LFO */


static int decib45[16];
/*decibel table to convert from D1L values to index in lookup table*/

static int attack_curve[ENV_RES];

unsigned int divia[64]; /*Attack dividers*/
unsigned int divid[64]; /*Decay dividers*/
static unsigned int A_DELTAS[64+31]; /*Attack deltas (64 keycodes + 31 RKS's = 95)*/
static unsigned int D_DELTAS[64+31]; /*Decay  deltas (64 keycodes + 31 RKS's = 95)*/



float DT1Tab[32][4]={ /* 8 octaves * 4 key codes, 4 DT1 values */
/*
 *  Table defines offset in Hertz from base frequency depending on KC and DT1
 *  User's Manual page 21
*/
/*OCT NOTE DT1=0 DT1=1  DT1=2  DT1=3*/
/* 0   0*/{0,    0,     0.053, 0.107},
/* 0   1*/{0,    0,     0.053, 0.107},
/* 0   2*/{0,    0,     0.053, 0.107},
/* 0   3*/{0,    0,     0.053, 0.107},
/* 1   0*/{0,    0.053, 0.107, 0.107},
/* 1   1*/{0,    0.053, 0.107, 0.160},
/* 1   2*/{0,    0.053, 0.107, 0.160},
/* 1   3*/{0,    0.053, 0.107, 0.160},
/* 2   0*/{0,    0.053, 0.107, 0.213},
/* 2   1*/{0,    0.053, 0.160, 0.213},
/* 2   2*/{0,    0.053, 0.160, 0.213},
/* 2   3*/{0,    0.053, 0.160, 0.267},
/* 3   0*/{0,    0.107, 0.213, 0.267},
/* 3   1*/{0,    0.107, 0.213, 0.320},
/* 3   2*/{0,    0.107, 0.213, 0.320},
/* 3   3*/{0,    0.107, 0.267, 0.373},
/* 4   0*/{0,    0.107, 0.267, 0.427},
/* 4   1*/{0,    0.160, 0.320, 0.427},
/* 4   2*/{0,    0.160, 0.320, 0.480},
/* 4   3*/{0,    0.160, 0.373, 0.533},
/* 5   0*/{0,    0.213, 0.427, 0.587},
/* 5   1*/{0,    0.213, 0.427, 0.640},
/* 5   2*/{0,    0.213, 0.480, 0.693},
/* 5   3*/{0,    0.267, 0.533, 0.747},
/* 6   0*/{0,    0.267, 0.587, 0.853},
/* 6   1*/{0,    0.320, 0.640, 0.907},
/* 6   2*/{0,    0.320, 0.693, 1.013},
/* 6   3*/{0,    0.373, 0.747, 1.067},
/* 7   0*/{0,    0.427, 0.853, 1.173},
/* 7   1*/{0,    0.427, 0.907, 1.173},
/* 7   2*/{0,    0.480, 1.013, 1.173},
/* 7   3*/{0,    0.533, 1.067, 1.173}
};

static unsigned short DT2Tab[4]={ /* 4 DT2 values */
/*
 *   DT2 defines offset in cents from base note
 *
 *   The table below defines offset in deltas table...
 *   User's Manual page 22
 *   Values below were calculated using formula:  value = orig.val / 1.5625
 *
 * DT2=0 DT2=1 DT2=2 DT2=3
 * 0     600   781   950
 */
	0,    384,  500,  608
};


static unsigned short KC_TO_INDEX[16*8];  /*16 note codes * 8 octaves */
/*translate key code KC (OCT2 OCT1 OCT0 N3 N2 N1 N0) into index in deltas table*/


static signed int DT1deltas[32][8];

static signed int deltas[9*12*64];/*9 octaves, 12 semitones, 64 'cents'  */
/*deltas in sintable (fixed point) to get the closest frequency possible */
/*there're 9 octaves because of DT2 (max 950 cents over base frequency)  */

static signed int LFOdeltas[256];  /*frequency deltas for LFO*/

void sin_init(void)
{
	int x,i;
	float m;

	for(i=0; i<SIN_LEN; i++)
	{
		m = sin(2*PI*i/SIN_LEN);  /*count sin value*/
		if ( (m<0.0001) && (m>-0.0001) ) /*is m very close to zero ?*/
			m = ENV_RES-1;
		else
		{
			if (m>0.0)
			{
				m = 20*log10(1.0/m);      /* and how many decibels is it? */
				m = m / ENV_STEP;
			}
			else
			{
				m = 20*log10(-1.0/m);     /* and how many decibels is it? */
				m = (m / ENV_STEP) + TL_TAB_LEN/2;
			}
		}
		sin_tab[ i ] = &TL_TAB[(unsigned int)m];  /**/
		/*if (errorlog) fprintf(errorlog,"sin %i = %i\n",i,sin_tab[i] );*/
	}

	for( x=0; x<TL_TAB_LEN/2; x++ )
	{
		if (x<ENV_RES)
		{
			if ( (x * ENV_STEP) < 6.0 )  /*have we passed 6 dB*/
			{	/*nope, we didn't */
				m = ((1<<12)-1) / pow(10, x * ENV_STEP /20);
			}
			else
			{	/*if yes, we simplify things (and the real chip */
				/*probably does it also) and assume that - 6dB   */
				/*halves the voltage (it _nearly_ does in fact)  */

				m = TL_TAB[ (int)((float)x - (6.0/ENV_STEP)) ] / 2;
			}
		}
		else
		{
			m = 0;
		}
		TL_TAB[         x        ] = m;
		TL_TAB[ x + TL_TAB_LEN/2 ] = -m;
		/*if (errorlog) fprintf(errorlog,"tl %04i =%08x\n",x,TL_TAB[x]);*/
	}


/* create attack curve */
	for (i=0; i<ENV_RES; i++)
	{
		m = (ENV_RES-1) / pow(10, i * (48.0/ENV_RES) /20);
		x = m*(1<<ENV_SH);
		attack_curve[ENV_RES-1-i] = x;
		/*if (errorlog) fprintf(errorlog,"attack [%04x] = %08x Volt=%08x\n", ENV_RES-1-i, x/(1<<ENV_SH), TL_TAB[x/(1<<ENV_SH)] );*/
	}


	for (x=0; x<16; x++)
	{
		i = (x<15?x:x+16) * (3.0/ENV_STEP);  /*every 3 dB except for ALL '1' = 45dB+48dB*/
		i<<=ENV_SH;
		decib45[x]=i;
		/*if (errorlog) fprintf(errorlog,"decib45[%04x]=%08x\n",x,i );*/
	}


#ifdef SAVE_SAMPLE
#ifdef SAVE_SEPARATE_CHANNELS
sample1=fopen("samp.raw","wb");
sample2=fopen("samp2.raw","wb");
sample3=fopen("samp3.raw","wb");
sample4=fopen("samp4.raw","wb");
sample5=fopen("samp5.raw","wb");
sample6=fopen("samp6.raw","wb");
#endif
samplesum=fopen("sampsum.raw","wb");
#endif
}


void hertz(void)
{
	int i,j,oct;
	long int mult;
	float pom,pom2,m;
	float scaler;	/* formula below is true for chip clock=3579545 */
	/* so we need to scale its output accordingly to the chip clock */

/*this loop calculates Hertz values for notes from c#0 up to c-8*/
/*including 64 'cents' (100/64 which is 1.5625 of real cent) for each semitone*/

	scaler= (float)YM2151_CLOCK / 3579545.0;

	oct=768;
	mult = (long int)1<<FREQ_SH;
	for (i=0; i<1*12*64; i++)
	{
		pom = scaler * 6.875 * pow (2, ((i+4*64)*1.5625/1200.0) ); /*13.75Hz is note A 12semitones below A-0, so C#0 is 4 semitones above then*/
		/*calculate phase increment for above precounted Hertz value*/
		pom2=((pom*SIN_LEN)/(float)YM2151_SAMPFREQ)*mult; /*fixed point*/

		deltas[i+oct*4] = pom2*16;  /*oct 4 - center*/
		deltas[i+oct*5] = deltas[oct*4+i]<<1; /*oct 5*/
		deltas[i+oct*6] = deltas[oct*4+i]<<2; /*oct 6*/
		deltas[i+oct*7] = deltas[oct*4+i]<<3; /*oct 7*/
		deltas[i+oct*8] = deltas[oct*4+i]<<4; /*oct 8*/

		deltas[i+oct*3] = deltas[oct*4+i]>>1; /*oct 3*/
		deltas[i+oct*2] = deltas[oct*4+i]>>2; /*oct 2*/
		deltas[i+oct*1] = deltas[oct*4+i]>>3; /*oct 1*/
		deltas[i+oct*0] = deltas[oct*4+i]>>4; /*oct 0*/

		/*if (errorlog) fprintf(errorlog,"deltas[%04i] = %08x\n",i,deltas[i]);*/
	}
	for (j=0; j<4; j++)
	{
		for (i=0; i<32; i++)
		{
			pom = scaler * DT1Tab[i][j];
			/*calculate phase increment for above precounted Hertz value*/
			DT1deltas[i][j]=((pom*SIN_LEN)/(float)YM2151_SAMPFREQ)*mult; /*fixed point*/
			DT1deltas[i][j+4]= -DT1deltas[i][j];
		}
	}

	mult = (long int)1<<LFO_SH;
	m= (float)YM2151_CLOCK;
	for (i=0; i<256; i++)
	{
		j = i & 0x0f;
		pom= scaler * fabs(  (m/65536/(1<<(i/16)) ) - (m/65536/32/(1<<(i/16)) * (j+1)) );

		/*calculate phase increment for above precounted Hertz value*/
		pom2=((pom*SIN_LEN)/(float)YM2151_SAMPFREQ)*mult; /*fixed point*/
		LFOdeltas[0xff-i]=pom2;
		/*if (errorlog) fprintf(errorlog, "LFO[%02x] = %08x\n",0xff-i, LFOdeltas[0xff-i]);*/
	}






/* calculate KC to index table */
	j=0;
        for (i=0; i<16*8; i++)
	{
		KC_TO_INDEX[i]=(i>>4)*12*64 +j*64 ;
		if ((i&3)!=3) j++;	/* change note code */
		if ((i&15)==15) j=0;	/* new octave */
		/*if (errorlog) fprintf(errorlog,"NOTE[%i] = %i\n",i,KC_TO_INDEX[i]);*/
	}

/* precalculate envelope times */
	for (i=0; i<64; i++)
	{
		pom=(16<<(i>>2))+(4<<(i>>2))*(i&0x03);
		if ((i>>2)==0)	pom=1; /*infinity*/
		if ((i>>2)==15) pom=524288; /*const*/
		divid[i]=pom;
	}

	for (i=0; i<64; i++)
	{
		pom=((128+64+32)<<(i>>2))+((32+16+8)<<(i>>2))*(i&0x03);
		if ((i>>2)==0)	pom=1;    /*infinity*/
		if ((i>>2)==15)
		{
			if ((i&0x03)==3)
				pom=153293300; /*zero attack time*/
			else
				pom=6422528; /*const attack time*/
		}
		divia[i]=pom;
	}

	mult = (long int)1<<ENV_SH;
	for (i=0; i<64; i++)
	{
		if (divid[i]==1)
			pom=0;  /*infinity*/
		else
			pom=(scaler * ENV_RES * mult)/ ( (float)YM2151_SAMPFREQ*((float)YM2151_CLOCK/1000.0/(float)divid[i]));
		/*if (errorlog) fprintf(errorlog,"i=%03i div=%i time=%f delta=%f\n",i,divid[i],*/
		/*		(float)YM2151_CLOCK/1000.0/(float)divid[i], pom );*/
		D_DELTAS[i] = pom;
	}

	for (i=0; i<64; i++)
	{
		if (divia[i]==1)
			pom=0;  /*infinity*/
		else
			pom=(scaler * ENV_RES * mult)/ ( (float)YM2151_SAMPFREQ*((float)YM2151_CLOCK/1000.0/(float)divia[i]));
		/*if (errorlog) fprintf(errorlog,"i=%03i div=%i time=%f delta=%f\n",i,divia[i],*/
		/*		(float)YM2151_CLOCK/1000.0/(float)divia[i], pom );*/
		A_DELTAS[i] = pom;
	}

	for (i=0; i<32; i++) /*this is only for speedup purposes -> to avoid testing if (keycode+RKS is over 63)*/
	{
		D_DELTAS[64+i]=D_DELTAS[63];
		A_DELTAS[64+i]=A_DELTAS[63];
	}


/* precalculate timers deltas */
/* User's Manual pages 15,16  */
	mult = (int)1<<TIMER_SH;
	for (i=0 ; i<1024; i++)
	{
		/* ASG 980324: changed to compute both TimerA and TimerATime */
		pom= ( 64.0  *  (1024.0-i) / YM2151_CLOCK ) ;
		TimerATime[i] = pom;
		TimerA[i] = pom * YM2151_SAMPFREQ * mult;  /*number of samples that timer period should last (fixed point) */
	}
	for (i=0 ; i<256; i++)
	{
		/* ASG 980324: changed to compute both TimerB and TimerBTime */
		pom= ( 1024.0 * (256.0-i)  / YM2151_CLOCK ) ;
		TimerBTime[i] = pom;
		TimerB[i] = pom * YM2151_SAMPFREQ * mult;  /*number of samples that timer period should last (fixed point) */
	}
}









void envelope_attack(OscilRec *op)
{
	if ( (op->attack_volume -= op->delta_AR) < MIN_VOLUME_INDEX )  /*is volume index min already ?*/
	{
		op->volume = MIN_VOLUME_INDEX;
		op->state++;
	}
	else
		op->volume = attack_curve[op->attack_volume>>ENV_SH];
}

void envelope_decay(OscilRec *op)
{
	if ( (op->volume += op->delta_D1R) > op->D1L )
	{
		/*op->volume = op->D1L;*/
		op->state++;
	}
}

void envelope_sustain(OscilRec *op)
{
	if ( (op->volume += op->delta_D2R) > MAX_VOLUME_INDEX )
	{
		op->state = 4;
		op->volume = VOLUME_OFF;
	}
}

void envelope_release(OscilRec *op)
{
	if ( (op->volume += op->delta_RR) > MAX_VOLUME_INDEX )
	{
		op->state = 4;
		op->volume = VOLUME_OFF;
	}
}


void envelope_nothing(OscilRec *op)
{
}

INLINE void envelope_KOFF(OscilRec * op)
{
		op->state=3; /*release*/
}
INLINE void envelope_KON(OscilRec * op)
{
		 /*this is to remove the gap time if TL>0*/
		op->volume = VOLUME_OFF; /*(ENV_RES - op->TL)<<ENV_SH;   **  <-  SURE ABOUT IT ?  No, but let's give it a try...  */
		op->attack_volume = op->volume;
		op->phase = 0;
		op->state = 0;    /*KEY ON = attack*/
		op->OscilFB = 0;  /*Clear feedback after key on */
}


void refresh_chip(YM2151 *PSG)
{
unsigned short kc_index_oscil, kc_index_channel, mul;
unsigned char op,v,kc;
signed char k,chan;
OscilRec * osc;


  for (chan=7; chan>=0; chan--)
  {
	kc = PSG->KC[chan];
	kc_index_channel = KC_TO_INDEX[kc] + PSG->KF[chan];
	kc >>=2;

	for (k=24; k>=0; k-=8)
	{
		op = chan + k;
		osc = &PSG->Oscils[ op ];

/*calc freq begin*/
		v = (PSG->Regs[YM_DT2_D2R_BASE+op]>>6) & 0x03;   /*DT2 value*/
	        kc_index_oscil = kc_index_channel + DT2Tab[ v ]; /*DT2 offset*/
		v = PSG->Regs[YM_DT1_MUL_BASE+op];
		mul= (v&0x0f) << 1;
		if (mul)
			osc->freq = deltas[ kc_index_oscil ] * mul;
		else
			osc->freq = deltas[ kc_index_oscil ];
		osc->freq += DT1deltas[ kc ][ (v>>4) & 0x07 ];  /*DT1 value*/
/*calc freq end*/


/*calc envelopes begin*/
	v = kc >> PSG->KS[op];
	osc->delta_AR  = A_DELTAS[ osc->AR + v];    /* 2*RR + RKS =max 95*/
	osc->delta_D1R = D_DELTAS[osc->D1R + v];    /* 2*RR + RKS =max 95*/
	osc->delta_D2R = D_DELTAS[osc->D2R + v];    /* 2*RR + RKS =max 95*/
	osc->delta_RR =  D_DELTAS[ osc->RR + v];    /* 2*RR + RKS =max 95*/
/*calc envelopes end*/

	}
  }

}


void write_YM_NON_EMULATED(unsigned char n, unsigned char r, unsigned char v)
{
    if (errorlog)
	fprintf(errorlog,"Write to non emulated register %02x value %02x\n",r,v);
}

void write_YM_KON(unsigned char n, unsigned char r, unsigned char v)
{
	unsigned char chan;
	YM2151 *PSG = &(YMPSG[n]);

	chan = v & 0x07;
	if (v&0x08) envelope_KON(&PSG->Oscils[ chan   ]); else envelope_KOFF(&PSG->Oscils[ chan   ]);
	if (v&0x10) envelope_KON(&PSG->Oscils[ chan+16]); else envelope_KOFF(&PSG->Oscils[ chan+16]);
	if (v&0x20) envelope_KON(&PSG->Oscils[ chan+ 8]); else envelope_KOFF(&PSG->Oscils[ chan+ 8]);
	if (v&0x40) envelope_KON(&PSG->Oscils[ chan+24]); else envelope_KOFF(&PSG->Oscils[ chan+24]);
}


void write_YM_CLOCKA1(unsigned char n, unsigned char r, unsigned char v)
{
    YMPSG[n].Regs[r]=v;
}

void write_YM_CLOCKA2(unsigned char n, unsigned char r, unsigned char v)
{
    YMPSG[n].Regs[r]=v & 0x03;
}

void write_YM_CLOCKB(unsigned char n, unsigned char r, unsigned char v)
{
    YMPSG[n].Regs[r]=v;
}

static void timer_callback_a (int n)
{
    YM2151 *PSG = &YMPSG[n];
    YM2151UpdateOne (n, cpu_scalebyfcount (YMBufSize));
    if (PSG->handler) (*PSG->handler)();
	PSG->TimA=0;
	PSG->TimIRQ|=1;
    PSG->TimATimer=0;
}

static void timer_callback_b (int n)
{
    YM2151 *PSG = &YMPSG[n];
    YM2151UpdateOne (n, cpu_scalebyfcount (YMBufSize));
    if (PSG->handler) (*PSG->handler)();
	PSG->TimB=0;
	PSG->TimIRQ|=2;
    PSG->TimBTimer=0;
}

void write_YM_CLOCKSET(unsigned char n, unsigned char r, unsigned char v)
{
    YM2151 *PSG = &(YMPSG[n]);

    v &= 0xbf;
    /*PSG->Regs[r]=v;*/
/*	if (errorlog) fprintf(errorlog,"CSM= %01x FRESET=%02x, IRQEN=%02x, LOAD=%02x\n",v>>7,(v>>4)&0x03,(v>>2)&0x03,v&0x03 );*/
    if (v&0x80) /*CSM*/
	{    }

	/* ASG 980324: remove the timers if they exist */
	if (PSG->TimATimer) timer_remove (PSG->TimATimer);
	if (PSG->TimBTimer) timer_remove (PSG->TimBTimer);
	PSG->TimATimer=0;
	PSG->TimBTimer=0;

    if (v&0x01) /*LOAD A*/
    {
		PSG->TimA=1;
		PSG->TimAVal=TimerA[ (PSG->Regs[YM_CLOCKA1]<<2) | PSG->Regs[YM_CLOCKA2] ];
		/* ASG 980324: added a real timer if we have a handler */
		if (PSG->handler)
			PSG->TimATimer = timer_set (TimerATime[ (PSG->Regs[YM_CLOCKA1]<<2) | PSG->Regs[YM_CLOCKA2] ], n, timer_callback_a);
    }
    else
	PSG->TimA=0;
    if (v&0x02) /*load B*/
    {
		PSG->TimB=1;
		PSG->TimBVal=TimerB[ PSG->Regs[YM_CLOCKB] ];
		/* ASG 980324: added a real timer if we have a handler */
		if (PSG->handler)
			PSG->TimBTimer = timer_set (TimerBTime[ PSG->Regs[YM_CLOCKB] ], n, timer_callback_b);
    }
    else
	PSG->TimB=0;
    if (v&0x04) /*IRQEN A*/
	{	}
    if (v&0x08) /*IRQEN B*/
	{       }
    if (v&0x10) /*FRESET A*/
    {
		PSG->TimIRQ &= 0xfe;
    }
    if (v&0x20) /*FRESET B*/
    {
		PSG->TimIRQ &= 0xfd;
    }

}

void write_YM_CT1_CT2_W(unsigned char n, unsigned char r, unsigned char v)
{
	YMPSG[n].Regs[r]=v;
}


void write_YM_CONNECT_BASE(unsigned char n, unsigned char r, unsigned char v)
{
    unsigned char chan;
    YM2151 *PSG;
    PSG = &(YMPSG[n]);

	/*PSG->Regs[r] = v;*/

	chan = r-YM_CONNECT_BASE;

	PSG->ConnectTab[chan] = v&7; /*connection number*/
	PSG->FeedBack[chan] = FEED[(v>>3)&7];
}


void write_YM_KC_BASE(unsigned char n, unsigned char r, unsigned char v)
{
        YMPSG[n].KC[ r-YM_KC_BASE ] = v;
	/*freq_calc(chan,PSG);*/
}

void write_YM_KF_BASE(unsigned char n, unsigned char r, unsigned char v)
{
	YMPSG[n].KF[ r-YM_KF_BASE ] = v>>2;
	/*freq_calc(chan,PSG);*/
}


void write_YM_PMS_AMS_BASE(unsigned char n, unsigned char r, unsigned char v)
{
    unsigned char chan,i;
    YM2151 *PSG = &(YMPSG[n]);

	PSG->Regs[r] = v;

	chan = r - YM_PMS_AMS_BASE;

	i = v>>4;  /*PMS;*/
	PMTab[chan] = i;
	if (i && errorlog)
		fprintf(errorlog,"PMS CHN %02x =%02x\n",chan,i);

	i = v&0x03; /*AMS;*/
	AMTab[chan] = i;
	if (i && errorlog)
		fprintf(errorlog,"AMS CHN %02x =%02x\n",chan,i);

}

void write_YM_DT1_MUL_BASE(unsigned char n, unsigned char r, unsigned char v)
{
	YMPSG[n].Regs[r] = v;
	/*freq_calc(chan,PSG);*/
}

void write_YM_TL_BASE(unsigned char n, unsigned char r, unsigned char v)
{
	v &= 0x7f;
	YMPSG[n].Oscils[ r - YM_TL_BASE ].TL =  v<<(ENV_BITS-7); /*7bit TL*/
}


void write_YM_KS_AR_BASE(unsigned char n, unsigned char r, unsigned char v)
{
    unsigned char op;
    YM2151 *PSG;

	op = r - YM_KS_AR_BASE;
	PSG = &(YMPSG[n]);

	PSG->KS[ op ] = 3-(v>>6);
	PSG->Oscils[ op ].AR = (v&0x1f) << 1;
}


void write_YM_AMS_D1R_BASE(unsigned char n, unsigned char r, unsigned char v)
{
    unsigned char op;

	op = r - YM_AMS_D1R_BASE;

	if ((v & 0x80) && errorlog)
		fprintf(errorlog,"AMS ON oper%02x\n",op);

     /*HERE something to do with AMS;*/

	YMPSG[n].Oscils[ op ].D1R =  (v&0x1f) << 1;
}

void write_YM_DT2_D2R_BASE(unsigned char n, unsigned char r, unsigned char v)
{
    OscilRec * osc;
    YM2151 *PSG = &(YMPSG[n]);

	osc = &PSG->Oscils[  r - YM_DT2_D2R_BASE ];
	PSG->Regs[r] = v;

	osc->D2R =  (v&0x1f) << 1;
	/*freq_calc(chan,PSG);*/
}


void write_YM_D1L_RR_BASE(unsigned char n, unsigned char r, unsigned char v)
{
    OscilRec * osc;

	osc = &YMPSG[n].Oscils[ r - YM_D1L_RR_BASE ];

	osc->D1L = decib45[ (v>>4) & 0x0f ];
	osc->RR= ((v&0x0f)<<2)|0x02;
}






/*
** Initialize YM2151 emulator(s).
**
** 'num' is the number of virtual YM2151's to allocate
** 'clock' is the chip clock
** 'rate' is sampling rate and 'bufsiz' is the size of the
** buffer that should be updated at each interval
*/
int YMInit(int num, int clock, int rate,int sample_bits, int bufsiz, SAMPLE **buffer )
{
    int i;

    if (YMPSG) return (-1);	/* duplicate init. */

    YMNumChips = num;
    YM2151_SAMPFREQ = rate;
    if (sample_bits==16)
	sample_16bit=1;
    else
	sample_16bit=0;

    YM2151_CLOCK = clock;
    YMBufSize = bufsiz;

    envelope_calc[0]=envelope_attack;
    envelope_calc[1]=envelope_decay;
    envelope_calc[2]=envelope_sustain;
    envelope_calc[3]=envelope_release;
    envelope_calc[4]=envelope_nothing;

    for (i=0; i<256; i++)
	    register_writes[i]=write_YM_NON_EMULATED;

    register_writes[YM_KON] 	= write_YM_KON;
    register_writes[YM_CLOCKA1]	= write_YM_CLOCKA1;
    register_writes[YM_CLOCKA2]	= write_YM_CLOCKA2;
    register_writes[YM_CLOCKB]	= write_YM_CLOCKB;
    register_writes[YM_CLOCKSET]= write_YM_CLOCKSET;
    register_writes[YM_CT1_CT2_W]= write_YM_CT1_CT2_W;

    for (i=YM_CONNECT_BASE; i<YM_CONNECT_BASE+8; i++)
	    register_writes[i]= write_YM_CONNECT_BASE;

    for (i=YM_KC_BASE; i<YM_KC_BASE+8; i++)
	    register_writes[i]= write_YM_KC_BASE;

    for (i=YM_KF_BASE; i<YM_KF_BASE+8; i++)
	    register_writes[i]= write_YM_KF_BASE;

    for (i=YM_PMS_AMS_BASE; i<YM_PMS_AMS_BASE+8; i++)
	    register_writes[i]= write_YM_PMS_AMS_BASE;

    for (i=YM_DT1_MUL_BASE; i<YM_DT1_MUL_BASE+32; i++)
	    register_writes[i]= write_YM_DT1_MUL_BASE;

    for (i=YM_TL_BASE; i<YM_TL_BASE+32; i++)
	    register_writes[i]= write_YM_TL_BASE;

    for (i=YM_KS_AR_BASE; i<YM_KS_AR_BASE+32; i++)
	    register_writes[i]= write_YM_KS_AR_BASE;

    for (i=YM_AMS_D1R_BASE; i<YM_AMS_D1R_BASE+32; i++)
	    register_writes[i]= write_YM_AMS_D1R_BASE;

    for (i=YM_DT2_D2R_BASE; i<YM_DT2_D2R_BASE+32; i++)
	    register_writes[i]= write_YM_DT2_D2R_BASE;

    for (i=YM_D1L_RR_BASE; i<YM_D1L_RR_BASE+32; i++)
	    register_writes[i]= write_YM_D1L_RR_BASE;


    YMPSG = (YM2151 *)malloc(sizeof(YM2151) * YMNumChips);
    if (YMPSG == NULL) return (1);

    TL_TAB = (signed int *)malloc(sizeof(signed int) * TL_TAB_LEN );
    if (TL_TAB == NULL) return (1);

    BuffTemp = (signed int *)malloc(sizeof(signed int) * YMBufSize );
    if (BuffTemp == NULL) return (1);


    hertz();
    sin_init();

    for ( i = 0 ; i < YMNumChips; i++ )
    {
	YMPSG[i].Buf = buffer[i];
	YMPSG[i].bufp=0;
        YMResetChip(i);
    }
    return(0);
}

void YMShutdown()
{
    if (!YMPSG) return;

    free(YMPSG);
    YMPSG = NULL;

    if (TL_TAB)
    {
	free(TL_TAB);
	TL_TAB=NULL;
    }

    if (BuffTemp)
    {
	free(BuffTemp);
	BuffTemp=NULL;
    }


    YM2151_SAMPFREQ = YMBufSize = 0;

#ifdef SAVE_SAMPLE
#ifdef SAVE_SEPARATE_CHANNELS
fclose(sample1);
fclose(sample2);
fclose(sample3);
fclose(sample4);
fclose(sample5);
fclose(sample6);
#endif
fclose(samplesum);
#endif
}

/* write a register on YM2151 chip number 'n' */
void YMWriteReg(int n, int r, int v)
{
	register_writes[ (unsigned char)r ] ( (unsigned char)n, (unsigned char)r, (unsigned char)v);
}

unsigned char YMReadReg( unsigned char n )
{
    return YMPSG[n].TimIRQ;
}


/*
** reset all chip registers.
*/
void YMResetChip(int num)
{
    int i;
    YM2151 *PSG = &(YMPSG[num]);

    memset(PSG->Buf,'\0',YMBufSize);

	/* ASG 980324 -- reset the timers before writing to the registers */
	PSG->TimATimer=0;
	PSG->TimBTimer=0;

    /* initialize hardware registers */
    for( i=0; i<256; i++ )
	PSG->Regs[i]=0;

    for( i=0; i<32; i++)
    {
	memset(&PSG->Oscils[i],'\0',sizeof(OscilRec));
	PSG->Oscils[i].volume=VOLUME_OFF;
	PSG->Oscils[i].state=4;  /*envelope off*/
    }

    for ( i=0; i<8; i++)
    {
	PSG->ConnectTab[i]=0;
	PSG->FeedBack[i]=0;
    }

	PSG->TimA=0;
	PSG->TimB=0;
	PSG->TimAVal=0;
	PSG->TimBVal=0;
	PSG->TimIRQ=0;

}


INLINE signed int op_calc(OscilRec *OP, signed int pm)
{
  return  sin_tab[ ( ((OP->phase+=OP->freq) >> FREQ_SH) + (pm) ) & SIN_MASK] [ OP->TL + (OP->volume>>ENV_SH) ];
}

void YM2151UpdateOne(int num, int endp)
{
    YM2151 * PSG = &(YMPSG[num]);
    SAMPLE * BUF;
    signed int * PSGBUF;
    OscilRec *OP0, *OP1, *OP2, *OP3;
    unsigned short i;
    signed int k, wy;
#ifdef SAVE_SEPARATE_CHANNELS
    signed int pom;
#endif

	refresh_chip(PSG);

	/*calculate timers...*/
	if (PSG->TimA)
	{
		PSG->TimAVal -= ((endp - PSG->bufp)<<TIMER_SH);
		if (PSG->TimAVal<=0)
		{
			PSG->TimA=0;
			PSG->TimIRQ|=1;
			/* ASG 980324 - handled by real timers now
			if (PSG->handler !=0) PSG->handler();*/
		}
	}
	if (PSG->TimB)
	{
		PSG->TimBVal -= ((endp - PSG->bufp)<<TIMER_SH);
		if (PSG->TimBVal<=0)
		{
			PSG->TimB=0;
			PSG->TimIRQ|=2;
			/* ASG 980324 - handled by real timers now
			if (PSG->handler !=0) PSG->handler();*/
		}
	}


OP0 = &PSG->Oscils[0     ];
OP1 = &PSG->Oscils[0 +  8];
OP2 = &PSG->Oscils[0 + 16];
OP3 = &PSG->Oscils[0 + 24];

for( PSGBUF = &BuffTemp[PSG->bufp]; PSGBUF < &BuffTemp[endp]; PSGBUF++ )
{
/*chan0*/
	envelope_calc[OP0->state](OP0);
	envelope_calc[OP1->state](OP1);
	envelope_calc[OP2->state](OP2);
	envelope_calc[OP3->state](OP3);

	wy = op_calc(OP0, OP0->OscilFB);
	if (PSG->FeedBack[0])
		OP0->OscilFB = wy >> PSG->FeedBack[0];
	else
		OP0->OscilFB = 0;

	switch(PSG->ConnectTab[0])
	{
	case 0:	*(PSGBUF) = op_calc(OP3, op_calc(OP1, op_calc(OP2,wy) ) );   break;
	case 1:	*(PSGBUF) = op_calc(OP3, op_calc(OP1, op_calc(OP2,0)+wy ) ); break;
	case 2:	*(PSGBUF) = op_calc(OP3, op_calc(OP1, op_calc(OP2,0)) +wy ); break;
	case 3:	*(PSGBUF) = op_calc(OP3, op_calc(OP2, wy)+op_calc(OP1,0) );  break;
	case 4:	*(PSGBUF) = op_calc(OP2,wy) + op_calc(OP3, op_calc(OP1,0));  break;
	case 5:	*(PSGBUF) = op_calc(OP2,wy) + op_calc(OP1, wy) + op_calc(OP3,wy); break;
	case 6:	*(PSGBUF) = op_calc(OP2,wy) + op_calc(OP1,0) + op_calc(OP3,0);    break;
	default:*(PSGBUF) = wy + op_calc(OP2,0) + op_calc(OP1,0) + op_calc(OP3,0);break;
	}
#ifdef SAVE_SEPARATE_CHANNELS
fputc((unsigned short)(*PSGBUF)&0xff,sample1);
fputc(((unsigned short)(*PSGBUF)>>8)&0xff,sample1);
#endif
}


/*chan1*/
OP0 = &PSG->Oscils[1     ];
OP1 = &PSG->Oscils[1 +  8];
OP2 = &PSG->Oscils[1 + 16];
OP3 = &PSG->Oscils[1 + 24];


for( PSGBUF = &BuffTemp[PSG->bufp]; PSGBUF < &BuffTemp[endp]; PSGBUF++ )
{

	envelope_calc[OP0->state](OP0);
	envelope_calc[OP1->state](OP1);
	envelope_calc[OP2->state](OP2);
	envelope_calc[OP3->state](OP3);

	wy = op_calc(OP0, OP0->OscilFB);
	if (PSG->FeedBack[1])
			OP0->OscilFB = wy >> PSG->FeedBack[1];
	else
			OP0->OscilFB = 0;
#ifdef SAVE_SEPARATE_CHANNELS
	pom=*(PSGBUF);
#endif
	switch(PSG->ConnectTab[1])
	{
	case 0:	*(PSGBUF) += op_calc(OP3, op_calc(OP1, op_calc(OP2,wy) ) );   break;
	case 1:	*(PSGBUF) += op_calc(OP3, op_calc(OP1, op_calc(OP2,0)+wy ) ); break;
	case 2:	*(PSGBUF) += op_calc(OP3, op_calc(OP1, op_calc(OP2,0)) +wy ); break;
	case 3:	*(PSGBUF) += op_calc(OP3, op_calc(OP2, wy)+op_calc(OP1,0) );  break;
	case 4:	*(PSGBUF) += op_calc(OP2,wy) + op_calc(OP3, op_calc(OP1,0));  break;
	case 5:	*(PSGBUF) += op_calc(OP2,wy) + op_calc(OP1, wy) + op_calc(OP3,wy); break;
	case 6:	*(PSGBUF) += op_calc(OP2,wy) + op_calc(OP1,0) + op_calc(OP3,0);    break;
	default:*(PSGBUF) += wy + op_calc(OP2,0) + op_calc(OP1,0) + op_calc(OP3,0);break;
	}
#ifdef SAVE_SEPARATE_CHANNELS
fputc((unsigned short)((*PSGBUF)-pom)&0xff,sample2);
fputc(((unsigned short)((*PSGBUF)-pom)>>8)&0xff,sample2);
#endif
}


/*chan2*/
	OP0 = &PSG->Oscils[2     ];
	OP1 = &PSG->Oscils[2 +  8];
	OP2 = &PSG->Oscils[2 + 16];
	OP3 = &PSG->Oscils[2 + 24];

for( PSGBUF = &BuffTemp[PSG->bufp]; PSGBUF < &BuffTemp[endp]; PSGBUF++ )
{
	envelope_calc[OP0->state](OP0);
	envelope_calc[OP1->state](OP1);
	envelope_calc[OP2->state](OP2);
	envelope_calc[OP3->state](OP3);

	wy = op_calc(OP0, OP0->OscilFB);
	if (PSG->FeedBack[2])
			OP0->OscilFB = wy >> PSG->FeedBack[2];
	else
			OP0->OscilFB = 0;

#ifdef SAVE_SEPARATE_CHANNELS
	pom=*(PSGBUF);
#endif
	switch(PSG->ConnectTab[2])
	{
	case 0:	*(PSGBUF) += op_calc(OP3, op_calc(OP1, op_calc(OP2,wy) ) );   break;
	case 1:	*(PSGBUF) += op_calc(OP3, op_calc(OP1, op_calc(OP2,0)+wy ) ); break;
	case 2:	*(PSGBUF) += op_calc(OP3, op_calc(OP1, op_calc(OP2,0)) +wy ); break;
	case 3:	*(PSGBUF) += op_calc(OP3, op_calc(OP2, wy)+op_calc(OP1,0) );  break;
	case 4:	*(PSGBUF) += op_calc(OP2,wy) + op_calc(OP3, op_calc(OP1,0));  break;
	case 5:	*(PSGBUF) += op_calc(OP2,wy) + op_calc(OP1, wy) + op_calc(OP3,wy); break;
	case 6:	*(PSGBUF) += op_calc(OP2,wy) + op_calc(OP1,0) + op_calc(OP3,0);    break;
	default:*(PSGBUF) += wy + op_calc(OP2,0) + op_calc(OP1,0) + op_calc(OP3,0);break;
	}
#ifdef SAVE_SEPARATE_CHANNELS
fputc((unsigned short)((*PSGBUF)-pom)&0xff,sample3);
fputc(((unsigned short)((*PSGBUF)-pom)>>8)&0xff,sample3);
#endif
}

/*chan3*/
	OP0 = &PSG->Oscils[3     ];
	OP1 = &PSG->Oscils[3 +  8];
	OP2 = &PSG->Oscils[3 + 16];
	OP3 = &PSG->Oscils[3 + 24];

for( PSGBUF = &BuffTemp[PSG->bufp]; PSGBUF < &BuffTemp[endp]; PSGBUF++ )
{
	envelope_calc[OP0->state](OP0);
	envelope_calc[OP1->state](OP1);
	envelope_calc[OP2->state](OP2);
	envelope_calc[OP3->state](OP3);

	wy = op_calc(OP0, OP0->OscilFB);
	if (PSG->FeedBack[3])
			OP0->OscilFB = wy >> PSG->FeedBack[3];
	else
			OP0->OscilFB = 0;

#ifdef SAVE_SEPARATE_CHANNELS
	pom=*(PSGBUF);
#endif
	switch(PSG->ConnectTab[3])
	{
	case 0:	*(PSGBUF) += op_calc(OP3, op_calc(OP1, op_calc(OP2,wy) ) );   break;
	case 1:	*(PSGBUF) += op_calc(OP3, op_calc(OP1, op_calc(OP2,0)+wy ) ); break;
	case 2:	*(PSGBUF) += op_calc(OP3, op_calc(OP1, op_calc(OP2,0)) +wy ); break;
	case 3:	*(PSGBUF) += op_calc(OP3, op_calc(OP2, wy)+op_calc(OP1,0) );  break;
	case 4:	*(PSGBUF) += op_calc(OP2,wy) + op_calc(OP3, op_calc(OP1,0));  break;
	case 5:	*(PSGBUF) += op_calc(OP2,wy) + op_calc(OP1, wy) + op_calc(OP3,wy); break;
	case 6:	*(PSGBUF) += op_calc(OP2,wy) + op_calc(OP1,0) + op_calc(OP3,0);    break;
	default:*(PSGBUF) += wy + op_calc(OP2,0) + op_calc(OP1,0) + op_calc(OP3,0);break;
	}
#ifdef SAVE_SEPARATE_CHANNELS
fputc((unsigned short)((*PSGBUF)-pom)&0xff,sample4);
fputc(((unsigned short)((*PSGBUF)-pom)>>8)&0xff,sample4);
#endif
}

/*chan4*/
	OP0 = &PSG->Oscils[4     ];
	OP1 = &PSG->Oscils[4 +  8];
	OP2 = &PSG->Oscils[4 + 16];
	OP3 = &PSG->Oscils[4 + 24];

for( PSGBUF = &BuffTemp[PSG->bufp]; PSGBUF < &BuffTemp[endp]; PSGBUF++ )
{
	envelope_calc[OP0->state](OP0);
	envelope_calc[OP1->state](OP1);
	envelope_calc[OP2->state](OP2);
	envelope_calc[OP3->state](OP3);

	wy = op_calc(OP0, OP0->OscilFB);
	if (PSG->FeedBack[4])
			OP0->OscilFB = wy >> PSG->FeedBack[4];
	else
			OP0->OscilFB = 0;

#ifdef SAVE_SEPARATE_CHANNELS
	pom=*(PSGBUF);
#endif
	switch(PSG->ConnectTab[4])
	{
	case 0:	*(PSGBUF) += op_calc(OP3, op_calc(OP1, op_calc(OP2,wy) ) );   break;
	case 1:	*(PSGBUF) += op_calc(OP3, op_calc(OP1, op_calc(OP2,0)+wy ) ); break;
	case 2:	*(PSGBUF) += op_calc(OP3, op_calc(OP1, op_calc(OP2,0)) +wy ); break;
	case 3:	*(PSGBUF) += op_calc(OP3, op_calc(OP2, wy)+op_calc(OP1,0) );  break;
	case 4:	*(PSGBUF) += op_calc(OP2,wy) + op_calc(OP3, op_calc(OP1,0));  break;
	case 5:	*(PSGBUF) += op_calc(OP2,wy) + op_calc(OP1, wy) + op_calc(OP3,wy); break;
	case 6:	*(PSGBUF) += op_calc(OP2,wy) + op_calc(OP1,0) + op_calc(OP3,0);    break;
	default:*(PSGBUF) += wy + op_calc(OP2,0) + op_calc(OP1,0) + op_calc(OP3,0);break;
	}
#ifdef SAVE_SEPARATE_CHANNELS
fputc((unsigned short)((*PSGBUF)-pom)&0xff,sample5);
fputc(((unsigned short)((*PSGBUF)-pom)>>8)&0xff,sample5);
#endif
}

/*chan5*/
	OP0 = &PSG->Oscils[5     ];
	OP1 = &PSG->Oscils[5 +  8];
	OP2 = &PSG->Oscils[5 + 16];
	OP3 = &PSG->Oscils[5 + 24];

for( PSGBUF = &BuffTemp[PSG->bufp]; PSGBUF < &BuffTemp[endp]; PSGBUF++ )
{
	envelope_calc[OP0->state](OP0);
	envelope_calc[OP1->state](OP1);
	envelope_calc[OP2->state](OP2);
	envelope_calc[OP3->state](OP3);

	wy = op_calc(OP0, OP0->OscilFB);
	if (PSG->FeedBack[5])
			OP0->OscilFB = wy >> PSG->FeedBack[5];
	else
			OP0->OscilFB = 0;

#ifdef SAVE_SEPARATE_CHANNELS
	pom=*(PSGBUF);
#endif
	switch(PSG->ConnectTab[5])
	{
	case 0:	*(PSGBUF) += op_calc(OP3, op_calc(OP1, op_calc(OP2,wy) ) );   break;
	case 1:	*(PSGBUF) += op_calc(OP3, op_calc(OP1, op_calc(OP2,0)+wy ) ); break;
	case 2:	*(PSGBUF) += op_calc(OP3, op_calc(OP1, op_calc(OP2,0)) +wy ); break;
	case 3:	*(PSGBUF) += op_calc(OP3, op_calc(OP2, wy)+op_calc(OP1,0) );  break;
	case 4:	*(PSGBUF) += op_calc(OP2,wy) + op_calc(OP3, op_calc(OP1,0));  break;
	case 5:	*(PSGBUF) += op_calc(OP2,wy) + op_calc(OP1, wy) + op_calc(OP3,wy); break;
	case 6:	*(PSGBUF) += op_calc(OP2,wy) + op_calc(OP1,0) + op_calc(OP3,0);    break;
	default:*(PSGBUF) += wy + op_calc(OP2,0) + op_calc(OP1,0) + op_calc(OP3,0);break;
	}
#ifdef SAVE_SEPARATE_CHANNELS
fputc((unsigned short)((*PSGBUF)-pom)&0xff,sample6);
fputc(((unsigned short)((*PSGBUF)-pom)>>8)&0xff,sample6);
#endif
}

/*chan6*/
	OP0 = &PSG->Oscils[6     ];
	OP1 = &PSG->Oscils[6 +  8];
	OP2 = &PSG->Oscils[6 + 16];
	OP3 = &PSG->Oscils[6 + 24];

for( PSGBUF = &BuffTemp[PSG->bufp]; PSGBUF < &BuffTemp[endp]; PSGBUF++ )
{
	envelope_calc[OP0->state](OP0);
	envelope_calc[OP1->state](OP1);
	envelope_calc[OP2->state](OP2);
	envelope_calc[OP3->state](OP3);

	wy = op_calc(OP0, OP0->OscilFB);
	if (PSG->FeedBack[6])
			OP0->OscilFB = wy >> PSG->FeedBack[6];
	else
			OP0->OscilFB = 0;

	switch(PSG->ConnectTab[6])
	{
	case 0:	*(PSGBUF) += op_calc(OP3, op_calc(OP1, op_calc(OP2,wy) ) );   break;
	case 1:	*(PSGBUF) += op_calc(OP3, op_calc(OP1, op_calc(OP2,0)+wy ) ); break;
	case 2:	*(PSGBUF) += op_calc(OP3, op_calc(OP1, op_calc(OP2,0)) +wy ); break;
	case 3:	*(PSGBUF) += op_calc(OP3, op_calc(OP2, wy)+op_calc(OP1,0) );  break;
	case 4:	*(PSGBUF) += op_calc(OP2,wy) + op_calc(OP3, op_calc(OP1,0));  break;
	case 5:	*(PSGBUF) += op_calc(OP2,wy) + op_calc(OP1, wy) + op_calc(OP3,wy); break;
	case 6:	*(PSGBUF) += op_calc(OP2,wy) + op_calc(OP1,0) + op_calc(OP3,0);    break;
	default:*(PSGBUF) += wy + op_calc(OP2,0) + op_calc(OP1,0) + op_calc(OP3,0);break;
	}
}

/*chan7*/
	OP0 = &PSG->Oscils[7     ];
	OP1 = &PSG->Oscils[7 +  8];
	OP2 = &PSG->Oscils[7 + 16];
	OP3 = &PSG->Oscils[7 + 24];

for( PSGBUF = &BuffTemp[PSG->bufp]; PSGBUF < &BuffTemp[endp]; PSGBUF++ )
{
	envelope_calc[OP0->state](OP0);
	envelope_calc[OP1->state](OP1);
	envelope_calc[OP2->state](OP2);
	envelope_calc[OP3->state](OP3);

	wy = op_calc(OP0, OP0->OscilFB);
	if (PSG->FeedBack[7])
		OP0->OscilFB = wy >> PSG->FeedBack[7];
	else
		OP0->OscilFB = 0;

	switch(PSG->ConnectTab[7])
	{
	case 0:	*(PSGBUF) += op_calc(OP3, op_calc(OP1, op_calc(OP2,wy) ) );   break;
	case 1:	*(PSGBUF) += op_calc(OP3, op_calc(OP1, op_calc(OP2,0)+wy ) ); break;
	case 2:	*(PSGBUF) += op_calc(OP3, op_calc(OP1, op_calc(OP2,0)) +wy ); break;
	case 3:	*(PSGBUF) += op_calc(OP3, op_calc(OP2, wy)+op_calc(OP1,0) );  break;
	case 4:	*(PSGBUF) += op_calc(OP2,wy) + op_calc(OP3, op_calc(OP1,0));  break;
	case 5:	*(PSGBUF) += op_calc(OP2,wy) + op_calc(OP1, wy) + op_calc(OP3,wy); break;
	case 6:	*(PSGBUF) += op_calc(OP2,wy) + op_calc(OP1,0) + op_calc(OP3,0);    break;
	default:*(PSGBUF) += wy + op_calc(OP2,0) + op_calc(OP1,0) + op_calc(OP3,0);break;
	}
}




BUF  = PSG->Buf;
PSGBUF = &BuffTemp[PSG->bufp];
for( i = PSG->bufp; i < endp; i++ )
{
	k = *(PSGBUF++);

#ifdef SAVE_SAMPLE
	fputc((unsigned short)(-k)&0xff,samplesum);
	fputc(((unsigned short)(-k)>>8)&0xff,samplesum);
#endif

	if (sample_16bit)
	{ /*16 bit mode*/
		k >>= FINAL_SH16;  /*AUDIO_CONV*/
		k <<= 2;
		if (k > 32767)
			k = 32767;
		else
			if (k < -32768)
				k = -32768;
		((unsigned short*)BUF)[i] = (unsigned short)k;
	}
	else
	{ /*8 bit mode*/
		k >>= FINAL_SH8;  /*AUDIO_CONV*/
		if (k > 127)
			k = 127;
		else
			if (k < -128)
				k = -128;
		((unsigned char*)BUF)[i] = (unsigned char)k;
	}

}

PSG->bufp = endp;

}


/*
** called to update all chips
*/
void YM2151Update(void)
{
    int i;
    for ( i = 0 ; i < YMNumChips; i++ )
    {
	if( YMPSG[i].bufp <  YMBufSize )
		YM2151UpdateOne(i , YMBufSize );
	YMPSG[i].bufp = 0;
    }
}

/*
** return the buffer into which YM2151Update() has just written it's sample
** data
*/
SAMPLE *YMBuffer(int n)
{
    return YMPSG[n].Buf;
}


void YMSetIrqHandler(int n, void(*handler)(void))
{
    YMPSG[n].handler = handler;
}
