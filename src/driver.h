#ifndef DRIVER_H
#define DRIVER_H

#include "osdepend.h"
#include "common.h"
#include "drawgfx.h"
#include "palette.h"
#include "mame.h"
#include "cpuintrf.h"
#include "sndintrf.h"
#include "memory.h"
#include "input.h"
#include "inptport.h"
#include "usrintrf.h"
#include "cheat.h"
#include "tilemap.h"
#include "sprite.h"
#include "gfxobj.h"
#include "profiler.h"

#ifdef MAME_NET
#include "network.h"
#endif /* MAME_NET */


struct MachineCPU
{
	int cpu_type;	/* see #defines below. */
	int cpu_clock;	/* in Hertz */
/* number of the memory region (allocated by loadroms()) */
/* where this CPU resides */
	int memory_region;
	const struct MemoryReadAddress *memory_read;
	const struct MemoryWriteAddress *memory_write;
	const struct IOReadPort *port_read;
	const struct IOWritePort *port_write;
	int (*vblank_interrupt)(void);
    int vblank_interrupts_per_frame;    /* usually 1 */
/* use this for interrupts which are not tied to vblank 	*/
/* usually frequency in Hz, but if you need 				*/
/* greater precision you can give the period in nanoseconds */
	int (*timed_interrupt)(void);
	int timed_interrupts_per_second;
/* pointer to a parameter to pass to the CPU cores reset function */
	void *reset_param;
};

enum
{
	CPU_DUMMY,
#if (HAS_Z80)
	CPU_Z80,
#endif
#if (HAS_Z80_VM)
	CPU_Z80_VM,
#endif
#if (HAS_8080)
	CPU_8080,
#endif
#if (HAS_8085A)
	CPU_8085A,
#endif
#if (HAS_M6502)
	CPU_M6502,
#endif
#if (HAS_M65C02)
	CPU_M65C02,
#endif
#if (HAS_M6510)
	CPU_M6510,
#endif
#if (HAS_N2A03)
	CPU_N2A03,
#endif
#if (HAS_H6280)
	CPU_H6280,
#endif
#if (HAS_I86)
	CPU_I86,
#endif
#if (HAS_V20)
	CPU_V20,
#endif
#if (HAS_V30)
	CPU_V30,
#endif
#if (HAS_V33)
	CPU_V33,
#endif
#if (HAS_I8035)
	CPU_I8035,		/* same as CPU_I8039 */
#endif
#if (HAS_I8039)
	CPU_I8039,
#endif
#if (HAS_I8048)
	CPU_I8048,		/* same as CPU_I8039 */
#endif
#if (HAS_N7751)
	CPU_N7751,		/* same as CPU_I8039 */
#endif
#if (HAS_M6800)
	CPU_M6800,		/* same as CPU_M6802/CPU_M6808 */
#endif
#if (HAS_M6801)
	CPU_M6801,		/* same as CPU_M6803 */
#endif
#if (HAS_M6802)
	CPU_M6802,		/* same as CPU_M6800/CPU_M6808 */
#endif
#if (HAS_M6803)
	CPU_M6803,		/* same as CPU_M6801 */
#endif
#if (HAS_M6808)
	CPU_M6808,		/* same as CPU_M6800/CPU_M6802 */
#endif
#if (HAS_HD63701)
	CPU_HD63701,	/* 6808 with some additional opcodes */
#endif
#if (HAS_M6805)
	CPU_M6805,
#endif
#if (HAS_M68705)
	CPU_M68705, 	/* same as CPU_M6805 */
#endif
#if (HAS_HD63705)
	CPU_HD63705,	/* M6805 family but larger address space, different stack size */
#endif
#if (HAS_M6309)
	CPU_M6309,		/* same as CPU_M6809 (actually it's not 100% compatible) */
#endif
#if (HAS_M6809)
	CPU_M6809,
#endif
#if (HAS_M68000)
	CPU_M68000,
#endif
#if (HAS_M68010)
	CPU_M68010,
#endif
#if (HAS_M68020)
	CPU_M68020,
#endif
#if (HAS_T11)
	CPU_T11,
#endif
#if (HAS_S2650)
	CPU_S2650,
#endif
#if (HAS_TMS34010)
	CPU_TMS34010,
#endif
#if (HAS_TMS9900)
	CPU_TMS9900,
#endif
#if (HAS_Z8000)
	CPU_Z8000,
#endif
#if (HAS_TMS320C10)
	CPU_TMS320C10,
#endif
#if (HAS_CCPU)
	CPU_CCPU,
#endif
#if (HAS_PDP1)
	CPU_PDP1,
#endif
#if (HAS_KONAMI)
	CPU_KONAMI,
#endif
	CPU_COUNT
};

/* set this if the CPU is used as a slave for audio. It will not be emulated if */
/* sound is disabled, therefore speeding up a lot the emulation. */
#define CPU_AUDIO_CPU 0x8000

/* the Z80 can be wired to use 16 bit addressing for I/O ports */
#define CPU_16BIT_PORT 0x4000

#define CPU_FLAGS_MASK 0xff00


#define MAX_CPU 4	/* MAX_CPU is the maximum number of CPUs which cpuintrf.c */
					/* can run at the same time. Currently, 4 is enough. */


#define MAX_SOUND 4	/* MAX_SOUND is the maximum number of sound subsystems */
					/* which can run at the same time. Currently, 4 is enough. */



struct MachineDriver
{
	/* basic machine hardware */
	struct MachineCPU cpu[MAX_CPU];
	int frames_per_second;
	int vblank_duration;	/* in microseconds - see description below */
	int cpu_slices_per_frame;	/* for multicpu games. 1 is the minimum, meaning */
								/* that each CPU runs for the whole video frame */
								/* before giving control to the others. The higher */
								/* this setting, the more closely CPUs are interleaved */
								/* and therefore the more accurate the emulation is. */
								/* However, an higher setting also means slower */
								/* performance. */
	void (*init_machine)(void);
#ifdef MESS
	void (*stop_machine)(void); /* needed for MESS */
#endif

    /* video hardware */
	int screen_width,screen_height;
	struct rectangle visible_area;
	struct GfxDecodeInfo *gfxdecodeinfo;
	unsigned int total_colors;	/* palette is 3*total_colors bytes long */
	unsigned int color_table_len;	/* length in shorts of the color lookup table */
	void (*vh_convert_color_prom)(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);

	int video_attributes;	/* ASG 081897 */
	int unused;	/* obsolete */

	int (*vh_start)(void);
	void (*vh_stop)(void);
	void (*vh_update)(struct osd_bitmap *bitmap,int full_refresh);

	/* sound hardware */
	int sound_attributes;
	int (*sh_start)(void);
	void (*sh_stop)(void);
	void (*sh_update)(void);
	struct MachineSound sound[MAX_SOUND];
};



/* VBlank is the period when the video beam is outside of the visible area and */
/* returns from the bottom to the top of the screen to prepare for a new video frame. */
/* VBlank duration is an important factor in how the game renders itself. MAME */
/* generates the vblank_interrupt, lets the game run for vblank_duration microseconds, */
/* and then updates the screen. This faithfully reproduces the behaviour of the real */
/* hardware. In many cases, the game does video related operations both in its vblank */
/* interrupt, and in the normal game code; it is therefore important to set up */
/* vblank_duration accurately to have everything properly in sync. An example of this */
/* is Commando: if you set vblank_duration to 0, therefore redrawing the screen BEFORE */
/* the vblank interrupt is executed, sprites will be misaligned when the screen scrolls. */

/* Here are some predefined, TOTALLY ARBITRARY values for vblank_duration, which should */
/* be OK for most cases. I have NO IDEA how accurate they are compared to the real */
/* hardware, they could be completely wrong. */
#define DEFAULT_60HZ_VBLANK_DURATION 0
#define DEFAULT_30HZ_VBLANK_DURATION 0
/* If you use IPT_VBLANK, you need a duration different from 0. */
#define DEFAULT_REAL_60HZ_VBLANK_DURATION 2500
#define DEFAULT_REAL_30HZ_VBLANK_DURATION 2500



/* flags for video_attributes */

/* bit 0 of the video attributes indicates raster or vector video hardware */
#define	VIDEO_TYPE_RASTER			0x0000
#define	VIDEO_TYPE_VECTOR			0x0001

/* bit 1 of the video attributes indicates whether or not dirty rectangles will work */
#define	VIDEO_SUPPORTS_DIRTY		0x0002

/* bit 2 of the video attributes indicates whether or not the driver modifies the palette */
#define	VIDEO_MODIFIES_PALETTE	0x0004

/* ASG 980209 - added: */
/* bit 3 of the video attributes indicates whether or not the driver wants 16-bit color */
#define	VIDEO_SUPPORTS_16BIT		0x0008

/* ASG 980417 - added: */
/* bit 4 of the video attributes indicates that the driver wants its refresh before the VBLANK */
/*       instead of after. You usually don't want to use this, but it might be necessary if */
/*       you are caching data during the video frame and want to update the screen before */
/*       the game starts calculating the next frame. */
#define	VIDEO_UPDATE_BEFORE_VBLANK	0x0010

/* In most cases we assume pixels are square (1:1 aspect ratio) but some games need */
/* different proportions, e.g. 1:2 for Blasteroids */
#define VIDEO_PIXEL_ASPECT_RATIO_MASK 0x0020
#define VIDEO_PIXEL_ASPECT_RATIO_1_1 0x0000
#define VIDEO_PIXEL_ASPECT_RATIO_1_2 0x0020

#define VIDEO_DUAL_MONITOR 0x0040

/* flags for sound_attributes */
#define	SOUND_SUPPORTS_STEREO		0x0001



struct GameDriver
{
	const char *source_file;	/* set this to __FILE__ */
	const struct GameDriver *clone_of;	/* if this is a clone, point to */
										/* the main version of the game */
	const char *name;
	const char *description;
	const char *year;
	const char *manufacturer;
	const char *obsolete;
	int flags;	/* see defines below */
	const struct MachineDriver *drv;
	void (*driver_init)(void);	/* optional function to be called during initialization */
								/* This is called ONCE, unlike Machine->init_machine */
								/* which is called every time the game is reset. */

	const struct RomModule *rom;
   #ifdef MESS
   int (*rom_load)(void); /* used to load the ROM and set up memory regions */
	int (*rom_id)(const char *name, const char *gamename); /* returns 1 if the ROM will work with this driver */
	int num_of_rom_slots;
	int num_of_floppy_drives;
	int num_of_hard_drives;
	int num_of_cassette_drives;
 	#endif

	void (*rom_decode)(void);		/* used to decrypt the ROMs after loading them */
	void (*opcode_decode)(void);	/* used to decrypt the CPU opcodes in the ROMs, */
									/* if the encryption is different from the above. */
	const char **samplenames;		/* optional array of names of samples to load. */
									/* drivers can retrieve them in Machine->samples */
	const unsigned char *sound_prom;

	struct InputPort *input_ports;

	/* if they are available, provide a dump of the color proms, or even */
	/* better load them from disk like the other ROMs. */
	/* If you load them from disk, you must place them in a memory region by */
	/* itself, and use the PROM_MEMORY_REGION macro below to say in which */
	/* region they are. */
	const unsigned char *color_prom;
	const unsigned char *palette;
	const unsigned short *colortable;
	int orientation;	/* orientation of the monitor; see defines below */

	int (*hiscore_load)(void);	/* will be called every vblank until it */
						/* returns nonzero */
	void (*hiscore_save)(void);	/* will not be called if hiscore_load() hasn't yet */
						/* returned nonzero, to avoid saving an invalid table */
};


/* values for the flags field */
#define GAME_NOT_WORKING		0x0001
#define GAME_WRONG_COLORS		0x0002	/* colors are totally wrong */
#define GAME_IMPERFECT_COLORS	0x0004	/* colors are not 100% accurate, but close */
#define GAME_NO_SOUND			0x0008	/* sound is missing */
#define GAME_IMPERFECT_SOUND	0x0010	/* sound is known to be wrong */

#ifdef MESS
 #define GAME_COMPUTER			0x8000	/* Driver is a computer (needs full keyboard) */
#endif

#define PROM_MEMORY_REGION(region) ((const unsigned char *)((-(region))-1))


#define	ORIENTATION_DEFAULT		0x00
#define	ORIENTATION_FLIP_X		0x01	/* mirror everything in the X direction */
#define	ORIENTATION_FLIP_Y		0x02	/* mirror everything in the Y direction */
#define ORIENTATION_SWAP_XY		0x04	/* mirror along the top-left/bottom-right diagonal */
#define	ORIENTATION_ROTATE_90	(ORIENTATION_SWAP_XY|ORIENTATION_FLIP_X)	/* rotate clockwise 90 degrees */
#define	ORIENTATION_ROTATE_180	(ORIENTATION_FLIP_X|ORIENTATION_FLIP_Y)	/* rotate 180 degrees */
#define	ORIENTATION_ROTATE_270	(ORIENTATION_SWAP_XY|ORIENTATION_FLIP_Y)	/* rotate counter-clockwise 90 degrees */
/* IMPORTANT: to perform more than one transformation, DO NOT USE |, use ^ instead. */
/* For example, to rotate 90 degrees counterclockwise and flip horizontally, use: */
/* ORIENTATION_ROTATE_270 ^ ORIENTATION_FLIP_X*/
/* Always remember that FLIP is performed *after* SWAP_XY. */


extern const struct GameDriver *drivers[];

#endif
