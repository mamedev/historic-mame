#include "mamalleg.h"
#include "driver.h"
#include <pc.h>
#include <conio.h>
#include <sys/farptr.h>
#include <go32.h>
#include <time.h>
#include <math.h>
#include "TwkUser.h"
#include "gen15khz.h"
#include "ati15khz.h"

/* from video.c */
void center_mode(Register *pReg,int low_scanrate);


int	wait_interlace;      /* config flag - indicates if we're waiting for odd/even updates */
static int display_interlaced=0;  /* interlaced display */

extern int video_sync;
extern int wait_vsync;
extern int use_triplebuf;
extern int unchained;

/* Generic SVGA 15.75KHz code and SVGA 15.75KHz driver selection */

/* To add a SVGA 15.75KHz driver for a given chipset you need to write (at least) */
/* 3 functions and put them into an entry in the array below. */
/* The 3 functions are as follows */

/* 1) 'Detect' -  checks for your chipset and returns 1 if found ,  0 if not */

/* 2) 'Setmode' - reprograms your chipset's clock and CRTC timing registers so that */
/* it produces a signal with a horizontal scanrate of 15.75KHz and a vertical refresh rate of 60Hz. */
/* (possibly producing an interlaced image if the chipset supports it) */
/* NOTE: 'vdouble' is passed into the function to indicate if MAME is doubling the */
/* height of the screen bitmap */
/* If it is - you can set your 15.75KHz mode to drop every other scanline */
/* and avoid having to interlace the display */
/* (see calc_mach64_height in ati15khz.c) */
/* Function should return 1 if it managed to set the 15.75KHz mode, 0 if it didn't */
/* It should also call 'setinterlaceflag(<n>)'; with a value of 1 or 0 to indicate if */
/* it setup an interlaced display or not. */
/* This is used to determine if the refresh needs to be delayed while both odd and even fields */
/* are drawn (if requested with -waitinter flag) */

/* 3) 'Reset' - restores any bios settings/tables you may have changed whilst setting up the 15.75KHz */
/* mode */
/* The Driver should support 640x480 */
/* and you should use center_x and center_y as 'tweak' values */
/* for your horizontal and vertical retrace start/end values */

/* Sources for information about programming SVGA chipsets- */
/* VGADOC4B.ZIP - from any SimTel site */
/* XFree86 source code */
/* the Allegro library */



/* our array of 15.75KHz drivers */
/* NOTE: Generic *MUST* be the last in the list as its */
/* detect function will always return 1 */
SVGA15KHZDRIVER drivers15KHz[]=
{
	{"ATI", detectati, setati15KHz, resetati15KHz},   /* ATI driver */
	{"Generic", genericsvga, setgeneric15KHz, resetgeneric15KHz}   /* Generic driver (must be last in list) */

};

/* array of CRTC settings for Generic SVGA 15.75KHz mode */
static Register scrSVGA_15KHz[] =
{
	{ 0x3c2, 0x00, 0xe7},{ 0x3d4, 0x00, 0x69},{ 0x3d4, 0x01, 0x4f},
	{ 0x3d4, 0x02, 0x4f},{ 0x3d4, 0x03, 0x91},{ 0x3d4, 0x04, 0x55},
	{ 0x3d4, 0x05, 0x1f},{ 0x3d4, 0x06, 0x05},{ 0x3d4, 0x07, 0x11},
	{ 0x3d4, 0x08, 0x00},{ 0x3d4, 0x09, 0x40},{ 0x3d4, 0x10, 0xf2},
	{ 0x3d4, 0x11, 0x46},{ 0x3d4, 0x12, 0xef},{ 0x3d4, 0x13, 0x28},
	{ 0x3d4, 0x14, 0x00},{ 0x3d4, 0x15, 0xef},{ 0x3d4, 0x16, 0x04},
	{ 0x3d4, 0x17, 0xe3},{ 0x3c4, 0x01, 0x09},{ 0x3c4, 0x04, 0x0a},
	{ 0x3ce, 0x05, 0x40},{ 0x3ce, 0x06, 0x05},{ 0x3c0, 0x10, 0x41}
};



/* seletct 15.75KHz driver */
int getSVGA15KHzdriver(SVGA15KHZDRIVER **driver15KHz)
{
	int n,nodrivers;
/* setup some defaults */
	*driver15KHz=NULL;
	display_interlaced = 0;
	nodrivers = sizeof(drivers15KHz) / sizeof(SVGA15KHZDRIVER);

/* then find a driver */
	for (n=0; n<nodrivers; n++)
	{
		if (drivers15KHz[n].detectsvgacard())
		{
			*driver15KHz = &drivers15KHz[n];
			return 1;
		}
	}
	return 0;
}

/* generic SVGA detect, always returns true */
/* UNLESS we've got triple buffering on - the virtual page size makes it impossible to */
/* set the memory offset through the 8bit register */
int genericsvga()
{
	if (use_triplebuf)
	{
		if (errorlog)
			fprintf (errorlog, "15.75KHz: Triple buffering is on - unable to use generic driver\n");
		return 0;
	}
	return 1;
}

/* set 15.75KHz SVGA mode using just standard VGA registers */
/* may or may *NOT* work with a given chipset */
int setgeneric15KHz(int vdouble, int width, int height)
{
	int temp,reglen;

	if (!sup_15Khz_res(width,height))
		return 0;
/* now check we support the character count */
	temp = readCRTC(HZ_DISPLAY_END);
	if (temp != 79&&temp != 99)
	{
		if (errorlog)
			fprintf(errorlog, "15.75KHz: Unsupported %dx%d mode (%d char. clocks)\n", width, height, temp);
		return 0;
	}
/* set the character count */
	scrSVGA_15KHz[H_DISPLAY_INDEX].value=temp;
	reglen = (sizeof(scrSVGA_15KHz) / sizeof(Register));
/* get the memory offset per line - this *should* make the driver handle different colour depths */
	temp = readCRTC (MEM_OFFSET);
/* skip every other line */
	scrSVGA_15KHz[MEM_OFFSET_INDEX].value = temp << 1;
/* indicate we're not interlacing */
	setinterlaceflag (0);
/* center the display */
	center_mode (scrSVGA_15KHz, 1);
/* write out the array */
	outRegArray (scrSVGA_15KHz,reglen);

	return 1;
}

/* generic cleanup - do nothing as we only changed values in the standard VGA register set */
void resetgeneric15KHz()
{
	return;
}

/* set our interlaced flag */
void setinterlaceflag(int interlaced)
{
	display_interlaced = interlaced;
	if (display_interlaced && wait_interlace)
	{
		/* make sure we're going to hit around 30FPS */
		video_sync = 0;
		wait_vsync = 0;
	}
}

/* function to delay for 2 screen updates */
/* allows complete odd/even field update of an interlaced display */
void interlace_sync()
{
	/* check it's been asked for, and that the display is actually interlaced */
	if (wait_interlace && display_interlaced)
	{
		/* make sure we get 2 vertical retraces */
		vsync();
		vsync();
	}
}


/* read a CRTC register at a given index */
int readCRTC(int nIndex)
{
	outportb (0x3d4, nIndex);
	return inportb (0x3d5);
}

/* Get vertical display height */
/* -it's stored over 3 registers */
int getVtEndDisplay()
{
	int   nVert;
/* get first 8 bits */
	nVert = readCRTC (VT_DISPLAY_END);
/* get 9th bit */
	nVert |= (readCRTC (CRTC_OVERFLOW) & 0x02) << 7;
/* and the 10th */
	nVert |= (readCRTC (CRTC_OVERFLOW) & 0x40) << 3;
	return nVert;
}


int sup_15Khz_res(int width,int height)
{
	if (width == 640 && height == 480)
		return 1;
/*800x600 - not working just yet */
/* 	if(width==800&&height==600) */
/* 		return 1; */
	if (errorlog)
		fprintf (errorlog, "15.75KHz: Unsupported %dx%d mode\n", width, height);
	return 0;
}


