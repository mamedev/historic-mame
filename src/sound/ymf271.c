/*
    Yamaha YMF271-F "OPX" emulator v0.1
    By R. Belmont.
    Based in part on YMF278B emulator by R. Belmont and O. Galibert.
    12June04 update by Toshiaki Nijiura
    Copyright (c) 2003 R. Belmont.

    This software is dual-licensed: it may be used in MAME and properly licensed
    MAME derivatives under the terms of the MAME license.  For use outside of
    MAME and properly licensed derivatives, it is available under the
    terms of the GNU Lesser General Public License (LGPL), version 2.1.
    You may read the LGPL at http://www.gnu.org/licenses/lgpl.html
*/

#include <math.h>
#include "driver.h"
#include "cpuintrf.h"
#include "ymf271.h"

#define VERBOSE		(1)

#define CLOCK (44100 * 384)	// = 16.9344 MHz

#define MAXOUT		(+32767)
#define MINOUT		(-32768)

#define SIN_BITS		10
#define SIN_LEN			(1<<SIN_BITS)
#define SIN_MASK		(SIN_LEN-1)

#define log2(n) (log((float) n)/log((float) 2))

typedef struct
{
	INT8  extout;
	INT16 lfoFreq;
	INT8  lfowave;
	INT8  pms, ams;
	INT8  detune;
	INT8  multiple;
	INT8  tl;
	INT8  keyscale;
	INT8  ar;
	INT8  decay1rate, decay2rate;
	INT8  decay1lvl;
	INT8  relrate;
	INT32 fns;
	INT8  block;
	INT8  feedback;
	INT8  waveform;
	INT8  accon;
	INT8  algorithm;
	INT8  ch0_level, ch1_level, ch2_level, ch3_level;

	UINT32 startaddr;
	UINT32 loopaddr;
	UINT32 endaddr;
	INT8   fs, srcnote, srcb;

	UINT64 step;
	UINT64 stepptr;

	INT8 active;
	INT8 bits;

	// envelope generator
	INT32 volume;
	int env_state;
	INT32 env_attack_step;		// volume increase step in attack state
	INT32 env_decay1_step;
	INT32 env_decay2_step;
	INT32 env_release_step;

	INT64 feedback_modulation;
} YMF271Slot;

typedef struct
{
	INT8 sync, pfm;
} YMF271Group;

typedef struct
{
	YMF271Slot slots[48];
	YMF271Group groups[12];

	INT32 timerA, timerB;
	INT32 timerAVal, timerBVal;
	INT32 irqstate;
	INT8  status;
	INT8  enable;

	void *timA, *timB;

	INT8  reg0, reg1, reg2, reg3, pcmreg, timerreg;
	UINT32 ext_address;
	UINT8 ext_read;

	const UINT8 *rom;
	read8_handler ext_mem_read;
	write8_handler ext_mem_write;
	void (*irq_callback)(int);

	INT32 volume[256*4];			// precalculated attenuation values with some marging for enveloppe and pan levels
	int index;
	sound_stream * stream;
} YMF271Chip;

// slot mapping assists
static const int fm_tab[] = { 0, 1, 2, -1, 3, 4, 5, -1, 6, 7, 8, -1, 9, 10, 11, -1 };
static const int pcm_tab[] = { 0, 4, 8, -1, 12, 16, 20, -1, 24, 28, 32, -1, 36, 40, 44, -1 };

static INT16 *wavetable[7];

#define ENV_ATTACK		0
#define ENV_DECAY1		1
#define ENV_DECAY2		2
#define ENV_RELEASE		3

#define ENV_VOLUME_SHIFT	16

#define INF		100000000.0

static const double ARTime[] =
{
	INF,		INF,		INF,		INF,		6188.12,	4980.68,	4144.76,	3541.04,
	3094.06,	2490.34,	2072.38,	1770.52,	1547.03,	1245.17,	1036.19,	885.26,
	773.51,		622.59,		518.10,		441.63,		386.76,		311.29,		259.05,		221.32,
	193.38,		155.65,		129.52,		110.66,		96.69,		77.82,		64.76,		55.33,
	48.34,		38.91,		32.38,		27.66,		24.17,		19.46,		16.19,		13.83,
	12.09,		9.73,		8.10,		6.92,		6.04,		4.86,		4.05,		3.46,
	3.02,		2.47,		2.14,		1.88,		1.70,		1.38,		1.16,		1.02,
	0.88,		0.70,		0.57,		0.48,		0.43,		0.43,		0.43,		0.07
};

static const double DCTime[] =
{
	INF,		INF,		INF,		INF,		93599.64,	74837.91,	62392.02,	53475.56,
	46799.82,	37418.96,	31196.01,	26737.78,	23399.91,	18709.48,	15598.00,	13368.89,
	11699.95,	9354.74,	7799.00,	6684.44,	5849.98,	4677.37,	3899.50,	3342.22,
	2924.99,	2338.68,	1949.75,	1671.11,	1462.49,	1169.34,	974.88,		835.56,
	731.25,		584.67,		487.44,		417.78,		365.62,		292.34,		243.72,		208.89,
	182.81,		146.17,		121.86,		104.44,		91.41,		73.08,		60.93,		52.22,
	45.69,		36.55,		33.85,		26.09,		22.83,		18.28,		15.22,		13.03,
	11.41,		9.12,		7.60,		6.51,		5.69,		5.69,		5.69,		5.69
};

static const int RKS_Table[32][8] =
{
	{  0,  0,  0,  0,  0,  2,  4,  8 },
	{  0,  0,  0,  0,  1,  3,  5,  9 },
	{  0,  0,  0,  1,  2,  4,  6, 10 },
	{  0,  0,  0,  1,  3,  5,  7, 11 },
	{  0,  0,  1,  2,  4,  6,  8, 12 },
	{  0,  0,  1,  2,  5,  7,  9, 13 },
	{  0,  0,  1,  3,  6,  8, 10, 14 },
	{  0,  0,  1,  3,  7,  9, 11, 15 },
	{  0,  1,  2,  4,  8, 10, 12, 16 },
	{  0,  1,  2,  4,  9, 11, 13, 17 },
	{  0,  1,  2,  5, 10, 12, 14, 18 },
	{  0,  1,  2,  5, 11, 13, 15, 19 },
	{  0,  1,  3,  6, 12, 14, 16, 20 },
	{  0,  1,  3,  6, 13, 15, 17, 21 },
	{  0,  1,  3,  7, 14, 16, 18, 22 },
	{  0,  1,  3,  7, 15, 17, 19, 23 },
	{  0,  2,  4,  8, 16, 18, 20, 24 },
	{  0,  2,  4,  8, 17, 19, 21, 25 },
	{  0,  2,  4,  9, 18, 20, 22, 26 },
	{  0,  2,  4,  9, 19, 21, 23, 27 },
	{  0,  2,  5, 10, 20, 22, 24, 28 },
	{  0,  2,  5, 10, 21, 23, 25, 29 },
	{  0,  2,  5, 11, 22, 24, 26, 30 },
	{  0,  2,  5, 11, 23, 25, 27, 31 },
	{  0,  3,  6, 12, 24, 26, 28, 31 },
	{  0,  3,  6, 12, 25, 27, 29, 31 },
	{  0,  3,  6, 13, 26, 28, 30, 31 },
	{  0,  3,  6, 13, 27, 29, 31, 31 },
	{  0,  3,  7, 14, 28, 30, 31, 31 },
	{  0,  3,  7, 14, 29, 31, 31, 31 },
	{  0,  3,  7, 15, 30, 31, 31, 31 },
	{  0,  3,  7, 15, 31, 31, 31, 31 },
};

static const double channel_attenuation_table[16] =
{
	0.0, 2.5, 6.0, 8.5, 12.0, 14.5, 18.1, 20.6, 24.1, 26.6, 30.1, 32.6, 36.1, 96.1, 96.1, 96.1
};

static const int modulation_level[8] = { 16, 8, 4, 2, 1, 32, 64, 128 };

// feedback_level * 16
static const int feedback_level[8] = { 0, 1, 2, 4, 8, 16, 32, 64 };

static int channel_attenuation[16];
static int total_level[128];

INLINE int GET_KEYSCALED_RATE(int rate, int keycode, int keyscale)
{
	int newrate = rate + RKS_Table[keycode][keyscale];

	if (newrate > 63)
	{
		newrate = 63;
	}
	if (newrate < 0)
	{
		newrate = 0;
	}
	return newrate;
}

INLINE int GET_INTERNAL_KEYCODE(int block, int fns)
{
	int n43;
	if (fns < 0x780)
	{
		n43 = 0;
	}
	else if (fns < 0x900)
	{
		n43 = 1;
	}
	else if (fns < 0xa80)
	{
		n43 = 2;
	}
	else
	{
		n43 = 3;
	}

	return ((block & 7) * 4) + n43;
}

INLINE int GET_EXTERNAL_KEYCODE(int block, int fns)
{
	/* TODO: SrcB and SrcNote !? */
	int n43;
	if (fns < 0x100)
	{
		n43 = 0;
	}
	else if (fns < 0x300)
	{
		n43 = 1;
	}
	else if (fns < 0x500)
	{
		n43 = 2;
	}
	else
	{
		n43 = 3;
	}

	return ((block & 7) * 4) + n43;
}

void update_envelope(YMF271Slot *slot)
{
	switch (slot->env_state)
	{
		case ENV_ATTACK:
		{
			slot->volume += slot->env_attack_step;

			if (slot->volume >= (255 << ENV_VOLUME_SHIFT))
			{
				slot->volume = (255 << ENV_VOLUME_SHIFT);
				slot->env_state = ENV_DECAY1;
			}
			break;
		}

		case ENV_DECAY1:
		{
			int decay_level = 255 - (slot->decay1lvl << 4);
			slot->volume -= slot->env_decay1_step;

			if ((slot->volume >> (ENV_VOLUME_SHIFT)) <= decay_level)
			{
				slot->env_state = ENV_DECAY2;
			}
			break;
		}

		case ENV_DECAY2:
		{
			slot->volume -= slot->env_decay2_step;

			if (slot->volume < 0)
			{
				slot->volume = 0;
			}
			break;
		}

		case ENV_RELEASE:
		{
			slot->volume -= slot->env_release_step;

			if (slot->volume <= (0 << ENV_VOLUME_SHIFT))
			{
				slot->active = 0;
				slot->volume = 0;
			}
			break;
		}
	}
}

static void init_envelope(YMF271Slot *slot)
{
	int keycode, rate;
	int attack_length, decay1_length, decay2_length, release_length;
	int decay_level = 255 - (slot->decay1lvl << 4);

	double time;

	if (slot->waveform != 7)
	{
		keycode = GET_INTERNAL_KEYCODE(slot->block, slot->fns);
	}
	else
	{
		keycode = GET_EXTERNAL_KEYCODE(slot->block, slot->fns);
	}

	// init attack state
	rate = GET_KEYSCALED_RATE(slot->ar * 2, keycode, slot->keyscale);
	time = ARTime[rate];

	attack_length = (UINT32)((time * 44100.0) / 1000.0);		// attack end time in samples
	slot->env_attack_step = (int)(((double)(160-0) / (double)(attack_length)) * 65536.0);

	// init decay1 state
	rate = GET_KEYSCALED_RATE(slot->decay1rate * 2, keycode, slot->keyscale);
	time = DCTime[rate];

	decay1_length = (UINT32)((time * 44100.0) / 1000.0);
	slot->env_decay1_step = (int)(((double)(255-decay_level) / (double)(decay1_length)) * 65536.0);

	// init decay2 state
	rate = GET_KEYSCALED_RATE(slot->decay2rate * 2, keycode, slot->keyscale);
	time = DCTime[rate];

	decay2_length = (UINT32)((time * 44100.0) / 1000.0);
	slot->env_decay2_step = (int)(((double)(255-0) / (double)(decay2_length)) * 65536.0);

	// init release state
	rate = GET_KEYSCALED_RATE(slot->relrate * 4, keycode, slot->keyscale);
	time = ARTime[rate];

	release_length = (UINT32)((time * 44100.0) / 1000.0);
	slot->env_release_step = (int)(((double)(255-0) / (double)(release_length)) * 65536.0);

	slot->volume = (255-160) << ENV_VOLUME_SHIFT;		// -60db
	slot->env_state = ENV_ATTACK;
}

INLINE int calculate_slot_volume(YMF271Chip *chip, YMF271Slot *slot)
{
	int volume;
	volume = ((UINT64)chip->volume[255 - (slot->volume >> ENV_VOLUME_SHIFT)] *
			  (UINT64)total_level[slot->tl]) >> 16;

	return volume;
}

static void update_pcm(YMF271Chip *chip, int slotnum, INT32 *mixp, int length)
{
	int i;
	int final_volume;
	INT16 sample;
	int ch0_vol, ch1_vol, ch2_vol, ch3_vol;
	const UINT8 *rombase;

	YMF271Slot *slot = &chip->slots[slotnum];
	rombase = chip->rom;

	if (!slot->active)
	{
		return;
	}

	if (slot->waveform != 7)
	{
		osd_die("Waveform %d in update_pcm !!!\n", slot->waveform);
	}

	for (i = 0; i < length; i++)
	{
		if (slot->bits == 8)
		{
			sample = rombase[slot->startaddr + (slot->stepptr>>16)]<<8;
		}
		else
		{
			if (slot->stepptr & 1)
				sample = rombase[slot->startaddr + (slot->stepptr>>17)*3 + 2]<<8 | ((rombase[slot->startaddr + (slot->stepptr>>17)*3 + 1] << 4) & 0xf0);
			else
				sample = rombase[slot->startaddr + (slot->stepptr>>17)*3]<<8 | (rombase[slot->startaddr + (slot->stepptr>>17)*3 + 1] & 0xf0);
		}

		update_envelope(slot);

		final_volume = calculate_slot_volume(chip, slot);

		ch0_vol = ((UINT64)final_volume * (UINT64)channel_attenuation[slot->ch0_level]) >> 16;
		ch1_vol = ((UINT64)final_volume * (UINT64)channel_attenuation[slot->ch1_level]) >> 16;
		ch2_vol = ((UINT64)final_volume * (UINT64)channel_attenuation[slot->ch2_level]) >> 16;
		ch3_vol = ((UINT64)final_volume * (UINT64)channel_attenuation[slot->ch3_level]) >> 16;

		if (ch0_vol > 65536) ch0_vol = 65536;
		if (ch1_vol > 65536) ch1_vol = 65536;

		*mixp++ += (sample * ch0_vol) >> 16;
		*mixp++ += (sample * ch1_vol) >> 16;

		slot->stepptr += slot->step;
		if ((slot->stepptr>>16) > slot->endaddr)
		{
			// kill non-frac
			slot->stepptr &= 0xffff;
			slot->stepptr |= (slot->loopaddr<<16);
		}
	}
}

// calculates 2 operator FM using algorithm 0
// <--------|
// +--[S1]--+--[S3]-->
INLINE INT32 calculate_2op_fm_0(YMF271Chip *chip, int slotnum1, int slotnum2)
{
	YMF271Slot *slot1 = &chip->slots[slotnum1];
	YMF271Slot *slot2 = &chip->slots[slotnum2];
	int env1, env2;
	INT64 slot1_output, slot2_output;
	INT64 phase_mod;

	update_envelope(slot1);
	env1 = calculate_slot_volume(chip, slot1);
	update_envelope(slot2);
	env2 = calculate_slot_volume(chip, slot2);

	slot1_output = wavetable[slot1->waveform][((slot1->stepptr + slot1->feedback_modulation) >> 16) & SIN_MASK];
	phase_mod = (slot1_output << (SIN_BITS/2)) * modulation_level[slot2->feedback];
	slot2_output = wavetable[slot2->waveform][((slot2->stepptr + phase_mod) >> 16) & SIN_MASK];

	slot1->feedback_modulation = ((slot1_output << (SIN_BITS/2)) * feedback_level[slot1->feedback]) / 16;

	slot2_output = (slot2_output * env2) >> 16;

	slot1->stepptr += slot1->step;
	slot2->stepptr += slot2->step;

	return slot2_output;
}

// calculates 2 operator FM using algorithm 1
// <-----------------|
// +--[S1]--+--[S3]--|-->
INLINE INT32 calculate_2op_fm_1(YMF271Chip *chip, int slotnum1, int slotnum2)
{
	YMF271Slot *slot1 = &chip->slots[slotnum1];
	YMF271Slot *slot2 = &chip->slots[slotnum2];
	int env1, env2;
	INT64 slot1_output, slot2_output;
	INT64 phase_mod;

	update_envelope(slot1);
	env1 = calculate_slot_volume(chip, slot1);
	update_envelope(slot2);
	env2 = calculate_slot_volume(chip, slot2);

	slot1_output = wavetable[slot1->waveform][((slot1->stepptr + slot1->feedback_modulation) >> 16) & SIN_MASK];
	phase_mod = (slot1_output << (SIN_BITS/2)) * modulation_level[slot2->feedback];
	slot2_output = wavetable[slot2->waveform][((slot2->stepptr + phase_mod) >> 16) & SIN_MASK];

	slot1->feedback_modulation = ((slot2_output << (SIN_BITS/2)) * feedback_level[slot1->feedback]) / 16;

	slot2_output = (slot2_output * env2) >> 16;

	slot1->stepptr += slot1->step;
	slot2->stepptr += slot2->step;

	return slot2_output;
}

// calculates the output of one FM operator
INLINE INT32 calculate_1op_fm_0(YMF271Chip *chip, int slotnum, int phase_modulation)
{
	YMF271Slot *slot = &chip->slots[slotnum];
	int env;
	INT64 slot_output;
	INT64 phase_mod = phase_modulation;

	update_envelope(slot);
	env = calculate_slot_volume(chip, slot);

	phase_mod = (phase_mod << (SIN_BITS/2)) * modulation_level[slot->feedback];

	slot_output = wavetable[slot->waveform][((slot->stepptr + phase_mod) >> 16) & SIN_MASK];
	slot->stepptr += slot->step;

	slot_output = (slot_output * env) >> 16;

	return slot_output;
}

// calculates the output of one FM operator with feedback modulation
// <--------|
// +--[S1]--|
INLINE INT32 calculate_1op_fm_1(YMF271Chip *chip, int slotnum)
{
	YMF271Slot *slot = &chip->slots[slotnum];
	int env;
	INT64 slot_output;

	update_envelope(slot);
	env = calculate_slot_volume(chip, slot);

	slot_output = wavetable[slot->waveform][((slot->stepptr + slot->feedback_modulation) >> 16) & SIN_MASK];
	slot->feedback_modulation = ((slot_output << (SIN_BITS/2)) * feedback_level[slot->feedback]) / 16;
	slot->stepptr += slot->step;

	slot_output = (slot_output * env) >> 16;

	return slot_output;
}

static void ymf271_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int length)
{
	int i, j;
	int op;
	INT32 mix[48000*2];
	INT32 *mixp;
	YMF271Chip *chip = param;

	memset(mix, 0, sizeof(mix[0])*length*2);

	for (j = 0; j < 12; j++)
	{
		YMF271Group *slot_group = &chip->groups[j];
		mixp = &mix[0];

		if (slot_group->pfm && slot_group->sync != 3)
		{
			printf("Group %d: PFM, Sync = %d, Waveform Slot1 = %d, Slot2 = %d, Slot3 = %d, Slot4 = %d\n",
				j, slot_group->sync, chip->slots[j+0].waveform, chip->slots[j+12].waveform, chip->slots[j+24].waveform, chip->slots[j+36].waveform);
		}

		switch (slot_group->sync)
		{
			case 0:		// 4 operator FM
			{
				int slot1 = j + (0*12);
				int slot2 = j + (1*12);
				int slot3 = j + (2*12);
				int slot4 = j + (3*12);
				mixp = &mix[0];

				if (chip->slots[slot1].active)
				{
					for (i = 0; i < length; i++)
					{
						INT64 output1 = 0, output2 = 0, output3 = 0, output4 = 0, phase_mod1 = 0, phase_mod2 = 0;
						switch (chip->slots[slot1].algorithm)
						{
							// <--------|
							// +--[S1]--+--[S3]--+--[S2]--+--[S4]-->
							case 0:
								phase_mod1 = calculate_2op_fm_0(chip, slot1, slot3);
								phase_mod2 = calculate_1op_fm_0(chip, slot2, phase_mod1);
								output4 = calculate_1op_fm_0(chip, slot4, phase_mod2);
								break;

							// <-----------------|
							// +--[S1]--+--[S3]--+--[S2]--+--[S4]-->
							case 1:
								phase_mod1 = calculate_2op_fm_1(chip, slot1, slot3);
								phase_mod2 = calculate_1op_fm_0(chip, slot2, phase_mod1);
								output4 = calculate_1op_fm_0(chip, slot4, phase_mod2);
								break;

							// <--------|
							// +--[S1]--|
							// ---[S3]--+--[S2]--+--[S4]-->
							case 2:
								phase_mod1 = calculate_1op_fm_1(chip, slot1) + calculate_1op_fm_0(chip, slot3, 0);
								phase_mod2 = calculate_1op_fm_0(chip, slot2, phase_mod1);
								output4 = calculate_1op_fm_0(chip, slot4, phase_mod2);
								break;

							//          <--------|
							//          +--[S1]--|
							// ---[S3]--+--[S2]--+--[S4]-->
							case 3:
								phase_mod1 = calculate_1op_fm_0(chip, slot3, 0);
								phase_mod2 = calculate_1op_fm_0(chip, slot2, phase_mod1) + calculate_1op_fm_1(chip, slot1);
								output4 = calculate_1op_fm_0(chip, slot4, phase_mod2);
								break;

							// <--------|  --[S2]--|
							// ---[S1]--|-+--[S3]--+--[S4]-->
							case 4:
								phase_mod1 = calculate_2op_fm_0(chip, slot1, slot3) + calculate_1op_fm_0(chip, slot2, 0) ;
								output4 = calculate_1op_fm_0(chip, slot4, phase_mod1);
								break;

							//           --[S2]-----|
							// <-----------------|  |
							// ---[S1]--+--[S3]--|--+--[S4]-->
							case 5:
								phase_mod1 = calculate_2op_fm_1(chip, slot1, slot3) + calculate_1op_fm_0(chip, slot2, 0);
								output4 = calculate_1op_fm_0(chip, slot4, phase_mod1);
								break;

							// ---[S2]-----+--[S4]--|
							//                      |
							// <--------|           |
							// +--[S1]--|--+--[S3]--+-->
							case 6:
								output3 = calculate_2op_fm_0(chip, slot1, slot3);
								phase_mod1 = calculate_1op_fm_0(chip, slot2, 0);
								output4 = calculate_1op_fm_0(chip, slot4, phase_mod1);
								break;

							// ---[S2]--+--[S4]-----|
							//                      |
							// <-----------------|  |
							// +--[S1]--+--[S3]--|--+-->
							case 7:
								output3 = calculate_2op_fm_1(chip, slot1, slot3);
								phase_mod1 = calculate_1op_fm_0(chip, slot2, 0);
								output4 = calculate_1op_fm_0(chip, slot4, phase_mod1);
								break;

							// ---[S3]--+--[S2]--+--[S4]--|
							//                            |
							// <--------|                 |
							// +--[S1]--|-----------------+-->
							case 8:
								output1 = calculate_1op_fm_1(chip, slot1);
								phase_mod1 = calculate_1op_fm_0(chip, slot3, 0);
								phase_mod2 = calculate_1op_fm_0(chip, slot2, phase_mod1);
								output4 = calculate_1op_fm_0(chip, slot4, phase_mod2);
								break;

							//         <--------|
							// -----------[S1]--|
							//                  |
							// --[S3]--|        |
							// --[S2]--+--[S4]--+-->
							case 9:
								phase_mod1 = calculate_1op_fm_0(chip, slot2, 0) + calculate_1op_fm_0(chip, slot3, 0);
								output4 = calculate_1op_fm_0(chip, slot4, phase_mod1);
								output1 = calculate_1op_fm_1(chip, slot1);
								break;

							//           --[S4]--|
							//           --[S2]--+
							// <--------|        |
							// +--[S1]--+--[S3]--+-->
							case 10:
								output3 = calculate_2op_fm_0(chip, slot1, slot3);
								output2 = calculate_1op_fm_0(chip, slot2, 0);
								output4 = calculate_1op_fm_0(chip, slot4, 0);
								break;

							//           --[S4]-----|
							//           --[S2]-----+
							// <-----------------|  |
							// +--[S1]--+--[S3]--|--+-->
							case 11:
								output3 = calculate_2op_fm_1(chip, slot1, slot3);
								output2 = calculate_1op_fm_0(chip, slot2, 0);
								output4 = calculate_1op_fm_0(chip, slot4, 0);
								break;

							//            |--+--[S4]--+
							// <--------| |--+--[S3]--+
							// +--[S1]--+-|--+--[S2]--+-->
							case 12:
								phase_mod1 = calculate_1op_fm_1(chip, slot1);
								output2 = calculate_1op_fm_0(chip, slot2, phase_mod1);
								output3 = calculate_1op_fm_0(chip, slot3, phase_mod1);
								output4 = calculate_1op_fm_0(chip, slot4, phase_mod1);
								break;

							// ---[S3]--+--[S2]--+
							//                   |
							// ---[S4]-----------+
							// <--------|        |
							// +--[S1]--|--------+-->
							case 13:
								output1 = calculate_1op_fm_1(chip, slot1);
								phase_mod1 = calculate_1op_fm_0(chip, slot3, 0);
								output2 = calculate_1op_fm_0(chip, slot2, phase_mod1);
								output4 = calculate_1op_fm_0(chip, slot4, 0);
								break;

							// ---[S2]----+--[S4]--+
							//                     |
							// <--------| +--[S3]--|
							// +--[S1]--+-|--------+-->
							case 14:
								output1 = calculate_1op_fm_1(chip, slot1);
								phase_mod1 = output1;
								output3 = calculate_1op_fm_0(chip, slot3, phase_mod1);
								phase_mod2 = calculate_1op_fm_0(chip, slot2, 0);
								output4 = calculate_1op_fm_0(chip, slot4, phase_mod2);
								break;

							//  --[S4]-----+
							//  --[S2]-----+
							//  --[S3]-----+
							// <--------|  |
							// +--[S1]--|--+-->
							case 15:
								output1 = calculate_1op_fm_1(chip, slot1);
								output2 = calculate_1op_fm_0(chip, slot2, 0);
								output3 = calculate_1op_fm_0(chip, slot3, 0);
								output4 = calculate_1op_fm_0(chip, slot4, 0);
								break;
						}

						*mixp++ += ((output1 * channel_attenuation[chip->slots[slot1].ch0_level]) +
									(output2 * channel_attenuation[chip->slots[slot2].ch0_level]) +
									(output3 * channel_attenuation[chip->slots[slot3].ch0_level]) +
									(output4 * channel_attenuation[chip->slots[slot4].ch0_level])) >> 16;
						*mixp++ += ((output1 * channel_attenuation[chip->slots[slot1].ch1_level]) +
									(output2 * channel_attenuation[chip->slots[slot2].ch1_level]) +
									(output3 * channel_attenuation[chip->slots[slot3].ch1_level]) +
									(output4 * channel_attenuation[chip->slots[slot4].ch1_level])) >> 16;
					}
				}
				break;
			}

			case 1:		// 2x 2 operator FM
			{
				mixp = &mix[0];
				for (op = 0; op < 2; op++)
				{
					int slot1 = j + (op + 0) * 12;
					int slot2 = j + (op + 2) * 12;

					if (chip->slots[slot1].active)
					{
						for (i = 0; i < length; i++)
						{
							INT64 output1 = 0, output2 = 0, phase_mod = 0;
							switch (chip->slots[slot1].algorithm & 3)
							{
								// <--------|
								// +--[S1]--+--[S3]-->
								case 0:
									output2 = calculate_2op_fm_0(chip, slot1, slot2);
									break;

								// <-----------------|
								// +--[S1]--+--[S3]--|-->
								case 1:
									output2 = calculate_2op_fm_1(chip, slot1, slot2);
									break;

								// ---[S3]-----|
								// <--------|  |
								// +--[S1]--|--+-->
								case 2:
									output1 = calculate_1op_fm_1(chip, slot1);
									output2 = calculate_1op_fm_0(chip, slot2, 0);
									break;
								//
								// <--------| +--[S3]--|
								// +--[S1]--|-|--------+-->
								case 3:
									output1 = calculate_1op_fm_1(chip, slot1);
									phase_mod = output1;
									output2 = calculate_1op_fm_0(chip, slot2, phase_mod);
									break;
							}

							*mixp++ += ((output1 * channel_attenuation[chip->slots[slot1].ch0_level]) +
										(output2 * channel_attenuation[chip->slots[slot2].ch0_level])) >> 16;
							*mixp++ += ((output1 * channel_attenuation[chip->slots[slot1].ch1_level]) +
										(output2 * channel_attenuation[chip->slots[slot2].ch1_level])) >> 16;
						}
					}
				}
				break;
			}

			case 2:		// 3 operator FM + PCM
			{
				int slot1 = j + (0*12);
				int slot2 = j + (1*12);
				int slot3 = j + (2*12);
				mixp = &mix[0];

				if (chip->slots[slot1].active)
				{
					for (i = 0; i < length; i++)
					{
						INT64 output1 = 0, output2 = 0, output3 = 0, phase_mod = 0;
						switch (chip->slots[slot1].algorithm & 7)
						{
							// <--------|
							// +--[S1]--+--[S3]--+--[S2]-->
							case 0:
								phase_mod = calculate_2op_fm_0(chip, slot1, slot3);
								output2 = calculate_1op_fm_0(chip, slot2, phase_mod);
								break;

							// <-----------------|
							// +--[S1]--+--[S3]--+--[S2]-->
							case 1:
								phase_mod = calculate_2op_fm_1(chip, slot1, slot3);
								output2 = calculate_1op_fm_0(chip, slot2, phase_mod);
								break;

							// ---[S3]-----|
							// <--------|  |
							// +--[S1]--+--+--[S2]-->
							case 2:
								phase_mod = calculate_1op_fm_1(chip, slot1) + calculate_1op_fm_0(chip, slot3, 0);
								output2 = calculate_1op_fm_0(chip, slot2, phase_mod);
								break;

							// ---[S3]--+--[S2]--|
							// <--------|        |
							// +--[S1]--|--------+-->
							case 3:
								phase_mod = calculate_1op_fm_0(chip, slot3, 0);
								output2 = calculate_1op_fm_0(chip, slot2, phase_mod);
								output1 = calculate_1op_fm_1(chip, slot1);
								break;

							// ------------[S2]--|
							// <--------|        |
							// +--[S1]--+--[S3]--+-->
							case 4:
								output3 = calculate_2op_fm_0(chip, slot1, slot3);
								output2 = calculate_1op_fm_0(chip, slot2, 0);
								break;

							// ------------[S2]--|
							// <-----------------|
							// +--[S1]--+--[S3]--+-->
							case 5:
								output3 = calculate_2op_fm_1(chip, slot1, slot3);
								output2 = calculate_1op_fm_0(chip, slot2, 0);
								break;

							// ---[S2]-----|
							// ---[S3]-----+
							// <--------|  |
							// +--[S1]--+--+-->
							case 6:
								output1 = calculate_1op_fm_1(chip, slot1);
								output3 = calculate_1op_fm_0(chip, slot3, 0);
								output2 = calculate_1op_fm_0(chip, slot2, 0);
								break;

							// --------------[S2]--+
							// <--------| +--[S3]--|
							// +--[S1]--+-|--------+-->
							case 7:
								output1 = calculate_1op_fm_1(chip, slot1);
								phase_mod = output1;
								output3 = calculate_1op_fm_0(chip, slot3, phase_mod);
								output2 = calculate_1op_fm_0(chip, slot2, 0);
								break;
						}

						*mixp++ += ((output1 * channel_attenuation[chip->slots[slot1].ch0_level]) +
									(output2 * channel_attenuation[chip->slots[slot2].ch0_level]) +
									(output3 * channel_attenuation[chip->slots[slot3].ch0_level])) >> 16;
						*mixp++ += ((output1 * channel_attenuation[chip->slots[slot1].ch1_level]) +
									(output2 * channel_attenuation[chip->slots[slot2].ch1_level]) +
									(output3 * channel_attenuation[chip->slots[slot3].ch1_level])) >> 16;
					}
				}

				update_pcm(chip, j + (3*12), mixp, length);
				break;
			}

			case 3:		// PCM
			{
				update_pcm(chip, j + (0*12), mixp, length);
				update_pcm(chip, j + (1*12), mixp, length);
				update_pcm(chip, j + (2*12), mixp, length);
				update_pcm(chip, j + (3*12), mixp, length);
				break;
			}

			default: break;
		}
	}

	mixp = &mix[0];
	for (i = 0; i < length; i++)
	{
		outputs[0][i] = (*mixp++)>>2;
		outputs[1][i] = (*mixp++)>>2;
	}
}

static const double multiple_table[16] = { 0.5, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

static const double pow_table[16] = { 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 0.5, 1, 2, 4, 8, 16, 32, 64 };

static const int fs_frequency[4] = { (CLOCK/384), (CLOCK/384)/2, (CLOCK/384)/4, (CLOCK/384)/8 };

INLINE void calculate_step(YMF271Slot *slot)
{
	double st;

	if (slot->waveform == 7)	// external waveform (PCM)
	{
		st = (double)(2 * (slot->fns | 2048)) * pow_table[slot->block] * (double)(fs_frequency[slot->fs]);
		st = st * multiple_table[slot->multiple];
		st /= (double)(524288/65536);		// pre-multiply with 65536

		slot->step = (UINT64)st / (UINT64)Machine->sample_rate;
	}
	else						// internal waveform (FM)
	{
		st = (double)(2 * slot->fns) * pow_table[slot->block] * (double)(CLOCK/384);
		st = st * multiple_table[slot->multiple] * (double)(SIN_LEN);
		st /= (double)(536870912/65536);	// pre-multiply with 65536

		slot->step = (UINT64)st / (UINT64)Machine->sample_rate;
	}
}

static void write_register(YMF271Chip *chip, int slotnum, int reg, int data)
{
	YMF271Slot *slot = &chip->slots[slotnum];

	switch (reg)
	{
		case 0:
		{
			slot->extout = (data>>3)&0xf;

			if (data & 1)
			{
				// key on
				slot->step = 0;
				slot->stepptr = 0;

				slot->active = 1;

				calculate_step(slot);
				init_envelope(slot);
				slot->feedback_modulation = 0;
			}
			else
			{
				if (slot->active)
				{
					//slot->active = 0;
					slot->env_state = ENV_RELEASE;
				}
			}
			break;
		}

		case 1:
		{
			slot->lfoFreq = data;
			break;
		}

		case 2:
		{
			slot->lfowave = data & 3;
			slot->pms = (data >> 3) & 0x7;
			slot->ams = (data >> 6) & 0x7;
			break;
		}

		case 3:
		{
			slot->multiple = data & 0xf;
			slot->detune = (data >> 4) & 0x7;
			break;
		}

		case 4:
		{
			slot->tl = data & 0x7f;
			break;
		}

		case 5:
		{
			slot->ar = data & 0x1f;
			slot->keyscale = (data>>5)&0x7;
			break;
		}

		case 6:
		{
			slot->decay1rate = data & 0x1f;
			break;
		}

		case 7:
		{
			slot->decay2rate = data & 0x1f;
			break;
		}

		case 8:
		{
			slot->relrate = data & 0xf;
			slot->decay1lvl = (data >> 4) & 0xf;
			break;
		}

		case 9:
		{
			slot->fns &= ~0xff;
			slot->fns |= data;

			calculate_step(slot);
			break;
		}

		case 10:
		{
			slot->fns &= ~0xff00;
			slot->fns |= (data & 0xf)<<8;
			slot->block = (data>>4)&0xf;
			break;
		}

		case 11:
		{
			slot->waveform = data & 0x7;
			slot->feedback = (data >> 4) & 0x7;
			slot->accon = (data & 0x80) ? 1 : 0;
			break;
		}

		case 12:
		{
			slot->algorithm = data & 0xf;
			break;
		}

		case 13:
		{
			slot->ch0_level = data >> 4;
			slot->ch1_level = data & 0xf;
			break;
		}

		case 14:
		{
			slot->ch2_level = data >> 4;
			slot->ch3_level = data & 0xf;
			break;
		}
	}
}

static void ymf271_write_fm(YMF271Chip *chip, int grp, int adr, int data)
{
	int reg;
	int slotnum;
	int slot_group;
	int sync_mode, sync_reg;
	YMF271Slot *slot;

	slotnum = 12*grp;
	slotnum += fm_tab[adr & 0xf];
	slot = &chip->slots[slotnum];
	slot_group = fm_tab[adr & 0xf];

	reg = (adr >> 4) & 0xf;

	// check if the register is a synchronized register
	sync_reg = 0;
	switch (reg)
	{
		case 0:
		case 9:
		case 10:
		case 12:
		case 13:
		case 14:
			sync_reg = 1;
			break;

		default:
			break;
	}

	// check if the slot is key on slot for synchronizing
	sync_mode = 0;
	switch (chip->groups[slot_group].sync)
	{
		case 0:		// 4 slot mode
		{
			if (grp == 0)
				sync_mode = 1;
			break;
		}
		case 1:		// 2x 2 slot mode
		{
			if (grp == 0 || grp == 1)
				sync_mode = 1;
			break;
		}
		case 2:		// 3 slot + 1 slot mode
		{
			if (grp == 0)
				sync_mode = 1;
			break;
		}

		default:
			break;
	}

	if (sync_mode && sync_reg)		// key-on slot & synced register
	{
		switch (chip->groups[slot_group].sync)
		{
			case 0:		// 4 slot mode
			{
				write_register(chip, (12 * 0) + slot_group, reg, data);
				write_register(chip, (12 * 1) + slot_group, reg, data);
				write_register(chip, (12 * 2) + slot_group, reg, data);
				write_register(chip, (12 * 3) + slot_group, reg, data);
				break;
			}
			case 1:		// 2x 2 slot mode
			{
				if (grp == 0)		// Slot 1 - Slot 3
				{
					write_register(chip, (12 * 0) + slot_group, reg, data);
					write_register(chip, (12 * 2) + slot_group, reg, data);
				}
				else				// Slot 2 - Slot 4
				{
					write_register(chip, (12 * 1) + slot_group, reg, data);
					write_register(chip, (12 * 3) + slot_group, reg, data);
				}
				break;
			}
			case 2:		// 3 slot + 1 slot mode
			{
				// 1 slot is handled normally
				write_register(chip, (12 * 0) + slot_group, reg, data);
				write_register(chip, (12 * 1) + slot_group, reg, data);
				write_register(chip, (12 * 2) + slot_group, reg, data);
				break;
			}
			default:
				break;
		}
	}
	else		// write register normally
	{
		write_register(chip, (12 * grp) + slot_group, reg, data);
	}
}

static void ymf271_write_pcm(YMF271Chip *chip, int data)
{
	int slotnum;
	YMF271Slot *slot;

	slotnum = pcm_tab[chip->pcmreg&0xf];
	slot = &chip->slots[slotnum];

	switch ((chip->pcmreg>>4)&0xf)
	{
		case 0:
			slot->startaddr &= ~0xff;
			slot->startaddr |= data;
			break;
		case 1:
			slot->startaddr &= ~0xff00;
			slot->startaddr |= data<<8;
			break;
		case 2:
			slot->startaddr &= ~0xff0000;
			slot->startaddr |= data<<16;
			break;
		case 3:
			slot->endaddr &= ~0xff;
			slot->endaddr |= data;
			break;
		case 4:
			slot->endaddr &= ~0xff00;
			slot->endaddr |= data<<8;
			break;
		case 5:
			slot->endaddr &= ~0xff0000;
			slot->endaddr |= data<<16;
			break;
		case 6:
			slot->loopaddr &= ~0xff;
			slot->loopaddr |= data;
			break;
		case 7:
			slot->loopaddr &= ~0xff00;
			slot->loopaddr |= data<<8;
			break;
		case 8:
			slot->loopaddr &= ~0xff0000;
			slot->loopaddr |= data<<16;
			break;
		case 9:
			slot->fs = data & 0x3;
			slot->bits = (data & 0x4) ? 12 : 8;
			slot->srcnote = (data >> 3) & 0x3;
			slot->srcb = (data >> 5) & 0x7;
			break;
	}
}

static void ymf271_timer_a_tick(void *param)
{
	YMF271Chip *chip = param;

	chip->status |= 1;

	if (chip->enable & 4)
	{
		chip->irqstate |= 1;
		if (chip->irq_callback) chip->irq_callback(1);
	}
}

static void ymf271_timer_b_tick(void *param)
{
	YMF271Chip *chip = param;

	chip->status |= 2;

	if (chip->enable & 8)
	{
		chip->irqstate |= 2;
		if (chip->irq_callback) chip->irq_callback(1);
	}
}

static UINT8 ymf271_read_ext_memory(YMF271Chip *chip, UINT32 address)
{
	if( chip->ext_mem_read ) {
		return chip->ext_mem_read(address);
	} else {
		if( address < 0x800000)
			return chip->rom[address];
	}
	return 0xff;
}

static void ymf271_write_ext_memory(YMF271Chip *chip, UINT32 address, UINT8 data)
{
	if( chip->ext_mem_write ) {
		chip->ext_mem_write(address, data);
	}
}

static void ymf271_write_timer(YMF271Chip *chip, int data)
{
	int slotnum;
	YMF271Group *group;
	double period;

	slotnum = fm_tab[chip->timerreg & 0xf];
	group = &chip->groups[slotnum];

	if ((chip->timerreg & 0xf0) == 0)
	{
		group->sync = data & 0x3;
		group->pfm = data >> 7;
	}
	else
	{
		switch (chip->timerreg)
		{
			case 0x10:
				chip->timerA &= ~0xff;
				chip->timerA |= data;
				break;

			case 0x11:
				if (!(data & 0xfc))
				{
					chip->timerA &= 0x00ff;
					if ((data & 0x3) != 0x3)
					{
						chip->timerA |= (data & 0xff)<<8;
					}
				}
				break;

			case 0x12:
				chip->timerB = data;
				break;

			case 0x13:
				if (data & 1)
				{	// timer A load
					chip->timerAVal = chip->timerA;
				}
				if (data & 2)
				{	// timer B load
					chip->timerBVal = chip->timerB;
				}
				if (data & 4)
				{
					// timer A IRQ enable
					chip->enable |= 4;
				}
				if (data & 8)
				{
					// timer B IRQ enable
					chip->enable |= 8;
				}
				if (data & 0x10)
				{	// timer A reset
					chip->irqstate &= ~1;
					chip->status &= ~1;

					if (chip->irq_callback) chip->irq_callback(0);

					//period = (double)(256.0 - chip->timerAVal ) * ( 384.0 * 4.0 / (double)CLOCK);
					period = (384.0 * (1024.0 - chip->timerAVal)) / (double)CLOCK;

					timer_adjust_ptr(chip->timA, TIME_IN_SEC(period), chip, TIME_IN_SEC(period));
				}
				if (data & 0x20)
				{	// timer B reset
					chip->irqstate &= ~2;
					chip->status &= ~2;

					if (chip->irq_callback) chip->irq_callback(0);

					period = 6144.0 * (256.0 - (double)chip->timerBVal) / (double)CLOCK;

					timer_adjust_ptr(chip->timB, TIME_IN_SEC(period), chip, TIME_IN_SEC(period));
				}

				break;

			case 0x14:
				chip->ext_address &= ~0xff;
				chip->ext_address |= data;
				break;
			case 0x15:
				chip->ext_address &= ~0xff00;
				chip->ext_address |= data << 8;
				break;
			case 0x16:
				chip->ext_address &= ~0xff0000;
				chip->ext_address |= (data & 0x7f) << 16;
				chip->ext_read = (data & 0x80) ? 1 : 0;
				if( !chip->ext_read )
					chip->ext_address = (chip->ext_address + 1) & 0x7fffff;
				break;
			case 0x17:
				ymf271_write_ext_memory( chip, chip->ext_address, data );
				chip->ext_address = (chip->ext_address + 1) & 0x7fffff;
				break;
		}
	}
}

static void ymf271_w(int chipnum, int offset, int data)
{
	YMF271Chip *chip = sndti_token(SOUND_YMF271, chipnum);

	switch (offset)
	{
		case 0:
			chip->reg0 = data;
			break;
		case 1:
			ymf271_write_fm(chip, 0, chip->reg0, data);
			break;
		case 2:
			chip->reg1 = data;
			break;
		case 3:
			ymf271_write_fm(chip, 1, chip->reg1, data);
			break;
		case 4:
			chip->reg2 = data;
			break;
		case 5:
			ymf271_write_fm(chip, 2, chip->reg2, data);
			break;
		case 6:
			chip->reg3 = data;
			break;
		case 7:
			ymf271_write_fm(chip, 3, chip->reg3, data);
			break;
		case 8:
			chip->pcmreg = data;
			break;
		case 9:
			ymf271_write_pcm(chip, data);
			break;
		case 0xc:
			chip->timerreg = data;
			break;
		case 0xd:
			ymf271_write_timer(chip, data);
			break;
	}
}

static int ymf271_r(int chipnum, int offset)
{
	UINT8 value;
	YMF271Chip *chip = sndti_token(SOUND_YMF271, chipnum);

	switch(offset)
	{
		case 0:
			return chip->status;

		case 2:
			value = ymf271_read_ext_memory( chip, chip->ext_address );
			chip->ext_address = (chip->ext_address + 1) & 0x7fffff;
			return value;
	}

	return 0;
}

static void init_tables(void)
{
	int i;

	for (i=0; i < 7; i++)
	{
		wavetable[i] = auto_malloc(SIN_LEN * sizeof(INT16));
	}

	for (i=0; i < SIN_LEN; i++)
	{
		double m = sin( ((i*2)+1) * PI / SIN_LEN );
		double m2 = sin( ((i*4)+1) * PI / SIN_LEN );

		// Waveform 0: sin(wt)    (0 <= wt <= 2PI)
		wavetable[0][i] = (INT16)(m * MAXOUT);

		// Waveform 1: sin²(wt)   (0 <= wt <= PI)     -sin²(wt)  (PI <= wt <= 2PI)
		wavetable[1][i] = (i < (SIN_LEN/2)) ? (INT16)((m * m) * MAXOUT) : (INT16)((m * m) * MINOUT);

		// Waveform 2: sin(wt)    (0 <= wt <= PI)     -sin(wt)   (PI <= wt <= 2PI)
		wavetable[2][i] = (i < (SIN_LEN/2)) ? (INT16)(m * MAXOUT) : (INT16)(-m * MAXOUT);

		// Waveform 3: sin(wt)    (0 <= wt <= PI)     0
		wavetable[3][i] = (i < (SIN_LEN/2)) ? (INT16)(m * MAXOUT) : 0;

		// Waveform 4: sin(2wt)   (0 <= wt <= PI)     0
		wavetable[4][i] = (i < (SIN_LEN/2)) ? (INT16)(m2 * MAXOUT) : 0;

		// Waveform 5: |sin(2wt)| (0 <= wt <= PI)     0
		wavetable[5][i] = (i < (SIN_LEN/2)) ? (INT16)(fabs(m2) * MAXOUT) : 0;

		// Waveform 6:     1      (0 <= wt <= 2PI)
		wavetable[6][i] = (INT16)(1 * MAXOUT);
	}
}

static void ymf271_init(YMF271Chip *chip, UINT8 *rom, void (*cb)(int), read8_handler ext_read, write8_handler ext_write)
{
	chip->timA = timer_alloc_ptr(ymf271_timer_a_tick);
	chip->timB = timer_alloc_ptr(ymf271_timer_b_tick);

	chip->rom = rom;
	chip->irq_callback = cb;

	chip->ext_mem_read = ext_read;
	chip->ext_mem_write = ext_write;

	init_tables();
}

static void *ymf271_start(int sndindex, int clock, const void *config)
{
	const struct YMF271interface *intf;
	int i;
	YMF271Chip *chip;

	chip = auto_malloc(sizeof(*chip));
	memset(chip, 0, sizeof(*chip));
	chip->index = sndindex;

	intf = config;

	ymf271_init(chip, memory_region(intf->region), intf->irq_callback, intf->ext_read, intf->ext_write);
	chip->stream = stream_create(0, 2, Machine->sample_rate, chip, ymf271_update);

	// Volume table, 1 = -0.375dB, 8 = -3dB, 256 = -96dB
	for(i = 0; i < 256; i++)
		chip->volume[i] = 65536*pow(2.0, (-0.375/6)*i);
	for(i = 256; i < 256*4; i++)
		chip->volume[i] = 0;

	for (i = 0; i < 16; i++)
	{
		channel_attenuation[i] = 65536 * pow(2.0, (-channel_attenuation_table[i] / 6.0));
	}
	for (i = 0; i < 128; i++)
	{
		double db = 0.75 * (double)i;
		total_level[i] = 65536 * pow(2.0, (-db / 6.0));
	}

	return chip;
}

READ8_HANDLER( YMF271_0_r )
{
	return ymf271_r(0, offset);
}

WRITE8_HANDLER( YMF271_0_w )
{
	ymf271_w(0, offset, data);
}

READ8_HANDLER( YMF271_1_r )
{
	return ymf271_r(1, offset);
}

WRITE8_HANDLER( YMF271_1_w )
{
	ymf271_w(1, offset, data);
}

static void ymf271_reset(void *token)
{
	int i;
	YMF271Chip *chip = (YMF271Chip*)token;

	for (i = 0; i < 48; i++)
	{
		chip->slots[i].active = 0;
		chip->slots[i].volume = 0;
	}
}

/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void ymf271_set_info(void *token, UINT32 state, union sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void ymf271_get_info(void *token, UINT32 state, union sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = ymf271_set_info;		break;
		case SNDINFO_PTR_START:							info->start = ymf271_start;				break;
		case SNDINFO_PTR_STOP:							/* Nothing */							break;
		case SNDINFO_PTR_RESET:							info->reset = ymf271_reset;				break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "YMF271";						break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Yamaha FM";					break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}
