/*
* HP-UX dependent code
*/
#ifdef hpux
#define __HPUX_C_	
#include "xmame.h"

int sysdep_init(void) {
	play_sound	= FALSE;
	use_joystick	= FALSE;
}

void sysdep_exit(void) {
}	

void sysdep_poll_joystick() {
}

int sysdep_play_audio(byte *buf, int bufsize) {
        return write(audio_fd,buf,bufsize);
}

void sysdep_fill_audio_buffer( long *in, char *out, int start,int end ) {
	for(; start<end; start++) out[start]=(char)( in[start] >> 7) ; 
}

#endif 
