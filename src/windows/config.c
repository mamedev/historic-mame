//============================================================
//
//  config.c - Win32 configuration routines
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

/*
 * Configuration routines.
 *
 * 20010424 BW uses Hans de Goede's rc subsystem
 * last changed 20010727 BW
 *
 * TODO:
 * - make errorlog a ringbuffer
 *
 * Suggestions
 * - norotate? funny, leads to option -nonorotate ...
 *   fix when rotation options take turnable LCD's in account
 * - win_switch_bpp --> switch_bpp, swbpp
 * - give up distinction between vector_width and win_gfx_width
 *   eventually introduce options.width, options.height
 * - new core options:
 *   gamma (is already osd_)
 *   sound (enable/disable sound)
 *   volume
  * - get rid of #ifdef MESS's by providing appropriate hooks
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include "osdepend.h"
#include "driver.h"

#include "misc.h"
#ifndef NEW_RENDER
#include "videoold.h"
#else
#include "video.h"
#include "render.h"
#endif

#ifdef NEW_DEBUGGER
#include "debug/debugcpu.h"
#include "debug/debugcon.h"
#endif

#include "options.h"


int frontend_listxml(FILE *output);
int frontend_listfull(FILE *output);
int frontend_listsource(FILE *output);
int frontend_listclones(FILE *output);
int frontend_listcrc(FILE *output);
int frontend_listroms(FILE *output);
int frontend_listsamples(FILE *output);
int frontend_verifyroms(FILE *output);
int frontend_verifysamples(FILE *output);
int frontend_romident(FILE *output);
int frontend_isknown(FILE *output);

void set_pathlist(int file_type, const char *new_rawpath);

extern const options_entry fileio_opts[];
extern const options_entry video_opts[];
extern const options_entry input_opts[];
#ifdef MESS
extern const options_entry mess_opts[];
#endif

#ifdef MESS
#include "configms.h"
#endif

int win_erroroslog;

/* fix me - need to have the core call osd_set_mastervolume with this value */
/* instead of relying on the name of an osd variable */
extern int attenuation;
extern int audio_latency;
extern const char *wavwrite;
extern int verbose;

static char *gamename;
static char *gamepath;

char *rompath_extra;

static char *win_basename(char *filename);
static char *win_dirname(char *filename);
static char *win_strip_extension(char *filename);


#ifndef NEW_RENDER
static void set_old_video_options(const game_driver *driver);
#endif

static void extract_options(const game_driver *driver, machine_config *drv);

/* struct definitions */
static const options_entry config_opts[] =
{
	{ "",                         NULL,   0,                 NULL },
	{ "help;h;?",                 NULL,   OPTION_COMMAND,    "show help message" },

	// video options
	{ NULL,                       NULL,   OPTION_HEADER,     "CORE VIDEO OPTIONS" },
	{ "rotate",                   "1",    OPTION_BOOLEAN,    "rotate the game screen according to the game's orientation needs it" },
	{ "ror",                      "0",    OPTION_BOOLEAN,    "rotate screen clockwise 90 degrees" },
	{ "rol",                      "0",    OPTION_BOOLEAN,    "rotate screen counterclockwise 90 degrees" },
	{ "autoror",                  "0",    OPTION_BOOLEAN,    "automatically rotate screen clockwise 90 degrees if vertical" },
	{ "autorol",                  "0",    OPTION_BOOLEAN,    "automatically rotate screen counterclockwise 90 degrees if vertical" },
	{ "flipx",                    "0",    OPTION_BOOLEAN,    "flip screen left-right" },
	{ "flipy",                    "0",    OPTION_BOOLEAN,    "flip screen upside-down" },
	{ "brightness",               "1.0",  0,                 "brightness correction" },
	{ "pause_brightness",         "1.0",  0,                 "additional pause brightness" },

	// vector options
	{ NULL,                       NULL,   OPTION_HEADER,     "CORE VECTOR OPTIONS" },
	{ "antialias;aa",             "1",    OPTION_BOOLEAN,    "use antialiasing when drawing vectors" },
	{ "beam",                     "1.0",  0,                 "set vector beam width" },
	{ "flicker",                  "1.0",  0,                 "set vector flicker effect" },
	{ "intensity",                "1.0",  0,                 "set vector intensity" },

	// sound options
	{ NULL,                       NULL,   OPTION_HEADER,     "CORE SOUND OPTIONS" },
	{ "sound",                    "1",    OPTION_BOOLEAN,    "enable sound output" },
	{ "samplerate;sr",            "48000",0,                 "set sound output sample rate" },
	{ "samples",                  "1",    OPTION_BOOLEAN,    "enable the use of external samples if available" },
	{ "volume",                   "0",    0,                 "sound volume in decibels (-32 min, 0 max)" },
	{ "audio_latency",            "1",    0,                 "set audio latency (increase to reduce glitches)" },
	{ "wavwrite",                 NULL,   0,                 "save sound in wav file" },

	// misc options
	{ NULL,                       NULL,   OPTION_HEADER,     "CORE MISC OPTIONS" },
	{ "bios",                     "default", 0,              "select the system BIOS to use" },
	{ "cheat;c",                  "0",    OPTION_BOOLEAN,    "enable cheat subsystem" },
	{ "skip_gameinfo",            "0",    OPTION_BOOLEAN,    "skip displaying the information screen at startup" },
	{ "artwork;art",              "1",    OPTION_BOOLEAN,    "enable external artwork, if available" },
	{ "use_backdrops;backdrop",   "1",    OPTION_BOOLEAN,    "enable backdrops if artwork is enabled and available" },
	{ "use_overlays;overlay",     "1",    OPTION_BOOLEAN,    "enable overlays if artwork is enabled and available" },
	{ "use_bezels;bezel",         "1",    OPTION_BOOLEAN,    "enable bezels if artwork is enabled and available" },

	// save states and input recording
	{ NULL,                       NULL,   OPTION_HEADER,     "CORE STATE/PLAYBACK OPTIONS" },
	{ "playback;pb",              NULL,   0,                 "playback an input file" },
	{ "record;rec",               NULL,   0,                 "record an input file" },
	{ "state",                    NULL,   0,                 "saved state to load" },
	{ "autosave",                 "0",    OPTION_BOOLEAN,    "enable automatic restore at startup, and automatic save at exit time" },

	// debugging options
	{ NULL,                       NULL,   OPTION_HEADER,     "CORE DEBUGGING OPTIONS" },
	{ "log",                      "0",    OPTION_BOOLEAN,    "generate an error.log file" },
	{ "oslog",                    "0",    OPTION_BOOLEAN,    "output error.log data to the system debugger" },
	{ "verbose;v",                "0",    OPTION_BOOLEAN,    "display additional diagnostic information" },
	{ "validate;valid",           NULL,   OPTION_COMMAND,    "perform driver validation on all game drivers" },
#ifdef MAME_DEBUG
	{ "debug;d",                  "0",    OPTION_BOOLEAN,    "enable/disable debugger" },
	{ "debugscript",              NULL,   0,                 "script for debugger" },
#else
	{ "debug;d",                  "0",    OPTION_DEPRECATED, "(debugger-only command)" },
	{ "debugscript",              NULL,   OPTION_DEPRECATED, "(debugger-only command)" },
#endif

	// config options
	{ NULL,                       NULL,   OPTION_HEADER,     "CORE CONFIGURATION OPTIONS" },
	{ "createconfig;cc",          NULL,   OPTION_COMMAND,    "create the default configuration file" },
	{ "showconfig;sc",            NULL,   OPTION_COMMAND,    "display running parameters" },
	{ "showusage;su",             NULL,   OPTION_COMMAND,    "show this help" },
	{ "readconfig;rc",            "1",    OPTION_BOOLEAN,    "enable loading of configuration files" },

	// frontend commands
	{ "listxml;lx",               NULL,   OPTION_COMMAND,    "all available info on driver in XML format" },
	{ "listfull;ll",              NULL,   OPTION_COMMAND,    "short name, full name" },
	{ "listsource;ls",            NULL,   OPTION_COMMAND,    "driver sourcefile" },
	{ "listclones;lc",            NULL,   OPTION_COMMAND,    "show clones" },
	{ "listcrc",                  NULL,   OPTION_COMMAND,    "CRC-32s" },
#ifdef MESS
	{ "listdevices",              NULL,   OPTION_COMMAND,    "list available devices" },
#endif
	{ "listroms",                 NULL,   OPTION_COMMAND,    "list required roms for a driver" },
	{ "listsamples",              NULL,   OPTION_COMMAND,    "list optional samples for a driver" },
	{ "verifyroms",               NULL,   OPTION_COMMAND,    "report romsets that have problems" },
	{ "verifysamples",            NULL,   OPTION_COMMAND,    "report samplesets that have problems" },
	{ "romident",                 NULL,   OPTION_COMMAND,    "compare files with known MAME roms" },
	{ "isknown",                  NULL,   OPTION_COMMAND,    "compare files with known MAME roms (brief)" },

	// deprecated options
	{ "translucency",             "1",    OPTION_DEPRECATED, "(deprecated)" },
	{ "norotate",                 "0",    OPTION_DEPRECATED, "(deprecated)" },
#ifdef NEW_RENDER
	{ "cleanstretch",             "0",    OPTION_DEPRECATED, "(deprecated)" },
	{ "keepaspect",               "0",    OPTION_DEPRECATED, "(deprecated)" },
	{ "scanlines",                "0",    OPTION_DEPRECATED, "(deprecated)" },
	{ "matchrefresh",             "0",    OPTION_DEPRECATED, "(deprecated)" },
	{ "refresh",                  "0",    OPTION_DEPRECATED, "(deprecated)" },
	{ "gamma",                    "1.0",  OPTION_DEPRECATED, "(deprecated)" },
	{ "artwork_crop;artcrop",     "0",    OPTION_DEPRECATED, "(deprecated)" },
	{ "artwork_resolution;artres","0",    OPTION_DEPRECATED, "(deprecated)" },
#else
	{ "gamma",                    "1.0",  0,                 "color gamma" },
#endif

	{ NULL }
};


/*
 * Penalty string compare, the result _should_ be a measure on
 * how "close" two strings ressemble each other.
 * The implementation is way too simple, but it sort of suits the
 * purpose.
 * This used to be called fuzzy matching, but there's no randomness
 * involved and it is in fact a penalty method.
 */

int penalty_compare (const char *s, const char *l)
{
	int gaps = 0;
	int match = 0;
	int last = 1;

	for (; *s && *l; l++)
	{
		if (*s == *l)
			match = 1;
		else if (*s >= 'a' && *s <= 'z' && (*s - 'a') == (*l - 'A'))
			match = 1;
		else if (*s >= 'A' && *s <= 'Z' && (*s - 'A') == (*l - 'a'))
			match = 1;
		else
			match = 0;

		if (match)
			s++;

		if (match != last)
		{
			last = match;
			if (!match)
				gaps++;
		}
	}

	/* penalty if short string does not completely fit in */
	for (; *s; s++)
		gaps++;

	return gaps;
}

/*
 * We compare the game name given on the CLI against the long and
 * the short game names supported
 */
void show_approx_matches(void)
{
	struct { int penalty; int index; } topten[10];
	int i,j;
	int penalty; /* best fuzz factor so far */

	for (i = 0; i < 10; i++)
	{
		topten[i].penalty = 9999;
		topten[i].index = -1;
	}

	for (i = 0; (drivers[i] != 0); i++)
	{
		int tmp;

		if ((drivers[i]->flags & NOT_A_DRIVER) != 0)
			continue;

		penalty = penalty_compare (gamename, drivers[i]->description);
		tmp = penalty_compare (gamename, drivers[i]->name);
		if (tmp < penalty) penalty = tmp;

		/* eventually insert into table of approximate matches */
		for (j = 0; j < 10; j++)
		{
			if (penalty >= topten[j].penalty) break;
			if (j > 0)
			{
				topten[j-1].penalty = topten[j].penalty;
				topten[j-1].index = topten[j].index;
			}
			topten[j].index = i;
			topten[j].penalty = penalty;
		}
	}

	for (i = 9; i >= 0; i--)
	{
		if (topten[i].index != -1)
			fprintf (stderr, "%-10s%s\n", drivers[topten[i].index]->name, drivers[topten[i].index]->description);
	}
}


static void display_help(void)
{
#ifndef MESS
	printf("M.A.M.E. v%s - Multiple Arcade Machine Emulator\n"
		   "Copyright (C) 1997-2006 by Nicola Salmoria and the MAME Team\n\n",build_version);
	printf("%s\n", mame_disclaimer);
	printf("Usage:  MAME gamename [options]\n\n"
		   "        MAME -showusage    for a brief list of options\n"
		   "        MAME -showconfig   for a list of configuration options\n"
		   "        MAME -createconfig to create a mame.ini\n\n"
		   "For usage instructions, please consult the file windows.txt\n");
#else
	showmessinfo();
#endif
}


INLINE int is_directory_separator(char c)
{
	return (c == '\\' || c == '/' || c == ':');
}


static char *extract_base_name(const char *name, char *dest, int destsize)
{
	const char *start;
	int i;

	/* extract the base of the name */
	start = name + strlen(name);
	while (start > name && !is_directory_separator(start[-1]))
		start--;

	/* copy in the base name */
	for (i = 0; i < destsize; i++)
	{
		if (start[i] == 0 || start[i] == '.')
			break;
		else
			dest[i] = start[i];
	}

	/* NULL terminate */
	if (i < destsize)
		dest[i] = 0;
	else
		dest[destsize - 1] = 0;

	return dest;
}


static void execute_minimal_commands(void)
{
	// help?
	if (options_get_bool("help", FALSE))
	{
		display_help();
		exit(0);
	}

	// showusage?
	if (options_get_bool("showusage", FALSE))
	{
		printf("Usage: mame [game] [options]\n\nOptions:\n");
		options_output_help(stdout);
		exit(0);
	}

	// validate?
	if (options_get_bool("validate", FALSE))
	{
		extern int mame_validitychecks(int game);
		exit(mame_validitychecks(-1));
	}
}


static void execute_commands(const char *argv0)
{
	// frontend options
	static const struct
	{
		const char *option;
		int (*function)(FILE *output);
	} frontend_options[] =
	{
		{ "listxml",		frontend_listxml },
		{ "listfull",		frontend_listfull },
		{ "listsource",		frontend_listsource },
		{ "listclones",		frontend_listclones },
		{ "listcrc",		frontend_listcrc },
#ifdef MESS
		{ "listdevices",	frontend_listdevices },
#endif
		{ "listroms",		frontend_listroms },
		{ "listsamples",	frontend_listsamples },
		{ "verifyroms",		frontend_verifyroms },
		{ "verifysamples",	frontend_verifysamples },
		{ "romident",		frontend_romident },
		{ "isknown",		frontend_isknown  }
	};
	int i;

	// createconfig?
	if (options_get_bool("createconfig", FALSE))
	{
		char basename[128];
		FILE *file;

		// make the base name
		extract_base_name(argv0, basename, ARRAY_LENGTH(basename) - 5);
		strcat(basename, ".ini");
		file = fopen(basename, "w");

		// error if unable to create the file
		if (file == NULL)
		{
			fprintf(stderr, "Unable to create file %s\n", basename);
			exit(1);
		}

		// output the configuration and exit cleanly
		options_output_ini_file(file);
		fclose(file);
		exit(0);
	}

	// showconfig?
	if (options_get_bool("showconfig", FALSE))
	{
		options_output_ini_file(stdout);
		exit(0);
	}

	// frontend options?
	for (i = 0; i < ARRAY_LENGTH(frontend_options); i++)
		if (options_get_bool(frontend_options[i].option, FALSE))
		{
			int result;

			init_resource_tracking();
			cpuintrf_init();
			sndintrf_init();
			result = (*frontend_options[i].function)(stdout);
			exit_resource_tracking();
			exit(result);
		}
}


static int get_driver_index(const char *name)
{
	char basename[9];
	int drvnum;

	/* if no name, just bail */
	if (name == NULL)
		return -1;

	/* extract the base of the name */
	extract_base_name(name, basename, ARRAY_LENGTH(basename));

	/* scan the list for a match */
	for (drvnum = 0; drivers[drvnum] != NULL; drvnum++)
		if (strcmp(basename, drivers[drvnum]->name) == 0)
			return drvnum;

	/* return -1 if none */
	return -1;
}


static void parse_ini_file(const char *name)
{
	mame_file *file;

	/* don't parse if it has been disabled */
	if (!options_get_bool("readconfig", FALSE))
		return;

	/* reset the INI path so it gets re-expanded */
	set_pathlist(FILETYPE_INI, NULL);

	/* open the file; if we fail, that's ok */
	file = mame_fopen(name, NULL, FILETYPE_INI, 0);
	if (file == NULL)
		return;

	/* parse the file and close it */
	options_parse_ini_file(file);
	mame_fclose(file);
}


int cli_frontend_init(int argc, char **argv)
{
	machine_config drv;
	char buffer[128];
	int drvnum;

	// initialize the options manager
	options_free_entries();
	options_add_entries(fileio_opts);
	options_add_entries(config_opts);
	options_add_entries(input_opts);
	options_add_entries(video_opts);

	// parse the command line first; if we fail here, we're screwed
	if (options_parse_command_line(argc, argv))
		exit(1);

	// parse the minimal commmands
	execute_minimal_commands();

	// find out what game we might be referring to
	drvnum = get_driver_index(options_get_string("", FALSE));

	// now parse the core set of INI files
	parse_ini_file(CONFIGNAME ".ini");
	parse_ini_file(extract_base_name(argv[0], buffer, ARRAY_LENGTH(buffer)));
#ifdef MAME_DEBUG
	parse_ini_file("debug.ini");
#endif

	// if we have a valid game driver, parse game-specific INI files
	if (drvnum != -1)
	{
		const game_driver *driver = drivers[drvnum];
		const game_driver *parent = driver_get_clone(driver);
		const game_driver *gparent = (parent != NULL) ? driver_get_clone(parent) : NULL;

		// expand the machine driver to look at the info
		expand_machine_driver(driver->drv, &drv);

		// parse vector.ini for vector games
		if (drv.video_attributes & VIDEO_TYPE_VECTOR)
			parse_ini_file("vector.ini");

		// then parse sourcefile.ini
		parse_ini_file(extract_base_name(driver->source_file, buffer, ARRAY_LENGTH(buffer)));

		// then parent the grandparent, parent, and game-specific INIs
		if (gparent != NULL)
			parse_ini_file(gparent->name);
		if (parent != NULL)
			parse_ini_file(parent->name);
		parse_ini_file(driver->name);
	}

	// reparse the command line to ensure its options override all
	options_parse_command_line(argc, argv);

	// execute any commands specified
	execute_commands(argv[0]);

	// if no driver specified, display help
	if (options_get_string("", FALSE) == NULL)
	{
		display_help();
		exit(1);
	}

	// if we don't have a valid driver selected, we're hosed here
	if (drvnum == -1)
	{
		fprintf(stderr, "\n\"%s\" approximately matches the following\n"
				"supported " GAMESNOUN " (best match first):\n\n", gamename);
		show_approx_matches();
		exit(1);
	}

	// extract options information
	extract_options(drivers[drvnum], &drv);
	return drvnum;
}



static void setup_playback(const char *filename, const game_driver *driver)
{
	inp_header inp_header;

	// open the playback file
	options.playback = mame_fopen(filename, 0, FILETYPE_INPUTLOG, 0);
	assert_always(options.playback != NULL, "Failed to open file for playback");

	// read playback header
	mame_fread(options.playback, &inp_header, sizeof(inp_header));

	// if the first byte is not alphanumeric, it's an old INP file with no header
	if (!isalnum(inp_header.name[0]))
		mame_fseek(options.playback, 0, SEEK_SET);

	// else verify the header against the current game
	else if (strcmp(driver->name, inp_header.name) != 0)
		fatalerror("Input file is for " GAMENOUN " '%s', not for current " GAMENOUN " '%s'\n", inp_header.name, driver->name);

	// otherwise, print a message indicating what's happening
	else
		printf("Playing back previously recorded " GAMENOUN " %s\n", driver->name);
}


static void setup_record(const char *filename, const game_driver *driver)
{
	inp_header inp_header;

	// open the record file
	options.record = mame_fopen(filename, 0, FILETYPE_INPUTLOG, 1);
	assert_always(options.record != NULL, "Failed to open file for recording");

	// create a header
	memset(&inp_header, '\0', sizeof(inp_header));
	strcpy(inp_header.name, driver->name);
	mame_fwrite(options.record, &inp_header, sizeof(inp_header));
}


static void extract_options(const game_driver *driver, machine_config *drv)
{
	const char *stemp;

	// clear all core options
	memset(&options, 0, sizeof(options));

	// video options
#ifndef NEW_RENDER
	set_old_video_options(driver);
#else
	video_orientation = ROT0;

	// override if no rotation requested
	if (!options_get_bool("rotate", TRUE))
		video_orientation = orientation_reverse(driver->flags & ORIENTATION_MASK);

	// rotate right
	if (options_get_bool("ror", TRUE) || (options_get_bool("autoror", TRUE) && (driver->flags & ORIENTATION_SWAP_XY)))
		video_orientation = orientation_add(ROT90, video_orientation);

	// rotate left
	if (options_get_bool("rol", TRUE) || (options_get_bool("autorol", TRUE) && (driver->flags & ORIENTATION_SWAP_XY)))
		video_orientation = orientation_add(ROT270, video_orientation);

	// flip X/Y
	if (options_get_bool("flipx", TRUE))
		video_orientation ^= ORIENTATION_FLIP_X;
	if (options_get_bool("flipy", TRUE))
		video_orientation ^= ORIENTATION_FLIP_Y;
#endif

	// brightness
	options.brightness = options_get_float("brightness", TRUE);
	options.pause_bright = options_get_float("pause_brightness", TRUE);
#ifndef NEW_RENDER
	options.gamma = options_get_float("gamma", TRUE);
#else
	options.gamma = 1.0f;
#endif

	// vector options
	options.antialias = options_get_bool("antialias", TRUE);
	options.beam = (int)(options_get_float("beam", TRUE) * 65536.0f);
	options.vector_flicker = (int)(options_get_float("flicker", TRUE) * 2.55f);
	options.vector_intensity = options_get_float("intensity", TRUE);

	// sound options
	options.samplerate = options_get_bool("sound", TRUE) ? options_get_int("samplerate", TRUE) : 0;
	options.use_samples = options_get_bool("samples", TRUE);
	attenuation = options_get_int("volume", TRUE);
	audio_latency = options_get_int("audio_latency", TRUE);
	wavwrite = options_get_string("wavwrite", TRUE);

	// misc options
	options.bios = (char *)options_get_string("bios", TRUE);
	options.cheat = options_get_bool("cheat", TRUE);
	options.skip_gameinfo = options_get_bool("skip_gameinfo", TRUE);
	options.use_artwork = ARTWORK_USE_ALL;
	if (!options_get_bool("use_backdrops", TRUE)) options.use_artwork &= ~ARTWORK_USE_BACKDROPS;
	if (!options_get_bool("use_overlays", TRUE)) options.use_artwork &= ~ARTWORK_USE_OVERLAYS;
	if (!options_get_bool("use_bezels", TRUE)) options.use_artwork &= ~ARTWORK_USE_BEZELS;
	if (!options_get_bool("artwork", TRUE)) options.use_artwork = ARTWORK_USE_NONE;

	// save states and input recording
	stemp = options_get_string("playback", TRUE);
	if (stemp != NULL)
		setup_playback(stemp, driver);
	stemp = options_get_string("record", TRUE);
	if (stemp != NULL)
		setup_record(stemp, driver);
	options.savegame = options_get_string("state", TRUE);
	options.auto_save = options_get_bool("autosave", TRUE);

	// debugging options
	if (options_get_bool("log", TRUE))
	{
		options.logfile = mame_fopen(NULL, "error.log", FILETYPE_DEBUGLOG, TRUE);
		assert_always(options.logfile != NULL, "unable to open log file");
	}
	win_erroroslog = options_get_bool("oslog", TRUE);
	verbose = options_get_bool("verbose", TRUE);
#ifdef MAME_DEBUG
	options.mame_debug = options_get_bool("debug", TRUE);
#ifdef NEW_DEBUGGER
	stemp = options_get_string("debugscript", TRUE);
	if (stemp != NULL)
		debug_source_script(stemp);
#endif
#endif

	// need a decent default for debug width/height
	if (options.debug_width == 0)
		options.debug_width = 640;
	if (options.debug_height == 0)
		options.debug_height = 480;
	options.debug_depth = 8;
}


void cli_frontend_exit(void)
{
	if (gamename)
	{
		free(gamename);
		gamename = NULL;
	}
	if (gamepath)
	{
		free(gamepath);
		gamepath = NULL;
	}

	/* close open files */
	if (options.logfile)
	{
		mame_fclose(options.logfile);
		options.logfile = NULL;
	}

	if (options.playback)
	{
		mame_fclose(options.playback);
		options.playback = NULL;
	}
	if (options.record)
	{
		mame_fclose(options.record);
		options.record = NULL;
	}
	if (options.language_file)
	{
		mame_fclose(options.language_file);
		options.language_file = NULL;
	}

	options_free_entries();
}


#if 0
static int config_handle_arg(char *arg)
{
	int i;

	/* notice: for MESS game means system */
	if (got_gamename)
	{
		fprintf(stderr,"error: duplicate gamename: %s\n", arg);
		return -1;
	}

	if (!strcmp(arg, "random"))
	{
		/* special case: random driver */
		i = 0;
		while (drivers[i])
			i++;	/* count available drivers */

		srand(time(0));
		/* call rand() once to get away from the seed */
		rand();
		game_index = rand() % i;

		/* make sure that we prompt the driver name */
		prompt_driver_name = TRUE;
	}
	else
	{
		rompath_extra = win_dirname(arg);

		if (rompath_extra && !strlen(rompath_extra))
		{
			free (rompath_extra);
			rompath_extra = NULL;
		}

		gamename = arg;
		gamename = win_basename(gamename);
		gamename = win_strip_extension(gamename);
		gamepath = mame_strdup(arg);

		/* do we have a driver for this? */
		for (i = 0; drivers[i]; i++)
		{
			if (mame_stricmp(gamename, drivers[i]->name) == 0)
			{
				game_index = i;
				break;
			}
		}
	}

#ifdef MESS
	if (game_index >= 0)
		win_add_mess_device_options(rc, drivers[game_index]);
#endif /* MESS */

	got_gamename = TRUE;
	return 0;
}
#endif


//============================================================
//  win_basename
//============================================================

static char *win_basename(char *filename)
{
	char *c;

	// NULL begets NULL
	if (!filename)
		return NULL;

	// start at the end and return when we hit a slash or colon
	for (c = filename + strlen(filename) - 1; c >= filename; c--)
		if (*c == '\\' || *c == '/' || *c == ':')
			return c + 1;

	// otherwise, return the whole thing
	return filename;
}



//============================================================
//  win_dirname
//============================================================

static char *win_dirname(char *filename)
{
	char *dirname;
	char *c;

	// NULL begets NULL
	if (!filename)
		return NULL;

	// allocate space for it
	dirname = malloc(strlen(filename) + 1);
	if (!dirname)
	{
		fprintf(stderr, "error: malloc failed in win_dirname\n");
		return NULL;
	}

	// copy in the name
	strcpy(dirname, filename);

	// search backward for a slash or a colon
	for (c = dirname + strlen(dirname) - 1; c >= dirname; c--)
		if (*c == '\\' || *c == '/' || *c == ':')
		{
			// found it: NULL terminate and return
			*(c + 1) = 0;
			return dirname;
		}

	// otherwise, return an empty string
	dirname[0] = 0;
	return dirname;
}



//============================================================
//  win_strip_extension
//============================================================

static char *win_strip_extension(char *filename)
{
	char *newname;
	char *c;

	// NULL begets NULL
	if (!filename)
		return NULL;

	// allocate space for it
	newname = malloc(strlen(filename) + 1);
	if (!newname)
	{
		fprintf(stderr, "error: malloc failed in win_strip_extension\n");
		return NULL;
	}

	// copy in the name
	strcpy(newname, filename);

	// search backward for a period, failing if we hit a slash or a colon
	for (c = newname + strlen(newname) - 1; c >= newname; c--)
	{
		// if we hit a period, NULL terminate and break
		if (*c == '.')
		{
			*c = 0;
			break;
		}

		// if we hit a slash or colon just stop
		if (*c == '\\' || *c == '/' || *c == ':')
			break;
	}

	return newname;
}



#ifndef NEW_RENDER
static void set_old_video_options(const game_driver *driver)
{
	/* first start with the game's built in orientation */
	int orientation = driver->flags & ORIENTATION_MASK;

	options.ui_orientation = orientation;

	if (options.ui_orientation & ORIENTATION_SWAP_XY)
	{
		/* if only one of the components is inverted, switch them */
		if ((options.ui_orientation & ROT180) == ORIENTATION_FLIP_X ||
				(options.ui_orientation & ROT180) == ORIENTATION_FLIP_Y)
			options.ui_orientation ^= ROT180;
	}

	/* override if no rotation requested */
	if (!options_get_bool("rotate", TRUE))
		orientation = options.ui_orientation = ROT0;

	/* rotate right */
	if (options_get_bool("ror", TRUE))
	{
		/* if only one of the components is inverted, switch them */
		if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
				(orientation & ROT180) == ORIENTATION_FLIP_Y)
			orientation ^= ROT180;

		orientation ^= ROT90;
	}

	/* rotate left */
	if (options_get_bool("rol", TRUE))
	{
		/* if only one of the components is inverted, switch them */
		if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
				(orientation & ROT180) == ORIENTATION_FLIP_Y)
			orientation ^= ROT180;

		orientation ^= ROT270;
	}

	/* auto-rotate right (e.g. for rotating lcds), based on original orientation */
	if (options_get_bool("autoror", TRUE) && (driver->flags & ORIENTATION_SWAP_XY) )
	{
		/* if only one of the components is inverted, switch them */
		if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
				(orientation & ROT180) == ORIENTATION_FLIP_Y)
			orientation ^= ROT180;

		orientation ^= ROT90;
	}

	/* auto-rotate left (e.g. for rotating lcds), based on original orientation */
	if (options_get_bool("autorol", TRUE) && (driver->flags & ORIENTATION_SWAP_XY) )
	{
		/* if only one of the components is inverted, switch them */
		if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
				(orientation & ROT180) == ORIENTATION_FLIP_Y)
			orientation ^= ROT180;

		orientation ^= ROT270;
	}

	/* flip X/Y */
	if (options_get_bool("flipx", TRUE))
		orientation ^= ORIENTATION_FLIP_X;
	if (options_get_bool("flipy", TRUE))
		orientation ^= ORIENTATION_FLIP_Y;

	blit_flipx = ((orientation & ORIENTATION_FLIP_X) != 0);
	blit_flipy = ((orientation & ORIENTATION_FLIP_Y) != 0);
	blit_swapxy = ((orientation & ORIENTATION_SWAP_XY) != 0);

	if( options.vector_width == 0 && options.vector_height == 0 )
	{
		options.vector_width = 640;
		options.vector_height = 480;
	}
	if( blit_swapxy )
	{
		int temp;
		temp = options.vector_width;
		options.vector_width = options.vector_height;
		options.vector_height = temp;
	}
}
#endif
