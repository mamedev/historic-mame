/*************************************************************************

	vicdual.h

*************************************************************************/

#define FROGS_USE_SAMPLES 1

/*----------- defined in sndhrdw/vicdual.c -----------*/

WRITE8_HANDLER( frogs_sh_port2_w );

#ifndef FROGS_USE_SAMPLES

extern struct discrete_sound_block frogs_discrete_interface[];

#else

extern struct Samplesinterface frogs_samples_interface;

#endif
