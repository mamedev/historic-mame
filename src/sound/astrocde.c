/***********************************************************

     Astrocade custom 'IO' chip sound chip driver
	 Frank Palazzolo

	 Portions copied from the Pokey emulator by Ron Fries

	 First Release:
	 	09/20/98
	 Updated 11/2004
	 	Fixed noise generator bug
	 	Changed to stream system
	 	Fixed out of bounds memory access bug

***********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "driver.h"
#include "cpu/z80/z80.h"


static const struct astrocade_interface *intf;

static int div_by_N_factor;

static int current_count_A[MAX_ASTROCADE_CHIPS];
static int current_count_B[MAX_ASTROCADE_CHIPS];
static int current_count_C[MAX_ASTROCADE_CHIPS];
static int current_count_V[MAX_ASTROCADE_CHIPS];
static int current_count_N[MAX_ASTROCADE_CHIPS];

static int current_state_A[MAX_ASTROCADE_CHIPS];
static int current_state_B[MAX_ASTROCADE_CHIPS];
static int current_state_C[MAX_ASTROCADE_CHIPS];
static int current_state_V[MAX_ASTROCADE_CHIPS];

static int current_size_A[MAX_ASTROCADE_CHIPS];
static int current_size_B[MAX_ASTROCADE_CHIPS];
static int current_size_C[MAX_ASTROCADE_CHIPS];
static int current_size_V[MAX_ASTROCADE_CHIPS];
static int current_size_N[MAX_ASTROCADE_CHIPS];

static int stream;

/* Registers */

static int master_osc[MAX_ASTROCADE_CHIPS];
static int freq_A[MAX_ASTROCADE_CHIPS];
static int freq_B[MAX_ASTROCADE_CHIPS];
static int freq_C[MAX_ASTROCADE_CHIPS];
static int vol_A[MAX_ASTROCADE_CHIPS];
static int vol_B[MAX_ASTROCADE_CHIPS];
static int vol_C[MAX_ASTROCADE_CHIPS];
static int vibrato[MAX_ASTROCADE_CHIPS];
static int vibrato_speed[MAX_ASTROCADE_CHIPS];
static int mux[MAX_ASTROCADE_CHIPS];
static int noise_am[MAX_ASTROCADE_CHIPS];
static int vol_noise4[MAX_ASTROCADE_CHIPS];
static int vol_noise8[MAX_ASTROCADE_CHIPS];

static int randbyte[MAX_ASTROCADE_CHIPS];
static int randbit[MAX_ASTROCADE_CHIPS];

static void astrocade_update(int param, INT16 **buffer, int num_samples)
{
	int num;
	INT16 *bufptr;
	int count;
	int data, data16, noise_plus_osc, vib_plus_osc;

	/* For each chip */
	for (num=0; num < intf->num ;num++)
	{
		bufptr = buffer[num];
		count = num_samples;

		/* For each sample */
		while(count>0)
		{
			/* Update current output value */

			if (current_count_N[num] == 0)
			{
				randbyte[num] = rand() & 0xff;
			}

			current_size_V[num] = 32768*vibrato_speed[num]/div_by_N_factor;

			if (!mux[num])
			{
				if (current_state_V[num] == -1)
					vib_plus_osc = (master_osc[num]-vibrato[num])&0xff;
				else
					vib_plus_osc = master_osc[num];
				current_size_A[num] = vib_plus_osc*freq_A[num]/div_by_N_factor;
				current_size_B[num] = vib_plus_osc*freq_B[num]/div_by_N_factor;
				current_size_C[num] = vib_plus_osc*freq_C[num]/div_by_N_factor;
			}
			else
			{
				noise_plus_osc = ((master_osc[num]-(vol_noise8[num]&randbyte[num])))&0xff;
				current_size_A[num] = noise_plus_osc*freq_A[num]/div_by_N_factor;
				current_size_B[num] = noise_plus_osc*freq_B[num]/div_by_N_factor;
				current_size_C[num] = noise_plus_osc*freq_C[num]/div_by_N_factor;
				current_size_N[num] = 2*noise_plus_osc/div_by_N_factor;
			}

			data = (current_state_A[num]*vol_A[num] +
					current_state_B[num]*vol_B[num] +
					current_state_C[num]*vol_C[num]);

			if (noise_am[num])
			{
				randbit[num] = rand() & 1;
				data = data + randbit[num]*vol_noise4[num];
			}

			/* Put it in the buffer */

			data16 = data<<8;
			*bufptr = data16;
			bufptr++;

			/* Update the state of the chip */

			if (current_count_A[num] >= current_size_A[num])
			{
				current_state_A[num] = -current_state_A[num];
				current_count_A[num] = 0;
			}
			else
				current_count_A[num]++;

			if (current_count_B[num] >= current_size_B[num])
			{
				current_state_B[num] = -current_state_B[num];
				current_count_B[num] = 0;
			}
			else
				current_count_B[num]++;

			if (current_count_C[num] >= current_size_C[num])
			{
				current_state_C[num] = -current_state_C[num];
				current_count_C[num] = 0;
			}
			else
				current_count_C[num]++;

			if (current_count_V[num] >= current_size_V[num])
			{
				current_state_V[num] = -current_state_V[num];
				current_count_V[num] = 0;
			}
			else
				current_count_V[num]++;

			if (current_count_N[num] >= current_size_N[num])
			{
				current_count_N[num] = 0;
			}
			else
				current_count_N[num]++;

			count--;
		}
	}
}

static void astrocade_update_one(int ch, INT16 *buffer, int length)
{
	astrocade_update(ch, &buffer, length);
}

int astrocade_sh_start(const struct MachineSound *msound)
{
	int i;
	const char *names[2] = { "ASTROCADE1", "ASTROCADE2" };
	int default_mixing_levels[2];

	intf = msound->sound_interface;

	if (Machine->sample_rate == 0)
	{
		return 0;
	}

	div_by_N_factor = intf->baseclock/Machine->sample_rate;

	default_mixing_levels[0] = intf->mixing_level[0];
	default_mixing_levels[1] = intf->mixing_level[1];

	if (intf->num == 1)
	{
		stream = stream_init(names[0],default_mixing_levels[0],
				Machine->sample_rate,
				0,astrocade_update_one);
	}
	else
	{
		stream = stream_init_multi(intf->num,names,default_mixing_levels,
				Machine->sample_rate,
				0,astrocade_update);
	}

	if (stream == -1)
		return 1;

	/* init states */
	for (i = 0;i < intf->num;i++)
	{
		/* reset state */
		current_count_A[i] = 0;
		current_count_B[i] = 0;
		current_count_C[i] = 0;
		current_count_V[i] = 0;
		current_count_N[i] = 0;
		current_state_A[i] = 1;
		current_state_B[i] = 1;
		current_state_C[i] = 1;
		current_state_V[i] = 1;
		randbyte[i] = 0;
		randbit[i] = 1;
	}

	return 0;
}

void astrocade_sound_w(int num, int offset, int data)
{
	int i, temp_vib;

	/* update */
	stream_update(stream,0);

	switch(offset)
	{
		case 0:  /* Master Oscillator */
#ifdef VERBOSE
			logerror("Master Osc Write: %02x\n",data);
#endif
			master_osc[num] = data+1;
		break;

		case 1:  /* Tone A Frequency */
#ifdef VERBOSE
			logerror("Tone A Write:        %02x\n",data);
#endif
			freq_A[num] = data+1;
		break;

		case 2:  /* Tone B Frequency */
#ifdef VERBOSE
			logerror("Tone B Write:           %02x\n",data);
#endif
			freq_B[num] = data+1;
		break;

		case 3:  /* Tone C Frequency */
#ifdef VERBOSE
			logerror("Tone C Write:              %02x\n",data);
#endif
			freq_C[num] = data+1;
		break;

		case 4:  /* Vibrato Register */
#ifdef VERBOSE
			logerror("Vibrato Depth:                %02x\n",data&0x3f);
			logerror("Vibrato Speed:                %02x\n",data>>6);
#endif
			vibrato[num] = data & 0x3f;

			temp_vib = (data>>6) & 0x03;
			vibrato_speed[num] = 1;
			for(i=0;i<temp_vib;i++)
				vibrato_speed[num] <<= 1;

		break;

		case 5:  /* Tone C Volume, Noise Modulation Control */
			vol_C[num] = data & 0x0f;
			mux[num] = (data>>4) & 0x01;
			noise_am[num] = (data>>5) & 0x01;
#ifdef VERBOSE
			logerror("Tone C Vol:                      %02x\n",vol_C[num]);
			logerror("Mux Source:                      %02x\n",mux[num]);
			logerror("Noise Am:                        %02x\n",noise_am[num]);
#endif
		break;

		case 6:  /* Tone A & B Volume */
			vol_B[num] = (data>>4) & 0x0f;
			vol_A[num] = data & 0x0f;
#ifdef VERBOSE
			logerror("Tone A Vol:                         %02x\n",vol_A[num]);
			logerror("Tone B Vol:                         %02x\n",vol_B[num]);
#endif
		break;

		case 7:  /* Noise Volume Register */
			vol_noise8[num] = data;
			vol_noise4[num] = (data>>4) & 0x0f;
#ifdef VERBOSE
			logerror("Noise Vol:                             %02x\n",vol_noise8[num]);
			logerror("Noise Vol (4):                         %02x\n",vol_noise4[num]);
#endif
		break;
	}
}

WRITE8_HANDLER( astrocade_soundblock1_w )
{
	astrocade_sound_w(0, (offset>>8)&7, data);
}

WRITE8_HANDLER( astrocade_soundblock2_w )
{
	astrocade_sound_w(1, (offset>>8)&7, data);
}

WRITE8_HANDLER( astrocade_sound1_w )
{
	astrocade_sound_w(0, offset, data);
}

WRITE8_HANDLER( astrocade_sound2_w )
{
	astrocade_sound_w(1, offset, data);
}


