#include "driver.h"

extern int segar_sh_start(void);
extern void segar_sh_update(void);


struct TMS3617_interface
{
   int samplerate;      /* sample rate */
   int gain;            /* 16 * gain adjustment */
   int volume;          /* playback volume */
};

int TMS3617_sh_start(struct TMS3617_interface *intf);
void TMS3617_sh_stop(void);
void TMS3617_sh_update(void);

int monsterb_sh_start(void);
void monsterb_sh_update(void);
void monsterb_sh_stop(void);
void monsterb_sound_w(int offset,int data);


#define TMS3617_VOICES          6

#define BASE_FREQ       246.9416506
#define PITCH_1         BASE_FREQ * 1
#define PITCH_2         BASE_FREQ * 1.059463094
#define PITCH_3         BASE_FREQ * 1.122462048
#define PITCH_4         BASE_FREQ * 1.189207115
#define PITCH_5         BASE_FREQ * 1.259921050
#define PITCH_6         BASE_FREQ * 1.334839854
#define PITCH_7         BASE_FREQ * 1.414213562
#define PITCH_8         BASE_FREQ * 1.498307077
#define PITCH_9         BASE_FREQ * 1.587401052
#define PITCH_10        BASE_FREQ * 1.681792830
#define PITCH_11        BASE_FREQ * 1.781797436
#define PITCH_12        BASE_FREQ * 1.887748625
#define PITCH_13        BASE_FREQ * 2

/*
#define SHIFT_1	1
#define SHIFT_2	2
#define SHIFT_3	2*1.334839854
#define SHIFT_4	4
#define SHIFT_5	4*1.334839854
#define SHIFT_6	8
*/

#define SHIFT_1 4*1
#define SHIFT_2 4*2
#define SHIFT_3 4*2*1.334839854
#define SHIFT_4 4*4
#define SHIFT_5 4*4*1.334839854
#define SHIFT_6 4*8

static int TMS3617_freq[] =
{
  0, PITCH_1*SHIFT_1, PITCH_2*SHIFT_1, PITCH_3*SHIFT_1,
  PITCH_4*SHIFT_1, PITCH_5*SHIFT_1, PITCH_6*SHIFT_1, PITCH_7*SHIFT_1,
  PITCH_8*SHIFT_1, PITCH_9*SHIFT_1, PITCH_10*SHIFT_1, PITCH_11*SHIFT_1,
  PITCH_12*SHIFT_1, PITCH_13*SHIFT_1, 0, 0,

  0, PITCH_1*SHIFT_2, PITCH_2*SHIFT_2, PITCH_3*SHIFT_2,
  PITCH_4*SHIFT_2, PITCH_5*SHIFT_2, PITCH_6*SHIFT_2, PITCH_7*SHIFT_2,
  PITCH_8*SHIFT_2, PITCH_9*SHIFT_2, PITCH_10*SHIFT_2, PITCH_11*SHIFT_2,
  PITCH_12*SHIFT_2, PITCH_13*SHIFT_2, 0, 0,

  0, PITCH_1*SHIFT_3, PITCH_2*SHIFT_3, PITCH_3*SHIFT_3,
  PITCH_4*SHIFT_3, PITCH_5*SHIFT_3, PITCH_6*SHIFT_3, PITCH_7*SHIFT_3,
  PITCH_8*SHIFT_3, PITCH_9*SHIFT_3, PITCH_10*SHIFT_3, PITCH_11*SHIFT_3,
  PITCH_12*SHIFT_3, PITCH_13*SHIFT_3, 0, 0,

  0, PITCH_1*SHIFT_4, PITCH_2*SHIFT_4, PITCH_3*SHIFT_4,
  PITCH_4*SHIFT_4, PITCH_5*SHIFT_4, PITCH_6*SHIFT_4, PITCH_7*SHIFT_4,
  PITCH_8*SHIFT_4, PITCH_9*SHIFT_4, PITCH_10*SHIFT_4, PITCH_11*SHIFT_4,
  PITCH_12*SHIFT_4, PITCH_13*SHIFT_4, 0, 0,

  0, PITCH_1*SHIFT_5, PITCH_2*SHIFT_5, PITCH_3*SHIFT_5,
  PITCH_4*SHIFT_5, PITCH_5*SHIFT_5, PITCH_6*SHIFT_5, PITCH_7*SHIFT_5,
  PITCH_8*SHIFT_5, PITCH_9*SHIFT_5, PITCH_10*SHIFT_5, PITCH_11*SHIFT_5,
  PITCH_12*SHIFT_5, PITCH_13*SHIFT_5, 0, 0,

  0, PITCH_1*SHIFT_6, PITCH_2*SHIFT_6, PITCH_3*SHIFT_6,
  PITCH_4*SHIFT_6, PITCH_5*SHIFT_6, PITCH_6*SHIFT_6, PITCH_7*SHIFT_6,
  PITCH_8*SHIFT_6, PITCH_9*SHIFT_6, PITCH_10*SHIFT_6, PITCH_11*SHIFT_6,
  PITCH_12*SHIFT_6, PITCH_13*SHIFT_6, 0, 0

};

/* waveforms for the audio hardware */
static unsigned char TMS3617_waveform[] =
{
        0x00,0x00,0x01,0x01,0x02,0x02,0x03,0x03,0x04,0x04,0x05,0x05,0x06,0x06,0x07,0x07,
        0x08,0x08,0x09,0x09,0x0a,0x0a,0x0b,0x0b,0x0c,0x0c,0x0d,0x0d,0x0e,0x0e,0x0f,0x0f,
};

/* note: we work with signed samples here, unlike other drivers */
#define AUDIO_CONV(A) (A)

static struct TMS3617_interface *interface;
static int emulation_rate;
static int buffer_len;
static int sample_pos;
static signed char *outbuffer;

static int channel;

static int sound_enable;

static int wave_toggle[TMS3617_VOICES];
static int pitch;

static int counter[TMS3617_VOICES];

static signed char *mixer_table;
static signed char *mixer_lookup;
static signed short *mixer_buffer;


/* note: gain is specified as gain*16 */
static int make_mixer_table (int gain)
{
        int count = TMS3617_VOICES * 128;
	int i;

	/* allocate memory */
        mixer_table = malloc (256 * TMS3617_VOICES);
	if (!mixer_table)
		return 1;

	/* find the middle of the table */
        mixer_lookup = mixer_table + (TMS3617_VOICES * 128);

	/* fill in the table */
	for (i = 0; i < count; i++)
	{
                int val = i * gain / (TMS3617_VOICES * 16);
		if (val > 127) val = 127;
		mixer_lookup[ i] = AUDIO_CONV(val);
		mixer_lookup[-i] = AUDIO_CONV(-val);
	}

	return 0;
}

int TMS3617_sh_start(struct TMS3617_interface *intf)
{
	int voice;


	buffer_len = intf->samplerate / Machine->drv->frames_per_second;
	emulation_rate = buffer_len * Machine->drv->frames_per_second;

        channel = get_play_channels(1);

	if ((outbuffer = malloc(buffer_len)) == 0)
		return 1;

	if ((mixer_buffer = malloc(sizeof(short) * buffer_len)) == 0)
	{
		free (outbuffer);
		return 1;
	}

        if (make_mixer_table (intf->gain))
	{
		free (mixer_buffer);
		free (outbuffer);
		return 1;
	}

	sample_pos = 0;
	interface = intf;
	sound_enable = 1;	/* start with sound enabled, many games don't have a sound enable register */

        for (voice = 0;voice < TMS3617_VOICES;voice++)
	{
                wave_toggle[voice] = 0;
		counter[voice] = 0;
	}

        pitch=0;

	return 0;
}


void TMS3617_sh_stop(void)
{
	free (mixer_table);
	free (mixer_buffer);
	free (outbuffer);
}


static void TMS3617_update(signed char *buffer,int len)
{
	int i;
	int voice;

	short *mix;

	/* if no sound, fill with silence */
	if (sound_enable == 0)
	{
		for (i = 0; i < len; i++)
			*buffer++ = AUDIO_CONV (0x00);
		return;
	}


	mix = mixer_buffer;
	for (i = 0; i < len; i++)
		*mix++ = 0;

        for (voice = 0; voice < TMS3617_VOICES; voice++)
	{
                int f;
                int v;

                f = TMS3617_freq[pitch+voice*16]*wave_toggle[voice];
                v = 255*(f>0);

		if (v && f)
		{
                        const unsigned char *w = TMS3617_waveform;
			int c = counter[voice];

			mix = mixer_buffer;
			for (i = 0; i < len; i++)
			{
				c += f;
				*mix++ += ((w[(c >> 15) & 0x1f] & 0x0f) - 8) * v;
			}

			counter[voice] = c;
		}
	}

	mix = mixer_buffer;
	for (i = 0; i < len; i++)
		*buffer++ = mixer_lookup[*mix++];
}



void TMS3617_sh_update(void)
{
	if (Machine->sample_rate == 0) return;

	TMS3617_update(&outbuffer[sample_pos],buffer_len - sample_pos);
	sample_pos = 0;

	osd_play_streamed_sample(channel,outbuffer,buffer_len,emulation_rate,interface->volume);
}


/********************************************************************************/


static void doupdate(void)
{
	int newpos;


	newpos = cpu_scalebyfcount(buffer_len);	/* get current position based on the timer */

	TMS3617_update(&outbuffer[sample_pos],newpos - sample_pos);
	sample_pos = newpos;
}


/********************************************************************************/


static struct TMS3617_interface monsterb_interface =
{
   44100,           /* sample rate */
   48,              /* gain adjustment */
   128              /* playback volume */
};

void monsterb_sound_w(int offset,int data)
{
	doupdate();	/* update output buffer before changing the registers */

        pitch=((data & 0x0F)+1)%16;

        switch (data & 0xF0)
        {
                        /* mbsword */
                        case 0x00:
                                wave_toggle[0]=0;
                                wave_toggle[1]=0;
                                wave_toggle[2]=0;
                                wave_toggle[3]=0;
                                wave_toggle[4]=0;
                                wave_toggle[5]=1;
                                break;
                        /* mbbgm2 */
                        case 0x10:
                                wave_toggle[0]=0;
                                wave_toggle[1]=0;
                                wave_toggle[2]=0;
                                wave_toggle[3]=0;
                                wave_toggle[4]=0;
                                wave_toggle[5]=1;
                                break;
                        /* mbbgm1 */
                        case 0x20:
                                wave_toggle[0]=0;
                                wave_toggle[1]=0;
                                wave_toggle[2]=1;
                                wave_toggle[3]=0;
                                wave_toggle[4]=0;
                                wave_toggle[5]=1;
                                break;
                        /* mblitcandl */
                        case 0x30:
                                wave_toggle[0]=0;
                                wave_toggle[1]=0;
                                wave_toggle[2]=0;
                                wave_toggle[3]=0;
                                wave_toggle[4]=0;
                                wave_toggle[5]=1;
                                break;
                        /* mbsuccess */
                        case 0x40:
                                wave_toggle[0]=0;
                                wave_toggle[1]=0;
                                wave_toggle[2]=0;
                                wave_toggle[3]=0;
                                wave_toggle[4]=0;
                                wave_toggle[5]=1;
                                break;
                        /* mbshowmoon */
                        case 0x50:
                                wave_toggle[0]=0;
                                wave_toggle[1]=0;
                                wave_toggle[2]=0;
                                wave_toggle[3]=0;
                                wave_toggle[4]=0;
                                wave_toggle[5]=1;
                                break;
                        /* mbhidemoon */
                        case 0x60:
                                wave_toggle[0]=0;
                                wave_toggle[1]=0;
                                wave_toggle[2]=0;
                                wave_toggle[3]=0;
                                wave_toggle[4]=0;
                                wave_toggle[5]=1;
                                break;
                        /* mblost */
                        case 0x70:
                                wave_toggle[0]=0;
                                wave_toggle[1]=0;
                                wave_toggle[2]=0;
                                wave_toggle[3]=1;
                                wave_toggle[4]=0;
                                wave_toggle[5]=0;
                                break;
                        /* mbbetween */
                        case 0x80:
                                wave_toggle[0]=0;
                                wave_toggle[1]=0;
                                wave_toggle[2]=0;
                                wave_toggle[3]=0;
                                wave_toggle[4]=0;
                                wave_toggle[5]=1;
                                break;
                        /* mbgameover */
                        case 0xA0:
                                wave_toggle[0]=0;
                                wave_toggle[1]=0;
                                wave_toggle[2]=0;
                                wave_toggle[3]=1;
                                wave_toggle[4]=0;
                                wave_toggle[5]=0;
                                break;
                        /* mbcbonus */
                        case 0xB0:
                                wave_toggle[0]=0;
                                wave_toggle[1]=0;
                                wave_toggle[2]=0;
                                wave_toggle[3]=0;
                                wave_toggle[4]=0;
                                wave_toggle[5]=1;
                                break;
                        /* mbsuperzap */
                        case 0xC0:
                                wave_toggle[0]=0;
                                wave_toggle[1]=0;
                                wave_toggle[2]=0;
                                wave_toggle[3]=0;
                                wave_toggle[4]=0;
                                wave_toggle[5]=1;
                                break;
                        case 0x90:
                        case 0xD0:
                        case 0xE0:
                        case 0xF0:
                                wave_toggle[0]=0;
                                wave_toggle[1]=0;
                                wave_toggle[2]=0;
                                wave_toggle[3]=0;
                                wave_toggle[4]=0;
                                wave_toggle[5]=0;
                                break;
        }

}

int monsterb_sh_start(void)
{
        if (segar_sh_start()!=0)
                return 1;

	return TMS3617_sh_start(&monsterb_interface);	/* approximated clock, 8 voices */
}

void monsterb_sh_update(void)
{
        TMS3617_sh_update();
        segar_sh_update();
}


void monsterb_sh_stop(void)
{
        TMS3617_sh_stop();
}


