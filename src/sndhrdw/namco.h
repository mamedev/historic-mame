#ifndef namco_h
#define namco_h

struct namco_interface
{
	int samplerate;	/* sample rate */
	int voices;		/* number of voices */
	int gain;		/* 16 * gain adjustment */
	int volume;		/* playback volume */
	int region;		/* memory region */
	int stereo;		/* set to 1 to indicate stereo (e.g., System 1) */
};

int namco_sh_start(struct namco_interface *intf);
void namco_sh_stop(void);
void namco_sh_update(void);

void pengo_sound_enable_w(int offset,int data);
void pengo_sound_w(int offset,int data);

void mappy_sound_enable_w(int offset,int data);
void mappy_sound_w(int offset,int data);

void namcos1_sound_enable_w(int offset,int data);
void namcos1_sound_w(int offset,int data);

extern unsigned char *namco_soundregs;

#define mappy_soundregs namco_soundregs
#define pengo_soundregs namco_soundregs

#endif

