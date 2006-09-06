/***************************************************************************

    osdepend.h

    OS-dependent code interface.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __OSDEPEND_H__
#define __OSDEPEND_H__

#include "mamecore.h"
#include "timer.h"
#include <stdarg.h>

int osd_init(void);


void osd_wait_for_debugger(void);



/******************************************************************************

    Display

******************************************************************************/

int osd_update(mame_time emutime);


/*
  Returns a pointer to the text to display when the FPS display is toggled.
  This normally includes information about the frameskip, FPS, and percentage
  of full game speed.
*/
const char *osd_get_fps_text(const performance_info *performance);



/******************************************************************************

    Sound

******************************************************************************/

/*
  osd_start_audio_stream() is called at the start of the emulation to initialize
  the output stream, then osd_update_audio_stream() is called every frame to
  feed new data. osd_stop_audio_stream() is called when the emulation is stopped.

  The sample rate is fixed at Machine->sample_rate. Samples are 16-bit, signed.
  When the stream is stereo, left and right samples are alternated in the
  stream.

  osd_start_audio_stream() and osd_update_audio_stream() must return the number
  of samples (or couples of samples, when using stereo) required for next frame.
  This will be around Machine->sample_rate / Machine->screen[0].refresh,
  the code may adjust it by SMALL AMOUNTS to keep timing accurate and to
  maintain audio and video in sync when using vsync. Note that sound emulation,
  especially when DACs are involved, greatly depends on the number of samples
  per frame to be roughly constant, so the returned value must always stay close
  to the reference value of Machine->sample_rate / Machine->screen[0].refresh.
  Of course that value is not necessarily an integer so at least a +/- 1
  adjustment is necessary to avoid drifting over time.
*/
int osd_start_audio_stream(int stereo);
int osd_update_audio_stream(INT16 *buffer);
void osd_stop_audio_stream(void);

/*
  control master volume. attenuation is the attenuation in dB (a negative
  number). To convert from dB to a linear volume scale do the following:
    volume = MAX_VOLUME;
    while (attenuation++ < 0)
        volume /= 1.122018454;      //  = (10 ^ (1/20)) = 1dB
*/
void osd_set_mastervolume(int attenuation);
int osd_get_mastervolume(void);

void osd_sound_enable(int enable);



/******************************************************************************

    Controls

******************************************************************************/

typedef UINT32 os_code;

typedef struct _os_code_info os_code_info;
struct _os_code_info
{
	char *			name;			/* OS dependant name; 0 terminates the list */
	os_code		oscode;			/* OS dependant code */
	input_code	inputcode;		/* CODE_xxx equivalent from input.h, or one of CODE_OTHER_* if n/a */
};

/*
  return a list of all available inputs (see input.h)
*/
const os_code_info *osd_get_code_list(void);

/*
  return the value of the specified input. digital inputs return 0 or 1. analog
  inputs should return a value between -65536 and +65536. oscode is the OS dependent
  code specified in the list returned by osd_get_code_list().
*/
INT32 osd_get_code_value(os_code oscode);

/*
  Return the Unicode value of the most recently pressed key. This
  function is used only by text-entry routines in the user interface and should
  not be used by drivers. The value returned is in the range of the first 256
  bytes of Unicode, e.g. ISO-8859-1. A return value of 0 indicates no key down.

  Set flush to 1 to clear the buffer before entering text. This will avoid
  having prior UI and game keys leak into the text entry.
*/
int osd_readkey_unicode(int flush);

/*
  inptport.c defines some general purpose defaults for key and joystick bindings.
  They may be further adjusted by the OS dependent code to better match the
  available keyboard, e.g. one could map pause to the Pause key instead of P, or
  snapshot to PrtScr instead of F12. Of course the user can further change the
  settings to anything he/she likes.
  This function is called on startup, before reading the configuration from disk.
  Scan the list, and change the keys/joysticks you want.
*/
void osd_customize_inputport_list(input_port_default_entry *defaults);


/* Joystick calibration routines BW 19981216 */
/* Do we need to calibrate the joystick at all? */
int osd_joystick_needs_calibration(void);
/* Preprocessing for joystick calibration. Returns 0 on success */
void osd_joystick_start_calibration(void);
/* Prepare the next calibration step. Return a description of this step. */
/* (e.g. "move to upper left") */
const char *osd_joystick_calibrate_next(void);
/* Get the actual joystick calibration data for the current position */
void osd_joystick_calibrate(void);
/* Postprocessing (e.g. saving joystick data to config) */
void osd_joystick_end_calibration(void);



/******************************************************************************

    File I/O

******************************************************************************/

/* inp header */
typedef struct _inp_header inp_header;
struct _inp_header
{
	char name[9];      /* 8 bytes for game->name + NUL */
	char version[3];   /* byte[0] = 0, byte[1] = version byte[2] = beta_version */
	char reserved[20]; /* for future use, possible store game options? */
};


/* These values are returned by osd_get_path_info */
enum
{
	PATH_NOT_FOUND,
	PATH_IS_FILE,
	PATH_IS_DIRECTORY
};

typedef struct _osd_file osd_file;

/* Return the number of paths for a given type */
int osd_get_path_count(int pathtype);

/* Get information on the existence of a file */
int osd_get_path_info(int pathtype, int pathindex, const char *filename);

/* Create a directory if it doesn't already exist */
int osd_create_directory(int pathtype, int pathindex, const char *dirname);

/* Attempt to open a file with the given name and mode using the specified path type */
osd_file *osd_fopen(int pathtype, int pathindex, const char *filename, const char *mode, osd_file_error *error);

/* Seek within a file */
int osd_fseek(osd_file *file, INT64 offset, int whence);

/* Return current file position */
UINT64 osd_ftell(osd_file *file);

/* Return 1 if we're at the end of file */
int osd_feof(osd_file *file);

/* Read bytes from a file */
UINT32 osd_fread(osd_file *file, void *buffer, UINT32 length);

/* Write bytes to a file */
UINT32 osd_fwrite(osd_file *file, const void *buffer, UINT32 length);

/* Close an open file */
void osd_fclose(osd_file *file);



/******************************************************************************

    Timing

******************************************************************************/

typedef INT64 cycles_t;

/* return the current number of cycles, or some other high-resolution timer */
cycles_t osd_cycles(void);

/* return the number of cycles per second */
cycles_t osd_cycles_per_second(void);

/* return the current number of cycles, or some other high-resolution timer.
   This call must be the fastest possible because it is called by the profiler;
   it isn't necessary to know the number of ticks per seconds. */
cycles_t osd_profiling_ticks(void);



/******************************************************************************

    Miscellaneous

******************************************************************************/

/* called to allocate/free memory that can contain executable code */
void *osd_alloc_executable(size_t size);
void osd_free_executable(void *ptr, size_t size);

/* called while loading ROMs. It is called a last time with name == 0 to signal */
/* that the ROM loading process is finished. */
/* return non-zero to abort loading */
int osd_display_loading_rom_message(const char *name,rom_load_data *romdata);

/* checks to see if a pointer is bad */
int osd_is_bad_read_ptr(const void *ptr, size_t size);

/* multithreading locks; only need to implement if you use threads */
typedef struct _osd_lock osd_lock;
osd_lock *osd_lock_alloc(void);
void osd_lock_acquire(osd_lock *lock);
int osd_lock_try(osd_lock *lock);
void osd_lock_release(osd_lock *lock);
void osd_lock_free(osd_lock *lock);


#ifdef MESS
/* this is here to follow the current mame file hierarchy style */
#include "osd_mess.h"
#endif

#endif	/* __OSDEPEND_H__ */
