/*
 * X-Mame video specifics code
 *
 */
#define __XDEP_C_

/*
 * Include files.
 */

#include "xmame.h"

static Screen	*screen;

/*
 * makes a snapshot of screen when F12 is pressed.
 *
 * store it in xpm pixmap format
 * save in users'home dir as gamename-XXX.xpm ( XXX equals to #snapshot )
 * 
 */

int osd_snapshot(void)
{
#ifdef HAS_XPM
	char name[1024];
	FILE *f;
	int res;
	extern char *GameName;
	XpmAttributes *xpmattr;
	do
	{
		sprintf(name,"%s/%s-snap%03d.xpm",getenv("HOME"),GameName,snapshot_no);
		/* avoid overwriting of existing files */
		if ((f = fopen(name,"rb")) != 0)
		{
			fclose(f);
			snapshot_no++;
		}
	} while (f != 0);
	/* set propper colormap */
	xpmattr=calloc(1,sizeof(XpmAttributes));
	if ( ! xpmattr ) { 
		fprintf(stderr,"calloc() Failed in osd_snapshot\n");
		return (OSD_NOT_OK);
	}
	xpmattr->valuemask	= XpmColormap;
	xpmattr->colormap	= colormap;	/* set my own colormap */
	res = XpmWriteFileFromImage(display,name,image,0,xpmattr);
	free(xpmattr);				/* cleanup task */
	if ( res != XpmSuccess ) {
		fprintf(stderr,"Failed to save snapshot %s\n",name);
		return (OSD_NOT_OK);
	} else {
		fprintf(stderr,"Saved snapshot as %s\n",name);
		/* wait until key released */
		while( osd_key_pressed(OSD_KEY_F12) );
		return (OSD_OK);
	}
#else
	fprintf(stderr,"Sorry: XPM library not included. Cannot make snapshot\n");
	return (OSD_NOT_OK);
#endif
}

struct osd_bitmap *osd_create_bitmap (int width, int height)
{
    struct osd_bitmap  	*bitmap;
    int 		i;
    unsigned char  	*bm;

    if ((bitmap = malloc (sizeof (struct osd_bitmap) + (height) * sizeof (byte *))) != NULL) {
        bitmap->width = width;
        bitmap->height = height;

        if ((bm = malloc (width * height * sizeof(unsigned char))) == NULL) {
            free (bitmap);
            return (NULL);
        }
        for (i = 0;i < height;i++) { bitmap->line[i] = &bm[i * width]; }

        bitmap->private = bm;
    }
    return (bitmap);
}

void osd_free_bitmap (struct osd_bitmap *bitmap)
{
	if (bitmap) {
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

/* following routine traps missused MIT-SHM if not available */
int test_mit_shm(Display *display, XErrorEvent *error) {
	char msg[256];
	unsigned char ret=error->error_code;
	XGetErrorText(display,ret,msg,256);
	/* if MIT-SHM request failed, note and continue */
	if (error->error_code == BadAccess ) { mit_shm_avail=0; return 0; }
	/* else unspected error code: notify and exit */
	fprintf(stderr,"Unspected X Error %d: %s\n",ret,msg );
	exit(1);
}


struct osd_bitmap *osd_create_display (int width, int height)
{
	int 		 myscreen;
	XEvent		 event;
  	XSizeHints 	 hints;
  	XWMHints 	 wm_hints;
	XVisualInfo	 myvisual;
	Status		 result;
	int		 xsize,ysize;
	int		 i,j,k;
	extern char 	*title;
	/* Allocate the bitmap and set the image width and height. */
	bitmap = malloc ( sizeof (struct osd_bitmap) +
			  (height-1) * sizeof (unsigned char *) );
	if ( ! bitmap ) { return (NULL); }

	bitmap->width 	= width;
	bitmap->height  = height;
	xsize		= width * widthscale;
	ysize		= height * heightscale;
	first_free_pen  = 0;

	/* Open the display. */

	display = XOpenDisplay (displayname);
  	if(!display) {
  		fprintf (stderr,"OSD ERROR: failed to open the display.\n");
  		return (NULL);
  	}

  	screen 	 = DefaultScreenOfDisplay (display);
	myscreen = DefaultScreen(display);
	gc	 = DefaultGCOfScreen (screen);

	/* test for available 8bit bitmaps resources */
	result 	 = XMatchVisualInfo(display,myscreen,8,PseudoColor,&myvisual);
	if ( ! result ) {
		fprintf(stderr,"Cannot obtain a 8bit depth PseudoColor X-Visual Resource\n");
		fprintf(stderr,"Perhaps you'd better to restart X in -bpp 8 mode\n");
		return (NULL);
	}

	/* look for available Mitshm extensions */
	if (mit_shm_avail != -1 ) {
		mit_shm_avail	= 0;
#ifdef MITSHM
		/* get XExtensions to be if mit shared memory is available */
		result 	= XQueryExtension(display,"MIT-SHM",&i,&j,&k);
		if ( ! result) fprintf(stderr,"X-Server Doesn't support MIT-SHM extension\n");
		else 	       mit_shm_avail = 1;
#endif
	} else mit_shm_avail=0; /* has been user-disabled */

	/* Create the window. No buttons, no fancy stuff. */
  	window = XCreateSimpleWindow (display, 
				RootWindowOfScreen (screen), 
				0, 
				0,
  			    	xsize, 
				ysize, 
			  	0, 
				WhitePixelOfScreen(screen), 
				BlackPixelOfScreen(screen)
				);
  	if (!window) {
  		fprintf (stderr,"OSD ERROR: failed in XCreateSimpleWindow().\n");
  		return (NULL);
	}

	/* now get a color map structure */
	if ( ! use_private_cmap) colormap = DefaultColormapOfScreen(screen);
	else {
		colormap = XCreateColormap(display,window,myvisual.visual,AllocNone);
		XSetWindowColormap(display,window,colormap);
		fprintf(stderr,"Using private color map\n");
	}

	/*  Placement hints etc. */

	hints.flags	= PSize | PMinSize | PMaxSize;
 	hints.min_width	= hints.max_width = hints.base_width = xsize;
 	hints.min_height= hints.max_height = hints.base_height = ysize;

	wm_hints.input	= TRUE;
	wm_hints.flags	= InputHint;

	XSetWMHints 		(display, window, &wm_hints);
	XSetWMNormalHints 	(display, window, &hints);
	XStoreName 		(display, window, title);

	/* Map and expose the window. */

	XSelectInput   (display, window, FocusChangeMask | ExposureMask | KeyPressMask | KeyReleaseMask);
	XMapRaised     (display, window);
	XClearWindow   (display, window);
	XAutoRepeatOff (display);  /* should be a command line option */
	XWindowEvent   (display, window, ExposureMask, &event);

#ifdef MITSHM
	if ( mit_shm_avail ) {
	    /* Create a MITSHM image. */

	    XSetErrorHandler( test_mit_shm );

	    /* CS: scaling changes requested image size */
	    image = XShmCreateImage (display, 
				myvisual.visual ,
				8,
       			    	ZPixmap, 
				NULL, 
				&shm_info, 
				xsize, 
				ysize);
  	    if (!image) {
  		fprintf (stderr,"OSD ERROR: failed in XShmCreateImage().\n");
		mit_shm_avail=0;
		goto mit_shm_failed;
	    }
	    shm_info.shmid = shmget ( IPC_PRIVATE, 
				      image->bytes_per_line * image->height, 
				      (IPC_CREAT | 0777) );
	    if ( shm_info.shmid < 0) {
		    fprintf (stderr,"OSD ERROR: failed to create MITSHM block.\n");
		    return (NULL);
	    }

	    /* And allocate the bitmap buffer. */

	    /* CS: scaling uses 2 buffers for transparency with mame */
	    if (scaling) {
  	    	    scaled_buffer_ptr = (byte *)(image->data = 
			shm_info.shmaddr = (byte *) shmat ( shm_info.shmid,0 ,0));
  		    buffer_ptr = (byte *) malloc (sizeof(byte) * bitmap->width * bitmap->height);
  	    } else {
    		    buffer_ptr = scaled_buffer_ptr = (byte *)(image->data = 
			shm_info.shmaddr = (byte *) shmat (shm_info.shmid, 0, 0));
  	    }

  	    if (!buffer_ptr || (scaling && !scaled_buffer_ptr)) {
    	    	    fprintf (stderr,"OSD ERROR: failed to allocate MITSHM bitmap buffer.\n");
    		    return (NULL);
  	    }

	    shm_info.readOnly = FALSE;

	    /* Attach the MITSHM block. this will cause an exception if */
	    /* MIT-SHM is not available. so trap it and process         */

	    fprintf(stderr,"MIT-SHM Extension Available. trying to use...");

	    if(!XShmAttach (display, &shm_info)) {
  		    fprintf (stderr,"OSD ERROR: failed to attach MITSHM block.\n");
  		    return (NULL);
	    }
	    XSync(display,False); /* be sure to get request processed */
	    sleep(2); /* enought time to notify error if any */
	    /* agh!! sleep() disables timer alarm. have to re-arm */
	    if (play_sound) start_timer(); 
	    XSetErrorHandler( None ); /* Restore error handler to default */
	    if ( ! mit_shm_avail ) {
		fprintf (stderr," failed.\nReverting to normal XPutImage() mode\n");
		if (scaling) { shmdt(scaled_buffer_ptr); free(buffer_ptr); }
		else 	     shmdt(buffer_ptr);
		goto mit_shm_failed; /* AAAAAAAAAAAGGGGGGGGGGGHHHHHHHHH!!! */
	    } 
	    fprintf(stderr,"Success.\nUsing Shared Memory Features to speed up\n");

	} else { /* Not MITSHM Allocate a bitmap buffer. */
#endif
mit_shm_failed:  	/** AAGGGGGGHHHHH !!!! (again) */
  	    buffer_ptr = (byte *) malloc (sizeof(byte) * bitmap->width * bitmap->height);

  	    /* CS: allocated scaled image buffer - ugly, but leave all other code untouched. */
	    scaled_buffer_ptr=buffer_ptr;
	    if (scaling) {
  		scaled_buffer_ptr = (byte *) malloc (sizeof(byte) * xsize * ysize );
	    }
  	    if (!buffer_ptr || (scaling && !scaled_buffer_ptr)) {
    		fprintf (stderr,"OSD ERROR: failed to allocate bitmap buffer.\n");
    		return (NULL);
	    }
 	    /* CS: Ximage is allocated against our new image if scaling active */
  	    image = XCreateImage ( display, 
			myvisual.visual , 
			8, ZPixmap, 
			0, 
			(char *) scaled_buffer_ptr, 
			xsize, ysize,
			8, 
			0);
  	    if (!image) {
    		fprintf (stderr,"OSD ERROR: could not create image.\n");
    		return (NULL);
  	    }
#ifdef MITSHM
	}/* end of not MITSHM */
#endif
	for (i = 0;i < height; i++) bitmap->line[i] = &buffer_ptr[i * width];
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

	if (display && window) {
#ifdef MITSHM
		if (mit_shm_avail) {
        	    XShmDetach (display, &shm_info);	/* Detach the SHM. */
      		    if (shm_info.shmaddr) shmdt (shm_info.shmaddr);
      		    if (shm_info.shmid >= 0) shmctl (shm_info.shmid, IPC_RMID, 0);
		} else {
#endif
		/* Memory will be deallocated shortly (== buffer_ptr). */
#ifdef MITSHM
		}
#endif

		/* Destroy the image and free the colors. */
		if (image) {
    			XDestroyImage (image);
			/* CS: scaling hack means scaled_buffer_ptr is auto-freed, so... */
   			if (scaling) free(buffer_ptr);
   		}
    		for (i = 0; i < OSD_NUMPENS; i++) {
			if (xpixel[i] != BlackPixelOfScreen(screen) ) {
				l = (unsigned long) xpixel[i];
      				XFreeColors (display, colormap, &l, 1, 0);
       			}
  		}
		if (use_private_cmap) XFreeColormap(display,colormap);
  		if (display) {
  			XAutoRepeatOn (display);
  			XFlush (display); /* send all events to sync; */
  			XCloseDisplay (display);
  		}
	}
	if (xpixel) {
		free (xpixel);
		xpixel = NULL;
		first_free_pen = 0;
	}

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
	int		i;
	XColor		color;

	/* First time ? */
	if (xpixel == NULL) {
		if ((xpixel = (unsigned long *) malloc (sizeof (unsigned long) * OSD_NUMPENS)) == NULL)
		{
			fprintf (stderr,"OSD ERROR: failed to allocate pixel index table.\n");
			return (OSD_NOT_OK);
		}
		for (i = 0; i < OSD_NUMPENS; i++) xpixel[i] = BlackPixelOfScreen(screen) ;
	}

	/* Translate VGA 0-63 values to X 0-65536 values. */

	color.flags	= (DoRed | DoGreen | DoBlue);
	color.red	= 256 * (int) red;
	color.green	= 256 * (int) green;
	color.blue	= 256 * (int) blue;

	if (XAllocColor (display, colormap, &color)) xpixel[first_free_pen] = color.pixel;
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
  /* CS: scaling - here's the CPU burner.  Buy a faster computer
         sez Micro$oft. Ignore the ugly framerate stuff.*/
	
    int needupdate=0;
    int xsize=bitmap->width*widthscale;
    int ysize=bitmap->height*heightscale;
    /* if F12 pressed, make a snapshot of the screen */
    if(osd_key_pressed(OSD_KEY_F12) ) osd_snapshot(); 
    if (scaling) { /* IF SCALING ACTIVE */
    	int x,y,i,linechanged;
   	register byte *from = buffer_ptr;
    	register byte *to   = scaled_buffer_ptr;
	
    	for (y=0;y<bitmap->height;y++,to+=(heightscale-1)*xsize) {
    	    linechanged=0;
    	    for (x=0; x<bitmap->width; x++,from++,to+=widthscale) {
    		if ( *to == *from ) continue;
    		for (i=0;i<widthscale;i++) *(to+i)=*from;
    		linechanged++;
    	    } /* end of x search. if line changed, use memcpy to update */
    	    if(linechanged) {
		for (i=1;i<heightscale;i++) memcpy((to+(i-1)*xsize),to-xsize,xsize);
		needupdate=1;
	    }
    	} /* end of lines in bitmap */
    } /* if scaling */  else { needupdate=1; }
    /* now put image */
#ifdef MITSHM
    if (mit_shm_avail && needupdate) {
	XShmPutImage (display, window, gc, image, 0, 0, 0, 0, xsize, ysize, FALSE);
   	XSync (display,False); /* be sure to get request processed */
    } else { /* no shm: just use XPutImage */
#endif
        if(needupdate) XPutImage (display, window, gc, image, 0, 0, 0, 0, xsize, ysize);
   	XFlush (display);
#ifdef MITSHM
    }
#endif
}

