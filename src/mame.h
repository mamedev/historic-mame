#ifndef MACHINE_H
#define MACHINE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef MESS
#include "mess/mess.h"
#endif

extern char build_version[];
extern FILE *errorlog;

#define MAX_GFX_ELEMENTS 32
#define MAX_MEMORY_REGIONS 10

struct RunningMachine
{
	unsigned char *memory_region[MAX_MEMORY_REGIONS];
	unsigned int memory_region_length[MAX_MEMORY_REGIONS];	/* some drivers might find this useful */
	struct GfxElement *gfx[MAX_GFX_ELEMENTS];	/* graphic sets (chars, sprites) */
	struct osd_bitmap *scrbitmap;	/* bitmap to draw into */
	unsigned short *pens;	/* remapped palette pen numbers. When you write */
							/* directly to a bitmap, never use absolute values, */
							/* use this array to get the pen number. For example, */
							/* if you want to use color #6 in the palette, use */
							/* pens[6] instead of just 6. */
	unsigned short *colortable;	/* lookup table used to map gfx pen numbers to palette pen numbers */
	const struct GameDriver *gamedrv;	/* contains the definition of the game machine */
	const struct MachineDriver *drv;	/* same as gamedrv->drv */
	int sample_rate;	/* the digital audio sample rate; 0 if sound is disabled. */
						/* This is set to a default value, or a value specified by */
						/* the user; osd_init() is allowed to change it to the actual */
						/* sample rate supported by the audio card. */
	int sample_bits;	/* 8 or 16 */
	struct GameSamples *samples;	/* samples loaded from disk */
	struct InputPort *input_ports;	/* the input ports definition from the driver */
								/* is copied here and modified (load settings from disk, */
								/* remove cheat commands, and so on) */
	int orientation;	/* see #defines in driver.h */
	struct GfxElement *uifont;	/* font used by DisplayText() */
	int uifontwidth,uifontheight;
	int uixmin,uiymin;
	int uiwidth,uiheight;
	int ui_orientation;
};

/* The host platform should fill these fields with the preferences specified in the GUI */
/* or on the commandline. */
struct GameOptions {
	FILE *errorlog;
	void *record;
	void *playback;
	int mame_debug;
	int cheat;
	int gui_host;

	int samplerate;
	int samplebits;
	int no_fm;
	int use_samples;
	int use_emulated_ym3812;

	int norotate;
	int ror;
	int rol;
	int flipx;
	int flipy;
	int beam;
	int flicker;
	int translucency;
	int antialias;
	int use_artwork;

	#ifdef MESS
		/* This is ugly for now, but its temporary! */
	  char rom_name[MAX_ROM][2048];           /* MESS */
 	  char floppy_name[MAX_FLOPPY][2048];     /* MESS */
	  char hard_name[MAX_HARD][2048];         /* MESS */
	  char cassette_name[MAX_CASSETTE][2048]; /* MESS */
	#endif
};

extern struct GameOptions options;
extern struct RunningMachine *Machine;

int run_game (int game);
int updatescreen(void);
/* osd_fopen() must use this to know if high score files can be used */
int mame_highscore_enabled(void);


#endif
