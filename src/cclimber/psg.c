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
** Modifications (C) 1997 Andy Milne (andy@canetics.com)
** Fixed Everything except noise generator code. I left that as is.
** Added a few comments too. Also, added in a wave table feature that
** will enable low frequency reproduction
**
*/

#include <stdio.h>
#include <stdarg.h>
#include "psg.h"
#include <math.h>
/*
** some globals ... , that should be in the AY structure?
*/
static int AYSoundRate; 	/* Output rate (Hz) */
static int AYBufSize;		/* size of sound buffer, in samples */
static int AYNumChips;		/* total # of PSG's emulated */

static AY8910 *AYPSG;		/* array of PSG's */

static int _AYInitChip(int num, SAMPLE *buf);
static void _AYFreeChip(int num);


#define WTS (sizeof(WaveTable)/sizeof(WaveTable[0]))

/* the AY8910 uses square waves as tones. Then the square waves are        */
/* "rounded" via an RC network. This network may involve a few switches    */
/* to alter the frequency response of the RC network. The reason for       */
/* this is that lower frequencies will benefit by more "rounding".         */
/* (see scramble schematic)                                                */
/* Anyway, let's cut out the middle man, and simply use sine waves         */
/* to start off with.                                                      */
/*                                                                         */
/* Now you purists are gonna say something like, "But, the analog          */
/* RC network has a 3dB per octave roll off and that means there           */
/* might be some harmonics audiable on the real thing". Well, if you       */
/* want to add in the appropriate harmonics at the appropriate levels      */
/* onto this wavetable then go right ahead and do it                       */
/* and it'll work just fine - (the cost being the table will have to       */
/* have the resolution to hold the hrmonics, i.e. table will get bigger)   */
/*                                                                         */
/* And, for jollies you can set the size to 2 to get the original          */
/* crappy square wave sound                                                */
static int WaveTable[16];




/* also, since most emulators will only be able to output sound            */
/* at 1 sample rate you might want a few wave tables for ifferent          */
/* frequency bands. For example:                                           */
/* if you sample rate is 12KHz and you want to output a tone of            */
/* 100Hz then you could use a table upto about 120 points.                 */
/* BUT, if you wanted to output a tone of 1000Hz then your table           */
/* should be less than 12 points                                           */







/*
** Initialize AY8910 emulator(s).
**
** 'num' is the number of virtual AY8910's to allocate
** 'rate' is sampling rate and 'bufsiz' is the size of the
** buffer that should be updated at each interval
*/
int AYInit(int num, int rate, int bufsiz, ... )
{
    int i;
    va_list ap;
    SAMPLE *userbuffer = 0;
    int moreargs = 1;

/* I know, don't do math in games. But, let's face it - I'm evil.          */
   for (i=0;i<WTS;i++)
      {
      WaveTable[i] = 128*cos(2*M_PI*i /(WTS));
      }

    va_start(ap,bufsiz);

    if (AYPSG) return (-1);	/* duplicate init. */

    AYNumChips = num;
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


/* this takes the continue , attack, alternate and hold bits from          */
/* the  spec and maps them to equally strange values.                      */
/* but, atleast these are explained here:                                  */
void EnvelopeInit(AY8910 *PSG)
{
/* redundancy iss the key here:                                            */
/* startval and gradient                                                   */
/* hold, mirror and repeat                                                 */
     int startval=0;   /* value of the envelope in state 0 */
     int gradient=0;  /* going up or down , implied by startval really */
     int hold=0;      /* if at the end of state 15 we hold value */
     int flip=0;      /* if at the end of state 15 we flip our value by full scale */
     int mirror=0;   /* at state 15 we carry on, but with negative gradient */
     int repeat=0;   /* at state 15 we go to state 0 and carry on */


	switch ( PSG->Regs[AY_ESHAPE] )
      {
      case 0: /*0000:                                                                    */
      case 1: /*0001:                                                                    */
      case 2: /*0010:                                                                    */
      case 3: /*0011:                                                                    */
      case 9: /*1001:                                                                    */
         startval = 15;
         gradient = -1;
         hold=1;
         flip=0;
         mirror=0;
         repeat=0;
         break;
      case 4:/*0100:                                                                    */
      case 5:/*0101:                                                                    */
      case 6:/*0110:                                                                    */
      case 7:/*0111:                                                                    */
      case 15:/*1111:                                                                    */
         startval = 0;
         gradient = 1;
         hold=1;
         flip=1;
         mirror=0;
         repeat=0;
         break;
      case 8:/*1000:                                                                    */
         startval = 15;
         gradient = -1;
         hold=0;
         flip=0;
         mirror=0;
         repeat=1;
         break;
      case 12:/*1100:                                                                    */
         startval = 0;
         gradient = 1;
         hold=0;
         flip=0;
         mirror=0;
         repeat=1;
         break;

      case 13:/*1101:                                                                    */
         startval = 0;
         gradient = 1;
         hold=1;
         flip=0;
         mirror=0;
            repeat=0;
         break;

      case 11:/*1011:                                                                    */
         startval = 15;
         gradient = -1;
         hold=1;
         flip=1;
         mirror=0;
         repeat=0;
         break;

      case 10:/*1010:                                                                    */
         startval = 15;
         gradient = -1;
         hold=0;flip=00;
         mirror=1;repeat=1;
         break;

      case 14:/*1110:                                                                    */
         startval = 0;
         gradient = 1;
         hold=0;flip=0;
         mirror=1;repeat=1;
         break;
      }


   PSG->startval=  startval;
   PSG->gradient=  gradient;
   PSG->hold=      hold;
   PSG->flip=      flip;
   PSG->mirror=    mirror;
   PSG->repeat=    repeat;


/* spec doesn't say what happens to state of chip while you                */
/* program it. You'll need to experiment with a real chip                  */
/* to know how a good a guess I made here:                                 */

   PSG->EnvStateCounter=0;
   PSG->EnvelopeCounter=0;
   PSG->Envelope=PSG->startval;
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
    PSG->StateNoise = 0;
    PSG->Incr0= PSG->Incr1 = PSG->Incr2 = PSG->Increnv = PSG->Incrnoise = 0;
    PSG->l0=0;
    PSG->l1=0;
    PSG->l2=0;

    EnvelopeInit(PSG);
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





/* write a register on AY8910 chip number 'n' */
void AYWriteReg(int n, int r, int v)
{
    AY8910 *PSG = &(AYPSG[n]);

    PSG->Regs[r] = v;

    switch( r ) {
	case AY_AVOL:
	case AY_BVOL:
	case AY_CVOL:
	    PSG->Regs[r] &= 0x1F;	/* mask volume */
	    break;
	case AY_EFINE:       /* if they mess with any Envelope value then */
	case AY_ECOARSE:     /* Initialize the Envelope, coz we know no better */
	case AY_ESHAPE:
	    PSG->Regs[AY_ESHAPE] &= 0xF;
       EnvelopeInit(PSG);
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

byte AYReadReg(int n, int r)
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


/* When volume sets Bit 0x10 it means use "Amplitude Mode" and             */
/* ignore bits 0x0F                                                        */
/* Amplitude mode decompands the data to a wider dynamic range:            */
/* 15 -> FULL scale                                                        */
/* 13 -> HALF scale                                                        */
/* 11 -> QUATER scale                                                      */
/* etc...                                                                  */
int CompandedVol[]={
0,
255/255,
255*3/255,
255/64,
255*3/128,
255/32,
255*3/64,
255/16,
255*3/32,
255/8,
255*3/16,
255/4,
255*3/8,
255/2,
255*3/4,
255,
255
};

static void _AYUpdateChip(int num)
{
    AY8910 *PSG = &(AYPSG[num]);
    int i, x;


/*
      if (0 && fp)
      {
      fprintf(fp,"INUPDATE:TYPE = %d\n",PSG->Regs[AY_ESHAPE]);
      fprintf(fp,"INUPDATE:period %d\n",(PSG->Regs[AY_EFINE]+((unsigned)PSG->Regs[AY_ECOARSE]<<8)));
      }
*/

    x = (PSG->Regs[AY_AFINE]+((unsigned)(PSG->Regs[AY_ACOARSE]&0xF)<<8));
    PSG->Incr0 = x ? AY8910_CLOCK / (x*2*16) : 0;

    x = (PSG->Regs[AY_BFINE]+((unsigned)(PSG->Regs[AY_BCOARSE]&0xF)<<8));
    PSG->Incr1 = x ? AY8910_CLOCK /  (x*2*16) : 0;


    x = (PSG->Regs[AY_CFINE]+((unsigned)(PSG->Regs[AY_CCOARSE]&0xF)<<8));
    PSG->Incr2 = x ? AY8910_CLOCK /  (x*2*16) : 0;

    x = PSG->Regs[AY_NOISEPER]&0x1F;
    PSG->Incrnoise = AY8910_CLOCK / AYSoundRate * 4 / ( x ? x : 1 );

    x = (PSG->Regs[AY_EFINE]+((unsigned)PSG->Regs[AY_ECOARSE]<<8));
    PSG->Increnv = x ? AY8910_CLOCK / (x) : 0;



    PSG->Volnoise = (
	( ( PSG->Regs[AY_ENABLE] & 010 ) ? 0 : PSG->Vol0 ) +
	( ( PSG->Regs[AY_ENABLE] & 020 ) ? 0 : PSG->Vol1 ) +
	( ( PSG->Regs[AY_ENABLE] & 040 ) ? 0 : PSG->Vol2 ) ) / 2;


/* technically if the  envelope period is small the these lines below      */
/* belong in the loop below, so make sure your sound card sample rate      */
/* is nice and high or you'll bust this code.                              */

      PSG->EnvelopeCounter += PSG->Increnv*AYBufSize;
/* 100 coz AY8910_CLOCK is not in Hz but deca-milli Hz                     */
/* 256 coz is says 256 in the spec                                         */
      if (PSG->EnvelopeCounter > 100*256*AYSoundRate)
         {
         PSG->EnvelopeCounter -= 100*256*AYSoundRate;
         PSG->EnvStateCounter++;

         if (PSG->EnvStateCounter>15)
            {
            PSG->EnvStateCounter=1; /* goto 1st state                                                          */

            if (PSG->mirror || PSG->repeat)
               {
               if (PSG->mirror)
                  {
                  PSG->gradient *=-1;
                  if (PSG->startval ==15 )
                     PSG->startval = 0;
                  else
                     PSG->startval = 15;
                  }
               PSG->Envelope = PSG->startval;
               }
            else
               { /* hold flip                                                               */
               if (PSG->flip)
                  PSG->Envelope = PSG->startval;
               PSG->gradient=0;
               }
            }
         PSG->Envelope += PSG->gradient;
         }

/* do volume or "Amplitude Mode"                                           */
    PSG->Vol0 = ( PSG->Regs[AY_AVOL] & 16 ) ? CompandedVol[PSG->Envelope] :16*PSG->Regs[AY_AVOL] ;
    PSG->Vol1 = ( PSG->Regs[AY_BVOL] & 16 ) ? CompandedVol[PSG->Envelope] :16*PSG->Regs[AY_BVOL] ;
    PSG->Vol2 = ( PSG->Regs[AY_CVOL] & 16 ) ? CompandedVol[PSG->Envelope] :16*PSG->Regs[AY_CVOL] ;

/* enable/disable voice                                                    */
    PSG->Vol0 = ( PSG->Regs[AY_ENABLE] & 001 ) ? 0 : PSG->Vol0;
    PSG->Vol1 = ( PSG->Regs[AY_ENABLE] & 002 ) ? 0 : PSG->Vol1;
    PSG->Vol2 = ( PSG->Regs[AY_ENABLE] & 004 ) ? 0 : PSG->Vol2;

    for( i=0; i<AYBufSize; ++i )
      {
      int kluge2=100*2; /* should be 100, coz of Deca-milli Hz stuff                               */



/* add increment and do a while statement instead of an if because         */
/* the sound card sample rate may be too low. OR, the WaveTable            */
/* Size was too big :)                                                     */
	PSG->Counter0 += PSG->Incr0;
   while (PSG->Counter0 > kluge2*AYSoundRate/(WTS/2) )
      {
      PSG->Counter0 -= kluge2*AYSoundRate/(WTS/2);
      PSG->l0++;
      if (PSG->l0>=WTS)
            PSG->l0=0;
      }
	PSG->Counter1 += PSG->Incr1;
   while (PSG->Counter1 > kluge2*AYSoundRate/(WTS/2) )
      {
      PSG->Counter1 -= kluge2*AYSoundRate/(WTS/2);
      PSG->l1++;
      if (PSG->l1>=WTS)
            PSG->l1=0;
      }
	PSG->Counter2 += PSG->Incr2;
   while (PSG->Counter2 > kluge2*AYSoundRate/(WTS/2) )
      {
      PSG->Counter2 -= kluge2*AYSoundRate/(WTS/2);
      PSG->l2++;
      if (PSG->l2>=WTS)
            PSG->l2=0;
      }
	PSG->Countnoise &= 0xFFFF;
	if( ( PSG->Countnoise += PSG->Incrnoise ) & 0xFFFF0000 ) {
	    /*
	    ** The following code is a random bit generator :)
	    */
	    PSG->StateNoise =
	        ( ( PSG->NoiseGen <<= 1 ) & 0x80000000
	        ? PSG->NoiseGen ^= 0x00040001 : PSG->NoiseGen ) & 1;
	}

	PSG->Buf[i] = AUDIO_CONV (
            ( WaveTable[PSG->l0] * PSG->Vol0 +
              WaveTable[PSG->l1] * PSG->Vol1 +
              WaveTable[PSG->l2] * PSG->Vol2 ) / (6*128) +
	    ( PSG->StateNoise ? PSG->Volnoise : -PSG->Volnoise ) );
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
