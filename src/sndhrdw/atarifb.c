/*************************************************************************

	sndhrdw\atarifb.c

*************************************************************************/

#include "driver.h"
#include "atarifb.h"


/************************************************************************/
/* atarifb Sound System Analog emulation                                */
/************************************************************************/

const struct discrete_555_desc atarifbWhistl555 =
{
	DISC_555_OUT_CAP | DISC_555_OUT_AC,
	5,		// B+ voltage of 555
	DEFAULT_555_VALUES
};

const struct discrete_lfsr_desc atarifb_lfsr =
{
	16,			/* Bit Length */
	0,			/* Reset Value */
	0,			/* Use Bit 0 as XOR input 0 */
	14,			/* Use Bit 14 as XOR input 1 */
	DISC_LFSR_XNOR,		/* Feedback stage1 is XNOR */
	DISC_LFSR_OR,		/* Feedback stage2 is just stage 1 output OR with external feed */
	DISC_LFSR_REPLACE,	/* Feedback stage3 replaces the shifted register contents */
	0x000001,		/* Everything is shifted into the first bit only */
	0,			/* Output is already inverted by XNOR */
	16			/* Output bit is feedback bit */
};

/* Nodes - Sounds */
#define ATARIFB_NOISE			NODE_10
#define ATARIFB_HIT_SND			NODE_11
#define ATARIFB_WHISTLE_SND		NODE_12
#define ATARIFB_CROWD_SND		NODE_13

DISCRETE_SOUND_START(atarifb_discrete_interface)
	/************************************************/
	/* atarifb  Effects Relataive Gain Table        */
	/*                                              */
	/* Effect       V-ampIn   Gain ratio  Relative  */
	/* Hit           3.8      47/47         760.0   */
	/* Whistle       5.0      47/47        1000.0   */
	/* Crowd         3.8      47/220        162.4   */
	/************************************************/

	/************************************************/
	/* Input register mapping for atarifb           */
	/************************************************/
	/*                    NODE                GAIN      OFFSET INIT */
	DISCRETE_INPUT_LOGIC (ATARIFB_WHISTLE_EN)
	DISCRETE_INPUTX_DATA (ATARIFB_CROWD_DATA, 162.4/15, 0,      0.0)
	DISCRETE_INPUTX_LOGIC(ATARIFB_HIT_EN,        760.0, 0,      0.0)
	DISCRETE_INPUT_NOT   (ATARIFB_ATTRACT_EN)
	DISCRETE_INPUT_LOGIC (ATARIFB_NOISE_EN)

	/************************************************/
	/* Hit is a trigger fed directly to the amp     */
	/************************************************/
	DISCRETE_FILTER2(ATARIFB_HIT_SND, 1, ATARIFB_HIT_EN, 10.0, 5, DISC_FILTER_HIGHPASS)	// remove DC

	/************************************************/
	/* Crowd effect is variable amplitude, filtered */
	/* random noise.                                */
	/* LFSR clk = 256H = 15750.0Hz                  */
	/************************************************/
	DISCRETE_LFSR_NOISE(ATARIFB_NOISE, ATARIFB_NOISE_EN, ATARIFB_NOISE_EN, 15750.0, ATARIFB_CROWD_DATA, 0, 0, &atarifb_lfsr)
	DISCRETE_FILTER2(ATARIFB_CROWD_SND, 1, ATARIFB_NOISE, 330.0, (1.0 / 7.6), DISC_FILTER_BANDPASS)

	/************************************************/
	/* Whistle effect is a triggered 555 cap charge */
	/************************************************/
	DISCRETE_555_ASTABLE(NODE_20, ATARIFB_WHISTLE_EN, 2200, 2200, 1e-7, &atarifbWhistl555)
	DISCRETE_MULTIPLY(ATARIFB_WHISTLE_SND, ATARIFB_WHISTLE_EN, NODE_20, 1000.0/3.3)

	DISCRETE_ADDER3(NODE_90, ATARIFB_ATTRACT_EN, ATARIFB_HIT_SND, ATARIFB_WHISTLE_SND, ATARIFB_CROWD_SND)
	DISCRETE_GAIN(NODE_91, NODE_90, 65534.0/(506.7+1000.0+760.0))
	DISCRETE_OUTPUT(NODE_91, 100)
DISCRETE_SOUND_END


/************************************************************************/
/* abaseb Sound System Analog emulation                                 */
/************************************************************************/
/* Sounds indentical to atarifb, but gain values are different          */

/* Nodes - Sounds */
#define ABASEB_NOISE			NODE_10
#define ABASEB_HIT_SND			NODE_11
#define ABASEB_WHISTLE_SND		NODE_12
#define ABASEB_CROWD_SND		NODE_13

DISCRETE_SOUND_START(abaseb_discrete_interface)
	/************************************************/
	/* abaseb  Effects Relataive Gain Table         */
	/*                                              */
	/* Effect       V-ampIn   Gain ratio  Relative  */
	/* Hit           3.8      47/330        506.7   */
	/* Whistle       5.0      47/220       1000.0   */
	/* Crowd         3.8      47/220        760.0   */
	/************************************************/

	/************************************************/
	/* Input register mapping for abaseb            */
	/************************************************/
	/*                    NODE                GAIN      OFFSET  INIT */
	DISCRETE_INPUT_LOGIC (ATARIFB_WHISTLE_EN)
	DISCRETE_INPUTX_DATA (ATARIFB_CROWD_DATA, 760.0/15, 0,      0.0)
	DISCRETE_INPUTX_LOGIC(ATARIFB_HIT_EN,     506.7/2,  0,      0.0)
	DISCRETE_INPUT_NOT   (ATARIFB_ATTRACT_EN)
	DISCRETE_INPUT_LOGIC (ATARIFB_NOISE_EN)

	/************************************************/
	/* Hit is a trigger fed directly to the amp     */
	/************************************************/
DISCRETE_FILTER2(ABASEB_HIT_SND, 1, ATARIFB_HIT_EN, 10.0, 5, DISC_FILTER_HIGHPASS)	// remove DC

	/************************************************/
	/* Crowd effect is variable amplitude, filtered */
	/* random noise.                                */
	/* LFSR clk = 256H = 15750.0Hz                  */
	/************************************************/
	DISCRETE_LFSR_NOISE(ABASEB_NOISE, ATARIFB_NOISE_EN, ATARIFB_NOISE_EN, 15750.0, ATARIFB_CROWD_DATA, 0, 0, &atarifb_lfsr)
	DISCRETE_FILTER2(ABASEB_CROWD_SND, 1, ABASEB_NOISE, 330.0, (1.0 / 7.6), DISC_FILTER_BANDPASS)

	/************************************************/
	/* Whistle effect is a triggered 555 cap charge */
	/************************************************/
	DISCRETE_555_ASTABLE(NODE_20, ATARIFB_WHISTLE_EN, 2200, 2200, 1e-7, &atarifbWhistl555)
	DISCRETE_MULTIPLY(ATARIFB_WHISTLE_SND, ATARIFB_WHISTLE_EN, NODE_20, 1000.0/5)

	DISCRETE_ADDER3(NODE_90, ATARIFB_ATTRACT_EN, ABASEB_HIT_SND, ABASEB_WHISTLE_SND, ABASEB_CROWD_SND)
	DISCRETE_GAIN(NODE_91, NODE_90, 65534.0/(506.7+1000.0+760.0))
	DISCRETE_OUTPUT(NODE_91, 100)
DISCRETE_SOUND_END
