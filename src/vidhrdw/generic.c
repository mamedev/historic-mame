/***************************************************************************

  vidhrdw/generic.c

  Some general purpose functions used by many video drivers.

***************************************************************************/

#include "driver.h"
#include "generic.h"


unsigned char *videoram;
int videoram_size;
unsigned char *colorram;
int colorram_size;
unsigned char *spriteram;	/* not used in this module... */
unsigned char *spriteram_2;	/* ... */
unsigned char *spriteram_3;	/* ... */
int spriteram_size;	/* ... here just for convenience */
unsigned char *dirtybuffer;
struct osd_bitmap *tmpbitmap;



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int generic_vh_start(void)
{
	if ((dirtybuffer = malloc(videoram_size)) == 0)
		return 1;
	memset(dirtybuffer,0,videoram_size);

	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer);
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void generic_vh_stop(void)
{
	free(dirtybuffer);
	osd_free_bitmap(tmpbitmap);
}



int videoram_r(int offset)
{
	return videoram[offset];
}



int colorram_r(int offset)
{
	return colorram[offset];
}



void videoram_w(int offset,int data)
{
	if (videoram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		videoram[offset] = data;
	}
}



void colorram_w(int offset,int data)
{
	if (colorram[offset] != data)
	{
		dirtybuffer[offset] = 1;

		colorram[offset] = data;
	}
}
