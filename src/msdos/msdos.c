/***************************************************************************

  osdepend.c

  OS dependant stuff (display handling, keyboard scan...)
  This is the only file which should me modified in order to port the
  emulator to a different system.

***************************************************************************/

#include <pc.h>
#include <sys/farptr.h>
#include <stdio.h>
#include <string.h>
#include <go32.h>
#include <allegro.h>
#include "osdepend.h"
#include "TwkUser.c"
#include <audio.h>


#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define SCREEN_MODE GFX_VESA1


struct osd_bitmap *bitmap;
int first_free_pen;
int use_vesa;
int play_sound;
int noscanlines;
int use_joystick;


int osd_joy_up, osd_joy_down, osd_joy_left, osd_joy_right;
int osd_joy_b1, osd_joy_b2, osd_joy_b3, osd_joy_b4;

/* audio related stuff */
#define NUMVOICES 4
#define SAMPLE_RATE 44100
#define SAMPLE_BUFFER_LENGTH 50000
HAC hVoice[NUMVOICES];
LPAUDIOWAVE lpWave[NUMVOICES];


/* put here anything you need to do when the program is started. Return 0 if */
/* initialization was successful, nonzero otherwise. */
int osd_init(int argc,char **argv)
{
	int i;


	use_vesa = 0;
	play_sound = 1;
	noscanlines = 0;
	use_joystick = 1;
	for (i = 1;i < argc;i++)
	{
		if (stricmp(argv[i],"-vesa") == 0)
			use_vesa = 1;
		if (stricmp(argv[i],"-nosound") == 0)
			play_sound = 0;
		if (stricmp(argv[i],"-noscanlines") == 0)
			noscanlines = 1;
		if (stricmp(argv[i],"-nojoy") == 0)
			use_joystick = 0;
	}

	if (use_joystick)
	{
		/* Try to install Allegro joystick handler */
		if (initialise_joystick())
			use_joystick = 0; /* joystick not found */
	}

	if (play_sound)
	{
	    AUDIOINFO info;
	    AUDIOCAPS caps;


		/* initialize SEAL audio library */
		AInitialize();

		printf("\nSelect the audio device:\n(if you have an AWE 32, choose Sound Blaster for a more faithful emulation)\n");
		for (i = 0;i < AGetAudioNumDevs();i++)
		{
			AGetAudioDevCaps(i, &caps);
			printf("  %2d. %s\n",i,caps.szProductName);
		}
		printf("\n");

		scanf("%d",&i);

		/* open audio device */
//		info.nDeviceId = AUDIO_DEVICE_MAPPER;
		info.nDeviceId = i;
		info.wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO;
		info.nSampleRate = SAMPLE_RATE;
		AOpenAudio(&info);

		/* open and allocate voices, allocate waveforms */
		AOpenVoices(NUMVOICES);
		for (i = 0; i < NUMVOICES; i++)
		{
			ACreateAudioVoice(&hVoice[i]);
			ASetVoicePanning(hVoice[i],128);

			if ((lpWave[i] = (LPAUDIOWAVE)malloc(sizeof(AUDIOWAVE))) == 0)
// have to handle this better...
				return 1;

			lpWave[i]->wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO;
			lpWave[i]->nSampleRate = SAMPLE_RATE;
			lpWave[i]->dwLength = SAMPLE_BUFFER_LENGTH;
			lpWave[i]->dwLoopStart = lpWave[i]->dwLoopEnd = 0;
			if (ACreateAudioData(lpWave[i]) != AUDIO_ERROR_NONE)
			{
// have to handle this better...
				free(lpWave[i]);

				return 1;
			}
		}
	}

	allegro_init();
	install_keyboard();		/* Allegro keyboard handler */
	first_free_pen = 0;

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
	}
}



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
	{ 0x3d4, 0x02, 0x40},{ 0x3d4, 0x03, 0x82},{ 0x3d4, 0x04, 0x4A},
	{ 0x3d4, 0x05, 0x9A},{ 0x3d4, 0x06, 0x23},{ 0x3d4, 0x07, 0x1f},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x60},{ 0x3d4, 0x10, 0x0a},
	{ 0x3d4, 0x11, 0xac},{ 0x3d4, 0x12, 0x00},{ 0x3d4, 0x13, 0x20},
	{ 0x3d4, 0x14, 0x40},{ 0x3d4, 0x15, 0x07},{ 0x3d4, 0x16, 0x1a},
	{ 0x3d4, 0x17, 0xa3},{ 0x3c4, 0x01, 0x01},{ 0x3c4, 0x04, 0x0e},
	{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41},
	{ 0x3c0, 0x13, 0x00}
};


/* Create a display screen, or window, large enough to accomodate a bitmap */
/* of the given dimensions. I don't do any test here (224x288 will just do */
/* for now) but one could e.g. open a window of the exact dimensions */
/* provided. Return a osd_bitmap pointer or 0 in case of error. */
struct osd_bitmap *osd_create_display(int width,int height)
{
	if (!(width == 224 && height == 288) &&
			!(width == 256 && height == 256))
		use_vesa = 1;

	if (use_vesa)
	{
		if (set_gfx_mode(SCREEN_MODE,SCREEN_WIDTH,SCREEN_HEIGHT,0,0) != 0)
			return 0;
	}
	else
	{
		/* big hack: open a mode 13h screen using Allegro, then load the custom screen */
		/* definition over it. */
		if (set_gfx_mode(GFX_VGA,320,200,0,0) != 0)
			return 0;

		if (width == 224 && height == 288)
			outRegArray(scr224x288,sizeof(scr224x288)/sizeof(Register));
		else if (width == 256 && height == 256)
		{
			if (noscanlines)
				outRegArray(scr256x256,sizeof(scr256x256)/sizeof(Register));
			else
				outRegArray(scr256x256scanlines,sizeof(scr256x256)/sizeof(Register));
		}
	}

	bitmap = osd_create_bitmap(width,height);

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



/* Update the display. */
/* As an additional bonus, this function also saves the screen as a PCX file */
/* when the user presses F5. This is not required for porting. */
void osd_update_display(void)
{
	if (use_vesa)
	{
		int y;
		int width4 = bitmap->width / 4;
		unsigned long *lb = (unsigned long *)bitmap->private;


		for (y = 0;y < bitmap->height;y++)
		{
			unsigned long address;


			address = bmp_write_line(screen,y + (SCREEN_HEIGHT - bitmap->height) / 2)
					+ (SCREEN_WIDTH - bitmap->width) / 2;
			_dosmemputl(lb,width4,address);
			lb += width4;
		}
	}
	else
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
	/* upload the data to the audio DRAM local memory */
	AWriteAudioData(lpWave[channel],0,len);
	APlayVoice(hVoice[channel],lpWave[channel]);
	ASetVoiceFrequency(hVoice[channel],freq);
	ASetVoiceVolume(hVoice[channel],volume/4);
}



void osd_play_streamed_sample(int channel,unsigned char *data,int len,int freq,int volume)
{
	static int playing = 0;
	static int c;


	if (play_sound == 0 || channel >= NUMVOICES) return;

	/* check if the waveform is large enough for double buffering */
//	if (2*sizeof(aBuffer) > lpWave->dwLength) {

	if (!playing)
	{
		memcpy(lpWave[channel]->lpData,data,len);
		lpWave[channel]->wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO | AUDIO_FORMAT_LOOP;
		lpWave[channel]->dwLoopStart = 0;
		lpWave[channel]->dwLoopEnd = 3*len;
		lpWave[channel]->dwLength = 3*len;
		/* upload the data to the audio DRAM local memory */
		AWriteAudioData(lpWave[channel],0,len);
		APlayVoice(hVoice[channel],lpWave[channel]);
		ASetVoiceFrequency(hVoice[channel],freq);
		ASetVoiceVolume(hVoice[channel],volume/4);
		playing = 1;
		c = 1;
	}
	else
	{
		DWORD pos;


		for(;;)
		{
			AGetVoicePosition(hVoice[channel],&pos);
			if (c == 0 && pos > len) break;
			if (c == 1 && (pos < len || pos > 2*len)) break;
			if (c == 2 && pos < 2*len) break;
			osd_update_audio();
		}
		memcpy(&lpWave[channel]->lpData[len * c],data,len);
		AWriteAudioData(lpWave[channel],len*c,len);
		c++;
		if (c == 3) c = 0;
	}
}



void osd_adjust_sample(int channel,int freq,int volume)
{
	if (play_sound == 0 || channel >= NUMVOICES) return;

	ASetVoiceFrequency(hVoice[channel],freq);
	ASetVoiceVolume(hVoice[channel],volume/4);
}



void osd_stop_sample(int channel)
{
	if (play_sound == 0 || channel >= NUMVOICES) return;

	AStopVoice(hVoice[channel]);
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
	clear_keybuf();
	return readkey() >> 8;
}



void osd_poll_joystick(void)
{
	if (use_joystick == 1)
	{
		unsigned char joystatus;


		poll_joystick();
		joystatus = inportb(0x201);
		osd_joy_up = joy_up;
		osd_joy_down = joy_down;
		osd_joy_left = joy_left;
		osd_joy_right = joy_right;
		osd_joy_b1 = joy_b1;
		osd_joy_b2 = joy_b2;
		osd_joy_b3 = ((joystatus & 0x40) == 0);
		osd_joy_b4 = ((joystatus & 0x80) == 0);
	}
}
