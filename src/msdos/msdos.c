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


struct osd_bitmap *bitmap;
int first_free_pen;

int videofreq;
int video_sync;
int play_sound;
int use_joystick;

int use_vesa_scan;
int use_rotate;
int vesa_mode;
int vesa_width;
int vesa_height;
int vesa_skiplines;
int vesa_xoffset;
int vesa_yoffset;
int vesa_display_lines;

int noscanlines;

struct { char *desc; int x, y; } gfx_res[] = {
	{ "-320x240"	, 320, 240 },
	{ "-320"	, 320, 240 },
	{ "-512x384"	, 512, 384 },
	{ "-512"	, 512, 384 },
	{ "-640x480"	, 640, 480 },
	{ "-640"	, 640, 480 },
	{ "-800x600"	, 800, 600 },
	{ "-800"	, 800, 600 },
	{ "-1024x768"	, 1024, 768 },
	{ "-1024"	, 1024, 768 },
	{ NULL		, 0, 0 }
	};




/* audio related stuff */
#define NUMVOICES 8
#define SAMPLE_RATE 44100
#define SAMPLE_BUFFER_LENGTH 50000
HAC hVoice[NUMVOICES];
LPAUDIOWAVE lpWave[NUMVOICES];
int Volumi[NUMVOICES];
int MasterVolume = 100;
unsigned char No_FM = 0;


/* put here anything you need to do when the program is started. Return 0 if */
/* initialization was successful, nonzero otherwise. */
int osd_init(int argc,char **argv)
{
	int i,j;
	int soundcard;


	first_free_pen = 0;

	vesa_mode = 0;
	vesa_width = 0;
	vesa_height = 0;
	noscanlines = 0;
	use_rotate = 0;
	videofreq = 0;
	video_sync = 0;
	play_sound = 1;
	use_joystick = 1;
	soundcard = -1;
	for (i = 1;i < argc;i++)
	{

		if ((stricmp(argv[i],"-vesa") == 0) ||
		    (stricmp(argv[i],"-vesascan") == 0))
			vesa_mode = GFX_VESA1;
		if (stricmp(argv[i],"-vesa2b") == 0)
			vesa_mode = GFX_VESA2B;
		if (stricmp(argv[i],"-vesa2l") == 0)
			vesa_mode = GFX_VESA2L;
		if (stricmp(argv[i],"-vesaskip") == 0)
		{
			i++;
			if (i < argc) vesa_skiplines = atoi(argv[i]);
			if (!vesa_width)
			{
				vesa_width=640;
				vesa_height=480;
			}
		}
                for (j=0; gfx_res[j].desc != NULL; j++)
		{
			if (stricmp(argv[i], gfx_res[j].desc) == 0)
			{
				vesa_width=gfx_res[j].x;
				vesa_height=gfx_res[j].y;
				break;
			}
		}
		if (stricmp(argv[i],"-rotate") == 0)
		{
			use_rotate=1;
			vesa_width = 320;
			vesa_height = 240;
		}
		if (stricmp(argv[i],"-soundcard") == 0)
		{
			i++;
			if (i < argc) soundcard = atoi(argv[i]);
		}
		if (stricmp(argv[i],"-noscanlines") == 0)
			noscanlines = 1;
		if (stricmp(argv[i],"-vgafreq") == 0)
		{
			i++;
			if (i < argc) videofreq = atoi(argv[i]);
			if (videofreq < 0) videofreq = 0;
			if (videofreq > 3) videofreq = 3;
		}
		if (stricmp(argv[i],"-vsync") == 0)
			video_sync = 1;
		if (stricmp(argv[i],"-nojoy") == 0)
			use_joystick = 0;
                if (stricmp(argv[i],"-nofm") == 0)
                        No_FM = 1;
	}

	if (play_sound)
	{
	    AUDIOINFO info;
	    AUDIOCAPS caps;


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

	return 0;
}



/* put here cleanup routines to be executed when the program is terminated. */
void osd_exit(void)
{
	if (play_sound)
	{
		int n;


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

		bitmap->private = bm;

		clearbitmap(bitmap);
	}

	return bitmap;
}



void osd_free_bitmap(struct osd_bitmap *bitmap)
{
	if (bitmap)
	{
		free(bitmap->private);
		free(bitmap);
	}
}



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
	{ 0x3c2, 0x0, 0xe3},{ 0x3d4, 0x0, 0x5f},{ 0x3d4, 0x1, 0x47},
	{ 0x3d4, 0x2, 0x50},{ 0x3d4, 0x3, 0x82},{ 0x3d4, 0x4, 0x50},
	{ 0x3d4, 0x5, 0x80},{ 0x3d4, 0x6, 0xb},{ 0x3d4, 0x7, 0x3e},
	{ 0x3d4, 0x8, 0x0},{ 0x3d4, 0x9, 0x41},{ 0x3d4, 0x10, 0xda},
	{ 0x3d4, 0x11, 0x9c},{ 0x3d4, 0x12, 0xbf},{ 0x3d4, 0x13, 0x24},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0xc7},{ 0x3d4, 0x16, 0x4},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x1, 0x1},{ 0x3c4, 0x4, 0xe},
	{ 0x3ce, 0x5, 0x40},{ 0x3ce, 0x6, 0x5},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x0}
};

Register scr288x224scanlines[] =
{
	{ 0x3c2, 0x0, 0xe3},{ 0x3d4, 0x0, 0x5f},{ 0x3d4, 0x1, 0x47},
	{ 0x3d4, 0x2, 0x47},{ 0x3d4, 0x3, 0x82},{ 0x3d4, 0x4, 0x50},
	{ 0x3d4, 0x5, 0x9a},{ 0x3d4, 0x6, 0xb},{ 0x3d4, 0x7, 0x19},
	{ 0x3d4, 0x8, 0x0},{ 0x3d4, 0x9, 0x40},{ 0x3d4, 0x10, 0xf5},
	{ 0x3d4, 0x11, 0xac},{ 0x3d4, 0x12, 0xdf},{ 0x3d4, 0x13, 0x24},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0xc7},{ 0x3d4, 0x16, 0x4},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x1, 0x1},{ 0x3c4, 0x4, 0xe},
	{ 0x3ce, 0x5, 0x40},{ 0x3ce, 0x6, 0x5},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x0}
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
        { 0x3c2, 0x0, 0xe3}, { 0x3d4, 0x0, 0x5f}, { 0x3d4, 0x1, 0x3b},
        { 0x3d4, 0x2, 0x38}, { 0x3d4, 0x3, 0x82}, { 0x3d4, 0x4, 0x4a},
        { 0x3d4, 0x5, 0x9a}, { 0x3d4, 0x6, 0x55}, { 0x3d4, 0x7, 0xf0},
        { 0x3d4, 0x8, 0x0},  { 0x3d4, 0x9, 0x61}, { 0x3d4, 0x10, 0x40},
        { 0x3d4, 0x11, 0xac},{ 0x3d4, 0x12, 0x20},{ 0x3d4, 0x13, 0x1e},
        { 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0x40},{ 0x3d4, 0x16, 0x4a},
        { 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x1, 0x1},  { 0x3c4, 0x4, 0xe},
        { 0x3ce, 0x5, 0x40}, { 0x3ce, 0x6, 0x5},  { 0x3c0, 0x10, 0x41},
        { 0x3c0, 0x13, 0x0}
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

        if (use_rotate)
	{ int t; t = width; width = height; height = t; }

	if (!(width == 224 && height == 288) &&
            !(width == 256 && height == 256) &&
	    !(width == 288 && height == 224) &&
	    !(width == 320 && height == 204) &&
            !(width == 240 && height == 272))
 		vesa_mode = GFX_VESA1;

 	if (vesa_mode || vesa_width)
  	{
 		if (!vesa_mode) vesa_mode = GFX_VESA1;
 		if (!vesa_width)
  		{
 			vesa_width=800;
 			vesa_height=600;
  		}

 		if (set_gfx_mode(vesa_mode,vesa_width,vesa_height,0,0) != 0)
  		{
			printf ("%dx%d not possible\n", vesa_width,vesa_height);
			printf ("%s\n",allegro_error);
			return 0;
 		}	
 		if (width > vesa_width/2)
 		{
 			noscanlines = 1;
                        use_vesa_scan = 0;
 			vesa_yoffset = (vesa_height - height) / 2;
 			vesa_xoffset = (vesa_width - width) / 2;
 			vesa_display_lines = height - vesa_skiplines;
 			if (vesa_height < vesa_display_lines)
 				vesa_display_lines = vesa_height;
 		}
 		else
 		{
/*/			noscanlines = 0;*/
                        use_vesa_scan = 1;
 			vesa_yoffset = (vesa_height - height * 2) / 2;
 			vesa_xoffset = (vesa_width - width * 2) / 2;
 			vesa_display_lines = height - vesa_skiplines;
 			if (vesa_height/2 < vesa_display_lines)
 				vesa_display_lines = vesa_height / 2;
 		}
 		/* Allign on a quadword !*/
 		vesa_xoffset -= vesa_xoffset % 4;
 		/* Just in case */
 		if (vesa_yoffset < 0) vesa_yoffset = 0;
 		if (vesa_xoffset < 0) vesa_xoffset = 0;
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
			        reg = scr224x288scanlines;
				reglen = sizeof(scr224x288scanlines)/sizeof(Register);
			}
		}
		else if (width == 256 && height == 256)
		{
			if (noscanlines)
			{
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
		reg[0].value = (reg[0].value & 0xf3) | (videofreq << 2);

		outRegArray(reg,reglen);
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
	lb = bitmap->private + bitmap->width*vesa_skiplines;

	for (y = vesa_display_lines;y !=0 ; y--)
	{
		address = bmp_write_line (screen, vesa_line) + vesa_xoffset;
		if (use_vesa_scan)
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

inline void rotate_pixels(unsigned char *lb, short seg,
			  unsigned long address, int height, int width4)
{
	__asm__ __volatile__ ("
	pushw %%es		\n
	movw %%dx, %%es		\n
	.align 4		\n
	0:			\n
	lodsb			\n
	rorl $8, %%eax		\n
	subl %%ebx, %%esi	\n
	lodsb			\n
	rorl $8, %%eax		\n
	subl %%ebx, %%esi	\n
	lodsb			\n
	rorl $8, %%eax		\n
	subl %%ebx, %%esi	\n
	lodsb			\n
	rorl $8, %%eax		\n
	subl %%ebx, %%esi	\n
	stosl			\n
	loop 0b			\n
	popw %%ax		\n
	movw %%ax, %%es		\n
	"
	::
	"d" (seg),
	"c" (width4),
	"b" (height+1),
	"S" (lb),
	"D" (address):
	"ax", "bx", "cx", "dx", "si", "di", "cc", "memory");	
}

inline void update_rotate(void)
{
	short src_seg, dest_seg;
	int y, vesa_line, width4;
	unsigned char *lb;
        unsigned long address;

 	src_seg	= _my_ds();	
	dest_seg = screen->seg;
	vesa_line = vesa_yoffset;
	width4 = bitmap->height/4;

	lb = bitmap->private + (bitmap->height-1)*bitmap->width
             +vesa_skiplines;
	
	for (y = vesa_display_lines;y !=0 ; y--)
	{
		address = bmp_write_line (screen, vesa_line) +vesa_xoffset;
		rotate_pixels (lb, dest_seg, address, bitmap->width, width4);
		vesa_line++;
		lb++;
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
		if (use_rotate)
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
			(vesa_skiplines+vesa_display_lines < height))
			vesa_skiplines++;
		if (osd_key_pressed(OSD_KEY_PGUP) && (vesa_skiplines>0))
			vesa_skiplines--;
	}
	else 	/* no vesa-modes */
	{
		/* copy the bitmap to screen memory */
		_dosmemputl(bitmap->private,bitmap->width * bitmap->height / 4,0xa0000);
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



int joy_b3, joy_b4;

void osd_poll_joystick(void)
{
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
