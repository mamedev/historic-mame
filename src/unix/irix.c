/*
* IRIX dependent code
*/
#ifdef sgi
#define __IRIX_C_
#include "xmame.h"

#include <sys/stropts.h>
#include <dmedia/audio.h>
#include <errno.h>

ALport devAudio;
ALconfig devAudioConfig;
	
int sysdep_init(void) {
	long parambuf[16];
	int i;
  
	if (use_joystick) {
	    printf("Joystick under IRIX is not still supported. Sorry\n");
	    use_joystick = FALSE;
	}

	if (play_sound) {

	    printf("IRIX sound system initializing ...");

	    /* first of all, look for anyone using driver */
	    parambuf[0] = AL_INPUT_COUNT;
	    parambuf[2] = AL_OUTPUT_COUNT;
	    ALgetparams( AL_DEFAULT_DEVICE,parambuf,4);
	    if (parambuf[1] || parambuf[3]) {
		printf("Someone is already using the sound system.Exiting..\n");
		exit(1);
	    }

	    /* try to get a configuration descriptor */
	    if ( ( devAudioConfig=ALnewconfig() ) == (ALconfig) NULL ){
		printf ("Cannot get a config Descriptor.Exiting\n");
		exit(1);
	    }

	    /* configure parameters of audio channel */
	    abuf_ptr	=	0;
	    abuf_size	=	2048;
	    abuf_inc	= 	abuf_size/4; 

	    /* channel-specific parameters */	
	    ALsetqueuesize(devAudioConfig,(long)abuf_size);   	/* buffer size */
	    ALsetwidth(devAudioConfig,AL_SAMPLE_8);   	/* 8 bits */
	    ALsetsampfmt(devAudioConfig,AL_SAMPFMT_TWOSCOMP);	/* linear signed */
	    ALsetchannels(devAudioConfig,AL_MONO); 		/* mono */

	    /* global audio-device parameters */
	    parambuf[0]	= AL_OUTPUT_RATE;	parambuf[1] =	audio_sample_freq;
	    parambuf[2]	= AL_INPUT_RATE;	parambuf[3] =	audio_sample_freq;
	    parambuf[4]	= AL_RIGHT_SPEAKER_GAIN;parambuf[5] =	128;
	    parambuf[6]	= AL_LEFT_SPEAKER_GAIN; parambuf[7] =	128;
	    parambuf[8]	= AL_SPEAKER_MUTE_CTL;  parambuf[9] =	AL_SPEAKER_MUTE_OFF;
	    if( ALsetparams( AL_DEFAULT_DEVICE,parambuf,10) < 0) {
		printf("Cannot Configure the sound system.Exiting...\n");
		exit(1);
	    }

	    /* try to get a Audio channel descriptor with pre-calculated params */
	    if ( ( devAudio=ALopenport("audio_fd","w",devAudioConfig) ) == (ALport) NULL ){
		printf ("Cannot get an Audio Channel Descriptor.Exiting\n");
		exit(1);
	    }

	    for (i=0; i< AUDIO_NUM_VOICES; i++) audio_on[i]=0;

	} /* if (play_sound) */
	return TRUE;
}

void sysdep_exit(void) {
	if(play_sound){
		ALcloseport(devAudio);
		ALfreeconfig(devAudioConfig);
	}
}	

void sysdep_poll_joystick() {
}

/* routine to dump audio samples into audio device */
int sysdep_play_audio(byte *buff, int size) {
/* in a first approach, try only to send samples that no cause blocking */
        long maxsize=ALgetfillable(devAudio);
	return ALwritesamps(devAudio,(void *)buff,(size<=maxsize)?size:maxsize);	
}

void sysdep_fill_audio_buffer( long *in, char *out, int start,int end ) {
	for(; start<end; start++) out[start]=(char)( in[start] >> 7) ; 
}

#endif 
/****** end of IRIX dependent code; DO NOT WRITE BELOW THIS LINE *********/
