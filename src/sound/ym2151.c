#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "driver.h"
#include "ym2151.h"

/*
**  some globals ...
*/

//#define FM_EMU

#define USE_MAME_TIMERS
/*undef this if you don't want MAME timer system to be used*/

#ifdef FM_EMU
  FILE *errorlog = NULL;
  #define INLINE inline
  #ifdef USE_MAME_TIMERS
    #undef USE_MAME_TIMERS
  #endif
#endif


typedef struct{      /*oscillator data */
	unsigned int phase;	/*accumulated oscillator phase (in indexes)    (fixed point)*/
	unsigned int freq;	/*oscillator frequency (in indexes per sample) (fixed point)*/
	signed int LFOpm;	/*phase modulation from LFO*/

	unsigned int volume;	/*oscillator volume (fixed point)   */
	signed int attack_volume;/*used for attack phase calculations*/

	unsigned int TL;	/*Total attenuation Level*/
	unsigned int delta_AR;	/*volume delta for attack  phase (fixed point)*/
	unsigned int delta_D1R;	/*volume delta for decay   phase (fixed point)*/
	unsigned int D1L;	/*when envelope reaches this level Envelope Generator goes into D2R*/
	unsigned int delta_D2R;	/*volume delta for sustain phase (fixed point)*/
	unsigned int delta_RR;	/*volume delta for release phase (fixed point)*/
	unsigned int state;	/*Envelope state: 4-attack(AR) 3-decay(D1R) 2-sustain(D2R) 1-release(RR) 0-off*/

	unsigned int AMSmask;   /*LFO AMS enable mask*/

	signed int * connect;	/*oscillator output direction*/

/*Begin of channel specific data*/
/*The trick : each operator number 0 contains channel specific data*/
	signed int OscilFB;	/*oscillator self feedback value used only by operators 0*/
	unsigned int FeedBack;	/*feedback shift value for operators 0 in each channel*/
	unsigned int KC; 	/*KC for each channel*/
	unsigned int KF;	/*KF for each channel*/
	unsigned int PMS;	/*PMS for each channel*/
	unsigned int AMS;	/*AMS for each channel*/
	unsigned int PAN;	/*bit 7-right enable, bit 6-left enable: 0-off, 1-on*/
/*End of channel specific data*/

	unsigned int key;	/*0=last key was KEYOFF, 1=last key was KEON*/

	unsigned int KS;	/*Key Scale*/
	unsigned int AR;	/*Attack rate*/
	unsigned int D1R;	/*Decay rate*/
	unsigned int D2R;	/*Sustain rate*/
	unsigned int RR;	/*Release rate*/
	unsigned int mul;	/*phase multiply*/
	unsigned int DT1;	/*DT1 index*32 */
	unsigned int DT2;	/*DT2 index*/
	unsigned int KCindex;	/*Used for LFO pm calculations ...........*/
	unsigned int DT1val;	/*used for LFO pm calculations ...........*/

} OscilRec;

/* here's the virtual YM2151 */
typedef struct ym2151_v{
	int status;		/*chip status (BUSY, IRQ Flags)*/

	OscilRec Oscils[32];	/*there are 32 operators in YM2151*/

	unsigned int LFOphase;	/*accumulated LFO phase (in indexes)    (fixed point)*/
	unsigned int LFOfrq;	/*LFO frequency (in indexes per sample) (fixed point)*/
	unsigned int PMD;	/*LFO Phase Modulation Depth*/
	unsigned int AMD;	/*LFO Amplitude Modulation Depth*/
	unsigned int LFOwave;	/*LFO waveform (0-saw, 1-square, 2-triangle, 3-random noise)*/
	unsigned int LFA;	/*current AM from LFO*/
	signed int LFP;		/*current PM from LFO*/

	unsigned int CT;	/*output control pins (bit7 CT2, bit6 CT1)*/
	unsigned int noise;	/*noise register (bit 7 - noise enable, bits 4-0 - noise freq*/

	unsigned int IRQenable;		/*IRQ enable for timer B (bit 3) and timer A (bit 2)*/
#ifdef USE_MAME_TIMERS
	void *TimATimer,*TimBTimer;	/* ASG 980324 -- added for tracking timers */
	double TimerATime[1024];	/*Timer A times for MAME*/
	double TimerBTime[256];		/*Timer B times for MAME*/
#else
	int TimA,TimB;			/*timer A,B enable (0-disabled)*/
	signed int TimAVal,TimBVal;	/*current value of timer*/
	unsigned int TimerA[1024];	/*Timer A deltas*/
	unsigned int TimerB[256];	/*Timer B deltas*/
#endif
	unsigned int TimAIndex,TimBIndex;/*timers' indexes*/

	void (*handler)(int irq);		/*IRQ function handler*/
	void (*porthandler)(int offset,int data);	/*port write handler*/

	unsigned int freq[11*12*64];	/*11 octaves, 12 semitones, 64 'cents' */
	/*
	*   Frequency-deltas to get the closest frequency possible.
	*   There're 11 octaves because of DT2 (max 950 cents over base frequency)
	*   and LFO phase modulation (max 700 cents _over_ AND _below_ base frequency)
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

	signed int DT1freq[8*32];
	/*
	*   Frequency deltas for DT1. These deltas alter operator frequency
	*   after it has been taken from frequency-deltas table.
	*/

	unsigned int LFOfreq[256];  /*frequency deltas for LFO*/

	unsigned int A_Time[64+32]; /*attack deltas (64 keycodes + 32 RKS's = 96)*/
	unsigned int D_Time[64+32]; /*decay  deltas (64 keycodes + 32 RKS's = 96)*/

	unsigned int clock;	/* this will be passed from 2151intf.c */
	unsigned int sampfreq;	/* this will be passed from 2151intf.c */

	unsigned int need_refresh; /* 1 if emulator needs refreshing before calculating new samples*/
} YM2151;



/*
**  Shifts below are subject to change if sampling frequency changes...
*/
#define FREQ_SH 16  /* 16.16 fixed point for frequency calculations     */
#define LFO_SH 24   /*  8.24 fixed point for LFO frequency calculations */
#define ENV_SH 16   /* 16.16 fixed point for envelope calculations      */
#define TIMER_SH 16 /* 16.16 fixed point for timers calculations        */

/*if you change these values (ENV_SOMETHING) you have to take care of attack_curve*/
#define ENV_BITS 10
#define ENV_RES ((int)1<<ENV_BITS)
#define ENV_STEP (96.0/ENV_RES)

#define MAX_VOLUME_INDEX ((ENV_RES-1)<<ENV_SH)
#define MIN_VOLUME_INDEX ((1<<ENV_SH)-1)
#define VOLUME_OFF (ENV_RES<<ENV_SH)

#define SIN_BITS 12
#define SIN_LEN ((int)1<<SIN_BITS)
#define SIN_MASK (SIN_LEN-1)

#define LFO_BITS 8
#define LFO_LEN ((int)1<<LFO_BITS)
#define LFO_MASK (LFO_LEN-1)

#define FINAL_SH16 0  /*this shift is applied to final output of all channels to get 16-bit sample*/
#define FINAL_SH8 8   /*this shift is applied to final output of all channels to get 8-bit sample*/


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

static signed int * sin_tab[SIN_LEN];
/* sin waveform table in decibel scale */

static unsigned int D1LTab[16];
/*decibel table to convert from D1L values to volume index*/

static unsigned int attack_curve[ENV_RES];
/*attack curve for envelope attack phase*/

static unsigned int KC_TO_INDEX[16*8];  /*8 octaves * 16 note codes*/
/*
 *   Translate from key code KC (OCT2 OCT1 OCT0 N3 N2 N1 N0) to
 *   index in frequency-deltas table.
*/


#define EG_ATT 4
#define EG_DEC 3
#define EG_SUS 2
#define EG_REL 1
#define EG_OFF 0


static unsigned char sample_16bit;	/* 1 -> 16 bit sample, 0 -> 8 bit */
static unsigned int YMNumChips;		/* total # of YM2151's emulated */


/*these variables stay here because of speedup purposes only */
static SAMP *bufL, *bufR;
static unsigned int mask[16];
static signed int c1,m2,c2; /*Phase Modulation input for operators 2,3,4*/
static signed int chanout;


static YM2151 * PSG;
static YM2151 * YMPSG = NULL;	/* array of YM2151's */

/*save output as raw 16-bit sample*/
/*#define SAVE_SAMPLE*/
/*#define SAVE_SEPARATE_CHANNELS*/
#ifdef SAVE_SAMPLE
    FILE *samplesum;
#endif
#ifdef SAVE_SEPARATE_CHANNELS
    FILE *sample[8];
#endif



static unsigned char DT1Tab[4*32]={ /* 4*32 DT1 values */
/*
 *   DT1 defines offset in Hertz from base note * mul
 *
 *   This table is converted while initialization...
*/
/* DT1=0 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* DT1=1 */
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2,
  2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8, 9,10,
/* DT1=2 */
  1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5,
  5, 6, 6, 7, 8, 8, 9,10,11,12,13,14,16,17,19,20,
/* DT1=3 */
  2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7,
  8, 8, 9,10,11,12,13,14,16,17,19,20,22,22,22,22
};

static unsigned int DT2Tab[4]={ /* 4 DT2 values */
/*
 *   DT2 defines offset in cents from base note
 *
 *   This table defines offset in frequency-deltas table.
 *   User's Manual page 22
 *   Values below were calculated using formula:  value = orig.val / 1.5625
 *
 *	DT2=0 DT2=1 DT2=2 DT2=3
 *	0     600   781   950
*/
	0,    384,  500,  608
};

static unsigned int AttackTab[73]={
/*
 * This data is valid ONLY for YM2151. Other Yamaha's FM chips seems to use
 * different shape of envelope attack phase. I'm quite sure that curve of OPL2/3
 * is *similiar*, but much smoother. These chips seem to use something
 * like "a 42dB curve".
 *
 *    This data was reverse-engineered by me (Jarek Burczynski). Keep in mind
 * that I spent a _lot_ of time researching this information, so be so kind as
 * to send me an e-mail before you use it, please.
*/
	0  ,1  ,2  ,3  ,4  ,5  ,6  ,7  ,8  ,9  ,10 ,11 ,12 ,13 ,14 ,16,  /*16*/
	18 ,20 ,22 ,24 ,26 ,28 ,31 ,34 ,37 ,40 ,43 ,46 ,50 ,54 ,58 ,63,  /*16*/
	68 ,73 ,78 ,84 ,90 ,97 ,104,111,120,129,138,148,158,168,180,193, /*16*/
	207,222,238,254,271,289,308,329,352,377,404,433,464,498,535,575, /*16*/
	618,664,713,766,823,884,950,1021,1023  /*9*/
};

static unsigned char MULTab[16]={1,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30};

void init_tables(void)
{
	signed int x,i;
	double m;

	for(i=0; i<SIN_LEN; i++){
		m = sin(2*PI*i/SIN_LEN);              /*count sin value*/
		if ( (m<0.0001) && (m>-0.0001) ){     /*is m near zero ?*/
			m = ENV_RES-1;
		}else{
			if (m>0.0){
				m = 20*log10(1.0/m);  /*and how many decibels is it?*/
				m = m / ENV_STEP;
			}else{
				m = 20*log10(-1.0/m); /*and how many decibels is it?*/
				m = (m / ENV_STEP) + TL_TAB_LEN/2;
			}
		}
		sin_tab[ i ] = &TL_TAB[ (unsigned int)m ];
		/*if (errorlog) fprintf(errorlog,"sin %i = %i\n",i,sin_tab[i] );*/
	}

	for( x=0; x<TL_TAB_LEN/2; x++ ){
		if (x<ENV_RES)
			m = ((1<<(SIN_BITS+2))-1) / pow(10, x * ENV_STEP /20);
		else
			m = 0;
		TL_TAB[         x        ] = m;
		TL_TAB[ x + TL_TAB_LEN/2 ] = -m;
		/*if (errorlog) fprintf(errorlog,"tl %04i =%08x\n",x,TL_TAB[x]);*/
	}

	/* create (almost perfect) attack curve */
	for (i=0; i<ENV_RES; i++)                     /*clear the curve*/
		attack_curve[i] = MAX_VOLUME_INDEX;

	for (i=0; i<73; i++){                         /*create curve*/
		for (x=i*14; x<i*14+14; x++){
			attack_curve[x] = AttackTab[i] << ENV_SH;
			/*if (errorlog) fprintf(errorlog,"attack [%04x] = %08x Volt=%08x\n", x, attack_curve[x]>>ENV_SH, TL_TAB[ attack_curve[x]>>ENV_SH] );*/
		}
	}

	for (i=0; i<16; i++){
		m = (i<15?i:i+16) * (3.0/ENV_STEP);   /*every 3 dB except for all bits = '1' = 45dB+48dB*/
		x = m * (double)(1<<ENV_SH);
		D1LTab[i] = x;
		/*if (errorlog) fprintf(errorlog,"D1LTab[%04x]=%08x\n",i,x );*/
	}

	/* calculate KC to index table */
	x=0;
	for (i=0; i<16*8; i++){
		KC_TO_INDEX[i]=((i>>4)*12*64) + x*64 + 12*64;
		if ((i&3)!=3) x++;	/* change note code */
		if ((i&15)==15) x=0;	/* new octave */
	}

#ifdef FM_EMU
errorlog=fopen("errorlog.txt","wb");
#endif

#ifdef SAVE_SAMPLE
samplesum=fopen("sampsum.raw","wb");
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
	int i,j,oct;
	double mult,pom,pom2,clk;

	double scaler;	/* formula below is true for chip clock=3579545 */
	/* so we need to scale its output accordingly to the chip clock */

	scaler= (double)chip->clock / 3579545.0;

	/*this loop calculates Hertz values for notes from c#0 to c-8*/
	/*including 64 'cents' (100/64 that is 1.5625 of real cent) for each semitone*/

	oct = 12*64;
	mult = (int)1<<FREQ_SH;
	for (i=0; i<11*12*64; i++){
		pom = scaler * 6.875 * pow (2, ((i+4*64)*1.5625/1200.0) ); /*13.75Hz is note A 12semitones below A-0, so C#0 is 4 semitones above then*/
		/*calculate phase increment for above precounted Hertz value*/
		j = ( (pom*SIN_LEN) / (double)chip->sampfreq )*mult; /*fixed point*/

		j /= 2.0; /*divided by 2, because our first octave is one octave below standard */
		chip->freq[i] = j;
#if 0
		j = j/16.0;
		chip->freq[i+oct*5] = j;    /*oct 5 - center*/
		chip->freq[i+oct*6] = j<<1; /*oct 6*/
		chip->freq[i+oct*7] = j<<2; /*oct 7*/
		chip->freq[i+oct*8] = j<<3; /*oct 8*/
		chip->freq[i+oct*9] = j<<4; /*oct 9*/
		chip->freq[i+oct*10] =j<<5; /*oct 10*/
		chip->freq[i+oct*4] = j>>1; /*oct 4*/
		chip->freq[i+oct*3] = j>>2; /*oct 3*/
		chip->freq[i+oct*2] = j>>3; /*oct 2*/
		chip->freq[i+oct*1] = j>>4; /*oct 1*/
		chip->freq[i+oct*0] = j>>5; /*oct 0*/
#endif

		/*if (errorlog) fprintf(errorlog,"deltas[%04i] = %08x\n",i,chip->freq[i]);*/
	}
#if 0
	if (errorlog){
		for (i=0; i<11*12*64; i++){
			pom = (double)chip->freq[i] / mult;
			pom *= (double)chip->sampfreq;
			pom /= (double)SIN_LEN;
			fprintf(errorlog,"KC[%i] = %f Hz\n",i, pom);
		}
	}
#endif

	mult = (int)1<<FREQ_SH;
	for (j=0; j<4; j++){
		for (i=0; i<32; i++){
			pom = ( (double)DT1Tab[j*32+i] * ((double)chip->clock/4.0) ) / (double)(1<<24);
			/*calculate phase increment for above precounted Hertz value*/
			chip->DT1freq[j*32+i] = ( (pom*SIN_LEN) / (double)chip->sampfreq )*mult; /*fixed point*/
			chip->DT1freq[(j+4)*32+i] = -chip->DT1freq[j*32+i];

			/*if (errorlog) fprintf(errorlog,"DT1[%i][%i] = %f Hz\n",i,j,
				(((double)chip->DT1freq[j*32+i] / mult) * (double)chip->sampfreq ) / (double)SIN_LEN ); */
		}
	}

	mult = (int)1<<LFO_SH;
	clk  = (double)chip->clock;
	for (i=0; i<256; i++){
		j = i & 0x0f;
		pom = fabs(  (clk/65536/(1<<(i/16)) ) - (clk/65536/32/(1<<(i/16)) * (j+1)) );
		/*calculate phase increment for above precounted Hertz value*/
		chip->LFOfreq[0xff-i] = ( (pom*LFO_LEN) / (double)chip->sampfreq ) * mult; /*fixed point*/
		/*if (errorlog) fprintf(errorlog, "LFO[%02x] = %f Hz (calculated %f Hz)\n",0xff-i,
			(((double)chip->LFOfreq[0xff-i] / mult) * (double)chip->sampfreq ) / (double)LFO_LEN , pom);*/
	}

	for (i=0; i<4; i++)
		chip->A_Time[i] = chip->D_Time[i] = 0;		/*infinity*/

	for (i=4; i<64; i++){
		pom2 = ( (double)chip->clock / (double)chip->sampfreq );
		if (i<60) pom2 *= 1.0 + (i&3)*0.25;
		pom2 *= 1<<((i>>2)-1);
		pom2 *= (double)(1<<ENV_SH);
		if (i<52){
			pom  = pom2 / 27876.0;   		/*AR scale value 0-12*/
		}else{
			if ( (i>=52) && (i<56) ){
				pom  = pom2 / 30600.0;   	/*AR scale value 13*/
			}else{
				if ( (i>=56) && (i<60) ){
					pom  = pom2 / 32200.0;	/*AR scale value 14*/
				}else{
					pom  = pom2 / 30400.0;	/*AR scale value 15*/
				}
			}
		}
		if (i==63) pom=(ENV_RES<<ENV_SH);
		pom2 = pom2 / 385303.0;				/*DR scale value*/
		chip->A_Time[i] = pom;
		chip->D_Time[i] = pom2;
		/*if (errorlog)
			fprintf(errorlog,"Rate %2i %1i  Attack time=%.2f ms  Decay time=%.2f ms\n",i>>2, i&3,
			( ((double)(ENV_RES<<ENV_SH)) / pom  ) * (1000.0 / (double)chip->sampfreq),
			( ((double)(ENV_RES<<ENV_SH)) / pom2 ) * (1000.0 / (double)chip->sampfreq) );*/
	}

	for (i=0; i<32; i++){
		chip->A_Time[64+i]=chip->A_Time[63];
		chip->D_Time[64+i]=chip->D_Time[63];
	}

/* precalculate timers' deltas */
/* User's Manual pages 15,16  */
	mult = (int)1<<TIMER_SH;
	for (i=0; i<1024; i++){
		/* ASG 980324: changed to compute both TimerA and TimerATime */
		pom= ( 64.0  *  (1024.0-i) / chip->clock ) ;
		#ifdef USE_MAME_TIMERS
			chip->TimerATime[i] = pom;
		#else
			chip->TimerA[i] = pom * chip->sampfreq * mult;  /*number of samples that timer period takes (fixed point) */
		#endif
	}
	for (i=0; i<256; i++){
		/* ASG 980324: changed to compute both TimerB and TimerBTime */
		pom= ( 1024.0 * (256.0-i)  / chip->clock ) ;
		#ifdef USE_MAME_TIMERS
			chip->TimerBTime[i] = pom;
		#else
			chip->TimerB[i] = pom * chip->sampfreq * mult;  /*number of samples that timer period takes (fixed point) */
		#endif
	}

}





INLINE void envelope_KONKOFF(OscilRec * op, int v)
{
	/*OPL3 on SoundBlaster doesn't change envelope volume when KeyOn'ed */
	/*However credit sound in Paper Boy and music tune in Toobin, both  */
	/*proove that YM2151 always zeroes volume after KEYON */
if (v&8)
{
	if (!op->key){
		op->key = 1; /*KEYON'ed*/
		/*op->attack_volume = op->volume;*/      /*don't reset volume*/
		op->attack_volume = op->volume = VOLUME_OFF;   /*reset volume*/
		op->OscilFB = op->phase = 0;      /*clear feedback and phase */
		op->state = EG_ATT;               /*KEY ON = attack*/
	}
}
else
{
	if (op->key){
		op->key = 0; /*KEYOFF'ed*/
		op->state=EG_REL; /*release*/
	}
}

op+=8;

if (v&0x20)
{
	if (!op->key){
		op->key = 1; /*KEYON'ed*/
		/*op->attack_volume = op->volume;*/      /*don't reset volume*/
		op->attack_volume = op->volume = VOLUME_OFF;   /*reset volume*/
		op->OscilFB = op->phase = 0;      /*clear feedback and phase */
		op->state = EG_ATT;               /*KEY ON = attack*/
	}
}
else
{
	if (op->key){
		op->key = 0; /*KEYOFF'ed*/
		op->state=EG_REL; /*release*/
	}
}

op+=8;

if (v&0x10)
{
	if (!op->key){
		op->key = 1; /*KEYON'ed*/
		/*op->attack_volume = op->volume;*/      /*don't reset volume*/
		op->attack_volume = op->volume = VOLUME_OFF;   /*reset volume*/
		op->OscilFB = op->phase = 0;      /*clear feedback and phase */
		op->state = EG_ATT;               /*KEY ON = attack*/
	}
}
else
{
	if (op->key){
		op->key = 0; /*KEYOFF'ed*/
		op->state=EG_REL; /*release*/
	}
}

op+=8;

if (v&0x40)
{
	if (!op->key){
		op->key = 1; /*KEYON'ed*/
		/*op->attack_volume = op->volume;*/      /*don't reset volume*/
		op->attack_volume = op->volume = VOLUME_OFF;   /*reset volume*/
		op->OscilFB = op->phase = 0;      /*clear feedback and phase */
		op->state = EG_ATT;               /*KEY ON = attack*/
	}
}
else
{
	if (op->key){
		op->key = 0; /*KEYOFF'ed*/
		op->state=EG_REL; /*release*/
	}
}

}


#ifdef USE_MAME_TIMERS
static void timer_callback_a (int n)
{
    YM2151 *chip = &YMPSG[n];
	chip->TimATimer=0;
	if ( chip->IRQenable & 0x04 ){
		int oldstate = chip->status;
		chip->status |= 1;
		if( !(oldstate) && (chip->handler) ) (*chip->handler)(1);
	}
}
static void timer_callback_b (int n)
{
    YM2151 *chip = &YMPSG[n];
	chip->TimBTimer=0;
	if ( chip->IRQenable & 0x08 ){
		int oldstate = chip->status;
		chip->status |= 2;
		if( !(oldstate) && (chip->handler) ) (*chip->handler)(1);
	}
}
#endif



static void set_connect_feedback_pan( OscilRec *om1, int v )
{
	OscilRec *om2 = om1+8;
	OscilRec *oc1 = om1+16;
	OscilRec *oc2 = om1+24;

	om1->PAN = v;

	om1->FeedBack = (v>>3)&7 ? 8 - ((v>>3)&7) : 0;
	/*for values 0,1,2,3,4,5,6,7 this formula gives: 0,7,6,5,4,3,2,1*/

	/* set connect algorithm */
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
	oc2->connect = &chanout;
}




/* write a register on YM2151 chip number 'n' */
void YM2151WriteReg(int n, int r, int v)
{
    YM2151 *chip = &(YMPSG[n]);
    OscilRec * osc= &chip->Oscils[ r&0x1f ];

	if ( (r>=0x80) || (r>=0x28 && r<0x38) || (r>=0x40 && r<0x60) )
		chip->need_refresh = 1;

	switch(r & 0xe0){
	case 0x00:
		switch(r){
		case 0x01: /*LFO Reset(bit 1), Test Register (other bits)*/
			if (v&2) chip->LFOphase=0;
			if (errorlog && (v & 0xfd) )
				fprintf(errorlog,"YM2151 TEST MODE ON (%02x)\n",v);
			break;
		case 0x08:
			envelope_KONKOFF(&chip->Oscils[ (v&7) ], v );
			break;
		case 0x0f: /*Noise mode select, noise freq*/
			chip->noise = v;
			break;
		case 0x10: /*Timer A  hi*/
			chip->TimAIndex = (chip->TimAIndex & 0x03 ) | (v<<2);
			break;
		case 0x11: /*Timer A low*/
			chip->TimAIndex = (chip->TimAIndex & 0x3fc) | (v & 3);
			break;
		case 0x12: /*Timer B */
			chip->TimBIndex = v;
			break;
		case 0x14: /*CSM, irq flag reset, irq enable, timer start/stop*/
			if (errorlog && (v&0x80) ) fprintf(errorlog,"YM2151 CSM ON\n"); /*CSM*/
			chip->IRQenable = v;               /*bit 3-timer B, bit 2-timer A*/
			if (v&0x20)
			{
				int old_state = chip->status;
				chip->status &= 0xfd;  /*FRESET B*/
				if( (old_state==2) && (chip->handler) ) (*chip->handler)(0);
			}
			if (v&0x10)
			{
				int old_state = chip->status;
				chip->status &= 0xfe;  /*FRESET A*/
				if( (old_state==1) && (chip->handler) ) (*chip->handler)(0);
			}

			if (v&0x02){                       /*LOAD & START B*/
				#ifdef USE_MAME_TIMERS
				/* ASG 980324: added a real timer */
					if (chip->TimBTimer) timer_remove(chip->TimBTimer);
					chip->TimBTimer = timer_set (chip->TimerBTime[ chip->TimBIndex ], n, timer_callback_b);
				#else
					chip->TimB = 1;
					chip->TimBVal = chip->TimerB[ chip->TimBIndex ];
				#endif
			}else{                             /*STOP B*/
				#ifdef USE_MAME_TIMERS
				/* ASG 980324: added a real timer if we have a handler */
					if (chip->TimBTimer) timer_remove (chip->TimBTimer);
					chip->TimBTimer = 0;
				#else
					chip->TimB = 0;
				#endif
			}
			if (v&0x01){                       /*LOAD & START A*/
				#ifdef USE_MAME_TIMERS
				/* ASG 980324: added a real timer */
					if (chip->TimATimer) timer_remove(chip->TimATimer);
					chip->TimATimer = timer_set (chip->TimerATime[ chip->TimAIndex ], n, timer_callback_a);
				#else
					chip->TimA = 1;
					chip->TimAVal = chip->TimerA[ chip->TimAIndex ];
				#endif
			}else{                             /*STOP A*/
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
			if (errorlog && chip->PMD) fprintf(errorlog,"PMDAMD %02x\n",v);
			break;
		case 0x1b: /*CT2, CT1, LFO Waveform*/
			chip->CT = v;
			chip->LFOwave = v & 3;
			if (chip->porthandler) (*chip->porthandler)(0 , (chip->CT)>>6 );
			else
if (errorlog) fprintf(errorlog,"Write %x to YM2151 output port\n",(chip->CT)>>6);
			break;
		default:
			if (errorlog) fprintf(errorlog,"Write %02x to undocumented YM2151 register #%02x\n",v,r);
			break;
		}
		break;

	case 0x20:
		switch(r & 0x18){

		case 0x00: /*RL enable, Feedback, Connection */
			set_connect_feedback_pan(&chip->Oscils[r&7], v );
			break;

		case 0x08: /*Key Code*/
		        chip->Oscils[ r&7 ].KC = v & 0x7f;
			break;

		case 0x10: /*Key Fraction*/
			chip->Oscils[ r&7 ].KF = v >> 2 ;
			break;

		case 0x18: /*PMS,AMS*/
			chip->Oscils[ r&7 ].PMS = (v>>4) & 7;
			chip->Oscils[ r&7 ].AMS = v & 3;
			if (errorlog && chip->Oscils[r&7].PMS) fprintf(errorlog,"PMS CH %02x =%02x\n",r&7,(v>>4)&7);
			break;
		}
		break;

	case 0x40: /*DT1, MUL*/
		osc->DT1 = (v<<1) & 0xe0;
		osc->mul = MULTab [v & 0x0f];
		break;

	case 0x60: /*TL*/
		osc->TL = (v&0x7f)<<(ENV_BITS-7); /*7bit TL*/
		break;

	case 0x80: /*KS, AR*/
		osc->KS = 3-(v>>6);
		osc->AR = (v&0x1f) << 1;
		break;

	case 0xa0: /*AMS-EN, D1R*/
		osc->AMSmask = (v & 0x80) ? 0xffffffff : 0;
		osc->D1R = (v&0x1f) << 1;
		break;

	case 0xc0: /*DT2, D2R*/
		osc->DT2 = DT2Tab[(v>>6) & 3];
		osc->D2R = (v&0x1f) << 1;
		break;

	case 0xe0: /*D1L, RR*/
		osc->D1L = D1LTab[ (v>>4) & 0x0f ];
		osc->RR = ((v&0x0f)<<2) | 2;
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
** 'sample_bits' is 8/16 bit selector
*/
int YM2151Init(int num, int clock, int rate,int sample_bits )
{
    int i;

    if (YMPSG) return (-1);	/* duplicate init. */

    YMNumChips = num;
    if (sample_bits==16)
	sample_16bit=1;
    else
	sample_16bit=0;

    YMPSG = (YM2151 *)malloc(sizeof(YM2151) * YMNumChips);
    if (YMPSG == NULL) return (1);

    TL_TAB = (signed int *)malloc(sizeof(signed int) * TL_TAB_LEN );
    if (TL_TAB == NULL) { free(YMPSG); YMPSG=NULL; return (1); }

    init_tables();
    for ( i = 0 ; i < YMNumChips; i++ )
    {
	YMPSG[i].clock = clock;
	YMPSG[i].sampfreq = rate ? rate : 10000;	/* avoid division by 0 in init_chip_tables() */
	YMPSG[i].handler = NULL;     /*interrupt handler */
	YMPSG[i].porthandler = NULL; /*port write handler*/
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

    if (TL_TAB){
	free(TL_TAB);
	TL_TAB=NULL;
    }

#ifdef FM_EMU
fclose(errorlog);
#endif

#ifdef SAVE_SAMPLE
fclose(samplesum);
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

	for( i=0; i<32; i++){
		memset(&chip->Oscils[i],'\0',sizeof(OscilRec));
		chip->Oscils[i].volume = VOLUME_OFF;
	}

	for ( i=0; i<8; i++){
		set_connect_feedback_pan(&chip->Oscils[i],0);
	}

	chip->LFOphase = 0;
	chip->LFOfrq = 0;
	chip->PMD = 0;
	chip->AMD = 0;
	chip->LFOwave = 0;
	chip->LFA = 0;
	chip->LFP = 0;

	chip->IRQenable = 0;
#ifdef USE_MAME_TIMERS
	/* ASG 980324 -- reset the timers before writing to the registers */
	chip->TimATimer = 0;
	chip->TimBTimer = 0;
#else
	chip->TimA = 0;
	chip->TimB = 0;
	chip->TimAVal = 0;
	chip->TimBVal = 0;
#endif
	chip->TimAIndex = 0;
	chip->TimBIndex = 0;

	chip->CT = 0;
	chip->noise = 0;

	chip->status = 0;
	if (chip->handler) (*chip->handler)(0);
	chip->need_refresh = 1; /*emulator needs refresh after reset*/
}



void refresh_chip(void)
{
unsigned int kc_index_oscil, kc_index_channel, kc, chan;
unsigned char v;
OscilRec * osc;

    if (PSG->need_refresh==0)
	return;

    PSG->need_refresh = 0;

    for (chan=0; chan<8; chan++)
    {
	osc = &PSG->Oscils[chan];
	kc = osc->KC;
	kc_index_channel = KC_TO_INDEX[kc] + osc->KF;
	kc >>=2;

/*calc freq begin operator 0*/
        kc_index_oscil = kc_index_channel + osc->DT2;  /*DT2 offset*/
	osc->freq = PSG->freq[ kc_index_oscil ] * osc->mul;
	osc->freq += PSG->DT1freq[ osc->DT1 + kc ];    /*DT1 value*/
	//osc->KCindex = kc_index_oscil;
	//osc->DT1val = PSG->DT1freq[ osc->DT1 + kc ];  /*DT1 value*/
/*calc freq end*/ /*calc envelopes begin*/
	v = kc >> osc->KS;
	osc->delta_AR  = PSG->A_Time[ osc->AR + v];    /* 2*RR + RKS =max 95*/
	osc->delta_D1R = PSG->D_Time[osc->D1R + v];
	osc->delta_D2R = PSG->D_Time[osc->D2R + v];
	osc->delta_RR =  PSG->D_Time[ osc->RR + v];
/*calc envelopes end*/

	osc+=8;

/*calc freq begin operator 1*/
        kc_index_oscil = kc_index_channel + osc->DT2;  /*DT2 offset*/
	osc->freq = PSG->freq[ kc_index_oscil ] * osc->mul;
	osc->freq += PSG->DT1freq[ osc->DT1 + kc ];    /*DT1 value*/
	//osc->KCindex = kc_index_oscil;
	//osc->DT1val = PSG->DT1freq[ kc ][ osc->DT1];  /*DT1 value*/
/*calc freq end*/ /*calc envelopes begin*/
	v = kc >> osc->KS;
	osc->delta_AR  = PSG->A_Time[ osc->AR + v];    /* 2*RR + RKS =max 95*/
	osc->delta_D1R = PSG->D_Time[osc->D1R + v];
	osc->delta_D2R = PSG->D_Time[osc->D2R + v];
	osc->delta_RR =  PSG->D_Time[ osc->RR + v];
/*calc envelopes end*/

	osc+=8;

/*calc freq begin operator 2*/
        kc_index_oscil = kc_index_channel + osc->DT2;  /*DT2 offset*/
	osc->freq = PSG->freq[ kc_index_oscil ] * osc->mul;
	osc->freq += PSG->DT1freq[ osc->DT1 + kc ];    /*DT1 value*/
	//osc->KCindex = kc_index_oscil;
	//osc->DT1val = PSG->DT1freq[ kc ][ osc->DT1];  /*DT1 value*/
/*calc freq end*/ /*calc envelopes begin*/
	v = kc >> osc->KS;
	osc->delta_AR  = PSG->A_Time[ osc->AR + v];    /* 2*RR + RKS =max 95*/
	osc->delta_D1R = PSG->D_Time[osc->D1R + v];
	osc->delta_D2R = PSG->D_Time[osc->D2R + v];
	osc->delta_RR =  PSG->D_Time[ osc->RR + v];
/*calc envelopes end*/

	osc+=8;

/*calc freq begin operator 3*/
        kc_index_oscil = kc_index_channel + osc->DT2;  /*DT2 offset*/
	osc->freq = PSG->freq[ kc_index_oscil ] * osc->mul;
	osc->freq += PSG->DT1freq[ osc->DT1 + kc ];    /*DT1 value*/
	//osc->KCindex = kc_index_oscil;
	//osc->DT1val = PSG->DT1freq[ kc ][ osc->DT1];  /*DT1 value*/
/*calc freq end*/ /*calc envelopes begin*/
	v = kc >> osc->KS;
	osc->delta_AR  = PSG->A_Time[ osc->AR + v];    /* 2*RR + RKS =max 95*/
	osc->delta_D1R = PSG->D_Time[osc->D1R + v];
	osc->delta_D2R = PSG->D_Time[osc->D2R + v];
	osc->delta_RR =  PSG->D_Time[ osc->RR + v];
/*calc envelopes end*/

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
	pom = PSG->freq[ op->KCindex + inde ] * op->mul + op->DT1val;
	//op->LFOpm = pom - op->freq ;
}


static unsigned int volume_calc(OscilRec *op)
{

	op->phase+=op->freq;

	switch(op->state){
	case EG_ATT:	/*attack phase*/
		if ( (op->attack_volume -= op->delta_AR) < MIN_VOLUME_INDEX ){
			op->volume = MIN_VOLUME_INDEX;
			op->state = EG_DEC;
		}
		else
			op->volume = attack_curve[op->attack_volume>>ENV_SH];
		break;
	case EG_DEC:	/*decay phase*/
		if ( (op->volume += op->delta_D1R) > op->D1L )
			op->state = EG_SUS;
		break;
	case EG_SUS:	/*sustain phase*/
		if ( (op->volume += op->delta_D2R) > MAX_VOLUME_INDEX ){
			op->state = EG_OFF;
			op->volume = VOLUME_OFF;
		}
		break;
	case EG_REL:	/*release phase*/
		if ( (op->volume += op->delta_RR) > MAX_VOLUME_INDEX ){
			op->state = EG_OFF;
			op->volume = VOLUME_OFF;
		}
		break;
	}

	return op->TL + (op->volume>>ENV_SH);
}


// signed int PM; pm_calc(OP0,chan); pm_calc(OP1,chan); pm_calc(OP2,chan); pm_calc(OP3,chan);



//#define op_calc(OP,env,pm)  sin_tab[ ( (OP->phase>>FREQ_SH) + pm ) & SIN_MASK] [  env + (pm>>7) /*+ (AM & OP->AMSmask)*/ ]

#define op_calc(OP,env,pm)  sin_tab[ ( (OP->phase>>FREQ_SH) + pm ) & SIN_MASK] [  env  /*+ (AM & OP->AMSmask)*/ ]

static void chan_calc(unsigned int chan)
{
unsigned int env;
OscilRec *OP;

//unsigned int AM;

	chanout = c1 = m2 = c2 = 0;
	OP = &PSG->Oscils[chan]; /*M1*/

#if 0
	if (OP->AMS)
		AM = PSG->LFA << (OP->AMS - 1 + (ENV_BITS - 10)); /*LFA (8bits)*/
	else
		AM = 0;
#endif

	env = volume_calc(OP);
	if (env < ENV_RES)
	{
		signed int out;

		if (OP->FeedBack){
			out = OP->OscilFB;
			OP->OscilFB = op_calc(OP, env, (OP->OscilFB>>OP->FeedBack) );
			out = (OP->OscilFB + out)/2;
		}else{
			out = op_calc(OP, env, 0);
		}
		if( !OP->connect ){
			/* algorithm 5  */
			c1 = m2 = c2 = out;
		}else{
			/* other algorithm */
			*OP->connect += out;
		}
	}

	OP+=16; /*C1*/
	env = volume_calc(OP);
	if( env < ENV_RES )
		*OP->connect += op_calc(OP, env, c1);

	OP-=8;  /*M2*/
	env = volume_calc(OP);
	if( env < ENV_RES )
		*OP->connect += op_calc(OP, env, m2);

	OP+=16; /*C2*/
	env = volume_calc(OP);
	if( env < ENV_RES )
		*OP->connect += op_calc(OP, env, c2);

}

/*
** Generate samples for one of the YM2151's
**
** 'num' is the number of virtual YM2151
** '**buffers' is table of pointers to the buffers: left and right
** 'length' is the number of samples should be generated
*/
void YM2151UpdateOne(int num, SAMP **buffers, int length)
{
    unsigned int i;
    signed int outl,outr;

	bufL = buffers[0];
	bufR = buffers[1];

	PSG = &YMPSG[num];
	refresh_chip();

#ifndef USE_MAME_TIMERS
	/*calculate timers...*/
	if (PSG->TimA){
		PSG->TimAVal -= ( length << TIMER_SH );
		if (PSG->TimAVal<=0){
			PSG->TimA=0;
			/* ASG 980324 - handled by real timers now*/
			if ( PSG->IRQenable & 0x04 ){
				PSG->status |= 1;
				if( (PSG->status==1) && (chip->handler) ) (*chip->handler)(1);
			}
		}
	}
	if (PSG->TimB){
		PSG->TimBVal -= ( length << TIMER_SH );
		if (PSG->TimBVal<=0){
			PSG->TimB=0;
			/* ASG 980324 - handled by real timers now*/
			if ( PSG->IRQenable & 0x08 ){
				PSG->status |= 2;
				if( (PSG->status==2) && (chip->handler) ) (*chip->handler)(1);
			}
		}
	}
#endif

for (i=0; i<8; i++)
{
  outl=PSG->Oscils[i].PAN;
  mask[i*2] = (outl&0x40) ? 0xffffffff : 0x0;
  mask[i*2+1]=(outl&0x80) ? 0xffffffff : 0x0;
}


for( i = 0; i<length; i++ ){

//lfo_calc();

chan_calc(0);
    outl = chanout & mask[0];
    outr = chanout & mask[1];
chan_calc(1);
    outl += (chanout & mask[2]);
    outr += (chanout & mask[3]);
chan_calc(2);
    outl += (chanout & mask[4]);
    outr += (chanout & mask[5]);
chan_calc(3);
    outl += (chanout & mask[6]);
    outr += (chanout & mask[7]);
chan_calc(4);
    outl += (chanout & mask[8]);
    outr += (chanout & mask[9]);
chan_calc(5);
    outl += (chanout & mask[10]);
    outr += (chanout & mask[11]);
chan_calc(6);
    outl += (chanout & mask[12]);
    outr += (chanout & mask[13]);
chan_calc(7);
    outl += (chanout & mask[14]);
    outr += (chanout & mask[15]);


#ifdef SAVE_SAMPLE
    {	int pom = -(outl>>1);
	if (pom > 32767) pom = 32767; else if (pom < -32768) pom = -32768;
	fputc((unsigned short)pom&0xff,samplesum);
	fputc(((unsigned short)pom>>8)&0xff,samplesum);
    }
#endif

	if (sample_16bit){ 	/*16 bit mode*/
		outl >>= FINAL_SH16;
		outr >>= FINAL_SH16;
		if (outl > 32767) outl = 32767;
			else if (outl < -32768) outl = -32768;
		if (outr > 32767) outr = 32767;
			else if (outr < -32768) outr = -32768;
		((unsigned short*)bufL)[i] = (unsigned short)outl;
		((unsigned short*)bufR)[i] = (unsigned short)outr;
	}else{			/*8 bit mode*/
		outl >>= FINAL_SH8;
		outr >>= FINAL_SH8;
		if (outl > 127) outl = 127;
			else if (outl < -128) outl = -128;
		if (outr > 127) outr = 127;
			else if (outr < -128) outr = -128;
		((unsigned char*)bufL)[i] = (unsigned char)outl;
		((unsigned char*)bufR)[i] = (unsigned char)outr;
	}

}

}






#if 0
#ifdef SAVE_SEPARATE_CHANNELS
	for (j=7; j>=0; j--)
	{
	    pom1= -(ChanOut[j] & mask[j]);
		if (pom1 > 32767) pom1 = 32767;
			else if (pom1 < -32768) pom1 = -32768;
	    fputc((unsigned short)pom1&0xff,sample[j]);
	    fputc(((unsigned short)pom1>>8)&0xff,sample[j]);
	}
#endif
#endif

void YM2151SetIrqHandler(int n, void(*handler)(int irq))
{
    YMPSG[n].handler = handler;
}

void YM2151SetPortWriteHandler(int n, void(*handler)(int offset, int data))
{
    YMPSG[n].porthandler = handler;
}
