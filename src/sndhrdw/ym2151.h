/*
**
** File: ym2151.h -- header file for software implementation of YM-2151
**						FM Operator Type-M(OPM).
**
** (C) Jarek Burczynski (s0246@goblin.pjwstk.waw.pl) September, 1st 1997
**
** First beta release: December, 16th 1997
** Version 1.0 beta release: March, 16th 1998
**
**
** CAUTION !!! CAUTION !!!           H E L P   W A N T E D !!!
**
** If you have some very technical information about YM-2151, or possibly you
** own a computer (like X68000) which has YM-2151 built-in (so you could make
** a few samples of real chip), and you would like to help:
**                  - PLEASE CONTACT ME ASAP!!! -
**
**
**
** I would like to thank to Shigeharu Isoda without whom this project wouldn't
** be possible to finish (possibly even start). He has been so kind and helped
** me by scanning his YM2151 Japanese Manual first, and then by answering MANY
** of questions I've asked (I can't read Japanese still, although I study
** informatics at Polish-Japanese Institute here in Poland).
**
** I would like to thank to nao (nao@ms6.hinet.net) also - he has been the one
** who gave me some information about YM2151, and pointed me to Shigeharu.
**
** Big thanks to Aaron Giles and Chris Hardy - they've made some samples of one
** of my favourite arcade games so I could compare it to my emulator. That's
** how I know there's still something I need to know about this chip.
**
** Greetings to Ishmair (thanks for the datasheet !) who has inspired me to
** write my own 2151 emu by telling me "... maybe with some somes and a lot of
** luck ... in theory it's difficult ... " - wow that really was a challenge :)
**
*/

#ifndef _H_YM2151_
#define _H_YM2151_

/* register id's */
#define YM_KON		(0x08)
#define YM_NOISE	(0x0f)

#define YM_CLOCKA1	(0x10)
#define YM_CLOCKA2	(0x11)
#define YM_CLOCKB	(0x12)
#define YM_CLOCKSET	(0x14)

#define YM_LFRQ		(0x18)
#define YM_PMD_AMD	(0x19)
#define YM_CT1_CT2_W	(0x1b)


#define YM_CONNECT_BASE	(0x20) /*1 reg per FM channel*/
#define YM_KC_BASE	(0x28)
#define YM_KF_BASE	(0x30)
#define YM_PMS_AMS_BASE	(0x38)

#define YM_DT1_MUL_BASE	(0x40) /*1 reg per operator*/
#define YM_TL_BASE	(0x60)
#define YM_KS_AR_BASE	(0x80)
#define YM_AMS_D1R_BASE	(0xa0)
#define YM_DT2_D2R_BASE	(0xc0)
#define YM_D1L_RR_BASE	(0xe0)



typedef void SAMPLE;  /*Yep, 16 bit is supported, at last*/


typedef struct{      /*oscillator data */
	signed int freq;		/*oscillator frequency (in indexes per sample) (fixed point)*/
	signed int phase;		/*accumulated oscillator phase (in indexes)    (fixed point)*/

	unsigned int TL;		/*this is output level, in fact     */
	signed int volume;		/*oscillator volume (fixed point)   */
        signed int attack_volume;	/*used for attack phase calculations*/

	signed int OscilFB;		/*oscillator self feedback used only by operators 0*/

	int D1L;
	unsigned char AR;
	unsigned char D1R;
	unsigned char D2R;
	unsigned char RR;
	signed int delta_AR;	/*volume delta for attack  phase (fixed point)*/
	signed int delta_D1R;	/*volume delta for decay   phase (fixed point)*/
	signed int delta_D2R;	/*volume delta for sustain phase (fixed point)*/
	signed int delta_RR;	/*volume delta for release phase (fixed point)*/
	unsigned char state;	/*Envelope state: 0-attack(AR) 1-decay(D1R) 2-sustain(D2R) 3-release(RR) 4-finished*/

} OscilRec;


/* here's the virtual YM2151 */
typedef struct ym2151_f {
    SAMPLE *Buf;	/* sound buffer */
    int bufp;
    OscilRec Oscils[32];         /*there are 32 operators in YM2151*/
    unsigned char Regs[256];     /*table of internal chip registers*/
    unsigned char FeedBack[8];   /*feedback shift value for operators 0 in each channel*/
    unsigned char ConnectTab[8]; /*connection algorithm number for each channel*/
    unsigned char KC[8]; 	/*KC for each channel*/
    unsigned char KF[32];       /*KF for each operator*/
    unsigned char KS[32];	/*KS for each operator*/

    unsigned char TimA,TimB,TimIRQ;
    signed int TimAVal,TimBVal;
    void *TimATimer,*TimBTimer;  /* ASG 980324 -- added for tracking timers */
    void (*handler)(void);
    void (*portwrite)(int, int);
} YM2151;


/*
** Initialize YM2151 emulator(s).
**
** 'num' is the number of virtual YM2151's to allocate
** 'rate' is sampling rate and 'bufsiz' is the size of the
** buffer that should be updated at each interval
*/
int YMInit(int num, int clock, int rate, int sample_bits, int bufsiz, SAMPLE **buffer);

/*
** shutdown the YM2151 emulators ...
*/
void YMShutdown(void);

/*
** reset all chip registers for YM2151 number 'num'
*/
void YMResetChip(int num);

/*
** called to update all chips; should be called about 50 - 70 times per
** second ... (depends on sample rate and buffer size)
*/
void YM2151Update(void);

void YM2151UpdateOne(int num, int endp);

/*
** write 'v' to register 'r' on YM2151 chip number 'n'
*/
void YMWriteReg(int n, int r, int v);

/*
** read status register on YM2151 chip number 'n'
*/
unsigned char YMReadReg(unsigned char n);


/*
** return pointer to synthesized data
*/
SAMPLE *YMBuffer(int n);

void YMSetIrqHandler(int n, void (*handler)(void));

void YMSetPortWriteHandler(int n, void (*handler)(int,int));

#endif /* _H_YM2151_ */
