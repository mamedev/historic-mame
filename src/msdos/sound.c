#include "driver.h"
#include <dos.h>
#include <conio.h>
#include <time.h>
#include <audio.h>
#include <allegro.h>
#include "ym2203.h"

/* cut down Allegro size */
DECLARE_DIGI_DRIVER_LIST()
DECLARE_MIDI_DRIVER_LIST()


/* audio related stuff */
#define NUMVOICES 16
HAC hVoice[NUMVOICES];
LPAUDIOWAVE lpWave[NUMVOICES];
int Volumi[NUMVOICES];
int MasterVolume = 100;
unsigned char No_FM = 1;
unsigned char No_OPL = 1;
unsigned char RegistersYM[264*5];  /* MAX 5 YM-2203 */
int nominal_sample_rate;
int soundcard,usefm;

AUDIOINFO info;
AUDIOCAPS caps;


int msdos_init_seal (void)
{
	if (AInitialize() == AUDIO_ERROR_NONE)
		return 0;
	else
		return 1;
}

int msdos_init_sound(void)
{
	int i;
	char *blaster_env;

	No_OPL = No_FM = !usefm;

	/* Ask the user if no soundcard was chosen */
	if (soundcard == -1)
	{
		unsigned int k;

		printf("\nSelect the audio device:\n"
		       "If you have an AWE 32, choose Sound Blaster for a more faithful emulation)\n");

		for (k = 0;k < AGetAudioNumDevs();k++)
		{
			if (AGetAudioDevCaps(k,&caps) == AUDIO_ERROR_NONE)
				printf("  %2d. %s\n",k,caps.szProductName);
		}
		printf("\n");

		if (k < 10)
		{
			i = getch();
			soundcard = i - '0';
		}
		else
			scanf("%d",&soundcard);
	}

	/* initialize SEAL audio library */
	if (soundcard == 0)     /* silence */
	{
		/* update the Machine structure to show that sound is disabled */
		Machine->sample_rate = 0;
		return 0;
	}

	/* open audio device */
	/*                              info.nDeviceId = AUDIO_DEVICE_MAPPER;*/
	info.nDeviceId = soundcard;
	/* always use 16 bit mixing if possible - better quality and same speed of 8 bit */
	info.wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_MONO;
	info.nSampleRate = Machine->sample_rate;
	if (AOpenAudio(&info) != AUDIO_ERROR_NONE)
	{
		printf("audio initialization failed\n");
		return 1;
	}

	AGetAudioDevCaps(info.nDeviceId,&caps);
	if (errorlog) fprintf(errorlog,"Using %s at %d-bit %s %u Hz\n",
			caps.szProductName,
			info.wFormat & AUDIO_FORMAT_16BITS ? 16 : 8,
			info.wFormat & AUDIO_FORMAT_STEREO ? "stereo" : "mono",
			info.nSampleRate);

	/* open and allocate voices, allocate waveforms */
	if (AOpenVoices(NUMVOICES) != AUDIO_ERROR_NONE)
	{
		printf("voices initialization failed\n");
		return 1;
	}

	for (i = 0; i < NUMVOICES; i++)
	{
		if (ACreateAudioVoice(&hVoice[i]) != AUDIO_ERROR_NONE)
		{
			printf("voice #%d creation failed\n",i);
			return 1;
		}

		ASetVoicePanning(hVoice[i],128);

		lpWave[i] = 0;
	}

	/* update the Machine structure to reflect the actual sample rate */
	Machine->sample_rate = info.nSampleRate;

	if (errorlog) fprintf(errorlog,"set sample rate: %d\n",Machine->sample_rate);

	{
		uclock_t a,b;
		LONG start,end;


		if ((lpWave[0] = (LPAUDIOWAVE)malloc(sizeof(AUDIOWAVE))) == 0)
			return 1;

		lpWave[0]->wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO;
		lpWave[0]->nSampleRate = Machine->sample_rate;
		lpWave[0]->dwLength = 3*Machine->sample_rate;
		lpWave[0]->dwLoopStart = 0;
		lpWave[0]->dwLoopEnd = 3*Machine->sample_rate;
		if (ACreateAudioData(lpWave[0]) != AUDIO_ERROR_NONE)
		{
			free(lpWave[0]);
			lpWave[0] = 0;

			return 1;
		}

		memset(lpWave[0]->lpData,0,3*Machine->sample_rate);
		/* upload the data to the audio DRAM local memory */
		AWriteAudioData(lpWave[0],0,3*Machine->sample_rate);
		APrimeVoice(hVoice[0],lpWave[0]);
		ASetVoiceFrequency(hVoice[0],Machine->sample_rate);
		ASetVoiceVolume(hVoice[0],0);
		AStartVoice(hVoice[0]);

		a = uclock();
		/* wait some time to let everything stabilize */
		do
		{
			osd_update_audio();
			b = uclock();
		} while (b-a < UCLOCKS_PER_SEC/10);

		a = uclock();
		AGetVoicePosition(hVoice[0],&start);
		do
		{
			osd_update_audio();
			b = uclock();
		} while (b-a < UCLOCKS_PER_SEC);
		AGetVoicePosition(hVoice[0],&end);

		nominal_sample_rate = Machine->sample_rate;
		Machine->sample_rate = end - start;
		if (errorlog) fprintf(errorlog,"actual sample rate: %d\n",Machine->sample_rate);

		AStopVoice(hVoice[0]);
		ADestroyAudioData(lpWave[0]);
		free(lpWave[0]);
		lpWave[0] = 0;
	}


	{
		int totalsound = 0;


		while (Machine->drv->sound[totalsound].sound_type != 0 && totalsound < MAX_SOUND)
		{
			if (Machine->drv->sound[totalsound].sound_type == SOUND_YM2203 &&
					((struct YM2203interface *)Machine->drv->sound[totalsound].sound_interface)->handler[0])
				No_OPL = No_FM = 1;      /* YM2203 must trigger interrupts, cannot emulate it with the OPL */

			if (Machine->drv->sound[totalsound].sound_type == SOUND_YM3812)
                        {
                                No_FM = 1;      /* can't use the OPL chip to emulate the YM2203 */
                                No_OPL = 0;     /* but the OPL chip is used */
                        }

                        totalsound++;
		}

		osd_update_audio();
	}

	if (!No_FM) {
		/* Get Soundblaster base address from environment variabler BLASTER   */
		/* Soundblaster OPL base port, at some compatibles this must be 0x388 */

		if(!getenv("BLASTER"))
		{
			printf("\nBLASTER variable not found, disabling fm sound!\n");
                        No_OPL = No_FM = 1;
		}
		else
		{
			blaster_env = getenv("BLASTER");
			BaseSb = i = 0;
			while ((blaster_env[i] & 0x5f) != 0x41) i++;        /* Look for 'A' char */
			while (blaster_env[++i] != 0x20) {
				BaseSb = (BaseSb << 4) + (blaster_env[i]-0x30);
			}

			DelayReg=4;   /* Delay after an OPL register write increase it to avoid problems ,but you'll lose speed */
			DelayData=7;  /* same as above but after an OPL data write this usually is greater than above */
			InitYM();     /* inits OPL in mode OPL3 and 4ops per channel,also reset YM2203 registers */
		}
	}
	return 0;
}

void msdos_shutdown_sound(void)
{
	if (Machine->sample_rate != 0)
	{
		int n;

		if (!No_OPL)
		   InitOpl();  /* Do this only before quiting , or some cards will make noise during playing */
				   /* It resets entire OPL registers to zero */

		/* stop and release voices */
		for (n = 0; n < NUMVOICES; n++)
		{
			AStopVoice(hVoice[n]);
			ADestroyAudioVoice(hVoice[n]);
			if (lpWave[n])
			{
				ADestroyAudioData(lpWave[n]);
				free(lpWave[n]);
				lpWave[n] = 0;
			}
		}
		ACloseVoices();
		ACloseAudio();
	}
}



void osd_update_audio(void)
{
	if (Machine->sample_rate == 0) return;

	AUpdateAudio();
}



static void playsample(int channel,signed char *data,int len,int freq,int volume,int loop,int bits)
{
	if (Machine->sample_rate == 0 || channel >= NUMVOICES) return;

	if (lpWave[channel] && lpWave[channel]->dwLength != len)
	{
		AStopVoice(hVoice[channel]);
		ADestroyAudioData(lpWave[channel]);
		free(lpWave[channel]);
		lpWave[channel] = 0;
	}

	if (lpWave[channel] == 0)
	{
		if ((lpWave[channel] = (LPAUDIOWAVE)malloc(sizeof(AUDIOWAVE))) == 0)
			return;

		if (loop) lpWave[channel]->wFormat = (bits == 8 ? AUDIO_FORMAT_8BITS : AUDIO_FORMAT_16BITS)
				| AUDIO_FORMAT_MONO | AUDIO_FORMAT_LOOP;
		else lpWave[channel]->wFormat = (bits == 8 ? AUDIO_FORMAT_8BITS : AUDIO_FORMAT_16BITS)
				| AUDIO_FORMAT_MONO;
		lpWave[channel]->nSampleRate = nominal_sample_rate;
		lpWave[channel]->dwLength = len;
		lpWave[channel]->dwLoopStart = 0;
		lpWave[channel]->dwLoopEnd = len;
		if (ACreateAudioData(lpWave[channel]) != AUDIO_ERROR_NONE)
		{
			free(lpWave[channel]);
			lpWave[channel] = 0;

			return;
		}
	}
	else
	{
		if (loop) lpWave[channel]->wFormat = (bits == 8 ? AUDIO_FORMAT_8BITS : AUDIO_FORMAT_16BITS)
				| AUDIO_FORMAT_MONO | AUDIO_FORMAT_LOOP;
		else lpWave[channel]->wFormat = (bits == 8 ? AUDIO_FORMAT_8BITS : AUDIO_FORMAT_16BITS)
				| AUDIO_FORMAT_MONO;
	}

	memcpy(lpWave[channel]->lpData,data,len);
	/* upload the data to the audio DRAM local memory */
	AWriteAudioData(lpWave[channel],0,len);
	APrimeVoice(hVoice[channel],lpWave[channel]);
	/* need to cast to double because freq*nominal_sample_rate can exceed the size of an int */
	ASetVoiceFrequency(hVoice[channel],(double)freq*nominal_sample_rate/Machine->sample_rate);
	ASetVoiceVolume(hVoice[channel],MasterVolume*volume/400);
	AStartVoice(hVoice[channel]);

	Volumi[channel] = volume/4;
}

void osd_play_sample(int channel,signed char *data,int len,int freq,int volume,int loop)
{
	playsample(channel,data,len,freq,volume,loop,8);
}

void osd_play_sample_16(int channel,signed short *data,int len,int freq,int volume,int loop)
{
	playsample(channel,(signed char *)data,len,freq,volume,loop,16);
}



static void playstreamedsample(int channel,signed char *data,int len,int freq,int volume,int bits)
{
	static int playing[NUMVOICES];
	static int c[NUMVOICES];


	if (Machine->sample_rate == 0 || channel >= NUMVOICES) return;

	if (!playing[channel])
	{
		if (lpWave[channel])
		{
			AStopVoice(hVoice[channel]);
			ADestroyAudioData(lpWave[channel]);
			free(lpWave[channel]);
			lpWave[channel] = 0;
		}

		if ((lpWave[channel] = (LPAUDIOWAVE)malloc(sizeof(AUDIOWAVE))) == 0)
			return;

		lpWave[channel]->wFormat = (bits == 8 ? AUDIO_FORMAT_8BITS : AUDIO_FORMAT_16BITS)
				| AUDIO_FORMAT_MONO | AUDIO_FORMAT_LOOP;
		lpWave[channel]->nSampleRate = nominal_sample_rate;
		lpWave[channel]->dwLength = 3*len;
		lpWave[channel]->dwLoopStart = 0;
		lpWave[channel]->dwLoopEnd = 3*len;
		if (ACreateAudioData(lpWave[channel]) != AUDIO_ERROR_NONE)
		{
			free(lpWave[channel]);
			lpWave[channel] = 0;

			return;
		}

		memset(lpWave[channel]->lpData,0,3*len);
		memcpy(lpWave[channel]->lpData,data,len);
		/* upload the data to the audio DRAM local memory */
		AWriteAudioData(lpWave[channel],0,3*len);
		APrimeVoice(hVoice[channel],lpWave[channel]);
	/* need to cast to double because freq*nominal_sample_rate can exceed the size of an int */
		ASetVoiceFrequency(hVoice[channel],(double)freq*nominal_sample_rate/Machine->sample_rate);
		ASetVoiceVolume(hVoice[channel],MasterVolume*volume/400);
		AStartVoice(hVoice[channel]);
		playing[channel] = 1;
		c[channel] = 1;
	}
	else
	{
		LONG pos;
		extern int throttle;


		if (throttle)   /* sync with audio only when speed throttling is not turned off */
		{
			for(;;)
			{
				AGetVoicePosition(hVoice[channel],&pos);
				if (c[channel] == 0 && pos >= len) break;
				if (c[channel] == 1 && (pos < len || pos >= 2*len)) break;
				if (c[channel] == 2 && pos < 2*len) break;
				osd_update_audio();
			}
		}

		memcpy(&lpWave[channel]->lpData[len * c[channel]],data,len);
		AWriteAudioData(lpWave[channel],len*c[channel],len);
		c[channel]++;
		if (c[channel] == 3) c[channel] = 0;
	}

	Volumi[channel] = volume/4;
}

void osd_play_streamed_sample(int channel,signed char *data,int len,int freq,int volume)
{
	playstreamedsample(channel,data,len,freq,volume,8);
}

void osd_play_streamed_sample_16(int channel,signed short *data,int len,int freq,int volume)
{
	playstreamedsample(channel,(signed char *)data,len,freq,volume,16);
}



void osd_adjust_sample(int channel,int freq,int volume)
{
	if (Machine->sample_rate == 0 || channel >= NUMVOICES) return;

	Volumi[channel] = volume/4;
	/* need to cast to double because freq*nominal_sample_rate can exceed the size of an int */
	if (freq != -1)
		ASetVoiceFrequency(hVoice[channel],(double)freq*nominal_sample_rate/Machine->sample_rate);
	if (volume != -1)
		ASetVoiceVolume(hVoice[channel],MasterVolume*volume/400);
}



void osd_stop_sample(int channel)
{
	if (Machine->sample_rate == 0 || channel >= NUMVOICES) return;

	AStopVoice(hVoice[channel]);
}


void osd_restart_sample(int channel)
{
	if (Machine->sample_rate == 0 || channel >= NUMVOICES) return;

	AStartVoice(hVoice[channel]);
}


int osd_get_sample_status(int channel)
{
	int stopped=0;
	if (Machine->sample_rate == 0 || channel >= NUMVOICES) return -1;

	AGetVoiceStatus(hVoice[channel], &stopped);
	return stopped;
}


void osd_ym2203_write(int n, int r, int v)
{
      if (No_FM) return;
      else {
	YMNumber = n;
	RegistersYM[r+264*n] = v;
	if (r == 0x28) SlotCh();
	return;
      }
}


void osd_ym2203_update(void)
{
      if (No_FM) return;
      YM2203();  /* this is absolutely necesary */
}


void osd_set_mastervolume(int volume)
{
	int channel;

	MasterVolume = volume;
	for (channel=0; channel < NUMVOICES; channel++) {
	  ASetVoiceVolume(hVoice[channel],MasterVolume*Volumi[channel]/100);
	}
}



void osd_ym3812_control(int reg)
{
	outportb(0x388,reg);
}

void osd_ym3812_write(int data)
{
	outportb(0x389,data);
}
