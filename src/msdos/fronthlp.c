#include "driver.h"
#include "info.h"
#include "audit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <dirent.h>
#include <unzip.h>


/* Mame frontend interface & commandline */
/* parsing rountines by Maurizio Zanello */

extern unsigned int crc32 (unsigned int crc, const unsigned char *buf, unsigned int len);


void get_rom_sample_path (int argc, char **argv, int game_index);

static const struct GameDriver *gamedrv;

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

	return stricmp(s1, s2);
}


/* Identifies a rom from from this checksum */
void identify_rom(const char* name, int checksum)
{
/* Nicola output format */
#if 1
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
	printf("%-12s ",&name[i]);

	for (i = 0; drivers[i]; i++)
	{
		const struct RomModule *romp;

		romp = drivers[i]->rom;

		while (romp->name || romp->offset || romp->length)
		{
			if (romp->name && romp->name != (char *)-1 && checksum == romp->crc)
			{
				if (found != 0)
					printf("             ");
				printf("= %-12s  %s\n",romp->name,drivers[i]->description);
				found++;
			}
			romp++;
		}
	}
	if (found == 0)
		printf("NO MATCH\n");
#else
/* New output format */
	int i;
	printf("%s\n",name);

	for (i = 0; drivers[i]; i++) {
		const struct RomModule *romp;

		romp = drivers[i]->rom;

		while (romp->name || romp->offset || romp->length)
		{
			if (romp->name && romp->name != (char *)-1 && checksum == romp->crc)
			{
				printf("\t%s/%s %s, %s, %s\n",drivers[i]->name,romp->name,
					drivers[i]->description,
					drivers[i]->manufacturer,
					drivers[i]->year);
			}
			romp++;
		}
	}
#endif
}

/* Identifies a file from from this checksum */
void identify_file(const char* name)
{
	FILE *f;
	int length;
	char* data;

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
	data = (char*)malloc(length);
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

	identify_rom(name, crc32(0L,(const unsigned char*)data,length));

	free(data);
}

void identify_zip(const char* zipname)
{
	struct zipent* ent;

	ZIP* zip = openzip( zipname );
	if (!zip)
		return;

	while ((ent = readzip(zip))) {
		/* Skip empty file and directory */
		if (ent->uncompressed_size!=0) {
			char* buf = (char*)malloc(strlen(zipname)+1+strlen(ent->name)+1);
			sprintf(buf,"%s/%s",zipname,ent->name);
			identify_rom(buf,ent->crc32);
			free(buf);
		}
	}

	closezip(zip);
}

void romident(const char* name, int enter_dirs);

void identify_dir(const char* dirname)
{
	DIR *dir;
	struct dirent *ent;

	dir = opendir(dirname);
	if (!dir) {
		return;
	}

	ent = readdir(dir);
	while (ent) {
		/* Skip special files */
		if (ent->d_name[0]!='.') {
			char* buf = (char*)malloc(strlen(dirname)+1+strlen(ent->d_name)+1);
			sprintf(buf,"%s/%s",dirname,ent->d_name);
			romident(buf,0);
			free(buf);
		}

		ent = readdir(dir);
	}
	closedir(dir);
}

void romident(const char* name,int enter_dirs) {
	struct stat s;

	if (stat(name,&s) != 0)	{
		printf("%s: %s\n",name,strerror(errno));
		return;
	}

	if (S_ISDIR(s.st_mode)) {
		if (enter_dirs)
			identify_dir(name);
	} else {
		unsigned l = strlen(name);
		if (l>=4 && stricmp(name+l-4,".zip")==0)
			identify_zip(name);
		else
			identify_file(name);
		return;
	}
}


enum { LIST_LIST = 1, LIST_LISTINFO, LIST_LISTFULL, LIST_LISTSAMDIR, LIST_LISTROMS, LIST_LISTSAMPLES,
		LIST_LMR, LIST_LISTDETAILS, LIST_LISTGAMES, LIST_LISTCLONES,
		LIST_WRONGORIENTATION, LIST_LISTCRC, LIST_LISTDUPCRC };

int frontend_help (int argc, char **argv)
{
	int i, j;
	int list = 0;
	int listclones = 1;
	int verify = 0;
	int ident = 0;
	int help = 1;    /* by default is TRUE */
	char gamename[9];
	extern struct GameDriver neogeo_bios;


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
		if (!stricmp(argv[i],"-list")) list = LIST_LIST;
 		if (!stricmp(argv[i],"-listinfo")) list = LIST_LISTINFO;
		if (!stricmp(argv[i],"-listfull")) list = LIST_LISTFULL;
        if (!stricmp(argv[i],"-listdetails")) list = LIST_LISTDETAILS; /* A detailed MAMELIST.TXT type roms lister */
		if (!stricmp(argv[i],"-listgames")) list = LIST_LISTGAMES;
		if (!stricmp(argv[i],"-listclones")) list = LIST_LISTCLONES;
		if (!stricmp(argv[i],"-listsamdir")) list = LIST_LISTSAMDIR;
		if (!stricmp(argv[i],"-listcrc")) list = LIST_LISTCRC;
		if (!stricmp(argv[i],"-listdupcrc")) list = LIST_LISTDUPCRC;

#ifdef MAME_DEBUG /* do not put this into a public release! */
		if (!stricmp(argv[i],"-lmr")) list = LIST_LMR;
		if (!stricmp(argv[i],"-wrongorientation")) list = LIST_WRONGORIENTATION;
#endif
		if (!stricmp(argv[i],"-noclones")) listclones = 0;

		/* these options REQUIRES gamename field to work */
		if (strlen(gamename) > 0)
		{
			if (!stricmp(argv[i],"-listroms")) list = LIST_LISTROMS;
			if (!stricmp(argv[i],"-listsamples")) list = LIST_LISTSAMPLES;
			if (!stricmp(argv[i],"-verifyroms")) verify = 1;
			if (!stricmp(argv[i],"-verifysamples")) verify = 2;
			if (!stricmp(argv[i],"-romident")) ident = 1;
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
		case LIST_LIST: /* simple games list */
			printf("\nMAME currently supports the following games:\n\n");
			i = 0; j = 0;
			while (drivers[i])
			{
				if ((listclones || drivers[i]->clone_of == 0 || drivers[i]->clone_of == &neogeo_bios) &&
						!strwildcmp(gamename, drivers[i]->name))
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

		case LIST_LISTFULL: /* games list with descriptions */
			printf("Name:     Description:\n");
			i = 0;
			while (drivers[i])
			{
				if ((listclones || drivers[i]->clone_of == 0 || drivers[i]->clone_of == &neogeo_bios) &&
						!strwildcmp(gamename, drivers[i]->name))
					printf("%-10s\"%s\"\n",drivers[i]->name,drivers[i]->description);
				i++;
			}
			return 0;
			break;

		case LIST_LISTSAMDIR: /* games list with samples directories */
			printf("Name:     Samples dir:\n");
			i = 0;
			while (drivers[i])
			{
				if ((listclones || drivers[i]->clone_of == 0 || drivers[i]->clone_of == &neogeo_bios) &&
						!strwildcmp(gamename, drivers[i]->name))
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

		case LIST_LISTROMS: /* game roms list or */
		case LIST_LISTSAMPLES: /* game samples list */
			j = 0;
			while (drivers[j] && (stricmp(gamename,drivers[j]->name) != 0))
				j++;
			if (drivers[j] == 0)
			{
				printf("Game \"%s\" not supported!\n",gamename);
				return 1;
			}
			gamedrv = drivers[j];
			if (list == LIST_LISTROMS)
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

		case LIST_LMR:
			for (i = 0; drivers[i]; i++)
			{
				get_rom_sample_path (argc, argv, i);
				if (RomsetMissing (i))
					printf ("%s\n", drivers[i]->name);
			}
			return 0;
			break;

        case LIST_LISTDETAILS: /* A detailed MAMELIST.TXT type roms lister */

            /* First, we shall print the header */

            printf(" romname  driver      cpu 1   cpu 2   cpu 3   sound 1   sound 2   sound 3   name\n");
            printf("--------  ----------  -----   -----   -----   -------   -------   -------   --------------------------\n");

            /* Let's cycle through the drivers */

            i = 0;

            while (drivers[i])
			{
				if (!strwildcmp(gamename, drivers[i]->name))
				{
					/* Dummy structs to fetch the information from */

					const struct MachineDriver *x_driver = drivers[i]->drv;
					const struct MachineCPU *x_cpu = x_driver->cpu;
					const struct MachineSound *x_sound = x_driver->sound;

					/* First, the rom name */

					printf("%-8s  ",drivers[i]->name);

					/* source file (skip the leading "src/drivers/" */

					printf("%-10s  ",&drivers[i]->source_file[12]);

					/* Then, cpus */

					for(j=0;j<MAX_CPU-1;j++) /* Increase to table to 4, when a game with 4 cpus will appear */
					{
						switch(x_cpu[j].cpu_type & (~CPU_FLAGS_MASK | CPU_AUDIO_CPU))
				{
							case 0:         printf("        "); break;
							case CPU_Z80:   printf("Z80     "); break;
							case CPU_8085A: printf("I8085   "); break;
							case CPU_M6502: printf("M6502   "); break;
							case CPU_I86:   printf("I86     "); break;
							case CPU_I8039: printf("I8039   "); break;
							case CPU_M6803: printf("M6808   "); break;
							case CPU_M6805: printf("M6805   "); break;
							case CPU_M6809: printf("M6809   "); break;
							case CPU_M68000:printf("M68000  "); break;
							case CPU_T11   :printf("T-11    "); break;
							case CPU_S2650 :printf("S2650   "); break;
							case CPU_TMS34010 :printf("TMS34010"); break;
							case CPU_TMS9900:printf("TMS9900"); break;

							case CPU_Z80   |CPU_AUDIO_CPU: printf("[Z80]   "); break; /* Brackets mean that the cpu is only needed for sound. In cpu flags, 0x8000 means it */
							case CPU_8085A |CPU_AUDIO_CPU: printf("[I8085] "); break;
							case CPU_M6502 |CPU_AUDIO_CPU: printf("[M6502] "); break;
							case CPU_I86   |CPU_AUDIO_CPU: printf("[I86]   "); break;
							case CPU_I8039 |CPU_AUDIO_CPU: printf("[I8039] "); break;
							case CPU_M6803 |CPU_AUDIO_CPU: printf("[M6808] "); break;
							case CPU_M6805 |CPU_AUDIO_CPU: printf("[M6805] "); break;
							case CPU_M6809 |CPU_AUDIO_CPU: printf("[M6809] "); break;
							case CPU_M68000|CPU_AUDIO_CPU: printf("[M68000]"); break;
							case CPU_T11   |CPU_AUDIO_CPU: printf("[T-11]  "); break;
							case CPU_S2650 |CPU_AUDIO_CPU: printf("[S2650] "); break;
							case CPU_TMS34010 |CPU_AUDIO_CPU: printf("[TMS34010] "); break;
							case CPU_TMS9900|CPU_AUDIO_CPU: printf("[TMS9900] "); break;
						}
					}

					for(j=0;j<MAX_CPU-1;j++) /* Increase to table to 4, when a game with 4 cpus will appear */
					{

						/* Dummy int to hold the number of specific sound chip.
						   In every multiple-chip interface, number of chips
						   is defined as the first variable, and it is integer. */

						int *x_num = x_sound[j].sound_interface;

						switch(x_sound[j].sound_type)
						{
							case 0: printf("          "); break; /* These don't have a number of chips, only one possible */
							case SOUND_CUSTOM:  printf("Custom    "); break;
							case SOUND_SAMPLES: printf("Samples   "); break;
							case SOUND_NAMCO:   printf("Namco     "); break;
							case SOUND_TMS5220: printf("TMS5520   "); break;
							case SOUND_VLM5030: printf("VLM5030   "); break;

							default:

									/* Let's print out the number of the chips */

									printf("%dx",*x_num);

									/* Then the chip's name */

									switch(x_sound[j].sound_type)
									{
										case SOUND_DAC:        printf("DAC     "); break;
										case SOUND_AY8910:     printf("AY-8910 "); break;
										case SOUND_YM2203:     printf("YM-2203 "); break;
										case SOUND_YM2151:     printf("YM-2151 "); break;
										case SOUND_YM2151_ALT: printf("YM-2151a"); break;
										case SOUND_YM2413:     printf("YM-2413 "); break;
										case SOUND_YM2610:     printf("YM-2610 "); break;
										case SOUND_YM3526:     printf("YM-3526 "); break;
										case SOUND_YM3812:     printf("YM-3812 "); break;
										case SOUND_SN76496:    printf("SN76496 "); break;
										case SOUND_POKEY:      printf("Pokey   "); break;
										case SOUND_NES:        printf("NES     "); break;
										case SOUND_ADPCM:      printf("ADPCM   "); break;
										case SOUND_OKIM6295:   printf("OKI6295 "); break;
										case SOUND_MSM5205:    printf("MSM5205 "); break;
										case SOUND_ASTROCADE:  printf("ASTRCADE"); break;
									}
									break;
						}
					}

					/* Lastly, the name of the game and a \newline */

					printf("%s\n",drivers[i]->description);
				}
                i++;
            }
	    return 0;
            break;

		case LIST_LISTGAMES: /* list games, production year, manufacturer */
			i = 0;
			while (drivers[i])
			{
				if ((listclones || drivers[i]->clone_of == 0 || drivers[i]->clone_of == &neogeo_bios) &&
						!strwildcmp(gamename, drivers[i]->description))
					printf("%-5s%-36s %s\n",drivers[i]->year,drivers[i]->manufacturer,drivers[i]->description);
				i++;
			}
			return 0;
			break;

		case LIST_LISTCLONES: /* list clones */
			printf("Name:    Clone of:\n");
			i = 0;
			while (drivers[i])
			{
				if (drivers[i]->clone_of &&
						(!strwildcmp(gamename,drivers[i]->name) || !strwildcmp(gamename,drivers[i]->clone_of->name)))
					printf("%-8s %-8s\n",drivers[i]->name,drivers[i]->clone_of->name);
				i++;
			}
			return 0;
			break;

		case LIST_WRONGORIENTATION: /* list drivers which incorrectly use the orientation and visible area fields */
			while (drivers[i])
			{
				if ((drivers[i]->drv->video_attributes & VIDEO_TYPE_VECTOR) == 0 &&
						drivers[i]->drv->visible_area.max_x - drivers[i]->drv->visible_area.min_x + 1 <=
						drivers[i]->drv->visible_area.max_y - drivers[i]->drv->visible_area.min_y + 1)
					printf("%s %dx%d\n",drivers[i]->name,
							drivers[i]->drv->visible_area.max_x - drivers[i]->drv->visible_area.min_x + 1,
							drivers[i]->drv->visible_area.max_y - drivers[i]->drv->visible_area.min_y + 1);
				i++;
			}
			return 0;
			break;

		case LIST_LISTCRC: /* list all crc-32 */
			i = 0;
			while (drivers[i])
			{
				const struct RomModule *romp;

				romp = drivers[i]->rom;

				while (romp->name || romp->offset || romp->length)
				{
					if (romp->name && romp->name != (char *)-1)
						printf("%08x %-12s %s\n",romp->crc,romp->name,drivers[i]->description);

					romp++;
				}

				i++;
			}
			return 0;
			break;

		case LIST_LISTDUPCRC: /* list duplicate crc-32 (with different ROM name) */
			i = 0;
			while (drivers[i])
			{
				const struct RomModule *romp;

				romp = drivers[i]->rom;

				while (romp->name || romp->offset || romp->length)
				{
					if (romp->name && romp->name != (char *)-1)
					{
						j = i+1;
						while (drivers[j])
						{
							const struct RomModule *romp1;

							romp1 = drivers[j]->rom;

							while (romp1->name || romp1->offset || romp1->length)
							{
								if (romp1->name && romp1->name != (char *)-1 &&
										strcmp(romp->name,romp1->name) &&
										romp1->crc == romp->crc)
								{
									printf("%08x %-12s %-8s <-> %-12s %-8s\n",romp->crc,
											romp->name,drivers[i]->name,
											romp1->name,drivers[j]->name);
								}

								romp1++;
							}

							j++;
						}
					}

					romp++;
				}

				i++;
			}
			return 0;
			break;

		case LIST_LISTINFO: /* list all info */
			print_mame_info( stdout, drivers );
			return 0;
	}

	if (verify)  /* "verify" utilities */
	{
		int err = 0;
		int correct = 0;
		int incorrect = 0;
		int res = 0;
		int total;

		total = 0;
		for (i = 0; drivers[i]; i++)
		{
			if (!strwildcmp(gamename, drivers[i]->name))
				total++;
		}

		for (i = 0; drivers[i]; i++)
		{
			if (strwildcmp(gamename, drivers[i]->name))
				continue;

			/* set rom and sample path correctly */
			get_rom_sample_path (argc, argv, i);

			/* if using wildcards, ignore games we don't have romsets for. */
			if (!osd_faccess (drivers[i]->name, OSD_FILETYPE_ROM))
			{
				{
					/* if the game is a clone, try loading the ROM from the main version */
					if (drivers[i]->clone_of == 0 ||
							!osd_faccess(drivers[i]->clone_of->name,OSD_FILETYPE_ROM))
						if (stricmp(gamename, drivers[i]->name) != 0) continue;
				}
			}

			if (verify == 1)
			{
				res = VerifyRomSet (i,(verify_printf_proc)printf);

				if (res == CLONE_NOTFOUND) continue;

				if (res != CORRECT)
					printf ("romset %s ", drivers[i]->name);
			}
			if (verify == 2)
			{
				/* ignore games that need no samples */
				if (drivers[i]->samplenames == 0 ||
					drivers[i]->samplenames[0] == 0)
					continue;

				res = VerifySampleSet (i,(verify_printf_proc)printf);
				if (res != CORRECT)
					printf ("sampleset %s ", drivers[i]->name);
			}

			if (res == NOTFOUND)
				printf ("not found\n\n");
			else if (res == INCORRECT)
			{
				printf ("incorrect\n\n");
				incorrect++;
			}
			else
				correct++;
			if (res)
				err = res;

			fprintf(stderr,"%d%%\r",100 * (correct+incorrect) / total);
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
	if (ident)
	{
		for (i = 1;i < argc;i++)
		{
			/* find the FIRST "name" field (without '-') */
			if (argv[i][0] != '-')
			{
				romident(argv[i],1);
				break;
			}
		}
		return 0;
	}

	/* use a special return value if no frontend function used */

	return 1234;
}
