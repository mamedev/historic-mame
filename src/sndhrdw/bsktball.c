/*************************************************************************

	sndhrdw\bsktball.c

*************************************************************************/
#include "driver.h"
#include "bsktball.h"


/***************************************************************************
Sound handlers
***************************************************************************/
WRITE8_HANDLER( bsktball_bounce_w )
{
	discrete_sound_w(BSKTBALL_CROWD_DATA, data & 0x0f);	// Crowd
	discrete_sound_w(BSKTBALL_BOUNCE_EN, data & 0x10);	// Bounce
}

WRITE8_HANDLER( bsktball_note_w )
{
	discrete_sound_w(BSKTBALL_NOTE_DATA, data);	// Note
}

WRITE8_HANDLER( bsktball_noise_reset_w )
{
	discrete_sound_w(BSKTBALL_NOISE_EN, offset & 0x01);
}


/************************************************************************/
/* bsktball Sound System Analog emulation                               */
/************************************************************************/

const struct discrete_lfsr_desc bsktball_lfsr={
	16,			/* Bit Length */
	0,			/* Reset Value */
	0,			/* Use Bit 0 as XOR input 0 */
	14,			/* Use Bit 14 as XOR input 1 */
	DISC_LFSR_XNOR,		/* Feedback stage1 is XNOR */
	DISC_LFSR_OR,		/* Feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,	/* Feedback stage3 replaces the shifted register contents */
	0x000001,		/* Everything is shifted into the first bit only */
	0,			/* Output is already inverted by XNOR */
	15			/* Output bit */
};

/* Nodes - Sounds */
#define BSKTBALL_NOISE			NODE_10
#define BSKTBALL_BOUNCE_SND		NODE_11
#define BSKTBALL_NOTE_SND		NODE_12
#define BSKTBALL_CROWD_SND		NODE_13

DISCRETE_SOUND_START(bsktball_discrete_interface)
	/************************************************/
	/* bsktball  Effects Relataive Gain Table       */
	/*                                              */
	/* Effect       V-ampIn   Gain ratio  Relative  */
	/* Note          3.8      47/47        1000.0   */
	/* Bounce        3.8      47/47        1000.0   */
	/* Crowd         3.8      47/220        213.6   */
	/************************************************/

	/************************************************/
	/* Input register mapping for bsktball          */
	/************************************************/
	/*              NODE                       GAIN     OFFSET  INIT */
	DISCRETE_INPUTX_DATA  (BSKTBALL_NOTE_DATA, -1, 0xff, 0)
	DISCRETE_INPUTX_DATA (BSKTBALL_CROWD_DATA, 213.6/15, 0,     0.0)
	DISCRETE_INPUTX_LOGIC(BSKTBALL_BOUNCE_EN,  1000.0/2, 0,     0.0)
	DISCRETE_INPUT_NOT   (BSKTBALL_NOISE_EN)

	/************************************************/
	/* Bounce is a trigger fed directly to the amp  */
	/************************************************/
	DISCRETE_CRFILTER(BSKTBALL_BOUNCE_SND, 1, BSKTBALL_BOUNCE_EN, 100000, 1.e-8)	// remove DC (C54)

	/************************************************/
	/* Crowd effect is variable amplitude, filtered */
/* random noise.                                */
	/* LFSR clk = 256H = 15750.0Hz                  */
	/************************************************/
	DISCRETE_LFSR_NOISE(BSKTBALL_NOISE, BSKTBALL_NOISE_EN, BSKTBALL_NOISE_EN, 15750.0, BSKTBALL_CROWD_DATA, 0, 0, &bsktball_lfsr)
	DISCRETE_FILTER2(BSKTBALL_CROWD_SND, 1, BSKTBALL_NOISE, 330.0, (1.0 / 7.6), DISC_FILTER_BANDPASS)

	/************************************************/
	/* Note sound is created by a divider circuit.  */
	/* The master clock is the 32H signal, which is */
	/* 12.096MHz/128.  This is then sent to a       */
	/* preloadable 8 bit counter, which loads the   */
	/* value from OUT30 when overflowing from 0xFF  */
	/* to 0x00.  Therefore it divides by 2 (OUT30   */
	/* = FE) to 256 (OUT30 = 00).                   */
	/* There is also a final /2 stage.              */
	/* Note that there is no music disable line.    */
	/* When there is no music, the game sets the    */
	/* oscillator to 0Hz.  (OUT30 = FF)             */
	/************************************************/
	DISCRETE_ADDER2(NODE_20, 1, BSKTBALL_NOTE_DATA, 1)	/* To get values of 1 - 256 */
	DISCRETE_DIVIDE(NODE_21, 1, 12096000.0/128/2, NODE_20)
	DISCRETE_SQUAREWAVE(BSKTBALL_NOTE_SND, BSKTBALL_NOTE_DATA, NODE_21, 1000, 50.0, 0, 0.0)	/* NOTE=FF Disables audio */

	DISCRETE_ADDER3(NODE_90, 1, BSKTBALL_BOUNCE_SND, BSKTBALL_NOTE_SND, BSKTBALL_CROWD_SND)
	DISCRETE_GAIN(NODE_91, NODE_90, 65534.0/(1000.0+2000.0+213.6))
	DISCRETE_OUTPUT(NODE_91, 100)
DISCRETE_SOUND_END
