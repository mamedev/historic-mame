/****************************************************************************

  emu2413.c -- a YM2413 emulator : written by Mitsutaka Okazaki 2001

  2001 01-08 : Version 0.10 -- 1st version.
  2001 01-15 : Version 0.20 -- semi-public version.
  2001 01-16 : Version 0.30 -- 1st public version.
  2001 01-17 : Version 0.31 -- Fixed bassdrum problem.
             : Version 0.32 -- LPF implemented.
  2001 01-18 : Version 0.33 -- Fixed the drum problem, refine the mix-down method.
                            -- Fixed the LFO bug.
  2001 01-24 : Version 0.35 -- Fixed the drum problem,
                               support undocumented EG behavior.
  2001 02-02 : Version 0.38 -- Improved the performance.
                               Fixed the hi-hat and cymbal model.
                               Fixed the default percussive datas.
                               Noise reduction.
                               Fixed the feedback problem.
  References:
    fmopl.c        -- 1999,2000 written by Tatsuyuki Satoh (MAME development).
    fmopl.c(fixed) -- 2000 modified by mamiya (NEZplug development).
    fmgen.cpp      -- 1999,2000 written by cisc.
    fmpac.ill      -- 2000 created by NARUTO.
    MSX-Datapack
    YMU757 data sheet
    YM2143 data sheet

  http://www.angel.ne.jp/~okazaki/ym2413/index_en.html

*****************************************************************************/

#include "driver.h"
#include <math.h>
#include "ym2413.h"

/*
  If you want to use the attack, decay and release time
  which are introduced on YM2413 specification sheet,
  please define USE_SPEC_ENV_SPEED(not recommended).
*/
/*#define USE_SPEC_ENV_SPEED*/

/* Size of Sintable ( 1 -- 18 can be used, but 7 -- 14 recommended.)*/
#define PG_BITS 9
#define PG_WIDTH (1<<PG_BITS)

/* Phase increment counter */
#define DP_BITS 18
#define DP_WIDTH (1<<DP_BITS)
#define DP_BASE_BITS (DP_BITS - PG_BITS)

/* Bits number for 48dB (7:0.375dB/step, 8:0.1875dB/step) */
#define DB_BITS 8
/* Resolution */
#define DB_STEP ((double)48.0/(1<<DB_BITS))
/* Resolution of sine table */
#define SIN_STEP (DB_STEP/16)

#define DB_MUTE ((1<<DB_BITS)-1)

/* Volume of Noise (dB) */
#define DB_NOISE 6

/* Bits for envelope */
#define EG_BITS 7

/* Bits for total level */
#define TL_BITS   6

/* Bits for sustine level */
#define SL_BITS   4

/* Bits for liner value */
#define DB2LIN_AMP_BITS 9
#define SLOT_AMP_BITS (DB2LIN_AMP_BITS)

/* Bits for envelope phase incremental counter */
#define EG_DP_BITS 22
#define EG_DP_WIDTH (1<<EG_DP_BITS)

/* Bits for Pitch and Amp modulator */
#define PM_PG_BITS 8
#define PM_PG_WIDTH (1<<PM_PG_BITS)
#define PM_DP_BITS 16
#define PM_DP_WIDTH (1<<PM_DP_BITS)
#define AM_PG_BITS 8
#define AM_PG_WIDTH (1<<AM_PG_BITS)
#define AM_DP_BITS 16
#define AM_DP_WIDTH (1<<AM_DP_BITS)

/* PM table is calcurated by PM_AMP * pow(2,PM_DEPTH*sin(x)/1200) */
#define PM_AMP_BITS 8
#define PM_AMP (1<<PM_AMP_BITS)

/* PM speed(Hz) and depth(cent) */
#define PM_SPEED 6.4
#define PM_DEPTH 13.75

/* AM speed(Hz) and depth(dB) */
#define AM_SPEED 3.7
#define AM_DEPTH 4.8

/* Cut the lower b bit(s) off. */
#define HIGHBITS(c,b) ((c)>>(b))

/* Leave the lower b bit(s). */
#define LOWBITS(c,b) ((c)&((1<<(b))-1))

/* Expand x which is s bits to d bits. */
#define EXPAND_BITS(x,s,d) ((x)<<((d)-(s)))

/* Expand x which is s bits to d bits and fill expanded bits '1' */
#define EXPAND_BITS_X(x,s,d) (((x)<<((d)-(s)))|((1<<((d)-(s)))-1))

/* Adjust envelope speed which depends on sampling rate. */
#define rate_adjust(x) (UINT32)((double)(x)*clk/72/rate + 0.5) /* +0.5 for round */

#define MOD(x) slot[x*2]
#define CAR(x) slot[x*2+1]
/*
#define MOD(x) ch[x]->mod
#define CAR(x) ch[x]->car
*/

/* Sampling rate */
static INT32 rate ;

/* Input clock */
static double clk ;

/* WaveTable for each envelope amp */
static INT32 fullsintable[PG_WIDTH] ;
static INT32 halfsintable[PG_WIDTH] ;

static INT32 *waveform[2] = {fullsintable,halfsintable} ;

/* LFO Table */
static INT32 pmtable[PM_PG_WIDTH] ;
static INT32 amtable[AM_PG_WIDTH] ;

/* dB to Liner table */
static INT32 DB2LIN_TABLE[DB_MUTE+1] ;

/* Liner to Log curve conversion table (for Attack rate). */
static UINT32 AR_ADJUST_TABLE[1<<EG_BITS] ;

/* Empty voice data */
static OPLL_PATCH null_patch = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } ;

/* Basic voice Data */
static OPLL_PATCH default_patch[(16+3)*2] ;

static unsigned char default_inst[(16+3)*16]={
0x49, 0x4C, 0x4C, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x61, 0x61, 0x1E, 0x17, 0xF0, 0x7F, 0x07, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x13, 0x41, 0x0F, 0x0D, 0xCE, 0xD2, 0x43, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x03, 0x01, 0x99, 0x04, 0xFF, 0xC3, 0x03, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x21, 0x61, 0x1B, 0x07, 0xAF, 0x63, 0x40, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x22, 0x21, 0x1E, 0x06, 0xF0, 0x76, 0x08, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x31, 0x22, 0x16, 0x05, 0x90, 0x7F, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x21, 0x61, 0x1D, 0x07, 0x82, 0x81, 0x10, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x23, 0x21, 0x2D, 0x16, 0xC0, 0x70, 0x07, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x61, 0x21, 0x1B, 0x06, 0x64, 0x65, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x61, 0x61, 0x0C, 0x18, 0x85, 0xA0, 0x79, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x23, 0x21, 0x87, 0x11, 0xF0, 0xA4, 0x00, 0xF7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x97, 0xE1, 0x28, 0x07, 0xFF, 0xF3, 0x02, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x61, 0x10, 0x0C, 0x05, 0xF2, 0xC4, 0x40, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x01, 0x01, 0x56, 0x03, 0xB4, 0xB2, 0x23, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x61, 0x41, 0x89, 0x03, 0xF1, 0xF4, 0xF0, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x08, 0x21, 0x28, 0x00, 0xDF, 0xF8, 0xFF, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x23, 0x22, 0x00, 0x00, 0xA8, 0xF8, 0xF8, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x35, 0x18, 0x00, 0x00, 0xF7, 0xA9, 0xF7, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
} ;

/* Definition of envelope mode */
enum { SETTLE,ATTACK,DECAY,SUSHOLD,SUSTINE,RELEASE,FINISH } ;

#ifdef USE_SPEC_ENV_SPEED
static double attacktime[16][4] = {
  {0,0,0,0},
  {1730.15, 1400.60, 1153.43, 988.66},
  {865.08, 700.30, 576.72, 494.33},
  {432.54, 350.15, 288.36, 247.16},
  {216.27, 175.07, 144.18, 123.58},
  {108.13, 87.54, 72.09, 61.79},
  {54.07, 43.77, 36.04, 30.90},
  {27.03, 21.88, 18.02, 15.45},
  {13.52, 10.94, 9.01, 7.72},
  {6.76, 5.47, 4.51, 3.86},
  {3.38, 2.74, 2.25, 1.93},
  {1.69, 1.37, 1.13, 0.97},
  {0.84, 0.70, 0.60, 0.54},
  {0.50, 0.42, 0.34, 0.30},
  {0.28, 0.22, 0.18, 0.14},
  {0.00, 0.00, 0.00, 0.00}
} ;

static double decaytime[16][4] = {
  {0,0,0,0},
  {20926.60,16807.20,14006.00,12028.60},
  {10463.30,8403.58,7002.98,6014.32},
  {5231.64,4201.79,3501.49,3007.16},
  {2615.82,2100.89,1750.75,1503.58},
  {1307.91,1050.45,875.37,751.79},
  {653.95,525.22,437.69,375.90},
  {326.98,262.61,218.84,187.95},
  {163.49,131.31,109.42,93.97},
  {81.74,65.65,54.71,46.99},
  {40.87,32.83,27.36,23.49},
  {20.44,16.41,13.68,11.75},
  {10.22,8.21,6.84,5.87},
  {5.11,4.10,3.42,2.94},
  {2.55,2.05,1.71,1.47},
  {1.27,1.27,1.27,1.27}
} ;
static UINT32 attacktable[16][4] ;
static UINT32 decaytable[16][4] ;
#endif

/* Phase incr table for Attack */
static UINT32 dphaseARTable[16][16] ;
/* Phase incr table for Decay and Release */
static UINT32 dphaseDRTable[16][16] ;

/* KSL + TL Table */
static UINT32 tllTable[16][8][1<<TL_BITS][4] ;
static INT32 rksTable[2][8][2] ;

/* Phase incr table for PG */
static UINT32 dphaseTable[512][8][16] ;


/***************************************************

                  Create tables

****************************************************/
INLINE INT32 Max(INT32 i,INT32 j){

  if(i>j) return i ; else return j ;

}

INLINE INT32 Min(INT32 i,INT32 j){

  if(i<j) return i ; else return j ;

}

/* Table for AR to LogCurve. */
static void makeAdjustTable(void){

  int i ;

  AR_ADJUST_TABLE[0] = (1<<EG_BITS) ;
  for ( i=1 ; i < 128 ; i++)
    AR_ADJUST_TABLE[i] = (UINT32)((double)(1<<EG_BITS) - 1 - (1<<EG_BITS) * log(i) / log(128)) ;

}


/* Table for dB(0 -- (1<<DB_BITS)) to Liner(0 -- DB2LIN_AMP_WIDTH) */
static void makeDB2LinTable(void){

  int i ;

  for( i=0 ; i < DB_MUTE ; i++)
    DB2LIN_TABLE[i] = (INT32)((double)(1<<DB2LIN_AMP_BITS) * pow(10,-(double)i*DB_STEP/20)) ;

  DB2LIN_TABLE[DB_MUTE] = 0 ;

}

/* Liner(+0.0 - +1.0) to dB((1<<DB_BITS) - 1 -- 0) */
static INT32 lin2db(double d){

  if(d == 0) return (1<<DB_BITS) - 1 ;
  else return (INT32)(-((INT32)((double)20*log10(d)/SIN_STEP)) * (SIN_STEP/DB_STEP)) ;

}

/*
   Sin Table
   Plus(minus) 1.0dB to positive(negative) area to distinguish +0 and -0dB)
 */
static void makeSinTable(void){

  int i ;

  for( i = 0 ; i < PG_WIDTH/4 ; i++ )
    fullsintable[i] = lin2db(sin(2.0*PI*i/PG_WIDTH)) + 1 ;

  for( i = 0 ; i < PG_WIDTH/4 ; i++ )
    fullsintable[PG_WIDTH/2 - 1 - i] = fullsintable[i] ;

  for( i = 0 ; i < PG_WIDTH/2 ; i++ )
    fullsintable[PG_WIDTH/2+i] = -fullsintable[i] ;

  for( i = 0 ; i < PG_WIDTH/2 ; i++ )
    halfsintable[i] = fullsintable[i] ;

  for( i = PG_WIDTH/2 ; i< PG_WIDTH ; i++ )
    halfsintable[i] = fullsintable[0] ;

}

/* Table for Pitch Modulator */
static void makePmTable(void){

  int i ;

  for(i = 0 ; i < PM_PG_WIDTH ; i++ )
    pmtable[i] = (INT32)((double)PM_AMP * pow(2,(double)PM_DEPTH*sin(2.0*PI*i/PM_PG_WIDTH)/1200)) ;

}

/* Table for Amp Modulator */
static void makeAmTable(void){

  int i ;

  for(i = 0 ; i < AM_PG_WIDTH ; i++ )
    amtable[i] = (INT32)((double)AM_DEPTH/2/DB_STEP * (1.0 + sin(2.0*PI*i/PM_PG_WIDTH))) ;

}

/* Phase increment counter table */
static void makeDphaseTable(void){

  UINT32 fnum, block , ML ;
  UINT32 mltable[16]={ 1,1*2,2*2,3*2,4*2,5*2,6*2,7*2,8*2,9*2,10*2,10*2,12*2,12*2,15*2,15*2 } ;

  for(fnum=0; fnum<512; fnum++)
    for(block=0; block<8; block++)
      for(ML=0; ML<16; ML++)
        dphaseTable[fnum][block][ML] = rate_adjust(((fnum * mltable[ML]) << block ) >> (20 - DP_BITS)) ;

}

static void makeTllTalbe(void){

#if DB_BITS == 7
#define dB2(x) (UINT32)(((UINT32)(x/0.375)) * 0.375 * 2)
#else
#define dB2(x) (UINT32)((x)*2)
#endif

	static UINT32 kltable[16] = {
    dB2( 0.000),dB2( 9.000),dB2(12.000),dB2(13.875),dB2(15.000),dB2(16.125),dB2(16.875),dB2(17.625),
    dB2(18.000),dB2(18.750),dB2(19.125),dB2(19.500),dB2(19.875),dB2(20.250),dB2(20.625),dB2(21.000)
	} ;

  INT32 tmp ;
  int fnum, block ,TL , KL ;

  for(fnum=0; fnum<16; fnum++)
    for(block=0; block<8; block++)
      for(TL=0; TL<64; TL++)
        for(KL=0; KL<4; KL++){

          if(KL==0){
            tllTable[fnum][block][TL][KL] = EXPAND_BITS(TL,TL_BITS,DB_BITS) ;
          }else{
  	        tmp = kltable[fnum] - dB2(3.000) * (7 - block) ;
            if(tmp <= 0)
              tllTable[fnum][block][TL][KL] = EXPAND_BITS(TL,TL_BITS,DB_BITS) ;
            else
              tllTable[fnum][block][TL][KL] = (UINT32)((tmp >> ( 3 - KL ))/DB_STEP) + EXPAND_BITS(TL,TL_BITS,DB_BITS) ;
          }

       }

}

/* Rate Table for Attack */
static void makeDphaseARTable(void){

  int AR,Rks,RM,RL ;

#ifdef USE_SPEC_ENV_SPEED
  for(RM=0; RM<16; RM++)
    for(RL=0; RL<4 ; RL++) {
      if(RM == 0)
        attacktable[RM][RL] = 0 ;
      else if(RM == 15)
        attacktable[RM][RL] = EG_DP_WIDTH ;
      else
        attacktable[RM][RL] = (UINT32)((double)(1<<EG_DP_BITS)/(attacktime[RM][RL]*clk/72/1000));

    }
#endif

  for(AR=0; AR<16; AR++)
    for(Rks=0; Rks<16; Rks++){
      RM = AR + (Rks>>2) ;
      if(RM>15) RM = 15 ;
      RL = Rks&3 ;
      switch(AR){
       case 0:
        dphaseARTable[AR][Rks] = 0 ;
        break ;
      case 15:
        dphaseARTable[AR][Rks] = EG_DP_WIDTH ;
        break ;
      default:
#ifdef USE_SPEC_ENV_SPEED
        dphaseARTable[AR][Rks] = rate_adjust(attacktable[RM][RL]);
#else
        dphaseARTable[AR][Rks] = rate_adjust(( 3 * (RL + 4) << (RM + 1))) ;
#endif
        break ;
      }

    }

}

static void makeDphaseDRTable(void){

  int DR,Rks,RM,RL ;

#ifdef USE_SPEC_ENV_SPEED
  for(RM=0; RM<16; RM++)
    for(RL=0; RL<4 ; RL++)
      if(RM == 0)
        decaytable[RM][RL] = 0 ;
      else
        decaytable[RM][RL] = (UINT32)((double)(1<<EG_DP_BITS)/(decaytime[RM][RL]*clk/72/1000)) ;
#endif

  for(DR=0; DR<16; DR++)
    for(Rks=0; Rks<16; Rks++){
      RM = DR + (Rks>>2) ;
      RL = Rks&3 ;
      if(RM>15) RM = 15 ;
      switch(DR){
      case 0:
        dphaseDRTable[DR][Rks] = 0 ;
        break ;
      default:
#ifdef USE_SPEC_ENV_SPEED
        dphaseDRTable[DR][Rks] = rate_adjust(decaytable[RM][RL]) ;
#else
        dphaseDRTable[DR][Rks] = rate_adjust((RL + 4) << (RM - 1));
#endif
        break ;
      }

    }

}

static void makeRksTable(void){

  int fnum8, block, KR ;

  for(fnum8 = 0 ; fnum8 < 2 ; fnum8++)
    for(block = 0 ; block < 8 ; block++)
      for(KR = 0; KR < 2 ; KR++){
        if(KR!=0)
          rksTable[fnum8][block][KR] = ( block << 1 ) + fnum8 ;
        else
          rksTable[fnum8][block][KR] = block >> 1 ;
      }


}


void dump2patch(unsigned char *dump, OPLL_PATCH *patch){

  patch[0].AM = (dump[0]>>7)&1 ;
  patch[1].AM = (dump[1]>>7)&1 ;
  patch[0].PM = (dump[0]>>6)&1 ;
  patch[1].PM = (dump[1]>>6)&1 ;
  patch[0].EG = (dump[0]>>5)&1 ;
  patch[1].EG = (dump[1]>>5)&1 ;
  patch[0].KR = (dump[0]>>4)&1 ;
  patch[1].KR = (dump[1]>>4)&1 ;
  patch[0].ML = (dump[0])&15 ;
  patch[1].ML = (dump[1])&15 ;
  patch[0].KL = (dump[2]>>6)&3 ;
  patch[1].KL = (dump[3]>>6)&3 ;
  patch[0].TL = (dump[2])&63 ;
  patch[0].FB = (dump[3])&7 ;
  patch[0].WF = (dump[3]>>3)&1 ;
  patch[1].WF = (dump[3]>>4)&1 ;
  patch[0].AR = (dump[4]>>4)&15 ;
  patch[1].AR = (dump[5]>>4)&15 ;
  patch[0].DR = (dump[4])&15 ;
  patch[1].DR = (dump[5])&15 ;
  patch[0].SL = (dump[6]>>4)&15 ;
  patch[1].SL = (dump[7]>>4)&15 ;
  patch[0].RR = (dump[6])&15 ;
  patch[1].RR = (dump[7])&15 ;

}

static void makeDefaultPatch(void){

  int i ;

  for(i=1;i<19;i++)
    dump2patch(default_inst+(i*16),&default_patch[i*2]) ;

}

/************************************************************

                      Calc Parameters

************************************************************/

INLINE UINT32 calc_eg_dphase(OPLL_SLOT *slot){

  switch(slot->eg_mode){

  case ATTACK:
    return dphaseARTable[slot->patch->AR][slot->rks] ;

  case DECAY:
    return dphaseDRTable[slot->patch->DR][slot->rks] ;

  case SUSHOLD:
    return 0 ;

  case SUSTINE:
    return dphaseDRTable[slot->patch->RR][slot->rks] ;

  case RELEASE:
    if(slot->sustine)
      return dphaseDRTable[5][slot->rks] ;
    else if(slot->patch->EG)
      return dphaseDRTable[slot->patch->RR][slot->rks] ;
    else
      return dphaseDRTable[7][slot->rks] ;

  case FINISH:
    return 0 ;

  default:
    return 0 ;

  }

}

/*************************************************************

                    OPLL internal interfaces

*************************************************************/
#define SLOT_BD1 12
#define SLOT_BD2 13
#define SLOT_HH 14
#define SLOT_SD 15
#define SLOT_TOM 16
#define SLOT_CYM 17

#define UPDATE_PG 1
#define UPDATE_EG 2
#define UPDATE_TLL 4
#define UPDATE_RKS 8
#define UPDATE_WF 16
#define UPDATE_ALL (UPDATE_PG|UPDATE_EG|UPDATE_TLL|UPDATE_RKS|UPDATE_WF)

/* Force Refresh (When external program changes some parameters). */
void OPLL_forceRefresh(OPLL *opll){

  int i ;

  if(opll==NULL) return ;
  for(i=0; i<18 ;i++)
    opll->slot[i]->update |= UPDATE_ALL ;

}

/* Refresh slot parameters */
INLINE void refresh(OPLL_SLOT *slot){

  if(slot->update&UPDATE_PG)
    slot->dphase = dphaseTable[slot->fnum][slot->block][slot->patch->ML] ;

  if(slot->update&UPDATE_EG)
    slot->eg_dphase = calc_eg_dphase(slot) ;

  if(slot->update&UPDATE_TLL){
    if(slot->type == 0)
      slot->tll = tllTable[slot->fnum>>5][slot->block][slot->patch->TL][slot->patch->KL] ;
    else
      slot->tll = tllTable[slot->fnum>>5][slot->block][slot->volume][slot->patch->KL] ;
  }

  if(slot->update&UPDATE_RKS)
    slot->rks = rksTable[slot->fnum>>8][slot->block][slot->patch->KR] ;

  if(slot->update&UPDATE_WF)
    slot->sintbl = waveform[slot->patch->WF] ;

  slot->update = 0 ;

}

/* Slot key on  */
INLINE void slotOn(OPLL_SLOT *slot){

  slot->eg_mode = ATTACK ;
  slot->phase = 0 ;
  slot->eg_phase = 0 ;
  slot->update |= UPDATE_EG ;

}

/* Slot key off */
INLINE void slotOff(OPLL_SLOT *slot){

  if(slot->eg_mode == ATTACK)
    slot->eg_phase = EXPAND_BITS(AR_ADJUST_TABLE[HIGHBITS(slot->eg_phase, EG_DP_BITS - EG_BITS)],EG_BITS,EG_DP_BITS) ;

  slot->eg_mode = RELEASE ;
  slot->update |= UPDATE_EG ;

}

/* Channel key on */
INLINE void keyOn(OPLL *opll, int i){

  if(!opll->slot_on_flag[i*2]) slotOn(opll->MOD(i)) ;
  if(!opll->slot_on_flag[i*2+1]) slotOn(opll->CAR(i)) ;
  opll->ch[i]->key_status = 1 ;

}

/* Channel key off */
INLINE void keyOff(OPLL *opll, int i){

  if(opll->slot_on_flag[i*2+1]) slotOff(opll->CAR(i)) ;
  opll->ch[i]->key_status = 0 ;

}

/* Drum key on */
#ifdef OPLL_ENABLE_DEBUG
INLINE void keyOn_BD(OPLL *opll){
  if(!opll->slot_on_flag[SLOT_BD1]){
    slotOn(opll->MOD(6)) ;
  }
  if(!opll->slot_on_flag[SLOT_BD2]){
    slotOn(opll->CAR(6)) ;
    opll->debug_rythm_flag |= 16 ;
  }
}
INLINE void keyOn_SD(OPLL *opll){
  if(!opll->slot_on_flag[SLOT_SD]){
    slotOn(opll->CAR(7)) ;
    opll->debug_rythm_flag |= 8 ;
  }
}
INLINE void keyOn_TOM(OPLL *opll){
  if(!opll->slot_on_flag[SLOT_TOM]){
    slotOn(opll->MOD(8)) ;
    opll->debug_rythm_flag |= 4 ;
  }
}
INLINE void keyOn_HH(OPLL *opll){
  if(!opll->slot_on_flag[SLOT_HH]){
    slotOn(opll->MOD(7)) ;
    opll->debug_rythm_flag |= 2 ;
  }
}
INLINE void keyOn_CYM(OPLL *opll){
  if(!opll->slot_on_flag[SLOT_CYM]){
    slotOn(opll->CAR(8)) ;
    opll->debug_rythm_flag |= 1 ;
  }
}
#else
INLINE void keyOn_BD(OPLL *opll){ keyOn(opll,6) ; }
INLINE void keyOn_SD(OPLL *opll){ if(!opll->slot_on_flag[SLOT_SD]) slotOn(opll->CAR(7)) ; }
INLINE void keyOn_TOM(OPLL *opll){ if(!opll->slot_on_flag[SLOT_TOM]) slotOn(opll->MOD(8)) ; }
INLINE void keyOn_HH(OPLL *opll){ if(!opll->slot_on_flag[SLOT_HH]) slotOn(opll->MOD(7)) ; }
INLINE void keyOn_CYM(OPLL *opll){ if(!opll->slot_on_flag[SLOT_CYM]) slotOn(opll->CAR(8)) ; }
#endif

/* Drum key off */
INLINE void keyOff_BD(OPLL *opll){ keyOff(opll,6) ; }
INLINE void keyOff_SD(OPLL *opll){ if(opll->slot_on_flag[SLOT_SD]) slotOff(opll->CAR(7)) ; }
INLINE void keyOff_TOM(OPLL *opll){ if(opll->slot_on_flag[SLOT_TOM]) slotOff(opll->MOD(8)) ; }
INLINE void keyOff_HH(OPLL *opll){ if(opll->slot_on_flag[SLOT_HH]) slotOff(opll->MOD(7)) ; }
INLINE void keyOff_CYM(OPLL *opll){ if(opll->slot_on_flag[SLOT_CYM]) slotOff(opll->CAR(8)) ; }

/* Change a voice */
INLINE void setPatch(OPLL *opll, int i, int num){

#ifdef OPLL_ENABLE_DEBUG
  if(opll->ch[i]->key_status) {
    if(opll->ch[i]->debug_keyonpatch[0] != opll->ch[i]->patch_number + 1){
      opll->ch[i]->debug_keyonpatch[2] = opll->ch[i]->debug_keyonpatch[1] ;
      opll->ch[i]->debug_keyonpatch[1] = opll->ch[i]->debug_keyonpatch[0] ;
      opll->ch[i]->debug_keyonpatch[0] = opll->ch[i]->patch_number + 1 ;
    }
  }else{
    opll->ch[i]->debug_keyonpatch[2] = 0 ;
    opll->ch[i]->debug_keyonpatch[1] = 0 ;
    opll->ch[i]->debug_keyonpatch[0] = 0 ;
  }
#endif

  opll->ch[i]->patch_number = num ;
  opll->MOD(i)->patch = &(opll->patch[num][0]) ;
  opll->CAR(i)->patch = &(opll->patch[num][1]) ;

  opll->MOD(i)->update |= UPDATE_ALL ;
  opll->CAR(i)->update |= UPDATE_ALL ;

}

/* Change a rythm voice */
INLINE void setSlotPatch(OPLL_SLOT *slot, OPLL_PATCH *patch){

  slot->patch = patch ;
  slot->update |= UPDATE_ALL ;

}

/* Set sustine parameter */
INLINE void setSustine(OPLL *opll, int c, int sustine){

  opll->CAR(c)->sustine = sustine ;
  opll->MOD(c)->sustine = sustine ;
  opll->CAR(c)->update |= UPDATE_EG ;
  opll->MOD(c)->update |= UPDATE_EG ;

}

/*
  Volume setting
  volume : 6bit ( Volume register << 2 )
*/
INLINE void setVolume(OPLL *opll, int c, int volume){

  opll->CAR(c)->volume = volume ;
  opll->CAR(c)->update |= UPDATE_TLL ;

}

INLINE void setSlotVolume(OPLL_SLOT *slot, int volume){

  slot->volume = volume ;
  slot->update |= UPDATE_TLL ;

}

/* Volume setting for Drum */
INLINE void setVolume_BD(OPLL *opll, int volume){ setVolume(opll,6,volume) ; }
INLINE void setVolume_HH(OPLL *opll, int volume){ setSlotVolume(opll->MOD(7),volume) ; }
INLINE void setVolume_SD(OPLL *opll, int volume){ setSlotVolume(opll->CAR(7),volume) ; }
INLINE void setVolume_TOM(OPLL *opll, int volume){ setSlotVolume(opll->MOD(8),volume) ; }
INLINE void setVolume_CYM(OPLL *opll, int volume){ setSlotVolume(opll->CAR(8),volume) ; }

/* Set F-Number ( fnum : 9bit ) */
INLINE void setFnumber(OPLL *opll, int c, int fnum){

  opll->CAR(c)->fnum = fnum ;
  opll->MOD(c)->fnum = fnum ;
  opll->CAR(c)->update |= UPDATE_PG | UPDATE_TLL | UPDATE_RKS ;
  opll->MOD(c)->update |= UPDATE_PG | UPDATE_TLL | UPDATE_RKS ;

}

/* Set Block data (block : 3bit ) */
INLINE void setBlock(OPLL *opll, int c, int block){

  opll->CAR(c)->block = block ;
  opll->MOD(c)->block = block ;
  opll->CAR(c)->update |= UPDATE_PG | UPDATE_TLL | UPDATE_RKS ;
  opll->MOD(c)->update |= UPDATE_PG | UPDATE_TLL | UPDATE_RKS ;

}

/* Change Rythm Mode */
INLINE void setRythmMode(OPLL *opll, int mode){

  opll->rythm_mode = mode ;

  if(mode){

    opll->ch[6]->patch_number = 16 ;
    opll->ch[7]->patch_number = 17 ;
    opll->ch[8]->patch_number = 18 ;
    setSlotPatch(opll->MOD(6), &(opll->patch[16][0])) ;
    setSlotPatch(opll->CAR(6), &(opll->patch[16][1])) ;
    setSlotPatch(opll->MOD(7), &(opll->patch[17][0])) ;
    setSlotPatch(opll->CAR(7), &(opll->patch[17][1])) ;
    opll->MOD(7)->type = 1 ;
    setSlotPatch(opll->MOD(8), &(opll->patch[18][0])) ;
    setSlotPatch(opll->CAR(8), &(opll->patch[18][1])) ;
    opll->MOD(8)->type = 1 ;

  }else{

    setPatch(opll, 6, opll->reg[0x36]>>4) ;
    setPatch(opll, 7, opll->reg[0x37]>>4) ;
    opll->MOD(7)->type = 0 ;
    setPatch(opll, 8, opll->reg[0x38]>>4) ;
    opll->MOD(8)->type = 0 ;

  }

  slotOff(opll->MOD(6));
  slotOff(opll->MOD(7));
  slotOff(opll->MOD(8));
  slotOff(opll->CAR(6));
  slotOff(opll->CAR(7));
  slotOff(opll->CAR(8));
  opll->MOD(6)->update |= UPDATE_ALL ;
  opll->MOD(7)->update |= UPDATE_ALL ;
  opll->MOD(8)->update |= UPDATE_ALL ;
  opll->CAR(6)->update |= UPDATE_ALL ;
  opll->CAR(7)->update |= UPDATE_ALL ;
  opll->CAR(8)->update |= UPDATE_ALL ;

}

void OPLL_copyPatch(OPLL *opll, int num, OPLL_PATCH *patch){

  memcpy(opll->patch[num],patch,sizeof(OPLL_PATCH)*2) ;

}

/***********************************************************

                      Initializing

***********************************************************/

static void OPLL_SLOT_reset(OPLL_SLOT *slot){

  slot->sintbl = waveform[0] ;
  slot->phase = 0 ;
  slot->dphase = 0 ;
  slot->output[0] = 0 ;
  slot->output[1] = 0 ;
  slot->eg_mode = SETTLE ;
  slot->eg_phase = EG_DP_WIDTH ;
  slot->eg_dphase = 0 ;
  slot->rks = 0 ;
  slot->tll = 0 ;
  slot->sustine = 0 ;
  slot->patch = &null_patch ;
  slot->fnum = 0 ;
  slot->block = 0 ;
  slot->volume = 0 ;
  slot->pgout = 0 ;
  slot->egout = 0 ;

}

static OPLL_SLOT *OPLL_SLOT_new(void){

  OPLL_SLOT *slot ;

  slot = malloc(sizeof(OPLL_SLOT)) ;
  if(slot == NULL) return NULL ;

  return slot ;

}

static void OPLL_SLOT_delete(OPLL_SLOT *slot){

  free(slot) ;

}

static void OPLL_CH_reset(OPLL_CH *ch){

  if(ch->mod!=NULL) OPLL_SLOT_reset(ch->mod) ;
  if(ch->car!=NULL) OPLL_SLOT_reset(ch->car) ;
  ch->key_status = 0 ;

}

static OPLL_CH *OPLL_CH_new(void){

  OPLL_CH *ch ;
  OPLL_SLOT *mod, *car ;

  mod = OPLL_SLOT_new() ;
  if(mod == NULL) return NULL ;

  car = OPLL_SLOT_new() ;
  if(car == NULL){
    OPLL_SLOT_delete(mod) ;
    return NULL ;
  }

  ch = malloc(sizeof(OPLL_CH)) ;
  if(ch == NULL){
    OPLL_SLOT_delete(mod) ;
    OPLL_SLOT_delete(car) ;
    return NULL ;
  }

  mod->type = 0 ;
  car->type = 1 ;
  ch->mod = mod ;
  ch->car = car ;

  return ch ;

}


static void OPLL_CH_delete(OPLL_CH *ch){

  OPLL_SLOT_delete(ch->mod) ;
  OPLL_SLOT_delete(ch->car) ;
  free(ch) ;

}

OPLL *OPLL_new(void){

  OPLL *opll ;
  OPLL_CH *ch[9] ;
  OPLL_PATCH *patch[19] ;
  int i, j ;

  for( i = 0 ; i < 19 ; i++ ){
    patch[i] = calloc(sizeof(OPLL_PATCH),2) ;
    if(patch[i] == NULL){
      for ( j = i ; i > 0 ; i++ ) free(patch[j-1]) ;
      return NULL ;
    }
  }

  for( i = 0 ; i < 9 ; i++ ){
    ch[i] = OPLL_CH_new() ;
    if(ch[i]==NULL){
      for ( j = i ; i > 0 ; i++ ) OPLL_CH_delete(ch[j-1]) ;
      for ( j = 0 ; j < 19 ; j++ ) free(patch[j]) ;
      return NULL ;
    }
  }


  opll = malloc(sizeof(OPLL)) ;
  if(opll == NULL) return NULL ;

  for ( i = 0 ; i < 19 ; i++ ) opll->patch[i] = patch[i] ;
  for ( i = 0 ; i < 9 ; i++ ) opll->ch[i] = ch[i] ;


  /* Slot aliases for sequential access. */
  for ( i = 0 ; i <9 ; i++){
      opll->ch[i]->mod->plfo_am = &(opll->lfo_am) ;
      opll->ch[i]->mod->plfo_pm = &(opll->lfo_pm) ;
      opll->slot[i*2+0] = opll->ch[i]->mod ;
      opll->ch[i]->car->plfo_am = &(opll->lfo_am) ;
      opll->ch[i]->car->plfo_pm = &(opll->lfo_pm) ;
      opll->slot[i*2+1] = opll->ch[i]->car ;
  }

  for( i = 0 ; i < 10 ; i++ ) opll->mask[i] = 0 ;

  OPLL_reset(opll) ;

  opll->masterVolume = 64 ;
  opll->rythmVolume = 64 ;

  return opll ;


}

void OPLL_delete(OPLL *opll){

  int i ;

  for ( i = 0 ; i < 9 ; i++ )
    OPLL_CH_delete(opll->ch[i]) ;

  for ( i = 0 ; i < 19 ; i++ )
    free(opll->patch[i]) ;

#ifdef OPLL_LOGFILE

  fclose(opll->logfile) ;

#endif

  free(opll) ;

}

/* Reset patch datas by system default. */
void OPLL_reset_patch(OPLL *opll){

  int i ;

  for ( i = 0 ; i < 19 ; i++ )
    OPLL_copyPatch(opll, i, &default_patch[i*2]) ;

}

/* Reset whole of OPLL except patch datas. */
void OPLL_reset(OPLL *opll){

  int i ;

  if(!opll) return ;

  opll->output[0] = 0 ;
  opll->output[1] = 0 ;

  opll->rythm_mode = 0 ;
  opll->pm_phase = 0 ;
  opll->pm_dphase = (UINT32)rate_adjust(PM_SPEED * PM_DP_WIDTH / (clk/72) ) ;
  opll->am_phase = 0 ;
  opll->am_dphase = (UINT32)rate_adjust(AM_SPEED * AM_DP_WIDTH / (clk/72) ) ;

  for ( i = 0 ; i < 0x38 ; i++ ) opll->reg[i] = 0 ;

  for ( i = 0 ; i < 9 ; i++ ){
      OPLL_CH_reset(opll->ch[i]) ;
      setPatch(opll,i,0) ;
  }

#ifdef OPLL_LOGFILE

  opll->logfile = fopen("opll.log","w") ;

#endif


}

void OPLL_init(UINT32 c, UINT32 r){


  clk = c ;
  rate = r ;
  makePmTable() ;
  makeAmTable() ;
  makeDB2LinTable() ;
  makeAdjustTable() ;
  makeDphaseTable() ;
  makeTllTalbe() ;
  makeSinTable() ;
  makeDphaseARTable() ;
  makeDphaseDRTable() ;
  makeRksTable() ;
  makeDefaultPatch() ;

}

void OPLL_close(void){

}

/*********************************************************

                 Generate wave data

*********************************************************/
/* Convert Amp(0 to EG_HEIGHT) to Phase(0 to 2PI). */
#if ( SLOT_AMP_BITS - PG_BITS ) > 0
#define wave2_2pi(e)  ( (e) >> ( SLOT_AMP_BITS - PG_BITS ))
#else
#define wave2_2pi(e)  ( (e) << ( PG_BITS - SLOT_AMP_BITS ))
#endif

/* Convert Amp(0 to EG_HEIGHT) to Phase(0 to 4PI). */
#if ( SLOT_AMP_BITS - PG_BITS - 1 ) > 0
#define wave2_4pi(e)  ( (e) >> ( SLOT_AMP_BITS - PG_BITS - 1 ))
#else
#define wave2_4pi(e)  ( (e) << ( 1 + PG_BITS - SLOT_AMP_BITS ))
#endif

/* Convert Amp(0 to EG_HEIGHT) to Phase(0 to 8PI). */
#if ( SLOT_AMP_BITS - PG_BITS - 2 ) > 0
#define wave2_8pi(e)  ( (e) >> ( SLOT_AMP_BITS - PG_BITS - 2 ))
#else
#define wave2_8pi(e)  ( (e) << ( 2 + PG_BITS - SLOT_AMP_BITS ))
#endif

INLINE UINT32 mrand(void){

  static unsigned int seed = 0xffff ;

  seed = ((seed>>15)^((seed>>12)&1)) | (( seed << 1 ) & 0xffff) ;

  return seed ;

}

/* Update Noise unit */
INLINE void update_noise(OPLL *opll){

  opll->whitenoise = (((UINT32)(DB_NOISE/DB_STEP)) * mrand()) >> 16 ;

}

/* Update AM, PM unit */
INLINE void update_ampm(OPLL *opll){

  opll->pm_phase = (opll->pm_phase + opll->pm_dphase)&(PM_DP_WIDTH - 1) ;
  opll->am_phase = (opll->am_phase + opll->am_dphase)&(AM_DP_WIDTH - 1) ;
  opll->lfo_am = amtable[HIGHBITS(opll->am_phase, AM_DP_BITS - AM_PG_BITS)] ;
  opll->lfo_pm = pmtable[HIGHBITS(opll->pm_phase, PM_DP_BITS - PM_PG_BITS)] ;

}

/* PG */
INLINE UINT32 calc_phase(OPLL_SLOT *slot, INT32 lfo_pm){

  if(slot->patch->PM)
    slot->phase += (slot->dphase * lfo_pm) >> PM_AMP_BITS ;
  else
    slot->phase += slot->dphase ;

  slot->phase &= (DP_WIDTH - 1) ;

  return HIGHBITS(slot->phase, DP_BASE_BITS) ;

}

/* EG */
INLINE UINT32 calc_envelope(OPLL_SLOT *slot, UINT32 lfo_am){

  #define SL(x) EXPAND_BITS((UINT32)(x/DB_STEP),DB_BITS,EG_DP_BITS)
  static UINT32 SLtable[16] = {
    SL( 0), SL( 3), SL( 6), SL( 9), SL(12), SL(15), SL(18), SL(21),
    SL(24), SL(27), SL(30), SL(33), SL(36), SL(39), SL(42), SL(48)
  } ;

  UINT32 egout ;

  switch(slot->eg_mode){

  case ATTACK:
    slot->eg_phase += slot->eg_dphase ;
    if(EG_DP_WIDTH & slot->eg_phase){ /* (it is as same meaning as EG_DP_WIDTH <= slot->eg_phase) */
      egout = 0 ;
      slot->eg_phase= 0 ;
      slot->eg_mode = DECAY ;
      slot->update |= UPDATE_EG ;
    }else
      egout = AR_ADJUST_TABLE[HIGHBITS(slot->eg_phase, EG_DP_BITS - EG_BITS)] ;
    break;

  case DECAY:
    slot->eg_phase += slot->eg_dphase ;
    egout = HIGHBITS(slot->eg_phase, EG_DP_BITS - EG_BITS) ;
    if(slot->eg_phase >= SLtable[slot->patch->SL]){
      if(slot->patch->EG){
        slot->eg_phase = SLtable[slot->patch->SL] ;
	      slot->eg_mode = SUSHOLD ;
        slot->update |= UPDATE_EG ;
      }else{
        slot->eg_phase = SLtable[slot->patch->SL] ;
        slot->eg_mode = SUSTINE ;
        slot->update |= UPDATE_EG ;
      }
      egout = HIGHBITS(slot->eg_phase, EG_DP_BITS - EG_BITS) ;
    }

    break;

  case SUSHOLD:
    egout = HIGHBITS(slot->eg_phase, EG_DP_BITS - EG_BITS) ;
    if(slot->patch->EG == 0){
      slot->eg_mode = SUSTINE ;
      slot->update |= UPDATE_EG ;
    }
    break;

  case SUSTINE:
  case RELEASE:
    slot->eg_phase += slot->eg_dphase ;
    egout = HIGHBITS(slot->eg_phase, EG_DP_BITS - EG_BITS) ;
    if(egout >= (1<<EG_BITS)){
      slot->eg_mode = FINISH ;
      egout = (1<<EG_BITS) - 1 ;
    }
    break;

  case FINISH:
    egout = (1<<EG_BITS) - 1 ;
    break ;

  default:
    egout = (1<<EG_BITS) - 1 ;
    break;
  }

  if(slot->patch->AM)
    egout = EXPAND_BITS_X(egout,EG_BITS,DB_BITS) + slot->tll + lfo_am ;
  else
    egout = EXPAND_BITS_X(egout,EG_BITS,DB_BITS) + slot->tll ;

  if(egout >= (int)(48/DB_STEP)) egout = DB_MUTE ;

  return egout ;

}

INLINE INT32 calc_slot_car(OPLL_SLOT *slot, INT32 fm){

  INT32 tmp ;

  slot->update |= slot->patch->update ;
  if(slot->update) refresh(slot) ;
  slot->egout = calc_envelope(slot,*(slot->plfo_am)) ;
  slot->pgout = calc_phase(slot,*(slot->plfo_pm)) ;
  if(slot->egout>=DB_MUTE) return 0 ;

  tmp = slot->sintbl[(slot->pgout+wave2_8pi(fm))&(PG_WIDTH-1)] ;

  if(tmp>0)
    return DB2LIN_TABLE[Min(DB_MUTE, tmp-1+slot->egout)] ;
  else
    return -DB2LIN_TABLE[Min(DB_MUTE, 1-tmp+slot->egout)] ;

}


INLINE INT32 calc_slot_mod(OPLL_SLOT *slot){

  INT32 tmp, fm ;

  slot->update |= slot->patch->update ;
  if(slot->update) refresh(slot) ;
  slot->egout = calc_envelope(slot,*(slot->plfo_am)) ;
  slot->pgout = calc_phase(slot,*(slot->plfo_pm)) ;
  if(slot->egout>=DB_MUTE){
    slot->output[1] = slot->output[0] ;
    slot->output[0] = 0 ;
    return 0 ;
  }

  if(slot->patch->FB!=0){
    fm = wave2_4pi((slot->output[0]+slot->output[1])>>1) >> (7 - slot->patch->FB) ;
    tmp = slot->sintbl[(slot->pgout+fm)&(PG_WIDTH-1)] ;
  }else
    tmp = slot->sintbl[slot->pgout] ;

  slot->output[1] = slot->output[0] ;
  if(tmp>0)
    slot->output[0] =  DB2LIN_TABLE[Min(DB_MUTE, tmp-1+slot->egout)] ;
  else
    slot->output[0] = -DB2LIN_TABLE[Min(DB_MUTE, 1-tmp+slot->egout)] ;

  return slot->output[0] ;

}

INLINE INT32 calc_slot_tom(OPLL_SLOT *slot){

  INT32 tmp ;

  if(slot->update) refresh(slot) ;
  slot->egout = calc_envelope(slot,0) ;
  slot->pgout = calc_phase(slot,0) ;
  if(slot->egout>=DB_MUTE) return 0 ;

  tmp = slot->sintbl[slot->pgout] ;
  if(tmp>0) return DB2LIN_TABLE[Min(DB_MUTE, tmp-1+slot->egout+4)] ;
  else return -DB2LIN_TABLE[Min(DB_MUTE, 1-tmp+slot->egout+4)] ;

}

/* calc SNARE slot */
INLINE INT32 calc_slot_snare(OPLL_SLOT *slot, UINT32 whitenoise){

  INT32 tmp ;

  if(slot->update) refresh(slot) ;
  slot->egout = calc_envelope(slot,0) ;
  slot->pgout = calc_phase(slot,0) ;
  if(slot->egout>=DB_MUTE) return 0 ;

  tmp = fullsintable[slot->pgout] ;

  if(tmp > 0){
      tmp = ((tmp - 1)>>2) + slot->egout + whitenoise ;
    return DB2LIN_TABLE[Min(DB_MUTE, tmp)] ;
  }else{
      tmp = (((1 - tmp)>>2) + slot->egout + whitenoise) ;
    return -DB2LIN_TABLE[Min(DB_MUTE,tmp)] ;
  }

}

INLINE INT32 calc_slot_hat(OPLL_SLOT *slot, INT32 fm, INT32 whitenoise){

  INT32 tmp ;

  if(slot->update) refresh(slot) ;
  slot->egout = calc_envelope(slot,0) ;
  slot->pgout = calc_phase(slot,0) ;
  if(slot->egout>=DB_MUTE) return 0 ;

  tmp = fullsintable[(slot->pgout + wave2_8pi(fm))&(PG_WIDTH - 1)] ;

  if(tmp > 0){
    tmp = tmp + ((int)(6/DB_STEP)) + slot->egout + whitenoise ;
    return DB2LIN_TABLE[Min(DB_MUTE, slot->egout<<2)] + DB2LIN_TABLE[Min(DB_MUTE,tmp-1)] ;
  }else{
    tmp = tmp - ((int)(6/DB_STEP)) - slot->egout - whitenoise ;
    return DB2LIN_TABLE[Min(DB_MUTE, slot->egout<<2)] - DB2LIN_TABLE[Min(DB_MUTE,1-tmp)] ;
  }

}

#ifdef OPLL_ENABLE_DEBUG
  static UINT32 tick = 0 ;
#endif

INT16 OPLL_calc(OPLL *opll){

  INT32 inst = 0 , perc = 0 , out = 0 ;
  INT32 basesin ;
  int i ;

#ifdef OPLL_ENABLE_DEBUG
  tick++ ;
#endif

  update_ampm(opll) ;
  update_noise(opll) ;

  for(i=0 ; i < 6 ; i++)
    if((!opll->mask[i])&&(opll->CAR(i)->eg_mode!=FINISH))
      inst += calc_slot_car(opll->CAR(i),calc_slot_mod(opll->MOD(i))) ;

  if(!opll->rythm_mode){

    for(i=6 ; i < 9 ; i++)
      if((!opll->mask[i])&&(opll->CAR(i)->eg_mode!=FINISH))
        inst += calc_slot_car(opll->CAR(i),calc_slot_mod(opll->MOD(i))) ;

  }else if(!opll->mask[9]){

#ifdef OPLL_ENABLE_DEBUG
    basesin = fullsintable[(opll->MOD(8)->pgout*opll->debug_base_ml)&(PG_WIDTH-1)] ;
#else
    basesin = fullsintable[(opll->MOD(8)->pgout<<3)&(PG_WIDTH-1)] ;
#endif

    if((!opll->mask[6])&&(opll->CAR(6)->eg_mode!=FINISH)){
      perc += calc_slot_car(opll->CAR(6),calc_slot_mod(opll->MOD(6))) ;
    }

    if(!opll->mask[7]){
      if(opll->MOD(7)->eg_mode!=FINISH)
        perc += calc_slot_hat(opll->MOD(7),basesin,opll->whitenoise<<2) ;
      if(opll->CAR(7)->eg_mode!=FINISH)
        perc += calc_slot_snare(opll->CAR(7),opll->whitenoise<<1) ;
    }

    if(!opll->mask[8]){
      if(opll->MOD(8)->eg_mode!=FINISH)
        perc += calc_slot_tom(opll->MOD(8)) ;
      else{
        if(opll->MOD(8)->update) refresh(opll->MOD(8)) ;
        opll->MOD(8)->pgout = calc_phase(opll->MOD(8),0) ;
      }
      if(opll->CAR(8)->eg_mode!=FINISH)
        perc += calc_slot_hat(opll->CAR(8),basesin,opll->whitenoise<<1) ;
    }else{
      if(opll->MOD(8)->update) refresh(opll->MOD(8)) ;
      opll->MOD(8)->pgout = calc_phase(opll->MOD(8),0) ;
    }
  }

  perc = perc << 1 ;

#if SLOT_AMP_BITS > 8
  inst = (inst >> (SLOT_AMP_BITS - 8)) ;
  perc = (perc >> (SLOT_AMP_BITS - 9)) ;
#else
  inst = (inst << (8 - SLOT_AMP_BITS)) ;
  perc = (perc << (9 - SLOT_AMP_BITS)) ;
#endif

  out = inst + ((perc * opll->rythmVolume) >> 6) ;
  out = ( out  * opll->masterVolume ) >> 2 ;

  if(out>32767) out = 32767 ;
  else if(out<-32768) out = -32768 ;

  opll->patch[0][0].update = 0 ;
  opll->patch[0][1].update = 0 ;

  return (INT16)out ;

}

/****************************************************

          Interfaces for through I/O port.

*****************************************************/
void OPLL_writeReg(OPLL *opll, UINT32 reg, UINT32 data){

  OPLL_PATCH *p ;
  int i,v ;

  p = opll->patch[0] ;
  data = data&0xff ;

  switch(reg){
  case 0x00:
  case 0x01:
    p[reg%2].AM = (data>>7)&1 ;
    p[reg%2].PM = (data>>6)&1 ;
    p[reg%2].EG = (data>>5)&1 ;
    p[reg%2].KR = (data>>4)&1 ;
    p[reg%2].ML = (data)&15 ;
    p[reg%2].update |= UPDATE_ALL ;
    break;

  case 0x02:
    p[0].KL = (data>>6)&3 ;
    p[0].TL = (data)&63 ;
    p[reg%2].update |= UPDATE_TLL ;
    break ;

  case 0x03:
    p[1].KL = (data>>6)&3 ;
    p[1].WF = (data>>4)&1 ;
    p[0].WF = (data>>3)&1 ;
    p[0].FB = (data)&7 ;
    p[reg%2].update |= UPDATE_TLL | UPDATE_WF ;
    break ;

  case 0x04:
  case 0x05:
    p[reg%2].AR = (data>>4)&15 ;
    p[reg%2].DR = (data)&15 ;
    p[reg%2].update |= UPDATE_EG ;
    break ;

  case 0x06:
  case 0x07:
    p[reg%2].SL = (data>>4)&15 ;
    p[reg%2].RR = (data)&15 ;
    p[reg%2].update |= UPDATE_EG ;
    break ;

  case 0x0e:
    if(((data>>5)&1)^(opll->rythm_mode)) setRythmMode(opll,(data&32)>>5) ;

    if(opll->rythm_mode){
      opll->slot_on_flag[SLOT_BD1] = (opll->reg[0x0e]&0x10) | (opll->reg[0x26]&0x10) ;
      opll->slot_on_flag[SLOT_BD2] = (opll->reg[0x0e]&0x10) | (opll->reg[0x26]&0x10) ;
      opll->slot_on_flag[SLOT_SD]  = (opll->reg[0x0e]&0x08) | (opll->reg[0x27]&0x10) ;
      opll->slot_on_flag[SLOT_HH]  = (opll->reg[0x0e]&0x01) | (opll->reg[0x27]&0x10) ;
      opll->slot_on_flag[SLOT_TOM] = (opll->reg[0x0e]&0x04) | (opll->reg[0x28]&0x10) ;
      opll->slot_on_flag[SLOT_CYM] = (opll->reg[0x0e]&0x02) | (opll->reg[0x28]&0x10) ;
      if(data&0x10) keyOn_BD(opll) ; else keyOff_BD(opll) ;
      if(data&0x8) keyOn_SD(opll) ; else keyOff_SD(opll) ;
      if(data&0x4) keyOn_TOM(opll) ; else keyOff_TOM(opll) ;
      if(data&0x2) keyOn_CYM(opll) ; else keyOff_CYM(opll) ;
      if(data&0x1) keyOn_HH(opll) ; else keyOff_HH(opll) ;
    }
    break ;

  case 0x0f:
    break ;

  case 0x10:  case 0x11:  case 0x12:  case 0x13:
  case 0x14:  case 0x15:  case 0x16:  case 0x17:
  case 0x18:
    setFnumber(opll, reg-0x10, data + ((opll->reg[reg+0x10]&1)<<8)) ;
    break ;

  case 0x20:  case 0x21:  case 0x22:  case 0x23:
  case 0x24:  case 0x25:
    setFnumber(opll, reg-0x20, ((data&1)<<8) + opll->reg[reg-0x10]) ;
    setBlock(opll, reg-0x20, (data>>1)&7 ) ;
    opll->slot_on_flag[(reg-0x20)*2] = opll->slot_on_flag[(reg-0x20)*2+1] = (opll->reg[reg])&0x10 ;
    if(data&0x10) keyOn(opll, reg-0x20) ;
    else keyOff(opll, reg-0x20) ;
    if((opll->reg[reg]^data)&0x20)
      setSustine(opll, reg-0x20, (data>>5)&1) ;
    break ;

  case 0x26:
    setFnumber(opll, 6, ((data&1)<<8) + opll->reg[0x16]) ;
    setBlock(opll, 6, (data>>1)&7 ) ;
    opll->slot_on_flag[SLOT_BD1] = opll->slot_on_flag[SLOT_BD2] = (opll->reg[0x26])&0x10 ;
    if(opll->reg[0x0e]&32){
      opll->slot_on_flag[SLOT_BD1] |= (opll->reg[0x0e])&0x10 ;
      opll->slot_on_flag[SLOT_BD2] |= (opll->reg[0x0e])&0x10 ;
    }
    if(data&0x10) keyOn(opll, 6) ; else keyOff(opll, 6) ;
    if((opll->reg[0x26]^data)&0x20) setSustine(opll, 6, (data>>5)&1) ;
    break ;

  case 0x27:
    setFnumber(opll, 7, ((data&1)<<8) + opll->reg[0x17]) ;
    setBlock(opll, 7, (data>>1)&7 ) ;
    opll->slot_on_flag[SLOT_SD] = opll->slot_on_flag[SLOT_HH] = (opll->reg[0x27])&0x10 ;
    if(opll->reg[0x0e]&32){
      opll->slot_on_flag[SLOT_SD]  |= (opll->reg[0x0e])&0x08 ;
      opll->slot_on_flag[SLOT_HH]  |= (opll->reg[0x0e])&0x01 ;
    }
    if(data&0x10) keyOn(opll, 7) ; else keyOff(opll, 7) ;
    if((opll->reg[0x27]^data)&0x20) setSustine(opll, 7, (data>>5)&1) ;
    break ;

  case 0x28:
    setFnumber(opll, 8, ((data&1)<<8) + opll->reg[0x18]) ;
    setBlock(opll, 8, (data>>1)&7 ) ;
    opll->slot_on_flag[SLOT_TOM] = opll->slot_on_flag[SLOT_CYM] = (opll->reg[0x28])&0x10 ;
    if(opll->reg[0x0e]&32){
      opll->slot_on_flag[SLOT_TOM] |= (opll->reg[0x0e])&0x04 ;
      opll->slot_on_flag[SLOT_CYM] |= (opll->reg[0x0e])&0x02 ;
    }
    if(data&0x10) keyOn(opll, 8) ; else keyOff(opll, 8) ;
    if((opll->reg[reg]^data)&0x20) setSustine(opll, 8, (data>>5)&1) ;
    break ;

  case 0x30: case 0x31: case 0x32: case 0x33: case 0x34:
  case 0x35: case 0x36: case 0x37: case 0x38:
    i = (data>>4)&15 ;
    v = data&15 ;
    if((opll->rythm_mode)&&(reg>=0x36)) {
      switch(reg){
      case 0x37 :
        setSlotVolume(opll->MOD(7), i<<2) ;
        break ;
      case 0x38 :
        setSlotVolume(opll->MOD(8), i<<2) ;
        break ;
      }
    }else{
      setPatch(opll, reg-0x30, i) ;
    }

    setVolume(opll, reg-0x30, v<<2) ;
    break ;

  default:
    break ;

  }

  opll->reg[reg] = (unsigned char)data ;

}

/****************************************************************

                     debug controller

****************************************************************/
#ifdef OPLL_ENABLE_DEBUG
void debug_base_ml_ctrl(OPLL *opll,int i){

  opll->debug_base_ml += i ;
  if(opll->debug_base_ml>31) opll->debug_base_ml = 31 ;
  if(opll->debug_base_ml<1) opll->debug_base_ml = 1 ;

}
#endif
