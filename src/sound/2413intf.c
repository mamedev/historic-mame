/****************************************************************

	MAME / MESS functions

****************************************************************/

#include "driver.h"
#include "ym2413.h"

static OPLL *opll[MAX_2413];
static int stream[MAX_2413], ym_latch[MAX_2413], num;

static void YM2413_update (int ch, INT16 *buffer, int length)
{
	while (length--) *buffer++ = OPLL_calc (opll[ch]);
}

int YM2413_sh_start (const struct MachineSound *msound)
{
	const struct YM2413interface *intf = msound->sound_interface;
	int i;
	char buf[40];

	OPLL_init (intf->baseclock/2, Machine->sample_rate);
	num = intf->num;

	for (i=0;i<num;i++)
		{
		opll[i] = OPLL_new ();
		if (!opll[i]) return 1;
		OPLL_reset (opll[i]);
		OPLL_reset_patch (opll[i]);

		if (num > 1)
			sprintf (buf, "YM-2413 #%d", i);
		else
			strcpy (buf, "YM-2413");

		stream[i] = stream_init (buf, intf->mixing_level[i],
			Machine->sample_rate, i, YM2413_update);
		}

	return 0;
}

void YM2413_sh_stop (void)
{
	int i;

	for (i=0;i<num;i++)
	{
		OPLL_delete (opll[i]);
	}
	OPLL_close ();
}

void YM2413_sh_reset (void)
{
	int i;

	for (i=0;i<num;i++)
	{
		OPLL_reset (opll[i]);
		OPLL_reset_patch (opll[i]);
	}
}

WRITE_HANDLER( YM2413_register_port_0_w ) { ym_latch[0] = data; }
WRITE_HANDLER( YM2413_register_port_1_w ) { ym_latch[1] = data; }
WRITE_HANDLER( YM2413_register_port_2_w ) { ym_latch[2] = data; }
WRITE_HANDLER( YM2413_register_port_3_w ) { ym_latch[3] = data; }

static void YM2413_write_reg (int chip, int data)
{
	OPLL_writeReg (opll[chip], ym_latch[chip], data);
	stream_update(stream[chip], chip);
}

WRITE_HANDLER( YM2413_data_port_0_w ) { YM2413_write_reg (0, data); }
WRITE_HANDLER( YM2413_data_port_1_w ) { YM2413_write_reg (1, data); }
WRITE_HANDLER( YM2413_data_port_2_w ) { YM2413_write_reg (2, data); }
WRITE_HANDLER( YM2413_data_port_3_w ) { YM2413_write_reg (3, data); }

