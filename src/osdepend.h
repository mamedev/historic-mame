#ifndef OSDEPEND_H
#define OSDEPEND_H

#include "osd_cpu.h"
#include "inptport.h"


/* The Win32 port requires this constant for variable arg routines. */
#ifndef CLIB_DECL
#define CLIB_DECL
#endif

/* TODO: this should be removed and put into a unix-specific header */
/** suggested by  Scott Trent */
#ifdef aix
#include <sys/time.h>
#endif

#ifdef __LP64__
#define FPTR long   /* 64bit: sizeof(void *) is sizeof(long)  */
#else
#define FPTR int
#endif


int osd_init(void);
void osd_exit(void);


/******************************************************************************

  Display

******************************************************************************/

struct osd_bitmap
{
	int width,height;       /* width and height of the bitmap */
	int depth;		/* bits per pixel - ASG 980209 */
	void *_private; /* don't touch! - reserved for osdepend use */
	unsigned char **line; /* pointers to the start of each line */
};

/* VERY IMPORTANT: the function must allocate also a "safety area" 16 pixels wide all */
/* around the bitmap. This is required because, for performance reasons, some graphic */
/* routines don't clip at boundaries of the bitmap. */
struct osd_bitmap *osd_new_bitmap(int width,int height,int depth);	/* ASG 980209 */
#define osd_create_bitmap(w,h) osd_new_bitmap((w),(h),8)		/* ASG 980209 */
void osd_clearbitmap(struct osd_bitmap *bitmap);
void osd_free_bitmap(struct osd_bitmap *bitmap);
/* Create a display screen, or window, large enough to accomodate a bitmap */
/* of the given dimensions. Attributes are the ones defined in driver.h. */
/* Return a osd_bitmap pointer or 0 in case of error. */
struct osd_bitmap *osd_create_display(int width,int height,int attributes);
int osd_set_display(int width,int height,int attributes);
void osd_close_display(void);
/* palette is an array of 'totalcolors' R,G,B triplets. The function returns */
/* in *pens the pen values corresponding to the requested colors. */
/* If 'totalcolors' is 32768, 'palette' is ignored and the *pens array is filled */
/* with pen values corresponding to a 5-5-5 15-bit palette */
void osd_allocate_colors(unsigned int totalcolors,const unsigned char *palette,unsigned short *pens);
void osd_modify_pen(int pen,unsigned char red, unsigned char green, unsigned char blue);
void osd_get_pen(int pen,unsigned char *red, unsigned char *green, unsigned char *blue);
void osd_mark_dirty(int xmin, int ymin, int xmax, int ymax, int ui);    /* ASG 971011 */
int osd_skip_this_frame(void);
void osd_update_video_and_audio(void);
void osd_set_gamma(float _gamma);
float osd_get_gamma(void);
void osd_set_brightness(int brightness);
int osd_get_brightness(void);
void osd_save_snapshot(void);


/******************************************************************************

  Sound

******************************************************************************/

void osd_play_sample(int channel,signed char *data,int len,int freq,int volume,int loop);
void osd_play_sample_16(int channel,signed short *data,int len,int freq,int volume,int loop);
void osd_play_streamed_sample(int channel,signed char *data,int len,int freq,int volume,int pan);
void osd_play_streamed_sample_16(int channel,signed short *data,int len,int freq,int volume,int pan);
void osd_set_sample_freq(int channel,int freq);
void osd_set_sample_volume(int channel,int volume);
void osd_stop_sample(int channel);
void osd_restart_sample(int channel);
int osd_get_sample_status(int channel);
void osd_opl_control(int chip,int reg);
void osd_opl_write(int chip,int data);
void osd_set_mastervolume(int attenuation);
int osd_get_mastervolume(void);
void osd_sound_enable(int enable);


/******************************************************************************

  Keyboard

******************************************************************************/

/*
  return a list of all available keys (see input.h)
*/
const struct KeyboardInfo *osd_get_key_list(void);

/*
  tell whether the specified key is pressed or not. keycode is the OS dependant
  code specified in the list returned by osd_get_key_list().
*/
int osd_is_key_pressed(int keycode);

/*
  wait for the user to press a key and return its code. This function is not
  required to do anything, it is here so we can avoid bogging down multitasking
  systems while using the debugger. If you don't want to or can't support this
  function you can just return KEYCODE_NONE.
*/
int osd_wait_keypress(void);


/******************************************************************************

  Joystick & Mouse/Trackball

******************************************************************************/

/*
  return a list of all available joystick inputs (see input.h)
*/
const struct JoystickInfo *osd_get_joy_list(void);

/*
  tell whether the specified joystick direction/button is pressed or not.
  joycode is the OS dependant code specified in the list returned by
  osd_get_joy_list().
*/
int osd_is_joy_pressed(int joycode);


/* We support 4 players for each analog control */
#define OSD_MAX_JOY_ANALOG	4
#define X_AXIS          1
#define Y_AXIS          2

void osd_poll_joysticks(void);

/* Joystick calibration routines BW 19981216 */
/* Do we need to calibrate the joystick at all? */
int osd_joystick_needs_calibration (void);
/* Preprocessing for joystick calibration. Returns 0 on success */
void osd_joystick_start_calibration (void);
/* Prepare the next calibration step. Return a description of this step. */
/* (e.g. "move to upper left") */
char *osd_joystick_calibrate_next (void);
/* Get the actual joystick calibration data for the current position */
void osd_joystick_calibrate (void);
/* Postprocessing (e.g. saving joystick data to config) */
void osd_joystick_end_calibration (void);

void osd_trak_read(int player,int *deltax,int *deltay);

/* return values in the range -128 .. 128 (yes, 128, not 127) */
void osd_analogjoy_read(int player,int *analog_x, int *analog_y);


/*
  inptport.c defines some general purpose defaults for key and joystick bindings.
  They may be further adjusted by the OS dependant code to better match the
  available keyboard, e.g. one could map pause to the Pause key instead of P, or
  snapshot to PrtScr instead of F12. Of course the user can further change the
  settings to anything he/she likes.
  This function is called on startup, before reading the configuration from disk.
  Scan the list, and change the keys/joysticks you want.
*/
void osd_customize_inputport_defaults(struct ipd *defaults);


/******************************************************************************

  File I/O

******************************************************************************/

/* inp header */
typedef struct {
    char name[9];      /* 8 bytes for game->name + NULL */
    char version[3];   /* byte[0] = 0, byte[1] = version byte[2] = beta_version */
    char reserved[20]; /* for future use, possible store game options? */
} INP_HEADER;


/* file handling routines */
#define OSD_FILETYPE_ROM 1
#define OSD_FILETYPE_SAMPLE 2
#define OSD_FILETYPE_HIGHSCORE 3
#define OSD_FILETYPE_CONFIG 4
#define OSD_FILETYPE_INPUTLOG 5
#define OSD_FILETYPE_STATE 6
#define OSD_FILETYPE_ARTWORK 7
#define OSD_FILETYPE_MEMCARD 8
#define OSD_FILETYPE_SCREENSHOT 9
#ifdef MESS
  #define OSD_FILETYPE_ROM_CART 10
  #define OSD_FILETYPE_IMAGE 11
#endif

/* gamename holds the driver name, filename is only used for ROMs and    */
/* samples. If 'write' is not 0, the file is opened for write. Otherwise */
/* it is opened for read. */

int osd_faccess(const char *filename, int filetype);
void *osd_fopen(const char *gamename,const char *filename,int filetype,int read_or_write);
int osd_fread(void *file,void *buffer,int length);
int osd_fwrite(void *file,const void *buffer,int length);
int osd_fread_swap(void *file,void *buffer,int length);
int osd_fwrite_swap(void *file,const void *buffer,int length);
#if LSB_FIRST
#define osd_fread_msbfirst osd_fread_swap
#define osd_fwrite_msbfirst osd_fwrite_swap
#define osd_fread_lsbfirst osd_fread
#define osd_fwrite_lsbfirst osd_fwrite
#else
#define osd_fread_msbfirst osd_fread
#define osd_fwrite_msbfirst osd_fwrite
#define osd_fread_lsbfirst osd_fread_swap
#define osd_fwrite_lsbfirst osd_fwrite_swap
#endif
int osd_fread_scatter(void *file,void *buffer,int length,int increment);
int osd_fseek(void *file,int offset,int whence);
void osd_fclose(void *file);
int osd_fchecksum(const char *gamename, const char *filename, unsigned int *length, unsigned int *sum);
int osd_fsize(void *file);
unsigned int osd_fcrc(void *file);


/******************************************************************************

  Miscellaneous

******************************************************************************/

/* called while loading ROMs. It is called a last time with name == 0 to signal */
/* that the ROM loading process is finished. */
/* return non-zero to abort loading */
int osd_display_loading_rom_message(const char *name,int current,int total);

/* called when the game is paused/unpaused, so the OS dependant code can do special */
/* things like changing the title bar or darkening the display. */
/* Note that the OS dependant code must NOT stop processing input, since the user */
/* interface is still active while the game is paused. */
void osd_pause(int paused);

/* control keyboard leds or other indicators */
void osd_led_w(int led,int on);

/* config */
void osd_set_config(int def_samplerate, int def_samplebits);
void osd_save_config(int frameskip, int samplerate, int samplebits);
int osd_get_config_samplerate(int def_samplerate);
int osd_get_config_samplebits(int def_samplebits);
int osd_get_config_frameskip(int def_frameskip);


#ifdef MAME_NET
/* network */
int osd_net_init(void);
int osd_net_send(int player, unsigned char buf[], int *size);
int osd_net_recv(int player, unsigned char buf[], int *size);
int osd_net_sync(void);
int osd_net_input_sync(void);
int osd_net_exit(void);
int osd_net_add_player(void);
int osd_net_remove_player(int player);
int osd_net_game_init(void);
int osd_net_game_exit(void);
#endif /* MAME_NET */

#endif
