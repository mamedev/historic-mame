/*
**
** File: fmopl.c -- software implementation of FM sound generator
**
** Copyright (C) 1999 Tatsuyuki Satoh , MultiArcadeMachineEmurator development
**
** Version 0.36a
**
*/

/*
	no support:
		Rythm sound mode
		LFO (ams,vib)
	preliminary :
		CSM speech mode
	Problem :
	note:
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "driver.h"		/* use M.A.M.E. */
#include "fmopl.h"

#ifndef PI
#define PI 3.14159265358979323846
#endif

/* -------------------- preliminary define section --------------------- */
/* attack/decay rate time rate */
#define OPL_ARRATE     141280  /* RATE 4 =  2826.24ms @ 3.6MHz */
#define OPL_DRRATE    1956000  /* RATE 4 = 39280.64ms @ 3.6MHz */

#define FREQ_BITS 24			/* frequency turn          */

/* counter bits = 20 , octerve 7 */
#define FREQ_RATE   (1<<(FREQ_BITS-20))
#define TL_BITS    (FREQ_BITS+2)

/* final output shift , limit minimum and maximum */
#define OPL_OUTSB   (TL_BITS+2-16)		/* OPL output final shift 16bit */
#define OPL_MAXOUT (0x7fff<<OPL_OUTSB)
#define OPL_MINOUT (-0x8000<<OPL_OUTSB)

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

#define EG_OFF   ((2*EG_ENT)<<ENV_BITS)  /* OFF          */
#define EG_DED   EG_OFF
#define EG_DST   (EG_ENT<<ENV_BITS)      /* DECAY  START */
#define EG_AED   EG_DST
#define EG_AST   0                       /* ATTACK START */

#define EG_STEP (96.0/EG_ENT) /* OPL is 0.1875 dB step  */

/* LFO table entries */
#define LFO_ENT 512

/* -------------------- local defines , macros --------------------- */

/* register number to channel number , slot offset */
#define SLOT1 0
#define SLOT2 1

/* envelope phase */
#define ENV_MOD_RR  0x00
#define ENV_MOD_DR  0x01
#define ENV_MOD_AR  0x02

/* -------------------- tables --------------------- */
static const signed char slot_array[32]=
{
	 0, 2, 4, 1, 3, 5,-1,-1,
	 6, 8,10, 7, 9,11,-1,-1,
	12,14,16,13,15,17,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1
};

/* key scale level */
#define ML (0.1875*2/EG_STEP)
static const unsigned short KSL_TABLE[8*16]=
{
	/* OCT 0 */
	 0.000*ML, 0.000*ML, 0.000*ML, 0.000*ML,
	 0.000*ML, 0.000*ML, 0.000*ML, 0.000*ML,
	 0.000*ML, 0.000*ML, 0.000*ML, 0.000*ML,
	 0.000*ML, 0.000*ML, 0.000*ML, 0.000*ML,
	/* OCT 1 */
	 0.000*ML, 0.000*ML, 0.000*ML, 0.000*ML,
	 0.000*ML, 0.000*ML, 0.000*ML, 0.000*ML,
	 0.000*ML, 0.750*ML, 1.125*ML, 1.500*ML,
	 1.875*ML, 2.250*ML, 2.625*ML, 3.000*ML,
	/* OCT 2 */
	 0.000*ML, 0.000*ML, 0.000*ML, 0.000*ML,
	 0.000*ML, 1.125*ML, 1.875*ML, 2.625*ML,
	 3.000*ML, 3.750*ML, 4.125*ML, 4.500*ML,
	 4.875*ML, 5.250*ML, 5.625*ML, 6.000*ML,
	/* OCT 3 */
	 0.000*ML, 0.000*ML, 0.000*ML, 1.875*ML,
	 3.000*ML, 4.125*ML, 4.875*ML, 5.625*ML,
	 6.000*ML, 6.750*ML, 7.125*ML, 7.500*ML,
	 7.875*ML, 8.250*ML, 8.625*ML, 9.000*ML,
	/* OCT 4 */
	 0.000*ML, 0.000*ML, 3.000*ML, 4.875*ML,
	 6.000*ML, 7.125*ML, 7.875*ML, 8.625*ML,
	 9.000*ML, 9.750*ML,10.125*ML,10.500*ML,
	10.875*ML,11.250*ML,11.625*ML,12.000*ML,
	/* OCT 5 */
	 0.000*ML, 3.000*ML, 6.000*ML, 7.875*ML,
	 9.000*ML,10.125*ML,10.875*ML,11.625*ML,
	12.000*ML,12.750*ML,13.125*ML,13.500*ML,
	13.875*ML,14.250*ML,14.625*ML,15.000*ML,
	/* OCT 6 */
	 0.000*ML, 6.000*ML, 9.000*ML,10.875*ML,
	12.000*ML,13.125*ML,13.875*ML,14.625*ML,
	15.000*ML,15.750*ML,16.125*ML,16.500*ML,
	16.875*ML,17.250*ML,17.625*ML,18.000*ML,
	/* OCT 7 */
	 0.000*ML, 9.000*ML,12.000*ML,13.875*ML,
	15.000*ML,16.125*ML,16.875*ML,17.625*ML,
	18.000*ML,18.750*ML,19.125*ML,19.500*ML,
	19.875*ML,20.250*ML,20.625*ML,21.000*ML
};
#undef ML

/* sustain lebel table (3db per step) */
/* 0 - 15: 0, 3, 6, 9,12,15,18,21,24,27,30,33,36,39,42,93 (dB)*/
#define SC(db) (db*((3/EG_STEP)*(1<<ENV_BITS)))+EG_DST
static const int SL_TABLE[16]={
 SC( 0),SC( 1),SC( 2),SC(3 ),SC(4 ),SC(5 ),SC(6 ),SC( 7),
 SC( 8),SC( 9),SC(10),SC(11),SC(12),SC(13),SC(14),SC(31)
};
#undef SC

#define TL_MAX (EG_ENT*3) /* limit(tl + ksr + envelope) + sinwave */
/* TotalLevel : 48 24 12  6  3 1.5 0.75 (dB) */
/* TL_TABLE[ 0      to TL_MAX          ] : plus  section */
/* TL_TABLE[ TL_MAX to TL_MAX+TL_MAX-1 ] : minus section */
static signed int *TL_TABLE;

/* pointers to TL_TABLE with sinwave output offset */
static signed int **SIN_TABLE;

/* envelope output curve table */
/* attack + decay + OFF */
static int ENV_CURVE[2*EG_ENT+1];

/* multiple table */
#define ML 2
static const int MUL_TABLE[16]= {
/* 1/2, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15 */
   0.50*ML, 1.00*ML, 2.00*ML, 3.00*ML, 4.00*ML, 5.00*ML, 6.00*ML, 7.00*ML,
   8.00*ML, 9.00*ML,10.00*ML,10.00*ML,12.00*ML,12.00*ML,15.00*ML,15.00*ML
};
#undef ML

/* dummy attack / decay rate ( when rate == 0 ) */
static int RATE_0[16]=
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/* -------------------- static state --------------------- */

/* lock level of common table */
static int num_lock = 0;

/* work table */
static void *cur_chip = NULL;	/* current chip point */
/* currenct chip state */
/* static FMSAMPLE  *bufL,*bufR; */
static OPL_CH     *S_CH;
static OPL_CH     *E_CH;
static signed int outd[1];
static unsigned int amsIncr;
static unsigned int vibIncr;
static signed int ams;
static signed int vib;
static signed int feedback1;		/* connect for SLOT 1 */
static signed int feedback2;		/* connect for SLOT 2 */

/* log output level */
#define LOG_ERR  3      /* ERROR       */
#define LOG_WAR  2      /* WARNING     */
#define LOG_INF  1      /* INFORMATION */

#define LOG_LEVEL LOG_INF

static void Log(int level,char *format,...)
{
	int i;
	va_list argptr;

	if( level < LOG_LEVEL ) return;
	va_start(argptr,format);
	/* */
	if (errorlog) vfprintf( errorlog, format , argptr);
}

/* --------------------- subroutines  --------------------- */

INLINE int Limit( int val, int max, int min ) {
	if ( val > max )
		val = max;
	else if ( val < min )
		val = min;

	return val;
}

/* status set and IRQ handling */
INLINE void OPL_STATUS_SET(FM_OPL *OPL,int flag)
{
	/* set status flag */
	OPL->status |= flag;
	if(!(OPL->status & 0x80))
	{
		if(OPL->status & OPL->statusmask)
		{	/* IRQ on */
			OPL->status |= 0x80;
			/* callback user interrupt handler (IRQ is OFF to ON) */
			if(OPL->IRQHandler) (OPL->IRQHandler)(OPL->IRQParam,1);
		}
	}
}

/* status reset and IRQ handling */
INLINE void OPL_STATUS_RESET(FM_OPL *OPL,int flag)
{
	/* reset status flag */
	OPL->status &=~flag;
	if((OPL->status & 0x80))
	{
		if (!(OPL->status & OPL->statusmask) )
		{
			OPL->status &= 0x7f;
			/* callback user interrupt handler (IRQ is ON to OFF) */
			if(OPL->IRQHandler) (OPL->IRQHandler)(OPL->IRQParam,0);
		}
	}
}

/* IRQ mask set */
INLINE void OPL_STATUSMASK_SET(FM_OPL *OPL,int flag)
{
	OPL->statusmask = flag;
	/* IRQ handling check */
	OPL_STATUS_SET(OPL,0);
	OPL_STATUS_RESET(OPL,0);
}

/* ----- key on  ----- */
INLINE void OPL_KEYON(OPL_SLOT *SLOT)
{
	/* sin wave restart */
	SLOT->Cnt = 0;
	/* set attack */
	SLOT->evm = ENV_MOD_AR;
	SLOT->evs = SLOT->evsa;
	SLOT->evc = EG_AST;
	SLOT->eve = EG_AED;
}
/* ----- key off ----- */
INLINE void OPL_KEYOFF(OPL_SLOT *SLOT)
{
	if( SLOT->evm > ENV_MOD_RR)
	{
		/* set envelope counter from envleope output */
		SLOT->evm = ENV_MOD_RR;
		if( !(SLOT->evc&EG_DST) )
			//SLOT->evc = (ENV_CURVE[SLOT->evc>>ENV_BITS]<<ENV_BITS) + EG_DST;
			SLOT->evc = EG_DST;
		SLOT->eve = EG_DED;
		SLOT->evs = SLOT->evsr;
	}
}

/* ---------- calcrate Envelope Generator & Phase Generator ---------- */
/* return : envelope output */
INLINE signed int FM_CALC_SLOT( OPL_SLOT *SLOT )
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
		case ENV_MOD_DR: /* DECAY -> SL or RR */
			SLOT->evc = SLOT->SL;
			SLOT->eve = EG_DED;
			if(SLOT->eg_typ)
			{
				SLOT->evs = 0;
			}
			else
			{
				SLOT->evm = ENV_MOD_RR;
				SLOT->evs = SLOT->evsr;
			}
			break;
		case ENV_MOD_RR: /* RR -> OFF */
			SLOT->evc = EG_OFF;
			SLOT->eve = EG_OFF+1;
			SLOT->evs = 0;
			break;
		}
	}
	/* calcrate envelope */
	return SLOT->TLL+ENV_CURVE[SLOT->evc>>ENV_BITS]; /* LFO_out[SLOT->AMS] */
}

/* set algorythm connection */
static void set_algorythm( OPL_CH *CH)
{
	signed int *carrier = &outd[0];
	CH->connect1 = CH->CON ? carrier : &feedback2;
	CH->connect2 = carrier;
}

/* ---------- frequency counter for operater update ---------- */
INLINE void CALC_FCSLOT(OPL_CH *CH,OPL_SLOT *SLOT)
{
	int ksr;

	/* frequency step counter */
	SLOT->Incr = CH->fc * SLOT->mul;
	ksr = CH->kcode >> SLOT->KSR;

	if( SLOT->ksr != ksr )
	{
		SLOT->ksr = ksr;
		/* attack , decay rate recalcration */
		SLOT->evsa = SLOT->AR[ksr];
		SLOT->evsd = SLOT->DR[ksr];
		SLOT->evsr = SLOT->RR[ksr];
	}
	SLOT->TLL = SLOT->TL + (CH->ksl_base>>SLOT->ksl);
}

/* set multi,am,vib,EG-TYP,KSR,mul */
INLINE void set_mul(FM_OPL *OPL,int slot,int v)
{
	OPL_CH   *CH   = &OPL->P_CH[slot/2];
	OPL_SLOT *SLOT = &CH->SLOT[slot&1];

	SLOT->mul    = MUL_TABLE[v&0x0f];
	SLOT->KSR    = (v&0x10) ? 0 : 2;
	SLOT->eg_typ = (v&0x20)>>5;
	SLOT->vib    = (v&0x40)>>6;
	SLOT->ams    = (v&0x80)>>7;
	CALC_FCSLOT(CH,SLOT);
}

/* set ksl & tl */
INLINE void set_ksl_tl(FM_OPL *OPL,int slot,int v)
{
	OPL_CH   *CH   = &OPL->P_CH[slot/2];
	OPL_SLOT *SLOT = &CH->SLOT[slot&1];
	int ksl = v>>6; /* 0 / 1.5 / 3 / 6 db/OCT */

	SLOT->ksl = ksl ? 3-ksl : 31;
	SLOT->TL  = (v&0x3f)*(0.75/EG_STEP); /* 0.75db step */

	if( !(OPL->mode&0x80) )
	{	/* not CSM latch total level */
		SLOT->TLL = SLOT->TL + (CH->ksl_base>>SLOT->ksl);
	}
}

/* set attack rate & decay rate  */
INLINE void set_ar_dr(FM_OPL *OPL,int slot,int v)
{
	OPL_CH   *CH   = &OPL->P_CH[slot/2];
	OPL_SLOT *SLOT = &CH->SLOT[slot&1];
	int ar = v>>4;
	int dr = v&0x0f;

	SLOT->AR = ar ? &OPL->AR_TABLE[ar<<2] : RATE_0;
	SLOT->evsa = SLOT->AR[SLOT->ksr];
	if( SLOT->evm == ENV_MOD_AR ) SLOT->evs = SLOT->evsa;

	SLOT->DR = dr ? &OPL->DR_TABLE[dr<<2] : RATE_0;
	SLOT->evsd = SLOT->DR[SLOT->ksr];
	if( SLOT->evm == ENV_MOD_DR ) SLOT->evs = SLOT->evsd;
}

/* set sustain level & release rate */
INLINE void set_sl_rr(FM_OPL *OPL,int slot,int v)
{
	OPL_CH   *CH   = &OPL->P_CH[slot/2];
	OPL_SLOT *SLOT = &CH->SLOT[slot&1];
	int sl = v>>4;
	int rr = v & 0x0f;

	SLOT->SL = SL_TABLE[sl];
	if( SLOT->evm == ENV_MOD_DR ) SLOT->eve = SLOT->SL;
	SLOT->RR = &OPL->DR_TABLE[rr<<2];
	SLOT->evsr = SLOT->RR[SLOT->ksr];
	if( SLOT->evm == ENV_MOD_RR ) SLOT->evs = SLOT->evsr;
}

/* operator output calcrator */
#define OP_OUT(slot,env,con)   slot.wavetable[((slot.Cnt+con)/(0x1000000/SIN_ENT))&(SIN_ENT-1)][env]
/* ---------- calcrate one of channel ---------- */
INLINE void OPL_CALC_CH( OPL_CH *CH )
{
	int env_out;

	feedback1 = CH->SLOT[SLOT1].ams ? ams : 0;
	feedback2 = CH->SLOT[SLOT2].ams ? ams : 0;

	/* SLOT 1 */
	env_out=FM_CALC_SLOT(&CH->SLOT[SLOT1]);
	if( env_out < EG_ENT-1 )
	{
		feedback1 += (CH->op1_out>>CH->FB);
		*CH->connect1 += CH->op1_out = OP_OUT(CH->SLOT[SLOT1],env_out,feedback1);
	}
	else CH->op1_out = 0;
	/* SLOT 2 */
	env_out=FM_CALC_SLOT(&CH->SLOT[SLOT2]);
	if( env_out < EG_ENT-1 )
		outd[0] += OP_OUT(CH->SLOT[SLOT2],env_out, feedback2);
}

/* ---------- calcrate rythm block ---------- */
	static int wcnt=0;
INLINE void OPL_CALC_RH( OPL_CH *CH )
{
	int env_tam,env_sd,env_top,env_hh;
	int whitenoise = (rand()&1)*EG_ENT;
	signed int tone7;

	// SD  (17) = mul14[fnum7] + white noise
	// TAM (15) = mul15[fnum8]
	// TOP (18) = fnum7(mul18[fnum8]+whitenoise)
	// HH  (14) = fnum7(mul18[fnum8]+whitenoise) + white noise
	env_sd =FM_CALC_SLOT(&CH[7].SLOT[SLOT2]);
	env_tam=FM_CALC_SLOT(&CH[8].SLOT[SLOT1]);
	env_top=FM_CALC_SLOT(&CH[8].SLOT[SLOT2]);
	env_hh =FM_CALC_SLOT(&CH[7].SLOT[SLOT1]);

	CH[7].SLOT[SLOT1].Cnt += CH[7].SLOT[SLOT1].Incr;

	/* TAM */
	if( env_tam < EG_ENT-1 )
		outd[0] += OP_OUT(CH[8].SLOT[SLOT1],env_tam, 0);

	tone7 = SIN_TABLE[(CH[7].fc/(0x1000000/SIN_ENT))&(SIN_ENT-1)][0];

	if( (wcnt^=1) )
	{	/* white noise phase */
		/* SD */
		if( env_sd < EG_ENT-1 )
			outd[0] += TL_TABLE[env_sd+whitenoise];
		/* TOP-CY */
		if( env_top < EG_ENT-1 )
			outd[0] += OP_OUT(CH[8].SLOT[SLOT2],env_top,tone7);
		/* HH */
		//if( env_hh  < EG_ENT-1 )
			//outd[0] += TL_TABLE[env_hh+whitenoise];
	}
	else
	{
		/* SD */
		if( env_sd < EG_ENT-1 )
			outd[0] += OP_OUT(CH[7].SLOT[SLOT1],env_sd, 0);
		/* TOP-CY */
		if( env_top < EG_ENT-1 )
			outd[0] += OP_OUT(CH[8].SLOT[SLOT2],env_top,tone7);
		/* HH */
		if( env_hh < EG_ENT-1 )
			outd[0] += OP_OUT(CH[8].SLOT[SLOT2],env_hh,tone7);
	}
}

/* ----------- initialize time tabls ----------- */
static void init_timetables( FM_OPL *OPL , int ARRATE , int DRRATE )
{
	int i,d;
	double rate;

	/* make attack rate & decay rate tables */
	for (i = 0;i < 4;i++) OPL->AR_TABLE[i] = OPL->DR_TABLE[i] = 0;
	for (i = 4;i <= 60;i++){
		rate  = (double)OPL->freqbase / 4096.0;		/* frequency rate */
		if( i < 60 ) rate *= 1.0+(i&3)*0.25;		/* b0-1 : x1 , x1.25 , x1.5 , x1.75 */
		rate *= 1<<((i>>2)-1);						/* b2-5 : shift bit */
		rate *= (double)(EG_ENT<<ENV_BITS);
		OPL->AR_TABLE[i] = rate / ARRATE;
		OPL->DR_TABLE[i] = rate / DRRATE;
	}
	for (i = 60;i < 76;i++)
	{
		OPL->AR_TABLE[i] = EG_AED-1;
		OPL->DR_TABLE[i] = OPL->DR_TABLE[60];
	}
#if 0
	for (i = 0;i < 64 ;i++){	/* make for overflow area */
		Log(LOG_WAR,"rate %2d , ar %f ms , dr %f ms \n",i,
			((double)(EG_ENT<<ENV_BITS) / OPL->AR_TABLE[i]) * (1000.0 / OPL->rate),
			((double)(EG_ENT<<ENV_BITS) / OPL->DR_TABLE[i]) * (1000.0 / OPL->rate) );
	}
#endif
}

/* ---------- generic table initialize ---------- */
static int OPLOpenTable( void )
{
	int s,t;
	double rate;
	int i,j;
	double pom;

	/* allocate dynamic tables */
	if( (TL_TABLE = malloc(TL_MAX*2*sizeof(int))) == NULL)
		return 0;
	if( (SIN_TABLE = malloc(SIN_ENT*4 *sizeof(signed int *))) == NULL)
	{
		free(TL_TABLE);
		return 0;
	}
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
	for (s = 0;s <= SIN_ENT;s++)
	{
		SIN_TABLE[SIN_ENT*1+s] = s<(SIN_ENT/2) ? SIN_TABLE[s] : 0;
		SIN_TABLE[SIN_ENT*2+s] = SIN_TABLE[s % (SIN_ENT/2)];
		SIN_TABLE[SIN_ENT*3+s] = (s/(SIN_ENT/4))&1 ? 0 : SIN_TABLE[s];
	}
	/* envelope counter -> envelope output table */
	for (i=0; i<EG_ENT; i++)
	{
		/* ATTACK curve */
		pom = pow( ((double)(EG_ENT-1-i)/EG_ENT) , 8 ) * EG_ENT;
		/* if( pom >= EG_ENT ) pom = EG_ENT-1; */
		ENV_CURVE[i] = (int)pom;
		/* DECAY ,RELEASE curve */
		ENV_CURVE[(EG_DST>>ENV_BITS)+i]= i;
	}
	/* off */
	ENV_CURVE[EG_OFF>>ENV_BITS]= EG_ENT-1;

	return 1;
}


static void OPLCloseTable( void )
{
	free( TL_TABLE );
	free( SIN_TABLE );
	return;
}

/* CSM Key Controll */
INLINE void CSMKeyControll(OPL_CH *CH)
{
	OPL_SLOT *slot1 = &CH->SLOT[SLOT1];
	OPL_SLOT *slot2 = &CH->SLOT[SLOT2];
	/* all key off */
	OPL_KEYOFF(slot1);
	OPL_KEYOFF(slot2);
	/* total level latch */
	slot1->TLL = slot1->TL + (CH->ksl_base>>slot1->ksl);
	slot1->TLL = slot1->TL + (CH->ksl_base>>slot1->ksl);
	/* key on */
	CH->op1_out = 0;
	OPL_KEYON(slot1);
	OPL_KEYON(slot2);
}

/* ---------- opl initialize ---------- */
static void OPL_initalize(FM_OPL *OPL)
{
	int blk,fn;

	/* frequency base */
	OPL->freqbase = (OPL->rate) ? ((double)OPL->clock * 4096.0 / OPL->rate) / 72  : 0;
	/* Timer base time */
	OPL->TimerBase = (OPL->rate) ? 1.0/((double)OPL->clock / 72.0 ) : 0;
	/* make time tables */
	init_timetables( OPL , OPL_ARRATE , OPL_DRRATE );
	/* make fnumber -> increment counter table */
	for( fn=0 ; fn < 1024 ; fn++ )
	{
		OPL->FN_TABLE[fn] = (double)fn * OPL->freqbase / 4096  * FREQ_RATE * (1<<7) / 2;
	}
}

/* ---------- write a OPL registers ---------- */
static void OPLWriteReg(FM_OPL *OPL, int r, int v)
{
	OPL_CH *CH;
	int slot;
	int block_fnum;

	switch(r&0xe0)
	{
	case 0x00: /* 00-1f:controll */
		switch(r&0x1f)
		{
		case 0x01:
			/* wave selector enable */
			if(OPL->type&OPL_TYPE_WAVESEL) OPL->wavesel = v&0x20;
			return;
		case 0x02:	/* Timer 1 */
			OPL->T[0] = (256-v)*4;
			break;
		case 0x03:	/* Timer 2 */
			OPL->T[1] = (256-v)*16;
			return;
		case 0x04:	/* IRQ clear / mask and Timer enable */
			if(v&0x80)
			{	/* IRQ flag clear */
				OPL_STATUS_RESET(OPL,0x7f);
			}
			else
			{	/* set IRQ mask ,timer enable*/
				int st1 = v&1;
				int st2 = (v>>1)&1;
				/* IRQRST,T1MSK,t2MSK,EOSMSK,BRMSK,x,ST2,ST1 */
				OPL_STATUS_RESET(OPL,v&0x78);
				OPL_STATUSMASK_SET(OPL,((~v)&0x78)|0x01);
				/* timer 2 */
				if(OPL->st[1] != st2)
				{
					double interval = st2 ? (double)OPL->T[1]*OPL->TimerBase : 0.0;
					OPL->st[1] = st2;
					if (OPL->TimerHandler) (OPL->TimerHandler)(OPL->TimerParam+1,interval);
				}
				/* timer 1 */
				if(OPL->st[0] != st1)
				{
					double interval = st1 ? (double)OPL->T[0]*OPL->TimerBase : 0.0;
					OPL->st[0] = st1;
					if (OPL->TimerHandler) (OPL->TimerHandler)(OPL->TimerParam+0,interval);
				}
			}
			return;
#if 0
		case 0x06:		/* Key Board OUT */
			if(!(OPL->type&OPL_TYPE_KEYBOARD)) return;
			return;
		case 0x07:	/* ADPCM controll : START,REC,MEMDATA,REPT,SPOFF,x,x,RST */
			if(!(OPL->type&OPL_TYPE_ADPCM)) return;
			return;
#endif
		case 0x08:	/* MODE : CSM,NOTESEL,x,x,smpl,da/ad,64k,rom */
			OPL->mode = v;
			return;
#if 0
		case 0x09:		/* START ADD */
		case 0x0a:
		case 0x0b:		/* STOP ADD  */
		case 0x0c:
		case 0x0d:		/* PRESCALE   */
		case 0x0e:
		case 0x0f:		/* ADPCM data */
		case 0x10: 		/* DELTA-N    */
		case 0x11: 		/* DELTA-N    */
		case 0x12: 		/* EG-CTRL    */
		case 0x15:		/* DAC data    */
		case 0x16:
		case 0x17:		/* SHIFT    */
			return;
		case 0x18:		/* I/O CTRL (Direction) */
			if(!(OPL->type&OPL_TYPE_IO)) return;
			/* OPL->portDirection = v&0x0f; */
			return;
		case 0x19:		/* I/O DATA */
			if(!(OPL->type&OPL_TYPE_IO)) return;
			/* OPL->portLatch = v; */
			/* if(OPL->porthandler_w) OPL->porthandler_w(v&OPL->IODirection); */
			return;
		case 0x1a:		/* PCM data */
			return;
#endif
		}
		break;
	case 0x20:	/* am,vib,ksr,eg type,mul */
		slot = slot_array[r&0x1f];
		if(slot == -1) return;
		set_mul(OPL,slot,v);
		return;
	case 0x40:
		slot = slot_array[r&0x1f];
		if(slot == -1) return;
		set_ksl_tl(OPL,slot,v);
		return;
	case 0x60:
		slot = slot_array[r&0x1f];
		if(slot == -1) return;
		set_ar_dr(OPL,slot,v);
		return;
	case 0x80:
		slot = slot_array[r&0x1f];
		if(slot == -1) return;
		set_sl_rr(OPL,slot,v);
		return;
	case 0xa0:
		switch(r)
		{
		case 0xbd:
			/* amdep,vibdep,r,bd,sd,tom,tc,hh */
			OPL->amdep  = (v>>7)&1; /* 1=4.8dB , 0=1dB */
			OPL->vibdep = (v>>6)&1; /* 1=14cent , 0=7cent */
			OPL->rythm  = (v>>5)&1;
			if(OPL->rythm)
			{
#if 0
				usrintf_showmessage("OPL Rythm mode select");
#endif
				/* BD */
				if(v&0x10)
				{
					OPL->P_CH[6].op1_out = 0;
					OPL_KEYON(&OPL->P_CH[6].SLOT[SLOT1]);
					OPL_KEYON(&OPL->P_CH[6].SLOT[SLOT2]);
				}
				else
				{
					OPL_KEYOFF(&OPL->P_CH[6].SLOT[SLOT1]);
					OPL_KEYOFF(&OPL->P_CH[6].SLOT[SLOT2]);
				}
				/* SD */
				if(v&0x08) OPL_KEYON(&OPL->P_CH[7].SLOT[SLOT2]);
				else       OPL_KEYOFF(&OPL->P_CH[7].SLOT[SLOT2]);
				/* TAM */
				if(v&0x04) OPL_KEYON(&OPL->P_CH[8].SLOT[SLOT1]);
				else       OPL_KEYOFF(&OPL->P_CH[8].SLOT[SLOT1]);
				/* TOP-CY */
				if(v&0x02) OPL_KEYON(&OPL->P_CH[8].SLOT[SLOT2]);
				else       OPL_KEYOFF(&OPL->P_CH[8].SLOT[SLOT2]);
				/* HH */
				if(v&0x01) OPL_KEYON(&OPL->P_CH[7].SLOT[SLOT1]);
				else       OPL_KEYOFF(&OPL->P_CH[7].SLOT[SLOT1]);
			}
			return;
		}
		/* keyon,block,fnum */
		if( (r&0x0f) > 8) return;
		CH = &OPL->P_CH[r&0x0f];
		if(!(r&0x10))
		{	/* a0-a8 */
			block_fnum  = (CH->block_fnum&0x1f00) | v;
		}
		else
		{	/* b0-b8 */
			int keyon = (v>>5)&1;
			block_fnum = ((v&0x1f)<<8) | (CH->block_fnum&0xff);
			if(CH->keyon != keyon)
			{
				if( (CH->keyon=keyon) )
				{
					CH->op1_out = 0;
					OPL_KEYON(&CH->SLOT[SLOT1]);
					OPL_KEYON(&CH->SLOT[SLOT2]);
				}
				else
				{
					OPL_KEYOFF(&CH->SLOT[SLOT1]);
					OPL_KEYOFF(&CH->SLOT[SLOT2]);
				}
			}
		}
		/* update */
		if(CH->block_fnum != block_fnum)
		{
			int blockRv = 7-(block_fnum>>10);
			int fnum   = block_fnum&0x3ff;
			CH->block_fnum = block_fnum;

			CH->ksl_base = KSL_TABLE[block_fnum>>6];
			CH->fc = OPL->FN_TABLE[fnum]>>blockRv;
			CH->kcode = CH->block_fnum>>9;
			if( (OPL->mode&0x40) && CH->block_fnum&0x100) CH->kcode |=1;
			CALC_FCSLOT(CH,&CH->SLOT[SLOT1]);
			CALC_FCSLOT(CH,&CH->SLOT[SLOT2]);
		}
		return;
	case 0xc0:
		/* FB,C */
		if( (r&0x0f) > 8) return;
		CH = &OPL->P_CH[r&0x0f];
		{
		int feedback = (v>>1)&7;
		CH->FB   = feedback ? 8 - feedback : 31;
		CH->CON = v&1;
		set_algorythm(CH);
		}
		return;
	case 0xe0: /* wave type */
		slot = slot_array[r&0x1f];
		if(slot == -1) return;
		CH = &OPL->P_CH[slot/2];
		if(OPL->wavesel)
		{
			/* Log(LOG_INF,"OPL SLOT %d wave select %d\n",slot,v&3); */
			CH->SLOT[slot].wavetable = &SIN_TABLE[(v&0x03)*SIN_ENT];
		}
		return;
	}
}

/* lock/unlock for common table */
static int OPL_LockTable(void)
{
	num_lock++;
	if(num_lock>1) return 0;
	/* first time */
	cur_chip = NULL;
	/* allocate total level table (128kb space) */
	if( !OPLOpenTable() )
	{
		num_lock--;
		return -1;
	}
	return 0;
}

static void OPL_UnLockTable(void)
{
	if(num_lock) num_lock--;
	if(num_lock) return;
	/* last time */
	cur_chip = NULL;
	OPLCloseTable();
}

#if (BUILD_YM3812 || BUILD_YM3526)
/*******************************************************************************/
/*		YM3812 local section                                                   */
/*******************************************************************************/

/* ---------- update one of chip ----------- */
void YM3812UpdateOne(FM_OPL *OPL, void *buffer, int length)
{
    int i,ch;
	int data;
	FMSAMPLE *buf = (FMSAMPLE *)buffer;
	unsigned int amsCnt  = OPL->amsCnt;
	unsigned int vibCnt  = OPL->vibCnt;
	int rythm = OPL->rythm;
	OPL_CH *CH,*R_CH;

	if( (void *)OPL != cur_chip ){
		cur_chip = (void *)OPL;
		/* channel pointers */
		S_CH = OPL->P_CH;
		E_CH = &S_CH[9];
		/* LFO state */
		amsIncr = OPL->amsIncr;
		vibIncr = OPL->vibIncr;
	}
	R_CH = rythm ? &S_CH[7] : E_CH;

    for( i=0; i < length ; i++ )
	{
		/*            channel A         channel B         channel C      */
		/* LFO */
		ams = 0; /* (amsCnt+=amsIncr) */
		vib = 0; /* (vibCnt+=vibIncr) */
		outd[0] = 0;
		/* FM part */
		for(CH=S_CH ; CH < R_CH ; CH++)
			OPL_CALC_CH(CH);
		/* Rythn part */
		if(rythm)
		{
			OPL_CALC_RH(S_CH);
		}/* limit check */
		data = Limit( outd[0] , OPL_MAXOUT, OPL_MINOUT );
		/* store to sound buffer */
		buf[i] = data >> OPL_OUTSB;
	}

	OPL->amsCnt = amsCnt;
	OPL->vibCnt = vibCnt;
}
#endif /* (BUILD_YM3812 || BUILD_YM3526) */

#if BUILD_Y8950
void Y8950UpdateOne(FM_OPL *OPL, void *buffer, int length)
{
}
#endif

/* ---------- reset one of chip ---------- */
void OPLResetChip(FM_OPL *OPL)
{
	int c,s;
	int i;

	/* reset chip */
	OPL->mode   = 0;	/* normal mode */
	OPL_STATUS_RESET(OPL,0x7f);
	/* reset with register write */
	OPLWriteReg(OPL,0x02,0); /* Timer1 */
	OPLWriteReg(OPL,0x03,0); /* Timer2 */
	OPLWriteReg(OPL,0x04,0); /* IRQ mask clear */
	OPLWriteReg(OPL,0x01,0x00); /* Test flag */
	for(i = 0xff ; i >= 0x20 ; i-- ) OPLWriteReg(OPL,i,0);
	/* reset OPerator paramater */
	for( c = 0 ; c < OPL->max_ch ; c++ )
	{
		OPL_CH *CH = &OPL->P_CH[c];
		/* OPL->P_CH[c].PAN = OPN_CENTER; */
		for(s = 0 ; s < 2 ; s++ )
		{
			/* wave table */
			CH->SLOT[s].wavetable = &SIN_TABLE[0];
			/* CH->SLOT[s].evm = ENV_MOD_RR; */
			CH->SLOT[s].evc = EG_OFF;
			CH->SLOT[s].eve = EG_OFF+1;
			CH->SLOT[s].evs = 0;
		}
	}
}

/* ----------  Create one of vietual YM3812 ----------       */
/* 'rate'  is sampling rate and 'bufsiz' is the size of the  */
FM_OPL *OPLCreate(int type, int clock, int rate)
{
	int i;
	char *ptr;
	FM_OPL *OPL;
	int state_size;
	int max_ch = 9; /* normaly 9 channels */

	if( OPL_LockTable() ==-1) return NULL;
	/* allocate OPL state space */
	state_size  = sizeof(FM_OPL);
	state_size += sizeof(OPL_CH)*max_ch;
	/* if(type&OPL_TYPE_ADPCM) state_size+= sizeof(YM_DELTAT) */
	/* allocate memory block */
	ptr = malloc(state_size);
	if(ptr==NULL) return NULL;
	/* clear */
	memset(ptr,0,state_size);
	OPL        = (FM_OPL *)ptr; ptr+=sizeof(FM_OPL);
	OPL->P_CH  = (OPL_CH *)ptr; ptr+=sizeof(OPL_CH)*max_ch;
	/* if(type&OPL_TYPE_ADPCM) OPL->adpcm = (FM_DELTAT *)ptr; ptr+=sizeof(YM_DELTAT) */

	/* set channel state pointer */
	OPL->type  = type;
	OPL->clock = clock;
	OPL->rate  = rate;
	OPL->max_ch = max_ch;
	/* init grobal tables */
	OPL_initalize(OPL);
	/* reset chip */
	OPLResetChip(OPL);
	return OPL;
}

/* ----------  Destroy one of vietual YM3812 ----------       */
void OPLDestroy(FM_OPL *OPL)
{
	OPL_UnLockTable();
	free(OPL);
}

/* ----------  Option handlers ----------       */

void OPLSetTimerHandler(FM_OPL *OPL,OPL_TIMERHANDLER TimerHandler,int channelOffset)
{
	OPL->TimerHandler   = TimerHandler;
	OPL->TimerParam = channelOffset;
}
void OPLSetIRQHandler(FM_OPL *OPL,OPL_IRQHANDLER IRQHandler,int param)
{
	OPL->IRQHandler     = IRQHandler;
	OPL->IRQParam = param;
}
void OPLSetUpdateHandler(FM_OPL *OPL,OPL_UPDATEHANDLER UpdateHandler,int param)
{
	OPL->UpdateHandler = UpdateHandler;
	OPL->UpdateParam = param;
}

/* ---------- YM3812 I/O interface ---------- */
int OPLWrite(FM_OPL *OPL,int a,int v)
{
	if( !(a&1) )
	{	/* address port */
		OPL->address = v & 0xff;
	}
	else
	{	/* data port */
		if(OPL->UpdateHandler) OPL->UpdateHandler(OPL->UpdateParam,0);
		OPLWriteReg(OPL,OPL->address,v);
	}
	return OPL->status>>7;
}

unsigned char OPLRead(FM_OPL *OPL,int a)
{
	if( !(a&1) )
	{	/* status port */
		return OPL->status & (OPL->statusmask|0x80);
	}
#if 0
	/* data port */
	switch(OPL->address)
	{
	case 0x05: /* KeyBoard IN */
		return 0;
	case 0x0f: /* ADPCM-DATA  */
		return 0;
	case 0x19: /* I/O DATA    */
		if(!(OPL->type&OPL_TYPE_IO)) return 0;
		/* if(OPL->porthandler_w) return OPL->porthandler_w(); */
		return 0;
	case 0x1a: /* PCM-DATA    */
		return 0;
	}
#endif
	return 0;
}

int OPLTimerOver(FM_OPL *OPL,int c)
{
	if( c )
	{	/* Timer B */
		OPL_STATUS_SET(OPL,0x20);
	}
	else
	{	/* Timer A */
		OPL_STATUS_SET(OPL,0x40);
		/* CSM mode key,TL controll */
		if( OPL->mode & 0x80 )
		{	/* CSM mode total level latch and auto key on */
			if(OPL->UpdateHandler) OPL->UpdateHandler(OPL->UpdateParam,0);
			CSMKeyControll( &OPL->P_CH[0] );
			CSMKeyControll( &OPL->P_CH[1] );
			CSMKeyControll( &OPL->P_CH[2] );
			CSMKeyControll( &OPL->P_CH[3] );
			CSMKeyControll( &OPL->P_CH[4] );
			CSMKeyControll( &OPL->P_CH[5] );
			/* !!!!! */
		}
	}
	/* reload timer */
	if (OPL->TimerHandler) (OPL->TimerHandler)(OPL->TimerParam+c,(double)OPL->T[c]*OPL->TimerBase);
	return OPL->status>>7;
}
