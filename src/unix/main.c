/*
 * An adaptation of MAME 0.2 for X11 / 0.81 for DOS.
 * Can be played. Has only been tested on Linux, but should port easily.
 */

#define __MAIN_C_
#include "xmame.h"

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
	widthscale		= 1;
	heightscale		= 1;
        osd_joy_up = osd_joy_down = osd_joy_left = osd_joy_right = 0 ;
	osd_joy_b1 = osd_joy_b2 = osd_joy_b3 = osd_joy_b4 = 0;
	mit_shm_avail		= 0;
	displayname		= ":0";
	snapshot_no		= 0;

	/* parse argument invocation */
	for (i = 1;i < argc;i++) {
		if (strcasecmp(argv[i], "-sound") == 0) {
			play_sound        = TRUE;
			audio_sample_freq = AUDIO_SAMPLE_FREQ;
			audio_timer_freq  = AUDIO_TIMER_FREQ;
		}
		if (strcasecmp(argv[i], "-nojoy") == 0) use_joystick = FALSE;
		if (strcasecmp(argv[i], "-widthscale") == 0) {
			widthscale = atoi (argv[++i]);
			if (widthscale <= 0) {
				fprintf (stderr,"illegal widthscale (%d)\n", widthscale);
				exit (1);
			}
		}
		if (strcasecmp(argv[i], "-heightscale") == 0) {
			heightscale = atoi (argv[++i]);
			if (heightscale <= 0) {
				fprintf (stderr,"illegal heightscale (%d)\n", heightscale);
				exit (1);
			}
		}
		if (strcasecmp(argv[i], "-nomitshm") == 0) {
			fprintf(stderr,"MIT Shared Memory use disabled\n");
			mit_shm_avail=-1;
		}
		if (strcasecmp(argv[i], "-display") == 0) {
			if ( ! *argv[i+1] ) {
				fprintf(stderr,"Invalid use of -display option\n");
				exit(1);
			}
			displayname=argv[++i];
		}
	}
	/* now invoice unix-dependent initialization */
	sysdep_init();
	if (play_sound) start_timer();
	first_free_pen = 0;

	/* Initialize key array - no keys are pressed. */
	for (i = 0; i < OSD_NUMKEYS; i++) key[i] = FALSE;

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
 * Joystick routines
 * should be in a separate archive, but ...
 */

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

