#include "driver.h"
#include "strings.h"

/* Mame frontend interface & commandline */
/* parsing rountines by Maurizio Zanello */

/* compare string[8] using standard(?) DOS wildchars ('?' & '*')      */
/* for this to work correctly, the shells internal wildcard expansion */
/* mechanism has to be disabled. Look into msdos.c */

int  VerifyRomSet (int game);
int  VerifySampleSet (int game);
void get_rom_sample_path (int argc, char **argv, int game_index);

static const struct GameDriver *gamedrv;

int strwildcmp(const char *sp1, const char *sp2)
{
	char s1[9], s2[9];
	int i, l1, l2;
	char *p;

	strncpy(s1, sp1, 8); s1[8] = 0; if (s1[0] == 0) strcpy(s1, "*");

	strncpy(s2, sp2, 8); s2[8] = 0; if (s2[0] == 0) strcpy(s2, "*");

	p = strchr(s1, '*');
	if (p)
	{
		for (i = p - s1; i < 8; i++) s1[i] = '?';
		s1[8] = 0;
	}

	p = strchr(s2, '*');
	if (p)
	{
		for (i = p - s2; i < 8; i++) s2[i] = '?';
		s2[8] = 0;
	}

	l1 = strlen(s1);
	if (l1 < 8)
	{
		for (i = l1 + 1; i < 8; i++) s1[i] = ' ';
		s1[8] = 0;
	}

	l2 = strlen(s2);
	if (l2 < 8)
	{
		for (i = l2 + 1; i < 8; i++) s2[i] = ' ';
		s2[8] = 0;
	}

	for (i = 0; i < 8; i++)
	{
		if (s1[i] == '?' && s2[i] != '?') s1[i] = s2[i];
		if (s2[i] == '?' && s1[i] != '?') s2[i] = s1[i];
	}

	return stricmp(s1, s2);
}


int frontend_help (int argc, char **argv)
{
	int i, j;
	int list = 0;
	int verify = 0;
	int help = 1;    /* by default is TRUE */
	char gamename[9];

	/* covert '/' in '-' */
	for (i = 1;i < argc;i++) if (argv[i][0] == '/') argv[i][0] = '-';

	/* by default display the help unless */
	/* a game or an utility are specified */

	strcpy(gamename, "");

	for (i = 1;i < argc;i++)
	{
		/* find the FIRST "gamename" field (without '-') */
		if ((strlen(gamename) == 0) && (argv[i][0] != '-'))
		{
			strncpy(gamename, argv[i], 8);
			gamename[8] = 0;
		}
	}

	for (i = 1; i < argc; i++)
	{
		/* check for front-end utilities */
		if (!stricmp(argv[i],"-list")) list = 1;
		if (!stricmp(argv[i],"-listfull")) list = 2;
		if (!stricmp(argv[i],"-listsamdir")) list = 3;

#ifdef MAME_DEBUG /* do not put this into a public release! */
		if (!stricmp(argv[i],"-lmr")) list = 6;
#endif

		/* these options REQUIRES gamename field to work */
		if (strlen(gamename) > 0)
		{
			if (!stricmp(argv[i],"-listroms")) list = 4;
			if (!stricmp(argv[i],"-listsamples")) list = 5;
			if (!stricmp(argv[i],"-verifyroms")) verify = 1;
			if (!stricmp(argv[i],"-verifysamples")) verify = 2;
		}
	}

	if ((strlen(gamename)> 0) || list || verify) help = 0;

	for (i = 1;i < argc;i++)
	{
		/* ...however, I WANT the help! */
		if (!stricmp(argv[i],"-?") || !stricmp(argv[i],"-h") || !stricmp(argv[i],"-help"))
			help = 1;
	}

	if (help)  /* brief help - useful to get current version info */
	{
		printf("M.A.M.E. v%s - Multiple Arcade Machine Emulator\n"
				"Copyright (C) 1997-98 by Nicola Salmoria and the MAME Team\n\n",mameversion);
		showdisclaimer();
		printf("Usage:  MAME gamename [options]\n\n"
				"        MAME -list      for a brief list of supported games\n"
				"        MAME -listfull  for a full list of supported games\n\n"
				"See readme.txt for a complete list of options.\n");

		return 0;
	}

	switch (list)  /* front-end utilities ;) */
	{
		case 1: /* simple games list */
			printf("\nMAME currently supports the following games:\n\n");
			i = 0; j = 0;
			while (drivers[i])
			{
				if (!strwildcmp(gamename, drivers[i]->name))
				{
					printf("%10s",drivers[i]->name);
					j++;
					if (!(j % 7)) printf("\n");
				}
				i++;
			}
			if (j % 7) printf("\n");
			printf("\n");
			if (j != i) printf("Total ROM sets displayed: %4d - ", j);
			printf("Total ROM sets supported: %4d\n", i);
			return 0;
			break;
		case 2: /* games list with descriptions */
			printf("Name:     Description:\n");
			i = 0;
			while (drivers[i])
			{
				if (!strwildcmp(gamename, drivers[i]->name))
					printf("%-10s\"%s\"\n",drivers[i]->name,drivers[i]->description);
				i++;
			}
			return 0;
			break;
		case 3: /* games list with samples directories */
			printf("Name:     Samples dir:\n");
			i = 0;
			while (drivers[i])
			{
				if (!strwildcmp(gamename, drivers[i]->name))
					if (drivers[i]->samplenames != 0 && drivers[i]->samplenames[0] != 0)
					{
						printf("%-10s",drivers[i]->name);
						if (drivers[i]->samplenames[0][0] == '*')
							printf("%s\n",drivers[i]->samplenames[0]+1);
						else
							printf("%s\n",drivers[i]->name);
					}
				i++;
			}
			return 0;
			break;
		case 4: /* game roms list or */
		case 5: /* game samples list */
			j = 0;
			while (drivers[j] && (stricmp(gamename,drivers[j]->name) != 0))
				j++;
			if (drivers[j] == 0)
			{
				printf("Game \"%s\" not supported!\n",gamename);
				return 1;
			}
			gamedrv = drivers[j];
			if (list == 4)
				printromlist(gamedrv->rom,gamename);
			else
			{
				if (gamedrv->samplenames != 0 && gamedrv->samplenames[0] != 0)
				{
					i = 0;
					while (gamedrv->samplenames[i] != 0)
					{
						printf("%s\n",gamedrv->samplenames[i]);
						i++;
					}
				}
			}
			return 0;
			break;
		case 6:
			for (i = 0; drivers[i]; i++)
			{
				get_rom_sample_path (argc, argv, i);
				if (!osd_faccess (drivers[i]->name, OSD_FILETYPE_ROM))
						printf ("%s\n", drivers[i]->name);
			}
			return 0;
			break;
	}

	if (verify)  /* "verify" utilities */
	{
		int err = 0;
		int correct = 0;
		int incorrect = 0;
		int res = 0;
		for (i = 0; drivers[i]; i++)
		{
			if (strwildcmp(gamename, drivers[i]->name))
				continue;

			/* set rom and sample path correctly */
			get_rom_sample_path (argc, argv, i);

			/* if using wildcards, ignore games we don't have romsets for. */
			if (!osd_faccess (drivers[i]->name, OSD_FILETYPE_ROM))
			{
				if (stricmp(gamename, drivers[i]->name) != 0) continue;
			}

			if (verify == 1)
			{
				res = VerifyRomSet (i);
				if (res)
					printf ("romset %s ", drivers[i]->name);
			}
			if (verify == 2)
			{
				/* ignore games that need no samples */
   				if (drivers[i]->samplenames == 0 ||
					drivers[i]->samplenames[0] == 0)
					continue;

				res = VerifySampleSet (i);
				if (res)
					printf ("sampleset %s ", drivers[i]->name);
			}

			if (res == 1)
				printf ("not found\n");
			else if (res == 2)
			{
				printf ("incorrect\n");
				incorrect++;
			}
			else
				correct++;
			if (res)
				err = res;
		}

		if (correct+incorrect == 0)
		{
			printf("Game \"%s\" not supported!\n",gamename);
			return 1;
		}
		else
		{
			printf("%d %s found, %d were OK.\n", correct+incorrect,
					(verify == 1)? "romsets" : "samplesets", correct);
			if (incorrect > 0)
				return 2;
			else
				return 0;
		}
	}

	/* use a special return value if no frontend function used */

	return 1234;
}
