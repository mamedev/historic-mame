/*
 * An adaptation of MAME 0.2 for X11 / 0.81 for DOS.
 * Can be played. Has only been tested on Linux, but should port easily.
 */

#define __MAIN_C_
#include "xmame.h"

extern int frameskip; /* defined in src/mame.c */

char *title="X-Mame version 0.23.2";

/* 
 * show help and exit
 */
int show_usage() 
{
char message[]="%s\n\
Usage: xmame [game] [options]\n\
Options:\n\
	-sound				Enable sound ( if available )\n\
	-nofm				Disable use of FM Synthesis\n\
	-nojoy				Disable joystick ( if supported )\n\
	-heightscale	<scale> 	Set Y-Scale aspect ratio\n\
	-widthscale	<scale> 	Set X-Scale aspect ratio\n\
	-scale 		<scale>		Set X-Y Scale to the same aspect ratio\n\
	-frameskip	<#frames>	Skip <#frames> in video refresh\n\
	-help				Show this help\n\
	-mamedir	<dir>		Tell XMame where roms base tree resides\n\
	-display	<display>	To select display\n\
	-nomitshm			Disable MIT shared memory usage (if any)\n\
	-privatecmap			To use private color maps\n\
	-mapkey	    <Xcode>,<Mamecode>  Set an specific key mapping\n\
\n\
Files: \n\
	${HOME}/.xmamerc			User configuration file\n\
\n\
Environment variables:\n\
\n\
	HOME				Users's home directory\n\
	MAMEDIR				Arcade Roms base directory\n\
	DISPLAY				X-Server to display in\n\
\n\
Mame is an Original Idea of Nicola Salmoria and Mirko Buffoni\n\
X-Mame port mantained by Juan Antonio Martinez\n\
";
	fprintf(stderr,message,title);
	return (OSD_NOT_OK); /* force exit on return */
}
/*
 * check for existence of ${HOME}/.xmamerc
 * if found parse it
 */
int parse_xmamerc_file()
{
	struct passwd   *pw;
	FILE            *file;
	int             lineno;
	char            buffer[2048];

	/* locate user's home directory */
	pw=getpwuid(getuid());
	if(!pw) { 
		fprintf(stderr,"Who are you? Not found in passwd database!!\n");
		return (OSD_NOT_OK);
	}
	sprintf(buffer,"%s/%s",pw->pw_dir,".xmamerc");
	/* try to open file. If so parse it */
	if ( (file=fopen(buffer,"r")) == (FILE *) NULL) {
		fprintf(stderr,"File %s not found. Skipping ...\n",buffer);
		return (OSD_OK);
	}
	lineno=0;
	while(fgets( buffer,2047,file) ) {
	    char *p;
	    char *(q[5]);
	    int i,j;
	    lineno++;
	    /* skip comments */
	    if ( ( p=strchr(buffer,'#') ) ) *p='\0';
	    /* scan for words */
	    for(i=0;i<5;i++) q[i]=(char *)NULL;
	    for(i=0,j=0,p=buffer; *p; p++ ) {
		if ( isspace(*p) ) { *p='\0'; j=0; }
		else               { if(!j++) q[i++]=p; }
	    }   /* end of stripping words ( for i=0 )*/
	    /* test for wrong number of args */
	    if ( i==0 ) continue; /* empty line */
	    if ( i!=2 ) { 
		    fprintf(stderr,"Line %d: wrong number of parameters \n",lineno);
		    fclose(file);
		    return (OSD_NOT_OK);
	    }
	    /* now parse commands */
	    if (strcasecmp(q[0], "play_sound") == 0) {
		play_sound        = atoi( q[1] );
		continue;
	    } /* sound */
	    if (strcasecmp(q[0], "dontuse_fm_synth") == 0) {
		No_FM        = atoi( q[1] );
		continue;
	    } /* sound */
	    if (strcasecmp(q[0], "use_joystick") == 0) {
		use_joystick = atoi( q[1] ) ;
		continue; 
	    } /* joystick */
	    if (strcasecmp(q[0], "frameskip") == 0) {
		frameskip = atoi( q[1] ) ;
		if (frameskip>3) frameskip=3;
		continue; 
	    } /* frameskip */
	    if (strcasecmp(q[0], "widthscale") == 0) {
		widthscale = atoi ( q[1] );
		if (widthscale <= 0) {
			fprintf (stderr,"Line %d: Illegal widthscale (%d)\n", lineno,widthscale);
			return (OSD_NOT_OK);
		}
		continue;
	    } /* widthscale */
	    if (strcasecmp(q[0], "heightscale") == 0) {
		heightscale = atoi ( q[1] );
		if (heightscale <= 0) {
			fprintf (stderr,"Line %d: Illegal heightscale (%d)\n", lineno,heightscale);
			return (OSD_NOT_OK);
		}
		continue;
	    } /* heightscale */
	    if (strcasecmp(q[0], "private_colormap") == 0) {
		use_private_cmap = atoi(q[1]);
		continue;
	    } /* Force use of private color maps */
	    if (strcasecmp(q[0], "use_mitshm") == 0) {
		mit_shm_avail = ( atoi(q[1]) ) ? 0 : -1 ;
		continue;
	    } /* mit shared memory */
	    if (strcasecmp(q[0], "defaultgame") == 0) {
		char *pt;
		extern char *GameName;
		pt=malloc(1+strlen(q[1]) );
		if( ! pt ) { 
			fprintf(stderr,"Malloc error: line %d\n",lineno);
			return (OSD_NOT_OK);
		}
		strcpy(pt,q[1]);
		GameName=pt;
		continue;
	    } /* mamedir */
	    if (strcasecmp(q[0], "mamedir") == 0) {
		mamedir=malloc(1+strlen(q[1]) );
		if( ! mamedir ) { 
			fprintf(stderr,"Malloc error: line %d\n",lineno);
			return (OSD_NOT_OK);
		}
		strcpy(mamedir,q[1]);
		continue;
	    } /* mamedir */
	    if (strcasecmp(q[0], "display") == 0) {
		displayname=malloc(1+strlen(q[1]) );
		if( ! displayname ) { 
			fprintf(stderr,"Malloc error: line %d\n",lineno);
			return (OSD_NOT_OK);
		}
		strcpy(displayname,q[1]);
		continue;
	    } /* displayname */
	    if (strcasecmp(q[0],"mapkey") == 0) {
		int from,to;
		if ( sscanf(q[1],"%x,%x",&from,&to) != 2 )
		    fprintf(stderr,"Line %d: Invalid keymapping %s. Ignoring...\n",lineno,q[1]);
		else    if (sysdep_mapkey(from,to) ) {
			    fprintf(stderr,"Line %d: invalid keymapping\n",lineno);
			    return (OSD_NOT_OK);
			}
		continue;
	    } /* mapkey */
	    /* if arrives here means unrecognized command line */
	    fprintf(stderr,"Line %d: unknown command %s\n",lineno,q[0]);
	    return (OSD_NOT_OK);
	} /* while */
	fclose(file);
	return (OSD_OK);
}

/*
 * Put anything here you need to do when the program is started.
 * Return 0 if initialization was successful, nonzero otherwise.
 */

int osd_init_environment (void)
{
	char *pt;
	/* initialize some variables to defalut values */
	use_joystick            = TRUE;
	use_private_cmap	= FALSE;
	play_sound              = FALSE;
	No_FM			= FALSE; /* by default use it */
	widthscale              = 1;
	heightscale             = 1;
	mit_shm_avail           = 0;
	snapshot_no             = 0;
	mamedir                 = ".";
	displayname             = ":0.0";

	/* parse .xmamerc file */
	if( parse_xmamerc_file() ) {
		fprintf(stderr,"Error in parsing .xmamerc file\n");
		return (OSD_NOT_OK);
	}

	/* main programs need to chdir to mamedir before doing anything. */

	pt=getenv("MAMEDIR");
	if (pt) mamedir=pt;
	chdir(mamedir);	/* may fail; dont worry about it */

	return (OSD_OK);
}

int osd_init (int argc, char *argv[])
{
	int i;
	char *pt;

	/* get environment variables. This overrides xmamerc options */

	pt=getenv("DISPLAY");
	if (pt) displayname=pt;

	pt=getenv("MAMEDIR");
	if (pt) mamedir=pt;

	/* parse argument invocation. */
	/* These has Higher prioryty than conf file and environment */

	for (i = 1;i < argc;i++) {
		if (strcasecmp(argv[i], "-help") == 0) return show_usage();
		if (strcasecmp(argv[i], "-h") == 0) return show_usage();
		if (strcasecmp(argv[i], "-?") == 0) return show_usage();
/*
		if (strcasecmp(argv[i], "-list") == 0) return show_game_list();
		if (strcasecmp(argv[i], "-info") == 0) {
			if ( ! *argv[i+1] ) {
				fprintf(stderr,"Invalid use of -info option\n");
				return (OSD_NOT_OK);
			}
			return show_game_info(argv[++i]);;
		}
*/
		if (strcasecmp(argv[i], "-nofm") == 0) {
			No_FM = TRUE;
			continue;
		}
		if (strcasecmp(argv[i], "-nojoy") == 0) {
			use_joystick = FALSE;
			continue;
		}
		if (strcasecmp(argv[i], "-sound") == 0) {
			play_sound        = TRUE;
			audio_sample_freq = AUDIO_SAMPLE_FREQ;
			audio_timer_freq  = AUDIO_TIMER_FREQ;
			continue;
		}
		if (strcasecmp(argv[i], "-privatecmap") == 0) {
			use_private_cmap = TRUE;
			continue;
		}
		if (strcasecmp(argv[i], "-widthscale") == 0) {
			widthscale = atoi (argv[++i]);
			if (widthscale <= 0) {
				fprintf (stderr,"illegal widthscale (%d)\n", widthscale);
				return (OSD_NOT_OK);
			}
			continue;
		}
		if (strcasecmp(argv[i], "-heightscale") == 0) {
			heightscale = atoi (argv[++i]);
			if (heightscale <= 0) {
				fprintf (stderr,"illegal heightscale (%d)\n", heightscale);
				return (OSD_NOT_OK);
			}
			continue;
		}
		if (strcasecmp(argv[i], "-frameskip") == 0) {
			frameskip = atoi (argv[++i]);
			if (frameskip <= 0) {
				fprintf (stderr,"illegal frameskip value (%d)\n", heightscale);
				return (OSD_NOT_OK);
			}
			if (frameskip >3) frameskip=3;
			continue;
		}
		if (strcasecmp(argv[i], "-scale") == 0) {
			heightscale = widthscale = atoi (argv[++i]);
			if (heightscale <= 0) {
				fprintf (stderr,"illegal scale factor (%d)\n", heightscale);
				return (OSD_NOT_OK);
			}
			continue;
		}
		if (strcasecmp(argv[i], "-nomitshm") == 0) {
			fprintf(stderr,"MIT Shared Memory use disabled\n");
			mit_shm_avail=-1;
			continue;
		}
		if (strcasecmp(argv[i], "-mamedir") == 0) {
			if ( ! *argv[i+1] ) {
				fprintf(stderr,"Invalid use of -mamedir option\n");
				return (OSD_NOT_OK);
			}
			mamedir=argv[++i];
			continue;
		}
		if (strcasecmp(argv[i], "-display") == 0) {
			if ( ! *argv[i+1] ) {
				fprintf(stderr,"Invalid use of -display option\n");
				return (OSD_NOT_OK);
			}
			displayname=argv[++i];
			continue;
		}
		if (strcasecmp(argv[i],"-mapkey") == 0) {
			if ( ! *argv[i+1] ) {
				fprintf(stderr,"Invalid use of -mapkey option\n");
				return (OSD_NOT_OK);
			} else {
				int from,to;
				if ( sscanf(argv[++i],"%x,%x",&from,&to) != 2 )
					fprintf(stderr,"Invalid keymapping %s. Ignoring...\n",argv[i]);
				else    sysdep_mapkey(from,to);
			}
			continue;
		}
		/* if arrives here and not at first argument, mean syntax error */
		if (i>1) {
			fprintf(stderr,"Unknown option. Try %s -help\n",argv[0]);
			return (OSD_NOT_OK);
		}

	}

	/* try to set current working directory to mamedir */
	if ( chdir(mamedir)<0 ) {
		fprintf(stderr,"Cannot chdir() to %s. Sorry\n",mamedir);
		return (OSD_NOT_OK);
	}

	/* call several global initialization routines */
	sysdep_joy_initvars();
	sysdep_audio_initvars();
	sysdep_keyboard_init();
	/* now invoice system-dependent initialization */
	sysdep_init();
	if (play_sound) start_timer();
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


int  sysdep_joy_initvars(void)
{
	osd_joy_up = osd_joy_down = 0;
	osd_joy_left = osd_joy_right = 0 ;
	osd_joy_b1 = osd_joy_b2 = 0;
	osd_joy_b3 = osd_joy_b4 = 0;
	return (0);
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

#ifdef HAVE_GETTIMEOFDAY
/* Standard UNIX clock() is based on CPU time, not real time.
   Here is a real-time drop in replacement for UNIX systems that have the
   gettimeofday() routine.  This results in much more accurate timing for
   throttled emulation.
*/
clock_t clock()
{
  static long init_sec = 0;

  struct timeval tv;
  gettimeofday(&tv, 0);
  if (init_sec == 0)
    init_sec = tv.tv_sec;
  return (tv.tv_sec - init_sec) * 1000000 + tv.tv_usec;
}
#endif
