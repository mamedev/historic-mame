/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *qix_paletteram,*qix_palettebank;
unsigned char *qix_videoaddress;
static unsigned char qixpal[256];
static unsigned char *screen;
static int dirtypalette;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  Qix doesn't have colors PROMs, it uses RAM. The meaning of the bits are
  bit 7 -- Red
        -- Red
        -- Green
        -- Green
        -- Blue
        -- Blue
        -- Intensity
  bit 0 -- Intensity

***************************************************************************/
void qix_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 256;i++)
	{
		/* this conversion table is probably quite wrong, but at least it gives */
		/* a reasonable gray scale in the test screen. However, in the very same */
		/* test screen the red, green and blue squares are almost invisible since */
		/* they are very dark (value = 1, intensity = 0) */
		static unsigned char table[16] =
		{
			0x00,	/* value = 0, intensity = 0 */
			0x12,	/* value = 0, intensity = 1 */
			0x24,	/* value = 0, intensity = 2 */
			0x49,	/* value = 0, intensity = 3 */
			0x12,	/* value = 1, intensity = 0 */
			0x24,	/* value = 1, intensity = 1 */
			0x49,	/* value = 1, intensity = 2 */
			0x92,	/* value = 1, intensity = 3 */
			0x5b,	/* value = 2, intensity = 0 */
			0x6d,	/* value = 2, intensity = 1 */
			0x92,	/* value = 2, intensity = 2 */
			0xdb,	/* value = 2, intensity = 3 */
			0x7f,	/* value = 3, intensity = 0 */
			0x91,	/* value = 3, intensity = 1 */
			0xb6,	/* value = 3, intensity = 2 */
			0xff	/* value = 3, intensity = 3 */
		};
		int bits,intensity;


		intensity = (i >> 0) & 0x03;
		bits = (i >> 6) & 0x03;
		palette[3*i] = table[(bits << 2) | intensity];
		bits = (i >> 4) & 0x03;
		palette[3*i + 1] = table[(bits << 2) | intensity];
		bits = (i >> 2) & 0x03;
		palette[3*i + 2] = table[(bits << 2) | intensity];
	}
}



/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int qix_vh_start(void)
{
	dirtypalette = 1;

	if ((screen = malloc(256*256)) == 0)
		return 1;

	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
	{
		free(screen);
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void qix_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
	free(screen);
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
	offset += (qix_videoaddress[0] & 0x80) * 0x100;
	return screen[offset];
}

void qix_videoram_w(int offset,int data)
{
	int x, y;


	offset += (qix_videoaddress[0] & 0x80) * 0x100;

	/* bitmap is rotated -90 deg. */
	x = offset >> 8;
	y = ~offset & 0xff;
	tmpbitmap->line[y][x] = qixpal[data];

	screen[offset] = data;
}



int qix_addresslatch_r(int offset)
{
	offset = qix_videoaddress[0] * 0x100 + qix_videoaddress[1];
	return screen[offset];
}



void qix_addresslatch_w(int offset,int data)
{
	int x, y;


	offset = qix_videoaddress[0] * 0x100 + qix_videoaddress[1];

	/* bitmap is rotated -90 deg. */
	x = offset >> 8;
	y = ~offset & 0xff;
	tmpbitmap->line[y][x] = qixpal[data];

	screen[offset] = data;
}



/* The color RAM works as follows.  The color RAM contains palette values for
four pages (0-3).  When a write to $8800 on the video CPU occurs, the color
RAM page is taken from the lowest 2 bits of the value.  This selects one of
the color RAM pages as follows:

     colorRAMAddr = 0x9000 + ((data & 0x03) * 0x100);

Qix uses a palette of 64 colors (2 each RGB) and four intensities (RRGGBBII).
*/
void qix_paletteram_w(int offset,int data)
{
	if (qix_paletteram[offset] != data)
	{
		dirtypalette = 1;
		qix_paletteram[offset] = data;
	}
}



void qix_palettebank_w(int offset,int data)
{
	if ((*qix_palettebank & 0x03) != (data & 0x03))
		dirtypalette = 1;

	*qix_palettebank = data;
}



void qix_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	if (dirtypalette)
	{
		int i,x;
		unsigned char *bm,*pram;


		dirtypalette = 0;

		pram = &qix_paletteram[256 * (*qix_palettebank & 0x03)];

		for (i = 0;i < 256;i++)
			qixpal[i] = Machine->pens[*pram++];


		/* refresh the bitmap with new colors */
		for (i = 0;i < 256;i++)
		{
			bm = tmpbitmap->line[i];
			for (x = 0;x < 256;x++)
				*bm++ = qixpal[screen[(x << 8) + (255 - i)]];
		}
	}


	/* copy the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
}
