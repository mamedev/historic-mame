/*********************************************************

Irem GA20 PCM Sound Chip

It's not currently known whether this chip is stereo.


Revision History:

04-18-2002 Acho A. Tang
- rewrote channel mixing
- added volume and sample rate handling but
  nothing conclusive
* equations are statistically induced with constants
  that make better sense

*********************************************************/
#include <math.h>
#include "driver.h"
#include "iremga20.h"

#define DEF_MAX_SR 22050
#define MAX_VOL 256

//AT: sorry about the x86-ish variables.
#define MIX_CH(CH) \
		if (play[CH]) \
		{ \
			eax = pos[CH]; \
			ebx = eax; \
			eax >>= 8; \
			eax = (int)*(char *)(esi + eax); \
			eax *= vol[CH]; \
			ebx += rate[CH]; \
			pos[CH] = ebx; \
			ebx = (ebx < end[CH]); \
			play[CH] = ebx; \
			edx += eax; \
		}

static float sr_exponent;
//ZT
static struct IremGA20_chip_def
{
	const struct IremGA20_interface *intf;
	unsigned char *rom;
	int rom_size;
	int channel;
	int mode;
	int regs[0x40];
} IremGA20_chip;

static struct IremGA20_channel_def
{
	unsigned long rate;
	unsigned long size;
	unsigned long start;
	unsigned long pos;
	unsigned long end;
	unsigned long volume;
	unsigned long pan;
	unsigned long effect;
	unsigned long play;
} IremGA20_channel[4];

void IremGA20_update( int param, INT16 **buffer, int length )
{
	unsigned long rate[4], pos[4], end[4], vol[4], play[4];
	register int edi, ebp, esi, eax, ebx, ecx, edx;

	if (!Machine->sample_rate) return;

	/* precache some values */
	for (ecx=0; ecx<4; ecx++)
	{
		rate[ecx] = IremGA20_channel[ecx].rate;
		pos[ecx] = IremGA20_channel[ecx].pos;
		end[ecx] = (IremGA20_channel[ecx].end - 0x20) << 8;
		vol[ecx] = IremGA20_channel[ecx].volume;
		play[ecx] = IremGA20_channel[ecx].play;
	}

	ecx = length << 1;
	esi = (int)IremGA20_chip.rom;
	edi = (int)buffer[0];
	ebp = (int)buffer[1];
	edi += ecx;
	ebp += ecx;
	ecx = -ecx;

	for (; ecx; ecx+=2)
	{
		edx ^= edx;

		MIX_CH(0);
		MIX_CH(1);
		MIX_CH(2);
		MIX_CH(3);

		edx >>= 2;
		*(short *)(edi + ecx) = (short)edx;
		*(short *)(ebp + ecx) = (short)edx;
	}

	/* update the regs now */
	for (ecx=0; ecx< 4; ecx++)
	{
		IremGA20_channel[ecx].pos = pos[ecx];
		IremGA20_channel[ecx].play = play[ecx];
	}
}

WRITE_HANDLER( IremGA20_w )
{
	int channel;

	//logerror("GA20:  Offset %02x, data %04x\n",offset,data);

	if (Machine->sample_rate)
		stream_update(IremGA20_chip.channel, 0);

	channel = offset >> 4;

	IremGA20_chip.regs[offset] = data;

	switch (offset & 0xf)
	{
		case 0: /* start address low */
			IremGA20_channel[channel].start = ((IremGA20_channel[channel].start)&0xff000) | (data<<4);
		break;

		case 2: /* start address high */
			IremGA20_channel[channel].start = ((IremGA20_channel[channel].start)&0x00ff0) | (data<<12);
		break;

		case 4: /* end address low */
			IremGA20_channel[channel].end = ((IremGA20_channel[channel].end)&0xff000) | (data<<4);
		break;

		case 6: /* end address high */
			IremGA20_channel[channel].end = ((IremGA20_channel[channel].end)&0x00ff0) | (data<<12);
		break;

		case 8: //AT: exponential function(guesswork)
			if (data < 0x80) data += 0x10; // Pefect Soldier seems to defy this relation
			IremGA20_channel[channel].rate = pow(data, sr_exponent)*256 / Machine->sample_rate;
		break;

		case 0xa: //AT: standard parabola(guesswork)
			if (data > 0x3f) data = 0x3f;
			// fail safe though no M92 game I've tested goes beyond this value
			IremGA20_channel[channel].volume = data * MAX_VOL/32 - (data^2) * MAX_VOL/4096;
		break;

		case 0xc: //AT: presumably a toggler for left, right or both channels.
		{
			IremGA20_channel[channel].play = data;
			IremGA20_channel[channel].pos = IremGA20_channel[channel].start << 8;
		}
		break;
	}
}

READ_HANDLER( IremGA20_r )
{
	/* Todo - Looks like there is a status bit to show whether each channel is playing */
	return 0xff;
}

static void IremGA20_reset( void )
{
	int i;

	for( i = 0; i < 4; i++ ) {
		IremGA20_channel[i].rate = 0;
		IremGA20_channel[i].size = 0;
		IremGA20_channel[i].start = 0;
		IremGA20_channel[i].pos = 0;
		IremGA20_channel[i].end = 0;
		IremGA20_channel[i].volume = 0;
		IremGA20_channel[i].pan = 0;
		IremGA20_channel[i].effect = 0;
		IremGA20_channel[i].play = 0;
	}
}

int IremGA20_sh_start(const struct MachineSound *msound)
{
	int i;
	const char *names[2];
	char ch_names[2][40];

	if (!Machine->sample_rate) return 0;

	/* Initialize our chip structure */
	IremGA20_chip.intf = msound->sound_interface;
	IremGA20_chip.mode = 0;
	IremGA20_chip.rom = memory_region(IremGA20_chip.intf->region);
	IremGA20_chip.rom_size = memory_region_length(IremGA20_chip.intf->region);

//AT: little kludges to speed up execution
	// change the sinage of PCM samples in advance
	for (i=0; i<IremGA20_chip.rom_size; i++)
		IremGA20_chip.rom[i] -= 0x80;

	sr_exponent = log(DEF_MAX_SR)/log(256);
//ZT
	IremGA20_reset();

	for ( i = 0; i < 0x40; i++ )
		IremGA20_chip.regs[i] = 0;

	for ( i = 0; i < 2; i++ ) {
		names[i] = ch_names[i];
		sprintf(ch_names[i],"%s Ch %d",sound_name(msound),i);
	}

	IremGA20_chip.channel = stream_init_multi( 2, names,
						IremGA20_chip.intf->mixing_level, Machine->sample_rate,
						0, IremGA20_update );

	return 0;
}

void IremGA20_sh_stop( void )
{
}
