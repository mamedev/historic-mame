/*
 * Linux Dependent system file 
*/

#ifdef linux

#define __LINUX_C

#include "xmame.h"

#ifdef JOYSTICK

#include <linux/joystick.h>

struct 	 JS_DATA_TYPE joy_data,joy_orig;
int	 joy_fd; /* joystick file descriptor */

#endif

/*
 * Put anything here you need to do when the program is started.
 * Return 0 if initialization was successful, nonzero otherwise.
 */

int sysdep_init ()
{
	int			i;
  	int                     tmps;
	int			dspfreq, dspbyte, dspstereo;
	/**********  initialize joystick device */

#ifdef JOYSTICK
    	if (use_joystick) {
		printf ("Linux Joystick interface initialization...\n");
		if ((joy_fd = open ("/dev/js0", O_RDONLY | O_NONBLOCK)) < 0) {
		  printf ("Couldn't open joystick device /dev/js0 \n");
		  printf ("Be sure to joystick module is instaled\n");
		  use_joystick = FALSE;
		}
	else if ( read(joy_fd,&joy_orig,sizeof(struct JS_DATA_TYPE)) < 0) {
	  	 printf ("Cannot read joystic device.sorry\n");
	 	 exit(1);
		}
		/* mask x e y pos to skip "flickering" */
		joy_orig.x >>=5;
		joy_orig.y >>=5;

    	}
#endif

	/********* * initialize sound */
    	if (play_sound) {
	  	dspbyte 	= AUDIO_SAMPLE_BITS;
 		dspfreq 	= audio_sample_freq;
		dspstereo	= 0;

	    	printf ("Linux sound device initialization...\n");

    		if ((audio_fd = open ("/dev/dsp", O_WRONLY | O_NONBLOCK)) < 0)
    		{
     	 		printf ("couldn't open audio device\n");
    	  		exit(1);
   	 	}

   	 	abuf_size = 0x0004000b;
    		(void)ioctl(audio_fd, SNDCTL_DSP_SETFRAGMENT, &abuf_size);

    		tmps = dspbyte;
    		ioctl (audio_fd, SNDCTL_DSP_SAMPLESIZE, &dspbyte);
    		if (tmps != dspbyte) {
     	 		printf ("can't set DSP number of bits\n");
      			exit (1);
    		}
  	 	else {
 	     		printf ("DSP set to %d bits\n", dspbyte);
	    	}

	    	if (ioctl (audio_fd, SNDCTL_DSP_STEREO, &dspstereo) == -1) {
	    		printf ("can't set DSP to mono\n");
	    		exit (1);
	    	}
		else {
			printf ("DSP set to %s\n", dspstereo ? "stereo":"mono");
		}

	    	if (ioctl (audio_fd, SNDCTL_DSP_SPEED, &dspfreq)) {
	      		printf ("can't set DSP frequency\n");
	      		exit(1);
	    	}
		else {
			printf ("DSP set to %d Hz\n", dspfreq);
		}
		ioctl(audio_fd, SNDCTL_DSP_GETBLKSIZE, &abuf_size);
		if (abuf_size < 4 || abuf_size > 65536) {
			if (abuf_size == -1)	perror ("abuf_size");
		 	else		fprintf (stderr, "Invalid audio buffers size %d\n", abuf_size);
			exit (-1);
		}
		abuf_ptr = 0;
		abuf_inc = abuf_size / 4;
		printf("DSP buffer is %d bytes\n", abuf_size);

		for (i = 0; i < AUDIO_NUM_VOICES; i++) audio_on[i] = FALSE;
	
	}
	return (TRUE);
}


/*
 * Cleanup routines to be executed when the program is terminated.
 */

void sysdep_exit (void) {
	if (play_sound)		close (audio_fd);
#ifdef JOYSTICK
  	if (use_joystick)	close (joy_fd);
#endif
}

/* define abs(x) ( ( (x) >= 0 )? (x) : -(x) ) */
inline int abs(int x) {if (x>=0) return (x); return -(x); }

void sysdep_poll_joystick (void)
{
#ifdef JOYSTICK
	int res;
  int xgreater;

	res = read(joy_fd,&joy_data,sizeof(struct JS_DATA_TYPE) );
	if (res == sizeof(struct JS_DATA_TYPE )) {
		/* get value of buttons */
		osd_joy_b1 = joy_data.buttons & 0x01;	
		osd_joy_b2 = joy_data.buttons & 0x02;	
		osd_joy_b3 = joy_data.buttons & 0x04;	
		osd_joy_b4 = joy_data.buttons & 0x08;	
	        /* now parse movements */
		joy_data.x >>=5;
		joy_data.y >>=5;
		/* simulate a four button joystick ( a little tricky ) */
		/* patched to allow simultaneous x-y shifts 2-May-1997 */
		if ( abs(joy_data.x-joy_orig.x) == abs(joy_data.y-joy_orig.y) ){
		    osd_joy_left  = (joy_data.x < joy_orig.x) ;
		    osd_joy_right = (joy_data.x > joy_orig.x) ;
		    osd_joy_up    = (joy_data.y < joy_orig.y) ;
		    osd_joy_down  = (joy_data.y > joy_orig.y) ;
		} else {
		    xgreater = abs(joy_data.x-joy_orig.x) > abs(joy_data.y-joy_orig.y) ;
		    osd_joy_left  = (joy_data.x < joy_orig.x) && ( xgreater );
		    osd_joy_right = (joy_data.x > joy_orig.x) && ( xgreater );
		    osd_joy_up    = (joy_data.y < joy_orig.y) && ( ! xgreater );
		    osd_joy_down  = (joy_data.y > joy_orig.y) && ( ! xgreater );
		}
	}
#endif
}

int sysdep_play_audio( byte *buf, int bufsize) {
	return write(audio_fd,buf,bufsize);
}

void sysdep_fill_audio_buffer( long *in, char *out, int start,int end ) {
	for(; start<end; start++) out[start]=(char)( in[start] >> 7) ; 
}

#endif 
/****** end of Linux dependent code; DO NOT WRITE BELOW THIS LINE *********/
