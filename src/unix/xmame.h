/*
 ******************* X-Mame header file *********************
 * file "xmame.h"
 *
 * by jantonio@dit.upm.es
 *
 ************************************************************
*/

#ifndef __XMAME_H_
#define __XMAME_H_

#ifdef __MAIN_C_
#define EXTERN
#else
#define EXTERN extern
#endif

/*
 * Include files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <ctype.h>
#include <pwd.h>

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

#ifdef MITSHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif

#include "osdepend.h"

/*
 * Definitions.
 */

typedef unsigned char		byte;

#define OSD_NUMKEYS		(100)
#define OSD_NUMPENS		(256)

#define	TRUE			(1)
#define FALSE			(0)

#define OSD_OK			(0)
#define OSD_NOT_OK		(1)

#define DEBUG(x)


/*
 * Global variables.
 */

#ifdef MITSHM
EXTERN XShmSegmentInfo 	shm_info;
#endif
EXTERN Display 		*display;
EXTERN Window 		 window;
EXTERN Colormap 	 colormap;
EXTERN XImage 		*image;
EXTERN GC 		 gc;
EXTERN int 		 mit_shm_avail; /* use mitshm if available */
EXTERN char 		*displayname; /* default is use local */
EXTERN char		*mamedir;     /* home directory of mame tree */

/* CS: scaling hack - integer factors only */

EXTERN unsigned int 	 widthscale;	 	/* X scale */
EXTERN unsigned int 	 heightscale;		/* Y SCALE */
#define scaling		( (heightscale + widthscale) > 2 ) 
EXTERN byte  		*scaled_buffer_ptr ;
			
EXTERN  unsigned long 	*xpixel; /* Pixel index values */
EXTERN  byte		*buffer_ptr;
EXTERN  int 		 snapshot_no;

EXTERN struct osd_bitmap	*bitmap;
EXTERN int			 use_joystick;
EXTERN int			 play_sound;
EXTERN int			 video_sync;
EXTERN int	 		 first_free_pen;
EXTERN int			 use_private_cmap;

EXTERN int      osd_joy_up, osd_joy_down, osd_joy_left, osd_joy_right;
EXTERN int 	osd_joy_b1, osd_joy_b2, osd_joy_b3, osd_joy_b4;

/*
 * Workaround: in DO$, the Allegro library maintains this array.
 * In X, we'll have to do it ourselves. Bummer.
 *
 * Note: this can be speeded up considerably (see the enormous switch
 * statement in osd_key_pressed).
 */

EXTERN byte			 key[OSD_NUMKEYS];

/*
 * Audio variables.
 */

#define AUDIO_SAMPLE_FREQ	(22050)
#define AUDIO_SAMPLE_LOWFREQ	(8000)
#define AUDIO_SAMPLE_BITS	(8)
#define AUDIO_NUM_BUFS		(0)
#define AUDIO_BUFSIZE_BITS	(8)
#define AUDIO_MIX_BUFSIZE	(128)
#define AUDIO_NUM_VOICES	(8)
#define AUDIO_TIMER_FREQ	(56)

EXTERN	int	   	audio_fd;

EXTERN	int	   	audio_sample_freq; 
EXTERN	int	   	audio_timer_freq; 

EXTERN	int  		abuf_size;
EXTERN	int		abuf_ptr;
EXTERN	int    		abuf_inc;

EXTERN	int		audio_vol[AUDIO_NUM_VOICES];
EXTERN	int		audio_dvol[AUDIO_NUM_VOICES];
EXTERN	int		audio_freq[AUDIO_NUM_VOICES];
EXTERN	int		audio_dfreq[AUDIO_NUM_VOICES];
EXTERN	int		audio_len[AUDIO_NUM_VOICES];
EXTERN	byte 		*audio_data[AUDIO_NUM_VOICES];
EXTERN	int		audio_on[AUDIO_NUM_VOICES];

EXTERN  struct sigaction sig_action;	/* used for arm alarm signal */

/* system dependent functions */

int sysdep_init(void);
void sysdep_exit(void);
void sysdep_poll_joystick(void);
int sysdep_play_audio(byte *buf, int size);
void sysdep_fill_audio_buffer(long *in,char *out, int start,int end);
int start_timer();
int sysdep_mapkey(int from, int to);
int sysdep_keyboard_init(void);
int sysdep_audio_initvars(void);
int sysdep_joy_initvars(void);

#endif

