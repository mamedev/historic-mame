/*$DEADSERIOUSCLAN$*********************************************************************
* FILE
*	Yamaha 3812 emulator
*
* CREATED BY
*	(c) Carl-Henrik Skårstedt, Dead Serious Clan. http://www.deadserious.com/
*
* UPDATE LOG
*	CHS 1999-01-03	Changed digital sound generation.
*	CHS 1998-10-31	Created general version combining all specifics from other versions EXCEPT include files.
*	CHS 1998-10-05	Fixed bug that caused ym3526 games to overwrite other OPL registers.
*	EHC 1998-09-25	Changed timers to double precision for Bubble Bobble fix (see comment in WriteReg)
*	CHS 1998-09-22	Release version 1.0.
*	CHS 1998-09-17	Fixed bug group: All C++ specific stuff was removed. Now actually compiles as C code.
*	CHS 1998-09-16	Fixed bug: the octave was shifted right 1 bit too much. Fixed related bugs.
*	CHS 1998-09-15	Fixed the update routine to output sound more like the original chip.
*	CHS 1998-09-12	Added subdivision of the update loop (recalc of envelop etc. during update loop) Warning: Using this feature adds noise.
*	CHS 1998-09-01	Beta version.
*
* TO DO
*	KSR correction
*	Computer Speech Mode (requires real timers and a speech driver which makes testing a bit difficult) (later update)
*
* Version 1.2:
* ------------
* Updated algorithm.
* The emulator was first used in the Shark emulator from The Dead Serious Clan.
*
* Thanks to:
* ----------
* CAB (Author of AMUSE), Neil Bradley, Mamedev, Richard Bush and
* lots of people for testing the emulation and suggesting improvements.
*
* Legal:
* ------
* The ym3812 emulator may not be sold, or sold as a part of a commercial
* package without the express written permission of Carl-Henrik Skarstedt
* (carl@c64.org). This includes shareware.
*
* Licensing:
* ----------
* Licensing of ym3812 emulator for commercial applications is available.
* Please email carl@c64.org for details.
*
* Distribution:
* -------------
* Modified versions of the ym3812 emulator may not be publicly redistributed
* without author approval (carl@c64.org). This includes distributing via a
* publicly accessible LAN. You may make your own source modifications and
* distribute the ym3812 emulator in source or object form, but if you make
* modifications to the ym3812 emulator then it should be noted in the top
* as a comment in ym3812.h and ym3812.c. Please also inform me of any
* useful modifications of the source if redistributed (carl@c64.org).
* The legal, distribution, licensing, guarantee, support and credits
* sections in the file header may not be altered in redistributed modified
* source form.
*
* Do the emulator come with any guarantee or author responsibility?
* -----------------------------------------------------------------
* No. It just plays some sounds, that's it. The source is provided "as-is".
* The author can not be held responsible for any damage done by the use
* of the ym3812 emulator. Any hardware patents or licensing infringements
* issues are referred to the general legal status of emulation. No
* intellectual property has been included from the original chip.
*
* Support?
* --------
* carl@c64.org
* ym3812, ym3526 and ym2413 (with 2413 register conversion patch)
*
* Credits:
* --------
* Please add something like "ym3812 emu by Carl-Henrik Skarstedt, DSC."
* in your product if you use the emulator, or feel free to expand that in
* any appropriate way. The "a" in my last name is supposed to have a
* little ring over it if you manage to find that character.
*
* How to run the emulator:
* ------------------------
*
* You must get drum samples (see ym3812_pDrumNames for a list of
* filenames of drums that are programmed into the OPL2 chip). These
* samples are 8-bit unsigned .raw (no header) 11025 Hz.
* I have unfortunately not got any freeware drum sounds available
* at this time. I wish I had the st-01: floppy somewhere....
*
* Initialize with:
*
* // This will allocate a structure and initialize it
* pOPL = ym3812_Init( int nReplayFrq, int nClock, int f16Bit );
* pOPL->SetTimer = My_Timer_Code;
*
* Where pOPL is the work structure for the ym3812 emulator and
* My_Timer_Code handles timer control (You don't NEED to use
* your own timers, but it is recommended. If you do not program
* your own timers, ignore the second line)
*
* The parameters for the Init function are:
* nReplayFrq = The frequency you intend to play the generated sample with (for example 44100 Hz)
* nClock = Set to ym3812_StdClock unless you know it is tweaked for something else.
* f16Bit = false means generate 8 bit samples, true means generate 16 bit samples.
*
* *1: If you use 16 bit sound, your buffer needs to be twice as big (in bytes) as the
* number of samples you pass into the emulator. This is because I can not fit 16
* bits in one byte.
*
* Each (sound) frame do the following:
* Call ym3812_Update( pOPL, pBuffer, nLength )
*
* Where:
* pOPL is the OPL structure
* pBuffer is a pointer to where in memory you want the digital emulated OPL sound
* nLength is the amount of samples you want by calling the routine this time
*	(bytes in 8-bit mode or shorts in 16-bit mode)
*
* In the place where the hardware writes and reads the
* OPL chip registers (2 addresses) do this:
*
* Write to address 0: void ym3812_SetReg( ym3812*, Byte );
* Read from address 0: Byte = ym3812_ReadStatus( ym3812* );
* Write to address 1: void ym3812_WriteReg( ym3812*, Byte );
* Read from address 1: Byte = ym3812_ReadReg( ym3812* );
*
* When you're done emulating, Call ym3812_DeInit( ym3812* ) to free some memory.
*
* If you wish the emulator to subdivide the generated sound
* more than each frame ( refresh envelope, vibrato, etc.), put the
* number of subdivisions in pOPL->nSubDivide after the initialization.
*
* You can change replay frequency during the emulation by
* setting your desired frequency in pOPL->nReplayFrq (in Hz).
*
* (And don't forget to replay the buffer)
*
* How to use the timer emulation without real timers in the emulator:
* -------------------------------------------------------------------
* First of all, use of this method is not recommended. Instead,
* see below how to create your own timer code. If you still want to
* use my placeholder timer routines, this is the way I use them.
*
* Keep the define ym3812_AUTOMATIC which counts up the timers each frame.
* Call ym3812_CheckTimer1Int() before the interrupt associated with the
* timer. If it returns false you skip the interrupt. Make sure you
* perform this check more often than the timers overflow, otherwise they
* will only accumulate and not run fast enough. Usually you can get away
* by calling the timerchecks two times for each screen frame. Perform the
* same procedure with ym3812_CheckTimer2Int() if you have any interrupt
* associated with that timer. I have not seen any example of this though.
*
* You can also switch off ym3812_AUTOMATIC and call
* ym3812_UpdateTimers( ym3812*, float Time ) with Time being the
* (emulated) time between each call of ym3812_UpdateTimers.
*
* Here's how code your own timers:
* --------------------------------
* Initialize the ym3812 emulator in this way:
*
* pOPL = ym3812_Init();
* pOPL->SetTimer = My_Timer_Code;
*
* Where My_Timer_Code is a function that is called each time a timer
* is created or removed. The timer will run continously, and generate
* signals for the interrupt.
*
* This pseudo code example will probably explain clearer than
* just words how to handle the timer code:
*
* void My_Timer_Code(int nTimer, float vPeriod, ym3812_s *pOPL, int fRemove)
* {
*    switch( nTimer )
*    {
*       case 1:
*           if( fRemove )   timer_delete( __Timer1ID );
*           else            timer_create( __Timer1ID, My_Timer_Event_1, vPeriod );
*           break;
*       case 2:
*           if( fRemove )   timer_delete( __Timer2ID );
*           else            timer_create( __Timer2ID, My_Timer_Event_2, vPeriod );
*           break;
*    }
* }
*
* void My_Timer_Event_1()
* {
*    if( ym3812_TimerEvent( pOPL, 1 ) ) Generate_Sound_Interrupt_From_Timer1();
* }
*
* And My_Timer_Event_2() would be similar to My_Timer_Event_1().
*
* You _NEED_ to call ym3812_TimerEvent() to update the Status Register
* and to find out if the timer is masked (not generating interrupts).
* If you don't call ym3812_TimerEvent() the sound code emulation will
* get confused and usually halts the whole game.
*
* My_Timer_Code will ONLY be called when a Timer is to be created or deleted,
* unless someone overwrites the timer control byte. The timer event will
* of course be continous until the timer is removed.
*
* (END OF INSTRUCTIONS AND WARRANTY INFORMATION)
*
* NOTES
*
****************************************************************************************/

//#define SHARKVER
#define MAMEVER
//#define GENERALVER

#ifdef SHARKVER
#include "wininc.h"
#endif
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

// Include MAME stuff. If not Mame, then default to LSB_FIRST (PC)

#ifdef MAMEVER
#include "driver.h"
#include "common.h"
#else
#define LSB_FIRST
#endif

#include "ym3812.h"
#ifdef SHARKVER
#include "sharkdata.h"
#endif

// The number of entries in the sinus table! Do not change! Tweaked code depends on these values!
#define SINTABLE_SIZE 8192
#define SINTABLE_MAX 32767
#define SINTABLE_SHIFT 15
#define SINTABLE_SIZESHIFT 13

// These numbers define attack and decay/sustain rates
#define cATTMUL 0.11597
#define cDECMUL 0.6311

#ifndef PI
#define PI 3.141592654f
#endif

#ifdef LSB_FIRST
#define INTPART 1
#define FRACPART 0
#else
#define INTPART 0
#define FRACPART 1
#endif

// Mame converter
#ifndef TIME_IN_USEC
#define TIME_IN_USEC(x) ( ((double)x)/1000000.0 )
#endif

/*$DEADSERIOUSCLAN$*********************************************************************
* ARRAY
*  int	RegSlot_Relation[] =
* NOTES
*  Gives the relation between address and slot...
****************************************************************************************/
int	RegSlot_Relation[] =
{
	0x00,0x02,0x04,0x01,0x03,0x05,-1,-1,
	0x06,0x08,0x0a,0x07,0x09,0x0b,-1,-1,
	0x0c,0x0e,0x10,0x0d,0x0f,0x11,-1,-1,
	  -1,  -1,  -1,  -1,  -1,  -1,-1,-1,-1,-1
};

/*$DEADSERIOUSCLAN$*********************************************************************
* ROUTINE
*  int ym3812_Multi[] = { 1,2,4,6,8,10,12,14,16,18,20,20,24,24,30,30 };
* NOTES
*  Gives the multi. This table is multiplied by 2 since 0 means 1/2.
****************************************************************************************/
int ym3812_Multi[] = { 1,2,4,6,8,10,12,14,16,18,20,20,24,24,30,30 };

/*$DEADSERIOUSCLAN$*********************************************************************
* ROUTINE
*  int16	aSinTable[SINTABLE_SIZE];
* FUNCTION
* NOTES
****************************************************************************************/
short	*ym3812_aSinTable0;
short	ym3812_aSinTable[4][SINTABLE_SIZE];

/*$DEADSERIOUSCLAN$*********************************************************************
* ROUTINE
*  float	ym3812_aAttackTime[16];
*  float	ym3812_aDecayTime[16];
* NOTES
*  Time for attack and decay rates
****************************************************************************************/
float	ym3812_aAttackTime[16];
float	ym3812_aDecayTime[16];

/*$DEADSERIOUSCLAN$*********************************************************************
* ROUTINE
*  char *ym3812_pDrumNames
* FUNCTION
* NOTES
*  Sample file names for each drum
****************************************************************************************/
#ifdef GENERALVER
char *ym3812_pDrumNames[5] = { "ym3812/bassdrum.raw", "ym3812/snardrum.raw", "ym3812/tomtom.raw", "ym3812/topcmbal.raw", "ym3812/hihat.raw" };
#endif
#ifdef MAMEVER
static const char *ym3812_pDrumNames[] =
{
	"bassdrum.wav",
	"snardrum.wav",
	"tomtom.wav",
	"topcmbal.wav",
	"hihat.wav",
	0
};
#endif

/*$DEADSERIOUSCLAN$*********************************************************************
* ROUTINE
*  ym3812* ym3812_Init( int nReplayFrq, int nClock, int f16Bit )
* FUNCTION
* NOTES
****************************************************************************************/
ym3812* ym3812_Init( int nReplayFrq, int nClock, int f16Bit )
{
	int 				l;
	ym3812				*pOPL;
	float				vValue,vMinVal;
	signed char 		*pDrum,*pDrum8;
	signed short		*pDrum16;
#ifdef MAMEVER
	struct GameSamples	*psSamples;
	int					k;
#endif

	pOPL = (ym3812*)malloc(sizeof(ym3812));
	memset( pOPL, 0x00, sizeof(ym3812));

	// Set appropriate sample type (16bit or 8bit)

	pOPL->f16Bit = f16Bit;

	// Insert stuff from host

	pOPL->nReplayFrq = nReplayFrq;
	pOPL->nYM3812Clk = nClock;
	pOPL->nYM3812DivClk = nClock/72;
	pOPL->SetTimer = NULL;
	pOPL->nSubDivide = 1;
	pOPL->vTimer1 = 0.01f;
	pOPL->vTimer2 = 0.01f;

	// Generate some volume levels.. (ln)

	vValue = (float)(ym3812_StdVolume<<(1+8));
	vMinVal = vValue*pow(0.9994,4096);
	for( l=0 ; l<4096 ; l++ )
	{
		vValue *= 0.9994f;
		pOPL->aVolumes[4095-l] = (int)(vValue-vMinVal);
	}

	// Generate a sinus table
	for( l=0 ; l<SINTABLE_SIZE ; l++ )
	{
		ym3812_aSinTable[0][l] = (short)(SINTABLE_MAX * sin( 2 * PI * l / SINTABLE_SIZE ));
		ym3812_aSinTable[1][l] = (l<SINTABLE_SIZE/2) ? ym3812_aSinTable[0][l] : 0;
		ym3812_aSinTable[2][l] = abs(ym3812_aSinTable[0][l]);
		ym3812_aSinTable[3][l] = ~(l&(SINTABLE_SIZE/4)) ? ym3812_aSinTable[0][l] : 0;
	}

	// Calculate attack and decay time (release is same as decay)
	for( l=1 ; l<16 ; l++ )
	{
		ym3812_aAttackTime[l] = (float)(( 1<<(15-l) ) * cATTMUL)/1000.0f;
		ym3812_aDecayTime[l] = (float)(( 1<<(15-l) ) * cDECMUL)/1000.0f;
	}
	ym3812_aAttackTime[0] = 0;
	ym3812_aDecayTime[0] = 0;

	// Initialize all slots
	for( l=0 ; l<18 ; l++ )
	{
		pOPL->vEnvTime[l] = 0.0f;			// No time
		pOPL->nEnvState[l] = ADSR_Silent;	// Envelop is not playing
		pOPL->nTotalLevel[l] = 0;			// Envelop volume is 0
		pOPL->fEGTyp[l] = cFALSE;
		pOPL->fVibrato[l] = cFALSE;
		pOPL->fAM[l] = cFALSE;
		pOPL->fKSR[l] = cFALSE;
		pOPL->nKSL[l] = 0;
		pOPL->nMulti[l] = 0;
		pOPL->nWave[l] = 0;
		pOPL->nAttack[l] = 0x8;
		pOPL->nDecay[l] = 0x8;
		pOPL->nSustain[l] = 0x8;
		pOPL->nRelease[l] = 0x8;
		pOPL->nCurrPos[l] = 0;//rand() * SINTABLE_SIZE / RAND_MAX;
	}

	// Initialize all channels
	for( l=0; l<9; l++ )
	{
		pOPL->nFNumber[l]=0x100;
		pOPL->nOctave[l]=0;
		pOPL->nFeedback[l]=0;
		pOPL->fConnection[l]=cFALSE;
		pOPL->fKeyDown[l]=cFALSE;
	}

	// Load Rhythm sounds
#ifdef SHARKVER
	pOPL->pDrum[0] = ym3812_BassDrum_Raw;
	pOPL->pDrum[1] = ym3812_SnareDrum_Raw;
	pOPL->pDrum[2] = ym3812_TomTom_Raw;
	pOPL->pDrum[3] = ym3812_TopCymbal_Raw;
	pOPL->pDrum[4] = ym3812_HiHat_Raw;

	pOPL->nDrumSize[0] = sizeof( ym3812_BassDrum_Raw );
	pOPL->nDrumSize[1] = sizeof( ym3812_SnareDrum_Raw );
	pOPL->nDrumSize[2] = sizeof( ym3812_TomTom_Raw );
	pOPL->nDrumSize[3] = sizeof( ym3812_TopCymbal_Raw );
	pOPL->nDrumSize[4] = sizeof( ym3812_HiHat_Raw );

	for( l=0;l<5;l++ )
	{
		pOPL->nDrumRate[l] = 11025;
		pOPL->nDrumOffs[l] = -1;
	}
#endif

#ifdef MAMEVER
	// MAME version
	psSamples = readsamples(ym3812_pDrumNames,"ym3812");
	for( l=0; l<5; l++ )
	{
		pOPL->pDrum[l] = NULL;
		pOPL->nDrumOffs[l] = -1;
		if( psSamples && psSamples->sample[l] != 0 )
		{
			pOPL->nDrumRate[l] = psSamples->sample[l]->smpfreq; 			// Get frequency
			pOPL->nDrumSize[l] = psSamples->sample[l]->length;				// Get size
			pOPL->pDrum[l] = (signed char*) malloc( pOPL->nDrumSize[l]+1024 );
			memset( pOPL->pDrum[l], 0, pOPL->nDrumSize[l]+1024 );			// Clear buffer

			pDrum8 = psSamples->sample[l]->data;							// Get ptr to sample

			pDrum16 = (signed short*) pDrum8;								// Get 16 bit ptr to sample
			pDrum = pOPL->pDrum[l];
			for( k=pOPL->nDrumSize[l]; k>0; k-- )
			{
				if( psSamples->sample[l]->resolution==16 )	*pDrum++ = (*pDrum16++)>>8;
				else										*pDrum++ = *pDrum8++;
			}
		}
	}
	freesamples( psSamples );
#endif

#ifdef GENERALVER
	for( l=0 ; l<5 ; l++ )
	{
		pOPL->pDrum[l] = NULL;
		pOPL->nDrumOffs[l] = -1;
		pOPL->nDrumRate[l] = 11025;
		pFile = fopen( ym3812_pDrumNames[l], "rb" );
		if( pFile != NULL )
		{
			fseek( pFile, 0, SEEK_END );									// Go to end
			pOPL->nDrumSize[l] = ftell( pFile );							// Get file size
			fseek( pFile, 0, SEEK_SET );									// Rewind
			pOPL->pDrum[l] = (char*) malloc( pOPL->nDrumSize[l]+1024 );		// Alloc buffer
			memset( pOPL->pDrum[l], 0, pOPL->nDrumSize[l]+1024 );			// Clear buffer
			fread( pOPL->pDrum[l], pOPL->nDrumSize[l], 1, pFile );			// Read buffer
			fclose( pFile );												// Close file
			//Make sample signed.. Originally unsigned.
			pSamp = pOPL->pDrum[l];
			for( k=pOPL->nDrumSize[l]; k>0; k-- )
			{
				*pSamp++ ^= 0x80;
			}
		}
	}
#endif

	return pOPL;
}

/*$DEADSERIOUSCLAN$*********************************************************************
* ROUTINE
*  ym3812* ym3812_DeInit( ym3812 *pOPL )
* FUNCTION
* NOTES
****************************************************************************************/
ym3812* ym3812_DeInit( ym3812 *pOPL )
{
	if( pOPL != NULL )
	{
		free( pOPL );
		pOPL = NULL;
	}
	return pOPL;
}

/*$DEADSERIOUS$*******************************************************************
* ROUTINE
*	void ym3812_UpdateTimers( float vTime )
* AUTHOR
*	Carl-Henrik Skårstedt
* FUNCTION/NOTES
*	Call this with the time that has passed from previous UpdateTimers.
*	Do not use this function externally if you have opted to update timers
*	in the main update routine (ym3812_AUTOMATIC).
**********************************************************************************/

void ym3812_UpdateTimers( ym3812 *pOPL, float vTime )
{
	if( pOPL->nTimerCtrl & ym3812_TCST1 ) pOPL->vTimer1IntCnt += vTime;
	if( pOPL->nTimerCtrl & ym3812_TCST2 ) pOPL->vTimer2IntCnt += vTime;
}

/*$DEADSERIOUSCLAN$*********************************************************************
* ROUTINE
*  ym3812_CheckTimer1Int( ym3812 *pOPL )
* FUNCTION
*  Check if it is time for an ym3812 Timer A interrupt
* NOTES
****************************************************************************************/
int ym3812_CheckTimer1Int( ym3812 *pOPL )
{
	if( ((~pOPL->nTimerCtrl)&ym3812_TCMASK1) && (pOPL->nTimerCtrl&ym3812_TCST1) )
	{
		if( pOPL->vTimer1IntCnt>pOPL->vTimer1 )
		{
			pOPL->nStatus |= ym3812_STIRQ | ym3812_STFLAG1;
			pOPL->vTimer1IntCnt -= pOPL->vTimer1;
			if( pOPL->vTimer1IntCnt > 0.250f ) pOPL->vTimer1IntCnt = 0.0f;	// Avoid incremental overflow of counter
			return cTRUE;
		}
	}
	return cFALSE;
}

/*$DEADSERIOUSCLAN$*********************************************************************
* ROUTINE
*  ym3812_CheckTimer2Int( ym3812 *pOPL )
* FUNCTION
*  Check if it is time for an ym3812 Timer A interrupt
* NOTES
****************************************************************************************/
int ym3812_CheckTimer2Int( ym3812 *pOPL )
{
	if( ((~pOPL->nTimerCtrl)&ym3812_TCMASK2) && (pOPL->nTimerCtrl&ym3812_TCST2) )
	{
		if( pOPL->vTimer2IntCnt>pOPL->vTimer2 )
		{
			pOPL->nStatus |= ym3812_STIRQ | ym3812_STFLAG2;
			pOPL->vTimer2IntCnt -= pOPL->vTimer2;
			if( pOPL->vTimer2IntCnt > 0.250f ) pOPL->vTimer2IntCnt = 0.0f;	// Avoid incremental overflow of counter
			return cTRUE;
		}
 	}
	return cFALSE;
}

/*$DEADSERIOUS$*******************************************************************
* ROUTINE
*	void ym3812_TimerOverflow( pOPL, int nTimer )
* AUTHOR
*	Carl-Henrik Skårstedt
* FUNCTION/NOTES
*	Call this every time a timer overflows.
**********************************************************************************/

int ym3812_TimerEvent( ym3812 *pOPL, int nTimer )
{
	switch( nTimer )
	{
		case 1:
			if( (~pOPL->nTimerCtrl) & ym3812_TCMASK1)
			{
				pOPL->nStatus |= ym3812_STFLAG1|ym3812_STIRQ;
				return cTRUE;
			}
			else
			{
				return cFALSE;	// Masking timer 1
			}
		case 2:
			if( (~pOPL->nTimerCtrl) & ym3812_TCMASK2)
			{
				pOPL->nStatus |= ym3812_STFLAG2|ym3812_STIRQ;
				return cTRUE;
			}
			else
			{
				return cFALSE;	// Masking timer 2
			}
	}
	return cFALSE;	// Bad timer
}

/*$DEADSERIOUSCLAN$*********************************************************************
* ROUTINE
*	ym3812_KeyOn( ym3812 *pOPL, nChannel )
* FUNCTION
*	Set Key On State for both slots related to this channel
* NOTES
****************************************************************************************/
void ym3812_KeyOn( ym3812 *pOPL, int nChannel )
{
	int nSlot;

	if( !pOPL->fKeyDown[nChannel] )
	{
		pOPL->fKeyDown[nChannel]=cTRUE;
		for( nSlot=nChannel*2; nSlot<nChannel*2+2; nSlot++ )
		{
			pOPL->vEnvTime[nSlot] = 0.0f;
			pOPL->nEnvState[nSlot] = ADSR_Attack;
		}
	}
}

/*$DEADSERIOUSCLAN$*********************************************************************
* ROUTINE
*  ym3812_KeyOff( ym3812 *pOPL, int nChannel )
* FUNCTION
* NOTES
****************************************************************************************/
void ym3812_KeyOff( ym3812 *pOPL, int nChannel )
{
	pOPL->fKeyDown[nChannel] = cFALSE;
}

/*$DEADSERIOUS$*******************************************************************
* ROUTINE
*	int ym3812_ReadStatus( ym3812 *pOPL )
* AUTHOR
*	Carl-Henrik Skårstedt
* FUNCTION/NOTES
*	Returns the status of interrupts and timer overflow flags.
**********************************************************************************/

int ym3812_ReadStatus( ym3812 *pOPL )
{
	return pOPL->nStatus;
}

/*$DEADSERIOUS$*******************************************************************
* ROUTINE
*	int ym3812_ReadReg( ym3812 *pOPL )
* AUTHOR
*	Carl-Henrik Skårstedt
* FUNCTION/NOTES
*	Reads back register contents.
**********************************************************************************/

int ym3812_ReadReg( ym3812 *pOPL )
{
	return pOPL->aRegArray[pOPL->nReg];
}

/*$DEADSERIOUSCLAN$*********************************************************************
* ROUTINE
*  ym3812_SetReg( ym3812 *pOPL, unsigned char nReg )
* FUNCTION
* NOTES
****************************************************************************************/
void ym3812_SetReg( ym3812 *pOPL, unsigned char nReg )
{
	pOPL->nReg = nReg;
}

/*$DEADSERIOUSCLAN$*********************************************************************
* ROUTINE
*  ym3812_WriteReg( ym3812 *pOPL, unsigned char nData)
* FUNCTION
* NOTES
****************************************************************************************/
void ym3812_WriteReg( ym3812 *pOPL, unsigned char nData)
{
	int 	nReg, nCh, nSlot, nTemp;

	nReg = pOPL->nReg;

	pOPL->aRegArray[nReg] = nData;

	switch( nReg )
	{
		case 0x01:		// TEST - bit 5 means ym3812 mode on, not means ym3526 full compatibility
			pOPL->fWave = nData & 0x20;
			return;
		case 0x02:		// DATA OF TIMER 1
			/* EHC 09/25/98 */
			/* Adjusted timers for higher precision */
			/* Bubble Bobble loses sound after a while otherwise */
			pOPL->vTimer1 = TIME_IN_USEC ((256-nData) * 80) * ( (double)ym3812_StdClock / (double)pOPL->nYM3812Clk );
			if( pOPL->nTimerCtrl&ym3812_TCST1 ) // Change timer interval
			{
				if(pOPL->SetTimer!=NULL) (*pOPL->SetTimer)( 1, pOPL->vTimer1, pOPL, cTRUE );
				if(pOPL->SetTimer!=NULL) (*pOPL->SetTimer)( 1, pOPL->vTimer1, pOPL, cFALSE );
			}
			return;
		case 0x03:		// DATA OF TIMER 2
			/* EHC 09/25/98 */
			/* Adjusted timers for higher precision */
			/* Bubble Bobble loses sound after a while otherwise */
			pOPL->vTimer2 = TIME_IN_USEC ((256-nData) * 320) * ( (double)ym3812_StdClock / (double)pOPL->nYM3812Clk );
			if( pOPL->nTimerCtrl&ym3812_TCST2 ) // Change timer interval
			{
				if(pOPL->SetTimer!=NULL) (*pOPL->SetTimer)( 2, pOPL->vTimer2, pOPL, cTRUE );
				if(pOPL->SetTimer!=NULL) (*pOPL->SetTimer)( 2, pOPL->vTimer2, pOPL, cFALSE );
			}
			return;
		case 0x04:		// IRQ-RESET/CONTROL OF TIMER 1 AND 2
			if( nData & ym3812_TCIRQRES )
			{
				pOPL->nStatus &= ~(ym3812_STIRQ|ym3812_STFLAG1|ym3812_STFLAG2);
				return;
			}

			pOPL->nStatus &= ~(nData & 0x60);

			nTemp = pOPL->nTimerCtrl;
			pOPL->nTimerCtrl = nData;
			if( (nData&(~nTemp)) & ym3812_TCST1 )
			{			// Start new timer if START TIMER1 was set this write.
				if(pOPL->SetTimer!=NULL) (*pOPL->SetTimer)( 1, pOPL->vTimer1, pOPL, cFALSE );
				pOPL->vTimer1IntCnt = 0.0f;	// Set timer to 0
			}
			else if( ((~nData)&nTemp) & ym3812_TCST1 )
			{			// Remove existing timer if START TIMER1 was cleared this write.
				if(pOPL->SetTimer!=NULL) (*pOPL->SetTimer)( 1, pOPL->vTimer1, pOPL, cTRUE );
			}
			if( (nData&(~nTemp)) & ym3812_TCST2 )
			{			// Start new timer if START TIMER2 was set this write.
				if(pOPL->SetTimer!=NULL) (*pOPL->SetTimer)( 2, pOPL->vTimer2, pOPL, cFALSE );
				pOPL->vTimer2IntCnt = 0.0f;	// Set timer to 0
			}
			else if( ((~nData)&nTemp) & ym3812_TCST2 )
			{			// Remove existing timer if START TIMER2 was cleared this write.
				if(pOPL->SetTimer!=NULL) (*pOPL->SetTimer)( 2, pOPL->vTimer2, pOPL, cTRUE );
			}
			return;
		case 0x08:		// CSM SPEECH SYNTHESIS MODE/NOTE SELECT
			pOPL->nCSM = nData;
			return;
		case 0xbd:		// DEPTH(AM/VIB)/RHYTHM(BD, SD, TOM, TC, HH)
			nTemp = pOPL->nDepthRhythm;
			pOPL->nDepthRhythm=nData;		// 0xbd - control of depth and rhythm
			nData &= ~nTemp;
			if( nData&0x10 ) pOPL->nDrumOffs[0] = 0;	// Bass Drum
			if( nData&0x08 ) pOPL->nDrumOffs[1] = 0;	// Snare Drum
			if( nData&0x04 ) pOPL->nDrumOffs[2] = 0;	// Tom tom
			if( nData&0x02 ) pOPL->nDrumOffs[3] = 0;	// Top cymbal
			if( nData&0x01 ) pOPL->nDrumOffs[4] = 0;	// Hihat
			return;
	}
	nCh = nReg & 0x0f;
	if( nCh < 9 )
	{
		switch( nReg & 0xf0 )
		{
			case 0xa0:		// F-NUMBER (LOW 8 BITS) a0-a8
				pOPL->nFNumber[nCh] = (pOPL->nFNumber[nCh] & 0x300) + (nData&0xff);
				return;
			case 0xb0:		// KEY-ON/BLOCK/F-NUMER(LOW 2 BITS) b0-b8
				pOPL->nFNumber[nCh] = (pOPL->nFNumber[nCh] & 0x0ff) + ((nData&0x3)<<8);
				pOPL->nOctave[nCh] = (nData&0x1c)>>2;
				if( nData & 0x20 )
				{
					ym3812_KeyOn( pOPL, nCh );
				}
				else
				{
					ym3812_KeyOff( pOPL, nCh );
				}
				return;
			case 0xc0:		// FEEDBACK/CONNECTION
				pOPL->fConnection[nCh] = nData & 0x01;
				nData >>= 1;
				if( !nData )	// Calculate Feedback RATIO!
				{
					pOPL->nFeedback[nCh] = 0;
				}
				else
				{
					nData &= 7;
					pOPL->nFeedback[nCh] = 15-nData;
				}
				return;
		}
	}
	nSlot = RegSlot_Relation[nReg&0x1f];
	if( nSlot != -1 )
	{
		switch( nReg & 0xe0 )
		{
			case 0x20:		// AM-MOD/VIBRATO/EG-TYP/KEY-SCALE-RATE/MULTIPLE
				pOPL->fAM[nSlot] = nData & 0x80;
				pOPL->fVibrato[nSlot] = nData & 0x40;
				pOPL->fEGTyp[nSlot] = nData & 0x20;
				pOPL->fKSR[nSlot] = nData & 0x10;
				pOPL->nMulti[nSlot] = nData & 0x0f;
				return;
			case 0x40:		// KEY-SCALE LEVEL/TOTAL LEVEL
				pOPL->nTotalLevel[nSlot] = nData&0x3f;
				pOPL->nKSL[nSlot] = (nData&0xc0)>>6;
				return;
			case 0x60:		// ATTACK RATE/DECAY RATE
				if( nData&0xf0 ) pOPL->nAttack[nSlot] = nData >> 4;
				if( nData&0x0f ) pOPL->nDecay[nSlot] = nData & 0x0f;
				return;
			case 0x80:		// SUSTAIN LEVEL/RELEASE RATE
				if( nData&0xf0 ) pOPL->nSustain[nSlot] = (nData >> 4)^0x0f;
				if( nData&0x0f ) pOPL->nRelease[nSlot] = nData & 0x0f;
				return;
			case 0xE0:		// WAVE SELECT
				if( pOPL->fWave )	pOPL->nWave[nSlot] = nData & 0x03;
				else				pOPL->nWave[nSlot] = 0;
				return;
		}
	}
	return; // Writing to unused register - probably a bug
}


int dB2U( int nVal, int *aVol )
{
	if( nVal>=0 )
	{
		if( nVal>(4095<<(SINTABLE_SHIFT+4)) ) nVal = (4095<<(SINTABLE_SHIFT+4));
		return aVol[nVal>>(SINTABLE_SHIFT+4)];
	}
	else
	{
		if( nVal<-(4095<<(SINTABLE_SHIFT+4)) ) nVal = -(4095<<(SINTABLE_SHIFT+4));
		return -aVol[-nVal>>(SINTABLE_SHIFT+4)];
	}
}

/*$DEADSERIOUS$********************************************
* ROUTINE
*	void ym3812_Update_stream( int nChipID, void *pBuffer_in, int nLength )
*
* FUNCTION
*	This routine plays streamed audio rather than a
*	set size buffer. Tailored for Mame streaming sounds.
*
***********************************************************/
void ym3812_Update( ym3812* pOPL, void *pBuffer_in, int nLength )
{
	int16_16	nCurrVol[18];					// Volume for each slot
	int16_16	nNowVol[18];					// What we actually multiply with..
	int			nCurrAdd[18];					// Frequency per channel
	int 		fPlaying[9];					// Is channel X playing??
	int 		nVibMul,nAMAdd; 				// Vibrato and Amplitude modulation numbers
	int 		nRhythm,l,samp,Value,f16Bit;
	int 		nSlot,nSlot2,nOffs,nOffs2,nDiffAdd[5],nAdd[5];
	int 		nSubDiv,nBufSize,nBufferLeft,nCh,nSinValue;
	float		vMulLevel,vTime;				// Temporary max volume level in envelope generator
	int			nElapseTime;
	signed char *pRhythm[5];
	char		*pBuffer;
	short		*pBuffer16;

	if( (nLength == 0)||(pOPL->nReplayFrq == 0) ) return;

// 16bit or 8bit samples?

	f16Bit = pOPL->f16Bit;
	pBuffer = pBuffer_in;
	pBuffer16 = (short*)pBuffer;

	nBufferLeft = nLength;

	nElapseTime = (nLength<<16) / pOPL->nReplayFrq;	// Time is in 16:16 format in seconds.

// Update Timers

#if defined(ym3812_AUTOMATIC) && !defined(MAMEVER)
	ym3812_UpdateTimers( pOPL, ((float)nElapseTime)/65536.0f );
#endif

// Do the subdivision stuff

	for( nSubDiv=0; nSubDiv<pOPL->nSubDivide; nSubDiv++ )
	{
		nBufSize = (nLength / pOPL->nSubDivide) + 1;
		nBufferLeft -= nBufSize;
		if (nBufferLeft < 0) nBufSize += nBufferLeft;
		if( nBufSize == 0 ) return;

// Check how many of the Rhythm sounds are playing..

		nRhythm = 0;
		for( l=0; l<5; l++ )
		{
			if( pOPL->nDrumOffs[l]>=0 )
			{
				if( (pOPL->pDrum[l]!=NULL)&&(pOPL->nDrumOffs[l]<pOPL->nDrumSize[l]) )
				{
					pRhythm[nRhythm] = pOPL->pDrum[l]+pOPL->nDrumOffs[l];
					pOPL->nDrumOffs[l] += (pOPL->nDrumRate[l] * nElapseTime / pOPL->nSubDivide ) >> 16;
					nDiffAdd[nRhythm] = (pOPL->nDrumRate[l]<<8)/(pOPL->nReplayFrq);
					nAdd[nRhythm] = nDiffAdd[nRhythm]>>1;
					nRhythm += 1;
				}
				else
				{
					pOPL->nDrumOffs[l] = -1;
				}
			}
		}

// Get values from and then update Vibrato and AMDepth sinus offsets.

		pOPL->nVibratoOffs += (int)(SINTABLE_SIZE * 6.4 * pOPL->nYM3812Clk / ym3812_StdClock / pOPL->nSubDivide * nElapseTime)>>16;	// Vibrato @ 6.4 Hz (3.6 MHz OPL)
		pOPL->nAMDepthOffs += (int)(SINTABLE_SIZE * 3.7 * pOPL->nYM3812Clk / ym3812_StdClock / pOPL->nSubDivide * nElapseTime)>>16;	// AMDepth @ 3.7 Hz (3.6 MHz OPL)

		// Make vibrato only affect the frequency down. Depth is 7 or 14 cent.
		nVibMul = -(int)((SINTABLE_MAX+ym3812_aSinTable[0][pOPL->nVibratoOffs]) * 0.007f);
		if( pOPL->nDepthRhythm & 0x40 ) nVibMul *= 2;
		if( pOPL->nDepthRhythm & 0x80 )
		{	// 4.8 dB max Amplitude Modulation (48 out of 945 .1 dB)
			nAMAdd = -(SINTABLE_MAX+ym3812_aSinTable[0][pOPL->nAMDepthOffs]) * 48 / (945*(SINTABLE_MAX/256));
		}
		else
		{	// 1.0 dB max Amplitude Modulation (10 out of 945 .1 dB)
			nAMAdd = -(SINTABLE_MAX+ym3812_aSinTable[0][pOPL->nAMDepthOffs]) * 10 / (945*(SINTABLE_MAX/256));
		}
		pOPL->nVibratoOffs &= SINTABLE_SIZE-1;
		pOPL->nAMDepthOffs &= SINTABLE_SIZE-1;

// Calculate frequency for each slot

		for( l=0; l<18; l++ )
		{
// Sometimes there's a pie, sometimes you're safe
			nCurrAdd[l] = ( ym3812_Multi[pOPL->nMulti[l]] * pOPL->nFNumber[l>>1] * pOPL->nYM3812DivClk / pOPL->nReplayFrq ) << ( (SINTABLE_SIZESHIFT-20+7)+pOPL->nOctave[l>>1] );
			if( pOPL->fVibrato[l] ) nCurrAdd[l] += (nCurrAdd[l]*nVibMul)>>SINTABLE_SHIFT;
		}

// Update currently playing envelopes (one per slot)

		for( l=0; l<18 ; l++ )
		{
			if( (l&1)==0 ) fPlaying[l>>1] = cFALSE;
			nNowVol[l].whole = pOPL->nPrevVol[l].whole;
			if( pOPL->nEnvState[l] == ADSR_Silent )
			{
				nCurrVol[l].whole = 0;
				nNowVol[l].whole = 0;
			}
			else
			{
				nCurrVol[l].parts[FRACPART] = 0;
				nCh = l>>1;
				vMulLevel = (float)(0xfc-4*pOPL->nTotalLevel[l]);

				// Emulate KSL
				switch( pOPL->nKSL[l] )
				{
					case 3:
						vMulLevel += pOPL->nOctave[nCh]*(6.0f*256.0f/94.5f);
						break;
					case 2:
						vMulLevel += pOPL->nOctave[nCh]*(1.5f*256.0f/94.5f);
						break;
					case 1:
						vMulLevel += pOPL->nOctave[nCh]*(3.0f*256.0f/94.5f);
						break;
				}

				// Emulate amplitude modulation
				if( pOPL->fAM[l] ) vMulLevel += nAMAdd;

				if( vMulLevel>255.0f ) vMulLevel = 255.0f;
				if( vMulLevel<0.0f ) vMulLevel = 0.0f;

				switch( pOPL->nEnvState[l] )
				{

					case ADSR_Attack:
						vTime = ym3812_aAttackTime[pOPL->nAttack[l]];
						if( pOPL->vEnvTime[l] < vTime )
						{	// The attack is actually linear, so this is not a typo.
							nCurrVol[l].parts[INTPART] = (int)(64 * vMulLevel * pOPL->vEnvTime[l] / vTime);
							break;	// Do not fall through to Decay
						}
						else
						{
							pOPL->vEnvTime[l] -= vTime;
							pOPL->nEnvState[l] = ADSR_Decay;
						}	// Fall through to Decay..
					case ADSR_Decay:
						if( (!pOPL->fEGTyp[l])||(pOPL->fKeyDown[nCh]) )
						{
							vTime = ym3812_aDecayTime[pOPL->nDecay[l]];
							if( pOPL->vEnvTime[l] < vTime )
							{
								nCurrVol[l].parts[INTPART] = (int)(64 * vMulLevel * ((1.0f - pOPL->vEnvTime[l]/vTime) *
									(15 - pOPL->nSustain[l]) + (pOPL->nSustain[l])))>>4;
								break;	// Do not fall through to Sustain/Release
							}
							else
							{
								pOPL->vEnvTime[l] -= vTime;
								pOPL->nEnvState[l] = ADSR_Sustain;
							}
						}
						// Fall through to Sustain/Release..
					case ADSR_Sustain:
						if( (pOPL->fEGTyp[l])&&(pOPL->fKeyDown[nCh]) )
						{	// Wait forever (until KeyOff)
							nCurrVol[l].parts[INTPART] = (int)(64 * vMulLevel*pOPL->nSustain[l])>>4;
							break; // No fall through to release
						}
						else
						{	// No hold state in Sustain (Ignore KeyOff)
							pOPL->vEnvTime[l] = 0.0f;
							pOPL->nEnvState[l] = ADSR_Release;
						}
					case ADSR_Release:
						vTime = ym3812_aDecayTime[pOPL->nRelease[l]];	// Release timing = decay timing
						if( pOPL->vEnvTime[l] < vTime )
						{
							nCurrVol[l].parts[INTPART] = (int)(64 * vMulLevel * (1.0f-pOPL->vEnvTime[l]/vTime)*pOPL->nSustain[l])>>4;
							break;
						}
						else
						{	// Envelop has played and died
							nCurrVol[l].whole = 0;
							pOPL->vEnvTime[l] = 0.0f;
							pOPL->nEnvState[l] = ADSR_Silent;
							break;
						}
				}
				// Update envelop time for _NEXT_ frame
				pOPL->vEnvTime[l] += (float)nElapseTime/65536.0f/(float)pOPL->nSubDivide;
				if( nCurrVol[l].parts[INTPART]>0 )
				{
					fPlaying[nCh] = cTRUE;
					pOPL->nPrevVol[l].whole = nCurrVol[l].whole;
					nCurrVol[l].whole = (nCurrVol[l].whole-nNowVol[l].whole+nBufSize/2)/nBufSize;
				}
			}
		}

		// Generate sample buffer
		for( samp=nBufSize; samp>0; --samp )
		{
			// Calculate one sample from all active FM channels
			Value = 0;

			for( l=8; l>=0; --l )												// Do all channels
			{
				if( fPlaying[l] )												// Is this channel playing at all?
				{
					nSlot = l*2;												// Set Slot A
					nSlot2 = nSlot+1;											// Set Slot B

					if( pOPL->nFeedback[l]==0 ) 								// Fix feedback
					{
						nOffs = (pOPL->nCurrPos[nSlot]/256)&(SINTABLE_SIZE-1);	// No feedback offset
					}
					else
					{
						nOffs = ((pOPL->nCurrPos[nSlot]-(pOPL->nSinValue[nSlot]>>pOPL->nFeedback[l]))/256)&(SINTABLE_SIZE-1);// Add feedback offset
					}
					nSinValue = nNowVol[nSlot].parts[INTPART]*ym3812_aSinTable[pOPL->nWave[nSlot]][nOffs];			// Set old sample value

					pOPL->nSinValue[nSlot] += nSinValue;
					pOPL->nSinValue[nSlot] /= 2;

					nOffs2 = (pOPL->nCurrPos[nSlot2] / 256)&(SINTABLE_SIZE-1);	// Calculate sintable offset this frame Slot B

					if( pOPL->fConnection[l] )									// Connect or Parallell?
					{
						Value += dB2U( nSinValue +
								       nNowVol[nSlot2].parts[INTPART]*ym3812_aSinTable[pOPL->nWave[nSlot2]][nOffs2&(SINTABLE_SIZE-1)],
									   pOPL->aVolumes );
					}
					else
					{
						Value += dB2U( nNowVol[nSlot2].parts[INTPART]*ym3812_aSinTable[pOPL->nWave[nSlot2]][
									   ( nOffs2 + (nSinValue/0x8000) ) & (SINTABLE_SIZE-1)],
									   pOPL->aVolumes );
					}
					pOPL->nCurrPos[nSlot] += nCurrAdd[nSlot];					// Increase sintable offset Slot A
					pOPL->nCurrPos[nSlot2] += nCurrAdd[nSlot2]; 				// Increase sintable offset Slot B

					nNowVol[nSlot].whole += nCurrVol[nSlot].whole;
					nNowVol[nSlot2].whole += nCurrVol[nSlot2].whole;
				}
			}
			for( l=0; l<nRhythm; l++ )
			{
				Value += (*pRhythm[l] * ym3812_StdVolume)>>3;
				pRhythm[l] += nAdd[l]/256;
				nAdd[l] = (nAdd[l]&0xff)+nDiffAdd[l];
			}

			// Put sample value into sample buffer
			if( f16Bit )	*pBuffer16++ = ym3812_Sign16(Value / 2);
			else			*pBuffer++ = ym3812_Sign8(Value / 512);
		}
	}
}

