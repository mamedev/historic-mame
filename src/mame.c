#include "driver.h"
#include "strings.h"
#include <time.h>


#if defined (UNIX) || defined (__MWERKS__)
#define uclock_t clock_t
#define	uclock clock
#define UCLOCKS_PER_SEC CLOCKS_PER_SEC
#else
#if defined (WIN32)
#include <windows.h>
LONGLONG osd_win32_uclocks_per_sec();

#define uclock_t DWORDLONG
#define uclock osd_win32_uclock
#define UCLOCKS_PER_SEC osd_win32_uclocks_per_sec()

int strcasecmp(const char *a,const char *b)
{
   return stricmp(a,b);
}
#endif
#endif

static char mameversion[8] = "0.28";   /* M.Z.: current version */

static struct RunningMachine machine;
struct RunningMachine *Machine = &machine;
static const struct GameDriver *gamedrv;
static const struct MachineDriver *drv;

int hiscoreloaded;
char hiscorename[50];



int frameskip;
int throttle = 1;	/* toggled by F10 */
int VolumePTR = 0;
int CurrentVolume = 100;


#define MAX_COLOR_TUPLE 16	/* no more than 4 bits per pixel, for now */
#define MAX_COLOR_CODES 256	/* no more than 256 color codes, for now */


unsigned char *RAM;
unsigned char *ROM;


static unsigned char remappedtable[MAX_COLOR_TUPLE*MAX_COLOR_CODES];


#define DEFAULT_NAME "pacman"


FILE *errorlog;


int init_machine(const char *gamename,int argc,char **argv);
void shutdown_machine(void);
int run_machine(const char *gamename);



int main(int argc,char **argv)
{
	int i,j,list = 0,help,log,success;  /* MAURY_BEGIN: varie */
	char *gamename;

	for (i = 1;i < argc;i++) if (argv[i][0] == '/') argv[i][0] = '-';  /* covert '/' in '-' */

	help = (argc == 1);
	for (i = 1;i < argc;i++)       /* help me, please! */
	{
		if ((stricmp(argv[i],"-?") == 0) || (stricmp(argv[i],"-h") == 0) || (stricmp(argv[i],"-help") == 0))
			help = 1;
	}

	if (help)    /* brief help - useful to get current version info */
	{
		printf("M.A.M.E. v%s - Multiple Arcade Machine Emulator\n"
		       "Copyright (C) 1997  by Nicola Salmoria and the MAME team\n\n",mameversion);
		showdisclaimer();
		printf("Usage:  MAME gamename [options]\n");

		return 0;
	}

	for (i = 1;i < argc;i++)    /* Front end utilities ;) */
	{
		if (stricmp(argv[i],"-list") == 0) list = 1;
		if (stricmp(argv[i],"-listfull") == 0) list = 2;
		if (stricmp(argv[i],"-listroms") == 0) list = 3;
		if (stricmp(argv[i],"-listsamples") == 0) list = 4;
	}

	switch (list)
	{
		case 1:      /* simple games list */
			printf("\nMAME currently supports the following games:\n\n");
			i = 0;
			while (drivers[i])
			{
				printf("%10s",drivers[i]->name);
				i++;
				if (!(i % 7)) printf("\n");
			}
			if (i % 7) printf("\n");
			printf("\nTotal games supported: %4d\n", i);
			return 0;
			break;
		case 2:      /* games list with descriptions */
			printf("NAMES:    DESCRIPTIONS:\n");
			i = 0;
			while (drivers[i])
			{
				printf("%-10s\"%s\"\n",drivers[i]->name,drivers[i]->description);
				i++;
			}
			return 0;
			break;
		case 3:      /* game roms list */
		case 4:      /* game samples list */
			gamename = (argc > 1) && (argv[1][0] != '-') ? argv[1] : DEFAULT_NAME;
			j = 0;
			while (drivers[j] && stricmp(gamename,drivers[j]->name) != 0) j++;
			if (drivers[j] == 0)
			{
				printf("game \"%s\" not supported\n",gamename);
				return 1;
			}
			gamedrv = drivers[j];
			if (list == 3)
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
	}                                          /* MAURY_END: varie */

	success = 1;

	log = 0;
	for (i = 1;i < argc;i++)
	{
		if (stricmp(argv[i],"-log") == 0)
			log = 1;
	}

	if (log) errorlog = fopen("error.log","wa");

	if (init_machine(argc > 1 && argv[1][0] != '-' ? argv[1] : DEFAULT_NAME,argc,argv) == 0)
	{
		if (osd_init(argc,argv) == 0)
		{
			if (run_machine(argc > 1 && argv[1][0] != '-' ? argv[1] : DEFAULT_NAME) == 0)
				success = 0;
			else printf("Unable to start emulation\n");

			osd_exit();
		}
		else printf("Unable to initialize system\n");

		shutdown_machine();
	}
	else printf("Unable to initialize machine emulation\n");

	if (errorlog) fclose(errorlog);

	return success;
}



/***************************************************************************

  Initialize the emulated machine (load the roms, initialize the various
  subsystems...). Returns 0 if successful.

***************************************************************************/
int init_machine(const char *gamename,int argc,char **argv)
{
	int i;
	int nocheat;


	frameskip = 0;
	for (i = 1;i < argc;i++)
	{
		if (stricmp(argv[i],"-frameskip") == 0)
		{
			i++;
			if (i < argc)
			{
				frameskip = atoi(argv[i]);
				if (frameskip < 0) frameskip = 0;
				if (frameskip > 3) frameskip = 3;
			}
		}
	}

	nocheat = 1;
	for (i = 1;i < argc;i++)
	{
		if (stricmp(argv[i],"-cheat") == 0)
			nocheat = 0;
	}


	i = 0;
	while (drivers[i] && stricmp(gamename,drivers[i]->name) != 0)
		i++;

	if (drivers[i] == 0)
	{
		printf("game \"%s\" not supported\n",gamename);
		return 1;
	}

	Machine->gamedrv = gamedrv = drivers[i];
	Machine->drv = drv = gamedrv->drv;


Machine->orientation = gamedrv->orientation;
for (i = 1;i < argc;i++)
{
	if (stricmp(argv[i],"-ror") == 0)
	{
		if (Machine->orientation & ORIENTATION_SWAP_XY)
			Machine->orientation ^= ORIENTATION_ROTATE_180;

		Machine->orientation ^= ORIENTATION_ROTATE_90;
	}
}
for (i = 1;i < argc;i++)
{
	if (stricmp(argv[i],"-rol") == 0)
	{
		if (Machine->orientation & ORIENTATION_SWAP_XY)
			Machine->orientation ^= ORIENTATION_ROTATE_180;

		Machine->orientation ^= ORIENTATION_ROTATE_270;
	}
}
for (i = 1;i < argc;i++)
{
	if (stricmp(argv[i],"-flipx") == 0)
		Machine->orientation ^= ORIENTATION_FLIP_X;
}
for (i = 1;i < argc;i++)
{
	if (stricmp(argv[i],"-flipy") == 0)
		Machine->orientation ^= ORIENTATION_FLIP_Y;
}


	if (gamedrv->new_input_ports)
	{
		int total;
		const struct NewInputPort *from;
		struct NewInputPort *to;

		from = gamedrv->new_input_ports;

		total = 0;
		do
		{
			total++;
		} while ((from++)->type != IPT_END);

		if ((Machine->input_ports = malloc(total * sizeof(struct NewInputPort))) == 0)
			return 1;

		from = gamedrv->new_input_ports;
		to = Machine->input_ports;

		do
		{
			memcpy(to,from,sizeof(struct NewInputPort));
			if (nocheat && (to->type & IPF_CHEAT))
			{
				to->type = IPT_UNUSED;	/* disable cheats */
			}

			to++;
		} while ((from++)->type != IPT_END);
	}

	if (readroms(gamedrv->rom,gamename) != 0)
	{
		free(Machine->input_ports);
		return 1;
	}

	RAM = Machine->memory_region[drv->cpu[0].memory_region];
	ROM = RAM;

	/* decrypt the ROMs if necessary */
	if (gamedrv->rom_decode) (*gamedrv->rom_decode)();

	if (gamedrv->opcode_decode)
	{
		int j;


		/* find the first available memory region pointer */
		j = 0;
		while (Machine->memory_region[j]) j++;

		if ((ROM = malloc(0x10000)) == 0)
		{
			free(Machine->input_ports);
			/* TODO: should also free the allocated memory regions */
			return 1;
		}

		Machine->memory_region[j] = ROM;

		(*gamedrv->opcode_decode)();
	}


	/* read audio samples if available */
        Machine->samples = readsamples(gamedrv->samplenames,gamename);


	/* first of all initialize the memory handlers, which could be used by the */
	/* other initialization routines */
	cpu_init();

	if (drv->vh_init && (*drv->vh_init)(gamename) != 0)
		/* TODO: should also free the resources allocated before */
		return 1;

	if (drv->sh_init && (*drv->sh_init)(gamename) != 0)
		/* TODO: should also free the resources allocated before */
		return 1;

	return 0;
}



void shutdown_machine(void)
{
	int i;


	/* free audio samples */
	freesamples(Machine->samples);
	Machine->samples = 0;

	/* free the memory allocated for ROM and RAM */
	for (i = 0;i < MAX_MEMORY_REGIONS;i++)
	{
		free(Machine->memory_region[i]);
		Machine->memory_region[i] = 0;
	}

	/* free the memory allocated for input ports definition */
	free(Machine->input_ports);
	Machine->input_ports = 0;
}



void vh_close(void)
{
	int i;


	for (i = 0;i < MAX_GFX_ELEMENTS;i++) freegfx(Machine->gfx[i]);
	freegfx(Machine->uifont);
	osd_close_display();
}



int vh_open(void)
{
	int i;
	const unsigned char *palette,*colortable;
	unsigned char convpalette[3 * MAX_PENS];
	unsigned char convtable[MAX_COLOR_TUPLE*MAX_COLOR_CODES];


	for (i = 0;i < MAX_GFX_ELEMENTS;i++) Machine->gfx[i] = 0;
	Machine->uifont = 0;

	/* convert the gfx ROMs into character sets. This is done BEFORE calling the driver's */
	/* convert_color_prom() routine because it might need to check the Machine->gfx[] data */
	if (drv->gfxdecodeinfo)
	{
		for (i = 0;i < MAX_GFX_ELEMENTS && drv->gfxdecodeinfo[i].memory_region != -1;i++)
		{
			if ((Machine->gfx[i] = decodegfx(Machine->memory_region[drv->gfxdecodeinfo[i].memory_region]
					+ drv->gfxdecodeinfo[i].start,
					drv->gfxdecodeinfo[i].gfxlayout)) == 0)
			{
				vh_close();
				return 1;
			}
			Machine->gfx[i]->colortable = &remappedtable[drv->gfxdecodeinfo[i].color_codes_start];
			Machine->gfx[i]->total_colors = drv->gfxdecodeinfo[i].total_color_codes;
		}
	}


	/* convert the palette */
	if (drv->vh_convert_color_prom)
	{
		(*drv->vh_convert_color_prom)(convpalette,convtable,gamedrv->color_prom);
		palette = convpalette;
		colortable = convtable;
	}
	else
	{
		palette = gamedrv->palette;
		colortable = gamedrv->colortable;
	}


	/* create the display bitmap, and allocate the palette */
	if ((Machine->scrbitmap = osd_create_display(drv->screen_width,drv->screen_height,drv->video_attributes)) == 0)
		return 1;

	for (i = 0;i < drv->total_colors;i++)
		Machine->pens[i] = osd_obtain_pen(palette[3*i],palette[3*i+1],palette[3*i+2]);

	for (i = 0;i < drv->color_table_len;i++)
		remappedtable[i] = Machine->pens[colortable[i]];


	/* build our private user interface font */
	if ((Machine->uifont = builduifont()) == 0)
	{
		vh_close();
		return 1;
	}


	/* free the graphics ROMs, they are no longer needed */
	free(Machine->memory_region[1]);
	Machine->memory_region[1] = 0;

	return 0;
}



/***************************************************************************

  This function takes care of refreshing the screen, processing user input,
  and throttling the emulation speed to obtain the required frames per second.

***************************************************************************/
int updatescreen(void)
{
	static int framecount = 0, showfpstemp = 0, showvoltemp = 0; /* M.Z.: new options */


	/* read hi scores from disk */
	if (hiscoreloaded == 0 && gamedrv->hiscore_load)
		hiscoreloaded = (*gamedrv->hiscore_load)(hiscorename);

	/* if the user pressed ESC, stop the emulation */
	if (osd_key_pressed(OSD_KEY_ESC)) return 1;

	/* if the user pressed F3, reset the emulation */
	if (osd_key_pressed(OSD_KEY_F3))
	{
		machine_reset();
	}

        if (osd_key_pressed(OSD_KEY_F9)) {
                if (++VolumePTR > 4) VolumePTR = 0;
		CurrentVolume = (4-VolumePTR)*25;  /* M.Z.: array no more needed */
                osd_set_mastervolume(CurrentVolume);
		while (osd_key_pressed(OSD_KEY_F9)) {
                  if (drv->sh_update) {
		     (*drv->sh_update)();	/* update sound */
		     osd_update_audio();
                  }
                }
		showvoltemp = 50;    /* M.Z.: new options */
        }

	if (osd_key_pressed(OSD_KEY_F8))           /* MAURY_BEGIN: New_options */
	{                                          /* change frameskip */
		if (frameskip < 3) frameskip++;
		else frameskip = 0;
		while (osd_key_pressed(OSD_KEY_F8));
		showfpstemp = 50;
	}

	if (osd_key_pressed(OSD_KEY_MINUS_PAD))     /* decrease volume */
	{
		if (CurrentVolume > 0) CurrentVolume--;
		osd_set_mastervolume(CurrentVolume);
		showvoltemp = 50;
	}

	if (osd_key_pressed(OSD_KEY_PLUS_PAD))      /* increase volume */
	{
		if (CurrentVolume < 100) CurrentVolume++;
		osd_set_mastervolume(CurrentVolume);
		showvoltemp = 50;
	}                                          /* MAURY_END: new options */

	if (osd_key_pressed(OSD_KEY_P)) /* pause the game */
	{
		struct DisplayText dt[2];


		dt[0].text = "PAUSED";
		dt[0].color = DT_COLOR_RED;
		dt[0].x = (Machine->scrbitmap->width - Machine->uifont->width * strlen(dt[0].text)) / 2;
		dt[0].y = (Machine->scrbitmap->height - Machine->uifont->height) / 2;
		dt[1].text = 0;
		displaytext(dt,0);

                osd_set_mastervolume(0);

		while (osd_key_pressed(OSD_KEY_P))
			osd_update_audio();	/* give time to the sound hardware to apply the volume change */

		while (osd_key_pressed(OSD_KEY_P) == 0 && osd_key_pressed(OSD_KEY_ESC) == 0)
		{
			if (osd_key_pressed(OSD_KEY_TAB)) setup_menu();	/* call the configuration menu */

			clearbitmap(Machine->scrbitmap);
			(*drv->vh_update)(Machine->scrbitmap);	/* redraw screen */

			if (uclock() % UCLOCKS_PER_SEC < UCLOCKS_PER_SEC/2)
				displaytext(dt,0);	/* make PAUSED blink */
			osd_update_display();
		}

		while (osd_key_pressed(OSD_KEY_ESC));	/* wait for jey release */
		while (osd_key_pressed(OSD_KEY_P));	/* ditto */

			osd_set_mastervolume(CurrentVolume);
			clearbitmap(Machine->scrbitmap);
	}

	/* if the user pressed TAB, go to the setup menu */
	if (osd_key_pressed(OSD_KEY_TAB))
	{
		osd_set_mastervolume(0);

		while (osd_key_pressed(OSD_KEY_TAB))
			osd_update_audio();	/* give time to the sound hardware to apply the volume change */

		if (setup_menu()) return 1;

		osd_set_mastervolume(CurrentVolume);
	}

	/* if the user pressed F4, show the character set */
	if (osd_key_pressed(OSD_KEY_F4))
	{
                osd_set_mastervolume(0);

		while (osd_key_pressed(OSD_KEY_F4))
			osd_update_audio();	/* give time to the sound hardware to apply the volume change */

		if (showcharset()) return 1;

		osd_set_mastervolume(CurrentVolume);
	}


	if (++framecount > frameskip)
	{
		static int showfps,f11pressed;
		static int f10pressed;
		uclock_t curr;
		#define MEMORY 10
		static uclock_t prev[MEMORY];
		static int i,speed;


		framecount = 0;

		if (osd_key_pressed(OSD_KEY_F11))
		{
			if (f11pressed == 0)
			{
				showfps ^= 1;
				if (showfps == 0) clearbitmap(Machine->scrbitmap);
			}
			f11pressed = 1;
		}
		else f11pressed = 0;

		if (showfpstemp)         /* MAURY_BEGIN: nuove opzioni */
		{
			showfpstemp--;
			if ((showfps == 0) && (showfpstemp == 0)) clearbitmap(Machine->scrbitmap);
		}

		if (showvoltemp)
		{
			showvoltemp--;
			if (!showvoltemp) clearbitmap(Machine->scrbitmap);
		}                        /* MAURY_END: nuove opzioni */

		if (osd_key_pressed(OSD_KEY_F10))
		{
			if (f10pressed == 0) throttle ^= 1;
			f10pressed = 1;
		}
		else f10pressed = 0;


		(*drv->vh_update)(Machine->scrbitmap);	/* update screen */

		if (showfps || showfpstemp) /* MAURY: nuove opzioni */
		{
			int trueorientation;
			int fps,i,l;
			char buf[30];


			/* hack: force the display into standard orientation to avoid */
			/* rotating the text */
			trueorientation = Machine->orientation;
			Machine->orientation = ORIENTATION_DEFAULT;

			fps = (Machine->drv->frames_per_second * speed + 50) / 100;
			sprintf(buf," %d%% (%-3dfps)",speed,fps);
			l = strlen(buf);
			for (i = 0;i < l;i++)
				drawgfx(Machine->scrbitmap,Machine->uifont,buf[i],DT_COLOR_WHITE,0,0,Machine->scrbitmap->width-(l-i)*Machine->uifont->width,0,0,TRANSPARENCY_NONE,0);

			Machine->orientation = trueorientation;
		}

		if (showvoltemp)      /* MAURY_BEGIN: nuove opzioni */
		{                     /* volume-meter */
			int i,x;
			char volstr[25];
			x = (drv->screen_width - 24*Machine->uifont->width)/2;
			strcpy(volstr,"                      ");
			for (i = 0;i < (CurrentVolume/5);i++) volstr[i+1] = '\x15';

			drawgfx(Machine->scrbitmap,Machine->uifont,16,DT_COLOR_RED,0,0,x,drv->screen_height/2,0,TRANSPARENCY_NONE,0);
			drawgfx(Machine->scrbitmap,Machine->uifont,17,DT_COLOR_RED,0,0,x+23*Machine->uifont->width,drv->screen_height/2,0,TRANSPARENCY_NONE,0);
			for (i = 0;i < 22;i++)
			    drawgfx(Machine->scrbitmap,Machine->uifont,volstr[i],DT_COLOR_WHITE,
                                        0,0,x+(i+1)*Machine->uifont->width,drv->screen_height/2,0,TRANSPARENCY_NONE,0);
		}                     /* MAURY_END: nuove opzioni */

		osd_poll_joystick();

		/* now wait until it's time to trigger the interrupt */
		do
		{
			curr = uclock();
		} while (throttle != 0 && video_sync == 0 && (curr - prev[i]) < (frameskip+1) * UCLOCKS_PER_SEC/drv->frames_per_second);

		osd_update_display();

		i = (i+1) % MEMORY;

		if (curr - prev[i])
		{
			int divdr = drv->frames_per_second * (curr - prev[i]) / (100 * MEMORY);


			speed = (UCLOCKS_PER_SEC * (frameskip+1) + divdr/2) / divdr;
		}

		prev[i] = curr;
	}

	/* update audio. Do it after the speed throttling to be in better sync. */
	if (drv->sh_update)
	{
		(*drv->sh_update)();
		osd_update_audio();
	}


	return 0;
}



/***************************************************************************

  Run the emulation. Start the various subsystems and the CPU emulation.
  Returns non zero in case of error.

***************************************************************************/
int run_machine(const char *gamename)
{
	int res = 1;


	if (vh_open() == 0)
	{
		if (drv->vh_start == 0 || (*drv->vh_start)() == 0)	/* start the video hardware */
		{
			if (drv->sh_start == 0 || (*drv->sh_start)() == 0)	/* start the audio hardware */
			{
				unsigned int i;
				struct DisplayText dt[2];


				dt[0].text = "PLEASE DO NOT DISTRIBUTE THE SOURCE CODE AND/OR THE EXECUTABLE "
						"APPLICATION WITH ANY ROM IMAGES.\n"
						"DOING AS SUCH WILL HARM ANY FURTHER DEVELOPMENT OF MAME AND COULD "
						"RESULT IN LEGAL ACTION BEING TAKEN BY THE LAWFUL COPYRIGHT HOLDERS "
						"OF ANY ROM IMAGES.\n\n"
						"IF YOU DO NOT AGREE WITH THESE CONDITIONS THEN PLEASE PRESS ESC NOW.";

				dt[0].color = DT_COLOR_RED;
				dt[0].x = 0;
				dt[0].y = 0;
				dt[1].text = 0;
				displaytext(dt,1);

				i = osd_read_key();
				while (osd_key_pressed(i));	        /* wait for key release */
				if (i != OSD_KEY_ESC)
				{
					char cfgname[100];


					showcredits();	/* show the driver credits */

					clearbitmap(Machine->scrbitmap);
					osd_update_display();


					sprintf(cfgname,"%s/%s.cfg",gamename,gamename);
					/* load input ports settings (keys, dip switches, and so on) */
					load_input_port_settings(cfgname);

					/* we have to load the hi scores, but this will be done while */
					/* the game is running */
					hiscoreloaded = 0;
					sprintf(hiscorename,"%s/%s.hi",gamename,gamename);

					cpu_run();	/* run the emulation! */

					if (drv->sh_stop) (*drv->sh_stop)();
					if (drv->vh_stop) (*drv->vh_stop)();

					/* write hi scores to disk */
					if (hiscoreloaded != 0 && gamedrv->hiscore_save)
						(*gamedrv->hiscore_save)(hiscorename);

					/* save input ports settings */
					save_input_port_settings(cfgname);
				}

				res = 0;
			}
			else printf("Unable to start audio emulation\n");
		}
		else printf("Unable to start video emulation\n");

		vh_close();
	}
	else printf("Unable to initialize display\n");

	return res;
}
