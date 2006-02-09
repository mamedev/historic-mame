//============================================================
//
//  fronthlp.c - Win32 frontend management functions
//
//  Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

#include <windows.h>
#include "driver.h"
#include "info.h"
#include "audit.h"
#include "rc.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unzip.h>
#include <zlib.h>
#include <tchar.h>
#include "sound/samples.h"

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

struct rc_option frontend_opts[] = {
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


static int silentident,knownstatus;

void get_rom_sample_path (int argc, char **argv, int game_index, char *override_default_rompath);

static const game_driver *gamedrv;

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


/* Identifies a rom from from this checksum */
static void match_roms(const game_driver *driver,const char* hash,int *found)
{
	const rom_entry *region, *rom;

	for (region = rom_first_region(driver); region; region = rom_next_region(region))
	{
		for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
		{
			if (hash_data_is_equal(hash, ROM_GETHASHDATA(rom), 0))
			{
				char baddump = hash_data_has_info(ROM_GETHASHDATA(rom), HASH_INFO_BAD_DUMP);

				if (!silentident)
				{
					if (*found != 0)
						printf("             ");
					printf("= %s%-12s  %s\n",baddump ? "(BAD) " : "",ROM_GETNAME(rom),driver->description);
				}
				(*found)++;
			}
		}
	}
}


void identify_rom(const char* name, const char* hash, int length)
{
	int found = 0;

	/* remove directory name */
	int i;
	for (i = strlen(name)-1;i >= 0;i--)
	{
		if (name[i] == '/' || name[i] == '\\')
		{
			i++;
			break;
		}
	}
	if (!silentident)
		printf("%s ",&name[0]);

	for (i = 0; drivers[i]; i++)
		match_roms(drivers[i],hash,&found);

	if (found == 0)
	{
		unsigned size = length;
		while (size && (size & 1) == 0) size >>= 1;
		if (size & ~1)
		{
			if (!silentident)
				printf("NOT A ROM\n");
		}
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
	else
	{
		if (knownstatus == KNOWN_START)
			knownstatus = KNOWN_ALL;
		else if (knownstatus == KNOWN_NONE)
			knownstatus = KNOWN_SOME;
	}
}

/* Identifies a file from this checksum */
void identify_file(const char* name)
{
	FILE *f;
	int length;
	unsigned char* data;
	char hash[HASH_BUF_SIZE];

	f = fopen(name,"rb");
	if (!f) {
		return;
	}

	/* determine length of file */
	if (fseek (f, 0L, SEEK_END)!=0)	{
		fclose(f);
		return;
	}

	length = ftell(f);
	if (length == -1L) {
		fclose(f);
		return;
	}

	/* empty file */
	if (!length) {
		fclose(f);
		return;
	}

	/* allocate space for entire file */
	data = (unsigned char*)malloc(length);
	if (!data) {
		fclose(f);
		return;
	}

	if (fseek (f, 0L, SEEK_SET)!=0) {
		free(data);
		fclose(f);
		return;
	}

	if (fread(data, 1, length, f) != length) {
		free(data);
		fclose(f);
		return;
	}

	fclose(f);

	/* Compute checksum of all the available functions. Since MAME for
       now carries inforamtions only for CRC and SHA1, we compute only
       these */
	hash_compute(hash, data, length, HASH_CRC|HASH_SHA1);

	/* Try to identify the ROM */
	identify_rom(name, hash, length);

	free(data);
}

void identify_zip(const char* zipname)
{
	zip_entry* ent;

	zip_file* zip = openzip( FILETYPE_RAW, 0, zipname );
	if (!zip)
		return;

	while ((ent = readzip(zip))) {
		/* Skip empty file and directory */
		if (ent->uncompressed_size!=0) {
			char* buf = (char*)malloc(strlen(zipname)+1+strlen(ent->name)+1);
			char hash[HASH_BUF_SIZE];
			UINT8 crcs[4];

//          sprintf(buf,"%s/%s",zipname,ent->name);
			sprintf(buf,"%-12s",ent->name);

			/* Decompress the ROM from the ZIP, and compute all the needed
               checksums. Since MAME for now carries informations only for CRC and
               SHA1, we compute only these (actually, CRC is extracted from the
               ZIP header) */
			hash_data_clear(hash);

			{
				UINT8* data =  (UINT8*)malloc(ent->uncompressed_size);
				readuncompresszip(zip, ent, data);
				hash_compute(hash, data, ent->uncompressed_size, HASH_SHA1);
				free(data);
			}

			crcs[0] = (UINT8)(ent->crc32 >> 24);
			crcs[1] = (UINT8)(ent->crc32 >> 16);
			crcs[2] = (UINT8)(ent->crc32 >> 8);
			crcs[3] = (UINT8)(ent->crc32 >> 0);
			hash_data_insert_binary_checksum(hash, HASH_CRC, crcs);

			/* Try to identify the ROM */
			identify_rom(buf, hash, ent->uncompressed_size);

			free(buf);
		}
	}

	closezip(zip);
}

void romident(const char* name, int enter_dirs);

void identify_dir(const char* dirname)
{
	HANDLE dir;
	WIN32_FIND_DATA ent;
	int more;
	char *dirfilter;
	int dirlen = strlen(dirname);

	memset(&ent, 0, sizeof(WIN32_FIND_DATA));
	dirfilter = malloc(dirlen+5);
	if (dirfilter == NULL)
		return;
	memcpy(dirfilter, dirname, dirlen);
	memcpy(dirfilter+dirlen, "/*.*", 5);

    dir = FindFirstFile(dirfilter, &ent);
    free(dirfilter);

    if (INVALID_HANDLE_VALUE == dir) {
        return;
    }

    do {
        /* Skip special files */
		if (ent.cFileName[0] != '.') {
			char* buf = (char*)malloc(strlen(dirname)+1+strlen(ent.cFileName)+1);
			sprintf(buf,"%s/%s",dirname,ent.cFileName);
			romident(buf,0);
			free(buf);
		}

		more = FindNextFileA(dir, &ent);
	}
	while (more);
	FindClose(dir);
}

void romident(const char* name,int enter_dirs)
{
	DWORD attr;
#ifdef UNICODE
	WCHAR tname[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, name, -1, tname, sizeof(tname) / sizeof(tname[0]));
#else
	const char *tname = name;
#endif
	TCHAR error[256];

	attr = GetFileAttributes(tname);
	if (attr == INVALID_FILE_ATTRIBUTES)
	{
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0,
			error, sizeof(error) / sizeof(error[0]), NULL);
		_tprintf("%s: %s\n",name, error);
		return;
	}

	if (attr & FILE_ATTRIBUTE_DIRECTORY)
	{
		if (enter_dirs)
			identify_dir(name);
	}
	else
	{
		unsigned l = strlen(name);
		if (l>=4 && mame_stricmp(name+l-4,".zip")==0)
			identify_zip(name);
		else
			identify_file(name);
		return;
	}
}


int CLIB_DECL compare_names(const void *elem1, const void *elem2)
{
	int cmp;
	game_driver *drv1 = *(game_driver **)elem1;
	game_driver *drv2 = *(game_driver **)elem2;
	char name1[200],name2[200];
	namecopy(name1,drv1->description);
	namecopy(name2,drv2->description);
	cmp = mame_stricmp(name1,name2);
	if (cmp == 0)
		cmp = mame_stricmp(drv1->description, drv2->description);
	return cmp;
}


int CLIB_DECL compare_driver_names(const void *elem1, const void *elem2)
{
	game_driver *drv1 = *(game_driver **)elem1;
	game_driver *drv2 = *(game_driver **)elem2;
	return strcmp(drv1->name, drv2->name);
}


int frontend_help (const char *gamename)
{
	machine_config drv;
	int i, j;
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
		showdisclaimer();
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
				if (!strwildcmp(gamename, drivers[i]->name))
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
				if (drivers[i]->clone_of && !(drivers[i]->clone_of->flags & NOT_A_DRIVER) &&
						(!strwildcmp(gamename,drivers[i]->name)
								|| !strwildcmp(gamename,drivers[i]->clone_of->name)))
					printf("%-8s %-8s\n",drivers[i]->name,drivers[i]->clone_of->name);
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
				if (drivers[i]->clone_of && !(drivers[i]->clone_of->flags & NOT_A_DRIVER))
					printf ("[%s] ", drivers[i]->clone_of->name);
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
		romident(gamename,1);
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
		return 0;
	}

	/* FIXME: horrible hack to tell that no frontend option was used */
	return 1234;
}
