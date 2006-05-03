//============================================================
//
//  fronthlp.c - Win32 frontend management functions
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <ctype.h>

// MAME headers
#include "driver.h"
#include "hash.h"
#include "info.h"
#include "audit.h"
#include "unzip.h"
#include "jedparse.h"
#include "sound/samples.h"

#include "rc.h"

#ifndef MESS
enum { LIST_XML = 1, LIST_FULL, LIST_ROMS, LIST_SAMPLES,
		LIST_CLONES,
		LIST_CRC,
		LIST_CPU, LIST_CPUCLASS, LIST_SOURCEFILE,
		 };
#else
#include "infomess.h"
enum { LIST_XML = 1, LIST_FULL, LIST_ROMS, LIST_SAMPLES,
		LIST_CLONES,
		LIST_CRC,
		LIST_SOURCEFILE,
		LIST_MESSDEVICES };
#endif

#define VERIFY_ROMS		0x00000001
#define VERIFY_SAMPLES	0x00000002

#define KNOWN_START 0
#define KNOWN_ALL   1
#define KNOWN_NONE  2
#define KNOWN_SOME  3

static int list = 0;
static int verify = 0;
static int ident = 0;
static int help = 0;

struct rc_option frontend_opts[] =
{
	{ "Frontend Related", NULL,	rc_seperator, NULL, NULL, 0, 0,	NULL, NULL },

	{ "help", "h", rc_set_int, &help, NULL, 1, 0, NULL, "show help message" },
	{ "?", NULL,   rc_set_int, &help, NULL, 1, 0, NULL, "show help message" },

	/* list options follow */
	{ "listxml", "lx", rc_set_int, &list, NULL, LIST_XML, 0, NULL, "all available info on driver in XML format" },
	{ "listfull", "ll", rc_set_int,	&list, NULL, LIST_FULL,	0, NULL, "short name, full name" },
	{ "listsource",	"ls", rc_set_int, &list, NULL, LIST_SOURCEFILE, 0, NULL, "driver sourcefile" },
	{ "listclones", "lc", rc_set_int, &list, NULL, LIST_CLONES, 0, NULL, "show clones" },
	{ "listcrc", NULL, rc_set_int, &list, NULL, LIST_CRC, 0, NULL, "CRC-32s" },
#ifdef MESS
	{ "listdevices", NULL, rc_set_int, &list, NULL, LIST_MESSDEVICES, 0, NULL, "list available devices" },
#endif
	{ "listroms", NULL, rc_set_int, &list, NULL, LIST_ROMS, 0, NULL, "list required roms for a driver" },
	{ "listsamples", NULL, rc_set_int, &list, NULL, LIST_SAMPLES, 0, NULL, "list optional samples for a driver" },
	{ "verifyroms", NULL, rc_set_int, &verify, NULL, VERIFY_ROMS, 0, NULL, "report romsets that have problems" },
	{ "verifysamples", NULL, rc_set_int, &verify, NULL, VERIFY_SAMPLES, 0, NULL, "report samplesets that have problems" },
	{ "romident", NULL, rc_set_int, &ident, NULL, 1, 0, NULL, "compare files with known MAME roms" },
	{ "isknown", NULL, rc_set_int, &ident, NULL, 2, 0, NULL, "compare files with known MAME roms (brief)" },
	{ NULL, NULL, rc_end, NULL, NULL, 0, 0, NULL, NULL }
};


static int silentident,knownstatus,identfiles,identmatches,identnonroms;



/* compare string[8] using standard(?) DOS wildchars ('?' & '*')      */
/* for this to work correctly, the shells internal wildcard expansion */
/* mechanism has to be disabled. Look into msdos.c */

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

	return mame_stricmp(s1, s2);
}


static void namecopy(char *name_ref,const char *desc)
{
	char name[200];

	strcpy(name,desc);

	/* remove details in parenthesis */
	if (strstr(name," (")) *strstr(name," (") = 0;

	/* Move leading "The" to the end */
	if (strncmp(name,"The ",4) == 0)
	{
		sprintf(name_ref,"%s, The",name+4);
	}
	else
		sprintf(name_ref,"%s",name);
}


/*-------------------------------------------------
    match_roms - scan for a matching ROM by hash
-------------------------------------------------*/

static void match_roms(const game_driver *driver, const char *hash, int length, int *found)
{
	const rom_entry *region, *rom;

	/* iterate over regions and files within the region */
	for (region = rom_first_region(driver); region; region = rom_next_region(region))
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
			if (hash_data_is_equal(hash, ROM_GETHASHDATA(rom), 0))
			{
				int baddump = hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_BAD_DUMP);

				if (!silentident)
				{
					if (*found != 0)
						printf("             ");
					printf("= %s%-12s  %s\n", baddump ? "(BAD) " : "", ROM_GETNAME(rom), driver->description);
				}
				(*found)++;
			}
}


/*-------------------------------------------------
    identify_data - identify a buffer full of
    data; if it comes from a .JED file, parse the
    fusemap into raw data first
-------------------------------------------------*/

void identify_data(const char *name, const UINT8 *data, int length)
{
	int namelen = strlen(name);
	char hash[HASH_BUF_SIZE];
	UINT8 *tempjed = NULL;
	int found = 0;
	jed_data jed;
	int i;

	/* if this is a '.jed' file, process it into raw bits first */
	if (namelen > 4 && name[namelen - 4] == '.' &&
		tolower(name[namelen - 3]) == 'j' &&
		tolower(name[namelen - 2]) == 'e' &&
		tolower(name[namelen - 1]) == 'd' &&
		jed_parse(data, length, &jed) == JEDERR_NONE)
	{
		/* now determine the new data length and allocate temporary memory for it */
		length = jedbin_output(&jed, NULL, 0);
		tempjed = malloc(length);
		if (!tempjed)
			return;

		/* create a binary output of the JED data and use that instead */
		jedbin_output(&jed, tempjed, length);
		data = tempjed;
	}

	/* compute the hash of the data */
	hash_data_clear(hash);
	hash_compute(hash, data, length, HASH_SHA1 | HASH_CRC);

	/* remove directory portion of the name */
	for (i = namelen - 1; i > 0; i--)
		if (name[i] == '/' || name[i] == '\\')
		{
			i++;
			break;
		}

	/* output the name */
	identfiles++;
	if (!silentident)
		printf("%s ", &name[i]);

	/* see if we can find a match in the ROMs */
	for (i = 0; drivers[i]; i++)
		match_roms(drivers[i], hash, length, &found);

	/* if we didn't find it, try to guess what it might be */
	if (found == 0)
	{
		/* if not a power of 2, assume it is a non-ROM file */
		if ((length & (length - 1)) != 0)
		{
			if (!silentident)
				printf("NOT A ROM\n");
			identnonroms++;
		}

		/* otherwise, it's just not a match */
		else
		{
			if (!silentident)
				printf("NO MATCH\n");
			if (knownstatus == KNOWN_START)
				knownstatus = KNOWN_NONE;
			else if (knownstatus == KNOWN_ALL)
				knownstatus = KNOWN_SOME;
		}
	}

	/* if we did find it, count it as a match */
	else
	{
		identmatches++;
		if (knownstatus == KNOWN_START)
			knownstatus = KNOWN_ALL;
		else if (knownstatus == KNOWN_NONE)
			knownstatus = KNOWN_SOME;
	}

	/* free any temporary JED data */
	if (tempjed)
		free(tempjed);
}


/*-------------------------------------------------
    identify_file - identify a file; if it is a
    ZIP file, scan it and identify all enclosed
    files
-------------------------------------------------*/

void identify_file(const char *name)
{
	int namelen = strlen(name);
	int length;
	FILE *f;

	/* if the file has a 3-character extension, check it */
	if (namelen > 4 && name[namelen - 4] == '.' &&
		tolower(name[namelen - 3]) == 'z' &&
		tolower(name[namelen - 2]) == 'i' &&
		tolower(name[namelen - 1]) == 'p')
	{
		/* first attempt to examine it as a valid ZIP file */
		zip_file *zip = openzip(FILETYPE_RAW, 0, name);
		if (zip != NULL)
		{
			zip_entry *entry;

			/* loop over entries in the ZIP, skipping empty files and directories */
			for (entry = readzip(zip); entry; entry = readzip(zip))
				if (entry->uncompressed_size != 0)
				{
					UINT8 *data = (UINT8 *)malloc(entry->uncompressed_size);
					if (data != NULL)
					{
						readuncompresszip(zip, entry, data);
						identify_data(entry->name, data, entry->uncompressed_size);
						free(data);
					}
				}

			/* close up and exit early */
			closezip(zip);
			return;
		}
	}

	/* open the file directly */
	f = fopen(name, "rb");
	if (f)
	{
		/* determine the length of the file */
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);

		/* skip empty files */
		if (length != 0)
		{
			UINT8 *data = (UINT8 *)malloc(length);
			if (data != NULL)
			{
				fread(data, 1, length, f);
				identify_data(name, data, length);
				free(data);
			}
		}
		fclose(f);
	}
}


/*-------------------------------------------------
    identify_dir - scan a directory and identify
    all the files in it
-------------------------------------------------*/

void identify_dir(const char *dirname)
{
	WIN32_FIND_DATA entry;
	char *dirfilter;
	HANDLE dir;
	int more;

	/* create the search name */
	dirfilter = malloc(strlen(dirname) + 5);
	if (dirfilter == NULL)
		return;
	sprintf(dirfilter, "%s\\*", dirname);

	/* find the first file */
	memset(&entry, 0, sizeof(WIN32_FIND_DATA));
    dir = FindFirstFile(dirfilter, &entry);
    free(dirfilter);

	/* bail on failure */
    if (dir == INVALID_HANDLE_VALUE)
        return;

	/* loop until we run out of entries */
    do
    {
    	/* skip directories */
		if (!(entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			/* create a full path to the file and identify it */
			char *buf = (char *)malloc(strlen(dirname) + 1 + strlen(entry.cFileName) + 1);
			sprintf(buf, "%s\\%s", dirname, entry.cFileName);
			identify_file(buf);
			free(buf);
		}

		/* find the next entry */
		more = FindNextFileA(dir, &entry);
	} while (more);

	/* stop the search */
	FindClose(dir);
}


/*-------------------------------------------------
    romident - identify files
-------------------------------------------------*/

void romident(const char *name)
{
	TCHAR error[256];
	DWORD attr;

	/* see what kind of file we're looking at */
	attr = GetFileAttributes(name);

	/* invalid -- nothing to see here */
	if (attr == INVALID_FILE_ATTRIBUTES)
	{
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, error, ARRAY_LENGTH(error), NULL);
		_tprintf("%s: %s\n",name, error);
		return;
	}

	/* directory -- scan it */
	if (attr & FILE_ATTRIBUTE_DIRECTORY)
		identify_dir(name);
	else
		identify_file(name);
}


int frontend_help (const char *gamename, const char *filename)
{
	machine_config drv;
	int i, j;
	const game_driver *gamedrv;
	const game_driver *clone_of;
	const char *all_games = "*";
	char *pdest = NULL;
	int result = 0;

	/* display help unless a game or an utility are specified */
	if (!gamename && !help && !list && !ident && !verify)
		help = 1;

	if (help)  /* brief help - useful to get current version info */
	{
		#ifndef MESS
		printf("M.A.M.E. v%s - Multiple Arcade Machine Emulator\n"
				"Copyright (C) 1997-2005 by Nicola Salmoria and the MAME Team\n\n",build_version);
		printf("%s\n", mame_disclaimer);
		printf("Usage:  MAME gamename [options]\n\n"
				"        MAME -showusage    for a brief list of options\n"
				"        MAME -showconfig   for a list of configuration options\n"
				"        MAME -createconfig to create a mame.ini\n\n"
				"For usage instructions, please consult the file windows.txt\n");
		#else
		showmessinfo();
		#endif
		return 0;
	}

	/* HACK: some options REQUIRE gamename field to work: default to "*" */
	if (!gamename || (strlen(gamename) == 0))
		gamename = all_games;

	/* since the cpuintrf structure is filled dynamically now, we have to init first */
	cpuintrf_init();
	sndintrf_init();

#define isclone(i) \
	((clone_of = driver_get_clone(drivers[i])) != NULL && (clone_of->flags & NOT_A_DRIVER) == 0)

	switch (list)  /* front-end utilities ;) */
	{
        #ifdef MESS
		case LIST_MESSDEVICES:
			/* send the gamename to MESS */
			print_mess_devices(gamename);
			return 0;
			break;
		#endif

		case LIST_FULL: /* games list with descriptions */
			printf("Name:     Description:\n");
			for (i = 0; drivers[i]; i++)
				if (	((drivers[i]->flags & NOT_A_DRIVER) == 0) &&
					!strwildcmp(gamename, drivers[i]->name))
				{
					char name[200];

					printf("%-10s",drivers[i]->name);

					namecopy(name,drivers[i]->description);
					printf("\"%s",name);

					/* print the additional description only if we are listing clones */
					{
						pdest = strchr(drivers[i]->description,'(');
						result = pdest - drivers[i]->description;
						if (pdest != NULL && result > 0 )
							printf(" %s",strchr(drivers[i]->description,'('));
					}
					printf("\"\n");
				}
			return 0;
			break;

		case LIST_ROMS: /* game roms list or */
		case LIST_SAMPLES: /* game samples list */
			j = 0;
			while (drivers[j] && (mame_stricmp(gamename,drivers[j]->name) != 0))
				j++;
			if (drivers[j] == 0)
			{
				printf("Game \"%s\" not supported!\n",gamename);
				return 1;
			}
			gamedrv = drivers[j];
			if (list == LIST_ROMS)
			{
				const rom_entry *region, *rom, *chunk;
				char buf[512];

				printf("This is the list of the ROMs required for driver \"%s\".\n"
						"Name            Size Checksum\n",gamename);

				for (region = gamedrv->rom; region; region = rom_next_region(region))
				{
					for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
					{
						const char *name = ROM_GETNAME(rom);
						const char* hash = ROM_GETHASHDATA(rom);
						int length = -1; /* default is for disks! */

						if (ROMREGION_ISROMDATA(region))
						{
							length = 0;
							for (chunk = rom_first_chunk(rom); chunk; chunk = rom_next_chunk(chunk))
								length += ROM_GETLENGTH(chunk);
						}

						printf("%-12s ", name);
						if (length >= 0)
							printf("%7d",length);
							else
							printf("       ");

						if (!hash_data_has_info(hash, HASH_INFO_NO_DUMP))
						{
							if (hash_data_has_info(hash, HASH_INFO_BAD_DUMP))
								printf(" BAD");

							hash_data_print(hash, 0, buf);
							printf(" %s", buf);
						}
						else
							printf(" NO GOOD DUMP KNOWN");

						printf("\n");
					}
				}
			}
			else
			{
#if (HAS_SAMPLES)
				int k;
				expand_machine_driver(gamedrv->drv, &drv);
				for( k = 0; drv.sound[k].sound_type && k < MAX_SOUND; k++ )
				{
					const char **samplenames = NULL;
					if( drv.sound[k].sound_type == SOUND_SAMPLES )
							samplenames = ((struct Samplesinterface *)drv.sound[k].config)->samplenames;
					if (samplenames != 0 && samplenames[0] != 0)
					{
						i = 0;
						while (samplenames[i] != 0)
						{
							printf("%s\n",samplenames[i]);
							i++;
						}
					}
				}
#endif
			}
			return 0;
			break;

		case LIST_CLONES: /* list clones */
			printf("Name:    Clone of:\n");
			for (i = 0; drivers[i]; i++)
				if (	isclone(i) &&
						(!strwildcmp(gamename,drivers[i]->name)
								|| !strwildcmp(gamename,clone_of->name)))
					printf("%-8s %-8s\n",drivers[i]->name,clone_of->name);
			return 0;
			break;

		case LIST_SOURCEFILE:
			for (i = 0; drivers[i]; i++)
				if (!strwildcmp(gamename,drivers[i]->name))
					printf("%-8s %s\n",drivers[i]->name,drivers[i]->source_file);
			return 0;
			break;

		case LIST_CRC: /* list all crc-32 */
			for (i = 0; drivers[i]; i++)
			{
				const rom_entry *region, *rom;

				for (region = rom_first_region(drivers[i]); region; region = rom_next_region(region))
					for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
					{
						char chksum[256];

						if (hash_data_extract_printable_checksum(ROM_GETHASHDATA(rom), HASH_CRC, chksum))
							printf("%s %-12s %s\n",chksum,ROM_GETNAME(rom),drivers[i]->description);
					}
			}
			return 0;
			break;

		case LIST_XML: /* list all info */
			print_mame_xml( stdout, drivers );
			return 0;
	}

	if (verify)  /* "verify" utilities */
	{
		int err = 0;
		int correct = 0;
		int incorrect = 0;
		int res = 0;
		int total = 0;
		int checked = 0;
		int notfound = 0;


		for (i = 0; drivers[i]; i++)
		{
			if (!strwildcmp(gamename, drivers[i]->name))
				total++;
		}

		for (i = 0; drivers[i]; i++)
		{
			if (strwildcmp(gamename, drivers[i]->name))
				continue;

			if (verify & VERIFY_ROMS)
			{
				res = audit_verify_roms (i, (verify_printf_proc)printf);

				if (res == CLONE_NOTFOUND || res == NOTFOUND)
				{
					notfound++;
					goto nextloop;
				}

				printf ("romset %s ", drivers[i]->name);
				if (isclone(i))
					printf ("[%s] ", clone_of->name);
			}
			if (verify & VERIFY_SAMPLES)
			{
				const char **samplenames = NULL;
				expand_machine_driver(drivers[i]->drv, &drv);
#if (HAS_SAMPLES)
 				for( j = 0; drv.sound[j].sound_type && j < MAX_SOUND; j++ )
 					if( drv.sound[j].sound_type == SOUND_SAMPLES )
 						samplenames = ((struct Samplesinterface *)drv.sound[j].config)->samplenames;
#endif
				/* ignore games that need no samples */
				if (samplenames == 0 || samplenames[0] == 0)
					goto nextloop;

				res = audit_verify_samples (i, (verify_printf_proc)printf);
				if (res == NOTFOUND)
				{
					notfound++;
					goto nextloop;
				}
				printf ("sampleset %s ", drivers[i]->name);
			}

			if (res == NOTFOUND)
			{
				printf ("oops, should never come along here\n");
			}
			else if (res == INCORRECT)
			{
				printf ("is bad\n");
				incorrect++;
			}
			else if (res == CORRECT)
			{
				printf ("is good\n");
				correct++;
			}
			else if (res == BEST_AVAILABLE)
			{
				printf ("is best available\n");
				correct++;
			}
			else if (res == MISSING_OPTIONAL)
			{
				printf ("is missing optional files\n");
				correct++;
			}
			if (res)
				err = res;

nextloop:
			checked++;
			fprintf(stderr,"%d%%\r",100 * checked / total);
		}

		if (correct+incorrect == 0)
		{
			printf ("%s ", (verify & VERIFY_ROMS) ? "romset" : "sampleset" );
			if (notfound > 0)
				printf("\"%8s\" not found!\n",gamename);
			else
				printf("\"%8s\" not supported!\n",gamename);
			return 1;
		}
		else
		{
			printf("%d %s found, %d were OK.\n", correct+incorrect,
					(verify & VERIFY_ROMS)? "romsets" : "samplesets", correct);
			if (incorrect > 0)
				return 2;
			else
				return 0;
		}
		return 0;
	}
	if (ident)
	{
		if (ident == 2) silentident = 1;
		else silentident = 0;

		knownstatus = KNOWN_START;
		identfiles = identmatches = identnonroms = 0;
		romident(filename);
		if (ident == 2)
		{
			switch (knownstatus)
			{
				case KNOWN_START: printf("ERROR     %s\n",gamename); break;
				case KNOWN_ALL:   printf("KNOWN     %s\n",gamename); break;
				case KNOWN_NONE:  printf("UNKNOWN   %s\n",gamename); break;
				case KNOWN_SOME:  printf("PARTKNOWN %s\n",gamename); break;
			}
		}

		if (identmatches == identfiles)
			return 0;
		else if (identmatches == identfiles - identnonroms)
			return 1;
		else if (identmatches > 0)
			return 2;
		else
			return 3;
	}

	/* FIXME: horrible hack to tell that no frontend option was used */
	return 1234;
}
