#define __INLINE__ static __inline__	/* keep allegro.h happy */
#include <allegro.h>
#undef __INLINE__
#include "driver.h"
#include <dos.h>
#include <conio.h>
#include <time.h>
#include <audio.h>

/* cut down Allegro size */
DECLARE_DIGI_DRIVER_LIST()
DECLARE_MIDI_DRIVER_LIST()


/* audio related stuff */
#define NUMVOICES 16
HAC hVoice[NUMVOICES];
LPAUDIOWAVE lpWave[NUMVOICES];
static int used_3812;
int nominal_sample_rate;
int soundcard,usestereo;

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

	/* Ask the user if no soundcard was chosen */
	if (soundcard == -1)
	{
		unsigned int k;

		printf("\nSelect the audio device:\n");

		for (k = 0;k < AGetAudioNumDevs();k++)
		{
			/* don't show the AWE32, it's too slow, users must choose Sound Blaster */
			if (AGetAudioDevCaps(k,&caps) == AUDIO_ERROR_NONE &&
					strcmp(caps.szProductName,"Sound Blaster AWE32"))
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

	/* use stereo output if supported */
	if (usestereo)
	{
		if (Machine->drv->sound_attributes & SOUND_SUPPORTS_STEREO)
			info.wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO;
	}

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
			AUpdateAudioEx(Machine->sample_rate / Machine->drv->frames_per_second);
			b = uclock();
		} while (b-a < UCLOCKS_PER_SEC/10);

		a = uclock();
		AGetVoicePosition(hVoice[0],&start);
		do
		{
			AUpdateAudioEx(Machine->sample_rate / Machine->drv->frames_per_second);
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


#if 0
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
		}
#endif
	used_3812 = 0;

	osd_set_mastervolume(0);	/* start at maximum volume */

	return 0;
}

void msdos_shutdown_sound(void)
{
	if (Machine->sample_rate != 0)
	{
		int n;

		if (used_3812)
		{
			/* silence the OPL */
			for (n = 0x40;n <= 0x55;n++)
			{
				osd_ym3812_control(n);
				osd_ym3812_write(0x3f);
			}
			for (n = 0x60;n <= 0x95;n++)
			{
				osd_ym3812_control(n);
				osd_ym3812_write(0xff);
			}
		}

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



static void playsample(int channel,signed char *data,int len,int freq,int volume,int loop,int bits)
{
	if (Machine->sample_rate == 0 || channel >= NUMVOICES) return;

	/* backwards compatibility with old 0-255 volume range */
	if (volume > 100) volume = volume * 25 / 255;

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
	ASetVoiceVolume(hVoice[channel],volume * 64 / 100);
	AStartVoice(hVoice[channel]);
}

void osd_play_sample(int channel,signed char *data,int len,int freq,int volume,int loop)
{
	playsample(channel,data,len,freq,volume,loop,8);
}

void osd_play_sample_16(int channel,signed short *data,int len,int freq,int volume,int loop)
{
	playsample(channel,(signed char *)data,len,freq,volume,loop,16);
}


static void *stream_cache_data[NUMVOICES];
static int stream_cache_len[NUMVOICES];
static int stream_cache_freq[NUMVOICES];
static int stream_cache_volume[NUMVOICES];
static int stream_cache_pan[NUMVOICES];
static int stream_cache_bits[NUMVOICES];
static int streams_playing;
#define NUM_BUFFERS 3	/* raising this number should improve performance with frameskip, */
						/* but also increases the latency. */

static int playstreamedsample(int channel,signed char *data,int len,int freq,int volume,int pan,int bits)
{
	static int playing[NUMVOICES];
	static int c[NUMVOICES];


	/* backwards compatibility with old 0-255 volume range */
	if (volume > 100) volume = volume * 25 / 255;

	if (pan != OSD_PAN_CENTER) volume /= 2;

	if (Machine->sample_rate == 0 || channel >= NUMVOICES) return 1;

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
			return 1;

		lpWave[channel]->wFormat = (bits == 8 ? AUDIO_FORMAT_8BITS : AUDIO_FORMAT_16BITS)
				| AUDIO_FORMAT_MONO | AUDIO_FORMAT_LOOP;
		lpWave[channel]->nSampleRate = nominal_sample_rate;
		lpWave[channel]->dwLength = NUM_BUFFERS*len;
		lpWave[channel]->dwLoopStart = 0;
		lpWave[channel]->dwLoopEnd = NUM_BUFFERS*len;
		if (ACreateAudioData(lpWave[channel]) != AUDIO_ERROR_NONE)
		{
			free(lpWave[channel]);
			lpWave[channel] = 0;

			return 1;
		}

		memset(lpWave[channel]->lpData,0,NUM_BUFFERS*len);
		memcpy(lpWave[channel]->lpData,data,len);
		/* upload the data to the audio DRAM local memory */
		AWriteAudioData(lpWave[channel],0,NUM_BUFFERS*len);
		APrimeVoice(hVoice[channel],lpWave[channel]);
	/* need to cast to double because freq*nominal_sample_rate can exceed the size of an int */
		ASetVoiceFrequency(hVoice[channel],(double)freq*nominal_sample_rate/Machine->sample_rate);
		AStartVoice(hVoice[channel]);
		playing[channel] = 1;
		c[channel] = 1;

		streams_playing = 1;
	}
	else
	{
		LONG pos;
		extern int throttle;


		if (throttle)   /* sync with audio only when speed throttling is not turned off */
		{
			AGetVoicePosition(hVoice[channel],&pos);
			if (pos >= c[channel] * len && pos < (c[channel]+1)*len) return 0;
		}

		memcpy(&lpWave[channel]->lpData[len * c[channel]],data,len);
		AWriteAudioData(lpWave[channel],len*c[channel],len);
		c[channel] = (c[channel]+1) % NUM_BUFFERS;

		streams_playing = 1;
	}


	ASetVoiceVolume(hVoice[channel],volume * 64 / 100);
	if (pan == OSD_PAN_CENTER)
		ASetVoicePanning(hVoice[channel],128);
	else if (pan == OSD_PAN_LEFT)
		ASetVoicePanning(hVoice[channel],0);
	else if (pan == OSD_PAN_RIGHT)
		ASetVoicePanning(hVoice[channel],255);

	return 1;
}

void osd_play_streamed_sample(int channel,signed char *data,int len,int freq,int volume,int pan)
{
	stream_cache_data[channel] = data;
	stream_cache_len[channel] = len;
	stream_cache_freq[channel] = freq;
	stream_cache_volume[channel] = volume;
	stream_cache_pan[channel] = pan;
	stream_cache_bits[channel] = 8;
}

void osd_play_streamed_sample_16(int channel,signed short *data,int len,int freq,int volume,int pan)
{
	stream_cache_data[channel] = data;
	stream_cache_len[channel] = len;
	stream_cache_freq[channel] = freq;
	stream_cache_volume[channel] = volume;
	stream_cache_pan[channel] = pan;
	stream_cache_bits[channel] = 16;
}



void osd_adjust_sample(int channel,int freq,int volume)
{
	if (Machine->sample_rate == 0 || channel >= NUMVOICES) return;

	/* backwards compatibility with old 0-255 volume range */
	if (volume > 100) volume = volume * 25 / 255;

	/* need to cast to double because freq*nominal_sample_rate can exceed the size of an int */
	if (freq != -1)
		ASetVoiceFrequency(hVoice[channel],(double)freq*nominal_sample_rate/Machine->sample_rate);
	if (volume != -1)
		ASetVoiceVolume(hVoice[channel],volume * 64 / 100);
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



static int update_streams(void)
{
	int channel;
	int res = 1;


	streams_playing = 0;

	for (channel = 0;channel < NUMVOICES;channel++)
	{
		if (stream_cache_data[channel])
		{
			if (playstreamedsample(
					channel,
					stream_cache_data[channel],
					stream_cache_len[channel],
					stream_cache_freq[channel],
					stream_cache_volume[channel],
					stream_cache_pan[channel],
					stream_cache_bits[channel]))
				stream_cache_data[channel] = 0;
			else
				res = 0;
		}
	}

	return res;
}

int msdos_update_audio(void)
{
	if (Machine->sample_rate == 0) return 0;

	osd_profiler(OSD_PROFILE_SOUND);
	AUpdateAudioEx(Machine->sample_rate / Machine->drv->frames_per_second);
	osd_profiler(OSD_PROFILE_END);

	osd_profiler(OSD_PROFILE_IDLE);
	while (update_streams() == 0)
		AUpdateAudioEx(Machine->sample_rate / Machine->drv->frames_per_second);
	osd_profiler(OSD_PROFILE_END);

	return streams_playing;
}





static int attenuation = 0;
static int master_volume = 256;

/* attenuation in dB */
void osd_set_mastervolume(int _attenuation)
{
	float volume;


	attenuation = _attenuation;

 	volume = 256.0;	/* range is 0-256 */
	while (_attenuation++ < 0)
		volume /= 1.122018454;	/* = (10 ^ (1/20)) = 1dB */

	master_volume = volume;

	ASetAudioMixerValue(AUDIO_MIXER_MASTER_VOLUME,master_volume);
}

int osd_get_mastervolume(void)
{
	return attenuation;
}

void osd_sound_enable(int enable_it)
{
	if (enable_it)
	{
		ASetAudioMixerValue(AUDIO_MIXER_MASTER_VOLUME,master_volume);
	}
	else
	{
		ASetAudioMixerValue(AUDIO_MIXER_MASTER_VOLUME,0);
	}
}



/* linux sound driver opl3.c does a so called tenmicrosec() delay */
static void tenmicrosec(void)
{
    int i;
    for (i = 0; i < 16; i++)
        inportb(0x80);
}

void osd_ym3812_control(int reg)
{
    if (Machine->sample_rate == 0) return;

    tenmicrosec();
    outportb(0x388,reg);
}

void osd_ym3812_write(int data)
{
    if (Machine->sample_rate == 0) return;

    tenmicrosec();
    outportb(0x389,data);

	used_3812 = 1;
}
