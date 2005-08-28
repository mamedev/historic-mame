/***************************************************************************

    driver.h

    Include this with all MAME files. Includes all the core system pieces.

***************************************************************************/

#pragma once

#ifndef __DRIVER_H__
#define __DRIVER_H__


/***************************************************************************

    Macros for declaring common callbacks

***************************************************************************/

#define DRIVER_INIT(name)		void init_##name(void)

#define INTERRUPT_GEN(func)		void func(void)

#define MACHINE_INIT(name)		void machine_init_##name(void)
#define MACHINE_STOP(name)		void machine_stop_##name(void)

#define NVRAM_HANDLER(name)		void nvram_handler_##name(mame_file *file, int read_or_write)

#define PALETTE_INIT(name)		void palette_init_##name(UINT16 *colortable, const UINT8 *color_prom)

#define VIDEO_START(name)		int video_start_##name(void)
#define VIDEO_STOP(name)		void video_stop_##name(void)
#define VIDEO_EOF(name)			void video_eof_##name(void)
#define VIDEO_UPDATE(name)		void video_update_##name(int screen, struct mame_bitmap *bitmap, const struct rectangle *cliprect)

/* NULL versions */
#define init_NULL				NULL
#define machine_init_NULL 		NULL
#define nvram_handler_NULL 		NULL
#define palette_init_NULL		NULL
#define video_start_NULL 		NULL
#define video_stop_NULL 		NULL
#define video_eof_NULL 			NULL
#define video_update_NULL 		NULL



/***************************************************************************

    Core MAME includes

***************************************************************************/

#include "mamecore.h"
#include "memory.h"
#include "mamedbg.h"
#include "mame.h"
#include "common.h"
#include "drawgfx.h"
#include "palette.h"
#include "cpuintrf.h"
#include "cpuexec.h"
#include "cpuint.h"
#include "sndintrf.h"
#include "input.h"
#include "inptport.h"
#include "usrintrf.h"
#include "cheat.h"
#include "tilemap.h"
#include "profiler.h"

#ifdef MESS
#include "messdrv.h"
#endif


/***************************************************************************

    Macros for building machine drivers

***************************************************************************/

/* use this to declare external references to a machine driver */
#define MACHINE_DRIVER_EXTERN(game)										\
	void construct_##game(machine_config *machine)						\


/* start/end tags for the machine driver */
#define MACHINE_DRIVER_START(game) 										\
	void construct_##game(machine_config *machine)						\
	{																	\
		cpu_config *cpu = NULL;											\
		sound_config *sound = NULL;										\
		(void)cpu;														\
		(void)sound;													\

#define MACHINE_DRIVER_END 												\
	}																	\


/* importing data from other machine drivers */
#define MDRV_IMPORT_FROM(game) 											\
	construct_##game(machine); 											\


/* add/modify/remove/replace CPUs */
#define MDRV_CPU_ADD_TAG(tag, type, clock)								\
	cpu = machine_add_cpu(machine, (tag), CPU_##type, (clock));			\

#define MDRV_CPU_ADD(type, clock)										\
	MDRV_CPU_ADD_TAG(NULL, type, clock)									\

#define MDRV_CPU_MODIFY(tag)											\
	cpu = machine_find_cpu(machine, tag);								\

#define MDRV_CPU_REMOVE(tag)											\
	machine_remove_cpu(machine, tag);									\
	cpu = NULL;															\

#define MDRV_CPU_REPLACE(tag, type, clock)								\
	cpu = machine_find_cpu(machine, tag);								\
	if (cpu)															\
	{																	\
		cpu->cpu_type = (CPU_##type);									\
		cpu->cpu_clock = (clock);										\
	}																	\


/* CPU parameters */
#define MDRV_CPU_FLAGS(flags)											\
	if (cpu)															\
		cpu->cpu_flags = (flags);										\

#define MDRV_CPU_CONFIG(config)											\
	if (cpu)															\
		cpu->reset_param = &(config);									\

#define MDRV_CPU_PROGRAM_MAP(readmem, writemem)							\
	if (cpu)															\
	{																	\
		cpu->construct_map[ADDRESS_SPACE_PROGRAM][0] = (construct_map_##readmem); \
		cpu->construct_map[ADDRESS_SPACE_PROGRAM][1] = (construct_map_##writemem); \
	}																	\

#define MDRV_CPU_DATA_MAP(readmem, writemem)							\
	if (cpu)															\
	{																	\
		cpu->construct_map[ADDRESS_SPACE_DATA][0] = (construct_map_##readmem); \
		cpu->construct_map[ADDRESS_SPACE_DATA][1] = (construct_map_##writemem); \
	}																	\

#define MDRV_CPU_IO_MAP(readmem, writemem)								\
	if (cpu)															\
	{																	\
		cpu->construct_map[ADDRESS_SPACE_IO][0] = (construct_map_##readmem); \
		cpu->construct_map[ADDRESS_SPACE_IO][1] = (construct_map_##writemem); \
	}																	\

#define MDRV_CPU_VBLANK_INT(func, rate)									\
	if (cpu)															\
	{																	\
		cpu->vblank_interrupt = func;									\
		cpu->vblank_interrupts_per_frame = (rate);						\
	}																	\

#define MDRV_CPU_PERIODIC_INT(func, rate)								\
	if (cpu)															\
	{																	\
		cpu->timed_interrupt = func;									\
		cpu->timed_interrupt_period = (rate);							\
	}																	\


/* core parameters */
#define MDRV_FRAMES_PER_SECOND(rate)									\
	machine->frames_per_second = (rate);								\

#define MDRV_VBLANK_DURATION(duration)									\
	machine->vblank_duration = (duration);								\

#define MDRV_INTERLEAVE(interleave)										\
	machine->cpu_slices_per_frame = (interleave);						\

#define MDRV_WATCHDOG_VBLANK_INIT(watch_count)							\
	machine->watchdog_vblank_count = (watch_count);						\

#define MDRV_WATCHDOG_TIME_INIT(time)									\
	machine->watchdog_time = (time);									\

/* core functions */
#define MDRV_MACHINE_INIT(name)											\
	machine->machine_init = machine_init_##name;						\

#define MDRV_MACHINE_STOP(name)											\
	machine->machine_stop = machine_stop_##name;						\

#define MDRV_NVRAM_HANDLER(name)										\
	machine->nvram_handler = nvram_handler_##name;						\


/* core video parameters */
#define MDRV_VIDEO_ATTRIBUTES(flags)									\
	machine->video_attributes = (flags);								\

#define MDRV_ASPECT_RATIO(num, den)										\
	machine->aspect_x = (num);											\
	machine->aspect_y = (den);											\

#define MDRV_SCREEN_SIZE(width, height)									\
	machine->screen_width = (width);									\
	machine->screen_height = (height);									\

#define MDRV_VISIBLE_AREA(minx, maxx, miny, maxy)						\
	machine->default_visible_area.min_x = (minx);						\
	machine->default_visible_area.max_x = (maxx);						\
	machine->default_visible_area.min_y = (miny);						\
	machine->default_visible_area.max_y = (maxy);						\

#define MDRV_GFXDECODE(gfx)												\
	machine->gfxdecodeinfo = (gfx);										\

#define MDRV_PALETTE_LENGTH(length)										\
	machine->total_colors = (length);									\

#define MDRV_COLORTABLE_LENGTH(length)									\
	machine->color_table_len = (length);								\


/* core video functions */
#define MDRV_PALETTE_INIT(name)											\
	machine->init_palette = palette_init_##name;						\

#define MDRV_VIDEO_START(name)											\
	machine->video_start = video_start_##name;							\

#define MDRV_VIDEO_STOP(name)											\
	machine->video_stop = video_stop_##name;							\

#define MDRV_VIDEO_EOF(name)											\
	machine->video_eof = video_eof_##name;								\

#define MDRV_VIDEO_UPDATE(name)											\
	machine->video_update = video_update_##name;						\


/* add/remove speakers */
#define MDRV_SPEAKER_ADD(tag, x, y, z)									\
	machine_add_speaker(machine, (tag), (x), (y), (z));					\

#define MDRV_SPEAKER_REMOVE(tag)										\
	machine_remove_speaker(machine, (tag));								\

#define MDRV_SPEAKER_STANDARD_MONO(tag)									\
	MDRV_SPEAKER_ADD(tag, 0.0, 0.0, 1.0)								\

#define MDRV_SPEAKER_STANDARD_STEREO(tagl, tagr)						\
	MDRV_SPEAKER_ADD(tagl, -0.2, 0.0, 1.0)								\
	MDRV_SPEAKER_ADD(tagr, 0.2, 0.0, 1.0)								\


/* add/remove/replace sounds */
#define MDRV_SOUND_ADD_TAG(tag, type, clock)							\
	sound = machine_add_sound(machine, (tag), SOUND_##type, (clock));	\

#define MDRV_SOUND_ADD(type, clock)										\
	MDRV_SOUND_ADD_TAG(NULL, type, clock)								\

#define MDRV_SOUND_REMOVE(tag)											\
	machine_remove_sound(machine, tag);									\

#define MDRV_SOUND_MODIFY(tag)											\
	sound = machine_find_sound(machine, tag);							\
	if (sound)															\
		sound->routes = 0;												\

#define MDRV_SOUND_CONFIG(_config)										\
	if (sound)															\
		sound->config = &(_config);										\

#define MDRV_SOUND_REPLACE(tag, type, _clock)							\
	sound = machine_find_sound(machine, tag);							\
	if (sound)															\
	{																	\
		sound->sound_type = SOUND_##type;								\
		sound->clock = (_clock);										\
		sound->config = NULL;											\
		sound->routes = 0;												\
	}																	\

#define MDRV_SOUND_ROUTE(_output, _target, _gain)						\
	if (sound)															\
	{																	\
		sound->route[sound->routes].output = (_output);					\
		sound->route[sound->routes].target = (_target);					\
		sound->route[sound->routes].gain = (_gain);						\
		sound->routes++;												\
	}																	\


cpu_config *machine_add_cpu(machine_config *machine, const char *tag, int type, int cpuclock);
cpu_config *machine_find_cpu(machine_config *machine, const char *tag);
void machine_remove_cpu(machine_config *machine, const char *tag);

speaker_config *machine_add_speaker(machine_config *machine, const char *tag, float x, float y, float z);
speaker_config *machine_find_speaker(machine_config *machine, const char *tag);
void machine_remove_speaker(machine_config *machine, const char *tag);

sound_config *machine_add_sound(machine_config *machine, const char *tag, int type, int clock);
sound_config *machine_find_sound(machine_config *machine, const char *tag);
void machine_remove_sound(machine_config *machine, const char *tag);



/***************************************************************************

    Internal representation of a machine driver, built from the constructor

***************************************************************************/

#define MAX_CPU 8	/* MAX_CPU is the maximum number of CPUs which cpuintrf.c */
					/* can run at the same time. Currently, 8 is enough. */

#define MAX_SOUND 32/* MAX_SOUND is the maximum number of sound subsystems */
					/* which can run at the same time. Currently, 32 is enough. */

#define MAX_SPEAKER 4 /* MAX_SPEAKER is the maximum number of speakers */

struct _machine_config
{
	cpu_config cpu[MAX_CPU];
	float frames_per_second;
	int vblank_duration;
	UINT32 cpu_slices_per_frame;
	INT32 watchdog_vblank_count;
	double watchdog_time;

	void (*machine_init)(void);
	void (*machine_stop)(void);
	void (*nvram_handler)(mame_file *file, int read_or_write);

	UINT32 video_attributes;
	UINT32 aspect_x, aspect_y;
	int screen_width,screen_height;
	struct rectangle default_visible_area;
	gfx_decode *gfxdecodeinfo;
	UINT32 total_colors;
	UINT32 color_table_len;

	void (*init_palette)(UINT16 *colortable,const UINT8 *color_prom);
	int (*video_start)(void);
	void (*video_stop)(void);
	void (*video_eof)(void);
	void (*video_update)(int screen, struct mame_bitmap *bitmap,const struct rectangle *cliprect);

	sound_config sound[MAX_SOUND];
	speaker_config speaker[MAX_SPEAKER];
};
/* In mamecore.h: typedef struct _machine_config machine_config; */



/***************************************************************************

    Machine driver constants and flags

***************************************************************************/

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


/* ----- flags for video_attributes ----- */

/* bit 0 of the video attributes indicates raster or vector video hardware */
#define	VIDEO_TYPE_RASTER			0x0000
#define	VIDEO_TYPE_VECTOR			0x0001

/* bit 3 of the video attributes indicates that the game's palette has 6 or more bits */
/*       per gun, and would therefore require a 24-bit display. This is entirely up to */
/*       the OS dependant layer, the bitmap will still be 16-bit. */
#define VIDEO_NEEDS_6BITS_PER_GUN	0x0008

/* ASG 980417 - added: */
/* bit 4 of the video attributes indicates that the driver wants its refresh after */
/*       the VBLANK instead of before. */
#define	VIDEO_UPDATE_BEFORE_VBLANK	0x0000
#define	VIDEO_UPDATE_AFTER_VBLANK	0x0010

/* In most cases we assume pixels are square (1:1 aspect ratio) but some games need */
/* different proportions, e.g. 1:2 for Blasteroids */
#define VIDEO_PIXEL_ASPECT_RATIO_MASK 0x0060
#define VIDEO_PIXEL_ASPECT_RATIO_1_1 0x0000
#define VIDEO_PIXEL_ASPECT_RATIO_1_2 0x0020
#define VIDEO_PIXEL_ASPECT_RATIO_2_1 0x0040

#define VIDEO_DUAL_MONITOR			0x0080

/* Mish 181099:  See comments in vidhrdw/generic.c for details */
#define VIDEO_BUFFERS_SPRITERAM		0x0100

/* game wants to use a hicolor or truecolor bitmap (e.g. for alpha blending) */
#define VIDEO_RGB_DIRECT 			0x0200

/* automatically extend the palette creating a darker copy for shadows */
#define VIDEO_HAS_SHADOWS			0x0400

/* automatically extend the palette creating a brighter copy for highlights */
#define VIDEO_HAS_HIGHLIGHTS		0x0800


/* -----    defaults for watchdog   ----- */
#define DEFAULT_60HZ_3S_VBLANK_WATCHDOG	180
#define DEFAULT_30HZ_3S_VBLANK_WATCHDOG	90


/***************************************************************************

    Game driver structure

***************************************************************************/

struct _game_driver
{
	const char *source_file;	/* set this to __FILE__ */
	const struct _game_driver *clone_of;	/* if this is a clone, point to */
										/* the main version of the game */
	const char *name;
	const bios_entry *bios;	/* if this system has alternate bios roms use this */
									/* structure to list names and ROM_BIOSFLAGS. */
	const char *description;
	const char *year;
	const char *manufacturer;
	void (*drv)(machine_config *);
	void (*construct_ipt)(input_port_init_params *param);
	void (*driver_init)(void);	/* optional function to be called during initialization */
								/* This is called ONCE, unlike Machine->init_machine */
								/* which is called every time the game is reset. */

	const rom_entry *rom;
#ifdef MESS
	void (*sysconfig_ctor)(struct SystemConfigurationParamBlock *cfg);
	const struct _game_driver *compatible_with;
#endif

	UINT32 flags;	/* orientation and other flags; see defines below */
};
/* In mamecore.h: typedef struct _game_driver game_driver; */



/***************************************************************************

    Game driver flags

***************************************************************************/

/* ----- values for the flags field ----- */

#define ORIENTATION_MASK        	0x0007
#define	ORIENTATION_FLIP_X			0x0001	/* mirror everything in the X direction */
#define	ORIENTATION_FLIP_Y			0x0002	/* mirror everything in the Y direction */
#define ORIENTATION_SWAP_XY			0x0004	/* mirror along the top-left/bottom-right diagonal */

#define GAME_NOT_WORKING			0x0008
#define GAME_UNEMULATED_PROTECTION	0x0010	/* game's protection not fully emulated */
#define GAME_WRONG_COLORS			0x0020	/* colors are totally wrong */
#define GAME_IMPERFECT_COLORS		0x0040	/* colors are not 100% accurate, but close */
#define GAME_IMPERFECT_GRAPHICS		0x0080	/* graphics are wrong/incomplete */
#define GAME_NO_COCKTAIL			0x0100	/* screen flip support is missing */
#define GAME_NO_SOUND				0x0200	/* sound is missing */
#define GAME_IMPERFECT_SOUND		0x0400	/* sound is known to be wrong */
#define NOT_A_DRIVER				0x4000	/* set by the fake "root" driver_0 and by "containers" */
											/* e.g. driver_neogeo. */
#ifdef MESS
#define GAME_COMPUTER               0x8000  /* Driver is a computer (needs full keyboard) */
#define GAME_COMPUTER_MODIFIED      0x0800	/* Official? Hack */
#endif



/***************************************************************************

    Macros for building game drivers

***************************************************************************/

#define GAME(YEAR,NAME,PARENT,MACHINE,INPUT,INIT,MONITOR,COMPANY,FULLNAME)	\
extern const game_driver driver_##PARENT;	\
const game_driver driver_##NAME =		\
{											\
	__FILE__,								\
	&driver_##PARENT,						\
	#NAME,									\
	system_bios_0,							\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	construct_##MACHINE,					\
	construct_ipt_##INPUT,					\
	init_##INIT,							\
	rom_##NAME,								\
	MONITOR									\
};

#define GAMEX(YEAR,NAME,PARENT,MACHINE,INPUT,INIT,MONITOR,COMPANY,FULLNAME,FLAGS)	\
extern const game_driver driver_##PARENT;	\
const game_driver driver_##NAME =		\
{											\
	__FILE__,								\
	&driver_##PARENT,						\
	#NAME,									\
	system_bios_0,							\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	construct_##MACHINE,					\
	construct_ipt_##INPUT,					\
	init_##INIT,							\
	rom_##NAME,								\
	(MONITOR)|(FLAGS)						\
};

#define GAMEB(YEAR,NAME,PARENT,BIOS,MACHINE,INPUT,INIT,MONITOR,COMPANY,FULLNAME)	\
extern const game_driver driver_##PARENT;	\
const game_driver driver_##NAME =		\
{											\
	__FILE__,								\
	&driver_##PARENT,						\
	#NAME,									\
	system_bios_##BIOS,						\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	construct_##MACHINE,					\
	construct_ipt_##INPUT,					\
	init_##INIT,							\
	rom_##NAME,								\
	MONITOR									\
};

#define GAMEBX(YEAR,NAME,PARENT,BIOS,MACHINE,INPUT,INIT,MONITOR,COMPANY,FULLNAME,FLAGS)	\
extern const game_driver driver_##PARENT;	\
const game_driver driver_##NAME =		\
{											\
	__FILE__,								\
	&driver_##PARENT,						\
	#NAME,									\
	system_bios_##BIOS,						\
	FULLNAME,								\
	#YEAR,									\
	COMPANY,								\
	construct_##MACHINE,					\
	construct_ipt_##INPUT,					\
	init_##INIT,							\
	rom_##NAME,								\
	(MONITOR)|(FLAGS)						\
};

/* monitor parameters to be used with the GAME() macro */
#define	ROT0	0
#define	ROT90	(ORIENTATION_SWAP_XY|ORIENTATION_FLIP_X)	/* rotate clockwise 90 degrees */
#define	ROT180	(ORIENTATION_FLIP_X|ORIENTATION_FLIP_Y)		/* rotate 180 degrees */
#define	ROT270	(ORIENTATION_SWAP_XY|ORIENTATION_FLIP_Y)	/* rotate counter-clockwise 90 degrees */

/* this allows to leave the INIT field empty in the GAME() macro call */
#define init_0 0

/* this allows to leave the BIOS field empty in the GAMEB() macro call */
#define system_bios_0 0



/***************************************************************************

    Global variables

***************************************************************************/

extern const game_driver *drivers[];

#endif	/* __DRIVER_H__ */
