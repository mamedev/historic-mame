#include "driver.h"
#include "generic.h"
#include "sn76496.h"
#include "dac.h"
#include "vlm5030.h"

/* filename for trackn field sample files */
const char *trackfld_sample_names[] =
{
	"speech00","speech01","speech02","speech03",
	"speech04","speech05","speech06","speech07",
	"speech08","speech09","speech0a","speech0b",
	"speech0c","speech0d","speech0e","speech0f",
	"speech10","speech11","speech12","speech13",
	"speech14","speech15","speech16","speech17",
	"speech18","speech19","speech1a","speech1b",
	"speech1c","speech1d","speech1e","speech1f",
	"speech20","speech21","speech22","speech23",
	"speech24","speech25","speech26","speech27",
	"speech28","speech29","speech2a","speech2b",
	"speech2c","speech2d","speech2e","speech2f",
	"speech30","speech31","speech32","speech33",
	"speech34","speech35","speech36","speech37",
	"speech38","speech39","speech3a","speech3b",
	"speech3c","speech3d",
	0
};

/* filename for hyper sports sample files */
const char *hyperspt_sample_names[] =
{
	"speech00","speech01","speech02","speech03",
	"speech04","speech05","speech06","speech07",
	"speech08","speech09","speech0a","speech0b",
	"speech0c","speech0d","speech0e","speech0f",
	"speech10","speech11","speech12","speech13",
	"speech14","speech15","speech16","speech17",
	"speech18","speech19","speech1a","speech1b",
	"speech1c","speech1d","speech1e","speech1f",
	"speech20","speech21","speech22","speech23",
	"speech24","speech25","speech26","speech27",
	"speech28","speech29","speech2a","speech2b",
	"speech2c","speech2d","speech2e","speech2f",
	"speech30","speech31","speech32","speech33",
	"speech34","speech35","speech36","speech37",
	"speech38","speech39","speech3a","speech3b",
	"speech3c","speech3d","speech3e","speech3f",
	"speech40","speech41","speech42","speech43",
	"speech44","speech45","speech46","speech47",
	"speech48","speech49",
	0
};


/* VLM5030 speech command */
unsigned char *konami_speech;

static struct VLM5030interface vlm5030_interface =
{
    3580000,    /* master clock */
    255         /* volume       */
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

static struct DACinterface DAinterface =
{
	1,
	441000,
	{255,255 },
	{  1,  1 }
};

int trackfld_sh_timer_r(int offset)
{
	int clock;

#define trackfld_TIMER_RATE (14318180/4096)

	clock = (cpu_gettotalcycles()*4) / trackfld_TIMER_RATE;

	return clock & 0xF;
}

int trackfld_speech_r(int offset)
{
	return VLM5030_busy() ? 0x10 : 0;
}

static int last_addr = 0;

void trackfld_sound_w(int offset , int data)
{
	if( (offset & 0x07) == 0x03 )
	{
		/* A7 = data enable for VLM5030 (don't care )          */
		/* A8 = STA pin (1->0 data data  , 0->1 start speech   */
		/* A9 = RST pin 1=reset                                */
		if( ( (offset^last_addr) & offset) & 0x100 )
		{
			/* start speech */
			VLM5030_w( 0 , *konami_speech );
		}
#if 0
		if( ( (offset^last_addr) & offset) &0x200 )
		{
			VLM5030_reset();
		}
#endif
	}
	last_addr = offset;
}

int hyperspt_sh_timer_r(int offset)
{
	int clock;
#define hyperspt_TIMER_RATE (14318180/4096)

	clock = (cpu_gettotalcycles()*4) / hyperspt_TIMER_RATE;

	return (clock & 0x3) | (VLM5030_busy()? 0x04 : 0);
}

void hyperspt_sound_w(int offset , int data)
{
	/* A3 = data enable for VLM5030 (don't care )          */
	/* A4 = STA pin (1->0 data data  , 0->1 start speech   */
	/* A5 = RST pin 1=reset                                */
	/* A6 = VLM5030    output disable (don't care ) */
	/* A7 = kONAMI DAC output disable (don't care ) */
	/* A8 = SN76489    output disable (don't care ) */
	if( ( (offset^last_addr) & offset) & 0x0010 )
	{
		/* start speech */
		VLM5030_w( 0 , *konami_speech );
	}
#if 0
	if( ( (offset^last_addr) & offset) &0x0020 )
	{
		VLM5030_reset();
	}
#endif
	last_addr = offset;
}



static struct SN76496interface SNinterface =
{
	1,	/* 1 chip */
	14318180/8,	/*  1.7897725 Mhz */
	{ 255*2, 255*2 }
};



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



int konami_sh_start(void)
{
	if( DAC_sh_start(&DAinterface) ) return 1;
	if (SN76496_sh_start(&SNinterface) != 0)
	{
		DAC_sh_stop();
		return 1;
	}
#if 0
	/* real time speech sounds emuration */
	if( VLM5030_sh_start( &vlm5030_interface ,  Machine->memory_region[3] ) != 0)
#else
	/* use sample files for speech sounds */
	if( VLM5030_sh_start( &vlm5030_interface ,  0 ) != 0)
#endif
	{
		SN76496_sh_stop();
		DAC_sh_stop();
		return 1;
	}
	return 0;
}

void konami_sh_stop(void)
{
	SN76496_sh_stop();
	DAC_sh_stop();
	VLM5030_sh_stop();
}

void konami_sh_update(void)
{
	SN76496_sh_update();
	DAC_sh_update();
    VLM5030_sh_update();
}
