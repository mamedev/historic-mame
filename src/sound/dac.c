#include "driver.h"


static int channel[MAX_DAC];
static signed char latch[MAX_DAC];
static signed char UnsignedVolTable[256];
static signed char SignedVolTable[256];


static void DAC_update(int num,void *buffer,int length)
{
	memset(buffer,latch[num],length);
}


void DAC_data_w(int num,int data)
{
	/* update the output buffer before changing the registers */
	if (latch[num] != UnsignedVolTable[data])
	{
		stream_update(channel[num],0);
		latch[num] = UnsignedVolTable[data];
	}
}


void DAC_signed_data_w(int num,int data)
{
	/* update the output buffer before changing the registers */
	if (latch[num] != SignedVolTable[data])
	{
		stream_update(channel[num],0);
		latch[num] = SignedVolTable[data];
	}
}


static void DAC_build_voltable(void)
{
	int i;


	/* build volume table (linear) */
	for (i = 0;i < 256;i++)
	{
		UnsignedVolTable[i] = i / 2;	/* range    0..127 */
		SignedVolTable[i] = i - 0x80;	/* range -128..127 */
	}
}


int DAC_sh_start(const struct MachineSound *msound)
{
	int i;
	const struct DACinterface *intf = msound->sound_interface;


	DAC_build_voltable();

	for (i = 0;i < intf->num;i++)
	{
		char name[40];


		sprintf(name,"DAC #%d",i);
		channel[i] = stream_init(name,intf->mixing_level[i],Machine->sample_rate,8,
				i,DAC_update);

		if (channel[i] == -1)
			return 1;

		latch[i] = 0;
	}

	return 0;
}
