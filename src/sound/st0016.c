/************************************
      Seta custom ST-0016 chip
      sound emulation by R. Belmont, Tomasz Slanina, and David Haywood
************************************/

#include <math.h>
#include "driver.h"
#include "cpuintrf.h"

#define VERBOSE (0)

extern data8_t *st0016_charram;

data8_t *st0016_sound_regs;
static int st0016_vpos[8], st0016_frac[8], st0016_lponce[8];

WRITE_HANDLER(st0016_snd_w)
{
	int voice = offset/32;
	int reg = offset & 0x1f;
	int oldreg = st0016_sound_regs[offset];
#if VERBOSE
	int vbase = offset & ~0x1f;
#endif

	st0016_sound_regs[offset] = data;

	if ((voice < 8) && (data != oldreg))
	{
		if ((reg == 0x16) && (data != 0))
		{
			st0016_vpos[voice] = st0016_frac[voice] = st0016_lponce[voice] = 0;

#if VERBOSE
			logerror("Key on V%02d: st %06x-%06x lp %06x-%06x frq %x flg %x\n", voice,
				st0016_sound_regs[vbase+2]<<16 | st0016_sound_regs[vbase+1]<<8 | st0016_sound_regs[vbase+2],
				st0016_sound_regs[vbase+0xe]<<16 | st0016_sound_regs[vbase+0xd]<<8 | st0016_sound_regs[vbase+0xc],
				st0016_sound_regs[vbase+6]<<16 | st0016_sound_regs[vbase+5]<<8 | st0016_sound_regs[vbase+4],
				st0016_sound_regs[vbase+0xa]<<16 | st0016_sound_regs[vbase+0x9]<<8 | st0016_sound_regs[vbase+0x8],
				st0016_sound_regs[vbase+0x11]<<8 | st0016_sound_regs[vbase+0x10],
				st0016_sound_regs[vbase+0x16]);
#endif
		}
	}
}

static void st0016_update(int num, INT16 **outputs, int length) 
{
	int v, i, snum;
	unsigned char *slot;
	INT32 mix[48000*2];
	INT32 *mixp;
	INT16 sample;
	char *sndrom = (char *)st0016_charram;
	int sptr, eptr, freq, lsptr, leptr;

	memset(mix, 0, sizeof(mix[0])*length*2);

	for (v = 0; v < 8; v++)
	{
		slot = (unsigned char *)&st0016_sound_regs[v * 32];

		if (slot[0x16] & 0x06)
		{
			mixp = &mix[0];

			sptr = slot[0x02]<<16 | slot[0x01]<<8 | slot[0x00];
			eptr = slot[0x0e]<<16 | slot[0x0d]<<8 | slot[0x0c];
			freq = slot[0x11]<<8 | slot[0x10];
			lsptr = slot[0x06]<<16 | slot[0x05]<<8 | slot[0x04];
			leptr = slot[0x0a]<<16 | slot[0x09]<<8 | slot[0x08];

			for (snum = 0; snum < length; snum++)
			{
				sample = sndrom[sptr + st0016_vpos[v]]<<8;

				*mixp++ += (sample * (char)slot[0x14]) >> 8;
				*mixp++ += (sample * (char)slot[0x15]) >> 8;

				st0016_frac[v] += freq;
				st0016_vpos[v] += (st0016_frac[v]>>16);
				st0016_frac[v] &= 0xffff;

				// stop if we're at the end
				if (st0016_lponce[v])
				{
					// we've looped once, check loop end rather than sample end
					if ((st0016_vpos[v] + sptr) >= leptr)
					{
						st0016_vpos[v] = (lsptr - sptr);
					}
				}
				else
				{
					// not looped yet, check sample end
					if ((st0016_vpos[v] + sptr) >= eptr)
					{
						if (slot[0x16] & 0x01)	// loop?
						{
							st0016_vpos[v] = (lsptr - sptr);
							st0016_lponce[v] = 1;
						}
						else
						{
							slot[0x16] = 0;
							st0016_vpos[v] = st0016_frac[v] = 0;
						}
					}
				}
			}
		}
	}

	mixp = &mix[0];
	for (i = 0; i < length; i++)
	{
		outputs[0][i] = (*mixp++)>>4;
		outputs[1][i] = (*mixp++)>>4;
	}
}

int st0016_sh_start(const struct MachineSound *msound)
{
	char buf[2][40];
	const char *name[2];
	int  vol[2];

	sprintf(buf[0], "ST-0016 L");
	sprintf(buf[1], "ST-0016 R");
	name[0] = buf[0];
	name[1] = buf[1];
	vol[0]=100;
	vol[1]=100;
	stream_init_multi(2, name, vol, 44100, 0, st0016_update);

	return 0;
}

void st0016_sh_stop( void )
{
}


