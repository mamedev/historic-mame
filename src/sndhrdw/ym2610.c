/***************************************************************/
/*    YM2610 (OPNB?)                                           */
/*    base system : fm.c(OPN : 2203) by Tatsuyuki Satoh        */
/*    modified by Hiromitsu Shioya                             */
/***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "driver.h"
#include "fm.h"
#include "ym2610.h"

#define ADPCM_SHIFT    (10)
#ifdef SIGNED_SAMPLES
	#define AUDIO_CONV(A) ((A))
	#define AUDIO_CONV16(A) ((A))
#else
	#define AUDIO_CONV(A) ((A)^0x80)
	#define AUDIO_CONV16(A) ((A)^0x8000)
#endif


/* ------------------------------------------------------------------ */
#define MAME_TIMER 			/* use Aaron's timer system      */
/* -------------------- speed up optimize switch -------------------- */
/*#define SEG_SUPPORT		   OPNB SSG type envelope support   */
#define S3_SUPPORT			/* OPNB 3SLOT mode support        */
#define CSM_SUPPORT			/* CSM mode support              */
#define TL_SAVE_MEM			/* save some memories for total level */
/*#define LFO_SUPPORT			   LFO support                 */

/* -------------------- preliminary define section --------------------- */
/* attack/decay rate time rate */
#define OPNB_ARRATE     399128
#define OPNB_DRRATE    5514396

#define FREQ_BITS 24			/* frequency turn          */

/* counter bits = 21 , octerve 7 */
#define FREQ_RATE   (1<<(FREQ_BITS-21))
#define TL_BITS    (FREQ_BITS+2)

/* final output shift */
#define OPNB_OUTSB   (TL_BITS+(3)-16)		/* OPNB output final shift 16bit */
#define OPNB_OUTSB_8 (OPNB_OUTSB+8)		/* OPNB output final shift  8bit */
/* limit minimum and maximum */
#define OPNB_MAXOUT (0x7fff<<OPNB_OUTSB)
#define OPNB_MINOUT (-0x8000<<OPNB_OUTSB)
#define OPNBAD_MAXOUT (0x7fff)
#define OPNBAD_MINOUT (-0x8000)

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
#define OPNB_CHAN(N) (N&3)
#define OPNB_SLOT(N) ((N>>2)&3)
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
#define OPNB_RIGHT  1
#define OPNB_LEFT   2
#define OPNB_CENTER 3
/* */

/* ---------- OPNB  one channel  ---------- */
typedef struct fm_chan {
  unsigned int *freq_table;	/* frequency table */
  /* registers , sound type */
  int *DT[4];			/* detune          :DT_TABLE[DT]       */
  int DT2[4];			/* multiple,Detune2:(DT2<<4)|ML for OPM*/
  int TL[4];			/* total level     :TL << 8            */
  signed int TLL[4];		/* adjusted now TL                     */
  unsigned char KSR[4];		/* key scale rate  :3-KSR              */
  int *AR[4];			/* attack rate     :&AR_TABLE[AR<<1]   */
  int *DR[4];			/* decay rate      :&DR_TALBE[DR<<1]   */
  int *SR[4];			/* sustin rate     :&DR_TABLE[SR<<1]   */
  int  SL[4];			/* sustin level    :SL_TALBE[SL]       */
  int *RR[4];			/* release rate    :&DR_TABLE[RR<<2+2] */
  unsigned char SEG[4];		/* SSG EG type     :SSGEG              */
  unsigned char PAN;		/* PAN 0=off,3=center                  */
  unsigned char FB;		/* feed back       :&FB_TABLE[FB<<8]   */
  /* algorythm state */
  int *connect1;		/* operator 1 connection pointer       */
  int *connect2;		/* operator 2 connection pointer       */
  int *connect3;		/* operator 3 connection pointer       */
  /* phase generator state */
  unsigned int  fc;		/* fnum,blk        :calcrated          */
  unsigned char fn_h;		/* freq latch      :                   */
  unsigned char kcode;		/* key code        :                   */
  unsigned char ksr[4];		/* key scale rate  :kcode>>(3-KSR)     */
  unsigned int mul[4];		/* multiple        :ML_TABLE[ML]       */
  unsigned int Cnt[4];		/* frequency count :                   */
  unsigned int Incr[4];		/* frequency step  :                   */
  int op1_out;			/* op1 output foe beedback             */
  /* envelope generator state */
  unsigned char evm[4];		/* envelope phase                      */
  signed int evc[4];		/* envelope counter                    */
  signed int arc[4];		/* envelope counter for AR             */
  signed int evsa[4];		/* envelope step for AR                */
  signed int evsd[4];		/* envelope step for DR                */
  signed int evss[4];		/* envelope step for SR                */
  signed int evsr[4];		/* envelope step for RR                */
} FM_CH;

/* generic fm state */
typedef struct fm_state {
  int bufp;			/* update buffer point */
  int clock;			/* master clock  (Hz)  */
  int rate;			/* sampling rate (Hz)  */
  int freqbase;			/* frequency base      */
  unsigned char status;		/* status flag         */
  unsigned int mode;		/* mode  CSM / 3SLOT   */
  int TA;			/* timer a             */
  int TAC;			/* timer a counter     */
  unsigned char TB;		/* timer b             */
  int TBC;			/* timer b counter     */
  void (*handler)(void);	/* interrupt handler   */
  void *timer_a_timer;		/* timer for a         */
  void *timer_b_timer;		/* timer for b         */
  /* speedup customize */
  /* time tables */
  signed int DT_TABLE[8][32];	/* detune tables       */
  signed int AR_TABLE[94];	/* atttack rate tables */
  signed int DR_TABLE[94];	/* decay rate tables   */
}FM_ST;

typedef struct adpcm_state {
  int          flag;
  unsigned int now_addr;
  unsigned int step;
  unsigned int start;
  unsigned int end;
  unsigned int delta;
  int volume;
  int pan;
  int          adpcmx, adpcmd;
}ADPCM_CH;


/* here's the virtual YM2610(OPNB)  */
typedef struct ym2610_f {
  FMSAMPLE *Buf[YM2610_NUMBUF];		/* sound buffer */
  FM_ST ST;			/* general state     */
  FM_CH CH[6];			/* channel state     */
  /*	unsigned char PR;			   freqency priscaler  */
  /* 3 slot mode state */
  unsigned int  fc3[3];		/* fnum3,blk3  :calcrated */
  unsigned char fn3_h[3];	/* freq3 latch       */
  unsigned char kcode3[3];	/* key code    :     */
  /* fnumber -> increment counter */
  unsigned int FN_TABLE[2048];

  /**** ADPCM control ****/
  char *pcmbuf[2];
  unsigned int pcm_size[2];
  ADPCM_CH adpcm[7];			/* normal ADPCM & deltaT ADPCM */
  unsigned int adpcmreg[2][0x30];
  int port0state, port0control, port0shift;
  int port1state, port1control, port1shift;
} YM2610;

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
    0.000/DV ,15.000/DV ,18.000/DV ,19.875/DV /* OCT 7 */
#endif
};
#undef DV

/* OPNB key frequency number -> key code follow table */
/* fnum higher 4bit -> keycode lower 2bit */
static char OPNB_FKTABLE[16]={0,0,0,0,0,0,0,1,2,3,3,3,3,3,3,3};

static int KC_TO_SEMITONE[16]={
  /*translate note code KC into more usable number of semitone*/
  0*64, 1*64, 2*64, 2*64,
  3*64, 4*64, 5*64, 5*64,
  6*64, 7*64, 8*64, 8*64,
  9*64,10*64,11*64,11*64
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

static char OPNB_DTTABLE[4 * 32]={
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
static int OPNB_LFO_TABLE[256];
#endif

/* dummy attack / decay rate ( when rate == 0 ) */
static int RATE_0[32]=
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/* -------------------- state --------------------- */

/* some globals */
static int FMBufSize;		/* size of sound buffer, in samples */
static int FMNumChips;		/* total # of FM emulated */

static YM2610 *FMOPNB=NULL;	/* array of OPNB's */

static unsigned char sample_16bit;/* output bits    */

/* work table */
static void *cur_chip = 0;	/* current chip point */

/* currenct chip state */
FM_ST            *State;
static FM_CH     *chA,*chB,*chC,*chD,*chE,*chF;
static int *pan[6+6+1];			/* pointer to outd[0-3]; FM:6 ADPCM:6 dADPCM:1 */
static int outd[4];			/*0=off,1=left,2=right,3=center */
static int outdA[4];			/*0=off,1=left,2=right,3=center */
static int outdB[4];			/*0=off,1=left,2=right,3=center */

/* operator connection work area */
static int feedback2;		/* connect for operator 2 */
static int feedback3;		/* connect for operator 3 */
static int feedback4;		/* connect for operator 4 */
static int carrier;			/* connect for carrier    */

/************************************************************/
/************************************************************/
/* --------------------- subroutines  --------------------- */
/************************************************************/
/************************************************************/
static unsigned char adjust_adpcm[] = {
  0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40,
  0x80, 0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0,
};

static int decode_table1[16] = {
  1,   3,   5,   7,   9,  11,  13,  15,
  -1,  -3,  -5,  -7,  -9, -11, -13, -15,
};
static int decode_table2[16] = {
  57,  57,  57,  57,  77, 102, 128, 153,
  57,  57,  57,  57,  77, 102, 128, 153,
  /*
  58,  58,  58,  58,  78, 105, 129, 153,
  58,  58,  58,  58,  78, 105, 129, 153,
  */
};

INLINE int Limit( int val, int max, int min ) {

	if ( val > max )
		val = max;

	if ( val < min )
		val = min;

	return val;
}

/**** ADPCM *****/
INLINE int OPNB_ADPCM_CALC_CH( ADPCM_CH *ch, char *nowpcmbuf, unsigned int rom_size  ){
	unsigned int bit, addr, mask, stepaddr, endaddr;
	int data;

	if ( nowpcmbuf == 0 ) {
		if ( errorlog )
			fprintf( errorlog, "YM2610: Attempting to play Delta T adpcm but no rom is mapped?\n" );
		return 0;
	}

	addr = ch->now_addr;
	ch->now_addr += ch->step;
	mask = ~( ( 1 << ADPCM_SHIFT ) -1 );
	if ( ( addr & mask ) < ( ch->now_addr & mask ) ) {
		endaddr = ch->now_addr & mask;
	    for ( stepaddr = addr & mask; stepaddr < endaddr; stepaddr += ( 1 << ADPCM_SHIFT ) ) {
			addr = ( ( stepaddr >> ( ADPCM_SHIFT + 1 ) ) & 0x003fffff ) + ch->start;
			if ( addr > ch->end ) {
				ch->flag = 0;
				return 0;
			}

			if ( addr > rom_size ) {
				if ( errorlog )
					fprintf( errorlog, "YM2610: Attempting to play past Delta T rom size!\n" );
				return 0;
			}

			bit = ( ( stepaddr >> ( ADPCM_SHIFT ) ) & 0x01 ) ? 0 : 4;
			data = ( *( nowpcmbuf + addr ) >> bit ) & 0x0f;
			ch->adpcmx = Limit( ch->adpcmx + ( ( decode_table1[data] * ch->adpcmd ) / 8 ), 32767, -32768 );
			ch->adpcmd = Limit( ( ch->adpcmd * decode_table2[data] ) / 64, 24576, 127 );
		}
	}

	return ( ( ch->adpcmx * ch->volume ) >> 7 );
}

/****************************************************************/
/****************************************************************/
//#define GORRYS_YM2610

#ifdef GORRYS_YM2610
static int decodeb_table1[] = {
  -1, -1, -1, -1, 2, 5, 7, 9,
  -1, -1, -1, -1, 2, 5, 7, 9,
};
static int decodeb_table2[] = {
  0x0010, 0x0011, 0x0013, 0x0015, 0x0017, 0x0019, 0x001c, 0x001f,
  0x0022, 0x0025, 0x0029, 0x002d, 0x0032, 0x0037, 0x003c, 0x0042,
  0x0049, 0x0050, 0x0058, 0x0061, 0x006b, 0x0076, 0x0082, 0x008f,
  0x009d, 0x00ad, 0x00be, 0x00d1, 0x00e6, 0x00fd, 0x0117, 0x0133,
  0x0151, 0x0173, 0x0198, 0x01c1, 0x01ee, 0x0220, 0x0256, 0x0292,
  0x02d4, 0x031c, 0x036c, 0x03c3, 0x0424, 0x048e, 0x0502, 0x0583,
  0x0610
};
#else
static int decodeb_table1[16] = {
  1,   3,   5,   7,   9,  11,  13,  15,
  -1,  -3,  -5,  -7,  -9, -11, -13, -15,
};
#define DDEF   (-2)
static int decodeb_table2[16] = {
  58,  58,  58,  58,  78, 105, 129, 153,
  58,  58,  58,  58,  78, 105, 129, 153,
  /*
  57,  57,  57,  57,  77, 102, 128, 153,
  57,  57,  57,  57,  77, 102, 128, 153,
  */
  /*
  57-DDEF,  57-DDEF,  57-DDEF,  57-DDEF,  77+DDEF, 102+DDEF, 128+DDEF, 153+DDEF,
  57-DDEF,  57-DDEF,  57-DDEF,  57-DDEF,  77+DDEF, 102+DDEF, 128+DDEF, 153+DDEF,
  */
};
#endif

static int get_decodeb( int num ){
  if( num < 0x08 ){
    return decodeb_table1[num];
    //return decode_table1[num];
  }
#ifndef GORRYS_YM2610
  return decodeb_table2[num - 0x08];
  //return decode_table2[num - 0x08];
#else
  return decodeb_table2[num - 0x08];
#endif
}
void set_decodeb( int num, int data ){
  if( num < 0x08 ){
#ifndef GORRYS_YM2610
    decodeb_table1[num] = data;
    decodeb_table1[num + 0x08] = -data;
    //decode_table1[num] = data;
    //decode_table1[num + 0x08] = -data;
#else
    decodeb_table1[num] = data;
    decodeb_table1[num + 0x08] = data;
#endif
  } else {
#ifndef GORRYS_YM2610
    decodeb_table2[num - 0x08] = data;
    decodeb_table2[(num - 0x08) + 0x08] = data;
    //decode_table2[num - 0x08] = data;
    //decode_table2[(num - 0x08) + 0x08] = data;
#else
    decodeb_table2[num - 0x08] = data;
#endif
  }
}

/**** ADPCM *****/
INLINE int OPNB_ADPCM_CALC_CHB( ADPCM_CH *ch, char *nowpcmbuf, unsigned int rom_size ){
	unsigned int bit, addr, mask, stepaddr, endaddr;
	int data;

	if ( nowpcmbuf == 0 ) {
		if ( errorlog )
			fprintf( errorlog, "YM2610: Attempting to play regular adpcm but no rom is mapped?\n" );
		return 0;
	}

	addr = ch->now_addr;
	ch->now_addr += ch->step;
	mask = ~( ( 1 << ADPCM_SHIFT ) -1 );

	if ( ( addr & mask ) < ( ch->now_addr & mask ) ) {
		endaddr = ch->now_addr & mask;

		for( stepaddr = addr & mask; stepaddr < endaddr; stepaddr += ( 1 << ADPCM_SHIFT ) ) {
			addr = ( ( stepaddr >> ( ADPCM_SHIFT + 1 ) ) & 0x003fffff ) + ch->start;
			if ( addr > ch->end ) {
				ch->flag = 0;
				return 0;
			}

			if ( addr > rom_size ) {
				if ( errorlog )
					fprintf( errorlog, "YM2610: Attempting to play past adpcm rom size!\n" );
				return 0;
			}

			bit = ( ( stepaddr >> ( ADPCM_SHIFT ) ) & 0x01 ) ? 0 : 4;
			data = ( *( nowpcmbuf + addr ) >> bit ) & 0x0f;
#ifdef GORRYS_YM2610
			ans = 0;
			if ( data&0x04 )
				ans += decodeb_table2[ch->adpcmd];
			if ( data&0x02 )
				ans += decodeb_table2[ch->adpcmd] >> 1;
			if ( data&0x01 )
				ans += decodeb_table2[ch->adpcmd] >> 2;

			ans += decodeb_table2[ch->adpcmd] >> 3;

			if ( data&0x08 )
				ans = -ans;

			ch->adpcmx = Limit( ch->adpcmx + ans, 32767, -32768 );
			ch->adpcmd = Limit( ch->adpcmd + decodeb_table1[data], 48, 0 );
#else
			ch->adpcmx = Limit( ch->adpcmx + ( ( ( decodeb_table1[data] ) * ch->adpcmd ) / 8 ), 32767, -32768 );
			ch->adpcmd = Limit( ( ch->adpcmd * decodeb_table2[data] / 64 ), 24576, 127 );
#endif
		}
	}
#ifdef GORRYS_YM2610
	return ( ch->adpcmx * ch->volume ) >> 3;
#else
	return ( ch->adpcmx * ch->volume ) >> 4;
#endif
}


/* ----- key on  ----- */
INLINE void OPNB_FM_KEYON(FM_CH *CH , int s )
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
INLINE void OPNB_FM_KEYOFF(FM_CH *CH , int s )
{
  if( CH->evm[s] > ENV_MOD_RR){
    CH->evm[s] = ENV_MOD_RR;
    /*if( CH->evsr[s] == 0 ) CH->evm[s] &= 0xfe;*/
  }
}

/* ---------- calcrate Envelope Generator & Phase Generator ---------- */
/* return == -1:envelope silent */
/*        >=  0:envelope output */
INLINE signed int OPNB_FM_CALC_SLOT( FM_CH *CH  , int s )
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
  case ENV_SSG_DR:		/* downside */
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
  case ENV_SSG_SR:		/* upside */
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
    CH->connect1 = 0;		/* special mark */
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
INLINE int OPNB_FM_CALC_CH( FM_CH *CH )
{
  int op_out;
  int env_out;

  /* bypass all SLOT output off (SILENCE) */
  if( !(*(long *)(&CH->evm[0])) ) return 0;

  /* clear carrier output */
  feedback2 = feedback3 = feedback4 = carrier = 0;

  /* SLOT 1 */
  if( (env_out=OPNB_FM_CALC_SLOT(CH,SLOT1))>=0 )
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
  if( (env_out=OPNB_FM_CALC_SLOT(CH,SLOT2))>=0 )
    *CH->connect2 += OP_OUT(SLOT2, feedback2 );
  /* SLOT 3 */
  if( (env_out=OPNB_FM_CALC_SLOT(CH,SLOT3))>=0 )
    *CH->connect3 += OP_OUT(SLOT3, feedback3 );
  /* SLOT 4 */
  if( (env_out=OPNB_FM_CALC_SLOT(CH,SLOT4))>=0 )
    carrier       += OP_OUT(SLOT4, feedback4 );

  return carrier;
}
/* ---------- frequency counter for operater update ---------- */
INLINE void OPNB_CALC_FCSLOT(FM_CH *CH , int s , int fc , int kc )
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
INLINE void OPNB_CALC_FCOUNT(FM_CH *CH )
{
  if( CH->Incr[SLOT1]==-1){
    int fc = CH->fc;
    int kc = CH->kcode;
    OPNB_CALC_FCSLOT(CH , SLOT1 , fc , kc );
    OPNB_CALC_FCSLOT(CH , SLOT2 , fc , kc );
    OPNB_CALC_FCSLOT(CH , SLOT3 , fc , kc );
    OPNB_CALC_FCSLOT(CH , SLOT4 , fc , kc );
  }
}

#ifdef MAME_TIMER
static void timer_callback_A_OPNB(int param)
{
	YM2610 *OPNB = &FMOPNB[param];

	OPNB->ST.timer_a_timer = 0;

	if (OPNB->ST.mode & 0x04) {

	  /* set status flag */
	  OPNB->ST.status |= 0x01;

	  /* callback user interrupt handler */
	    if( OPNB->ST.handler != 0 ) OPNB->ST.handler();
	}

	/* update the counter */
	OPNB->ST.TAC = 0;

#ifdef CSM_SUPPORT
	if ( OPNB->ST.mode & 0x80 ) {
		/* CSM mode total level latch and auto key on */
		FM_CH *CSM_CH = &OPNB->CH[2];
		int ksl = KSL[CSM_CH->kcode];

		CSM_CH->TLL[SLOT1] = CSM_CH->TL[SLOT1] + ksl;
		CSM_CH->TLL[SLOT2] = CSM_CH->TL[SLOT2] + ksl;
		CSM_CH->TLL[SLOT3] = CSM_CH->TL[SLOT3] + ksl;
		CSM_CH->TLL[SLOT4] = CSM_CH->TL[SLOT4] + ksl;

		/* all key on */
		OPNB_FM_KEYON(CSM_CH,SLOT1);
		OPNB_FM_KEYON(CSM_CH,SLOT2);
		OPNB_FM_KEYON(CSM_CH,SLOT3);
		OPNB_FM_KEYON(CSM_CH,SLOT4);
	}
#endif
}

static void timer_callback_B_OPNB(int param)
{
  YM2610 *OPNB = &FMOPNB[param];

  OPNB->ST.timer_b_timer = 0;

	if ( OPNB->ST.mode & 0x08 ) {

		/* set status flag */
		OPNB->ST.status |= 0x02;

		/* callback user interrupt handler */
		if( OPNB->ST.handler != 0 ) OPNB->ST.handler();
	}

  /* update the counter */
  OPNB->ST.TBC = 0;
}
#else /* MAME_TIMER */

/* ---------- calcrate timer A ---------- */
INLINE void OPNB_CALC_TIMER_A(FM_ST *ST , FM_CH *CSM_CH )
{
  if( ST->TAC )
    {
      if( (ST->TAC -= ST->freqbase) <= 0 )
	{
	  ST->TAC = 0;		/* stop timer */
	  if( (ST->mode & 0x04) && (ST->status & 0x03)==0 )
	    {			/* user interrupt handler call back */
	      if( ST->handler != 0 ) ST->handler();
	    }
	  ST->status |= 0x01;
#ifdef CSM_SUPPORT
	  if( ST->mode & 0x80 )
	    {			/* CSM mode total level latch and auto key on */
	      int ksl = KSL[CSM_CH->kcode];
	      CSM_CH->TLL[SLOT1] = CSM_CH->TL[SLOT1] + ksl;
	      CSM_CH->TLL[SLOT2] = CSM_CH->TL[SLOT2] + ksl;
	      CSM_CH->TLL[SLOT3] = CSM_CH->TL[SLOT3] + ksl;
	      CSM_CH->TLL[SLOT4] = CSM_CH->TL[SLOT4] + ksl;
	      /* all key on */
	      OPNB_FM_KEYON(CSM_CH,SLOT1);
	      OPNB_FM_KEYON(CSM_CH,SLOT2);
	      OPNB_FM_KEYON(CSM_CH,SLOT3);
	      OPNB_FM_KEYON(CSM_CH,SLOT4);
	    }
#endif
	}
    }
}
/* ---------- calcrate timer B ---------- */
INLINE void OPNB_CALC_TIMER_B(FM_ST *ST,int step)
{
  if( ST->TBC ){
    if( (ST->TBC -= ST->freqbase*step) <= 0 )
      {
	ST->TBC = 0;
	if( (ST->mode & 0x08) && (ST->status & 0x03)==0 )
	  {			/* interrupt reqest */
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
	rate  = (float)ST->freqbase / 4096.0; /* frequency rate */
	if( i < 60 ) rate *= 1.0+(i&3)*0.25; /* b0-1 : x1 , x1.25 , x1.5 , x1.75 */
	rate *= 1<<((i>>2)-1);	/* b2-5 : shift bit */
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

  ST->mode   = 0;		/* normal mode */
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
static int OPNB_FMInitTable( void )
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
    rate = ((1<<TL_BITS)-1)/pow(10,EG_STEP*t/20); /* dB -> voltage */
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
    pom = sin(2*PI*s/SIN_ENT);	/* sin     */
    pom = 20*log10(1/pom);	/* decibel */
    j = pom / EG_STEP;		/* TL_TABLE steps */

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

static void OPNB_FMCloseTable( void )
{
  if( TL_TABLE ) free( TL_TABLE );
  return;
}

/* ---------- priscaler set(and make time tables) ---------- */
static void OPNBSetPris(int num , int pris , int SSGpris)
{
  YM2610 *OPNB = &(FMOPNB[num]);
  int fn;

  /*	OPNB->PR = pris; */
  if (OPNB->ST.rate)
    OPNB->ST.freqbase = ((float)OPNB->ST.clock * 4096.0 / OPNB->ST.rate) / 12 / pris;
  else OPNB->ST.freqbase = 0;

  /* SSG part  priscaler set */
  AY8910_set_clock( num, OPNB->ST.clock * 2 / SSGpris );
  /* make time tables */
  init_timetables( &OPNB->ST , OPNB_DTTABLE , OPNB_ARRATE , OPNB_DRRATE );
  /* make fnumber -> increment counter table */
  for( fn=0 ; fn < 2048 ; fn++ )
    {
      /* it is freq table for octave 7 */
      /* opnb freq counter = 20bit */
      OPNB->FN_TABLE[fn] = (float)fn * OPNB->ST.freqbase / 4096  * FREQ_RATE * (1<<7) / 2;
    }
  /*	if(errorlog) fprintf(errorlog,"OPNB %d set priscaler %d\n",num,pris);*/
}

/* ---------- reset one of chip ---------- */
static void OPNBResetChip(int num)
{
  int i;
  YM2610 *OPNB = &(FMOPNB[num]);

  /*	OPNBSetPris( num , 6 , 4);    1/6 , 1/4   */
  OPNBSetPris( num , 3 , 2);	/* 1/3 , 1/2 */
  reset_channel( &OPNB->ST , &OPNB->CH[0] , 3 );
  /* status clear */
  OPNBWriteReg(num,0,0x27,0x30);	/* mode 0 , timer reset */
  /* reset OPerator paramater */
  for(i = 0xb6 ; i >= 0x30 ; i-- ){
    OPNBWriteReg(num,0,i,0);
    OPNBWriteReg(num,1,i,0);
  }
  for(i = 0x27 ; i >= 0x20 ; i-- ) OPNBWriteReg(num,0,i,0);

  /**** ADPCM work initial ****/
  for( i = 0; i < 6+1; i++ ){
    OPNB->adpcm[i].now_addr  = 0;
    OPNB->adpcm[i].step      = 0;
    OPNB->adpcm[i].start     = 0;
    OPNB->adpcm[i].end       = 0;
    OPNB->adpcm[i].delta     = 21866; /* default sampling rate */
    OPNB->adpcm[i].volume    = 0;
    OPNB->adpcm[i].pan       = OPNB_CENTER; /* default center */
    OPNB->adpcm[i].flag      = 0;
    OPNB->adpcm[i].adpcmx    = 0;
    OPNB->adpcm[i].adpcmd    = 127;
  }
  OPNB->port0state = 0;
  OPNB->port0shift = 8;		/* allways 8bits shift */
  OPNB->port1state = 0;
  OPNB->port1shift = 8;		/* allways 8bits shift */

#if 0
  /**** start ADPCM ****/
  OPNB->adpcm[6].step     = (unsigned int)((float)(OPNB->adpcm[6].delta<<(ADPCM_SHIFT)) / (float)OPNB->ST.rate);
  OPNB->adpcm[6].now_addr = -OPNB->adpcm[6].step;
  OPNB->adpcm[6].adpcmx   = 0;
  OPNB->adpcm[6].adpcmd   = 127;
  OPNB->adpcm[6].volume   = 0xff<<5;
  OPNB->adpcm[6].flag = 1;
  /**** test ****/
  OPNB->adpcm[6].start = 0x00100000;
  OPNB->adpcm[6].end   = 0x0017ffff;
#endif
}

/* ---------- release storage for a chip ---------- */
static void _OPNBFreeChip(int num){
  int i;
  for( i = 0 ; i < YM2610_NUMBUF ; i++){
    FMOPNB[num].Buf[i] = NULL;
  }
}

/* ----------  Initialize YM2610 emulator(s) ----------    */
/* 'num' is the number of virtual YM2610's to allocate     */
/* 'rate' is sampling rate and 'bufsiz' is the size of the */
/* buffer that should be updated at each interval          */
int OPNBInit(int num, int inclock, int rate,  int bitsize, int bufsiz , FMSAMPLE **buffer, int *pcmroma, int *pcmromb )
{
  int i, j;

  if (FMOPNB) return (-1);	/* duplicate init. */

  FMNumChips = num;
  FMBufSize = bufsiz;
  if( bitsize == 16 ) sample_16bit = 1;
  else                sample_16bit = 0;

  if( (FMOPNB) ) return -1;
  /* allocate ym2610 state space */
  if( (FMOPNB = (YM2610 *)malloc(sizeof(YM2610) * FMNumChips))==NULL)
    return (-1);
  /* allocate total lebel table (128kb space) */
  if( !OPNB_FMInitTable() )
    {
      free( FMOPNB );
      return (-1);
    }

  for ( i = 0 ; i < FMNumChips; i++ ) {
    FMOPNB[i].ST.clock = inclock;
    FMOPNB[i].ST.rate = rate;
    FMOPNB[i].ST.handler = 0;
    FMOPNB[i].pcmbuf[0] = (char *)(Machine->memory_region[pcmroma[i]]);
    FMOPNB[i].pcm_size[0] = Machine->memory_region_length[pcmroma[i]];
    FMOPNB[i].pcmbuf[1] = (char *)(Machine->memory_region[pcmromb[i]]);
	FMOPNB[i].pcm_size[1] = Machine->memory_region_length[pcmromb[i]];

    if( OPNBSetBuffer(i,&buffer[i*YM2610_NUMBUF]) < 0 ) {
      for ( j = 0 ; j < i ; j ++ ) {
	_OPNBFreeChip(j);
      }
      return(-1);
    }
    OPNBResetChip(i);
  }

  return(0);
}


/* ---------- shut down emurator ----------- */
void OPNBShutdown()
{
  int i;

  if (!FMOPNB) return;
  for ( i = 0 ; i < FMNumChips ; i++ ) {
    _OPNBFreeChip(i);
  }
  free(FMOPNB); FMOPNB = NULL;
  OPNB_FMCloseTable();
  FMBufSize = 0;
}


unsigned int getNowAdpcmAddr( int num ){
  return FMOPNB[0].adpcm[num].now_addr>>(ADPCM_SHIFT+1);
}
unsigned char getNowAdpcmReg( int port, int num ){
  return FMOPNB[0].adpcmreg[port][num];
}
/* ---------- write a register on YM2610 chip number 'n' ---------- */
void OPNBWriteReg(int n, int port, int r, int v){
  unsigned char c,s,cc;
  FM_CH *CH;

  YM2610 *OPNB = &(FMOPNB[n]);

  /* do not support SSG part */

  /**** ADPCM area ****/
  if( !port && r < 0x20 ){
    OPNB->adpcmreg[port][r] = v&0xff; /* stock data */
    c = 6;			/* last ADPCM channel */
    switch( r ){
    case 0x10:
      OPNB->port0state = v&0xff;
      if( v&0x80 ){
	/**** start ADPCM ****/
	OPNB->adpcm[c].step     = (unsigned int)(((float)(OPNB->adpcm[c].delta<<(ADPCM_SHIFT)) / (float)OPNB->ST.rate) * 0.85);
	OPNB->adpcm[c].now_addr = -OPNB->adpcm[c].step;
	OPNB->adpcm[c].adpcmx   = 0;
	OPNB->adpcm[c].adpcmd   = 127;
	if( !OPNB->adpcm[c].step ) OPNB->adpcm[c].flag = 0;
	else                       OPNB->adpcm[c].flag = 1;
      } else if( v&0x01 ){
	OPNB->adpcm[c].flag = 0;
      }
      break;
    case 0x11:
      OPNB->port0control = v&0xff;
      OPNB->adpcm[c].pan = (v>>6)&0x03;
      break;
    case 0x12:
    case 0x13:
      OPNB->adpcm[c].start  = (OPNB->adpcmreg[0][0x13]*0x0100 | OPNB->adpcmreg[0][0x12]) << OPNB->port0shift;
      break;
    case 0x14:
    case 0x15:
      OPNB->adpcm[c].end    = (OPNB->adpcmreg[0][0x15]*0x0100 | OPNB->adpcmreg[0][0x14]) << OPNB->port0shift;
      OPNB->adpcm[c].end   += (1<<OPNB->port0shift) - 1;
      break;
    case 0x19:
    case 0x1a:
      OPNB->adpcm[c].delta  = (OPNB->adpcmreg[0][0x1a]*0x0100 | OPNB->adpcmreg[0][0x19]);
      OPNB->adpcm[c].delta  = (OPNB->adpcm[c].delta&0x00ff) | ((v&0x00ff)<<8);
      OPNB->adpcm[c].step     = (unsigned int)(((float)(OPNB->adpcm[c].delta<<(ADPCM_SHIFT)) / (float)OPNB->ST.rate) * 0.85);
      break;
    case 0x1b:
      OPNB->adpcm[c].volume = (v&0xff);
      break;
    case 0x1c:
      /**** flag control ****/
      break;

    }
    return;
  } else if( port && r < 0x30 ){
    OPNB->adpcmreg[port][r] = v&0xff; /* stock data */
    if( r == 0x00 ){
      OPNB->port1state = v&0xff;
      if( !(v&0x80) ){
	for( c = 0; c < 6; c++ ){
	  if( (1<<c)&v ){
	    /**** start ADPCM ****/
	    OPNB->adpcm[c].step     = (unsigned int)(((float)(OPNB->adpcm[c].delta<<(ADPCM_SHIFT)) / (float)OPNB->ST.rate) * 0.85);
	    OPNB->adpcm[c].now_addr = 0;
#ifdef GORRYS_YM2610
	    OPNB->adpcm[c].adpcmx   = 0;
	    OPNB->adpcm[c].adpcmd   = 0;
#else
	    OPNB->adpcm[c].adpcmx   = 0;
	    OPNB->adpcm[c].adpcmd   = 127;
#endif
	    OPNB->adpcm[c].flag = 1;
	  }
	}
      } else{
	for( c = 0; c < 6; c++ ){
	  if( (1<<c)&v )  OPNB->adpcm[c].flag = 0;
	}
      }
    } else if( r == 0x01 ){
      //OPNB->port1shift   = 8;
      /* ‚±‚±‚Íbank select‚Å‚Í‚È‚¢‚Ì‚©H */
    } else{
      c = r&0x07;
      if( c >= 0x06 )    return;
      switch( r&0x38 ){
      case 0x08:
	OPNB->adpcm[c].volume = (v&0x1f);
	OPNB->adpcm[c].pan    = (v>>6)&0x03;
	break;
      case 0x10:
      case 0x18:
	OPNB->adpcm[c].start  = ( (OPNB->adpcmreg[1][0x18 + c]*0x0100 | OPNB->adpcmreg[1][0x10 + c]) << OPNB->port1shift);
	break;
      case 0x20:
      case 0x28:
	OPNB->adpcm[c].end    = ( (OPNB->adpcmreg[1][0x28 + c]*0x0100 | OPNB->adpcmreg[1][0x20 + c]) << OPNB->port1shift);
	OPNB->adpcm[c].end   += (1<<OPNB->port1shift) - 1;
	break;
      }
    }
    return;
  }

  /**** FM area ****/
  if( r < 0x30 ){
    switch(r){
    case 0x21:			/* Test */
      break;
    case 0x22:			/* LFO FREQ (YM2612) */
      /* 3.98Hz,5.56Hz,6.02Hz,6.37Hz,6.88Hz,9.63Hz,48.1Hz,72.2Hz */
      break;
    case 0x24:			/* timer A High 8*/
      OPNB->ST.TA = (OPNB->ST.TA & 0x03)|(((int)v)<<2);
      break;
    case 0x25:			/* timer A Low 2*/
      OPNB->ST.TA = (OPNB->ST.TA & 0x3fc)|(v&3);
      break;
    case 0x26:			/* timer B */
      OPNB->ST.TB = v;
      break;
    case 0x27:			/* mode , timer control */
      /* b7 = CSM MODE */
      /* b6 = 3 slot mode */
      /* b5 = reset b */
      /* b4 = reset a */
      /* b3 = enable b */
      /* b2 = enable a */
      /* b1 = load b */
      /* b0 = load a */
      OPNB->ST.mode = v;
#ifdef MAME_TIMER
		if ( v & 0x20 ) { /* Reset Timer B */
			OPNB->ST.TBC = 0;		/* timer stop */
			OPNB->ST.status &= 0xfd;
			if ( OPNB->ST.timer_b_timer ) {
				timer_remove( OPNB->ST.timer_b_timer );
				OPNB->ST.timer_b_timer = 0;
			}
		}

		if ( v & 0x10 ) {  /* Reset Timer A */
			OPNB->ST.TAC = 0;		/* timer stop */
			OPNB->ST.status &=0xfe;
			if ( OPNB->ST.timer_a_timer ) {
				timer_remove( OPNB->ST.timer_a_timer );
				OPNB->ST.timer_a_timer = 0;
			}
		}

		if ( v & 0x02 ) { /* Set Timer B */
			if ( OPNB->ST.TBC == 0 ) {
				OPNB->ST.TBC = ( 256 - OPNB->ST.TB ) << ( 4 + 12 );
				if ( OPNB->ST.TAC ) OPNB->ST.TBC -= 1023;
				OPNB->ST.timer_b_timer = timer_set ((double)OPNB->ST.TBC / ((double)OPNB->ST.freqbase * (double)OPNB->ST.rate), n, timer_callback_B_OPNB);
			}
		}

		if ( v & 0x01 ) { /* Set Timer A */
			if ( OPNB->ST.TAC == 0 ) {
				OPNB->ST.TAC = ( 1024 - OPNB->ST.TA ) << 12;
				if( OPNB->ST.TBC ) OPNB->ST.TAC -= 1023;
				OPNB->ST.timer_a_timer = timer_set ((double)OPNB->ST.TAC / ((double)OPNB->ST.freqbase * (double)OPNB->ST.rate), n, timer_callback_A_OPNB);
			}
		}
#else  /* MAME_TIMER */
      if( v & 0x20 )
	{
	  OPNB->ST.status &=0xfd; /* reset TIMER B */
	  OPNB->ST.TBC = 0;	/* timer stop */
	}
      if( v & 0x10 )
	{
	  OPNB->ST.status &=0xfe; /* reset TIMER A */
	  OPNB->ST.TAC = 0;	/* timer stop */
	}
      if( (v & 0x02) && !(OPNB->ST.status&0x02) ) OPNB->ST.TBC = ( 256-OPNB->ST.TB)<<(4+12);
      if( (v & 0x01) && !(OPNB->ST.status&0x01) ) OPNB->ST.TAC = (1024-OPNB->ST.TA)<<12;
#endif /* MAME_TIMER */

#ifndef S3_SUPPORT
      if(errorlog && v&0x40 ) fprintf(errorlog,"OPNB 3SLOT mode selected (not supported)\n");
#endif
#ifndef CSM_SUPPORT
      if(errorlog && v&0x80 ) fprintf(errorlog,"OPNB CSM mode selected (not supported)\n");
#endif
      break;
    case 0x28:			/* key on / off */
      c = v&0x03;
      if( c == 3 ) break;
#ifdef CSM_SUPPORT
      /* csm mode */
      if( c == 2 && (OPNB->ST.mode & 0x80) ) break;
#endif
      CH = &OPNB->CH[c + ((v&0x04) ? 3 : 0)];
      if(v&0x10) OPNB_FM_KEYON(CH,SLOT1); else OPNB_FM_KEYOFF(CH,SLOT1);
      if(v&0x20) OPNB_FM_KEYON(CH,SLOT2); else OPNB_FM_KEYOFF(CH,SLOT2);
      if(v&0x40) OPNB_FM_KEYON(CH,SLOT3); else OPNB_FM_KEYOFF(CH,SLOT3);
      if(v&0x80) OPNB_FM_KEYON(CH,SLOT4); else OPNB_FM_KEYOFF(CH,SLOT4);
      /*			if(errorlog) fprintf(errorlog,"OPNB %d:%d : KEY %02X\n",n,c,v&0xf0);*/
      break;
    case 0x2a:			/* DAC data (YM2612) */
      break;
    case 0x2b:			/* DAC Sel  (YM2612) */
      /* b7 = dac enable */
      break;
    case 0x2d:			/* divider sel */
      OPNBSetPris( n, 6 , 4 );	/* OPNB = 1/6 , SSG = 1/4 */
      break;
    case 0x2e:			/* divider sel */
      OPNBSetPris( n, 3 , 2 );	/* OPNB = 1/3 , SSG = 1/2 */
      break;
    case 0x2f:			/* divider sel */
      OPNBSetPris( n, 2 , 1 );	/* OPNB = 1/2 , SSG = 1/1 */
      break;
    }
    return;
  }
  /* 0x30 - 0xff */
  if( (c = OPNB_CHAN(r)) ==3 ) return; /* 0xX3,0xX7,0xXB,0xXF */
  s  = OPNB_SLOT(r);
  cc = c + (port ? 3 : 0);
  CH = &OPNB->CH[cc];
  switch( r & 0xf0 ) {
  case 0x30:			/* DET , MUL */
    CH->mul[s] = MUL_TABLE[v&0xf];
    CH->DT[s]  = OPNB->ST.DT_TABLE[(v>>4)&7];
    CH->Incr[SLOT1]=-1;
    break;
  case 0x40:			/* TL */
    v &= 0x7f;
    v = (v<<7)|v;		/* 7bit -> 14bit */
    CH->TL[s] = (v*EG_ENT)>>14;
#ifdef CSM_SUPPORT
    if( cc == 2 && (OPNB->ST.mode & 0x80) ) break; /* CSM MODE */
#endif
    CH->TLL[s] = CH->TL[s] + KSL[CH->kcode];
    break;
  case 0x50:			/* KS, AR */
    /*		if(errorlog) fprintf(errorlog,"OPNB %d,%d,%d : AR %d\n",n,c,s,v&0x1f);*/
    CH->KSR[s]  = 3-(v>>6);
    CH->AR[s]   = (v&=0x1f) ? &OPNB->ST.AR_TABLE[v<<1] : RATE_0;
    CH->evsa[s] = CH->AR[s][CH->ksr[s]];
    CH->Incr[SLOT1]=-1;
    break;
  case 0x60:			/*     DR */
    /*		if(errorlog) fprintf(errorlog,"OPNB %d,%d,%d : DR %d\n",n,c,s,v&0x1f);*/
    /* bit7 = AMS ENABLE(YM2612) */
    CH->DR[s] = (v&=0x1f) ? &OPNB->ST.DR_TABLE[v<<1] : RATE_0;
    CH->evsd[s] = CH->DR[s][CH->ksr[s]];
    break;
  case 0x70:			/*     SR */
    /*		if(errorlog) fprintf(errorlog,"OPNB %d,%d,%d : SR %d\n",n,c,s,v&0x1f);*/
    CH->SR[s] = (v&=0x1f) ? &OPNB->ST.DR_TABLE[v<<1] : RATE_0;
    CH->evss[s] = CH->SR[s][CH->ksr[s]];
    break;
  case 0x80:			/* SL, RR */
    /*		if(errorlog) fprintf(errorlog,"OPNB %d,%d,%d : SL %d RR %d \n",n,c,s,v>>4,v&0x0f);*/
    CH->SL[s] = SL_TABLE[(v>>4)];
    CH->RR[s] = (v&=0x0f) ? &OPNB->ST.DR_TABLE[(v<<2)|2] : RATE_0;
    CH->evsr[s] = CH->RR[s][CH->ksr[s]];
    break;
  case 0x90:			/* SSG-EG */
#ifndef SEG_SUPPORT
    if(errorlog && (v&0x08)) fprintf(errorlog,"OPNB %d,%d,%d :SSG-TYPE envelope selected (not supported )\n",n,c,s );
#endif
    CH->SEG[s] = v&0x0f;
    break;
  case 0xa0:
    switch(s){
    case 0:			/* 0xa0-0xa2 : FNUM1 */
      {
	unsigned int fn  = (((unsigned int)( (CH->fn_h)&7))<<8) + v;
	unsigned char blk = CH->fn_h>>3;
	/* make keyscale code */
	CH->kcode = (blk<<2)|OPNB_FKTABLE[(fn>>7)];
	/* make basic increment counter 32bit = 1 cycle */
	CH->fc = OPNB->FN_TABLE[fn]>>(7-blk);
	CH->Incr[SLOT1]=-1;
      }
      break;
    case 1:			/* 0xa4-0xa6 : FNUM2,BLK */
      CH->fn_h = v&0x3f;
      break;
    case 2:			/* 0xa8-0xaa : 3CH FNUM1 */
      /**** error sato's code. modified by CAB ****/
      {
	unsigned int fn  = (((unsigned int)(OPNB->fn3_h[c]&7))<<8) + v;
	unsigned char blk = OPNB->fn3_h[c]>>3;
	/* make keyscale code */
	OPNB->kcode3[c]= (blk<<2)|OPNB_FKTABLE[(fn>>7)];
	/* make basic increment counter 32bit = 1 cycle */
	OPNB->fc3[c] = OPNB->FN_TABLE[fn]>>(7-blk);
	OPNB->CH[2].Incr[SLOT1]=-1;
      }
      break;
    case 3:			/* 0xac-0xae : 3CH FNUM2,BLK */
      OPNB->fn3_h[c] = v&0x3f;
      break;
    }
    break;
  case 0xb0:
    if( r < 0xb4 ){
      set_algorythm( CH , v );
    } else{
      /* LR , AMS , PMS (YM2612) */
      /* b0-2 PMS */
      /* 0,3.4,6.7,10,14,20,40,80(cent) */
      /* b4-5 AMS */
      /* 0,1.4,5.9,11.8(dB) */
      /* b0 = right , b1 = left */
      CH->PAN = (v>>6)&0x03;
    }
    break;
  case 0xb4:			/* LR , AMS , PMS (YM2612) */
    /* b0-2 PMS */
    /* 0,3.4,6.7,10,14,20,40,80(cent) */
    /* b4-5 AMS */
    /* 0,1.4,5.9,11.8(dB) */
    /* b0 = right , b1 = left */
    CH->PAN = (v>>6)&0x03;
  }
}

/* ---------- read status port ---------- */
unsigned char OPNBReadStatus(int n)
{
	return FMOPNB[n].ST.status;
}

/* ---------- read adpcm status port ---------- */
unsigned char OPNBReadADPCMStatus(int n)
{
	YM2610 *OPNB = &(FMOPNB[n]);
	int i, status = 0;

	for ( i = 0; i < 6; i++ ) {
		if ( OPNB->adpcm[i].flag )
			status |= 1 << i;
	}

	if ( OPNB->adpcm[6].flag )
		status |= 0x80;

	return ~status;
}

/* ---------- make digital sound data ---------- */
/* ---------- update one of chip ----------- */
void OPNBUpdateOne(int num, int endp)
{
  YM2610 *OPNB = &(FMOPNB[num]);
  static FMSAMPLE  *buf[YM2610_NUMBUF];
  int i, j;
  int dataR,dataL;
  int dataRA,dataLA;
#ifdef OPNB_ADPCM_MIX
  int dataRB,dataLB;
#endif

  State = &OPNB->ST;
  for( i = 0; i < YM2610_NUMBUF; i++ )  buf[i] = OPNB->Buf[i];

  chA   = &OPNB->CH[0];
  chB   = &OPNB->CH[1];
  chC   = &OPNB->CH[2];
  chD   = &OPNB->CH[3];
  chE   = &OPNB->CH[4];
  chF   = &OPNB->CH[5];

  pan[0] = &outd[chA->PAN];
  pan[1] = &outd[chB->PAN];
  pan[2] = &outd[chC->PAN];
  pan[3] = &outd[chD->PAN];
  pan[4] = &outd[chE->PAN];
  pan[5] = &outd[chF->PAN];

  /**** delta-T ADPCM ****/
  pan[12] = &outdA[OPNB->adpcm[6].pan];

#ifdef OPNB_ADPCM_MIX
  /**** non delta-T ADPCM ****/
  pan[6]  = &outdB[OPNB->adpcm[0].pan];
  pan[7]  = &outdB[OPNB->adpcm[1].pan];
  pan[8]  = &outdB[OPNB->adpcm[2].pan];
  pan[9]  = &outdB[OPNB->adpcm[3].pan];
  pan[10] = &outdB[OPNB->adpcm[4].pan];
  pan[11] = &outdB[OPNB->adpcm[5].pan];
#endif

  /* frequency counter channel A */
  OPNB_CALC_FCOUNT( chA );
  /* frequency counter channel B */
  OPNB_CALC_FCOUNT( chB );
  /* frequency counter channel C */
#ifdef S3_SUPPORT
  if( (State->mode & 0x40) ){
    if( chC->Incr[SLOT1]==-1){
      /* 3 slot mode */
      OPNB_CALC_FCSLOT(chC , SLOT1 , OPNB->fc3[1] , OPNB->kcode3[1] );
      OPNB_CALC_FCSLOT(chC , SLOT2 , OPNB->fc3[2] , OPNB->kcode3[2] );
      OPNB_CALC_FCSLOT(chC , SLOT3 , OPNB->fc3[0] , OPNB->kcode3[0] );
      OPNB_CALC_FCSLOT(chC , SLOT4 , chC->fc , chC->kcode );
    }
  }else{
    OPNB_CALC_FCOUNT( chC );
  }
#else
  OPNB_CALC_FCOUNT( chC );
#endif

  /* frequency counter channel D */
  OPNB_CALC_FCOUNT( chD );
  /* frequency counter channel E */
  OPNB_CALC_FCOUNT( chE );
  /* frequency counter channel F */
  OPNB_CALC_FCOUNT( chF );

  for( i=State->bufp; i < endp ; ++i ){
    /* clear output acc. */
    outd[OPNB_LEFT] = outd[OPNB_RIGHT]= outd[OPNB_CENTER] = 0;
    outdA[OPNB_LEFT] = outdA[OPNB_RIGHT]= outdA[OPNB_CENTER] = 0;
#ifdef OPNB_ADPCM_MIX
    outdB[OPNB_LEFT] = outdB[OPNB_RIGHT]= outdB[OPNB_CENTER] = 0;
#endif
    /* calcrate channel output */
    /**** FM ****/
    *pan[0] += OPNB_FM_CALC_CH( chA );
    *pan[1] += OPNB_FM_CALC_CH( chB );
    *pan[2] += OPNB_FM_CALC_CH( chC );
    *pan[3] += OPNB_FM_CALC_CH( chD );
    *pan[4] += OPNB_FM_CALC_CH( chE );
    *pan[5] += OPNB_FM_CALC_CH( chF );
    /**** deltaT ADPCM ****/
    if( OPNB->adpcm[6].flag )  *pan[12] += OPNB_ADPCM_CALC_CH( &OPNB->adpcm[6], OPNB->pcmbuf[0], OPNB->pcm_size[0] );

    /**** ADPCM ****/
#ifdef OPNB_ADPCM_MIX
    for( j = 0; j < 6; j++ ){
      if( OPNB->adpcm[j].flag )  *pan[6+j] += OPNB_ADPCM_CALC_CHB( &OPNB->adpcm[j], OPNB->pcmbuf[1], OPNB->pcm_size[1] );
    }
#endif

    /* get left & right output */
    dataL = Limit( outd[OPNB_CENTER] + outd[OPNB_LEFT], OPNB_MAXOUT, OPNB_MINOUT );
    dataR = Limit( outd[OPNB_CENTER] + outd[OPNB_RIGHT], OPNB_MAXOUT, OPNB_MINOUT );
    dataLA = Limit( outdA[OPNB_CENTER] + outdA[OPNB_LEFT], OPNBAD_MAXOUT, OPNBAD_MINOUT );
    dataRA = Limit( outdA[OPNB_CENTER] + outdA[OPNB_RIGHT], OPNBAD_MAXOUT, OPNBAD_MINOUT );
#ifdef OPNB_ADPCM_MIX
    dataLB = Limit( outdB[OPNB_CENTER] + outdB[OPNB_LEFT], OPNBAD_MAXOUT, OPNBAD_MINOUT );
    dataRB = Limit( outdB[OPNB_CENTER] + outdB[OPNB_RIGHT], OPNBAD_MAXOUT, OPNBAD_MINOUT );
#endif
	if( sample_16bit ){
		/* stereo separate */
		((unsigned short *)buf[0])[i] = dataL>>OPNB_OUTSB;
		((unsigned short *)buf[1])[i] = dataR>>OPNB_OUTSB;
		((unsigned short *)buf[2])[i] = AUDIO_CONV16(dataLA);
		((unsigned short *)buf[3])[i] = AUDIO_CONV16(dataRA);
#ifdef OPNB_ADPCM_MIX
		((unsigned short *)buf[4])[i] = AUDIO_CONV16(dataLB);
		((unsigned short *)buf[5])[i] = AUDIO_CONV16(dataRB);
#else
		for( j = 0; j < 6; j++ ) {
			if ( OPNB->adpcm[j].flag ) {
				dataL = Limit( OPNB_ADPCM_CALC_CHB( &OPNB->adpcm[j], OPNB->pcmbuf[1], OPNB->pcm_size[1] ), 32767, -32768 );
				((signed short *)buf[2+2+j])[i] = AUDIO_CONV16(dataL);
			} else
				((unsigned short *)buf[2+2+j])[i] = AUDIO_CONV16(0);
		}
#endif
    } else{
		/* stereo separate */
		((unsigned char *)buf[0])[i] = dataL>>OPNB_OUTSB_8;
		((unsigned char *)buf[1])[i] = dataR>>OPNB_OUTSB_8;
		((unsigned char *)buf[2])[i] = AUDIO_CONV((dataLA>>8));
		((unsigned char *)buf[3])[i] = AUDIO_CONV((dataRA>>8));
#ifdef OPNB_ADPCM_MIX
		((unsigned char *)buf[4])[i] = AUDIO_CONV((dataLB>>8));
		((unsigned char *)buf[5])[i] = AUDIO_CONV((dataRB>>8));
#else
		for( j = 0; j < 6; j++ ) {
			if( OPNB->adpcm[j].flag ) {
				dataL = Limit( OPNB_ADPCM_CALC_CHB( &OPNB->adpcm[j], OPNB->pcmbuf[1], OPNB->pcm_size[1] ), 32767, -32768 );
				((unsigned char *)buf[2+2+j])[i] = AUDIO_CONV((dataL>>8));
			} else
				((unsigned char *)buf[2+2+j])[i] = AUDIO_CONV(0);
		}
#endif
    }
#ifndef MAME_TIMER
    /* timer controll */
    OPNB_CALC_TIMER_A( State , chC );
#endif
  }
#ifndef MAME_TIMER
  OPNB_CALC_TIMER_B( State , endp-State->bufp );
#endif
  State->bufp = endp;

  /**** ???? ****/
  if( OPNB->adpcm[6].flag )
    OPNB->ST.status |= 0x20;
  else{
    OPNB->ST.status &= ~0x20;
    OPNB->ST.status |= 0x04;
  }

}

/* ---------- update all chips ----------- */
void OPNBUpdate(void)
{
  int i;

  for ( i = 0 ; i < FMNumChips; i++ ) {
    if( FMOPNB[i].ST.bufp <  FMBufSize ) OPNBUpdateOne(i , FMBufSize );
    FMOPNB[i].ST.bufp = 0;
  }
}

/* ---------- return the buffer ---------- */
FMSAMPLE *OPNBBuffer(int n)
{
  return FMOPNB[n].Buf;
}
/* ---------- set buffer ---------- */
int OPNBSetBuffer(int n, FMSAMPLE **buf ){
  int i;
  for( i = 0 ; i < YM2610_NUMBUF ; i++){
    if( buf[i] == 0 ) return -1;
    FMOPNB[n].Buf[i] = buf[i];
  }
  return 0;
}
/* ---------- set interrupt handler --------- */
void OPNBSetIrqHandler(int n, void (*handler)(void) )
{
  FMOPNB[n].ST.handler = handler;
}

/**************************************************************************/
/*    ADPCM control                                                       */
/**************************************************************************/

/**************** end of file ****************/
