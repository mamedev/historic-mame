/***************************************************************************

  Capcom System Q Sound
  =====================

  A big sample player. I don't know how this works yet.


***************************************************************************/




#include "driver.h"

#define CLICKY_NOISES 0

#if CLICKY_NOISES
static signed char *buffer1;
static int channel;
const int cpsq_buffer_len = 4096;
#endif


void cpsq_set_command(int data, int value)
{
#if CLICKY_NOISES
	/* Dummy routine, copy first part of Q sample ROMS */
	unsigned char *Samples = Machine->memory_region[3];
	memcpy(buffer1, Samples, cpsq_buffer_len);
#endif
	if (errorlog)
	{
		int ch,reg;
		ch=0;
		reg=0;
		if (data < 0x80)
		{
			ch=data/8;
			reg=data & 0x07;
		}
		else
		{
			if (data < 0x90)
			{
				ch=data-0x80;
				reg=8;
			}
			else
			{
					if (data >= 0xba && data < 0xca)
					{
						ch=data-0xba;
						reg=9;
					}
					else
					{

						ch=99;
						reg=99;
					}
			}
		}
		fprintf(errorlog, "QSOUND WRITE %02x CH%02d-R%02d =%04x\n", data, ch, reg, value);
	}

}


int cpsq_sh_start(const struct MachineSound *msound)
{
#if CLICKY_NOISES
	int vol[1];

	/* allocate the buffers */
	buffer1 = malloc(cpsq_buffer_len);
	if (buffer1 == 0 )
	{
		free(buffer1);
		return 1;
	}
	memset(buffer1,0,cpsq_buffer_len);

	/* request a sound channel */
	vol[0] = 25;
	channel = mixer_allocate_channels(1,vol);
	mixer_set_name(channel+0,"streamed DAC #0");
#endif
	return 0;
}

void cpsq_sh_stop (void)
{
#if CLICKY_NOISES
	free(buffer1);
	buffer1 = 0;
#endif
}

void cpsq_sh_update(void)
{
#if CLICKY_NOISES
    int data, dacpos;
    signed char *buf;

    if (Machine->sample_rate == 0) return;

	mixer_play_streamed_sample(channel+0,buffer1,cpsq_buffer_len,cpsq_buffer_len * Machine->drv->frames_per_second);
	memset(buffer1,0,cpsq_buffer_len);
#endif
}
