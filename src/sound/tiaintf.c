/***************************************************************************

   tiaintf.c

   Interface the routines in tiasound.c to MESS

***************************************************************************/

#include "driver.h"
#include "sound/tiasound.h"
#include "sound/tiaintf.h"

#define MIN_SLICE 10    /* minimum update step */

static unsigned int buffer_len;
static unsigned int emulation_rate;
static unsigned int sample_pos;

static int channel;

static const struct TIAinterface *intf;
static unsigned char *buffer;

int tia_sh_start (const struct TIAinterface *interface)
{
	int i, res;

	intf = interface;

	buffer_len = Machine->sample_rate / Machine->drv->frames_per_second;
	if (buffer_len == 0) return 0;
	emulation_rate = buffer_len * Machine->drv->frames_per_second;
	sample_pos = 0;

	channel = get_play_channels(1);

	if ((buffer = malloc(buffer_len)) == 0)
		return 1;
	memset(buffer,0,buffer_len);

	Tia_sound_init (intf->clock, emulation_rate);
	return 0;
}


void tia_sh_stop (void)
{
	/* Nothing to do here */
}

void tia_sh_update (void)
{
	if (Machine->sample_rate == 0) return;

	if (sample_pos < buffer_len)
		Tia_process (buffer + sample_pos, buffer_len - sample_pos);
	sample_pos = 0;

	osd_play_streamed_sample(channel,(signed char *)buffer,buffer_len,emulation_rate,intf->volume, OSD_PAN_CENTER);
}
