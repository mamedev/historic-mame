/*
**
** File: fm.c -- software implementation of FM sound generator
**
** Copyright (C) 1997 Tatsuyuki Satoh , MultiArcadeMachineEmurator development
**
** Date       :  Name            :  version,function
** 1997.11.28 :  Tatsuyuki Satoh :  Beta 5a
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

/* -------------------- speed up optimize switch -------------------- */
//#define SEG_SUPPORT		/* OPN SSG type envelope support */
#define S3_SUPPORT			/* OPN 3SLOT mode support        */
#define CSM_SUPPORT			/* CSM mode support              */

/* -------------------- debug switch -------------------- */
//#define FM_DEBUG		/* connect,feedback dynamic change */

/* -------------------- preliminary define section --------------------- */
/* attack/decay rate time rate */
#define OPN_ARRATE  (32768)
#define OPN_DRRATE  (32768*2)
#define OPM_ARRATE   OPN_ARRATE
#define OPM_DRRATE   OPN_DRRATE

#define CON_BITS 27		/* moduration to carrier connection rate */

#ifdef FM_DEBUG
static int FB_BITS =  8;	/* feed back level */
#else
/* modurator to carrier multiple rate (+shift bits) */
/* self feed back divide rate (-shift bits ) */
#define FB_BITS  8
#endif

/* -------------------- generic define section --------------------- */
/* slot number */
#define SLOT1 0
#define SLOT2 2
#define SLOT3 1
#define SLOT4 3

#define SIN_BITS 8			/* OPN sin table cycle bits */
#define EG_BITS  9			/* envelope attenater bits (0.1875dB step) */
#define ENV_BITS 16			/* envelope lower bits */

#define DT_BITS 6			/* detune bits     */

#define EG_STEP (96.0/(1<<EG_BITS)) /* OPL is 0.1875 dB step */

/* register number to channel number / slot offset */
#define OPN_CHAN(N) (N&3)
#define OPN_SLOT(N) ((N>>2)&3)
#define OPM_CHAN(N) (N&7)
#define OPM_SLOT(N) ((N>>3)&3)

/* envelope phase */
/* bit0  : enable(Incrementabl)   */
/* bit1  : attack                 */
/* bit2  : decay downside         */
/* bit3  : decay upside(SSG-TYPE) */
/* bit4-6: phase number           */

#define ENV_MOD_OFF 0x00
#define ENV_MOD_RR  0x15
#define ENV_MOD_SR  0x25
#define ENV_MOD_DR  0x35
#define ENV_MOD_AR  0x43
#define ENV_SSG_SR  0x59
#define ENV_SSG_DR  0x65
/* number of envelope maximun count */
#define ENV_OFF ((1<<(ENV_BITS+EG_BITS))-1)

#ifdef FM_16BITS
#define ENV_CUT ENV_OFF
#else
/* dcut of dynamic range for 8bit sampling system (24dB) */
#define ENV_CUT (ENV_OFF>>1)
#endif

#define ENV_OFF ((1<<(ENV_BITS+EG_BITS))-1)

/* key scale level */
/* !!!!! preliminary !!!!! */

#define ML ((1<<ENV_BITS)/EG_STEP)
static unsigned char KSL[32]=
{
#if 1
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
#else
 0.000*ML , 0.000*ML , 0.000*ML , 0.000*ML ,	/* OCT 0 */
 0.000*ML , 0.000*ML , 0.000*ML , 1.875*ML ,	/* OCT 1 */
 0.000*ML , 0.000*ML , 3.000*ML , 4.875*ML ,	/* OCT 2 */
 0.000*ML , 3.000*ML , 6.000*ML , 7.875*ML ,	/* OCT 3 */
 0.000*ML , 6.000*ML , 9.000*ML ,10.875*ML ,	/* OCT 4 */
 0.000*ML , 9.000*ML ,12.000*ML ,13.875*ML ,	/* OCT 5 */
 0.000*ML ,12.000*ML ,15.000*ML ,16.875*ML ,	/* OCT 6 */
 0.000*ML ,15.000*ML ,18.000*ML ,19.875*ML		/* OCT 7 */
#endif
};
#undef ML

/* OPN key frequency number -> key code follow table */
/* fnum higher 4bit -> keycode lower 2bit */
static char OPN_FKTABLE[16]={0,0,0,0,0,0,0,1,2,3,3,3,3,3,3,3};

/* OPM keycode -> frequency counter base tables */
/* C#,D,D#,(E),E,F,F#,(G),G,G#,A,A#,A#,B,C,C# */
#define ML ((1<<16)/(3580000/64))
static int OPM_KCTABLE[16+1] =
{ ML * 4434.9, /* C# */
  ML * 4698.6, /* D  */
  ML * 4978.0, /* D# */
  ML * 5274.0, /* E  */
  ML * 5274.0, /* E  */
  ML * 5587.7, /* F  */
  ML * 5919.9, /* F# */
  ML * 6271.9, /* G  */
  ML * 6271.9, /* G  */
  ML * 6644.9, /* G# */
  ML * 7040.0, /* A  */
  ML * 7458.6, /* A# */
  ML * 7458.6, /* A# */
  ML * 7902.1, /* B# */
  ML * 8372.0, /*+C  */
  ML * 8868.0, /*+C# */

  ML * 9397.2  /*+D  */
};
#undef ML
/* sustain lebel table */
/* 0 - 15: 0, 3, 6, 9,12,15,18,21,24,27,30,33,36,39,42,93 (dB)*/
#define ML ((3/EG_STEP)*(1<<ENV_BITS))
static int SL_TABLE[16]={
 ML* 0,ML* 1,ML* 2,ML* 3,ML* 4,ML* 5,ML* 6,ML* 7,
 ML* 8,ML* 9,ML*10,ML*11,ML*12,ML*13,ML*14,ML*31
};
#undef ML


/* TotalLevel : 48 24 12  6  3 1.5 0.75 (dB) */
/* TL_TABLE[ TL<<2 ]     */
static int TL_TABLE[2<<EG_BITS];
/* sinwave table */
static signed char SIN_TABLE[256];

#if 0
static int AR_CURVE[1<<EG_BITS];
#endif

#if 0
/* feedback -> sin angle conversion table */
static unsigned char FB_TABLE[8<<SIN_BITS];
#endif

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

/* multiple table ( with detune 2) */
#define ML (1<<DT_BITS)
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

/* dummy attack / decay rate */
static int RATE_0[32]=
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/* some globals */
static int FMBufSize;		/* size of sound buffer, in samples */
static int FMNumChips;		/* total # of FM emulated */

static YM2203 *FMOPN=NULL;	/* array of OPN's */
static YM2151 *FMOPM=NULL;	/* array of OPM's */

/* work table */
static void *cur_chip = 0;	/* current chip point */

/* currenct chip state */
FM_ST            *State;
static FM_CH     *chA,*chB,*chC,*chD,*chE,*chF,*chG,*chH;
#ifdef FM_16BITS
static FMSAMPLE  *bufL,*bufR;
static int *outdR[8];
static int *outdL[8];
#else
static FMSAMPLE  *bufA,*bufB,*bufC,*bufD,*bufE,*bufF,*bufG,*bufH;
#endif
static int outd[8];
static int zero = 0;

/* --------------------- subroutines  --------------------- */
/* ----- key on  ----- */
static inline void FM_KEYON(FM_CH *CH , int s )
{
	if( CH->evm[s]<= ENV_MOD_RR){
		/* sin wave restart */
		CH->Cnt[s] = 0;
		/* attack rate */
		CH->evm[s] = ENV_MOD_AR;
		CH->evc[s] = 0; /* dicay reset */
		CH->arc[s] = ENV_OFF;
		CH->evs[s] = CH->AR[s][CH->ksr[s]];
//		if( s == SLOT1 ) CH->fb_latch = 0;
	}
}
/* ----- key off ----- */
static inline void FM_KEYOFF(FM_CH *CH , int s )
{
	if( CH->evm[s] > ENV_MOD_RR){
		CH->evm[s] = ENV_MOD_RR;
		CH->evs[s] = CH->RR[s][CH->ksr[s]];
	}
}

/* calcrate envelope output */
#define CALC_EG(CNT) TL_TABLE[(CH->TLL[s]+CNT)>>ENV_BITS];

/* ---------- calcrate Envelope Generator & Phase Generator ---------- */
/* return envelope output level */
static inline int FM_CALC_SLOT( FM_CH *CH  , int s )
{
	unsigned char md = CH->evm[s];

	/* bypass envelope output = off */
	if( !(md&1) ) return 0; /* zero */
	/* calcrate phage generator */
	CH->Cnt[s] += CH->Incr[s];

	switch( md ){
	case ENV_MOD_AR:
		/* attack rate : linear(dB) curve */
		if( (CH->arc[s]-= CH->evs[s]*((CH->arc[s]>>ENV_BITS)+1)) > 0 )
			return CALC_EG(CH->arc[s]);
		/* change to next phase */
#ifdef SEG_SUPPORT
		if( !(CH->SEG[s]&8) ){
#endif
			/* next DR */
			CH->evm[s] = ENV_MOD_DR;
			CH->evc[s] = 0;
			CH->evs[s] = CH->DR[s][CH->ksr[s]];
#ifdef SEG_SUPPORT
		}else{
			/* SSG-EG mode */
			if( CH->SEG[s]&4){	/* start direction */
				/* next SSG-SR (upside start ) */
				CH->evm[s] = ENV_SSG_SR;
				CH->evc[s] = CH->SL[s];
				CH->evs[s] = CH->SR[s][CH->ksr[s]]*2;	/* ? */
			}else{
				/* next SSG-DR (downside start ) */
				CH->evm[s] = ENV_SSG_DR;
				CH->evc[s] = 0;
				CH->evs[s] = CH->DR[s][CH->ksr[s]]*2;	/* ?? */
			}
		}
#endif
		return CALC_EG(0);
	case ENV_MOD_DR:
		if( (CH->evc[s]+=CH->evs[s]) < CH->SL[s] )
			return CALC_EG(CH->evc[s]);
		/* change phase */
		/* next SR */
		CH->evm[s] = ENV_MOD_SR;
		CH->evc[s] = CH->SL[s];
		CH->evs[s] = CH->SR[s][CH->ksr[s]];
		return CALC_EG(CH->evc[s]);
	case ENV_MOD_SR:
	case ENV_MOD_RR:
		if( (CH->evc[s]+=CH->evs[s]) < ENV_CUT )
			return CALC_EG(CH->evc[s]);
		/* wait for key off */
		CH->evm[s]&= 0xf0;		/* disable */
//		CH->evc[s] = ENV_OFF;
//		CH->evs[s] = 0;
		return 0;
#ifdef SEG_SUPPORT
	/* SSG type envelope */
	case ENV_SSG_DR:	/* downside */
		if( (CH->evc[s]+=CH->evs[s]) < CH->SL[s] )
			return CALC_EG(CH->evc[s]);
		/* change next phase */
		if( CH->SEG[s]&2){
			/* reverce */
			CH->evm[s] = ENV_SSG_SR;
			CH->evc[s] = CH->SL[s];
			CH->evs[s] = CH->SR[s][CH->ksr[s]]*2;	/* ? */
		}else{
			/* again */
			CH->evc[s] = 0;
		}
		/* hold check */
		if( CH->SEG[s]&1) CH->evs[s] = 0;
		break;
	case ENV_SSG_SR:	/* upside */
		if( (CH->evc[s]-=CH->evs[s]) > 0 )
			return CALC_EG(CH->evc[s]);
		/* next phase */
		if( CH->SEG[s]&2){
			/* reverce  */
			CH->evm[s] = ENV_SSG_DR;
			CH->evc[s] = 0;
			CH->evs[s] = CH->DR[s][CH->ksr[s]]*2;	/* ?? */
		}else{
			/* again */
			CH->evc[s] = CH->SL[s];
		}
		/* hold check */
		if( CH->SEG[s]&1) CH->evs[s] = 0;
#endif
	}
	return CALC_EG(CH->evc[s]);
}

/* operator output calcrator */
#define OP_OUT(SLOT,CON) (env[SLOT]*SIN_TABLE[((CH->Cnt[SLOT]+CON)>>16)&0xff])
#define OP_OUTN(SLOT) (env[SLOT]*SIN_TABLE[(CH->Cnt[SLOT]>>16)&0xff])

/* ---------- calcrate one of channel ---------- */
static inline int FM_CALC_CH( FM_CH *CH )
{
	static int op_out1;
	static int env[4];

	/* bypass all SLOT output off (SILENCE) */
	if( !( (*(long *)(&CH->evm[0])) &0x01010101) ) return 0;

	/* calcrate slot 1 step */
	env[SLOT1] = FM_CALC_SLOT(CH , SLOT1 );
	env[SLOT2] = FM_CALC_SLOT(CH , SLOT2 );
	env[SLOT3] = FM_CALC_SLOT(CH , SLOT3 );
	env[SLOT4] = FM_CALC_SLOT(CH , SLOT4 );

	/* Connection and output calcrate */
#if 0
	/* ?? self feed back source is before envelope */
	CH->fb_latch = SIN_TABLE[((CH->Cnt[SLOT1]+CH->fb_latch)>>16)&0xff];
	op_out1 = env[SLOT1] * CH->fb_latch;
	CH->fb_latch <<= CH->FB;
#else
	/* ?? self feed back source is after envelope */
	op_out1 = OP_OUT(SLOT1,CH->fb_latch);
	CH->fb_latch = op_out1>>CH->FB;
#endif

	switch(CH->AL){
	case 0:
	/*  PG---S1---S2---S3---S4---OUT */
	return OP_OUT(SLOT4, OP_OUT(SLOT3, OP_OUT(SLOT2, op_out1)));
	case 1:
	/*  PG---S1-+-S3---S4---OUT */
	/*  PG---S2-+               */
	return OP_OUT(SLOT4, OP_OUT(SLOT3, OP_OUTN(SLOT2) + op_out1));
	case 2:
	/* PG---S1------+-S4---OUT */
	/* PG---S2---S3-+          */
	return OP_OUT(SLOT4, OP_OUT(SLOT3, OP_OUTN(SLOT2)) + op_out1);
	case 3:
	/* PG---S1---S2-+-S4---OUT */
	/* PG---S3------+          */
	return OP_OUT(SLOT4, OP_OUTN(SLOT3) + OP_OUT(SLOT2, op_out1));
	case 4:
	/* PG---S1---S2-+--OUT */
	/* PG---S3---S4-+      */
	return OP_OUT(SLOT4, OP_OUTN(SLOT3)) + OP_OUT(SLOT2, op_out1);
	case 5:
	/*         +-S2-+     */
	/* PG---S1-+-S3-+-OUT */
	/*         +-S4-+     */
	return OP_OUT(SLOT4,op_out1) + OP_OUT(SLOT3,op_out1) +OP_OUT(SLOT2,op_out1);
	case 6:
	/* PG---S1---S2-+     */
	/* PG--------S3-+-OUT */
	/* PG--------S4-+     */
	return OP_OUTN(SLOT4) + OP_OUTN(SLOT3) + OP_OUT(SLOT2, OP_OUT(SLOT1,op_out1));
	}
//	case 7:
	/* PG---S1-+     */
	/* PG---S2-+-OUT */
	/* PG---S3-+     */
	/* PG---S4-+     */
	return OP_OUTN(SLOT4) + OP_OUTN(SLOT3) + OP_OUTN(SLOT2) + op_out1;
}
/* ---------- frequency counter for operater update ---------- */
static inline void CALC_FCSLOT(FM_CH *CH , int s , int fc , int kc )
{
	CH->Incr[s]= (fc * CH->mul[s]) + (CH->DT[s][kc]);

	CH->ksr[s] = kc >> CH->KSR[s];

	CH->TLL[s] = CH->TL[s] + KSL[kc];
}

/* ---------- frequency counter  ---------- */
static inline void CALC_FCOUNT(FM_CH *CH )
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
/* ---------- calcrate timer A ---------- */
static inline void CALC_TIMER_A(FM_ST *ST , FM_CH *CSM_CH )
{
	if( ST->TAC )
	{
		if( (ST->TAC -= ST->freqbase) <= 0 )
		{
			ST->TAC = 0;
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
				CSM_CH->TLL[SLOT3] = CSM_CH->TL[SLOT3] + ksl ;
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
static inline void CALC_TIMER_B(FM_ST *ST,int step)
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
/* ----------- initialize time tabls ----------- */
static void init_timetables( FM_ST *ST , char *DTTABLE , int ARRATE , int DRRATE )
{
	int i,d,rate,ml,sh;
	double rated;

	/* make detune table */
	for (d = 0;d <= 3;d++){
		for (i = 0;i <= 31;i++){
			rated = (DTTABLE[d*32 + i] * ST->freqbase)>>(DT_BITS+2);
			ST->DT_TABLE[d][i]   =  rated;
			ST->DT_TABLE[d+4][i] = -rated;
		}
	}
	/* make attack rate & decay rate tables */
	for (i = 0;i < 4;i++) ST->AR_TABLE[i] = ST->DR_TABLE[i] = 0;
	for (i = 4;i < 64;i++){
		ml = 4+(i&3);					/* x1 , x1.25 , x1.5 , x1.75 */
		sh = ENV_BITS-(i>>2);	/* 2^n */
		rate = ( ((ml * ST->freqbase)>>8) * ARRATE )>>(sh+4);
		ST->AR_TABLE[i] = rate;
		rate = ( ((ml * ST->freqbase)>>8) * DRRATE )>>(sh+4);
		ST->DR_TABLE[i] = rate;
//		if(errorlog) fprintf(errorlog,"%d : AR %x dr %x\n",i,ST->AR_TABLE[i],ST->DR_TABLE[i]);
	}
	ST->AR_TABLE[63] = (2<<ENV_BITS);
	for (i = 64;i < 94 ;i++){	/* make for overflow area */
		ST->AR_TABLE[i] = ST->AR_TABLE[63];
		ST->DR_TABLE[i] = ST->DR_TABLE[63];
	}
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

	for( c = 0 ; c < chan ; c++ )
	{
		ch[c].fc = 0;
		for(s = 0 ; s < 4 ; s++ )
		{
			ch[c].SEG[s] = 0;
			ch[c].evm[s] = ENV_MOD_OFF;
		}
	}
}

/* ---------- generic table initialize ---------- */
static int FMInitTable( void )
{
	int s,t;
	double rated;
#if 0
	int fb,fn;
#endif
	/* make sinwave table */
	for (s = 0;s < 256;s++){
		SIN_TABLE[s] = (1<<6) * sin(2*PI*s/256);
//		if(errorlog) fprintf(errorlog,"sin(%3d) = %d\n",s,SIN_TABLE[s]);
	}
	/* make total level table */
	for (t = 0;t < ENV_CUT>>ENV_BITS ;t++){
		rated = ((1<<(CON_BITS-6-1))-1)/pow(10,EG_STEP*t/20);	/* dB -> voltage */
		TL_TABLE[t] = (int)rated;
//		if(errorlog) fprintf(errorlog,"TotalLevel(%3d) = %x\n",t,TL_TABLE[t]);
	}
	for (t = ENV_CUT>>ENV_BITS ; t < (2<<EG_BITS) ;t++){
		TL_TABLE[t] = 0;
	}
#if 0
	for (t = 0;t < 1<<EG_BITS ;t++){
		rated = ((1<<(CON_BITS-6-1))-1)/pow(10,EG_STEP*t/20);	/* dB -> voltage */
		AR_CURVE[t] = t;
//		if(errorlog) fprintf(errorlog,"arcurve(%3d) = %x\n",t,AR_CURVE[t]);
	}
#endif

#if 0
	/* calcrate feedback -> sin convert table */
	/* feed back table */
	/* 0 , PI/16 , PI/8 , PI/4 , PI/2 , PI , 2*PI , 4*PI */
	for( fb = 0 ; fb < 8 ; fb++ ){
		for( s = 0 ; s < (1<<SIN_BITS) ; s++ ){
			fn = sin(2*PI*s/256) * ( fb ? 1<<(FB_BITS+fb) : 0 );
			FB_TABLE[(fb<<SIN_BITS)+s] = s + fn;
		}
	}
#endif
	return 1;
}

static void FMCloseTable( void )
{
	return;
}


#ifdef FM_DEBUG
/* ---------- dynamic debug functions ---------- */
void fm_tune( int fb , int con )
{
	int i;

	FB_BITS = fb;
	for( i = 0 ; i < 32 ; i++ ) KSL[i] = con<<ENV_BITS;

	if( FB_BITS < 7) FB_BITS = 7;

	if(errorlog) fprintf( errorlog , "FB %d , CON %d \n",fb,con);
}
#else
void fm_tune( int fb , int con )
{
	return;
}
#endif





/* ---------- priscaler set(and make time tables) ---------- */
void OPNSetPris(int num , int pris , int SSGpris)
{
    YM2203 *OPN = &(FMOPN[num]);
	int fn;

/*	OPN->PR = pris; */
	if (OPN->ST.rate)
		OPN->ST.freqbase = (((OPN->ST.clock<<8) / OPN->ST.rate)<<4) / 12 / pris;
	else OPN->ST.freqbase = 0;

	/* SSG part  priscaler set */
	AYSetClock( num, OPN->ST.clock * 2 / SSGpris , OPN->ST.rate );
	/* make time tables */
	init_timetables( &OPN->ST , OPN_DTTABLE , OPN_ARRATE , OPN_DRRATE );
	/* make fnumber -> increment counter table */
	for( fn=0 ; fn < 2048 ; fn++ )
	{
		OPN->FN_TABLE[fn] = (fn * OPN->ST.freqbase )>>(DT_BITS+2);
	}
}

/* ---------- reset one of chip ---------- */
void OPNResetChip(int num)
{
    int i;
    YM2203 *OPN = &(FMOPN[num]);

	/* clock base 20bit -> 32 bit */
	OPNSetPris( num , 6 , 4); /* 1/6 , 1/4 */
	/* status clear */
	OPNWriteReg(num,0x27,0x30); /* mode 0 , timer reset */
	/* reset OPerator paramater */
	for(i = 0xb6 ; i >= 0x20 ; i-- ) OPNWriteReg(num,i,0);
	reset_channel( &OPN->ST , &OPN->CH[0] , 3 );
}
/* ---------- release storage for a chip ---------- */
static void _OPNFreeChip(int num)
{
    YM2203 *OPN = &(FMOPN[num]);

#ifdef OPN_MIX
 	OPN->Buf = NULL;
#else
 	OPN->Buf[0] = NULL;
 	OPN->Buf[1] = NULL;
 	OPN->Buf[2] = NULL;
#endif
}

/* ----------  Initialize YM2203 emulator(s) ----------    */
/* 'num' is the number of virtual YM2203's to allocate     */
/* 'rate' is sampling rate and 'bufsiz' is the size of the */
/* buffer that should be updated at each interval          */
int OPNInit(int num, int clock, int rate, int bufsiz , FMSAMPLE **buffer )
{
    int i;

    if (FMOPN) return (-1);	/* duplicate init. */

    FMNumChips = num;
    FMBufSize = bufsiz;

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
#ifdef OPN_MIX
		if( OPNSetBuffer(i,buffer[i]) < 0 ) {
#else
		if( OPNSetBuffer(i,&buffer[i*3]) < 0 ){
#endif
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
	FM_CH *ch;

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
			if( v & 0x01 ) OPN->ST.TAC = (1024-OPN->ST.TA)<<12;
			if( v & 0x02 ) OPN->ST.TBC = ( 256-OPN->ST.TB)<<(4+12);
			if( v & 0x10 ) OPN->ST.status &=0xfe; /* reset TIMER A */
			if( v & 0x20 ) OPN->ST.status &=0xfd; /* reset TIMER B */
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
			ch = &OPN->CH[c];
			if(v&0x10) FM_KEYON(ch,SLOT1); else FM_KEYOFF(ch,SLOT1);
			if(v&0x20) FM_KEYON(ch,SLOT2); else FM_KEYOFF(ch,SLOT2);
			if(v&0x40) FM_KEYON(ch,SLOT3); else FM_KEYOFF(ch,SLOT3);
			if(v&0x80) FM_KEYON(ch,SLOT4); else FM_KEYOFF(ch,SLOT4);
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
	ch = &OPN->CH[c];
	switch( r & 0xf0 ) {
	case 0x30:	/* DET , MUL */
		ch->mul[s] = MUL_TABLE[v&0xf];
		ch->DT[s]  = OPN->ST.DT_TABLE[(v>>4)&7];
		ch->Incr[SLOT1]=-1;
		break;
	case 0x40:	/* TL */
		ch->TL[s] = ((int)(v&0x7f))<<(EG_BITS+ENV_BITS-7);
#ifdef CSM_SUPPORT
		if( c == 2 && (OPN->ST.mode & 0x80) ) break;	/* CSM MODE */
#endif
		ch->TLL[s] = ch->TL[s] + KSL[ch->kcode];
		break;
	case 0x50:	/* KS, AR */
//		if(errorlog) fprintf(errorlog,"OPN %d,%d,%d : AR %d\n",n,c,s,v&0x1f);
		ch->KSR[s] = 3-(v>>6);
		ch->AR[s] = (v&=0x1f) ? &OPN->ST.AR_TABLE[v<<1] : RATE_0;
		ch->Incr[SLOT1]=-1;
		break;
	case 0x60:	/*     DR */
//		if(errorlog) fprintf(errorlog,"OPN %d,%d,%d : DR %d\n",n,c,s,v&0x1f);
		/* bit7 = AMS ENABLE(YM2612) */
		ch->DR[s] = (v&=0x1f) ? &OPN->ST.DR_TABLE[v<<1] : RATE_0;
		break;
	case 0x70:	/*     SR */
//		if(errorlog) fprintf(errorlog,"OPN %d,%d,%d : SR %d\n",n,c,s,v&0x1f);
		ch->SR[s] = (v&=0x1f) ? &OPN->ST.DR_TABLE[v<<1] : RATE_0;
		break;
	case 0x80:	/* SL, RR */
//		if(errorlog) fprintf(errorlog,"OPN %d,%d,%d : SL %d RR %d \n",n,c,s,v>>4,v&0x0f);
		ch->SL[s] = SL_TABLE[(v>>4)];
		ch->RR[s] = (v&=0x0f) ? &OPN->ST.DR_TABLE[(v<<2)|2] : RATE_0;
		break;
	case 0x90:	/* SSG-EG */
#ifndef SEG_SUPPORT
		if(errorlog && (v&0x08)) fprintf(errorlog,"OPN %d,%d,%d :SSG-TYPE envelope selected (not supported )\n",n,c,s );
#endif
		ch->SEG[s] = v&0x0f;
		break;
	case 0xa0:
		switch(s){
		case 0:		/* 0xa0-0xa2 : FNUM1 */
			{
				unsigned int fn  = (((unsigned int)( (ch->fn_h)&7))<<8) + v;
				unsigned char blk = ch->fn_h>>3;
				/* make keyscale code */
				ch->kcode = (blk<<2)|OPN_FKTABLE[(fn>>7)];
				/* make basic increment counter 32bit = 1 cycle */
				ch->fc = OPN->FN_TABLE[fn]>>(7-blk);
				ch->Incr[SLOT1]=-1;
			}
			break;
		case 1:		/* 0xa4-0xa6 : FNUM2,BLK */
			ch->fn_h = v&0x3f;
			break;
		case 2:		/* 0xa8-0xaa : 3CH FNUM1 */
			{
				unsigned int fn  = (((unsigned int)(OPN->fn3_h[s]&7))<<8) + v;
				unsigned char blk = OPN->fn3_h[s]>>3;
				/* make keyscale code */
				OPN->kcode3[s]= (blk<<2)|OPN_FKTABLE[(fn>>7)];
				/* make basic increment counter 32bit = 1 cycle */
				OPN->fc3[s] = OPN->FN_TABLE[fn]>>(7-blk);
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
			ch->AL = v&7;
			ch->FB = (v&0x38) ? FB_BITS - ((v>>3)&7) : 31;
//			ch->FB = (v&0x38) ? CON_BITS - SIN_BITS - FB_BITS + ((v>>3)&7) : 0;
			ch->fb_latch = 0;
//			if(errorlog) fprintf(errorlog,"OPN %d,%d : AL %d FB %d\n",n,c,v&7,(v>>3)&7);
			break;
		}
		break;
	case 0xb4:		/* LR , AMS , PMS (YM2612) */
		/* b0-2 PMS */
		/* 0,3.4,6.7,10,14,20,40,80(cent) */
		/* b4-5 AMS */
		/* 0,1.4,5.9,11.8(dB) */
		ch->Ldatap = v&0x80 ? &outd[c] : &zero;
		ch->Rdatap = v&0x40 ? &outd[c] : &zero;
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
	FM_ST  *ST  = &OPN->ST;
    int i;
	FM_CH *chA = &OPN->CH[0];
	FM_CH *chB = &OPN->CH[1];
	FM_CH *chC = &OPN->CH[2];

#ifdef OPN_MIX
	FMSAMPLE *buffer = OPN->Buf;
	int opn_data;
#else
	FMSAMPLE *bufA,*bufB,*bufC;

	bufA = OPN->Buf[0];
	bufB = OPN->Buf[1];
	bufC = OPN->Buf[2];
#endif
	/* frequency counter channel A */
	CALC_FCOUNT( chA );
	/* frequency counter channel B */
	CALC_FCOUNT( chB );
	/* frequency counter channel C */
#ifdef S3_SUPPORT
	if( (ST->mode & 0x40) ){
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

    for( i=ST->bufp; i < endp ; ++i )
	{
#ifdef OPN_MIX
		opn_data  = FMOUT_0;
		/* channel A */
		opn_data += FM_CALC_CH(chA);
		/* channel B */
		opn_data += FM_CALC_CH(chB);
		/* channel C */
		opn_data += FM_CALC_CH(chC);
		/* store to sound buffer */
		buffer[i] = opn_data>>(19-FM_BITS);
#else
		/* channel A */
		bufA[i] = FM_CALC_CH(chA)>>(CON_BITS+2-FM_BITS);
		/* channel B */
		bufB[i] = FM_CALC_CH(chB)>>(CON_BITS+2-FM_BITS);
		/* channel C */
		bufC[i] = FM_CALC_CH(chC)>>(CON_BITS+2-FM_BITS);
#endif
		/* timer controll */
		CALC_TIMER_A( ST , chC );
    }
	CALC_TIMER_B( ST , endp-ST->bufp );
    ST->bufp = endp;
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

#ifdef OPN_MIX
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
#else
/* ---------- return the buffer ---------- */
FMSAMPLE *OPNBuffer(int n,int c)
{
    return FMOPN[n].Buf[c];
}
/* ---------- set buffer ---------- */
int OPNSetBuffer(int n, FMSAMPLE **buf )
{
	int i;
	for( i = 0 ; i < 3 ; i++){
		if( buf[i] == 0 ) return -1;
		FMOPN[n].Buf[i] = buf[i];
		if( cur_chip == &FMOPN[n] ) cur_chip = NULL;
	}
	return 0;
}
#endif
/* ---------- set interrupt handler --------- */
void OPNSetIrqHandler(int n, void (*handler)() )
{
	FMOPN[n].ST.handler = handler;
}










/* -------------------------- OPM ---------------------------------- */

/* ---------- priscaler set(and make time tables) ---------- */
void OPMInitTable( int num )
{
    YM2151 *OPM = &(FMOPM[num]);
	int kc,kf,kn,kp;

	/* make keycode,keyfunction -> frequency table */
	for( kc = 0 ; kc < 16 ; kc++ ){
		kn = OPM_KCTABLE[kc];
		kp = OPM_KCTABLE[kc+1]-kn;
		if( kp == 0 ) kp = OPM_KCTABLE[kc+2] - kn;
		kn *= OPM->ST.freqbase;
		kp *= OPM->ST.freqbase;
		for( kf = 0 ; kf < 16 ; kf++ ){
			OPM->KC_TABLE[(kc<<5)|kf] = ( kn + ((kp*kf)>>5) )>>(DT_BITS+6);
//			if(errorlog) fprintf(errorlog,"OPM %d : KC %x ,KF %x ,CNT =%x\n",kc,kf,OPM->KC_TABLE[(kc<<5)+kf]);
		}
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
	/* status clear */
	OPMWriteReg(num,0x1b,0x00);
	/* reset OPerator paramater */
	for(i = 0xff ; i >= 0x30 ; i-- ) OPMWriteReg(num,i,0);
	reset_channel( &OPM->ST , &OPM->CH[0] , 8 );
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
int OPMInit(int num, int clock, int rate, int bufsiz , FMSAMPLE **buffer )
{
    int i,j;

    if (FMOPM) return (-1);	/* duplicate init. */

    FMNumChips = num;
    FMBufSize = bufsiz;

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
		if (rate)
			FMOPM[i].ST.freqbase = (((clock<<8) / (rate))<<4) / 64;
		else
			FMOPM[i].ST.freqbase = 0;
		for ( j = 0 ; j < YM2151_NUMBUF ; j ++ ) {
			FMOPM[i].Buf[j] = 0;
		}
		if( OPMSetBuffer(i,&buffer[i*8]) < 0 ){
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
	FM_CH *ch;

    YM2151 *OPM = &(FMOPM[n]);

	c  = OPM_CHAN(r);
	ch = &OPM->CH[c];
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
			ch = &OPM->CH[c];
			if(v&0x08) FM_KEYON(ch,SLOT1); else FM_KEYOFF(ch,SLOT1);
			if(v&0x10) FM_KEYON(ch,SLOT2); else FM_KEYOFF(ch,SLOT2);
			if(v&0x20) FM_KEYON(ch,SLOT3); else FM_KEYOFF(ch,SLOT3);
			if(v&0x40) FM_KEYON(ch,SLOT4); else FM_KEYOFF(ch,SLOT4);
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
			if( v & 0x01 ) OPM->ST.TAC = (1024-OPM->ST.TA)<<12;
			if( v & 0x02 ) OPM->ST.TBC = ( 256-OPM->ST.TB)<<(4+12);
			if( v & 0x10 ) OPM->ST.status &=0xfe; /* reset TIMER A */
			if( v & 0x20 ) OPM->ST.status &=0xfd; /* reset TIMER B */
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
			ch->AL = v&7;
			ch->FB = (v&0x38) ? FB_BITS - ((v>>3)&7) : 31;
//			ch->FB = (v&0x38) ? CON_BITS - SIN_BITS - FB_BITS + ((v>>3)&7) : 0;
			ch->fb_latch = 0;
			ch->Rdatap = v&0x80 ? &outd[c] : &zero;
			ch->Ldatap = v&0x40 ? &outd[c] : &zero;
			if( cur_chip == OPM ) cur_chip =  NULL;
			break;
		case 1: /* 0x28-0x2f : Keycode */
			{
				int blk = (v>>4)&7;
				int fc  = OPM->KC_TABLE[ (((int)(v&0x0f))<<5)|ch->fn_h ];
				/* make keyscale code */
				ch->kcode = (v>>2)&0x1f;
				/* make basic increment counter 22bit = 1 cycle */
				ch->fc = fc>>(7-blk);
				ch->Incr[SLOT1]=-1;
			}
			break;
		case 2: /* 0x30-0x37 : Keyfunction */
			ch->fn_h = v>>2;
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
		ch->ML[s]  = (ch->ML[s]&0x30) | (v&0x0f);
		ch->mul[s] = MUL_TABLE[ch->ML[s]];
		ch->DT[s]  = OPM->ST.DT_TABLE[(v>>4)&7];
		ch->Incr[SLOT1]=-1;
		break;
	case 0x60:	/* TL */
		ch->TL[s] = ((int)(v&0x7f))<<(EG_BITS+ENV_BITS-7);
#ifdef CSM_SUPPORT
		if( c == 7 && (OPM->ST.mode & 0x80) ) break;	/* CSM MODE */
#endif
		ch->TLL[s] = ch->TL[s] + KSL[ch->kcode];
		break;
	case 0x80:	/* KS, AR */
		ch->KSR[s] = 3-(v>>6);
		ch->AR[s] = (v&=0x1f) ? &OPM->ST.AR_TABLE[v<<1] : RATE_0 ;
		ch->Incr[SLOT1]=-1;
		break;
	case 0xa0:	/* AMS EN,D1R */
		/* bit7 = AMS ENABLE */
		ch->DR[s] = (v&=0x1f) ? &OPM->ST.DR_TABLE[v<<1] : RATE_0 ;
		break;
	case 0xc0:	/* DT2 ,D2R */
		ch->ML[s]  = (ch->ML[s]&0x0f) | ((v>>2)&0x30);
		ch->mul[s] = MUL_TABLE[ch->ML[s]];
		ch->SR[s]  = (v&=0x1f) ? &OPM->ST.DR_TABLE[v<<1] : RATE_0 ;
		ch->Incr[SLOT1]=-1;
		break;
	case 0xe0:	/* D1L, RR */
		ch->SL[s] = SL_TABLE[(v>>4)];
		/* recalrate */
		ch->RR[s] = (v&=0x0f) ? &OPM->ST.DR_TABLE[(v<<2)|2] : RATE_0 ;
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
	int i;
#ifdef FM_16BITS
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
		outdL[0] = chA->Ldatap; outdR[0] = chA->Rdatap;
		outdL[1] = chB->Ldatap; outdR[1] = chB->Rdatap;
		outdL[2] = chC->Ldatap; outdR[2] = chC->Rdatap;
		outdL[3] = chD->Ldatap; outdR[3] = chD->Rdatap;
		outdL[4] = chE->Ldatap; outdR[4] = chE->Rdatap;
		outdL[5] = chF->Ldatap; outdR[5] = chF->Rdatap;
		outdL[6] = chG->Ldatap; outdR[6] = chG->Rdatap;
		outdL[7] = chH->Ldatap; outdR[7] = chH->Rdatap;
	}
	CALC_FCOUNT( chA );
	CALC_FCOUNT( chB );
	CALC_FCOUNT( chC );
	CALC_FCOUNT( chD );
	CALC_FCOUNT( chE );
	CALC_FCOUNT( chF );
	CALC_FCOUNT( chG );
	CALC_FCOUNT( chH );

    for( i=State->bufp; i < endp ; ++i )
	{
		outd[0] = FM_CALC_CH( chA );
		outd[1] = FM_CALC_CH( chB );
		outd[2] = FM_CALC_CH( chC );
		outd[3] = FM_CALC_CH( chD );
		outd[4] = FM_CALC_CH( chE );
		outd[5] = FM_CALC_CH( chF );
		outd[6] = FM_CALC_CH( chG );
		outd[7] = FM_CALC_CH( chH );
		bufL[i] = (*outdL[0]+*outdL[1]+*outdL[2]+*outdL[3]+*outdL[4]+*outdL[5]+*outdL[6]+*outdL[7])>>(CON_BITS+3-FM_BITS);
		bufR[i] = (*outdR[0]+*outdR[1]+*outdR[2]+*outdR[3]+*outdR[4]+*outdR[5]+*outdR[6]+*outdR[7])>>(CON_BITS+3-FM_BITS);
		CALC_TIMER_A( State , chH );
    }
	CALC_TIMER_B( State , endp-State->bufp );
    State->bufp = endp;
#else /* OPM_MIX */

	if( (void *)OPM != cur_chip ){
		cur_chip = (void *)OPM;

		State = &OPM->ST;
		/* set bufer */
		bufA = OPM->Buf[0];
		bufB = OPM->Buf[1];
		bufC = OPM->Buf[2];
		bufD = OPM->Buf[3];
		bufE = OPM->Buf[4];
		bufF = OPM->Buf[5];
		bufG = OPM->Buf[6];
		bufH = OPM->Buf[7];
		/* channel pointer */
		chA = &OPM->CH[0];
		chB = &OPM->CH[1];
		chC = &OPM->CH[2];
		chD = &OPM->CH[3];
		chE = &OPM->CH[4];
		chF = &OPM->CH[5];
		chG = &OPM->CH[6];
		chH = &OPM->CH[7];
//		outdL[0] = chA->Ldatap; outdR[0] = chA->Rdatap;
//		outdL[1] = chB->Ldatap; outdR[1] = chB->Rdatap;
//		outdL[2] = chC->Ldatap; outdR[2] = chC->Rdatap;
//		outdL[3] = chD->Ldatap; outdR[3] = chD->Rdatap;
//		outdL[4] = chE->Ldatap; outdR[4] = chE->Rdatap;
//		outdL[5] = chF->Ldatap; outdR[5] = chF->Rdatap;
//		outdL[6] = chG->Ldatap; outdR[6] = chG->Rdatap;
//		outdL[7] = chH->Ldatap; outdR[7] = chH->Rdatap;
	}
	CALC_FCOUNT( chA );
	CALC_FCOUNT( chB );
	CALC_FCOUNT( chC );
	CALC_FCOUNT( chD );
	CALC_FCOUNT( chE );
	CALC_FCOUNT( chF );
	CALC_FCOUNT( chG );
	CALC_FCOUNT( chH );

    for( i=State->bufp; i < endp ; ++i )
	{
		bufA[i] = FM_CALC_CH( chA )>>(CON_BITS+1-FM_BITS);
		bufB[i] = FM_CALC_CH( chB )>>(CON_BITS+1-FM_BITS);
		bufC[i] = FM_CALC_CH( chC )>>(CON_BITS+1-FM_BITS);
		bufD[i] = FM_CALC_CH( chD )>>(CON_BITS+1-FM_BITS);
		bufE[i] = FM_CALC_CH( chE )>>(CON_BITS+1-FM_BITS);
		bufF[i] = FM_CALC_CH( chF )>>(CON_BITS+1-FM_BITS);
		bufG[i] = FM_CALC_CH( chG )>>(CON_BITS+1-FM_BITS);
		bufH[i] = FM_CALC_CH( chH )>>(CON_BITS+1-FM_BITS);
		CALC_TIMER_A( State , chH );
    }
	CALC_TIMER_B( State , endp-State->bufp );
    State->bufp = endp;
#endif /* OPM_MIX */
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
void OPMSetIrqHandler(int n, void (*handler)() )
{
	FMOPM[n].ST.handler = handler;
}
