/*
 * $Id: sbdrv.c 1.7 1996/08/05 18:51:19 chasan released $
 *              1.8 1998/12/24 00:16:00 chasan
 *
 * Sound Blaster series DSP audio drivers.
 *
 * Copyright (C) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <string.h>
#include "audio.h"
#include "drivers.h"
#include "msdos.h"

#define DEBUG(code)

/*
 * Sound Blaster DSP and Mixer register offsets
 */
#define MIXER_ADDR              0x04    /* mixer index address register */
#define MIXER_DATA              0x05    /* mixer indexed data register */
#define DSP_RESET               0x06    /* master reset register */
#define DSP_READ_DATA           0x0A    /* read data register */
#define DSP_READ_READY          0x0E    /* data available register */
#define DSP_WRITE_DATA          0x0C    /* write data register */
#define DSP_WRITE_BUSY          0x0C    /* write status register */
#define DSP_DMA_ACK_8BIT        0x0E    /* 8 bit DMA acknowledge */
#define DSP_DMA_ACK_16BIT       0x0F    /* 16 bit DMA acknowledge */

/*
 * Sound Blaster 1.0 (DSP 1.x, 2.0, 2.01, 3.xx, 4.xx) command defines
 */
#define DSP_PIO_DAC_8BIT        0x10    /* direct 8 bit output */
#define DSP_PIO_ADC_8BIT        0x20    /* direct 8 bit input */
#define DSP_DMA_TIME_CONST      0x40    /* transfer time constant */
#define DSP_DMA_DAC_8BIT        0x14    /* 8 bit DMA output transfer */
#define DSP_DMA_ADC_8BIT        0x24    /* 8 bit DMA input transfer */
#define DSP_DMA_PAUSE_8BIT      0xD0    /* pause 8 bit DMA transfer */
#define DSP_DMA_CONTINUE_8BIT   0xD4    /* continue 8 bit DMA transfer */
#define DSP_SPEAKER_ON          0xD1    /* turn speaker on */
#define DSP_SPEAKER_OFF         0xD3    /* turn speaker off */
#define DSP_GET_VERSION         0xE1    /* get DSP version */

/*
 * Sound Blaster 1.5 (DSP 2.0, 2.01, 3.xx, 4.xx) command defines
 */
#define DSP_DMA_BLOCK_SIZE      0x48    /* transfer block size */
#define DSP_DMA_DAC_AI_8BIT     0x1C    /* 8 bit low-speed A/I output */
#define DSP_DMA_ADC_AI_8BIT     0x2C    /* 8 bit low-speed A/I input */

/*
 * Sound Blaster 2.0 (DSP 2.01, 3.xx) command defines
 */
#define DSP_DMA_DAC_HS_8BIT     0x91    /* 8 bit high-speed output */
#define DSP_DMA_ADC_HS_8BIT     0x99    /* 8 bit high-speed input */
#define DSP_DMA_DAC_AI_HS_8BIT  0x90    /* 8 bit high-speed A/I output */
#define DSP_DMA_ADC_AI_HS_8BIT  0x98    /* 8 bit high-speed A/I input */

/*
 * Sound Blaster Pro (DSP 3.xx) command defines
 */
#define DSP_DMA_ADC_MONO        0xA0    /* 8 bit mono input mode */
#define DSP_DMA_ADC_STEREO      0xA8    /* 8 bit stereo input mode */

/*
 * Sound Blaster 16 (DSP 4.xx) command defines
 */
#define DSP_DMA_DAC_RATE        0x41    /* set output sample rate */
#define DSP_DMA_ADC_RATE        0x42    /* set input sample rate */
#define DSP_DMA_START_16BIT     0xB0    /* start 16 bit DMA transfer */
#define DSP_DMA_START_8BIT      0xC0    /* start 8 bit DMA transfer */
#define   B_DSP_DMA_DAC_MODE    0x06    /* start DAC output mode */
#define   B_DSP_DMA_ADC_MODE    0x0E    /* start ADC input mode */
#define DSP_DMA_STOP_16BIT      0xD9    /* stop 16 bit DMA transfer */
#define DSP_DMA_STOP_8BIT       0xDA    /* stop 8 bit DMA transfer */
#define DSP_DMA_PAUSE_16BIT     0xD5    /* pause 16 bit DMA transfer */
#define DSP_DMA_CONTINUE_16BIT  0xD6    /* continue 16 bit DMA transfer */

/*
 * Sound Blaster Pro mixer indirect registers
 */
#define MIXER_RESET             0x00    /* mixer reset register */
#define MIXER_INPUT_CONTROL     0x0C    /* input control register */
#define MIXER_OUTPUT_CONTROL    0x0E    /* output control register */
#define MIXER_MASTER_VOLUME     0x22    /* master volume register */
#define MIXER_VOICE_VOLUME      0x04    /* voice volume register */
#define MIXER_MIDI_VOLUME       0x26    /* MIDI volume register */
#define MIXER_CD_VOLUME         0x28    /* CD volume register */
#define MIXER_LINE_VOLUME       0x2E    /* line volume register */
#define MIXER_MIC_MIXING        0x0A    /* mic volume register */
#define MIXER_IRQ_LINE          0x80    /* IRQ line register */
#define MIXER_DMA_CHANNEL       0x81    /* DMA channel register */
#define MIXER_IRQ_STATUS        0x82    /* IRQ status register */

/*
 * Sound Blaster 16 mixer indirect registers
 */
#define MIXER_MASTER_LEFT       0x30    /* master left volume register */
#define MIXER_MASTER_RIGHT      0x31    /* master right volume register */
#define MIXER_VOICE_LEFT        0x32    /* voice left volume register */
#define MIXER_VOICE_RIGHT       0x33    /* voice right volume register */
#define MIXER_MIDI_LEFT         0x34    /* MIDI left volume register */
#define MIXER_MIDI_RIGHT        0x35    /* MIDI right volume register */
#define MIXER_CD_LEFT           0x36    /* CD left volume register */
#define MIXER_CD_RIGHT          0x37    /* CD right volume register */
#define MIXER_LINE_LEFT         0x38    /* line left volume register */
#define MIXER_LINE_RIGHT        0x39    /* line right volume register */
#define MIXER_MIC_VOLUME        0x3A    /* mic input volume register */
#define MIXER_SPKR_VOLUME       0x3B    /* speaker volume register */
#define MIXER_OUT_CONTROL       0x3C    /* Output Line/CD/MIC control */
#define MIXER_INPUT_LEFT        0x3D    /* input left control bits */
#define MIXER_INPUT_RIGHT       0x3E    /* input right control bits */
#define MIXER_INPUT_GAIN        0x3F    /* input gain level */
#define MIXER_OUT_GAIN_LEFT     0x41    /* Output gain left level */
#define MIXER_OUT_GAIN_RIGHT    0x42    /* Output gain right level */
#define MIXER_INPUT_AUTO_GAIN   0x43    /* input auto gain control */
#define MIXER_TREBLE_LEFT       0x44    /* Treble left level */
#define MIXER_TREBLE_RIGHT      0x45    /* Treble right level */
#define MIXER_BASS_LEFT         0x46    /* Bass left level */
#define MIXER_BASS_RIGHT        0x47    /* Bass right level */

/*
 * Input source and filter select register bit fields (MIXER_INPUT_CONTROL)
 */
#define INPUT_SOURCE_MIC        0x00    /* select mic input */
#define INPUT_SOURCE_CD         0x02    /* select CD input */
#define INPUT_SOURCE_LINE       0x06    /* select line input */
#define INPUT_LOW_FILTER        0x00    /* select low-pass filter */
#define INPUT_HIGH_FILTER       0x08    /* select high-pass filter */
#define INPUT_NO_FILTER         0x20    /* disable filters */

/*
 * Output voice feature settings register bit fields (MIXER_OUTPUT_CONTROL)
 */
#define OUTPUT_DISABLE_DNFI     0x20    /* disable output filter */
#define OUTPUT_ENABLE_VSTC      0x02    /* enable stereo mode */

/*
 * Mixer output control register bit fields (MIXER_OUT_CONTROL)
 */
#define MUTE_LINE_LEFT          0x10    /* disable left line-out */
#define MUTE_LINE_RIGHT         0x08    /* disable right line-out */
#define MUTE_CD_LEFT            0x04    /* disable left CD output */
#define MUTE_CD_RIGHT           0x02    /* disable right CD output */
#define MUTE_MICROPHONE         0x01    /* disable MIC output */

/*
 * Start DMA DAC transfer format bit fields (DSP_DMA_START_8/16BIT)
 */
#define B_DSP_DMA_UNSIGNED      0x00    /* select unsigned samples */
#define B_DSP_DMA_SIGNED        0x10    /* select signed samples */
#define B_DSP_DMA_MONO          0x00    /* select mono output */
#define B_DSP_DMA_STEREO        0x20    /* select stereo output */

/*
 * Timeout and DMA buffer size defines
 */
#define TIMEOUT                 200000  /* number of times to wait for */
#define BUFFERSIZE              50      /* buffer length in milliseconds */
#define BUFFRAGSIZE             32      /* buffer fragment size in BYTEs */


/*
 * Sound Blaster configuration structure
 */
static struct {
    WORD    wId;                        /* audio device indentifier */
    WORD    wFormat;                    /* playback encoding format */
    WORD    nSampleRate;                /* sampling frequency */
    WORD    wPort;                      /* DSP base port address */
    BYTE    nIrqLine;                   /* interrupt line */
    BYTE    nDmaChannel;                /* output DMA channel */
    BYTE    nLowDmaChannel;             /* 8-bit DMA channel */
    BYTE    nHighDmaChannel;            /* 16-bit DMA channel */
    LPBYTE  lpBuffer;                   /* DMA buffer address */
    UINT    nBufferSize;                /* DMA buffer length */
    UINT    nPosition;                  /* DMA buffer playing position */
    LPFNAUDIOWAVE lpfnAudioWave;        /* user callback routine */
} SB;


/*
 * Sound Blaster DSP & Mixer programming routines
 */
static VOID DSPPortB(BYTE bData)
{
    UINT n;

    for (n = 0; n < TIMEOUT; n++)
        if (!(INB(SB.wPort + DSP_WRITE_BUSY) & 0x80))
            break;
    OUTB(SB.wPort + DSP_WRITE_DATA, bData);
}

static BYTE DSPPortRB(VOID)
{
    UINT n;

    for (n = 0; n < TIMEOUT; n++)
        if (INB(SB.wPort + DSP_READ_READY) & 0x80)
            break;
    return INB(SB.wPort + DSP_READ_DATA);
}

static VOID DSPMixerB(BYTE nIndex, BYTE bData)
{
    OUTB(SB.wPort + MIXER_ADDR, nIndex);
    OUTB(SB.wPort + MIXER_DATA, bData);
}

static BYTE DSPMixerRB(BYTE nIndex)
{
    OUTB(SB.wPort + MIXER_ADDR, nIndex);
    return INB(SB.wPort + MIXER_DATA);
}

static WORD DSPGetVersion(VOID)
{
    WORD nMinor, nMajor;

    DSPPortB(DSP_GET_VERSION);
    nMajor = DSPPortRB();
    nMinor = DSPPortRB();
    return MAKEWORD(nMinor, nMajor);
}

static VOID DSPReset(VOID)
{
    UINT n;

    OUTB(SB.wPort + DSP_RESET, 1);

    /* wait 3 microseconds */
    for (n = 0; n < 32; n++)
        INB(SB.wPort + DSP_RESET);

    OUTB(SB.wPort + DSP_RESET, 0);
}

static BOOL DSPProbe(VOID)
{
    WORD nVersion;
    UINT n;

    DEBUG(printf("DSPProbe: reset DSP processor\n"));

    /* reset the DSP device */
    for (n = 0; n < 8; n++) {
        DSPReset();
        if (DSPPortRB() == 0xAA)
            break;
    }
    if (n >= 8)
        return AUDIO_ERROR_NODEVICE;

    DEBUG(printf("DSPProbe: read DSP version\n"));

    /* get the DSP version */
    nVersion = DSPGetVersion();

    DEBUG(printf("DSPProbe: DSP version 0x%03x\n", nVersion));

    /* [1998/12/24] don't use anything higher than specified by the user */
    if (nVersion >= 0x400) {
        if (SB.wId >= AUDIO_PRODUCT_SB16)
            SB.wId = AUDIO_PRODUCT_SB16;
    }
    else if (nVersion >= 0x300) {
        if (SB.wId >= AUDIO_PRODUCT_SBPRO)
            SB.wId = AUDIO_PRODUCT_SBPRO;
    }
    else if (nVersion >= 0x201) {
        if (SB.wId >= AUDIO_PRODUCT_SB20)
            SB.wId = AUDIO_PRODUCT_SB20;
    }
    else if (nVersion >= 0x200) {
        if (SB.wId >= AUDIO_PRODUCT_SB15)
            SB.wId = AUDIO_PRODUCT_SB15;
    }
    else {
        SB.wId = AUDIO_PRODUCT_SB;
    }
    return AUDIO_ERROR_NONE;
}

static VOID AIAPI DSPInterruptHandler(VOID)
{
    if (SB.wId == AUDIO_PRODUCT_SB16) {
        /*
         * Acknowledge 8/16 bit mono/stereo high speed
         * autoinit DMA transfer for SB16 (DSP 4.x) cards.
         */
        INB(SB.wPort + (SB.wFormat & AUDIO_FORMAT_16BITS ?
			DSP_DMA_ACK_16BIT : DSP_DMA_ACK_8BIT));
    }
    else if (SB.wId != AUDIO_PRODUCT_SB) {
        /*
         * Acknowledge 8 bit mono/stereo low/high speed autoinit
         * DMA transfer for SB Pro (DSP 3.x), SB 2.0 (DSP 2.01)
         * and SB 1.5 (DSP 2.0) cards.
         */
        INB(SB.wPort + DSP_DMA_ACK_8BIT);
    }
    else {
        /*
         * Acknowledge and restart 8 bit mono low speed
         * oneshot DMA transfer for SB 1.0 (DSP 1.x) cards.
         */
        INB(SB.wPort + DSP_DMA_ACK_8BIT);
        DSPPortB(DSP_DMA_DAC_8BIT);
        DSPPortB(0xF0);
        DSPPortB(0xFF);
    }
}

static VOID DSPStartPlayback(VOID)
{
    DWORD dwBytesPerSecond;

	DEBUG(printf("DSPStartPlayback: reset DSP processor\n"));

    /* reset the DSP processor */
    DSPReset();

    DEBUG(printf("DSPStartPlayback: set DMA %d and IRQ %d resources\n", SB.nDmaChannel, SB.nIrqLine));

    /* setup the DMA channel parameters */
    DosSetupChannel(SB.nDmaChannel, DMA_WRITE | DMA_AUTOINIT, 0);

    /* setup our IRQ interrupt handler */
    DosSetVectorHandler(SB.nIrqLine, DSPInterruptHandler);

    DEBUG(printf("DSPStartPlayback: turn on speaker\n"));

    /* turn on the DSP speaker */
    DSPPortB(DSP_SPEAKER_ON);

    /* [1998/12/04] turn off filter and enable/disable SBPro stereo mode */
    if (SB.wId == AUDIO_PRODUCT_SBPRO) {
        DEBUG(printf("DSPStartPlayback: setup SBPro output control\n"));
        DSPMixerB(MIXER_OUTPUT_CONTROL, DSPMixerRB(MIXER_OUTPUT_CONTROL) |
		  OUTPUT_DISABLE_DNFI |
		  (SB.wFormat & AUDIO_FORMAT_STEREO ? OUTPUT_ENABLE_VSTC : 0));
    }

    DEBUG(printf("DSPStartPlayback: set sample rate\n"));

    /* set DSP output playback rate */
    if (SB.wId == AUDIO_PRODUCT_SB16) {
        /* set output sample rate for SB16 cards */
        DSPPortB(DSP_DMA_DAC_RATE);
        DSPPortB(HIBYTE(SB.nSampleRate));
        DSPPortB(LOBYTE(SB.nSampleRate));
    }
    else {
        /* set input/output sample rate for SB/SB20/SBPro cards */
        dwBytesPerSecond = SB.nSampleRate;
        if (SB.wFormat & AUDIO_FORMAT_STEREO)
            dwBytesPerSecond <<= 1;
        DSPPortB(DSP_DMA_TIME_CONST);
        DSPPortB((65536 - (256000000L / dwBytesPerSecond)) >> 8);
    }

    DEBUG(printf("DSPStartPlayback: start playback transfer\n"));

    /* start DMA playback transfer */
    if (SB.wId == AUDIO_PRODUCT_SB) {
        /*
         * Start 8 bit mono low speed oneshot DMA output
         * transfer for SB 1.0 (DSP 1.x) cards.
         */
        DSPPortB(DSP_DMA_DAC_8BIT);
        DSPPortB(0xF0);
        DSPPortB(0xFF);
    }
    else if (SB.wId == AUDIO_PRODUCT_SB15) {
        /*
         * Start 8 bit mono low speed autoinit DMA output
         * transfer for SB 1.5 (DSP 2.0) cards.
         */
        DSPPortB(DSP_DMA_BLOCK_SIZE);
        DSPPortB(0xF0);
        DSPPortB(0xFF);
        DSPPortB(DSP_DMA_DAC_AI_8BIT);
    }
    else if (SB.wId == AUDIO_PRODUCT_SB20) {
        /*
         * Start 8 bit mono high speed autoinit DMA transfer
         * output transfer for SB 2.0 (DSP 2.01) cards.
         */
        DSPPortB(DSP_DMA_BLOCK_SIZE);
        DSPPortB(0xF0);
        DSPPortB(0xFF);
        DSPPortB(DSP_DMA_DAC_AI_HS_8BIT);
    }
    else if (SB.wId == AUDIO_PRODUCT_SBPRO) {
        /*
         * Start 8 bit mono/stereo high speed autoinit
         * DMA transfer for SB Pro (DSP 3.x) cards.
         */
        DSPPortB(DSP_DMA_BLOCK_SIZE);
        DSPPortB(0xF0);
        DSPPortB(0xFF);
        DSPPortB(DSP_DMA_DAC_AI_HS_8BIT);
    }
    else {
        /*
         * Start 8/16 bit mono/stereo high speed autoinit
         * DMA transfer for SB16 (DSP 4.x) cards.
         */
        if (SB.wFormat & AUDIO_FORMAT_16BITS) {
            DSPPortB(DSP_DMA_START_16BIT | B_DSP_DMA_DAC_MODE);
            DSPPortB(B_DSP_DMA_SIGNED | (SB.wFormat & AUDIO_FORMAT_STEREO ?
					 B_DSP_DMA_STEREO : B_DSP_DMA_MONO));
            DSPPortB(0xF0);
            DSPPortB(0xFF);
        }
        else {
            DSPPortB(DSP_DMA_START_8BIT | B_DSP_DMA_DAC_MODE);
            DSPPortB(B_DSP_DMA_UNSIGNED | (SB.wFormat & AUDIO_FORMAT_STEREO ?
					   B_DSP_DMA_STEREO : B_DSP_DMA_MONO));
            DSPPortB(0xF0);
            DSPPortB(0xFF);
        }
    }
}

static VOID DSPStopPlayback(VOID)
{
    DEBUG(printf("DSPStopPlayback: reset DSP processor\n"));

    /* reset the DSP processor */
    DSPReset();

    DEBUG(printf("DSPStopPlayback: turn off speaker\n"));

    /* turn off DSP speaker */
    DSPPortB(DSP_SPEAKER_OFF);

    /* turn off the stereo flag for SBPro cards */
    if (SB.wId == AUDIO_PRODUCT_SBPRO) {
        DSPMixerB(MIXER_OUTPUT_CONTROL,
		  DSPMixerRB(MIXER_OUTPUT_CONTROL) & ~OUTPUT_ENABLE_VSTC);
    }

    DEBUG(printf("DSPStopPlayback: restore DMA %d and IRQ %d resources\n", SB.nDmaChannel, SB.nIrqLine));

    /* restore the interrupt handler */
    DosSetVectorHandler(SB.nIrqLine, NULL);

    /* reset the DMA channel parameters */
    DosDisableChannel(SB.nDmaChannel);
}

static UINT DSPInitAudio(VOID)
{

#define CLIP(nRate, nMinRate, nMaxRate)         \
        (nRate < nMinRate ? nMinRate :          \
        (nRate > nMaxRate ? nMaxRate : nRate))  \

    DEBUG(printf("DSPInitAudio: probe SB hardware at port 0x%03x\n", SB.wPort));

    /*
     * Probe if there is a SB present and detect the DSP version.
     */
    if (DSPProbe())
        return AUDIO_ERROR_NODEVICE;

    DEBUG(printf("DSPInitAudio: check format and sample rate\n"));

    /*
     * Check playback encoding format and sampling frequency
     */
    if (SB.wId == AUDIO_PRODUCT_SB) {
        SB.nSampleRate = CLIP(SB.nSampleRate, 4000, 22050);
        SB.wFormat &= ~(AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO);
    }
    else if (SB.wId == AUDIO_PRODUCT_SB15) {
        SB.nSampleRate = CLIP(SB.nSampleRate, 5000, 22050);
        SB.wFormat &= ~(AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO);
    }
    else if (SB.wId == AUDIO_PRODUCT_SB20) {
        SB.nSampleRate = CLIP(SB.nSampleRate, 5000, 44100);
        SB.wFormat &= ~(AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO);
    }
    else if (SB.wId == AUDIO_PRODUCT_SBPRO) {
        if (SB.wFormat & AUDIO_FORMAT_STEREO) {
            SB.nSampleRate = CLIP(SB.nSampleRate, 5000, 22050);
        }
        else {
            SB.nSampleRate = CLIP(SB.nSampleRate, 5000, 44100);
        }
        SB.wFormat &= ~AUDIO_FORMAT_16BITS;
    }
    else {
        SB.nSampleRate = CLIP(SB.nSampleRate, 5000, 44100);
    }
    if (SB.wId != AUDIO_PRODUCT_SB16) {
        SB.nSampleRate = (65536 - (256000000L / SB.nSampleRate)) >> 8;
        SB.nSampleRate = 1000000L / (256 - SB.nSampleRate);
    }
    SB.nDmaChannel = (SB.wFormat & AUDIO_FORMAT_16BITS ?
		      SB.nHighDmaChannel : SB.nLowDmaChannel);

    DEBUG(printf("DSPInitAudio: DMA channel %d selected\n", SB.nDmaChannel));

    return AUDIO_ERROR_NONE;
}

static VOID DSPDoneAudio(VOID)
{
    DEBUG(printf("DSPDoneAudio: reset DSP processor\n"));

    DSPReset();
}


/*
 * Sound Blaster audio driver API interface
 */
static UINT AIAPI GetAudioCaps(LPAUDIOCAPS lpCaps)
{
    static AUDIOCAPS Caps =
    {
        AUDIO_PRODUCT_SB, "Sound Blaster",
        AUDIO_FORMAT_1M08 |
        AUDIO_FORMAT_2M08
    };
    static AUDIOCAPS Caps15 =
    {
        AUDIO_PRODUCT_SB15, "Sound Blaster 1.5",
        AUDIO_FORMAT_1M08 |
        AUDIO_FORMAT_2M08
    };
    static AUDIOCAPS Caps20 =
    {
        AUDIO_PRODUCT_SB20, "Sound Blaster 2.0",
        AUDIO_FORMAT_1M08 |
        AUDIO_FORMAT_2M08 |
        AUDIO_FORMAT_4M08
    };
    static AUDIOCAPS CapsPro =
    {
        AUDIO_PRODUCT_SBPRO, "Sound Blaster Pro",
        AUDIO_FORMAT_1M08 | AUDIO_FORMAT_1S08 |
        AUDIO_FORMAT_2M08 | AUDIO_FORMAT_2S08 |
        AUDIO_FORMAT_4M08
    };
    static AUDIOCAPS Caps16 =
    {
        AUDIO_PRODUCT_SB16, "Sound Blaster 16",
        AUDIO_FORMAT_1M08 | AUDIO_FORMAT_1S08 |
        AUDIO_FORMAT_1M16 | AUDIO_FORMAT_1S16 |
        AUDIO_FORMAT_2M08 | AUDIO_FORMAT_2S08 |
        AUDIO_FORMAT_2M16 | AUDIO_FORMAT_2S16 |
        AUDIO_FORMAT_4M08 | AUDIO_FORMAT_4S08 |
        AUDIO_FORMAT_4M16 | AUDIO_FORMAT_4S16
    };

    switch (SB.wId) {
    case AUDIO_PRODUCT_SB16:
        memcpy(lpCaps, &Caps16, sizeof(AUDIOCAPS));
        break;
    case AUDIO_PRODUCT_SBPRO:
        memcpy(lpCaps, &CapsPro, sizeof(AUDIOCAPS));
        break;
    case AUDIO_PRODUCT_SB20:
        memcpy(lpCaps, &Caps20, sizeof(AUDIOCAPS));
        break;
    case AUDIO_PRODUCT_SB15:
        memcpy(lpCaps, &Caps15, sizeof(AUDIOCAPS));
        break;
    default:
        memcpy(lpCaps, &Caps, sizeof(AUDIOCAPS));
        break;
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI PingAudio(VOID)
{
    LPSTR lpszBlaster;
    UINT nChar;

    DEBUG(printf("SBPingAudio: fetch BLASTER variable\n"));

    SB.wId = AUDIO_PRODUCT_SB16;
    SB.wPort = 0x220;
    SB.nIrqLine = 5;
    SB.nLowDmaChannel = 1;
    SB.nHighDmaChannel = 5;
    if ((lpszBlaster = DosGetEnvironment("BLASTER")) != NULL) {
        nChar = DosParseString(lpszBlaster, TOKEN_CHAR);
        while (nChar != 0) {
            switch (nChar) {
            case 'A':
            case 'a':
                SB.wPort = DosParseString(NULL, TOKEN_HEX);
                break;
            case 'I':
            case 'i':
                SB.nIrqLine = DosParseString(NULL, TOKEN_DEC);
                break;
            case 'D':
            case 'd':
                SB.nLowDmaChannel = DosParseString(NULL, TOKEN_DEC);
                break;
            case 'H':
            case 'h':
                SB.nHighDmaChannel = DosParseString(NULL, TOKEN_DEC);
                break;
            case 'T':
            case 't':
                switch (DosParseString(NULL, TOKEN_DEC)) {
                case 1: SB.wId = AUDIO_PRODUCT_SB; break;
                case 2: SB.wId = AUDIO_PRODUCT_SB15; break; /*[1998/12/24]*/
                case 3: SB.wId = AUDIO_PRODUCT_SB20; break;
                case 4: SB.wId = AUDIO_PRODUCT_SBPRO; break;
                case 5: SB.wId = AUDIO_PRODUCT_SBPRO; break;
                case 6: SB.wId = AUDIO_PRODUCT_SB16; break;
                }
                break;
            }
            nChar = DosParseString(NULL, TOKEN_CHAR);
        }

        DEBUG(printf("SBPingAudio: probe SB hardware at Port 0x%03x, IRQ %d, DMA %d/%d\n", SB.wPort, SB.nIrqLine, SB.nLowDmaChannel, SB.nHighDmaChannel));

        return DSPProbe();
    }
    return AUDIO_ERROR_NODEVICE;
}

static UINT AIAPI OpenAudio(LPAUDIOINFO lpInfo)
{
    DWORD dwBytesPerSecond;

    DEBUG(printf("SBOpenAudio: initialize SB driver\n"));

    /*
     * Initialize the SB audio driver for playback
     */
    memset(&SB, 0, sizeof(SB));
    SB.wFormat = lpInfo->wFormat;
    SB.nSampleRate = lpInfo->nSampleRate;
    if (PingAudio())
        return AUDIO_ERROR_NODEVICE;
    if (DSPInitAudio())
        return AUDIO_ERROR_NODEVICE;

    DEBUG(printf("SBOpenAudio: allocate DMA buffer\n"));

    /*
     * Allocate DMA channel buffer and start playback transfers
     */
    dwBytesPerSecond = SB.nSampleRate;
    if (SB.wFormat & AUDIO_FORMAT_16BITS)
        dwBytesPerSecond <<= 1;
    if (SB.wFormat & AUDIO_FORMAT_STEREO)
        dwBytesPerSecond <<= 1;
    SB.nBufferSize = dwBytesPerSecond / (1000 / BUFFERSIZE);
    SB.nBufferSize = (SB.nBufferSize + BUFFRAGSIZE) & -BUFFRAGSIZE;
    if (DosAllocChannel(SB.nDmaChannel, SB.nBufferSize)) {
        DSPDoneAudio();
        return AUDIO_ERROR_NOMEMORY;
    }
    if ((SB.lpBuffer = DosLockChannel(SB.nDmaChannel)) != NULL) {
        memset(SB.lpBuffer, SB.wFormat & AUDIO_FORMAT_16BITS ?
	       0x00 : 0x80, SB.nBufferSize);
        DosUnlockChannel(SB.nDmaChannel);
    }

    DEBUG(printf("SBOpenAudio: start playback\n"));

    DSPStartPlayback();

    /* refresh caller's encoding format and sampling frequency */
    lpInfo->wFormat = SB.wFormat;
    lpInfo->nSampleRate = SB.nSampleRate;
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI CloseAudio(VOID)
{
    DEBUG(printf("SBCloseAudio: shutdown playback and release resources\n"));

    /*
     * Stop DMA playback transfers and reset the DSP processor
     */
    DSPStopPlayback();
    DosFreeChannel(SB.nDmaChannel);
    DSPDoneAudio();
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudio(UINT nFrames)
{
    int nBlockSize, nSize;

    if (SB.wFormat & AUDIO_FORMAT_16BITS) nFrames <<= 1;
    if (SB.wFormat & AUDIO_FORMAT_STEREO) nFrames <<= 1;
    if (nFrames <= 0 || nFrames > SB.nBufferSize)
        nFrames = SB.nBufferSize;

    if ((SB.lpBuffer = DosLockChannel(SB.nDmaChannel)) != NULL) {
        nBlockSize = SB.nBufferSize - SB.nPosition -
            DosGetChannelCount(SB.nDmaChannel);
        if (nBlockSize < 0)
            nBlockSize += SB.nBufferSize;

        if (nBlockSize > nFrames)
            nBlockSize = nFrames;

        nBlockSize &= -BUFFRAGSIZE;
        while (nBlockSize != 0) {
            nSize = SB.nBufferSize - SB.nPosition;
            if (nSize > nBlockSize)
                nSize = nBlockSize;
            if (SB.lpfnAudioWave != NULL) {
                SB.lpfnAudioWave(&SB.lpBuffer[SB.nPosition], nSize);
            }
            else {
                memset(&SB.lpBuffer[SB.nPosition],
		       SB.wFormat & AUDIO_FORMAT_16BITS ?
		       0x00 : 0x80, nSize);
            }
            if ((SB.nPosition += nSize) >= SB.nBufferSize)
                SB.nPosition -= SB.nBufferSize;
            nBlockSize -= nSize;
        }
        DosUnlockChannel(SB.nDmaChannel);
    }
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioCallback(LPFNAUDIOWAVE lpfnAudioWave)
{
    SB.lpfnAudioWave = lpfnAudioWave;
    return AUDIO_ERROR_NONE;
}


/*
 * Sound Blaster drivers public interface
 */
AUDIOWAVEDRIVER SoundBlasterWaveDriver =
{
    GetAudioCaps, PingAudio, OpenAudio, CloseAudio,
    UpdateAudio, SetAudioCallback
};

AUDIODRIVER SoundBlasterDriver =
{
    &SoundBlasterWaveDriver, NULL
};
