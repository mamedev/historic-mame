/*
* Solaris dependent code
*
* Preliminary audio support by jantonio@dit.upm.es
* no control on mono/stereo output channel and so  (yet)
*
*/

#ifdef solaris

#define __SOLARIS_C
#include "xmame.h"
#include "lin2ulaw.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stropts.h>
#include <sys/audioio.h>

int audioctl_fd; 		/* audio control device for solaris */

audio_info_t    	a_info; /* info about audio settings */
audio_device_t		a_dev;  /* info about audio hardware */

int sysdep_init(void) {

	int 			i;
	if (use_joystick) {
		printf("Joystick is no still supported under Solaris. Sorry\n");
		use_joystick = FALSE;
	}
	if (play_sound) {
		int supported=FALSE;
                printf ("Solaris sound device initialization...\n");
                /* try to open audio device */
                if ( (audio_fd=open("/dev/audio",O_WRONLY | O_NDELAY)) < 0)
                {
                        printf ("couldn't open audio device\n");
                        exit(1);
                }
                /* try to open audioctl device */
                if ( (audioctl_fd=open("/dev/audioctl",O_RDWR | O_NDELAY)) < 0)
                {
                        printf ("couldn't open audioctl device\n");
			close(audio_fd);
                        exit(1);
                }
	       	
		/* empty buffers before change config */
        	ioctl(audio_fd, AUDIO_DRAIN, 0);    /* drain everything out */
        	ioctl(audio_fd, I_FLUSH, FLUSHRW);  /* flush everything     */
        	ioctl(audioctl_fd, I_FLUSH, FLUSHRW);   /* flush everything */            

		/* identify audio device. if AMD chipset, bad, luck :-( */
		if(ioctl(audio_fd,AUDIO_GETDEV,&a_dev)<0) {
			printf("Cannot get sound device type\n");
			close(audio_fd);
			close(audioctl_fd);
			exit(1);
		}
		printf("Sound device is a %s %s version %s\n",a_dev.config,a_dev.name,a_dev.version);
		
                /* get audio parameters. */ 
                ioctl(audioctl_fd,AUDIO_GETINFO,&a_info);
                AUDIO_INITINFO(&a_info);

		if (strcmp(a_dev.name,"SUNW,dbri")== 0) {
			supported=TRUE;
			audio_sample_freq=AUDIO_SAMPLE_LOWFREQ;
			/* Sun Dbri doesn't support 8bits linear */
#if 0
                	a_info.play.sample_rate   = (uint_t) audio_sample_freq;
                	a_info.record.sample_rate = (uint_t) audio_sample_freq;
                	a_info.play.encoding      = (uint_t) AUDIO_ENCODING_ULAW;
                	a_info.record.encoding    = (uint_t) AUDIO_ENCODING_ULAW;
                	a_info.play.precision     = (uint_t) 16;
                	a_info.record.precision   = (uint_t) 16;
#endif
                	a_info.play.channels      = (uint_t) 1;	/* mono */
                	a_info.record.channels    = (uint_t) 1;
		} 
		if (strcmp(a_dev.name,"SUNW,CS4231")==0) {
			supported=TRUE;
                	a_info.play.sample_rate   = (uint_t) audio_sample_freq;
                	a_info.record.sample_rate = (uint_t) audio_sample_freq;
                	a_info.play.encoding      = (uint_t) AUDIO_ENCODING_LINEAR;
                	a_info.record.encoding    = (uint_t) AUDIO_ENCODING_LINEAR;
                	a_info.play.precision     = (uint_t) AUDIO_SAMPLE_BITS;
                	a_info.record.precision   = (uint_t) AUDIO_SAMPLE_BITS;
                	a_info.play.channels      = (uint_t) 1;   	/* mono */
                	a_info.record.channels    = (uint_t) 1;
		}
		if (strcmp(a_dev.name,"SUNW,am79c30")==0) {
			/* chipset AMD. preset to 8khz ulaw.... so pray */
			supported=TRUE;
			audio_sample_freq=AUDIO_SAMPLE_LOWFREQ;
#if 0
			/* ioctl fails when try to set ... */
       		         a_info.play.sample_rate   = (uint_t) audio_sample_freq;
       		         a_info.record.sample_rate = (uint_t) audio_sample_freq;
       		         a_info.play.encoding      = (uint_t) AUDIO_ENCODING_ULAW;
       		         a_info.record.encoding    = (uint_t) AUDIO_ENCODING_ULAW;
#endif
		}
		if (supported==FALSE) {
			printf("Unsupported audio chipset %s \n",a_dev.name);
			printf("Please contact with authors\n");
			close(audioctl_fd);
			close(audio_fd);
			exit(1);
		}

                a_info.play.port          = (uint_t) AUDIO_SPEAKER;
                a_info.play.gain          = (uint_t) 130; 	/* volumen */
		a_info.monitor_gain       = 0;  		/* no bypass */
		a_info.output_muted	  = 0; 			/* not muted */

		/* patches by djr@dit.upm.es */
		a_info.record.buffer_size = 2536;
		a_info.play.buffer_size = 2536;
		abuf_ptr  = 0;
		/* abuf_size = 0x0004000b; */
		abuf_size = 2536;
		abuf_inc  = abuf_size / 4;

		i=0;
		if (strcmp(a_dev.name,"SUNW,CS4231") == 0 )
                	if ( ioctl(audio_fd,AUDIO_SETINFO,&a_info) < 0 ) i+=1;
                if ( ioctl(audioctl_fd,AUDIO_SETINFO,&a_info) < 0 )	i+=2;
		if (i) { 
                        printf ("Error %d: cannot set audio device parameters\n",i);
                        close(audio_fd);
                        close(audioctl_fd);
                        exit(1);
                }
		/* set all audio channels off */
                for (i = 0; i < AUDIO_NUM_VOICES; i++) audio_on[i] = FALSE;
        }
        return (TRUE);
}

void sysdep_exit(void) {
	if (play_sound) {
		close(audio_fd);
		close(audioctl_fd);
	}
}	

void sysdep_poll_joystick() {
	/* !! no joystick available on Sparc :-( */
}

int sysdep_play_audio(byte *buf, int bufsize) {
        return write(audio_fd,buf,bufsize);
}

void sysdep_fill_audio_buffer( long *in, char *out, int start,int end ) {
	if (strcmp(a_dev.name,"SUNW,dbri")==0 ) {
		u16 *pt=(u16 *)out;
/*		for(; start<end; start++) *pt++=(u16)(in[start]); */
/*		for(; start<end; start++) *pt++=(u16)linearToUlaw[(u16)(in[start])]<<8;*/
		for(; start<end; start++) out[start]=(char)linearToUlaw[(u16)(in[start])];
		return;
	}
	if (strcmp(a_dev.name,"SUNW,am79c30")==0) {
		/* 8 bits ulaw at 8000hz */
		for(; start<end; start++) out[start]=(char)linearToUlaw[(u16)(in[start])];
		return;
	}
	if (strcmp(a_dev.name,"SUNW,CS4231")==0) {
		/* 8 bits linear 22050hz */
		for(; start<end; start++) out[start]=(char)( in[start] >> 7); 
		return;	
	}
}

#endif
