/*
    Ricoh RF5C400 emulator

    Written by Ville Linde
*/

#include "driver.h"
#include "rf5c400.h"
#include <math.h>

#define CLOCK (44100 * 384)	// = 16.9344 MHz

struct rf5c400_info
{
	const struct RF5C400interface *intf;

	INT16 *rom;
	UINT32 rom_length;

	sound_stream *stream;

	int current_channel;

	struct RF5C400_CHANNEL
	{
		UINT32 start;
		UINT32 end;
		UINT64 pos;
		UINT64 step;
		int keyon;
		int volume;
		int pan;
	} channels[32];
};

static int pan_table[2][32];
static int volume_table[32];

/*****************************************************************************/

static void rf5c400_update(void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length)
{
	int i, ch;
	struct rf5c400_info *info = param;
	INT16 *rom = info->rom;

	memset(buffer[0], 0, length*sizeof(*buffer[0]));
	memset(buffer[1], 0, length*sizeof(*buffer[1]));

	for (ch=0; ch < 32; ch++)
	{
		struct RF5C400_CHANNEL *channel = &info->channels[ch];

		for (i=0; i < length; i++)
		{
			if (channel->keyon != 0)
			{
				INT64 sample;
				UINT32 cur_pos = channel->start + (channel->pos >> 16);

				sample = rom[cur_pos];
				//sample = (sample * volume_table[31]) >> 16;

				//buffer[0][i] += (sample * pan_table[0][channel->pan]) >> 16;
				//buffer[1][i] += (sample * pan_table[1][channel->pan]) >> 16;

				buffer[0][i] += sample;
				buffer[1][i] += sample;

				channel->pos += channel->step;
				if (cur_pos > info->rom_length || cur_pos > channel->end)
				{
					channel->keyon = 0;
					channel->pos = 0;
				}
			}
		}
	}

	for (i=0; i < length; i++)
	{
		buffer[0][i] >>= 3;
		buffer[1][i] >>= 3;
	}
}

static void rf5c400_init_chip(struct rf5c400_info *info, int sndindex)
{
	UINT16 *sample;
	int i;

	info->rom = (INT16*)memory_region(info->intf->region);
	info->rom_length = memory_region_length(info->intf->region) / 2;

	// preprocess sample data
	sample = (UINT16*)memory_region(info->intf->region);
	for (i=0; i < memory_region_length(info->intf->region) / 2; i++)
	{
		if (sample[i] & 0x8000)
		{
			sample[i] ^= 0x7fff;
		}
	}

	info->stream = stream_create(0, 2, Machine->sample_rate, info, rf5c400_update);

	// init pan table
	for (i=0; i < 32; i++)
	{
		pan_table[0][i] = (int)(65536.0 / pow(10.0, ((double)(i) / (32.0 / 96.0)) / 20.0));
		pan_table[1][i] = (int)(65536.0 / pow(10.0, ((double)(31-i) / (32.0 / 96.0)) / 20.0));
	}

	// init volume table
	for (i=0; i < 32; i++)
	{
		volume_table[i] = (int)(65536.0 / pow(10.0, ((double)(31-i) / (32.0 / 96.0)) / 20.0));
	}
}


static void *rf5c400_start(int sndindex, int clock, const void *config)
{
	struct rf5c400_info *info;

	info = auto_malloc(sizeof(*info));
	memset(info, 0, sizeof(*info));

	info->intf = config;

	rf5c400_init_chip(info, sndindex);

	return info;
}

/*****************************************************************************/

static UINT16 rf5c400_status = 0;
static UINT16 rf5c400_r(int chipnum, int offset)
{
	switch(offset)
	{
		case 0x00:
		{
			return rf5c400_status;
		}

		case 0x04:
		{
			return 0;
		}
	}

	return 0;
}

static void rf5c400_w(int chipnum, int offset, UINT16 data)
{
	struct rf5c400_info *info = sndti_token(SOUND_RF5C400, chipnum);

	if (offset < 0x400)
	{
		switch(offset)
		{
			case 0x00:
			{
				rf5c400_status = data;
				break;
			}

			case 0x01:		// channel / pan / volume
			{
				data &= 0xff;
				if (data < 0x20)
				{
					info->current_channel = data;
				}
				else if (data >= 0x40 && data < 0x60)
				{
					info->channels[info->current_channel].pan = data - 0x40;
				}
				else if (data >= 0x60 && data < 0x80)
				{
					info->channels[info->current_channel].volume = data - 0x60;
				}
				break;
			}

			default:
			{
				//printf("rf5c400_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
				break;
			}
		}
		//printf("rf5c400_w: %08X, %08X, %08X at %08X\n", data, offset, mem_mask, activecpu_get_pc());
	}
	else
	{
		// channel registers
		int ch = (offset >> 5) & 0x1f;
		int reg = (offset & 0x1f);

		struct RF5C400_CHANNEL *channel = &info->channels[ch];

		switch (reg)
		{
			case 0x00:		// sample start address, bits 23 - 16
			{
				channel->start &= 0xffff;
				channel->start |= (UINT32)(data & 0xff00) << 8;
				break;
			}
			case 0x01:		// sample start address, bits 15 - 0
			{
				channel->start &= 0xff0000;
				channel->start |= data;

				channel->pos = 0;
				channel->keyon = 1;		// TODO: the key on is surely somewhere else..
				break;
			}
			case 0x02:		// sample playing frequency
			{
				int frequency = (CLOCK/384) / 8;
				int multiple = 1 << (data >> 13);
				double rate = ((double)(data & 0x1fff) / 2048.0) * (double)multiple;
				channel->step = (UINT64)((((double)(frequency) * rate) / (double)(Machine->sample_rate)) * 65536.0);
				break;
			}
			case 0x03:		// sample end address, bits 15 - 0
			{
				channel->end &= 0xff0000;
				channel->end |= data;
				break;
			}
			case 0x04:		// sample end address, bits 23 - 16
			{
				channel->end &= 0xffff;
				channel->end |= (UINT32)(data & 0xff) << 16;
				break;
			}
			case 0x05:		// unknown
			{
				break;
			}
			case 0x06:		// unknown
			{
				break;
			}
			case 0x07:		// unknown
			{
				break;
			}
			case 0x08:		// unknown
			{
				break;
			}
			case 0x10:		// unknown
			{
				break;
			}
		}
	}
}

READ16_HANDLER( RF5C400_0_r )
{
	return rf5c400_r(0, offset);
}

WRITE16_HANDLER( RF5C400_0_w )
{
	rf5c400_w(0, offset, data);
}

/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void rf5c400_set_info(void *token, UINT32 state, union sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void rf5c400_get_info(void *token, UINT32 state, union sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = rf5c400_set_info;		break;
		case SNDINFO_PTR_START:							info->start = rf5c400_start;			break;
		case SNDINFO_PTR_STOP:							/* nothing */							break;
		case SNDINFO_PTR_RESET:							/* nothing */							break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "RF5C400";					break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Ricoh PCM";					break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}
