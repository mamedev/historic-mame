/***************************************************************************

  osdepend.c

  OS dependant stuff (display handling, keyboard scan...)
  This is the only file which should me modified in order to port the
  emulator to a different system.

***************************************************************************/

#include <math.h>
#include <pc.h>
#include <conio.h>
#include <sys/farptr.h>
#include <go32.h>
#include <time.h>
#include "TwkUser.c"
#include <allegro.h>
#include "driver.h"
#include <audio.h>
#include "ym2203.h"
#include "vgafreq.h"

DECLARE_GFX_DRIVER_LIST(
	GFX_DRIVER_VGA
	GFX_DRIVER_VESA2L
	GFX_DRIVER_VESA2B
	GFX_DRIVER_VESA1)

DECLARE_COLOR_DEPTH_LIST(
	COLOR_DEPTH_8)

DECLARE_DIGI_DRIVER_LIST()
DECLARE_MIDI_DRIVER_LIST()


struct osd_bitmap *bitmap;


unsigned char current_palette[256][3];
PALETTE adjusted_palette;
unsigned char dirtycolor[256];

int vgafreq;
int video_sync;
int use_mouse;
int use_alternate;
int vector_game;
int vector_updates;
int vector_sign;
int use_anti_alias;
int use_dirty;
int use_synced;

extern int usecfg;
extern int savecfg;

int gfx_mode;
int gfx_width;
int gfx_height;
int skiplines;
int gfx_xoffset;
int gfx_yoffset;
int gfx_display_lines;

int noscanlines;
int nodouble,doubling;

void joy_calibration(void);

struct { char *desc; int x, y; } gfx_res[] = {
	{ "-224x288"    , 224, 288 },
	{ "-224"        , 224, 288 },
	{ "-320x200"    , 320, 200 },
	{ "-320x240"    , 320, 240 },
	{ "-320"        , 320, 240 },
	{ "-400x300"    , 400, 300 },
	{ "-400"        , 400, 300 },
	{ "-512x384"    , 512, 384 },
	{ "-512"        , 512, 384 },
	{ "-640x480"    , 640, 480 },
	{ "-640x400"    , 640, 400 },
	{ "-640"        , 640, 480 },
	{ "-800x600"    , 800, 600 },
	{ "-800"        , 800, 600 },
	{ "-1024x768"   , 1024, 768 },
	{ "-1024"       , 1024, 768 },
	{ "-1280x1024"  , 1280, 1024 },
	{ "-1280"       , 1280, 1024 },
	{ "-1600x1200"  , 1600, 1200 },
	{ "-1600"       , 1600, 1200 },
	{ NULL          , 0, 0 }
	};

char *vesa_desc[6]= {
	"AUTO_DETECT", "VGA", "MODEX (not supported)",
	"VESA 1.2", "VESA 2.0 (banked)", "VESA 2 (linear)"
};


/* audio related stuff */
#define NUMVOICES 16
HAC hVoice[NUMVOICES];
LPAUDIOWAVE lpWave[NUMVOICES];
int Volumi[NUMVOICES];
int MasterVolume = 100;
unsigned char No_FM = 1;
unsigned char RegistersYM[264*5];  /* MAX 5 YM-2203 */


/* put here anything you need to do when the program is started. Return 0 if */
/* initialization was successful, nonzero otherwise. */
int osd_init(int argc,char **argv)
{
	int i,j;
	int soundcard;

	vector_game = 0;
	vector_updates = 0;
	use_anti_alias = 0;
	use_dirty = 0;
	gfx_mode = GFX_VGA;
	gfx_width = 0;
	gfx_height = 0;
	noscanlines = 0;
	nodouble = -1;
	vgafreq = -1;
	video_sync = 0;
	joy_type = -1;
	use_mouse = 1;
	soundcard = -1;
	use_synced = 1;

	set_config_file ("mame.cfg");

	if (usecfg) /* initialized in mame.c */
	{
		/* Read data from config file - V.V_121997 */
		if (stricmp(get_config_string("config","scanlines",""),"no") == 0)
			noscanlines = 1;
		if (stricmp(get_config_string("config","double",""),"no") == 0)
			nodouble = 1;
		if (stricmp(get_config_string("config","double",""),"yes") == 0)
			nodouble = 0;
		if (stricmp(get_config_string("config","vsync",""),"yes") == 0)
			video_sync = 1;
		if (stricmp(get_config_string("config","oplfm",""),"yes") == 0)
			No_FM = 0;
		if (stricmp(get_config_string("config","antialias",""),"yes") == 0)
			use_anti_alias = 1;
		if (stricmp(get_config_string("config","mouse",""),"no") == 0)
			use_mouse = 0;
		if (stricmp(get_config_string("config","vesa",""),"yes") == 0)
			gfx_mode = GFX_VESA2L;

		soundcard = get_config_int("config","soundcard",-1);
		vgafreq = get_config_int("config","vgafreq",-1);
		joy_type = get_config_int("config","joytype",-1);
	}

	if (stricmp(get_config_string("config","syncedtweak",""),"no") == 0)
		use_synced = 0;

	for (i = 1;i < argc;i++)
	{
		if (stricmp(argv[i],"-novesa") == 0)
			gfx_mode = GFX_VGA;
		if (stricmp(argv[i],"-vesa") == 0)
			gfx_mode = GFX_VESA2L;
		if (stricmp(argv[i],"-vesa1") == 0)
			gfx_mode = GFX_VESA1;
		if (stricmp(argv[i],"-vesa2b") == 0)
			gfx_mode = GFX_VESA2B;
		if (stricmp(argv[i],"-vesa2l") == 0)
			gfx_mode = GFX_VESA2L;
		if (stricmp(argv[i],"-skiplines") == 0)
		{
			i++;
			if (i < argc) skiplines = atoi(argv[i]);
		}
		for (j=0; gfx_res[j].desc != NULL; j++)
		{
			if (stricmp(argv[i], gfx_res[j].desc) == 0)
			{
				if (gfx_res[j].x == 224)
					use_alternate = 1;
				else
				{
					if (gfx_mode==GFX_VGA)
						gfx_mode=GFX_VESA2L;
					gfx_width = gfx_res[j].x;
					gfx_height = gfx_res[j].y;
				}
				break;
			}
		}
		if (stricmp(argv[i],"-noscanlines") == 0)
			noscanlines = 1;
		if (stricmp(argv[i],"-scanlines") == 0)
			noscanlines = 0;
		if (stricmp(argv[i],"-nodouble") == 0)
			nodouble = 1;
		if (stricmp(argv[i],"-double") == 0)
			nodouble = 0;
		if (stricmp(argv[i],"-novsync") == 0)
			video_sync = 0;
		if (stricmp(argv[i],"-vsync") == 0)
			video_sync = 1;

		if (stricmp(argv[i],"-soundcard") == 0)
		{
			i++;
			if (i < argc) soundcard = atoi(argv[i]);
		}

		if (stricmp(argv[i],"-joy") == 0)
		{
			i++;
			if (i < argc) joy_type = atoi(argv[i]);
		}
		if (stricmp(argv[i],"-nojoy") == 0)
		{
			joy_type = -1;
		}

		if (stricmp(argv[i],"-vgafreq") == 0)
		{
			i++;
			if (i < argc) vgafreq = atoi(argv[i]);
		}
		if (stricmp(argv[i],"-fm") == 0)
			No_FM = 0;
		if (stricmp(argv[i],"-nofm") == 0)
			No_FM = 1;

		/* NTB */
		if (stricmp(argv[i],"-nomouse") == 0)
			use_mouse = 0;
		if (stricmp(argv[i],"-mouse") == 0)
			use_mouse = 1;

		if (stricmp(argv[i],"-aa") == 0)
			use_anti_alias=1;
		if (stricmp(argv[i],"-noaa") == 0)
			use_anti_alias=0;

	}

	{
	    AUDIOINFO info;
	    AUDIOCAPS caps;
	    char *blaster_env;


		/* initialize SEAL audio library */
		if (AInitialize() == AUDIO_ERROR_NONE)
		{
			if (soundcard == -1)
			{
				unsigned int k;


				printf("\nSelect the audio device:\n(if you have an AWE 32, choose Sound Blaster for a more faithful emulation)\n");
				for (k = 0;k < AGetAudioNumDevs();k++)
				{
					if (AGetAudioDevCaps(k,&caps) == AUDIO_ERROR_NONE)
						printf("  %2d. %s\n",k,caps.szProductName);
				}
				printf("\n");

				if (k < 10)
				{
				   i = getch();
				   soundcard = i - '0';
				}
				else
				   scanf("%d",&soundcard);
			}


			if (soundcard == 0)     /* silence */
				/* update the Machine structure to show that sound is disabled */
				Machine->sample_rate = 0;
			else
			{
				/* open audio device */
/*                              info.nDeviceId = AUDIO_DEVICE_MAPPER;*/
				info.nDeviceId = soundcard;
	/* always use 16 bit mixing if possible - better quality and same speed of 8 bit */
				info.wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_MONO;
/* I don't know why, but playing at 44100Hz causes some crackling, which */
/* disappears at 44099Hz. So here we just subtract one from the target sample rate. */
				info.nSampleRate = Machine->sample_rate-1;
				if (AOpenAudio(&info) != AUDIO_ERROR_NONE)
				{
					printf("audio initialization failed\n");
					return 1;
				}

				AGetAudioDevCaps(info.nDeviceId,&caps);
if (errorlog) fprintf(errorlog,"Using %s at %d-bit %s %u Hz\n",
					caps.szProductName,
					info.wFormat & AUDIO_FORMAT_16BITS ? 16 : 8,
					info.wFormat & AUDIO_FORMAT_STEREO ? "stereo" : "mono",
					info.nSampleRate);

				/* update the Machine structure to reflect the actual sample rate */
				Machine->sample_rate = info.nSampleRate;

				/* open and allocate voices, allocate waveforms */
				if (AOpenVoices(NUMVOICES) != AUDIO_ERROR_NONE)
				{
					printf("voices initialization failed\n");
					return 1;
				}

				for (i = 0; i < NUMVOICES; i++)
				{
					if (ACreateAudioVoice(&hVoice[i]) != AUDIO_ERROR_NONE)
					{
						printf("voice #%d creation failed\n",i);
						return 1;
					}

					ASetVoicePanning(hVoice[i],128);

					lpWave[i] = 0;
				}

	if (!No_FM) {
	   /* Get Soundblaster base address from environment variabler BLASTER   */
	   /* Soundblaster OPL base port, at some compatibles this must be 0x388 */

	   if(!getenv("BLASTER"))
	   {
	      printf("\nBLASTER variable not found, disabling fm sound!\n");
	      No_FM=1;
	   }
	   else
	   {
	      blaster_env = getenv("BLASTER");
	      BaseSb = i = 0;
	      while ((blaster_env[i] & 0x5f) != 0x41) i++;        /* Look for 'A' char */
	      while (blaster_env[++i] != 0x20) {
		BaseSb = (BaseSb << 4) + (blaster_env[i]-0x30);
	      }

	      DelayReg=4;   /* Delay after an OPL register write increase it to avoid problems ,but you'll lose speed */
	      DelayData=7;  /* same as above but after an OPL data write this usually is greater than above */
	      InitYM();     /* inits OPL in mode OPL3 and 4ops per channel,also reset YM2203 registers */
	   }
	}

			}
		}
	}


	allegro_init();
	install_keyboard();             /* Allegro keyboard handler */
	set_leds(0);    /* turn off all leds */

	if (joy_type != -1)
	{
		int tmp,err;

		/* Try to install Allegro's joystick handler */
		/* load_joystick_data overwrites joy_type... */
		tmp=joy_type;
		err=load_joystick_data(0);

		if (err || (tmp != joy_type))
		/* no valid cfg file or the joy_type doesn't match */
		{
			joy_type=tmp;
			if (initialise_joystick() != 0)
			{
				printf ("Joystick not found.\n");
				joy_type=-1;
			}
		}
	}

	/* NTB: Initialiase mouse */
	if (use_mouse && (install_mouse() == -1))
		use_mouse = 0;


	/* Look if this is a vector game and set default vesa-res */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		vector_game=1;
		use_dirty=1;
		vector_sign=1;
	}

	if (Machine->drv->video_attributes & VIDEO_SUPPORTS_DIRTY)
		use_dirty=1;

	if (savecfg) /* save msdos config data - V.V_121997 */
	{
		if (noscanlines)
			set_config_string("config","scanlines","no");
		else
			set_config_string("config","scanlines","yes");
		if (nodouble == 1)
			set_config_string("config","double","no");
		else if (nodouble == 0)
			set_config_string("config","double","yes");
		else
			set_config_string("config","double","auto");
		if (video_sync)
			set_config_string("config","vsync","yes");
		else
			set_config_string("config","vsync","no");
		if (No_FM)
			set_config_string("config","oplfm","no");
		else
			set_config_string("config","oplfm","yes");
		if (use_anti_alias)
			set_config_string("config","antialias","yes");
		else
			set_config_string("config","antialias","no");
		if (use_mouse)
			set_config_string("config","mouse","yes");
		else
			set_config_string("config","mouse","no");
		if (gfx_mode == GFX_VESA2L)
			set_config_string("config","vesa","yes");
		else
			set_config_string("config","vesa","no");
		set_config_int("config","soundcard",soundcard);
		set_config_int("config","vgafreq",vgafreq);
		set_config_int("config","joytype",joy_type);
	}

	return 0;
}



/* put here cleanup routines to be executed when the program is terminated. */
void osd_exit(void)
{
	if (Machine->sample_rate != 0)
	{
		int n;

		if (!No_FM)
		   InitOpl();  /* Do this only before quiting , or some cards will make noise during playing */
			       /* It resets entire OPL registers to zero */

		/* stop and release voices */
		for (n = 0; n < NUMVOICES; n++)
		{
			AStopVoice(hVoice[n]);
			ADestroyAudioVoice(hVoice[n]);
			if (lpWave[n])
			{
				ADestroyAudioData(lpWave[n]);
				free(lpWave[n]);
				lpWave[n] = 0;
			}
		}
		ACloseVoices();
		ACloseAudio();
	}
}


/* Create a bitmap. Also calls osd_clearbitmap() to appropriately initialize */
/* it to the background color. */
struct osd_bitmap *osd_create_bitmap(int width,int height)
{
	struct osd_bitmap *bitmap;

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = width;
		width = height;
		height = temp;
	}

	if ((bitmap = malloc(sizeof(struct osd_bitmap))) != 0)
	{
		int i;
		unsigned char *bm;


		bitmap->width = width;
		bitmap->height = height;
		if ((bm = malloc(width * height * sizeof(unsigned char))) == 0)
		{
			free(bitmap);
			return 0;
		}

		if ((bitmap->line = malloc(height * sizeof(unsigned char *))) == 0)
		{
			free(bm);
			free(bitmap);
			return 0;
		}

		for (i = 0;i < height;i++)
			bitmap->line[i] = &bm[i * width];

		bitmap->_private = bm;

		osd_clearbitmap(bitmap);
	}

	return bitmap;
}



/* set the bitmap to black */
void osd_clearbitmap(struct osd_bitmap *bitmap)
{
	int i;


	for (i = 0;i < bitmap->height;i++)
		memset(bitmap->line[i],0,bitmap->width);
}



void osd_free_bitmap(struct osd_bitmap *bitmap)
{
	if (bitmap)
	{
		free(bitmap->line);
		free(bitmap->_private);
		free(bitmap);
	}
}



Register scr200x320[] =
{
	{ 0x3c2, 0x00, 0xe3}, { 0x3d4, 0x00, 0x5f}, { 0x3d4, 0x01, 0x31},
	{ 0x3d4, 0x02, 0x38}, { 0x3d4, 0x03, 0x82}, { 0x3d4, 0x04, 0x4a},
	{ 0x3d4, 0x05, 0x9a}, { 0x3d4, 0x06, 0x4e}, { 0x3d4, 0x07, 0x1f},
	{ 0x3d4, 0x08, 0x00}, { 0x3d4, 0x09, 0x40}, { 0x3d4, 0x10, 0x40},
	{ 0x3d4, 0x11, 0x90}, { 0x3d4, 0x12, 0x3f}, { 0x3d4, 0x13, 0x19},
	{ 0x3d4, 0x14, 0x40}, { 0x3d4, 0x15, 0x80}, { 0x3d4, 0x16, 0x40},
	{ 0x3d4, 0x17, 0xa3}, { 0x3c4, 0x01, 0x01}, { 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40}, { 0x3ce, 0x06, 0x05}, { 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x00}
};

Register scr224x288_alternate[] =
{
	{ 0x3c2, 0x00, 0xa7},{ 0x3d4, 0x00, 0x71},{ 0x3d4, 0x01, 0x37},
	{ 0x3d4, 0x02, 0x64},{ 0x3d4, 0x03, 0x92},{ 0x3d4, 0x04, 0x4f},
	{ 0x3d4, 0x05, 0x98},{ 0x3d4, 0x06, 0x46},{ 0x3d4, 0x07, 0x1f},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x40},{ 0x3d4, 0x10, 0x31},
	{ 0x3d4, 0x11, 0x80},{ 0x3d4, 0x12, 0x1f},{ 0x3d4, 0x13, 0x1c},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0x2f},{ 0x3d4, 0x16, 0x44},
	{ 0x3d4, 0x17, 0xe3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x02, 0x0f},
	{ 0x3c4, 0x04, 0x0e},{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},
	{ 0x3c0, 0x10, 0x41},{ 0x3c0, 0x13, 0x00}
};

Register scr224x288[] =
{
	{ 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, SY_FREQ224NH},{ 0x3d4, 0x01, 0x37},
	{ 0x3d4, 0x02, 0x38},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x4a},
	{ 0x3d4, 0x05, 0x9a},{ 0x3d4, 0x06, SY_FREQ224NV},{ 0x3d4, 0x07, 0xf0},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x61},{ 0x3d4, 0x10, 0x40},
	{ 0x3d4, 0x11, 0xac},{ 0x3d4, 0x12, 0x3f},{ 0x3d4, 0x13, 0x1c},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0x40},{ 0x3d4, 0x16, 0x4a},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x0}
};

Register scr224x288scanlines[] =
{
	{ 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, SY_FREQ224SH},{ 0x3d4, 0x01, 0x37},
	{ 0x3d4, 0x02, 0x38},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x4a},
	{ 0x3d4, 0x05, 0x9a},{ 0x3d4, 0x06, SY_FREQ224SV},{ 0x3d4, 0x07, 0x1f},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x60},{ 0x3d4, 0x10, 0x2a},
	{ 0x3d4, 0x11, 0xac},{ 0x3d4, 0x12, 0x1f},{ 0x3d4, 0x13, 0x1c},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0x27},{ 0x3d4, 0x16, 0x3a},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x00}
};


Register scr256x256[] =
{
	{ 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, SY_FREQ256NH},{ 0x3d4, 0x01, 0x3f},
	{ 0x3d4, 0x02, 0x40},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x4A},
	{ 0x3d4, 0x05, 0x9A},{ 0x3d4, 0x06, SY_FREQ256NV},{ 0x3d4, 0x07, 0xb2},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x61},{ 0x3d4, 0x10, 0x14},
	{ 0x3d4, 0x11, 0xac},{ 0x3d4, 0x12, 0xff},{ 0x3d4, 0x13, 0x20},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0x07},{ 0x3d4, 0x16, 0x1a},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x00}
};

Register scr256x256scanlines[] =
{
	{ 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, SY_FREQ256SH},{ 0x3d4, 0x01, 0x3f},
	{ 0x3d4, 0x02, 0x40},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x4a},
	{ 0x3d4, 0x05, 0x9a},{ 0x3d4, 0x06, SY_FREQ256SV},{ 0x3d4, 0x07, 0x1d},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x60},{ 0x3d4, 0x10, 0x0a},
	{ 0x3d4, 0x11, 0xac},{ 0x3d4, 0x12, 0xff},{ 0x3d4, 0x13, 0x20},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0x07},{ 0x3d4, 0x16, 0x1a},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x00}
};

Register scr288x224[] =
{
	{ 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, SY_FREQ288NH},{ 0x3d4, 0x01, 0x47},
	{ 0x3d4, 0x02, 0x50},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x50},
	{ 0x3d4, 0x05, 0x80},{ 0x3d4, 0x06, SY_FREQ288NV},{ 0x3d4, 0x07, 0x3e},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x41},{ 0x3d4, 0x10, 0xda},
	{ 0x3d4, 0x11, 0x9c},{ 0x3d4, 0x12, 0xbf},{ 0x3d4, 0x13, 0x24},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0xc7},{ 0x3d4, 0x16, 0x04},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x0}
};

Register scr288x224scanlines[] =
{
	{ 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, SY_FREQ288SH},{ 0x3d4, 0x01, 0x47},
	{ 0x3d4, 0x02, 0x47},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x50},
	{ 0x3d4, 0x05, 0x9a},{ 0x3d4, 0x06, SY_FREQ288SV},{ 0x3d4, 0x07, 0x19},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x40},{ 0x3d4, 0x10, 0xf5},
	{ 0x3d4, 0x11, 0xac},{ 0x3d4, 0x12, 0xdf},{ 0x3d4, 0x13, 0x24},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0xc7},{ 0x3d4, 0x16, 0x04},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x00}
};

Register orig_scr224x288[] =
{
	{ 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, OR_FREQ224NH},{ 0x3d4, 0x01, 0x37},
	{ 0x3d4, 0x02, 0x38},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x4a},
	{ 0x3d4, 0x05, 0x9a},{ 0x3d4, 0x06, OR_FREQ224NV},{ 0x3d4, 0x07, 0xf0},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x61},{ 0x3d4, 0x10, 0x40},
	{ 0x3d4, 0x11, 0xac},{ 0x3d4, 0x12, 0x3f},{ 0x3d4, 0x13, 0x1c},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0x40},{ 0x3d4, 0x16, 0x4a},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x0}
};

Register orig_scr224x288scanlines[] =
{
	{ 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, OR_FREQ224SH},{ 0x3d4, 0x01, 0x37},
	{ 0x3d4, 0x02, 0x38},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x4a},
	{ 0x3d4, 0x05, 0x9a},{ 0x3d4, 0x06, OR_FREQ224SV},{ 0x3d4, 0x07, 0x1f},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x60},{ 0x3d4, 0x10, 0x2a},
	{ 0x3d4, 0x11, 0xac},{ 0x3d4, 0x12, 0x1f},{ 0x3d4, 0x13, 0x1c},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0x27},{ 0x3d4, 0x16, 0x3a},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x00}
};


Register orig_scr256x256[] =
{
	{ 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, OR_FREQ256NH},{ 0x3d4, 0x01, 0x3f},
	{ 0x3d4, 0x02, 0x40},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x4A},
	{ 0x3d4, 0x05, 0x9A},{ 0x3d4, 0x06, OR_FREQ256NV},{ 0x3d4, 0x07, 0xb2},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x61},{ 0x3d4, 0x10, 0x14},
	{ 0x3d4, 0x11, 0xac},{ 0x3d4, 0x12, 0xff},{ 0x3d4, 0x13, 0x20},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0x07},{ 0x3d4, 0x16, 0x1a},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x00}
};

Register orig_scr256x256scanlines[] =
{
	{ 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, OR_FREQ256SH},{ 0x3d4, 0x01, 0x3f},
	{ 0x3d4, 0x02, 0x40},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x4a},
	{ 0x3d4, 0x05, 0x9a},{ 0x3d4, 0x06, OR_FREQ256SV},{ 0x3d4, 0x07, 0x1d},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x60},{ 0x3d4, 0x10, 0x0a},
	{ 0x3d4, 0x11, 0xac},{ 0x3d4, 0x12, 0xff},{ 0x3d4, 0x13, 0x20},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0x07},{ 0x3d4, 0x16, 0x1a},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x00}
};

Register orig_scr288x224[] =
{
	{ 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, OR_FREQ288NH},{ 0x3d4, 0x01, 0x47},
	{ 0x3d4, 0x02, 0x50},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x50},
	{ 0x3d4, 0x05, 0x80},{ 0x3d4, 0x06, OR_FREQ288NV},{ 0x3d4, 0x07, 0x3e},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x41},{ 0x3d4, 0x10, 0xda},
	{ 0x3d4, 0x11, 0x9c},{ 0x3d4, 0x12, 0xbf},{ 0x3d4, 0x13, 0x24},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0xc7},{ 0x3d4, 0x16, 0x04},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x0}
};

Register orig_scr288x224scanlines[] =
{
	{ 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, OR_FREQ288SH},{ 0x3d4, 0x01, 0x47},
	{ 0x3d4, 0x02, 0x47},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x50},
	{ 0x3d4, 0x05, 0x9a},{ 0x3d4, 0x06, OR_FREQ288SV},{ 0x3d4, 0x07, 0x19},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x40},{ 0x3d4, 0x10, 0xf5},
	{ 0x3d4, 0x11, 0xac},{ 0x3d4, 0x12, 0xdf},{ 0x3d4, 0x13, 0x24},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0xc7},{ 0x3d4, 0x16, 0x04},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x00}
};

Register scr320x204[] =
{
	{ 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, 0x5f},{ 0x3d4, 0x01, 0x4f},
	{ 0x3d4, 0x02, 0x50},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x54},
	{ 0x3d4, 0x05, 0x80},{ 0x3d4, 0x06, 0xbf},{ 0x3d4, 0x07, 0x1f},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x41},{ 0x3d4, 0x10, 0x9c},
	{ 0x3d4, 0x11, 0x8e},{ 0x3d4, 0x12, 0x97},{ 0x3d4, 0x13, 0x28},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0x96},{ 0x3d4, 0x16, 0xb9},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x00}
};

Register scr240x272[] =
{
	{ 0x3c2, 0x00, 0xe3}, { 0x3d4, 0x00, 0x5f}, { 0x3d4, 0x01, 0x3b},
	{ 0x3d4, 0x02, 0x38}, { 0x3d4, 0x03, 0x82}, { 0x3d4, 0x04, 0x4a},
	{ 0x3d4, 0x05, 0x9a}, { 0x3d4, 0x06, 0x55}, { 0x3d4, 0x07, 0xf0},
	{ 0x3d4, 0x08, 0x00}, { 0x3d4, 0x09, 0x61}, { 0x3d4, 0x10, 0x40},
	{ 0x3d4, 0x11, 0xac}, { 0x3d4, 0x12, 0x20}, { 0x3d4, 0x13, 0x1e},
	{ 0x3d4, 0x14, 0x40}, { 0x3d4, 0x15, 0x40}, { 0x3d4, 0x16, 0x4a},
	{ 0x3d4, 0x17, 0xa3}, { 0x3c4, 0x01, 0x01}, { 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40}, { 0x3ce, 0x06, 0x05}, { 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x00}
};



#define MAXDIRTY 1600
char line1[MAXDIRTY];
char line2[MAXDIRTY];
char *dirty_old=line1;
char *dirty_new=line2;


/* ASG 971011 */
void osd_mark_dirty(int x1, int y1, int x2, int y2, int ui)
{
	/* DOS doesn't do dirty rects ... yet */
	/* But the VESA code does dirty lines now. BW */
	if (use_dirty)
	{
		if (y1 < 0) y1 = 0;
		if (y1 >= MAXDIRTY) y1 = MAXDIRTY-1;
		if (y2 >= MAXDIRTY) y2 = MAXDIRTY-1;

		memset(&dirty_new[y1], 1, y2-y1+1);
	}
}

void init_dirty(char dirty)
{
	memset(&dirty_new[0], dirty, MAXDIRTY);
}

void swap_dirty(void)
{
	char *tmp;

	tmp = dirty_old;
	dirty_old = dirty_new;
	dirty_new = tmp;
}

/*
 * This function tries to find the best display mode.
 */
void select_display_mode(int width, int height)
{
	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = width;
		width = height;
		height = temp;
	}

	/* If using tweaked modes, Check if there exists one to fit
	   the screen in, otherwise use VESA */

	if ((gfx_mode==GFX_VGA) &&
			!(width <= 200 && height <= 320) &&
			!(width <= 224 && height <= 288) &&
			!(width <= 240 && height <= 272) &&
			!(width <= 256 && height <= 256) &&
			!(width <= 288 && height <= 224) &&
			!(width <= 320 && height <= 204))
		gfx_mode = GFX_VESA2L;

	/* If no VESA resolution has been given, we choose a sensible one. */
	/* 640x480 and 800x600 are common to all VESA drivers.             */

	if ((gfx_mode!=GFX_VGA) && !gfx_width)
	{
		if (vector_game || (height <= 480 && width <= 640 &&
				/* see if pixel doubling can be applied */
				((height <= 240 && width <= 320) || height > 300 || width > 400)))
		{
			gfx_width=640;
			gfx_height=480;
		}
		else
		{
			gfx_width=800;
			gfx_height=600;
		}
	}
}

/* Centers VESA modes */
void adjust_vesa_display(int width, int height)
{
	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = width;
		width = height;
		height = temp;
	}

	if (width > gfx_width/2 || (nodouble != 0 && height > gfx_height/2))
	{
		doubling = 0;
		gfx_yoffset = (gfx_height - height) / 2;
		gfx_xoffset = (gfx_width - width) / 2;
		gfx_display_lines = height;
		if (gfx_height < gfx_display_lines)
			gfx_display_lines = gfx_height;
	}
	else
	{
		doubling = 1;
		gfx_yoffset = (gfx_height - height * 2) / 2;
		gfx_xoffset = (gfx_width - width * 2) / 2;
		gfx_display_lines = height;
		if (gfx_height/2 < gfx_display_lines)
			gfx_display_lines = gfx_height / 2;
	}
	/* Allign on a quadword !*/
	gfx_xoffset -= gfx_xoffset % 4;
	/* Just in case */
	if (gfx_yoffset < 0) gfx_yoffset = 0;
	if (gfx_xoffset < 0) gfx_xoffset = 0;
	if (gfx_display_lines+skiplines > height)
		skiplines=height-gfx_display_lines;
}

/* Scale the vector games to a given resolution */
void scale_vectorgames(int *width, int *height)
{
	double x_scale, y_scale, scale;

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		x_scale=(double)gfx_width/(double)(*height);
		y_scale=(double)gfx_height/(double)(*width);
	}
	else
	{
		x_scale=(double)gfx_width/(double)(*width);
		y_scale=(double)gfx_height/(double)(*height);
	}
	if (x_scale<y_scale)
		scale=x_scale;
	else
		scale=y_scale;
	*width=(int)((double)*width*scale);
	*height=(int)((double)*height*scale);

	/* Padding to an dword value */
	*width-=*width % 4;
	*height-=*height % 4;
}

int game_width;
int game_height;
int game_attributes;

/* Create a display screen, or window, large enough to accomodate a bitmap */
/* of the given dimensions. Attributes are the ones defined in driver.h. */
/* palette is an array of 'totalcolors' R,G,B triplets. The function returns */
/* in *pens the pen values corresponding to the requested colors. */
/* Return a osd_bitmap pointer or 0 in case of error. */
struct osd_bitmap *osd_create_display(int width,int height,int totalcolors,
		const unsigned char *palette,unsigned char *pens,int attributes)
{
	if (errorlog)
		fprintf (errorlog, "width %d, height %d\n", width,height);

	select_display_mode(width, height);
	if (vector_game && (gfx_mode != GFX_VGA))
		scale_vectorgames (&width, &height);
	if (gfx_mode != GFX_VGA)
		adjust_vesa_display( width,height);

	game_width = width;
	game_height = height;
	game_attributes = attributes;

	bitmap = osd_create_bitmap(width,height);

	if (!osd_set_display(width, height, attributes))
		return 0;

	/* initialize the palette */
	{
		int i;

		for (i = 0;i < 256;i++)
		{
			current_palette[i][0] = current_palette[i][1] = current_palette[i][2] = 0;
		}

		if (totalcolors < 256)
		{
			/* if we have free places, fill the palette starting from the end, */
			/* so we don't touch color 0, which is better left black */
			for (i = 0;i < totalcolors;i++)
				pens[i] = 255-i;
		}
		else
		{
			for (i = 0;i < totalcolors;i++)
				pens[i] = i;
		}


		for (i = 0;i < totalcolors;i++)
		{
			current_palette[pens[i]][0] = palette[3*i];
			current_palette[pens[i]][1] = palette[3*i+1];
			current_palette[pens[i]][2] = palette[3*i+2];
		}
	}

	return bitmap;
}

/* set the actual display screen but don't allocate the screen bitmap */
int osd_set_display(int width,int height, int attributes)
{
	Register *reg = 0;
	int reglen = 0;
	int     i;

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = width;
		width = height;
		height = temp;
	}
	/* Mark the dirty buffers as dirty */

	if (use_dirty)
	{
		init_dirty(1);
		swap_dirty();
		init_dirty(1);
	}
	for (i = 0;i < 256;i++)
	{
		dirtycolor[i] = 1;
	}


	if (gfx_mode!=GFX_VGA)
	{
		int mode;

		/* Try the specified vesamode and all lower ones BW 131097 */
		for (mode=gfx_mode; mode>=GFX_VESA1; mode--)
		{
			if (errorlog)
				fprintf (errorlog,"Trying %s\n", vesa_desc[mode]);
			if (set_gfx_mode(mode,gfx_width,gfx_height,0,0) == 0)
				break;
			if (errorlog)
				fprintf (errorlog,"%s\n",allegro_error);
		}
		if (mode < GFX_VESA1) {
			printf ("No %dx%d VESA modes available. Try using UNIVBE\n", gfx_width,gfx_height);
			return 0;
		}

		gfx_mode = mode;

	}
	else
	{
		int videofreq;


		videofreq = vgafreq;

		/* big hack: open a mode 13h screen using Allegro, then load the custom screen */
		/* definition over it. */
		if (set_gfx_mode(GFX_VGA,320,200,0,0) != 0)
			return 0;

		if (width <= 200 && height <= 320)
		{
			reg = scr200x320;
			reglen = sizeof(scr200x320)/sizeof(Register);
		}
		if (width <= 224 && height <= 288)
		{
			if (noscanlines)
			{
				if (use_synced)
				{
					if (videofreq == -1) videofreq = 1;
					reg = scr224x288;
				}
				else
					reg = orig_scr224x288;
				reglen = sizeof(scr224x288)/sizeof(Register);
			}
			else
			{
				if (use_alternate)
					reg = scr224x288_alternate;
				else
					if (use_synced)
					{
						if (videofreq == -1) videofreq = 2;
						reg = scr224x288scanlines;
					}
					else
						reg = orig_scr224x288scanlines;
				reglen = sizeof(scr224x288scanlines)/sizeof(Register);
			}
		}
		else if (width <= 240 && height <= 272)
		{
			reg = scr240x272;
			reglen = sizeof(scr240x272)/sizeof(Register);
		}
		else if (width <= 256 && height <= 256)
		{
			if (noscanlines)
			{
				if (use_synced)
				{
					if (videofreq == -1) videofreq = 1;
					reg = scr256x256;
				}
				else
					reg = orig_scr256x256;
				reglen = sizeof(scr256x256)/sizeof(Register);
			}
			else
			{
				if (use_synced)
				{
					if (videofreq == -1) videofreq = 1;
					reg = scr256x256scanlines;
				}
				else
					reg = orig_scr256x256scanlines;
				reglen = sizeof(scr256x256scanlines)/sizeof(Register);
			}
		}
		else if (width <= 288 && height <= 224)
		{
			if (noscanlines)
			{
				if (use_synced)
				{
					if (videofreq == -1) videofreq = 0;
					reg = scr288x224;
				}
				else
					reg = orig_scr288x224;
				reglen = sizeof(scr288x224)/sizeof(Register);
			}
			else
			{
				if (use_synced)
				{
					if (videofreq == -1) videofreq = 0;
					reg = scr288x224scanlines;
				}
				else
					reg = orig_scr288x224scanlines;
				reglen = sizeof(scr288x224scanlines)/sizeof(Register);
			}
		}
		else if (width <= 320 && height <= 204)
		{
			reg = scr320x204;
			reglen = sizeof(scr320x204)/sizeof(Register);
		}

		if (videofreq < 0) videofreq = 0;
		else if (videofreq > 3) videofreq = 3;

		/* set the VGA clock */
		if (!use_alternate)
		  reg[0].value = (reg[0].value & 0xf3) | (videofreq << 2);

		outRegArray(reg,reglen);
	}


	if (video_sync)
	{
		uclock_t a,b;
		int i,rate;


		/* wait some time to let everything stabilize */
		for (i = 0;i < 60;i++)
		{
			vsync();
			a = uclock();
		}
		vsync();
		b = uclock();
if (errorlog) fprintf(errorlog,"video frame rate = %3.2fHz\n",((float)UCLOCKS_PER_SEC)/(b-a));

		rate = UCLOCKS_PER_SEC/(b-a);

		/* don't allow more than 5% difference between target and actual frame rate */
		while (rate > Machine->drv->frames_per_second * 105 / 100)
			rate /= 2;

		if (rate < Machine->drv->frames_per_second * 95 / 100)
		{
			osd_close_display();
			printf("-vsync option cannot be used with this display mode:\n"
					"video refresh frequency = %dHz, target frame rate = %dfps\n",
					(int)(UCLOCKS_PER_SEC/(b-a)),Machine->drv->frames_per_second);
		}
	}
	return 1;
}



/* shut up the display */
void osd_close_display(void)
{
	set_gfx_mode(GFX_TEXT,80,25,0,0);
	osd_free_bitmap(bitmap);
	bitmap = 0;
}



void osd_modify_pen(int pen,unsigned char red, unsigned char green, unsigned char blue)
{
	if (current_palette[pen][0] != red ||
			current_palette[pen][1] != green ||
			current_palette[pen][2] != blue)
	{
		current_palette[pen][0] = red;
		current_palette[pen][1] = green;
		current_palette[pen][2] = blue;

		dirtycolor[pen] = 1;
	}
}



void osd_get_pen(int pen,unsigned char *red, unsigned char *green, unsigned char *blue)
{
	*red = current_palette[pen][0];
	*green = current_palette[pen][1];
	*blue = current_palette[pen][2];
}

/* Writes messages in the middle of the screen. */
void my_textout (char *buf)
{
	int trueorientation,l,x,y,i;

	/* hack: force the display into standard orientation to avoid */
	/* rotating the text */
	trueorientation = Machine->orientation;
	Machine->orientation = ORIENTATION_DEFAULT;

	l = strlen(buf);
	x = (Machine->scrbitmap->width - Machine->uifont->width * l) / 2;
	y = (Machine->scrbitmap->height - Machine->uifont->height) / 2;
	for (i = 0;i < l;i++)
		drawgfx(Machine->scrbitmap,Machine->uifont,buf[i],DT_COLOR_WHITE,0,0,
				x + i*Machine->uifont->width,y,0,TRANSPARENCY_NONE,0);

	Machine->orientation = trueorientation;
}

inline void double_pixels(unsigned long *lb, short seg,
			  unsigned long address, int width4)
{
	__asm__ __volatile__ ("
	pushw %%es              \n
	movw %%dx, %%es         \n
	cld                     \n
	.align 4                \n
	0:                      \n
	lodsw                   \n
	rorl $8, %%eax          \n
	movb %%al, %%ah         \n
	roll $16, %%eax         \n
	movb %%ah, %%al         \n
	stosl                   \n
	lodsw                   \n
	rorl $8, %%eax          \n
	movb %%al, %%ah         \n
	roll $16, %%eax         \n
	movb %%ah, %%al         \n
	stosl                   \n
	loop 0b                 \n
	popw %%ax               \n
	movw %%ax, %%es         \n
	"
	::
	"d" (seg),
	"c" (width4),
	"S" (lb),
	"D" (address):
	"ax", "bx", "cx", "dx", "si", "di", "cc", "memory");
}

inline void update_dirty_vga(void)
{
	int width,width4,y;
	unsigned long *lb, address;

	width=bitmap->width;
	width4=width/4;
	address=0xa0000;
	lb = bitmap->_private;
	for (y = 0; y < bitmap->height; y++)
	{
		if ((dirty_new[y+skiplines]+dirty_old[y+skiplines]) != 0)
			_dosmemputl(lb,width4,address);
		lb+=width4;
		address+=width;
	}
}

inline void update_vesa(void)
{
	short src_seg, dest_seg;
	int y, vesa_line, width4;
	unsigned long *lb, address;

	src_seg = _my_ds();
	dest_seg = screen->seg;
	vesa_line = gfx_yoffset;
	width4 = bitmap->width /4;
	lb = bitmap->_private + bitmap->width*skiplines;

	for (y = 0; y < gfx_display_lines; y++)
	{
		if (use_dirty)
		if ((dirty_new[y+skiplines]+dirty_old[y+skiplines]) == 0) {
			lb+=width4;
			vesa_line++;
			if (doubling != 0)
				vesa_line++;
			/* No need to update. Next line */
			continue;
		}

		address = bmp_write_line (screen, vesa_line) + gfx_xoffset;
		if (doubling != 0)
		{
			double_pixels(lb,dest_seg,address,width4);
			if (noscanlines) {
			   address = bmp_write_line (screen, vesa_line+1) + gfx_xoffset;
			   double_pixels(lb,dest_seg,address,width4);
			}
			vesa_line+=2;
		}
		else
		{
			_movedatal (src_seg,(unsigned long)lb,dest_seg,address,width4);
			vesa_line++;
		}
		lb+=width4;
	}
}

/* Update the display. */
/* As an additional bonus, this function also saves the screen as a PCX file */
/* when the user presses F5. This is not required for porting. */
void osd_update_display(void)
{
	int i;
	static float gamma_correction = 1.00;
	static float gamma_update = 0.00;
	static int showgammatemp;
	static int firsttime = 1;

	if (firsttime && usecfg) /* reads default gamma from mame.cfg - V.V_121997 */
	{
		gamma_correction = get_config_float("config","gamma",1.0);
		firsttime = 0;
	}

	if (video_sync && throttle)
	{
		static uclock_t last;
		uclock_t curr;


		do
		{
			vsync();
			curr = uclock();
		} while (UCLOCKS_PER_SEC / (curr - last) > Machine->drv->frames_per_second * 11 /10);

		last = curr;
	}

	if (osd_key_pressed(OSD_KEY_F7))
		joy_calibration();

	if (osd_key_pressed(OSD_KEY_LSHIFT) &&
			(osd_key_pressed(OSD_KEY_PLUS_PAD) || osd_key_pressed(OSD_KEY_MINUS_PAD)))
	{
		for (i = 0;i < 256;i++) dirtycolor[i] = 1;

		if (osd_key_pressed(OSD_KEY_MINUS_PAD)) gamma_update -= 0.02;
		if (osd_key_pressed(OSD_KEY_PLUS_PAD)) gamma_update += 0.02;

		if (gamma_update < -0.09)
		{
			gamma_update = 0.00;
			gamma_correction -= 0.10;
			if (savecfg) /* V.V_121997 */
			{
				set_config_file ("mame.cfg");
				set_config_float("config","gamma",gamma_correction);
			}
		}
		if (gamma_update > 0.09)
		{
			gamma_update = 0.00;
			gamma_correction += 0.10;
			if (savecfg) /* V.V_121997 */
			{
				set_config_file ("mame.cfg");
				set_config_float("config","gamma",gamma_correction);
			}
		}

		if (gamma_correction < 0.2) gamma_correction = 0.2;
		if (gamma_correction > 3.0) gamma_correction = 3.0;

		showgammatemp = Machine->drv->frames_per_second;
	}

	if (showgammatemp > 0)
	{
		if (--showgammatemp)
		{
			char buf[20];

			sprintf(buf,"Gamma = %1.1f",gamma_correction);
			my_textout(buf);
		}
		else
			osd_clearbitmap(Machine->scrbitmap);
	}

	for (i = 0;i < 256;i++)
	{
		if (dirtycolor[i])
		{
			int r,g,b;


			dirtycolor[i] = 0;

			r = 255 * pow(current_palette[i][0] / 255.0, 1 / gamma_correction);
			g = 255 * pow(current_palette[i][1] / 255.0, 1 / gamma_correction);
			b = 255 * pow(current_palette[i][2] / 255.0, 1 / gamma_correction);

			adjusted_palette[i].r = r >> 2;
			adjusted_palette[i].g = g >> 2;
			adjusted_palette[i].b = b >> 2;

			set_color(i,&adjusted_palette[i]);
		}
	}


	if (gfx_mode==GFX_VGA)
	{
		/* copy the bitmap to screen memory */
		if (use_dirty)
			update_dirty_vga();
		else
			_dosmemputl(bitmap->_private,bitmap->width * bitmap->height / 4,0xa0000);
	}
	else    /* vesa-modes */
	{
		update_vesa();

		/* Check for PGUP, PGDN and scroll screen */
		if (osd_key_pressed(OSD_KEY_PGDN) &&
			(skiplines+gfx_display_lines < bitmap->height))
		{
			skiplines++;
			if (use_dirty) init_dirty(1);
		}
		if (osd_key_pressed(OSD_KEY_PGUP) && (skiplines>0))
		{
			skiplines--;
			if (use_dirty) init_dirty(1);
		}
	}

	if (use_dirty)
	{
		/* Vector games may not have updated the screen */
		if (!vector_game || (vector_updates != 0))
		{
			/* for the vector games */
			vector_updates=0;
			swap_dirty();
			init_dirty(0);
		}
	}

	/* if the user pressed F12, save a snapshot of the screen. */
	if (osd_key_pressed(OSD_KEY_F12))
	{
		BITMAP *bmp;
		PALETTE pal;
		char name[30];
		FILE *f;
		static int snapno;
		int y;

		do
		{
			sprintf(name,"%s/snap%04d.pcx",get_config_string("directory","pcx","PCX"),snapno);
			/* avoid overwriting of existing files */

			if ((f = fopen(name,"rb")) != 0)
			{
				fclose(f);
				snapno++;
			}
		} while (f != 0);

		get_palette(pal);
		bmp = create_bitmap(bitmap->width,bitmap->height);
		for (y = 0;y < bitmap->height;y++)
			memcpy(bmp->line[y],bitmap->line[y],bitmap->width);
		save_pcx(name,bmp,pal);
		destroy_bitmap(bmp);
		snapno++;

		/* wait for the user to release F12 */
		while (osd_key_pressed(OSD_KEY_F12));
	}
}



void osd_update_audio(void)
{
	if (Machine->sample_rate == 0) return;

	AUpdateAudio();
}



static void playsample(int channel,signed char *data,int len,int freq,int volume,int loop,int bits)
{
	if (Machine->sample_rate == 0 || channel >= NUMVOICES) return;

	if (lpWave[channel] && lpWave[channel]->dwLength != len)
	{
		AStopVoice(hVoice[channel]);
		ADestroyAudioData(lpWave[channel]);
		free(lpWave[channel]);
		lpWave[channel] = 0;
	}

	if (lpWave[channel] == 0)
	{
		if ((lpWave[channel] = (LPAUDIOWAVE)malloc(sizeof(AUDIOWAVE))) == 0)
			return;

		if (loop) lpWave[channel]->wFormat = (bits == 8 ? AUDIO_FORMAT_8BITS : AUDIO_FORMAT_16BITS)
				| AUDIO_FORMAT_MONO | AUDIO_FORMAT_LOOP;
		else lpWave[channel]->wFormat = (bits == 8 ? AUDIO_FORMAT_8BITS : AUDIO_FORMAT_16BITS)
				| AUDIO_FORMAT_MONO;
		lpWave[channel]->nSampleRate = Machine->sample_rate;
		lpWave[channel]->dwLength = len;
		lpWave[channel]->dwLoopStart = 0;
		lpWave[channel]->dwLoopEnd = len;
		if (ACreateAudioData(lpWave[channel]) != AUDIO_ERROR_NONE)
		{
			free(lpWave[channel]);
			lpWave[channel] = 0;

			return;
		}
	}
	else
	{
		if (loop) lpWave[channel]->wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO | AUDIO_FORMAT_LOOP;
		else lpWave[channel]->wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO;
	}

	memcpy(lpWave[channel]->lpData,data,len);
	/* upload the data to the audio DRAM local memory */
	AWriteAudioData(lpWave[channel],0,len);
	APlayVoice(hVoice[channel],lpWave[channel]);
	ASetVoiceFrequency(hVoice[channel],freq);
	ASetVoiceVolume(hVoice[channel],MasterVolume*volume/400);

	Volumi[channel] = volume/4;
}

void osd_play_sample(int channel,signed char *data,int len,int freq,int volume,int loop)
{
	playsample(channel,data,len,freq,volume,loop,8);
}

void osd_play_sample_16(int channel,signed short *data,int len,int freq,int volume,int loop)
{
	playsample(channel,(signed char *)data,2*len,freq,volume,loop,16);
}



static void playstreamedsample(int channel,signed char *data,int len,int freq,int volume,int bits)
{
	static int playing[NUMVOICES];
	static int c[NUMVOICES];


	if (Machine->sample_rate == 0 || channel >= NUMVOICES) return;

	if (!playing[channel])
	{
		if (lpWave[channel])
		{
			AStopVoice(hVoice[channel]);
			ADestroyAudioData(lpWave[channel]);
			free(lpWave[channel]);
			lpWave[channel] = 0;
		}

		if ((lpWave[channel] = (LPAUDIOWAVE)malloc(sizeof(AUDIOWAVE))) == 0)
			return;

		lpWave[channel]->wFormat = (bits == 8 ? AUDIO_FORMAT_8BITS : AUDIO_FORMAT_16BITS)
				| AUDIO_FORMAT_MONO | AUDIO_FORMAT_LOOP;
		lpWave[channel]->nSampleRate = Machine->sample_rate;
		lpWave[channel]->dwLength = 3*len;
		lpWave[channel]->dwLoopStart = 0;
		lpWave[channel]->dwLoopEnd = 3*len;
		if (ACreateAudioData(lpWave[channel]) != AUDIO_ERROR_NONE)
		{
			free(lpWave[channel]);
			lpWave[channel] = 0;

			return;
		}

		memset(lpWave[channel]->lpData,0,3*len);
		memcpy(lpWave[channel]->lpData,data,len);
		/* upload the data to the audio DRAM local memory */
		AWriteAudioData(lpWave[channel],0,3*len);
		APlayVoice(hVoice[channel],lpWave[channel]);
		ASetVoiceFrequency(hVoice[channel],freq);
		ASetVoiceVolume(hVoice[channel],MasterVolume*volume/400);
		playing[channel] = 1;
		c[channel] = 1;
	}
	else
	{
		DWORD pos;


		if (throttle)   /* sync with audio only when speed throttling is not turned off */
		{
			for(;;)
			{
				AGetVoicePosition(hVoice[channel],&pos);
				if (c[channel] == 0 && pos >= len) break;
				if (c[channel] == 1 && (pos < len || pos >= 2*len)) break;
				if (c[channel] == 2 && pos < 2*len) break;
				osd_update_audio();
			}
		}

		memcpy(&lpWave[channel]->lpData[len * c[channel]],data,len);
		AWriteAudioData(lpWave[channel],len*c[channel],len);
		c[channel]++;
		if (c[channel] == 3) c[channel] = 0;
	}

	Volumi[channel] = volume/4;
}

void osd_play_streamed_sample(int channel,signed char *data,int len,int freq,int volume)
{
	playstreamedsample(channel,data,len,freq,volume,8);
}

void osd_play_streamed_sample_16(int channel,signed short *data,int len,int freq,int volume)
{
	playstreamedsample(channel,(signed char *)data,2*len,freq,volume,16);
}



void osd_adjust_sample(int channel,int freq,int volume)
{
	if (Machine->sample_rate == 0 || channel >= NUMVOICES) return;

	Volumi[channel] = volume/4;
	ASetVoiceFrequency(hVoice[channel],freq);
	ASetVoiceVolume(hVoice[channel],MasterVolume*volume/400);
}



void osd_stop_sample(int channel)
{
	if (Machine->sample_rate == 0 || channel >= NUMVOICES) return;

	AStopVoice(hVoice[channel]);
}


void osd_restart_sample(int channel)
{
	if (Machine->sample_rate == 0 || channel >= NUMVOICES) return;

	AStartVoice(hVoice[channel]);
}


int osd_get_sample_status(int channel)
{
	int stopped=0;
	if (Machine->sample_rate == 0 || channel >= NUMVOICES) return -1;

	AGetVoiceStatus(hVoice[channel], &stopped);
	return stopped;
}


void osd_ym2203_write(int n, int r, int v)
{
      if (No_FM) return;
      else {
	YMNumber = n;
	RegistersYM[r+264*n] = v;
	if (r == 0x28) SlotCh();
	return;
      }
}


void osd_ym2203_update(void)
{
      if (No_FM) return;
      YM2203();  /* this is absolutely necesary */
}


void osd_set_mastervolume(int volume)
{
	int channel;

	MasterVolume = volume;
	for (channel=0; channel < NUMVOICES; channel++) {
	  ASetVoiceVolume(hVoice[channel],MasterVolume*Volumi[channel]/100);
	}
}


/* check if a key is pressed. The keycode is the standard PC keyboard code, as */
/* defined in osdepend.h. Return 0 if the key is not pressed, nonzero otherwise. */
int osd_key_pressed(int keycode)
{
	if (keycode == OSD_KEY_RCONTROL) keycode = KEY_RCONTROL;
	if (keycode == OSD_KEY_ALTGR) keycode = KEY_ALTGR;

	return key[keycode];
}



/* wait for a key press and return the keycode */
int osd_read_key(void)
{
	int key;


	/* wait for all keys to be released */
	do
	{
	      for (key = OSD_MAX_KEY;key >= 0;key--)
	      if (osd_key_pressed(key)) break;
	} while (key >= 0);

	/* wait for a key press */
	do
	{
	      for (key = OSD_MAX_KEY;key >= 0;key--)
	      if (osd_key_pressed(key)) break;
	} while (key < 0);

	return key;
}



/* Wait for a key press and return keycode.  Support repeat */
int osd_read_keyrepeat(void)
{
	int res;


	clear_keybuf();
	res = readkey() >> 8;

	if (res == KEY_RCONTROL) res = OSD_KEY_RCONTROL;
	if (res == KEY_ALTGR) res = OSD_KEY_ALTGR;

	return res;
}


/* return the name of a key */
const char *osd_key_name(int keycode)
{
	static char *keynames[] =
	{
		"ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9",
		"0", "MINUS", "EQUAL", "BACKSPACE", "TAB", "Q", "W", "E", "R", "T",
		"Y", "U", "I", "O", "P", "OPENBRACE", "CLOSEBRACE", "ENTER", "LEFTCONTROL",     "A",
		"S", "D", "F", "G", "H", "J", "K", "L", "COLON", "QUOTE",
		"TILDE", "LEFTSHIFT", "Error", "Z", "X", "C", "V", "B", "N", "M",
		"COMMA", "STOP", "SLASH", "RIGHTSHIFT", "ASTERISK", "ALT", "SPACE", "CAPSLOCK", "F1", "F2",
		"F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "NUMLOCK",     "SCRLOCK",
		"HOME", "UP", "PGUP", "MINUS PAD", "LEFT", "5 PAD", "RIGHT", "PLUS PAD", "END", "DOWN",
		"PGDN", "INS", "DEL", "RIGHTCONTROL", "ALTGR", "Error", "F11", "F12"
	};
	static char *nonedefined = "None";

	if (keycode && keycode <= OSD_MAX_KEY) return keynames[keycode-1];
	else return (char *)nonedefined;
}


/* return the name of a joystick button */
const char *osd_joy_name(int joycode)
{
	static char *joynames[] = {
		"Left", "Right", "Up", "Down", "Button A",
		"Button B", "Button C", "Button D", "Button E", "Button F",
		"Button G", "Button H", "Button I", "Button J", "Any Button",
		"J2 Left", "J2 Right", "J2 Up", "J2 Down", "J2 Button A",
		"J2 Button B", "J2 Button C", "J2 Button D", "J2 Button E", "J2 Button F",
		"J2 Button G", "J2 Button H", "J2 Button I", "J2 Button J", "J2 Any Button",
		"J3 Left", "J3 Right", "J3 Up", "J3 Down", "J3 Button A",
		"J3 Button B", "J3 Button C", "J3 Button D", "J3 Button E", "J3 Button F",
		"J3 Button G", "J3 Button H", "J3 Button I", "J3 Button J", "J3 Any Button",
		"J4 Left", "J4 Right", "J4 Up", "J4 Down", "J4 Button A",
		"J4 Button B", "J4 Button C", "J4 Button D", "J4 Button E", "J4 Button F",
		"J4 Button G", "J4 Button H", "J4 Button I", "J4 Button J", "J4 Any Button"
	};

	if (joycode == 0) return "None";
	else if (joycode <= OSD_MAX_JOY) return (char *)joynames[joycode-1];
	else return "Unknown";
}


void osd_poll_joystick(void)
{
	if (joy_type != -1)
		poll_joystick();
}


/* check if the joystick is moved in the specified direction, defined in */
/* osdepend.h. Return 0 if it is not pressed, nonzero otherwise. */
int osd_joy_pressed(int joycode)
{
	/* compiler bug? If I don't declare these variables as volatile, */
	/* joystick right is not detected */
#if 0
	extern volatile joy_left, joy_right, joy_up, joy_down;
	extern volatile joy2_left, joy2_right, joy2_up, joy2_down;
	extern volatile joy_b1, joy_b2, joy_b3, joy_b4, joy_b5, joy_b6;
	extern volatile joy2_b1, joy2_b2;
	extern volatile joy_hat;
#endif

	switch (joycode)
	{
		case OSD_JOY_LEFT:
			return joy_left;
			break;
		case OSD_JOY_RIGHT:
			return joy_right;
			break;
		case OSD_JOY_UP:
			return joy_up;
			break;
		case OSD_JOY_DOWN:
			return joy_down;
			break;
		case OSD_JOY_FIRE1:
			return (joy_b1 || (mouse_b & 1));
			break;
		case OSD_JOY_FIRE2:
			return (joy_b2 || (mouse_b & 2));
			break;
		case OSD_JOY_FIRE3:
			return (joy_b3 || (mouse_b & 4));
			break;
		case OSD_JOY_FIRE4:
			return joy_b4;
			break;
		case OSD_JOY_FIRE5:
			return joy_b5;
			break;
		case OSD_JOY_FIRE6:
			return joy_b6;
			break;
		case OSD_JOY_FIRE:
			return (joy_b1 || joy_b2 || joy_b3 || joy_b4 || joy_b5 || joy_b6 || mouse_b);

		/* The second joystick (only 2 fire buttons!)*/
		case OSD_JOY2_LEFT:
			return joy2_left;
			break;
		case OSD_JOY2_RIGHT:
			return joy2_right;
			break;
		case OSD_JOY2_UP:
			return joy2_up;
			break;
		case OSD_JOY2_DOWN:
			return joy2_down;
			break;
		case OSD_JOY2_FIRE1:
			return (joy2_b1);
			break;
		case OSD_JOY2_FIRE2:
			return (joy2_b2);
			break;
		case OSD_JOY2_FIRE:
			return (joy2_b1 || joy2_b2);
			break;

		/* The third joystick (coolie hat) */
		case OSD_JOY3_LEFT:
			return (joy_hat==JOY_HAT_LEFT);
			break;
		case OSD_JOY3_RIGHT:
			return (joy_hat==JOY_HAT_RIGHT);
			break;
		case OSD_JOY3_UP:
			return (joy_hat==JOY_HAT_UP);
			break;
		case OSD_JOY3_DOWN:
			return (joy_hat==JOY_HAT_DOWN);
			break;
		default:
//                      if (errorlog)
//                              fprintf (errorlog,"Error: osd_joy_pressed called with no valid joyname\n");
			break;
	}
	return 0;
}

int osd_analogjoy_read(int axis)
{
	if (joy_type==-1)
		return (NO_ANALOGJOY);

	switch(axis) {
		case X_AXIS:
			return(joy_x);
			break;
		case Y_AXIS:
			return(joy_y);
			break;
	}
	fprintf (errorlog, "osd_analogjoy_read(): no axis selected\n");
	return 0;
}

void joy_calibration(void)
{
	char buf[60];
	int i;
	char *hat_posname[4] = { "up", "down", "left", "right" };
	int hat_pos[4] = { JOY_HAT_UP, JOY_HAT_DOWN, JOY_HAT_LEFT, JOY_HAT_RIGHT };

	if (joy_type==-1)
	{
		return;
	}

#define SCREEN_UPDATE \
	{       \
		if (use_dirty) \
			init_dirty(1); \
		if (gfx_mode == GFX_VGA) \
		_dosmemputl(bitmap->_private,bitmap->width * bitmap->height / 4,0xa0000); \
	else \
		update_vesa(); \
	}

	osd_clearbitmap(Machine->scrbitmap);
	sprintf (buf,"Center joystick");
	my_textout(buf);
	SCREEN_UPDATE;
	(void)osd_read_key();
	initialise_joystick();

	osd_clearbitmap(Machine->scrbitmap);
	sprintf (buf,"Move to upper left");
	my_textout(buf);
	SCREEN_UPDATE;
	(void)osd_read_key();
	calibrate_joystick_tl();

	osd_clearbitmap(Machine->scrbitmap);
	sprintf (buf,"Move to lower right");
	my_textout(buf);
	SCREEN_UPDATE;
	(void)osd_read_key();
	calibrate_joystick_br();

	if (joy_type != JOY_TYPE_WINGEX ) goto end_calibration;

	calibrate_joystick_hat(JOY_HAT_CENTRE);
	for (i=0; i<4; i++)
	{
		osd_clearbitmap(Machine->scrbitmap);
		sprintf (buf,"Move hat %s",hat_posname[i]);
		my_textout(buf);
		SCREEN_UPDATE;
		(void)osd_read_key();
		calibrate_joystick_hat(hat_pos[i]);
	}

end_calibration:

	osd_clearbitmap(Machine->scrbitmap);
	save_joystick_data(0);
}

int osd_trak_read(int axis)
{
	static int deltax;
	static int deltay;
	int mickeyx, mickeyy;
	int ret;

	if (!use_mouse)
		return(0);

	get_mouse_mickeys(&mickeyx,&mickeyy);

	deltax+=mickeyx;
	deltay+=mickeyy;

	switch(axis) {
		case X_AXIS:
			ret=deltax;
			deltax=0;
			break;
		case Y_AXIS:
			ret=deltay;
			deltay=0;
			break;
		default:
			ret=0;
			if (errorlog)
				fprintf (errorlog, "Error: no axis in osd_track_read\n");
	}

	return ret;
}




#define MAXPIXELS 200000
char * pixel[MAXPIXELS];        /* TODO: allocate this dynamically */
int p_index=-1;

inline void osd_draw_pixel (int x, int y, int col)
{
	char *address;


	if (x<0 || x >= bitmap->width)
		return;
	if (y<0 || y >= bitmap->height)
		return;
	address=&(bitmap->line[y][x]);
	*address=(char)col;
	if (p_index<MAXPIXELS-1) {
		p_index++;
		pixel[p_index]=address;
	}

	/* Mark this line as dirty */
	dirty_new[y]=1;

}

int osd_update_vectors (int *x_res, int *y_res, int step)
{
	int i;
	unsigned char bg;

	/* At most one update per MAME frame */
	vector_updates++;
	if (vector_updates>1) {
		if (errorlog) fprintf(errorlog,"Vector update #%d in the same frame.\n", vector_updates);
		return 1;
	}

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		*y_res=bitmap->width;
		*x_res=bitmap->height;
	}
	else
	{
		*x_res=bitmap->width;
		*y_res=bitmap->height;
	}

	/* Clear the old bitmap. Delete pixel for pixel, this is
	   faster than memset. */
	bg=Machine->pens[0];
	for (i=p_index; i>=0; i--)
	{
		*(pixel[i])=bg;
	}
	p_index=-1;

	return 0;
}

void osd_draw_to (int x2, int y2, int col)
{
	int orientation;
	int dx,dy,cx,cy,sx,sy;
	static int x1,y1;

#if 0
	if (errorlog)
		fprintf(errorlog,
		"line:%d,%d nach %d,%d color %d\n",
		 x1,y1,x2,y2,col);
#endif

	orientation = Machine->orientation;

	if (orientation != ORIENTATION_DEFAULT)
	{
		if (orientation & ORIENTATION_SWAP_XY)
		{
			int temp;
			temp = x2;
			x2 = y2;
			y2 = temp;
		}
		if (orientation & ORIENTATION_FLIP_X)
			x2 = bitmap->width-1-x2;
		if (orientation & ORIENTATION_FLIP_Y)
			y2 = bitmap->height-1-y2;
	}

	if (col<0)
	{
		x1=x2;
		y1=y2;
		return;
	} else
		col=Machine->gfx[0]->colortable[col];


	dx=abs(x1-x2);
	dy=abs(y1-y2);

	sx = ((x1 <= x2) ? 1 : -1);
	sy = ((y1 <= y2) ? 1 : -1);
	cx=dx/2;
	cy=dy/2;

	if (dx>=dy)
	{
		for (;;)
		{
			osd_draw_pixel(x1,y1,col);
			if (use_anti_alias)
				osd_draw_pixel(x1,y1+1,col);
			if (x1 == x2) break;
			x1+=sx;
			cx-=dy;
			if (cx < 0)
			{
				y1+=sy;
				cx+=dx;
			}
		 }
	}
	else
	{
		for (;;)
		{
			osd_draw_pixel(x1,y1,col);
			if (use_anti_alias)
				osd_draw_pixel(x1+1,y1,col);
			if (y1 == y2) break;
			y1+=sy;
			cy-=dx;
			if (cy < 0)
			{
				x1+=sx;
				cy+=dy;
			}
		}
	}
}

/* file handling routines */

/* gamename holds the driver name, filename is only used for ROMs and samples. */
/* if 'write' is not 0, the file is opened for write. Otherwise it is opened */
/* for read. */
void *osd_fopen(const char *gamename,const char *filename,int filetype,int write)
{
	char name[100];
	void *f;
	char *dirname;

	set_config_file ("mame.cfg");

	switch (filetype)
	{
		case OSD_FILETYPE_ROM:
		case OSD_FILETYPE_SAMPLE:
			sprintf(name,"%s/%s",gamename,filename);
			f = fopen(name,write ? "wb" : "rb");
			if (f == 0)
			{
				/* try with a .zip directory (if ZipMagic is installed) */
				sprintf(name,"%s.zip/%s",gamename,filename);
				f = fopen(name,write ? "wb" : "rb");
			}
			if (f == 0)
			{
				/* try with a .zif directory (if ZipFolders is installed) */
				sprintf(name,"%s.zif/%s",gamename,filename);
				f = fopen(name,write ? "wb" : "rb");
			}
			if (f == 0)
			{

				/* try again in the appropriate subdirectory */
				dirname = "";
				if (filetype == OSD_FILETYPE_ROM)
					dirname = get_config_string("directory","roms","ROMS");
				if (filetype == OSD_FILETYPE_SAMPLE)
					dirname = get_config_string("directory","samples","SAMPLES");

				sprintf(name,"%s/%s/%s",dirname,gamename,filename);
				f = fopen(name,write ? "wb" : "rb");
				if (f == 0)
				{
					/* try with a .zip directory (if ZipMagic is installed) */
					sprintf(name,"%s/%s.zip/%s",dirname,gamename,filename);
					f = fopen(name,write ? "wb" : "rb");
				}
				if (f == 0)
				{
					/* try with a .zif directory (if ZipFolders is installed) */
					sprintf(name,"%s/%s.zif/%s",dirname,gamename,filename);
					f = fopen(name,write ? "wb" : "rb");
				}
			}
			return f;
			break;
		case OSD_FILETYPE_HIGHSCORE:
			/* try again in the appropriate subdirectory */
			dirname = get_config_string("directory","hi","HI");

			sprintf(name,"%s/%s.hi",dirname,gamename);
			f = fopen(name,write ? "wb" : "rb");
			if (f == 0)
			{
				/* try with a .zip directory (if ZipMagic is installed) */
				sprintf(name,"%s.zip/%s.hi",dirname,gamename);
				f = fopen(name,write ? "wb" : "rb");
			}
			if (f == 0)
			{
				/* try with a .zif directory (if ZipFolders is installed) */
				sprintf(name,"%s.zif/%s.hi",dirname,gamename);
				f = fopen(name,write ? "wb" : "rb");
			}
			return f;
			break;
		case OSD_FILETYPE_CONFIG:
			/* try again in the appropriate subdirectory */
			dirname = get_config_string("directory","cfg","CFG");

			sprintf(name,"%s/%s.cfg",dirname,gamename);
			f = fopen(name,write ? "wb" : "rb");
			if (f == 0)
			{
				/* try with a .zip directory (if ZipMagic is installed) */
				sprintf(name,"%s.zip/%s.cfg",dirname,gamename);
				f = fopen(name,write ? "wb" : "rb");
			}
			if (f == 0)
			{
				/* try with a .zif directory (if ZipFolders is installed) */
				sprintf(name,"%s.zif/%s.cfg",dirname,gamename);
				f = fopen(name,write ? "wb" : "rb");
			}
			return f;
			break;
		case OSD_FILETYPE_INPUTLOG:
			sprintf(name,"%s.inp",filename);
			return fopen(name,write ? "wb" : "rb");
			break;
		default:
			return 0;
			break;
	}
}



int osd_fread(void *file,void *buffer,int length)
{
	return fread(buffer,1,length,(FILE *)file);
}



int osd_fwrite(void *file,const void *buffer,int length)
{
	return fwrite(buffer,1,length,(FILE *)file);
}



int osd_fseek(void *file,int offset,int whence)
{
	return fseek((FILE *)file,offset,whence);
}



void osd_fclose(void *file)
{
	fclose((FILE *)file);
}



static int leds=0;
static const int led_flags[3] = {
  KB_NUMLOCK_FLAG,
  KB_CAPSLOCK_FLAG,
  KB_SCROLOCK_FLAG
};
void osd_led_w(int led,int on) {
  int temp=leds;
  if (led<3) {
    if (on&1)
	temp |=  led_flags[led];
    else
	temp &= ~led_flags[led];
    if (temp!=leds) {
	leds=temp;
	set_leds (leds);
    }
  }
}


int osd_get_config_samplerate (int def_samplerate)
{
	set_config_file ("mame.cfg");
	return get_config_int("config","samplerate",def_samplerate);
}

int osd_get_config_samplebits (int def_samplebits)
{
	set_config_file ("mame.cfg");
	return get_config_int("config","samplebits",def_samplebits);
}

int osd_get_config_frameskip (int def_frameskip)
{
	set_config_file ("mame.cfg");
	return get_config_int("config","frameskip",def_frameskip);
}

void osd_set_config (int def_samplerate, int def_samplebits)
{
	/* set config data in mame.cfg - V.V_121997 */
	set_config_file ("mame.cfg");

	/* set shared (not os-dependant) data */
	if (get_config_int("config","samplerate",-1) == -1)
		set_config_int("config","samplerate",def_samplerate);
	if (get_config_int("config","samplebits",-1) == -1)
		set_config_int("config","samplebits",def_samplebits);
	if (get_config_int("config","frameskip",-1) == -1)
		set_config_int("config","frameskip",0);

	/* set msdos config data */
	if (get_config_string("config","scanlines",0) == 0)
		set_config_string("config","scanlines","yes");
	if (get_config_string("config","double",0) == 0)
		set_config_string("config","double","auto");
	if (get_config_string("config","vsync",0) == 0)
		set_config_string("config","vsync","no");
	if (get_config_string("config","oplfm",0) == 0)
		set_config_string("config","oplfm","no");
	if (get_config_string("config","antialias",0) == 0)
		set_config_string("config","antialias","no");
	if (get_config_string("config","mouse",0) == 0)
		set_config_string("config","mouse","yes");
	if (get_config_string("config","vesa",0) == 0)
		set_config_string("config","vesa","no");
	if (get_config_string("config","syncedtweak",0) == 0)
		set_config_string("config","syncedtweak","yes");

	if (get_config_int("config","soundcard",100) == 100)
		set_config_int("config","soundcard",-1);
	if (get_config_int("config","vgafreq",100) == 100)
		set_config_int("config","vgafreq",0);
	if (get_config_int("config","joytype",100) == 100)
		set_config_int("config","joytype",-1);
	if (get_config_float("config","gamma",0.0) == 0.0)
		set_config_float("config","gamma",1.0);

	/* set default subdirectories */
	if (get_config_string("directory","roms",0) == 0)
		set_config_string("directory","roms","ROMS");
	if (get_config_string("directory","samples",0) == 0)
		set_config_string("directory","samples","SAMPLES");
	if (get_config_string("directory","hi",0) == 0)
		set_config_string("directory","hi","HI");
	if (get_config_string("directory","cfg",0) == 0)
		set_config_string("directory","cfg","CFG");
	if (get_config_string("directory","pcx",0) == 0)
		set_config_string("directory","pcx","PCX");
}

void osd_save_config (int frameskip, int samplerate, int samplebits)
{
	/* save config data in mame.cfg - V.V_121997 */
	set_config_file ("mame.cfg");

	/* set shared (not os-dependant) data */
	set_config_int("config","samplerate",samplerate);
	set_config_int("config","samplebits",samplebits);
	set_config_int("config","frameskip",frameskip);
}
