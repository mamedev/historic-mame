/*********************************************************/
/*    SEGA 16ch 8bit PCM                                 */
/*********************************************************/

#include "driver.h"
#include "segapcm.h"

struct segapcm
{
	UINT8  *ram;
	UINT16 low[16];
	const UINT8 *rom, *rom_end;
	UINT32 *step;
	int rate;
	int bankshift;
	int bankmask;
	sound_stream * stream;
};

static void SEGAPCM_update(void *param, stream_sample_t **inputs, stream_sample_t **buffer, int length)
{
	struct segapcm *spcm = param;
	int ch;
	memset(buffer[0], 0, length*sizeof(*buffer[0]));
	memset(buffer[1], 0, length*sizeof(*buffer[1]));

	for(ch=0; ch<16; ch++)
		if(!(spcm->ram[0x86+8*ch] & 1)) {
			UINT8 *base = spcm->ram+8*ch;
			UINT32 addr = (base[5] << 24) | (base[4] << 16) | spcm->low[ch];
			UINT16 loop = (base[0x85] << 8)|base[0x84];
			UINT8 end = base[6]+1;
			UINT8 delta = base[7];
			UINT32 step = spcm->step[delta];
			UINT8 voll = base[2];
			UINT8 volr = base[3];
			UINT8 flags = base[0x86];
			const UINT8 *rom = spcm->rom + ((flags & spcm->bankmask) << spcm->bankshift);
			int i;

			for(i=0; i<length; i++) {
				INT8 v;
				const UINT8 *ptr;
				if((addr >> 24) == end) {
					if(!(flags & 2))
						addr = loop << 16;
					else {
						flags |= 1;
						break;
					}
				}
				ptr = rom + (addr >> 16);
				if(ptr < spcm->rom_end)
					v = rom[addr>>16] - 0x80;
				else
					v = 0;
				buffer[0][i] += (v*voll);
				buffer[1][i] += (v*volr);
				addr += step;
			}
			base[0x86] = flags;
			base[4] = addr >> 16;
			base[5] = addr >> 24;
			spcm->low[ch] = flags & 1 ? 0 : addr;
		}
}

static void *segapcm_start(int sndindex, int clock, const void *config)
{
	const struct SEGAPCMinterface *intf = config;
	int mask, rom_mask;
	int i;
	struct segapcm *spcm;
	
	spcm = auto_malloc(sizeof(*spcm));
	memset(spcm, 0, sizeof(*spcm));

	spcm->rate = clock;

	spcm->rom = (const UINT8 *)memory_region(intf->region);
	spcm->rom_end = spcm->rom + memory_region_length(intf->region);
	spcm->ram = auto_malloc(0x800);
	spcm->step = auto_malloc(sizeof(UINT32)*256);

	for(i=0; i<256; i++)
		spcm->step[i] = i*spcm->rate*(double)(65536/128) / Machine->sample_rate;

	memset(spcm->ram, 0xff, 0x800);

	spcm->bankshift = (UINT8)(intf->bank);
	mask = intf->bank >> 16;
	if(!mask)
		mask = BANK_MASK7>>16;

	for(rom_mask = 1; rom_mask < memory_region_length(intf->region); rom_mask *= 2);
	rom_mask--;

	spcm->bankmask = mask & (rom_mask >> spcm->bankshift);

	spcm->stream = stream_create(0, 2, Machine->sample_rate, spcm, SEGAPCM_update );

	return spcm;
}


WRITE8_HANDLER( SegaPCM_w )
{
	struct segapcm *spcm = sndti_token(SOUND_SEGAPCM, 0);
	stream_update(spcm->stream, 0);
	spcm->ram[offset & 0x07ff] = data;
}

READ8_HANDLER( SegaPCM_r )
{
	struct segapcm *spcm = sndti_token(SOUND_SEGAPCM, 0);
	stream_update(spcm->stream, 0);
	return spcm->ram[offset & 0x07ff];
}



/**************************************************************************
 * Generic get_info
 **************************************************************************/

static void segapcm_set_info(void *token, UINT32 state, union sndinfo *info)
{
	switch (state)
	{
		/* no parameters to set */
	}
}


void segapcm_get_info(void *token, UINT32 state, union sndinfo *info)
{
	switch (state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case SNDINFO_PTR_SET_INFO:						info->set_info = segapcm_set_info;		break;
		case SNDINFO_PTR_START:							info->start = segapcm_start;			break;
		case SNDINFO_PTR_STOP:							/* Nothing */							break;
		case SNDINFO_PTR_RESET:							/* Nothing */							break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case SNDINFO_STR_NAME:							info->s = "Sega PCM";					break;
		case SNDINFO_STR_CORE_FAMILY:					info->s = "Sega custom";				break;
		case SNDINFO_STR_CORE_VERSION:					info->s = "1.0";						break;
		case SNDINFO_STR_CORE_FILE:						info->s = __FILE__;						break;
		case SNDINFO_STR_CORE_CREDITS:					info->s = "Copyright (c) 2004, The MAME Team"; break;
	}
}
