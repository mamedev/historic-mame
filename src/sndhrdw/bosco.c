#include "driver.h"
#include "sndhrdw/8910intf.h"


static unsigned char speech[0x6000];	/* 24k for speech */
int pengo_sh_start(void);

int bosco_sh_start(void)
{
	int i;
	unsigned char bits;

	/* decode the rom samples */
	for (i = 0;i < 0x3000;i++)
	{
		bits = Machine->memory_region[4][i] & 0x0f;
		speech[2 * i] = ((bits << 4) | bits) + 0x80;

		bits = Machine->memory_region[4][i] & 0xf0;
		speech[2 * i + 1] = (bits | (bits >> 4)) + 0x80;
	}

	return pengo_sh_start();
}

void bosco_sample_play(int offset, int length)
{
	if (Machine->sample_rate == 0)
		return;

	osd_play_sample(4,speech + offset,length,4000,0xff,0);
}
