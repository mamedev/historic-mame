#include "driver.h"

struct VLM5030interface konami_vlm5030_interface =
{
    3580000,    /* master clock  */
    255,        /* volume        */
    3,         /* memory region  */
    0,         /* VCU            */
};

struct SN76496interface konami_sn76496_interface =
{
	1,	/* 1 chip */
	14318180/8,	/*  1.7897725 Mhz */
	{ 255 }
};

struct DACinterface konami_dac_interface =
{
	1,
	441000,
	{255,255 },
	{  1,  1 }
};

unsigned char *konami_dac;

/* The timer port on TnF and HyperSports sound hardware is derived from
   a 14.318 mhz clock crystal which is passed  through a couple of 74ls393
	ripple counters.
	Various outputs of the ripper counters clock the various chips.
	The Z80 uses 14.318 mhz / 4 (3.4mhz)
	The SN chip uses 14.318 ,hz / 8 (1.7mhz)
	And the timer is connected to 14.318 mhz / 4096
	As we are using the Z80 clockrate as a base value we need to multiply
	the no of cycles by 4 to undo the 14.318/4 operation
*/

int trackfld_sh_timer_r(int offset)
{
	int clock;

#define trackfld_TIMER_RATE (14318180/4096)

	clock = (cpu_gettotalcycles()*4) / trackfld_TIMER_RATE;

	return clock & 0xF;
}

int trackfld_speech_r(int offset)
{
	return VLM5030_BSY() ? 0x10 : 0;
}

static int last_addr = 0;

void trackfld_sound_w(int offset , int data)
{
	if( (offset & 0x07) == 0x03 )
	{
		int changes = offset^last_addr;
		/* A7 = data enable for VLM5030 (don't care )          */
		/* A8 = STA pin (1->0 data data  , 0->1 start speech   */
		/* A9 = RST pin 1=reset                                */

		/* A8 VLM5030 ST pin */
		if( changes & 0x100 )
			VLM5030_ST( offset&0x100 );
		/* A9 VLM5030 RST pin */
		if( changes & 0x200 )
			VLM5030_RST( offset&0x200 );
	}
	last_addr = offset;
}

int hyperspt_sh_timer_r(int offset)
{
	int clock;
#define hyperspt_TIMER_RATE (14318180/4096)

	clock = (cpu_gettotalcycles()*4) / hyperspt_TIMER_RATE;

	return (clock & 0x3) | (VLM5030_BSY()? 0x04 : 0);
}

void hyperspt_sound_w(int offset , int data)
{
	int changes = offset^last_addr;
	/* A3 = data enable for VLM5030 (don't care )          */
	/* A4 = STA pin (1->0 data data  , 0->1 start speech   */
	/* A5 = RST pin 1=reset                                */
	/* A6 = VLM5030    output disable (don't care ) */
	/* A7 = kONAMI DAC output disable (don't care ) */
	/* A8 = SN76489    output disable (don't care ) */

	/* A4 VLM5030 ST pin */
	if( changes & 0x10 )
		VLM5030_ST( offset&0x10 );
	/* A5 VLM5030 RST pin */
	if( changes & 0x20 )
		VLM5030_RST( offset&0x20 );

	last_addr = offset;
}



void konami_dac_w(int offset,int data)
{

	*konami_dac = data;

	/* bit 7 doesn't seem to be used */
	DAC_data_w(0,data<<1);
}



void konami_sh_irqtrigger_w(int offset,int data)
{
	static int last;


//	if (last == 0 && data == 1)
	if (last == 0 && (data))
	{
		/* setting bit 0 low then high triggers IRQ on the sound CPU */
		cpu_cause_interrupt(1,0xff);
	}

	last = data;
}
