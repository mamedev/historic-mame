#define NICK_BUBLBOBL_CHANGE

/*
**
** File: fm.c -- software implementation of FM sound generator
**
** Copyright (C) 1998 Tatsuyuki Satoh , MultiArcadeMachineEmurator development
**
** Version 0.33d
**
*/

/*
    no check:
		CSM talking mode
		OPN 3 slot fnumber mode
		OPN SSG type envelope

	no support:
		status busy flag (already not busy )
		OPM LFO contoller
		OPM noise mode
		OPM output pins (CT1,CT0)
		OPM stereo output

	preliminary :
		key scale level rate (?)
		attack rate time rate , curve (?)
		decay  rate time rate , curve (?)
		self feedback rate

	note:
                        OPN                           OPM
		fnum          fMus * 2^20 / (fM/(12*n))
		TimerOverA    (12*n)*(1024-NA)/fFM        64*(1024-Na)/fm
		TimerOverB    (12*n)*(256-NB)/fFM       1024*(256-Nb)/fm
		output bits   10bit<<3bit               16bit * 2ch (YM3012=10bit<<3bit)
		sampling rate fFM / (12*6) ?            fFM / 64
		lfo freq                                ( fM*2^(LFRQ/16) ) / (4295*10^6)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "driver.h"
#include "psg.h"
#include "fm.h"

/* ------------------------------------------------------------------ */
#define MAME_TIMER 			/* use Aaron's timer system      */
/* -------------------- speed up optimize switch -------------------- */
/*#define SEG_SUPPORT		   OPN SSG type envelope support   */
#define S3_SUPPORT			/* OPN 3SLOT mode support        */
#define CSM_SUPPORT			/* CSM mode support              */
#define TL_SAVE_MEM			/* save some memories for total level */
/*#define LFO_SUPPORT			   LFO support                 */

/* -------------------- preliminary define section --------------------- */
/* attack/decay rate time rate */
#define OPM_ARRATE     399128
#define OPM_DRRATE    5514396
/* It is not checked , because I haven't YM2203 rate */
#define OPN_ARRATE  OPM_ARRATE
#define OPN_DRRATE  OPM_DRRATE

#define FREQ_BITS 24			/* frequency turn          */

/* counter bits = 21 , octerve 7 */
#define FREQ_RATE   (1<<(FREQ_BITS-21))
#define TL_BITS    (FREQ_BITS+2)

/* final output shift */
#define OPN_OUTSB   (TL_BITS+2-16)		/* OPN output final shift 16bit */
#define OPN_OUTSB_8 (OPN_OUTSB+8)		/* OPN output final shift  8bit */
#define OPM_OUTSB   (TL_BITS+2-16) 		/* OPM output final shift 16bit */
#define OPM_OUTSB_8 (OPM_OUTSB+8) 		/* OPM output final shift  8bit */
/* limit minimum and maximum */
#define OPN_MAXOUT (0x7fff<<OPN_OUTSB)
#define OPN_MINOUT (-0x8000<<OPN_OUTSB)
#define OPM_MAXOUT (0x7fff<<OPM_OUTSB)
#define OPM_MINOUT (-0x8000<<OPM_OUTSB)

/* -------------------- quality selection --------------------- */

/* sinwave entries */
/* used static memory = SIN_ENT * 4 (byte) */
#define SIN_ENT 2048

/* output level entries (envelope,sinwave) */
/* used dynamic memory = EG_ENT*4*4(byte)or EG_ENT*6*4(byte) */
/* used static  memory = EG_ENT*4 (byte)                     */
#define EG_ENT   4096
#define EG_STEP (96.0/EG_ENT) /* OPL is 0.1875 dB step  */
#define ENV_BITS 16			/* envelope lower bits      */

/* LFO table entries */
#define LFO_ENT 512

/* -------------------- local defines , macros --------------------- */

/* number of maximum envelope counter */
#define ENV_OFF ((EG_ENT<<ENV_BITS)-1)

/* register number to channel number , slot offset */
#define OPN_CHAN(N) (N&3)
#define OPN_SLOT(N) ((N>>2)&3)
#define OPM_CHAN(N) (N&7)
#define OPM_SLOT(N) ((N>>3)&3)
/* slot number */
#define SLOT1 0
#define SLOT2 2
#define SLOT3 1
#define SLOT4 3

/* envelope phase */
/* bit0  : not hold(Incrementabl)   */
/* bit1  : attack                   */
/* bit2  : decay downside           */
/* bit3  : decay upside(SSG-TYPE)   */
/* bit4-6: phase number             */

#define ENV_MOD_OFF 0x00
#define ENV_MOD_RR  0x15
#define ENV_MOD_SR  0x25
#define ENV_MOD_DR  0x35
#define ENV_MOD_AR  0x43
#define ENV_SSG_SR  0x59
#define ENV_SSG_DR  0x65

/* bit0 = right enable , bit1 = left enable (FOR YM2612) */
#define OPN_RIGHT  1
#define OPN_LEFT   2
#define OPN_CENTER 3

/* bit0 = left enable , bit1 = right enable */
#define OPM_LEFT   1
#define OPM_RIGHT  2
#define OPM_CENTER 3
/* */

/* ---------- OPN / OPM one channel  ---------- */
typedef struct fm_chan {
	unsigned int *freq_table;	/* frequency table */
	/* registers , sound type */
	int *DT[4];					/* detune          :DT_TABLE[DT]       */
	int DT2[4];					/* multiple,Detune2:(DT2<<4)|ML for OPM*/
	int TL[4];					/* total level     :TL << 8            */
	signed int TLL[4];			/* adjusted now TL                     */
	unsigned char KSR[4];		/* key scale rate  :3-KSR              */
	int *AR[4];					/* attack rate     :&AR_TABLE[AR<<1]   */
	int *DR[4];					/* decay rate      :&DR_TALBE[DR<<1]   */
	int *SR[4];					/* sustin rate     :&DR_TABLE[SR<<1]   */
	int  SL[4];					/* sustin level    :SL_TALBE[SL]       */
	int *RR[4];					/* release rate    :&DR_TABLE[RR<<2+2] */
	unsigned char SEG[4];		/* SSG EG type     :SSGEG              */
	unsigned char PAN;			/* PAN 0=off,3=center                  */
	unsigned char FB;			/* feed back       :&FB_TABLE[FB<<8]   */
	/* algorythm state */
	int *connect1;				/* operator 1 connection pointer       */
	int *connect2;				/* operator 2 connection pointer       */
	int *connect3;				/* operator 3 connection pointer       */
	/* phase generator state */
	unsigned int  fc;			/* fnum,blk        :calcrated          */
	unsigned char fn_h;			/* freq latch      :                   */
	unsigned char kcode;		/* key code        :                   */
	unsigned char ksr[4];		/* key scale rate  :kcode>>(3-KSR)     */
	unsigned int mul[4];		/* multiple        :ML_TABLE[ML]       */
	unsigned int Cnt[4];		/* frequency count :                   */
	unsigned int Incr[4];		/* frequency step  :                   */
	int op1_out;				/* op1 output foe beedback             */
	/* envelope generator state */
	unsigned char evm[4];		/* envelope phase                      */
	signed int evc[4];			/* envelope counter                    */
	signed int arc[4];			/* envelope counter for AR             */
	signed int evsa[4];			/* envelope step for AR                */
	signed int evsd[4];			/* envelope step for DR                */
	signed int evss[4];			/* envelope step for SR                */
	signed int evsr[4];			/* envelope step for RR                */
} FM_CH;

/* generic fm state */
typedef struct fm_state {
	int bufp;					/* update buffer point */
	int clock;					/* master clock  (Hz)  */
	int rate;					/* sampling rate (Hz)  */
	int freqbase;				/* frequency base      */
	unsigned char status;		/* status flag         */
	unsigned int mode;			/* mode  CSM / 3SLOT   */
	int TA;						/* timer a             */
	int TAC;					/* timer a counter     */
	unsigned char TB;			/* timer b             */
	int TBC;					/* timer b counter     */
	void (*handler)(void);			/* interrupt handler   */
	void *timer_a_timer;		/* timer for a         */
	void *timer_b_timer;		/* timer for b         */
	/* speedup customize */
	/* time tables */
	signed int DT_TABLE[8][32];     /* detune tables       */
	signed int AR_TABLE[94];	/* atttack rate tables */
	signed int DR_TABLE[94];	/* decay rate tables   */
}FM_ST;

/* here's the virtual YM2203(OPN)  */
typedef struct ym2203_f {
	FMSAMPLE *Buf;				/* sound buffer */
	FM_ST ST;					/* general state     */
	FM_CH CH[3];				/* channel state     */
/*	unsigned char PR;			   freqency priscaler  */
	/* 3 slot mode state */
	unsigned int  fc3[3];		/* fnum3,blk3  :calcrated */
	unsigned char fn3_h[3];		/* freq3 latch       */
	unsigned char kcode3[3];	/* key code    :     */
	/* fnumber -> increment counter */
	unsigned int FN_TABLE[2048];
} YM2203;

/* here's the virtual YM2151(OPM)  */
typedef struct ym2151_f {
	FMSAMPLE *Buf[YM2151_NUMBUF];/* sound buffers    */
	FM_ST  ST;					/* general state     */
	FM_CH CH[8];				/* channel state     */
	unsigned char NReg;			/* noise enable,freq */
	unsigned char pmd;			/* LFO pmd level     */
	unsigned char amd;			/* LFO amd level     */
	unsigned char ctw;			/* CT0,1 and waveform */
	unsigned int KC_TABLE[8*12*64+950];/* keycode,keyfunction -> count */
} YM2151;


/* -------------------- tables --------------------- */

/* key scale level */
/* !!!!! preliminary !!!!! */

#define DV (1/EG_STEP)
static unsigned char KSL[32]=
{
#if 1
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
#else
 0.000/DV , 0.000/DV , 0.000/DV , 0.000/DV ,	/* OCT 0 */
 0.000/DV , 0.000/DV , 0.000/DV , 1.875/DV ,	/* OCT 1 */
 0.000/DV , 0.000/DV , 3.000/DV , 4.875/DV ,	/* OCT 2 */
 0.000/DV , 3.000/DV , 6.000/DV , 7.875/DV ,	/* OCT 3 */
 0.000/DV , 6.000/DV , 9.000/DV ,10.875/DV ,	/* OCT 4 */
 0.000/DV , 9.000/DV ,12.000/DV ,13.875/DV ,	/* OCT 5 */
 0.000/DV ,12.000/DV ,15.000/DV ,16.875/DV ,	/* OCT 6 */
 0.000/DV ,15.000/DV ,18.000/DV ,19.875/DV		/* OCT 7 */
#endif
};
#undef DV

/* OPN key frequency number -> key code follow table */
/* fnum higher 4bit -> keycode lower 2bit */
static char OPN_FKTABLE[16]={0,0,0,0,0,0,0,1,2,3,3,3,3,3,3,3};

static int KC_TO_SEMITONE[16]={
	/*translate note code KC into more usable number of semitone*/
	0*64, 1*64, 2*64, 3*64,
	3*64, 4*64, 5*64, 6*64,
	6*64, 7*64, 8*64, 9*64,
	9*64,10*64,11*64,12*64
};

int DT2_TABLE[4]={ /* 4 DT2 values */
/*
 *   DT2 defines offset in cents from base note
 *
 *   The table below defines offset in deltas table...
 *   User's Manual page 22
 *   Values below were calculated using formula:  value = orig.val * 1.5625
 *
 * DT2=0 DT2=1 DT2=2 DT2=3
 * 0     600   781   950
 */
	0,    384,  500,  608
};

/* sustain lebel table (3db per step) */
/* 0 - 15: 0, 3, 6, 9,12,15,18,21,24,27,30,33,36,39,42,93 (dB)*/
#define ML ((3/EG_STEP)*(1<<ENV_BITS))
static int SL_TABLE[16]={
 ML* 0,ML* 1,ML* 2,ML* 3,ML* 4,ML* 5,ML* 6,ML* 7,
 ML* 8,ML* 9,ML*10,ML*11,ML*12,ML*13,ML*14,ML*31
};
#undef ML

#ifdef TL_SAVE_MEM
  #define TL_MAX (EG_ENT*2) /* limit(tl + ksr + envelope) + sinwave */
#else
  #define TL_MAX (EG_ENT*4) /* tl + ksr + envelope + sinwave */
#endif

/* TotalLevel : 48 24 12  6  3 1.5 0.75 (dB) */
/* TL_TABLE[ 0      to TL_MAX          ] : plus  section */
/* TL_TABLE[ TL_MAX to TL_MAX+TL_MAX-1 ] : minus section */
static int *TL_TABLE;

/* pointers to TL_TABLE with sinwave output offset */
static signed int *SIN_TABLE[SIN_ENT];

/* attack count to enveloe count conversion */
static int AR_CURVE[EG_ENT];

#define OPM_DTTABLE OPN_DTTABLE
static char OPN_DTTABLE[4 * 32]={
/* this table is YM2151 and YM2612 data */
/* FD=0 */
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* FD=1 */
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2,
  2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8, 8, 8,
/* FD=2 */
  1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5,
  5, 6, 6, 7, 8, 8, 9,10,11,12,13,14,16,16,16,16,
/* FD=3 */
  2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7,
  8 , 8, 9,10,11,12,13,14,16,17,19,20,22,22,22,22
};

/* multiple table */
#define ML 2
static int MUL_TABLE[4*16]= {
/* 1/2, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15 */
   0.50*ML, 1.00*ML, 2.00*ML, 3.00*ML, 4.00*ML, 5.00*ML, 6.00*ML, 7.00*ML,
   8.00*ML, 9.00*ML,10.00*ML,11.00*ML,12.00*ML,13.00*ML,14.00*ML,15.00*ML,
/* DT2=1 *SQL(2)   */
   0.71*ML, 1.41*ML, 2.82*ML, 4.24*ML, 5.65*ML, 7.07*ML, 8.46*ML, 9.89*ML,
  11.30*ML,12.72*ML,14.10*ML,15.55*ML,16.96*ML,18.37*ML,19.78*ML,21.20*ML,
/* DT2=2 *SQL(2.5) */
   0.78*ML, 1.57*ML, 3.14*ML, 4.71*ML, 6.28*ML, 7.85*ML, 9.42*ML,10.99*ML,
  12.56*ML,14.13*ML,15.70*ML,17.27*ML,18.84*ML,20.41*ML,21.98*ML,23.55*ML,
/* DT2=3 *SQL(3)   */
   0.87*ML, 1.73*ML, 3.46*ML, 5.19*ML, 6.92*ML, 8.65*ML,10.38*ML,12.11*ML,
  13.84*ML,15.57*ML,17.30*ML,19.03*ML,20.76*ML,22.49*ML,24.22*ML,25.95*ML
};
#undef ML

#ifdef LFO_SUPPORT
/* LFO frequency timer table */
static int OPM_LFO_TABLE[256];
#endif

/* dummy attack / decay rate ( when rate == 0 ) */
static int RATE_0[32]=
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/* -------------------- state --------------------- */

/* some globals */
static int FMBufSize;		/* size of sound buffer, in samples */
static int FMNumChips;		/* total # of FM emulated */

static YM2203 *FMOPN=NULL;	/* array of OPN's */
static YM2151 *FMOPM=NULL;	/* array of OPM's */

static unsigned char sample_16bit;/* output bits    */

/* work table */
static void *cur_chip = 0;	/* current chip point */

/* currenct chip state */
FM_ST            *State;
static FM_CH     *chA,*chB,*chC,*chD,*chE,*chF,*chG,*chH;
static int *pan[8];			/* pointer to outd[0-3]; */
static int outd[4];			/*0=off,1=left,2=right,3=center */

/* operator connection work area */
static int feedback2;		/* connect for operator 2 */
static int feedback3;		/* connect for operator 3 */
static int feedback4;		/* connect for operator 4 */
static int carrier;			/* connect for carrier    */

/* --------------------- subroutines  --------------------- */
/* ----- key on  ----- */
INLINE void FM_KEYON(FM_CH *CH , int s )
{
	if( CH->evm[s]<= ENV_MOD_RR){
		/* sin wave restart */
		CH->Cnt[s] = 0;
		if( s == SLOT1 ) CH->op1_out = 0;
		/* set attack */
		CH->evm[s] = ENV_MOD_AR;
		CH->arc[s] = ENV_OFF;
#if 1
		CH->evc[s] = 0;
#else
		CH->evc[s] = CH->evsa[s] >= ENV_OFF )  ? 0 : ENV_OFF;
#endif
	}
}
/* ----- key off ----- */
INLINE void FM_KEYOFF(FM_CH *CH , int s )
{
	if( CH->evm[s] > ENV_MOD_RR){
		CH->evm[s] = ENV_MOD_RR;
		/*if( CH->evsr[s] == 0 ) CH->evm[s] &= 0xfe;*/
	}
}

/* ---------- calcrate Envelope Generator & Phase Generator ---------- */
/* return == -1:envelope silent */
/*        >=  0:envelope output */
INLINE signed int FM_CALC_SLOT( FM_CH *CH  , int s )
{
	signed int env_out;

	/* calcrate phage generator */
	CH->Cnt[s] += CH->Incr[s];

	/* bypass when envelope holding */
	if( !(CH->evm[s]&0x0f) ) return -1;

	switch( CH->evm[s] ){
	case ENV_MOD_AR:
		/* attack rate : linear(dB) curve */
		if( (CH->arc[s]-= CH->evsa[s]) > 0 )
		{
			CH->evc[s] = AR_CURVE[CH->arc[s]>>ENV_BITS];
			break;
		}
		/* change to next phase */
#ifdef SEG_SUPPORT
		if( !(CH->SEG[s]&8) ){
#endif
			/* next DR */
			CH->evm[s] = ENV_MOD_DR;
/* MB 980409  commented out the following line to fix Bubble Bobble */
			/*if( CH->evsd[s] == 0 ) CH->evm[s] &= 0xfe;    stop   */

			CH->evc[s] = 0;
#ifdef SEG_SUPPORT
		}else{
			/* SSG-EG mode */
			if( CH->SEG[s]&4){	/* start direction */
				/* next SSG-SR (upside start ) */
				CH->evm[s] = ENV_SSG_SR;
				CH->evc[s] = CH->SL[s];
			}else{
				/* next SSG-DR (downside start ) */
				CH->evm[s] = ENV_SSG_DR;
				CH->evc[s] = 0;
			}
		}
#endif
		break;
	case ENV_MOD_DR:
		if( (CH->evc[s]+=CH->evsd[s]) < CH->SL[s] )
		{
			break;
		}
		/* change phase */
		/* next SR */
		CH->evm[s] = ENV_MOD_SR;
/* MB 980409  commented out the following line to fix Bubble Bobble */
		/*if( CH->evss[s] == 0 ) CH->evm[s] &= 0xfe;    stop   */
		CH->evc[s] = CH->SL[s];
		break;
	case ENV_MOD_SR:
		if( (CH->evc[s]+=CH->evss[s]) <= ENV_OFF )
		{
			break;
		}
		/* wait for key off */
		/*CH->evm[s]&= 0xf0;		   off   */
		CH->evc[s] = ENV_OFF;
		break;
	case ENV_MOD_RR:
		if( (CH->evc[s]+=CH->evsr[s]) < ENV_OFF )
		{
			break;
		}
		/* wait for key on */
		CH->evm[s] = ENV_MOD_OFF;
		CH->evc[s] = ENV_OFF;
		break;
#ifdef SEG_SUPPORT
	/* SSG type envelope */
	case ENV_SSG_DR:	/* downside */
		if( (CH->evc[s]+=CH->evsd[s]) < CH->SL[s] )
		{
			break;
		}
		/* change next phase */
		if( CH->SEG[s]&2){
			/* reverce */
			CH->evm[s] = ENV_SSG_SR;
			CH->evc[s] = CH->SL[s];
		}else{
			/* again */
			CH->evc[s] = 0;
		}
		/* hold check */
		if( CH->SEG[s]&1) CH->evm[s] &= 0xf0;
		break;
	case ENV_SSG_SR:	/* upside */
		if( (CH->evc[s]-=CH->evss[s]) > 0 )
		{
			break;
		}
		/* next phase */
		if( CH->SEG[s]&2){
			/* reverce  */
			CH->evm[s] = ENV_SSG_DR;
			CH->evc[s] = 0;
		}else{
			/* again */
			CH->evc[s] = CH->SL[s];
		}
		/* hold check */
		if( CH->SEG[s]&1) CH->evm[s] &= 0xf0;
#endif
	}
	/* calcrate envelope */
	env_out = CH->TLL[s]+(CH->evc[s]>>ENV_BITS);
#ifdef TL_SAVE_MEM
	if(env_out >= (EG_ENT-1) )
	{
		return -1;
	}
#endif
	return env_out;
}

/* set algorythm and self feedback */
/* b0-2 : algorythm                */
/* b3-5 : self feedback            */
static void set_algorythm( FM_CH *CH , int algo_fb )
{
	/* setup connect algorythm */
	switch( algo_fb & 7 ){
	case 0:
		/*  PG---S1---S2---S3---S4---OUT */
		CH->connect1 = &feedback2;
		CH->connect2 = &feedback3;
		CH->connect3 = &feedback4;
		break;
	case 1:
		/*  PG---S1-+-S3---S4---OUT */
		/*  PG---S2-+               */
		CH->connect1 = &feedback3;
		CH->connect2 = &feedback3;
		CH->connect3 = &feedback4;
		break;
	case 2:
		/* PG---S1------+-S4---OUT */
		/* PG---S2---S3-+          */
		CH->connect1 = &feedback4;
		CH->connect2 = &feedback3;
		CH->connect3 = &feedback4;
		break;
	case 3:
		/* PG---S1---S2-+-S4---OUT */
		/* PG---S3------+          */
		CH->connect1 = &feedback2;
		CH->connect2 = &feedback4;
		CH->connect3 = &feedback4;
		break;
	case 4:
		/* PG---S1---S2-+--OUT */
		/* PG---S3---S4-+      */
		CH->connect1 = &feedback2;
		CH->connect2 = &carrier;
		CH->connect3 = &feedback4;
		break;
	case 5:
		/*         +-S2-+     */
		/* PG---S1-+-S3-+-OUT */
		/*         +-S4-+     */
		CH->connect1 = 0;	/* special mark */
		CH->connect2 = &carrier;
		CH->connect3 = &carrier;
		break;
	case 6:
		/* PG---S1---S2-+     */
		/* PG--------S3-+-OUT */
		/* PG--------S4-+     */
		CH->connect1 = &feedback2;
		CH->connect2 = &carrier;
		CH->connect3 = &carrier;
		break;
	case 7:
		/* PG---S1-+     */
		/* PG---S2-+-OUT */
		/* PG---S3-+     */
		/* PG---S4-+     */
		CH->connect1 = &carrier;
		CH->connect2 = &carrier;
		CH->connect3 = &carrier;
	}
	/* self feedback */
	algo_fb = (algo_fb>>3) & 7;
	CH->FB = algo_fb ? 8 - algo_fb : 0;
	CH->op1_out = 0;
}

/* operator output calcrator */
#define OP_OUT(SLOT,CON)   SIN_TABLE[((CH->Cnt[SLOT]+CON)/(0x1000000/SIN_ENT))&(SIN_ENT-1)][env_out]
/* ---------- calcrate one of channel ---------- */
INLINE int FM_CALC_CH( FM_CH *CH )
{
	int op_out;
	int env_out;

	/* bypass all SLOT output off (SILENCE) */
	if( !(*(long *)(&CH->evm[0])) ) return 0;

	/* clear carrier output */
	feedback2 = feedback3 = feedback4 = carrier = 0;

	/* SLOT 1 */
	if( (env_out=FM_CALC_SLOT(CH,SLOT1))>=0 )
	{
		if( CH->FB ){
			/* with self feed back */
			op_out = CH->op1_out;
			CH->op1_out = OP_OUT(SLOT1,(CH->op1_out>>CH->FB) );
			op_out = (op_out + CH->op1_out)/2;
		}else{
			/* without self feed back */
			op_out = OP_OUT(SLOT1,0);
		}
		if( !CH->connect1 )
		{
			/* algorythm 5  */
			feedback2 = feedback3 = feedback4 = op_out;
		}else{
			/* other algorythm */
			*CH->connect1 = op_out;
		}
	}
	/* SLOT 2 */
	if( (env_out=FM_CALC_SLOT(CH,SLOT2))>=0 )
		*CH->connect2 += OP_OUT(SLOT2, feedback2 );
	/* SLOT 3 */
	if( (env_out=FM_CALC_SLOT(CH,SLOT3))>=0 )
		*CH->connect3 += OP_OUT(SLOT3, feedback3 );
	/* SLOT 4 */
	if( (env_out=FM_CALC_SLOT(CH,SLOT4))>=0 )
		carrier       += OP_OUT(SLOT4, feedback4 );

	return carrier;
}
/* ---------- frequency counter for operater update ---------- */
INLINE void CALC_FCSLOT(FM_CH *CH , int s , int fc , int kc )
{
	int ksr;

	CH->Incr[s]= (fc * CH->mul[s]) + CH->DT[s][kc];

	ksr = kc >> CH->KSR[s];
	if( CH->ksr[s] != ksr )
	{
		CH->ksr[s] = ksr;
		/* attack . decay rate recalcration */
		CH->evsa[s] = CH->AR[s][ksr];
		CH->evsd[s] = CH->DR[s][ksr];
		CH->evss[s] = CH->SR[s][ksr];
		CH->evsr[s] = CH->RR[s][ksr];
	}
	CH->TLL[s] = CH->TL[s] + KSL[kc];
}

/* ---------- frequency counter  ---------- */
INLINE void CALC_FCOUNT(FM_CH *CH )
{
	if( CH->Incr[SLOT1]==-1){
		int fc = CH->fc;
		int kc = CH->kcode;
		CALC_FCSLOT(CH , SLOT1 , fc , kc );
		CALC_FCSLOT(CH , SLOT2 , fc , kc );
		CALC_FCSLOT(CH , SLOT3 , fc , kc );
		CALC_FCSLOT(CH , SLOT4 , fc , kc );
	}
}

/* ---------- frequency counter  ---------- */
INLINE void OPM_CALC_FCOUNT(YM2151 *OPM , FM_CH *CH )
{
	if( CH->Incr[SLOT1]==-1)
	{
		int kc = CH->kcode;
		CALC_FCSLOT(CH , SLOT1 , OPM->KC_TABLE[CH->fc + CH->DT2[SLOT1]] , kc );
		CALC_FCSLOT(CH , SLOT2 , OPM->KC_TABLE[CH->fc + CH->DT2[SLOT2]] , kc );
		CALC_FCSLOT(CH , SLOT3 , OPM->KC_TABLE[CH->fc + CH->DT2[SLOT3]] , kc );
		CALC_FCSLOT(CH , SLOT4 , OPM->KC_TABLE[CH->fc + CH->DT2[SLOT4]] , kc );
	}
}

#ifdef MAME_TIMER
static void timer_callback_A_OPM(int param)
{
	YM2151 *OPM = &FMOPM[param];

	OPM->ST.timer_a_timer = 0;

	/* update the system */
	OPMUpdateOne (param, cpu_scalebyfcount (FMBufSize));
	/* callback user interrupt handler */
	if (OPM->ST.mode & 0x04)
		if( OPM->ST.handler != 0 ) OPM->ST.handler();
	/* set status flag */
	OPM->ST.status |= 0x01;
	/* update the counter */
	OPM->ST.TAC = 0;

#ifdef CSM_SUPPORT
	if( OPM->ST.mode & 0x80 )
	{	/* CSM mode total level latch and auto key on */
		FM_CH *CSM_CH = &OPM->CH[7];
		int ksl = KSL[CSM_CH->kcode];
		CSM_CH->TLL[SLOT1] = CSM_CH->TL[SLOT1] + ksl;
		CSM_CH->TLL[SLOT2] = CSM_CH->TL[SLOT2] + ksl;
		CSM_CH->TLL[SLOT3] = CSM_CH->TL[SLOT3] + ksl;
		CSM_CH->TLL[SLOT4] = CSM_CH->TL[SLOT4] + ksl;
		/* all key on */
		FM_KEYON(CSM_CH,SLOT1);
		FM_KEYON(CSM_CH,SLOT2);
		FM_KEYON(CSM_CH,SLOT3);
		FM_KEYON(CSM_CH,SLOT4);
	}
#endif
}

static void timer_callback_B_OPM(int param)
{
	YM2151 *OPM = &FMOPM[param];

	OPM->ST.timer_b_timer = 0;

	/* update the system */
	OPMUpdateOne (param, cpu_scalebyfcount (FMBufSize));
	/* callback user interrupt handler */
	if (OPM->ST.mode & 0x08)
		if( OPM->ST.handler != 0 ) OPM->ST.handler();
	/* set status flag */
	OPM->ST.status |= 0x02;
	/* update the counter */
	OPM->ST.TBC = 0;
}

static void timer_callback_A_OPN(int param)
{
	YM2203 *OPN = &FMOPN[param];

	OPN->ST.timer_a_timer = 0;

	/* update the system */
	OPNUpdateOne (param, cpu_scalebyfcount (FMBufSize));
	/* callback user interrupt handler */
	if (OPN->ST.mode & 0x04)
		if( OPN->ST.handler != 0 ) OPN->ST.handler();
	/* set status flag */
	OPN->ST.status |= 0x01;
	/* update the counter */
#ifdef NICK_BUBLBOBL_CHANGE
	OPN->ST.TAC = (1024-OPN->ST.TA)<<12;
	/*if (OPN->ST.handler)*/
		OPN->ST.timer_a_timer = timer_set ((double)OPN->ST.TAC / ((double)OPN->ST.freqbase * (double)OPN->ST.rate), param, timer_callback_A_OPN);
#else
	OPN->ST.TAC = 0;
#endif

#ifdef CSM_SUPPORT
	if( OPN->ST.mode & 0x80 )
	{	/* CSM mode total level latch and auto key on */
		FM_CH *CSM_CH = &OPN->CH[2];
		int ksl = KSL[CSM_CH->kcode];
		CSM_CH->TLL[SLOT1] = CSM_CH->TL[SLOT1] + ksl;
		CSM_CH->TLL[SLOT2] = CSM_CH->TL[SLOT2] + ksl;
		CSM_CH->TLL[SLOT3] = CSM_CH->TL[SLOT3] + ksl;
		CSM_CH->TLL[SLOT4] = CSM_CH->TL[SLOT4] + ksl;
		/* all key on */
		FM_KEYON(CSM_CH,SLOT1);
		FM_KEYON(CSM_CH,SLOT2);
		FM_KEYON(CSM_CH,SLOT3);
		FM_KEYON(CSM_CH,SLOT4);
	}
#endif
}

static void timer_callback_B_OPN(int param)
{
	YM2203 *OPN = &FMOPN[param];

	OPN->ST.timer_b_timer = 0;

	/* update the system */
	OPNUpdateOne (param, cpu_scalebyfcount (FMBufSize));
	/* callback user interrupt handler */
	if (OPN->ST.mode & 0x08)
		if( OPN->ST.handler != 0 ) OPN->ST.handler();
	/* set status flag */
	OPN->ST.status |= 0x02;
	/* update the counter */
#ifdef NICK_BUBLBOBL_CHANGE
	OPN->ST.TBC = ( 256-OPN->ST.TB)<<(4+12);
	/*if (OPN->ST.handler)*/
		OPN->ST.timer_b_timer = timer_set ((double)OPN->ST.TBC / ((double)OPN->ST.freqbase * (double)OPN->ST.rate), param, timer_callback_B_OPN);
#else
	OPN->ST.TBC = 0;
#endif
}
#else /* MAME_TIMER */

/* ---------- calcrate timer A ---------- */
INLINE void CALC_TIMER_A(FM_ST *ST , FM_CH *CSM_CH )
{
	if( ST->TAC )
	{
		if( (ST->TAC -= ST->freqbase) <= 0 )
		{
			ST->TAC = 0; /* stop timer */
			if( (ST->mode & 0x04) && (ST->status & 0x03)==0 )
			{	/* user interrupt handler call back */
				if( ST->handler != 0 ) ST->handler();
			}
			ST->status |= 0x01;
#ifdef CSM_SUPPORT
			if( ST->mode & 0x80 )
			{	/* CSM mode total level latch and auto key on */
				int ksl = KSL[CSM_CH->kcode];
				CSM_CH->TLL[SLOT1] = CSM_CH->TL[SLOT1] + ksl;
				CSM_CH->TLL[SLOT2] = CSM_CH->TL[SLOT2] + ksl;
				CSM_CH->TLL[SLOT3] = CSM_CH->TL[SLOT3] + ksl;
				CSM_CH->TLL[SLOT4] = CSM_CH->TL[SLOT4] + ksl;
				/* all key on */
				FM_KEYON(CSM_CH,SLOT1);
				FM_KEYON(CSM_CH,SLOT2);
				FM_KEYON(CSM_CH,SLOT3);
				FM_KEYON(CSM_CH,SLOT4);
			}
#endif
		}
	}
}
/* ---------- calcrate timer B ---------- */
INLINE void CALC_TIMER_B(FM_ST *ST,int step)
{
	if( ST->TBC ){
		if( (ST->TBC -= ST->freqbase*step) <= 0 )
		{
			ST->TBC = 0;
			if( (ST->mode & 0x08) && (ST->status & 0x03)==0 )
			{	/* interrupt reqest */
				if( ST->handler != 0 ) ST->handler();
			}
			ST->status |= 0x02;
		}
	}
}

#endif /* MAME_TIMER */

/* ----------- initialize time tabls ----------- */
static void init_timetables( FM_ST *ST , char *DTTABLE , int ARRATE , int DRRATE )
{
	int i,d;
	float rate;

	/* make detune table */
	for (d = 0;d <= 3;d++){
		for (i = 0;i <= 31;i++){
			rate = (float)DTTABLE[d*32 + i] * ST->freqbase / 4096 * FREQ_RATE;
			ST->DT_TABLE[d][i]   =  rate;
			ST->DT_TABLE[d+4][i] = -rate;
		}
	}
	/* make attack rate & decay rate tables */
	for (i = 0;i < 4;i++) ST->AR_TABLE[i] = ST->DR_TABLE[i] = 0;
	for (i = 4;i < 64;i++){
		if( i == 0 ) rate = 0;
		else
		{
			rate  = (float)ST->freqbase / 4096.0;		/* frequency rate */
			if( i < 60 ) rate *= 1.0+(i&3)*0.25;		/* b0-1 : x1 , x1.25 , x1.5 , x1.75 */
			rate *= 1<<((i>>2)-1);						/* b2-5 : shift bit */
			rate *= (float)(EG_ENT<<ENV_BITS);
		}
		ST->AR_TABLE[i] = rate / ARRATE;
		ST->DR_TABLE[i] = rate / DRRATE;
	}
	ST->AR_TABLE[63] = ENV_OFF;

	for (i = 64;i < 94 ;i++){	/* make for overflow area */
		ST->AR_TABLE[i] = ST->AR_TABLE[63];
		ST->DR_TABLE[i] = ST->DR_TABLE[63];
	}

#if 0
	if(errorlog){
		for (i = 0;i < 64 ;i++){	/* make for overflow area */
			fprintf(errorlog,"rate %2d , ar %f ms , dr %f ms \n",i,
				((float)(EG_ENT<<ENV_BITS) / ST->AR_TABLE[i]) * (1000.0 / ST->rate),
				((float)(EG_ENT<<ENV_BITS) / ST->DR_TABLE[i]) * (1000.0 / ST->rate) );
		}
	}
#endif
}

/* ---------- reset one of channel  ---------- */
static void reset_channel( FM_ST *ST , FM_CH *ch , int chan )
{

	int c,s;

	ST->mode   = 0;	/* normal mode */
	ST->status = 0;
	ST->bufp   = 0;
	ST->TA     = 0;
	ST->TAC    = 0;
	ST->TB     = 0;
	ST->TBC    = 0;

	ST->timer_a_timer = 0;
	ST->timer_b_timer = 0;

	for( c = 0 ; c < chan ; c++ )
	{
		ch[c].fc = 0;
		for(s = 0 ; s < 4 ; s++ )
		{
			ch[c].SEG[s] = 0;
			ch[c].evc[s] = ENV_OFF;
			ch[c].evm[s] = ENV_MOD_OFF;
		}
	}
}

/* ---------- generic table initialize ---------- */
static int FMInitTable( void )
{
	int s,t;
	float rate;
	int i,j;
	float pom;

	/* allocate total level table */
	TL_TABLE = malloc(TL_MAX*2*sizeof(int));
	if( TL_TABLE == 0 ) return 0;
	/* make total level table */
	for (t = 0;t < EG_ENT-1 ;t++){
		rate = ((1<<TL_BITS)-1)/pow(10,EG_STEP*t/20);	/* dB -> voltage */
		TL_TABLE[       t] =  (int)rate;
		TL_TABLE[TL_MAX+t] = -TL_TABLE[t];
/*		if(errorlog) fprintf(errorlog,"TotalLevel(%3d) = %x\n",t,TL_TABLE[t]);*/
	}
	/* fill volume off area */
	for ( t = EG_ENT-1; t < TL_MAX ;t++){
		TL_TABLE[t] = TL_TABLE[TL_MAX+t] = 0;
	}

	/* make sinwave table (total level offet) */
	 /* degree 0 = degree 180                   = off */
	SIN_TABLE[0] = SIN_TABLE[SIN_ENT/2]         = &TL_TABLE[EG_ENT-1];
	for (s = 1;s <= SIN_ENT/4;s++){
		pom = sin(2*PI*s/SIN_ENT); /* sin     */
		pom = 20*log10(1/pom);	   /* decibel */
		j = pom / EG_STEP;         /* TL_TABLE steps */

        /* degree 0   -  90    , degree 180 -  90 : plus section */
		SIN_TABLE[          s] = SIN_TABLE[SIN_ENT/2-s] = &TL_TABLE[j];
        /* degree 180 - 270    , degree 360 - 270 : minus section */
		SIN_TABLE[SIN_ENT/2+s] = SIN_TABLE[SIN_ENT  -s] = &TL_TABLE[TL_MAX+j];
/*		if(errorlog) fprintf(errorlog,"sin(%3d) = %f:%f db\n",s,pom,(float)j * EG_STEP);*/
	}

	/* attack count -> envelope count */
	for (i=0; i<EG_ENT; i++)
	{
		/* !!!!! preliminary !!!!! */
		pom = pow( ((float)i/EG_ENT) , 8 ) * EG_ENT;
		j = pom * (1<<ENV_BITS);
		if( j > ENV_OFF ) j = ENV_OFF;

		AR_CURVE[i]=j;
/*		if (errorlog) fprintf(errorlog,"arc->env %04i = %x\n",i,j>>ENV_BITS);*/
	}

	return 1;
}

static void FMCloseTable( void )
{
	if( TL_TABLE ) free( TL_TABLE );
	return;
}

/* ---------- priscaler set(and make time tables) ---------- */
void OPNSetPris(int num , int pris , int SSGpris)
{
    YM2203 *OPN = &(FMOPN[num]);
	int fn;

/*	OPN->PR = pris; */
	if (OPN->ST.rate)
		OPN->ST.freqbase = ((float)OPN->ST.clock * 4096.0 / OPN->ST.rate) / 12 / pris;
	else OPN->ST.freqbase = 0;

	/* SSG part  priscaler set */
	AYSetClock( num, OPN->ST.clock * 2 / SSGpris , OPN->ST.rate );
	/* make time tables */
	init_timetables( &OPN->ST , OPN_DTTABLE , OPN_ARRATE , OPN_DRRATE );
	/* make fnumber -> increment counter table */
	for( fn=0 ; fn < 2048 ; fn++ )
	{
		/* it is freq table for octave 7 */
		/* opn freq counter = 20bit */
		OPN->FN_TABLE[fn] = (float)fn * OPN->ST.freqbase / 4096  * FREQ_RATE * (1<<7) / 2;
	}
/*	if(errorlog) fprintf(errorlog,"OPN %d set priscaler %d\n",num,pris);*/
}

/* ---------- reset one of chip ---------- */
void OPNResetChip(int num)
{
    int i;
    YM2203 *OPN = &(FMOPN[num]);

/*	OPNSetPris( num , 6 , 4);    1/6 , 1/4   */
	OPNSetPris( num , 3 , 2); /* 1/3 , 1/2 */
	reset_channel( &OPN->ST , &OPN->CH[0] , 3 );
	/* status clear */
	OPNWriteReg(num,0x27,0x30); /* mode 0 , timer reset */
	/* reset OPerator paramater */
	for(i = 0xb6 ; i >= 0x30 ; i-- ) OPNWriteReg(num,i,0);
	for(i = 0x27 ; i >= 0x20 ; i-- ) OPNWriteReg(num,i,0);
}
/* ---------- release storage for a chip ---------- */
static void _OPNFreeChip(int num)
{
    YM2203 *OPN = &(FMOPN[num]);
 	OPN->Buf = NULL;
}

/* ----------  Initialize YM2203 emulator(s) ----------    */
/* 'num' is the number of virtual YM2203's to allocate     */
/* 'rate' is sampling rate and 'bufsiz' is the size of the */
/* buffer that should be updated at each interval          */
int OPNInit(int num, int clock, int rate,  int bitsize, int bufsiz , FMSAMPLE **buffer )
{
    int i;

    if (FMOPN) return (-1);	/* duplicate init. */

    FMNumChips = num;
    FMBufSize = bufsiz;
    if( bitsize == 16 ) sample_16bit = 1;
    else                sample_16bit = 0;

	if( (FMOPN) ) return -1;
	/* allocate ym2203 state space */
	if( (FMOPN = (YM2203 *)malloc(sizeof(YM2203) * FMNumChips))==NULL)
		return (-1);
	/* allocate total lebel table (128kb space) */
	if( !FMInitTable() )
	{
		free( FMOPN );
		return (-1);
	}

	for ( i = 0 ; i < FMNumChips; i++ ) {
		FMOPN[i].ST.clock = clock;
		FMOPN[i].ST.rate = rate;
		FMOPN[i].ST.handler = 0;
		if( OPNSetBuffer(i,buffer[i]) < 0 ) {
		    int j;
		    for ( j = 0 ; j < i ; j ++ ) {
				_OPNFreeChip(j);
		    }
		    return(-1);
		}
		OPNResetChip(i);
    }
    return(0);
}


/* ---------- shut down emurator ----------- */
void OPNShutdown()
{
    int i;

    if (!FMOPN) return;
    for ( i = 0 ; i < FMNumChips ; i++ ) {
		_OPNFreeChip(i);
	}
	free(FMOPN); FMOPN = NULL;
	FMCloseTable();
	FMBufSize = 0;
}
/* ---------- write a register on YM2203 chip number 'n' ---------- */
void OPNWriteReg(int n, int r, int v)
{
	unsigned char c,s;
	FM_CH *CH;

    YM2203 *OPN = &(FMOPN[n]);

	/* do not support SSG part */
	if( r < 0x30 ){
		switch(r){
		case 0x21:	/* Test */
			break;
		case 0x22:	/* LFO FREQ (YM2612) */
			/* 3.98Hz,5.56Hz,6.02Hz,6.37Hz,6.88Hz,9.63Hz,48.1Hz,72.2Hz */
			break;
		case 0x24:	/* timer A High 8*/
			OPN->ST.TA = (OPN->ST.TA & 0x03)|(((int)v)<<2);
			break;
		case 0x25:	/* timer A Low 2*/
			OPN->ST.TA = (OPN->ST.TA & 0x3fc)|(v&3);
			break;
		case 0x26:	/* timer B */
			OPN->ST.TB = v;
			break;
		case 0x27:	/* mode , timer controll */
			/* b7 = CSM MODE */
			/* b6 = 3 slot mode */
			/* b5 = reset b */
			/* b4 = reset a */
			/* b3 = enable b */
			/* b2 = enable a */
			/* b1 = load b */
			/* b0 = load a */
			OPN->ST.mode = v;
#ifdef MAME_TIMER
			if( v & 0x20 )
			{
				OPN->ST.status &=0xfd; /* reset TIMER B */
#ifndef NICK_BUBLBOBL_CHANGE
				if (OPN->ST.timer_b_timer)
				{
					timer_remove (OPN->ST.timer_b_timer);
					OPN->ST.timer_b_timer = 0;
				}
#endif
			}
			if( v & 0x10 )
			{
				OPN->ST.status &=0xfe; /* reset TIMER A */
#ifndef NICK_BUBLBOBL_CHANGE
				if (OPN->ST.timer_a_timer)
				{
					timer_remove (OPN->ST.timer_a_timer);
					OPN->ST.timer_a_timer = 0;
				}
#endif
			}
			if( v & 0x02 )
			{
				if (OPN->ST.timer_b_timer == 0)
				{
					OPN->ST.TBC = ( 256-OPN->ST.TB)<<(4+12);
					/*if (OPN->ST.handler)*/
						OPN->ST.timer_b_timer = timer_set ((double)OPN->ST.TBC / ((double)OPN->ST.freqbase * (double)OPN->ST.rate), n, timer_callback_B_OPN);
				}
			}
#ifndef NICK_BUBLBOBL_CHANGE
			else
			{
				if (OPN->ST.timer_b_timer)
				{
					timer_remove (OPN->ST.timer_b_timer);
					OPN->ST.timer_b_timer = 0;
				}
			}
#endif
			if( v & 0x01 )
			{
				if (OPN->ST.timer_a_timer == 0)
				{
					OPN->ST.TAC = (1024-OPN->ST.TA)<<12;
					/*if (OPN->ST.handler)*/
						OPN->ST.timer_a_timer = timer_set ((double)OPN->ST.TAC / ((double)OPN->ST.freqbase * (double)OPN->ST.rate), n, timer_callback_A_OPN);
				}
			}
#ifdef NICK_BUBLBOBL_CHANGE
			else
			{
				if (OPN->ST.timer_a_timer)
				{
					timer_remove (OPN->ST.timer_a_timer);
					OPN->ST.timer_a_timer = 0;
				}
			}
#endif
#else /* MAME_TIMER */
			if( v & 0x20 )
			{
				OPN->ST.status &=0xfd; /* reset TIMER B */
				OPN->ST.TBC = 0;		/* timer stop */
			}
			if( v & 0x10 )
			{
				OPN->ST.status &=0xfe; /* reset TIMER A */
				OPN->ST.TAC = 0;		/* timer stop */
			}
			if( (v & 0x02) && !(OPN->ST.status&0x02) ) OPN->ST.TBC = ( 256-OPN->ST.TB)<<(4+12);
			if( (v & 0x01) && !(OPN->ST.status&0x01) ) OPN->ST.TAC = (1024-OPN->ST.TA)<<12;
#endif /* MAME_TIMER */

#ifndef S3_SUPPORT
			if(errorlog && v&0x40 ) fprintf(errorlog,"OPN 3SLOT mode selected (not supported)\n");
#endif
#ifndef CSM_SUPPORT
			if(errorlog && v&0x80 ) fprintf(errorlog,"OPN CSM mode selected (not supported)\n");
#endif
			break;
		case 0x28:	/* key on / off */
			c = v&0x03;
			if( c == 3 ) break;
#ifdef CSM_SUPPORT
			/* csm mode */
			if( c == 2 && (OPN->ST.mode & 0x80) ) break;
#endif
			CH = &OPN->CH[c];
			if(v&0x10) FM_KEYON(CH,SLOT1); else FM_KEYOFF(CH,SLOT1);
			if(v&0x20) FM_KEYON(CH,SLOT2); else FM_KEYOFF(CH,SLOT2);
			if(v&0x40) FM_KEYON(CH,SLOT3); else FM_KEYOFF(CH,SLOT3);
			if(v&0x80) FM_KEYON(CH,SLOT4); else FM_KEYOFF(CH,SLOT4);
/*			if(errorlog) fprintf(errorlog,"OPN %d:%d : KEY %02X\n",n,c,v&0xf0);*/
			break;
		case 0x2a:	/* DAC data (YM2612) */
			break;
		case 0x2b:	/* DAC Sel  (YM2612) */
			/* b7 = dac enable */
			break;
		case 0x2d:	/* divider sel */
			OPNSetPris( n, 6 , 4 ); /* OPN = 1/6 , SSG = 1/4 */
			break;
		case 0x2e:	/* divider sel */
			OPNSetPris( n, 3 , 2 ); /* OPN = 1/3 , SSG = 1/2 */
			break;
		case 0x2f:	/* divider sel */
			OPNSetPris( n, 2 , 1 ); /* OPN = 1/2 , SSG = 1/1 */
			break;
		}
		return;
	}
	/* 0x30 - 0xff */
	if( (c = OPN_CHAN(r)) ==3 ) return; /* 0xX3,0xX7,0xXB,0xXF */
	s  = OPN_SLOT(r);
	CH = &OPN->CH[c];
	switch( r & 0xf0 ) {
	case 0x30:	/* DET , MUL */
		CH->mul[s] = MUL_TABLE[v&0xf];
		CH->DT[s]  = OPN->ST.DT_TABLE[(v>>4)&7];
		CH->Incr[SLOT1]=-1;
		break;
	case 0x40:	/* TL */
		v &= 0x7f;
		v = (v<<7)|v; /* 7bit -> 14bit */
		CH->TL[s] = (v*EG_ENT)>>14;
#ifdef CSM_SUPPORT
		if( c == 2 && (OPN->ST.mode & 0x80) ) break;	/* CSM MODE */
#endif
		CH->TLL[s] = CH->TL[s] + KSL[CH->kcode];
		break;
	case 0x50:	/* KS, AR */
/*		if(errorlog) fprintf(errorlog,"OPN %d,%d,%d : AR %d\n",n,c,s,v&0x1f);*/
		CH->KSR[s]  = 3-(v>>6);
		CH->AR[s]   = (v&=0x1f) ? &OPN->ST.AR_TABLE[v<<1] : RATE_0;
		CH->evsa[s] = CH->AR[s][CH->ksr[s]];
		CH->Incr[SLOT1]=-1;
		break;
	case 0x60:	/*     DR */
/*		if(errorlog) fprintf(errorlog,"OPN %d,%d,%d : DR %d\n",n,c,s,v&0x1f);*/
		/* bit7 = AMS ENABLE(YM2612) */
		CH->DR[s] = (v&=0x1f) ? &OPN->ST.DR_TABLE[v<<1] : RATE_0;
		CH->evsd[s] = CH->DR[s][CH->ksr[s]];
		break;
	case 0x70:	/*     SR */
/*		if(errorlog) fprintf(errorlog,"OPN %d,%d,%d : SR %d\n",n,c,s,v&0x1f);*/
		CH->SR[s] = (v&=0x1f) ? &OPN->ST.DR_TABLE[v<<1] : RATE_0;
		CH->evss[s] = CH->SR[s][CH->ksr[s]];
		break;
	case 0x80:	/* SL, RR */
/*		if(errorlog) fprintf(errorlog,"OPN %d,%d,%d : SL %d RR %d \n",n,c,s,v>>4,v&0x0f);*/
		CH->SL[s] = SL_TABLE[(v>>4)];
		CH->RR[s] = (v&=0x0f) ? &OPN->ST.DR_TABLE[(v<<2)|2] : RATE_0;
		CH->evsr[s] = CH->RR[s][CH->ksr[s]];
		break;
	case 0x90:	/* SSG-EG */
#ifndef SEG_SUPPORT
		if(errorlog && (v&0x08)) fprintf(errorlog,"OPN %d,%d,%d :SSG-TYPE envelope selected (not supported )\n",n,c,s );
#endif
		CH->SEG[s] = v&0x0f;
		break;
	case 0xa0:
		switch(s){
		case 0:		/* 0xa0-0xa2 : FNUM1 */
			{
				unsigned int fn  = (((unsigned int)( (CH->fn_h)&7))<<8) + v;
				unsigned char blk = CH->fn_h>>3;
				/* make keyscale code */
				CH->kcode = (blk<<2)|OPN_FKTABLE[(fn>>7)];
				/* make basic increment counter 32bit = 1 cycle */
				CH->fc = OPN->FN_TABLE[fn]>>(7-blk);
				CH->Incr[SLOT1]=-1;
			}
			break;
		case 1:		/* 0xa4-0xa6 : FNUM2,BLK */
			CH->fn_h = v&0x3f;
			break;
		case 2:		/* 0xa8-0xaa : 3CH FNUM1 */
			{
				unsigned int fn  = (((unsigned int)(OPN->fn3_h[c]&7))<<8) + v;
				unsigned char blk = OPN->fn3_h[c]>>3;
				/* make keyscale code */
				OPN->kcode3[c]= (blk<<2)|OPN_FKTABLE[(fn>>7)];
				/* make basic increment counter 32bit = 1 cycle */
				OPN->fc3[c] = OPN->FN_TABLE[fn]>>(7-blk);
				OPN->CH[2].Incr[SLOT1]=-1;
			}
			break;
		case 3:		/* 0xac-0xae : 3CH FNUM2,BLK */
			OPN->fn3_h[c] = v&0x3f;
			break;
		}
		break;
	case 0xb0:
		switch(s){
		case 0:		/* 0xb0-0xb2 : FB,ALGO */
			set_algorythm( CH , v );
/*			if(errorlog) fprintf(errorlog,"OPN %d,%d : AL %d FB %d\n",n,c,v&7,(v>>3)&7);*/
			break;
		}
		break;
	case 0xb4:		/* LR , AMS , PMS (YM2612) */
		/* b0-2 PMS */
		/* 0,3.4,6.7,10,14,20,40,80(cent) */
		/* b4-5 AMS */
		/* 0,1.4,5.9,11.8(dB) */
		/* b0 = right , b1 = left */
		CH->PAN = (v>>6)&0x03;
    }
}

/* ---------- read status port ---------- */
unsigned char OPNReadStatus(int n)
{
	return FMOPN[n].ST.status;
}

/* ---------- make digital sound data ---------- */
/* ---------- update one of chip ----------- */
void OPNUpdateOne(int num, int endp)
{
    YM2203 *OPN = &(FMOPN[num]);
    int i;
	int opn_data;
	FMSAMPLE *buffer = OPN->Buf;

	State = &OPN->ST;
	chA   = &OPN->CH[0];
	chB   = &OPN->CH[1];
	chC   = &OPN->CH[2];

	/* frequency counter channel A */
	CALC_FCOUNT( chA );
	/* frequency counter channel B */
	CALC_FCOUNT( chB );
	/* frequency counter channel C */
#ifdef S3_SUPPORT
	if( (State->mode & 0x40) ){
		if( chC->Incr[SLOT1]==-1){
			/* 3 slot mode */
			CALC_FCSLOT(chC , SLOT1 , OPN->fc3[1] , OPN->kcode3[1] );
			CALC_FCSLOT(chC , SLOT2 , OPN->fc3[2] , OPN->kcode3[2] );
			CALC_FCSLOT(chC , SLOT3 , OPN->fc3[0] , OPN->kcode3[0] );
			CALC_FCSLOT(chC , SLOT4 , chC->fc , chC->kcode );
		}
	}else{
		CALC_FCOUNT( chC );
	}
#else
	CALC_FCOUNT( chC );
#endif

    for( i=State->bufp; i < endp ; ++i )
	{
		/*            channel A         channel B         channel C      */
		opn_data  = FM_CALC_CH(chA) + FM_CALC_CH(chB) + FM_CALC_CH(chC);
		/* limit check */
		if( opn_data > OPN_MAXOUT )      opn_data = OPN_MAXOUT;
		else if( opn_data < OPN_MINOUT ) opn_data = OPN_MINOUT;
		/* store to sound buffer */
		if( sample_16bit ) ((unsigned short *)buffer)[i] = opn_data >> OPN_OUTSB;
		else           ((unsigned char  *)buffer)[i] = opn_data >> OPN_OUTSB_8;
#ifndef MAME_TIMER
		/* timer controll */
		CALC_TIMER_A( State , chC );
#endif
	}
#ifndef MAME_TIMER
	CALC_TIMER_B( State , endp-State->bufp );
#endif
    State->bufp = endp;
}

/* ---------- update all chips ----------- */
void OPNUpdate(void)
{
    int i;

    for ( i = 0 ; i < FMNumChips; i++ ) {
		if( FMOPN[i].ST.bufp <  FMBufSize ) OPNUpdateOne(i , FMBufSize );
		FMOPN[i].ST.bufp = 0;
    }
}

/* ---------- return the buffer ---------- */
FMSAMPLE *OPNBuffer(int n)
{
    return FMOPN[n].Buf;
}
/* ---------- set buffer ---------- */
int OPNSetBuffer(int n, FMSAMPLE *buf )
{
	if( buf == 0 ) return -1;
    FMOPN[n].Buf = buf;
	return 0;
}
/* ---------- set interrupt handler --------- */
void OPNSetIrqHandler(int n, void (*handler)(void) )
{
	FMOPN[n].ST.handler = handler;
}










/* -------------------------- OPM ---------------------------------- */
#undef SEG_SUPPORT		/* OPM has not SEG type envelope */

/* ---------- priscaler set(and make time tables) ---------- */
void OPMInitTable( int num )
{
    YM2151 *OPM = &(FMOPM[num]);
	int i;
	float pom;
	float rate = (float)(1<<FREQ_BITS) / (3579545.0 / FMOPM[num].ST.clock * FMOPM[num].ST.rate);

	for (i=0; i<8*12*64+950; i++)
	{
		/* This calculation type was used from the Jarek's YM2151 emulator */
		pom = 6.875 * pow (2, ((i+4*64)*1.5625/1200.0) ); /*13.75Hz is note A 12semitones below A-0, so D#0 is 4 semitones above then*/
		/*calculate phase increment for above precounted Hertz value*/
		OPM->KC_TABLE[i] = (unsigned int)(pom * rate);
		/*if(errorlog) fprintf(errorlog,"OPM KC %d = %x\n",i,OPM->KC_TABLE[i]);*/
	}

	/* make time tables */
	init_timetables( &OPM->ST , OPM_DTTABLE , OPM_ARRATE , OPM_DRRATE );
}

/* ---------- reset one of chip ---------- */
void OPMResetChip(int num)
{
	int i;
    YM2151 *OPM = &(FMOPM[num]);

	OPMInitTable( num );
	reset_channel( &OPM->ST , &OPM->CH[0] , 8 );
	/* status clear */
	OPMWriteReg(num,0x1b,0x00);
	/* reset OPerator paramater */
	for(i = 0xff ; i >= 0x20 ; i-- ) OPMWriteReg(num,i,0);
}
/* ---------- release storage for a chip ---------- */
static void _OPMFreeChip(int num)
{
	int i;

	for( i = 0 ; i < YM2151_NUMBUF ; i++){
	    FMOPM[num].Buf[i] = NULL;
	}
}

/* ----------  Initialize YM2151 emulator(s) ----------    */
/* 'num' is the number of virtual YM2151's to allocate     */
/* 'rate' is sampling rate and 'bufsiz' is the size of the */
/* buffer that should be updated at each interval          */
int OPMInit(int num, int clock, int rate, int bitsize, int bufsiz , FMSAMPLE **buffer )
{
    int i,j;

    if (FMOPM) return (-1);	/* duplicate init. */

    FMNumChips = num;
    FMBufSize = bufsiz;
    if( bitsize == 16 ) sample_16bit = 1;
    else                sample_16bit = 0;

	/* allocate ym2151 state space */
	if( (FMOPM = (YM2151 *)malloc(sizeof(YM2151) * FMNumChips))==NULL)
		return (-1);
	/* allocate total lebel table (128kb space) */
	if( !FMInitTable() )
	{
		free( FMOPM );
		return (-1);
	}
	for ( i = 0 ; i < FMNumChips; i++ ) {
		FMOPM[i].ST.clock = clock;
		FMOPM[i].ST.rate = rate;
		if( rate ) FMOPM[i].ST.freqbase = ((float)clock * 4096.0 / rate) / 64;
		else       FMOPM[i].ST.freqbase = 0;
		for ( j = 0 ; j < YM2151_NUMBUF ; j ++ ) {
			FMOPM[i].Buf[j] = 0;
		}
		if( OPMSetBuffer(i,&buffer[i*YM2151_NUMBUF]) < 0 ){
		    for ( j = 0 ; j < i ; j ++ ) {
				_OPMFreeChip(j);
		    }
		    return(-1);
		}
		FMOPM[i].ST.handler = 0;
		OPMResetChip(i);
    }
    return(0);
}

/* ---------- shut down emurator ----------- */
void OPMShutdown()
{
    int i;

    if (!FMOPM) return;
    for ( i = 0 ; i < FMNumChips ; i++ ) {
		_OPMFreeChip(i);
	}
	free(FMOPM); FMOPM = NULL;
	FMCloseTable();
	FMBufSize = 0;
}
/* ---------- write a register on YM2151 chip number 'n' ---------- */
void OPMWriteReg(int n, int r, int v)
{
	unsigned char c,s;
	FM_CH *CH;

    YM2151 *OPM = &(FMOPM[n]);

	c  = OPM_CHAN(r);
	CH = &OPM->CH[c];
	s  = OPM_SLOT(r);

	switch( r & 0xe0 ){
	case 0x00:
		switch( r ){
		case 0x01:	/* test */
			break;
		case 0x08:	/* key on / off */
			c = v&7;
			/* csm mode */
#ifdef CSM_SUPPORT
			if( c == 7 && (OPM->ST.mode & 0x80) ) break;
#endif
			CH = &OPM->CH[c];
			if(v&0x08) FM_KEYON(CH,SLOT1); else FM_KEYOFF(CH,SLOT1);
			if(v&0x10) FM_KEYON(CH,SLOT2); else FM_KEYOFF(CH,SLOT2);
			if(v&0x20) FM_KEYON(CH,SLOT3); else FM_KEYOFF(CH,SLOT3);
			if(v&0x40) FM_KEYON(CH,SLOT4); else FM_KEYOFF(CH,SLOT4);
			break;
		case 0x0f:	/* Noise freq (ch7.op4) */
			/* b7 = Noise enable */
			/* b0-4 noise freq  */
			if( v & 0x80 ){
				/* !!!!! do not supported noise mode  !!!!! */
				if(errorlog) fprintf(errorlog,"OPM Noise mode sel ( not supported )\n");
			}
			OPM->NReg = v & 0x8f;
			break;
		case 0x10:	/* timer A High 8*/
			OPM->ST.TA = (OPM->ST.TA & 0x03)|(((int)v)<<2);
			break;
		case 0x11:	/* timer A Low 2*/
			OPM->ST.TA = (OPM->ST.TA & 0x3fc)|(v&3);
			break;
		case 0x12:	/* timer B */
			OPM->ST.TB = v;
			break;
		case 0x14:	/* mode , timer controll */
			/* b7 = CSM MODE */
			/* b5 = reset b */
			/* b4 = reset a */
			/* b3 = irq enable b */
			/* b2 = irq enable a */
			/* b1 = load b */
			/* b0 = load a */
			OPM->ST.mode = v % 0xbf;	/* 3slot = off */
#ifdef MAME_TIMER
			if( v & 0x20 )
			{
				OPM->ST.status &=0xfd; /* reset TIMER B */
				if (OPM->ST.timer_b_timer)
				{
					timer_remove (OPM->ST.timer_b_timer);
					OPM->ST.timer_b_timer = 0;
				}
			}
			if( v & 0x10 )
			{
				OPM->ST.status &=0xfe; /* reset TIMER A */
				if (OPM->ST.timer_a_timer)
				{
					timer_remove (OPM->ST.timer_a_timer);
					OPM->ST.timer_a_timer = 0;
				}
			}
			if( v & 0x02 )
			{
				if (OPM->ST.timer_b_timer == 0)
				{
					OPM->ST.TBC = ( 256-OPM->ST.TB)<<(4+12);
					/*if (OPM->ST.handler)*/
						OPM->ST.timer_b_timer = timer_set ((double)OPM->ST.TBC / ((double)OPM->ST.freqbase * (double)OPM->ST.rate), n, timer_callback_B_OPM);
				}
			}
			if( v & 0x01 )
			{
				if (OPM->ST.timer_a_timer == 0)
				{
					OPM->ST.TAC = (1024-OPM->ST.TA)<<12;
					/*if (OPM->ST.handler)*/
						OPM->ST.timer_a_timer = timer_set ((double)OPM->ST.TAC / ((double)OPM->ST.freqbase * (double)OPM->ST.rate), n, timer_callback_A_OPM);
				}
			}
#else /* MAME_TIMER */
			if( v & 0x20 )
			{
				OPM->ST.status &=0xfd; /* reset TIMER B */
				OPM->ST.TBC = 0;
			}
			if( v & 0x10 )
			{
				OPM->ST.status &=0xfe; /* reset TIMER A */
				OPM->ST.TAC = 0;
			}
			if( (v & 0x02) && !(OPM->ST.status&0x02) ) OPM->ST.TBC = ( 256-OPM->ST.TB)<<(4+12);
			if( (v & 0x01) && !(OPM->ST.status&0x01) ) OPM->ST.TAC = (1024-OPM->ST.TA)<<12;
#endif /* MAME_TIMER */

#ifndef CSM_SUPPORT
			if(errorlog && v&0x80 ) fprintf(errorlog,"OPM CSM select ( not supported )\n");
#endif
			break;
		case 0x18:	/* lfreq   */
			/* !!!!! picjup lfo frequency table !!!!! */
			break;
		case 0x19:	/* PMD/AMD */
			if( v & 0x80 ) OPM->pmd = v & 0x7f;
			else           OPM->amd = v & 0x7f;
			break;
		case 0x1b:	/* CT , W  */
			/* b7 = CT */
			/* b6 = CT */
			/* b0-3 = wave form(LFO) 0=nokogiri,1=houkei,2=sankaku,3=noise */
			break;
		}
		break;
	case 0x20:	/* 20-3f */
		switch( s ){
		case 0: /* 0x20-0x27 : RL,FB,CON */
			set_algorythm( CH , v );
			/* b0 = left , b1 = right */
			CH->PAN = ((v>>6)&0x03);
			if( cur_chip == OPM ) pan[c] = &outd[CH->PAN];
			break;
		case 1: /* 0x28-0x2f : Keycode */
			{
				int blk = (v>>4)&7;
				/* make keyscale code */
				CH->kcode = (v>>2)&0x1f;
				/* make basic increment counter 22bit = 1 cycle */
				CH->fc = (blk * (12*64)) + KC_TO_SEMITONE[v&0x0f] + CH->fn_h;
				CH->Incr[SLOT1]=-1;
			}
			break;
		case 2: /* 0x30-0x37 : Keyfunction */
			CH->fc -= CH->fn_h;
			CH->fn_h = v>>2;
			CH->fc += CH->fn_h;
			CH->Incr[SLOT1]=-1;
			break;
		case 3: /* 0x38-0x3f : PMS / AMS */
			/* b0-1 AMS */
			/* AMS * 23.90625db */
			/* b4-6 PMS */
			/* 0,5,10,20,50,100,400,700 (cent) */
			break;
		}
		break;
	case 0x40:	/* DT1,MUL */
		CH->mul[s] = MUL_TABLE[v&0x0f];
		CH->DT[s]  = OPM->ST.DT_TABLE[(v>>4)&7];
		CH->Incr[SLOT1]=-1;
		break;
	case 0x60:	/* TL */
		v &= 0x7f;
		v = (v<<7)|v; /* 7bit -> 14bit */
		CH->TL[s] = (v*EG_ENT)>>14;
#ifdef CSM_SUPPORT
		if( c == 7 && (OPM->ST.mode & 0x80) ) break;	/* CSM MODE */
#endif
		CH->TLL[s] = CH->TL[s] + KSL[CH->kcode];
		break;
	case 0x80:	/* KS, AR */
		CH->KSR[s] = 3-(v>>6);
		CH->AR[s] = (v&=0x1f) ? &OPM->ST.AR_TABLE[v<<1] : RATE_0;
		CH->evsa[s] = CH->AR[s][CH->ksr[s]];
		CH->Incr[SLOT1]=-1;
		break;
	case 0xa0:	/* AMS EN,D1R */
		/* bit7 = AMS ENABLE */
		CH->DR[s] = (v&=0x1f) ? &OPM->ST.DR_TABLE[v<<1] : RATE_0 ;
		CH->evsd[s] = CH->DR[s][CH->ksr[s]];
		break;
	case 0xc0:	/* DT2 ,D2R */
		CH->DT2[s]  = DT2_TABLE[v>>6];
		CH->SR[s]  = (v&=0x1f) ? &OPM->ST.DR_TABLE[v<<1] : RATE_0 ;
		CH->evss[s] = CH->SR[s][CH->ksr[s]];
		CH->Incr[SLOT1]=-1;
		break;
	case 0xe0:	/* D1L, RR */
		CH->SL[s] = SL_TABLE[(v>>4)];
		/* recalrate */
		CH->RR[s] = (v&=0x0f) ? &OPM->ST.DR_TABLE[(v<<2)|2] : RATE_0 ;
		CH->evsr[s] = CH->RR[s][CH->ksr[s]];
		break;
    }
}

/* ---------- read status port ---------- */
unsigned char OPMReadStatus(int n)
{
	return FMOPM[n].ST.status;
}

/* ---------- make digital sound data ---------- */
void OPMUpdateOne(int num , int endp)
{
	YM2151 *OPM = &(FMOPM[num]);
	static FMSAMPLE  *bufL,*bufR;
	int i;
	int dataR,dataL;
	if( (void *)OPM != cur_chip ){
		cur_chip = (void *)OPM;

		State = &OPM->ST;
		/* set bufer */
		bufL = OPM->Buf[0];
		bufR = OPM->Buf[1];
		/* channel pointer */
		chA = &OPM->CH[0];
		chB = &OPM->CH[1];
		chC = &OPM->CH[2];
		chD = &OPM->CH[3];
		chE = &OPM->CH[4];
		chF = &OPM->CH[5];
		chG = &OPM->CH[6];
		chH = &OPM->CH[7];
		pan[0] = &outd[chA->PAN];
		pan[1] = &outd[chB->PAN];
		pan[2] = &outd[chC->PAN];
		pan[3] = &outd[chD->PAN];
		pan[4] = &outd[chE->PAN];
		pan[5] = &outd[chF->PAN];
		pan[6] = &outd[chG->PAN];
		pan[7] = &outd[chH->PAN];
	}
	OPM_CALC_FCOUNT( OPM , chA );
	OPM_CALC_FCOUNT( OPM , chB );
	OPM_CALC_FCOUNT( OPM , chC );
	OPM_CALC_FCOUNT( OPM , chD );
	OPM_CALC_FCOUNT( OPM , chE );
	OPM_CALC_FCOUNT( OPM , chF );
	OPM_CALC_FCOUNT( OPM , chG );
	OPM_CALC_FCOUNT( OPM , chH );

    for( i=State->bufp; i < endp ; ++i )
	{
		/* clear output acc. */
		outd[OPM_LEFT] = outd[OPM_RIGHT]= outd[OPM_CENTER] = 0;
		/* calcrate channel output */
		*pan[0] += FM_CALC_CH( chA );
		*pan[1] += FM_CALC_CH( chB );
		*pan[2] += FM_CALC_CH( chC );
		*pan[3] += FM_CALC_CH( chD );
		*pan[4] += FM_CALC_CH( chE );
		*pan[5] += FM_CALC_CH( chF );
		*pan[6] += FM_CALC_CH( chG );
		*pan[7] += FM_CALC_CH( chH );
		/* get left & right output */
		dataL = outd[OPM_CENTER] + outd[OPM_LEFT];
		dataR = outd[OPM_CENTER] + outd[OPM_RIGHT];
		/* clipping data */
		if( dataL > OPM_MAXOUT ) dataL = OPM_MAXOUT;
		else if( dataL < OPM_MINOUT ) dataL = OPM_MINOUT;
		if( dataR > OPM_MAXOUT ) dataR = OPM_MAXOUT;
		else if( dataR < OPM_MINOUT ) dataR = OPM_MINOUT;

		if( sample_16bit )
		{
#ifdef OPM_STEREO		/* stereo mixing */
			/* stereo mix */
			((unsigned long *)bufL)[i] = ((dataL>>OPM_OUTSB)<<16)(dataR>>OPM_OUTSB);
#else
			/* stereo separate */
			((unsigned short *)bufL)[i] = dataL>>OPM_OUTSB;
			((unsigned short *)bufR)[i] = dataR>>OPM_OUTSB;
#endif
		}
		else
		{
#ifdef OPM_STEREO		/* stereo mixing */
			/* stereo mix */
			((unsigned shart *)bufL)[i] = ((dataL>>OPM_OUTSB_8)<<8)(dataR>>OPM_OUTSB_8);
#else
			/* stereo separate */
			((unsigned char *)bufL)[i] = dataL>>OPM_OUTSB_8;
			((unsigned char *)bufR)[i] = dataR>>OPM_OUTSB_8;
#endif
		}
#ifdef LFO_SUPPORT
		CALC_LOPM_LFO;
#endif

#ifndef MAME_TIMER
		CALC_TIMER_A( State , chH );
#endif
    }
#ifndef MAME_TIMER
	CALC_TIMER_B( State , endp-State->bufp );
#endif
    State->bufp = endp;
}

/* ---------- update all chips ----------- */
void OPMUpdate(void)
{
    int i;

    for ( i = 0 ; i < FMNumChips; i++ ) {
		if( FMOPM[i].ST.bufp <  FMBufSize ) OPMUpdateOne(i , FMBufSize );
		FMOPM[i].ST.bufp = 0;
    }
}

/* ---------- return the buffer ---------- */
FMSAMPLE *OPMBuffer(int n,int c)
{
    return FMOPM[n].Buf[c];
}
/* ---------- set buffer ---------- */
int OPMSetBuffer(int n, FMSAMPLE **buf )
{
	int i;
	for( i = 0 ; i < YM2151_NUMBUF ; i++){
		if( buf[i] == 0 ) return -1;
	    FMOPM[n].Buf[i] = buf[i];
		if( cur_chip == &FMOPM[n] ) cur_chip = NULL;
	}
	return 0;
}
/* ---------- set interrupt handler --------- */
void OPMSetIrqHandler(int n, void (*handler)(void) )
{
	FMOPM[n].ST.handler = handler;
}
