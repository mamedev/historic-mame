/*
* NetBSD/i386 dependent code
*
*/

#ifdef netbsd_i386

#include "xmame.h"

#include <stdio.h>
#include <stdlib.h>

int sysdep_init(void) {

	int 			i;
	if (use_joystick) {
		printf("Joystick is not yet supported under NetBSD/i386.  Sorry.\n");
		use_joystick = FALSE;
	}
	if (play_sound) {
		printf("Sound is not yet supported under NetBSD/i386.  Sorry.\n");
		play_sound = FALSE;
        }
        return (TRUE);
}

void sysdep_exit(void) {
}	

void sysdep_poll_joystick() {
}

int sysdep_play_audio(byte *buf, int bufsize) {
}

void sysdep_fill_audio_buffer( long *in, char *out, int start,int end ) {
}

#endif
