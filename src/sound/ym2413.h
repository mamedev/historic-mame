#ifndef _EMU2413_H_
#define _EMU2413_H_

#ifdef __cplusplus
extern "C" {
#endif

/*#define OPLL_ENABLE_DEBUG*/
/*#define OPLL_LOGFILE*/

/*#define PI 3.14159265358979*/

/* voice data */
typedef struct {
  unsigned int TL,FB,EG,ML,AR,DR,SL,RR,KR,KL,AM,PM,WF ;
  int update ; /* for real-time update */
} OPLL_PATCH ;

/* slot */
typedef struct {

  OPLL_PATCH *patch;

  int type ;          /* 0 : modulator 1 : carrier */
  int update ;

  /* OUTPUT */
  INT32 output[2] ;      /* Output value of slot */

  /* for Phase Generator (PG) */
  INT32 *sintbl ;     /* Wavetable */
  UINT32 phase ;      /* Phase */
  UINT32 dphase ;     /* Phase increment amount */
  UINT32 pgout ;      /* output */

  /* for Envelope Generator (EG) */
  int fnum ;          /* F-Number */
  int block ;         /* Block */
  int volume ;        /* Current volume */
  int sustine ;       /* Sustine 1 = ON, 0 = OFF */
  UINT32 tll ;	      /* Total Level + Key scale level*/
  UINT32 rks ;        /* Key scale offset (Rks) */
  int eg_mode ;       /* Current state */
  UINT32 eg_phase ;   /* Phase */
  UINT32 eg_dphase ;  /* Phase increment amount */
  UINT32 egout ;      /* output */

  /* LFO (refer to opll->*) */
  INT32 *plfo_am ;
  INT32 *plfo_pm ;

} OPLL_SLOT ;

/* Channel */
typedef struct {

  int patch_number ;
  int key_status ;
  OPLL_SLOT *mod, *car ;

#ifdef OPLL_ENABLE_DEBUG
  int debug_keyonpatch[4] ;
#endif

} OPLL_CH ;

/* opll */
typedef struct {

  INT32 output[4] ;

  /* Register */
  unsigned char reg[0x40] ;
  int slot_on_flag[18] ;

  /* Rythm Mode : 0 = OFF, 1 = ON */
  int rythm_mode ;

  /* Pitch Modulator */
  UINT32 pm_phase ;
  UINT32 pm_dphase ;
  INT32 lfo_pm ;

  /* Amp Modulator */
  UINT32 am_phase ;
  UINT32 am_dphase ;
  INT32 lfo_am ;

  /* Noise Generator */
  UINT32 whitenoise ;

  /* Channel & Slot */
  OPLL_CH *ch[9] ;
  OPLL_SLOT *slot[18] ;

  /* Voice Data */
  OPLL_PATCH *patch[19] ;
  int user_patch_update[2] ; /* flag for check patch update */

  int mask[10] ; /* mask[9] = RYTHM */

  int masterVolume ; /* 0min -- 64 -- 127 max (Liner) */
  int rythmVolume ;  /* 0min -- 64 -- 127 max (Liner) */

#ifdef OPLL_ENABLE_DEBUG
  int debug_rythm_flag ;
  int debug_base_ml ;
  int feedback_type ;    /* feedback type select */
  FILE *logfile ;
#endif

} OPLL ;

void OPLL_init(UINT32 clk, UINT32 rate) ;
void OPLL_close(void) ;
OPLL *OPLL_new(void) ;
void OPLL_reset(OPLL *) ;
void OPLL_reset_patch(OPLL *) ;
void OPLL_delete(OPLL *) ;
void OPLL_writeReg(OPLL *, UINT32, UINT32) ;
INT16 OPLL_calc(OPLL *) ;
void OPLL_setPatch(OPLL *, int, OPLL_PATCH *) ;
void OPLL_forceRefresh(OPLL *) ;
void dump2patch(unsigned char *dump, OPLL_PATCH *patch) ;

#ifdef OPLL_ENABLE_DEBUG
void debug_base_ml_ctrl(OPLL *,int) ;
void OPLL_changeFeedbackMode(OPLL *opll) ;
#endif

#ifdef __cplusplus
}
#endif

#endif

