
/*
 * Configuration routines.
 *
 * 20010424 BW uses Hans de Goede's rc subsystem
 *
 * TODO:
 * - norotate? funny, leads to option -nonorotate ...
 *   should possibly be renamed
 * - remove #ifdef MESS and provide generic hooks for projects using the MAME
 *   core
 * - suggestion for new core options:
 *   gamma (is already osd_)
 *   sound (enable/disable sound)
 *   volume
 *   resolution: introduce options.width, options.height?
 * - rename	options.use_emulated_ym3812 to options_use_real_ym3812;
 * - repair playback when no gamename given
 * - reintroduce override_rompath
 * - consolidate namespace "long_option" vs. "longoption" vs. "longopt"
 *
 * other suggestions
 * - switchres --> switch_resolution, swres
 * - switchbpp --> switch_bpp, swbpp
 */

#include <ctype.h>
#include <time.h>
#include "driver.h"
#include "rc.h"
#include "misc.h"


#ifdef MESS
extern char *crcdir;
static char crcfilename[256] = "";
const char *crcfile = crcfilename;
extern char *pcrcdir;
static char pcrcfilename[256] = "";
const char *pcrcfile = pcrcfilename;
#endif


extern struct rc_option frontend_opts[];

/* FIXME: the following parts should be moved to the files they belong to */

/* START: probably everything for fileio.c, datafile.c and cheat.c */
/* FIXME: needs to be sorted out much more, leave here for the moment */
static const char *rompath;
static const char *samplepath;
extern const char *cfgdir;
extern const char *nvdir;
extern const char *hidir;
extern const char *inpdir;
extern const char *cheatdir;
extern const char *stadir;
extern const char *memcarddir;
extern const char *artworkdir;
extern const char *screenshotdir;
/* from datafile.c */
extern const char *history_filename;
extern const char *mameinfo_filename;
/* from cheat.c */
extern char *cheatfile;

void decompose_rom_sample_path (const char *_rompath, const char *_samplepath);

static struct rc_option fileio_opts[] =
{
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ "Windows path and directory options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "rompath", "rp", rc_string, &rompath, "roms", 0, 0, NULL, "path to romsets" },
	{ "samplepath", "sp", rc_string, &samplepath, "samples", 0, 0, NULL, "path to samplesets" },
	{ "cfg_directory", NULL, rc_string, &cfgdir, "cfg", 0, 0, NULL, "directory to save configurations" },
	{ "nvram_directory", NULL, rc_string, &nvdir, "nvram", 0, 0, NULL, "directory to save nvram contents" },
	{ "memcard_directory", NULL, rc_string, &memcarddir, "memcard", 0, 0, NULL, "directory to save memory card contents" },
	{ "input_directory", NULL, rc_string, &inpdir, "inp", 0, 0, NULL, "directory to save input device logs" },
	{ "hiscore_directory", NULL, rc_string, &hidir, "hi", 0, 0, NULL, "directory to save hiscores" },
	{ "state_directory", NULL, rc_string, &stadir, "sta", 0, 0, NULL, "directory to save states" },
	{ "artwork_directory", NULL, rc_string, &artworkdir, "artwork", 0, 0, NULL, "directory for Artwork (Overlays etc.)" },
	{ "snapshot_directory", NULL, rc_string, &screenshotdir, "snap", 0, 0, NULL, "directory for screenshots (.png format)" },
//	{ "cheat_directory", NULL, rc_string, &cheatdir, "cheat", 0, 0, NULL, "directory for cheatfiles" },
	{ "cheat_file", NULL, rc_string, &cheatfile, "cheat.dat", 0, 0, NULL, "cheat filename" },
	{ "history_file", NULL, rc_string, &history_filename, "history.dat", 0, 0, NULL, NULL },
	{ "mameinfo_file", NULL, rc_string, &mameinfo_filename, "mameinfo.dat", 0, 0, NULL, NULL },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};
/* END: move into fileio.c */


extern struct rc_option input_opts[];
extern struct rc_option sound_opts[];
extern struct rc_option video_opts[];


/* <<<----------- snip ----------->>> */
/* config.c really starts here */

extern int frontend_help(char *gamename);
static int config_handle_arg(char *arg);

int errorlog;
static int showconfig;
static int showusage;
static int readconfig;
static int createconfig;
extern int verbose;

static struct rc_struct *rc;

/* fix me - need to have the core call osd_set_gamma with this value */
/* instead of relying on the name of an osd variable */
extern float gamma_correct;

/* fix me - need to have the core call osd_set_mastervolume with this value */
/* instead of relying on the name of an osd variable */
extern int attenuation;

static char *debugres;
static char *playbackname;
static char *recordname;
static char *gamename;

static float f_beam;
static float f_flicker;

static int enable_sound = 1;

static int video_set_beam(struct rc_option *option, const char *arg, int priority)
{
	options.beam = (int)(f_beam * 0x00010000);
	if (options.beam < 0x00010000)
		options.beam = 0x00010000;
	if (options.beam > 0x00100000)
		options.beam = 0x00100000;
	option->priority = priority;
	return 0;
}

static int video_set_flicker(struct rc_option *option, const char *arg, int priority)
{
	options.vector_flicker = (int)(f_flicker * 2.55);
	if (options.vector_flicker < 0)
		options.vector_flicker = 0;
	if (options.vector_flicker > 255)
		options.vector_flicker = 255;
	option->priority = priority;
	return 0;
}

static int video_set_debugres(struct rc_option *option, const char *arg, int priority)
{
	if (!strcmp(arg, "auto"))
	{
		options.debug_width = options.debug_height = 0;
	}
	else if(sscanf(arg, "%dx%d", &options.debug_width, &options.debug_height) != 2)
	{
		options.debug_width = options.debug_height = 0;
		fprintf(stderr, "error: invalid value for debugres: %s\n", arg);
		return -1;
	}
	option->priority = priority;
	return 0;
}

static int video_verify_bpp(struct rc_option *option, const char *arg, int priority)
{
	if ((options.color_depth != 0) &&
		(options.color_depth != 8) &&
		(options.color_depth != 15) &&
		(options.color_depth != 16) &&
		(options.color_depth != 32))
	{
		options.color_depth = 0;
		fprintf(stderr, "error: invalid value for bpp: %s\n", arg);
		return -1;
	}
	option->priority = priority;
	return 0;
}

/* struct definitions */
static struct rc_option opts[] = {
	/* name, shortname, type, dest, deflt, min, max, func, help */
	{ NULL, NULL, rc_link, frontend_opts, NULL, 0, 0, NULL, NULL },
	{ NULL, NULL, rc_link, fileio_opts, NULL, 0, 0, NULL, NULL },
	{ NULL, NULL, rc_link, video_opts, NULL, 0,	0, NULL, NULL },
	{ NULL, NULL, rc_link, sound_opts, NULL, 0,	0, NULL, NULL },
	{ NULL, NULL, rc_link, input_opts, NULL, 0,	0, NULL, NULL },

	/* options supported by the mame core */
	/* video */
	{ "Mame CORE video options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "bpp", NULL, rc_int, &options.color_depth, "0",	0, 0, video_verify_bpp, "specify the colordepth the core should render in bits per pixel (bpp), one of: auto(0), 8, 16, 32" },
	{ "norotate", NULL, rc_bool , &options.norotate, "0", 0, 0, NULL, "do not apply rotation" },
	{ "ror", NULL, rc_bool, &options.ror, "0", 0, 0, NULL, "rotate screen clockwise" },
	{ "rol", NULL, rc_bool, &options.rol, "0", 0, 0, NULL, "rotate screen anti-clockwise" },
	{ "flipx", NULL, rc_bool, &options.flipx, "0", 0, 0, NULL, "flip screen upside-down" },
	{ "flipy", NULL, rc_bool, &options.flipy, "0", 0, 0, NULL, "flip screen left-right" },
	{ "debug_resolution", "dr", rc_string, &debugres, "auto", 0, 0, video_set_debugres, "set resolution for debugger window" },
	/* make it options.gamma_correction? */
	{ "gamma", NULL, rc_float, &gamma_correct , "1.0", 0.5, 2.0, NULL, "gamma correction"},

	/* vector */
	{ "Mame CORE vector game options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "antialias", "aa", rc_bool, &options.antialias, "1", 0, 0, NULL, "draw antialiased vectors" },
	{ "translucency", "tl", rc_bool, &options.translucency, "1", 0, 0, NULL, "draw translucent vectors" },
	{ "beam", NULL, rc_float, &f_beam, "1.0", 1.0, 16.0, video_set_beam, "set beam width in vector games" },
	{ "flicker", NULL, rc_float, &f_flicker, "0.0", 0.0, 100.0, video_set_flicker, "set flickering in vector games" },

	/* sound */
	{ "Mame CORE sound options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "samplerate", "sr", rc_int, &options.samplerate, "44100", 5000, 50000, NULL, "set samplerate" },
	{ "samples", NULL, rc_bool, &options.use_samples, "1", 0, 0, NULL, "use samples" },
	{ "resamplefilter", NULL, rc_bool, &options.use_filter, "1", 0, 0, NULL, "resample if samplerate does not match" },
	{ "sound", NULL, rc_bool, &enable_sound, "1", 0, 0, NULL, "enable/disable sound and sound CPUs" },
	{ "volume", "vol", rc_int, &attenuation, "0", -32, 0, NULL, "volume (range [-32,0])" },

	/* misc */
	{ "Mame CORE misc options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "artwork", "art", rc_bool, &options.use_artwork, "0", 0, 0, NULL, "use additional game artwork" },
	{ "cheat", "c", rc_bool, &options.cheat, "0", 0, 0, NULL, "enable/disable cheat subsystem" },
	{ "debug", "d", rc_bool, &options.mame_debug, "0", 0, 0, NULL, "enable/disable debugger (only if available)" },
	{ "playback", "pb", rc_string, &playbackname, NULL, 0, 0, NULL, "playback an input file" },
	{ "record", "rec", rc_string, &recordname, NULL, 0, 0, NULL, "record an input file" },
	{ "log", NULL, rc_bool, &errorlog, "0", 0, 0, NULL, "generate error.log" },

	/* config options */
	{ "Configuration options", NULL, rc_seperator, NULL, NULL, 0, 0, NULL, NULL },
	{ "createconfig", "cc", rc_set_int, &createconfig, NULL, 1, 0, NULL, "create the default configuration file" },
	{ "showconfig",	"sc", rc_set_int, &showconfig, NULL, 1, 0, NULL, "display running parameters in rc style" },
	{ "showusage", "su", rc_set_int, &showusage, NULL, 1, 0, NULL, "show this help" },
	{ "readconfig",	"rc", rc_bool, &readconfig, "1", 0, 0, NULL, "enable/disable loading of configfiles" },
	{ "verbose", "v", rc_bool, &verbose, "0", 0, 0, NULL, "display additional diagnostic information" },
	{ NULL,	NULL, rc_end, NULL, NULL, 0, 0,	NULL, NULL }
};

/* fuzzy string compare, compare short string against long string        */
/* e.g. astdel == "Asteroids Deluxe". The return code is the fuzz index, */
/* we simply count the gaps between maching chars.                       */
int fuzzycmp (const char *s, const char *l)
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

void show_fuzzy_matches(void)
{
	struct { int fuzz; int index; } topten[10];
	int i,j;
	int fuzz; /* best fuzz factor so far */

	for (i = 0; i < 10; i++)
	{
		topten[i].fuzz = 9999;
		topten[i].index = -1;
	}

	for (i = 0; (drivers[i] != 0); i++)
	{
		fuzz = fuzzycmp(gamename, drivers[i]->description);
		for (j = 0; j < 10; j++)
		{
			if (fuzz >= topten[j].fuzz) break;
			if (j > 0)
			{
				topten[j-1].fuzz = topten[j].fuzz;
				topten[j-1].index = topten[j].index;
			}
			topten[j].index = i;
			topten[j].fuzz = fuzz;
		}
	}

	for (i = 0; i < 10; i++)
	{
		if (topten[i].index != -1)
			fprintf (stderr, "%-10s%s\n", drivers[topten[i].index]->name, drivers[topten[i].index]->description);
	}
}

/*
 * gamedrv  = NULL --> parse named configfile
 * gamedrv != NULL --> parse gamename.ini and all parent.ini's (recursively)
 * return 0 --> no problem
 * return 1 --> something went wrong
 */
int parse_config (const char* filename, const struct GameDriver *gamedrv)
{
	FILE *f;
	char buffer[128];
	int retval = 0;

	if (gamedrv)
	{
		if (gamedrv->clone_of && strlen(gamedrv->clone_of->name))
		{
			retval = parse_config (NULL, gamedrv->clone_of);
			if (retval)
				return retval;
		}
		sprintf(buffer, "%s.ini", gamedrv->name);
	}
	else
	{
		sprintf(buffer, "%s", filename);
	}

	if (verbose)
		fprintf(stderr, "trying to parse %s\n", buffer);

	f = fopen (buffer, "r");
	if (f)
	{
		if(rc_read(rc, f, buffer, 1, 1))
		{
			if (verbose)
				fprintf (stderr, "problem parsing %s\n", buffer);
			retval = 1;
		}
	}

	if (f)
		fclose (f);

	return retval;
}

int parse_config_and_cmdline (int argc, char **argv)
{
	int game_index;
	int i;

	gamename = NULL;

	/* clear all core options */
	memset(&options,0,sizeof(options));

	/* directly define these */
	options.use_emulated_ym3812 = 1;

	/* create the rc object */
	if (!(rc = rc_create()))
	{
		fprintf (stderr, "error on rc creation\n");
		exit(1);
	}

	if (rc_register(rc, opts))
	{
		fprintf (stderr, "error on registering opts\n");
		exit(1);
	}

	/* parse the commandline */
	if (rc_parse_commandline(rc, argc, argv, 2, config_handle_arg))
	{
		fprintf (stderr, "error while parsing cmdline\n");
		exit(1);
	}

	/* parse the global configfile */
	if (readconfig)
		if (parse_config ("mame.ini", NULL))
			exit(1);

	if (createconfig)
	{
		rc_save(rc, "mame.ini", 0);
		exit(0);
	}

	if (showconfig)
	{
		rc_write(rc, stdout, " Mame running parameters");
		exit(0);
	}

	if (showusage)
	{
		fprintf(stdout, "Usage: mame [game] [options]\n" "Options:\n");

		/* actual help message */
		rc_print_help(rc, stdout);
		exit(0);
	}

	/* FIXME: split this up into two functions and use rc-callbacks" */
	decompose_rom_sample_path (rompath, samplepath);

	/* check for frontend options, horrible 1234 hack */
	if (frontend_help(gamename) != 1234)
		exit(0);

	/* we're pretty sure that the users wants to play a game now */
	game_index = -1;

	/* handle playback which is not available in mame.cfg */
    if (playbackname != NULL)
        options.playback = osd_fopen(playbackname,0,OSD_FILETYPE_INPUTLOG,0);

    /* check for game name embedded in .inp header */
    if (options.playback)
    {
        INP_HEADER inp_header;

        /* read playback header */
        osd_fread(options.playback, &inp_header, sizeof(INP_HEADER));

        if (!isalnum(inp_header.name[0])) /* If first byte is not alpha-numeric */
            osd_fseek(options.playback, 0, SEEK_SET); /* old .inp file - no header */
        else
        {
            for (i = 0; (drivers[i] != 0); i++) /* find game and play it */
			{
                if (strcmp(drivers[i]->name, inp_header.name) == 0)
                {
                    game_index = i;
                    printf("Playing back previously recorded game %s (%s) [press return]\n",
                        drivers[game_index]->name,drivers[game_index]->description);
                    getchar();
                    break;
                }
            }
        }
    }

	/* if not given by .inp file yet */
	if (game_index == -1)
	{
		/* do we have a driver for this? */
		for (i = 0; drivers[i]; i++)
			if (strcasecmp(gamename,drivers[i]->name) == 0)
			{
				game_index = i;
				break;
			}
	}

	/* random ?*/
#ifdef MAME_DEBUG
	if (game_index == -1)
	{
		/* pick a random game */
		if (strcmp(gamename,"random") == 0)
		{
			i = 0;
			while (drivers[i]) i++;	/* count available drivers */

			srand(clock());
			game_index = rand() % i;

			fprintf(stderr, "Running %s (%s) [press return]\n",drivers[game_index]->name,drivers[game_index]->description);
			getchar();
		}
	}
#endif

	/* we give up. print a few fuzzy matches */
	if (game_index == -1)
	{
		fprintf(stderr, "game \"%s\" not supported\n", gamename);
		fprintf(stderr, "the name fuzzy matches the following supported games:\n");
		show_fuzzy_matches();
		exit(1);
	}

	/* if this is a vector game, parse vector.ini first */
	if (drivers[game_index]->drv->video_attributes & VIDEO_TYPE_VECTOR)
		if (parse_config ("vector.ini", NULL))
			exit(1);

	/* ok, got a gamename. now load gamename.ini */
	/* this possibly checks for clonename.ini recursively! */

	if (readconfig)
		if (parse_config (NULL, drivers[game_index]))
			exit(1);

	#ifdef MESS
	/* This function has been added to MESS.C as load_image() */
	/* FIXME: broken, sorry */
//	load_image(argc, argv, j, game_index);
	#endif

	/* handle record which is not available in mame.cfg */
	if (recordname)
	{
		options.record = osd_fopen(recordname,0,OSD_FILETYPE_INPUTLOG,1);
	}

    if (options.record)
    {
        INP_HEADER inp_header;

        memset(&inp_header, '\0', sizeof(INP_HEADER));
        strcpy(inp_header.name, drivers[game_index]->name);
        /* MAME32 stores the MAME version numbers at bytes 9 - 11
         * MAME DOS keeps this information in a string, the
         * Windows code defines them in the Makefile.
         */
        /*
        inp_header.version[0] = 0;
        inp_header.version[1] = VERSION;
        inp_header.version[2] = BETA_VERSION;
        */
        osd_fwrite(options.record, &inp_header, sizeof(INP_HEADER));
    }

	#ifdef MESS
	/* Build the CRC database filename */
	sprintf(crcfilename, "%s/%s.crc", crcdir, drivers[game_index]->name);
	if (drivers[game_index]->clone_of->name)
		sprintf (pcrcfilename, "%s/%s.crc", crcdir, drivers[game_index]->clone_of->name);
	else
		pcrcfilename[0] = 0;
    #endif

/* FIXME */
/*	logerror("cheatfile = %s - cheatdir = %s\n",cheatfile,cheatdir); */

	/* need a decent default for debug width/height */
	if (options.debug_width == 0)
		options.debug_width = 640;
	if (options.debug_height == 0)
		options.debug_height = 480;

	/* no sound is indicated by a 0 samplerate */
	if (!enable_sound)
		options.samplerate = 0;

	return game_index;
}


static int config_handle_arg(char *arg)
{
	static int got_gamename = 0;

	if (!got_gamename) /* notice: for MESS game means system */
	{
		gamename     = arg;
		got_gamename = 1;
	}
	else
#ifdef MESS
	{
		if (options.image_count >= MAX_IMAGES)
		{
			fprintf(stderr, "error: too many image names specified!\n");
			return -1;
		}
		options.image_files[options.image_count].type = iodevice_type;
		options.image_files[options.image_count].name = arg;
		options.image_count++;
	}
#else
	{
		fprintf(stderr,"error: duplicate gamename: %s\n", arg);
		return -1;
	}
#endif

	return 0;
}



