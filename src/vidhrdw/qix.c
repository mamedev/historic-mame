/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include <stdlib.h> /* JB 970524 */
#ifdef __MWERKS__
#include "vgeneric.h"
#else
#include "vidhrdw/generic.h"
#endif

extern int first_free_pen; /* JB 970520 */

static unsigned char qixpal[256];
static unsigned char *screen; /* JB 970524 */

/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
/* JB 970524 */
int qix_vh_start(void)
{
	if( screen==0 )
	{
		screen = malloc( 256*256 );
		if( screen==0 ) return 1;
	}
	return generic_vh_start();
}

/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
/* JB 970524 */
void qix_vh_stop(void)
{
	if( screen ) free( screen );
	screen = 0;
	generic_vh_stop();
}

/* The screen is 256x256 with eight bit pixels (64K).  The screen is divided
into two halves each half mapped by the video CPU at $0000-$7FFF.  The
high order bit of the address latch at $9402 specifies which half of the
screen is being accessed.

The address latch works as follows.  When the video CPU accesses $9400,
the screen address is computed by using the values at $9402 (high byte)
and $9403 (low byte) to get a value between $0000-$FFFF.  The value at
that location is either returned or written. */

int qix_videoram_r(int offset)
{
	if( RAM[0x9402] & 0x80 ) offset += 0x8000;
	return screen[offset];
}

void qix_videoram_w(int offset,int data)
{
	unsigned char *bm;
	unsigned char x, y;
	
	if( RAM[0x9402] & 0x80 ) offset += 0x8000;
	
	/* bitmap is rotated -90 deg. */
	x = offset >> 8;
	y = offset & 0xff; y = ~y;
	
	bm = tmpbitmap->line[y] + x;
	*bm = qixpal[data];

	screen[offset] = data;
}
	

int qix_addresslatch_r(int offset)
{
	offset = RAM[0x9402] << 8 | RAM[0x9403];
	return screen[offset];
}

void qix_addresslatch_w(int offset, int data)
{
	unsigned char *bm;
	unsigned char x, y;
	
	offset = RAM[0x9402] << 8 | RAM[0x9403];
	
	/* bitmap is rotated -90 deg. */
	x = RAM[0x9402];
	y = ~ RAM[0x9403];

	bm = tmpbitmap->line[y] + x;
	*bm = qixpal[data];

	screen[offset] = data;
}


/* The color RAM works as follows.  The color RAM contains palette values for
four pages (0-3).  When a write to $8800 on the video CPU occurs, the color
RAM page is taken from the lowest 2 bits of the value.  This selects one of
the color RAM pages as follows:

     colorRAMAddr = 0x9000 + ((data & 0x03) * 0x100);

The palette values are then read from that address:
     for (i = 0; i < 256; i++ )
     {
         data = RAM[colorRAMAddr++];
         palette[i][0] = data & 0xC0;          / Red /
         palette[i][1] = (data & 0x30) << 2;   / Green /
         palette[i][2] = (dtat & 0x0C) << 4;   / Blue /
     }
     setpallete(palette);

Qix uses a palette of 64 colors (2 each RGB) and four intensities (RRGGBBII).
*/

void qix_colorram_w(int offset, int data)
{
	int colorRAMAddr;
	int i,x;
	int red, green, blue /*,intensity*/;
	unsigned char *bm;

        colorRAMAddr = 0x9000 + ((data & 0x03) * 0x100);

	first_free_pen = 0; /* JB 970520 */

	for (i = 0; i < 256; i++ )
	{
		data = RAM[colorRAMAddr++];
		red = data & 0xC0;				/* Red */
		green = (data & 0x30) << 2;		/* Green */
		blue = (data & 0x0C) << 4;		/* Blue */
		/* intensity = data & 0x03; */

#ifdef __MWERKS__
		qixpal[i] = new_osd_obtain_pen( red, green, blue );
#else
		qixpal[i] = osd_obtain_pen( red, green, blue );
#endif
	}

	/* refresh the bitmap with new colors */
	for( i=0; i<256; i++ )
	{
		bm = tmpbitmap->line[i];
		for( x=0; x<256; x++ )
		{
			*bm++ = qixpal[ screen[ (x<<8) + (255-i) ] ];
		}
	}
}

/* JB 970524 */
void qix_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	/* copy the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}