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

static char mameversion[8] = "0.27";   /* M.Z.: current version */

static struct RunningMachine machine;
struct RunningMachine *Machine = &machine;
static const struct GameDriver *gamedrv;
static const struct MachineDriver *drv;

static int hiscoreloaded;
static char hiscorename[50];


struct KEYSet *MM_DirectKEYSet;
unsigned char MM_PatchedPort;
unsigned char MM_OldPortValue=0;
unsigned char MM_LastChangedValue=0;
unsigned char MM_updir=1;
unsigned char MM_leftdir=1;
unsigned char MM_rightdir=1;
unsigned char MM_downdir=1;
unsigned char MM_orizdir=1;     /* Minimum between ldir and rdir */
unsigned char MM_lrdir=1;
unsigned char MM_totale;
unsigned char MM_inverted=0;
unsigned char MM_dir4=0;


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
		       "Copyright (C) 1997  by Nicola Salmoria and Mirko Buffoni\n\n",mameversion);
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

	if (readroms(gamedrv->rom,gamename) != 0)
		return 1;

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
			return 1;

		Machine->memory_region[j] = ROM;

		(*gamedrv->opcode_decode)();
	}


	/* read audio samples if available */
        Machine->samples = readsamples(gamedrv->samplenames,gamename);


	/* first of all initialize the memory handlers, which could be used by the */
	/* other initialization routines */
	cpu_init();

	if (*drv->init_machine && (*drv->init_machine)(gamename) != 0)
		return 1;

	if (*drv->vh_init && (*drv->vh_init)(gamename) != 0)
		return 1;

	if (*drv->sh_init && (*drv->sh_init)(gamename) != 0)
		return 1;

        for (i = 1;i < argc;i++) if (stricmp(argv[i],"-dir4") == 0) MM_dir4 = 1;

        if (MM_dir4)
        {
                MM_PatchedPort = Machine->gamedrv->keysettings[0].num;
                MM_updir <<= (Machine->gamedrv->keysettings[0].mask);
                MM_leftdir <<= (Machine->gamedrv->keysettings[1].mask);
                MM_rightdir <<= (Machine->gamedrv->keysettings[2].mask);
                MM_downdir <<= (Machine->gamedrv->keysettings[3].mask);
                MM_lrdir = MM_leftdir + MM_rightdir;
                MM_totale = MM_lrdir + MM_updir + MM_downdir;
                MM_orizdir = MM_leftdir;
                if (MM_rightdir < MM_orizdir) MM_orizdir = MM_rightdir;
                if (Machine->gamedrv->input_ports[MM_PatchedPort].default_value == 0xff) MM_inverted = 1;
        };

	return 0;
}



void shutdown_machine(void)
{
	int i;


	/* free audio samples */
	freesamples(Machine->samples);

	/* free the memory allocated for ROM and RAM */
	for (i = 0;i < MAX_MEMORY_REGIONS;i++)
	{
		free(Machine->memory_region[i]);
		Machine->memory_region[i] = 0;
	}
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


	if ((Machine->scrbitmap = osd_create_display(drv->screen_width,drv->screen_height)) == 0)
		return 1;

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

	for (i = 0;i < drv->total_colors;i++)
		Machine->pens[i] = osd_obtain_pen(palette[3*i],palette[3*i+1],palette[3*i+2]);

	for (i = 0;i < drv->color_table_len;i++)
		remappedtable[i] = Machine->pens[colortable[i]];


	for (i = 0;i < MAX_GFX_ELEMENTS;i++) Machine->gfx[i] = 0;
	Machine->uifont = 0;

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

	if ((Machine->uifont = builduifont(drv->total_colors,palette,Machine->pens)) == 0)
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
	if (hiscoreloaded == 0 && *gamedrv->hiscore_load)
		hiscoreloaded = (*gamedrv->hiscore_load)(hiscorename);

	/* if the user pressed ESC, stop the emulation */
	if (osd_key_pressed(OSD_KEY_ESC)) return 1;

	/* if the user pressed F3, reset the emulation */
	if (osd_key_pressed(OSD_KEY_F3))
	{
		/* write hi scores to disk */
		if (hiscoreloaded != 0 && *gamedrv->hiscore_save)
			(*gamedrv->hiscore_save)(hiscorename);
		hiscoreloaded = 0;

		return 2;
	}

        if (osd_key_pressed(OSD_KEY_F9)) {
                if (++VolumePTR > 4) VolumePTR = 0;
		CurrentVolume = (4-VolumePTR)*25;  /* M.Z.: array no more needed */
                osd_set_mastervolume(CurrentVolume);
		while (osd_key_pressed(OSD_KEY_F9)) {
                  if (*drv->sh_update) {
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
		int key;


		dt[0].text = "PAUSED";
		dt[0].color = DT_COLOR_RED;
		dt[0].x = gamedrv->paused_x;
		dt[0].y = gamedrv->paused_y;
		dt[1].text = 0;
		displaytext(dt,0);

                osd_set_mastervolume(0);

		while (osd_key_pressed(OSD_KEY_P))
			osd_update_audio();	/* give time to the sound hardware to apply the volume change */

		do
		{
			key = osd_read_key();

			/* if (key == OSD_KEY_ESC) return 1;
			else */                                /* Do not quit with ESC */
                        if (key == OSD_KEY_TAB)
			{
				if (setup_menu()) return 1;
				(*drv->vh_update)(Machine->scrbitmap);	/* redraw screen */
				displaytext(dt,0);
			}
		} while ((key == 0) || (key == OSD_KEY_TAB) || (key == OSD_KEY_ALT));    /* MAURY_BEGIN: any key restarts */
		if ((key == OSD_KEY_ESC) || (key == OSD_KEY_P))
			while (osd_key_pressed(key));            /* MAURY_END: Filter ESC or P */

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
		uclock_t curr,mtpf;
		#define MEMORY 10
		static uclock_t prev[MEMORY];
		static int i,fps;


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
			if (!showfpstemp) clearbitmap(Machine->scrbitmap);
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
			drawgfx(Machine->scrbitmap,Machine->uifont,(fps%1000)/100+'0',DT_COLOR_WHITE,0,0,0,0,0,TRANSPARENCY_NONE,0);
			drawgfx(Machine->scrbitmap,Machine->uifont,(fps%100)/10+'0',DT_COLOR_WHITE,0,0,Machine->uifont->width,0,0,TRANSPARENCY_NONE,0);
			drawgfx(Machine->scrbitmap,Machine->uifont,fps%10+'0',DT_COLOR_WHITE,0,0,2*Machine->uifont->width,0,0,TRANSPARENCY_NONE,0);
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

		mtpf = ((curr - prev[i])/(MEMORY))/2;
		if (mtpf) fps = (UCLOCKS_PER_SEC+mtpf)/2/mtpf;

		prev[i] = curr;
	}

	/* update audio. Do it after the speed throttling to be in better sync. */
	if (*drv->sh_update)
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
	float temp;


	if (vh_open() == 0)
	{
		if (*drv->vh_start == 0 || (*drv->vh_start)() == 0)	/* start the video hardware */
		{
			if (*drv->sh_start == 0 || (*drv->sh_start)() == 0)	/* start the audio hardware */
			{
				FILE *f;
				char name[100];
				unsigned int i,j;
				unsigned int len,incount,keycount,trakcount;
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
					clearbitmap(Machine->scrbitmap);	/* initialize the bitmap to the correct background color */
					osd_update_display();


					incount = 0;
					while (gamedrv->input_ports[incount].default_value != -1) incount++;

					keycount = 0;
					while (gamedrv->keysettings[keycount].num != -1) keycount++;

									trakcount = 0;
									while (gamedrv->trak_ports[trakcount].axis != -1) trakcount++;

									/* find the configuration file */
									sprintf(name,"%s/%s.cfg",gamename,gamename);
									if ((f = fopen(name,"rb")) != 0)
									{
									  for (j=0; j < 4; j++)
									  {
										 if ((len = fread(name,1,4,f)) == 4)
										 {
											len = (unsigned char)name[3];
											name[3] = '\0';
											if (stricmp(name,"dsw") == 0) {
											  if ((len == incount) && (fread(name,1,incount,f) == incount))
						  {
							  for (i = 0;i < incount;i++)
								  gamedrv->input_ports[i].default_value = ((unsigned char)name[i]);
						  }
											} else if (stricmp(name,"key") == 0) {
											  if ((len == keycount) && (fread(name,1,keycount,f) == keycount))
						  {
							  for (i = 0;i < keycount;i++)
								  gamedrv->input_ports[ gamedrv->keysettings[i].num ].keyboard[ gamedrv->keysettings[i].mask ] = ((unsigned char)name[i]);
						  }
											} else if (stricmp(name,"joy") == 0) {
											  if ((len == keycount) && (fread(name,1,keycount,f) == keycount))
						  {
							  for (i = 0;i < keycount;i++)
								  gamedrv->input_ports[ gamedrv->keysettings[i].num ].joystick[ gamedrv->keysettings[i].mask ] = ((unsigned char)name[i]);
											  }
											} else if (stricmp(name,"trk") == 0) {
											  if ((len == trakcount) && (fread(name,sizeof(float),trakcount,f) == trakcount))
						  {
							  for (i = 0;i < trakcount;i++) {
														  memcpy(&temp, &name[i*sizeof(float)], sizeof(float));
								  gamedrv->trak_ports[i].scale = temp;
													  }
											  }
						}
										 }
									  }
									  fclose(f);
									}
									else
									{
					   /* read dipswitch settings from disk */
					   sprintf(name,"%s/%s.dsw",gamename,gamename);
					   if ((f = fopen(name,"rb")) != 0)
					   {
						   /* use name as temporary buffer */
						   if (fread(name,1,incount,f) == incount)
						   {
							   for (i = 0;i < incount;i++)
								   gamedrv->input_ports[i].default_value = ((unsigned char)name[i]);
						   }
						   fclose(f);
					   }

					   /* read keysettings from disk */
					   sprintf(name,"%s/%s.key",gamename,gamename);
					   if ((f = fopen(name,"rb")) != 0)
					   {
						   /* use name as temporary buffer */
						   if (fread(name,1,keycount,f) == keycount)
						   {
							   for (i = 0;i < keycount;i++)
								   gamedrv->input_ports[ gamedrv->keysettings[i].num ].keyboard[ gamedrv->keysettings[i].mask ] = ((unsigned char)name[i]);
						   }
						   fclose(f);
					   }


					   /* read joystick settings from disk */
					   sprintf(name,"%s/%s.joy",gamename,gamename);
					   if ((f = fopen(name,"rb")) != 0)
					   {
						   /* use name as temporary buffer */
						   if (fread(name,1,keycount,f) == keycount)
						   {
							   for (i = 0;i < keycount;i++)
								   gamedrv->input_ports[ gamedrv->keysettings[i].num ].joystick[ gamedrv->keysettings[i].mask ] = ((unsigned char)name[i]);
						   }
						   fclose(f);
					   }
									}

					/* we have to load the hi scores, but this will be done while */
					/* the game is running */
					hiscoreloaded = 0;
					sprintf(hiscorename,"%s/%s.hi",gamename,gamename);

					cpu_run();	/* run the emulation! */

					if (*drv->sh_stop) (*drv->sh_stop)();
					if (*drv->vh_stop) (*drv->vh_stop)();

					/* write hi scores to disk */
					if (hiscoreloaded != 0 && *gamedrv->hiscore_save)
						(*gamedrv->hiscore_save)(hiscorename);

									/* write the configuration file */
									sprintf(name,"%s/%s.cfg",gamename,gamename);
									if ((f = fopen(name,"wb")) != 0)
									{
											sprintf(name, "dsw ");
											name[3] = incount;
											fwrite(name,1,4,f);
						/* use name as temporary buffer */
						for (i = 0;i < incount;i++)
							name[i] = gamedrv->input_ports[i].default_value;
						fwrite(name,1,incount,f);

											sprintf(name, "key ");
											name[3] = keycount;
											fwrite(name,1,4,f);
						/* use name as temporary buffer */
						for (i = 0;i < keycount;i++)
							name[i] = gamedrv->input_ports[ gamedrv->keysettings[i].num ].keyboard[ gamedrv->keysettings[i].mask ];
						fwrite(name,1,keycount,f);

											sprintf(name, "joy ");
											name[3] = keycount;
											fwrite(name,1,4,f);
						/* use name as temporary buffer */
						for (i = 0;i < keycount;i++)
							name[i] = gamedrv->input_ports[ gamedrv->keysettings[i].num ].joystick[ gamedrv->keysettings[i].mask ];
						fwrite(name,1,keycount,f);

											sprintf(name, "trk ");
											name[3] = trakcount;
											fwrite(name,1,4,f);
						/* use name as temporary buffer */
						for (i = 0;i < trakcount;i++)
												memcpy(&name[i*sizeof(float)], &gamedrv->trak_ports[i].scale, sizeof(float));
						fwrite(name,sizeof(float),trakcount,f);
											fclose(f);
									}
#if 0
					/* write dipswitch settings to disk */
					sprintf(name,"%s/%s.dsw",gamename,gamename);
					if ((f = fopen(name,"wb")) != 0)
					{
						/* use name as temporary buffer */
						for (i = 0;i < incount;i++)
							name[i] = gamedrv->input_ports[i].default_value;

						fwrite(name,1,incount,f);
						fclose(f);
					}

					/* write key settings to disk */
					sprintf(name,"%s/%s.key",gamename,gamename);
					if ((f = fopen(name,"wb")) != 0)
					{
						/* use name as temporary buffer */
						for (i = 0;i < keycount;i++)
							name[i] = gamedrv->input_ports[ gamedrv->keysettings[i].num ].keyboard[ gamedrv->keysettings[i].mask ];

						fwrite(name,1,keycount,f);
						fclose(f);
					}

					/* write joy settings to disk */
					sprintf(name,"%s/%s.joy",gamename,gamename);
					if ((f = fopen(name,"wb")) != 0)
					{
						/* use name as temporary buffer */
						for (i = 0;i < keycount;i++)
							name[i] = gamedrv->input_ports[ gamedrv->keysettings[i].num ].joystick[ gamedrv->keysettings[i].mask ];

						fwrite(name,1,keycount,f);
						fclose(f);
					}
#endif
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
