/****************************************************************************
 *
 * geebee.c
 *
 * sound driver
 * juergen buchmueller <pullmoll@t-online.de>, jan 2000
 *
 ****************************************************************************/

#include "driver.h"

static int channel;
static int sound_latch = 0;
static int signal = 0;
static int volume = 0;
static int noise = 0;

void geebee_sound_w(int offs, int data)
{
    stream_update(channel,0);
	sound_latch = data;
	volume = 0x7fff; /* set volume */
	noise = 0x0000;  /* reset noise shifter */
}

static void geebee_sound_update(int param, INT16 *buffer, int length)
{
    static int vcarry = 0;
    static int vcount = 0;

    while (length--)
    {
		if (sound_latch & 8)
			*buffer++ = signal / 2;
        else
			*buffer++ = signal;
        vcarry -= 18432000 / 6 / 256;
        while (vcarry < 0)
        {
            vcarry += Machine->sample_rate;
            vcount++;
			switch (sound_latch & 7)
            {
            case 0: /* 4V */
				signal = (vcount & 4) ? volume : 0;
                break;
            case 1: /* 8V */
				signal = (vcount & 8) ? volume : 0;
                break;
            case 2: /* 16V */
				signal = (vcount & 16) ? volume : 0;
                break;
            case 3: /* 32V */
				signal = (vcount & 32) ? volume : 0;
                break;
            case 4: /* TONE1 */
				signal = !(vcount & 1) && !(vcount & 16) ? volume : 0;
                break;
            case 5: /* TONE2 */
				signal = !(vcount & 2) && !(vcount & 32) ? volume : 0;
                break;
            case 6: /* TONE3 */
				signal = !(vcount & 4) && !(vcount & 64) ? volume : 0;
                break;
            default: /* NOISE */
                /* QH of 74164 #4V */
				signal = (noise & 0x8000) ? volume : 0;
                /* clocked with raising edge of 2V */
                if ((vcount & 3) == 2)
                {
                    /* bit0 = bit0 ^ !bit10 */
					if ((noise & 1) == ((noise >> 10) & 1))
						noise = ((noise << 1) & 0xfffe) | 1;
                    else
						noise = (noise << 1) & 0xfffe;
                }
            }
			if ((volume -= 8) < 0)
				volume = 0;
        }
    }
}

int geebee_sh_start(const struct MachineSound *msound)
{
	channel = stream_init("GeeBee", 100, Machine->sample_rate, 0, geebee_sound_update);
    return 0;
}

void geebee_sh_stop(void)
{
}

void geebee_sh_update(void)
{
    stream_update(channel,0);
}

