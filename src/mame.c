#include "driver.h"

/* Mame frontend interface rountines by Maurizio Zanello */

char mameversion[] = "0.31 ("__DATE__")";

static struct RunningMachine machine;
struct RunningMachine *Machine = &machine;
static const struct GameDriver *gamedrv;
static const struct MachineDriver *drv;

int nocheat;    /* 0 when the -cheat option was specified */
int mame_debug; /* !0 when -debug option is specified */

int frameskip;
int VolumePTR = 0;
int CurrentVolume = 100;

#define MAX_COLOR_TUPLE 16      /* no more than 4 bits per pixel, for now */
#define MAX_COLOR_CODES 256     /* no more than 256 color codes, for now */


unsigned char *RAM;
unsigned char *ROM;

FILE *errorlog;

void *record;   /* for -record */
void *playback; /* for -playback */


int init_machine(void);
void shutdown_machine(void);
int run_machine(void);


int run_game(int game, struct GameOptions *options)
{
	int err;

	errorlog   = options->errorlog;
	record     = options->record;
	playback   = options->playback;
	mame_debug = options->mame_debug;

	Machine->gamedrv = gamedrv = drivers[game];
	Machine->drv = drv = gamedrv->drv;

	/* copy configuration */
	Machine->sample_rate = options->samplerate;
	Machine->sample_bits = options->samplebits;
	frameskip = options->frameskip;
	nocheat = !options->cheat;

	/* get orientation right */
	Machine->orientation = gamedrv->orientation;
	if (options->norotate)
		Machine->orientation = ORIENTATION_DEFAULT;
	if (options->ror)
	{
		if (Machine->orientation & ORIENTATION_SWAP_XY)
			Machine->orientation ^= ORIENTATION_ROTATE_180;

		Machine->orientation ^= ORIENTATION_ROTATE_90;
	}
	if (options->rol)
	{
		if (Machine->orientation & ORIENTATION_SWAP_XY)
			Machine->orientation ^= ORIENTATION_ROTATE_180;

		Machine->orientation ^= ORIENTATION_ROTATE_270;
	}
	if (options->flipx)
		Machine->orientation ^= ORIENTATION_FLIP_X;
	if (options->flipy)
		Machine->orientation ^= ORIENTATION_FLIP_Y;


	/* Do the work*/
	err = 1;

	if (init_machine() == 0)
	{
		if (osd_init() == 0)
		{
			if (run_machine() == 0)
				err = 0;
			else printf("Unable to start machine emulation\n");

			osd_exit();
		}
		else printf ("Unable to initialize system\n");

		shutdown_machine();
	}
	else printf("Unable to initialize machine emulation\n");

	return err;
}



/***************************************************************************

  Initialize the emulated machine (load the roms, initialize the various
  subsystems...). Returns 0 if successful.

***************************************************************************/
int init_machine(void)
{
	if (gamedrv->new_input_ports)
	{
		int total;
		const struct InputPort *from;
		struct InputPort *to;

		from = gamedrv->new_input_ports;

		total = 0;
		do
		{
			total++;
		} while ((from++)->type != IPT_END);

		if ((Machine->input_ports = malloc(total * sizeof(struct InputPort))) == 0)
			return 1;

		from = gamedrv->new_input_ports;
		to = Machine->input_ports;

		do
		{
			memcpy(to,from,sizeof(struct InputPort));

			to++;
		} while ((from++)->type != IPT_END);
	}


	if (readroms (gamedrv->rom, gamedrv->name) != 0)
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
	Machine->samples = readsamples(gamedrv->samplenames,gamedrv->name);


	/* first of all initialize the memory handlers, which could be used by the */
	/* other initialization routines */
	cpu_init();

	/* ASG 971007 move from mame.c */
	if( !initmemoryhandlers() )
	{
		free(Machine->input_ports);
		return 1;
	}

	if (Machine->drv->sh_init && (*Machine->drv->sh_init)(gamedrv->name) != 0)
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

	/* ASG 971007 free memory element map */
	shutdownmemoryhandler();

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



static void vh_close(void)
{
	int i;


	for (i = 0;i < MAX_GFX_ELEMENTS;i++) freegfx(Machine->gfx[i]);
	freegfx(Machine->uifont);
	/* ASG 980209 - added: */
	free(Machine->pens);
	osd_close_display();
}



static int vh_open(void)
{
	int i;
	const unsigned char *palette,*colortable;
	unsigned char convpalette[3 * MAX_PENS];
	unsigned char *convtable;


	convtable = malloc(drv->color_table_len);
	if (!convtable) return 1;

	if ((Machine->colortable = malloc(drv->color_table_len * sizeof(unsigned short))) == 0)
	{
		free (convtable);
		return 1;
	}

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
				free (convtable);
				return 1;
			}
			Machine->gfx[i]->colortable = &Machine->colortable[drv->gfxdecodeinfo[i].color_codes_start];
			Machine->gfx[i]->total_colors = drv->gfxdecodeinfo[i].total_color_codes;
		}
	}


	/* convert the palette */
	if (drv->video_attributes & VIDEO_SUPPORTS_16BIT)       /* ASG 980209 - RGB/332 for 16-bit games */
	{
		unsigned char *p = convpalette;
		int r, g, b;

		for (r = 0; r < 8; r++)
			for (g = 0; g < 8; g++)
				for (b = 0; b < 4; b++)
				{
					*p++ = (r << 5) | (r << 2) | (r >> 1);
					*p++ = (g << 5) | (g << 2) | (g >> 1);
					*p++ = (b << 6) | (b << 4) | (b << 2) | b;
				}
		palette = convpalette;
		colortable = convtable;
	}
	else if (drv->vh_convert_color_prom)
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


	/* ASG 980209 - allocate space for the pens */
	if (drv->video_attributes & VIDEO_SUPPORTS_16BIT)
		Machine->pens = malloc(32768 * sizeof (short));
	else
		Machine->pens = malloc(MAX_PENS * sizeof (short));

	if (Machine->pens == 0)
	{
		free (convtable);
		return 1;
	}

	/* create the display bitmap, and allocate the palette */
	if ((Machine->scrbitmap = osd_create_display(
			drv->screen_width,drv->screen_height,
			drv->total_colors,
			palette,Machine->pens,drv->video_attributes)) == 0)
	{
		free (convtable);
		return 1;
	}


	for (i = 0;i < drv->color_table_len;i++)
		Machine->colortable[i] = Machine->pens[colortable[i]];


	/* build our private user interface font */
	if ((Machine->uifont = builduifont()) == 0)
	{
		vh_close();
		free(convtable);
		return 1;
	}

	free (convtable);


	/* do a first screen update to make the OS dependant code pick the */
	/* background pen before the copyright screen is displayed. */
	osd_update_display();

	return 0;
}



/***************************************************************************

  This function takes care of refreshing the screen, processing user input,
  and throttling the emulation speed to obtain the required frames per second.

***************************************************************************/
int updatescreen(void)
{
	static int framecount = 0, showvoltemp = 0; /* M.Z.: new options */


#ifndef macintosh /* LBO 061497 */
	/* if the user pressed ESC, stop the emulation */
	if (osd_key_pressed(OSD_KEY_ESC)) return 1;
#else
	/* LBO - moved most of the key-handling stuff to the OS routines so menu selections       */
	/* can get trapped as well rather than having a sick hack in the osd_key_pressed routine  */
	if (osd_handle_event() == true)
		return 1;
#endif


	/* if the user pressed F3, reset the emulation */
	if (osd_key_pressed(OSD_KEY_F3))
		machine_reset();


#ifndef macintosh /* LBO 061497 */
	if (osd_key_pressed(OSD_KEY_MINUS_PAD) && osd_key_pressed(OSD_KEY_LSHIFT) == 0)
	{
		/* decrease volume */
		if (CurrentVolume > 0) CurrentVolume--;
		osd_set_mastervolume(CurrentVolume);
		showvoltemp = 50;
	}

	if (osd_key_pressed(OSD_KEY_PLUS_PAD) && osd_key_pressed(OSD_KEY_LSHIFT) == 0)
	{
		/* increase volume */
		if (CurrentVolume < 100) CurrentVolume++;
		osd_set_mastervolume(CurrentVolume);
		showvoltemp = 50;
	}                                          /* MAURY_END: new options */

	if (osd_key_pressed(OSD_KEY_P)) /* pause the game */
	{
		struct DisplayText dt[2];
		int count = 0;


		dt[0].text = "PAUSED";
		dt[0].color = DT_COLOR_RED;
		dt[0].x = (Machine->uiwidth - Machine->uifont->width * strlen(dt[0].text)) / 2;
		dt[0].y = (Machine->uiheight - Machine->uifont->height) / 2;
		dt[1].text = 0;

		osd_set_mastervolume(0);

		while (osd_key_pressed(OSD_KEY_P))
			osd_update_audio();     /* give time to the sound hardware to apply the volume change */

		while (osd_key_pressed(OSD_KEY_P) == 0 && osd_key_pressed(OSD_KEY_ESC) == 0)
		{
			if (osd_key_pressed(OSD_KEY_TAB)) setup_menu(); /* call the configuration menu */

			osd_clearbitmap(Machine->scrbitmap);
			(*drv->vh_update)(Machine->scrbitmap);  /* redraw screen */

			if (count < Machine->drv->frames_per_second / 2)
				displaytext(dt,0);      /* make PAUSED blink */
			else
				osd_update_display();

			count = (count + 1) % (Machine->drv->frames_per_second / 1);
		}

		while (osd_key_pressed(OSD_KEY_ESC));   /* wait for jey release */
		while (osd_key_pressed(OSD_KEY_P));     /* ditto */

		osd_set_mastervolume(CurrentVolume);
		osd_clearbitmap(Machine->scrbitmap);
	}

	/* if the user pressed TAB, go to the setup menu */
	if (osd_key_pressed(OSD_KEY_TAB))
	{
		osd_set_mastervolume(0);

		while (osd_key_pressed(OSD_KEY_TAB))
			osd_update_audio();     /* give time to the sound hardware to apply the volume change */

		if (setup_menu()) return 1;

		osd_set_mastervolume(CurrentVolume);
	}

	/* if the user pressed F4, show the character set */
	if (osd_key_pressed(OSD_KEY_F4))
	{
		osd_set_mastervolume(0);

		while (osd_key_pressed(OSD_KEY_F4))
			osd_update_audio();     /* give time to the sound hardware to apply the volume change */

		if (showcharset()) return 1;

		osd_set_mastervolume(CurrentVolume);
	}
#endif /* LBO 061497 */


	if (++framecount > frameskip)
	{
		framecount = 0;

		if (showvoltemp)
		{
			showvoltemp--;
			if (!showvoltemp)
			{
				osd_clearbitmap(Machine->scrbitmap);
			}
		}                        /* MAURY_END: nuove opzioni */

		(*drv->vh_update)(Machine->scrbitmap);  /* update screen */

		/* This call is for the cheat, it must be called at least each frames */
		if (nocheat == 0) DoCheat(CurrentVolume);

		if (showvoltemp)      /* MAURY_BEGIN: nuove opzioni */
		{                     /* volume-meter */
			int trueorientation;
			int i,x;
			char volstr[25];


			trueorientation = Machine->orientation;
			Machine->orientation = ORIENTATION_DEFAULT;

			x = (drv->screen_width - 24*Machine->uifont->width)/2;
			strcpy(volstr,"                      ");
			for (i = 0;i < (CurrentVolume/5);i++) volstr[i+1] = '\x15';

			drawgfx(Machine->scrbitmap,Machine->uifont,16,DT_COLOR_RED,0,0,x,drv->screen_height/2,0,TRANSPARENCY_NONE,0);
			drawgfx(Machine->scrbitmap,Machine->uifont,17,DT_COLOR_RED,0,0,x+23*Machine->uifont->width,drv->screen_height/2,0,TRANSPARENCY_NONE,0);
			for (i = 0;i < 22;i++)
			    drawgfx(Machine->scrbitmap,Machine->uifont,(unsigned int)volstr[i],DT_COLOR_WHITE,
					0,0,x+(i+1)*Machine->uifont->width,drv->screen_height/2,0,TRANSPARENCY_NONE,0);

			Machine->orientation = trueorientation;
		}                     /* MAURY_END: nuove opzioni */

		osd_poll_joystick();

		osd_update_display();
	}

	/* update audio. Do it after the speed throttling to be in better sync. */
/*	ASG 980417 -- moved to the update function in cpuintrf.c
	sound_update();*/

	return 0;
}



/***************************************************************************

  Run the emulation. Start the various subsystems and the CPU emulation.
  Returns non zero in case of error.

***************************************************************************/
int run_machine(void)
{
	int res = 1;


	if (vh_open() == 0)
	{
		if (drv->vh_start == 0 || (*drv->vh_start)() == 0)      /* start the video hardware */
		{
			if (sound_start() == 0) /* start the audio hardware */
			{
				int i;
				struct DisplayText dt[2];


#ifndef macintosh /* LBO - This text is displayed in a dialog box. */
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
				while (osd_key_pressed(i));             /* wait for key release */
				if (i != OSD_KEY_ESC)
#endif
				{
					showcredits();  /* show the driver credits */

					osd_clearbitmap(Machine->scrbitmap);
					osd_update_display();


					/* free the graphics ROMs, they are no longer needed */
					/* TODO: instead of hardcoding region 1, use a flag to mark regions */
					/*       which can be freed after initialization. */
					free(Machine->memory_region[1]);
					Machine->memory_region[1] = 0;

					/* load input ports settings (keys, dip switches, and so on) */
					load_input_port_settings();

					if (nocheat == 0) InitCheat();

					cpu_run();      /* run the emulation! */

					if (nocheat == 0) StopCheat();

					/* save input ports settings */
					save_input_port_settings();

					/* the following MUST be done after hiscore_save() otherwise */
					/* some 68000 games will not work */
					sound_stop();
					if (drv->vh_stop) (*drv->vh_stop)();
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



int mame_highscore_enabled(void)
{
	/* disable high score when record/playback is on */
	if (record != 0 || playback != 0) return 0;

	/* disable high score when cheats are used */
	if (he_did_cheat != 0) return 0;

	return 1;
}
