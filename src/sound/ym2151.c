#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "driver.h"
#include "ym2151.h"

/*
**  some globals ...
*/

/*own PI definition */
#ifdef PI
	#undef PI
#endif
#define PI 3.14159265358979323846


/*undef this if you don't want MAME timer system to be used*/
#define USE_MAME_TIMERS

/*#define FM_EMU*/

#ifdef FM_EMU
	FILE *errorlog = NULL;
	#define INLINE inline
	#ifdef USE_MAME_TIMERS
		#undef USE_MAME_TIMERS
	#endif
#endif


typedef struct{			/*oscillator data */
	unsigned int phase;	/*accumulated oscillator phase */
	unsigned int freq;	/*oscillator frequency */

/*Begin of channel specific data*/
/*The trick : each operator number 0 contains channel specific data*/
	signed   int OscilFB;	/*oscillator self feedback value used only by operators 0*/
	unsigned int FeedBack;	/*feedback shift value for operators 0 in each channel*/
	unsigned int KC;		/*KC for each operator (the same for all operators)*/
	unsigned int KF;		/*KF for each operator (the same for all operators)*/
	unsigned int PMS;		/*PMS for each channel*/
	unsigned int AMS;		/*AMS for each channel*/
/*End of channel specific data*/

	unsigned int volume;	/*oscillator volume*/
	signed   int a_volume;	/*used for attack phase calculations*/

	unsigned int TL;		/*Total attenuation Level*/
	unsigned int delta_AR;	/*volume delta for attack phase */
	unsigned int delta_D1R;	/*volume delta for decay  phase */
	unsigned int D1L;		/*when envelope reaches this level Envelope Generator goes into D2R*/
	unsigned int delta_D2R;	/*volume delta for sustain phase*/
	unsigned int delta_RR;	/*volume delta for release phase*/
	unsigned int state;		/*Envelope state: 4-attack(AR) 3-decay(D1R) 2-sustain(D2R) 1-release(RR) 0-off*/

	unsigned int AMSmask;	/*LFO AMS enable mask*/
	signed   int LFOpm;		/*phase modulation from LFO*/

	signed   int *connect;	/*oscillator output direction*/

	unsigned int key;		/*0=last key was KEY OFF, 1=last key was KEY ON*/

	unsigned int KS;		/*Key Scale     */
	unsigned int AR;		/*Attack rate   */
	unsigned int D1R;		/*Decay rate    */
	unsigned int D2R;		/*Sustain rate  */
	unsigned int RR;		/*Release rate  */
	unsigned int mul;		/*Phase multiply*/
	unsigned int DT1;		/*DT1 index*32  */
	unsigned int DT2;		/*DT2 index     */
	unsigned int KCindex;	/*used for LFO pm calculations */
	unsigned int DT1val;	/*used for LFO pm calculations */

} OscilRec;

/* here's the virtual YM2151 */
typedef struct ym2151_v{
	OscilRec Oscils[32];	/*there are 32 operators in YM2151*/

	unsigned int LFOphase;	/*accumulated LFO phase         */
	unsigned int LFOfrq;	/*LFO frequency                 */
	unsigned int LFOwave;	/*LFO waveform (0-saw, 1-square, 2-triangle, 3-random noise)*/
	unsigned int PMD;		/*LFO Phase Modulation Depth    */
	unsigned int AMD;		/*LFO Amplitude Modulation Depth*/
	unsigned int LFA;		/*current AM from LFO*/
	signed   int LFP;		/*current PM from LFO*/

	unsigned int CT;		/*output control pins (bit7 CT2, bit6 CT1)*/
	unsigned int noise;		/*noise register (bit 7 - noise enable, bits 4-0 - noise freq*/

	unsigned int IRQenable;	/*IRQ enable for timer B (bit 3) and timer A (bit 2)*/
	unsigned int status;	/*chip status (BUSY, IRQ Flags)*/

#ifdef USE_MAME_TIMERS
	void *TimATimer,*TimBTimer;	/* ASG 980324 -- added for tracking timers */
	double TimerATime[1024];	/*Timer A times for MAME*/
	double TimerBTime[256];		/*Timer B times for MAME*/
#else
	int TimA,TimB;				/*timer A,B enable (0-disabled)*/
	signed int TimAVal,TimBVal;	/*current value of timer*/
	unsigned int TimerA[1024];	/*Timer A deltas*/
	unsigned int TimerB[256];	/*Timer B deltas*/
#endif
	unsigned int TimAIndex,TimBIndex;/*timers' indexes*/

	/*
	*   Frequency-deltas to get the closest frequency possible.
	*   There're 11 octaves because of DT2 (max 950 cents over base frequency)
	*   and LFO phase modulation (max 700 cents below AND over base frequency)
	*   Summary:   octave  explanation
	*              0       note code - LFO PM
	*              1       note code
	*              2       note code
	*              3       note code
	*              4       note code
	*              5       note code
	*              6       note code
	*              7       note code
	*              8       note code
	*              9       note code + DT2 + LFO PM
	*              10      note code + DT2 + LFO PM
	*/
	unsigned int freq[11*12*64];/*11 octaves, 12 semitones, 64 'cents'*/

	/*
	*   Frequency deltas for DT1. These deltas alter operator frequency
	*   after it has been taken from frequency-deltas table.
	*/
	signed   int DT1freq[8*32];	/*8 DT1 levels, 32 DT1 values*/

	unsigned int LFOfreq[256];	/*frequency deltas for LFO*/

	unsigned int A_Time[64+32];	/*attack deltas (64 keycodes + 32 RKS's)*/
	unsigned int D_Time[64+32];	/*decay  deltas (64 keycodes + 32 RKS's)*/

	unsigned int PAN[16];		/*channels output masks (0xffffffff = enable)*/

	void (*irqhandler)(int irq);				/*IRQ function handler*/
	void (*porthandler)(int offset, int data);	/*port write handler*/

	unsigned int clock;			/* this will be passed from 2151intf.c */
	unsigned int sampfreq;		/* this will be passed from 2151intf.c */

} YM2151;



/*
**  Shifts below are subject to change when sampling frequency changes...
*/
#define FREQ_SH  16  /* 16.16 fixed point                           */
#define ENV_SH   16  /* 16.16 fixed point for envelope calculations */
#define LFO_SH   24  /*  8.24 fixed point for LFO calculations      */
#define TIMER_SH 16  /* 16.16 fixed point for timers calculations   */

/*#define NEW_WAY*/
/*undef this if you want real dB output*/

/*if you change these values (ENV_SOMETHING) you have to take care of attack_curve*/
#define ENV_BITS		10
#define ENV_LEN			(1<<ENV_BITS)
#define ENV_STEP		(96.0/ENV_LEN)
#define ENV_STEPN		(127.0/ENV_LEN)

#define MAX_VOL_INDEX	((ENV_LEN-1)<<ENV_SH)
#define MIN_VOL_INDEX	((1<<ENV_SH)-1)
#define VOLUME_OFF		(ENV_LEN<<ENV_SH)

#define SIN_BITS		11
#define SIN_LEN			(1<<SIN_BITS)
#define SIN_MASK		(SIN_LEN-1)
#define SIN_AMPLITUDE_BITS (SIN_BITS+2+FREQ_SH)
/*
** SIN_AMPLITUDE_BITS = 11+2+16=29 bits
*/

#define LFO_BITS		8
#define LFO_LEN			(1<<LFO_BITS)
#define LFO_MASK		(LFO_LEN-1)

#define EG_ATT		4
#define EG_DEC		3
#define EG_SUS		2
#define EG_REL		1
#define EG_OFF		0

#if (SAMPLE_BITS==16)
	#define FINAL_SH	0
	#define MAXOUT		32767
	#define MINOUT		-32768
#else
	#define FINAL_SH	8
	#define MAXOUT		127
	#define MINOUT		-128
#endif


/*
 *  Offset in this table is calculated as:
 *
 *  1st ENV_LEN:
 *     TL- main offset (Total attenuation Level); range 0 to ENV_LEN (0-96 dB)
 *  2nd ENV_LEN:
 *     current envelope value of the operator;    range 0 to ENV_LEN (0-96 dB)
 *  3rd ENV_LEN:
 *     Amplitude Modulation from LFO;             range 0 to ENV_LEN (0-96 dB)
 *  4th SIN_LEN:
 *     Sin Wave Offset from sin_tab, range 0 to about 56 dB only, but lets
 *     simplify things and assume sin could be 96 dB, range 0 to 1023
 *
 *  Length of this table is doubled because we have two separate parts
 *  for positive and negative halves of sin wave (above and below X axis).
*/
#define TL_TAB_LEN (2*(ENV_LEN + ENV_LEN + ENV_LEN + SIN_LEN))
static signed int * TL_TAB = NULL;

/* sin waveform table in decibel scale*/
static signed int * sin_tab[SIN_LEN];

/*decibel table to convert from D1L values to volume index*/
static unsigned int D1LTab[16];

/*attack curve for envelope attack phase*/
static unsigned int attack_curve[ENV_LEN];

/*
 *   Translate from key code KC (OCT2 OCT1 OCT0 N3 N2 N1 N0) to
 *   index in frequency-deltas table.
*/
static unsigned int KC_TO_INDEX[8*16];  /*8 octaves * 16 note codes*/


static YM2151 * PSG;
static YM2151 * YMPSG = NULL;	/* array of YM2151's */
static unsigned int YMNumChips;		/* total # of YM2151's emulated */


/*these variables stay here because of speedup purposes only */
static unsigned int mask[16];
static signed int chanout;
static signed int c1,m2,c2; /*Phase Modulation input for operators 2,3,4*/



/*save output as raw 16-bit sample*/
/*#define SAVE_SAMPLE*/
/*#define SAVE_SEPARATE_CHANNELS*/

#if defined SAVE_SAMPLE || defined SAVE_SEPARATE_CHANNELS
static FILE *sample[9];
#endif

/*
 *   DT1 defines offset in Hertz from base note * mul
 *
 *   This table is converted while initialization...
*/
static unsigned char DT1Tab[4*32]={ /* 4*32 DT1 values */
/*DT_TABLE based on YM2608 docs (which is correct)*/
/* DT1=0 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* DT1=1 */
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2,
  2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8, 8, 8,
/* DT1=2 */
  1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5,
  5, 6, 6, 7, 8, 8, 9,10,11,12,13,14,16,16,16,16,
/* DT1=3 */
  2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7,
  8, 8, 9,10,11,12,13,14,16,17,19,20,22,22,22,22
};

/*
 *   DT2 defines offset in cents from base note
 *
 *   This table defines offset in frequency-deltas table.
 *   User's Manual page 22
 *
 *   Values below were calculated using formula: value =  orig.val / 1.5625
 *
 *	DT2=0 DT2=1 DT2=2 DT2=3
 *	0     600   781   950
*/
static unsigned int DT2Tab[4]={ 0, 384, 500, 608 };


/*
 * This data is valid ONLY for YM2151. Other Yamaha's FM chips seem to use
 * different shape of envelope attack. I'm quite sure that curve of OPL2/3
 * is similiar, but much smoother. These chips seem to use something
 * like "a 42dB curve".
 *
 *    This data was reverse-engineered by me (Jarek Burczynski). Keep in mind
 * that I spent a _lot_ of time researching this information, so be so kind as
 * to send me an e-mail before you use it, please.
 *
 * 10-bits decibel values with bits weighting:
 *
 * bit | 9  | 8  | 7  | 6 | 5 | 4   | 3    | 2     | 1      | 0       |
 * ----+----+----+----+---+---+-----+------|-------|--------|---------|
 * dB  | 48 | 24 | 12 | 6 | 3 | 1.5 | 0.75 | 0.375 | 0.1875 | 0.09375 |
 *
*/
static unsigned int AttackTab[73]={
	0  ,1  ,2  ,3  ,4  ,5  ,6  ,7  ,8  ,9  ,10 ,11 ,12 ,13 ,14 ,16,  /*16*/
	18 ,20 ,22 ,24 ,26 ,28 ,31 ,34 ,37 ,40 ,43 ,46 ,50 ,54 ,58 ,63,  /*16*/
	68 ,73 ,78 ,84 ,90 ,97 ,104,111,120,129,138,148,158,168,180,193, /*16*/
	207,222,238,254,271,289,308,329,352,377,404,433,464,498,535,575, /*16*/
	618,664,713,766,823,884,950,1021,1023  /*9*/
};


void init_tables(void)
{
	signed int i,x;
	double m;

	for(i=0; i<SIN_LEN; i++)
	{
#if 1
		/*standard sinus*/
		m = sin(i*2*PI/SIN_LEN);
#else
		/*sidebanded sinus*/
		m = 0.0;
		for (x=1; x<=15; x+=2) /*sidebands: 1st, 3rd, 5th, 7th .. 15th*/
		{
			double dB;
			dB = 18 * log(x)/log(2); /*18 dB per octave*/
			m = m+ ( sin(x*i*2*PI/SIN_LEN) / pow(10,dB/20) );
		}
#endif
		if ( (m<0.0001) && (m>-0.0001) ){     /*is m near zero ?*/
			m = ENV_LEN-1;
		}else{
			if (m>0.0){
/*if (errorlog) fprintf(errorlog,"sin(%i) => %8.6f  %8.6f\n",i, 8.0*log(1.0/m)/log(2), 20*log10(1.0/m) );*/
#ifdef NEW_WAY
				m = 8*log(1.0/m)/log(2);  /*and how many "decibels" is it?*/
				m = m / ENV_STEPN;
#else
				m = 20*log10(1.0/m);
				m = m / ENV_STEP;
#endif
			}else{
/*if (errorlog) fprintf(errorlog,"sin(%i) => %8.6f  %8.6f\n",i, 8.0*log(-1.0/m)/log(2), 20*log10(-1.0/m) );*/
#ifdef NEW_WAY
				m = 8*log(-1.0/m)/log(2); /*and how many "decibels" is it?*/
				m = (m / ENV_STEPN) + TL_TAB_LEN/2;
#else
				m = 20*log10(-1.0/m);
				m = (m / ENV_STEP) + TL_TAB_LEN/2;
#endif
			}
		}
		sin_tab[ i ] = &TL_TAB[ (unsigned int)m ];
		/*if (errorlog) fprintf(errorlog,"sin %i = %i\n",i,sin_tab[i] );*/
	}

	for( x=0; x<TL_TAB_LEN/2; x++ )
	{
		if (x<ENV_LEN)
		{
		#if 0
			if (errorlog) fprintf(errorlog,"TL_TAB[%i] => %8.6f  %8.6f\n",x,
			   ( (1<<SIN_AMPLITUDE_BITS) ) / pow(2 , x * ENV_STEPN / 8.0 ),
			   ( (1<<SIN_AMPLITUDE_BITS) ) / pow(10, x * ENV_STEP  / 20.0)
			);
		#endif
#ifdef NEW_WAY
			m = ( (1<<SIN_AMPLITUDE_BITS) ) / pow(2, x * ENV_STEPN / 8.0);
#else
			m = ( ( (1<<(SIN_AMPLITUDE_BITS-FREQ_SH)) -1) << FREQ_SH ) / pow(10, x * ENV_STEP / 20.0);
#endif
		}
		else
		{
			m = 0;
		}
		TL_TAB[         x        ] = m;
		TL_TAB[ x + TL_TAB_LEN/2 ] = -m;
		/*if (errorlog) fprintf(errorlog,"tl %04i =%08x\n",x,TL_TAB[x]);*/
	}

	/* create (almost perfect) attack curve */
	for (i=0; i<ENV_LEN; i++)                     /*clear the curve*/
		attack_curve[i] = MAX_VOL_INDEX;

	for (i=0; i<73; i++){                         /*create curve*/
		for (x=i*14; x<i*14+14; x++){
			attack_curve[x] = ((int)((double)AttackTab[i] * 0.09375 / (double)ENV_STEP ) ) << ENV_SH;
/*if (errorlog) fprintf(errorlog,"attack [%04x] = %f dB\n", x, (double)(attack_curve[x]>>ENV_SH) * (double)ENV_STEP );*/
		}
	}

	for (i=0; i<16; i++)
	{
#ifdef NEW_WAY
		m = (i<15?i:i+16) * (3.0/ENV_STEPN);   /*every 3 "dB" except for all bits = '1' = 45dB+48dB*/
#else
		m = (i<15?i:i+16) * (3.0/ENV_STEP);   /*every 3 dB except for all bits = '1' = 45dB+48dB*/
#endif
		D1LTab[i] = m * (1<<ENV_SH);
		/*if (errorlog) fprintf(errorlog,"D1LTab[%04x]=%08x\n",i,D1LTab[i] );*/
	}

	/* calculate KC to index table */
	x=0;
	for (i=0; i<16*8; i++)
	{
		KC_TO_INDEX[i]=((i>>4)*12*64) + x*64 + 12*64;
		if ((i&3)!=3) x++;	/* change note code */
		if ((i&15)==15) x=0;	/* new octave */
	}

#ifdef FM_EMU
errorlog=fopen("errorlog.txt","wb");
#endif

#ifdef SAVE_SAMPLE
sample[8]=fopen("sampsum.raw","wb");
#endif
#ifdef SAVE_SEPARATE_CHANNELS
sample[0]=fopen("samp0.raw","wb");
sample[1]=fopen("samp1.raw","wb");
sample[2]=fopen("samp2.raw","wb");
sample[3]=fopen("samp3.raw","wb");
sample[4]=fopen("samp4.raw","wb");
sample[5]=fopen("samp5.raw","wb");
sample[6]=fopen("samp6.raw","wb");
sample[7]=fopen("samp7.raw","wb");
#endif
}


void init_chip_tables(YM2151 *chip)
{
	int i,j;
	double mult,pom,pom2,clk,phaseinc,Hz;

	double scaler;	/* formula below is true for chip clock=3579545 */
	/* so we need to scale its output accordingly to the chip clock */

	scaler= (double)chip->clock / 3579545.0;

	/*this loop calculates Hertz values for notes from c#0 to c-8*/
	/*including 64 'cents' (100/64 that is 1.5625 of real cent) for each semitone*/
	/* i*100/64/1200 is equal to i/768 */

	mult = (1<<FREQ_SH);
#if 0
	for (i=0; i<11*12*64; i++){
		/*3.4375Hz is note A; C# is 4 semitones higher*/
		Hz = scaler * 3.4375 * pow (2, (i+4*64) / 768.0 );
		/*calculate phase increment*/
		phaseinc = (Hz*SIN_LEN) / (double)chip->sampfreq;
		chip->freq[i] = phaseinc*mult;
		#if 0
		if (errorlog){
			pom = (double)chip->freq[i] / mult;
			pom = pom * (double)chip->sampfreq / (double)SIN_LEN;
			fprintf(errorlog,"freq[%04i][%08x]= real %20.15f Hz  emul %20.15f Hz\n", i, chip->freq[i], Hz, pom);
		}
		#endif
	}
#else
	for (i=0; i<1*12*64; i++)
	{
		/*3.4375Hz is note A; C# is 4 semitones higher*/
		Hz = scaler * 3.4375 * pow (2, (i+4*64) / 768.0 );

		/*calculate phase increment*/
		phaseinc = (Hz*SIN_LEN) / (double)chip->sampfreq;

		chip->freq[i] = phaseinc*mult;
		for (j=1; j<11; j++)
		{
			chip->freq[i+j*12*64] = chip->freq[i]*(1<<j);
		}

		#if 0
		if (errorlog)
		{
			pom = (double)chip->freq[i] / mult;
			pom = pom * (double)chip->sampfreq / (double)SIN_LEN;
			fprintf(errorlog,"freq[%04i][%08x]= real %20.15f Hz  emul %20.15f Hz\n", i, chip->freq[i], Hz, pom);
		}
		#endif
	}
#endif

	mult = (1<<FREQ_SH);
	for (j=0; j<4; j++)
	{
		for (i=0; i<32; i++)
		{
			Hz = ( (double)DT1Tab[j*32+i] * ((double)chip->clock/8.0) ) / (double)(1<<24);
			/*calculate phase increment*/
			phaseinc = (Hz*SIN_LEN) / (double)chip->sampfreq;

			/*positive and negative values*/
			chip->DT1freq[ (j+0)*32 + i ] = phaseinc * mult;
			chip->DT1freq[ (j+4)*32 + i ] = -chip->DT1freq[ (j+0)*32 + i ];
#if 0
			if (errorlog)
			{
				int x = j*32+i;
				pom = (double)chip->DT1freq[x] / mult;
				pom = pom * (double)chip->sampfreq / (double)SIN_LEN;
				fprintf(errorlog,"DT1(%03i)[%02i %02i][%08x]= real %20.15f Hz  emul %20.15f Hz\n", x, j, i, chip->DT1freq[x], Hz, pom);
			}
#endif
		}
	}

	mult = (1<<LFO_SH);
	clk  = (double)chip->clock;
	for (i=0; i<256; i++)
	{
		j = i & 0x0f;
		pom = fabs(  (clk/65536/(1<<(i/16)) ) - (clk/65536/32/(1<<(i/16)) * (j+1)) );

		/*calculate phase increment*/
		chip->LFOfreq[0xff-i] = ( (pom*LFO_LEN) / (double)chip->sampfreq ) * mult; /*fixed point*/
		/*if (errorlog) fprintf(errorlog, "LFO[%02x] = real %20.15f Hz  emul %20.15f Hz\n",0xff-i, pom,
			(((double)chip->LFOfreq[0xff-i] / mult) * (double)chip->sampfreq ) / (double)LFO_LEN );*/
	}

	for (i=0; i<4; i++)
		chip->A_Time[i] = chip->D_Time[i] = 0;		/*infinity*/

	for (i=4; i<64; i++)
	{
		pom2 = (double)chip->clock / (double)chip->sampfreq;
		if (i<60) pom2 *= ( 1.0 + (i&3)*0.25 );
		pom2 *= 1<<((i>>2)-1);
		pom2 *= (double)(1<<ENV_SH);
		if (i<52)
		{
			pom  = pom2 / 27876.0;   		/*AR scale value 0-12*/
		}
		else
		{
			if ( (i>=52) && (i<56) )
			{
				pom  = pom2 / 30600.0;   	/*AR scale value 13*/
			}
			else
			{
				if ( (i>=56) && (i<60) )
				{
					pom  = pom2 / 32200.0;	/*AR scale value 14*/
				}else{
					pom  = pom2 / 30400.0;	/*AR scale value 15*/
				}
			}
		}
		if (i>=62) pom=(ENV_LEN<<ENV_SH);
		pom2 = pom2 / 385303.47656;		/*DR scale value*/
		chip->A_Time[i] = pom;
		chip->D_Time[i] = pom2;
#if 0
		if (errorlog)
			fprintf(errorlog,"Rate %2i %1i  Attack [real %9.4f ms][emul %9.4f ms]  Decay [real %11.4f ms][emul %11.4f ms]\n",i>>2, i&3,
			( ((double)(ENV_LEN<<ENV_SH)) / pom )                     * (1000.0 / (double)chip->sampfreq),
			( ((double)(ENV_LEN<<ENV_SH)) / (double)chip->A_Time[i] ) * (1000.0 / (double)chip->sampfreq),
			( ((double)(ENV_LEN<<ENV_SH)) / pom2 )                    * (1000.0 / (double)chip->sampfreq),
			( ((double)(ENV_LEN<<ENV_SH)) / (double)chip->D_Time[i] ) * (1000.0 / (double)chip->sampfreq) );
#endif
	}

	for (i=0; i<32; i++)
	{
		chip->A_Time[ 64+i ] = chip->A_Time[63];
		chip->D_Time[ 64+i ] = chip->D_Time[63];
	}


	/* precalculate timers' deltas */
	/* User's Manual pages 15,16  */
	mult = (1<<TIMER_SH);
	for (i=0; i<1024; i++)
	{
		/* ASG 980324: changed to compute both TimerA and TimerATime */
		pom= ( 64.0  *  (1024.0-i) / (double)chip->clock );
		#ifdef USE_MAME_TIMERS
			chip->TimerATime[i] = pom;
		#else
			chip->TimerA[i] = pom * (double)chip->sampfreq * mult;  /*number of samples that timer period takes (fixed point) */
		#endif
	}
	for (i=0; i<256; i++)
	{
		/* ASG 980324: changed to compute both TimerB and TimerBTime */
		pom= ( 1024.0 * (256.0-i)  / (double)chip->clock );
		#ifdef USE_MAME_TIMERS
			chip->TimerBTime[i] = pom;
		#else
			chip->TimerB[i] = pom * (double)chip->sampfreq * mult;  /*number of samples that timer period takes (fixed point) */
		#endif
	}

}


#define RESET_VOLUME_ON_KEYON

/*
** This switch is defined here because I'm not _really_ sure if YM2151
** _always_ zeroes envelope volume after KEYON (or if it does at all).
** OPL3 on SoundBlaster does not change envelope volume when KeyOn'ed.
**
** Credit sound in Paper Boy and music tune in Toobin, both sound _better_
** when YM2151 zeroes the envelope volume after KEYON.
** On the contrary music in Magic Sword needs the volume unchanged (SOUND 02H
** in test mode - listen to the lead instrument from 45th second to about
** 87th second). Also tune (number 03 in sound test mode) in Golden Axe needs
** the volume unchanged (there are nice trills in it).
*/

INLINE void envelope_KONKOFF(OscilRec * op, int v)
{
	if (v&8)
	{
		if (!op->key){
			op->key = 1; /*KEYON'ed*/
		#ifdef RESET_VOLUME_ON_KEYON
			op->a_volume = op->volume = VOLUME_OFF; /*reset volume*/
		#else
			op->a_volume = op->volume;              /*don't reset volume*/
		#endif
			op->OscilFB = 0;
			op->phase   = 0;      /*clear feedback and phase */
			op->state   = EG_ATT; /*KEY ON = attack*/
		}
	}
	else
	{
		if (op->key){
			op->key   = 0;      /*KEYOFF'ed*/
			op->state = EG_REL; /*release*/
		}
	}

	op+=8;

	if (v&0x20)
	{
		if (!op->key){
			op->key = 1; /*KEYON'ed*/
		#ifdef RESET_VOLUME_ON_KEYON
			op->a_volume = op->volume = VOLUME_OFF; /*reset volume*/
		#else
			op->a_volume = op->volume;              /*don't reset volume*/
		#endif
			op->phase   = 0;      /*clear feedback and phase */
			op->state   = EG_ATT; /*KEY ON = attack*/
		}
	}
	else
	{
		if (op->key){
			op->key   = 0;      /*KEYOFF'ed*/
			op->state = EG_REL; /*release*/
		}
	}

	op+=8;

	if (v&0x10)
	{
		if (!op->key){
			op->key = 1; /*KEYON'ed*/
		#ifdef RESET_VOLUME_ON_KEYON
			op->a_volume = op->volume = VOLUME_OFF; /*reset volume*/
		#else
			op->a_volume = op->volume;              /*don't reset volume*/
		#endif
			op->phase   = 0;      /*clear feedback and phase */
			op->state   = EG_ATT; /*KEY ON = attack*/
		}
	}
	else
	{
		if (op->key){
			op->key   = 0;      /*KEYOFF'ed*/
			op->state = EG_REL; /*release*/
		}
	}

	op+=8;

	if (v&0x40)
	{
		if (!op->key){
			op->key = 1; /*KEYON'ed*/
		#ifdef RESET_VOLUME_ON_KEYON
			op->a_volume = op->volume = VOLUME_OFF; /*reset volume*/
		#else
			op->a_volume = op->volume;              /*don't reset volume*/
		#endif
			op->phase   = 0;      /*clear feedback and phase */
			op->state   = EG_ATT; /*KEY ON = attack*/
		}
	}
	else
	{
		if (op->key){
			op->key   = 0;      /*KEYOFF'ed*/
			op->state = EG_REL; /*release*/
		}
	}
}


#ifdef USE_MAME_TIMERS
static void timer_callback_a (int n)
{
	YM2151 *chip = &YMPSG[n];
	chip->TimATimer = timer_set (chip->TimerATime[ chip->TimAIndex ], n, timer_callback_a);
	if ( chip->IRQenable & 0x04 )
	{
		int oldstate = chip->status & 3;
		chip->status |= 1;
		if( (!oldstate) && (chip->irqhandler) ) (*chip->irqhandler)(1);
	}
}
static void timer_callback_b (int n)
{
	YM2151 *chip = &YMPSG[n];
	chip->TimBTimer = timer_set (chip->TimerBTime[ chip->TimBIndex ], n, timer_callback_b);
	if ( chip->IRQenable & 0x08 )
	{
		int oldstate = chip->status & 3;
		chip->status |= 2;
		if( (!oldstate) && (chip->irqhandler) ) (*chip->irqhandler)(1);
	}
}
#if 0
static void timer_callback_ChipWriteBusy (int n)
{
	YM2151 *chip = &YMPSG[n];
	chip->status &= 0x7f;	/*reset busy flag*/
}
#endif
#endif


static void set_connect_feedback( OscilRec *om1, int v )
{
	OscilRec *om2 = om1+8;
	OscilRec *oc1 = om1+16;
	OscilRec *oc2 = om1+24;

	om1->FeedBack = (8 - ((v>>3)&7) ) & 7;
	/*for values 0,1,2,3,4,5,6,7 this formula gives: 0,7,6,5,4,3,2,1*/

	/* set connect algorithm */
	oc2->connect = &chanout;
	switch( v & 7 ){
	case 0:
		/*  PG---M1---C1---M2---C2---OUT */
		om1->connect = &c1;
		oc1->connect = &m2;
		om2->connect = &c2;
		break;
	case 1:
		/*  PG---M1-+-M2---C2---OUT */
		/*  PG---C1-+               */
		om1->connect = &m2;
		oc1->connect = &m2;
		om2->connect = &c2;
		break;
	case 2:
		/* PG---M1------+-C2---OUT */
		/* PG---C1---M2-+          */
		om1->connect = &c2;
		oc1->connect = &m2;
		om2->connect = &c2;
		break;
	case 3:
		/* PG---M1---C1-+-C2---OUT */
		/* PG---M2------+          */
		om1->connect = &c1;
		oc1->connect = &c2;
		om2->connect = &c2;
		break;
	case 4:
		/* PG---M1---C1-+--OUT */
		/* PG---M2---C2-+      */
		om1->connect = &c1;
		oc1->connect = &chanout;
		om2->connect = &c2;
		break;
	case 5:
		/*         +-C1-+     */
		/* PG---M1-+-M2-+-OUT */
		/*         +-C2-+     */
		om1->connect = 0;	/* special mark */
		oc1->connect = &chanout;
		om2->connect = &chanout;
		break;
	case 6:
		/* PG---M1---C1-+     */
		/* PG--------M2-+-OUT */
		/* PG--------C2-+     */
		om1->connect = &c1;
		oc1->connect = &chanout;
		om2->connect = &chanout;
		break;
	case 7:
		/* PG---M1-+     */
		/* PG---C1-+-OUT */
		/* PG---M2-+     */
		/* PG---C2-+     */
		om1->connect = &chanout;
		oc1->connect = &chanout;
		om2->connect = &chanout;
		break;
	}
}


INLINE void refresh_EG( YM2151 *chip, int chan)
{
	OscilRec * op;
	unsigned int kc;
	unsigned char v;

	op = &chip->Oscils[chan];
	kc = op->KC;

	/* v = 2*RR + RKS (max 95)*/

	v = kc >> op->KS;
	op->delta_AR  = chip->A_Time[ op->AR + v];
	op->delta_D1R = chip->D_Time[op->D1R + v];
	op->delta_D2R = chip->D_Time[op->D2R + v];
	op->delta_RR  = chip->D_Time[ op->RR + v];

	op+=8;

	v = kc >> op->KS;
	op->delta_AR  = chip->A_Time[ op->AR + v];
	op->delta_D1R = chip->D_Time[op->D1R + v];
	op->delta_D2R = chip->D_Time[op->D2R + v];
	op->delta_RR  = chip->D_Time[ op->RR + v];

	op+=8;

	v = kc >> op->KS;
	op->delta_AR  = chip->A_Time[ op->AR + v];
	op->delta_D1R = chip->D_Time[op->D1R + v];
	op->delta_D2R = chip->D_Time[op->D2R + v];
	op->delta_RR  = chip->D_Time[ op->RR + v];

	op+=8;

	v = kc >> op->KS;
	op->delta_AR  = chip->A_Time[ op->AR + v];
	op->delta_D1R = chip->D_Time[op->D1R + v];
	op->delta_D2R = chip->D_Time[op->D2R + v];
	op->delta_RR  = chip->D_Time[ op->RR + v];

}


/* write a register on YM2151 chip number 'n' */
void YM2151WriteReg(int n, int r, int v)
{
	YM2151 *chip = &(YMPSG[n]);
	OscilRec *op = &chip->Oscils[ r&0x1f ];

#if 0
	/*There's no info on what YM-2151 really does when busy flag is set*/
	if ( chip->status & 0x80 ) return;
	timer_set ( 68.0 / (double)chip->clock, n, timer_callback_ChipWriteBusy);
	chip->status |= 0x80;    /* set busy flag for 68 chip clock cycles */
#endif

	switch(r & 0xe0){
	case 0x00:
		switch(r){
		case 0x01: /*LFO Reset(bit 1), Test Register (other bits)*/
			if (v&2) chip->LFOphase = 0;
			if (errorlog && (v&0xfd) )
				fprintf(errorlog,"\nYM2151 TEST MODE ON (%02x)\n",v);
			break;
		case 0x08:
			envelope_KONKOFF(&chip->Oscils[ (v&7) ], v );
			break;
		case 0x0f: /*Noise mode select, noise freq*/
			chip->noise = v;
			#if 0
			if (v!=0)
			{
				char t[80];
				sprintf(t,"YM2151 noise mode = %02x",v);
				usrintf_showmessage(t);
				if (errorlog) fprintf(errorlog,"YM2151 noise (%02x)\n",v);
			}
			#endif
			break;
		case 0x10: /*Timer A hi */
			chip->TimAIndex = (chip->TimAIndex & 0x03 ) | (v<<2);
			break;
		case 0x11: /*Timer A low*/
			chip->TimAIndex = (chip->TimAIndex & 0x3fc) | (v & 3);
			break;
		case 0x12: /*Timer B */
			chip->TimBIndex = v;
			break;
		case 0x14: /*CSM, irq flag reset, irq enable, timer start/stop*/
			if (errorlog && (v&0x80) )
				fprintf(errorlog,"\nYM2151 CSM MODE ON\n");
			chip->IRQenable = v;	/*bit 3-timer B, bit 2-timer A*/
			if (v&0x20)	/*FRESET B*/
			{
				int oldstate = chip->status & 3;
				chip->status &= 0xfd;
				if( (oldstate==2) && (chip->irqhandler) ) (*chip->irqhandler)(0);
			}
			if (v&0x10)	/*FRESET A*/
			{
				int oldstate = chip->status & 3;
				chip->status &= 0xfe;
				if( (oldstate==1) && (chip->irqhandler) ) (*chip->irqhandler)(0);
			}
			if (v&0x02){/*LOAD & START B*/
				#ifdef USE_MAME_TIMERS
				/* ASG 980324: added a real timer */
/*start timer _only_ if it wasn't already started*/
					if (chip->TimBTimer==0)
					{
						chip->TimBTimer = timer_set (chip->TimerBTime[ chip->TimBIndex ], n, timer_callback_b);
					}
				#else
					chip->TimB = 1;
					chip->TimBVal = chip->TimerB[ chip->TimBIndex ];
				#endif
			}else{		/*STOP B*/
				#ifdef USE_MAME_TIMERS
				/* ASG 980324: added a real timer */
					if (chip->TimBTimer) timer_remove (chip->TimBTimer);
					chip->TimBTimer = 0;
				#else
					chip->TimB = 0;
				#endif
			}
			if (v&0x01){/*LOAD & START A*/
				#ifdef USE_MAME_TIMERS
				/* ASG 980324: added a real timer */
/*start timer _only_ if it wasn't already started*/
					if (chip->TimATimer==0)
					{
						chip->TimATimer = timer_set (chip->TimerATime[ chip->TimAIndex ], n, timer_callback_a);
					}
				#else
					chip->TimA = 1;
					chip->TimAVal = chip->TimerA[ chip->TimAIndex ];
				#endif
			}else{		/*STOP A*/
				#ifdef USE_MAME_TIMERS
				/* ASG 980324: added a real timer */
					if (chip->TimATimer) timer_remove (chip->TimATimer);
					chip->TimATimer = 0;
				#else
					chip->TimA = 0;
				#endif
			}
			break;
		case 0x18: /*LFO FREQ*/
			chip->LFOfrq = chip->LFOfreq[ v ];
			break;
		case 0x19: /*PMD (bit 7=1) or AMD (bit 7=0) */
			if (v&0x80)
				chip->PMD = (v & 0x7f); /*7bits + sign bit*/
			else
				chip->AMD = ((v & 0x7f)<<1) | 1; /*7bits->8bits*/
			/*if (errorlog && (v&0x7f)) fprintf(errorlog,"YM2151 PMDAMD %02x\n",v);*/
			break;
		case 0x1b: /*CT2, CT1, LFO Waveform*/
			chip->CT = v;
			chip->LFOwave = v & 3;
			if (chip->porthandler) (*chip->porthandler)(0 , (chip->CT) >> 6 );
			if (errorlog && (chip->CT>>6) ) fprintf(errorlog,"YM2151 port CT=%02x\n",v);
			break;
		default:
			if (errorlog) fprintf(errorlog,"YM2151 Write %02x to undocumented register #%02x\n",v,r);
			break;
		}
		break;

	case 0x20:
		op = &chip->Oscils[ r&7 ];
		switch(r & 0x18){
		case 0x00: /*RL enable, Feedback, Connection */
			set_connect_feedback(op, v );
			chip->PAN[ (r&7)*2    ] = (v & 0x40) ? 0xffffffff : 0x0;
			chip->PAN[ (r&7)*2 +1 ] = (v & 0x80) ? 0xffffffff : 0x0;
			break;

		case 0x08: /*Key Code*/
			v &= 0x7f;
			if ( op->KC != v )
			{
				unsigned int kc,kc_oscil,kc_channel;

				op->KC = v;

				kc = op->KC;
				kc_channel = KC_TO_INDEX[kc] + op->KF;
				kc >>=2;

		/*calc freq begin operator 0*/
				kc_oscil = kc_channel + op->DT2;  /*DT2 offset*/
				op->freq = ( chip->freq[ kc_oscil ] + chip->DT1freq[ op->DT1 + kc ] ) * op->mul;
				/*op->KCindex = kc_oscil;*/
				/*op->DT1val = chip->DT1freq[ op->DT1 + kc ];*/  /*DT1 value*/
		/*calc freq end*/
				op+=8;
				op->KC = v;
				kc_oscil = kc_channel + op->DT2;  /*DT2 offset*/
				op->freq = ( chip->freq[ kc_oscil ] + chip->DT1freq[ op->DT1 + kc ] ) * op->mul;

				op+=8;
				op->KC = v;
				kc_oscil = kc_channel + op->DT2;  /*DT2 offset*/
				op->freq = ( chip->freq[ kc_oscil ] + chip->DT1freq[ op->DT1 + kc ] ) * op->mul;

				op+=8;
				op->KC = v;
				kc_oscil = kc_channel + op->DT2;  /*DT2 offset*/
				op->freq = ( chip->freq[ kc_oscil ] + chip->DT1freq[ op->DT1 + kc ] ) * op->mul;

				refresh_EG( chip, r&7 );
			}
			break;

		case 0x10: /*Key Fraction*/
			v >>= 2;
			if ( v != op->KF )
			{
				unsigned int kc,kc_oscil,kc_channel;

				op->KF = v;

				kc = op->KC;
				kc_channel = KC_TO_INDEX[kc] + op->KF;
				kc >>=2;

		/*calc freq begin operator 0*/
				kc_oscil = kc_channel + op->DT2;  /*DT2 offset*/
				op->freq = ( chip->freq[ kc_oscil ] + chip->DT1freq[ op->DT1 + kc ] ) * op->mul;
		/*calc freq end*/
				op+=8;
				op->KF = v;
				kc_oscil = kc_channel + op->DT2;  /*DT2 offset*/
				op->freq = ( chip->freq[ kc_oscil ] + chip->DT1freq[ op->DT1 + kc ] ) * op->mul;

				op+=8;
				op->KF = v;
				kc_oscil = kc_channel + op->DT2;  /*DT2 offset*/
				op->freq = ( chip->freq[ kc_oscil ] + chip->DT1freq[ op->DT1 + kc ] ) * op->mul;

				op+=8;
				op->KF = v;
				kc_oscil = kc_channel + op->DT2;  /*DT2 offset*/
				op->freq = ( chip->freq[ kc_oscil ] + chip->DT1freq[ op->DT1 + kc ] ) * op->mul;
			}
			break;

		case 0x18: /*PMS,AMS*/
			op->PMS = v>>4;
			op->AMS = v & 3;
			/*if (errorlog && op->PMS) fprintf(errorlog,"YM2151 PMS CH %02x =%02x\n",r&7,op->PMS);*/
			break;
		}
		break;

	case 0x40: /*DT1, MUL*/
		{
			unsigned int oldDT1 = op->DT1;
			unsigned int oldmul = op->mul;
			op->DT1 = (v<<1) & 0xe0;
			op->mul = (v&0x0f) ? (v&0x0f)<<1: 1;
			if ( (oldDT1!=op->DT1) || (oldmul!=op->mul) )
			{
				unsigned int kc,kc_oscil;
				kc = op->KC;
				kc_oscil = KC_TO_INDEX[kc] + op->KF + op->DT2;  /*DT2 offset*/

				kc >>=2;
				op->freq = ( chip->freq[ kc_oscil ] + chip->DT1freq[ op->DT1 + kc ] ) * op->mul;
			}
		}
		break;

	case 0x60: /*TL*/
		op->TL = (v&0x7f)<<(ENV_BITS-7); /*7bit TL*/
		break;

	case 0x80: /*KS, AR*/
		{
			int oldKS = op->KS;
			int oldAR = op->AR;
			op->KS = 5-(v>>6);
			op->AR = (v&0x1f) << 1;
			/*refresh*/
			if ( (op->AR != oldAR) || (op->KS != oldKS) )
				op->delta_AR  = chip->A_Time[op->AR  + (op->KC>>op->KS) ];
			if ( op->KS != oldKS )
			{
				op->delta_D1R = chip->D_Time[op->D1R + (op->KC>>op->KS) ];
				op->delta_D2R = chip->D_Time[op->D2R + (op->KC>>op->KS) ];
				op->delta_RR  = chip->D_Time[op->RR  + (op->KC>>op->KS) ];
			}
		}
		break;

	case 0xa0: /*AMS-EN, D1R*/
		op->AMSmask = (v & 0x80) ? 0xffffffff : 0;
		op->D1R = (v&0x1f) << 1;
		op->delta_D1R = chip->D_Time[op->D1R + (op->KC>>op->KS) ];
		break;

	case 0xc0: /*DT2, D2R*/
		{
			unsigned int oldDT2 = op->DT2;
			op->DT2 = DT2Tab[ v>>6 ];
			if (op->DT2 != oldDT2)
			{
				unsigned int kc,kc_oscil;
				kc = op->KC;
				kc_oscil = KC_TO_INDEX[kc] + op->KF + op->DT2;  /*DT2 offset*/

				kc >>=2;
				op->freq = ( chip->freq[ kc_oscil ] + chip->DT1freq[ op->DT1 + kc ] ) * op->mul;
			}
		}
		op->D2R = (v&0x1f) << 1;
		op->delta_D2R = chip->D_Time[op->D2R + (op->KC>>op->KS) ];
		break;

	case 0xe0: /*D1L, RR*/
		op->D1L = D1LTab[ (v>>4) & 0x0f ];
		op->RR = ((v&0x0f)<<2) | 2;
		op->delta_RR  = chip->D_Time[op->RR  + (op->KC>>op->KS) ];
		break;
	}

}

int YM2151ReadStatus( int n )
{
	return YMPSG[n].status;
}

/*
** Initialize YM2151 emulator(s).
**
** 'num' is the number of virtual YM2151's to allocate
** 'clock' is the chip clock in Hz
** 'rate' is sampling rate
*/
int YM2151Init(int num, int clock, int rate)
{
	int i;

	if (YMPSG) return (-1);	/* duplicate init. */

	YMNumChips = num;

	YMPSG = (YM2151 *)malloc(sizeof(YM2151) * YMNumChips);
	if (YMPSG == NULL) return (1);

	TL_TAB = (signed int *)malloc(sizeof(signed int) * TL_TAB_LEN );
	if (TL_TAB == NULL)
	{
		free(YMPSG);
		YMPSG=NULL;
		return (1);
	}

	init_tables();
	for ( i=0 ; i<YMNumChips; i++ )
	{
		YMPSG[i].clock = clock;
		YMPSG[i].sampfreq = rate ? rate : 10000;	/* avoid division by 0 in init_chip_tables() */
		YMPSG[i].irqhandler = NULL;					/*interrupt handler */
		YMPSG[i].porthandler = NULL;				/*port write handler*/
		init_chip_tables(&YMPSG[i]);
		YM2151ResetChip(i);
	}

	return(0);
}

void YM2151Shutdown()
{
	if (!YMPSG) return;

	free(YMPSG);
	YMPSG = NULL;
	if (TL_TAB)
	{
		free(TL_TAB);
		TL_TAB=NULL;
	}

#ifdef FM_EMU
	fclose(errorlog);
#endif

#ifdef SAVE_SAMPLE
	fclose(sample[8]);
#endif
#ifdef SAVE_SEPARATE_CHANNELS
	fclose(sample[0]);
	fclose(sample[1]);
	fclose(sample[2]);
	fclose(sample[3]);
	fclose(sample[4]);
	fclose(sample[5]);
	fclose(sample[6]);
	fclose(sample[7]);
#endif
}


/*
** reset all chip registers.
*/
void YM2151ResetChip(int num)
{
	int i;
	YM2151 *chip = &YMPSG[num];

	/* initialize hardware registers */

	for( i=0; i<32; i++)
	{
		memset(&chip->Oscils[i],'\0',sizeof(OscilRec));
		chip->Oscils[i].volume = VOLUME_OFF;
	}

	chip->LFOphase = 0;
	chip->LFOfrq   = 0;
	chip->LFOwave  = 0;
	chip->PMD = 0;
	chip->AMD = 0;
	chip->LFA = 0;
	chip->LFP = 0;

	chip->IRQenable = 0;
#ifdef USE_MAME_TIMERS
	/* ASG 980324 -- reset the timers before writing to the registers */
	chip->TimATimer = 0;
	chip->TimBTimer = 0;
#else
	chip->TimA      = 0;
	chip->TimB      = 0;
	chip->TimAVal   = 0;
	chip->TimBVal   = 0;
#endif
	chip->TimAIndex = 0;
	chip->TimBIndex = 0;

	chip->noise     = 0;

	chip->status    = 0;

	YM2151WriteReg(num, 0x1b, 0); /*only because of CT1, CT2 output pins*/
	for ( i=0x20; i<0x100; i++)   /*just to set the PM operators */
	{
		YM2151WriteReg(num, i, 0);
	}

}


void lfo_calc(void)
{
signed int phase,pom;

	phase = ( (PSG->LFOphase+=PSG->LFOfrq) >> LFO_SH ) & LFO_MASK;
	switch(PSG->LFOwave){
	case 0:	/*saw ?*/
		if (phase>=(LFO_LEN/2)){
			PSG->LFP = 0;
		}else{
			PSG->LFP = 0;
		}
		phase = PSG->AMD - phase;
		if (phase<0)	PSG->LFA = 0;
		else		PSG->LFA = phase;
		break;
	case 1:	/*square OK*/
		if (phase>=(LFO_LEN/2)){
			PSG->LFA = 0;
			PSG->LFP = -PSG->PMD;
		}else{
			PSG->LFA = PSG->AMD;
			PSG->LFP = PSG->PMD;
		}
		break;
	case 2:	/*triangle ?*/
		pom = phase;
		if (phase>=(LFO_LEN/2)){
			phase = LFO_LEN - ((phase-(LFO_LEN/2))<<1);
			phase = PSG->AMD - phase;
			if (phase<0)	PSG->LFA = 0;
			else		PSG->LFA = phase;
		}else{
			phase = PSG->AMD - (phase<<1);
			if (phase<0)	PSG->LFA = 0;
			else		PSG->LFA = phase;
		}
		if (pom<(LFO_LEN/4)){
			phase = PSG->PMD - pom;
			if (phase>0) phase = PSG->PMD - phase; else phase = PSG->PMD;
		}else{
			phase = PSG->PMD;
		}
		PSG->LFP = 0;
		break;
	case 3:	/*noise  (!!! real implementation is unknown !!!) */
		pom = ( ( (phase & 1)^((phase&4)>>2) ) <<7) | (phase>>1);
		phase = PSG->AMD - pom;
		if (phase<0)	PSG->LFA = 0;
		else		PSG->LFA = phase;
		phase = PSG->PMD - pom;
		if (abs(phase)>abs(PSG->PMD))	PSG->LFP = 0;
		else				PSG->LFP = phase;
		break;
	}
}


void pm_calc(OscilRec *op, int chan)
{
int inde,pom;

	inde = PSG->LFP; /* -127..+127 (8bits signed)*/
	switch( PSG->Oscils[chan].PMS ){
	case 0:	op->LFOpm=0; return; break;
	case 1: inde<<=5; break;
	case 2: inde<<=4; break;
	case 3: inde<<=3; break;
	case 4: inde<<=2; break;
	case 5: inde<<=1; break;
	case 6: inde>>=1; break;
	case 7: inde = (inde/2)+inde+(inde*2); break;
	}
	pom = PSG->freq[ op->KCindex + inde + op->DT1val] * op->mul;
	/*op->LFOpm = pom - op->freq ;*/
}


INLINE unsigned int volume_calc(OscilRec *op)
{
	op->phase+=op->freq;

	switch(op->state){
	case EG_ATT:	/*attack phase*/
		if ( (op->a_volume -= op->delta_AR) < MIN_VOL_INDEX )
		{
			op->volume = MIN_VOL_INDEX;
			op->state = EG_DEC;
		}
		else
			op->volume = attack_curve[op->a_volume>>ENV_SH];
		break;
	case EG_DEC:	/*decay phase*/
		if ( (op->volume += op->delta_D1R) > op->D1L )
		{
		#if 1
			op->volume = op->D1L; /*is this correct adjustment ?*/
		#endif
			op->state = EG_SUS;
		}
		break;
	case EG_SUS:	/*sustain phase*/
		if ( (op->volume += op->delta_D2R) > MAX_VOL_INDEX )
		{
			op->state = EG_OFF;
			op->volume = VOLUME_OFF;
		}
		break;
	case EG_REL:	/*release phase*/
		if ( (op->volume += op->delta_RR) > MAX_VOL_INDEX )
		{
			op->state = EG_OFF;
			op->volume = VOLUME_OFF;
		}
		break;
	}

	return op->TL + (op->volume>>ENV_SH);
}


/* signed int PM; pm_calc(OP0,chan); pm_calc(OP1,chan); pm_calc(OP2,chan); pm_calc(OP3,chan);*/


#define op_calc(OP,env,pm)  sin_tab[ ((OP->phase + pm)>>FREQ_SH) & SIN_MASK] [  env  /*+ (AM & OP->AMSmask)*/ ]

static void chan_calc(unsigned int chan)
{
unsigned int env;
OscilRec *OP;
/*unsigned int AM;*/

	chanout = c1 = m2 = c2 = 0;
	OP = &PSG->Oscils[chan]; /*M1*/

#if 0
	if (OP->AMS)
		AM = PSG->LFA << (OP->AMS - 1 + (ENV_BITS - 10)); /*LFA (8bits)*/
	else
		AM = 0;
#endif

	env = volume_calc(OP);
	if (env < ENV_LEN)
	{
		signed int out;

		if (OP->FeedBack){
			out = OP->OscilFB;
			OP->OscilFB = op_calc(OP, env, (OP->OscilFB>>OP->FeedBack) );
#if 1
			out = (OP->OscilFB + out)/2;
#else
			out = OP->OscilFB;
#endif
		}else{
			out = op_calc(OP, env, 0);
		}
		if( OP->connect == 0 ){
			/* algorithm 5 */
			c1 = m2 = c2 = out;
		}else{
			/* other algorithms */
			*OP->connect += out;
		}
	}

	OP+=16; /*C1*/
	env = volume_calc(OP);
	if( env < ENV_LEN )
		*OP->connect += op_calc(OP, env, c1);

	OP-=8;  /*M2*/
	env = volume_calc(OP);
	if( env < ENV_LEN )
		*OP->connect += op_calc(OP, env, m2);

	OP+=16; /*C2*/
	env = volume_calc(OP);
	if( env < ENV_LEN )
		*OP->connect += op_calc(OP, env, c2);

	chanout >>= FREQ_SH;

}


#ifdef SAVE_SEPARATE_CHANNELS
  #define SAVE_SINGLE_CHANNEL(j) \
  {	int pom= -(chanout & mask[j*2]); \
	if (pom > 32767) pom = 32767; else if (pom < -32768) pom = -32768; \
	fputc((unsigned short)pom&0xff,sample[j]); \
	fputc(((unsigned short)pom>>8)&0xff,sample[j]);  }
#else
  #define SAVE_SINGLE_CHANNEL(j)
#endif

#ifdef SAVE_SAMPLE
  #define SAVE_ALL_CHANNELS \
  {	int pom = -outl; \
	if (pom > 32767) pom = 32767; else if (pom < -32768) pom = -32768; \
	fputc((unsigned short)pom&0xff,sample[8]); \
	fputc(((unsigned short)pom>>8)&0xff,sample[8]); \
	pom = -outr; \
	if (pom > 32767) pom = 32767; else if (pom < -32768) pom = -32768; \
	fputc((unsigned short)pom&0xff,sample[8]); \
	fputc(((unsigned short)pom>>8)&0xff,sample[8]); \
 }
#else
  #define SAVE_ALL_CHANNELS
#endif

/*
** Generate samples for one of the YM2151's
**
** 'num' is the number of virtual YM2151
** '**buffers' is table of pointers to the buffers: left and right
** 'length' is the number of samples should be generated
*/
void YM2151UpdateOne(int num, void **buffers, int length)
{
	int i;
	signed int outl,outr;
	SAMP *bufL, *bufR;

	bufL = buffers[0];
	bufR = buffers[1];

	PSG = &YMPSG[num];

	for (i=0; i<16; i++)
		mask[i] = PSG->PAN[i];

	#ifdef USE_MAME_TIMERS
	/* ASG 980324 - handled by real timers now */
	#else
	/* calculate timers */
	if (PSG->TimA){
		PSG->TimAVal -= ( length << TIMER_SH );
		if (PSG->TimAVal<=0){
			PSG->TimAVal += PSG->TimerA[ PSG->TimAIndex ];
			if ( PSG->IRQenable & 0x04 ){
				int oldstate = PSG->status & 3;
				PSG->status |= 1;
				if( (!oldstate) && (PSG->irqhandler) ) (*PSG->irqhandler)(1);
			}
		}
	}
	if (PSG->TimB){
		PSG->TimBVal -= ( length << TIMER_SH );
		if (PSG->TimBVal<=0){
			PSG->TimBVal += PSG->TimerB[ PSG->TimBIndex ];
			if ( PSG->IRQenable & 0x08 ){
				int oldstate = PSG->status & 3;
				PSG->status |= 2;
				if( (!oldstate) && (PSG->irqhandler) ) (*PSG->irqhandler)(1);
			}
		}
	}
	#endif

    for( i=0; i<length; i++ )
    {
		/*lfo_calc();*/

		chan_calc(0);
		outl = chanout & mask[0];
		outr = chanout & mask[1];
		SAVE_SINGLE_CHANNEL(0)

		chan_calc(1);
		outl += (chanout & mask[2]);
		outr += (chanout & mask[3]);
		SAVE_SINGLE_CHANNEL(1)

		chan_calc(2);
		outl += (chanout & mask[4]);
		outr += (chanout & mask[5]);
		SAVE_SINGLE_CHANNEL(2)

		chan_calc(3);
		outl += (chanout & mask[6]);
		outr += (chanout & mask[7]);
		SAVE_SINGLE_CHANNEL(3)

		chan_calc(4);
		outl += (chanout & mask[8]);
		outr += (chanout & mask[9]);
		SAVE_SINGLE_CHANNEL(4)

		chan_calc(5);
		outl += (chanout & mask[10]);
		outr += (chanout & mask[11]);
		SAVE_SINGLE_CHANNEL(5)

		chan_calc(6);
		outl += (chanout & mask[12]);
		outr += (chanout & mask[13]);
		SAVE_SINGLE_CHANNEL(6)

		chan_calc(7);
		outl += (chanout & mask[14]);
		outr += (chanout & mask[15]);
		SAVE_SINGLE_CHANNEL(7)

		SAVE_ALL_CHANNELS

		outl >>= FINAL_SH;
		outr >>= FINAL_SH;
		if (outl > MAXOUT) outl = MAXOUT;
			else if (outl < MINOUT) outl = MINOUT;
		if (outr > MAXOUT) outr = MAXOUT;
			else if (outr < MINOUT) outr = MINOUT;
		((SAMP*)bufL)[i] = (SAMP)outl;
		((SAMP*)bufR)[i] = (SAMP)outr;
    }
}

void YM2151SetIrqHandler(int n, void(*handler)(int irq))
{
	YMPSG[n].irqhandler = handler;
}

void YM2151SetPortWriteHandler(int n, void(*handler)(int offset, int data))
{
	YMPSG[n].porthandler = handler;
}
