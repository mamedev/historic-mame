/*
 * $Id: mixdrv.c 1.13 1996/09/08 21:14:54 chasan released $
 *               1.14 1998/10/24 18:20:53 chasan released (Mixer API)
 *
 * Software-based waveform synthesizer emulator driver.
 *
 * Copyright (C) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define __FILTER__

#ifdef __GNUC__
#include <memory.h>
#define cdecl
#endif
#include <string.h>
#include <malloc.h>
#include "audio.h"
#include "drivers.h"


/*
 * Voice control bit fields
 */
#define VOICE_STOP              0x01
#define VOICE_16BITS            0x02
#define VOICE_LOOP              0x04
#define VOICE_BIDILOOP          0x08
#define VOICE_REVERSE           0x10

/*
 * Voice pitch accurary and mixing buffer size
 */
#define ACCURACY                12
#define BUFFERSIZE              512


/*
 * Waveform synthesizer voice structure
 */
typedef struct {
    LPVOID  lpData;
    LONG    dwAccum;
    LONG    dwFrequency;
    LONG    dwLoopStart;
    LONG    dwLoopEnd;
    BYTE    nVolume;
    BYTE    nPanning;
    BYTE    bControl;
    BYTE    bReserved;
} VOICE, *LPVOICE;

/*
 * Low level voice mixing routine prototype
 */
typedef VOID (cdecl* LPFNMIXAUDIO)(LPLONG, UINT, LPVOICE);

/*
 * Waveform synthesizer state structure
 */
static struct {
    VOICE   aVoices[AUDIO_MAX_VOICES];
    UINT    nVoices;
    UINT    wFormat;
    UINT    nSampleRate;
    LONG    dwTimerRate;
    LONG    dwTimerAccum;
    LPBYTE  lpMemory;
    LPFNAUDIOTIMER lpfnAudioTimer;
    LPFNMIXAUDIO lpfnMixAudioProc[2];
} Synth;

LPLONG lpVolumeTable;
LPBYTE lpFilterTable;

/*JB 2000-02-25*/
static signed int MixerValue = 0;
/*JB END*/


static VOID AIAPI UpdateVoices(LPBYTE lpData, UINT nCount);

/* low level resamplation and quantization routines */
#ifdef __ASM__

VOID cdecl QuantAudioData08(LPVOID lpBuffer, LPLONG lpData, UINT nCount);
VOID cdecl QuantAudioData16(LPVOID lpBuffer, LPLONG lpData, UINT nCount);
VOID cdecl MixAudioData08M(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice);
VOID cdecl MixAudioData08S(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice);
VOID cdecl MixAudioData08MI(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice);
VOID cdecl MixAudioData08SI(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice);
VOID cdecl MixAudioData16M(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice);
VOID cdecl MixAudioData16S(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice);
VOID cdecl MixAudioData16MI(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice);
VOID cdecl MixAudioData16SI(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice);

static VOID QuantAudioData(LPVOID lpBuffer, LPLONG lpData, UINT nCount)
{
    if (Synth.wFormat & AUDIO_FORMAT_16BITS)
	QuantAudioData16(lpBuffer, lpData, nCount);
    else
	QuantAudioData08(lpBuffer, lpData, nCount);
}

#else

typedef short SHORT;
typedef SHORT* LPSHORT;

static VOID QuantAudioData(LPVOID lpBuffer, LPLONG lpData, UINT nCount)
{
    LPSHORT lpwBuffer;
    LPBYTE lpbBuffer;
    LONG dwSample;

    if (Synth.wFormat & AUDIO_FORMAT_16BITS) {
	lpwBuffer = (LPSHORT) lpBuffer;
	while (nCount-- > 0) {
	    dwSample = *lpData++;
	    if (dwSample < -32768)
		dwSample = -32768;
	    else if (dwSample > +32767)
		dwSample = +32767;
	    *lpwBuffer++ = (SHORT) dwSample;
	}
    }
    else {
	lpbBuffer = (LPBYTE) lpBuffer;
	while (nCount-- > 0) {
	    dwSample = *lpData++;
	    if (dwSample < -32768)
		dwSample = -32768;
	    else if (dwSample > +32767)
		dwSample = +32767;
	    *lpbBuffer++ = (BYTE) ((dwSample >> 8) + 128);
	}
    }
}

static VOID AIAPI
MixAudioData08M(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
{
    register UINT count;
    register DWORD accum, delta;
    register LPLONG table, buf;
    register LPBYTE data;

    accum = lpVoice->dwAccum;
    delta = lpVoice->dwFrequency;
    table = lpVolumeTable + ((UINT) lpVoice->nVolume << 8);
    data = lpVoice->lpData;
    buf = lpBuffer;
    count = nCount;

    do {
	*buf++ += table[data[accum >> ACCURACY]];
	accum += delta;
    } while (--count != 0);

    lpVoice->dwAccum = accum;
}

#define min(a, b) ((a) < (b) ? (a) : (b))

static VOID AIAPI
MixAudioData08S(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
{
    register UINT a;
    register DWORD accum, delta;
    register LPLONG ltable, rtable, buf;
    register LPBYTE data;

    accum = lpVoice->dwAccum;
    delta = lpVoice->dwFrequency;
    a = ((UINT) lpVoice->nVolume * lpVoice->nPanning) >> 8;
    rtable = lpVolumeTable + (a << 8);
    ltable = lpVolumeTable + ((lpVoice->nVolume - a) << 8);
    data = lpVoice->lpData;
    buf = lpBuffer;

    do {
	a = data[accum >> ACCURACY];
	buf[0] += ltable[a];
	buf[1] += rtable[a];
	accum += delta;
	buf += 2;
    } while (--nCount != 0);

    lpVoice->dwAccum = accum;
}

static VOID AIAPI
MixAudioData08MI(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
{
    register INT a, frac;
    register DWORD accum, delta;
    register LPLONG table, buf;
    register LPBYTE data, ftable;

    /* adaptive oversampling interpolation comparison */
    if (lpVoice->dwFrequency < -(1 << ACCURACY) ||
	lpVoice->dwFrequency > +(1 << ACCURACY)) {
	MixAudioData08M(lpBuffer, nCount, lpVoice);
	return;
    }

    accum = lpVoice->dwAccum;
    delta = lpVoice->dwFrequency;
    table = lpVolumeTable + ((UINT) lpVoice->nVolume << 8);
    data = lpVoice->lpData;
    buf = lpBuffer;

#ifdef __FILTER__
    a = (BYTE) lpVoice->bReserved;
    frac = ((long) delta < 0 ? -delta : +delta) >> (ACCURACY - 5);
    ftable = lpFilterTable + (frac << 8);
    do {
	a = (BYTE)(a + ftable[data[accum >> ACCURACY]] - ftable[a]);
	*buf++ += table[a];
	accum += delta;
    } while (--nCount != 0);
    lpVoice->bReserved = (BYTE) a;
#else
    do {
	register INT b;
	a = (signed char) data[accum >> ACCURACY];
	b = (signed char) data[(accum >> ACCURACY) + 1];
	frac = (accum & ((1 << ACCURACY) - 1)) >> (ACCURACY - 5);
	a = (BYTE)(a + ((frac * (b - a)) >> 5));
	*buf++ += table[a];
	accum += delta;
    } while (--nCount != 0);
#endif

    lpVoice->dwAccum = accum;
}

static VOID AIAPI
MixAudioData08SI(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
{
    register INT a, frac;
    register DWORD accum, delta;
    register LPLONG buf, ltable, rtable;
    register LPBYTE data, ftable;

    /* adaptive oversampling interpolation comparison */
    if (lpVoice->dwFrequency < -(1 << ACCURACY) ||
	lpVoice->dwFrequency > +(1 << ACCURACY)) {
	MixAudioData08S(lpBuffer, nCount, lpVoice);
	return;
    }

    accum = lpVoice->dwAccum;
    delta = lpVoice->dwFrequency;
    a = ((UINT) lpVoice->nVolume * lpVoice->nPanning) >> 8;
    rtable = lpVolumeTable + (a << 8);
    ltable = lpVolumeTable + ((lpVoice->nVolume - a) << 8);
    data = lpVoice->lpData;
    buf = lpBuffer;

#ifdef __FILTER__
    a = (BYTE) lpVoice->bReserved;
    frac = ((long) delta < 0 ? -delta : +delta) >> (ACCURACY - 5);
    ftable = lpFilterTable + (frac << 8);
    do {
	a = (BYTE)(a + ftable[data[accum >> ACCURACY]] - ftable[a]);
	buf[0] += ltable[a];
	buf[1] += rtable[a];
	accum += delta;
	buf += 2;
    } while (--nCount != 0);
    lpVoice->bReserved = (BYTE) a;
#else
    do {
	register INT b;
	a = (signed char) data[accum >> ACCURACY];
	b = (signed char) data[(accum >> ACCURACY) + 1];
	frac = (accum & ((1 << ACCURACY) - 1)) >> (ACCURACY - 5);
	a = (BYTE)(a + ((frac * (b - a)) >> 5));
	buf[0] += ltable[a];
	buf[1] += rtable[a];
	accum += delta;
	buf += 2;
    } while (--nCount != 0);
#endif

    lpVoice->dwAccum = accum;
}

static VOID AIAPI
MixAudioData16M(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
{
    register UINT a, count;
    register DWORD accum, delta;
    register LPLONG table, buf;
    register LPWORD data;

    accum = lpVoice->dwAccum;
    delta = lpVoice->dwFrequency;
    table = lpVolumeTable + ((UINT) lpVoice->nVolume << 8);
    data = lpVoice->lpData;
    buf = lpBuffer;
    count = nCount;

    do {
	a = data[accum >> ACCURACY];
	*buf++ += table[a >> 8];
	accum += delta;
    } while (--count != 0);

    lpVoice->dwAccum = accum;
}

static VOID AIAPI
MixAudioData16S(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
{
    register UINT a;
    register DWORD accum, delta;
    register LPLONG ltable, rtable, buf;
    register LPWORD data;

    accum = lpVoice->dwAccum;
    delta = lpVoice->dwFrequency;
    a = ((UINT) lpVoice->nVolume * lpVoice->nPanning) >> 8;
    rtable = lpVolumeTable + (a << 8);
    ltable = lpVolumeTable + ((lpVoice->nVolume - a) << 8);
    data = lpVoice->lpData;
    buf = lpBuffer;

    do {
	a = data[accum >> ACCURACY];
	buf[0] += ltable[a >> 8];
	buf[1] += rtable[a >> 8];
	accum += delta;
	buf += 2;
    } while (--nCount != 0);

    lpVoice->dwAccum = accum;
}


static VOID AIAPI
MixAudioData16MI(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
{
    register UINT frac, count;
    register DWORD a, b, accum, delta;
    register LPLONG table, buf;
    register LPWORD data;

    /* adaptive oversampling interpolation comparison */
    if (lpVoice->dwFrequency < -(1 << ACCURACY) ||
	lpVoice->dwFrequency > +(1 << ACCURACY)) {
	MixAudioData16M(lpBuffer, nCount, lpVoice);
	return;
    }

    accum = lpVoice->dwAccum;
    delta = lpVoice->dwFrequency;
    table = lpVolumeTable + ((UINT) lpVoice->nVolume << 8);
    data = lpVoice->lpData;
    buf = lpBuffer;
    count = nCount;

    do {
	a = data[accum >> ACCURACY];
	b = data[(accum >> ACCURACY) + 1];
	frac = (accum & ((1 << ACCURACY) - 1)) >> (ACCURACY - 5);
	a = (WORD)((SHORT)a + ((frac * ((SHORT)b - (SHORT)a)) >> 5));
	*buf++ += table[a >> 8];
	accum += delta;
    } while (--count != 0);

    lpVoice->dwAccum = accum;
}

static VOID AIAPI
MixAudioData16SI(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
{
    register UINT frac;
    register DWORD a, b, accum, delta;
    register LPLONG ltable, rtable, buf;
    register LPWORD data;

    /* adaptive oversampling interpolation comparison */
    if (lpVoice->dwFrequency < -(1 << ACCURACY) ||
	lpVoice->dwFrequency > +(1 << ACCURACY)) {
	MixAudioData16S(lpBuffer, nCount, lpVoice);
	return;
    }

    accum = lpVoice->dwAccum;
    delta = lpVoice->dwFrequency;
    a = ((UINT) lpVoice->nVolume * lpVoice->nPanning) >> 8;
    rtable = lpVolumeTable + (a << 8);
    ltable = lpVolumeTable + ((lpVoice->nVolume - a) << 8);
    data = lpVoice->lpData;
    buf = lpBuffer;

    do {
	a = data[accum >> ACCURACY];
	b = data[(accum >> ACCURACY) + 1];
	frac = (accum & ((1 << ACCURACY) - 1)) >> (ACCURACY - 5);
	a = (WORD)((SHORT)a + ((frac * ((int)(SHORT)b - (int)(SHORT)a)) >> 5));
	buf[0] += ltable[a >> 8];
	buf[1] += rtable[a >> 8];
	accum += delta;
	buf += 2;
    } while (--nCount != 0);

    lpVoice->dwAccum = accum;
}

#endif

/*JB 2000-02-25*/
static VOID AIAPI
MixAudioData16MRaw(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
{
    signed int a;

    DWORD accum, delta;
    signed long* buf;
    signed short* data;

    accum = lpVoice->dwAccum;
    delta = lpVoice->dwFrequency;
    data = lpVoice->lpData;
    buf = lpBuffer;


    do {
	a = data[accum >> ACCURACY];
	a = (a*MixerValue)>>8; //since MixerValue is in 0-256 range
	buf[0] += a;
	accum += delta;
        buf++;
    } while (--nCount != 0);

    lpVoice->dwAccum = accum;
}

static VOID AIAPI
MixAudioData16SRaw(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
{
    signed int a;
    DWORD accum, delta;
    signed long* buf;
    signed short* data;

    accum = lpVoice->dwAccum;
    delta = lpVoice->dwFrequency;
    data = lpVoice->lpData;
    buf = lpBuffer;

    if (lpVoice->nPanning >= 0x80)
	buf++; //set pointer to right channel

    do {
	a = data[accum >> ACCURACY];
	a = (a*MixerValue)>>8; //since MixerValue is in 0-256 range
	buf[0] += a;
	//buf[1] += 0; //skip the other channel
	accum += delta;
	buf += 2;
    } while (--nCount != 0);

    lpVoice->dwAccum = accum;
}
/*JB END*/


static VOID MixAudioData(LPLONG lpBuffer, UINT nCount, LPVOICE lpVoice)
{
    UINT nSize;

    if (Synth.wFormat & AUDIO_FORMAT_STEREO)
	nCount >>= 1;

    while (nCount > 0 && !(lpVoice->bControl & VOICE_STOP)) {
	/* check boundary conditions */
	if (lpVoice->bControl & VOICE_REVERSE) {
	    if (lpVoice->dwAccum < lpVoice->dwLoopStart) {
		if (lpVoice->bControl & VOICE_BIDILOOP) {
		    lpVoice->dwAccum = lpVoice->dwLoopStart +
			(lpVoice->dwLoopStart - lpVoice->dwAccum);
		    lpVoice->bControl ^= VOICE_REVERSE;
		    continue;
		}
		else if (lpVoice->bControl & VOICE_LOOP) {
		    lpVoice->dwAccum = lpVoice->dwLoopEnd -
			(lpVoice->dwLoopStart - lpVoice->dwAccum);
		    continue;
		}
		else {
		    lpVoice->bControl |= VOICE_STOP;
		    break;
		}
	    }
	}
	else {
	    if (lpVoice->dwAccum > lpVoice->dwLoopEnd) {
		if (lpVoice->bControl & VOICE_BIDILOOP) {
		    lpVoice->dwAccum = lpVoice->dwLoopEnd -
			(lpVoice->dwAccum - lpVoice->dwLoopEnd);
		    lpVoice->bControl ^= VOICE_REVERSE;
		    continue;
		}
		else if (lpVoice->bControl & VOICE_LOOP) {
		    lpVoice->dwAccum = lpVoice->dwLoopStart +
			(lpVoice->dwAccum - lpVoice->dwLoopEnd);
		    continue;
		}
		else {
		    lpVoice->bControl |= VOICE_STOP;
		    break;
		}
	    }
	}

	/* check for overflow and clip if necessary */
	nSize = nCount;
	if (lpVoice->bControl & VOICE_REVERSE) {
	    if (nSize * lpVoice->dwFrequency >
		lpVoice->dwAccum - lpVoice->dwLoopStart)
		nSize = (lpVoice->dwAccum - lpVoice->dwLoopStart +
			 lpVoice->dwFrequency) / lpVoice->dwFrequency;
	}
	else {
	    if (nSize * lpVoice->dwFrequency >
		lpVoice->dwLoopEnd - lpVoice->dwAccum)
		nSize = (lpVoice->dwLoopEnd - lpVoice->dwAccum +
			 lpVoice->dwFrequency) / lpVoice->dwFrequency;
	}

	if (lpVoice->bControl & VOICE_REVERSE)
	    lpVoice->dwFrequency = -lpVoice->dwFrequency;

	/* mixes chunk of data in a burst mode */
	if (lpVoice->bControl & VOICE_16BITS) {
	    Synth.lpfnMixAudioProc[1] (lpBuffer, nSize, lpVoice);
	}
	else {
	    Synth.lpfnMixAudioProc[0] (lpBuffer, nSize, lpVoice);
	}

	if (lpVoice->bControl & VOICE_REVERSE)
	    lpVoice->dwFrequency = -lpVoice->dwFrequency;

	/* update mixing buffer address and counter */
	lpBuffer += nSize;
	if (Synth.wFormat & AUDIO_FORMAT_STEREO)
	    lpBuffer += nSize;
	nCount -= nSize;
    }
}

static VOID MixAudioVoices(LPLONG lpBuffer, UINT nCount)
{
    UINT nVoice, nSize;

    while (nCount > 0) {
	nSize = nCount;
	if (Synth.lpfnAudioTimer != NULL) {
	    nSize = (Synth.dwTimerRate - Synth.dwTimerAccum + 64L) & ~63L;
	    if (nSize > nCount)
		nSize = nCount;
	    if ((Synth.dwTimerAccum += nSize) >= Synth.dwTimerRate) {
		Synth.dwTimerAccum -= Synth.dwTimerRate;
		Synth.lpfnAudioTimer();
	    }
	}
	for (nVoice = 0; nVoice < Synth.nVoices; nVoice++) {
	    MixAudioData(lpBuffer, nSize, &Synth.aVoices[nVoice]);
	}
	lpBuffer += nSize;
	nCount -= nSize;
    }
}


/*
 * High level waveform synthesizer interface
 */
static UINT AIAPI GetAudioCaps(LPAUDIOCAPS lpCaps)
{
    memset(lpCaps, 0, sizeof(AUDIOCAPS));
    return AUDIO_ERROR_NOTSUPPORTED;
}

static UINT AIAPI PingAudio(VOID)
{
    return AUDIO_ERROR_NOTSUPPORTED;
}

static UINT AIAPI OpenAudio(LPAUDIOINFO lpInfo)
{
    memset(&Synth, 0, sizeof(Synth));
    Synth.wFormat = lpInfo->wFormat;
    Synth.nSampleRate = lpInfo->nSampleRate;
    if (Synth.wFormat & AUDIO_FORMAT_FILTER) {
	Synth.lpfnMixAudioProc[0] = (Synth.wFormat & AUDIO_FORMAT_STEREO) ?
	    MixAudioData08SI : MixAudioData08MI;
	Synth.lpfnMixAudioProc[1] = (Synth.wFormat & AUDIO_FORMAT_STEREO) ?
	    MixAudioData16SI : MixAudioData16MI;
    }
    else {
	Synth.lpfnMixAudioProc[0] = (Synth.wFormat & AUDIO_FORMAT_STEREO) ?
	    MixAudioData08S : MixAudioData08M;
/*JB 2000-02-25*/
	if (Synth.wFormat & AUDIO_FORMAT_RAW_SAMPLE)
	{
		/*printf("SEAL mixdrv.c: Synth.wFormat |= FORMAT_RAW_SAMPLE\n");*/
		Synth.lpfnMixAudioProc[1] = (Synth.wFormat & AUDIO_FORMAT_STEREO) ?
		    MixAudioData16SRaw : MixAudioData16MRaw;
	}
	else
	{
		Synth.lpfnMixAudioProc[1] = (Synth.wFormat & AUDIO_FORMAT_STEREO) ?
		    MixAudioData16S : MixAudioData16M;
	}
/*JB END*/
    }

    /* allocate volume (0-64) and filter (0-31) table */
    Synth.lpMemory = malloc(sizeof(LONG) * 65 * 256 +
			    sizeof(BYTE) * 32 * 256 + 1023);
    if (Synth.lpMemory != NULL) {
	lpVolumeTable = (LPLONG) (((DWORD) Synth.lpMemory + 1023) & ~1023);
	lpFilterTable = (LPBYTE) (lpVolumeTable + 65 * 256);
	ASetAudioMixerValue(AUDIO_MIXER_MASTER_VOLUME, 96);
	ASetAudioCallback(UpdateVoices);
	return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_NOMEMORY;
}

static UINT AIAPI CloseAudio(VOID)
{
    if (Synth.lpMemory != NULL)
	free(Synth.lpMemory);
    memset(&Synth, 0, sizeof(Synth));
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioMixerValue(UINT nChannel, UINT nMixerValue)
{
    LPBYTE lpFilter;
    LPLONG lpVolume;
    UINT nVolume, nSample;
    LONG dwAccum, dwDelta;

    if (Synth.lpMemory == NULL)
	return AUDIO_ERROR_NOTSUPPORTED;

    /* master volume must be less than 256 units */
    if (nChannel != AUDIO_MIXER_MASTER_VOLUME || nMixerValue > 256)
	return AUDIO_ERROR_INVALPARAM;

    /* half dynamic range for mono output */
    if (!(Synth.wFormat & AUDIO_FORMAT_STEREO))
	nMixerValue >>= 1;

/*JB 2000-02-25*/
    MixerValue = nMixerValue;
/*JB END*/

    /* build volume table (0-64) */
    lpVolume = lpVolumeTable;
    dwDelta = 0;
    for (nVolume = 0; nVolume <= 64; nVolume++, dwDelta += nMixerValue) {
	dwAccum = 0;
	for (nSample = 0; nSample < 128; nSample++, dwAccum += dwDelta)
	    *lpVolume++ = dwAccum >> 4;
	dwAccum = -dwAccum;
	for (nSample = 0; nSample < 128; nSample++, dwAccum += dwDelta)
	    *lpVolume++ = dwAccum >> 4;
    }

#ifdef __FILTER__
    /* build lowpass filter table (0-31) */
    lpFilter = lpFilterTable;
    for (nVolume = 0; nVolume < 32; nVolume++) {
	dwAccum = 0;
	for (nSample = 0; nSample < 128; nSample++, dwAccum += nVolume)
	    *lpFilter++ = dwAccum >> 5;
	dwAccum = -dwAccum;
	for (nSample = 0; nSample < 128; nSample++, dwAccum += nVolume)
	    *lpFilter++ = dwAccum >> 5;
    }
#endif

    return AUDIO_ERROR_NONE;
}

static UINT AIAPI OpenVoices(UINT nVoices)
{
    UINT nVoice;

    /*
     * Initialize waveform synthesizer structure for playback
     */
    if (nVoices >= 1 && nVoices <= AUDIO_MAX_VOICES) {
	Synth.nVoices = nVoices;
	for (nVoice = 0; nVoice < Synth.nVoices; nVoice++)
	    Synth.aVoices[nVoice].bControl = VOICE_STOP;
	return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALPARAM;
}

static UINT AIAPI CloseVoices(VOID)
{
    UINT nVoice;

    memset(Synth.aVoices, 0, sizeof(Synth.aVoices));
    for (nVoice = 0; nVoice < AUDIO_MAX_VOICES; nVoice++)
	Synth.aVoices[nVoice].bControl = VOICE_STOP;
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudio(VOID)
{
    return AUDIO_ERROR_NONE;
}

static LONG AIAPI GetAudioDataAvail(VOID)
{
    return 0;
}

static UINT AIAPI CreateAudioData(LPAUDIOWAVE lpWave)
{
    if (lpWave != NULL) {
	lpWave->dwHandle = (DWORD) lpWave->lpData;
	return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI DestroyAudioData(LPAUDIOWAVE lpWave)
{
    if (lpWave != NULL && lpWave->dwHandle != 0) {
	return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI WriteAudioData(LPAUDIOWAVE lpWave, DWORD dwOffset, UINT nCount)
{
    if (lpWave != NULL && lpWave->dwHandle != 0) {
	/* anticlick removal work around */
	if (lpWave->wFormat & AUDIO_FORMAT_LOOP) {
	    *(LPDWORD) (lpWave->dwHandle + lpWave->dwLoopEnd) =
		*(LPDWORD) (lpWave->dwHandle + lpWave->dwLoopStart);
	}
	else if (dwOffset + nCount >= lpWave->dwLength) {
	    *(LPDWORD) (lpWave->dwHandle + lpWave->dwLength) = 0;
	}
	return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI PrimeVoice(UINT nVoice, LPAUDIOWAVE lpWave)
{
    LPVOICE lpVoice;

    if (nVoice < Synth.nVoices && lpWave != NULL && lpWave->dwHandle != 0) {
	lpVoice = &Synth.aVoices[nVoice];
	lpVoice->lpData = (LPVOID) lpWave->dwHandle;
	lpVoice->bControl = VOICE_STOP;
	lpVoice->dwAccum = 0;
	if (lpWave->wFormat & (AUDIO_FORMAT_LOOP | AUDIO_FORMAT_BIDILOOP)) {
	    lpVoice->dwLoopStart = lpWave->dwLoopStart;
	    lpVoice->dwLoopEnd = lpWave->dwLoopEnd;
	    lpVoice->bControl |= VOICE_LOOP;
	    if (lpWave->wFormat & AUDIO_FORMAT_BIDILOOP)
		lpVoice->bControl |= VOICE_BIDILOOP;
	}
	else {
	    lpVoice->dwLoopStart = lpWave->dwLength;
	    lpVoice->dwLoopEnd = lpWave->dwLength;
	}
	if (lpWave->wFormat & AUDIO_FORMAT_16BITS) {
	    lpVoice->dwLoopStart >>= 1;
	    lpVoice->dwLoopEnd >>= 1;
	    lpVoice->bControl |= VOICE_16BITS;
	}
	lpVoice->dwAccum <<= ACCURACY;
	lpVoice->dwLoopStart <<= ACCURACY;
	lpVoice->dwLoopEnd <<= ACCURACY;
	return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI StartVoice(UINT nVoice)
{
    if (nVoice < Synth.nVoices) {
	Synth.aVoices[nVoice].bControl &= ~VOICE_STOP;
	return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI StopVoice(UINT nVoice)
{
    if (nVoice < Synth.nVoices) {
	Synth.aVoices[nVoice].bControl |= VOICE_STOP;
	return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;

}

static UINT AIAPI SetVoicePosition(UINT nVoice, LONG dwPosition)
{
    if (nVoice < Synth.nVoices) {
	dwPosition <<= ACCURACY;
	if (dwPosition >= 0 && dwPosition < Synth.aVoices[nVoice].dwLoopEnd) {
	    Synth.aVoices[nVoice].dwAccum = dwPosition;
	    return AUDIO_ERROR_NONE;
	}
	return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI SetVoiceFrequency(UINT nVoice, LONG dwFrequency)
{
    if (nVoice < Synth.nVoices) {
	if (dwFrequency >= AUDIO_MIN_FREQUENCY &&
	    dwFrequency <= AUDIO_MAX_FREQUENCY) {
	    Synth.aVoices[nVoice].dwFrequency = ((dwFrequency << ACCURACY) +
						 (Synth.nSampleRate >> 1)) / Synth.nSampleRate;
	    return AUDIO_ERROR_NONE;
	}
	return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI SetVoiceVolume(UINT nVoice, UINT nVolume)
{
    if (nVoice < Synth.nVoices) {
	if (nVolume <= AUDIO_MAX_VOLUME) {
	    Synth.aVoices[nVoice].nVolume = nVolume >> 1;
	    return AUDIO_ERROR_NONE;
	}
	return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI SetVoicePanning(UINT nVoice, UINT nPanning)
{
    if (nVoice < Synth.nVoices) {
	if (nPanning <= AUDIO_MAX_PANNING) {
	    Synth.aVoices[nVoice].nPanning = nPanning;
	    return AUDIO_ERROR_NONE;
	}
	return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoicePosition(UINT nVoice, LPLONG lpdwPosition)
{
    if (nVoice < Synth.nVoices) {
	if (lpdwPosition != NULL) {
	    *lpdwPosition = Synth.aVoices[nVoice].dwAccum >> ACCURACY;
	    return AUDIO_ERROR_NONE;
	}
	return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoiceFrequency(UINT nVoice, LPLONG lpdwFrequency)
{
    if (nVoice < Synth.nVoices) {
	if (lpdwFrequency != NULL) {
	    *lpdwFrequency = (Synth.aVoices[nVoice].dwFrequency *
			      Synth.nSampleRate) >> ACCURACY;
	    return AUDIO_ERROR_NONE;
	}
	return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoiceVolume(UINT nVoice, LPUINT lpnVolume)
{
    if (nVoice < Synth.nVoices) {
	if (lpnVolume != NULL) {
	    *lpnVolume = Synth.aVoices[nVoice].nVolume << 1;
	    return AUDIO_ERROR_NONE;
	}
	return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoicePanning(UINT nVoice, LPUINT lpnPanning)
{
    if (nVoice < Synth.nVoices) {
	if (lpnPanning != NULL) {
	    *lpnPanning = Synth.aVoices[nVoice].nPanning;
	    return AUDIO_ERROR_NONE;
	}
	return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoiceStatus(UINT nVoice, LPBOOL lpnStatus)
{
    if (nVoice < Synth.nVoices) {
	if (lpnStatus != NULL) {
	    *lpnStatus = (Synth.aVoices[nVoice].bControl & VOICE_STOP) != 0;
	    return AUDIO_ERROR_NONE;
	}
	return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI SetAudioTimerProc(LPFNAUDIOTIMER lpfnAudioTimer)
{
    Synth.lpfnAudioTimer = lpfnAudioTimer;
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioTimerRate(UINT nBPM)
{
    if (nBPM >= 0x20 && nBPM <= 0xFF) {
	Synth.dwTimerRate = Synth.nSampleRate;
	if (Synth.wFormat & AUDIO_FORMAT_STEREO)
	    Synth.dwTimerRate <<= 1;
	Synth.dwTimerRate = (5 * Synth.dwTimerRate) / (2 * nBPM);
	return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALPARAM;
}

static VOID AIAPI UpdateVoices(LPBYTE lpData, UINT nCount)
{
    static LONG aBuffer[BUFFERSIZE];
    UINT nSamples;

    if (Synth.wFormat & AUDIO_FORMAT_16BITS)
	nCount >>= 1;
    while (nCount > 0) {
	if ((nSamples = nCount) > BUFFERSIZE)
	    nSamples = BUFFERSIZE;
	memset(aBuffer, 0, nSamples << 2);
	MixAudioVoices(aBuffer, nSamples);
	QuantAudioData(lpData, aBuffer, nSamples);
	lpData += nSamples << ((Synth.wFormat & AUDIO_FORMAT_16BITS) != 0);
	nCount -= nSamples;
    }
}


/*
 * Waveform synthesizer public interface
 */
AUDIOSYNTHDRIVER EmuSynthDriver =
{
    GetAudioCaps, PingAudio, OpenAudio, CloseAudio,
    UpdateAudio, OpenVoices, CloseVoices,
    SetAudioTimerProc, SetAudioTimerRate, SetAudioMixerValue,
    GetAudioDataAvail, CreateAudioData, DestroyAudioData,
    WriteAudioData, PrimeVoice, StartVoice, StopVoice,
    SetVoicePosition, SetVoiceFrequency, SetVoiceVolume,
    SetVoicePanning, GetVoicePosition, GetVoiceFrequency,
    GetVoiceVolume, GetVoicePanning, GetVoiceStatus
};
