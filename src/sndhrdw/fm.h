/*

  File: fm.h -- header file for software emuration for FM sound generator

   Copyright(c) 1998 Tatsuyuki Satoh

*/
#ifndef _H_FM_FM_
#define _H_FM_FM_

#define BUILD_OPN    1		/* build YM2203 or YM2608 or YM2612 emurator */
#define BUILD_YM2203 1		/* build YM2203(OPN) emurator */
#if 0
#define BUILD_YM2608 1		/* build YM2608(OPNA)emurator */
#define BUILD_YM2612 1		/* build YM2612 emurator */
#endif

#define BUILD_YM2151 1		/* build YM2151(OPM) emurator */

#define YM2151_NUMBUF 2    /* stereo separate L and R ch */

/* For YM2151/YM2608/YM2612 option */
//#define FM_STEREO_MIX		/* stereo mixing / separate */

typedef void FMSAMPLE;

/*
** Set Timer handler
**
** Handler : Pointer of FM Timer Set/Clear Request
**         n      : bit 0-6 = chip number
**                  bit 7   = 0 TimerA , 1=TimerB
**         timeSec: 0.0   = Reest Timer
**                  other = Start Timer
*/
void FMSetTimerHandler( void (*TimerHandler)(int n,int c,double timeSec),
                        void (*IRQHandler)(int n,int irq) );

#ifdef BUILD_OPN
/* -------------------- YM2203/YM2608 Interface -------------------- */
/*
** 'n' : YM2203 chip number 'n'
** 'r' : register
** 'v' : value
*/
//void OPNWriteReg(int n, int r, int v);

/*
** read status  YM2203 chip number 'n'
*/
unsigned char OPNReadStatus(int n);

#endif /* BUILD_OPN */

#ifdef BUILD_YM2203
/* -------------------- YM2203(OPN) Interface -------------------- */

#define YM2203_NUMBUF 1
/*
** Initialize YM2203 emulator(s).
**
** 'num'     is the number of virtual YM2203's to allocate
** 'rate'    is sampling rate
** 'bitsize' is sampling bits (8 or 16)
** 'bufsiz' is the size of the buffer
** return    0 = success
*/
int YM2203Init(int num, int clock, int rate, int bitsize , int bufsiz , FMSAMPLE **buffer );

/*
** shutdown the YM2203 emulators .. make sure that no sound system stuff
** is touching our audio buffers ...
*/
void YM2203Shutdown(void);

/*
** reset all chip registers for YM2203 number 'num'
*/
void YM2203ResetChip(int num);

/*
*/
void YM2203Update(void);
/*
** return : InterruptLevel
*/
int YM2203Write(int n,int a,int v);
unsigned char YM2203Read(int n,int a);

/*
**	Timer OverFlow
*/
int YM2203TimerOver(int n, int c);


/* ----- get pointer sound buffer ----- */
FMSAMPLE *OPNBuffer(int n);
/* ----- set sound buffer ----- */
int YM2203SetBuffer(int n, FMSAMPLE *buf );

#endif /* BUILD_YM2203 */

#ifdef BUILD_YM2608
/* -------------------- YM2608(OPNA) Interface -------------------- */
int YM2608Init(int num, int clock, int rate, int bitsize , int bufsiz , FMSAMPLE **buffer );
void YM2608Shutdown(void);
void YM2608ResetChip(int num);
void YM2608Update(void);
int YM2608Write(int n, int a,int v);
unsigned char YM2608Read(int n,int a);
int YM2608TimerOver(int n, int c );
FMSAMPLE *YM2608Buffer(int n);
int YM2608SetBuffer(int n, FMSAMPLE **buf );
#endif /* BUILD_YM2608 */

#ifdef BUILD_YM2612
int YM2612Init(int num, int clock, int rate, int bitsize , int bufsiz , FMSAMPLE **buffer );
void YM2612Shutdown(void);
void YM2612ResetChip(int num);
void YM2612Update(void);
int YM2612Write(int n, int a,int v);
unsigned char YM2612Read(int n,int a);
int YM2612TimerOver(int n, int c );
FMSAMPLE *YM2612Buffer(int n);
int YM2612SetBuffer(int n, FMSAMPLE **buf );

#endif /* BUILD_YM2612 */

#ifdef BUILD_YM2151
/* -------------------- YM2151(OPM) Interface -------------------- */
/*
** Initialize YM2151 emulator(s).
**
** 'num'     is the number of virtual YM2151's to allocate
** 'rate'    is sampling rate
** 'bitsize' is sampling bits (8 or 16)
** 'bufsiz' is the size of the buffer
*/
int OPMInit(int num, int clock, int rate, int bitsize, int bufsiz , FMSAMPLE **buffer );
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
void OPMSetIrqHandler(int n, void (*handler)(void) );

int YM2151Write(int n,int a,int v);
unsigned char YM2151Read(int n,int a);
int YM2151TimerOver(int n,int c);
#endif /* BUILD_YM2151 */

#endif /* _H_FM_FM_ */
