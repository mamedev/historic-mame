/***************************************************************************

  osdepend.c

  OS dependant stuff (display handling, keyboard scan...)
  This is the only file which should me modified in order to port the
  emulator to a different system.

***************************************************************************/

#include <pc.h>
#include <conio.h>
#include <sys/farptr.h>
#include <go32.h>
#include "TwkUser.c"
#include <allegro.h>
#include "driver.h"
#include <audio.h>
#include "ym2203.h"


#define ROL -1
#define ROR 1

struct osd_bitmap *bitmap;
int first_free_pen;

int videofreq;
int video_sync;
int play_sound;
int use_joystick;
int use_alternate;

int rotation;
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
	{ NULL		, 0, 0 }
	};


/* NTB: Mouse specific variables. */
static int mouse_left = 0, mouse_right = 0, mouse_up = 0, mouse_down = 0;
static int mouse_ox, mouse_oy;
static int mouse_poll_count = 0;
static int use_mouse = 0;

#define MOUSE_CENTRE_X 160
#define MOUSE_CENTRE_Y 120
#define MOUSE_THRESHOLD 0
#define TAN_22_5        106
#define TAN_67_5        618

void osd_poll_mouse(void);
int osd_mouse_pressed(int joycode);


/*
 *   Trackball Related stuff
 */

extern volatile int mouse_x, mouse_y;
static int use_trak = 0;
static int large_trak_x = 0;
static int large_trak_y = 0;

#define TRAK_MAXX_RES 120
#define TRAK_MAXY_RES 120
#define TRAK_CENTER_X TRAK_MAXX_RES/2
#define TRAK_CENTER_Y TRAK_MAXY_RES/2
#define TRAK_MIN_X TRAK_MAXX_RES/6
#define TRAK_MAX_X TRAK_MAXX_RES*5/6
#define TRAK_MIN_Y TRAK_MAXY_RES/6
#define TRAK_MAX_Y TRAK_MAXY_RES*5/6

int osd_trak_read(int axis);
int osd_trak_pressed(int joycode);


/* audio related stuff */
#define NUMVOICES 8
#define SAMPLE_RATE 44100
#define SAMPLE_BUFFER_LENGTH 50000
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


	first_free_pen = 0;

	vesa_mode = 0;
	vesa_width = 800;
	vesa_height = 600;
	noscanlines = 0;
	nodouble = 0;
	rotation = 0;
	videofreq = 0;
	video_sync = 0;
	play_sound = 1;
	use_joystick = 1;
	soundcard = -1;
	for (i = 1;i < argc;i++)
	{

		if (stricmp(argv[i],"-vesa") == 0)
			vesa_mode = GFX_VESA1;
		if (stricmp(argv[i],"-vesa2b") == 0)
			vesa_mode = GFX_VESA2B;
		if (stricmp(argv[i],"-vesa2l") == 0)
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
					vesa_mode = GFX_VESA1;
				  vesa_width=gfx_res[j].x;
				  vesa_height=gfx_res[j].y;
                                }
				break;
			}
		}
		if (stricmp(argv[i],"-ror") == 0)
			rotation=ROR;
		if (stricmp(argv[i],"-rol") == 0)
			rotation=ROL;
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
                if (stricmp(argv[i],"-mouse") == 0)
                {
                    use_mouse = 1;
                    use_joystick = 0;
                }

		if (stricmp(argv[i],"-trak") == 0)
		{
		    use_mouse = 0;
                    use_joystick = 1;
                    use_trak = 1;
                }
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
				printf("\nSelect the audio device:\n(if you have an AWE 32, choose Sound Blaster for a more faithful emulation)\n");
				for (i = 0;i < AGetAudioNumDevs();i++)
				{
					if (AGetAudioDevCaps(i,&caps) == AUDIO_ERROR_NONE)
						printf("  %2d. %s\n",i,caps.szProductName);
				}
				printf("\n");

                                if (i < 10)
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

					if ((lpWave[i] = (LPAUDIOWAVE)malloc(sizeof(AUDIOWAVE))) == 0)
/* have to handle this better...*/
						return 1;

					lpWave[i]->wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO;
					lpWave[i]->nSampleRate = SAMPLE_RATE;
					lpWave[i]->dwLength = SAMPLE_BUFFER_LENGTH;
					lpWave[i]->dwLoopStart = lpWave[i]->dwLoopEnd = 0;
					if (ACreateAudioData(lpWave[i]) != AUDIO_ERROR_NONE)
					{
/* have to handle this better...*/
						free(lpWave[i]);

						return 1;
					}
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

	if (use_joystick)
	{
		/* Try to install Allegro joystick handler */
		if (initialise_joystick())
			use_joystick = 0; /* joystick not found */
	}

        /* NTB: Initialiase mouse */
        if( use_mouse || use_trak )
        {
            if( install_mouse() == - 1 )
            {
                use_mouse = 0;
		use_trak = 0;
            }
            else
            {
	        if( use_mouse )
                {
                    set_mouse_speed( 0, 0 );
                    position_mouse( MOUSE_CENTRE_X, MOUSE_CENTRE_Y );
                    mouse_ox = MOUSE_CENTRE_X;
                    mouse_oy = MOUSE_CENTRE_Y;
                }
            }
        }

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
			ADestroyAudioData(lpWave[n]);
			free(lpWave[n]);
		}
		ACloseVoices();
		ACloseAudio();
	}
}



/* Create a bitmap. Also call clearbitmap() to appropriately initialize it to */
/* the background color. */
struct osd_bitmap *osd_create_bitmap(int width,int height)
{
	struct osd_bitmap *bitmap;


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

		clearbitmap(bitmap);
	}

	return bitmap;
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


Register scr256x232[] =
{
        { 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, 0x5f},{ 0x3d4, 0x01, 0x3f},
        { 0x3d4, 0x02, 0x40},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x4a},
        { 0x3d4, 0x05, 0x9a},{ 0x3d4, 0x06, 0x11},{ 0x3d4, 0x07, 0x3e},
        { 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x41},{ 0x3d4, 0x10, 0xe5},
        { 0x3d4, 0x11, 0x9c},{ 0x3d4, 0x12, 0xcf},{ 0x3d4, 0x13, 0x20},
        { 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0xd7},{ 0x3d4, 0x16, 0x04},
        { 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x04, 0x0e},
        { 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41},
        { 0x3c0, 0x13, 0x00}
};

Register scr256x232scanlines[] =
{
        { 0x3c2, 0x00, 0xe3},{ 0x3d4, 0x00, 0x5f},{ 0x3d4, 0x01, 0x3f},
        { 0x3d4, 0x02, 0x40},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x4a},
        { 0x3d4, 0x05, 0x9a},{ 0x3d4, 0x06, 0x12},{ 0x3d4, 0x07, 0x1d},
        { 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x60},{ 0x3d4, 0x10, 0x00},
        { 0x3d4, 0x11, 0xac},{ 0x3d4, 0x12, 0xe7},{ 0x3d4, 0x13, 0x20},
        { 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0x07},{ 0x3d4, 0x16, 0x1a},
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

/* Create a display screen, or window, large enough to accomodate a bitmap */
/* of the given dimensions. I don't do any test here (224x288 will just do */
/* for now) but one could e.g. open a window of the exact dimensions */
/* provided. Return a osd_bitmap pointer or 0 in case of error. */
struct osd_bitmap *osd_create_display(int width,int height)
{
	Register *reg = 0;
	int reglen = 0;

 	/* Check if there exists a tweaked mode to fit
           the screen in, otherwise use VESA */

	bitmap = osd_create_bitmap(width,height);

        if (rotation)
	{
		int t;
		if (!vesa_mode) vesa_mode = GFX_VESA1;
		t = width; width = height; height = t;
	}


	if (!vesa_mode &&
	    !(width == 224 && height == 288) &&
            !(width == 256 && height == 256) &&
            !(width == 256 && height == 232) &&
	    !(width == 288 && height == 224) &&
	    !(width == 320 && height == 204) &&
            !(width == 240 && height == 272))
 		vesa_mode = GFX_VESA1;

 	if (vesa_mode)
  	{
 		if (set_gfx_mode(vesa_mode,vesa_width,vesa_height,0,0) != 0)
  		{
			printf ("%dx%d not possible\n", vesa_width,vesa_height);
			printf ("%s\n",allegro_error);
			return 0;
 		}
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
		else if (width == 256 && height == 256)
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
		else if (width == 256 && height == 232)
		{
			if (noscanlines)
			{
			        reg = scr256x232;
				reglen = sizeof(scr256x232)/sizeof(Register);
			}
			else
			{
				reg = scr256x232scanlines;
				reglen = sizeof(scr256x232scanlines)/sizeof(Register);
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

	/* We need to do this AFTER the screen is made... */
	if( use_trak ) {
	  set_mouse_speed( 0, 0 );
          set_mouse_range(0, 0, TRAK_MAXX_RES-1, TRAK_MAXY_RES-1);
	  position_mouse( TRAK_CENTER_X, TRAK_CENTER_Y );
	}

	return bitmap;
}



/* shut up the display */
void osd_close_display(void)
{
	set_gfx_mode(GFX_TEXT,80,25,0,0);
	osd_free_bitmap(bitmap);
}



int osd_obtain_pen(unsigned char red, unsigned char green, unsigned char blue)
{
	RGB rgb;


	rgb.r = red >> 2;
	rgb.g = green >> 2;
	rgb.b = blue >> 2;
	set_color(first_free_pen,&rgb);

	return first_free_pen++;
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

	for (y = vesa_display_lines;y !=0 ; y--)
	{
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
		lb += width4;
	}
}

inline void rotate(unsigned char *lb, short seg,				
			  unsigned long address, int offset, int width4)
{									
	__asm__ __volatile__ ("						\
	pushw %%es		;					\
	movw %%dx, %%es		;					\
	.align 4		;					\
	0:			;					\
	lodsb			;					\
	rorl $8, %%eax		;					\
	addl %%ebx, %%esi	;					\
	lodsb			;					\
	rorl $8, %%eax		;					\
	addl %%ebx, %%esi	;					\
	lodsb			;					\
	rorl $8, %%eax		;					\
	addl %%ebx, %%esi	;					\
	lodsb			;					\
	rorl $8, %%eax		;					\
	addl %%ebx, %%esi	;					\
	stosl			;					\
	loop 0b			;					\
	popw %%ax		;					\
	movw %%ax, %%es		;					\
	"								\
	::								\
	"d" (seg),							\
	"c" (width4),							\
	"b" (offset),							\
	"S" (lb),							\
	"D" (address):							\
	"ax", "bx", "cx", "dx", "si", "di", "cc", "memory");		\
}
inline void rotate_double(unsigned char *lb, short seg,				
			  unsigned long address, int offset, int width4)
{									
	__asm__ __volatile__ ("						\
	pushw %%es		;					\
	movw %%dx, %%es		;					\
	.align 4		;					\
	0:			;					\
	lodsb			;					\
	movb %%al, %%ah		;					\
	rorl $16, %%eax		;					\
	addl %%ebx, %%esi	;					\
	lodsb			;					\
	movb %%al, %%ah		;					\
	rorl $16, %%eax		;					\
	stosl			;					\
	addl %%ebx, %%esi	;					\
	lodsb			;					\
	movb %%al, %%ah		;					\
	rorl $16, %%eax		;					\
	addl %%ebx, %%esi	;					\
	lodsb			;					\
	movb %%al, %%ah	;						\
	rorl $16, %%eax		;					\
	addl %%ebx, %%esi	;					\
	stosl			;					\
	loop 0b			;					\
	popw %%ax		;					\
	movw %%ax, %%es		;					\
	"								\
	::								\
	"d" (seg),							\
	"c" (width4),							\
	"b" (offset),							\
	"S" (lb),							\
	"D" (address):							\
	"ax", "bx", "cx", "dx", "si", "di", "cc", "memory");		\
}

inline void update_rotate(void)
{
	short src_seg, dest_seg;
	int y, vesa_line, offset, width4;
	unsigned char *lb;
	unsigned long address;

 	src_seg	= _my_ds();	
	dest_seg = screen->seg;
	vesa_line = vesa_yoffset;
	width4 = bitmap->height/4;

	if (rotation == ROR)
	{
		lb = bitmap->_private + (bitmap->height-1)*bitmap->width+skiplines;
		offset = -bitmap->width-1;
	}
	else 	
	{
		lb = bitmap->_private + bitmap->width -1 -skiplines;
		offset = bitmap->width-1;
	}
	
	for (y = vesa_display_lines;y !=0 ; y--)
	{
		address = bmp_write_line (screen, vesa_line)+vesa_xoffset;
		if (!nodouble)
		{
			rotate_double (lb, dest_seg, address, offset, width4);
			if (noscanlines)
			{
				address = bmp_write_line (screen, vesa_line+1)+vesa_xoffset;
				rotate_double (lb, dest_seg, address, offset, width4);
			}
		vesa_line+=2;
		lb+=rotation;
		}
		else
		{
			rotate (lb, dest_seg, address, offset, width4);
			vesa_line++;
			lb+=rotation;
		}
	}
}

/* Update the display. */
/* As an additional bonus, this function also saves the screen as a PCX file */
/* when the user presses F5. This is not required for porting. */
void osd_update_display(void)
{
	if (video_sync) vsync();

	if (vesa_mode)
	{
		int height;
		if (rotation)
		{
			update_rotate();
			height = bitmap->width;
		}
		else
		{
			update_vesa();
			height = bitmap->height;
		}
		/* Check for PGUP, PGDN and scroll screen */

		if (osd_key_pressed(OSD_KEY_PGDN) &&
			(skiplines+vesa_display_lines < height))
			skiplines++;
		if (osd_key_pressed(OSD_KEY_PGUP) && (skiplines>0))
			skiplines--;
	}
	else 	/* no vesa-modes */
	{
		/* copy the bitmap to screen memory */
		_dosmemputl(bitmap->_private,bitmap->width * bitmap->height / 4,0xa0000);
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

	memcpy(lpWave[channel]->lpData,data,len);
	if (loop) lpWave[channel]->wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO | AUDIO_FORMAT_LOOP;
	else lpWave[channel]->wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO;
	lpWave[channel]->dwLoopStart = 0;
	lpWave[channel]->dwLoopEnd = len;
	lpWave[channel]->dwLength = len;
        Volumi[channel] = volume/4;
	/* upload the data to the audio DRAM local memory */
	AWriteAudioData(lpWave[channel],0,len);
	APlayVoice(hVoice[channel],lpWave[channel]);
	ASetVoiceFrequency(hVoice[channel],freq);
	ASetVoiceVolume(hVoice[channel],MasterVolume*volume/400);
}



void osd_play_streamed_sample(int channel,unsigned char *data,int len,int freq,int volume)
{
	static int playing[NUMVOICES];
	static int c[NUMVOICES];


	if (play_sound == 0 || channel >= NUMVOICES) return;

	/* check if the waveform is large enough for double buffering */
/*	if (2*sizeof(aBuffer) > lpWave->dwLength) {*/

	if (!playing[channel])
	{
		memcpy(lpWave[channel]->lpData,data,len);
		lpWave[channel]->wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO | AUDIO_FORMAT_LOOP;
		lpWave[channel]->dwLoopStart = 0;
		lpWave[channel]->dwLoopEnd = 3*len;
		lpWave[channel]->dwLength = 3*len;
                Volumi[channel] = volume/4;
		/* upload the data to the audio DRAM local memory */
		AWriteAudioData(lpWave[channel],0,len);
		APlayVoice(hVoice[channel],lpWave[channel]);
		ASetVoiceFrequency(hVoice[channel],freq);
		ASetVoiceVolume(hVoice[channel],MasterVolume*volume/400);
		playing[channel] = 1;
		c[channel] = 1;
	}
	else
	{
		DWORD pos;


		for(;;)
		{
			AGetVoicePosition(hVoice[channel],&pos);
			if (c[channel] == 0 && pos > len) break;
			if (c[channel] == 1 && (pos < len || pos > 2*len)) break;
			if (c[channel] == 2 && pos < 2*len) break;
			osd_update_audio();
		}
		memcpy(&lpWave[channel]->lpData[len * c[channel]],data,len);
		AWriteAudioData(lpWave[channel],len*c[channel],len);
		c[channel]++;
		if (c[channel] == 3) c[channel] = 0;
	}
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
  static char *keynames[] = { "ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
                               "MINUS", "EQUAL", "BACKSPACE", "TAB", "Q", "W", "E", "R", "T", "Y",
                               "U", "I", "O", "P", "OPENBRACE", "CLOSEBRACE", "ENTER", "CONTROL",
                               "A", "S", "D", "F", "G", "H", "J", "K", "L", "COLON", "QUOTE",
                               "TILDE", "LEFTSHIFT", "NULL", "Z", "X", "C", "V", "B", "N", "M", "COMMA",
                               "STOP", "SLASH", "RIGHTSHIFT", "ASTERISK", "ALT", "SPACE", "CAPSLOCK",
                               "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "NUMLOCK",
                               "SCRLOCK", "HOME", "UP", "PGUP", "MINUS PAD", "LEFT", "5 PAD", "RIGHT",
                               "PLUS PAD", "END", "DOWN", "PGDN", "INS", "DEL", "F11", "F12" };

  if (keycode && keycode <= OSD_MAX_KEY) return (char*)keynames[keycode-1];
  else return 0;
}


/* return the name of a joystick button */
const char *osd_joy_name(int joycode)
{
  static char *joynames[] = { "LEFT", "RIGHT", "UP", "DOWN", "Button A",
			      "Button B", "Button C", "Button D", "Any Button" };
  static char *nonedefined = "None";

  if (joycode && joycode <= OSD_MAX_JOY) return (char*)joynames[joycode-1];
  else return (char *)nonedefined;
}



int joy_b3, joy_b4;

void osd_poll_joystick(void)
{
        /* NTB. */
        if( use_mouse == 1 )
        {
            osd_poll_mouse();
            return;
        }

	if (use_joystick == 1)
	{
		unsigned char joystatus;


		poll_joystick();
		joystatus = inportb(0x201);
		joy_b3 = ((joystatus & 0x40) == 0);
		joy_b4 = ((joystatus & 0x80) == 0);
	}
}



/* check if the joystick is moved in the specified direction, defined in */
/* osdepend.h. Return 0 if it is not pressed, nonzero otherwise. */
int osd_joy_pressed(int joycode)
{
	/* compiler bug? If I don't declare these variables as volatile, */
	/* joystick right is not detected */
	extern volatile int joy_left, joy_right, joy_up, joy_down;
	extern volatile int joy_b1, joy_b2;


        /* NTB. */
        if( use_mouse == 1 )
        {
            return( osd_mouse_pressed( joycode ));
        }

	if( use_trak == 1) {
	  return( osd_trak_pressed(joycode));
	}

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
			return joy_b1;
			break;
		case OSD_JOY_FIRE2:
			return joy_b2;
			break;
		case OSD_JOY_FIRE3:
			return joy_b3;
			break;
		case OSD_JOY_FIRE4:
			return joy_b4;
			break;
		case OSD_JOY_FIRE:
			return (joy_b1 || joy_b2 || joy_b3 || joy_b4);
			break;
		default:
			return 0;
			break;
	}
}


/* NTB. Read the mouse movement and equate it to a joystick direction. */

void osd_poll_mouse(void)
{
    int mouse_dx, mouse_dy;
    int mouse_tan;

    extern volatile int mouse_x, mouse_y;


    ++mouse_poll_count;

    if( mouse_poll_count == 2 )
	{
        mouse_left = mouse_right = mouse_up = mouse_down = 0;

        mouse_dx = mouse_x - mouse_ox;
        mouse_dy = mouse_y - mouse_oy;

        /* Only register mouse movement over a certain threshold */
        if( ( abs(mouse_dx) > MOUSE_THRESHOLD ) ||
            ( abs(mouse_dy) > MOUSE_THRESHOLD )
        )
        {
            /* Pre-check for division by zero */
            if( mouse_dy == 0 )
            {
                ++mouse_dy;
            }

            /* Calculate the mouse delta tangent using fixed pt. */            
            mouse_tan = (mouse_dx * 256)/mouse_dy;

            /* Approximate the mouse direction to a joystick direction */
            if( mouse_dy <= 0 )
            {
                if( mouse_tan < -TAN_67_5 )
                {
                    mouse_right = 1;
                }
                else if( mouse_tan < -TAN_22_5 )
                {
                    mouse_up = 1;
                    mouse_right = 1;
                }
                else if( mouse_tan < TAN_22_5 )
                {
                    mouse_up = 1;
                }
                else if( mouse_tan < TAN_67_5 )
                {
                    mouse_up = 1;
                    mouse_left = 1;
                }
                else
                {
                    mouse_left = 1;
                }
            }
            else
            {
                if( mouse_tan < -TAN_67_5 )
                {
                    mouse_left = 1;
                }
                else if( mouse_tan < -TAN_22_5 )
                {
                    mouse_down = 1;
                    mouse_left = 1;
                }
                else if( mouse_tan < TAN_22_5 )
                {
                    mouse_down = 1;
                }
                else if( mouse_tan < TAN_67_5 )
                {
                    mouse_down = 1;
                    mouse_right = 1;
                }
                else
                {
                    mouse_right = 1;
                }
            }
        }
        position_mouse( MOUSE_CENTRE_X, MOUSE_CENTRE_Y );
        mouse_ox = MOUSE_CENTRE_X;
        mouse_oy = MOUSE_CENTRE_Y;
        mouse_poll_count = 0;
	}
}

/* NTB. Same as osd_joy_pressed() */

int osd_mouse_pressed(int joycode)
{
	switch (joycode)
	{
		case OSD_JOY_LEFT:
            return mouse_left;
			break;
		case OSD_JOY_RIGHT:
            return mouse_right;
			break;
		case OSD_JOY_UP:
            return mouse_up;
			break;
		case OSD_JOY_DOWN:
            return mouse_down;
			break;
		case OSD_JOY_FIRE1:
            return (mouse_b & 1);
			break;
		case OSD_JOY_FIRE2:
            return (mouse_b & 2);
			break;
		case OSD_JOY_FIRE3:
            return (mouse_b & 4);
			break;
		case OSD_JOY_FIRE:
            return ((mouse_b & 1) || (mouse_b & 2) || (mouse_b & 4));
			break;
		default:
			return 0;
			break;
	}
}

int osd_trak_pressed(int joycode)
{
	switch (joycode)
	{
		case OSD_JOY_LEFT:
            return 0;
			break;
		case OSD_JOY_RIGHT:
            return 0;
			break;
		case OSD_JOY_UP:
            return 0;
			break;
		case OSD_JOY_DOWN:
            return 0;
			break;
		case OSD_JOY_FIRE1:
            return (mouse_b & 1);
			break;
		case OSD_JOY_FIRE2:
            return (mouse_b & 2);
			break;
		case OSD_JOY_FIRE3:
            return (mouse_b & 4);
			break;
		case OSD_JOY_FIRE:
            return ((mouse_b & 1) || (mouse_b & 2) || (mouse_b & 4));
			break;
		default:
			return 0;
			break;
	}
}

int osd_trak_read(int axis) {
  if(!use_trak) {
    return(NO_TRAK);
  }

  if(mouse_x > TRAK_MAX_X) {
    large_trak_x++;
    position_mouse(TRAK_MIN_X+(mouse_x-TRAK_MAX_X),mouse_y);
  }

  if(mouse_x < TRAK_MIN_X) {
    large_trak_x--;
    position_mouse(TRAK_MAX_X-(TRAK_MIN_X-mouse_x),mouse_y);
  }

  if(mouse_y > TRAK_MAX_Y) {
    large_trak_y++;
    position_mouse(mouse_x,TRAK_MIN_Y+(mouse_y-TRAK_MAX_Y));
  }

  if(mouse_y < TRAK_MIN_Y) {
    large_trak_y--;
    position_mouse(mouse_x,TRAK_MAX_Y-(TRAK_MIN_Y-mouse_y));
  }

  switch(axis) {
  case X_AXIS:
    return((large_trak_x*TRAK_MAXX_RES*2/3)+mouse_x-TRAK_CENTER_X);
    break;
  case Y_AXIS:
    return((large_trak_y*TRAK_MAXY_RES*2/3)+mouse_y-TRAK_CENTER_Y);
    break;
  }
  
  return(0);
}

void osd_trak_center_x(void) {
  large_trak_x = 0;
  position_mouse(TRAK_CENTER_X, mouse_y);
}

void osd_trak_center_y(void) {
  large_trak_y = 0;
  position_mouse(mouse_x, TRAK_CENTER_Y);
}


#define MAXPIXELS 100000
char * pixel[MAXPIXELS];
int p_index=-1;

inline void draw_pixel (int x, int y, int col)
{
	char *address;

	if (x<0 || x >= bitmap->width)
		return;
	if (y<0 || y >= bitmap->height)
		return;
	address=&(bitmap->line[y][x]);
	*address=(char)col;
	p_index++;
	pixel[p_index]=address;
}

void open_page (int *x_res, int *y_res, int *portrait, int step)
{
	int i;
	unsigned char bg;
	
	*x_res=bitmap->width;
	*y_res=bitmap->height;
	*portrait=(bitmap->height > bitmap->width) ? 1 : 0;		
	bg=Machine->background_pen;
	for (i=p_index; i>=0; i--)
	{
		*(pixel[i])=bg;
	}
	p_index=-1;
}

void close_page (void)
{
}

void draw_to (int x2, int y2, int col, int z)
{
	static int x1=0;
	static int y1=0;

	int temp_x, temp_y;
	int dx,dy,cx,cy,sx,sy;
	
	if (!z)
	{
		x1=x2;
		y1=y2;
		return;
	}

	temp_x = x2; temp_y = y2;
	col += (z << 3);
	col &= 0x7f;
	
#ifdef 0
	if (x1<0 || y1<0 || x2<0 || y2<0 ||
		x1>=bitmap->width || y1>=bitmap->height ||
		x2>=bitmap->width || y2>=bitmap->height)
	{
		if (errorlog)
			fprintf(errorlog,
			"line:%d,%d nach %d,%d color %d, intensity %d\n",
			 x1,y1,x2,y2,col,z);
		return;
	}
#endif
	dx=abs(x1-x2);
	dy=abs(y1-y2);

	if ((dx>=dy && x1>x2) || (dy>dx && y1>y2))
	{
		int t;
		t = x1; x1 = x2; x2 = t;
		t = y1; y1 = y2; y2 = t;
	}
	sx = ((x1 <= x2) ? 1 : -1);
	sy = ((y1 <= y2) ? 1 : -1);
	cx=dx/2;
	cy=dy/2;

	if (dx == dy)
	{
		while (x1 <= x2)
		{
			draw_pixel(x1,y1,col);
			x1+=sx;
			y1+=sy;
		}
	}
	else if (dx>dy)
	{
		while (x1 <= x2)
		{
			draw_pixel(x1,y1,col);
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
		while (y1 <= y2)
		{
			draw_pixel(x1,y1,col);
			y1+=sy;
			cy-=dx;
			if (cy < 0)
			{
				x1+=sx;
				cy+=dy;
			}
		}
	}	
	x1 = temp_x;
	y1 = temp_y;
}
