/***************************************************************************

  win32.c

  OS dependant stuff (display handling, keyboard scan...)
  This is the only file which should me modified in order to port the
  emulator to a different system.

  Created 6/29/97 by Christopher Kirmse (ckirmse@ricochet.net)

  Although MAME isn't ideally suited to the event driven windows model,
  we can poll for events on a regular basis (because the main loop calls
  us to check for keys all the time).  This makes the Windows version
  run quite well, and reduces many hardware compatibility problems!

***************************************************************************/

#include "win32.h"
#include "audio.h"

#include "dirent.h"

void osd_win32_sound_init();
void osd_win32_joy_init();

long CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);


int Win32KeyToOSDKey(UINT vk);
void dprintf(char *fmt,...);

void osd_win32_create_palette();

void osd_win32_mouse_moved(int x,int y);

int first_free_pen; 

struct osd_bitmap *bitmap;

int win32_debug = 1;

BOOL is_active = TRUE; /* whether we're currently the topmost app. */

/* joystick related stuff */      

/* percent of min-max range that's considered a full left or right or up or down */
#define JOY_RANGE 20    

int use_joystick = TRUE;
DWORD joy1_left_max;
DWORD joy1_right_min;
DWORD joy1_top_max;
DWORD joy1_bottom_min;


/* display related stuff */

byte *buffer_ptr;
int video_sync;

int desired_width,desired_height;

int palette_offset; /* to work in window mode, must be 10 (because of 10 system colors) */

int vector_game;
int use_video_memory;
int is_window;
int scale;
int flip;
int rotate;
struct osd_bitmap *bitmap;

/* This is a good one here--pitch_space.  We normally would allocate an array
   of size width*height.  However, take the case of Q*bert, Zaxxon, and some other
   games.  They draw into a screen 256x256.  If you look at the rotate code,
   you'll see that consecutive reads are done at the same column on each row.
   With a row width of 256, this will use the same cache line each time, which
   is like having no cache!  By just padding each row, performance is MUCH
   better for rotation.
   */
int pitch_space;

BOOL ending;

char szAppName[] = "MAME";

HWND hMain;

/* direct draw drawing stuff */
LPDIRECTDRAWSURFACE ddframe;


BOOLEAN palette_created;
CKPALETTE pal;

/* audio related stuff */
int play_sound = 1;

#define NUMVOICES 8
#define SAMPLE_RATE 44100
#define SAMPLE_BUFFER_LENGTH 100000
HAC hVoice[NUMVOICES];
LPAUDIOWAVE lpWave[NUMVOICES];
int Volumi[NUMVOICES];
int MasterVolume = 100;
unsigned char No_FM = 0;
unsigned char RegistersYM[264*5];  /* MAX 5 YM-2203 */

/*
 *   Trackball Related stuff
 * not supported yet, but working on it!
 */

int mouse_x,mouse_y;
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

/* vector game support */
BOOL is_vector = FALSE;
HDC hdc_vector = NULL;
/* keyboard related stuff */

#ifndef OSD_KEY_NONE
#define OSD_KEY_NONE	0
#endif
#define OSD_NUMKEYS		(100)


static int key_code_table[] = {

/*                   0x00 */ OSD_KEY_NONE,
/* VK_LBUTTON        0x01 */ OSD_KEY_NONE,
/* VK_RBUTTON        0x02 */ OSD_KEY_NONE,
/* VK_CANCEL         0x03 */ OSD_KEY_NONE,
/* VK_MBUTTON        0x04 */ OSD_KEY_NONE,
/*                   0x05 */ OSD_KEY_NONE,
/*                   0x06 */ OSD_KEY_NONE,
/*                   0x07 */ OSD_KEY_NONE,
/* VK_BACK           0x08 */ OSD_KEY_BACKSPACE,
/* VK_TAB            0x09 */ OSD_KEY_TAB,
/*                   0x0A */ OSD_KEY_NONE,
/*                   0x0B */ OSD_KEY_NONE,
/* VK_CLEAR          0x0C */ OSD_KEY_5_PAD,
/* VK_RETURN         0x0D */ OSD_KEY_ENTER,
/*                   0x0E */ OSD_KEY_NONE,
/*                   0x0F */ OSD_KEY_NONE,
/* VK_SHIFT          0x10 */ OSD_KEY_LSHIFT,
/* VK_CONTROL        0x11 */ OSD_KEY_CONTROL,
/* VK_MENU           0x12 */ OSD_KEY_ALT,
/* VK_PAUSE          0x13 */ OSD_KEY_NONE,
/* VK_CAPITAL        0x14 */ OSD_KEY_CAPSLOCK,
/*                   0x15 */ OSD_KEY_NONE,
/*                   0x16 */ OSD_KEY_NONE,
/*                   0x17 */ OSD_KEY_NONE,
/*                   0x18 */ OSD_KEY_NONE,
/*                   0x19 */ OSD_KEY_NONE,
/*                   0x1A */ OSD_KEY_NONE,
/* VK_ESCAPE         0x1B */ OSD_KEY_ESC,
/*                   0x1C */ OSD_KEY_NONE,
/*                   0x1D */ OSD_KEY_NONE,
/*                   0x1E */ OSD_KEY_NONE,
/*                   0x1F */ OSD_KEY_NONE,
/* VK_SPACE          0x20 */ OSD_KEY_SPACE,
/* VK_PRIOR          0x21 */ OSD_KEY_PGUP,
/* VK_NEXT           0x22 */ OSD_KEY_PGDN,
/* VK_END            0x23 */ OSD_KEY_END,
/* VK_HOME           0x24 */ OSD_KEY_HOME,
/* VK_LEFT           0x25 */ OSD_KEY_LEFT,
/* VK_UP             0x26 */ OSD_KEY_UP,
/* VK_RIGHT          0x27 */ OSD_KEY_RIGHT,
/* VK_DOWN           0x28 */ OSD_KEY_DOWN,
/* VK_SELECT         0x29 */ OSD_KEY_NONE,
/* VK_PRINT          0x2A */ OSD_KEY_NONE,
/* VK_EXECUTE        0x2B */ OSD_KEY_NONE,
/* VK_SNAPSHOT       0x2C */ OSD_KEY_NONE,
/* VK_INSERT         0x2D */ OSD_KEY_INSERT,
/* VK_DELETE         0x2E */ OSD_KEY_DEL,
/* VK_HELP           0x2F */ OSD_KEY_NONE,
/*                   '0'  */ OSD_KEY_0,
/*                   '1'  */ OSD_KEY_1,
/*                   '2'  */ OSD_KEY_2,
/*                   '3'  */ OSD_KEY_3,
/*                   '4'  */ OSD_KEY_4,
/*                   '5'  */ OSD_KEY_5,
/*                   '6'  */ OSD_KEY_6,
/*                   '7'  */ OSD_KEY_7,
/*                   '8'  */ OSD_KEY_8,
/*                   '9'  */ OSD_KEY_9,
/*                   0x3A */ OSD_KEY_NONE,
/*                   0x3B */ OSD_KEY_NONE,
/*                   0x3C */ OSD_KEY_NONE,
/*                   0x3D */ OSD_KEY_NONE,
/*                   0x3E */ OSD_KEY_NONE,
/*                   0x3F */ OSD_KEY_NONE,
/*                   0x40 */ OSD_KEY_NONE,
/*                   'A'  */ OSD_KEY_A,
/*                   'B'  */ OSD_KEY_B,
/*                   'C'  */ OSD_KEY_C,
/*                   'D'  */ OSD_KEY_D,
/*                   'E'  */ OSD_KEY_E,
/*                   'F'  */ OSD_KEY_F,
/*                   'G'  */ OSD_KEY_G,
/*                   'H'  */ OSD_KEY_H,
/*                   'I'  */ OSD_KEY_I,
/*                   'J'  */ OSD_KEY_J,
/*                   'K'  */ OSD_KEY_K,
/*                   'L'  */ OSD_KEY_L,
/*                   'M'  */ OSD_KEY_M,
/*                   'N'  */ OSD_KEY_N,
/*                   'O'  */ OSD_KEY_O,
/*                   'P'  */ OSD_KEY_P,
/*                   'Q'  */ OSD_KEY_Q,
/*                   'R'  */ OSD_KEY_R,
/*                   'S'  */ OSD_KEY_S,
/*                   'T'  */ OSD_KEY_T,
/*                   'U'  */ OSD_KEY_U,
/*                   'V'  */ OSD_KEY_V,
/*                   'W'  */ OSD_KEY_W,
/*                   'X'  */ OSD_KEY_X,
/*                   'Y'  */ OSD_KEY_Y,
/*                   'Z'  */ OSD_KEY_Z,
/* VK_LWIN           0x5B */ OSD_KEY_NONE,
/* VK_RWIN           0x5C */ OSD_KEY_NONE,
/* VK_APPS           0x5D */ OSD_KEY_NONE,
/*                   0x5E */ OSD_KEY_NONE,
/*                   0x5F */ OSD_KEY_NONE,
/* VK_NUMPAD0        0x60 */ OSD_KEY_NONE,
/* VK_NUMPAD1        0x61 */ OSD_KEY_NONE,
/* VK_NUMPAD2        0x62 */ OSD_KEY_NONE,
/* VK_NUMPAD3        0x63 */ OSD_KEY_NONE,
/* VK_NUMPAD4        0x64 */ OSD_KEY_NONE,
/* VK_NUMPAD5        0x65 */ OSD_KEY_NONE,
/* VK_NUMPAD6        0x66 */ OSD_KEY_NONE,
/* VK_NUMPAD7        0x67 */ OSD_KEY_NONE,
/* VK_NUMPAD8        0x68 */ OSD_KEY_NONE,
/* VK_NUMPAD9        0x69 */ OSD_KEY_NONE,
/* VK_MULTIPLY       0x6A */ OSD_KEY_NONE,
/* VK_ADD            0x6B */ OSD_KEY_NONE,
/* VK_SEPARATOR      0x6C */ OSD_KEY_NONE,
/* VK_SUBTRACT       0x6D */ OSD_KEY_NONE,
/* VK_DECIMAL        0x6E */ OSD_KEY_NONE,
/* VK_DIVIDE         0x6F */ OSD_KEY_NONE,
/* VK_F1             0x70 */ OSD_KEY_F1,
/* VK_F2             0x71 */ OSD_KEY_F2,
/* VK_F3             0x72 */ OSD_KEY_F3,
/* VK_F4             0x73 */ OSD_KEY_F4,
/* VK_F5             0x74 */ OSD_KEY_F5,
/* VK_F6             0x75 */ OSD_KEY_F6,
/* VK_F7             0x76 */ OSD_KEY_F7,
/* VK_F8             0x77 */ OSD_KEY_F8,
/* VK_F9             0x78 */ OSD_KEY_F9,
/* VK_F10            0x79 */ OSD_KEY_F10,
/* VK_F11            0x7A */ OSD_KEY_F11,
/* VK_F12            0x7B */ OSD_KEY_F12,
/* VK_F13            0x7C */ OSD_KEY_NONE,
/* VK_F14            0x7D */ OSD_KEY_NONE,
/* VK_F15            0x7E */ OSD_KEY_NONE,
/* VK_F16            0x7F */ OSD_KEY_NONE,
/* VK_F17            0x80 */ OSD_KEY_NONE,
/* VK_F18            0x81 */ OSD_KEY_NONE,
/* VK_F19            0x82 */ OSD_KEY_NONE,
/* VK_F20            0x83 */ OSD_KEY_NONE,
/* VK_F21            0x84 */ OSD_KEY_NONE,
/* VK_F22            0x85 */ OSD_KEY_NONE,
/* VK_F23            0x86 */ OSD_KEY_NONE,
/* VK_F24            0x87 */ OSD_KEY_NONE,
/*                   0x88 */ OSD_KEY_NONE,
/*                   0x89 */ OSD_KEY_NONE,
/*                   0x8A */ OSD_KEY_NONE,
/*                   0x8B */ OSD_KEY_NONE,
/*                   0x8C */ OSD_KEY_NONE,
/*                   0x8D */ OSD_KEY_NONE,
/*                   0x8E */ OSD_KEY_NONE,
/*                   0x8F */ OSD_KEY_NONE,
/* VK_NUMLOCK        0x90 */ OSD_KEY_NUMLOCK,
/* VK_SCROLL         0x91 */ OSD_KEY_SCRLOCK,
/*                   0x92 */ OSD_KEY_NONE,
/*                   0x93 */ OSD_KEY_NONE,
/*                   0x94 */ OSD_KEY_NONE,
/*                   0x95 */ OSD_KEY_NONE,
/*                   0x96 */ OSD_KEY_NONE,
/*                   0x97 */ OSD_KEY_NONE,
/*                   0x98 */ OSD_KEY_NONE,
/*                   0x99 */ OSD_KEY_NONE,
/*                   0x9A */ OSD_KEY_NONE,
/*                   0x9B */ OSD_KEY_NONE,
/*                   0x9C */ OSD_KEY_NONE,
/*                   0x9D */ OSD_KEY_NONE,
/*                   0x9E */ OSD_KEY_NONE,
/*                   0x9F */ OSD_KEY_NONE,
/*                   0xA0 */ OSD_KEY_NONE,
/*                   0xA1 */ OSD_KEY_NONE,
/*                   0xA2 */ OSD_KEY_NONE,
/*                   0xA3 */ OSD_KEY_NONE,
/*                   0xA4 */ OSD_KEY_NONE,
/*                   0xA5 */ OSD_KEY_NONE,
/*                   0xA6 */ OSD_KEY_NONE,
/*                   0xA7 */ OSD_KEY_NONE,
/*                   0xA8 */ OSD_KEY_NONE,
/*                   0xA9 */ OSD_KEY_NONE,
/*                   0xAA */ OSD_KEY_NONE,
/*                   0xAB */ OSD_KEY_NONE,
/*                   0xAC */ OSD_KEY_NONE,
/*                   0xAD */ OSD_KEY_NONE,
/*                   0xAE */ OSD_KEY_NONE,
/*                   0xAF */ OSD_KEY_NONE,
/*                   0xB0 */ OSD_KEY_NONE,
/*                   0xB1 */ OSD_KEY_NONE,
/*                   0xB2 */ OSD_KEY_NONE,
/*                   0xB3 */ OSD_KEY_NONE,
/*                   0xB4 */ OSD_KEY_NONE,
/*                   0xB5 */ OSD_KEY_NONE,
/*                   0xB6 */ OSD_KEY_NONE,
/*                   0xB7 */ OSD_KEY_NONE,
/*                   0xB8 */ OSD_KEY_NONE,
/*                   0xB9 */ OSD_KEY_NONE,
/*                   0xBA */ OSD_KEY_COLON,
/*                   0xBB */ OSD_KEY_EQUALS,
/*                   0xBC */ OSD_KEY_COMMA,
/*                   0xBD */ OSD_KEY_MINUS,
/*                   0xBE */ OSD_KEY_NONE,
/*                   0xBF */ OSD_KEY_SLASH,
/*                   0xC0 */ OSD_KEY_NONE,
/*                   0xC1 */ OSD_KEY_NONE,
/*                   0xC2 */ OSD_KEY_NONE,
/*                   0xC3 */ OSD_KEY_NONE,
/*                   0xC4 */ OSD_KEY_NONE,
/*                   0xC5 */ OSD_KEY_NONE,
/*                   0xC6 */ OSD_KEY_NONE,
/*                   0xC7 */ OSD_KEY_NONE,
/*                   0xC8 */ OSD_KEY_NONE,
/*                   0xC9 */ OSD_KEY_NONE,
/*                   0xCA */ OSD_KEY_NONE,
/*                   0xCB */ OSD_KEY_NONE,
/*                   0xCC */ OSD_KEY_NONE,
/*                   0xCD */ OSD_KEY_NONE,
/*                   0xCE */ OSD_KEY_NONE,
/*                   0xCF */ OSD_KEY_NONE,
/*                   0xD0 */ OSD_KEY_NONE,
/*                   0xD1 */ OSD_KEY_NONE,
/*                   0xD2 */ OSD_KEY_NONE,
/*                   0xD3 */ OSD_KEY_NONE,
/*                   0xD4 */ OSD_KEY_NONE,
/*                   0xD5 */ OSD_KEY_NONE,
/*                   0xD6 */ OSD_KEY_NONE,
/*                   0xD7 */ OSD_KEY_NONE,
/*                   0xD8 */ OSD_KEY_NONE,
/*                   0xD9 */ OSD_KEY_NONE,
/*                   0xDA */ OSD_KEY_NONE,
/*                   0xDB */ OSD_KEY_OPENBRACE,
/*                   0xDC */ OSD_KEY_NONE,
/*                   0xDD */ OSD_KEY_CLOSEBRACE,
/*                   0xDE */ OSD_KEY_QUOTE,
/*                   0xDF */ OSD_KEY_NONE,
/*                   0xE0 */ OSD_KEY_NONE,
/*                   0xE1 */ OSD_KEY_NONE,
/*                   0xE2 */ OSD_KEY_NONE,
/*                   0xE3 */ OSD_KEY_NONE,
/*                   0xE4 */ OSD_KEY_NONE,
/*                   0xE5 */ OSD_KEY_NONE,
/*                   0xE6 */ OSD_KEY_NONE,
/*                   0xE7 */ OSD_KEY_NONE,
/*                   0xE8 */ OSD_KEY_NONE,
/*                   0xE9 */ OSD_KEY_NONE,
/*                   0xEA */ OSD_KEY_NONE,
/*                   0xEB */ OSD_KEY_NONE,
/*                   0xEC */ OSD_KEY_NONE,
/*                   0xED */ OSD_KEY_NONE,
/*                   0xEE */ OSD_KEY_NONE,
/*                   0xEF */ OSD_KEY_NONE,
/*                   0xF0 */ OSD_KEY_NONE,
/*                   0xF1 */ OSD_KEY_NONE,
/*                   0xF2 */ OSD_KEY_NONE,
/*                   0xF3 */ OSD_KEY_NONE,
/*                   0xF4 */ OSD_KEY_NONE,
/*                   0xF5 */ OSD_KEY_NONE,
/*                   0xF6 */ OSD_KEY_NONE,
/*                   0xF7 */ OSD_KEY_NONE,
/*                   0xF8 */ OSD_KEY_NONE,
/*                   0xF9 */ OSD_KEY_NONE,
/*                   0xFA */ OSD_KEY_NONE,
/*                   0xFB */ OSD_KEY_NONE,
/*                   0xFC */ OSD_KEY_NONE,
/*                   0xFD */ OSD_KEY_NONE,
/*                   0xFE */ OSD_KEY_NONE,
/*                   0xFF */ OSD_KEY_NONE,
};

byte key[OSD_NUMKEYS];

/* stupid stub functions to prevent changing common.c */
DIR * opendir(const char *basename)
{
   return (DIR *)1;
}

struct dirent * readdir(DIR *dirp)
{
   return NULL;
}

void closedir(DIR *dirp)
{

}

LARGE_INTEGER uclocks_per_sec;

DWORDLONG osd_win32_uclock()
{
   LARGE_INTEGER t;

   if (uclocks_per_sec.QuadPart == 1000)
      return (DWORDLONG)timeGetTime();
   QueryPerformanceCounter(&t);
   return (DWORDLONG)t.QuadPart;
}

DWORDLONG osd_win32_uclocks_per_sec()
{
   return (DWORDLONG)uclocks_per_sec.QuadPart;
}

/* put here anything you need to do when the program is started. Return 0 if */
/* initialization was successful, nonzero otherwise. */
int osd_init(int argc,char **argv) 
{ 
   int i;
   int help;

   /* dprintf("osd_init\n"); */

   vector_game = FALSE;
   use_video_memory = FALSE;
   is_window = FALSE;
   scale = 1;
   flip = TRUE;
   rotate = FALSE;
   first_free_pen = 0;
   ending = FALSE;
   palette_created = FALSE;

   if (!QueryPerformanceFrequency(&uclocks_per_sec))
      uclocks_per_sec.QuadPart = 1000;

   help = (argc == 1);
   for (i=1;i<argc;i++)
   {
      if ((stricmp(argv[i],"-win32") == 0) || (stricmp(argv[i],"-win32help") == 0))
	 help = 1;
   }
   
   if (help)
   {
      printf("M.A.M.E. - Multiple Arcade Machine Emulator\n"
	     "Copyright (C) 1997  by Nicola Salmoria and Mirko Buffoni\n\n");
      printf("M.A.M.E. Win32 version created and maintained by Chris Kirmse.\n");
      printf("Please read the readme.txt for more information.  Report ONLY win32\n");
      printf("specific bugs to ckirmse@ricochet.net.\n\n");
      printf("Options:\n");
      printf(" -widthxheight\tSpecify desired screen size \n"
	     "\t\t(must be supported by DirectDraw)\n"
	     "\t\tCommon are -320x240, -400x300, and -640x480\n");
      printf(" -nosound\tDisable sound (speeds up emulation)\n");
      printf(" -rotate\tRotate the display 90 degrees clockwise.\n"
	     "\t\tGreat for those sideways monitors!\n");
      printf(" -noflip\tDon't flip between two primary screen buffers--\n"
	     "\t\tblit from the back buffer.  May be faster on some computers.\n");
      printf(" -nojoy \tDon't use a joystick even if present.\n");
      printf(" -scale 2\tStretch the display by a fact of two.\n"
	     "\t\tGood for larger video modes, if you have a fast video card.\n");
      printf(" -usevideomemory Use video memory for a back buffer\n"
	     "\t\t(only used if using -stretch 2 or -noflip)\n");
      printf(" -window\tRun in a window\n");
      
      return 1;
   }


   for (i = 1;i < argc;i++)
   {
      if (sscanf(argv[i],"-%ix%i",&desired_width,&desired_height) != 2)
      {
	 desired_width = 0;
	 desired_height = 0;
	 if (argv[i][1] >= '0' && argv[i][1] <= '9')
	 {
	    printf("Unable to parse parameter %s\n",argv[i]);
	    continue;
	 }
      }

      if (stricmp(argv[i],"-nosound") == 0)
      {
	 play_sound = FALSE;
	 continue;
      }
      if (stricmp(argv[i],"-rotate") == 0)
      {
	 rotate = TRUE;
	 continue;
      }
      if (stricmp(argv[i],"-norotate") == 0)
      {
	 rotate = FALSE;
	 continue;
      }
      if (stricmp(argv[i],"-window") == 0)
      {
	 is_window = TRUE;
	 continue;
      }
      if (stricmp(argv[i],"-nowindow") == 0)
      {
	 is_window = FALSE;
	 continue;
      }
      if (stricmp(argv[i],"-flip") == 0)
      {
	 flip = TRUE;
	 continue;
      }
      if (stricmp(argv[i],"-noflip") == 0)
      {
	 flip = FALSE;
	 continue;
      }
      if (stricmp(argv[i],"-nojoy") == 0)
      {
	 use_joystick = FALSE;
	 continue;
      }
      if (stricmp(argv[i],"-usevideomemory") == 0)
      {
	 use_video_memory = TRUE;
	 continue;
      }
      if (stricmp(argv[i],"-debug") == 0)
      {
	 win32_debug = 1;
	 continue;
      }
      if (stricmp(argv[i],"-debug2") == 0)
      {
	 win32_debug = 2;
	 continue;
      }

      if (stricmp(argv[i],"-scale") == 0)
      {
	 i++;
	 if (i < argc) 
	    scale = atoi(argv[i]);
	 if (scale < 1) scale = 1;
	 if (scale > 2) scale = 2;
	 continue;
      }
      if (stricmp(argv[i],"-vg") == 0)
	 vector_game = 1;

      /* dprintf("Unable to parse parameter %s\n",argv[i]); */

   }

   if (is_window || scale != 1)
      flip = FALSE;

   if (is_window)
      palette_offset = 10;
   else
      palette_offset = 0;
   /*
   if (!flip)
      dprintf("NOT ");
   dprintf("flipping\n");
   */

   for (i=0;i<OSD_NUMKEYS;i++) 
      key[i]=FALSE;

   osd_win32_joy_init();
   
   if (!is_window)
      ShowCursor(FALSE);

   return 0;

}

void osd_win32_sound_init()
{
   int i;
   int soundcard;

   AUDIOINFO info;
   AUDIOCAPS caps;
   char *blaster_env;

   if (!play_sound)
      return;
   
   /* initialize SEAL audio library */
   if (AInitialize() != AUDIO_ERROR_NONE)
   {
      play_sound = 0;
      return;
   }

   soundcard = -1;


   for (i = 0;i < AGetAudioNumDevs();i++)
   {
      if (AGetAudioDevCaps(i,&caps) == AUDIO_ERROR_NONE)
      {
	 if (caps.wProductId == AUDIO_PRODUCT_DSOUND)
	    soundcard = i;

	 /* printf("  %2d. %i. %s\n",i,caps.wProductId,caps.szProductName); */

      }
   }

   if (soundcard == 0)	/* silence */
   {
      play_sound = 0;
      return;
   }

   /* open audio device */
/* info.nDeviceId = AUDIO_DEVICE_MAPPER;*/
   info.nDeviceId = soundcard;
   info.wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO;
   info.nSampleRate = SAMPLE_RATE;
   if (AOpenAudio(&info) != AUDIO_ERROR_NONE)
   {
      printf("audio initialization failed\n");
      play_sound = 0;
      return;
   }
   
   /* open and allocate voices, allocate waveforms */
   if (AOpenVoices(NUMVOICES) != AUDIO_ERROR_NONE)
   {
      printf("voices initialization failed\n");
      play_sound = 0;
      return;
   }
   
   for (i = 0; i < NUMVOICES; i++)
   {
      if (ACreateAudioVoice(&hVoice[i]) != AUDIO_ERROR_NONE)
      {
	 printf("voice #%d creation failed\n",i);
	 play_sound = 0;
	 return;
      }
      
      ASetVoicePanning(hVoice[i],128);
      
      if ((lpWave[i] = (LPAUDIOWAVE)malloc(sizeof(AUDIOWAVE))) == 0)
      {
         /* have to handle this better...*/
         play_sound = 0;
	 return;
      }
      
      lpWave[i]->wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO;
      lpWave[i]->nSampleRate = SAMPLE_RATE;
      lpWave[i]->dwLength = SAMPLE_BUFFER_LENGTH;
      lpWave[i]->dwLoopStart = lpWave[i]->dwLoopEnd = 0;
      if (ACreateAudioData(lpWave[i]) != AUDIO_ERROR_NONE)
      {
	 dprintf("Error opening audio\n");
	 play_sound = 0;
         /* have to handle this better...*/
	 free(lpWave[i]);
	 
	 return;
      }
   }
}

void osd_win32_joy_init()
{
   JOYINFOEX ji;
   DWORD ret;
   JOYCAPS jc;

   if (!use_joystick)
      return;

   ret = joyGetDevCaps(JOYSTICKID1,&jc,sizeof(JOYCAPS));

/*
   dprintf("joyGetDevCaps %i %i %i\n",ret,JOYERR_NOERROR,GetLastError());
   dprintf("%i %i, %i %i\n",jc.wXmin,jc.wXmax,jc.wYmin,jc.wYmax);
   dprintf("%i\n",jc.wNumButtons);
   */ 
   ji.dwSize = sizeof(JOYINFOEX);
   ji.dwFlags = JOY_RETURNALL;
   ret = joyGetPosEx(JOYSTICKID1,&ji);
   
   use_joystick = (ret == JOYERR_NOERROR);
   if (use_joystick)
   {
      printf("Found joystick 1\n");
      joy1_left_max = jc.wXmin + (jc.wXmax-jc.wXmin)/JOY_RANGE;
      joy1_right_min = jc.wXmax - (jc.wXmax-jc.wXmin)/JOY_RANGE;
      joy1_top_max = jc.wYmin + (jc.wYmax-jc.wYmin)/JOY_RANGE;
      joy1_bottom_min = jc.wYmax - (jc.wYmax-jc.wYmin)/JOY_RANGE;
   }
}   


/* put here cleanup routines to be executed when the program is terminated. */
void osd_exit(void)
{
   if (!is_window)
      ShowCursor(TRUE);
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
      
      for (i = 0;i < bitmap->height;i++)
	 memset(bitmap->line[i],0,bitmap->width);
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


/* Create a display screen, or window, large enough to accomodate a bitmap */
/* of the given dimensions. I don't do any test here (224x288 will just do */
/* for now) but one could e.g. open a window of the exact dimensions */
/* provided. Return a osd_bitmap pointer or 0 in case of error. */
struct osd_bitmap *osd_create_display(int width,int height)
{
   int i;
   WNDCLASS wndclass;
   int style;
   int wndWidth,wndHeight;

   double x_scale, y_scale, vector_scale;
   if (vector_game) {
      if (!rotate) {
	 x_scale=(double)480/(double)(width);
	 y_scale=(double)640/(double)(height);
      } else {
	 x_scale=(double)640/(double)(width);
	 y_scale=(double)480/(double)(height);
      }
      if (x_scale<y_scale)
	 vector_scale=x_scale;
      else
	 vector_scale=y_scale;
      width=(int)((double)width*vector_scale);
      width-=width % 4;
      height=(int)((double)height*vector_scale);
      height-=height % 4;
   }


   dprintf("osd_create_display width %i height %i\n",width,height);

   bitmap = malloc ( sizeof (struct osd_bitmap) + (height-1) * sizeof (unsigned char *) );
   if ( ! bitmap ) { return (NULL); }

   bitmap->width = width;
   bitmap->height = height;

   /* see top for discussion of pitch_space */
   pitch_space = 0;
   if ((bitmap->width & 32) == 0)
      pitch_space = 28;

   buffer_ptr = (byte *) malloc (sizeof(byte) * (bitmap->width+pitch_space) * bitmap->height);
   
   if (!buffer_ptr)
   {
      fprintf (stderr,"OSD ERROR: failed to allocate bitmap buffer.\n");
      return NULL;
   }

   for (i = 0;i < height; i++) 
      bitmap->line[i] = &buffer_ptr[i * (width+pitch_space)];

   bitmap->_private = buffer_ptr;

   wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_CLASSDC;
   wndclass.lpfnWndProc   = WndProc;
   wndclass.cbClsExtra    = 0;
   wndclass.cbWndExtra    = 0;
   wndclass.hInstance     = GetModuleHandle(NULL);
   wndclass.hIcon         = NULL;
   wndclass.hCursor       = LoadCursor(NULL,IDC_ARROW);
   wndclass.hbrBackground = NULL;
   wndclass.lpszMenuName  = NULL;
   wndclass.lpszClassName = szAppName;
   RegisterClass(&wndclass);

   style = WS_OVERLAPPEDWINDOW;

   wndWidth = width*scale + 2*GetSystemMetrics(SM_CXFRAME);
   wndHeight = height*scale + 2*GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYCAPTION);
   hMain = CreateWindow(szAppName,szAppName,style,280,0,wndWidth,wndHeight,NULL,NULL,
			wndclass.hInstance,NULL);

   if (!DirectDrawInitialize())
      return NULL;

   if (!is_window)
   {
      if (desired_width != 0 && desired_height != 0)
	 DirectDrawSetMode(desired_width,desired_height,8);
      else
      {
	 if (rotate)
	    DirectDrawSetClosestMode(height*scale,width*scale);
	 else
	    DirectDrawSetClosestMode(width*scale,height*scale);
      }
   }



   if (!DirectDrawCreateSurfaces(&ddframe))
   {
      printf("Error creating direct draw surfaces\n");
      return NULL;
   }

   if (is_window)
   {
      if (!DirectDrawClipToWindow())
	 printf("Error setting direct draw clipper--trying to proceed without it.\n");
   }
   else
   {
      if (!DirectDrawClipToScreen())
	 printf("Error creating direct draw clipper--trying to proceed without it.\n");
   }

   osd_win32_sound_init();

   ShowWindow(hMain,SW_SHOWNORMAL);

   return bitmap;
}

/*
 * code helpful for making cheats =)
 */

/*
BOOL mem_scanning = FALSE;
BYTE mem_scan_value = 0;
char *memory_mask;

void MemScanInit()
{
   memory_mask = (char *)malloc(65536);
   memset(memory_mask,1,65536);
}

void MemScanPareList()
{
   int i,possible;

   if (!memory_mask)
      MemScanInit();

   for (i=0;i<65536;i++)
      if (RAM[i] != mem_scan_value)
	 memory_mask[i] = 0;

   possible = 0;
   for (i=0;i<65536;i++)
      if (memory_mask[i])
	 possible++;

   dprintf("paring list with %i; %i still possible matches\n",mem_scan_value,possible);
   if (possible < 10)
   {
      for (i=0;i<65536;i++)
	 if (memory_mask[i])
	    dprintf("LOCATION %04X may be it\n",i);
   }
}

void MemScanAddKey(int vk)
{
   if (vk == VK_RETURN)
   {
      MemScanPareList();
      mem_scanning = FALSE;
      return;
   }

   if (vk == VK_F6)
   {
      dprintf("dumping list\n");
      return;
   }

   if (vk >= '0' && vk <= '9')
      mem_scan_value = 10*mem_scan_value + vk - '0';

   
}
*/

long CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
   case WM_NCPAINT :
      if (!is_window)
	 return 0;
      break;
   case WM_PAINT :
   {
      HDC hdc;
      RECT rect;
      PAINTSTRUCT ps;
      
      hdc = BeginPaint(hwnd,&ps);

      /*
      GetClientRect(hwnd,&rect);
      FillRect(hdc,&rect,(HBRUSH)(1+COLOR_BTNFACE));
      */

      osd_update_display();

      EndPaint(hwnd,&ps);
      break;
   }
   case WM_SYSKEYDOWN :
      if (wParam == VK_MENU)
      {
	 /* stupid alt key! */
	 if (Win32KeyToOSDKey(wParam) != 0)
	    key[Win32KeyToOSDKey(wParam)] = 1;
	 return 0;
      }
      break;

   case WM_SYSKEYUP :
      if (wParam == VK_MENU)
      {
	 /* stupid alt key! */
	 if (Win32KeyToOSDKey(wParam) != 0)
	    key[Win32KeyToOSDKey(wParam)] = 0;
	 return 0;
      }
      break;

   case WM_KEYDOWN :
/*
      if (wParam == VK_F9)
      {
	 memory_mask = NULL;
	 break;
      }
      if (wParam == VK_F8)
      {
         RAM[0x9820] = 4; // WORKS FOR GALAGA !! RESETS YOU TO 4 LIVES !! 
	 break;
      }

      if (wParam == VK_F7)
      {
	 if (mem_scanning)
	 {
	    mem_scanning = FALSE;
	    break;
	 }
	 mem_scanning = TRUE;
	 mem_scan_value = 0;
	 break;
      }
      if (mem_scanning)
      {
	 MemScanAddKey(wParam);
	 break;
      }
*/
      
      if (Win32KeyToOSDKey(wParam) != 0)
	 key[Win32KeyToOSDKey(wParam)] = 1;
      break;
   case WM_KEYUP :
      if (Win32KeyToOSDKey(wParam) != 0)
	 key[Win32KeyToOSDKey(wParam)] = 0;
      break;
   case WM_LBUTTONDOWN :
      break;
   case WM_MOUSEMOVE :
      osd_win32_mouse_moved(LOWORD(lParam),HIWORD(lParam));
      break;
   case WM_CLOSE:

      ending = TRUE;
      DestroyWindow(hMain);
      break;

   case WM_ACTIVATEAPP :
      is_active = (BOOL) wParam;
      break;
   }
   return DefWindowProc (hwnd, message, wParam, lParam);
}

/* shut up the display */
void osd_close_display(void)
{
   DirectDrawClose();

   if (play_sound)
      ACloseAudio();
   osd_free_bitmap(bitmap);
}

int osd_obtain_pen(unsigned char red, unsigned char green, unsigned char blue)
{
   int retval;

   pal.entries[first_free_pen].peRed = red;
   pal.entries[first_free_pen].peGreen = green;
   pal.entries[first_free_pen].peBlue = blue;
   pal.entries[first_free_pen].peFlags = PC_NOCOLLAPSE;

   /* dprintf("OOP %u %u %u : %i\n",red,green,blue,
      palette_offset+pal.palNumEntries); */
   retval = (palette_offset+first_free_pen) % 256;
   first_free_pen = (first_free_pen + 1)%256;

   return retval;
}

void osd_win32_set_color(int offset,int data) /* palette animation ! */
{
   if (is_window)
      return; // doesn't work right in a window
   //dprintf("offset %i\n",offset);
   
   pal.entries[offset].peRed = ((data & 0x07)<<5);
   if (pal.entries[offset].peRed != 0)
      pal.entries[offset].peRed += 7;
   
   pal.entries[offset].peGreen = (((data>>3) & 0x07)<<5);
   if (pal.entries[offset].peGreen != 0)
      pal.entries[offset].peGreen += 7;
   
   pal.entries[offset].peBlue = (((data>>6) & 0x03)<<6);
   if (pal.entries[offset].peBlue != 0)
      pal.entries[offset].peBlue += 3;

   DirectDrawSetColor(offset,&pal.entries[offset]);
}

void RotateBitmap(char *dest,char *source,int source_width,int source_height,
		  int dest_pitch,int source_pitch)
{
   int i,j;
   char *ptr,*source_ptr;
   DWORD transfer;

   if (source_width & 3 != 0 || source_height & 3 != 0)
   {
      printf("Error--can't rotate non-multiple of 4 size screens (%ix%i)\n",
	      source_width,source_height);
      printf("Please report the game that caused this error.\n");
   }

   for (j=0;j<source_width;j++)
   {
      ptr = dest + j*dest_pitch;
      source_ptr = source + (source_height-1)*source_pitch + j;
      for (i=0;i<(source_height/4);i++)
      {
	 *((char *)&transfer ) = *source_ptr;
	 source_ptr -= source_pitch;
	 *((char *)&transfer + 1) = *source_ptr;
	 source_ptr -= source_pitch;
	 *((char *)&transfer + 2) = *source_ptr;
	 source_ptr -= source_pitch;
	 *((char *)&transfer + 3) = *source_ptr;
	 source_ptr -= source_pitch;

	 *((DWORD *)ptr) = transfer;

	 ptr += 4;
      }
   }

}

void CopyTransparent(char *dest,char *source,int source_width,int source_height,
		     int dest_pitch,int source_pitch)
{
   int i;
   char *source_ptr,*source_base;

   for (i=0;i<source_height;i++)
   {
      source_ptr = source_base = source + source_pitch*i;
      while (source_ptr < (source_base+source_width))
      {
	 if (*source_ptr)
	    *(dest + dest_pitch*i + (source_ptr-source_base)) = *source_ptr;
	 source_ptr++;
      }
   }
}

/* Update the display. */
/* No need to support saving the screen to a file--while running, just hit
   alt-print screen and the display is saved to the clipboard */
void osd_update_display()
{
   int i;
   int millicount;
   static int frame = 0;

   Surface surface;

   if (!palette_created)
   {
      palette_created = TRUE;
      osd_win32_create_palette();
   }

   /* dprintf("Updating display\n"); */

   if (!is_active && !is_window)
      return;

   millicount = timeGetTime();

   /* lock surface */
   if (!DirectDrawLock(ddframe, &surface))
   {
      dprintf("osd_update_display failed to lock surface\n");
      return;
   }

   if (rotate)
   {
      RotateBitmap(surface.bits,bitmap->_private,bitmap->width,bitmap->height,
		   surface.pitch,bitmap->width+pitch_space);
   }
   else
   {
      if (!is_vector)
      {
	 for (i=0;i<bitmap->height;i++)
	    memcpy(surface.bits + i*surface.pitch,bitmap->line[i],bitmap->width);
      }
      else
	 CopyTransparent(surface.bits,bitmap->_private,bitmap->width,bitmap->height,
			 surface.pitch,bitmap->width+pitch_space);
   }

   /* Unlock surface */
   if (!DirectDrawUnlock(ddframe, &surface))
   {
      dprintf("Failed to unlock surface!\n");
      return;
   }

   DirectDrawUpdateScreen();

   frame++;
   if (win32_debug >= 2 && (frame % 100) == 0)
      dprintf("draw/stretch time %i\n",timeGetTime()-millicount);
}


void osd_update_audio(void)
{
   if (play_sound == 0) 
      return;
   
   AUpdateAudio();
}



void osd_play_sample(int channel,unsigned char *data,int len,int freq,int volume,int loop)
{
   if (play_sound == 0 || channel >= NUMVOICES) return;
   
   memcpy(lpWave[channel]->lpData,data,len);
   if (loop) 
      lpWave[channel]->wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO | AUDIO_FORMAT_LOOP;
   else 
      lpWave[channel]->wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO;
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

   if (play_sound == 0 || channel >= NUMVOICES) 
      return;
   
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
      if (c[channel] == 3)
	 c[channel] = 0;
   }
}



void osd_adjust_sample(int channel,int freq,int volume)
{
   if (play_sound == 0 || channel >= NUMVOICES) 
      return;
   
   Volumi[channel] = volume/4;
   ASetVoiceFrequency(hVoice[channel],freq);
   ASetVoiceVolume(hVoice[channel],MasterVolume*volume/400);
}



void osd_stop_sample(int channel)
{
   if (play_sound == 0 || channel >= NUMVOICES) 
      return;
   
   AStopVoice(hVoice[channel]);
}


void osd_restart_sample(int channel)
{
   if (play_sound == 0 || channel >= NUMVOICES) 
      return;
   
   AStartVoice(hVoice[channel]);
}


int osd_get_sample_status(int channel)
{
   int stopped = 0;

   if (play_sound == 0 || channel >= NUMVOICES) 
      return -1;
   
   AGetVoiceStatus(hVoice[channel], &stopped);
   return stopped;
}

void osd_ym2203_write(int n, int r, int v)
{

}


void osd_ym2203_update(void)
{

}

void osd_set_mastervolume(int volume)
{
   MasterVolume = 3*volume;
}


/* check if a key is pressed. The keycode is the standard PC keyboard code, as */
/* defined in osdepend.h. Return 0 if the key is not pressed, nonzero otherwise. */
int osd_key_pressed(int keycode)
{
   MSG msg;

   /* dprintf("Checking for key %i\n",keycode); */

   while (PeekMessage(&msg,hMain,0,0,PM_REMOVE))
   {
      /* dprintf("OKP Handling message %i\n",msg.message); */
   
      TranslateMessage(&msg);
      DispatchMessage(&msg);

   }

   if (keycode == OSD_KEY_ESC && ending == TRUE)
      return 1;

   /* dprintf("returning %i\n",key[keycode]); */
   return key[keycode];
}



/*
 * Wait until a key is pressed and return its code.
 */

int osd_read_key(void)
{
   MSG msg;
   int osd_key;

   static int first_time = TRUE;

   if (first_time)
   {
      DirectDrawClearScreen();
      first_time = FALSE;
   }

   while (GetMessage(&msg,hMain,0,0))
   {
      TranslateMessage(&msg);
      DispatchMessage(&msg);

      if (ending) 
	 break;

      if (msg.message == WM_KEYDOWN || 
	  (msg.message == WM_SYSKEYDOWN && msg.wParam == VK_MENU))
      {
	 osd_key = Win32KeyToOSDKey(msg.wParam);
	 if (osd_key == 0)
	    return OSD_KEY_1; /* in usrintrf.c, it crashes if we return 0 sometimes */
	 else
	    return osd_key;
      }
   }
   
   osd_exit();

   ExitProcess(1);
   return 1;
}

int Win32KeyToOSDKey(UINT vk)
{
   if (vk <= 255)
   {
      return key_code_table[vk];
   }

   return OSD_KEY_NONE;
}


/* Wait for a key press and return keycode.  Support repeat */
int osd_read_keyrepeat(void)
{
   int i;

   for (i=0;i<OSD_NUMKEYS;i++) 
      key[i]=FALSE;

   return osd_read_key();
}


/* return the name of a key */
const char *osd_key_name(int keycode)
{
   static char *keynames[] = 
   { 
      "ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
      "MINUS", "EQUAL", "BACKSPACE", "TAB", "Q", "W", "E", "R", "T", "Y",
      "U", "I", "O", "P", "OPENBRACE", "CLOSEBRACE", "ENTER", "CONTROL",
      "A", "S", "D", "F", "G", "H", "J", "K", "L", "COLON", "QUOTE",
      "TILDE", "LEFTSHIFT", "NULL", "Z", "X", "C", "V", "B", "N", "M", "COMMA",
      "STOP", "SLASH", "RIGHTSHIFT", "ASTERISK", "ALT", "SPACE", "CAPSLOCK",
      "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "NUMLOCK",
      "SCRLOCK", "HOME", "UP", "PGUP", "MINUS PAD", "LEFT", "5 PAD", "RIGHT",
      "PLUS PAD", "END", "DOWN", "PGDN", "INS", "DEL", "F11", "F12" 
   };
   
   if (keycode && keycode <= OSD_MAX_KEY) 
      return (char *)keynames[keycode-1];
   
   return 0;
}


/* return the name of a joystick button */
const char *osd_joy_name(int joycode)
{
   static char *joynames[] = 
   { 
      "LEFT", "RIGHT", "UP", "DOWN", "Button A",
      "Button B", "Button C", "Button D", "Any Button" };

   if (joycode && joycode <= OSD_MAX_JOY) 
      return (char *)joynames[joycode-1];
   
   return 0;
}


int joy_left, joy_right, joy_up, joy_down;
int joy_b1, joy_b2;
int joy_b3, joy_b4;

void osd_poll_joystick(void)
{
   JOYINFOEX joy1_ji;
   static char prev;

/* for qbert 
   if (RAM[0x0D1F] == 0)
   {
      dprintf("changing it\n");
      RAM[0x0D1F] = 5;
   }
   prev = RAM[0x0D1F];
   */

   /* dprintf("osd_poll_joystick\n"); */
   
   if (!use_joystick)
      return;

   joy1_ji.dwSize = sizeof(JOYINFOEX);
   joy1_ji.dwFlags = JOY_RETURNBUTTONS | JOY_RETURNX | JOY_RETURNY; /* | JOY_RETURNRAWDATA; */
   if (joyGetPosEx(JOYSTICKID1,&joy1_ji) != JOYERR_NOERROR)
   {
      dprintf("Error reading joystick 1 which we know exists!\n");
      return;
   }

   /* dprintf("x %i y %i\n",joy1_ji.dwXpos,joy1_ji.dwYpos); */
   /* dprintf("joy at %i, %i\n",joy1_ji.dwXpos,joy1_ji.dwYpos); */
   
   joy_left = joy1_ji.dwXpos < joy1_left_max;
   joy_right = joy1_ji.dwXpos > joy1_right_min;
   joy_up = joy1_ji.dwYpos < joy1_top_max;
   joy_down = joy1_ji.dwYpos > joy1_bottom_min;

   joy_b1 = joy1_ji.dwButtons & JOY_BUTTON1;
   joy_b2 = joy1_ji.dwButtons & JOY_BUTTON2;
   joy_b3 = joy1_ji.dwButtons & JOY_BUTTON3;
   joy_b4 = joy1_ji.dwButtons & JOY_BUTTON4;
}



/* check if the joystick is moved in the specified direction, defined in */
/* osdepend.h. Return 0 if it is not pressed, nonzero otherwise. */
int osd_joy_pressed(int joycode)
{
   switch (joycode)
   {
   case OSD_JOY_LEFT:
      return joy_left;
   case OSD_JOY_RIGHT:
      return joy_right;
   case OSD_JOY_UP:
      return joy_up;
   case OSD_JOY_DOWN:
      return joy_down;
   case OSD_JOY_FIRE1:
      return joy_b1;
   case OSD_JOY_FIRE2:
      return joy_b2;
   case OSD_JOY_FIRE3:
      return joy_b3;
   case OSD_JOY_FIRE4:
      return joy_b4;
   case OSD_JOY_FIRE:
      return joy_b1 || joy_b2 || joy_b3 || joy_b4;
   }
   return 0;
}

void dprintf(char *fmt,...)
{
   char s[500];
   DWORD written;
   va_list marker;

   if (!win32_debug)
      return;

   va_start(marker,fmt);

   vsprintf(s,fmt,marker);

#if 1
   WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),s,strlen(s),&written,NULL);
//#else
   {
      static FILE *junk;
      if (junk == NULL)
	 junk = fopen("debug.txt","wt");
      if (junk != NULL)
	 fprintf(junk, s);
   }
#endif
   va_end(marker);
}


void osd_win32_create_palette()
{
   HDC hdc;
   int i;
   BOOL is_palette_device;

   /* dprintf("creating palette of %i colors\n",pal.palNumEntries); */

   DirectDrawSetPalette(&pal);
}

void osd_win32_mouse_moved(int x,int y)
{
   mouse_x = x;
   mouse_y = y;
   /* printf("%i %i\n",mouse_x,mouse_y); */
}

int osd_trak_read(int axis) 
{
   if(!use_trak) 
   {
      return(NO_TRAK);
  }

   return 0;
}

void osd_trak_center_x(void) {
  large_trak_x = 0;
}

void osd_trak_center_y(void) {
  large_trak_y = 0;
}

#define MAXPIXELS 100000
char * pixel[MAXPIXELS];
int p_index=-1;

__inline void draw_pixel (int x, int y, int col)
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

void open_page (int *x_res, int *y_res, int step)
{
	int i;
	unsigned char bg;

	*x_res=bitmap->width;
	*y_res=bitmap->height;
	bg=Machine->pens[0];
	for (i=p_index; i>=0; i--)
	{
		*(pixel[i])=bg;
	}
	p_index=-1;
}

void close_page (void)
{
}

void draw_to (int x2, int y2, int col)
{
	static int x1=0;
	static int y1=0;

	int temp_x, temp_y;
	int dx,dy,cx,cy,sx,sy;
	
#if 0
	if (x1<0 || y1<0 || x2<0 || y2<0 ||
		x1>=bitmap->width || y1>=bitmap->height ||
		x2>=bitmap->width || y2>=bitmap->height)
	{
		if (errorlog)
			fprintf(errorlog,
			"line:%d,%d nach %d,%d color %d\n",
			 x1,y1,x2,y2,col);
		return;
	}
#endif

	if (col<0)
	{
		x1=x2;
		y1=y2;
		return;
	} else 
		col=Machine->gfx[0]->colortable[col];

	temp_x = x2; temp_y = y2;
	
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
