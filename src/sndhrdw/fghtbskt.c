/*

Fighting Basketball PCM unsigned 8 bit mono samples

*/

#include "driver.h"

static int channel;
static signed char *samplebuf;

WRITE8_HANDLER( fghtbskt_samples_w )
{
	if( data & 1 )
		mixer_play_sample(channel, samplebuf + ((data & 0xf0) << 8), 0x2000, 8000, 0);	
}

int fghtbskt_sh_start(const struct MachineSound *msound)
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
