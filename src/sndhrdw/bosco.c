#include "driver.h"

/* signed/unsigned 8-bit conversion macros */
#define AUDIO_CONV(A) ((A)-0x80)


static signed char *speech;	/* 24k for speech */
static int channel;



int bosco_sh_start(void)
{
	int i;
	unsigned char bits;

	channel = get_play_channels(1);

	speech = malloc(0x6000);
	if (!speech)
		return 1;

	/* decode the rom samples */
	for (i = 0;i < 0x3000;i++)
	{
		bits = Machine->memory_region[5][i] & 0x0f;
		speech[2 * i] = AUDIO_CONV ((bits << 4) | bits);

		bits = Machine->memory_region[5][i] & 0xf0;
		speech[2 * i + 1] = AUDIO_CONV (bits | (bits >> 4));
	}

	return 0;
}


void bosco_sh_stop (void)
{
	if (speech)
		free(speech);
	speech = NULL;
}


void bosco_sample_play(int offset, int length)
{
	if (Machine->sample_rate == 0)
		return;

	osd_play_sample(channel,speech + offset,length,4000,0xff,0);
}
