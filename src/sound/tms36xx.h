#ifndef TMS36XX_SOUND_H
#define TMS36XX_SOUND_H

#define MAX_TMS36XX 4

/* subtypes */
#define TMS3615 15
#define TMS3617 17

/* The interface structure */
struct TMS36XXinterface {
	int num;
	int mixing_level[MAX_TMS36XX];
	int subtype[MAX_TMS36XX];
	int basefreq[MAX_TMS36XX];		/* base frequenies of the chips */
	double decay[MAX_TMS36XX][6];	/* decay times for the six harmonic notes */
	double speed[MAX_TMS36XX];		/* tune speed (meaningful for the TMS3615 only) */
};

extern int tms36xx_sh_start(const struct MachineSound *msound);
extern void tms36xx_sh_stop(void);
extern void tms36xx_sh_update(void);

/* TMS3615 interface functions */
extern void tms3615_tune_w(int chip, int tune);

/* TMS3615 interface functions */
extern void tms3617_note_w(int chip, int octave, int note);
extern void tms3617_enable_w(int chip, int enable);

#endif
