/*************************************************************************

  Gauntlet sound hardware

*************************************************************************/

#include "mame.h"
#include "5220intf.h"
#include "pokyintf.h"
#include "2151intf.h"

static int mix;
static int speech_val;
static int last_speech_write;


/*************************************
 *
 *		Sound interfaces
 *
 *************************************/

static struct POKEYinterface pokey_interface =
{
	1,	/* 1 chip */
	FREQ_17_APPROX,	/* 1.7 Mhz */
	128,
	NO_CLIP,
	/* The 8 pot handlers */
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 },
	/* The allpot handler */
	{ 0 }
};

static struct TMS5220interface tms5220_interface =
{
    640000,     /* clock speed (80*samplerate) */
    255,        /* volume */
    0           /* irq handler */
};

static struct YM2151interface ym2151_interface =
{
	1,			/* 1 chip */
	3580000,	/* 3.58 MHZ ? */
	{ 255 },
	{ 0 }
};



/*************************************
 *
 *		Sound startup.
 *
 *************************************/

int gauntlet_sh_start (void)
{
	last_speech_write = 0x80;
	if (pokey_sh_start (&pokey_interface))
		return 1;
	if (YM2151_sh_start (&ym2151_interface))
		return 1;
	return tms5220_sh_start (&tms5220_interface);
}


/*************************************
 *
 *		Sound shutdown.
 *
 *************************************/

void gauntlet_sh_stop (void)
{
    pokey_sh_stop ();
    YM2151_sh_stop ();
    tms5220_sh_stop ();
}


/*************************************
 *
 *		Sound update.
 *
 *************************************/

void gauntlet_sh_update (void)
{
	 YM2151_sh_update ();
    pokey_sh_update();
    tms5220_sh_update();
}



/*************************************
 *
 *		Sound control write.
 *
 *************************************/

void gauntlet_sound_ctl_w (int offset, int data)
{
	switch (offset & 7)
	{
		case 0:	/* music reset, bit D7, low reset */
			break;

		case 1:	/* speech write, bit D7, active low */
			if (((data ^ last_speech_write) & 0x80) && (data & 0x80))
				tms5220_data_w (0, speech_val);
			last_speech_write = data;
			break;

		case 2:	/* speech reset, bit D7, active low */
			break;

		case 3:	/* speech squeak, bit D7, low = 650kHz clock */
			break;
	}
}


/*************************************
 *
 *		Sound mixer write.
 *
 *************************************/

void gauntlet_6502_mix_w (int offset, int data)
{
	mix = data;
}


/*************************************
 *
 *		Sound YM2151 read/write.
 *
 *************************************/

int gauntlet_ym2151_r (int offset)
{
	if ((offset & 1) == 1)
		return YM2151_status_port_0_r (offset);
	return 0;
}


void gauntlet_ym2151_w (int offset, int data)
{
	if ((offset & 1) == 0)
		YM2151_register_port_0_w (offset, data);
	else
		YM2151_data_port_0_w (offset, data);
}


/*************************************
 *
 *		Sound TMS5220 write.
 *
 *************************************/

void gauntlet_tms_w (int offset, int data)
{
	speech_val = data;
}
