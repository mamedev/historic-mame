/*
** $Id:$
**
** File: psg/psg.h -- header file for software implementation of AY-3-8910
**		Programmable sound generator.  This module is used in my
**		GYRUSS emulator.
**
** Based on: Sound.c, (C) Ville Hallik (ville@physic.ut.ee) 1996
**
** SCC emulation removed.  Made modular and capable of multiple PSG
** emulation.
**
** Modifications (C) 1996 Michael Cuddy, Fen's Ende Software.
** http://www.fensende.com/Users/mcuddy
**
*/
#ifndef _H_PSG_PSG_
#define _H_PSG_PSG_

/* register id's */
#define AY_AFINE	(0)
#define AY_ACOARSE	(1)
#define AY_BFINE	(2)
#define AY_BCOARSE	(3)
#define AY_CFINE	(4)
#define AY_CCOARSE	(5)
#define AY_NOISEPER	(6)
#define AY_ENABLE	(7)
#define AY_AVOL		(8)
#define AY_BVOL		(9)
#define AY_CVOL		(10)
#define AY_EFINE	(11)
#define AY_ECOARSE	(12)
#define AY_ESHAPE	(13)

#define AY_PORTA	(14)
#define AY_PORTB	(15)



/* SAMPLE_16BIT is not supported, yet */
#ifndef SAMPLE_16BIT
typedef unsigned char SAMPLE;
/* #define AUDIO_CONV(A) (128+(A))*/	/* use this macro for signed samples */
#define AUDIO_CONV(A) (A)		/* use this macro for unsigned samples */
#else
typedef unsigned short SAMPLE;
#endif

/* forward declaration of typedef, so we can use this in next typedef ... */
typedef struct ay8910_f AY8910;

/*
** this is a handler for the AY8910's I/O ports -- called when
** AYWriteReg(AY_PORTA) or AYWriteReg(AY_PORTB) is called.
*/
typedef unsigned char (*AYPortHandler)(AY8910 *, int port, int iswrite, unsigned char val);

/* here's the virtual AY8910 ... */
struct ay8910_f {
    SAMPLE *Buf;	/* sound buffer */
    int UserBuffer;	/* if user provided buffers */
    AYPortHandler Port[2];	/* 'A' and 'B' port */
    unsigned char Regs[264];

    /* state variables */
    int Incr0, Incr1, Incr2;
    int Increnv, Incrnoise;
    int StateNoise, NoiseGen;
    int Counter0, Counter1, Counter2, Countenv, Countnoise;
    int Vol0, Vol1, Vol2, Volnoise, Envelope;
};

/*
** Initialize AY8910 emulator(s).
**
** 'num' is the number of virtual AY8910's to allocate
** 'rate' is sampling rate and 'bufsiz' is the size of the
** buffer that should be updated at each interval
** varargs are user-supplied buffers, one for each PSG (or NULL
** to let PSG library allocate buffers)
*/
int AYInit(int num, int clock, int rate, int bufsiz, ...);

/*
** shutdown the AY8910 emulators .. make sure that no sound system stuff
** is touching our audio buffers ...
*/
void AYShutdown(void);

/*
** reset all chip registers for AY8910 number 'num'
*/
void AYResetChip(int num);

/*
** called to update all chips; should be called about 50 - 70 times per
** second ... (depends on sample rate and buffer size)
*/
void AYUpdate(void);

/*
** write 'v' to register 'r' on AY8910 chip number 'n'
*/
void AYWriteReg(int n, int r, int v);

/*
** read register 'r' on AY8910 chip number 'n'
*/
unsigned char AYReadReg(int n, int r);


/*
** return pointer to synthesized data
*/
SAMPLE *AYBuffer(int n);
/*
** setup buffer
*/
void AYSetBuffer(int n, SAMPLE *buf);

/*
** set handler function for I/O port
*/
void AYSetPortHandler(int n, int port, AYPortHandler func);

#endif /* _H_PSG_PSG_ */
/*
** Change Log
** ----------
** $Log:$
*/
