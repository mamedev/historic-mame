#include "driver.h"


/***************************************************************************

  Many games use a master-slave CPU setup. Typically, the main CPU writes
  a command to some register, and then writes to another register to trigger
  an interrupt on the slave CPU (the interrupt might also be triggered by
  the first write). The slave CPU, notified by the interrupt, goes and reads
  the command.

***************************************************************************/

static int cleared_value = 0x00;

static int latch,read_debug;


static void soundlatch_callback(int param)
{
if (errorlog && read_debug == 0 && latch != param)
	fprintf(errorlog,"Warning: sound latch written before being read. Previous: %02x, new: %02x\n",latch,param);
	latch = param;
	read_debug = 0;
}

void soundlatch_w(int offset,int data)
{
	/* make all the CPUs synchronize, and only AFTER that write the new command to the latch */
	timer_set(TIME_NOW,data,soundlatch_callback);
}

int soundlatch_r(int offset)
{
	read_debug = 1;
	return latch;
}

void soundlatch_clear_w(int offset, int data)
{
	latch = cleared_value;
}


static int latch2,read_debug2;

static void soundlatch2_callback(int param)
{
if (errorlog && read_debug2 == 0 && latch2 != param)
	fprintf(errorlog,"Warning: sound latch 2 written before being read. Previous: %02x, new: %02x\n",latch2,param);
	latch2 = param;
	read_debug2 = 0;
}

void soundlatch2_w(int offset,int data)
{
	/* make all the CPUs synchronize, and only AFTER that write the new command to the latch */
	timer_set(TIME_NOW,data,soundlatch2_callback);
}

int soundlatch2_r(int offset)
{
	read_debug2 = 1;
	return latch2;
}

void soundlatch2_clear_w(int offset, int data)
{
	latch2 = cleared_value;
}


static int latch3,read_debug3;

static void soundlatch3_callback(int param)
{
if (errorlog && read_debug3 == 0 && latch3 != param)
	fprintf(errorlog,"Warning: sound latch 3 written before being read. Previous: %02x, new: %02x\n",latch3,param);
	latch3 = param;
	read_debug3 = 0;
}

void soundlatch3_w(int offset,int data)
{
	/* make all the CPUs synchronize, and only AFTER that write the new command to the latch */
	timer_set(TIME_NOW,data,soundlatch3_callback);
}

int soundlatch3_r(int offset)
{
	read_debug3 = 1;
	return latch3;
}

void soundlatch3_clear_w(int offset, int data)
{
	latch3 = cleared_value;
}


static int latch4,read_debug4;

static void soundlatch4_callback(int param)
{
if (errorlog && read_debug4 == 0 && latch4 != param)
	fprintf(errorlog,"Warning: sound latch 4 written before being read. Previous: %02x, new: %02x\n",latch2,param);
	latch4 = param;
	read_debug4 = 0;
}

void soundlatch4_w(int offset,int data)
{
	/* make all the CPUs synchronize, and only AFTER that write the new command to the latch */
	timer_set(TIME_NOW,data,soundlatch4_callback);
}

int soundlatch4_r(int offset)
{
	read_debug4 = 1;
	return latch4;
}

void soundlatch4_clear_w(int offset, int data)
{
	latch4 = cleared_value;
}


void soundlatch_setclearedvalue(int value)
{
	cleared_value = value;
}

/***************************************************************************

  This function returns top of reserved sound channels

***************************************************************************/
static int reserved_channel = 0;

int get_play_channels( int request )
{
	int ret_value = reserved_channel;

	reserved_channel += request;
	return ret_value;
}

void reset_play_channels(void)
{
	reserved_channel = 0;
}





/***************************************************************************



***************************************************************************/

static void *sound_update_timer;
static double refresh_period;
static double refresh_period_inv;


struct snd_interface
{
	unsigned sound_num;						/* ID */
	const char *name;						/* description */
	int (*chips_num)(const void *intf);		/* returns number of chips if applicable */
	int (*chips_clock)(const void *intf);	/* returns chips clock if applicable */
	int (*start)(const void *intf);			/* starts sound emulation */
	void (*stop)(void);						/* stops sound emulation */
	void (*update)(void);					/* updates emulation once per frame if necessary */
	void (*reset)(void);					/* resets sound emulation */
};


#if (HAS_CUSTOM)
static const struct CustomSound_interface *cust_intf;

int custom_sh_start(const struct CustomSound_interface *intf)
{
	cust_intf = intf;

	if (intf->sh_start)
		return (*intf->sh_start)();
	else return 0;
}
void custom_sh_stop(void)
{
	if (cust_intf->sh_stop) (*cust_intf->sh_stop)();
}
void custom_sh_update(void)
{
	if (cust_intf->sh_update) (*cust_intf->sh_update)();
}
#endif
#if (HAS_DAC)
int DAC_num(const void* interface) { return ((struct DACinterface*)interface)->num; }
#endif
#if (HAS_ADPCM)
int ADPCM_num(const void* interface) { return ((struct ADPCMinterface*)interface)->num; }
#endif
#if (HAS_OKIM6295)
int OKIM6295_num(const void* interface) { return ((struct OKIM6295interface*)interface)->num; }
#endif
#if (HAS_MSM5205)
int MSM5205_num(const void* interface) { return ((struct MSM5205interface*)interface)->num; }
#endif
#if (HAS_HC55516)
int HC55516_num(const void* interface) { return ((struct CVSDinterface*)interface)->num; }
#endif
#if (HAS_AY8910)
int AY8910_clock(const void* interface) { return ((struct AY8910interface*)interface)->baseclock; }
int AY8910_num(const void* interface) { return ((struct AY8910interface*)interface)->num; }
#endif
#if (HAS_YM2203)
int YM2203_clock(const void* interface) { return ((struct YM2203interface*)interface)->baseclock; }
int YM2203_num(const void* interface) { return ((struct YM2203interface*)interface)->num; }
#endif
#if (HAS_YM2413)
int YM2413_clock(const void* interface) { return ((struct YM2413interface*)interface)->baseclock; }
int YM2413_num(const void* interface) { return ((struct YM2413interface*)interface)->num; }
#endif
#if (HAS_YM2608)
int YM2608_clock(const void* interface) { return ((struct YM2608interface*)interface)->baseclock; }
int YM2608_num(const void* interface) { return ((struct YM2608interface*)interface)->num; }
#endif
#if (HAS_YM2610)
int YM2610_clock(const void* interface) { return ((struct YM2610interface*)interface)->baseclock; }
int YM2610_num(const void* interface) { return ((struct YM2610interface*)interface)->num; }
#endif
#if (HAS_YM2612)
int YM2612_clock(const void* interface) { return ((struct YM2612interface*)interface)->baseclock; }
int YM2612_num(const void* interface) { return ((struct YM2612interface*)interface)->num; }
#endif
#if (HAS_POKEY)
int POKEY_clock(const void* interface) { return ((struct POKEYinterface*)interface)->baseclock; }
int POKEY_num(const void* interface) { return ((struct POKEYinterface*)interface)->num; }
#endif
#if (HAS_TIA)
int TIA_clock(const void* interface) { return ((struct TIAinterface*)interface)->baseclock; }
#endif
#if (HAS_YM3812)
int YM3812_clock(const void* interface) { return ((struct YM3812interface*)interface)->baseclock; }
int YM3812_num(const void* interface) { return ((struct YM3812interface*)interface)->num; }
#endif
#if (HAS_VLM5030)
int VLM5030_clock(const void* interface) { return ((struct VLM5030interface*)interface)->baseclock; }
#endif
#if (HAS_TMS5220)
int TMS5220_clock(const void* interface) { return ((struct TMS5220interface*)interface)->baseclock; }
#endif
#if (HAS_YM2151)
int YM2151_clock(const void* interface) { return ((struct YM2151interface*)interface)->baseclock; }
int YM2151_num(const void* interface) { return ((struct YM2151interface*)interface)->num; }
#endif
#if (HAS_NES)
int NES_clock(const void* interface) { return ((struct NESinterface*)interface)->baseclock; }
int NES_num(const void* interface) { return ((struct NESinterface*)interface)->num; }
#endif
#if (HAS_SN76496)
int SN76496_clock(const void* interface) { return ((struct SN76496interface*)interface)->baseclock; }
int SN76496_num(const void* interface) { return ((struct SN76496interface*)interface)->num; }
#endif
#if (HAS_UPD7759)
int UPD7759_clock(const void* interface) { return ((struct UPD7759_interface*)interface)->clock_rate; }
#endif
#if (HAS_ASTROCADE)
int ASTROCADE_clock(const void* interface) { return ((struct astrocade_interface*)interface)->baseclock; }
int ASTROCADE_num(const void* interface) { return ((struct astrocade_interface*)interface)->num; }
#endif


struct snd_interface sndintf[] =
{
    {
		SOUND_DUMMY,
		"",
		0,
		0,
		0,
		0,
		0,
		0
	},
#if (HAS_CUSTOM)
    {
		SOUND_CUSTOM,
		"Custom",
		0,
		0,
		(int  (*)(const void *))custom_sh_start,
		custom_sh_stop,
		custom_sh_update,
		0
	},
#endif
#if (HAS_SAMPLES)
    {
		SOUND_SAMPLES,
		"Samples",
		0,
		0,
		(int  (*)(const void *))samples_sh_start,
		0,
		0,
		0
	},
#endif
#if (HAS_DAC)
    {
		SOUND_DAC,
		"DAC",
		DAC_num,
		0,
		(int  (*)(const void *))DAC_sh_start,
		0,
		0,
		0
	},
#endif
#if (HAS_AY8910)
    {
		SOUND_AY8910,
		"AY-8910",
		AY8910_num,
		AY8910_clock,
		(int  (*)(const void *))AY8910_sh_start,
		0,
		0,
		0
	},
#endif
#if (HAS_YM2203)
    {
		SOUND_YM2203,
		"YM-2203",
		YM2203_num,
		YM2203_clock,
		(int  (*)(const void *))YM2203_sh_start,
		YM2203_sh_stop,
		0,
		YM2203_sh_reset,
	},
#endif
#if (HAS_YM2151)
    {
		SOUND_YM2151,
		"YM-2151",
		YM2151_num,
		YM2151_clock,
		(int  (*)(const void *))YM2151_sh_start,
		YM2151_sh_stop,
		0,
		0
	},
#endif
#if (HAS_YM2151_ALT)
    {
		SOUND_YM2151_ALT,
		"YM-2151a",
		YM2151_num,
		YM2151_clock,
		(int  (*)(const void *))YM2151_ALT_sh_start,
		YM2151_sh_stop,
		0,
		0
	},
#endif
#if (HAS_YM2608)
    {
		SOUND_YM2608,
		"YM-2608",
		YM2608_num,
		YM2608_clock,
		(int  (*)(const void *))YM2608_sh_start,
		YM2608_sh_stop,
		0,
		0
	},
#endif
#if (HAS_YM2610)
    {
		SOUND_YM2610,
		"YM-2610",
		YM2610_num,
		YM2610_clock,
		(int  (*)(const void *))YM2610_sh_start,
		YM2610_sh_stop,
		0,
		0
	},
#endif
#if (HAS_YM2612)
    {
		SOUND_YM2612,
		"YM-2612",
		YM2612_num,
		YM2612_clock,
		(int  (*)(const void *))YM2612_sh_start,
		YM2612_sh_stop,
		YM2612_sh_update,
		0
	},
#endif
#if (HAS_YM3438)
    {
		SOUND_YM3438,
		"YM-3438",
		YM2612_num,
		YM2612_clock,
		(int  (*)(const void *))YM2612_sh_start,
		YM2612_sh_stop,
		YM2612_sh_update,
		0
	},
#endif
#if (HAS_YM2413)
    {
		SOUND_YM2413,
		"YM-2413",
		YM2413_num,
		YM2413_clock,
		(int  (*)(const void *))YM2413_sh_start,
		YM2413_sh_stop,
		0,
		0
	},
#endif
#if (HAS_YM3812)
    {
		SOUND_YM3812,
		"YM-3812",
		YM3812_num,
		YM3812_clock,
		(int  (*)(const void *))YM3812_sh_start,
		YM3812_sh_stop,
		0,
		0
	},
#endif
#if (HAS_YM3526)
    {
		SOUND_YM3526,
		"YM-3526",
		YM3812_num,
		YM3812_clock,
		(int  (*)(const void *))YM3812_sh_start,
		YM3812_sh_stop,
		0,
		0
	},
#endif
#if (HAS_SN76496)
    {
		SOUND_SN76496,
		"SN76496",
		SN76496_num,
		SN76496_clock,
		(int  (*)(const void *))SN76496_sh_start,
		0,
		0,
		0
	},
#endif
#if (HAS_POKEY)
    {
		SOUND_POKEY,
		"Pokey",
		POKEY_num,
		POKEY_clock,
		(int  (*)(const void *))pokey_sh_start,
		pokey_sh_stop,
		0,
		0
	},
#endif
#if (HAS_TIA)
    {
		SOUND_TIA,
		"TIA",
		0,
		TIA_clock,
		(int  (*)(const void *))tia_sh_start,
		tia_sh_stop,
		tia_sh_update,
		0
	},
#endif
#if (HAS_NES)
    {
		SOUND_NES,
		"NES",
		NES_num,
		NES_clock,
		(int  (*)(const void *))NESPSG_sh_start,
		NESPSG_sh_stop,
		NESPSG_sh_update,
		0
	},
#endif
#if (HAS_ASTROCADE)
    {
		SOUND_ASTROCADE,
		"Astrocade",
		ASTROCADE_num,
		ASTROCADE_clock,
		(int  (*)(const void *))astrocade_sh_start,
		astrocade_sh_stop,
		astrocade_sh_update,
		0
	},
#endif
#if (HAS_NAMCO)
    {
		SOUND_NAMCO,
		"Namco",
		0,
		0,
		(int  (*)(const void *))namco_sh_start,
		namco_sh_stop,
		0,
		0
	},
#endif
#if (HAS_TMS5220)
    {
		SOUND_TMS5220,
		"TMS5520",
		0,
		TMS5220_clock,
		(int  (*)(const void *))tms5220_sh_start,
		tms5220_sh_stop,
		tms5220_sh_update,
		0
	},
#endif
#if (HAS_VLM5030)
    {
		SOUND_VLM5030,
		"VLM5030",
		0,
		VLM5030_clock,
		(int  (*)(const void *))VLM5030_sh_start,
		VLM5030_sh_stop,
		VLM5030_sh_update,
		0
	},
#endif
#if (HAS_ADPCM)
    {
		SOUND_ADPCM,
		"ADPCM",
		ADPCM_num,
		0,
		(int  (*)(const void *))ADPCM_sh_start,
		ADPCM_sh_stop,
		ADPCM_sh_update,
		0
	},
#endif
#if (HAS_OKIM6295)
    {
		SOUND_OKIM6295,
		"OKI6295",
		OKIM6295_num,
		0,
		(int  (*)(const void *))OKIM6295_sh_start,
		OKIM6295_sh_stop,
		OKIM6295_sh_update,
		0
	},
#endif
#if (HAS_MSM5205)
    {
		SOUND_MSM5205,
		"MSM5205",
		MSM5205_num,
		0,
		(int  (*)(const void *))MSM5205_sh_start,
		MSM5205_sh_stop,
		MSM5205_sh_update,
		0
	},
#endif
#if (HAS_UPD7759)
    {
		SOUND_UPD7759,
		"uPD7759",
		0,
		UPD7759_clock,
		(int  (*)(const void *))UPD7759_sh_start,
		UPD7759_sh_stop,
		0,
		0
	},
#endif
#if (HAS_HC55516)
    {
		SOUND_HC55516,
		"HC55516",
		HC55516_num,
		0,
		(int  (*)(const void *))CVSD_sh_start,
		0,
		0,
		0
	},
#endif
#if (HAS_K007232)
    {
		SOUND_K007232,
		"007232",
		0,
		0,
		(int  (*)(const void *))K007232_sh_start,
		0,
		0,
		0
	},
#endif
};



int sound_start(void)
{
	int totalsound = 0;
	int i;

	/* Verify the order of entries in the sndintf[] array */
	for (i = 0;i < SOUND_COUNT;i++)
	{
		if (sndintf[i].sound_num != i)
		{
if (errorlog) fprintf(errorlog,"Sound #%d wrong ID %d: check enum SOUND_... in src/driver.h!\n",i,sndintf[i].sound_num);
			return 1;
		}
	}


	reset_play_channels();

	if (streams_sh_start() != 0)
		return 1;

	while (Machine->drv->sound[totalsound].sound_type != 0 && totalsound < MAX_SOUND)
	{
		if ((*sndintf[Machine->drv->sound[totalsound].sound_type].start)(Machine->drv->sound[totalsound].sound_interface) != 0)
			goto getout;

		totalsound++;
	}

	/* call the custom initialization AFTER initializing the standard sections, */
	/* so it can tweak the default parameters (like panning) */
	if (Machine->drv->sh_start && (*Machine->drv->sh_start)() != 0)
		return 1;

	refresh_period = TIME_IN_HZ (Machine->drv->frames_per_second);
	refresh_period_inv = 1.0 / refresh_period;
	sound_update_timer = timer_set (TIME_NEVER, 0, NULL);

	return 0;


getout:
	/* TODO: should also free the resources allocated before */
	return 1;
}



void sound_stop(void)
{
	int totalsound = 0;


	if (Machine->drv->sh_stop) (*Machine->drv->sh_stop)();

	while (Machine->drv->sound[totalsound].sound_type != 0 && totalsound < MAX_SOUND)
	{
		if (sndintf[Machine->drv->sound[totalsound].sound_type].stop)
			(*sndintf[Machine->drv->sound[totalsound].sound_type].stop)();

		totalsound++;
	}

	streams_sh_stop();

	if (sound_update_timer)
	{
		timer_remove (sound_update_timer);
		sound_update_timer = 0;
	}
}



void sound_update(void)
{
	int totalsound = 0;


	osd_profiler(OSD_PROFILE_SOUND);

	if (Machine->drv->sh_update) (*Machine->drv->sh_update)();

	while (Machine->drv->sound[totalsound].sound_type != 0 && totalsound < MAX_SOUND)
	{
		if (sndintf[Machine->drv->sound[totalsound].sound_type].update)
			(*sndintf[Machine->drv->sound[totalsound].sound_type].update)();

		totalsound++;
	}

	streams_sh_update();

	timer_reset (sound_update_timer, TIME_NEVER);

	osd_profiler(OSD_PROFILE_END);
}


void sound_reset(void)
{
	int totalsound = 0;


	while (Machine->drv->sound[totalsound].sound_type != 0 && totalsound < MAX_SOUND)
	{
		if (sndintf[Machine->drv->sound[totalsound].sound_type].reset)
			(*sndintf[Machine->drv->sound[totalsound].sound_type].reset)();

		totalsound++;
	}
}



const char *sound_name(const struct MachineSound *msound)
{
	if (msound->sound_type < SOUND_COUNT)
		return sndintf[msound->sound_type].name;
	else
		return "";
}

int sound_num(const struct MachineSound *msound)
{
	if (msound->sound_type < SOUND_COUNT && sndintf[msound->sound_type].chips_num)
		return (*sndintf[msound->sound_type].chips_num)(msound->sound_interface);
	else
		return 0;
}

int sound_clock(const struct MachineSound *msound)
{
	if (msound->sound_type < SOUND_COUNT && sndintf[msound->sound_type].chips_clock)
		return (*sndintf[msound->sound_type].chips_clock)(msound->sound_interface);
	else
		return 0;
}


int sound_scalebufferpos(int value)
{
	int result = (int)((double)value * timer_timeelapsed (sound_update_timer) * refresh_period_inv);
	if (value >= 0) return (result < value) ? result : value;
	else return (result > value) ? result : value;
}
