/***************************************************************************

  osdepend.c

  OS dependant stuff (display handling, keyboard scan...)
  This is the only file which should me modified in order to port the
  emulator to a different system.

***************************************************************************/

#include "driver.h"
#include <allegro.h>


int  msdos_init_seal (void);
int  msdos_init_sound(void);
void msdos_init_input(void);
void msdos_shutdown_sound(void);
int  frontend_help (int argc, char **argv);
void parse_cmdline (int argc, char **argv, struct GameOptions *options, int game);

/* platform independent options go here */
struct GameOptions options;

int  ignorecfg;

/* avoid wild card expansion on the command line (DJGPP feature) */
char **__crt0_glob_function(void)
{
	return 0;
}


/* put here anything you need to do when the program is started. Return 0 if */
/* initialization was successful, nonzero otherwise. */
int osd_init(void)
{
	if (msdos_init_sound())
		return 1;
	msdos_init_input();
	install_keyboard();
	return 0;
}


/* put here cleanup routines to be executed when the program is terminated. */
void osd_exit(void)
{
	msdos_shutdown_sound();
	remove_keyboard();
}


int main (int argc, char **argv)
{
	int res, i, j, game_index;

	/* these two are not available in mame.cfg */
	ignorecfg = 0;
	errorlog = options.errorlog = 0;

	for (i = 1;i < argc;i++) /* V.V_121997 */
	{
		if (stricmp(argv[i],"-ignorecfg") == 0) ignorecfg = 1;
		if (stricmp(argv[i],"-log") == 0)
			errorlog = options.errorlog = fopen("error.log","wa");
	}

	allegro_init();

	set_config_file ("mame.cfg");

	/* Initialize the audio library */
	if (msdos_init_seal())
	{
		printf ("Unable to initialize SEAL\n");
		return (1);
	}

	/* check for frontend options */
	res = frontend_help (argc, argv);

	/* if frontend options were used, return to DOS with the error code */
	if (res != 1234)
		exit (res);

	/* take the first commandline argument without "-" as the game name */
	for (j = 1; j < argc; j++)
		if (argv[j][0] != '-') break;

	/* do we have a drivers for this? */
	for (i = 0; drivers[i]; i++)
		if (stricmp(argv[j],drivers[i]->name) == 0) break;

	if (drivers[i] == 0)
	{
		printf("Game \"%s\" not supported\n", argv[j]);
		return 1;
	}
	else
		game_index = i;

	/* parse generic (os-independent) options */
	parse_cmdline (argc, argv, &options, game_index);

	/* handle record and playback. These are not available in mame.cfg */
	for (i = 1; i < argc; i++)
	{
		if (stricmp(argv[i],"-record") == 0)
		{
			i++;
			if (i < argc)
				options.record = osd_fopen(argv[i],0,OSD_FILETYPE_INPUTLOG,1);
		}
		if (stricmp(argv[i],"-playback") == 0)
		{
			i++;
			if (i < argc)
				options.playback = osd_fopen(argv[i],0,OSD_FILETYPE_INPUTLOG,0);
		}
	}

	/* go for it */
	res = run_game (game_index , &options);

	/* close open files */
	if (options.errorlog) fclose (options.errorlog);
	if (options.playback) osd_fclose (options.playback);
	if (options.record)   osd_fclose (options.record);

	exit (res);
}
