/*
 * Configuration routines.
 *
 * 19971219 support for mame.cfg by Valerio Verrando
 * 19980402 moved out of msdos.c (N.S.), generalized routines (BW)
 * 19980917 added a "-cheatfile" option (misc) in MAME.CFG      JCK
 */

#define __INLINE__ static __inline__    /* keep allegro.h happy */
#include <allegro.h>
#undef __INLINE__
#include "driver.h"
#include <ctype.h>
/* types of monitors supported */
#include "monitors.h"
/* from main() */
extern int ignorecfg;

/* from video.c */
extern int frameskip,autoframeskip;
extern int scanlines, use_tweaked, video_sync, wait_vsync, use_triplebuf;
extern int stretch;
extern int vgafreq, always_synced, color_depth, skiplines, skipcolumns;
extern float osd_gamma_correction;
extern int gfx_mode, gfx_width, gfx_height;

extern int monitor_type;

extern unsigned char tw224x288ns_h, tw224x288ns_v, tw224x288sc_h, tw224x288sc_v;
extern unsigned char tw256x256ns_h, tw256x256ns_v, tw256x256sc_h, tw256x256sc_v;
extern unsigned char tw256x256ns_hor_h, tw256x256ns_hor_v, tw256x256sc_hor_h, tw256x256sc_hor_v;
extern unsigned char tw256x256ns_57_h, tw256x256ns_57_v, tw256x256sc_57_h, tw256x256sc_57_v;
extern unsigned char tw256x256ns_h57_h, tw256x256ns_h57_v, tw256x256sc_h57_h, tw256x256sc_h57_v;
extern unsigned char tw288x224ns_h, tw288x224ns_v, tw288x224sc_h, tw288x224sc_v;
extern unsigned char tw320x240ns_h, tw320x240ns_v, tw320x240sc_h, tw320x240sc_v;
extern unsigned char tw336x240ns_h, tw336x240ns_v, tw336x240sc_h, tw336x240sc_v;
extern unsigned char tw384x240ns_h, tw384x240ns_v, tw384x240sc_h, tw384x240sc_v;
extern unsigned char tw384x256ns_h, tw384x256ns_v, tw384x256sc_h, tw384x256sc_v;
extern int tw256x224_hor;

/* Tweak values for 15.75KHz arcade/ntsc/pal modes */
/* from video.c */
extern unsigned char tw224x288arc_h, tw224x288arc_v, tw288x224arc_h, tw288x224arc_v;
extern unsigned char tw256x256arc_h, tw256x256arc_v, tw256x240arc_h, tw256x240arc_v;
extern unsigned char tw320x240arc_h, tw320x240arc_v, tw320x256arc_h, tw320x256arc_v;
extern unsigned char tw352x240arc_h, tw352x240arc_v, tw352x256arc_h, tw352x256arc_v;
extern unsigned char tw368x240arc_h, tw368x240arc_v, tw368x256arc_h, tw368x256arc_v;
extern unsigned char tw512x224arc_h, tw512x224arc_v, tw512x256arc_h, tw512x256arc_v;
extern unsigned char tw512x448arc_h, tw512x448arc_v, tw512x512arc_h, tw512x512arc_v;


/* from sound.c */
extern int soundcard, usestereo,attenuation;
extern int use_emulated_ym3812;

/* from input.c */
extern int use_mouse, joystick;

/* from cheat.c */
extern char *cheatfile;

/* from datafile.c */
extern char *history_filename,*mameinfo_filename;

/* from fileio.c */
void decompose_rom_sample_path (char *rompath, char *samplepath);
extern char *hidir, *cfgdir, *inpdir, *stadir, *memcarddir;
extern char *artworkdir, *screenshotdir, *alternate_name;

/* from profiler.c */
extern int use_profiler;

/*from svga15kh.c centering for 15.75KHz modes (req. for 15.75KHz Modes)*/
extern int center_x;
extern int center_y;

/*from video.c flag for 15.75KHz modes (req. for 15.75KHz Arcade Monitor Modes)*/
extern int arcade_mode;

/*from svga15kh.c flag to allow delay for odd/even fields for interlaced displays (req. for 15.75KHz Arcade Monitor Modes)*/
extern int wait_interlace;

static int mame_argc;
static char **mame_argv;
static int game;
char *rompath, *samplepath;

struct { char *name; int id; } joy_table[] =
{
	{ "none",               JOY_TYPE_NONE },
	{ "standard",   JOY_TYPE_STANDARD },
	{ "dual",               JOY_TYPE_2PADS },
	{ "4button",    JOY_TYPE_4BUTTON },
	{ "6button",    JOY_TYPE_6BUTTON },
	{ "8button",    JOY_TYPE_8BUTTON },
	{ "fspro",              JOY_TYPE_FSPRO },
	{ "wingex",             JOY_TYPE_WINGEX },
	{ "sidewinder", JOY_TYPE_SIDEWINDER },
	{ "gamepadpro", JOY_TYPE_GAMEPAD_PRO },
	{ "sneslpt1",   JOY_TYPE_SNESPAD_LPT1 },
	{ "sneslpt2",   JOY_TYPE_SNESPAD_LPT2 },
	{ "sneslpt3",   JOY_TYPE_SNESPAD_LPT3 },
	{ NULL, NULL }
} ;



/* monitor type */
struct { char *name; int id; } monitor_table[] =
{
	{ "standard",	MONITOR_TYPE_STANDARD},
	{ "ntsc",       MONITOR_TYPE_NTSC},
	{ "pal",		MONITOR_TYPE_PAL},
	{ "arcade",		MONITOR_TYPE_ARCADE},
	{ NULL, NULL }
} ;


/*
 * gets some boolean config value.
 * 0 = false, >0 = true, <0 = auto
 * the shortcut can only be used on the commandline
 */
static int get_bool (char *section, char *option, char *shortcut, int def)
{
	char *yesnoauto;
	int res, i;

	res = def;

	if (ignorecfg) goto cmdline;

	/* look into mame.cfg, [section] */
	if (def == 0)
		yesnoauto = get_config_string(section, option, "no");
	else if (def > 0)
		yesnoauto = get_config_string(section, option, "yes");
	else /* def < 0 */
		yesnoauto = get_config_string(section, option, "auto");

	/* if the option doesn't exist in the cfgfile, create it */
	if (get_config_string(section, option, "#") == "#")
		set_config_string(section, option, yesnoauto);

	/* look into mame.cfg, [gamename] */
	yesnoauto = get_config_string((char *)drivers[game]->name, option, yesnoauto);

	/* also take numerical values instead of "yes", "no" and "auto" */
	if      (stricmp(yesnoauto, "no"  ) == 0) res = 0;
	else if (stricmp(yesnoauto, "yes" ) == 0) res = 1;
	else if (stricmp(yesnoauto, "auto") == 0) res = -1;
	else    res = atoi (yesnoauto);

cmdline:
	/* check the commandline */
	for (i = 1; i < mame_argc; i++)
	{
		if (mame_argv[i][0] != '-') continue;
		/* look for "-option" */
		if (stricmp(&mame_argv[i][1], option) == 0)
			res = 1;
		/* look for "-shortcut" */
		if (shortcut && (stricmp(&mame_argv[i][1], shortcut) == 0))
			res = 1;
		/* look for "-nooption" */
		if (strnicmp(&mame_argv[i][1], "no", 2) == 0)
		{
			if (stricmp(&mame_argv[i][3], option) == 0)
				res = 0;
			if (shortcut && (stricmp(&mame_argv[i][3], shortcut) == 0))
				res = 0;
		}
		/* look for "-autooption" */
		if (strnicmp(&mame_argv[i][1], "auto", 4) == 0)
		{
			if (stricmp(&mame_argv[i][5], option) == 0)
				res = -1;
			if (shortcut && (stricmp(&mame_argv[i][5], shortcut) == 0))
				res = -1;
		}
	}
	return res;
}

static int get_int (char *section, char *option, char *shortcut, int def)
{
	int res,i;

	res = def;

	if (!ignorecfg)
	{
		/* if the option does not exist, create it */
		if (get_config_int (section, option, -777) == -777)
			set_config_int (section, option, def);

		/* look into mame.cfg, [section] */
		res = get_config_int (section, option, def);

		/* look into mame.cfg, [gamename] */
		res = get_config_int ((char *)drivers[game]->name, option, res);
	}

	/* get it from the commandline */
	for (i = 1; i < mame_argc; i++)
	{
		if (mame_argv[i][0] != '-')
			continue;
		if ((stricmp(&mame_argv[i][1], option) == 0) ||
			(shortcut && (stricmp(&mame_argv[i][1], shortcut ) == 0)))
		{
			i++;
			if (i < mame_argc) res = atoi (mame_argv[i]);
		}
	}
	return res;
}

static float get_float (char *section, char *option, char *shortcut, float def)
{
	int i;
	float res;

	res = def;

	if (!ignorecfg)
	{
		/* if the option does not exist, create it */
		if (get_config_float (section, option, 9999.0) == 9999.0)
			set_config_float (section, option, def);

		/* look into mame.cfg, [section] */
		res = get_config_float (section, option, def);

		/* look into mame.cfg, [gamename] */
		res = get_config_float ((char *)drivers[game]->name, option, res);
	}

	/* get it from the commandline */
	for (i = 1; i < mame_argc; i++)
	{
		if (mame_argv[i][0] != '-')
			continue;
		if ((stricmp(&mame_argv[i][1], option) == 0) ||
			(shortcut && (stricmp(&mame_argv[i][1], shortcut ) == 0)))
		{
			i++;
			if (i < mame_argc) res = atof (mame_argv[i]);
		}
	}
	return res;
}

static char *get_string (char *section, char *option, char *shortcut, char *def)
{
	char *res;
	int i;

	res = def;

	if (!ignorecfg)
	{
		/* if the option does not exist, create it */
		if (get_config_string (section, option, "#") == "#" )
			set_config_string (section, option, def);

		/* look into mame.cfg, [section] */
		res = get_config_string(section, option, def);

		/* look into mame.cfg, [gamename] */
		res = get_config_string((char*)drivers[game]->name, option, res);
	}

	/* get it from the commandline */
	for (i = 1; i < mame_argc; i++)
	{
		if (mame_argv[i][0] != '-')
			continue;

		if ((stricmp(&mame_argv[i][1], option) == 0) ||
			(shortcut && (stricmp(&mame_argv[i][1], shortcut)  == 0)))
		{
			i++;
			if (i < mame_argc) res = mame_argv[i];
		}
	}
	return res;
}

void get_rom_sample_path (int argc, char **argv, int game_index)
{
	int i;

	alternate_name = 0;
	mame_argc = argc;
	mame_argv = argv;
	game = game_index;

	rompath    = get_string ("directory", "rompath",    NULL, ".;ROMS");
	samplepath = get_string ("directory", "samplepath", NULL, ".;SAMPLES");

	/* handle '-romdir' hack. We should get rid of this BW */
	alternate_name = 0;
	for (i = 1; i < argc; i++)
	{
		if (stricmp (argv[i], "-romdir") == 0)
		{
			i++;
			if (i < argc) alternate_name = argv[i];
		}
	}

	/* decompose paths into components (handled by fileio.c) */
	decompose_rom_sample_path (rompath, samplepath);
}

/* for playback of .inp files */
void init_inpdir(void)
{
    inpdir = get_string ("directory", "inp",     NULL, "INP");
}

void parse_cmdline (int argc, char **argv, int game_index)
{
	static float f_beam, f_flicker;
	char *resolution;
	char *vesamode;
	char *joyname;
	char tmpres[10];
	int i;
	char *tmpstr;
	char *monitorname;

	mame_argc = argc;
	mame_argv = argv;
	game = game_index;


	/* read graphic configuration */
	scanlines   = get_bool   ("config", "scanlines",    NULL,  1);
	stretch     = get_bool   ("config", "stretch",    NULL,  1);
	options.use_artwork = get_bool   ("config", "artwork",      NULL,  1);
	options.use_samples = get_bool   ("config", "samples",      NULL,  1);
	video_sync  = get_bool   ("config", "vsync",        NULL,  0);
	wait_vsync  = get_bool   ("config", "waitvsync",    NULL,  0);
	use_triplebuf  = get_bool("config", "triplebuffer",        NULL,  0);
	use_tweaked    = get_bool   ("config", "tweak",         NULL,  0);
	vesamode        = get_string ("config", "vesamode",             NULL,  "vesa3");
	options.antialias   = get_bool   ("config", "antialias",    NULL,  1);
	options.translucency = get_bool    ("config", "translucency", NULL, 1);

	vgafreq     = get_int    ("config", "vgafreq",      NULL,  -1);
	always_synced = get_bool ("config", "alwayssynced", NULL, 0);
	color_depth = get_int    ("config", "depth",        NULL, 16);
	skiplines   = get_int    ("config", "skiplines",    NULL, 0);
	skipcolumns = get_int    ("config", "skipcolumns",  NULL, 0);
	f_beam      = get_float  ("config", "beam",         NULL, 1.0);
	f_flicker   = get_float  ("config", "flicker",      NULL, 0.0);
	osd_gamma_correction = get_float ("config", "gamma",   NULL, 1.2);

	tmpstr             = get_string ("config", "frameskip", "fs", "auto");
	if (!stricmp(tmpstr,"auto"))
	{
		frameskip = 0;
		autoframeskip = 1;
	}
	else
	{
		frameskip = atoi(tmpstr);
		autoframeskip = 0;
	}
	options.norotate  = get_bool ("config", "norotate",  NULL, 0);
	options.ror       = get_bool ("config", "ror",       NULL, 0);
	options.rol       = get_bool ("config", "rol",       NULL, 0);
	options.flipx     = get_bool ("config", "flipx",     NULL, 0);
	options.flipy     = get_bool ("config", "flipy",     NULL, 0);

	/* read sound configuration */
	soundcard           = get_int  ("config", "soundcard",  NULL, -1);
	options.use_emulated_ym3812 = !get_bool ("config", "ym3812opl",  NULL,  1);
	options.samplerate = get_int  ("config", "samplerate", "sr", 22050);
	options.samplebits = get_int  ("config", "samplebits", "sb", 8);
	usestereo           = get_bool ("config", "stereo",  NULL,  1);
	attenuation         = get_int  ("config", "volume",  NULL,  0);

	/* read input configuration */
	use_mouse = get_bool   ("config", "mouse",   NULL,  1);
	joyname   = get_string ("config", "joystick", "joy", "none");

	/* misc configuration */
	options.cheat      = get_bool ("config", "cheat", NULL, 0);
	options.mame_debug = get_bool ("config", "debug", NULL, 0);
	cheatfile  = get_string ("config", "cheatfile", "cf", "CHEAT.DAT");    /* JCK 980917 */
	history_filename  = get_string ("config", "historyfile", NULL, "HISTORY.DAT");    /* JCK 980917 */
	mameinfo_filename  = get_string ("config", "mameinfofile", NULL, "MAMEINFO.DAT");    /* JCK 980917 */
	use_profiler        = get_bool ("config", "profiler", NULL,  0);

	/* get resolution */
	resolution  = get_string ("config", "resolution", NULL, "auto");

	/* set default subdirectories */
	hidir      = get_string ("directory", "hi",      NULL, "HI");
	cfgdir     = get_string ("directory", "cfg",     NULL, "CFG");
	screenshotdir = get_string ("directory", "snap",     NULL, "SNAP");
	memcarddir = get_string ("directory", "memcard", NULL, "MEMCARD");
	stadir     = get_string ("directory", "sta",     NULL, "STA");
	artworkdir = get_string ("directory", "artwork", NULL, "ARTWORK");

	/* get tweaked modes info */
	tw256x224_hor     = get_bool("tweaked", "256x224_hor",    NULL, 1);
	tw224x288ns_h     = get_int ("tweaked", "224x288ns_h",    NULL, 0x5f);
	tw224x288ns_v     = get_int ("tweaked", "224x288ns_v",    NULL, 0x53);
	tw224x288sc_h     = get_int ("tweaked", "224x288sc_h",    NULL, 0x5f);
	tw224x288sc_v     = get_int ("tweaked", "224x288sc_v",    NULL, 0x43);
	tw288x224ns_h     = get_int ("tweaked", "288x224ns_h",    NULL, 0x56);
	tw288x224ns_v     = get_int ("tweaked", "288x224ns_v",    NULL, 0x3f);
	tw288x224sc_h     = get_int ("tweaked", "288x224sc_h",    NULL, 0x5f);
	tw288x224sc_v     = get_int ("tweaked", "288x224sc_v",    NULL, 0x0b);
	tw256x256ns_h     = get_int ("tweaked", "256x256ns_h",    NULL, 0x62);
	tw256x256ns_v     = get_int ("tweaked", "256x256ns_v",    NULL, 0x42);
	tw256x256sc_h     = get_int ("tweaked", "256x256sc_h",    NULL, 0x5f);
	tw256x256sc_v     = get_int ("tweaked", "256x256sc_v",    NULL, 0x23);
	tw256x256ns_hor_h     = get_int ("tweaked", "256x256ns_hor_h",    NULL, 0x55);
	tw256x256ns_hor_v     = get_int ("tweaked", "256x256ns_hor_v",    NULL, 0x42);
	tw256x256sc_hor_h     = get_int ("tweaked", "256x256sc_hor_h",    NULL, 0x52);
	tw256x256sc_hor_v     = get_int ("tweaked", "256x256sc_hor_v",    NULL, 0x2b);
	tw256x256ns_57_h     = get_int ("tweaked", "256x256ns_57_h",    NULL, 0x5f);
	tw256x256ns_57_v     = get_int ("tweaked", "256x256ns_57_v",    NULL, 0x23);
	tw256x256sc_57_h     = get_int ("tweaked", "256x256sc_57_h",    NULL, 0x5f);
	tw256x256sc_57_v     = get_int ("tweaked", "256x256sc_57_v",    NULL, 0x10);
	tw256x256ns_h57_h     = get_int ("tweaked", "256x256ns_h57_h",    NULL, 0x55);
	tw256x256ns_h57_v     = get_int ("tweaked", "256x256ns_h57_v",    NULL, 0x61);
	tw256x256sc_h57_h     = get_int ("tweaked", "256x256sc_h57_h",    NULL, 0x54);
	tw256x256sc_h57_v     = get_int ("tweaked", "256x256sc_h57_v",    NULL, 0x33);
	/* 320x240 modes */
	tw320x240ns_h		= get_int ("tweaked", "320x240ns_h",	NULL, 0x5f);
	tw320x240ns_v		= get_int ("tweaked", "320x240ns_v",    NULL, 0x0b);
	tw320x240sc_h		= get_int ("tweaked", "320x240sc_h",    NULL, 0x5f);
	tw320x240sc_v		= get_int ("tweaked", "320x240sc_v",    NULL, 0x07);
	/* 336x240 modes */
	tw336x240ns_h		= get_int ("tweaked", "336x240ns_h",	NULL, 0x5f);
	tw336x240ns_v		= get_int ("tweaked", "336x240ns_v",    NULL, 0x08);
	tw336x240sc_h		= get_int ("tweaked", "336x240sc_h",    NULL, 0x5f);
	tw336x240sc_v		= get_int ("tweaked", "336x240sc_v",    NULL, 0x03);
	/* 384x240 modes */
	tw384x240ns_h		= get_int ("tweaked", "384x240ns_h",	NULL, 0x6c);
	tw384x240ns_v		= get_int ("tweaked", "384x240ns_v",    NULL, 0x0c);
	tw384x240sc_h		= get_int ("tweaked", "384x240sc_h",    NULL, 0x6c);
	tw384x240sc_v		= get_int ("tweaked", "384x240sc_v",    NULL, 0x05);
	/* 384x256 modes */
	tw384x256ns_h		= get_int ("tweaked", "384x256ns_h",	NULL, 0x6c);
	tw384x256ns_v		= get_int ("tweaked", "384x256ns_v",    NULL, 0x32);
	tw384x256sc_h		= get_int ("tweaked", "384x256sc_h",    NULL, 0x6c);
	tw384x256sc_v		= get_int ("tweaked", "384x256sc_v",    NULL, 0x19);
	/* Get 15.75KHz tweak values */
	tw224x288arc_h		= get_int ("tweaked", "224x288arc_h",	NULL, 0x5d);
	tw224x288arc_v		= get_int ("tweaked", "224x288arc_v",	NULL, 0x38);
	tw288x224arc_h		= get_int ("tweaked", "288x224arc_h",	NULL, 0x5d);
	tw288x224arc_v		= get_int ("tweaked", "288x224arc_v",	NULL, 0x09);
	tw256x240arc_h		= get_int ("tweaked", "256x240arc_h",	NULL, 0x5d);
	tw256x240arc_v		= get_int ("tweaked", "256x240arc_v",	NULL, 0x09);
	tw256x256arc_h		= get_int ("tweaked", "256x256arc_h",	NULL, 0x5d);
	tw256x256arc_v		= get_int ("tweaked", "256x256arc_v",	NULL, 0x17);
	tw320x240arc_h		= get_int ("tweaked", "320x240arc_h",	NULL, 0x69);
	tw320x240arc_v		= get_int ("tweaked", "320x240arc_v",	NULL, 0x09);
	tw320x256arc_h		= get_int ("tweaked", "320x256arc_h",	NULL, 0x69);
	tw320x256arc_v		= get_int ("tweaked", "320x256arc_v",	NULL, 0x17);
	tw352x240arc_h		= get_int ("tweaked", "352x240arc_h",	NULL, 0x6a);
	tw352x240arc_v		= get_int ("tweaked", "352x240arc_v",	NULL, 0x09);
	tw352x256arc_h		= get_int ("tweaked", "352x256arc_h",	NULL, 0x6a);
	tw352x256arc_v		= get_int ("tweaked", "352x256arc_v",	NULL, 0x17);
	tw368x240arc_h		= get_int ("tweaked", "368x240arc_h",	NULL, 0x6a);
	tw368x240arc_v		= get_int ("tweaked", "368x240arc_v",	NULL, 0x09);
	tw368x256arc_h		= get_int ("tweaked", "368x256arc_h",	NULL, 0x6a);
	tw368x256arc_v		= get_int ("tweaked", "368x256arc_v",	NULL, 0x17);
	tw512x224arc_h		= get_int ("tweaked", "512x224arc_h",	NULL, 0xbf);
	tw512x224arc_v		= get_int ("tweaked", "512x224arc_v",	NULL, 0x09);
	tw512x256arc_h		= get_int ("tweaked", "512x256arc_h",	NULL, 0xbf);
	tw512x256arc_v		= get_int ("tweaked", "512x256arc_v",	NULL, 0x17);
	tw512x448arc_h		= get_int ("tweaked", "512x448arc_h",	NULL, 0xbf);
	tw512x448arc_v		= get_int ("tweaked", "512x448arc_v",	NULL, 0x09);
	tw512x512arc_h		= get_int ("tweaked", "512x512arc_h",	NULL, 0xbf);
	tw512x512arc_v		= get_int ("tweaked", "512x512arc_v",	NULL, 0x17);

	/* this is handled externally cause the audit stuff needs it, too */
	get_rom_sample_path (argc, argv, game_index);

	/* get the monitor type */
	monitorname = get_string ("config", "monitor", NULL, "standard");
	/* get -centerx */
	center_x = get_int ("config", "centerx", NULL,  0);
	/* get -centery */
	center_y = get_int ("config", "centery", NULL,  0);
	/* get -waitinterlace */
	wait_interlace = get_bool ("config", "waitinterlace", NULL,  0);

	/* process some parameters */
	options.beam = (int)(f_beam * 0x00010000);
	if (options.beam < 0x00010000)
		options.beam = 0x00010000;
	if (options.beam > 0x00100000)
		options.beam = 0x00100000;

	options.flicker = (int)(f_flicker * 2.55);
	if (options.flicker < 0)
		options.flicker = 0;
	if (options.flicker > 255)
		options.flicker = 255;

	if (stricmp (vesamode, "vesa1") == 0)
		gfx_mode = GFX_VESA1;
	else if (stricmp (vesamode, "vesa2b") == 0)
		gfx_mode = GFX_VESA2B;
	else if (stricmp (vesamode, "vesa2l") == 0)
		gfx_mode = GFX_VESA2L;
	else if (stricmp (vesamode, "vesa3") == 0)
		gfx_mode = GFX_VESA3;
	else
	{
		if (errorlog)
			fprintf (errorlog, "%s is not a valid entry for vesamode\n",
					vesamode);
		gfx_mode = GFX_VESA3; /* default to VESA2L */
	}

	/* any option that starts with a digit is taken as a resolution option */
	/* this is to handle the old "-wxh" commandline option. */
	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-' && isdigit(argv[i][1]) &&
	/* additional kludge to handle negative arguments to -skiplines and -skipcolumns */
	/* and to -volume */
	/* and to -centerx (req. for 15.75KHz Modes)*/
	/* and to -centery (req. for 15.75KHz Modes)*/
			(i == 1 || (stricmp(argv[i-1],"-skiplines") &&
						stricmp(argv[i-1],"-skipcolumns") &&
						stricmp(argv[i-1],"-volume") &&
						stricmp(argv[i-1],"-centerx") &&
						stricmp(argv[i-1],"-centery"))))

			resolution = &argv[i][1];
	}

	/* break up resolution into gfx_width and gfx_height */
	gfx_height = gfx_width = 0;
	if (stricmp (resolution, "auto") != 0)
	{
		char *tmp;
		strncpy (tmpres, resolution, 10);
		tmp = strtok (tmpres, "xX");
		gfx_width = atoi (tmp);
		tmp = strtok (0, "xX");
		if (tmp)
			gfx_height = atoi (tmp);
	}

	/* convert joystick name into an Allegro-compliant joystick signature */
	joystick = -2; /* default to invalid value */

	for (i = 0; joy_table[i].name != NULL; i++)
	{
		char tmpnum[33];
		(void) itoa (i, tmpnum, 10); /* old Allegro joystick index */
		if ((stricmp (joy_table[i].name, joyname) == 0) ||
			(stricmp (tmpnum, joyname) == 0))
		{
			joystick = joy_table[i].id;
			break;
		}
	}

	if (joystick == -2)
	{
		if (errorlog)
			fprintf (errorlog, "%s is not a valid entry for a joystick\n",
					joyname);
		joystick = JOY_TYPE_NONE;
	}

	/* get monitor type from supplied name */
	monitor_type = MONITOR_TYPE_STANDARD; /* default to PC monitor */

	for (i = 0; monitor_table[i].name != NULL; i++)
	{
		if ((stricmp (monitor_table[i].name, monitorname) == 0))
		{
			monitor_type = monitor_table[i].id;
			break;
		}
	}
}

