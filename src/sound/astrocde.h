
#ifndef ASTROCADE_H
#define ASTROCADE_H

#define MAX_ASTROCADE_CHIPS 2   /* max number of emulated chips */

struct astrocade_interface
{
	int num;			/* total number of sound chips in the machine */
	int baseclock;			/* astrocade clock rate  */
	int mixing_level[MAX_ASTROCADE_CHIPS];	/* master volume, panning */
};

int astrocade_sh_start(const struct MachineSound *msound);
void astrocade_sh_stop(void);
void astrocade_sh_update(void);

WRITE8_HANDLER( astrocade_sound1_w );
WRITE8_HANDLER( astrocade_soundblock1_w );
WRITE8_HANDLER( astrocade_sound2_w );
WRITE8_HANDLER( astrocade_soundblock2_w );

#endif
