/*

  File: fm.h -- header file for software emuration for FM sound generator

   Copyright(c) 1997 Tatsuyuki Satoh

*/
#ifndef _H_FM_FM_
#define _H_FM_FM_

/* OPN buffering mode SEPARATE or MIXING */
//#define OPN_MIX			/* OPN MIXING */
//#define FM_16BITS			/* output data size 16 bit / 8 bit */
//#define OPM_STEREO		/* stereo / mono */

//#define AUDIO_SINED		/* AUDIO OUTPUT TYPE */

#ifdef FM_16BITS
 typedef unsigned short FMSAMPLE;
 #define FM_BITS 16
 #define YM2151_NUMBUF 2    /* stereo L and R ch */
#else
 typedef unsigned char FMSAMPLE;
 #define FM_BITS 8
 #define YM2151_NUMBUF 8    /* channels */
#endif

#ifdef AUDIO_UNSIGNED
 #define FMOUT_0 (1<<(FMBITS-1))
#else
 #define FMOUT_0 0
#endif

/* ---------- OPN / OPM one channel  ---------- */
typedef struct fm_chan {
	/* registers , sound type */
	int *DT[4];					/* detune          :DT_TABLE[DT]       */
	unsigned char ML[4];		/* multiple,Detune2:(DT2<<4)|ML for OPM*/
	int TL[4];					/* total level     :TL << 8            */
	signed int TLL[4];			/* adjusted now TL                     */
	unsigned char KSR[4];		/* key scale rate  :3-KSR              */
	int *AR[4];					/* attack rate     :&AR_TABLE[AR<<1]   */
	int *DR[4];					/* decay rate      :&DR_TALBE[DR<<1]   */
	int *SR[4];					/* sustin rate     :&DR_TABLE[SR<<1]   */
	int  SL[4];					/* sustin level    :SL_TALBE[SL]       */
	int *RR[4];					/* release rate    :&DR_TABLE[RR<<2+2] */
	unsigned char SEG[4];		/* SSG EG type     :SSGEG              */
	unsigned int *Rdatap;		/* R channel switch (pickup point)     */
	unsigned int *Ldatap;		/* L channel mask   (pickup point)     */
	unsigned char FB;			/* feed back       :&FB_TABLE[FB<<8]   */
	unsigned char AL;			/* algorythm       :AR                 */
	/* phase generator state */
	unsigned int  fc;			/* fnum,blk        :calcrated          */
	unsigned char fn_h;			/* freq latch      :                   */
	unsigned char kcode;		/* key code        :                   */
	unsigned char ksr[4];		/* key scale rate  :kcode>>(3-KSR)     */
	unsigned int mul[4];		/* multiple        :ML_TABLE[ML]       */
	unsigned int Cnt[4];		/* frequency count :                   */
	unsigned int Incr[4];		/* frequency step  :                   */
	unsigned int fb_latch;		/* beedback latch                      */
	/* envelope generator state */
	unsigned char evm[4];		/* envelope phase                      */
	signed int arc[4];			/* envelope attack counter             */
	signed int evc[4];			/* envelope dicay counter              */
	signed int evs[4];			/* envelope step                       */
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
	void (*handler)();			/* interrupt handler   */
	/* speedup customize */
	/* time tables */
	unsigned int DT_TABLE[8][32];/* detune tables       */
	signed int AR_TABLE[94];	/* atttack rate tables */
	signed int DR_TABLE[94];	/* decay rate tables   */
}FM_ST;

/* here's the virtual YM2203(OPN)  */
typedef struct ym2203_f {
#ifdef OPN_MIX
	FMSAMPLE *Buf;				/* sound buffer */
#else
	FMSAMPLE *Buf[3];			/* sound buffer */
#endif
	FM_ST ST;					/* general state     */
	FM_CH CH[3];				/* channel state     */
//	unsigned char PR;			/* freqency priscaler*/
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
	unsigned int KC_TABLE[1024];/* keycode,keyfunction -> count */
} YM2151;

/*
** Initialize YM2203 emulator(s).
**
** 'num' is the number of virtual YM2203's to allocate
** 'rate' is sampling rate
** 'bufsiz' is the size of the buffer
*/
int OPNInit(int num, int clock, int rate, int bufsiz , FMSAMPLE **buffer );

/*
** shutdown the YM2203 emulators .. make sure that no sound system stuff
** is touching our audio buffers ...
*/
void OPNShutdown(void);

/*
** reset all chip registers for YM2203 number 'num'
*/
void OPNResetChip(int num);

/*
*/
void OPNUpdate(void);

void OPNUpdateOne(int chip, int endp);

/*
** write 'v' to register 'r' on YM2203 chip number 'n'
*/
void OPNWriteReg(int n, int r, int v);

/*
** read status  YM2203 chip number 'n'
*/
unsigned char OPNReadStatus(int n);

#ifdef OPN_MIX
/* ----- get pointer sound buffer ----- */
FMSAMPLE *OPNBuffer(int n);
/* ----- set sound buffer ----- */
int OPNSetBuffer(int n, FMSAMPLE *buf );
#else
/* ----- get pointer sound buffer ----- */
FMSAMPLE *OPNBuffer(int n,int c );
/* ----- set sound buffer ----- */
int OPNSetBuffer(int n, FMSAMPLE **buf );
#endif
/* ---- set user interrupt handler ----- */
void OPNSetIrqHandler(int n, void (*handler)() );



int OPMInit(int num, int clock, int rate, int bufsiz , FMSAMPLE **buffer );

void OPMShutdown(void);

void OPMResetChip(int num);

void OPMUpdate(void);

void OPMUpdateOne(int num , int endp );

void OPMWriteReg(int n, int r, int v);

unsigned char OPMReadStatus(int n );

/* ----- get pointer sound buffer ----- */
FMSAMPLE *OPMBuffer(int n,int c );
/* ----- set sound buffer ----- */
int OPMSetBuffer(int n, FMSAMPLE **buf );

/* ---- set user interrupt handler ----- */
void OPMSetIrqHandler(int n, void (*handler)() );

#endif /* _H_FM_FM_ */
