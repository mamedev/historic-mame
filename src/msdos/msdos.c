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
#include "TwkUser.c"
#include <allegro.h>
#include "driver.h"
#include <audio.h>
#include "ym2203.h"

struct osd_bitmap *bitmap;


unsigned char current_palette[256][3];
PALETTE adjusted_palette;
unsigned char dirtycolor[256];

int videofreq;
int video_sync;
int play_sound;
int use_joystick;
int use_mouse;
int use_alternate;
int vector_game;
int vector_updates;
int vector_sign;
int use_anti_alias;
int use_dirty;

int vesa_mode;
int vesa_width;
int vesa_height;
int skiplines;
int vesa_xoffset;
int vesa_yoffset;
int vesa_display_lines;

int noscanlines;
int nodouble;

struct { char *desc; int x, y; } gfx_res[] = {
	{ "-224x288"    , 224, 288 },
	{ "-224"        , 224, 288 },
	{ "-320x240"	, 320, 240 },
	{ "-320"	, 320, 240 },
	{ "-512x384"	, 512, 384 },
	{ "-512"	, 512, 384 },
	{ "-640x480"	, 640, 480 },
	{ "-640x400"	, 640, 400 },
	{ "-640"	, 640, 480 },
	{ "-800x600"	, 800, 600 },
	{ "-800"	, 800, 600 },
	{ "-1024x768"	, 1024, 768 },
	{ "-1024"	, 1024, 768 },
	{ "-1280x1024"	, 1280, 1024 },
	{ "-1280"	, 1280, 1024 },
	{ "-1600x1200"	, 1600, 1200 },
	{ "-1600"	, 1600, 1200 },
	{ NULL		, 0, 0 }
	};

char *vesa_desc[6]= {
	"AUTO??", "VGA??", "MODEX??",
	"VESA 1.2", "VESA 2.0 (banked)", "VESA 2 (linear)"
};


/* audio related stuff */
#define NUMVOICES 16
#define SAMPLE_RATE 44100
HAC hVoice[NUMVOICES];
LPAUDIOWAVE lpWave[NUMVOICES];
int Volumi[NUMVOICES];
int MasterVolume = 100;
unsigned char No_FM = 0;
unsigned char RegistersYM[264*5];  /* MAX 5 YM-2203 */


/* put here anything you need to do when the program is started. Return 0 if */
/* initialization was successful, nonzero otherwise. */
int osd_init(int argc,char **argv)
{
	int i,j;
	int soundcard;


	vector_game = 0;
	vector_updates = 0;
	use_anti_alias = -1; /* auto */
	use_dirty = 0;
	vesa_mode = 0;
	vesa_width = 0;
	vesa_height = 0;
	noscanlines = 0;
	nodouble = 0;
	videofreq = 0;
	video_sync = 0;
	play_sound = 1;
	use_joystick = 1;
	use_mouse = 1;
	soundcard = -1;
	for (i = 1;i < argc;i++)
	{

		if (stricmp(argv[i],"-vesa1") == 0)
			vesa_mode = GFX_VESA1;
		if (stricmp(argv[i],"-vesa2b") == 0)
			vesa_mode = GFX_VESA2B;
		if (stricmp(argv[i],"-vesa2l") == 0)
			vesa_mode = GFX_VESA2L;
		if (stricmp(argv[i],"-vesa") == 0)
			vesa_mode = GFX_VESA2L;
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
					if (!vesa_mode)
						vesa_mode = GFX_VESA2L;
					vesa_width = gfx_res[j].x;
					vesa_height = gfx_res[j].y;
				}
				break;
			}
		}
		if (stricmp(argv[i],"-noscanlines") == 0)
			noscanlines = 1;
		if (stricmp(argv[i],"-nodouble") == 0)
			nodouble = 1;
		if (stricmp(argv[i],"-vsync") == 0)
			video_sync = 1;

		if (stricmp(argv[i],"-soundcard") == 0)
		{
			i++;
			if (i < argc) soundcard = atoi(argv[i]);
		}
		if (stricmp(argv[i],"-vgafreq") == 0)
		{
			i++;
			if (i < argc) videofreq = atoi(argv[i]);
			if (videofreq < 0) videofreq = 0;
			if (videofreq > 3) videofreq = 3;
		}
		if (stricmp(argv[i],"-nojoy") == 0)
			use_joystick = 0;
                if (stricmp(argv[i],"-nofm") == 0)
                        No_FM = 1;

                /* NTB */
                if (stricmp(argv[i],"-nomouse") == 0)
			use_mouse = 0;

		if (stricmp(argv[i],"-aa") == 0)
		 	use_anti_alias=1;
		if (stricmp(argv[i],"-naa") == 0)
			use_anti_alias=0;

		/* Just in case */
		/* Does not work yet since MAME does not cut the */
		/* game name out of the argv field. */
		/* printf ("Warning: unrecognized option %s.\n",argv[i]);*/
	}

	if (play_sound)
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

			if (soundcard == 0)	/* silence */
				play_sound = 0;
			else
			{
				/* open audio device */
/*				info.nDeviceId = AUDIO_DEVICE_MAPPER;*/
				info.nDeviceId = soundcard;
				info.wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO;
				info.nSampleRate = SAMPLE_RATE;
				if (AOpenAudio(&info) != AUDIO_ERROR_NONE)
				{
					printf("audio initialization failed\n");
					return 1;
				}

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
	install_keyboard();		/* Allegro keyboard handler */
	set_leds(0);	/* turn off all leds */

	if (use_joystick)
	{
		/* Try to install Allegro joystick handler */
		/* Support 4 button sticks */

		joy_type=JOY_TYPE_4BUTTON;
		if (initialise_joystick())
			use_joystick = 0; /* joystick not found */

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
		if (vesa_mode && !vesa_width)
		{
			vesa_width=640;
			vesa_height=480;
		}
	}
	if (Machine->drv->video_attributes & VIDEO_SUPPORTS_DIRTY)
		use_dirty=1;

	return 0;
}



/* put here cleanup routines to be executed when the program is terminated. */
void osd_exit(void)
{
	if (play_sound)
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


/* Create a bitmap. Also calls clearbitmap() to appropriately initialize it to */
/* the background color. */
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

	if ((bitmap = malloc(sizeof(struct osd_bitmap) + (height-1)*sizeof(unsigned char *))) != 0)
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
		free(bitmap->_private);
		free(bitmap);
	}
}



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
	{ 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, 0x5f},{ 0x3d4, 0x01, 0x37},
	{ 0x3d4, 0x02, 0x38},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x4a},
	{ 0x3d4, 0x05, 0x9a},{ 0x3d4, 0x06, 0x55},{ 0x3d4, 0x07, 0xf0},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x61},{ 0x3d4, 0x10, 0x40},
	{ 0x3d4, 0x11, 0xac},{ 0x3d4, 0x12, 0x3f},{ 0x3d4, 0x13, 0x1c},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0x40},{ 0x3d4, 0x16, 0x4a},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x0}
};

Register scr224x288scanlines[] =
{
	{ 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, 0x5f},{ 0x3d4, 0x01, 0x37},
	{ 0x3d4, 0x02, 0x38},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x4a},
	{ 0x3d4, 0x05, 0x9a},{ 0x3d4, 0x06, 0x43},{ 0x3d4, 0x07, 0x1f},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x60},{ 0x3d4, 0x10, 0x2a},
	{ 0x3d4, 0x11, 0xac},{ 0x3d4, 0x12, 0x1f},{ 0x3d4, 0x13, 0x1c},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0x27},{ 0x3d4, 0x16, 0x3a},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x00}
};


Register scr256x256[] =
{
	{ 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, 0x5f},{ 0x3d4, 0x01, 0x3f},
	{ 0x3d4, 0x02, 0x40},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x4A},
	{ 0x3d4, 0x05, 0x9A},{ 0x3d4, 0x06, 0x23},{ 0x3d4, 0x07, 0xb2},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x61},{ 0x3d4, 0x10, 0x0a},
	{ 0x3d4, 0x11, 0xac},{ 0x3d4, 0x12, 0xff},{ 0x3d4, 0x13, 0x20},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0x07},{ 0x3d4, 0x16, 0x1a},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x00}
};

Register scr256x256_synced[] = /* V.V */
{
	{ 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, 0x62},{ 0x3d4, 0x01, 0x3f},
	{ 0x3d4, 0x02, 0x40},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x4A},
	{ 0x3d4, 0x05, 0x9A},{ 0x3d4, 0x06, 0x42},{ 0x3d4, 0x07, 0xb2},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x61},{ 0x3d4, 0x10, 0x14},
	{ 0x3d4, 0x11, 0xac},{ 0x3d4, 0x12, 0xff},{ 0x3d4, 0x13, 0x20},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0x07},{ 0x3d4, 0x16, 0x1a},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x00}
};

Register scr256x256scanlines[] =
{
	{ 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, 0x5f},{ 0x3d4, 0x01, 0x3f},
	{ 0x3d4, 0x02, 0x40},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x4a},
	{ 0x3d4, 0x05, 0x9a},{ 0x3d4, 0x06, 0x23},{ 0x3d4, 0x07, 0x1d},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x60},{ 0x3d4, 0x10, 0x0a},
	{ 0x3d4, 0x11, 0xac},{ 0x3d4, 0x12, 0xff},{ 0x3d4, 0x13, 0x20},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0x07},{ 0x3d4, 0x16, 0x1a},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x00}
};

Register scr288x224[] =
{
	{ 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, 0x5f},{ 0x3d4, 0x01, 0x47},
	{ 0x3d4, 0x02, 0x50},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x50},
	{ 0x3d4, 0x05, 0x80},{ 0x3d4, 0x06, 0x0b},{ 0x3d4, 0x07, 0x3e},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x41},{ 0x3d4, 0x10, 0xda},
	{ 0x3d4, 0x11, 0x9c},{ 0x3d4, 0x12, 0xbf},{ 0x3d4, 0x13, 0x24},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0xc7},{ 0x3d4, 0x16, 0x04},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x0}
};

Register scr288x224scanlines[] =
{
	{ 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, 0x5f},{ 0x3d4, 0x01, 0x47},
	{ 0x3d4, 0x02, 0x47},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x50},
	{ 0x3d4, 0x05, 0x9a},{ 0x3d4, 0x06, 0x0b},{ 0x3d4, 0x07, 0x19},
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


/* Create a display screen, or window, large enough to accomodate a bitmap */
/* of the given dimensions. Attributes are the ones defined in driver.h. */
/* palette is an array of 'totalcolors' R,G,B triplets. The function returns */
/* in *pens the pen values corresponding to the requested colors. */
/* Return a osd_bitmap pointer or 0 in case of error. */
struct osd_bitmap *osd_create_display(int width,int height,int totalcolors,
		const unsigned char *palette,unsigned char *pens,int attributes)
{
	Register *reg = 0;
	int reglen = 0;

	double x_scale, y_scale, scale;

	/* Mark the dirty buffers as dirty */
	if (use_dirty)
	{
		init_dirty(1);
		swap_dirty();
		init_dirty(1);
	}

	if (vector_game && vesa_mode)
	{
		if (Machine->orientation & ORIENTATION_SWAP_XY)
		{
			x_scale=(double)vesa_width/(double)(height);
			y_scale=(double)vesa_height/(double)(width);
		}
		else
		{
			x_scale=(double)vesa_width/(double)(width);
			y_scale=(double)vesa_height/(double)(height);
		}
		if (x_scale<y_scale)
			scale=x_scale;
		else
			scale=y_scale;
		width=(int)((double)width*scale);
		height=(int)((double)height*scale);
		if (nodouble)
		{
			height = height/2;
			width = width/2;
		}
		/* Padding to an dword value */
		width-=width % 4;
		height-=height % 4;

	}

	bitmap = osd_create_bitmap(width,height);

	/*  anti aliasing? */
	if (use_anti_alias == -1)
	{
		if (height > 480)
			use_anti_alias=1;
		else
			use_anti_alias=0;
	}

	if (Machine->orientation & ORIENTATION_SWAP_XY)
	{
		int temp;

		temp = width;
		width = height;
		height = temp;
	}


 	/* Check if there exists a tweaked mode to fit
           the screen in, otherwise use VESA */
	if (!vesa_mode &&
			!(width == 224 && height == 288) &&
			!(width <= 256 && height <= 256) &&
			!(width == 288 && height == 224) &&
			!(width == 320 && height == 204) &&
			!(width == 240 && height == 272))
		vesa_mode = GFX_VESA2L;

        /* If we still don't know about the VESA resolution to use,
           we now choose a sensible one. Only 640x480 and 800x600
           are supported by all VESA drivers. */

        if (vesa_mode && !vesa_width)
        {
                if (((height<=240) && (width <=320)) ||
                    ((height> 300) && (width <=640)))
                {
                        vesa_width=640;
                        vesa_height=480;
                }
                /* Hopefully no game needs more than 800x600 */
                else
                {       vesa_width=800;
                        vesa_height=600;
                }
        }

 	if (vesa_mode)
  	{
 		int mode;

		/* Try the specified vesamode and all lower ones BW 131097 */
		for (mode=vesa_mode; mode>=GFX_VESA1; mode--)
		{
			if (errorlog)
				fprintf (errorlog,"Trying %s\n", vesa_desc[mode]);
			if (set_gfx_mode(mode,vesa_width,vesa_height,0,0) == 0)
				break;
			if (errorlog)
				fprintf (errorlog,"%s\n",allegro_error);
 		}
		if (mode < GFX_VESA1) {
			printf ("No %dx%d VESA modes available. Try using UNIVBE\n", vesa_width,vesa_height);
			return 0;
		}

		vesa_mode = mode;

 		if ((width > vesa_width/2) || nodouble)
 		{
                        nodouble = 1;
 			vesa_yoffset = (vesa_height - height) / 2;
 			vesa_xoffset = (vesa_width - width) / 2;
 			vesa_display_lines = height;
 			if (vesa_height < vesa_display_lines)
 				vesa_display_lines = vesa_height;
 		}
 		else
 		{
 			vesa_yoffset = (vesa_height - height * 2) / 2;
 			vesa_xoffset = (vesa_width - width * 2) / 2;
 			vesa_display_lines = height;
 			if (vesa_height/2 < vesa_display_lines)
 				vesa_display_lines = vesa_height / 2;
 		}
 		/* Allign on a quadword !*/
 		vesa_xoffset -= vesa_xoffset % 4;
 		/* Just in case */
 		if (vesa_yoffset < 0) vesa_yoffset = 0;
 		if (vesa_xoffset < 0) vesa_xoffset = 0;
  		if (vesa_display_lines+skiplines > height)
			skiplines=height-vesa_display_lines;
	}
	else
	{
		/* big hack: open a mode 13h screen using Allegro, then load the custom screen */
		/* definition over it. */
		if (set_gfx_mode(GFX_VGA,320,200,0,0) != 0)
			return 0;

		if (width == 224 && height == 288)
		{
			if (noscanlines)
			{
				reg = scr224x288;
				reglen = sizeof(scr224x288)/sizeof(Register);
			}
			else
			{
                                if (use_alternate)
                                  reg = scr224x288_alternate;
                                else
			          reg = scr224x288scanlines;
				reglen = sizeof(scr224x288scanlines)/sizeof(Register);
			}
		}
		else if (width <= 256 && height <= 256)
		{
			if (noscanlines)
			{
				if (video_sync && videofreq == 1) /* V.V */
				  reg = scr256x256_synced;
				else
				  reg = scr256x256;
				reglen = sizeof(scr256x256)/sizeof(Register);
			}
			else
			{
				reg = scr256x256scanlines;
				reglen = sizeof(scr256x256scanlines)/sizeof(Register);
			}
		}
		else if (width == 288 && height == 224)
		{
			if (noscanlines)
			{
				reg = scr288x224;
				reglen = sizeof(scr288x224)/sizeof(Register);
			}
			else
			{
				reg = scr288x224scanlines;
				reglen = sizeof(scr288x224scanlines)/sizeof(Register);
			}
		}
                else if (width == 240 && height == 272)
                {
			reg = scr240x272;
			reglen = sizeof(scr240x272)/sizeof(Register);
                }
		else if (width == 320 && height == 204)
		{
			reg = scr320x204;
			reglen = sizeof(scr320x204)/sizeof(Register);
		}

		/* set the VGA clock */
                if (!use_alternate)
		  reg[0].value = (reg[0].value & 0xf3) | (videofreq << 2);

		outRegArray(reg,reglen);
	}

	/* initialize the palette */
	{
		int i;

		for (i = 0;i < 256;i++)
		{
			current_palette[i][0] = current_palette[i][1] = current_palette[i][2] = 0;
			dirtycolor[i] = 1;
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



/* shut up the display */
void osd_close_display(void)
{
	set_gfx_mode(GFX_TEXT,80,25,0,0);
	osd_free_bitmap(bitmap);
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



inline void double_pixels(unsigned long *lb, short seg,
			  unsigned long address, int width4)
{
	__asm__ __volatile__ ("
	pushw %%es		\n
	movw %%dx, %%es		\n
	cld			\n
	.align 4		\n
	0:			\n
	lodsw			\n
	rorl $8, %%eax		\n
	movb %%al, %%ah		\n
	roll $16, %%eax		\n
	movb %%ah, %%al		\n
	stosl			\n
	lodsw			\n
	rorl $8, %%eax		\n
	movb %%al, %%ah		\n
	roll $16, %%eax		\n
	movb %%ah, %%al		\n
	stosl			\n
	loop 0b			\n
	popw %%ax		\n
	movw %%ax, %%es		\n
	"
	::
	"d" (seg),
	"c" (width4),
	"S" (lb),
	"D" (address):
	"ax", "bx", "cx", "dx", "si", "di", "cc", "memory");
}

inline void update_vesa(void)
{
	short src_seg, dest_seg;
	int y, vesa_line, width4;
	unsigned long *lb, address;

 	src_seg	= _my_ds();
	dest_seg = screen->seg;
	vesa_line = vesa_yoffset;
	width4 = bitmap->width /4;
	lb = bitmap->_private + bitmap->width*skiplines;

	for (y = 0; y < vesa_display_lines; y++)
	{
		if (use_dirty)
		if ((dirty_new[y+skiplines]+dirty_old[y+skiplines]) == 0) {
			lb+=width4;
			vesa_line++;
			if (!nodouble)
				vesa_line++;
			/* No need to update. Next line */
			continue;
		}

		address = bmp_write_line (screen, vesa_line) + vesa_xoffset;
		if (!nodouble)
		{
			double_pixels(lb,dest_seg,address,width4);
                        if (noscanlines) {
		           address = bmp_write_line (screen, vesa_line+1) + vesa_xoffset;
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


	if (video_sync) vsync();


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
		}
		if (gamma_update > 0.09)
		{
			gamma_update = 0.00;
			gamma_correction += 0.10;
		}

		if (gamma_correction < 0.2) gamma_correction = 0.2;
		if (gamma_correction > 3.0) gamma_correction = 3.0;

		showgammatemp = Machine->drv->frames_per_second;
	}

	if (showgammatemp > 0)
	{
		char buf[20];
		int trueorientation,l,x,y;


		showgammatemp--;

		/* hack: force the display into standard orientation to avoid */
		/* rotating the text */
		trueorientation = Machine->orientation;
		Machine->orientation = ORIENTATION_DEFAULT;

		sprintf(buf,"Gamma = %1.1f",gamma_correction);
		l = strlen(buf);
		x = (Machine->scrbitmap->width - Machine->uifont->width * l) / 2;
		y = (Machine->scrbitmap->height - Machine->uifont->height) / 2;
		for (i = 0;i < l;i++)
			drawgfx(Machine->scrbitmap,Machine->uifont,buf[i],DT_COLOR_WHITE,0,0,
					x + i*Machine->uifont->width,y,0,TRANSPARENCY_NONE,0);

		Machine->orientation = trueorientation;
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


	if (vesa_mode)
	{
		update_vesa();

		/* Check for PGUP, PGDN and scroll screen */
		if (osd_key_pressed(OSD_KEY_PGDN) &&
			(skiplines+vesa_display_lines < bitmap->height))
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
	else 	/* no vesa-modes */
	{
		/* copy the bitmap to screen memory */
		_dosmemputl(bitmap->_private,bitmap->width * bitmap->height / 4,0xa0000);
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
		char name[13];
		FILE *f;
		static int snapno;
		int y;


		do
		{
			sprintf(name,"snap%04d.pcx",snapno);
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
	if (play_sound == 0) return;

	AUpdateAudio();
}



void osd_play_sample(int channel,unsigned char *data,int len,int freq,int volume,int loop)
{
	if (play_sound == 0 || channel >= NUMVOICES) return;

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

		if (loop) lpWave[channel]->wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO | AUDIO_FORMAT_LOOP;
		else lpWave[channel]->wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO;
		lpWave[channel]->nSampleRate = SAMPLE_RATE;
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



void osd_play_streamed_sample(int channel,unsigned char *data,int len,int freq,int volume)
{
	static int playing[NUMVOICES];
	static int c[NUMVOICES];


	if (play_sound == 0 || channel >= NUMVOICES) return;

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

		lpWave[channel]->wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO | AUDIO_FORMAT_LOOP;
		lpWave[channel]->nSampleRate = SAMPLE_RATE;
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


		if (throttle)	/* sync with audio only when speed throttling is not turned off */
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



void osd_adjust_sample(int channel,int freq,int volume)
{
	if (play_sound == 0 || channel >= NUMVOICES) return;

        Volumi[channel] = volume/4;
	ASetVoiceFrequency(hVoice[channel],freq);
	ASetVoiceVolume(hVoice[channel],MasterVolume*volume/400);
}



void osd_stop_sample(int channel)
{
	if (play_sound == 0 || channel >= NUMVOICES) return;

	AStopVoice(hVoice[channel]);
}


void osd_restart_sample(int channel)
{
	if (play_sound == 0 || channel >= NUMVOICES) return;

	AStartVoice(hVoice[channel]);
}


int osd_get_sample_status(int channel)
{
        int stopped=0;
	if (play_sound == 0 || channel >= NUMVOICES) return -1;

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
        clear_keybuf();
        return readkey() >> 8;
}


/* return the name of a key */
const char *osd_key_name(int keycode)
{
	static char *keynames[] =
	{
		"ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9",
		"0", "MINUS", "EQUAL", "BACKSPACE", "TAB", "Q", "W", "E", "R", "T",
		"Y", "U", "I", "O", "P", "OPENBRACE", "CLOSEBRACE", "ENTER", "LEFTCONTROL",	"A",
		"S", "D", "F", "G", "H", "J", "K", "L", "COLON", "QUOTE",
		"TILDE", "LEFTSHIFT", "Error", "Z", "X", "C", "V", "B", "N", "M",
		"COMMA", "STOP", "SLASH", "RIGHTSHIFT", "ASTERISK", "ALT", "SPACE", "CAPSLOCK",	"F1", "F2",
		"F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "NUMLOCK",	"SCRLOCK",
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
   static char *joynames[] =
   {
      "Left", "Right", "Up", "Down", "Button A",
      "Button B", "Button C", "Button D", "Button E", "Button F",
      "Button G", "Button H", "Button I", "Button J", "Any Button",
      "J2 Left", "J2 Right", "J2 Up", "J2 Down", "J2 Button A",
      "J2 Button B", "J2 Button C", "J2 Button D", "J2 Button E", "J2 Button F",
      "J2 Button G", "J2 Button H", "J2 Button I", "J2 Button J", "J2 Any Button",
   };

   if (joycode && joycode <= OSD_MAX_JOY)
      return (char *)joynames[joycode-1];

   return "Unknown";
}


void osd_poll_joystick(void)
{
	if (use_joystick == 1)
		poll_joystick();
}


/* check if the joystick is moved in the specified direction, defined in */
/* osdepend.h. Return 0 if it is not pressed, nonzero otherwise. */
int osd_joy_pressed(int joycode)
{
	/* compiler bug? If I don't declare these variables as volatile, */
	/* joystick right is not detected */
	extern volatile int joy_left, joy_right, joy_up, joy_down;
	extern volatile int joy_b1, joy_b2, joy_b3, joy_b4;

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
		case OSD_JOY_FIRE:
			return (joy_b1 || joy_b2 || joy_b3 || joy_b4 || mouse_b);
			break;
		default:
//			if (errorlog)
//				fprintf (errorlog,"Error: osd_joy_pressed called with no valid joyname\n");
			break;
	}
	return 0;
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




#define MAXPIXELS 100000
char * pixel[MAXPIXELS];
int p_index=-1;

int dirty_sequence_nr;

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

	/* This is a _very_ simple scheme to support "dirty" lines */
	/* It can result in artefacts remaining on the screen for  */
	/* a while. BW 141097 */
#if 0   /* try it out yourself */
	if (vector_sign)
		dirty_new[y]+=(col+(x<<4));
	else
		dirty_new[y]-=(col+(x<<4));
	dirty_new[y]^=((col+(x<<4))<<16);
#else
	dirty_new[y]=1;
#endif
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
			return f;
			break;
		case OSD_FILETYPE_HIGHSCORE:
			sprintf(name,"hi/%s.hi",gamename);
			return fopen(name,write ? "wb" : "rb");
			break;
		case OSD_FILETYPE_CONFIG:
			sprintf(name,"cfg/%s.cfg",gamename);
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
