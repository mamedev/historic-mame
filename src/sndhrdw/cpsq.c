/***************************************************************************

  Capcom System Q Sound
  =====================

  A sample player.

  Register
  0     xxbb    xx = unknown bb = start high address
  1     ssss    ssss = sample start address
  2
  3
  4
  5
  6
  7
  8
  9

***************************************************************************/


#include "driver.h"


#define USE_QSOUND 0

struct QSOUND_CHANNEL
{
    int playing;
	int reg0;
	int reg1;
	int reg2;
	int reg3;
	int reg4;
	int reg5;
	int reg6;
	int reg7;
	int reg8;
	int reg9;
};

#define qsound_channels 16
static struct QSOUND_CHANNEL qsound_channel[qsound_channels];
#if USE_QSOUND
static int channel;
#endif

void cpsq_set_command(int data, int value)
{
    if (errorlog)
	{
        int ch=0,reg=0;
		if (data < 0x80)
		{
			ch=data>>3;
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
                        /* Unknown registers */
						ch=99;
						reg=99;
					}
			}
		}


		switch (reg)
		{
			case 0:
				/* strange ... */
				ch=(ch+1)&0x0f;
				qsound_channel[ch].reg0=value;
				break;
			case 1:
                qsound_channel[ch].playing=0;
				qsound_channel[ch].reg1=value;
				break;
			case 2:
				qsound_channel[ch].reg2=value;
				break;
			case 3:
				qsound_channel[ch].reg3=value;
				break;
			case 4:
				qsound_channel[ch].reg4=value;
				break;
			case 5:
				qsound_channel[ch].reg5=value;
				break;
			case 6:
				qsound_channel[ch].reg6=value;
				break;
			case 7:
				qsound_channel[ch].reg7=value;
				break;
			case 8:
				qsound_channel[ch].reg8=value;
				break;
			case 9:
				qsound_channel[ch].reg9=value;
				break;

		}
        fprintf(errorlog, "QSOUND WRITE %02x CH%02d-R%02d =%04x\n", data, ch, reg, value);
	}
}


int cpsq_sh_start(const struct MachineSound *msound)
{
#if USE_QSOUND
    int vol[qsound_channels];
    int i;

	if (Machine->sample_rate == 0) return 0;

	for (i=0; i<qsound_channels; i++)
	{
		vol[i]=0xff;
	}

	/* Allocate sound channels */
    channel = mixer_allocate_channels(qsound_channels, vol);

    for (i=0; i<qsound_channels; i++)
    {
        mixer_set_name(channel+i,"QSound channel");
    }
#endif
	return 0;
}

void cpsq_sh_stop (void)
{
	if (Machine->sample_rate == 0) return;
}

void cpsq_sh_update(void)
{
#if USE_QSOUND
    int i;
    signed char *Samples = (signed char *)Machine->memory_region[3];
	unsigned int SamplesLength=Machine->memory_region_length[3];
	static int old_start;

    if (Machine->sample_rate == 0) return;


    for (i=0; i<qsound_channels; i++)
    {
        unsigned int start = ((qsound_channel[i].reg0 & 0xff)<<16)+(qsound_channel[i].reg1);
        unsigned int length=0x2000;

        if (start && !qsound_channel[i].playing)
        {
			old_start=start;
			if (start + length < SamplesLength)
			{
                mixer_play_sample(channel+i,Samples+start, length,16000, 0);

                /*
                if (errorlog)
				{
					fprintf(errorlog, "QSOUND SAMPLE= %08x \n", start);
				}
                */
			}
            qsound_channel[i].playing=1;
        }
    }
#endif
}
