#include "driver.h"
#include "Z80.h"
#include "sndhrdw/generic.h"
#include "sndhrdw/8910intf.h"
#include "sndhrdw/dac.h"



static int sndnmi_disable = 1;
static unsigned char voltable[255];

static struct DACinterface DAinterface =
{
	1,
	441000,
	{ 128, 128 },
	{  1,  1 }
};



static void taito_sndnmi_msk(int offset,int data)
{
	sndnmi_disable = data & 0x01;
}



static void taito_digital_out(int offset,int data)
{
	DAC_data_w(0,voltable[data]);
}



int taito_sh_interrupt(void)
{
	static int count;


	count++;
	if (count>=16){
		count = 0;
		return 0xff;
	}
	else
	{
		if (pending_commands && !sndnmi_disable) return Z80_NMI_INT;
		else return Z80_IGNORE_INT;
	}
}



static struct AY8910interface interface =
{
	4,	/* 4 chips */
	1500000,	/* 1.5 MHz */
	{ 255, 255, 255, 0x40ff },
	{ input_port_5_r, 0, 0, 0 },		/* port Aread */
	{ input_port_6_r, 0, 0, 0 },		/* port Bread */
	{ 0, taito_digital_out, 0, 0 },				/* port Awrite */
	{ 0, 0, 0, taito_sndnmi_msk }	/* port Bwrite */
};



int taito_sh_start(void)
{
	int i,j;
	int weight[8],totweight;


	pending_commands = 0;

	if( DAC_sh_start(&DAinterface) ) return 1;
	if( AY8910_sh_start(&interface) ){
		DAC_sh_stop();
		return 1;
	}

	/* reproduce the resistor ladder

	   -- 30 -+
	          15
	   -- 30 -+
	          15
	   -- 30 -+
	          15
	   -- 30 -+
	          15
	   -- 30 -+
	          15
	   -- 30 -+
	          15
	   -- 30 -+
	          15
	   -- 30 -+-------- out
	*/

	totweight = 0;
	for (i = 0;i < 8;i++)
	{
		weight[i] = 75600 / (30 + (7-i) * 15);
		totweight += weight[i];
	}

	for (i = 0;i < 8;i++)
		weight[i] = (255 * weight[i] + totweight / 2) / totweight;

	for (i = 0;i < 256;i++)
	{
		voltable[i] = 0;

		for (j = 0;j < 8;j++)
		{
			if ((i >> j) & 1)
				voltable[i] += weight[j];
		}
	}

	return 0;
}

void taito_sh_stop(void)
{
	AY8910_sh_stop();
	DAC_sh_stop();
}

void taito_sh_update(void)
{
	AY8910_sh_update();
	DAC_sh_update();
}
