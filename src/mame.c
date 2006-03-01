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
                - calls hiscore_init() [hiscore.c] to initialize the hiscores
                - calls saveload_init() [mame.c] to set up for save/load
                - calls the driver's DRIVER_INIT callback
                - calls sound_init() [sound.c] to start the audio system
                - calls video_init() [video.c] to start the video system
                - calls cheat_init() [cheat.c] to initialize the cheat system
                - calls the driver's MACHINE_START, SOUND_START, and VIDEO_START callbacks
                - disposes of regions marked as disposable
                - calls mame_debug_init() [debugcpu.c] to set up the debugger

            - calls config_load_settings() [config.c] to load the configuration file
            - calls nvram_load [machine/generic.c] to load NVRAM
            - calls ui_init() [usrintrf.c] to initialize the user interface
            - begins resource tracking (level 2)
            - calls soft_reset() [mame.c] to reset all systems

                -------------------( at this point, we're up and running )----------------------

            - calls cpu_timeslice() [cpuexec.c] over and over until we exit
            - ends resource tracking (level 2), freeing all auto_mallocs and timers
            - calls the nvram_save() [machine/generic.c] to save NVRAM
            - calls config_save_settings() [config.c] to save the game's configuration
            - calls all registered exit routines [mame.c]
            - ends resource tracking (level 1), freeing all auto_mallocs and timers

        - exits the program

***************************************************************************/

#include "osdepend.h"
#include "driver.h"
#include "config.h"
#include "cheat.h"
#include "hiscore.h"
#include "debugger.h"
#include "profiler.h"

#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
#include "debug/debugcon.h"
#endif

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


typedef struct _callback_item callback_item;
struct _callback_item
{
	callback_item *	next;
	union
	{
		void		(*exit)(void);
		void		(*reset)(void);
		void		(*pause)(int);
	} func;
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
static UINT8 hard_reset_pending;
static UINT8 exit_pending;
static char *saveload_pending_file;

/* load/save statics */
static mame_timer *saveload_timer;
static mame_time saveload_schedule_time;

/* error recovery and exiting */
static callback_item *reset_callback_list;
static callback_item *pause_callback_list;
static callback_item *exit_callback_list;
static jmp_buf fatal_error_jmpbuf;

/* malloc tracking */
static void **malloc_list = NULL;
static int malloc_list_index = 0;
static int malloc_list_size = 0;

/* resource tracking */
int resource_tracking_tag = 0;

/* array of memory regions */
static region_info mem_region[MAX_MEMORY_REGIONS];

/* a giant string buffer for temporary strings */
char giant_string_buffer[65536];

/* the "disclaimer" that should be printed when run with no parameters */
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

extern int mame_validitychecks(void);

static void create_machine(int game);
static void init_machine(void);
static void soft_reset(int param);
static void free_callback_list(callback_item **cb);

static void saveload_init(void);
static void saveload_attempt(int is_save);
static int handle_save(void);
static int handle_load(void);



/***************************************************************************

    Core system management

***************************************************************************/

/*-------------------------------------------------
    run_game - run the given game in a session
-------------------------------------------------*/

int run_game(int game)
{
	callback_item *cb;
	int error = 0;

	/* perform validity checks before anything else */
	if (mame_validitychecks() != 0)
		return 1;

	/* loop across multiple hard resets */
	exit_pending = FALSE;
	while (error == 0 && !exit_pending)
	{
		/* use setjmp/longjmp for deep error recovery */
		error = setjmp(fatal_error_jmpbuf);
		if (error == 0)
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
			if (ui_init(!settingsloaded && !options.skip_disclaimer, !options.skip_warnings, !options.skip_gameinfo) != 0)
				fatalerror("User cancelled");

			/* ensure we don't show the opening screens on a reset */
			options.skip_disclaimer = options.skip_warnings = options.skip_gameinfo = TRUE;

			/* start resource tracking; note that soft_reset assumes it can */
			/* call end_resource_tracking followed by begin_resource_tracking */
			/* to clear out resources allocated between resets */
			begin_resource_tracking();

			/* perform a soft reset */
			soft_reset(0);

			/* run the CPUs until a reset or exit */
			hard_reset_pending = FALSE;
			while ((!hard_reset_pending && !exit_pending) || saveload_pending_file != NULL)
			{
				profiler_mark(PROFILER_EXTRA);

				/* execute CPUs if not paused */
				if (!mame_paused)
					cpu_timeslice();

				/* otherwise, just pump video updates through */
				else
				{
					updatescreen();
					reset_partial_updates();
				}

				profiler_mark(PROFILER_END);
			}

			/* stop tracking resources at this level */
			end_resource_tracking();

			/* save the NVRAM and configuration */
			nvram_save();
			config_save_settings();
		}

		/* call all exit callbacks registered */
		for (cb = exit_callback_list; cb; cb = cb->next)
			(*cb->func.exit)();

		/* close all inner resource tracking */
		while (resource_tracking_tag != 0)
			end_resource_tracking();

		/* free our callback lists */
		free_callback_list(&exit_callback_list);
		free_callback_list(&reset_callback_list);
		free_callback_list(&pause_callback_list);
	}

	/* return an error */
	return error;
}


/*-------------------------------------------------
    add_exit_callback - request a callback on
    termination
-------------------------------------------------*/

void add_exit_callback(void (*callback)(void))
{
	callback_item *cb;

	/* allocate memory */
	cb = malloc_or_die(sizeof(*cb));

	/* add us to the head of the list */
	cb->func.exit = callback;
	cb->next = exit_callback_list;
	exit_callback_list = cb;
}


/*-------------------------------------------------
    add_reset_callback - request a callback on
    reset
-------------------------------------------------*/

void add_reset_callback(void (*callback)(void))
{
	callback_item *cb, **cur;

	/* allocate memory */
	cb = malloc_or_die(sizeof(*cb));

	/* add us to the end of the list */
	cb->func.reset = callback;
	cb->next = NULL;
	for (cur = &reset_callback_list; *cur; cur = &(*cur)->next) ;
	*cur = cb;
}


/*-------------------------------------------------
    add_pause_callback - request a callback on
    pause
-------------------------------------------------*/

void add_pause_callback(void (*callback)(int))
{
	callback_item *cb, **cur;

	/* allocate memory */
	cb = malloc_or_die(sizeof(*cb));

	/* add us to the end of the list */
	cb->func.pause = callback;
	cb->next = NULL;
	for (cur = &pause_callback_list; *cur; cur = &(*cur)->next) ;
	*cur = cb;
}



/***************************************************************************

    Global System States

***************************************************************************/

/*-------------------------------------------------
    mame_schedule_exit - schedule a clean exit
-------------------------------------------------*/

void mame_schedule_exit(void)
{
	exit_pending = TRUE;

	/* if we're autosaving on exit, schedule a save as well */
	if (options.auto_save && (Machine->gamedrv->flags & GAME_SUPPORTS_SAVE))
		mame_schedule_save(Machine->gamedrv->name);
}


/*-------------------------------------------------
    mame_schedule_hard_reset - schedule a hard-
    reset of the system
-------------------------------------------------*/

void mame_schedule_hard_reset(void)
{
	hard_reset_pending = TRUE;
}


/*-------------------------------------------------
    mame_schedule_soft_reset - schedule a soft-
    reset of the system
-------------------------------------------------*/

void mame_schedule_soft_reset(void)
{
	mame_timer_set(time_zero, 0, soft_reset);
}


/*-------------------------------------------------
    mame_schedule_save - schedule a save to
    occur as soon as possible
-------------------------------------------------*/

void mame_schedule_save(const char *filename)
{
	/* can't do it if we don't yet have a timer */
	if (saveload_timer == NULL)
		return;

	/* free any existing request and allocate a copy of the requested name */
	if (saveload_pending_file != NULL)
		free(saveload_pending_file);
	saveload_pending_file = mame_strdup(filename);

	/* note the start time and set a timer for the next timeslice to actually schedule it */
	saveload_schedule_time = mame_timer_get_time();
	mame_timer_adjust(saveload_timer, time_zero, TRUE, time_zero);

	/* we can't be paused since we need to clear out anonymous timers */
	mame_pause(FALSE);
}


/*-------------------------------------------------
    mame_schedule_load - schedule a load to
    occur as soon as possible
-------------------------------------------------*/

void mame_schedule_load(const char *filename)
{
	/* can't do it if we don't yet have a timer */
	if (saveload_timer == NULL)
		return;

	/* free any existing request and allocate a copy of the requested name */
	if (saveload_pending_file != NULL)
		free(saveload_pending_file);
	saveload_pending_file = mame_strdup(filename);

	/* note the start time and set a timer for the next timeslice to actually schedule it */
	saveload_schedule_time = mame_timer_get_time();
	mame_timer_adjust(saveload_timer, time_zero, FALSE, time_zero);

	/* we can't be paused since we need to clear out anonymous timers */
	mame_pause(FALSE);
}


/*-------------------------------------------------
    mame_is_scheduled_event_pending - is a
    scheduled event pending?
-------------------------------------------------*/

int mame_is_scheduled_event_pending(void)
{
	/* we can't check for saveload_pending_file here because it will bypass */
	/* required UI screens if a state is queued from the command line */
	return exit_pending || hard_reset_pending;
}


/*-------------------------------------------------
    mame_pause - pause or resume the system
-------------------------------------------------*/

void mame_pause(int pause)
{
	callback_item *cb;

	/* ignore if nothing has changed */
	if (mame_paused == pause)
		return;
	mame_paused = pause;

	/* call all registered pause callbacks */
	for (cb = pause_callback_list; cb; cb = cb->next)
		(*cb->func.pause)(mame_paused);
}


/*-------------------------------------------------
    mame_is_paused - the system paused?
-------------------------------------------------*/

int mame_is_paused(void)
{
	return mame_paused;
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
			fatalerror("Unable to extend malloc tracking array to %d slots", malloc_list_size);
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

void *_auto_malloc(size_t size, const char *file, int line)
{
	void *result;

	/* fail horribly if it doesn't work */
	result = _malloc_or_die(size, file, line);

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
    fatalerror - print a message and escape back
    to the OSD layer
-------------------------------------------------*/

void CLIB_DECL fatalerror(const char *text, ...)
{
	va_list arg;

	/* dump to the buffer; assume no one writes >2k lines this way */
	va_start(arg, text);
	vsnprintf(giant_string_buffer, sizeof(giant_string_buffer), text, arg);
	va_end(arg);

	/* output and return */
	printf("%s\n", giant_string_buffer);
	longjmp(fatal_error_jmpbuf, 1);
}


/*-------------------------------------------------
    logerror - log to the debugger and any other
    OSD-defined output streams
-------------------------------------------------*/

void CLIB_DECL logerror(const char *text, ...)
{
	va_list arg;

	/* dump to the buffer; assume no one writes >2k lines to error.log */
	va_start(arg, text);
	vsnprintf(giant_string_buffer, sizeof(giant_string_buffer), text, arg);
	va_end(arg);

#if defined(MAME_DEBUG) && defined(NEW_DEBUGGER)
	/* output to the debugger if running */
	if (Machine && Machine->debug_mode)
		debug_errorlog_write_line(giant_string_buffer);
#endif

	/* let the OSD layer do its own logging if it wants */
	osd_logerror(giant_string_buffer);
}


/*-------------------------------------------------
    _malloc_or_die - allocate memory or die
    trying
-------------------------------------------------*/

void *_malloc_or_die(size_t size, const char *file, int line)
{
	void *result;

	/* fail on attempted allocations of 0 */
	if (size == 0)
		fatalerror("Attempted to malloc zero bytes (%s:%d)", file, line);

	/* allocate and return if we succeeded */
	result = malloc(size);
	if (result != NULL)
		return result;

	/* otherwise, die horribly */
	fatalerror("Failed to allocate %d bytes (%s:%d)", size, file, line);
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


/*-------------------------------------------------
    mame_stricmp - case-insensitive string compare
-------------------------------------------------*/

int mame_stricmp(const char *s1, const char *s2)
{
	for (;;)
 	{
		int c1 = tolower(*s1++);
		int c2 = tolower(*s2++);
		if (c1 == 0 || c1 != c2)
			return c1 - c2;
 	}
}


/*-------------------------------------------------
    mame_strdup - string duplication via malloc
-------------------------------------------------*/

char *mame_strdup(const char *str)
{
	char *cpy = NULL;
	if (str != NULL)
	{
		cpy = malloc(strlen(str) + 1);
		if (cpy != NULL)
			strcpy(cpy, str);
	}
	return cpy;
}



/***************************************************************************

    Internal initialization logic

***************************************************************************/

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

	/* start the hiscore system -- remove me */
	hiscore_init(Machine->gamedrv->name);

	/* start the save/load system */
	saveload_init();

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

	/* start the cheat engine */
	if (options.cheat)
		cheat_init();

	/* call the driver's _START callbacks */
	if (Machine->drv->machine_start != NULL && (*Machine->drv->machine_start)() != 0)
		fatalerror("Unable to start machine emulation");
	if (Machine->drv->sound_start != NULL && (*Machine->drv->sound_start)() != 0)
		fatalerror("Unable to start sound emulation");
	if (Machine->drv->video_start != NULL && (*Machine->drv->video_start)() != 0)
		fatalerror("Unable to start video emulation");

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
    soft_reset - actually perform a soft-reset
    of the system
-------------------------------------------------*/

static void soft_reset(int param)
{
	callback_item *cb;

	logerror("Soft reset\n");

	/* a bit gross -- back off of the resource tracking, and put it back at the end */
	assert(resource_tracking_tag == 2);
	end_resource_tracking();
	begin_resource_tracking();

	/* allow save state registrations during the reset */
	state_save_allow_registration(TRUE);

	/* call all registered reset callbacks */
	for (cb = reset_callback_list; cb; cb = cb->next)
		(*cb->func.reset)();

	/* run the driver's reset callbacks */
	if (Machine->drv->machine_reset != NULL)
		(*Machine->drv->machine_reset)();
	if (Machine->drv->sound_reset != NULL)
		(*Machine->drv->sound_reset)();
	if (Machine->drv->video_reset != NULL)
		(*Machine->drv->video_reset)();

	/* disallow save state registrations starting here */
	state_save_allow_registration(FALSE);

	/* set the global time to the current time */
	/* this allows 0-time queued callbacks to run before any CPUs execute */
	mame_timer_set_global_time(mame_timer_get_time());
}


/*-------------------------------------------------
    free_callback_list - free a list of callbacks
-------------------------------------------------*/

static void free_callback_list(callback_item **cb)
{
	while (*cb)
	{
		callback_item *temp = *cb;
		*cb = (*cb)->next;
		free(temp);
	}
}



/***************************************************************************

    Save/restore

***************************************************************************/

/*-------------------------------------------------
    saveload_init - initialize the save/load logic
-------------------------------------------------*/

static void saveload_init(void)
{
	/* allocate a timer */
	saveload_timer = timer_alloc(saveload_attempt);

	/* if we're coming in with a savegame request, process it now */
	if (options.savegame)
	{
		char name[20];

		if (strlen(options.savegame) == 1)
		{
			sprintf(name, "%s-%c", Machine->gamedrv->name, options.savegame[0]);
			mame_schedule_load(name);
		}
		else
			mame_schedule_load(options.savegame);
	}

	/* if we're in autosave mode, schedule a load */
	else if (options.auto_save && (Machine->gamedrv->flags & GAME_SUPPORTS_SAVE))
		mame_schedule_load(Machine->gamedrv->name);
}


/*-------------------------------------------------
    handle_save - attempt to perform a save
-------------------------------------------------*/

static void saveload_attempt(int is_save)
{
	int try_again;

	/* handle either save or load */
	if (is_save)
		try_again = handle_save();
	else
		try_again = handle_load();

	/* try again after the next timer fires */
	if (try_again)
		mame_timer_adjust(saveload_timer, make_mame_time(0, MAX_SUBSECONDS / 1000), is_save, time_zero);
}


/*-------------------------------------------------
    handle_save - attempt to perform a save
-------------------------------------------------*/

static int handle_save(void)
{
	mame_file *file;

	/* if no name, bail */
	if (saveload_pending_file == NULL)
		return FALSE;

	/* if there are anonymous timers, we can't save just yet */
	if (timer_count_anonymous() > 0)
	{
		/* if more than a second has passed, we're probably screwed */
		if (sub_mame_times(mame_timer_get_time(), saveload_schedule_time).seconds > 0)
		{
			ui_popup("Unable to save due to pending anonymous timers. See error.log for details.");
			goto cancel;
		}
		return TRUE;
	}

	/* open the file */
	file = mame_fopen(Machine->gamedrv->name, saveload_pending_file, FILETYPE_STATE, 1);
	if (file)
	{
		int cpunum;

		/* write the save state */
		if (state_save_save_begin(file) != 0)
		{
			ui_popup("Error: Unable to save state due to illegal registrations. See error.log for details.");
			mame_fclose(file);
			goto cancel;
		}

		/* write the default tag */
		state_save_push_tag(0);
		state_save_save_continue();
		state_save_pop_tag();

		/* loop over CPUs */
		for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
		{
			cpuintrf_push_context(cpunum);

			/* make sure banking is set */
			activecpu_reset_banking();

			/* save the CPU data */
			state_save_push_tag(cpunum + 1);
			state_save_save_continue();
			state_save_pop_tag();

			cpuintrf_pop_context();
		}

		/* finish and close */
		state_save_save_finish();
		mame_fclose(file);

		/* pop a warning if the game doesn't support saves */
		if (!(Machine->gamedrv->flags & GAME_SUPPORTS_SAVE))
			ui_popup("State successfully saved.\nWarning: Save states are not officially supported for this game.");
		else
			ui_popup("State successfully saved.");
	}
	else
		ui_popup("Error: Failed to save state");

cancel:
	/* unschedule the save */
	free(saveload_pending_file);
	saveload_pending_file = NULL;
	return FALSE;
}



/*-------------------------------------------------
    handle_load - attempt to perform a load
-------------------------------------------------*/

static int handle_load(void)
{
	mame_file *file;

	/* if no name, bail */
	if (saveload_pending_file == NULL)
		return FALSE;

	/* if there are anonymous timers, we can't load just yet because the timers might */
	/* overwrite data we have loaded */
	if (timer_count_anonymous() > 0)
	{
		/* if more than a second has passed, we're probably screwed */
		if (sub_mame_times(mame_timer_get_time(), saveload_schedule_time).seconds > 0)
		{
			ui_popup("Unable to load due to pending anonymous timers. See error.log for details.");
			goto cancel;
		}
		return TRUE;
	}

	/* open the file */
	file = mame_fopen(Machine->gamedrv->name, saveload_pending_file, FILETYPE_STATE, 0);
	if (file)
	{
		/* start loading */
		if (state_save_load_begin(file) == 0)
		{
			int cpunum;

			/* read tag 0 */
			state_save_push_tag(0);
			state_save_load_continue();
			state_save_pop_tag();

			/* loop over CPUs */
			for (cpunum = 0; cpunum < cpu_gettotalcpu(); cpunum++)
			{
				cpuintrf_push_context(cpunum);

				/* make sure banking is set */
				activecpu_reset_banking();

				/* load the CPU data */
				state_save_push_tag(cpunum + 1);
				state_save_load_continue();
				state_save_pop_tag();

				/* make sure banking is set */
				activecpu_reset_banking();

				cpuintrf_pop_context();
			}

			/* finish and close */
			state_save_load_finish();
			ui_popup("State successfully loaded.");
		}
		else
			ui_popup("Error: Failed to load state");
		mame_fclose(file);
	}
	else
		ui_popup("Error: Failed to load state");

cancel:
	/* unschedule the load */
	free(saveload_pending_file);
	saveload_pending_file = NULL;
	return FALSE;
}
