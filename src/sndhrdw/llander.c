/***************************************************************************

	Lunar Lander Specific Sound Code

***************************************************************************/

#include "driver.h"
#include "asteroid.h"

/************************************************************************/
/* Lunar Lander Sound System Analog emulation by K.Wilkins Nov 2000     */
/* Questions/Suggestions to mame@dysfunction.demon.co.uk                */
/************************************************************************/

#define LLANDER_TONE3K_NODE	NODE_50
#define LLANDER_TONE6K_NODE	NODE_51
#define LLANDER_THRUST_NODE	NODE_52
#define LLANDER_EXPLOD_NODE	NODE_53

/***************************************************************************
  Lander has 4 sound sources: 3kHz, 6kHz, thrust, explosion

  As the filtering removes a lot of the signal amplitute on thrust and
  explosion paths the gain is partitioned unequally:

  3kHz (tone)             Gain 1
  6kHz (tone)             Gain 1
  thrust (12kHz noise)    Gain 2
  explosion (12kHz noise) Gain 4

This is very simply implemented at 2 sinewave sources of fixed amplitude
and two white noise sources one of fixed amplitude and one of variable
amplitude. These are combined at the output stage with an adder

***************************************************************************/

DISCRETE_SOUND_START(llander_sound_interface)
	DISCRETE_INPUT(LLANDER_THRUST_NODE,0,0x0003,0)									// Input handlers, mostly for enable
	DISCRETE_INPUT(LLANDER_TONE3K_NODE,1,0x0003,0)
	DISCRETE_INPUT(LLANDER_TONE6K_NODE,2,0x0003,0)
	DISCRETE_INPUT(LLANDER_EXPLOD_NODE,3,0x0003,0)

	DISCRETE_SINEWAVE(NODE_10,LLANDER_TONE3K_NODE,3000,(((1.0/8.0)*32767.0)),0)		// 3KHz Sine wave amplitude 1/12 * 16384

	DISCRETE_SINEWAVE(NODE_20,LLANDER_TONE6K_NODE,6000,(((1.0/8.0)*32767.0)),0)		// 6KHz Sine wave amplitude 1/12 * 16384

	DISCRETE_GAIN(NODE_32,LLANDER_THRUST_NODE,((((2.0+4.0)/8.0)*32767.0)/8.0))		// Convert gain to amplitude for noise (some additional added to compansate for RC filter
	DISCRETE_NOISE(NODE_31,1,12000,NODE_32)			 							  	// 12KHz Noise source for thrust
	DISCRETE_RCFILTER(NODE_30,1,NODE_31,47000,0.1e-6) 						  		// Remove high freq noise

	DISCRETE_GAIN(NODE_41,LLANDER_THRUST_NODE,(((4.0/8.0)*32767.0)/8.0))			// Docs say that explosion also takes volume of thrust
	DISCRETE_NOISE(NODE_40,LLANDER_EXPLOD_NODE,12000,NODE_41)						// Noise source for explosion

	DISCRETE_ADDER4(NODE_90,1,NODE_10,NODE_20,NODE_30,NODE_40)						// Mix all four sound sources

	DISCRETE_OUTPUT(NODE_90)														// Take the output from the mixer
DISCRETE_SOUND_END

WRITE_HANDLER( llander_snd_reset_w )
{
	/* This does nothing in the discrete emulation but on the real machine */
	/* It resets the LFSR that is used for the white noise generator       */
}

WRITE_HANDLER( llander_sounds_w )
{
	discrete_sound_w(0,data&0x07);		/* Thrust volume */
	discrete_sound_w(1,data&0x10);		/* Tone 3KHz enable */
	discrete_sound_w(2,data&0x20);		/* Tone 6KHz enable */
	discrete_sound_w(3,data&0x08);		/* Explosion */
}

