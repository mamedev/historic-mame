/***************************************************************************

    sound.c

    Core sound functions and definitions.

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "driver.h"
#include "osdepend.h"
#include "streams.h"
#include "config.h"
#include "profiler.h"
#include "sound/wavwrite.h"



/***************************************************************************
    DEBUGGING
***************************************************************************/

#define VERBOSE			(0)
#define MAKE_WAVS		(1)

#if VERBOSE
#define VPRINTF(x)		mame_printf_debug x
#else
#define VPRINTF(x)
#endif



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MAX_MIXER_CHANNELS		100
#define SOUND_UPDATE_FREQUENCY	MAME_TIME_IN_HZ(50)



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _sound_output sound_output;
struct _sound_output
{
	sound_stream *	stream;					/* associated stream */
	int				output;					/* output number */
};


typedef struct _sound_info sound_info;
struct _sound_info
{
	const sound_config *sound;				/* pointer to the sound info */
	int				outputs;				/* number of outputs from this instance */
	sound_output *	output;					/* array of output information */
};


typedef struct _speaker_input speaker_input;
struct _speaker_input
{
	float			gain;					/* current gain */
	float			default_gain;			/* default gain */
	char *			name;					/* name of this input */
};


typedef struct _speaker_info speaker_info;
struct _speaker_info
{
	const speaker_config *speaker;			/* pointer to the speaker info */
	sound_stream *	mixer_stream;			/* mixing stream */
	int				inputs;					/* number of input streams */
	speaker_input *	input;					/* array of input information */
#ifdef MAME_DEBUG
	INT32			max_sample;				/* largest sample value we've seen */
	INT32			clipped_samples;		/* total number of clipped samples */
	INT32			total_samples;			/* total number of samples */
#endif
};



/***************************************************************************
    GLOBAL VARIABLES
***************************************************************************/

static mame_timer *sound_update_timer;

static int totalsnd;
static sound_info sound[MAX_SOUND];

static int totalspeakers;
static speaker_info speaker[MAX_SPEAKER];

static INT16 *finalmix;
static INT32 *leftmix, *rightmix;

static int sound_enabled;
static int sound_attenuation;
static int global_sound_enabled;
static int nosound_mode;

static wav_file *wavfile;



/***************************************************************************
    FUNCTION PROTOTYPES
***************************************************************************/

static void sound_reset(running_machine *machine);
static void sound_pause(running_machine *machine, int pause);
static void sound_exit(running_machine *machine);
static void sound_load(int config_type, xml_data_node *parentnode);
static void sound_save(int config_type, xml_data_node *parentnode);
static void sound_update(int param);
static int start_sound_chips(void);
static int start_speakers(void);
static int route_sound(void);
static void mixer_update(void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length);



/***************************************************************************
    INLINE FUNCTIONS
***************************************************************************/

/*-------------------------------------------------
    find_speaker_by_tag - find a tagged speaker
-------------------------------------------------*/

INLINE speaker_info *find_speaker_by_tag(const char *tag)
{
	int spknum;

	/* attempt to find the speaker in our list */
	for (spknum = 0; spknum < totalspeakers; spknum++)
		if (strcmp(speaker[spknum].speaker->tag, tag) == 0)
			return &speaker[spknum];
	return NULL;
}


/*-------------------------------------------------
    find_sound_by_tag - find a tagged sound chip
-------------------------------------------------*/

INLINE sound_info *find_sound_by_tag(const char *tag)
{
	int sndnum;

	/* attempt to find the speaker in our list */
	for (sndnum = 0; sndnum < totalsnd; sndnum++)
		if (sound[sndnum].sound->tag && !strcmp(sound[sndnum].sound->tag, tag))
			return &sound[sndnum];
	return NULL;
}



/***************************************************************************
    INITIALIZATION
***************************************************************************/

/*-------------------------------------------------
    sound_init - start up the sound system
-------------------------------------------------*/

int sound_init(running_machine *machine)
{
	mame_time update_frequency = SOUND_UPDATE_FREQUENCY;

	/* handle -nosound */
	nosound_mode = (Machine->sample_rate == 0);
	if (nosound_mode)
		Machine->sample_rate = 11025;

	/* count the speakers */
	for (totalspeakers = 0; Machine->drv->speaker[totalspeakers].tag; totalspeakers++) ;
	VPRINTF(("total speakers = %d\n", totalspeakers));

	/* allocate memory for mix buffers */
	leftmix = auto_malloc(Machine->sample_rate * sizeof(*leftmix));
	rightmix = auto_malloc(Machine->sample_rate * sizeof(*rightmix));
	finalmix = auto_malloc(Machine->sample_rate * sizeof(*finalmix));

	/* allocate a global timer for sound timing */
	sound_update_timer = mame_timer_alloc(sound_update);
	mame_timer_adjust(sound_update_timer, update_frequency, 0, update_frequency);

	/* initialize the streams engine */
	VPRINTF(("streams_init\n"));
	streams_init(machine, update_frequency.subseconds);

	/* now start up the sound chips and tag their streams */
	VPRINTF(("start_sound_chips\n"));
	if (start_sound_chips())
		return 1;

	/* then create all the speakers */
	VPRINTF(("start_speakers\n"));
	if (start_speakers())
		return 1;

	/* finally, do all the routing */
	VPRINTF(("route_sound\n"));
	if (route_sound())
		return 1;

	if (MAKE_WAVS)
		wavfile = wav_open("finalmix.wav", Machine->sample_rate, 2);

	/* enable sound by default */
	global_sound_enabled = TRUE;
	sound_enabled = TRUE;
	sound_set_attenuation(sound_attenuation);

	/* register callbacks */
	config_register("mixer", sound_load, sound_save);
	add_pause_callback(machine, sound_pause);
	add_reset_callback(machine, sound_reset);
	add_exit_callback(machine, sound_exit);

	return 0;
}


/*-------------------------------------------------
    sound_exit - clean up after ourselves
-------------------------------------------------*/

static void sound_exit(running_machine *machine)
{
	int sndnum;

	if (wavfile != NULL)
		wav_close(wavfile);

#ifdef MAME_DEBUG
{
	int spknum;

	/* log the maximum sample values for all speakers */
	for (spknum = 0; spknum < totalspeakers; spknum++)
		if (speaker[spknum].max_sample > 0)
		{
			speaker_info *spk = &speaker[spknum];
			mame_printf_debug("Speaker \"%s\" - max = %d (gain *= %f) - %d%% samples clipped\n", spk->speaker->tag, spk->max_sample, 32767.0 / (spk->max_sample ? spk->max_sample : 1), (int)((double)spk->clipped_samples * 100.0 / spk->total_samples));
		}
}
#endif /* MAME_DEBUG */

	/* stop all the sound chips */
	for (sndnum = 0; sndnum < MAX_SOUND; sndnum++)
		if (Machine->drv->sound[sndnum].sound_type != 0)
			sndintrf_exit_sound(sndnum);

	/* reset variables */
	totalspeakers = 0;
	totalsnd = 0;
	memset(&speaker, 0, sizeof(speaker));
	memset(&sound, 0, sizeof(sound));
}



/***************************************************************************
    INITIALIZATION HELPERS
***************************************************************************/

/*-------------------------------------------------
    start_sound_chips - loop over all sound chips
    and initialize them
-------------------------------------------------*/

static int start_sound_chips(void)
{
	int sndnum;

	/* reset the sound array */
	memset(sound, 0, sizeof(sound));

	/* start up all the sound chips */
	for (sndnum = 0; sndnum < MAX_SOUND; sndnum++)
	{
		const sound_config *msound = &Machine->drv->sound[sndnum];
		sound_info *info;
		int num_regs;
		int index;

		/* stop when we hit an empty entry */
		if (msound->sound_type == 0)
			break;
		totalsnd++;

		/* zap all the info */
		info = &sound[sndnum];
		memset(info, 0, sizeof(*info));

		/* copy in all the relevant info */
		info->sound = msound;

		/* start the chip, tagging all its streams */
		VPRINTF(("sndnum = %d -- sound_type = %d\n", sndnum, msound->sound_type));
		num_regs = state_save_get_reg_count();
		streams_set_tag(Machine, info);
		if (sndintrf_init_sound(sndnum, msound->sound_type, msound->clock, msound->config) != 0)
			return 1;

		/* if no state registered for saving, we can't save */
		num_regs = state_save_get_reg_count() - num_regs;
		if (num_regs == 0)
		{
			logerror("Sound chip #%d (%s) did not register any state to save!\n", sndnum, sndnum_name(sndnum));
			if (Machine->gamedrv->flags & GAME_SUPPORTS_SAVE)
				fatalerror("Sound chip #%d (%s) did not register any state to save!", sndnum, sndnum_name(sndnum));
		}

		/* now count the outputs */
		VPRINTF(("Counting outputs\n"));
		for (index = 0; ; index++)
		{
			sound_stream *stream = stream_find_by_tag(info, index);
			if (!stream)
				break;
			info->outputs += stream_get_outputs(stream);
			VPRINTF(("  stream %p, %d outputs\n", stream, stream_get_outputs(stream)));
		}

		/* if we have outputs, examine them */
		if (info->outputs)
		{
			/* allocate an array to hold them */
			info->output = auto_malloc(info->outputs * sizeof(*info->output));
			VPRINTF(("  %d outputs total\n", info->outputs));

			/* now fill the array */
			info->outputs = 0;
			for (index = 0; ; index++)
			{
				sound_stream *stream = stream_find_by_tag(info, index);
				int outputs, outputnum;

				if (!stream)
					break;
				outputs = stream_get_outputs(stream);

				/* fill in an entry for each output */
				for (outputnum = 0; outputnum < outputs; outputnum++)
				{
					info->output[info->outputs].stream = stream;
					info->output[info->outputs].output = outputnum;
					info->outputs++;
				}
			}
		}
	}
	return 0;
}


/*-------------------------------------------------
    start_speakers - loop over all speakers and
    initialize them
-------------------------------------------------*/

static int start_speakers(void)
{
	/* reset the speaker array */
	memset(speaker, 0, sizeof(speaker));

	/* start up all the speakers */
	for (totalspeakers = 0; totalspeakers < MAX_SPEAKER; totalspeakers++)
	{
		const speaker_config *mspeaker = &Machine->drv->speaker[totalspeakers];
		speaker_info *info;

		/* stop when we hit an empty entry */
		if (mspeaker->tag == NULL)
			break;

		/* zap all the info */
		info = &speaker[totalspeakers];
		memset(info, 0, sizeof(*info));

		/* copy in all the relevant info */
		info->speaker = mspeaker;
		info->mixer_stream = NULL;
		info->inputs = 0;
	}
	return 0;
}


/*-------------------------------------------------
    route_sound - route sound outputs to target
    inputs
-------------------------------------------------*/

static int route_sound(void)
{
	int sndnum, spknum, routenum, outputnum;

	/* iterate over all the sound chips */
	for (sndnum = 0; sndnum < totalsnd; sndnum++)
	{
		sound_info *info = &sound[sndnum];

		/* iterate over all routes */
		for (routenum = 0; routenum < info->sound->routes; routenum++)
		{
			const sound_route *mroute = &info->sound->route[routenum];
			speaker_info *speaker;
			sound_info *sound;

			/* find the target */
			speaker = find_speaker_by_tag(mroute->target);
			sound = find_sound_by_tag(mroute->target);

			/* if neither found, it's fatal */
			if (speaker == NULL && sound == NULL)
				fatalerror("Sound route \"%s\" not found!\n", mroute->target);

			/* if we got a speaker, bump its input count */
			if (speaker != NULL)
			{
				if (mroute->output >= 0 && mroute->output < info->outputs)
					speaker->inputs++;
				else if (mroute->output == ALL_OUTPUTS)
					speaker->inputs += info->outputs;
			}
		}
	}

	/* now allocate the mixers and input data */
	streams_set_tag(Machine, NULL);
	for (spknum = 0; spknum < totalspeakers; spknum++)
	{
		speaker_info *info = &speaker[spknum];
		if (info->inputs != 0)
		{
			info->mixer_stream = stream_create(info->inputs, 1, Machine->sample_rate, info, mixer_update);
			info->input = auto_malloc(info->inputs * sizeof(*info->input));
			info->inputs = 0;
		}
		else
			logerror("Warning: speaker \"%s\" has no inputs\n", info->speaker->tag);
	}

	/* iterate again over all the sound chips */
	for (sndnum = 0; sndnum < totalsnd; sndnum++)
	{
		sound_info *info = &sound[sndnum];

		/* iterate over all routes */
		for (routenum = 0; routenum < info->sound->routes; routenum++)
		{
			const sound_route *mroute = &info->sound->route[routenum];
			speaker_info *speaker;
			sound_info *sound;

			/* find the target */
			speaker = find_speaker_by_tag(mroute->target);
			sound = find_sound_by_tag(mroute->target);

			/* if it's a speaker, set the input */
			if (speaker != NULL)
			{
				for (outputnum = 0; outputnum < info->outputs; outputnum++)
					if (mroute->output == outputnum || mroute->output == ALL_OUTPUTS)
					{
						char namebuf[256];
						int index;

						sndnum_to_sndti(sndnum, &index);

						/* fill in the input data on this speaker */
						speaker->input[speaker->inputs].gain = mroute->gain;
						speaker->input[speaker->inputs].default_gain = mroute->gain;
						sprintf(namebuf, "%s:%s #%d.%d", speaker->speaker->tag, sndnum_name(sndnum), index, outputnum);
						speaker->input[speaker->inputs].name = auto_strdup(namebuf);

						/* connect the output to the input */
						stream_set_input(speaker->mixer_stream, speaker->inputs++, info->output[outputnum].stream, info->output[outputnum].output, mroute->gain);
					}
			}

			/* if it's a sound chip, set the input */
			else
			{
				for (outputnum = 0; outputnum < info->outputs; outputnum++)
					if (mroute->output == outputnum || mroute->output == ALL_OUTPUTS)
						stream_set_input(sound->output[0].stream, 0, info->output[outputnum].stream, info->output[outputnum].output, mroute->gain);
			}
		}
	}

	return 0;
}



/***************************************************************************
    GLOBAL STATE MANAGEMENT
***************************************************************************/

/*-------------------------------------------------
    sound_reset - reset all sound chips
-------------------------------------------------*/

static void sound_reset(running_machine *machine)
{
	int sndnum;

	/* reset all the sound chips */
	for (sndnum = 0; sndnum < MAX_SOUND; sndnum++)
		if (Machine->drv->sound[sndnum].sound_type != 0)
			sndnum_reset(sndnum);
}


/*-------------------------------------------------
    sound_pause - pause sound output
-------------------------------------------------*/

static void sound_pause(running_machine *machine, int pause)
{
	sound_enabled = !pause;
	osd_set_mastervolume(sound_enabled ? sound_attenuation : -32);
}


/*-------------------------------------------------
    sound_set_attenuation - set the global volume
-------------------------------------------------*/

void sound_set_attenuation(int attenuation)
{
	sound_attenuation = attenuation;
	osd_set_mastervolume(sound_enabled ? sound_attenuation : -32);
}


/*-------------------------------------------------
    sound_get_attenuation - return the global
    volume
-------------------------------------------------*/

int sound_get_attenuation(void)
{
	return sound_attenuation;
}


/*-------------------------------------------------
    sound_global_enable - enable/disable sound
    globally
-------------------------------------------------*/

void sound_global_enable(int enable)
{
	global_sound_enabled = enable;
}



/***************************************************************************
    SOUND SAVE/LOAD
***************************************************************************/

/*-------------------------------------------------
    sound_load - read and apply data from the
    configuration file
-------------------------------------------------*/

static void sound_load(int config_type, xml_data_node *parentnode)
{
	xml_data_node *channelnode;
	int mixernum;

	/* we only care about game files */
	if (config_type != CONFIG_TYPE_GAME)
		return;

	/* might not have any data */
	if (parentnode == NULL)
		return;

	/* iterate over channel nodes */
	for (channelnode = xml_get_sibling(parentnode->child, "channel"); channelnode; channelnode = xml_get_sibling(channelnode->next, "channel"))
	{
		mixernum = xml_get_attribute_int(channelnode, "index", -1);
		if (mixernum >= 0 && mixernum < MAX_MIXER_CHANNELS)
		{
			float defvol = xml_get_attribute_float(channelnode, "defvol", -1000.0);
			float newvol = xml_get_attribute_float(channelnode, "newvol", -1000.0);
			if (defvol == sound_get_default_gain(mixernum) && newvol != -1000.0)
				sound_set_user_gain(mixernum, newvol);
		}
	}
}


/*-------------------------------------------------
    sound_save - save data to the configuration
    file
-------------------------------------------------*/

static void sound_save(int config_type, xml_data_node *parentnode)
{
	int mixernum;

	/* we only care about game files */
	if (config_type != CONFIG_TYPE_GAME)
		return;

	/* iterate over mixer channels */
	if (parentnode != NULL)
		for (mixernum = 0; mixernum < MAX_MIXER_CHANNELS; mixernum++)
		{
			float defvol = sound_get_default_gain(mixernum);
			float newvol = sound_get_user_gain(mixernum);

			if (defvol != newvol)
			{
				xml_data_node *channelnode = xml_add_child(parentnode, "channel", NULL);
				if (channelnode != NULL)
				{
					xml_set_attribute_int(channelnode, "index", mixernum);
					xml_set_attribute_float(channelnode, "defvol", defvol);
					xml_set_attribute_float(channelnode, "newvol", newvol);
				}
			}
		}
}



/***************************************************************************
    MIXING STAGE
***************************************************************************/

/*-------------------------------------------------
    sound_update - mix everything down to
    its final form and send it to the OSD layer
-------------------------------------------------*/

static void sound_update(int param)
{
	int samples_this_update = 0;
	int sample, spknum;

	VPRINTF(("sound_frame_update\n"));

	profiler_mark(PROFILER_SOUND);

	/* force all the speaker streams to generate the proper number of samples */
	for (spknum = 0; spknum < totalspeakers; spknum++)
	{
		speaker_info *spk = &speaker[spknum];
		const stream_sample_t *stream_buf;

		/* get the output buffer */
		if (spk->mixer_stream != NULL)
		{
			int numsamples;

			/* update the stream, getting the start/end pointers around the operation */
			stream_buf = stream_get_output_since_last_update(spk->mixer_stream, 0, &numsamples);

			/* set or assert that all streams have the same count */
			if (samples_this_update == 0)
			{
				samples_this_update = numsamples;

				/* reset the mixing streams */
				memset(leftmix, 0, samples_this_update * sizeof(*leftmix));
				memset(rightmix, 0, samples_this_update * sizeof(*rightmix));
			}
			assert(samples_this_update == numsamples);

#ifdef MAME_DEBUG
			/* debug version: keep track of the maximum sample */
			for (sample = 0; sample < samples_this_update; sample++)
			{
				if (stream_buf[sample] > spk->max_sample)
					spk->max_sample = stream_buf[sample];
				else if (-stream_buf[sample] > spk->max_sample)
					spk->max_sample = -stream_buf[sample];
				if (stream_buf[sample] > 32767 || stream_buf[sample] < -32768)
					spk->clipped_samples++;
				spk->total_samples++;
			}
#endif

			/* mix if sound is enabled */
			if (global_sound_enabled && !nosound_mode)
			{
				/* if the speaker is centered, send to both left and right */
				if (spk->speaker->x == 0)
					for (sample = 0; sample < samples_this_update; sample++)
					{
						leftmix[sample] += stream_buf[sample];
						rightmix[sample] += stream_buf[sample];
					}

				/* if the speaker is to the left, send only to the left */
				else if (spk->speaker->x < 0)
					for (sample = 0; sample < samples_this_update; sample++)
						leftmix[sample] += stream_buf[sample];

				/* if the speaker is to the right, send only to the right */
				else
					for (sample = 0; sample < samples_this_update; sample++)
						rightmix[sample] += stream_buf[sample];
			}
		}
	}

	/* now downmix the final result */
	for (sample = 0; sample < samples_this_update; sample++)
	{
		INT32 samp;

		/* clamp the left side */
		samp = leftmix[sample];
		if (samp < -32768)
			samp = -32768;
		else if (samp > 32767)
			samp = 32767;
		finalmix[sample*2+0] = samp;

		/* clamp the right side */
		samp = rightmix[sample];
		if (samp < -32768)
			samp = -32768;
		else if (samp > 32767)
			samp = 32767;
		finalmix[sample*2+1] = samp;
	}

	/* play the result */
	if (samples_this_update > 0)
	{
		osd_update_audio_stream(finalmix, samples_this_update);
		if (wavfile != NULL)
			wav_add_data_16(wavfile, finalmix, samples_this_update * 2);
	}

	/* update the streamer */
	streams_update(Machine);

	profiler_mark(PROFILER_END);
}


/*-------------------------------------------------
    mixer_update - mix all inputs to one output
-------------------------------------------------*/

static void mixer_update(void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length)
{
	speaker_info *speaker = param;
	int numinputs = speaker->inputs;
	int pos;

	VPRINTF(("Mixer_update(%d)\n", length));

	/* loop over samples */
	for (pos = 0; pos < length; pos++)
	{
		INT32 sample = inputs[0][pos];
		int inp;

		/* add up all the inputs */
		for (inp = 1; inp < numinputs; inp++)
			sample += inputs[inp][pos];
		buffer[0][pos] = sample;
	}
}



/***************************************************************************
    MISCELLANEOUS HELPERS
***************************************************************************/

/*-------------------------------------------------
    sndti_set_output_gain - set the gain of a
    particular output
-------------------------------------------------*/

void sndti_set_output_gain(int type, int index, int output, float gain)
{
	int sndnum = sndti_to_sndnum(type, index);

	if (sndnum < 0)
	{
		logerror("sndti_set_output_gain called for invalid sound type %d, index %d\n", type, index);
		return;
	}
	if (output >= sound[sndnum].outputs)
	{
		logerror("sndti_set_output_gain called for invalid sound output %d (type %d, index %d)\n", output, type, index);
		return;
	}
	stream_set_output_gain(sound[sndnum].output[output].stream, sound[sndnum].output[output].output, gain);
}



/***************************************************************************
    USER GAIN CONTROLS
***************************************************************************/

/*-------------------------------------------------
    index_to_input - map an absolute index to
    a particular input
-------------------------------------------------*/

INLINE speaker_info *index_to_input(int index, int *input)
{
	int count = 0, speakernum;

	/* scan through the speakers until we find the indexed input */
	for (speakernum = 0; speakernum < totalspeakers; speakernum++)
	{
		if (index < count + speaker[speakernum].inputs)
		{
			*input = index - count;
			return &speaker[speakernum];
		}
		count += speaker[speakernum].inputs;
	}

	/* index out of range */
	return NULL;
}


/*-------------------------------------------------
    sound_get_user_gain_count - return the number
    of user-controllable gain parameters
-------------------------------------------------*/

int sound_get_user_gain_count(void)
{
	int count = 0, speakernum;

	/* count up the number of speaker inputs */
	for (speakernum = 0; speakernum < totalspeakers; speakernum++)
		count += speaker[speakernum].inputs;
	return count;
}


/*-------------------------------------------------
    sound_set_user_gain - set the nth user gain
    value
-------------------------------------------------*/

void sound_set_user_gain(int index, float gain)
{
	int inputnum;
	speaker_info *spk = index_to_input(index, &inputnum);

	if (spk != NULL)
	{
		spk->input[inputnum].gain = gain;
		stream_set_input_gain(spk->mixer_stream, inputnum, gain);
	}
}


/*-------------------------------------------------
    sound_get_user_gain - get the nth user gain
    value
-------------------------------------------------*/

float sound_get_user_gain(int index)
{
	int inputnum;
	speaker_info *spk = index_to_input(index, &inputnum);
	return (spk != NULL) ? spk->input[inputnum].gain : 0;
}


/*-------------------------------------------------
    sound_get_default_gain - return the default
    gain of the nth user value
-------------------------------------------------*/

float sound_get_default_gain(int index)
{
	int inputnum;
	speaker_info *spk = index_to_input(index, &inputnum);
	return (spk != NULL) ? spk->input[inputnum].default_gain : 0;
}


/*-------------------------------------------------
    sound_get_user_gain_name - return the name
    of the nth user value
-------------------------------------------------*/

const char *sound_get_user_gain_name(int index)
{
	int inputnum;
	speaker_info *spk = index_to_input(index, &inputnum);
	return (spk != NULL) ? spk->input[inputnum].name : NULL;
}


/*-------------------------------------------------
    sound_find_sndnum_by_tag - return a sndnum
    by finding the appropriate tag
-------------------------------------------------*/

int sound_find_sndnum_by_tag(const char *tag)
{
	int index;

	/* find a match */
	for (index = 0; index < MAX_SOUND; index++)
		if (sound[index].sound != NULL && sound[index].sound->tag != NULL)
			if (strcmp(tag, sound[index].sound->tag) == 0)
				return index;

	return -1;
}