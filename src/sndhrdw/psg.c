/*
** $Id:$
**
** File: psg.c -- software implementation of AY-3-8910 Programmable
**		sound generator.
**
** Based on: Sound.c, (C) Ville Hallik (ville@physic.ut.ee) 1996
**
** SCC emulation removed.  Made modular and capable of multiple PSG
** emulation.
**
** Modifications (C) 1996 Michael Cuddy, Fen's Ende Software.
** http://www.fensende.com/Users/mcuddy
**
** Modifications (C) 1997 Tatsuyuki Satoh
**		Modify 1.Calculation speed tuneup.
**		       2.When all channel silent, just clear the buffer.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "driver.h"
#include "psg.h"

/*
** some globals ...
*/
static int AYSoundRate; 	/* Output rate (Hz) */
static int AYBufSize;		/* size of sound buffer, in samples */
static int AYNumChips;		/* total # of PSG's emulated */
static int AYClockFreq;	/* in deciHz */

static AY8910 *AYPSG;		/* array of PSG's */

static int _AYInitChip(int num, SAMPLE *buf);
static void _AYFreeChip(int num);


/*
** Initialize AY8910 emulator(s).
**
** 'num' is the number of virtual AY8910's to allocate
** 'rate' is sampling rate and 'bufsiz' is the size of the
** buffer that should be updated at each interval
*/
int AYInit(int num, int clock, int rate, int bufsiz, ... )
{
    int i;
    va_list ap;
    SAMPLE *userbuffer = 0;
    int moreargs = 1;

    va_start(ap,bufsiz);

    if (AYPSG) return (-1);	/* duplicate init. */

    AYNumChips = num;
    AYClockFreq = clock;
    AYSoundRate = rate;
    AYBufSize = bufsiz;

    AYPSG = (AY8910 *)malloc(sizeof(AY8910) * AYNumChips);
    if (AYPSG == NULL) return (0);
    for ( i = 0 ; i < AYNumChips; i++ ) {
	if (moreargs) userbuffer = va_arg(ap,SAMPLE*);
	if (userbuffer == NULL) moreargs = 0;
	if (_AYInitChip(i,userbuffer) < 0) {
	    int j;
	    for ( j = 0 ; j < i ; j ++ ) {
		_AYFreeChip(j);
	    }
	    return(-1);
	}
    }
    return(0);
}

void AYShutdown()
{
    int i;

    if (!AYPSG) return;

    for ( i = 0 ; i < AYNumChips ; i++ ) {
	_AYFreeChip(i);
    }
    free(AYPSG); AYPSG = NULL;

    AYSoundRate = AYBufSize = 0;
}

/*
** reset all chip registers.
*/
void AYResetChip(int num)
{
    int i;
    AY8910 *PSG = &(AYPSG[num]);

    memset(PSG->Buf,'\0',AYBufSize);

    /* initialize hardware registers */
    for( i=0; i<16; i++ ) PSG->Regs[i] = AUDIO_CONV(0);

    PSG->Regs[AY_ENABLE] = 077;
    PSG->Regs[AY_AVOL] = 0;
    PSG->Regs[AY_BVOL] = 0;
    PSG->Regs[AY_CVOL] = 0;
    PSG->NoiseGen = 1;
    PSG->Envelope = 15;
    PSG->Incr0= PSG->Incr1 = PSG->Incr2 = PSG->Increnv = PSG->Incrnoise = 0;
}

/*
** allocate buffers and clear registers for one of the emulated
** AY8910 chips.
*/
static int _AYInitChip(int num, SAMPLE *buf)
{
    AY8910 *PSG = &(AYPSG[num]);

    PSG->UserBuffer = 0;
    if (buf) {
	PSG->Buf = buf;
	PSG->UserBuffer = 1;
    } else {
	if ((PSG->Buf=(SAMPLE *)malloc(AYBufSize))==NULL)
	    return -1;
    }
    memset(PSG->Buf,AUDIO_CONV(0),AYBufSize);

    PSG->Port[0] = PSG->Port[1] = NULL;

    AYResetChip(num);
    return 0;
}

/*
** release storage for a chip
*/
static void _AYFreeChip(int num)
{
    AY8910 *PSG = &(AYPSG[num]);

    if (PSG->Buf && !PSG->UserBuffer) free(PSG->Buf); PSG->Buf = NULL;
}

static unsigned char _AYEnvForms[16][32] = {
	{ 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
	   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
	{ 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
	   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
	{ 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
	   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
	{ 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
	   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
	{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
	{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
	{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
	{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
	{ 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
	  15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0 },
	{ 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
	   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
	{ 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
	   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
	{ 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
	  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15 },
	{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
	{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	  15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15 },
	{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	  15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0 },
	{  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }
};


/* write a register on AY8910 chip number 'n' */
void AYWriteReg(int n, int r, int v)
{

    AY8910 *PSG = &(AYPSG[n]);

    if (r > 15)
      osd_ym2203_write(n, r, v);

    PSG->Regs[r] = v;

    switch( r ) {
	case AY_AVOL:
	case AY_BVOL:
	case AY_CVOL:
	    PSG->Regs[r] &= 0x1F;	/* mask volume */
	    break;
	case AY_ESHAPE:
	    PSG->Regs[AY_ESHAPE] &= 0xF;
	    /* fall through */
	case AY_EFINE:
	case AY_ECOARSE:
	    PSG->Countenv=0;
	    break;
	case AY_NOISEPER:
	    PSG->Regs[AY_NOISEPER] &= 0x1F;
		break;
	case AY_PORTA:
	    if (PSG->Port[0]) {
		(PSG->Port[0])(PSG,AY_PORTA,1,v);
	    }
	    break;
	case AY_PORTB:
	    if (PSG->Port[1]) {
		(PSG->Port[1])(PSG,AY_PORTB,1,v);
	    }
	    break;
    }
}

unsigned char AYReadReg(int n, int r)
{
    AY8910 *PSG = &(AYPSG[n]);

    switch (r) {
	case AY_PORTA:
	    if (PSG->Port[0]) {
		(PSG->Port[0])(PSG,AY_PORTA,0,0);
	    }
	    break;
	case AY_PORTB:
	    if (PSG->Port[1]) {
		(PSG->Port[1])(PSG,AY_PORTB,0,0);
	    }
	    break;
    }
    return PSG->Regs[r];
}

static void _AYUpdateChip(int num)
{
    AY8910 *PSG = &(AYPSG[num]);
    int i, x;
    int c0, c1, l0, l1, l2;
	int vb0 , vb1 , vb2 , vbn;


    x = (PSG->Regs[AY_AFINE]+((unsigned)(PSG->Regs[AY_ACOARSE]&0xF)<<8));
    PSG->Incr0 = x ? AYClockFreq / AYSoundRate * 4 / x : 0;

    x = (PSG->Regs[AY_BFINE]+((unsigned)(PSG->Regs[AY_BCOARSE]&0xF)<<8));
    PSG->Incr1 = x ? AYClockFreq / AYSoundRate * 4 / x : 0;

    x = (PSG->Regs[AY_CFINE]+((unsigned)(PSG->Regs[AY_CCOARSE]&0xF)<<8));
    PSG->Incr2 = x ? AYClockFreq / AYSoundRate * 4 / x : 0;

    x = PSG->Regs[AY_NOISEPER];
    PSG->Incrnoise = x ? AYClockFreq / AYSoundRate * 4 / x : 0;

    x = (PSG->Regs[AY_EFINE]+((unsigned)PSG->Regs[AY_ECOARSE]<<8));
    PSG->Increnv = x ? AYClockFreq / AYSoundRate * 4 / x * AYBufSize : 0;

    PSG->Envelope = _AYEnvForms[PSG->Regs[AY_ESHAPE]][(PSG->Countenv>>16)&0x1F];

    if( ( PSG->Countenv += PSG->Increnv ) & 0xFFE00000 ) {
        switch( PSG->Regs[AY_ESHAPE] ) {
	    case 8:
	    case 10:
	    case 12:
	    case 14:
	        PSG->Countenv -= 0x200000;
	        break;
	    default:
	        PSG->Countenv = 0x100000;
	        PSG->Increnv = 0;
	}
    }

    PSG->Vol0 = ( PSG->Regs[AY_AVOL] < 16 ) ? PSG->Regs[AY_AVOL] : PSG->Envelope;
    PSG->Vol1 = ( PSG->Regs[AY_BVOL] < 16 ) ? PSG->Regs[AY_BVOL] : PSG->Envelope;
    PSG->Vol2 = ( PSG->Regs[AY_CVOL] < 16 ) ? PSG->Regs[AY_CVOL] : PSG->Envelope;

    PSG->Volnoise = (
		( ( PSG->Regs[AY_ENABLE] & 0x08 ) ? 0 : PSG->Vol0 ) +
		( ( PSG->Regs[AY_ENABLE] & 0x10 ) ? 0 : PSG->Vol1 ) +
		( ( PSG->Regs[AY_ENABLE] & 0x20 ) ? 0 : PSG->Vol2 ) );

    PSG->Vol0 = ( PSG->Regs[AY_ENABLE] & 0x01 ) ? 0 : PSG->Vol0;
    PSG->Vol1 = ( PSG->Regs[AY_ENABLE] & 0x02 ) ? 0 : PSG->Vol1;
    PSG->Vol2 = ( PSG->Regs[AY_ENABLE] & 0x04 ) ? 0 : PSG->Vol2;

	/* if the frequency is 0, shut down the voice */
    if (PSG->Incr0 == 0) PSG->Vol0 = 0;
    if (PSG->Incr1 == 0) PSG->Vol1 = 0;
    if (PSG->Incr2 == 0) PSG->Vol2 = 0;
    if (PSG->Incrnoise == 0) PSG->Volnoise = 0;

	/* set voltage base of output */
	if( !(vb0 = PSG->Vol0*((PSG->Counter0&0x8000)?-16:16))){
		PSG->Counter0 = (PSG->Counter0 + PSG->Incr0*AYBufSize)&0xffff;
	}
	if( !(vb1 = PSG->Vol1*((PSG->Counter1&0x8000)?-16:16))){
		PSG->Counter1 = (PSG->Counter1 + PSG->Incr1*AYBufSize)&0xffff;
	}
	if( !(vb2 = PSG->Vol2*((PSG->Counter2&0x8000)?-16:16))){
		PSG->Counter2 = (PSG->Counter2 + PSG->Incr2*AYBufSize)&0xffff;
	}
	vbn = PSG->Volnoise*((PSG->NoiseGen&1)?12:-12);

	if( !PSG->Vol0 && !PSG->Vol1 && !PSG->Vol2 && !PSG->Volnoise ){
		/* all silent */
	    memset(PSG->Buf,AUDIO_CONV(0),AYBufSize);
		return;
	}

    for( i=0; i<AYBufSize; ++i ) {

   	/*
	** These strange tricks are needed for getting rid
	** of nasty interferences between sampling frequency
	** and "rectangular sound" (which is also the output
	** of real AY-3-8910) we produce.
	*/

	if( (l0=vb0) ){
		c0 = PSG->Counter0;
		c1 = PSG->Counter0 + PSG->Incr0;
		if( (c0^c1)&0x8000 ) {
		    l0=l0*(0x8000-(c0&0x7FFF)-(c1&0x7FFF))/PSG->Incr0;
			vb0 = -vb0;
		}
		PSG->Counter0 = c1 & 0xFFFF;
	}

	if( (l1=vb1) ){
		c0 = PSG->Counter1;
		c1 = PSG->Counter1 + PSG->Incr1;
		if( (c0^c1)&0x8000 ) {
		  l1=l1*(0x8000-(c0&0x7FFF)-(c1&0x7FFF))/PSG->Incr1;
		  vb1 = -vb1;
		}
		PSG->Counter1 = c1 & 0xFFFF;
	}

	if( (l2=vb2) ){
		c0 = PSG->Counter2;
		c1 = PSG->Counter2 + PSG->Incr2;
		if( (c0^c1)&0x8000 ) {
		  l2=l2*(0x8000-(c0&0x7FFF)-(c1&0x7FFF))/PSG->Incr2;
		  vb2 = -vb2;
		}
		PSG->Counter2 = c1 & 0xFFFF;
	}

	if( vbn ){
		PSG->Countnoise &= 0xFFFF;
		if( ( PSG->Countnoise += PSG->Incrnoise ) & 0xFFFF0000 ) {
		    /*
		    ** The following code is a random bit generator :)
		    */
			if( ( PSG->NoiseGen <<= 1 ) & 0x80000000 ){
				PSG->NoiseGen ^= 0x00040001;
				vbn = PSG->Volnoise*12;
			}else{
				vbn = PSG->Volnoise*-12;
			}
		}
	}
	PSG->Buf[i] = AUDIO_CONV ( l0 + l1 + l2 + vbn ) / 8;
    }
}

/*
** called to update all chips
*/
void AYUpdate(void)
{
    int i;

    for ( i = 0 ; i < AYNumChips; i++ ) {
	_AYUpdateChip(i);
    }
}


/*
** return the buffer into which AYUpdate() has just written it's sample
** data
*/
SAMPLE *AYBuffer(int n)
{
    return AYPSG[n].Buf;
}

void AYSetBuffer(int n, SAMPLE *buf)
{
    AYPSG[n].Buf = buf;
}

/*
** set a port handler function to be called when AYWriteReg() or AYReadReg()
** is called for register AY_PORTA or AY_PORTB.
**
*/
void AYSetPortHandler(int n, int port, AYPortHandler func)
{
    port -= AY_PORTA;
    if (port > 1 || port < 0 ) return;

    AYPSG[n].Port[port] = func;
}

/*
** Change Log
** ----------
** $Log:$
*/
