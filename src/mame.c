/***************************************************************************

    mame.c

    Controls execution of the core MAME system.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

****************************************************************************

    Since there has been confusion in the past over the order of
    initialization and other such things, here it is, all spelled out
    as of February, 2006:

    main()
        - does platform-specific init
        - calls run_game() [mame.c]

        run_game() [mame.c]
            - calls mame_validitychecks() [validity.c] to perform validity checks on all compiled drivers
            - calls setjmp to prepare for deep error handling
            - begins resource tracking (level 1)
            - calls create_machine [mame.c] to initialize the Machine structure
            - calls init_machine() [mame.c]

            init_machine() [mame.c]
                - calls cpuintrf_init() [cpuintrf.c] to determine which CPUs are available
                - calls sndintrf_init() [sndintrf.c] to determine which sound chips are available
                - calls fileio_init() [fileio.c] to initialize file I/O info
                - calls config_init() [config.c] to initialize configuration system
                - calls state_init() [state.c] to initialize save state system
                - calls drawgfx_init() [drawgfx.c] to initialize rendering globals
                - calls generic_machine_init() [machine/generic.c] to initialize generic machine structures
                - calls generic_video_init() [vidhrdw/generic.c] to initialize generic video structures
                - calls memcard_init() [usrintrf.c] to initialize memory card state
                - calls osd_init() [osdepend.h] to do platform-specific initialization
                - calls code_init() [input.c] to initialize the input system
                - calls input_port_init() [inptport.c] to set up the input ports
                - calls rom_init() [romload.c] to load the game's ROMs
                - calls state_save_allow_registration() [state.c] to allow registrations
                - calls timer_init() [timer.c] to reset the timer system
                - calls memory_init() [memory.c] to process the game's memory maps
                - calls cpu_init() [cpuexec.c] to initialize the CPUs
                - calls cheat_init() [cheat.c] to initialize the cheat system
                - calls hiscore_init() [hiscore.c] to initialize the hiscores
                - calls the driver's DRIVER_INIT callback
                - calls sound_init() [sndintrf.c] to start the audio system
                - calls video_init() [video.c] to start the video system
                - calls the driver's MACHINE_START, SOUND_START, and VIDEO_START callbacks
                - disposes of regions marked as disposable
                - calls mame_debug_init() [debugcpu.c] to set up the debugger

            - calls config_load_settings() [config.c] to load the configuration file
            - calls nvram_load [machine/generic.c] to load NVRAM
            - calls ui_init() [usrintrf.c] to initialize the user interface
            - calls cpu_run() [cpuexec.c]

            cpu_run() [cpuexec.c]
                - calls cpu_pre_run() [cpuexec.c]

                cpu_pre_run() [cpuexec.c]
                    - begins resource tracking (level 2)
                    - calls cpu_inittimers() [cpuexec.c] to set up basic system timers
                    - calls watchdog_setup() [cpuexec.c] to set up watchdog timers
                    - calls sound_reset() [sndintrf.c] to reset all the sound chips
                    - loops over each CPU and initializes its internal state
                    - calls the driver's MACHINE_RESET callback
                    - loops over each CPU and calls cpunum_reset() [cpuintrf.c] to reset it
                    - calls cpu_vblankreset() [cpuexec.c] to set up the first VBLANK callback

                --------------( at this point, we're up and running )---------------------------

                - ends resource tracking (level 2), freeing all auto_mallocs and timers
                - if the machine is just being reset, loops back to the cpu_pre_run() step above

            - calls the nvram_save() [machine/generic.c] to save NVRAM
            - calls config_save_settings() [config.c] to save the game's configuration
            - calls call_and_free_exit_callbacks() [mame.c] to call all registered exit routines
            - ends resource tracking (level 1), freeing all auto_mallocs and timers

        - exits the program

***************************************************************************/

#include "driver.h"
#include "config.h"
#include "cheat.h"
#include "hiscore.h"
#include "debugger.h"
#include "vidhrdw/generic.h"

#include <ctype.h>
#include <stdarg.h>
#include <setjmp.h>



/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MAX_MEMORY_REGIONS		32



/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

typedef struct _region_info region_info;
struct _region_info
{
	UINT8 *			base;
	size_t			length;
	UINT32			type;
	UINT32			flags;
};


typedef struct _exit_callback exit_callback;
struct _exit_callback
{
	exit_callback *	next;
	void 			(*callback)(void);
};



/***************************************************************************
    GLOBALS
***************************************************************************/

/* the active machine */
static running_machine active_machine;
running_machine *Machine = &active_machine;

/* the active game driver */
static machine_config internal_drv;

/* various game options filled in by the OSD */
global_options options;

/* misc other statics */
static UINT8 mame_paused;

/* error recovery and exiting */
static exit_callback *exit_callback_list;
static jmp_buf fatal_error_jmpbuf;

/* malloc tracking */
static void** malloc_list = NULL;
static int malloc_list_index = 0;
static int malloc_list_size = 0;

/* resource tracking */
int resource_tracking_tag = 0;

/* array of memory regions */
static region_info mem_region[MAX_MEMORY_REGIONS];

const char *mame_disclaimer =
	"MAME is an emulator: it reproduces, more or less faithfully, the behaviour of\n"
	"several arcade machines. But hardware is useless without software, so an image\n"
	"of the ROMs which run on that hardware is required. Such ROMs, like any other\n"
	"commercial software, are copyrighted material and it is therefore illegal to\n"
	"use them if you don't own the original arcade machine. Needless to say, ROMs\n"
	"are not distributed together with MAME. Distribution of MAME together with ROM\n"
	"images is a violation of copyright law and should be promptly reported to the\n"
	"authors so that appropriate legal action can be taken.\n";



/***************************************************************************
    PROTOTYPES
***************************************************************************/

static void call_and_free_exit_callbacks(void);
static void init_machine(void);

static void recompute_fps(int skipped_it);
static void run_start_callbacks(void);
static void create_machine(int game);

extern int mame_validitychecks(void);



/***************************************************************************

    Core system management

***************************************************************************/

/*-------------------------------------------------
    run_game - run the given game in a session
-------------------------------------------------*/

int run_game(int game)
{
	/* perform validity checks before anything else */
	if (mame_validitychecks() != 0)
		return 1;

	/* use setjmp/longjmp for deep error recovery */
	if (setjmp(fatal_error_jmpbuf) == 0)
	{
		int settingsloaded;

		/* start tracking resources for real */
		begin_resource_tracking();

		/* create the Machine structure and driver */
		create_machine(game);

		/* then finish setting up our local machine */
		init_machine();

		/* load the configuration settings and NVRAM */
		settingsloaded = config_load_settings();
		nvram_load();

		/* initialize the UI and display the startup screens */
		if (ui_init(!settingsloaded && !options.skip_disclaimer, !options.skip_warnings, !options.skip_gameinfo) == 0)
		{
			cpu_run();

			/* save the NVRAM and configuration */
			nvram_save();
			config_save_settings();
		}

		/* clean up and stop tracking resources */
		call_and_free_exit_callbacks();
		end_resource_tracking();

		return 0;
	}

	/* error case: clean up */
	else
	{
		/* call all exit callbacks registered */
		call_and_free_exit_callbacks();

		/* close all inner resource tracking */
		while (get_resource_tag() != 0)
			end_resource_tracking();

		/* return an error */
		return 1;
	}
}


/*-------------------------------------------------
    fatalerror - print a message and escape back
    to the caller
-------------------------------------------------*/

void fatalerror(const char *message)
{
	printf("%s\n", message);
	longjmp(fatal_error_jmpbuf, 1);
}


/*-------------------------------------------------
    add_exit_callback - request a callback on
    termination
-------------------------------------------------*/

void add_exit_callback(void (*callback)(void))
{
	exit_callback *cb;

	/* allocate memory */
	cb = malloc(sizeof(*cb));
	if (!cb)
		fatalerror("Out of memory for callbacks");

	/* add us into the list */
	cb->callback = callback;
	cb->next = exit_callback_list;
	exit_callback_list = cb;
}


/*-------------------------------------------------
    call_and_free_exit_callbacks - call all the
    exit callbacks and free them along the way
-------------------------------------------------*/

static void call_and_free_exit_callbacks(void)
{
	while (exit_callback_list != NULL)
	{
		exit_callback *cb = exit_callback_list;

		/* remove us from the list */
		exit_callback_list = exit_callback_list->next;

		/* call the callback and free ourselves */
		(*cb->callback)();
		free(cb);
	}
}


/*-------------------------------------------------
    init_machine - initialize the emulated machine
-------------------------------------------------*/

static void init_machine(void)
{
	int num;

	/* initialize basic can't-fail systems here */
	cpuintrf_init();
	sndintrf_init();
	fileio_init();
	config_init();
	state_init();
	drawgfx_init();
	generic_machine_init();
	generic_video_init();
	memcard_init();

	/* init the osd layer */
	if (osd_init() != 0)
		fatalerror("osd_init failed");

	/* initialize the input system */
	/* this must be done before the input ports are initialized */
	if (code_init() != 0)
		fatalerror("code_init failed");

	/* initialize the input ports for the game */
	/* this must be done before memory_init in order to allow specifying */
	/* callbacks based on input port tags */
	if (input_port_init(Machine->gamedrv->construct_ipt) != 0)
		fatalerror("input_port_init failed");

	/* load the ROMs if we have some */
	/* this must be done before memory_init in order to allocate memory regions */
	if (rom_init(Machine->gamedrv->rom) != 0)
		fatalerror("rom_init failed");

	/* allow save state registrations starting here */
	state_save_allow_registration(TRUE);

	/* initialize the timers */
	/* this must be done before cpu_init so that CPU's can allocate timers */
	timer_init();

	/* initialize the memory system for this game */
	/* this must be done before cpu_init so that set_context can look up the opcode base */
	if (memory_init() != 0)
		fatalerror("memory_init failed");

	/* now set up all the CPUs */
	if (cpu_init() != 0)
		fatalerror("cpu_init failed");

#ifdef MESS
	/* initialize the devices */
	if (devices_init(Machine->gamedrv))
		fatalerror("devices_init failed");
#endif

	/* start the cheat engine */
	if (options.cheat)
		cheat_init();

	/* start the hiscore system -- remove me */
	hiscore_init(Machine->gamedrv->name);

	/* call the game driver's init function */
	/* this is where decryption is done and memory maps are altered */
	/* so this location in the init order is important */
	if (Machine->gamedrv->driver_init != NULL)
		(*Machine->gamedrv->driver_init)();

	/* start the audio system */
	if (sound_init() != 0)
		fatalerror("sound_init failed");

	/* start the video hardware */
	if (video_init() != 0)
		fatalerror("video_init failed");

	/* call the driver's _START callbacks */
	run_start_callbacks();

	/* free memory regions allocated with REGIONFLAG_DISPOSE (typically gfx roms) */
	for (num = 0; num < MAX_MEMORY_REGIONS; num++)
		if (mem_region[num].flags & ROMREGION_DISPOSE)
			free_memory_region(num);

#ifdef MAME_DEBUG
	/* initialize the debugger */
	if (Machine->debug_mode)
		mame_debug_init();
#endif
}



/*-------------------------------------------------
    mame_pause - pause or resume the system
-------------------------------------------------*/

void mame_pause(int pause)
{
	mame_paused = pause;
	cpu_pause(pause);
	osd_pause(pause);
	osd_sound_enable(!pause);
	palette_set_global_brightness_adjust(pause ? options.pause_bright : 1.00);
	schedule_full_refresh();
}


int mame_is_paused(void)
{
	return mame_paused;
}



/*-------------------------------------------------
    run_start_callbacks - execute the various
    _START() callbacks, and register the stop
    callbacks
-------------------------------------------------*/

static void run_start_callbacks(void)
{
	if (Machine->drv->machine_start != NULL && (*Machine->drv->machine_start)() != 0)
		fatalerror("Unable to start machine emulation");
	if (Machine->drv->sound_start != NULL && (*Machine->drv->sound_start)() != 0)
		fatalerror("Unable to start sound emulation");
	if (Machine->drv->video_start != NULL && (*Machine->drv->video_start)() != 0)
		fatalerror("Unable to start video emulation");
}



/*-------------------------------------------------
    create_machine - create the running machine
    object and initialize it based on options
-------------------------------------------------*/

static void create_machine(int game)
{
	/* first give the machine a good cleaning */
	memset(Machine, 0, sizeof(*Machine));

	/* initialize the driver-related variables in the Machine */
	Machine->gamedrv = drivers[game];
	Machine->drv = &internal_drv;
	expand_machine_driver(Machine->gamedrv->drv, &internal_drv);
	Machine->refresh_rate = Machine->drv->frames_per_second;

	/* copy some settings into easier-to-handle variables */
	Machine->record_file = options.record;
	Machine->playback_file = options.playback;
	Machine->debug_mode = options.mame_debug;

	/* determine the color depth */
	Machine->color_depth = 16;
	if (Machine->drv->video_attributes & VIDEO_RGB_DIRECT)
		Machine->color_depth = (Machine->drv->video_attributes & VIDEO_NEEDS_6BITS_PER_GUN) ? 32 : 15;

	/* update the vector width/height with defaults */
	if (options.vector_width == 0)
		options.vector_width = 640;
	if (options.vector_height == 0)
		options.vector_height = 480;

	/* initialize the samplerate */
	Machine->sample_rate = options.samplerate;

	/* get orientation right */
	Machine->ui_orientation = options.ui_orientation;

	/* if we're coming in with a savegame request, process it now */
	if (options.savegame)
	{
		if (strlen(options.savegame) == 1)
			cpu_loadsave_schedule(LOADSAVE_LOAD, options.savegame[0]);
		else
			cpu_loadsave_schedule_file(LOADSAVE_LOAD, options.savegame);
	}
	else if (options.auto_save && (Machine->gamedrv->flags & GAME_SUPPORTS_SAVE))
		cpu_loadsave_schedule_file(LOADSAVE_LOAD, Machine->gamedrv->name);
	else
		cpu_loadsave_reset();
}



/***************************************************************************

    Memory region code

***************************************************************************/

/*-------------------------------------------------
    memory_region_to_index - returns an index
    given either an index or a REGION_* identifier
-------------------------------------------------*/

int memory_region_to_index(int num)
{
	int i;

	/* if we're already an index, stop there */
	if (num < MAX_MEMORY_REGIONS)
		return num;

	/* scan for a match */
	for (i = 0; i < MAX_MEMORY_REGIONS; i++)
		if (mem_region[i].type == num)
			return i;

	return -1;
}


/*-------------------------------------------------
    new_memory_region - allocates memory for a
    region
-------------------------------------------------*/

int new_memory_region(int type, size_t length, UINT32 flags)
{
    int num;

    assert(type >= MAX_MEMORY_REGIONS);

    /* find a free slot */
	for (num = 0; num < MAX_MEMORY_REGIONS; num++)
		if (mem_region[num].base == NULL)
			break;
	if (num < 0)
		return 1;

    /* allocate the region */
	mem_region[num].length = length;
	mem_region[num].type = type;
	mem_region[num].flags = flags;
	mem_region[num].base = malloc(length);
	return (mem_region[num].base == NULL) ? 1 : 0;
}


/*-------------------------------------------------
    free_memory_region - releases memory for a
    region
-------------------------------------------------*/

void free_memory_region(int num)
{
	/* convert to an index and bail if invalid */
	num = memory_region_to_index(num);
	if (num < 0)
		return;

	/* free the region in question */
	free(mem_region[num].base);
	memset(&mem_region[num], 0, sizeof(mem_region[num]));
}


/*-------------------------------------------------
    memory_region - returns pointer to a memory
    region
-------------------------------------------------*/

UINT8 *memory_region(int num)
{
	/* convert to an index and return the result */
	num = memory_region_to_index(num);
	return (num >= 0) ? mem_region[num].base : NULL;
}


/*-------------------------------------------------
    memory_region_length - returns length of a
    memory region
-------------------------------------------------*/

size_t memory_region_length(int num)
{
	/* convert to an index and return the result */
	num = memory_region_to_index(num);
	return (num >= 0) ? mem_region[num].length : 0;
}



/***************************************************************************

    Resource tracking code

***************************************************************************/

/*-------------------------------------------------
    auto_malloc_add - add pointer to malloc list
-------------------------------------------------*/

INLINE void auto_malloc_add(void *result)
{
	/* make sure we have tracking space */
	if (malloc_list_index == malloc_list_size)
	{
		void **list;

		/* if this is the first time, allocate 256 entries, otherwise double the slots */
		if (malloc_list_size == 0)
			malloc_list_size = 256;
		else
			malloc_list_size *= 2;

		/* realloc the list */
		list = realloc(malloc_list, malloc_list_size * sizeof(list[0]));
		if (list == NULL)
			osd_die("Unable to extend malloc tracking array to %d slots", malloc_list_size);
		malloc_list = list;
	}
	malloc_list[malloc_list_index++] = result;
}


/*-------------------------------------------------
    auto_malloc_free - release auto_malloc'd memory
-------------------------------------------------*/

static void auto_malloc_free(void)
{
	/* start at the end and free everything till you reach the sentinel */
	while (malloc_list_index > 0 && malloc_list[--malloc_list_index] != NULL)
		free(malloc_list[malloc_list_index]);

	/* if we free everything, free the list */
	if (malloc_list_index == 0)
	{
		free(malloc_list);
		malloc_list = NULL;
		malloc_list_size = 0;
	}
}


/*-------------------------------------------------
    begin_resource_tracking - start tracking
    resources
-------------------------------------------------*/

void begin_resource_tracking(void)
{
	/* add a NULL as a sentinel */
	auto_malloc_add(NULL);

	/* increment the tag counter */
	resource_tracking_tag++;
}


/*-------------------------------------------------
    end_resource_tracking - stop tracking
    resources
-------------------------------------------------*/

void end_resource_tracking(void)
{
	/* call everyone who tracks resources to let them know */
	auto_malloc_free();
	timer_free();
	state_save_free();

	/* decrement the tag counter */
	resource_tracking_tag--;
}


/*-------------------------------------------------
    auto_malloc - allocate auto-freeing memory
-------------------------------------------------*/

void *auto_malloc(size_t size)
{
	void *result;

	/* malloc-ing zero bytes is inherently unportable; hence this check */
	assert_always(size != 0, "Attempted to auto_malloc zero bytes");

	/* fail horribly if it doesn't work */
	result = malloc(size);
	if (!result)
		osd_die("Out of memory attempting to allocate %d bytes\n", (int)size);

	/* track this item in our list */
	auto_malloc_add(result);
	return result;
}


/*-------------------------------------------------
    auto_strdup - allocate auto-freeing string
-------------------------------------------------*/

char *auto_strdup(const char *str)
{
	return strcpy(auto_malloc(strlen(str) + 1), str);
}



/***************************************************************************

    Miscellaneous bits & pieces

***************************************************************************/

/*-------------------------------------------------
    mame_highscore_enabled - return 1 if high
    scores are enabled
-------------------------------------------------*/

int mame_highscore_enabled(void)
{
	/* disable high score when record/playback is on */
	if (Machine->record_file != NULL || Machine->playback_file != NULL)
		return FALSE;

	/* disable high score when cheats are used */
	if (he_did_cheat != 0)
		return FALSE;

	return TRUE;
}



/*-------------------------------------------------
    mame_find_cpu_index - return the index of the
    given CPU, or -1 if not found
-------------------------------------------------*/

int mame_find_cpu_index(const char *tag)
{
	int cpunum;

	for (cpunum = 0; cpunum < MAX_CPU; cpunum++)
		if (Machine->drv->cpu[cpunum].tag && strcmp(Machine->drv->cpu[cpunum].tag, tag) == 0)
			return cpunum;

	return -1;
}
