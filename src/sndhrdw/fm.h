/*

  File: fm.h -- header file for software emuration for FM sound generator

   Copyright(c) 1998 Tatsuyuki Satoh

*/
#ifndef _H_FM_FM_
#define _H_FM_FM_

//#define OPM_STEREO		/* stereo mixing / separate */

typedef void FMSAMPLE;

#define YM2151_NUMBUF 2    /* stereo separate L and R ch */
//#define YM2151_NUMBUF 1    /* stereo mix L and R ch */
#define YM2203_NUMBUF 1


/*
** Initialize YM2203 emulator(s).
**
** 'num'     is the number of virtual YM2203's to allocate
** 'rate'    is sampling rate
** 'bitsize' is sampling bits (8 or 16)
** 'bufsiz' is the size of the buffer
*/
int OPNInit(int num, int clock, int rate, int bitsize , int bufsiz , FMSAMPLE **buffer );

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

/* ----- get pointer sound buffer ----- */
FMSAMPLE *OPNBuffer(int n);
/* ----- set sound buffer ----- */
int OPNSetBuffer(int n, FMSAMPLE *buf );
/* ---- set user interrupt handler ----- */
void OPNSetIrqHandler(int n, void (*handler)(void) );

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

#endif /* _H_FM_FM_ */
