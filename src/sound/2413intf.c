/****************************************************************

	MAME / MESS functions

****************************************************************/

#include "driver.h"
#include "ym2413.h"

static int num;

void YM2413DAC_update(int chip,INT16 *buffer,int length)
{
	static int out = 0;

	if ( ym2413[chip].reg[0x0F] & 0x01 )
	{
		out = ((ym2413[chip].reg[0x10] & 0xF0) << 7);
	}
	while (length--) *(buffer++) = out;
}

int YM2413_sh_start (const struct MachineSound *msound)
{
	const struct YM2413interface *intf = msound->sound_interface;
	int i, tst;
	char name[40];

	num = intf->num;

	tst = YM3812_sh_start (msound);
	if (tst)
		return 1;

	for (i=0;i<num;i++)
	{
		ym2413_reset (i);
		sprintf(name,"YM-2413 DAC #%d",i);

		ym2413[i].DAC_stream = stream_init(name,intf->mixing_level[i],
		                       Machine->sample_rate, i, YM2413DAC_update);

		if (ym2413[i].DAC_stream == -1)
			return 1;
	}
	return 0;
}

void YM2413_sh_stop (void)
{
	int i;

	for (i=0;i<num;i++)
	{
		ym2413_reset(i);
	}
	YM3812_sh_stop ();
}

void YM2413_sh_reset (void)
{
	int i;

	for (i=0;i<num;i++)
	{
		ym2413_reset(i);
	}
}


WRITE_HANDLER( YM2413_register_port_0_w ) { ym2413_write (0, 0, data); } /* 1st chip */
WRITE_HANDLER( YM2413_register_port_1_w ) { ym2413_write (1, 0, data); } /* 2nd chip */
WRITE_HANDLER( YM2413_register_port_2_w ) { ym2413_write (2, 0, data); } /* 3rd chip */
WRITE_HANDLER( YM2413_register_port_3_w ) { ym2413_write (3, 0, data); } /* 4th chip */

WRITE_HANDLER( YM2413_data_port_0_w ) { ym2413_write (0, 1, data); } /* 1st chip */
WRITE_HANDLER( YM2413_data_port_1_w ) { ym2413_write (1, 1, data); } /* 2nd chip */
WRITE_HANDLER( YM2413_data_port_2_w ) { ym2413_write (2, 1, data); } /* 3rd chip */
WRITE_HANDLER( YM2413_data_port_3_w ) { ym2413_write (3, 1, data); } /* 4th chip */

WRITE16_HANDLER( YM2413_register_port_0_lsb_w ) { if (ACCESSING_LSB) YM2413_register_port_0_w(offset,data & 0xff); }
WRITE16_HANDLER( YM2413_register_port_1_lsb_w ) { if (ACCESSING_LSB) YM2413_register_port_1_w(offset,data & 0xff); }
WRITE16_HANDLER( YM2413_register_port_2_lsb_w ) { if (ACCESSING_LSB) YM2413_register_port_2_w(offset,data & 0xff); }
WRITE16_HANDLER( YM2413_register_port_3_lsb_w ) { if (ACCESSING_LSB) YM2413_register_port_3_w(offset,data & 0xff); }
WRITE16_HANDLER( YM2413_data_port_0_lsb_w ) { if (ACCESSING_LSB) YM2413_data_port_0_w(offset,data & 0xff); }
WRITE16_HANDLER( YM2413_data_port_1_lsb_w ) { if (ACCESSING_LSB) YM2413_data_port_1_w(offset,data & 0xff); }
WRITE16_HANDLER( YM2413_data_port_2_lsb_w ) { if (ACCESSING_LSB) YM2413_data_port_2_w(offset,data & 0xff); }
WRITE16_HANDLER( YM2413_data_port_3_lsb_w ) { if (ACCESSING_LSB) YM2413_data_port_3_w(offset,data & 0xff); }


