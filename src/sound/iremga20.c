/*********************************************************

	Irem GA20 PCM Sound Chip

	It's not currently known whether this chip is stereo.

*********************************************************/

#include "driver.h"
#include "iremga20.h"

static struct IremGA20_channel_def {
	unsigned long		rate;
	unsigned long		size;
	unsigned long		start;
	unsigned long		end;
	unsigned long		volume;
	int					play;
	unsigned long		pan;
	unsigned long		pos;
} IremGA20_channel[4];

static struct IremGA20_chip_def {
	const struct IremGA20_interface	*intf;
	int								channel;
	int								mode;
	int								regs[0x40];
	unsigned char					*rom;
	int								rom_size;
} IremGA20_chip;

static unsigned long delta;

static void IremGA20_reset( void )
{
	int i;

	for( i = 0; i < 4; i++ ) {
		IremGA20_channel[i].rate = 0;
		IremGA20_channel[i].size = 0;
		IremGA20_channel[i].start = 0;
		IremGA20_channel[i].end = 0;
		IremGA20_channel[i].volume = 0;
		IremGA20_channel[i].play = 0;
		IremGA20_channel[i].pan = 0;
		IremGA20_channel[i].pos = 0;
	}
}

INLINE int limit( int val, int max, int min ) {
	if ( val > max )
		val = max;
	else if ( val < min )
		val = min;

	return val;
}

#define MAXOUT 0x7fff
#define MINOUT -0x8000

void IremGA20_update( int param, INT16 **buffer, int length )
{
	int i, j, lvol[4], rvol[4], play[4], loop[4];
	unsigned char *rom=memory_region(IremGA20_chip.intf->region);
	unsigned long end[4], pos[4];
	int dataL, dataR;
	signed char d;

	if (Machine->sample_rate==0) return;

	/* precache some values */
	for ( i = 0; i < 4; i++ ) {
		lvol[i] = IremGA20_channel[i].volume;// * IremGA20_channel[i].pan;
		rvol[i] = IremGA20_channel[i].volume;// * ( 8 - IremGA20_channel[i].pan );
		end[i] = IremGA20_channel[i].end;
		pos[i] = IremGA20_channel[i].pos;
		play[i] = IremGA20_channel[i].play;
		loop[i] = 0;//[i].loop;
	}

	for ( j = 0; j < length; j++ ) {
		dataL = dataR = 0;
		for ( i = 0; i < 4; i++ ) {
			if ( play[i] ) {
				if ((pos[i]>>8) >= end[i]-0x20 ) {
					play[i] = 0;
					continue;
				}

				d = (*(rom+(pos[i]>>8)))-0x80;
				pos[i]+=delta;//(1<<6);

				dataL += ( d * lvol[i] ) >> 2;
				dataR += ( d * rvol[i] ) >> 2;
			}
		}
		buffer[1][j] = limit( dataL, MAXOUT, MINOUT );
		buffer[0][j] = limit( dataR, MAXOUT, MINOUT );
	}

	/* update the regs now */
	for ( i = 0; i < 4; i++ ) {
		IremGA20_channel[i].pos = pos[i];
		IremGA20_channel[i].play = play[i];
	}
}

int IremGA20_sh_start(const struct MachineSound *msound)
{
	const char *names[2];
	char ch_names[2][40];
	int i;

	/* Initialize our chip structure */
	IremGA20_chip.intf = msound->sound_interface;
	IremGA20_chip.mode = 0;
	IremGA20_chip.rom = memory_region(IremGA20_chip.intf->region);
	IremGA20_chip.rom_size = memory_region_length(IremGA20_chip.intf->region) - 1;

	if (Machine->sample_rate==0) return 0;
	delta=IremGA20_chip.intf->clock/Machine->sample_rate;

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

WRITE_HANDLER( IremGA20_w )
{
	int channel = offset / 0x10;

	logerror("GA20:  Offset %02x, data %04x\n",offset,data);

	if ( Machine->sample_rate != 0 )
		stream_update( IremGA20_chip.channel, 0 );

	IremGA20_chip.regs[offset] = data;
	switch ( offset&0xf ) {
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

		case 0xc: /* Volume? */
			if (data==0) IremGA20_channel[channel].play=0;
			else IremGA20_channel[channel].play=1;
			IremGA20_channel[channel].pos=IremGA20_channel[channel].start<<8;
			IremGA20_channel[channel].volume=0x7f;//data<<4; ?
			break;
	}
}

READ_HANDLER( IremGA20_r )
{
	/* Todo - Looks like there is a status bit to show whether each channel is playing */
	return 0xff;
}
