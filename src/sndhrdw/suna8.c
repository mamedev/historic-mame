/*

	SunA 8 Bit Games samples
	
	Format: PCM unsigned 8 bit mono 4Khz

*/

#include "driver.h"

static int channel;
static signed char *samplebuf;
static int sample;

WRITE8_HANDLER( suna8_play_samples_w )
{
	if( data )
	{
		if( ~data & 0x10 )
		{
			mixer_play_sample(channel, &samplebuf[0x800*sample], 0x0800, 4000, 0);
		}
		else if( ~data & 0x08 )
		{
			sample &= 3;
			mixer_play_sample(channel, &samplebuf[0x800*(sample+7)], 0x0800, 4000, 0);
		}
	}
}

WRITE8_HANDLER( suna8_samples_number_w )
{
	sample = data & 0xf;
}

int suna8_sh_start(const struct MachineSound *msound)
{
	int i;
	unsigned char *ROM = memory_region(REGION_SOUND1);
	
	channel = mixer_allocate_channel(50);
	mixer_set_name(channel,"Samples");

	samplebuf = auto_malloc(memory_region_length(REGION_SOUND1));
	if( !samplebuf )
		return 1;

	for(i=0;i<memory_region_length(REGION_SOUND1);i++)
		samplebuf[i] = ROM[i] ^ 0x80;

	return 0;
}
