/*************************************************************************

	Atari Pool Shark hardware

*************************************************************************/

#include "sound/discrete.h"

/* Discrete Sound Input Nodes */

/*----------- defined in sndhrdw/poolshrk.c -----------*/

WRITE8_HANDLER( poolshrk_scratch_sound_w );
WRITE8_HANDLER( poolshrk_score_sound_w );
WRITE8_HANDLER( poolshrk_click_sound_w );
WRITE8_HANDLER( poolshrk_bump_sound_w );

extern struct discrete_sound_block poolshrk_discrete_interface[];
