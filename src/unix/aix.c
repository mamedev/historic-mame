/*
* AIX dependent code
*
* Audio support by Chris Sharp <sharp@hursley.ibm.com>
*
*/

#ifndef aix

#include "xmame.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stropts.h>
#include <UMS/UMSAudioDevice.h>
#include <UMS/UMSBAUDDevice.h>

int channels;
UMSAudioDeviceMClass audio_device_class;
UMSAudioDevice_ReturnCode rc;
UMSBAUDDevice audio_device;

Environment *ev;
UMSAudioTypes_Buffer buffer;
long samples_per_sec, bytes_per_sample;
long read_cnt, transferred_cnt;
long samples_read;
long samples_written;
long out_rate;
long left_gain, right_gain;

long flags;
UMSAudioDeviceMClass_ErrorCode audio_device_class_error;
char* error_string;
char* audio_formats_alias;
char* audio_inputs_alias;
char* audio_outputs_alias;
char* obyte_order;

int sysdep_init(void) {
	struct itimerval        timer_value;
	int 			i;
	if (use_joystick) {
		printf("Joystick is no still supported under AIX. Sorry\n");
		use_joystick = FALSE;
	}
	if (play_sound) {
		int supported=FALSE;
		channels = 2;
                printf ("AIX sound device initialization...\n");
                /* try to open audio device */
        	ev = somGetGlobalEnvironment();
        	audio_device = UMSBAUDDeviceNew();

        	rc = UMSAudioDevice_open(audio_device,ev,"/dev/paud0","PLAY", UMSAudioDevice_BlockingIO);
        	if (audio_device == NULL)
        	{
        	fprintf(stderr,"can't create audio device object\nError: %s\n",error_string);
        	exit(1);
        	}

    		abuf_size = 32768;
    		/*abuf_size = 16384;*/

    		rc = UMSAudioDevice_set_sample_rate(audio_device, ev, audio_sample_freq, &out_rate);
    		rc = UMSAudioDevice_set_bits_per_sample(audio_device, ev, AUDIO_SAMPLE_BITS);
    		rc = UMSAudioDevice_set_number_of_channels(audio_device, ev, channels);
    		rc = UMSAudioDevice_set_audio_format_type(audio_device, ev, "PCM");
    		rc = UMSAudioDevice_set_number_format(audio_device, ev, "TWOS_COMPLEMENT");
    		rc = UMSAudioDevice_set_volume(audio_device, ev, 100);
    		rc = UMSAudioDevice_set_balance(audio_device, ev, 0);

    		rc = UMSAudioDevice_set_time_format(audio_device,ev,UMSAudioTypes_Bytes);
		/*
    		rc = UMSAudioDevice_get_byte_order(audio_device,ev,&obyte_order);
		*/
    		/* you have to free the string after the query */

    		if (obyte_order) free(obyte_order);
    		rc = UMSAudioDevice_set_byte_order(audio_device, ev, "MSB");
    		left_gain = 100;
    		right_gain = 100;

    		rc = UMSAudioDevice_enable_output(audio_device, ev, "LINE_OUT", &left_gain, &right_gain);
    		rc = UMSAudioDevice_initialize(audio_device, ev);

    		buffer._buffer  = (char *) malloc(abuf_size);
    		buffer._length = 0;
    		buffer._maximum = abuf_size;

		/*
    		bytes_per_sample = (AUDIO_SAMPLE_BITS / 8) * channels;
    		samples_per_sec  = bytes_per_sample * out_rate;
    		buffer._buffer  = (char *) malloc(samples_per_sec);
    		buffer._length = 0;
    		buffer._maximum = samples_per_sec;
    		bbuf_size = buffer._maximum;
		*/
    		rc = UMSAudioDevice_start(audio_device, ev);
    		abuf_ptr = 0;
    		abuf_inc = abuf_size / 4;
        }
        return (TRUE);
}

void sysdep_exit(void) {
	if (play_sound) {
          rc = UMSAudioDevice_play_remaining_data(audio_device, ev, TRUE);
          UMSAudioDevice_stop(audio_device, ev);
          UMSAudioDevice_close(audio_device, ev);
          _somFree(audio_device);
          free(buffer._buffer);
	}
}	

void sysdep_poll_joystick() {
	/* !! no joystick available on Sparc :-( */
}

int sysdep_play_audio(byte *buf, int bufsize) {
        buffer._length = bufsize;
        rc = UMSAudioDevice_write(audio_device, ev, &buffer, bufsize, &samples_written);
        return rc;
}

void sysdep_fill_audio_buffer( long *in, char *out, int start,int end ) {

        /* 8 bits linear 22050hz */
        /* for(; start<end; start++) out[start]=(char)( in[start] >> 7);*/
        for (;start < end; start++)
        buffer._buffer[start] = in[start] >> 10;
	return;	
}

#endif
