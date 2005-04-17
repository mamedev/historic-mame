#include "sound/discrete.h"


/*----------- defined in sndhrdw/phoenix.c -----------*/

extern struct discrete_sound_block phoenix_discrete_interface[];

WRITE8_HANDLER( phoenix_sound_control_a_w );
WRITE8_HANDLER( phoenix_sound_control_b_w );
