
#include "driver.h"
#include "includes/blockade.h"

/*
 * This still needs the noise generator stuff,
 * along with proper mixing and volume control
 */

#define BLOCKADE_NOTE_DATA 		NODE_01
#define BLOCKADE_NOTE      		NODE_02
#define BLOCKADE_NOTE_AND_GAIN	NODE_03

DISCRETE_SOUND_START(blockade_discrete_interface)
	DISCRETE_INPUT_DATA  (BLOCKADE_NOTE_DATA)

	/************************************************/
	/* Note sound is created by a divider circuit.  */
	/* The master clock is the 93681.5 Hz, from the */
	/* 555 oscillator.  This is then sent to a      */
	/* preloadable 8 bit counter, which loads the   */
	/* value from OUT02 when overflowing from 0xFF  */
	/* to 0x00.  Therefore it divides by 2 (OUT02   */
	/* = FE) to 256 (OUT02 = 00).                   */
	/* There is also a final /2 stage.              */
	/* Note that there is no music disable line.    */
	/* When there is no music, the game sets the    */
	/* oscillator to 0Hz.  (OUT02 = FF)             */
	/************************************************/
	DISCRETE_NOTE(BLOCKADE_NOTE, 1, 93681.5, NODE_01, 255, 1)
	DISCRETE_GAIN(BLOCKADE_NOTE_AND_GAIN, BLOCKADE_NOTE, 7500)

	DISCRETE_OUTPUT(BLOCKADE_NOTE_AND_GAIN, 100)
DISCRETE_SOUND_END

WRITE8_HANDLER( blockade_sound_freq_w )
{
	discrete_sound_w(BLOCKADE_NOTE_DATA, data);
    return;
}

WRITE8_HANDLER( blockade_env_on_w )
{
//#ifdef BLOCKADE_LOG
//    printf("Boom Start\n");
//#endif
    sample_start(0,0,0);
    return;
}

WRITE8_HANDLER( blockade_env_off_w )
{
//#ifdef BLOCKADE_LOG
//    printf("Boom End\n");
//#endif
    return;
}

static const char *blockade_sample_names[] =
{
    "*blockade",
    "boom.wav",
    0   /* end of array */
};

struct Samplesinterface blockade_samples_interface =
{
    1,	/* 1 channel */
	25,	/* volume */
	blockade_sample_names
};
