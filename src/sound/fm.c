#define YM2610B_WARNING

/* YM2608 rhythm data is PCM ,not an ADPCM */
#define YM2608_RHYTHM_PCM

/*
**
** File: fm.c -- software implementation of FM sound generator
**
** Copyright (C) 1998 Tatsuyuki Satoh , MultiArcadeMachineEmurator development
**
** Version 0.35f
**
*/

/*
**** change log. (hiro-shi) ****
** 08-12-98:
** rename ADPCMA -> ADPCMB, ADPCMB -> ADPCMA
** move ROM limit check.(CALC_CH? -> 2610Write1/2)
** test program (ADPCMB_TEST)
** move ADPCM A/B end check.
** ADPCMB repeat flag(no check)
** change ADPCM volume rate (8->16) (32->48).
**
** 09-12-98:
** change ADPCM volume. (8->16, 48->64)
** replace ym2610 ch0/3 (YM-2610B)
** init cur_chip (restart bug fix)
** change ADPCM_SHIFT (10->8) missing bank change 0x4000-0xffff.
** add ADPCM_SHIFT_MASK
** change ADPCMA_DECODE_MIN/MAX.
*/

/*
    no check:
		YM2608 rhythm sound
		OPN  SSG type envelope
		YM2612 DAC output mode
		YM2151 CSM speech mode
	no support:
		status busy flag (already not busy)
		LFO contoller (YM2612/YM2610/YM2608/YM2151)
		YM2151 noise mode
		YM2608 DELTA-T-ADPCM and RYTHM
		YM2610 DELTA-T-ADPCM with PCM port
		YM2610 PCM memory data access

		YM2608 status mask (register :0x110)
	preliminary :
		key scale level rate (?)
		attack rate time rate , curve (?)
		decay  rate time rate , curve (?)
		self feedback calcration
		YM2610 ADPCM mixing level
	Problem :

	note:
                        OPN                           OPM
		fnum          fMus * 2^20 / (fM/(12*n))
		TimerOverA    (12*n)*(1024-NA)/fFM        64*(1024-Na)/fm
		TimerOverB    (12*n)*(256-NB)/fFM       1024*(256-Nb)/fm
		output bits   10bit<<3bit               16bit * 2ch (YM3012=10bit<<3bit)
		sampling rate fFM / (12*6) ?            fFM / 64
		lfo freq                                ( fM*2^(LFRQ/16) ) / (4295*10^6)
*/

/************************************************************************/
/*    comment of hiro-shi(Hiromitsu Shioya)                             */
/*    YM2610(B) = (OPN-B                                                */
/*    YM2610  : PSG:3ch FM:4ch ADPCM(18.5KHz):6ch DeltaT ADPCM:1ch      */
/*    YM2610B : PSG:3ch FM:6ch ADPCM(18.5KHz):6ch DeltaT ADPCM:1ch      */
/************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#ifndef __RAINE__
#include "driver.h"		/* use M.A.M.E. */
#else
#include "deftypes.h"		/* use RAINE */
#include "support.h"		/* use RAINE */
#endif
#include "ay8910.h"
#include "fm.h"

#ifndef PI
#define PI 3.14159265358979323846
#endif

/***** shared function building option ****/
#define BUILD_OPN (BUILD_YM2203||BUILD_YM2608||BUILD_YM2610||BUILD_YM2612)
#define BUILD_OPNB (BUILD_YM2610||BUILD_YM2610B)
#define BUILD_FM_ADPCMA (BUILD_YM2608||BUILD_YM2610)
#define BUILD_FM_ADPCMB (BUILD_YM2608||BUILD_YM2610)

/**** YM2610 ADPCM defines ****/
#define ADPCMA_VOLUME_RATE  (1)
#define ADPCMB_VOLUME_RATE  (2) /* DELTA-T volume rate */

#define ADPCM_SHIFT    (16)

#define AUDIO_CONV(A) ((A))
#define AUDIO_CONV16(A) ((A))

/* ------------------------------------------------------------------ */
#ifdef __RAINE__
#define INTERNAL_TIMER 			/* use internal timer */
#endif
/* -------------------- speed up optimize switch -------------------- */
/* ---------- Enable ---------- */
#define TL_SAVE_MEM		/* save some memories for total level */
/* ---------- Disable ---------- */
#if 0
#define SEG_SUPPORT		   	/* OPN SSG type envelope support   */
#define LFO_SUPPORT			/* LFO support                     */
#endif
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

/* final output shift , limit minimum and maximum */
#define OPN_OUTSB   (TL_BITS+2-16)		/* OPN output final shift 16bit */
#define OPN_MAXOUT (0x7fff<<OPN_OUTSB)
#define OPN_MINOUT (-0x8000<<OPN_OUTSB)

#define OPM_OUTSB   (TL_BITS+2-16) 		/* OPM output final shift 16bit */
#define OPM_MAXOUT (0x7fff<<OPM_OUTSB)
#define OPM_MINOUT (-0x8000<<OPM_OUTSB)

#define OPNB_OUTSB   (TL_BITS+2-16)		/* OPN output final shift 16bit */
#define OPNB_MAXOUT (0x7fff<<OPNB_OUTSB)
#define OPNB_MINOUT (-0x8000<<OPNB_OUTSB)

/* -------------------- quality selection --------------------- */

/* sinwave entries */
/* used static memory = SIN_ENT * 4 (byte) */
#define SIN_ENT 2048

/* output level entries (envelope,sinwave) */
/* envelope counter lower bits */
#define ENV_BITS 16
/* envelope output entries */
#define EG_ENT   4096
/* used dynamic memory = EG_ENT*4*4(byte)or EG_ENT*6*4(byte) */
/* used static  memory = EG_ENT*4 (byte)                     */

#ifdef SEG_SUPPORT
#define EG_OFF   ((3*EG_ENT)<<ENV_BITS)  /* OFF          */
#define EG_UED   EG_OFF
#define EG_UST   ((2*EG_ENT)<<ENV_BITS)  /* UPSISE START */
#define EG_DED   EG_UST
#else
#define EG_OFF   ((2*EG_ENT)<<ENV_BITS)  /* OFF          */
#define EG_DED   EG_OFF
#endif
#define EG_DST   (EG_ENT<<ENV_BITS)      /* DECAY  START */
#define EG_AED   EG_DST
#define EG_AST   0                       /* ATTACK START */

#define EG_STEP (96.0/EG_ENT) /* OPL is 0.1875 dB step  */

/* LFO table entries */
#define LFO_ENT 512

/* -------------------- local defines , macros --------------------- */
/* number of maximum envelope counter */
/* #define ENV_OFF ((EG_ENT<<ENV_BITS)-1) */

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
#define ENV_MOD_OFF 0x00
#define ENV_MOD_RR  0x01
#define ENV_MOD_SR  0x02
#define ENV_MOD_DR  0x03
#define ENV_MOD_AR  0x04
#define ENV_SSG_SR  0x05
#define ENV_SSG_DR  0x06
#define ENV_SSG_AR  0x07

/* bit0 = right enable , bit1 = left enable (FOR YM2612) */
#define OPN_RIGHT  1
#define OPN_LEFT   2
#define OPN_CENTER 3

/* bit0 = left enable , bit1 = right enable */
#define OPM_LEFT   1
#define OPM_RIGHT  2
#define OPM_CENTER 3
/* */

/* YM2608 Rhythm Number */
#define RY_BD  0
#define RY_SD  1
#define RY_TOP 2
#define RY_HH  3
#define RY_TOM 4
#define RY_RIM 5

/* FM timer model */
#define FM_TIMER_SINGLE (0)
#define FM_TIMER_INTERVAL (1)

/* ---------- OPN / OPM one channel  ---------- */
typedef struct fm_slot {
	int *DT;				/* detune          :DT_TABLE[DT]       */
	int DT2;				/* multiple,Detune2:(DT2<<4)|ML for OPM*/
	int TL;					/* total level     :TL << 8            */
	signed int TLL;			/* adjusted now TL                     */
	unsigned char KSR;		/* key scale rate  :3-KSR              */
	int *AR;				/* attack rate     :&AR_TABLE[AR<<1]   */
	int *DR;				/* decay rate      :&DR_TALBE[DR<<1]   */
	int *SR;				/* sustin rate     :&DR_TABLE[SR<<1]   */
	int  SL;				/* sustin level    :SL_TALBE[SL]       */
	int *RR;				/* release rate    :&DR_TABLE[RR<<2+2] */
	unsigned char SEG;		/* SSG EG type     :SSGEG              */
	unsigned char ksr;		/* key scale rate  :kcode>>(3-KSR)     */
	unsigned int mul;		/* multiple        :ML_TABLE[ML]       */
	unsigned int Cnt;		/* frequency count :                   */
	unsigned int Incr;		/* frequency step  :                   */
	/* envelope generator state */
	unsigned char evm;		/* envelope phase                      */
	signed int evc;			/* envelope counter                    */
	signed int eve;			/* envelope counter end point          */
	signed int evs;			/* envelope counter step               */
	signed int evsa;		/* envelope step for AR                */
	signed int evsd;		/* envelope step for DR                */
	signed int evss;		/* envelope step for SR                */
	signed int evsr;		/* envelope step for RR                */
	/* LFO */
	unsigned char ams;
	unsigned char pms;
}FM_SLOT;


typedef struct fm_chan {
	FM_SLOT	SLOT[4];
	unsigned char PAN;			/* PAN NONE,LEFT,RIGHT or CENTER       */
	unsigned char ALGO;			/* algorythm                           */
	unsigned char FB;			/* feed back       :&FB_TABLE[FB<<8]   */
	int op1_out;				/* op1 output foe beedback             */
	/* algorythm state */
	int *connect1;				/* operator 1 connection pointer       */
	int *connect2;				/* operator 2 connection pointer       */
	int *connect3;				/* operator 3 connection pointer       */
	int *connect4;				/* operator 4 connection pointer       */
	/* phase generator state */
	unsigned int  fc;			/* fnum,blk        :calcrated          */
	unsigned char fn_h;			/* freq latch      :                   */
	unsigned char kcode;		/* key code        :                   */
} FM_CH;

/* OPN/OPM common state */
typedef struct fm_state {
	unsigned char index;		/* chip index (number of chip) */
	int clock;					/* master clock  (Hz)  */
	int rate;					/* sampling rate (Hz)  */
	int freqbase;				/* frequency base      */
	double TimerBase;			/* Timer base time     */
	unsigned char address;		/* address register    */
	unsigned char irq;			/* interrupt level     */
	unsigned char irqmask;		/* irq mask            */
	unsigned char status;		/* status flag         */
	unsigned int mode;			/* mode  CSM / 3SLOT   */
	int TA;						/* timer a             */
	int TAC;					/* timer a counter     */
	unsigned char TB;			/* timer b             */
	int TBC;					/* timer b counter     */
	/* speedup customize */
	/* time tables */
	signed int DT_TABLE[8][32];     /* detune tables       */
	signed int AR_TABLE[94];	/* atttack rate tables */
	signed int DR_TABLE[94];	/* decay rate tables   */
	/* LFO */
	unsigned int LFOCnt;
	unsigned int LFOIncr;
	/* Extention Timer and IRQ handler */
	FM_TIMERHANDLER Timer_Handler;
	FM_IRQHANDLER   IRQ_Handler;
	/* timer model single / interval */
	unsigned char timermodel;
}FM_ST;

/* OPN 3slot struct */
typedef struct opn_3slot {
	unsigned int  fc[3];		/* fnum3,blk3  :calcrated */
	unsigned char fn_h[3];		/* freq3 latch            */
	unsigned char kcode[3];		/* key code    :          */
}FM_3SLOT;

/* adpcm type A and type B struct */
typedef struct adpcm_state {
  unsigned char flag;		/* port state */
  unsigned char flagMask;	/* arrived    */
  unsigned char now_data;
  unsigned int now_addr;
  unsigned int now_step;
  unsigned int step;
  unsigned int start;
  unsigned int end;
  unsigned int delta;
  int IL;
  int volume;
  int *pan;     /* &outd[OPN_xxxx] */
  int /*adpcmm,*/ adpcmx, adpcmd;
  int adpcml;			/* hiro-shi!! */

  /* leveling and re-sampling state for DELTA-T */
  int volume_w_step;   /* volume with step rate */
  int next_leveling;   /* leveling value        */
  int sample_step;     /* step of re-sampling   */
}ADPCM_CH;

/* OPN/A/B common state */
typedef struct opn_f {
	unsigned char type;		/* chip type         */
	FM_ST ST;				/* general state     */
	FM_3SLOT SL3;			/* 3 slot mode state */
	FM_CH *P_CH;			/* pointer of CH     */
	unsigned int FN_TABLE[2048]; /* fnumber -> increment counter */
} FM_OPN;

/* here's the virtual YM2203(OPN) (Used by YM2608 / YM2612)  */
typedef struct ym2203_f {
	FM_OPN OPN;				/* OPN state         */
/*	FMSAMPLE *Buf;*/			/* sound buffer      */
	FM_CH CH[3];			/* channel state     */
} YM2203;

/* here's the virtual YM2610 */
typedef struct ym2610_f {
	FM_OPN OPN;						/* OPN state    */
/*	FMSAMPLE *Buf[YM2610_NUMBUF];*/	/* sound buffer */
	FM_CH CH[6];					/* channel state */
	int address1;	/* address register1 */
	/**** ADPCM control ****/
	char *pcmbuf[2];
	unsigned int pcm_size[2];
	int *TL_adpcmb;
	ADPCM_CH adpcm[7];			/* normal ADPCM & deltaT ADPCM */
	unsigned int adpcmreg[2][0x30];
	int port0state, port0control, port0shift;
	int port1state, port1control, port1shift;
	unsigned char adpcm_arrivedEndAddress,adpcm_statusmask;
} YM2610;

/* here's the virtual YM2608 */
typedef YM2610 YM2608;

/* here's the virtual YM2612 */
typedef struct ym2612_f {
	FM_OPN OPN;						/* OPN state       */
/*	FMSAMPLE *Buf[YM2612_NUMBUF];*/	/* sound buffer */
	FM_CH CH[6];					/* channel state */
	int address1;	/* address register1 */
	/* dac output (YM2612) */
	int dacen;
	int dacout;
} YM2612;

/* here's the virtual YM2151(OPM)  */
typedef struct ym2151_f {
/*	FMSAMPLE *Buf[YM2151_NUMBUF];*//* sound buffers    */
	FM_ST ST;					/* general state     */
	FM_CH CH[8];				/* channel state     */
	unsigned char NReg;			/* noise enable,freq */
	unsigned char pmd;			/* LFO pmd level     */
	unsigned char amd;			/* LFO amd level     */
	unsigned char ctw;			/* CT0,1 and waveform */
	unsigned int KC_TABLE[8*12*64+950];/* keycode,keyfunction -> count */
	void (*PortWrite)(int offset,int data);/*  callback when write CT0/CT1 */
} YM2151;

/* -------------------- tables --------------------- */

/* key scale level */
/* !!!!! preliminary !!!!! */

#define DV (1/EG_STEP)
static const unsigned char KSL[32]=
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
static const char OPN_FKTABLE[16]={0,0,0,0,0,0,0,1,2,3,3,3,3,3,3,3};

static const int KC_TO_SEMITONE[16]={
	/*translate note code KC into more usable number of semitone*/
	0*64, 1*64, 2*64, 3*64,
	3*64, 4*64, 5*64, 6*64,
	6*64, 7*64, 8*64, 9*64,
	9*64,10*64,11*64,12*64
};

static const int DT2_TABLE[4]={ /* 4 DT2 values */
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
#define SC(db) (db*((3/EG_STEP)*(1<<ENV_BITS)))+EG_DST
static const int SL_TABLE[16]={
 SC( 0),SC( 1),SC( 2),SC(3 ),SC(4 ),SC(5 ),SC(6 ),SC( 7),
 SC( 8),SC( 9),SC(10),SC(11),SC(12),SC(13),SC(14),SC(31)
};
#undef SC

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

/* envelope output curve table */
#ifdef SEG_SUPPORT
/* attack + decay + SSG upside + OFF */
static int ENV_CURVE[3*EG_ENT+1];
#else
/* attack + decay + OFF */
static int ENV_CURVE[2*EG_ENT+1];
#endif
/* envelope counter conversion table when change Decay to Attack phase */
static int DRAR_TABLE[EG_ENT];

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
static const int MUL_TABLE[4*16]= {
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
#define TYPE_SSG    0x01    /* SSG support       */
#define TYPE_OPN    0x02    /* OPN device        */
#define TYPE_LFOPAN 0x04    /* OPN type LFO and PAN */
#define TYPE_6CH    0x08    /* FM 6CH / 3CH      */
#define TYPE_DAC    0x10    /* YM2612's DAC device */
#define TYPE_ADPCM  0x20    /* ADPCM device      */

#define TYPE_YM2203 (TYPE_SSG)
#define TYPE_YM2608 (TYPE_SSG |TYPE_LFOPAN |TYPE_6CH |TYPE_ADPCM)
#define TYPE_YM2610 (TYPE_SSG |TYPE_LFOPAN |TYPE_6CH |TYPE_ADPCM)
#define TYPE_YM2612 (TYPE_6CH |TYPE_LFOPAN |TYPE_DAC)

static int FMNumChips;		/* total # of FM emulated */

/* work table */
static void *cur_chip = 0;	/* current chip point */

/* currenct chip state */
static FM_ST     *State;
static FMSAMPLE  *bufL,*bufR;
static FM_CH     *cch[8];
static signed int outd[4];

/* operator connection work */
static int feedback2;		/* connect for operator 2 */
static int feedback3;		/* connect for operator 3 */
static int feedback4;		/* connect for operator 4 */

/* log output level */
#define LOG_ERR  3      /* ERROR       */
#define LOG_WAR  2      /* WARNING     */
#define LOG_INF  1      /* INFORMATION */

#define LOG_LEVEL LOG_INF

#ifndef __RAINE__
static void Log(int level,char *format,...)
{
	va_list argptr;

	if( level < LOG_LEVEL ) return;
	va_start(argptr,format);
	/* */
	if (errorlog) vfprintf( errorlog, format , argptr);
}
#endif

/* --------------- Customize External interface port (SSG,Timer,etc) ---------------*/
#include "fmext.c"

/* --------------------- subroutines  --------------------- */

INLINE int Limit( int val, int max, int min ) {
	if ( val > max )
		val = max;
	else if ( val < min )
		val = min;

	return val;
}

/* status set and IRQ handling */
INLINE void FM_STATUS_SET(FM_ST *ST,int flag)
{
	/* set status flag */
	ST->status |= flag;
	if ( !(ST->irq) && (ST->status & ST->irqmask) )
	{
		ST->irq = 1;
		/* callback user interrupt handler (IRQ is OFF to ON) */
		if(ST->IRQ_Handler) (ST->IRQ_Handler)(ST->index,1);
	}
}

/* status reset and IRQ handling */
INLINE void FM_STATUS_RESET(FM_ST *ST,int flag)
{
	/* reset status flag */
	ST->status &=~flag;
	if ( (ST->irq) && !(ST->status & ST->irqmask) )
	{
		ST->irq = 0;
		/* callback user interrupt handler (IRQ is ON to OFF) */
		if(ST->IRQ_Handler) (ST->IRQ_Handler)(ST->index,0);
	}
}

/* IRQ mask set */
INLINE void FM_IRQMASK_SET(FM_ST *ST,int flag)
{
	ST->irqmask = flag;
	/* IRQ handling check */
	FM_STATUS_SET(ST,0);
	FM_STATUS_RESET(ST,0);
}

/* ----- key on  ----- */
INLINE void FM_KEYON(FM_CH *CH , int s )
{
	FM_SLOT *SLOT = &CH->SLOT[s];
	if( SLOT->evm<= ENV_MOD_RR)
	{
		/* set envelope counter from envleope output */

		/* sin wave restart */
		SLOT->Cnt = 0;
		if( s == SLOT1 ) CH->op1_out = 0;
		/* set attack */
#ifdef SEG_SUPPORT
		if( SLOT->SEG&8 ) ENV_SSG_AR;
		else
#endif
		SLOT->evm = ENV_MOD_AR;
		SLOT->evs = SLOT->evsa;
#if 0
		/* convert decay count to attack count */
		/* --- This caused the problem by credit sound of paper boy. --- */
		SLOT->evc = EG_AST + DRAR_TABLE[ENV_CURVE[SLOT->evc>>ENV_BITS]];/* + SLOT->evs;*/
#else
		/* reset attack counter */
		SLOT->evc = EG_AST;
#endif
		SLOT->eve = EG_AED;
	}
}
/* ----- key off ----- */
INLINE void FM_KEYOFF(FM_CH *CH , int s )
{
	FM_SLOT *SLOT = &CH->SLOT[s];
	if( SLOT->evm > ENV_MOD_RR)
	{
		/* set envelope counter from envleope output */
		SLOT->evm = ENV_MOD_RR;
		if( !(SLOT->evc&EG_DST) )
			SLOT->evc = (ENV_CURVE[SLOT->evc>>ENV_BITS]<<ENV_BITS) + EG_DST;
		SLOT->eve = EG_DED;
		SLOT->evs = SLOT->evsr;
	}
}

/* ---------- calcrate Envelope Generator & Phase Generator ---------- */
/* return : envelope output */
INLINE signed int FM_CALC_SLOT( FM_SLOT *SLOT )
{
	/* calcrate phage generator */
	SLOT->Cnt += SLOT->Incr;
	/* calcrate envelope generator */
	if( (SLOT->evc+=SLOT->evs) >= SLOT->eve )
	{
		switch( SLOT->evm ){
		case ENV_MOD_AR: /* ATTACK -> DECAY1 */
			/* next DR */
			SLOT->evm = ENV_MOD_DR;
			SLOT->evc = EG_DST;
			SLOT->eve = SLOT->SL;
			SLOT->evs = SLOT->evsd;
			break;
		case ENV_MOD_DR: /* DECAY -> SUSTAIN */
			SLOT->evm = ENV_MOD_SR;
			SLOT->evc = SLOT->SL;
			SLOT->eve = EG_DED;
			SLOT->evs = SLOT->evss;
			break;
		case ENV_MOD_RR: /* RR -> OFF & STOP */
			SLOT->evm = ENV_MOD_OFF;
		case ENV_MOD_SR: /* SR -> OFF & STOP */
			SLOT->evc = EG_OFF;
			SLOT->eve = EG_OFF+1;
			SLOT->evs = 0;
			break;
#ifdef SEG_SUPPORT
		case ENV_SSG_AR: /* SSG ATTACK */
			if( SLOT->SEG&4){	/* start direction */
				/* next SSG-SR (upside start ) */
				SLOT->evm = ENV_SSG_SR;
				SLOT->evc = SLOT->SL + (EG_UST - EG_DST);
				SLOT->eve = EG_UED;
				SLOT->evs = SLOT->evss;
			}else{
				/* next SSG-DR (downside start ) */
				SLOT->evm = ENV_SSG_DR;
				SLOT->evc = EG_DST;
				SLOT->eve = EG_DED;
				SLOT->evs = SLOT->evsd;
			}
			break;
		case ENV_SSG_DR:	/* SEG down side */
			if( SLOT->SEG&2){
				/* reverce */
				SLOT->evm = ENV_SSG_SR;
				SLOT->evc = SLOT->SL + (EG_UST - EG_DST);
				SLOT->eve = EG_UED;
				SLOT->evs = SLOT->evss;
			}else{
				/* again */
				SLOT->evc = EG_DST;
			}
			/* hold */
			if( SLOT->SEG&1) SLOT->evs = 0;
			break;
		case ENV_SSG_SR:	/* upside */
			if( SLOT->SEG&2){
				/* reverce  */
				SLOT->evm = ENV_SSG_DR;
				SLOT->evc = EG_DST;
				SLOT->eve = EG_DED;
				SLOT->evs = SLOT->evsd;
			}else{
				/* again */
				SLOT->evc = SLOT->SL + (EG_UST - EG_DST);
			}
			/* hold check */
			if( SLOT->SEG&1) SLOT->evs = 0;
			break;
#endif
		}
	}
	/* calcrate envelope */
#if 0 /* ifdef TL_SAVE_MEM */
	signed int env_out = SLOT->TLL+ENV_CURVE[SLOT->evc>>ENV_BITS]; /* LFO_out[SLOT->AMS] */
	if(env_out >= (EG_ENT-1) ) return EG_ENT-1;
	return env_out;
#else
	return SLOT->TLL+ENV_CURVE[SLOT->evc>>ENV_BITS]; /* LFO_out[SLOT->AMS] */
#endif
}

/* set algorythm connection */
static void set_algorythm( FM_CH *CH )
{
	signed int *carrier = &outd[CH->PAN];

	/* setup connect algorythm */
	switch( CH->ALGO ){
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
		CH->connect2 = carrier;
		CH->connect3 = &feedback4;
		break;
	case 5:
		/*         +-S2-+     */
		/* PG---S1-+-S3-+-OUT */
		/*         +-S4-+     */
		CH->connect1 = 0;	/* special mark */
		CH->connect2 = carrier;
		CH->connect3 = carrier;
		break;
	case 6:
		/* PG---S1---S2-+     */
		/* PG--------S3-+-OUT */
		/* PG--------S4-+     */
		CH->connect1 = &feedback2;
		CH->connect2 = carrier;
		CH->connect3 = carrier;
		break;
	case 7:
		/* PG---S1-+     */
		/* PG---S2-+-OUT */
		/* PG---S3-+     */
		/* PG---S4-+     */
		CH->connect1 = carrier;
		CH->connect2 = carrier;
		CH->connect3 = carrier;
	}
	CH->connect4 = carrier;
}

/* set detune & multiple */
INLINE void set_det_mul(FM_ST *ST,FM_CH *CH,FM_SLOT *SLOT,int v)
{
	SLOT->mul = MUL_TABLE[v&0x0f];
	SLOT->DT  = ST->DT_TABLE[(v>>4)&7];
	CH->SLOT[SLOT1].Incr=-1;
}

/* set total level */
INLINE void set_tl(FM_CH *CH,FM_SLOT *SLOT , int v,int csmflag)
{
	v &= 0x7f;
	v = (v<<7)|v; /* 7bit -> 14bit */
	SLOT->TL = (v*EG_ENT)>>14;
	if( !csmflag )
	{	/* not CSM latch total level */
		SLOT->TLL = SLOT->TL + KSL[CH->kcode];
	}
}

/* set attack rate & key scale  */
INLINE void set_ar_ksr(FM_CH *CH,FM_SLOT *SLOT,int v,signed int *ar_table)
{
	SLOT->KSR  = 3-(v>>6);
	SLOT->AR   = (v&=0x1f) ? &ar_table[v<<1] : RATE_0;
	SLOT->evsa = SLOT->AR[SLOT->ksr];
	if( SLOT->evm == ENV_MOD_AR ) SLOT->evs = SLOT->evsa;
	CH->SLOT[SLOT1].Incr=-1;
}
/* set decay rate */
INLINE void set_dr(FM_SLOT *SLOT,int v,signed int *dr_table)
{
	SLOT->DR = (v&=0x1f) ? &dr_table[v<<1] : RATE_0;
	SLOT->evsd = SLOT->DR[SLOT->ksr];
	if( SLOT->evm == ENV_MOD_DR ) SLOT->evs = SLOT->evsd;
}
/* set sustain rate */
INLINE void set_sr(FM_SLOT *SLOT,int v,signed int *dr_table)
{
	SLOT->SR = (v&=0x1f) ? &dr_table[v<<1] : RATE_0;
	SLOT->evss = SLOT->SR[SLOT->ksr];
	if( SLOT->evm == ENV_MOD_SR ) SLOT->evs = SLOT->evss;
}
/* set release rate */
INLINE void set_sl_rr(FM_SLOT *SLOT,int v,signed int *dr_table)
{
	SLOT->SL = SL_TABLE[(v>>4)];
	SLOT->RR = &dr_table[((v&0x0f)<<2)|2];
	SLOT->evsr = SLOT->RR[SLOT->ksr];
	if( SLOT->evm == ENV_MOD_RR ) SLOT->evs = SLOT->evsr;
}

/* operator output calcrator */
#define OP_OUT(slot,env,con)   SIN_TABLE[((slot.Cnt+con)/(0x1000000/SIN_ENT))&(SIN_ENT-1)][env]
/* ---------- calcrate one of channel ---------- */
INLINE void FM_CALC_CH( FM_CH *CH )
{
	int op_out;
	int env_out;

	feedback2 = feedback3 = feedback4 = 0;

	/* SLOT 1 */
	env_out=FM_CALC_SLOT(&CH->SLOT[SLOT1]);
	if( env_out < EG_ENT-1 )
	{
		if( CH->FB ){
			/* with self feed back */
			op_out = CH->op1_out;
			CH->op1_out = OP_OUT(CH->SLOT[SLOT1],env_out,(CH->op1_out>>CH->FB) /* +LFOOut[SLOT->AMS]*/ );
			op_out = (op_out + CH->op1_out)/2;
		}else{
			/* without self feed back */
			op_out = OP_OUT(CH->SLOT[SLOT1],env_out,0 /* +LFOOut[SLOT->AMS]*/ );
		}
		/* output slot1 */
		if( !CH->connect1 )
		{
			/* algorythm 5  */
			feedback2 = feedback3 = feedback4 = op_out;
		}else{
			/* other algorythm */
			*CH->connect1 += op_out;
		}
	}
	/* SLOT 2 */
	env_out=FM_CALC_SLOT(&CH->SLOT[SLOT2]);
	if( env_out < EG_ENT-1 )
		*CH->connect2 += OP_OUT(CH->SLOT[SLOT2],env_out, feedback2 /* +LFOOut[SLOT->AMS]*/  );
	/* SLOT 3 */
	env_out=FM_CALC_SLOT(&CH->SLOT[SLOT3]);
	if( env_out < EG_ENT-1 )
		*CH->connect3 += OP_OUT(CH->SLOT[SLOT3],env_out, feedback3  /* +LFOOut[SLOT->AMS]*/ );
	/* SLOT 4 */
	env_out=FM_CALC_SLOT(&CH->SLOT[SLOT4]);
	if( env_out < EG_ENT-1 )
		*CH->connect4 += OP_OUT(CH->SLOT[SLOT4],env_out, feedback4  /* +LFOOut[SLOT->AMS]*/ );
}
/* ---------- frequency counter for operater update ---------- */
INLINE void CALC_FCSLOT(FM_SLOT *SLOT , int fc , int kc )
{
	int ksr;

	/* frequency step counter */
	SLOT->Incr= (fc+SLOT->DT[kc])*SLOT->mul;
	ksr = kc >> SLOT->KSR;
	if( SLOT->ksr != ksr )
	{
		SLOT->ksr = ksr;
		/* attack , decay rate recalcration */
		SLOT->evsa = SLOT->AR[ksr];
		SLOT->evsd = SLOT->DR[ksr];
		SLOT->evss = SLOT->SR[ksr];
		SLOT->evsr = SLOT->RR[ksr];
	}
	SLOT->TLL = SLOT->TL + KSL[kc];
}

/* ---------- frequency counter  ---------- */
INLINE void CALC_FCOUNT(FM_CH *CH )
{
	if( CH->SLOT[SLOT1].Incr==-1){
		int fc = CH->fc;
		int kc = CH->kcode;
		CALC_FCSLOT(&CH->SLOT[SLOT1] , fc , kc );
		CALC_FCSLOT(&CH->SLOT[SLOT2] , fc , kc );
		CALC_FCSLOT(&CH->SLOT[SLOT3] , fc , kc );
		CALC_FCSLOT(&CH->SLOT[SLOT4] , fc , kc );
	}
}

/* ---------- frequency counter  ---------- */
INLINE void OPM_CALC_FCOUNT(YM2151 *OPM , FM_CH *CH )
{
	if( CH->SLOT[SLOT1].Incr==-1)
	{
		int fc = CH->fc;
		int kc = CH->kcode;
		CALC_FCSLOT(&CH->SLOT[SLOT1] , OPM->KC_TABLE[fc + CH->SLOT[SLOT1].DT2] , kc );
		CALC_FCSLOT(&CH->SLOT[SLOT2] , OPM->KC_TABLE[fc + CH->SLOT[SLOT2].DT2] , kc );
		CALC_FCSLOT(&CH->SLOT[SLOT3] , OPM->KC_TABLE[fc + CH->SLOT[SLOT3].DT2] , kc );
		CALC_FCSLOT(&CH->SLOT[SLOT4] , OPM->KC_TABLE[fc + CH->SLOT[SLOT4].DT2] , kc );
	}
}
/* ----------- initialize time tabls ----------- */
static void init_timetables( FM_ST *ST , char *DTTABLE , int ARRATE , int DRRATE )
{
	int i,d;
	double rate;

	/* make detune table */
	for (d = 0;d <= 3;d++){
		for (i = 0;i <= 31;i++){
			rate = (double)DTTABLE[d*32 + i] * ST->freqbase / 4096 * FREQ_RATE;
			ST->DT_TABLE[d][i]   =  rate;
			ST->DT_TABLE[d+4][i] = -rate;
		}
	}
	/* make attack rate & decay rate tables */
	for (i = 0;i < 4;i++) ST->AR_TABLE[i] = ST->DR_TABLE[i] = 0;
	for (i = 4;i < 64;i++){
		rate  = (double)ST->freqbase / 4096.0;		/* frequency rate */
		if( i < 60 ) rate *= 1.0+(i&3)*0.25;		/* b0-1 : x1 , x1.25 , x1.5 , x1.75 */
		rate *= 1<<((i>>2)-1);						/* b2-5 : shift bit */
		rate *= (double)(EG_ENT<<ENV_BITS);
		ST->AR_TABLE[i] = rate / ARRATE;
		ST->DR_TABLE[i] = rate / DRRATE;
	}
	ST->AR_TABLE[62] = EG_AED-1;
	ST->AR_TABLE[63] = EG_AED-1;
	for (i = 64;i < 94 ;i++){	/* make for overflow area */
		ST->AR_TABLE[i] = ST->AR_TABLE[63];
		ST->DR_TABLE[i] = ST->DR_TABLE[63];
	}

#if 0
	for (i = 0;i < 64 ;i++){	/* make for overflow area */
		Log(LOG_WAR,"rate %2d , ar %f ms , dr %f ms \n",i,
			((double)(EG_ENT<<ENV_BITS) / ST->AR_TABLE[i]) * (1000.0 / ST->rate),
			((double)(EG_ENT<<ENV_BITS) / ST->DR_TABLE[i]) * (1000.0 / ST->rate) );
	}
#endif
}

/* ---------- reset one of channel  ---------- */
static void reset_channel( FM_ST *ST , FM_CH *CH , int chan )
{
	int c,s;

	ST->mode   = 0;	/* normal mode */
	FM_STATUS_RESET(ST,0xff);
	ST->TA     = 0;
	ST->TAC    = 0;
	ST->TB     = 0;
	ST->TBC    = 0;

	for( c = 0 ; c < chan ; c++ )
	{
		CH[c].fc = 0;
		CH[c].PAN = OPN_CENTER; /* or OPM_CENTER */
		for(s = 0 ; s < 4 ; s++ )
		{
			CH[c].SLOT[s].SEG = 0;
			CH[c].SLOT[s].evm = ENV_MOD_OFF;
			CH[c].SLOT[s].evc = EG_OFF;
			CH[c].SLOT[s].eve = EG_OFF+1;
			CH[c].SLOT[s].evs = 0;
		}
	}
}

/* ---------- generic table initialize ---------- */
static int FMInitTable( void )
{
	int s,t;
	double rate;
	int i,j;
	double pom;

	/* allocate total level table */
	TL_TABLE = malloc(TL_MAX*2*sizeof(int));
	if( TL_TABLE == 0 ) return 0;
	/* make total level table */
	for (t = 0;t < EG_ENT-1 ;t++){
		rate = ((1<<TL_BITS)-1)/pow(10,EG_STEP*t/20);	/* dB -> voltage */
		TL_TABLE[       t] =  (int)rate;
		TL_TABLE[TL_MAX+t] = -TL_TABLE[t];
/*		Log(LOG_INF,"TotalLevel(%3d) = %x\n",t,TL_TABLE[t]);*/
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
/*		Log(LOG_INF,"sin(%3d) = %f:%f db\n",s,pom,(double)j * EG_STEP);*/
	}
	/* envelope counter -> envelope output table */
	for (i=0; i<EG_ENT; i++)
	{
		/* ATTACK curve */
		/* !!!!! preliminary !!!!! */
		pom = pow( ((double)(EG_ENT-1-i)/EG_ENT) , 8 ) * EG_ENT;
		/* if( pom >= EG_ENT ) pom = EG_ENT-1; */
		ENV_CURVE[i] = (int)pom;
		/* DECAY ,RELEASE curve */
		ENV_CURVE[(EG_DST>>ENV_BITS)+i]= i;
#ifdef SEG_SUPPORT
		/* DECAY UPSIDE (SSG ENV) */
		ENV_CURVE[(EG_UST>>ENV_BITS)+i]= EG_ENT-1-i;
#endif
	}
	/* off */
	ENV_CURVE[EG_OFF>>ENV_BITS]= EG_ENT-1;

	/* decay to reattack envelope converttable */
	j = EG_ENT-1;
	for (i=0; i<EG_ENT; i++)
	{
		while( j && (ENV_CURVE[j] < i) ) j--;
		DRAR_TABLE[i] = j<<ENV_BITS;
		/* Log(LOG_INF,"DR %06X = %06X,AR=%06X\n",i,DRAR_TABLE[i],ENV_CURVE[DRAR_TABLE[i]>>ENV_BITS] ); */
	}
	return 1;
}


static void FMCloseTable( void )
{
	if( TL_TABLE ) free( TL_TABLE );
	return;
}

/* OPN/OPM Mode  Register Write */
INLINE void FMSetMode( FM_ST *ST ,int n,int v )
{
	/* b7 = CSM MODE */
	/* b6 = 3 slot mode */
	/* b5 = reset b */
	/* b4 = reset a */
	/* b3 = timer enable b */
	/* b2 = timer enable a */
	/* b1 = load b */
	/* b0 = load a */
	ST->mode = v;

	/* reset Timer b flag */
	if( v & 0x20 )
		FM_STATUS_RESET(ST,0x02);
	/* reset Timer a flag */
	if( v & 0x10 )
		FM_STATUS_RESET(ST,0x01);
	/* load b */
	if( v & 0x02 )
	{
		if( ST->TBC == 0 )
		{
			ST->TBC = ( 256-ST->TB)<<4;
			/* External timer handler */
			if (ST->Timer_Handler) (ST->Timer_Handler)(n,1,(double)ST->TBC,ST->TimerBase);
		}
	}else if (ST->timermodel == FM_TIMER_INTERVAL)
	{	/* stop interbval timer */
		if( ST->TBC != 0 )
		{
			ST->TBC = 0;
			if (ST->Timer_Handler) (ST->Timer_Handler)(n,1,0,ST->TimerBase);
		}
	}
	/* load a */
	if( v & 0x01 )
	{
		if( ST->TAC == 0 )
		{
			ST->TAC = (1024-ST->TA);
			/* External timer handler */
			if (ST->Timer_Handler) (ST->Timer_Handler)(n,0,(double)ST->TAC,ST->TimerBase);
		}
	}else if (ST->timermodel == FM_TIMER_INTERVAL)
	{	/* stop interbval timer */
		if( ST->TAC != 0 )
		{
			ST->TAC = 0;
			if (ST->Timer_Handler) (ST->Timer_Handler)(n,0,0,ST->TimerBase);
		}
	}
}

/* Timer A Overflow */
INLINE void TimerAOver(FM_ST *ST)
{
	/* status set if enabled */
	if(ST->mode & 0x04) FM_STATUS_SET(ST,0x01);
	/* clear or reload the counter */
	if (ST->timermodel == FM_TIMER_INTERVAL)
	{
		ST->TAC = (1024-ST->TA);
		if (ST->Timer_Handler) (ST->Timer_Handler)(ST->index,0,(double)ST->TAC,ST->TimerBase);
	}
	else ST->TAC = 0;
}
/* Timer B Overflow */
INLINE void TimerBOver(FM_ST *ST)
{
	/* status set if enabled */
	if(ST->mode & 0x08) FM_STATUS_SET(ST,0x02);
	/* clear or reload the counter */
	if (ST->timermodel == FM_TIMER_INTERVAL)
	{
		ST->TBC = ( 256-ST->TB)<<4;
		if (ST->Timer_Handler) (ST->Timer_Handler)(ST->index,1,(double)ST->TBC,ST->TimerBase);
	}
	else ST->TBC = 0;
}
/* CSM Key Controll */
INLINE void CSMKeyControll(FM_CH *CH)
{
	int ksl = KSL[CH->kcode];
	/* all key off */
	FM_KEYOFF(CH,SLOT1);
	FM_KEYOFF(CH,SLOT2);
	FM_KEYOFF(CH,SLOT3);
	FM_KEYOFF(CH,SLOT4);
	/* total level latch */
	CH->SLOT[SLOT1].TLL = CH->SLOT[SLOT1].TL + ksl;
	CH->SLOT[SLOT2].TLL = CH->SLOT[SLOT2].TL + ksl;
	CH->SLOT[SLOT3].TLL = CH->SLOT[SLOT3].TL + ksl;
	CH->SLOT[SLOT4].TLL = CH->SLOT[SLOT4].TL + ksl;
	/* all key on */
	FM_KEYON(CH,SLOT1);
	FM_KEYON(CH,SLOT2);
	FM_KEYON(CH,SLOT3);
	FM_KEYON(CH,SLOT4);
}

#ifdef INTERNAL_TIMER
/* ---------- calcrate timer A ---------- */
INLINE void CALC_TIMER_A( FM_ST *ST , FM_CH *CSM_CH ){
  if( ST->TAC &&  (ST->Timer_Handler==0) )
    if( (ST->TAC -= ST->freqbase) <= 0 ){
      TimerAOver( ST );
      /* CSM mode key,TL controll */
      if( ST->mode & 0x80 ){	/* CSM mode total level latch and auto key on */
	CSMKeyControll( CSM_CH );
      }
    }
}
/* ---------- calcrate timer B ---------- */
INLINE void CALC_TIMER_B( FM_ST *ST,int step){
  if( ST->TBC && (ST->Timer_Handler==0) )
    if( (ST->TBC -= ST->freqbase*step) <= 0 ){
      TimerBOver( ST );
    }
}
#endif /* INTERNAL_TIMER */

#if BUILD_OPN
/* ---------- priscaler set(and make time tables) ---------- */
void OPNSetPris(FM_OPN *OPN , int pris , int TimerPris, int SSGpris)
{
	int fn;

	/* frequency base */
	OPN->ST.freqbase = (OPN->ST.rate) ? ((double)OPN->ST.clock * 4096.0 / OPN->ST.rate) / pris : 0;
	/* Timer base time */
	OPN->ST.TimerBase = (OPN->ST.rate) ? 1.0/((double)OPN->ST.clock / (double)TimerPris) : 0;
	/* SSG part  priscaler set */
	if( SSGpris ) SSGClk( OPN->ST.index, OPN->ST.clock * 2 / SSGpris );
	/* make time tables */
	init_timetables( &OPN->ST , OPN_DTTABLE , OPN_ARRATE , OPN_DRRATE );
	/* make fnumber -> increment counter table */
	for( fn=0 ; fn < 2048 ; fn++ )
	{
		/* it is freq table for octave 7 */
		/* opn freq counter = 20bit */
		OPN->FN_TABLE[fn] = (double)fn * OPN->ST.freqbase / 4096  * FREQ_RATE * (1<<7) / 2;
	}
/*	Log(LOG_INF,"OPN %d set priscaler %d\n",OPN->ST.index,pris);*/
}

/* ---------- write a OPN mode register 0x20-0x2f ---------- */
static void OPNWriteMode(FM_OPN *OPN, int r, int v)
{
	unsigned char c;
	FM_CH *CH;

	switch(r){
	case 0x21:	/* Test */
		break;
	case 0x22:	/* LFO FREQ (YM2608/YM2612) */
		/* 3.98Hz,5.56Hz,6.02Hz,6.37Hz,6.88Hz,9.63Hz,48.1Hz,72.2Hz */
		/* FM2608[n].LFOIncr = FM2608[n].LFO_TABLE[v&0x0f]; */
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
		FMSetMode( &(OPN->ST),OPN->ST.index,v );
		break;
	case 0x28:	/* key on / off */
		c = v&0x03;
		if( c == 3 ) break;
		if( (v&0x04) && (OPN->type & TYPE_6CH) ) c+=3;
		CH = OPN->P_CH;
		CH = &CH[c];
		/* csm mode */
		if( c == 2 && (OPN->ST.mode & 0x80) ) break;
		if(v&0x10) FM_KEYON(CH,SLOT1); else FM_KEYOFF(CH,SLOT1);
		if(v&0x20) FM_KEYON(CH,SLOT2); else FM_KEYOFF(CH,SLOT2);
		if(v&0x40) FM_KEYON(CH,SLOT3); else FM_KEYOFF(CH,SLOT3);
		if(v&0x80) FM_KEYON(CH,SLOT4); else FM_KEYOFF(CH,SLOT4);
/*		Log(LOG_INF,"OPN %d:%d : KEY %02X\n",n,c,v&0xf0);*/
		break;
	}
}

/* ---------- write a OPN register (0x30-0xff) ---------- */
static void OPNWriteReg(FM_OPN *OPN, int r, int v)
{
	unsigned char c;
	FM_CH *CH;
	FM_SLOT *SLOT;

	/* 0x30 - 0xff */
	if( (c = OPN_CHAN(r)) == 3 ) return; /* 0xX3,0xX7,0xXB,0xXF */
	if( (r >= 0x100) /* && (OPN->type & TYPE_6CH) */ ) c+=3;
		CH = OPN->P_CH;
		CH = &CH[c];

	SLOT = &(CH->SLOT[OPN_SLOT(r)]);
	switch( r & 0xf0 ) {
	case 0x30:	/* DET , MUL */
		set_det_mul(&OPN->ST,CH,SLOT,v);
		break;
	case 0x40:	/* TL */
		set_tl(CH,SLOT,v,(c == 2) && (OPN->ST.mode & 0x80) );
		break;
	case 0x50:	/* KS, AR */
		set_ar_ksr(CH,SLOT,v,OPN->ST.AR_TABLE);
		break;
	case 0x60:	/*     DR */
		/* bit7 = AMS ENABLE(YM2612) */
		set_dr(SLOT,v,OPN->ST.DR_TABLE);
		break;
	case 0x70:	/*     SR */
		set_sr(SLOT,v,OPN->ST.DR_TABLE);
		break;
	case 0x80:	/* SL, RR */
		set_sl_rr(SLOT,v,OPN->ST.DR_TABLE);
		break;
	case 0x90:	/* SSG-EG */
#ifndef SEG_SUPPORT
		if(v&0x08) Log(LOG_ERR,"OPN %d,%d,%d :SSG-TYPE envelope selected (not supported )\n",OPN->ST.index,c,OPN_SLOT(r));
#endif
		SLOT->SEG = v&0x0f;
		break;
	case 0xa0:
		switch( OPN_SLOT(r) ){
		case 0:		/* 0xa0-0xa2 : FNUM1 */
			{
				unsigned int fn  = (((unsigned int)( (CH->fn_h)&7))<<8) + v;
				unsigned char blk = CH->fn_h>>3;
				/* make keyscale code */
				CH->kcode = (blk<<2)|OPN_FKTABLE[(fn>>7)];
				/* make basic increment counter 32bit = 1 cycle */
				CH->fc = OPN->FN_TABLE[fn]>>(7-blk);
				CH->SLOT[SLOT1].Incr=-1;
			}
			break;
		case 1:		/* 0xa4-0xa6 : FNUM2,BLK */
			CH->fn_h = v&0x3f;
			break;
		case 2:		/* 0xa8-0xaa : 3CH FNUM1 */
			if( r < 0x100)
			{
				unsigned int fn  = (((unsigned int)(OPN->SL3.fn_h[c]&7))<<8) + v;
				unsigned char blk = OPN->SL3.fn_h[c]>>3;
				/* make keyscale code */
				OPN->SL3.kcode[c]= (blk<<2)|OPN_FKTABLE[(fn>>7)];
				/* make basic increment counter 32bit = 1 cycle */
				OPN->SL3.fc[c] = OPN->FN_TABLE[fn]>>(7-blk);
				(OPN->P_CH)[2].SLOT[SLOT1].Incr=-1;
			}
			break;
		case 3:		/* 0xac-0xae : 3CH FNUM2,BLK */
			if( r < 0x100)
				OPN->SL3.fn_h[c] = v&0x3f;
			break;
		}
		break;
	case 0xb0:
		switch( OPN_SLOT(r) ){
		case 0:		/* 0xb0-0xb2 : FB,ALGO */
			{
				int feedback = (v>>3)&7;
				CH->ALGO = v&7;
				CH->FB   = feedback ? 8 - feedback : 0;
				set_algorythm( CH );
			}
			break;
		case 1:		/* 0xb4-0xb6 : L , R , AMS , PMS (YM2612/YM2608) */
			if( OPN->type & TYPE_LFOPAN)
			{
				/* b0-2 PMS */
				/* 0,3.4,6.7,10,14,20,40,80(cent) */
				SLOT->pms = (v>>4) & 0x07;
				/* b4-5 AMS */
				/* 0,1.4,5.9,11.8(dB) */
				SLOT->ams = v & 0x03;
				/* PAN */
				CH->PAN = (v>>6)&0x03; /* PAN : b6 = R , b7 = L */
				set_algorythm( CH );
				/* Log(LOG_INF,"OPN %d,%d : PAN %d\n",n,c,CH->PAN);*/
			}
			break;
		}
		break;
	}
}

#endif /* BUILD_OPN */

#if BUILD_YM2203
/*******************************************************************************/
/*		YM2203 local section                                                   */
/*******************************************************************************/
static YM2203 *FM2203=NULL;	/* array of YM2203's */

/* ---------- update one of chip ----------- */
void YM2203UpdateOne(int num, void *buffer, int length)
{
	YM2203 *F2203 = &(FM2203[num]);
	FM_OPN *OPN =   &(FM2203[num].OPN);
    int i,ch;
	int data;
	FMSAMPLE *buf = (FMSAMPLE *)buffer;

	State = &F2203->OPN.ST;
	cch[0]   = &F2203->CH[0];
	cch[1]   = &F2203->CH[1];
	cch[2]   = &F2203->CH[2];

	/* frequency counter channel A */
	CALC_FCOUNT( cch[0] );
	/* frequency counter channel B */
	CALC_FCOUNT( cch[1] );
	/* frequency counter channel C */
	if( (State->mode & 0xc0) ){
		/* 3SLOT MODE */
		if( cch[2]->SLOT[SLOT1].Incr==-1){
			/* 3 slot mode */
			CALC_FCSLOT(&cch[2]->SLOT[SLOT1] , OPN->SL3.fc[1] , OPN->SL3.kcode[1] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT2] , OPN->SL3.fc[2] , OPN->SL3.kcode[2] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT3] , OPN->SL3.fc[0] , OPN->SL3.kcode[0] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT4] , cch[2]->fc , cch[2]->kcode );
		}
	}else CALC_FCOUNT( cch[2] );

    for( i=0; i < length ; i++ )
	{
		/*            channel A         channel B         channel C      */
		outd[OPN_CENTER] = 0;
		/* calcrate FM */
		for( ch=0;ch<3;ch++) FM_CALC_CH( cch[ch] );
		/* limit check */
		data = Limit( outd[OPN_CENTER] , OPN_MAXOUT, OPN_MINOUT );
		/* store to sound buffer */
		buf[i] = data >> OPN_OUTSB;
#ifdef INTERNAL_TIMER
		/* timer controll */
		CALC_TIMER_A( State , cch[2] );
#endif
	}
#ifdef INTERNAL_TIMER
	CALC_TIMER_B( State , length );
#endif
}

/* ---------- reset one of chip ---------- */
void YM2203ResetChip(int num)
{
	int i;
	FM_OPN *OPN = &(FM2203[num].OPN);

	/* Reset Priscaler */
	OPNSetPris( OPN , 6*12 , 6*12 ,4); /* 1/6 , 1/4 */
	/* reset SSG section */
	SSGReset(OPN->ST.index);
	/* status clear */
	FM_IRQMASK_SET(&OPN->ST,0x03);
	OPNWriteMode(OPN,0x27,0x30); /* mode 0 , timer reset */
	reset_channel( &OPN->ST , FM2203[num].CH , 3 );
	/* reset OPerator paramater */
	for(i = 0xb6 ; i >= 0xb4 ; i-- ) OPNWriteReg(OPN,i,0xc0); /* PAN RESET */
	for(i = 0xb2 ; i >= 0x30 ; i-- ) OPNWriteReg(OPN,i,0);
	for(i = 0x26 ; i >= 0x20 ; i-- ) OPNWriteReg(OPN,i,0);
}
#if 0
/* ---------- return the buffer ---------- */
FMSAMPLE *YM2203Buffer(int n)
{
    return FM2203[n].Buf;
}

/* ---------- set buffer ---------- */
int YM2203SetBuffer(int n, FMSAMPLE *buf)
{
	if( buf == 0 ) return -1;
    FM2203[n].Buf = buf;
	return 0;
}
#endif

/* ----------  Initialize YM2203 emulator(s) ----------    */
/* 'num' is the number of virtual YM2203's to allocate     */
/* 'rate' is sampling rate and 'bufsiz' is the size of the */
/* buffer that should be updated at each interval          */
int YM2203Init(int num, int clock, int rate,
               FM_TIMERHANDLER TimerHandler,FM_IRQHANDLER IRQHandler)
{
	int i;

	if (FM2203) return (-1);	/* duplicate init. */
	cur_chip = NULL;	/* hiro-shi!! */

	FMNumChips = num;

	/* allocate ym2203 state space */
	if( (FM2203 = (YM2203 *)malloc(sizeof(YM2203) * FMNumChips))==NULL)
		return (-1);
	/* clear */
	memset(FM2203,0,sizeof(YM2203) * FMNumChips);
	/* allocate total level table (128kb space) */
	if( !FMInitTable() )
	{
		free( FM2203 );
		return (-1);
	}

	for ( i = 0 ; i < FMNumChips; i++ ) {
		FM2203[i].OPN.ST.index = i;
		FM2203[i].OPN.type = TYPE_YM2203;
		FM2203[i].OPN.P_CH = FM2203[i].CH;
		FM2203[i].OPN.ST.clock = clock;
		FM2203[i].OPN.ST.rate = rate;
		/* FM2203[i].OPN.ST.irq = 0; */
		/* FM2203[i].OPN.ST.satus = 0; */
		FM2203[i].OPN.ST.timermodel = FM_TIMER_SINGLE;
		/* Extend handler */
		FM2203[i].OPN.ST.Timer_Handler = TimerHandler;
		FM2203[i].OPN.ST.IRQ_Handler   = IRQHandler;
		YM2203ResetChip(i);
	}
	return(0);
}

/* ---------- shut down emurator ----------- */
void YM2203Shutdown(void)
{
    if (!FM2203) return;

	FMCloseTable();
	free(FM2203);
	FM2203 = NULL;
}

/* ---------- YM2203 I/O interface ---------- */
int YM2203Write(int n,int a,int v)
{
	FM_OPN *OPN = &(FM2203[n].OPN);

	if( !(a&1) )
	{	/* address port */
		OPN->ST.address = v & 0xff;
		/* Write register to SSG emurator */
		if( v < 16 ) SSGWrite(n,0,v);
		switch(OPN->ST.address)
		{
		case 0x2d:	/* divider sel */
			OPNSetPris( OPN, 6*12, 6*12 ,4); /* OPN 1/6 , SSG 1/4 */
			break;
		case 0x2e:	/* divider sel */
			OPNSetPris( OPN, 3*12, 3*12,2); /* OPN 1/3 , SSG 1/2 */
			break;
		case 0x2f:	/* divider sel */
			OPNSetPris( OPN, 2*12, 2*12,1); /* OPN 1/2 , SSG 1/1 */
			break;
		}
	}
	else
	{	/* data port */
		int addr = OPN->ST.address;
		switch( addr & 0xf0 )
		{
		case 0x00:	/* 0x00-0x0f : SSG section */
			/* Write data to SSG emurator */
			SSGWrite(n,a,v);
			break;
		case 0x20:	/* 0x20-0x2f : Mode section */
			YM2203UpdateReq(n);
			/* write register */
			 OPNWriteMode(OPN,addr,v);
			break;
		default:	/* 0x30-0xff : OPN section */
			YM2203UpdateReq(n);
			/* write register */
			 OPNWriteReg(OPN,addr,v);
		}
	}
	return OPN->ST.irq;
}

unsigned char YM2203Read(int n,int a)
{
	YM2203 *F2203 = &(FM2203[n]);
	int addr = F2203->OPN.ST.address;
	int ret = 0;

	if( !(a&1) )
	{	/* status port */
		ret = F2203->OPN.ST.status;
	}
	else
	{	/* data port (ONLY SSG) */
		if( addr < 16 ) ret = SSGRead(n);
	}
	return ret;
}

int YM2203TimerOver(int n,int c)
{
	YM2203 *F2203 = &(FM2203[n]);

	if( c )
	{	/* Timer B */
		TimerBOver( &(F2203->OPN.ST) );
	}
	else
	{	/* Timer A */
		YM2203UpdateReq(n);
		/* timer update */
		TimerAOver( &(F2203->OPN.ST) );
		/* CSM mode key,TL controll */
		if( F2203->OPN.ST.mode & 0x80 )
		{	/* CSM mode total level latch and auto key on */
			CSMKeyControll( &(F2203->CH[2]) );
		}
	}
	return F2203->OPN.ST.irq;
}

#endif /* BUILD_YM2203 */

#if (BUILD_FM_ADPCMA || BUILD_FM_ADPCMB)

//#define ADPCMA_DECODE_RANGE 1024
#define ADPCMA_DECODE_RANGE 1024
#define ADPCMA_DECODE_MIN (-(ADPCMA_DECODE_RANGE*ADPCMA_VOLUME_RATE))
#define ADPCMA_DECODE_MAX ((ADPCMA_DECODE_RANGE*ADPCMA_VOLUME_RATE)-1)
#define ADPCMA_VOLUME_DIV 1

#define ADPCMB_DECODE_RANGE 32768
#define ADPCMB_DECODE_MIN (-(ADPCMB_DECODE_RANGE))
#define ADPCMB_DECODE_MAX ((ADPCMB_DECODE_RANGE)-1)

/* DELTA-T particle adjuster */
#define ADPCMB_DELTA_MAX (24576)
#define ADPCMB_DELTA_MIN (127)
#define ADPCMB_DELTA_DEF (127)

/***************************************************************/
/*    ADPCM units are made by Hiromitsu Shioya (MMSND)         */
/***************************************************************/

static char         *pcmbufA, *pcmbufB;
static unsigned int pcmsizeA, pcmsizeB;


/************************************************************/
/************************************************************/
/* --------------------- subroutines  --------------------- */
/************************************************************/
/************************************************************/
/************************/
/*    ADPCM A tables    */
/************************/
static int jedi_table[49*16];
static int decode_tableA1[16] = {
  -1*16, -1*16, -1*16, -1*16, 2*16, 5*16, 7*16, 9*16,
  -1*16, -1*16, -1*16, -1*16, 2*16, 5*16, 7*16, 9*16
};

/* 0.9 , 0.9 , 0.9 , 0.9 , 1.2 , 1.6 , 2.0 , 2.4 */
/* 8 = -1 , 2 5 8 11 */
/* 9 = -1 , 2 5 9 13 */
/* 10= -1 , 2 6 10 14 */
/* 12= -1 , 2 7 12 17 */
/* 20= -2 , 4 12 20 32 */

#if 1
static void InitOPNB_ADPCMATable(void){
	int step, nib;

	for (step = 0; step <= 48; step++)
	{
		int stepval = floor (16.0 * pow (11.0 / 10.0, (double)step) * ADPCMA_VOLUME_RATE);
		/* loop over all nibbles and compute the difference */
		for (nib = 0; nib < 16; nib++)
		{
			int value = stepval*((nib&0x07)*2+1)/8;
			jedi_table[step*16+nib] = (nib&0x08) ? -value : value;
		}
	}
}
#else
static int decode_tableA2[49] = {
  0x0010, 0x0011, 0x0013, 0x0015, 0x0017, 0x0019, 0x001c, 0x001f,
  0x0022, 0x0025, 0x0029, 0x002d, 0x0032, 0x0037, 0x003c, 0x0042,
  0x0049, 0x0050, 0x0058, 0x0061, 0x006b, 0x0076, 0x0082, 0x008f,
  0x009d, 0x00ad, 0x00be, 0x00d1, 0x00e6, 0x00fd, 0x0117, 0x0133,
  0x0151, 0x0173, 0x0198, 0x01c1, 0x01ee, 0x0220, 0x0256, 0x0292,
  0x02d4, 0x031c, 0x036c, 0x03c3, 0x0424, 0x048e, 0x0502, 0x0583,
  0x0610
};
static void InitOPNB_ADPCMATable(void){
   int ta,tb,tc;
   for(ta=0;ta<49;ta++){
     for(tb=0;tb<16;tb++){
       tc=0;
       if(tb&0x04){tc+=((decode_tableA2[ta]*ADPCMA_VOLUME_RATE));}
       if(tb&0x02){tc+=((decode_tableA2[ta]*ADPCMA_VOLUME_RATE)>>1);}
       if(tb&0x01){tc+=((decode_tableA2[ta]*ADPCMA_VOLUME_RATE)>>2);}
       tc+=((decode_tableA2[ta]*ADPCMA_VOLUME_RATE)>>3);
       if(tb&0x08){tc=(0-tc);}
       jedi_table[ta*16+tb]=tc;
     }
   }
}
#endif

/************************/
/*    ADPCM B tables    */
/************************/
/* Forecast to next Forecast (rate = *8) */
/* 1/8 , 3/8 , 5/8 , 7/8 , 9/8 , 11/8 , 13/8 , 15/8 */
static const int decode_tableB1[16] = {
  1,   3,   5,   7,   9,  11,  13,  15,
  -1,  -3,  -5,  -7,  -9, -11, -13, -15,
};
/* delta to next delta (rate= *64) */
/* 0.9 , 0.9 , 0.9 , 0.9 , 1.2 , 1.6 , 2.0 , 2.4 */
static const int decode_tableB2[16] = {
  57,  57,  57,  57, 77, 102, 128, 153,
  57,  57,  57,  57, 77, 102, 128, 153
};

/* Forecast to Measurement (rate = *8) */
/*      n < 1/4 , 1/4 <= n > 1/2 , 1/2 <= n > 3/4 , 3/4 <= n > 1 */
/* 1 <= n > 5/4 , 5/4 <= n > 3/2 , 3/2 <= n > 7/4 , 7/4 <= n     */
#if 1
#define decode_tableB3 decode_tableB1
#else
static const int decode_tableB3[16] = {
  0, 2,  4,  6,  8,  10, 12, 14,
  0,-2, -4, -6, -8, -10,-12,-14
};
#endif

/**** ADPCM A (Non control type) ****/
INLINE void OPNB_ADPCM_CALC_CHA( YM2610 *F2610, ADPCM_CH *ch )
{
	unsigned int step;
	int data;

	ch->now_step += ch->step;
	if ( ch->now_step >= (1<<ADPCM_SHIFT) )
	{
		step = ch->now_step >> ADPCM_SHIFT;
		ch->now_step &= (1<<ADPCM_SHIFT)-1;
		/* end check */
		if ( (ch->now_addr+step) > (ch->end<<1) ) {
			ch->flag = 0;
			F2610->adpcm_arrivedEndAddress |= ch->flagMask & F2610->adpcm_statusmask;
			return;
		}
		do{
#if 0
			if ( ch->now_addr > (pcmsizeA<<1) ) {
				Log(LOG_WAR,"YM2610: Attempting to play past adpcm rom size!\n" );
				return;
			}
#endif
			if( ch->now_addr&1 ) data = ch->now_data & 0x0f;
			else
			{
				ch->now_data = *(pcmbufA+(ch->now_addr>>1));
				data = (ch->now_data >> 4)&0x0f;
			}
			ch->now_addr++;

			ch->adpcmx = Limit( ch->adpcmx + (jedi_table[ch->adpcmd+data]),
					    ADPCMA_DECODE_MAX, ADPCMA_DECODE_MIN );
			ch->adpcmd = Limit( ch->adpcmd + decode_tableA1[data], 48*16, 0*16 );
			/**** calc pcm * volume data ****/
			ch->adpcml = ch->adpcmx * ch->volume;
		}while(--step);
	}
	/* output for work of output channels (outd[OPNxxxx])*/
	*(ch->pan) += ch->adpcml;
}

/**** ADPCM B (Delta-T control type) ****/
INLINE void OPNB_ADPCM_CALC_CHB( YM2610 *F2610, ADPCM_CH *ch )
{
	unsigned int step;
	int data;

	int old_m;
	int now_leveling;
	int delta_next;

	ch->now_step += ch->step;
	if ( ch->now_step >= (1<<ADPCM_SHIFT) )
	{
		step = ch->now_step >> ADPCM_SHIFT;
		ch->now_step &= (1<<ADPCM_SHIFT)-1;
		do{
			if ( ch->now_addr > (ch->end<<1) ) {
				if( F2610->port0state&0x10 ){
					/**** repeat start ****/
					ch->now_addr = ch->start<<1;
					/*ch->adpcmm   = 0;*/
					ch->adpcmx   = 0;
					/* ch->adpcml   = 0; */
					ch->adpcmd   = ADPCMB_DELTA_DEF;
					ch->next_leveling = 0;
					ch->flag     = 1;
				}else{
					F2610->adpcm_arrivedEndAddress |= ch->flagMask & F2610->adpcm_statusmask;
					ch->flag = 0;
					ch->adpcml = 0;
					now_leveling = 0;
					return;
				}
			}
#if 0
			if ( ch->now_addr > (pcmsizeB<<1) ) {
				Log(LOG_WAR,"YM2610: Attempting to play past Delta T rom size!\n" );
				return;
			}
#endif
			if( ch->now_addr&1 ) data = ch->now_data & 0x0f;
			else
			{
				ch->now_data = *(pcmbufB+(ch->now_addr>>1));
				data = ch->now_data >> 4;
			}
			ch->now_addr++;
			/* shift Measurement value */
			old_m      = ch->adpcmx/*adpcmm*/;
			/* ch->adpcmm = Limit( ch->adpcmx + (decode_tableB3[data] * ch->adpcmd / 8) ,ADPCMB_DECODE_MAX, ADPCMB_DECODE_MIN ); */
			/* Forecast to next Forecast */
			ch->adpcmx = Limit( ch->adpcmx+(decode_tableB1[data] * ch->adpcmd / 8) ,ADPCMB_DECODE_MAX, ADPCMB_DECODE_MIN );
			/* delta to next delta */
			ch->adpcmd = Limit( ( ch->adpcmd * decode_tableB2[data] ) / 64, ADPCMB_DELTA_MAX, ADPCMB_DELTA_MIN );
			/* shift leveling value */
			delta_next        = ch->adpcmx/*adpcmm*/ - old_m;
			now_leveling      = ch->next_leveling;
			ch->next_leveling = old_m + (delta_next / 2);
		}while(--step);
//#define CUT_RE_SAMPLING
#ifdef CUT_RE_SAMPLING
		ch->adpcml  = ch->next_leveling * ch->volume;
		ch->adpcml  = ch->adpcmx/*adpcmm*/ * ch->volume;
	}
#else
		/* delta step of re-sampling */
		ch->sample_step = (ch->next_leveling - now_leveling) * ch->volume_w_step;
		/* output of start point */
		ch->adpcml  = now_leveling * ch->volume;
		/* adjust to now */
		ch->adpcml += (int)((double)ch->sample_step * ((double)ch->now_step/(double)ch->step));
	}
	ch->adpcml += ch->sample_step;
#endif
	/* output for work of output channels (outd[OPNxxxx])*/
	//*(ch->pan) += ch->adpcml;
	*(ch->pan) += ch->adpcml;
}

static YM2610 *FM2610=NULL;	/* array of YM2610's */

/* ADPCM type A */
static void FM_ADPCMAWrite(YM2610 *F2610,int r,int v)
{
	ADPCM_CH *adpcm = F2610->adpcm;
	unsigned char c = r&0x07;

	F2610->adpcmreg[1][r] = v&0xff; /* stock data */
	switch( r ){
	case 0x00: /* DM,--,C5,C4,C3,C2,C1,C0 */
	  F2610->port1state = v&0xff;
	  if( !(v&0x80) ){
	    /* KEY ON */
	    for( c = 0; c < 6; c++ ){
	      if( (1<<c)&v ){
		/**** start adpcm ****/
		adpcm[c].step     = (unsigned int)((float)(1<<ADPCM_SHIFT)*((float)F2610->OPN.ST.freqbase)/4096.0/3.0);
		adpcm[c].now_addr = adpcm[c].start<<1;
		adpcm[c].now_step = (1<<ADPCM_SHIFT)-adpcm[c].step;
		/*adpcm[c].adpcmm   = 0;*/
		adpcm[c].adpcmx   = 0;
		adpcm[c].adpcmd   = 0;
		adpcm[c].adpcml   = 0;
		adpcm[c].flag     = 1;
		if(F2610->pcmbuf[1]==NULL){			// Check ROM Mapped
#ifdef __RAINE__
		  PrintDebug("YM2610: main adpcm rom not mapped\n");
#else
		  Log(LOG_WAR,"YM2610: Attempting to play regular adpcm but no rom is mapped\n");
#endif
		  adpcm[c].flag = 0;
		} else{
		  if(adpcm[c].end >= F2610->pcm_size[1]){		// Check End in Range
#ifdef __RAINE__
		    PrintDebug("YM2610: main adpcm end out of range: $%08x\n",adpcm[c].end);
#endif
		    adpcm[c].end = F2610->pcm_size[1]-1;
		  }
		  if(adpcm[c].start >= F2610->pcm_size[1]){	// Check Start in Range
#ifdef __RAINE__
		    PrintDebug("YM2610: main adpcm start out of range: $%08x\n",adpcm[c].start);
#endif
		    adpcm[c].flag = 0;
		  }
		}
		/*** (1<<c)&v ***/
	      }
	      /**** for loop ****/
	    }
	  } else{
	    /* KEY OFF */
	    for( c = 0; c < 6; c++ ){
	      if( (1<<c)&v )  adpcm[c].flag = 0;
	    }
	  }
	  break;
	case 0x01:	/* B0-5 = TL 0.75dB step */
		F2610->TL_adpcmb = &(TL_TABLE[((v&0x3f)^0x3f)*(int)(0.75/EG_STEP)]);
		for( c = 0; c < 6; c++ ){
		  adpcm[c].volume = F2610->TL_adpcmb[adpcm[c].IL*(int)(0.75/EG_STEP)] / ADPCMA_DECODE_RANGE / ADPCMA_VOLUME_DIV;
		  /**** calc pcm * volume data ****/
		  adpcm[c].adpcml = adpcm[c].adpcmx * adpcm[c].volume;
		}
		break;
	default:
		c = r&0x07;
		if( c >= 0x06 ) return;
		switch( r&0x38 ){
		case 0x08:	/* B7=L,B6=R,B4-0=IL */
			adpcm[c].IL = (v&0x1f)^0x1f;
			adpcm[c].volume = F2610->TL_adpcmb[adpcm[c].IL*(int)(0.75/EG_STEP)] / ADPCMA_DECODE_RANGE / ADPCMA_VOLUME_DIV;
			adpcm[c].pan    = &outd[(v>>6)&0x03];
			/**** calc pcm * volume data ****/
			adpcm[c].adpcml = adpcm[c].adpcmx * adpcm[c].volume;
			break;
		case 0x10:
		case 0x18:
			adpcm[c].start  = ( (F2610->adpcmreg[1][0x18 + c]*0x0100 | F2610->adpcmreg[1][0x10 + c]) << F2610->port1shift);
			break;
		case 0x20:
		case 0x28:
			adpcm[c].end    = ( (F2610->adpcmreg[1][0x28 + c]*0x0100 | F2610->adpcmreg[1][0x20 + c]) << F2610->port1shift);
			adpcm[c].end   += (1<<F2610->port1shift) - 1;
			break;
		}
	}
}

/* ADPCM type B (DELTA-T) */
static void FM_ADPCMBWrite(YM2610 *F2610,int r,int v)
{
	ADPCM_CH *adpcm = &(F2610->adpcm[6]);

	F2610->adpcmreg[0][r] = v&0xff; /* stock data */
	switch( r ){
	case 0x00:	/* START,REC,MEMDATA,REPEAT,SPOFF,--,--,RESET */
#if 0
		case 0x60:	/* write buffer MEMORY from PCM data port */
		case 0x20:	/* read  buffer MEMORY to   PCM data port */
#endif
	  if( v&0x80 ){
		F2610->port0state = v&0x90; /* start req,memory mode,repeat flag copy */
		/**** start ADPCM ****/
		adpcm->volume_w_step = (double)adpcm->volume * adpcm->step / (1<<ADPCM_SHIFT);
		adpcm->now_addr = (adpcm->start)<<1;
		adpcm->now_step = (1<<ADPCM_SHIFT)-adpcm->step;
		/*adpcm->adpcmm   = 0;*/
		adpcm->adpcmx   = 0;
		adpcm->adpcml   = 0;
		adpcm->adpcmd   = ADPCMB_DELTA_DEF;
		adpcm->next_leveling=0;
		adpcm->flag     = 1; /* start ADPCM */
	    if( !adpcm->step ){
	      adpcm->flag = 0;
	      F2610->port0state = 0x00;
	    }
	    /**** PCMROM check & limit check ****/
	    if(F2610->pcmbuf[0] == NULL){			// Check ROM Mapped
#ifdef __RAINE__
	      PrintDebug("YM2610: Delta-T adpcm rom not mapped\n");
#endif
	      adpcm->flag = 0;
	      F2610->port0state = 0x00;
	    } else{
	      if( adpcm->end >= F2610->pcm_size[0] ){		// Check End in Range
#ifdef __RAINE__
		PrintDebug("YM2610: Delta-T adpcm end out of range: $%08x\n",adpcm->end);
#endif
		adpcm->end = F2610->pcm_size[0] - 1;
	      }
	      if( adpcm->start >= F2610->pcm_size[0] ){		// Check Start in Range
#ifdef __RAINE__
		PrintDebug("YM2610: Delta-T adpcm start out of range: $%08x\n",adpcm->start);
#endif
		adpcm->flag = 0;
		F2610->port0state = 0x00;
	      }
	    }
	  } else if( v&0x01 ){
		adpcm->flag = 0;
	    F2610->port0state = 0x00;
	  }
	  break;
	case 0x01:	/* L,R,-,-,SAMPLE,DA/AD,RAMTYPE,ROM */
	  F2610->port0control = v&0xff;
	  adpcm->pan = &outd[(v>>6)&0x03];
	  break;
	case 0x02:	/* Start Address L */
	case 0x03:	/* Start Address H */
	  adpcm->start  = (F2610->adpcmreg[0][0x3]*0x0100 | F2610->adpcmreg[0][0x2]) << F2610->port0shift;
		break;
	case 0x04:	/* Stop Address L */
	case 0x05:	/* Stop Address H */
		adpcm->end    = (F2610->adpcmreg[0][0x5]*0x0100 | F2610->adpcmreg[0][0x4]) << F2610->port0shift;
		adpcm->end   += (1<<F2610->port0shift) - 1;
		break;
	case 0x06:	/* Prescale L (PCM and Recoard frq) */
	case 0x07:	/* Proscale H */
	case 0x08:	/* ADPCM data */
	  break;
	case 0x09:	/* DELTA-N L (ADPCM Playback Prescaler) */
	case 0x0a:	/* DELTA-N H */
		adpcm->delta  = (F2610->adpcmreg[0][0xa]*0x0100 | F2610->adpcmreg[0][0x9]);
		adpcm->step     = (unsigned int)((float)(adpcm->delta*(1<<(ADPCM_SHIFT-16)))*((float)F2610->OPN.ST.freqbase)/4096.0);
		adpcm->volume_w_step = (double)adpcm->volume * adpcm->step / (1<<ADPCM_SHIFT);
		break;
	case 0x0b:	/* Level control (volume , voltage flat) */
		{
			int oldvol = adpcm->volume;
			adpcm->volume = ((v&0xff)<<(TL_BITS-8)) * ADPCMB_VOLUME_RATE / ADPCMB_DECODE_RANGE;
			if( oldvol != 0 )
			{
				adpcm->adpcml      = (int)((double)adpcm->adpcml      / (double)oldvol * (double)adpcm->volume);
				adpcm->sample_step = (int)((double)adpcm->sample_step / (double)oldvol * (double)adpcm->volume);
			}
			adpcm->volume_w_step = (int)((double)adpcm->volume * (double)adpcm->step / (double)(1<<ADPCM_SHIFT));
		}
		break;
	}
}

#endif /* BUILD_FM_ADPCM */


#if BUILD_YM2608
/*******************************************************************************/
/*		YM2608 local section                                                   */
/*******************************************************************************/
static YM2608 *FM2608=NULL;	/* array of YM2608's */

#if 0
/* Get next pcm data */
INLINE int YM2608ReadADPCM(int n)
{
	YM2608 *F2608 = &(FM2608[n]);
	if( F2608->ADMode & 0x20 )
	{	/* buffer memory */
		/* F2203->OPN.ST.status |= 0x04; */
		return 0;
	}
	else
	{	/* from PCM data register */
		FM_STATUS_SET(F2608->OPN.ST,0x08); /* BRDY = 1 */
		return F2608->ADData;
	}
}

/* Put decoded data */
INLINE void YM2608WriteADPCM(int n,int v)
{
	YM2608 *F2608 = &(FM2608[n]);
	if( F2608->ADMode & 0x20 )
	{	/* for buffer */
		return;
	}
	else
	{	/* for PCM data port */
		F2608->ADData = v;
		FM_STATUS_SET(F2608->OPN.ST,0x08) /* BRDY = 1 */
	}
}
#endif

/* ---------- IRQ flag Controll Write 0x110 ---------- */
INLINE void YM2608IRQFlagWrite(FM_ST *ST,int n,int v)
{
	if( v & 0x80 )
	{	/* Reset IRQ flag */
		FM_STATUS_RESET(ST,0xff);
	}
	else
	{	/* Set IRQ mask */
		/* !!!!!!!!!! pending !!!!!!!!!! */
		/* F2610->adpcm_statusmask = v & 0x1f; */
	}
}

#ifdef YM2608_RHYTHM_PCM
/**** RYTHM (PCM) ****/
INLINE void YM2608_RYTHM( YM2610 *F2610, ADPCM_CH *ch )

{
	unsigned int step;

	ch->now_step += ch->step;
	if ( ch->now_step >= (1<<ADPCM_SHIFT) )
	{
		step = ch->now_step >> ADPCM_SHIFT;
		ch->now_step &= (1<<ADPCM_SHIFT)-1;
		/* end check */
		if ( (ch->now_addr+step) > (ch->end<<1) ) {
			ch->flag = 0;
			F2610->adpcm_arrivedEndAddress |= ch->flagMask & F2610->adpcm_statusmask;
			return;
		}
		do{
			/* get a next pcm data */
			ch->adpcmx = ((short *)pcmbufA)[ch->now_addr];
			ch->now_addr++;
			/**** calc pcm * volume data ****/
			ch->adpcml = ch->adpcmx * ch->volume;
		}while(--step);
	}
	/* output for work of output channels (outd[OPNxxxx])*/
	*(ch->pan) += ch->adpcml;
}
#endif /* YM2608_RHYTHM_PCM */

/* ---------- update one of chip ----------- */
void YM2608UpdateOne(int num, void **buffer, int length)
{
	YM2608 *F2608 = &(FM2608[num]);
	FM_OPN *OPN   = &(FM2608[num].OPN);
	int dataR,dataL;
	int i,j;

	/* set bufer */
	bufL = (FMSAMPLE *)buffer[0];
	bufR = (FMSAMPLE *)buffer[1];

	if( (void *)F2608 != cur_chip ){
		cur_chip = (void *)F2608;

		State = &OPN->ST;
		cch[0]   = &F2608->CH[0];
		cch[1]   = &F2608->CH[1];
		cch[2]   = &F2608->CH[2];
		cch[3]   = &F2608->CH[3];
		cch[4]   = &F2608->CH[4];
		cch[5]   = &F2608->CH[5];
		/* setup adpcm rom address */
		pcmbufB  = F2608->pcmbuf[0];
		pcmsizeB = F2608->pcm_size[0];
		pcmbufA  = F2608->pcmbuf[1];
		pcmsizeA = F2608->pcm_size[1];
	}
	/* update frequency counter */
	CALC_FCOUNT( cch[0] );
	CALC_FCOUNT( cch[1] );
	if( (State->mode & 0xc0) ){
		/* 3SLOT MODE */
		if( cch[2]->SLOT[SLOT1].Incr==-1){
			/* 3 slot mode */
			CALC_FCSLOT(&cch[2]->SLOT[SLOT1] , OPN->SL3.fc[1] , OPN->SL3.kcode[1] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT2] , OPN->SL3.fc[2] , OPN->SL3.kcode[2] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT3] , OPN->SL3.fc[0] , OPN->SL3.kcode[0] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT4] , cch[2]->fc , cch[2]->kcode );
		}
	}else CALC_FCOUNT( cch[2] );
	CALC_FCOUNT( cch[3] );
	CALC_FCOUNT( cch[4] );
	CALC_FCOUNT( cch[5] );
	/* buffering */
    for( i=0; i < length ; i++ )
	{
		/* clear output acc. */
		outd[OPN_LEFT] = outd[OPN_RIGHT]= outd[OPN_CENTER] = 0;
		/**** deltaT ADPCM ****/
		if( F2608->adpcm[6].flag )
			OPNB_ADPCM_CALC_CHB( F2608, &F2608->adpcm[6]);
		/* FM */
		FM_CALC_CH( cch[0] );
		FM_CALC_CH( cch[1] );
		FM_CALC_CH( cch[2] );
		FM_CALC_CH( cch[3] );
		FM_CALC_CH( cch[4] );
		FM_CALC_CH( cch[5] );
		for( j = 0; j < 6; j++ )
		{
			/**** ADPCM ****/
			if( F2608->adpcm[j].flag )
#ifdef YM2608_RHYTHM_PCM
				YM2608_RYTHM(F2608, &F2608->adpcm[j]);
#else
				OPNB_ADPCM_CALC_CHA( F2608, &F2608->adpcm[j]);
#endif
		}
		/* get left & right output with clipping */
		dataL = Limit( outd[OPN_CENTER] + outd[OPN_LEFT], OPNB_MAXOUT, OPNB_MINOUT );
		dataR = Limit( outd[OPN_CENTER] + outd[OPN_RIGHT], OPNB_MAXOUT, OPNB_MINOUT );
		/* buffering */
		/* stereo separate */
#ifdef FM_STEREO_MIX		/* stereo mixing */
		/* stereo mix */
		((FMSAMPLE_MIX *)bufL)[i] = ((dataL>>OPNB_OUTSB)<<FM_OUTPUT_BIT)|(dataR>>OPNB_OUTSB);
#else
		/* stereo separate */
		bufL[i] = dataL>>OPNB_OUTSB;
		bufR[i] = dataR>>OPNB_OUTSB;
#endif

#ifdef LFO_SUPPORT
		CALC_LOPM_LFO;
#endif
#ifdef INTERNAL_TIMER
		/* timer controll */
		CALC_TIMER_A( State , cch[2] );
#endif
	}
#ifdef INTERNAL_TIMER
	CALC_TIMER_B( State , length );
#endif
}

/* -------------------------- YM2608(OPNA) ---------------------------------- */
int YM2608Init(int num, int clock, int rate,
               void **pcmroma,int *pcmsizea,short *rhythmrom,int *rhythmpos,
               FM_TIMERHANDLER TimerHandler,FM_IRQHANDLER IRQHandler)
{
	int i,j;

    if (FM2608) return (-1);	/* duplicate init. */
    cur_chip = NULL;	/* hiro-shi!! */

	FMNumChips = num;

	/* allocate extend state space */
	if( (FM2608 = (YM2608 *)malloc(sizeof(YM2608) * FMNumChips))==NULL)
		return (-1);
	/* clear */
	memset(FM2608,0,sizeof(YM2608) * FMNumChips);
	/* allocate total level table (128kb space) */
	if( !FMInitTable() )
	{
		free( FM2608 );
		return (-1);
	}

	for ( i = 0 ; i < FMNumChips; i++ ) {
		FM2608[i].OPN.ST.index = i;
		FM2608[i].OPN.type = TYPE_YM2608;
		FM2608[i].OPN.P_CH = FM2608[i].CH;
		FM2608[i].OPN.ST.clock = clock;
		FM2608[i].OPN.ST.rate = rate;
		/* FM2608[i].OPN.ST.irq = 0; */
		/* FM2608[i].OPN.ST.status = 0; */
		FM2608[i].OPN.ST.timermodel = FM_TIMER_SINGLE;
		/* Extend handler */
		FM2608[i].OPN.ST.Timer_Handler = TimerHandler;
		FM2608[i].OPN.ST.IRQ_Handler   = IRQHandler;
		/* ADPCM */
		FM2608[i].pcmbuf[0]   = (char *)(pcmroma[i]);
		FM2608[i].pcm_size[0] = pcmsizea[i];
		FM2608[i].pcmbuf[1]   = (char *)rhythmrom;
#ifdef YM2608_RHYTHM_PCM
		/* rhythm sound setup (PCM) */
		for(j=0;j<6;j++)
		{
			/* rhythm sound */
			FM2608[i].adpcm[j].start = rhythmpos[j];
			FM2608[i].adpcm[j].end   = rhythmpos[j+1]-1;
		}
		FM2608[i].pcm_size[1] = rhythmpos[6];
#else
		/* rhythm sound setup (ADPCM) */
		FM2608[i].pcm_size[1] = rhythmsize;
#endif
		YM2608ResetChip(i);
	}
	InitOPNB_ADPCMATable();
	return 0;
}

/* ---------- shut down emurator ----------- */
void YM2608Shutdown()
{
    if (!FM2608) return;

	FMCloseTable();
	free(FM2608);
	FM2608 = NULL;
}

/* ---------- reset one of chip ---------- */
void YM2608ResetChip(int num)
{
	int i;
	YM2608 *F2608 = &(FM2608[num]);
	FM_OPN *OPN   = &(FM2608[num].OPN);

	/* Reset Priscaler */
	OPNSetPris( OPN, 6*24, 6*24,4*2); /* OPN 1/6 , SSG 1/4 */
	/* reset SSG section */
	SSGReset(OPN->ST.index);
	/* status clear */
	FM_IRQMASK_SET(&OPN->ST,0x1f);
	OPNWriteMode(OPN,0x27,0x30); /* mode 0 , timer reset */

	/* extend 3ch. disable */
	//OPN->type &= (~TYPE_6CH);

	reset_channel( &OPN->ST , F2608->CH , 6 );
	/* reset OPerator paramater */
	for(i = 0xb6 ; i >= 0xb4 ; i-- )
	{
		OPNWriteReg(OPN,i      ,0xc0);
		OPNWriteReg(OPN,i|0x100,0xc0);
	}
	for(i = 0xb2 ; i >= 0x30 ; i-- )
	{
		OPNWriteReg(OPN,i      ,0);
		OPNWriteReg(OPN,i|0x100,0);
	}
	for(i = 0x26 ; i >= 0x20 ; i-- ) OPNWriteReg(OPN,i,0);
	/* reset ADPCM unit */
	/**** ADPCM work initial ****/
	for( i = 0; i < 6+1; i++ ){
		F2608->adpcm[i].now_addr  = 0;
		F2608->adpcm[i].now_step  = 0;
		F2608->adpcm[i].step      = 0;
		F2608->adpcm[i].start     = 0;
		F2608->adpcm[i].end       = 0;
		/* F2608->adpcm[i].delta     = 21866; */
		F2608->adpcm[i].volume    = 0;
		F2608->adpcm[i].pan       = &outd[OPN_CENTER]; /* default center */
		F2608->adpcm[i].flagMask  = (i == 6) ? 0x20 : 0;
		F2608->adpcm[i].flag      = 0;
		F2608->adpcm[i].adpcmx    = 0;
		F2608->adpcm[i].adpcmd    = 127;
		F2608->adpcm[i].adpcml    = 0;
		/* DELTA-T */
		/*F2608->adpcm[i].adpcmm    = 0;*/
		F2608->adpcm[i].volume_w_step = 0;
	    F2608->adpcm[i].next_leveling=0;
	}
	F2608->TL_adpcmb = &(TL_TABLE[0x3f*(int)(0.75/EG_STEP)]);
	F2608->port0state = 0;
	F2608->port0shift = 8;		/* allways 8bits shift */
	//F2608->port1state = 0;
	F2608->port1state = -1;
	F2608->port1shift = 8;		/* allways 8bits shift */
	F2608->adpcm_arrivedEndAddress = 0; /* don't used */
	F2608->adpcm_statusmask = 0xbf;     /* don't used */
}

/* YM2608 write */
/* n = number  */
/* a = address */
/* v = value   */
int YM2608Write(int n, int a,int v)
{
	YM2608 *F2608 = &(FM2608[n]);
	FM_OPN *OPN   = &(FM2608[n].OPN);
	int addr;

	switch(a&3){
	case 0:	/* address port 0 */
		OPN->ST.address = v & 0xff;
		/* Write register to SSG emurator */
		if( v < 16 ) SSGWrite(n,0,v);
		switch(OPN->ST.address)
		{
		case 0x2d:	/* divider sel */
			OPNSetPris( OPN, 6*24, 6*24, 4*2); /* OPN 1/6 , SSG 1/4 */
			break;
		case 0x2e:	/* divider sel */
			OPNSetPris( OPN, 3*24, 3*24,2*2); /* OPN 1/3 , SSG 1/2 */
			break;
		case 0x2f:	/* divider sel */
			OPNSetPris( OPN, 2*24, 2*24,1*2); /* OPN 1/2 , SSG 1/1 */
			break;
		}
		break;
	case 1:	/* data port 0    */
		addr = OPN->ST.address;
		switch(addr & 0xf0)
		{
		case 0x00:	/* SSG section */
			/* Write data to SSG emurator */
			SSGWrite(n,a,v);
			break;
		case 0x10:	/* 0x10-0x1f : Rhythm section */
			YM2608UpdateReq(n);
			FM_ADPCMAWrite(F2608,addr-0x10,v);
			break;
		case 0x20:	/* Mode Register */
			switch(addr)
			{
			case 0x29: /* SCH,xirq mask */
				/* SCH,xx,xxx,EN_ZERO,EN_BRDY,EN_EOS,EN_TB,EN_TA */
				/* extend 3ch. enable/disable */
				if(v&0x80) OPN->type |= TYPE_6CH;
				else       OPN->type &= ~TYPE_6CH;
				/* IRQ MASK */
				FM_IRQMASK_SET(&OPN->ST,v&0x1f);
				break;
			default:
				YM2608UpdateReq(n);
				OPNWriteMode(OPN,addr,v);
			}
			break;
		default:	/* OPN section */
			YM2608UpdateReq(n);
			OPNWriteReg(OPN,addr,v);
		}
		break;
	case 2:	/* address port 1 */
		F2608->address1 = v & 0xff;
		break;
	case 3:	/* data port 1    */
		addr = F2608->address1;
		YM2608UpdateReq(n);
		switch( addr & 0xf0 )
		{
		case 0x00:	/* ADPCM PORT */
			switch( addr )
			{
			case 0x0c:	/* Limit address L */
				//F2608->ADLimit = (F2608->ADLimit & 0xff00) | v;
				//break;
			case 0x0d:	/* Limit address H */
				//F2608->ADLimit = (F2608->ADLimit & 0x00ff) | (v<<8);
				//break;
			case 0x0e:	/* DAC data */
				//break;
			case 0x0f:	/* PCM data port */
				//F2608->ADData = v;
				//FM_STATUS_RESET(F2608->OPN.ST,0x08);
				break;
			default:
				/* 0x00-0x0b */
				FM_ADPCMBWrite(F2608,addr,v);
			}
			break;
		case 0x10:	/* IRQ Flag controll */
			if( addr == 0x10 )
				YM2608IRQFlagWrite(&(OPN->ST),n,v);
			break;
		default:
			OPNWriteReg(OPN,addr|0x100,v);
		}
	}
	return OPN->ST.irq;
}
unsigned char YM2608Read(int n,int a)
{
	YM2608 *F2608 = &(FM2608[n]);
	int addr = F2608->OPN.ST.address;
	int ret = 0;

	switch( a&3 ){
	case 0:	/* status 0 : YM2203 compatible */
		/* BUSY:x:x:x:x:x:FLAGB:FLAGA */
		if(addr==0xff) ret = 0x00; /* ID code */
		else ret = F2608->OPN.ST.status & 0x83;
		break;
	case 1:	/* status 0 */
		if( addr < 16 ) ret = SSGRead(n);
		break;
	case 2:	/* status 1 : + ADPCM status */
		/* BUSY:x:PCMBUSY:ZERO:BRDY:EOS:FLAGB:FLAGA */
		if(addr==0xff) ret = 0x00; /* ID code */
		else ret = F2608->OPN.ST.status | (F2608->adpcm[6].flag ? 0x20 : 0);
		break;
	case 3:
		ret = 0;
		break;
	}
	return ret;
}

int YM2608TimerOver(int n,int c)
{
	YM2608 *F2608 = &(FM2608[n]);

	if( c )
	{	/* Timer B */
		TimerBOver( &(F2608->OPN.ST) );
	}
	else
	{	/* Timer A */
		YM2608UpdateReq(n);
		/* timer update */
		TimerAOver( &(F2608->OPN.ST) );
		/* CSM mode key,TL controll */
		if( F2608->OPN.ST.mode & 0x80 )
		{	/* CSM mode total level latch and auto key on */
			CSMKeyControll( &(F2608->CH[2]) );
		}
	}
	return FM2608->OPN.ST.irq;
}

#if 0
/* ---------- return the buffer ---------- */
FMSAMPLE **YM2608Buffer(int n)
{
    return &(FM2608[n].Buf);
}
#endif

#if 0
/* ---------- set buffer ---------- */
int YM2608SetBuffer(int n, FMSAMPLE **buf )
{
	int i;
	for( i = 0 ; i < YM2608_NUMBUF ; i++){
		FM2608[n].Buf[i] = buf[i];
		if( cur_chip == &FM2608[n] ) cur_chip = NULL;
	}
	return 0;
}
#endif

#endif /* BUILD_YM2608 */

#if BUILD_YM2610
/* -------------------------- YM2610(OPNB) ---------------------------------- */
//static YM2610 *FM2610=NULL;	/* array of YM2610's */

/* ---------- update one of chip (YM2610B FM6: ADPCM-A6: ADPCM-B:1) ----------- */
void YM2610UpdateOne(int num, void **buffer, int length)
{
	YM2610 *F2610 = &(FM2610[num]);
	FM_OPN *OPN   = &(FM2610[num].OPN);
	int dataR,dataL;
	int i,j;

	/* buffer setup */
	bufL = (FMSAMPLE *)buffer[0];
	bufR = (FMSAMPLE *)buffer[1];

	if( (void *)F2610 != cur_chip ){
		cur_chip = (void *)F2610;
		State = &OPN->ST;
		//cch[0] = &F2610->CH[0];
		cch[1] = &F2610->CH[1];
		cch[2] = &F2610->CH[2];
		//cch[3] = &F2610->CH[3];
		cch[4] = &F2610->CH[4];
		cch[5] = &F2610->CH[5];
		/* setup adpcm rom address */
		pcmbufB  = F2610->pcmbuf[0];
		pcmsizeB = F2610->pcm_size[0];
		pcmbufA  = F2610->pcmbuf[1];
		pcmsizeA = F2610->pcm_size[1];
	}
#ifdef YM2610B_WARNING
#define FM_MSG_YM2610B "YM2610-%d.CH%d is playing,Check whether the type of the chip is YM2610B\n"
	/* Check YM2610B worning message */
	if(errorlog)
	{
		if( F2610->CH[0].SLOT[3].evm > ENV_MOD_OFF )
			Log(LOG_WAR,FM_MSG_YM2610B,num,0);
		if( F2610->CH[3].SLOT[3].evm > ENV_MOD_OFF )
			Log(LOG_WAR,FM_MSG_YM2610B,num,3);
	}
#endif
	/* update frequency counter */
	//CALC_FCOUNT( cch[0] );
	CALC_FCOUNT( cch[1] );
	if( (State->mode & 0xc0) ){
		/* 3SLOT MODE */
		if( cch[2]->SLOT[SLOT1].Incr==-1){
			/* 3 slot mode */
			CALC_FCSLOT(&cch[2]->SLOT[SLOT1] , OPN->SL3.fc[1] , OPN->SL3.kcode[1] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT2] , OPN->SL3.fc[2] , OPN->SL3.kcode[2] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT3] , OPN->SL3.fc[0] , OPN->SL3.kcode[0] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT4] , cch[2]->fc , cch[2]->kcode );
		}
	}else CALC_FCOUNT( cch[2] );
	//CALC_FCOUNT( cch[3] );
	CALC_FCOUNT( cch[4] );
	CALC_FCOUNT( cch[5] );

	/* buffering */
    for( i=0; i < length ; i++ )
	{
		/* clear output acc. */
		outd[OPN_LEFT] = outd[OPN_RIGHT]= outd[OPN_CENTER] = 0;
		/**** deltaT ADPCM ****/
		if( F2610->adpcm[6].flag )
			OPNB_ADPCM_CALC_CHB( F2610, &F2610->adpcm[6]);
		/* FM */
		//FM_CALC_CH( cch[0] );
		FM_CALC_CH( cch[1] );
		FM_CALC_CH( cch[2] );
		//FM_CALC_CH( cch[3] );
		FM_CALC_CH( cch[4] );
		FM_CALC_CH( cch[5] );
		for( j = 0; j < 6; j++ )
		{
			/**** ADPCM ****/
			if( F2610->adpcm[j].flag )
				OPNB_ADPCM_CALC_CHA( F2610, &F2610->adpcm[j]);
		}
		/* get left & right output with clipping */
		dataL = Limit( outd[OPN_CENTER] + outd[OPN_LEFT], OPNB_MAXOUT, OPNB_MINOUT );
		dataR = Limit( outd[OPN_CENTER] + outd[OPN_RIGHT], OPNB_MAXOUT, OPNB_MINOUT );
		/* buffering */
#ifdef FM_STEREO_MIX		/* stereo mixing */
		/* stereo mix */
		((FMSAMPLE_MIX *)bufL)[i] = ((dataL>>OPNB_OUTSB)<<FM_OUTPUT_BIT)|(dataR>>OPNB_OUTSB);
#else
		/* stereo separate */
		bufL[i] = dataL>>OPNB_OUTSB;
		bufR[i] = dataR>>OPNB_OUTSB;
#endif

#ifdef LFO_SUPPORT
		CALC_LOPM_LFO;
#endif
#ifdef INTERNAL_TIMER
		/* timer controll */
		CALC_TIMER_A( State , cch[2] );
#endif
	}
#ifdef INTERNAL_TIMER
	CALC_TIMER_B( State , length );
#endif
}
#endif /* BUILD_YM2610 */

#if BUILD_YM2610B
/* ---------- update one of chip (YM2610B FM6: ADPCM-A6: ADPCM-B:1) ----------- */
void YM2610BUpdateOne(int num, void **buffer, int length)
{
	YM2610 *F2610 = &(FM2610[num]);
	FM_OPN *OPN   = &(FM2610[num].OPN);
	int dataR,dataL;
	int i,j;

	/* buffer setup */
	bufL = (FMSAMPLE *)buffer[0];
	bufR = (FMSAMPLE *)buffer[1];

	if( (void *)F2610 != cur_chip ){
		cur_chip = (void *)F2610;
		State = &OPN->ST;
		cch[0] = &F2610->CH[0];
		cch[1] = &F2610->CH[1];
		cch[2] = &F2610->CH[2];
		cch[3] = &F2610->CH[3];
		cch[4] = &F2610->CH[4];
		cch[5] = &F2610->CH[5];
		/* setup adpcm rom address */
		pcmbufB  = F2610->pcmbuf[0];
		pcmsizeB = F2610->pcm_size[0];
		pcmbufA  = F2610->pcmbuf[1];
		pcmsizeA = F2610->pcm_size[1];
	}

	/* update frequency counter */
	CALC_FCOUNT( cch[0] );
	CALC_FCOUNT( cch[1] );
	if( (State->mode & 0xc0) ){
		/* 3SLOT MODE */
		if( cch[2]->SLOT[SLOT1].Incr==-1){
			/* 3 slot mode */
			CALC_FCSLOT(&cch[2]->SLOT[SLOT1] , OPN->SL3.fc[1] , OPN->SL3.kcode[1] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT2] , OPN->SL3.fc[2] , OPN->SL3.kcode[2] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT3] , OPN->SL3.fc[0] , OPN->SL3.kcode[0] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT4] , cch[2]->fc , cch[2]->kcode );
		}
	}else CALC_FCOUNT( cch[2] );
	CALC_FCOUNT( cch[3] );
	CALC_FCOUNT( cch[4] );
	CALC_FCOUNT( cch[5] );

	/* buffering */
    for( i=0; i < length ; i++ )
	{
		/* clear output acc. */
		outd[OPN_LEFT] = outd[OPN_RIGHT]= outd[OPN_CENTER] = 0;
		/**** deltaT ADPCM ****/
		if( F2610->adpcm[6].flag )
			OPNB_ADPCM_CALC_CHB( F2610, &F2610->adpcm[6]);
		/* FM */
		FM_CALC_CH( cch[0] );
		FM_CALC_CH( cch[1] );
		FM_CALC_CH( cch[2] );
		FM_CALC_CH( cch[3] );
		FM_CALC_CH( cch[4] );
		FM_CALC_CH( cch[5] );
		for( j = 0; j < 6; j++ )
		{
			/**** ADPCM ****/
			if( F2610->adpcm[j].flag )
				OPNB_ADPCM_CALC_CHA( F2610, &F2610->adpcm[j]);
		}
		/* get left & right output with clipping */
		dataL = Limit( outd[OPN_CENTER] + outd[OPN_LEFT], OPNB_MAXOUT, OPNB_MINOUT );
		dataR = Limit( outd[OPN_CENTER] + outd[OPN_RIGHT], OPNB_MAXOUT, OPNB_MINOUT );
		/* buffering */
		/* stereo separate */
#ifdef FM_STEREO_MIX		/* stereo mixing */
		/* stereo mix */
		((FMSAMPLE_MIX *)bufL)[i] = ((dataL>>OPNB_OUTSB)<<FM_OUTPUT_BIT)|(dataR>>OPNB_OUTSB);
#else
		/* stereo separate */
		bufL[i] = dataL>>OPNB_OUTSB;
		bufR[i] = dataR>>OPNB_OUTSB;
#endif

#ifdef LFO_SUPPORT
		CALC_LOPM_LFO;
#endif
#ifdef INTERNAL_TIMER
		/* timer controll */
		CALC_TIMER_A( State , cch[2] );
#endif
	}
#ifdef INTERNAL_TIMER
	CALC_TIMER_B( State , length );
#endif
}
#endif /* BUILD_YM2610B */

#if BUILD_OPNB
int YM2610Init(int num, int clock, int rate,
               void **pcmroma,int *pcmsizea,void **pcmromb,int *pcmsizeb,
               FM_TIMERHANDLER TimerHandler,FM_IRQHANDLER IRQHandler)

{
	int i;

    if (FM2610) return (-1);	/* duplicate init. */
    cur_chip = NULL;	/* hiro-shi!! */

	FMNumChips = num;

	/* allocate extend state space */
	if( (FM2610 = (YM2610 *)malloc(sizeof(YM2610) * FMNumChips))==NULL)
		return (-1);
	/* clear */
	memset(FM2610,0,sizeof(YM2610) * FMNumChips);
	/* allocate total level table (128kb space) */
	if( !FMInitTable() )
	{
		free( FM2610 );
		return (-1);
	}

	for ( i = 0 ; i < FMNumChips; i++ ) {
		/* FM */
		FM2610[i].OPN.ST.index = i;
		FM2610[i].OPN.type = TYPE_YM2610;
		FM2610[i].OPN.P_CH = FM2610[i].CH;
		FM2610[i].OPN.ST.clock = clock;
		FM2610[i].OPN.ST.rate = rate;
		/* FM2610[i].OPN.ST.irq = 0; */
		/* FM2610[i].OPN.ST.status = 0; */
		FM2610[i].OPN.ST.timermodel = FM_TIMER_INTERVAL;
		/* Extend handler */
		FM2610[i].OPN.ST.Timer_Handler = TimerHandler;
		FM2610[i].OPN.ST.IRQ_Handler   = IRQHandler;
		/* ADPCM */
		FM2610[i].pcmbuf[0]   = (char *)(pcmroma[i]);
		FM2610[i].pcm_size[0] = pcmsizea[i];
		FM2610[i].pcmbuf[1]   = (char *)(pcmromb[i]);
		FM2610[i].pcm_size[1] = pcmsizeb[i];
		/* */
		YM2610ResetChip(i);
	}
	InitOPNB_ADPCMATable();
	return 0;
}

/* ---------- shut down emurator ----------- */
void YM2610Shutdown()
{
    if (!FM2610) return;

	FMCloseTable();
	free(FM2610);
	FM2610 = NULL;
}

#if 0
unsigned int getNowAdpcmAddr( int num ){
  return FM2610[0].adpcm[num].now_addr;
}
unsigned char getNowAdpcmReg( int port, int num ){
  return FM2610[0].adpcmreg[port][num];
}
#endif

/* ---------- reset one of chip ---------- */
void YM2610ResetChip(int num)
{
	int i;
	YM2610 *F2610 = &(FM2610[num]);
	FM_OPN *OPN   = &(FM2610[num].OPN);

	/* Reset Priscaler */
	OPNSetPris( OPN, 6*24, 6*24, 4*2); /* OPN 1/6 , SSG 1/4 */
	/* reset SSG section */
	SSGReset(OPN->ST.index);
	/* status clear */
	FM_IRQMASK_SET(&OPN->ST,0x03);
	OPNWriteMode(OPN,0x27,0x30); /* mode 0 , timer reset */

	reset_channel( &OPN->ST , F2610->CH , 6 );
	/* reset OPerator paramater */
	for(i = 0xb6 ; i >= 0xb4 ; i-- )
	{
		OPNWriteReg(OPN,i      ,0xc0);
		OPNWriteReg(OPN,i|0x100,0xc0);
	}
	for(i = 0xb2 ; i >= 0x30 ; i-- )
	{
		OPNWriteReg(OPN,i      ,0);
		OPNWriteReg(OPN,i|0x100,0);
	}
	for(i = 0x26 ; i >= 0x20 ; i-- ) OPNWriteReg(OPN,i,0);
	/**** ADPCM work initial ****/
	for( i = 0; i < 6+1; i++ ){
		F2610->adpcm[i].now_addr  = 0;
		F2610->adpcm[i].now_step  = 0;
		F2610->adpcm[i].step      = 0;
		F2610->adpcm[i].start     = 0;
		F2610->adpcm[i].end       = 0;
		/* F2610->adpcm[i].delta     = 21866; */
		F2610->adpcm[i].volume    = 0;
		F2610->adpcm[i].pan       = &outd[OPN_CENTER]; /* default center */
		F2610->adpcm[i].flagMask  = (i == 6) ? 0x80 : (1<<i);
		F2610->adpcm[i].flag      = 0;
		F2610->adpcm[i].adpcmx    = 0;
		F2610->adpcm[i].adpcmd    = 127;
		F2610->adpcm[i].adpcml    = 0;
		/* DELTA-T */
		/*F2610->adpcm[i].adpcmm    = 0;*/
		F2610->adpcm[i].volume_w_step = 0;
	    F2610->adpcm[i].next_leveling=0;
	}
	F2610->TL_adpcmb = &(TL_TABLE[0x3f*(int)(0.75/EG_STEP)]);
	F2610->port0state = 0;
	F2610->port0shift = 8;		/* allways 8bits shift */
	//F2610->port1state = 0;
	F2610->port1state = -1;
	F2610->port1shift = 8;		/* allways 8bits shift */
	F2610->adpcm_arrivedEndAddress = 0;
	F2610->adpcm_statusmask = 0xbf;
}

/* YM2610 write */
/* n = number  */
/* a = address */
/* v = value   */
int YM2610Write(int n, int a,int v)
{
	YM2610 *F2610 = &(FM2610[n]);
	FM_OPN *OPN   = &(FM2610[n].OPN);
	int addr;

	switch( a&3 ){
	case 0:	/* address port 0 */
		OPN->ST.address = v & 0xff;
		/* Write register to SSG emurator */
		if( v < 16 ) SSGWrite(n,0,v);
		break;
	case 1:	/* data port 0    */
		addr = OPN->ST.address;
		switch(addr & 0xf0)
		{
		case 0x00:	/* SSG section */
			/* Write data to SSG emurator */
			SSGWrite(n,a,v);
			break;
		case 0x10: /* DeltaT ADPCM */
			YM2610UpdateReq(n);
			switch(addr)
			{
			case 0x1c: /*  FLAG CONTROL : Extend Status Clear/Mask */
				F2610->adpcm_statusmask = ~v;
				F2610->adpcm_arrivedEndAddress &= F2610->adpcm_statusmask;
				break;
			default:
				/* 0x10-0x1b */
				FM_ADPCMBWrite(F2610,addr & 0x0f,v);
			}
			break;
		case 0x20:	/* Mode Register */
			YM2610UpdateReq(n);
			OPNWriteMode(OPN,addr,v);
			break;
		default:	/* OPN section */
			YM2610UpdateReq(n);
			/* write register */
			 OPNWriteReg(OPN,addr,v);
		}
		break;
	case 2:	/* address port 1 */
		F2610->address1 = v & 0xff;
		break;
	case 3:	/* data port 1    */
		YM2610UpdateReq(n);
		addr = F2610->address1;
		if( addr < 0x30 )
			/* 100-12f : ADPCM A section */
			FM_ADPCMAWrite(F2610,addr,v);
		else
			OPNWriteReg(OPN,addr|0x100,v);
	}
	return OPN->ST.irq;
}
unsigned char YM2610Read(int n,int a)
{
	YM2610 *F2610 = &(FM2610[n]);
	int addr = F2610->OPN.ST.address;
	unsigned char ret = 0;

	switch( a&3){
	case 0:	/* status 0 : YM2203 compatible */
		ret = F2610->OPN.ST.status & 0x83;
		break;
	case 1:	/* data 0 */
		if( addr < 16 ) ret = SSGRead(n);
		if( addr == 0xff ) ret = 0x01;
		break;
	case 2:	/* status 1 : + ADPCM status */
		/* ADPCM STATUS (arrived End Address) */
		/* B,--,A5,A4,A3,A2,A1,A0 */
		/* B     = ADPCM-B(DELTA-T) arrived end address */
		/* A0-A5 = ADPCM-A          arrived end address */
#if 0
		ret = 0;
		for( i=0;i<7;i++)
			if(!(F2610->adpcm[i].flag)) ret |= F2610->adpcm[i].flagMask;
		ret &= F2610->adpcm_statusmask;
#else
		ret = F2610->adpcm_arrivedEndAddress;
#endif
#ifdef __RAINE__
	  //PrintDebug( "YM2610Status2 %02x\n", ret );
	  //PrintIngame(120,"YM2610Status2 %02x", ret );
#endif
		break;
	case 3:
		ret = 0;
		break;
	}
	return ret;
}

int YM2610TimerOver(int n,int c)
{
	YM2610 *F2610 = &(FM2610[n]);

	if( c )
	{	/* Timer B */
		TimerBOver( &(F2610->OPN.ST) );
	}
	else
	{	/* Timer A */
		YM2610UpdateReq(n);
		/* timer update */
		TimerAOver( &(F2610->OPN.ST) );
		/* CSM mode key,TL controll */
		if( F2610->OPN.ST.mode & 0x80 )
		{	/* CSM mode total level latch and auto key on */
			CSMKeyControll( &(F2610->CH[2]) );
		}
	}
	return F2610->OPN.ST.irq;
}

#if 0
/* ---------- return the buffer ---------- */
FMSAMPLE *YM2610Buffer(int n)
{
    return FMOPN[n].Buf[0];
}
#endif

#if 0
/* ---------- set buffer ---------- */
int YM2610SetBuffer(int n, FMSAMPLE **buf )
{
	int i;
	for( i = 0 ; i < YM2610_NUMBUF ; i++){
		FM2610[n].Buf[i] = buf[i];
		/*if( cur_chip == &FM2610[n] ) cur_chip = NULL;*/
	}
	return 0;
}
#endif

#endif /* BUILD_YM2610 */


#if BUILD_YM2612
/*******************************************************************************/
/*		YM2612 local section                                                   */
/*******************************************************************************/
static YM2612 *FM2612=NULL;	/* array of YM2612's */

/* ---------- update one of chip ----------- */
void YM2612UpdateOne(int num, void **buffer, int length)
{
	YM2612 *F2612 = &(FM2612[num]);
	FM_OPN *OPN   = &(FM2612[num].OPN);
	int dataR,dataL;
	int i,ch;
	int dacen = F2612->dacen;
	int dacout  = F2612->dacout;

	/* set bufer */
	bufL = (FMSAMPLE *)buffer[0];
	bufR = (FMSAMPLE *)buffer[1];

	if( (void *)F2612 != cur_chip ){
		cur_chip = (void *)F2612;

		State = &OPN->ST;
		cch[0]   = &F2612->CH[0];
		cch[1]   = &F2612->CH[1];
		cch[2]   = &F2612->CH[2];
		cch[3]   = &F2612->CH[3];
		cch[4]   = &F2612->CH[4];
		cch[5]   = &F2612->CH[5];
	}
	/* update frequency counter */
	CALC_FCOUNT( cch[0] );
	CALC_FCOUNT( cch[1] );
	if( (State->mode & 0xc0) ){
		/* 3SLOT MODE */
		if( cch[2]->SLOT[SLOT1].Incr==-1){
			/* 3 slot mode */
			CALC_FCSLOT(&cch[2]->SLOT[SLOT1] , OPN->SL3.fc[1] , OPN->SL3.kcode[1] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT2] , OPN->SL3.fc[2] , OPN->SL3.kcode[2] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT3] , OPN->SL3.fc[0] , OPN->SL3.kcode[0] );
			CALC_FCSLOT(&cch[2]->SLOT[SLOT4] , cch[2]->fc , cch[2]->kcode );
		}
	}else CALC_FCOUNT( cch[2] );
	CALC_FCOUNT( cch[3] );
	CALC_FCOUNT( cch[4] );
	CALC_FCOUNT( cch[5] );
	/* buffering */
    for( i=0; i < length ; i++ )
	{
		/* clear output acc. */
		outd[OPN_LEFT] = outd[OPN_RIGHT]= outd[OPN_CENTER] = 0;
		/* calcrate channel output */
		for( ch=0;ch<5;ch++) FM_CALC_CH( cch[ch] );
		if( dacen )  *cch[5]->connect4 += dacout;
		else         FM_CALC_CH( cch[5] );
		/* get left & right output */
		dataL = Limit( outd[OPN_CENTER] + outd[OPN_LEFT], OPN_MAXOUT, OPN_MINOUT );
		dataR = Limit( outd[OPN_CENTER] + outd[OPN_RIGHT], OPN_MAXOUT, OPN_MINOUT );
		/* buffering */
#ifdef FM_STEREO_MIX		/* stereo mixing */
		/* stereo mix */
		((FMSAMPLE_MIX *)bufL)[i] = ((dataL>>OPN_OUTSB)<<FM_OUTPUT_BIT)|(dataR>>OPN_OUTSB);
#else
		/* stereo separate */
		bufL[i] = dataL>>OPN_OUTSB;
		bufR[i] = dataR>>OPN_OUTSB;
#endif

#ifdef LFO_SUPPORT
		CALC_LOPM_LFO;
#endif
#ifdef INTERNAL_TIMER
		/* timer controll */
		CALC_TIMER_A( State , cch[2] );
#endif
	}
#ifdef INTERNAL_TIMER
	CALC_TIMER_B( State , length );
#endif
}

/* -------------------------- YM2612 ---------------------------------- */
int YM2612Init(int num, int clock, int rate,
               FM_TIMERHANDLER TimerHandler,FM_IRQHANDLER IRQHandler)
{
	int i;

    if (FM2612) return (-1);	/* duplicate init. */
    cur_chip = NULL;	/* hiro-shi!! */

	FMNumChips = num;

	/* allocate extend state space */
	if( (FM2612 = (YM2612 *)malloc(sizeof(YM2612) * FMNumChips))==NULL)
		return (-1);
	/* clear */
	memset(FM2612,0,sizeof(YM2612) * FMNumChips);
	/* allocate total level table (128kb space) */
	if( !FMInitTable() )
	{
		free( FM2612 );
		return (-1);
	}

	for ( i = 0 ; i < FMNumChips; i++ ) {
		FM2612[i].OPN.ST.index = i;
		FM2612[i].OPN.type = TYPE_YM2612;
		FM2612[i].OPN.P_CH = FM2612[i].CH;
		FM2612[i].OPN.ST.clock = clock;
		FM2612[i].OPN.ST.rate = rate;
		/* FM2612[i].OPN.ST.irq = 0; */
		/* FM2612[i].OPN.ST.status = 0; */
		FM2612[i].OPN.ST.timermodel = FM_TIMER_SINGLE;
		/* Extend handler */
		FM2612[i].OPN.ST.Timer_Handler = TimerHandler;
		FM2612[i].OPN.ST.IRQ_Handler   = IRQHandler;
		YM2612ResetChip(i);
	}
	return 0;
}

/* ---------- shut down emurator ----------- */
void YM2612Shutdown()
{
    if (!FM2612) return;

	FMCloseTable();
	free(FM2612);
	FM2612 = NULL;
}

/* ---------- reset one of chip ---------- */
void YM2612ResetChip(int num)
{
	int i;
	YM2612 *F2612 = &(FM2612[num]);
	FM_OPN *OPN   = &(FM2612[num].OPN);

	OPNSetPris( OPN , 12*12, 12*12, 0);
	/* status clear */
	FM_IRQMASK_SET(&OPN->ST,0x03);
	OPNWriteMode(OPN,0x27,0x30); /* mode 0 , timer reset */

	reset_channel( &OPN->ST , &F2612->CH[0] , 6 );

	for(i = 0xb6 ; i >= 0xb4 ; i-- )
	{
		OPNWriteReg(OPN,i      ,0xc0);
		OPNWriteReg(OPN,i|0x100,0xc0);
	}
	for(i = 0xb2 ; i >= 0x30 ; i-- )
	{
		OPNWriteReg(OPN,i      ,0);
		OPNWriteReg(OPN,i|0x100,0);
	}
	for(i = 0x26 ; i >= 0x20 ; i-- ) OPNWriteReg(OPN,i,0);
	/* DAC mode clear */
	F2612->dacen = 0;
}

/* YM2612 write */
/* n = number  */
/* a = address */
/* v = value   */
int YM2612Write(int n, int a,int v)
{
	YM2612 *F2612 = &(FM2612[n]);
	int addr;

	switch( a&3){
	case 0:	/* address port 0 */
		F2612->OPN.ST.address = v & 0xff;
		break;
	case 1:	/* data port 0    */
		addr = F2612->OPN.ST.address;
		switch( addr & 0xf0 )
		{
		case 0x20:	/* 0x20-0x2f Mode */
			switch( addr )
			{
			case 0x2a:	/* DAC data (YM2612) */
				YM2612UpdateReq(n);
				F2612->dacout = v<<(TL_BITS-8);
			case 0x2b:	/* DAC Sel  (YM2612) */
				/* b7 = dac enable */
				F2612->dacen = v & 0x80;
				break;
			default:	/* OPN section */
				YM2612UpdateReq(n);
				/* write register */
				 OPNWriteMode(&(F2612->OPN),addr,v);
			}
			break;
		default:	/* 0x30-0xff OPN section */
			YM2612UpdateReq(n);
			/* write register */
			 OPNWriteReg(&(F2612->OPN),addr,v);
		}
		break;
	case 2:	/* address port 1 */
		F2612->address1 = v & 0xff;
		break;
	case 3:	/* data port 1    */
		addr = F2612->address1;
		YM2612UpdateReq(n);
		OPNWriteReg(&(F2612->OPN),addr|0x100,v);
		break;
	}
	return F2612->OPN.ST.irq;
}
unsigned char YM2612Read(int n,int a)
{
	YM2612 *F2612 = &(FM2612[n]);

	switch( a&3){
	case 0:	/* status 0 */
		return F2612->OPN.ST.status;
	case 1:
	case 2:
	case 3:
		Log(LOG_WAR,"YM2612 #%d:A=%d read unmapped area\n");
		return F2612->OPN.ST.status;
	}
	return 0;
}

int YM2612TimerOver(int n,int c)
{
	YM2612 *F2612 = &(FM2612[n]);

	if( c )
	{	/* Timer B */
		TimerBOver( &(F2612->OPN.ST) );
	}
	else
	{	/* Timer A */
		YM2612UpdateReq(n);
		/* timer update */
		TimerAOver( &(F2612->OPN.ST) );
		/* CSM mode key,TL controll */
		if( F2612->OPN.ST.mode & 0x80 )
		{	/* CSM mode total level latch and auto key on */
			CSMKeyControll( &(F2612->CH[2]) );
		}
	}
	return F2612->OPN.ST.irq;
}

#if 0
/* ---------- set buffer ---------- */
int YM2612SetBuffer(int n, FMSAMPLE **buf )
{
	int i;
	for( i = 0 ; i < YM2612_NUMBUF ; i++){
		FM2612[n].Buf[i] = buf[i];
		if( cur_chip == &FM2612[n] ) cur_chip = NULL;
	}
	return 0;
}
#endif

#endif /* BUILD_YM2612 */



#if BUILD_YM2151
/*******************************************************************************/
/*		YM2151 local section                                                   */
/*******************************************************************************/
/* -------------------------- OPM ---------------------------------- */
#undef SEG_SUPPORT		/* OPM has not SEG type envelope */

static YM2151 *FMOPM=NULL;	/* array of YM2151's */

/* ---------- priscaler set(and make time tables) ---------- */
void OPMInitTable( int num )
{
    YM2151 *OPM = &(FMOPM[num]);
	int i;
	double pom;
	double rate;

	if (FMOPM[num].ST.rate)
		rate = (double)(1<<FREQ_BITS) / (3579545.0 / FMOPM[num].ST.clock * FMOPM[num].ST.rate);
	else rate = 1;

	for (i=0; i<8*12*64+950; i++)
	{
		/* This calculation type was used from the Jarek's YM2151 emulator */
		pom = 6.875 * pow (2, ((i+4*64)*1.5625/1200.0) ); /*13.75Hz is note A 12semitones below A-0, so D#0 is 4 semitones above then*/
		/*calculate phase increment for above precounted Hertz value*/
		OPM->KC_TABLE[i] = (unsigned int)(pom * rate);
		/*Log(LOG_WAR,"OPM KC %d = %x\n",i,OPM->KC_TABLE[i]);*/
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
	FM_IRQMASK_SET(&OPM->ST,0x03);
	OPMWriteReg(num,0x1b,0x00);
	/* reset OPerator paramater */
	for(i = 0xff ; i >= 0x20 ; i-- ) OPMWriteReg(num,i,0);
}

/* ----------  Initialize YM2151 emulator(s) ----------    */
/* 'num' is the number of virtual YM2151's to allocate     */
/* 'rate' is sampling rate and 'bufsiz' is the size of the */
/* buffer that should be updated at each interval          */
int OPMInit(int num, int clock, int rate,
               FM_TIMERHANDLER TimerHandler,FM_IRQHANDLER IRQHandler)
{
    int i;

    if (FMOPM) return (-1);	/* duplicate init. */
    cur_chip = NULL;	/* hiro-shi!! */

    FMNumChips = num;

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
		FMOPM[i].ST.index = i;
		FMOPM[i].ST.clock = clock;
		FMOPM[i].ST.rate = rate;
		/* FMOPM[i].ST.irq  = 0; */
		/* FMOPM[i].ST.status = 0; */
		FMOPM[i].ST.timermodel = FM_TIMER_SINGLE;
		FMOPM[i].ST.freqbase  = rate ? ((double)clock * 4096.0 / rate) / 64 : 0;
		FMOPM[i].ST.TimerBase = rate ? 1.0/((double)clock / 64.0) : 0;
		/*OPMSetBuffer(i,0,0);*/
		/* Extend handler */
		FMOPM[i].ST.Timer_Handler = TimerHandler;
		FMOPM[i].ST.IRQ_Handler   = IRQHandler;
		/* Reset callback handler of CT0/1 */
		FMOPM[i].PortWrite = 0;
		OPMResetChip(i);
	}
	return(0);
}

/* ---------- shut down emurator ----------- */
void OPMShutdown()
{
    if (!FMOPM) return;

	FMCloseTable();
	free(FMOPM);
	FMOPM = NULL;
}
/* ---------- write a register on YM2151 chip number 'n' ---------- */
void OPMWriteReg(int n, int r, int v)
{
	unsigned char c;
	FM_CH *CH;
	FM_SLOT *SLOT;

    YM2151 *OPM = &(FMOPM[n]);

	c   = OPM_CHAN(r);
	CH  = &OPM->CH[c];
	SLOT= &CH->SLOT[OPM_SLOT(r)];

	switch( r & 0xe0 ){
	case 0x00: /* 0x00-0x1f */
		switch( r ){
		case 0x01:	/* test */
			break;
		case 0x08:	/* key on / off */
			c = v&7;
			/* CSM mode */
			if( c == 7 && (OPM->ST.mode & 0x80) ) break;
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
				Log(LOG_WAR,"OPM Noise mode sel ( not supported )\n");
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
			FMSetMode( &(OPM->ST),n,v );
			break;
		case 0x18:	/* lfreq   */
			/* !!!!! pickup lfo frequency table !!!!! */
			break;
		case 0x19:	/* PMD/AMD */
			if( v & 0x80 ) OPM->pmd = v & 0x7f;
			else           OPM->amd = v & 0x7f;
			break;
		case 0x1b:	/* CT , W  */
			/* b7 = CT1 */
			/* b6 = CT0 */
			/* b0-3 = wave form(LFO) 0=nokogiri,1=houkei,2=sankaku,3=noise */
			OPM->ctw = v&0xff;
			if( OPM->PortWrite != 0)
				OPM->PortWrite(0, (OPM->ctw)>>6 ); /* bit0 = CT0,bit1 = CT1 */
			else
if (errorlog) fprintf(errorlog,"Write %x to YM2151 output port\n",(OPM->ctw)>>6);
			break;
		}
		break;
	case 0x20:	/* 20-3f */
		switch( OPM_SLOT(r) ){
		case 0: /* 0x20-0x27 : RL,FB,CON */
			{
				int feedback = (v>>3)&7;
				CH->ALGO = v&7;
				CH->FB  = feedback ? 8 - feedback : 0;
				CH->PAN = ((v>>6)&0x03);
				set_algorythm( CH );
			}
			break;
		case 1: /* 0x28-0x2f : Keycode */
			{
				int blk = (v>>4)&7;
				/* make keyscale code */
				CH->kcode = (v>>2)&0x1f;
				/* make basic increment counter 22bit = 1 cycle */
				CH->fc = (blk * (12*64)) + KC_TO_SEMITONE[v&0x0f] + CH->fn_h;
				CH->SLOT[SLOT1].Incr=-1;
			}
			break;
		case 2: /* 0x30-0x37 : Keyfunction */
			CH->fc -= CH->fn_h;
			CH->fn_h = v>>2;
			CH->fc += CH->fn_h;
			CH->SLOT[SLOT1].Incr=-1;
			break;
		case 3: /* 0x38-0x3f : PMS / AMS */
			/* b0-1 AMS */
			/* AMS * 23.90625db */
			CH->SLOT[SLOT1].ams = v & 0x03;
			CH->SLOT[SLOT2].ams = v & 0x03;
			CH->SLOT[SLOT3].ams = v & 0x03;
			CH->SLOT[SLOT4].ams = v & 0x03;
			/* b4-6 PMS */
			/* 0,5,10,20,50,100,400,700 (cent) */
			CH->SLOT[SLOT1].pms = (v>>4) & 0x07;
			CH->SLOT[SLOT2].pms = (v>>4) & 0x07;
			CH->SLOT[SLOT3].pms = (v>>4) & 0x07;
			CH->SLOT[SLOT4].pms = (v>>4) & 0x07;
			break;
		}
		break;
	case 0x40:	/* DT1,MUL */
		set_det_mul(&OPM->ST,CH,SLOT,v);
		break;
	case 0x60:	/* TL */
		set_tl(CH,SLOT,v,(c == 7) && (OPM->ST.mode & 0x80) );
		break;
	case 0x80:	/* KS, AR */
		set_ar_ksr(CH,SLOT,v,OPM->ST.AR_TABLE);
		break;
	case 0xa0:	/* AMS EN,D1R */
		/* bit7 = AMS ENABLE */
		set_dr(SLOT,v,OPM->ST.DR_TABLE);
		break;
	case 0xc0:	/* DT2 ,D2R */
		SLOT->DT2  = DT2_TABLE[v>>6];
		CH->SLOT[SLOT1].Incr=-1;
		set_sr(SLOT,v,OPM->ST.DR_TABLE);
		break;
	case 0xe0:	/* D1L, RR */
		set_sl_rr(SLOT,v,OPM->ST.DR_TABLE);
		break;
    }
}

/* ---------- read status port ---------- */
unsigned char OPMReadStatus(int n)
{
	return FMOPM[n].ST.status;
}

int YM2151Write(int n,int a,int v)
{
	YM2151 *F2151 = &(FMOPM[n]);

	if( !(a&1) )
	{	/* address port */
		F2151->ST.address = v & 0xff;
	}
	else
	{	/* data port */
		int addr = F2151->ST.address;
		YM2151UpdateReq(n);
		/* write register */
		 OPMWriteReg(n,addr,v);
	}
	return F2151->ST.irq;
}

unsigned char YM2151Read(int n,int a)
{
	if( !(a&1) ) return 0;
	else         return FMOPM[n].ST.status;
}

/* ---------- make digital sound data ---------- */
void OPMUpdateOne(int num, void **buffer, int length)
{
	YM2151 *OPM = &(FMOPM[num]);
	int i,ch;
	int dataR,dataL;

	/* set bufer */
	bufL = (FMSAMPLE *)buffer[0];
	bufR = (FMSAMPLE *)buffer[1];

	if( (void *)OPM != cur_chip ){
		cur_chip = (void *)OPM;

		State = &OPM->ST;
		/* channel pointer */
		cch[0] = &OPM->CH[0];
		cch[1] = &OPM->CH[1];
		cch[2] = &OPM->CH[2];
		cch[3] = &OPM->CH[3];
		cch[4] = &OPM->CH[4];
		cch[5] = &OPM->CH[5];
		cch[6] = &OPM->CH[6];
		cch[7] = &OPM->CH[7];
	}
	OPM_CALC_FCOUNT( OPM , cch[0] );
	OPM_CALC_FCOUNT( OPM , cch[1] );
	OPM_CALC_FCOUNT( OPM , cch[2] );
	OPM_CALC_FCOUNT( OPM , cch[3] );
	OPM_CALC_FCOUNT( OPM , cch[4] );
	OPM_CALC_FCOUNT( OPM , cch[5] );
	OPM_CALC_FCOUNT( OPM , cch[6] );
	/* CSM check */
	OPM_CALC_FCOUNT( OPM , cch[7] );

    for( i=0; i < length ; i++ )
	{
		/* clear output acc. */
		outd[OPM_LEFT] = outd[OPM_RIGHT]= outd[OPM_CENTER] = 0;
		/* calcrate channel output */
		for( ch=0;ch<8;ch++) FM_CALC_CH( cch[ch] );
		/* get left & right output */
		dataL = Limit( outd[OPM_CENTER] + outd[OPM_LEFT], OPM_MAXOUT, OPM_MINOUT );
		dataR = Limit( outd[OPM_CENTER] + outd[OPM_RIGHT], OPM_MAXOUT, OPM_MINOUT );

#ifdef FM_STEREO_MIX		/* stereo mixing */
		/* stereo mix */
		((FMSAMPLE_MIX *)bufL)[i] = ((dataL>>OPM_OUTSB)<<FM_OUTPUT_BIT)|(dataR>>OPM_OUTSB);
#else
		/* stereo separate */
		bufL[i] = dataL>>OPM_OUTSB;
		bufR[i] = dataR>>OPM_OUTSB;
#endif
#ifdef LFO_SUPPORT
		CALC_LOPM_LFO;
#endif

#ifdef INTERNAL_TIMER
		CALC_TIMER_A( State , cch[7] );
#endif
    }
#ifdef INTERNAL_TIMER
	CALC_TIMER_B( State , length );
#endif
}

void OPMSetPortHander(int n,void (*PortWrite)(int offset,int CT) )
{
	FMOPM[n].PortWrite = PortWrite;
}

#if 0
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
	    FMOPM[n].Buf[i] = buf[i];
		if( cur_chip == &FMOPM[n] ) cur_chip = NULL;
	}
	return 0;
}
#endif

int YM2151TimerOver(int n,int c)
{
	YM2151 *F2151 = &(FMOPM[n]);

	if( c )
	{	/* Timer B */
		TimerBOver( &(F2151->ST) );
	}
	else
	{	/* Timer A */
		YM2151UpdateReq(n);
		/* timer update */
		TimerAOver( &(F2151->ST) );
		/* CSM mode key,TL controll */
		if( F2151->ST.mode & 0x80 )
		{	/* CSM mode total level latch and auto key on */
			CSMKeyControll( &(F2151->CH[7]) );
		}
	}
	return F2151->ST.irq;
}

#endif /* BUILD_YM2151 */
