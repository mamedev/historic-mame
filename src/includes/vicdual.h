/*************************************************************************

	vicdual.h

*************************************************************************/

#include "sound/discrete.h"

/*----------- defined in drivers/vicdual.c -----------*/

extern mame_timer *croak_timer;


/*----------- defined in sndhrdw/vicdual.c -----------*/

WRITE8_HANDLER( frogs_sh_port2_w );
void croak_callback(int param);

extern struct Samplesinterface frogs_samples_interface;
extern struct discrete_sound_block frogs_discrete_interface[];
