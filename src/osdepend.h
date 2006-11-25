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
#include "fileio.h"
#include "timer.h"
#include <stdarg.h>



/******************************************************************************

    The prototypes in this file describe the interfaces that the MAME core
    relies upon to interact with the outside world. They are broken out into
    several categories.

    The general flow for an OSD port of MAME is as follows:

        - parse the command line or display the frontend
        - call run_game (mame.c) with the index in the driver list of
            the game selected
            - osd_init() is called shortly afterwards; at this time, you are
                expected to set up the display system and create render_targets
            - the input system will call osd_get_code_list()
            - the input port system will call osd_customize_inputport_list()
            - the sound system will call osd_start_audio_stream()
            - while the game runs, osd_update() will be called periodically
            - when the game exits, we return from run_game()
        - the OSD layer is now in control again

    This process is expected to be in flux over the next several versions
    (this was written during 0.109u2 development) as some of the OSD
    responsibilities are pushed into the core.

******************************************************************************/




/*-----------------------------------------------------------------------------
    osd_init: initialize the OSD system.

    Parameters:

        machine - pointer to a structure that contains parameters for the
            current "machine"

    Return value:

        0 if no errors occurred, or non-zero in case of an error.

    Notes:

        This function has several responsibilities.

        The most important responsibility of this function is to create the
        render_targets that will be used by the MAME core to provide graphics
        data to the system. Although it is possible to do this later, the
        assumption in the MAME core is that the user interface will be
        visible starting at osd_init() time, so you will have some work to
        do to avoid these assumptions.

        A secondary responsibility of this function is to initialize any
        other OSD systems that require information about the current
        running_machine.

        This callback is also the last opportunity to adjust the options
        before they are consumed by the rest of the core.

        Note that there is no corresponding osd_exit(). Rather, like most
        systems in MAME, you can register an exit callback via the
        add_exit_callback() function in mame.c.

    Future work/changes:

        The return value may be removed in the future. It is recommended that
        if you encounter an error at this time, that you call the core
        function fatalerror() with a message to the user rather than relying
        on this return value.

        Audio and input initialization may eventually move into here as well,
        instead of relying on independent callbacks from each system.
-----------------------------------------------------------------------------*/
int osd_init(running_machine *machine);


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
	char *		name;			/* OS dependant name; 0 terminates the list */
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


/* osd_file is an opaque type which represents an open file */
typedef struct _osd_file osd_file;


/*-----------------------------------------------------------------------------
    osd_open: open a new file.

    Parameters:

        path - path to the file to open

        openflags - some combination of:

            OPEN_FLAG_READ - open the file for read access
            OPEN_FLAG_WRITE - open the file for write access
            OPEN_FLAG_CREATE - create/truncate the file when opening; this
                    flag also implies that an non-existant paths should be
                    created if necessary

        file - pointer to an osd_file * to receive the newly-opened file
            handle; this is only valid if the function returns FILERR_NONE

        filesize - pointer to a UINT64 to receive the size of the opened
            file; this is only valid if the function returns FILERR_NONE

    Return value:

        a mame_file_error describing any error that occurred while opening
        the file, or FILERR_NONE if no error occurred

    Notes:

        This function is called by mame_fopen and several other places in
        the core to access files. These functions will construct paths by
        concatenating various search paths held in the options.c options
        database with partial paths specified by the core. The core assumes
        that the path separator is the first character of the string
        PATH_SEPARATOR, but does not interpret any path separators in the
        search paths, so if you use a different path separator in a search
        path, you may get a mixture of PATH_SEPARATORs (from the core) and
        alternate path separators (specified by users and placed into the
        options database).
-----------------------------------------------------------------------------*/
mame_file_error osd_open(const char *path, UINT32 openflags, osd_file **file, UINT64 *filesize);


/*-----------------------------------------------------------------------------
    osd_close: close an open file

    Parameters:

        file - handle to a file previously opened via osd_open

    Return value:

        a mame_file_error describing any error that occurred while closing
        the file, or FILERR_NONE if no error occurred
-----------------------------------------------------------------------------*/
mame_file_error osd_close(osd_file *file);


/*-----------------------------------------------------------------------------
    osd_read: read from an open file

    Parameters:

        file - handle to a file previously opened via osd_open

        buffer - pointer to memory that will receive the data read

        offset - offset within the file to read from

        length - number of bytes to read from the file

        actual - pointer to a UINT32 to receive the number of bytes actually
            read during the operation; valid only if the function returns
            FILERR_NONE

    Return value:

        a mame_file_error describing any error that occurred while reading
        from the file, or FILERR_NONE if no error occurred
-----------------------------------------------------------------------------*/
mame_file_error osd_read(osd_file *file, void *buffer, UINT64 offset, UINT32 length, UINT32 *actual);


/*-----------------------------------------------------------------------------
    osd_write: write to an open file

    Parameters:

        file - handle to a file previously opened via osd_open

        buffer - pointer to memory that contains the data to write

        offset - offset within the file to write to

        length - number of bytes to write to the file

        actual - pointer to a UINT32 to receive the number of bytes actually
            written during the operation; valid only if the function returns
            FILERR_NONE

    Return value:

        a mame_file_error describing any error that occurred while writing to
        the file, or FILERR_NONE if no error occurred
-----------------------------------------------------------------------------*/
mame_file_error osd_write(osd_file *file, const void *buffer, UINT64 offset, UINT32 length, UINT32 *actual);



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

/* checks to see if a pointer is bad */
int osd_is_bad_read_ptr(const void *ptr, size_t size);

/* breaks into OSD debugger if attached */
void osd_break_into_debugger(const char *message);

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
