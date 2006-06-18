//============================================================
//
//  config.c - Win32 configuration routines
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// standard C headers
#include <stdarg.h>
#include <ctype.h>
#include <time.h>

// MAME headers
#include "osdepend.h"
#include "driver.h"
#include "options.h"

#include "winmain.h"
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



//============================================================
//  EXTERNALS
//============================================================

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

#ifdef MESS
int frontend_listdevices(FILE *output);
#endif /* MESS */

void set_pathlist(int file_type, const char *new_rawpath);

extern const options_entry fileio_opts[];
extern const options_entry video_opts[];
extern const options_entry input_opts[];
#ifdef MESS
extern const options_entry mess_opts[];
#endif



//============================================================
//  GLOBALS
//============================================================

int win_erroroslog;

// fix me - need to have the core call osd_set_mastervolume with this value
// instead of relying on the name of an osd variable
extern int attenuation;
extern int audio_latency;
extern const char *wavwrite;



//============================================================
//  PROTOTYPES
//============================================================

static void extract_options(const game_driver *driver, machine_config *drv);
static void parse_ini_file(const char *name);
static void execute_simple_commands(void);
static void execute_commands(const char *argv0);
static void display_help(void);
static void extract_options(const game_driver *driver, machine_config *drv);
static void setup_playback(const char *filename, const game_driver *driver);
static void setup_record(const char *filename, const game_driver *driver);
static char *extract_base_name(const char *name, char *dest, int destsize);
static char *extract_path(const char *name, char *dest, int destsize);

#ifndef NEW_RENDER
static void set_old_video_options(const game_driver *driver);
#endif



//============================================================
//  OPTIONS
//============================================================

// struct definitions
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


#ifdef MESS
#include "configms.h"
#endif



//============================================================
//  INLINES
//============================================================

INLINE int is_directory_separator(char c)
{
	return (c == '\\' || c == '/' || c == ':');
}



//============================================================
//  cli_frontend_init
//============================================================

int cli_frontend_init(int argc, char **argv)
{
	const char *gamename;
	machine_config drv;
	char basename[20];
	char buffer[512];
	int drvnum = -1;

	// initialize the options manager
	options_free_entries();
	options_add_entries(fileio_opts);
	options_add_entries(config_opts);
	options_add_entries(input_opts);
	options_add_entries(video_opts);
#ifdef MESS
	options_add_entries(mess_opts);
	options_set_option_callback("", win_mess_driver_name_callback);
#endif // MESS

	// parse the command line first; if we fail here, we're screwed
	if (options_parse_command_line(argc, argv))
		exit(1);

	// parse the simple commmands before we go any further
	execute_simple_commands();

	// find out what game we might be referring to
	gamename = options_get_string("", FALSE);
	if (gamename != NULL)
		drvnum = driver_get_index(extract_base_name(gamename, basename, ARRAY_LENGTH(basename)));

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
	// note that we re-fetch the gamename here as it will get overridden
	options_parse_command_line(argc, argv);
	gamename = options_get_string("", FALSE);

	// execute any commands specified
	execute_commands(argv[0]);

	// if no driver specified, display help
	if (gamename == NULL)
	{
		display_help();
		exit(1);
	}

	// if we don't have a valid driver selected, offer some suggestions
	if (drvnum == -1)
	{
		int matches[10];
		fprintf(stderr, "\n\"%s\" approximately matches the following\n"
				"supported " GAMESNOUN " (best match first):\n\n", basename);
		driver_get_approx_matches(basename, ARRAY_LENGTH(matches), matches);
		for (drvnum = 0; drvnum < ARRAY_LENGTH(matches); drvnum++)
			if (matches[drvnum] != -1)
				fprintf(stderr, "%-10s%s\n", drivers[matches[drvnum]]->name, drivers[matches[drvnum]]->description);
		exit(1);
	}

	// extract the directory name
	extract_path(gamename, buffer, ARRAY_LENGTH(buffer));
	if (buffer[0] != 0)
	{
		// okay, we got one; prepend it to the rompath
		const char *rompath = options_get_string("rompath", FALSE);
		if (rompath == NULL)
			options_set_string("rompath", buffer);
		else
		{
			char *newpath = malloc_or_die(strlen(rompath) + strlen(buffer) + 2);
			sprintf(newpath, "%s;%s", buffer, rompath);
			options_set_string("rompath", newpath);
			free(newpath);
		}
	}

	// extract options information
	extract_options(drivers[drvnum], &drv);
	return drvnum;
}



//============================================================
//  cli_frontend_exit
//============================================================

void cli_frontend_exit(void)
{
	// close open files
	if (options.logfile != NULL)
		mame_fclose(options.logfile);
	options.logfile = NULL;

	if (options.playback != NULL)
		mame_fclose(options.playback);
	options.playback = NULL;

	if (options.record != NULL)
		mame_fclose(options.record);
	options.record = NULL;

	if (options.language_file != NULL)
		mame_fclose(options.language_file);
	options.language_file = NULL;

	// free the options that we added previously
	options_free_entries();
}



//============================================================
//  parse_ini_file
//============================================================

static void parse_ini_file(const char *name)
{
	mame_file *file;

	// don't parse if it has been disabled
	if (!options_get_bool("readconfig", FALSE))
		return;

	// open the file; if we fail, that's ok
	file = mame_fopen(name, NULL, FILETYPE_INI, 0);
	if (file == NULL)
		return;

	// parse the file and close it
	options_parse_ini_file(file);
	mame_fclose(file);

	// reset the INI path so it gets re-expanded next time
	set_pathlist(FILETYPE_INI, NULL);
}



//============================================================
//  execute_simple_commands
//============================================================

static void execute_simple_commands(void)
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



//============================================================
//  execute_commands
//============================================================

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



//============================================================
//  display_help
//============================================================

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



//============================================================
//  extract_options
//============================================================

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

#ifdef MESS
	win_mess_extract_options();
#endif /* MESS */

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
{
	extern int verbose;
	verbose = options_get_bool("verbose", TRUE);
}
#ifdef MAME_DEBUG
	options.mame_debug = options_get_bool("debug", TRUE);
#ifdef NEW_DEBUGGER
	stemp = options_get_string("debugscript", TRUE);
	if (stemp != NULL)
		debug_source_script(stemp);
#endif
#endif

{
	extern const char *cheatfile;
	cheatfile = options_get_string("cheat_file", TRUE);
}

	// need a decent default for debug width/height
	if (options.debug_width == 0)
		options.debug_width = 640;
	if (options.debug_height == 0)
		options.debug_height = 480;
	options.debug_depth = 8;

	// thread priority
	if (!options.mame_debug)
	{
		int priority = options_get_int("priority", TRUE);
		if (priority < -15 || priority > 1)
		{
			fprintf(stderr, "Invalid priority value %d; reverting to 0\n", priority);
			priority = 0;
		}
		SetThreadPriority(GetCurrentThread(), priority);
	}
}



//============================================================
//  setup_playback
//============================================================

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



//============================================================
//  setup_record
//============================================================

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



//============================================================
//  extract_base_name
//============================================================

static char *extract_base_name(const char *name, char *dest, int destsize)
{
	const char *start;
	int i;

	// extract the base of the name
	start = name + strlen(name);
	while (start > name && !is_directory_separator(start[-1]))
		start--;

	// copy in the base name
	for (i = 0; i < destsize; i++)
	{
		if (start[i] == 0 || start[i] == '.')
			break;
		else
			dest[i] = start[i];
	}

	// NULL terminate
	if (i < destsize)
		dest[i] = 0;
	else
		dest[destsize - 1] = 0;

	return dest;
}



//============================================================
//  extract_path
//============================================================

static char *extract_path(const char *name, char *dest, int destsize)
{
	const char *start;

	// find the base of the name
	start = name + strlen(name);
	while (start > name && !is_directory_separator(start[-1]))
		start--;

	// handle the null path case
	if (start == name)
		*dest = 0;

	// else just copy the path part
	else
	{
		int bytes = start - 1 - name;
		bytes = MIN(destsize - 1, bytes);
		memcpy(dest, name, bytes);
		dest[bytes] = 0;
	}
	return dest;
}





#ifndef NEW_RENDER
static void set_old_video_options(const game_driver *driver)
{
	// first start with the game's built in orientation
	int orientation = driver->flags & ORIENTATION_MASK;

	options.ui_orientation = orientation;

	if (options.ui_orientation & ORIENTATION_SWAP_XY)
	{
		// if only one of the components is inverted, switch them
		if ((options.ui_orientation & ROT180) == ORIENTATION_FLIP_X ||
				(options.ui_orientation & ROT180) == ORIENTATION_FLIP_Y)
			options.ui_orientation ^= ROT180;
	}

	// override if no rotation requested
	if (!options_get_bool("rotate", TRUE))
		orientation = options.ui_orientation = ROT0;

	// rotate right
	if (options_get_bool("ror", TRUE))
	{
		// if only one of the components is inverted, switch them
		if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
				(orientation & ROT180) == ORIENTATION_FLIP_Y)
			orientation ^= ROT180;

		orientation ^= ROT90;
	}

	// rotate left
	if (options_get_bool("rol", TRUE))
	{
		// if only one of the components is inverted, switch them
		if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
				(orientation & ROT180) == ORIENTATION_FLIP_Y)
			orientation ^= ROT180;

		orientation ^= ROT270;
	}

	// auto-rotate right (e.g. for rotating lcds), based on original orientation
	if (options_get_bool("autoror", TRUE) && (driver->flags & ORIENTATION_SWAP_XY) )
	{
		// if only one of the components is inverted, switch them
		if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
				(orientation & ROT180) == ORIENTATION_FLIP_Y)
			orientation ^= ROT180;

		orientation ^= ROT90;
	}

	// auto-rotate left (e.g. for rotating lcds), based on original orientation
	if (options_get_bool("autorol", TRUE) && (driver->flags & ORIENTATION_SWAP_XY) )
	{
		// if only one of the components is inverted, switch them
		if ((orientation & ROT180) == ORIENTATION_FLIP_X ||
				(orientation & ROT180) == ORIENTATION_FLIP_Y)
			orientation ^= ROT180;

		orientation ^= ROT270;
	}

	// flip X/Y
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
