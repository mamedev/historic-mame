/*
 * An adaptation of MAME 0.2 for X11 / 0.81 for DOS.
 * Can be played. Has only been tested on Linux, but should port easily.
 *
 * All thanks go to Nicola Salmoria for building a clean cut MSDOS emu.
 * Most of the adaptation of XMultiPac (a previous version of the
 * X-interface) was a copy/paste from his 'msdos.c'.
 *
 * ----- HELP! ---------------------------------------------------------------
 *
 * We've had terrible problems implementing sound. If there's anybody out
 * there who thinks she/he can do better: you're welcome. We just don't know
 * enough about it. You can test sound on your machine (Linux only, for
 * now) by specifying -sound on the command-line.
 *
 * Using the SEAL lib (which is available for Windoze 95, NT, Dos & Linux)
 * does not seem to be a good idea. Sound is terrible. Right now, a
 * primitive mixing scheme is implemented using the timer - see the code.
 * Our problem is, that we either have
 * - ticking sound (when we do not pump enough samples to /dev/dsp), or
 * - sound which is 1-5 seconds too slow (when we pump too much samples).
 * If we use the Buffers (now quoted) we get strange echoing sounds running
 * a few seconds after a sample is played. Note: we're testing on a GUS.
 *
 * ---------------------------------------------------------------------------
 *
 * By the way:
 *
 * - Joystick support should be possible in XFree 3.2 using the
 *   extension. Haven't looked at it yet.
 *
 * Created: March, 5th
 *          Allard van der Bas	avdbas@wi.leidenuniv.nl
 *          Dick de Ridder			dick@ph.tn.tudelft.nl
 *
 * Visit the arcade emulator programming repository at
 *   http://valhalla.ph.tn.tudelft.nl/emul8
 *
 * Revisions: #define DR1, #define DR2, Dick de Ridder, 8-3-97
 *            Some bugfixes after profiling with Purify on SunOS.
 *
 *            #define DR3, Dick de Ridder, 7-4-97
 *            Upgrade to Mame 0.15.
 *
 *
 * Added Joystick support under Linux. by Juan Antonio Martinez
 * jantonio@dit.upm.es 15-Apr-1997.
 * Need module joystick-0.8.0 to be insmod'ed prior to use it
 *
 * Some fixes by Jack Patton, 9-5-97.
 * Some fixes by Allard van der Bas 10-9-97 (who also broke the already
 * fragile sound under linux :-) ).
 *
 *            #define CS1, Terry Conklin, 23-3-97
 *            New code to support scaling the display by Terry Conklin 23-3-97
 *    				Wrote the Shared mem code blind. Hope it works.  conklin@mich.com
 *    				specify -widthscale and -heightscale on the command line.
 *    				Large scaling will KILL your processor.     
 *   
 *            #define DB1, David Black, 25-3-97
 *            Perfected sound. ( set as fixed option on xmame-0.18.1 )
 */

/*
 * Include files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#ifdef linux
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef FREEBSD
#include <sys/soundcard.h>
#else
#include <machine/soundcard.h>
#endif
#include <sys/ioctl.h>
#include <errno.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "osdepend.h"
/* include table for linear-to-ulaw conversion */
#if defined solaris || defined sunos
#include "lin2ulaw.h"
#endif

#ifdef UNIX
#define stricmp         strcasecmp
#define strnicmp        strncasecmp
#endif

/*
 * Definitions.
 *
 * Define MITSHM to use the MIT Shared Memory extension. This
 * will speed things up considerably. Should really be a command line switch
 * or auto-detection thing (XQueryExtensions).
 */

#define DR1
#define DR2
#define DR3
#define CS1

typedef unsigned char		byte;

#define OSD_NUMKEYS		(100)
#define OSD_NUMPENS		(256)

#define	TRUE			(1)
#define FALSE			(0)

#define OSD_OK			(0)
#define OSD_NOT_OK		(1)

#define DEBUG(x)

#ifdef MITSHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

XShmSegmentInfo 	shm_info;
#endif

/*
 * Global variables.
 */

Display 			*display;
Window 		  	 	 window;
Colormap 	 		 colormap;
XImage 				*image;
GC 				 gc;

#ifdef CS1

/* CS: scaling hack - integer factors only */

int 		scaling		= FALSE;
unsigned int 	widthscale	= 1;
unsigned int 	heightscale 	= 1;

/* CS: scaling hack */
byte  *scaled_buffer_ptr;

#endif

/* Just the default GC - we don't really draw. */

unsigned long  		 	White,
			 	Black;

unsigned long 	*xpixel = NULL;	/* Pixel index values for the colors we need. */


/* Fixed title to say Mame - Jack Patton (jpatton@intserv.com) */

byte	*buffer_ptr;
char	*title = "XMame 0.18";
int 	 snapshot_no	= 0;

struct osd_bitmap	*bitmap;
int			 use_joystick;
int			 play_sound;
#ifdef DR3
int			 video_sync;
#endif
int			 first_free_pen;

int      osd_joy_up, osd_joy_down, osd_joy_left, osd_joy_right;
int 	 osd_joy_b1, osd_joy_b2, osd_joy_b3, osd_joy_b4;

/*
 * Workaround: in DO$, the Allegro library maintains this array.
 * In X, we'll have to do it ourselves. Bummer.
 *
 * Note: this can be speeded up considerably (see the enormous switch
 * statement in osd_key_pressed).
 */

byte			 key[OSD_NUMKEYS];

/*
 * Audio variables.
 */

#define AUDIO_SAMPLE_FREQ			(22050)
#define AUDIO_SAMPLE_LOWFREQ			(8000)
#define AUDIO_SAMPLE_BITS			(8)
#define AUDIO_NUM_BUFS				(0)
#define AUDIO_BUFSIZE_BITS			(8)
#define AUDIO_MIX_BUFSIZE			(128)
#define AUDIO_NUM_VOICES			(6)

int	   audio_fd;

int	   audio_sample_freq; 
int	   audio_timer_freq; 


int    abuf_size;
int    abuf_ptr;
int    abuf_inc;

int		audio_vol[AUDIO_NUM_VOICES];
int		audio_dvol[AUDIO_NUM_VOICES];
int		audio_freq[AUDIO_NUM_VOICES];
int		audio_dfreq[AUDIO_NUM_VOICES];
int		audio_len[AUDIO_NUM_VOICES];
byte 		*audio_data[AUDIO_NUM_VOICES];
int		audio_on[AUDIO_NUM_VOICES];

struct sigaction	sig_action;	/* used for arm alarm signal */

/* system dependent functions */
int sysdep_init(void);
void sysdep_exist(void);
void sysdep_poll_joystick(void);
int sysdep_play_audio(byte *buf, int size);
void sysdep_fill_audio_buffer(long *in,char *out, int start,int end);

void audio_timer (int arg) {
        static int  in = FALSE;
        int   i, j;
        static byte buf[65536];
        static long  intbuf[65536];
        int   j1, j2;

        if (!in) {
                in = TRUE;
                j1 = abuf_ptr;  /* calc pointers for chunk of buffer */
                j2 = j1 + abuf_inc; /* to be filled this time around */
                for (i = 0; i < AUDIO_NUM_VOICES; i++) {
                    if (audio_on[i]) {
                        for (j = j1; j < j2; j++) {
                            if (i != 0) intbuf[j] += (audio_vol[i]/16) * 
                                        audio_data[i][((j * audio_freq[i]) / 
                                        audio_sample_freq) % audio_len[i]];
                            else        intbuf[j] = (audio_vol[0]/16) * 
                                        audio_data[0][((j * audio_freq[0]) / 
                                        audio_sample_freq) % audio_len[0]];
                            }
                        audio_dvol[i]  = FALSE;
                        audio_dfreq[i] = FALSE;
                    }
                }
		sysdep_fill_audio_buffer( intbuf,(char *)buf,j1,j2);
                if (j2 == abuf_size) {  /* buffer full - dump to audio */
			i=sysdep_play_audio(buf,abuf_size);
                        abuf_ptr = 0;    /* and reset */
                } else {
                        abuf_ptr = j2;   /* not full - write more next time */
                }
        in = FALSE;
        } /* else {
          fprintf(stderr, ".");
        } */
#if defined solaris || defined sgi
	/* rearm alarm timer */
	sigaction (SIGALRM, &sig_action, NULL);
#endif
}

/* 
* Included specific *ix dependent code 
* each file must be enclosed in a #ifdef -sysname- / #endif pair
*
* Must include the following functions
*
* int sysdep_init(void) 
* 	containning system initialization code
*
* void sysdep_exist(void)
* 	code to close & cleanup
*
* void sysdep_poll_joystick(void)
* 	to perform ( if any ) joystick polling
*
* int sysdep_play_audio(byte *buf, int size)
*       to extract audio samples from buffer and send to audio device
*
* Added by Juan Antonio Martinez <jantonio@dit.upm.es> 18-April-1997
*
*/
/* linux & freebsd specific resides in same file */

#include "linux.c"
#include "sunos.c"
#include "solaris.c"
#include "irix.c"

#ifndef AUDIO_TIMER_FREQ
/* should be defined in sysdep files, but.... */
#define AUDIO_TIMER_FREQ	(56)
#endif


/*
 * Put anything here you need to do when the program is started.
 * Return 0 if initialization was successful, nonzero otherwise.
 */

int osd_init (int argc, char *argv[])
{

	int			i;
	/* initialize some variables to defalut values */
	use_joystick		= TRUE;
	play_sound 		= FALSE;
	scaling			= FALSE;
        osd_joy_up = osd_joy_down = osd_joy_left = osd_joy_right = 0 ;
	osd_joy_b1 = osd_joy_b2 = osd_joy_b3 = osd_joy_b4 = 0;

	/* parse argument invocation */
	for (i = 1;i < argc;i++) {
		if (stricmp(argv[i], "-sound") == 0) {
			play_sound        = TRUE;
			audio_sample_freq = AUDIO_SAMPLE_FREQ;
			audio_timer_freq  = AUDIO_TIMER_FREQ;
		}
		if (stricmp(argv[i], "-nojoy") == 0) use_joystick = FALSE;
		if (stricmp(argv[i], "-widthscale") == 0) {
			widthscale = atoi (argv[i+1]);
			scaling = TRUE;
			if (widthscale <= 0) {
				printf ("illegal widthscale (%d)\n", widthscale);
				exit (1);
			}
		}
		if (stricmp(argv[i], "-heightscale") == 0) {
			heightscale = atoi (argv[i+1]);
			scaling = TRUE;
			if (heightscale <= 0) {
				printf ("illegal heightscale (%d)\n", heightscale);
				exit (1);
			}
		}
	}
	/* now invoice unix-dependent initialization */
	sysdep_init();

	first_free_pen = 0;
	/* Initialize key array - no keys are pressed. */

	for (i = 0; i < OSD_NUMKEYS; i++) {
		key[i] = FALSE;
	}

	return (OSD_OK);
}


/*
 * Cleanup routines to be executed when the program is terminated.
 */

void osd_exit (void)
{
	/* actually no global options: invoice directly unix-dep routines */
	sysdep_exit();
}


/*
 * Make a snapshot of the screen. Not implemented yet.
 * Current bug : because the path is set to the rom directory, PCX files
 * will be saved there.
 */

int osd_snapshot(void)
{
	return (OSD_OK);
}

struct osd_bitmap *osd_create_bitmap (int width, int height)
{
  struct osd_bitmap  	*bitmap;
  int 			i;
  unsigned char  	*bm;

#ifndef DR1
  if ((bitmap = malloc (sizeof (struct osd_bitmap) +
		(height-1) * sizeof (byte *))) != NULL)
#else
  if ((bitmap = malloc (sizeof (struct osd_bitmap) +
		(height) * sizeof (byte *))) != NULL)
#endif
  {
    bitmap->width = width;
    bitmap->height = height;

    if ((bm = malloc (width * height * sizeof(unsigned char))) == NULL)
    {
      free (bitmap);
      return (NULL);
    }

    for (i = 0;i < height;i++)
		{
      			bitmap->line[i] = &bm[i * width];
		}

		bitmap->private = bm;
	}

	return (bitmap);
}

void osd_free_bitmap (struct osd_bitmap *bitmap)
{
	if (bitmap)
	{
		free (bitmap->private);
		free (bitmap);
		bitmap = NULL;
	}
}

/*
 * Create a display screen, or window, large enough to accomodate a bitmap
 * of the given dimensions. I don't do any test here (640x480 will just do
 * for now) but one could e.g. open a window of the exact dimensions
 * provided. Return 0 if the display was set up successfully, nonzero
 * otherwise.
 *
 * Added: let osd_create_display allocate the actual display buffer. This
 * seems a bit dirty, but is more or less essential for X implementations
 * to avoid a lengthy memcpy().
 */

struct osd_bitmap *osd_create_display (int width, int height)
{
	Screen	 	*screen;
	XEvent		 event;
  XSizeHints 	 hints;
  XWMHints 	 wm_hints;
	int		 i;

	/* Allocate the bitmap and set the image width and height. */

	if ((bitmap = malloc (sizeof (struct osd_bitmap) +
												(height-1) * sizeof (unsigned char *))) == NULL)
	{
		return (NULL);
	}

	bitmap->width 	= width;
	bitmap->height =	height;

	/* Open the display. */

  display = XOpenDisplay (NULL);
  if(!display)
  {
  	printf ("OSD ERROR: failed to open the display.\n");
  	return (NULL);
  }

  screen 		= DefaultScreenOfDisplay (display);
  White			= WhitePixelOfScreen (screen);
  Black			= BlackPixelOfScreen (screen);

  colormap	= DefaultColormapOfScreen (screen);
  gc				= DefaultGCOfScreen (screen);

	/* Create the window. No buttons, no fancy stuff. */

#ifdef CS1
  if (scaling) 
	{
    window = XCreateSimpleWindow (display, RootWindowOfScreen (screen), 0, 0,
  														    bitmap->width*widthscale, bitmap->height*heightscale, 
																  0, White, Black);
  }
  else 
	{
    window = XCreateSimpleWindow (display, RootWindowOfScreen (screen), 0, 0,
    bitmap->width, bitmap->height, 0, White, Black);
  }
#else                                    
  window = XCreateSimpleWindow (display, RootWindowOfScreen (screen), 0, 0,
  										  			 	bitmap->width, bitmap->height, 0, White, Black);
#endif

  if (!window)
  {
  	printf ("OSD ERROR: failed to open a window.\n");
  	return (NULL);
  }

	/*  Placement hints etc. */

  hints.flags		= PSize | PMinSize | PMaxSize;
#ifdef CS1
	if (scaling)
	{			
 	 	hints.min_width		= hints.max_width 	= hints.base_width	= 
 	 		bitmap->width * widthscale;
 	 	hints.min_height	= hints.max_height 	= hints.base_height	=	
 	 		bitmap->height * heightscale;
 	}
 	else
 	{
	  hints.min_width		= hints.max_width=hints.base_width= bitmap->width;
 	 	hints.min_height	= hints.max_height=hints.base_height=bitmap->height;
	}
#else
  hints.min_width		= hints.max_width		= hints.base_width	= bitmap->width;
  hints.min_height	= hints.max_height	= hints.base_height	=	bitmap->height;
#endif

  wm_hints.input	= TRUE;
  wm_hints.flags	= InputHint;

  XSetWMHints 		(display, window, &wm_hints);
  XSetWMNormalHints 	(display, window, &hints);
  XStoreName 		(display, window, title);

	/* Map and expose the window. */

  XSelectInput   (display, window, FocusChangeMask | ExposureMask | KeyPressMask | KeyReleaseMask);
  XMapRaised     (display, window);
  XClearWindow   (display, window);
  XAutoRepeatOff (display);
  XWindowEvent   (display, window, ExposureMask, &event);

#ifdef MITSHM

	/* Create a MITSHM image. */

#ifdef CS1
  /* CS: scaling changes requested image size */

  if (scaling) 
	{
    image= XShmCreateImage (display, DefaultVisualOfScreen (screen), 8,
       									    ZPixmap, NULL, &shm_info, bitmap->width*widthscale, 
														bitmap->height*heightscale);
  }
  else 
	{
    image= XShmCreateImage (display, DefaultVisualOfScreen (screen), 8,
   									        ZPixmap, NULL, &shm_info, bitmap->width, bitmap->height);
  }
#else
  image= XShmCreateImage (display, DefaultVisualOfScreen (screen), 8,
  		  									ZPixmap, NULL, &shm_info, bitmap->width, bitmap->height);
#endif

  if (!image)
  {
  	printf ("OSD ERROR: failed to create a MITSHM image.\n");
  	return (NULL);
  }

  shm_info.shmid = shmget (IPC_PRIVATE, image->bytes_per_line * image->height,
  			 (IPC_CREAT | 0777));

  if (shm_info.shmid < 0)
  {
  	printf ("OSD ERROR: failed to create MITSHM block.\n");
  	return (NULL);
  }

	/* And allocate the bitmap buffer. */

#ifdef CS1
	/* CS: scaling uses 2 buffers for transparency with mame */

	if (scaling) 
	{
  	scaled_buffer_ptr = (byte *)(image->data = shm_info.shmaddr =
                				(byte *) shmat ( shm_info.shmid,0 ,0));
  	buffer_ptr = (byte *) malloc (sizeof(byte) * bitmap->width * bitmap->height);
  }
 	else 
	{
    buffer_ptr = (byte *)(image->data = shm_info.shmaddr =
         				 (byte *) shmat (shm_info.shmid, 0, 0));
  }

	/* CS: added extra check here */

  if (!buffer_ptr || (scaling && !scaled_buffer_ptr))
  {
    printf ("OSD ERROR: failed to allocate MITSHM bitmap buffer.\n");
    return (NULL);
  }
#else
  buffer_ptr = (byte *)(image->data = shm_info.shmaddr =
			 shmat (shm_info.shmid, 0, 0));

  if (!buffer_ptr)
  {
  	printf ("OSD ERROR: failed to allocate MITSHM bitmap buffer.\n");
  	return (NULL);
  }
#endif

  shm_info.readOnly = FALSE;

	/* Attach the MITSHM block. */

  if(!XShmAttach (display, &shm_info))
  {
  	printf ("OSD ERROR: failed to attach MITSHM block.\n");
  	return (NULL);
  }

#else

	/* Allocate a bitmap buffer. */

  buffer_ptr = (byte *) malloc (sizeof(byte) * bitmap->width * bitmap->height);

#ifdef CS1
  /* CS: allocated scaled image buffer - ugly, but leave all other code untouched. */

	if (scaling)
	{
  	scaled_buffer_ptr = (byte *) malloc (sizeof(byte) * bitmap->width *
 																				 bitmap->height * widthscale * heightscale);
	}
#endif

#ifdef CS1
  /* CS: scaling hack extends this check */

  if (!buffer_ptr || (scaling && !scaled_buffer_ptr))
  {
    printf ("OSD ERROR: failed to allocate bitmap buffer.\n");
    return (NULL);
  }
#else
  if (!buffer_ptr)
  {
  	printf ("OSD ERROR: failed to allocate bitmap buffer.\n");
  	return (NULL);
  }
#endif

	/* Create the image. It's a ZPixmap, which means that the buffer will
	 * contain a byte for each pixel. This also means that it will only work
	 * on PseudoColor visuals (i.e. 8-bit/256-color displays).
	 */

#ifdef CD1
 	/* CS: Ximage is allocated against our new image if scaling active */

 	if (scaling) 
	{
  	image = XCreateImage (display, DefaultVisualOfScreen (screen), 8,
          							  ZPixmap, 0, (char *) scaled_buffer_ptr,
            							bitmap->width*widthscale, bitmap->height*heightscale, 8, 0);
  }
 	else 
	{
  	image = XCreateImage (display, DefaultVisualOfScreen (screen), 8,
    							        ZPixmap, 0, (char *) buffer_ptr,
            							bitmap->width, bitmap->height, 8, 0);
  }
#else
  image = XCreateImage (display, DefaultVisualOfScreen (screen), 8,
						      			ZPixmap, 0, (char *) buffer_ptr,
      									bitmap->width, bitmap->height, 8, 0);
#endif

  if (!image)
  {
    printf ("OSD ERROR: could not create image.\n");
    return (NULL);
  }

#endif

	for (i = 0;i < height; i++)
	{
		bitmap->line[i] = &buffer_ptr[i * width];
	}

	bitmap->private = buffer_ptr;

  return (bitmap);
}

/*
 * Shut down the display.
 */

void osd_close_display (void)
{
	int		i;
	unsigned long	l;

  if (display && window)
  {
#ifdef MITSHM
			/* Detach the SHM. */

      XShmDetach (display, &shm_info);

      if (shm_info.shmaddr)
      {
      	shmdt (shm_info.shmaddr);
      }

      if (shm_info.shmid >= 0)
      {
      	shmctl (shm_info.shmid, IPC_RMID, 0);
      }
#else
	/* Memory will be deallocated shortly (== buffer_ptr). */
#endif

	/* Destroy the image and free the colors. */

    if (image)
    {
    	XDestroyImage (image);

#ifdef CS1
		  /* CS: scaling hack means scaled_buffer_ptr is auto-freed, so... */

   	  if (scaling)
			{
      	free(buffer_ptr);
			}
#endif
    }

    for (i = 0; i < 16; i++)
    {
      if (xpixel[i] != Black)
      {
      	l = (unsigned long) xpixel[i];
      	XFreeColors (display, colormap, &l, 1, 0);
      }

    	XAutoRepeatOn(display);
  	}

  	if (display)
  	{
  		XAutoRepeatOn (display);
  		XCloseDisplay (display);
  	}
	}

	if (xpixel)
	{
		free (xpixel);
		xpixel = NULL;
		first_free_pen = 0;
	}

/*
	if (bitmap)
	{
		if (bitmap->private)
		{
			free (bitmap->private);
		}
		free (bitmap);
		bitmap = NULL;
	}
*/
}

/*
 * Set the screen colors using the given palette.
 *
 * This function now also sets the palette map. The reason for this is that
 * in X we cannot tell in advance which pixel value (index) will be assigned
 * to which color. To avoid translating the entire bitmap each time we update
 * the screen, we'll have to use the color indices supplied by X directly.
 * This can only be done if we let this function update the color values used
 * in the calling program.... (hack).
 */

int osd_obtain_pen (byte red, byte green, byte blue)
{
	int				i;
	XColor		color;

	/* First time ? */
	if (xpixel == NULL)
	{
		if ((xpixel = (unsigned long *) malloc (sizeof (unsigned long) * OSD_NUMPENS)) == NULL)
		{
			printf ("OSD ERROR: failed to allocate pixel index table.\n");
			return (OSD_NOT_OK);
		}

		for (i = 0; i < OSD_NUMPENS; i++)
		{
			xpixel[i] = Black;
		}
	}

  /* Translate VGA 0-63 values to X 0-65536 values. */

  color.flags	= (DoRed | DoGreen | DoBlue);
  color.red	= 256 * (int) red;
  color.green	= 256 * (int) green;
  color.blue	= 256 * (int) blue;

  if (XAllocColor (display, colormap, &color))
  {
    xpixel[first_free_pen] = color.pixel;
  }

	first_free_pen++;

	return (color.pixel);
}

/*
 * Since *we* control the bitmap buffer and a pointer to the data is stored
 * in 'image', all we need to do is a XPutImage.
 *
 * For compatibility, we accept a parameter 'bitmap' (not used).
 */

void osd_update_display (void)
{
#ifdef CS1
  /* CS: scaling - here's the CPU burner.  Buy a faster computer
         sez Micro$oft. Ignore the ugly framerate stuff.*/

  /* Lame framerate code for optimization anaylsis
  	static int count = 0;

  	printf("%d ",count++);
  	fflush(stdout); 
	*/
	
  if (scaling) 
	{          
		/* IF SCALING ACTIVE */
    int i, j, k, l, linechanged;

    for (i = 0; i < bitmap->height; i++) 
		{
    	linechanged = 0;
      for (j = 0; j < bitmap->width; j++) 
  		{
      	k = (i * heightscale) * bitmap->width * widthscale + j * widthscale;

        if (scaled_buffer_ptr[k] == buffer_ptr[i*bitmap->width+j])
  			{
          continue;
        }
  		  linechanged = 1;

        for (l = 0; l < widthscale; l++)
  			{  
  				/* fill width fills */
          scaled_buffer_ptr[k++] = buffer_ptr[i*bitmap->width+j];
  			}
      }
      if (linechanged) 
  		{
        for (l =0; l < heightscale; l++) 
        {
          /* fill extra rows */

  		    memcpy(scaled_buffer_ptr+(i*heightscale+l)*(bitmap->width*widthscale),
         		     scaled_buffer_ptr+(i*heightscale)*(bitmap->width*widthscale),
            		  bitmap->width*widthscale);
        }
      }
		}

#ifdef MITSHM
	  XShmPutImage (display, window, gc, image, 0, 0, 0, 0,
  					      bitmap->width*widthscale, bitmap->height*heightscale, FALSE);
#else
	  XPutImage (display, window, gc, image, 0, 0, 0, 0,
    			     bitmap->width*widthscale, bitmap->height*heightscale);
#endif

  }
	else 
	{            
		/* NO SCALING ACTIVE - JUST PUTIMAGE */

#ifdef MITSHM
 	 	XShmPutImage (display, window, gc, image, 0, 0, 0, 0,
  				        bitmap->width, bitmap->height, FALSE);
#else
	 	XPutImage (display, window, gc, image, 0, 0, 0, 0,
   				     bitmap->width, bitmap->height);
#endif

  }
#else

#ifdef MITSHM

  XShmPutImage (display, window, gc, image, 0, 0, 0, 0,
   		bitmap->width, bitmap->height, FALSE);

#else

  XPutImage (display, window, gc, image, 0, 0, 0, 0,
  		 bitmap->width, bitmap->height);

#endif

#endif

  XFlush (display);
}

/*
 * Wait until player wants to resume game.
 */

/* Key handling was way to fast using this routine */
int slowdown = 30;

int osd_read_key (void)
{
	int	i;

	i = OSD_NUMKEYS;

	while (i == OSD_NUMKEYS)
	{
		osd_key_pressed (-1);

		i = 0;
		while (!key[i++] && (i < OSD_NUMKEYS));
	}
        if (slowdown == 0) {
          slowdown = 30;
	  return (i-1);
        }
	else {
	  slowdown--;
	  return (0);
	}

}

/*
 * Check if a key is pressed. The keycode is the standard PC keyboard code,
 * as defined in osdepend.h.
 *
 * Handles all incoming keypress/keyrelease events and updates the 'key'
 * array accordingly.
 *
 * This poses some problems, since keycodes returned from X aren't quite the
 * same as normal PC keyboard codes. For now, we only catch the keys we're
 * interested in in a switch-statement. Could be improved so that programs
 * don't expect osdepend.c to map keys to a certain value.
 *
 * Return 0 if the key is not pressed, nonzero otherwise.
 *
 */

int osd_key_pressed (int request)
{
  XEvent 		E;
  int	 		keycode;
  int			mask;

  while (XCheckWindowEvent (display,window, KeyPressMask | KeyReleaseMask, &E))
  {
  	mask = TRUE; 	/* In case of KeyPress. */

    if ((E.type == KeyPress) || (E.type == KeyRelease))
    {
			if (E.type == KeyRelease)
			{
				mask = FALSE;
			}

	    keycode = XLookupKeysym ((XKeyEvent *) &E, 0);

      switch (keycode)
      {
        case XK_Escape:
        {
        	key[OSD_KEY_ESC] = mask;
        	break;
        }
        case XK_Down:
        {
        	key[OSD_KEY_DOWN] = mask;
       		break;
       	}
       	case XK_Up:
       	{
       		key[OSD_KEY_UP] = mask;
       		break;
       	}
       	case XK_Left:
       	{
       		key[OSD_KEY_LEFT] = mask;
       		break;
       	}
       	case XK_Right:
       	{
       		key[OSD_KEY_RIGHT] = mask;
       		break;
       	}
       	case XK_Control_L:
       	case XK_Control_R:
       	{
       		key[OSD_KEY_CONTROL] = mask;
       		break;
       	}
       	case XK_Tab:
       	{
       		key[OSD_KEY_TAB] = mask;
       		break;
       	}
      	case XK_F1:
      	{
      		key[OSD_KEY_F1] = mask;
      		break;
      	}
      	case XK_F2:
      	{
      		key[OSD_KEY_F2] = mask;
      		break;
      	}
      	case XK_F3:
      	{
       		key[OSD_KEY_F3] = mask;
      		break;
      	}
      	case XK_F4:
      	{
      		key[OSD_KEY_F4] = mask;
      		break;
      	}
      	case XK_F5:
      	{
      		key[OSD_KEY_F5] = mask;
      		break;
      	}
      	case XK_F6:
      	{
      		key[OSD_KEY_F6] = mask;
      		break;
      	}
      	case XK_F7:
      	{
      		key[OSD_KEY_F7] = mask;
      		break;
      	}
      	case XK_F8:
      	{
      		key[OSD_KEY_F8] = mask;
      		break;
      	}
      	case XK_F9:
      	{
      		key[OSD_KEY_F9] = mask;
      		break;
      	}
        case XK_F10:
        {
        	key[OSD_KEY_F10] = mask;
        	break;
        }
        case XK_F11:
        {
        	key[OSD_KEY_F11] = mask;
        	break;
        }
        case XK_F12:
        {
        	key[OSD_KEY_F12] = mask;
        	break;
        }
      	case XK_0:
      	{
      		key[OSD_KEY_0] = mask;
      		break;
      	}
      	case XK_1:
      	{
      		key[OSD_KEY_1] = mask;
      		break;
      	}
      	case XK_2:
      	{
      		key[OSD_KEY_2] = mask;
      		break;
      	}
      	case XK_3:
      	{
      		key[OSD_KEY_3] = mask;
      		break;
      	}
      	case XK_4:
      	{
      		key[OSD_KEY_4] = mask;
      		break;
      	}
      	case XK_5:
      	{
      		key[OSD_KEY_5] = mask;
      		break;
      	}
      	case XK_6:
      	{
      		key[OSD_KEY_6] = mask;
      		break;
      	}
      	case XK_7:
      	{
      		key[OSD_KEY_7] = mask;
      		break;
      	}
      	case XK_8:
      	{
      		key[OSD_KEY_8] = mask;
      		break;
      	}
      	case XK_9:
      	{
      		key[OSD_KEY_9] = mask;
      		break;
      	}

/* Mike Zimmerman noticed this one */
/* Pause now works */
	case XK_p:
	case XK_P:
	{
		key[OSD_KEY_P]=mask;
		break;
	}
/* For Crazy Climber - Jack Patton (jpatton@intserv.com)*/
/* Based on this stuff from ../drivers/cclimber.c */
/* Left stick: OSD_KEY_E, OSD_KEY_D, OSD_KEY_S, OSD_KEY_F, */
/* Right stick: OSD_KEY_I, OSD_KEY_K, OSD_KEY_J, OSD_KEY_L */
/* and values from the keycode variable */

      	case 101:
      	{
      		key[OSD_KEY_E] = mask;
      		break;
      	}
      	case 100:
      	{
      		key[OSD_KEY_D] = mask;
      		break;
      	}
      	case 115:
      	{
      		key[OSD_KEY_S] = mask;
      		break;
      	}
      	case 102:
      	{
      		key[OSD_KEY_F] = mask;
      		break;
      	}
      	case 105:
      	{
      		key[OSD_KEY_I] = mask;
      		break;
      	}
      	case 106:
      	{
      		key[OSD_KEY_J] = mask;
      		break;
      	}
      	case 107:
      	{
      		key[OSD_KEY_K] = mask;
      		break;
      	}
      	case 108:
      	{
      		key[OSD_KEY_L] = mask;
      		break;
      	}
/* Scramble and Space Cobra bomb key OSD_KEY_ALT*/
/* from one of the keysym includes */
/* Jack Patton (jpatton@intserv.com) */
      	case XK_Alt_L:
      	{
      		key[OSD_KEY_ALT] = mask;
      		break;
      	}
      	case XK_Alt_R:
      	{
      		key[OSD_KEY_ALT] = mask;
      		break;
      	}
				default:
				{
				}
      }
		}
	}

#ifndef DR2
		return key[request];
#else
	if (request >= 0)
		return key[request];
	else
		return (FALSE);
#endif
}

void osd_poll_joystick (void)
{
	if (use_joystick) sysdep_poll_joystick();
}

int osd_joy_pressed (int joycode)
{
	/* if joystick not implemented , all variables set to zero */
	switch (joycode)
	{
		case OSD_JOY_LEFT:
			return osd_joy_left;
			break;
		case OSD_JOY_RIGHT:
			return osd_joy_right;
			break;
		case OSD_JOY_UP:
			return osd_joy_up;
			break;
		case OSD_JOY_DOWN:
			return osd_joy_down;
			break;
		case OSD_JOY_FIRE1:
			return osd_joy_b1;
			break;
		case OSD_JOY_FIRE2:
			return osd_joy_b2;
			break;
		case OSD_JOY_FIRE3:
			return osd_joy_b3;
			break;
		case OSD_JOY_FIRE4:
			return osd_joy_b4;
			break;
		case OSD_JOY_FIRE:
			return (osd_joy_b1 || osd_joy_b2 || osd_joy_b3 || osd_joy_b4);
			break;
		default:
			return FALSE;
			break;
	}
}

/*
 * Audio shit.
 */

void osd_update_audio (void)
{
	/* Not used. */
}

void osd_play_sample (int channel, byte *data, int len, int freq,
			int volume, int loop)
{
	if (!play_sound || (channel >= AUDIO_NUM_VOICES))
	{
		return;
 	}

	audio_on[channel]	= TRUE;
	audio_data[channel] 	= data;
	audio_len[channel]  	= len;
	audio_freq[channel]	= freq;
	audio_vol[channel]  	= volume;
}

void osd_play_streamed_sample (int channel, byte *data, int len, int freq, int volume)
{
	/* Not used. */
}

void osd_adjust_sample (int channel, int freq, int volume)
{
	if (play_sound && (channel < AUDIO_NUM_VOICES))
	{
		audio_dfreq[channel] |= (freq    != audio_freq[channel]);
		audio_dvol[channel]  |= (volume  != audio_vol[channel]);
		audio_freq[channel] = freq;
		audio_vol[channel]  = volume;
	}
}

void osd_stop_sample (int channel)
{
	if (play_sound && (channel < AUDIO_NUM_VOICES))
	{
		audio_on[channel] = FALSE;
	}
}
