/***************************************************************************

	sndintrf.c

	Core sound interface functions and definitions.

****************************************************************************

	Still to do:
		* fix drivers that used to use ADPCM
		* many cores do their own resampling; they should stop
		* many cores mix to a separate buffer; no longer necessary

***************************************************************************/

#include "driver.h"
#include "sound/streams.h"
#include "sound/wavwrite.h"

#define VERBOSE			(0)
#define MAKE_WAVS		(0)

#if VERBOSE
#define VPRINTF(x) printf x
#else
#define VPRINTF(x)
#endif


/*************************************
 *
 *	Type definitions
 *
 *************************************/

struct sound_output
{
	sound_stream *	stream;							/* associated stream */
	int				output;							/* output number */
};


struct sound_info
{
	const struct MachineSound *sound;				/* pointer to the sound info */
	int				sndindex;						/* index of this chip */
	struct snd_interface intf;						/* copy of the sound interface for this chip */
	void *			token;							/* token returned by the start callback */
	int				outputs;						/* number of outputs from this instance */
	struct sound_output *output;					/* array of output information */
};


struct speaker_input
{
	float			gain;							/* current gain */
	float			default_gain;					/* default gain */
	char *			name;							/* name of this input */
};


struct speaker_info
{
	const struct MachineSpeaker *speaker;			/* pointer to the speaker info */
	sound_stream *	mixer_stream;					/* mixing stream */
	int				inputs;							/* number of input streams */
	struct speaker_input *input;					/* array of input information */
#ifdef MAME_DEBUG
	INT32			max_sample;						/* largest sample value we've seen */
	INT32			clipped_samples;				/* total number of clipped samples */
	INT32			total_samples;					/* total number of samples */
#endif
};



/*************************************
 *
 *	Prototypes for all get info routines
 *
 *************************************/

void dummy_sound_get_info(void *token, UINT32 state, union sndinfo *info);
void custom_get_info(void *token, UINT32 state, union sndinfo *info);
void samples_get_info(void *token, UINT32 state, union sndinfo *info);
void dac_get_info(void *token, UINT32 state, union sndinfo *info);
void dmadac_get_info(void *token, UINT32 state, union sndinfo *info);
void discrete_get_info(void *token, UINT32 state, union sndinfo *info);
void ay8910_get_info(void *token, UINT32 state, union sndinfo *info);
void ym2203_get_info(void *token, UINT32 state, union sndinfo *info);
void ym2151_get_info(void *token, UINT32 state, union sndinfo *info);
void ym2608_get_info(void *token, UINT32 state, union sndinfo *info);
void ym2610_get_info(void *token, UINT32 state, union sndinfo *info);
void ym2610b_get_info(void *token, UINT32 state, union sndinfo *info);
void ym2612_get_info(void *token, UINT32 state, union sndinfo *info);
void ym3438_get_info(void *token, UINT32 state, union sndinfo *info);
void ym2413_get_info(void *token, UINT32 state, union sndinfo *info);
void ym3812_get_info(void *token, UINT32 state, union sndinfo *info);
void ym3526_get_info(void *token, UINT32 state, union sndinfo *info);
void ymz280b_get_info(void *token, UINT32 state, union sndinfo *info);
void y8950_get_info(void *token, UINT32 state, union sndinfo *info);
void sn76477_get_info(void *token, UINT32 state, union sndinfo *info);
void sn76496_get_info(void *token, UINT32 state, union sndinfo *info);
void pokey_get_info(void *token, UINT32 state, union sndinfo *info);
void nesapu_get_info(void *token, UINT32 state, union sndinfo *info);
void astrocade_get_info(void *token, UINT32 state, union sndinfo *info);
void namco_get_info(void *token, UINT32 state, union sndinfo *info);
void namco_15xx_get_info(void *token, UINT32 state, union sndinfo *info);
void namco_cus30_get_info(void *token, UINT32 state, union sndinfo *info);
void namco_52xx_get_info(void *token, UINT32 state, union sndinfo *info);
void namco_54xx_get_info(void *token, UINT32 state, union sndinfo *info);
void namco_63701x_get_info(void *token, UINT32 state, union sndinfo *info);
void namcona_get_info(void *token, UINT32 state, union sndinfo *info);
void tms36xx_get_info(void *token, UINT32 state, union sndinfo *info);
void tms5110_get_info(void *token, UINT32 state, union sndinfo *info);
void tms5220_get_info(void *token, UINT32 state, union sndinfo *info);
void vlm5030_get_info(void *token, UINT32 state, union sndinfo *info);
void adpcm_get_info(void *token, UINT32 state, union sndinfo *info);
void okim6295_get_info(void *token, UINT32 state, union sndinfo *info);
void msm5205_get_info(void *token, UINT32 state, union sndinfo *info);
void msm5232_get_info(void *token, UINT32 state, union sndinfo *info);
void upd7759_get_info(void *token, UINT32 state, union sndinfo *info);
void hc55516_get_info(void *token, UINT32 state, union sndinfo *info);
void k005289_get_info(void *token, UINT32 state, union sndinfo *info);
void k007232_get_info(void *token, UINT32 state, union sndinfo *info);
void k051649_get_info(void *token, UINT32 state, union sndinfo *info);
void k053260_get_info(void *token, UINT32 state, union sndinfo *info);
void k054539_get_info(void *token, UINT32 state, union sndinfo *info);
void segapcm_get_info(void *token, UINT32 state, union sndinfo *info);
void rf5c68_get_info(void *token, UINT32 state, union sndinfo *info);
void cem3394_get_info(void *token, UINT32 state, union sndinfo *info);
void c140_get_info(void *token, UINT32 state, union sndinfo *info);
void qsound_get_info(void *token, UINT32 state, union sndinfo *info);
void saa1099_get_info(void *token, UINT32 state, union sndinfo *info);
void iremga20_get_info(void *token, UINT32 state, union sndinfo *info);
void es5505_get_info(void *token, UINT32 state, union sndinfo *info);
void es5506_get_info(void *token, UINT32 state, union sndinfo *info);
void bsmt2000_get_info(void *token, UINT32 state, union sndinfo *info);
void ymf262_get_info(void *token, UINT32 state, union sndinfo *info);
void ymf278b_get_info(void *token, UINT32 state, union sndinfo *info);
void gaelco_cg1v_get_info(void *token, UINT32 state, union sndinfo *info);
void gaelco_gae1_get_info(void *token, UINT32 state, union sndinfo *info);
void x1_010_get_info(void *token, UINT32 state, union sndinfo *info);
void multipcm_get_info(void *token, UINT32 state, union sndinfo *info);
void c6280_get_info(void *token, UINT32 state, union sndinfo *info);
void tia_get_info(void *token, UINT32 state, union sndinfo *info);
void sp0250_get_info(void *token, UINT32 state, union sndinfo *info);
void scsp_get_info(void *token, UINT32 state, union sndinfo *info);
void psxspu_get_info(void *token, UINT32 state, union sndinfo *info);
void ymf271_get_info(void *token, UINT32 state, union sndinfo *info);
void cdda_get_info(void *token, UINT32 state, union sndinfo *info);
void ics2115_get_info(void *token, UINT32 state, union sndinfo *info);
void st0016_get_info(void *token, UINT32 state, union sndinfo *info);
void c352_get_info(void *token, UINT32 state, union sndinfo *info);
void vrender0_get_info(void *token, UINT32 state, union sndinfo *info);
void votrax_get_info(void *token, UINT32 state, union sndinfo *info);

#ifdef MESS
void beep_get_info(void *token, UINT32 state, union sndinfo *info);
void speaker_get_info(void *token, UINT32 state, union sndinfo *info);
void wave_get_info(void *token, UINT32 state, union sndinfo *info);
void sid6581_get_info(void *token, UINT32 state, union sndinfo *info);
void sid8580_get_info(void *token, UINT32 state, union sndinfo *info);
void es5503_get_info(void *token, UINT32 state, union sndinfo *info);
#endif

void filter_volume_get_info(void *token, UINT32 state, union sndinfo *info);
void filter_rc_get_info(void *token, UINT32 state, union sndinfo *info);



/*************************************
 *
 *	The core list of sound interfaces
 *
 *************************************/

struct snd_interface sndintrf[SOUND_COUNT];

const struct
{
	int		sndtype;
	void	(*get_info)(void *token, UINT32 state, union sndinfo *info);
} sndintrf_map[] =
{
	{ SOUND_DUMMY, dummy_sound_get_info },
#if (HAS_CUSTOM)
	{ SOUND_CUSTOM, custom_get_info },
#endif
#if (HAS_SAMPLES)
	{ SOUND_SAMPLES, samples_get_info },
#endif
#if (HAS_DAC)
	{ SOUND_DAC, dac_get_info },
#endif
#if (HAS_DMADAC)
	{ SOUND_DMADAC, dmadac_get_info },
#endif
#if (HAS_DISCRETE)
	{ SOUND_DISCRETE, discrete_get_info },
#endif
#if (HAS_AY8910)
	{ SOUND_AY8910, ay8910_get_info },
#endif
#if (HAS_YM2203)
	{ SOUND_YM2203, ym2203_get_info },
#endif
#if (HAS_YM2151)
	{ SOUND_YM2151, ym2151_get_info },
#endif
#if (HAS_YM2608)
	{ SOUND_YM2608, ym2608_get_info },
#endif
#if (HAS_YM2610)
	{ SOUND_YM2610, ym2610_get_info },
#endif
#if (HAS_YM2610B)
	{ SOUND_YM2610B, ym2610b_get_info },
#endif
#if (HAS_YM2612)
	{ SOUND_YM2612, ym2612_get_info },
#endif
#if (HAS_YM3438)
	{ SOUND_YM3438, ym3438_get_info },
#endif
#if (HAS_YM2413)
	{ SOUND_YM2413, ym2413_get_info },
#endif
#if (HAS_YM3812)
	{ SOUND_YM3812, ym3812_get_info },
#endif
#if (HAS_YM3526)
	{ SOUND_YM3526, ym3526_get_info },
#endif
#if (HAS_YMZ280B)
	{ SOUND_YMZ280B, ymz280b_get_info },
#endif
#if (HAS_Y8950)
	{ SOUND_Y8950, y8950_get_info },
#endif
#if (HAS_SN76477)
	{ SOUND_SN76477, sn76477_get_info },
#endif
#if (HAS_SN76496)
	{ SOUND_SN76496, sn76496_get_info },
#endif
#if (HAS_POKEY)
	{ SOUND_POKEY, pokey_get_info },
#endif
#if (HAS_NES)
	{ SOUND_NES, nesapu_get_info },
#endif
#if (HAS_ASTROCADE)
	{ SOUND_ASTROCADE, astrocade_get_info },
#endif
#if (HAS_NAMCO)
	{ SOUND_NAMCO, namco_get_info },
#endif
#if (HAS_NAMCO_15XX)
	{ SOUND_NAMCO_15XX, namco_15xx_get_info },
#endif
#if (HAS_NAMCO_CUS30)
	{ SOUND_NAMCO_CUS30, namco_cus30_get_info },
#endif
#if (HAS_NAMCO_52XX)
	{ SOUND_NAMCO_52XX, namco_52xx_get_info },
#endif
#if (HAS_NAMCO_54XX)
	{ SOUND_NAMCO_54XX, namco_54xx_get_info },
#endif
#if (HAS_NAMCO_63701X)
	{ SOUND_NAMCO_63701X, namco_63701x_get_info },
#endif
#if (HAS_NAMCONA)
	{ SOUND_NAMCONA, namcona_get_info },
#endif
#if (HAS_TMS36XX)
	{ SOUND_TMS36XX, tms36xx_get_info },
#endif
#if (HAS_TMS5110)
	{ SOUND_TMS5110, tms5110_get_info },
#endif
#if (HAS_TMS5220)
	{ SOUND_TMS5220, tms5220_get_info },
#endif
#if (HAS_VLM5030)
	{ SOUND_VLM5030, vlm5030_get_info },
#endif
#if (HAS_OKIM6295)
	{ SOUND_OKIM6295, okim6295_get_info },
#endif
#if (HAS_MSM5205)
	{ SOUND_MSM5205, msm5205_get_info },
#endif
#if (HAS_MSM5232)
	{ SOUND_MSM5232, msm5232_get_info },
#endif
#if (HAS_UPD7759)
	{ SOUND_UPD7759, upd7759_get_info },
#endif
#if (HAS_HC55516)
	{ SOUND_HC55516, hc55516_get_info },
#endif
#if (HAS_K005289)
	{ SOUND_K005289, k005289_get_info },
#endif
#if (HAS_K007232)
	{ SOUND_K007232, k007232_get_info },
#endif
#if (HAS_K051649)
	{ SOUND_K051649, k051649_get_info },
#endif
#if (HAS_K053260)
	{ SOUND_K053260, k053260_get_info },
#endif
#if (HAS_K054539)
	{ SOUND_K054539, k054539_get_info },
#endif
#if (HAS_SEGAPCM)
	{ SOUND_SEGAPCM, segapcm_get_info },
#endif
#if (HAS_RF5C68)
	{ SOUND_RF5C68, rf5c68_get_info },
#endif
#if (HAS_CEM3394)
	{ SOUND_CEM3394, cem3394_get_info },
#endif
#if (HAS_C140)
	{ SOUND_C140, c140_get_info },
#endif
#if (HAS_QSOUND)
	{ SOUND_QSOUND, qsound_get_info },
#endif
#if (HAS_SAA1099)
	{ SOUND_SAA1099, saa1099_get_info },
#endif
#if (HAS_IREMGA20)
	{ SOUND_IREMGA20, iremga20_get_info },
#endif
#if (HAS_ES5505)
	{ SOUND_ES5505, es5505_get_info },
#endif
#if (HAS_ES5506)
	{ SOUND_ES5506, es5506_get_info },
#endif
#if (HAS_BSMT2000)
	{ SOUND_BSMT2000, bsmt2000_get_info },
#endif
#if (HAS_YMF262)
	{ SOUND_YMF262, ymf262_get_info },
#endif
#if (HAS_YMF278B)
	{ SOUND_YMF278B, ymf278b_get_info },
#endif
#if (HAS_GAELCO_CG1V)
	{ SOUND_GAELCO_CG1V, gaelco_cg1v_get_info },
#endif
#if (HAS_GAELCO_GAE1)
	{ SOUND_GAELCO_GAE1, gaelco_gae1_get_info },
#endif
#if (HAS_X1_010)
	{ SOUND_X1_010, x1_010_get_info },
#endif
#if (HAS_MULTIPCM)
	{ SOUND_MULTIPCM, multipcm_get_info },
#endif
#if (HAS_C6280)
	{ SOUND_C6280, c6280_get_info },
#endif
#if (HAS_TIA)
	{ SOUND_TIA, tia_get_info },
#endif
#if (HAS_SP0250)
	{ SOUND_SP0250, sp0250_get_info },
#endif
#if (HAS_SCSP)
	{ SOUND_SCSP, scsp_get_info },
#endif
#if (HAS_PSXSPU)
	{ SOUND_PSXSPU, psxspu_get_info },
#endif
#if (HAS_YMF271)
	{ SOUND_YMF271, ymf271_get_info },
#endif
#if (HAS_CDDA)
	{ SOUND_CDDA, cdda_get_info },
#endif
#if (HAS_ICS2115)
	{ SOUND_ICS2115, ics2115_get_info },
#endif
#if (HAS_ST0016)
	{ SOUND_ST0016, st0016_get_info },
#endif
#if (HAS_C352)
	{ SOUND_C352, c352_get_info },
#endif
#if (HAS_VRENDER0)
	{ SOUND_VRENDER0, vrender0_get_info },
#endif
#if (HAS_VOTRAX)
	{ SOUND_VORTRAX, votrax_get_info },
#endif

#ifdef MESS
#if (HAS_BEEP)
	{ SOUND_BEEP, beep_get_info },
#endif
#if (HAS_SPEAKER)
	{ SOUND_SPEAKER, speaker_get_info },
#endif
#if (HAS_WAVE)
	{ SOUND_WAVE, wave_get_info },
#endif
#if (HAS_SID6581)
	{ SOUND_SID6581, sid6581_get_info },
#endif
#if (HAS_SID8580)
	{ SOUND_SID8580, sid8580_get_info },
#endif
#if (HAS_ES5503)
	{ SOUND_ES5503, es5503_get_info },
#endif
#endif

	{ SOUND_FILTER_VOLUME, filter_volume_get_info },
	{ SOUND_FILTER_RC, filter_rc_get_info },
};



/*************************************
 *
 *	Macros to help verify sound number
 *
 *************************************/

#define VERIFY_SNDNUM(retval, name)							\
	if (sndnum < 0 || sndnum >= totalsnd)					\
	{														\
		logerror(#name "() called for invalid sound num!\n");\
		return retval;										\
	}

#define VERIFY_SNDNUM_VOID(name)							\
	if (sndnum < 0 || sndnum >= totalsnd)					\
	{														\
		logerror(#name "() called for invalid sound num!\n");\
		return;												\
	}



/*************************************
 *
 *	Macros to help verify sound type+index
 *
 *************************************/

#define VERIFY_SNDTI(retval, name)							\
	if (sndtype < 0 || sndtype >= SOUND_COUNT)				\
	{														\
		logerror(#name "() called for invalid sound type!\n");\
		return retval;										\
	} \
	if (sndindex < 0 || sndindex >= totalsnd || !sound_matrix[sndtype][sndindex]) \
	{														\
		logerror(#name "() called for invalid sound index!\n");\
		return retval;										\
	}

#define VERIFY_SNDTI_VOID(name)								\
	if (sndtype < 0 || sndtype >= SOUND_COUNT)				\
	{														\
		logerror(#name "() called for invalid sound type!\n");\
		return;												\
	} \
	if (sndindex < 0 || sndindex >= totalsnd || !sound_matrix[sndtype][sndindex]) \
	{														\
		logerror(#name "() called for invalid sound index!\n");\
		return;												\
	}



/*************************************
 *
 *	Macros to help verify sound type
 *
 *************************************/

#define VERIFY_SNDTYPE(retval, name)						\
	if (sndtype < 0 || sndtype >= SOUND_COUNT)				\
	{														\
		logerror(#name "() called for invalid sound type!\n");\
		return retval;										\
	}

#define VERIFY_SNDTYPE_VOID(name)							\
	if (sndtype < 0 || sndtype >= SOUND_COUNT)				\
	{														\
		logerror(#name "() called for invalid sound type!\n");\
		return;												\
	}



/*************************************
 *
 *	Global variables
 *
 *************************************/

static mame_timer *sound_update_timer;

static int totalsnd;
static struct sound_info sound[MAX_SOUND];
static struct sound_info *current_sound_start;
static UINT8 sound_matrix[SOUND_COUNT][MAX_SOUND];

static int totalspeakers;
static struct speaker_info speaker[MAX_SPEAKER];

static INT16 *finalmix;
static INT32 *leftmix, *rightmix;
static int samples_this_frame;
static int global_sound_enabled;
static int nosound_mode;

static UINT16 latch_clear_value = 0x00;
static UINT16 latched_value[4];
static UINT8 latch_read[4];

static void *wavfile;



/*************************************
 *
 *	Prototypes
 *
 *************************************/

static int start_sound_chips(void);
static int start_speakers(void);
static int route_sound(void);
static void mixer_update(void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length);



/*************************************
 *
 *	Start up the sound system
 *
 *************************************/

int sound_start(void)
{
	/* handle -nosound */
	nosound_mode = (Machine->sample_rate == 0);
	if (nosound_mode)
		Machine->sample_rate = 11025;
	
	/* initialize the interfaces */
	VPRINTF(("sndintrf_init\n"));
	sndintrf_init();
	
	/* count the speakers */
	for (totalspeakers = 0; Machine->drv->speaker[totalspeakers].tag; totalspeakers++) ;
	VPRINTF(("total speakers = %d\n", totalspeakers));

	/* initialize the OSD layer */
	VPRINTF(("osd_start_audio_stream\n"));
	samples_this_frame = osd_start_audio_stream(1);
	if (!samples_this_frame)
		return 1;
	
	/* allocate memory for mix buffers */
	finalmix = auto_malloc(Machine->sample_rate * sizeof(*finalmix));
	leftmix = auto_malloc(Machine->sample_rate * sizeof(*leftmix));
	rightmix = auto_malloc(Machine->sample_rate * sizeof(*rightmix));

	/* allocate a global timer for sound timing */
	sound_update_timer = mame_timer_alloc(NULL);

	/* initialize the streams engine */
	VPRINTF(("streams_init\n"));
	streams_init();
	
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
	
	global_sound_enabled = 1;
	return 0;
}



/*************************************
 *
 *	Initialize the global interface
 *
 *************************************/

void sndintrf_init(void)
{
	int mapindex;

	/* reset the sndintrf array */
	memset(sndintrf, 0, sizeof(sndintrf));

	/* build the sndintrf array */
	for (mapindex = 0; mapindex < sizeof(sndintrf_map) / sizeof(sndintrf_map[0]); mapindex++)
	{
		int sndtype = sndintrf_map[mapindex].sndtype;
		struct snd_interface *intf = &sndintrf[sndtype];
		union sndinfo info;

		/* start with the get_info routine */
		intf->get_info = sndintrf_map[mapindex].get_info;

		/* bootstrap the rest of the function pointers */
		info.set_info = NULL; (*intf->get_info)(NULL, SNDINFO_PTR_SET_INFO,    &info); intf->set_info = info.set_info;
		info.start = NULL;    (*intf->get_info)(NULL, SNDINFO_PTR_START,       &info); intf->start = info.start;
		info.stop = NULL;     (*intf->get_info)(NULL, SNDINFO_PTR_STOP,        &info); intf->stop = info.stop;
		info.reset = NULL;    (*intf->get_info)(NULL, SNDINFO_PTR_RESET,       &info); intf->reset = info.reset;
	}

	/* fill in any empty entries with the dummy sound */
	for (mapindex = 0; mapindex < SOUND_COUNT; mapindex++)
		if (sndintrf[mapindex].get_info == NULL)
			sndintrf[mapindex] = sndintrf[SOUND_DUMMY];
}



/*************************************
 *
 *	Start up all the sound chips
 *
 *************************************/

static int start_sound_chips(void)
{
	int sndnum;
	
	/* reset the matrix and sound array */
	memset(sound_matrix, 0, sizeof(sound_matrix));
	memset(sound, 0, sizeof(sound));

	/* start up all the sound chips */
	for (sndnum = 0; sndnum < MAX_SOUND; sndnum++)
	{
		const struct MachineSound *msound = &Machine->drv->sound[sndnum];
		struct sound_info *info;
		int index;

		/* stop when we hit an empty entry */
		if (msound->sound_type == 0)
			break;
		totalsnd++;

		/* add to the matrix */
		for (index = 0; index < MAX_SOUND; index++)
			if (sound_matrix[msound->sound_type][index] == 0)
			{
				sound_matrix[msound->sound_type][index] = sndnum + 1;
				break;
			}

		/* zap all the info */
		info = &sound[sndnum];
		memset(info, 0, sizeof(*info));
		
		/* copy in all the relevant info */
		info->sound = msound;
		info->sndindex = index;
		info->intf = sndintrf[msound->sound_type];
		info->token = NULL;
		
		/* start the chip, tagging all its streams */
		VPRINTF(("sndnum = %d -- sound_type = %d, index = %d\n", sndnum, msound->sound_type, index));
		current_sound_start = info;
		streams_set_tag(info);
		info->token = (*info->intf.start)(index, msound->clock, msound->config);
		current_sound_start = NULL;
		VPRINTF(("  token = %p\n", info->token));
		
		/* if that failed, die */
		if (!info->token)
			return 1;
		
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



/*************************************
 *
 *	Start up the speakers
 *
 *************************************/

static int start_speakers(void)
{
	/* reset the speaker array */
	memset(speaker, 0, sizeof(speaker));

	/* start up all the speakers */
	for (totalspeakers = 0; totalspeakers < MAX_SPEAKER; totalspeakers++)
	{
		const struct MachineSpeaker *mspeaker = &Machine->drv->speaker[totalspeakers];
		struct speaker_info *info;
		
		/* stop when we hit an empty entry */
		if (!mspeaker->tag)
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



/*************************************
 *
 *	Helpers to locate speakers or
 *	sound chips by tag
 *
 *************************************/

static struct speaker_info *find_speaker_by_tag(const char *tag)
{
	int spknum;
	
	/* attempt to find the speaker in our list */
	for (spknum = 0; spknum < totalspeakers; spknum++)
		if (!strcmp(speaker[spknum].speaker->tag, tag))
			return &speaker[spknum];
	return NULL;
}


static struct sound_info *find_sound_by_tag(const char *tag)
{
	int sndnum;

	/* attempt to find the speaker in our list */
	for (sndnum = 0; sndnum < totalsnd; sndnum++)
		if (sound[sndnum].sound->tag && !strcmp(sound[sndnum].sound->tag, tag))
			return &sound[sndnum];
	return NULL;
}



/*************************************
 *
 *	Route the sounds to their
 *	destinations
 *
 *************************************/

static int route_sound(void)
{
	int sndnum, spknum, routenum, outputnum;
	
	/* iterate over all the sound chips */
	for (sndnum = 0; sndnum < totalsnd; sndnum++)
	{
		struct sound_info *info = &sound[sndnum];
		
		/* iterate over all routes */
		for (routenum = 0; routenum < info->sound->routes; routenum++)
		{
			const struct MachineSoundRoute *mroute = &info->sound->route[routenum];
			struct speaker_info *speaker;
			struct sound_info *sound;
		
			/* find the target */
			speaker = find_speaker_by_tag(mroute->target);
			sound = find_sound_by_tag(mroute->target);
			
			/* if neither found, it's fatal */
			if (!speaker && !sound)
			{
				printf("Sound route \"%s\" not found!\n", mroute->target);
				return 1;
			}
			
			/* if we got a speaker, bump its input count */
			if (speaker)
			{
				if (mroute->output >= 0 && mroute->output < info->outputs)
					speaker->inputs++;
				else if (mroute->output == ALL_OUTPUTS)
					speaker->inputs += info->outputs;
			}
		}
	}
	
	/* now allocate the mixers and input data */
	streams_set_tag(NULL);
	for (spknum = 0; spknum < totalspeakers; spknum++)
	{
		struct speaker_info *info = &speaker[spknum];
		if (info->inputs)
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
		struct sound_info *info = &sound[sndnum];
		
		/* iterate over all routes */
		for (routenum = 0; routenum < info->sound->routes; routenum++)
		{
			const struct MachineSoundRoute *mroute = &info->sound->route[routenum];
			struct speaker_info *speaker;
			struct sound_info *sound;
		
			/* find the target */
			speaker = find_speaker_by_tag(mroute->target);
			sound = find_sound_by_tag(mroute->target);
			
			/* if it's a speaker, set the input */
			if (speaker)
			{
				for (outputnum = 0; outputnum < info->outputs; outputnum++)
					if (mroute->output == outputnum || mroute->output == ALL_OUTPUTS)
					{
						char namebuf[256];
					
						/* fill in the input data on this speaker */
						speaker->input[speaker->inputs].gain = mroute->gain;
						speaker->input[speaker->inputs].default_gain = mroute->gain;
						sprintf(namebuf, "%s:%s #%d.%d", speaker->speaker->tag, sndnum_name(sndnum), info->sndindex, outputnum);
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



/*************************************
 *
 *	Shut down the sound system
 *
 *************************************/

void sound_stop(void)
{
	int sndnum;
	
	if (wavfile)
		wav_close(wavfile);

#ifdef MAME_DEBUG
{
	int spknum;
	
	/* log the maximum sample values for all speakers */
	for (spknum = 0; spknum < totalspeakers; spknum++)
	{
		struct speaker_info *spk = &speaker[spknum];
		printf("Speaker \"%s\" - max = %d (gain *= %f) - %d%% samples clipped\n", spk->speaker->tag, spk->max_sample, 32767.0 / (spk->max_sample ? spk->max_sample : 1), (int)((double)spk->clipped_samples * 100.0 / spk->total_samples));
	}
}
#endif

	/* stop all the sound chips */
	for (sndnum = 0; sndnum < MAX_SOUND; sndnum++)
		if (Machine->drv->sound[sndnum].sound_type != 0)
		{
			struct sound_info *info = &sound[sndnum];
			if (info->intf.stop)
				(*info->intf.stop)(info->token);
		}
	
	/* stop the OSD code */
	osd_stop_audio_stream();

	/* reset variables */
	totalspeakers = 0;
	totalsnd = 0;
	memset(&speaker, 0, sizeof(speaker));
	memset(&sound, 0, sizeof(sound));
}



/*************************************
 *
 *	Mixer update
 *
 *************************************/

static void mixer_update(void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length)
{
	struct speaker_info *speaker = param;
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



/*************************************
 *
 *	Reset all sound chips
 *
 *************************************/

void sound_reset(void)
{
	int sndnum;

	/* reset all the sound chips */
	for (sndnum = 0; sndnum < MAX_SOUND; sndnum++)
		if (Machine->drv->sound[sndnum].sound_type != 0)
		{
			struct sound_info *info = &sound[sndnum];
			if (info->intf.reset)
				(*info->intf.reset)(info->token);
		}
}



/*************************************
 *
 *	Early token registration
 *
 *************************************/

void sound_register_token(void *token)
{
	if (current_sound_start)
		current_sound_start->token = token;
}



/*************************************
 *
 *	Once/frame update routine
 *
 *************************************/

void sound_frame_update(void)
{
	int sample, spknum;
	
	VPRINTF(("sound_frame_update\n"));
	
	profiler_mark(PROFILER_SOUND);
	
	/* reset the mixing streams */
	memset(leftmix, 0, samples_this_frame * sizeof(*leftmix));
	memset(rightmix, 0, samples_this_frame * sizeof(*rightmix));
	
	/* force all the speaker streams to generate the proper number of samples */
	for (spknum = 0; spknum < totalspeakers; spknum++)
	{
		struct speaker_info *spk = &speaker[spknum];
		stream_sample_t *stream_buf;
		
		/* get the output buffer */
		if (spk->mixer_stream)
		{
			stream_buf = stream_consume_output(spk->mixer_stream, 0, samples_this_frame);

#ifdef MAME_DEBUG
			/* debug version: keep track of the maximum sample */
			for (sample = 0; sample < samples_this_frame; sample++)
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
					for (sample = 0; sample < samples_this_frame; sample++)
					{
						leftmix[sample] += stream_buf[sample];
						rightmix[sample] += stream_buf[sample];
					}

				/* if the speaker is to the left, send only to the left */
				else if (spk->speaker->x < 0)
					for (sample = 0; sample < samples_this_frame; sample++)
						leftmix[sample] += stream_buf[sample];
				
				/* if the speaker is to the right, send only to the right */
				else
					for (sample = 0; sample < samples_this_frame; sample++)
						rightmix[sample] += stream_buf[sample];
			}
		}
	}
	
	/* now downmix the final result */
	for (sample = 0; sample < samples_this_frame; sample++)
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

	if (wavfile)
		wav_add_data_16(wavfile, finalmix, samples_this_frame * 2);

	/* play the result */
	samples_this_frame = osd_update_audio_stream(finalmix);

	/* update the streamer */
	streams_frame_update();

	/* reset the timer to resync for this frame */
	mame_timer_adjust(sound_update_timer, time_never, 0, time_never);

	profiler_mark(PROFILER_END);
}



/*************************************
 *
 *	Compute the fractional part of
 *	value based on how far along we
 *	are within the current frame
 *
 *************************************/

int sound_scalebufferpos(int value)
{
	mame_time elapsed = mame_timer_timeelapsed(sound_update_timer);
	int result;
	
	/* clamp to protect against negative time */
	if (elapsed.seconds < 0)
		elapsed = time_zero;
	result = (int)((double)value * mame_time_to_double(elapsed) * Machine->refresh_rate);

	if (value >= 0)
		return (result < value) ? result : value;
	else
		return (result > value) ? result : value;
}



/*************************************
 *
 *	Helpers for sound engines
 *
 *************************************/

int sndti_to_sndnum(int type, int index)
{
	return sound_matrix[type][index] - 1;
}



/*************************************
 *
 *	Global sound enable/disable
 *
 *************************************/

void sound_global_enable(int enable)
{
	global_sound_enabled = enable;
}



/*************************************
 *
 *	Sound chip resetting
 *
 *************************************/

void sndti_reset(int type, int index)
{
	int sndnum = sound_matrix[type][index] - 1;
	if (sndnum >= 0 && sound[sndnum].intf.reset)
		(*sound[sndnum].intf.reset)(sound[sndnum].token);
}



/*************************************
 *
 *	Output gain get/set
 *
 *************************************/

void sndti_set_output_gain(int type, int index, int output, float gain)
{
	int sndnum = sound_matrix[type][index] - 1;

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



/*************************************
 *
 *	Get the number of user gain controls
 *
 *************************************/

int sound_get_user_gain_count(void)
{
	int count = 0, speakernum;
	
	/* count up the number of speaker inputs */
	for (speakernum = 0; speakernum < totalspeakers; speakernum++)
		count += speaker[speakernum].inputs;
	return count;
}



/*************************************
 *
 *	Set the nth user gain value
 *
 *************************************/

void sound_set_user_gain(int index, float gain)
{
	int count = 0, speakernum;
	
	/* count up the number of speaker inputs */
	for (speakernum = 0; speakernum < totalspeakers; speakernum++)
	{
		if (index < count + speaker[speakernum].inputs)
		{
			int inputnum = index - count;
			speaker[speakernum].input[inputnum].gain = gain;
			stream_set_input_gain(speaker[speakernum].mixer_stream, inputnum, gain);
			return;
		}
		count += speaker[speakernum].inputs;
	}
}



/*************************************
 *
 *	Get the nth user gain value
 *
 *************************************/

float sound_get_user_gain(int index)
{
	int count = 0, speakernum;
	
	/* count up the number of speaker inputs */
	for (speakernum = 0; speakernum < totalspeakers; speakernum++)
	{
		if (index < count + speaker[speakernum].inputs)
		{
			int inputnum = index - count;
			return speaker[speakernum].input[inputnum].gain;
		}
		count += speaker[speakernum].inputs;
	}
	return 0;
}



/*************************************
 *
 *	Get the nth user default gain value
 *
 *************************************/

float sound_get_default_gain(int index)
{
	int count = 0, speakernum;
	
	/* count up the number of speaker inputs */
	for (speakernum = 0; speakernum < totalspeakers; speakernum++)
	{
		if (index < count + speaker[speakernum].inputs)
		{
			int inputnum = index - count;
			return speaker[speakernum].input[inputnum].default_gain;
		}
		count += speaker[speakernum].inputs;
	}
	return 0;
}



/*************************************
 *
 *	Get the nth user gain name
 *
 *************************************/

const char *sound_get_user_gain_name(int index)
{
	int count = 0, speakernum;
	
	/* count up the number of speaker inputs */
	for (speakernum = 0; speakernum < totalspeakers; speakernum++)
	{
		if (index < count + speaker[speakernum].inputs)
		{
			int inputnum = index - count;
			return speaker[speakernum].input[inputnum].name;
		}
		count += speaker[speakernum].inputs;
	}
	return NULL;
}



/*************************************
 *
 *	Interfaces to a specific sound chip
 *
 *************************************/

/*--------------------------
 	Get info accessors
--------------------------*/

INT64 sndnum_get_info_int(int sndnum, UINT32 state)
{
	union sndinfo info;

	VERIFY_SNDNUM(0, sndnum_get_info_int);
	info.i = 0;
	(*sound[sndnum].intf.get_info)(sound[sndnum].token, state, &info);
	return info.i;
}

void *sndnum_get_info_ptr(int sndnum, UINT32 state)
{
	union sndinfo info;

	VERIFY_SNDNUM(0, sndnum_get_info_ptr);
	info.p = NULL;
	(*sound[sndnum].intf.get_info)(sound[sndnum].token, state, &info);
	return info.p;
}

genf *sndnum_get_info_fct(int sndnum, UINT32 state)
{
	union sndinfo info;

	VERIFY_SNDNUM(0, sndnum_get_info_fct);
	info.f = NULL;
	(*sound[sndnum].intf.get_info)(sound[sndnum].token, state, &info);
	return info.f;
}

const char *sndnum_get_info_string(int sndnum, UINT32 state)
{
	union sndinfo info;

	VERIFY_SNDNUM(0, sndnum_get_info_string);
	info.s = NULL;
	(*sound[sndnum].intf.get_info)(sound[sndnum].token, state, &info);
	return info.s;
}


/*--------------------------
 	Set info accessors
--------------------------*/

void sndnum_set_info_int(int sndnum, UINT32 state, INT64 data)
{
	union sndinfo info;
	VERIFY_SNDNUM_VOID(sndnum_set_info_int);
	info.i = data;
	(*sound[sndnum].intf.set_info)(sound[sndnum].token, state, &info);
}

void sndnum_set_info_ptr(int sndnum, UINT32 state, void *data)
{
	union sndinfo info;
	VERIFY_SNDNUM_VOID(sndnum_set_info_ptr);
	info.p = data;
	(*sound[sndnum].intf.set_info)(sound[sndnum].token, state, &info);
}

void sndnum_set_info_fct(int sndnum, UINT32 state, genf *data)
{
	union sndinfo info;
	VERIFY_SNDNUM_VOID(sndnum_set_info_ptr);
	info.f = data;
	(*sound[sndnum].intf.set_info)(sound[sndnum].token, state, &info);
}


/*--------------------------
 	Misc accessors
--------------------------*/

int sndnum_clock(int sndnum)
{
	VERIFY_SNDNUM(0, sndnum_clock);
	return sound[sndnum].sound->clock;
}

void *sndnum_token(int sndnum)
{
	VERIFY_SNDNUM(NULL, sndnum_token);
	return sound[sndnum].token;
}



/*************************************
 *
 *	Interfaces to a specific sound chip
 *
 *************************************/

/*--------------------------
 	Get info accessors
--------------------------*/

INT64 sndti_get_info_int(int sndtype, int sndindex, UINT32 state)
{
	union sndinfo info;
	int sndnum;

	VERIFY_SNDTI(0, sndti_get_info_int);
	sndnum = sound_matrix[sndtype][sndindex] - 1;
	info.i = 0;
	(*sound[sndnum].intf.get_info)(sound[sndnum].token, state, &info);
	return info.i;
}

void *sndti_get_info_ptr(int sndtype, int sndindex, UINT32 state)
{
	union sndinfo info;
	int sndnum;

	VERIFY_SNDTI(0, sndti_get_info_ptr);
	sndnum = sound_matrix[sndtype][sndindex] - 1;
	info.p = NULL;
	(*sound[sndnum].intf.get_info)(sound[sndnum].token, state, &info);
	return info.p;
}

genf *sndti_get_info_fct(int sndtype, int sndindex, UINT32 state)
{
	union sndinfo info;
	int sndnum;

	VERIFY_SNDTI(0, sndti_get_info_fct);
	sndnum = sound_matrix[sndtype][sndindex] - 1;
	info.f = NULL;
	(*sound[sndnum].intf.get_info)(sound[sndnum].token, state, &info);
	return info.f;
}

const char *sndti_get_info_string(int sndtype, int sndindex, UINT32 state)
{
	union sndinfo info;
	int sndnum;

	VERIFY_SNDTI(0, sndti_get_info_string);
	sndnum = sound_matrix[sndtype][sndindex] - 1;
	info.s = NULL;
	(*sound[sndnum].intf.get_info)(sound[sndnum].token, state, &info);
	return info.s;
}


/*--------------------------
 	Set info accessors
--------------------------*/

void sndti_set_info_int(int sndtype, int sndindex, UINT32 state, INT64 data)
{
	union sndinfo info;
	int sndnum;

	VERIFY_SNDTI_VOID(sndti_set_info_int);
	sndnum = sound_matrix[sndtype][sndindex] - 1;
	info.i = data;
	(*sound[sndnum].intf.set_info)(sound[sndnum].token, state, &info);
}

void sndti_set_info_ptr(int sndtype, int sndindex, UINT32 state, void *data)
{
	union sndinfo info;
	int sndnum;

	VERIFY_SNDTI_VOID(sndti_set_info_ptr);
	sndnum = sound_matrix[sndtype][sndindex] - 1;
	info.p = data;
	(*sound[sndnum].intf.set_info)(sound[sndnum].token, state, &info);
}

void sndti_set_info_fct(int sndtype, int sndindex, UINT32 state, genf *data)
{
	union sndinfo info;
	int sndnum;

	VERIFY_SNDTI_VOID(sndti_set_info_ptr);
	sndnum = sound_matrix[sndtype][sndindex] - 1;
	info.f = data;
	(*sound[sndnum].intf.set_info)(sound[sndnum].token, state, &info);
}


/*--------------------------
 	Misc accessors
--------------------------*/

int sndti_clock(int sndtype, int sndindex)
{
	int sndnum;
	VERIFY_SNDTI(0, sndti_clock);
	sndnum = sound_matrix[sndtype][sndindex] - 1;
	return sound[sndnum].sound->clock;
}

void *sndti_token(int sndtype, int sndindex)
{
	int sndnum;
	VERIFY_SNDTI(0, sndti_token);
	sndnum = sound_matrix[sndtype][sndindex] - 1;
	return sound[sndnum].token;
}



/*************************************
 *
 *	Interfaces to a specific sound type
 *
 *************************************/

/*--------------------------
 	Get info accessors
--------------------------*/

INT64 sndtype_get_info_int(int sndtype, UINT32 state)
{
	union sndinfo info;

	VERIFY_SNDTYPE(0, sndtype_get_info_int);
	info.i = 0;
	(*sndintrf[sndtype].get_info)(NULL, state, &info);
	return info.i;
}

void *sndtype_get_info_ptr(int sndtype, UINT32 state)
{
	union sndinfo info;

	VERIFY_SNDTYPE(0, sndtype_get_info_ptr);
	info.p = NULL;
	(*sndintrf[sndtype].get_info)(NULL, state, &info);
	return info.p;
}

genf *sndtype_get_info_fct(int sndtype, UINT32 state)
{
	union sndinfo info;

	VERIFY_SNDTYPE(0, sndtype_get_info_fct);
	info.f = NULL;
	(*sndintrf[sndtype].get_info)(NULL, state, &info);
	return info.f;
}

const char *sndtype_get_info_string(int sndtype, UINT32 state)
{
	union sndinfo info;

	VERIFY_SNDTYPE(0, sndtype_get_info_string);
	info.s = NULL;
	(*sndintrf[sndtype].get_info)(NULL, state, &info);
	return info.s;
}



/*************************************
 *
 *	Dummy sound interfaces
 *
 *************************************/
 
static void *dummy_sound_start(int index, int clock, const void *config)
{
	logerror("Warning: starting a dummy sound core -- you are missing a hookup in sndintrf.c!\n");
	return auto_malloc(1);
}


static void dummy_sound_set_info(void *token, UINT32 state, union sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void dummy_sound_get_info(void *token, UINT32 state, union sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = dummy_sound_set_info;	break;
		case SNDINFO_PTR_START:							info->start = dummy_sound_start;		break;
		case SNDINFO_PTR_STOP:							/* Nothing */							break;
		case SNDINFO_PTR_RESET:							/* Nothing */							break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "Dummy";						break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Dummy";						break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}



/***************************************************************************

  Many games use a master-slave CPU setup. Typically, the main CPU writes
  a command to some register, and then writes to another register to trigger
  an interrupt on the slave CPU (the interrupt might also be triggered by
  the first write). The slave CPU, notified by the interrupt, goes and reads
  the command.

***************************************************************************/

static void latch_callback(int param)
{
	UINT16 value = param >> 8;
	int which = param & 0xff;

	/* if the latch hasn't been read and the value is changed, log a warning */
	if (!latch_read[which] && latched_value[which] != value)
		logerror("Warning: sound latch %d written before being read. Previous: %02x, new: %02x\n", which, latched_value[which], value);

	/* store the new value and mark it not read */
	latched_value[which] = value;
	latch_read[which] = 0;
}


INLINE void latch_w(int which, UINT16 value)
{
	mame_timer_set(time_zero, which | (value << 8), latch_callback);
}


INLINE UINT16 latch_r(int which)
{
	latch_read[which] = 1;
	return latched_value[which];
}


INLINE void latch_clear(int which)
{
	latched_value[which] = latch_clear_value;
}


WRITE8_HANDLER( soundlatch_w )        { latch_w(0, data); }
WRITE16_HANDLER( soundlatch_word_w )  { latch_w(0, data); }
WRITE8_HANDLER( soundlatch2_w )       { latch_w(1, data); }
WRITE16_HANDLER( soundlatch2_word_w ) { latch_w(1, data); }
WRITE8_HANDLER( soundlatch3_w )       { latch_w(2, data); }
WRITE16_HANDLER( soundlatch3_word_w ) { latch_w(2, data); }
WRITE8_HANDLER( soundlatch4_w )       { latch_w(3, data); }
WRITE16_HANDLER( soundlatch4_word_w ) { latch_w(3, data); }

READ8_HANDLER( soundlatch_r )         { return latch_r(0); }
READ16_HANDLER( soundlatch_word_r )   { return latch_r(0); }
READ8_HANDLER( soundlatch2_r )        { return latch_r(1); }
READ16_HANDLER( soundlatch2_word_r )  { return latch_r(1); }
READ8_HANDLER( soundlatch3_r )        { return latch_r(2); }
READ16_HANDLER( soundlatch3_word_r )  { return latch_r(2); }
READ8_HANDLER( soundlatch4_r )        { return latch_r(3); }
READ16_HANDLER( soundlatch4_word_r )  { return latch_r(3); }

WRITE8_HANDLER( soundlatch_clear_w )  { latch_clear(0); }
WRITE8_HANDLER( soundlatch2_clear_w ) { latch_clear(1); }
WRITE8_HANDLER( soundlatch3_clear_w ) { latch_clear(2); }
WRITE8_HANDLER( soundlatch4_clear_w ) { latch_clear(3); }

void soundlatch_setclearedvalue(int value)
{
	latch_clear_value = value;
}
