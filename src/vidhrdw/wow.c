/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mame.h"
#include "common.h"
#include "driver.h"
#include "machine.h"
#include "osdepend.h"



unsigned char *wow_videoram;
int mask,unknown,collision;

static struct osd_bitmap *tmpbitmap;


static struct rectangle visiblearea =
{
	0*8, 40*8-1,
	0*8, 204-1
};



int wow_vh_start(void)
{
	int i;


	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	for (i = 0;i < tmpbitmap->height;i++)
		memset(tmpbitmap->line[i],Machine->background_pen,tmpbitmap->width);

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void wow_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



int wow_collision_r(int offset)
{
	return collision;
}



void wow_videoram_w(int offset,int data)
{
	if (wow_videoram[offset] != data)
	{
		int i;
		unsigned char *bm;


		wow_videoram[offset] = data;

		if (offset >= 2*40*204) return;

		if ((offset & 1) == 0)
		{
			offset /= 2;
			bm = tmpbitmap->line[offset / 40] + 8 * (offset % 40);
		}
		else
		{
			offset /= 2;
			bm = tmpbitmap->line[offset / 40] + 8 * (offset % 40) + 4;
		}

		for (i = 0;i < 4;i++)
		{
			*bm = 0;

			if (data & 0x80) *bm |= 1;
			if (data & 0x40) *bm |= 2;
/// TODO: remap the pixel color thru the color table

			bm++;
			data <<= 2;
		}
	}
}



void wow_mask_w(int offset,int data)
{
if (errorlog) fprintf(errorlog,"%04x: mask = %02x\n",Z80_GetPC(),data);
	mask = data;
}



void wow_unknown_w(int offset,int data)
{
if (errorlog) fprintf(errorlog,"%04x: unknown = %02x\n",Z80_GetPC(),data);
	unknown = data;
}



static void copywithflip(int offset,int data)
{
	if (unknown & 0x40)	/* copy backwards */
	{
		int bits,stib,k;

		bits = data;
		stib = 0;
		for (k = 0;k < 4;k++)
		{
			stib >>= 2;
			stib |= (bits & 0xc0);
			bits <<= 2;
		}

		data = stib;
	}

	if (unknown & 0x40)	/* copy backwards */
	{
		int shift,data1,mask;


		shift = unknown & 3;
		data1 = 0;
		mask = 0xff;
		while (shift > 0)
		{
			data1 <<= 2;
			data1 |= (data & 0xc0) >> 6;
			data <<= 2;
			mask <<= 2;
			shift--;
		}

		if ((mask & wow_videoram[offset]) || (~mask & wow_videoram[offset-1])) collision = 1;
		else collision = 0;

		if (unknown & 0x20) data ^= wow_videoram[offset];	/* draw in XOR mode */
		else data |= ~mask & wow_videoram[offset];	/* draw in copy mode */
		wow_videoram_w(offset,data);
		if (unknown & 0x20) data1 ^= wow_videoram[offset-1];	/* draw in XOR mode */
		else data1 |= mask & wow_videoram[offset-1];	/* draw in copy mode */
		wow_videoram_w(offset-1,data1);
	}
	else
	{
		int shift,data1,mask;


		shift = unknown & 3;
		data1 = 0;
		mask = 0xff;
		while (shift > 0)
		{
			data1 >>= 2;
			data1 |= (data & 0x03) << 6;
			data >>= 2;
			mask >>= 2;
			shift--;
		}

		if ((mask & wow_videoram[offset]) || (~mask & wow_videoram[offset+1])) collision = 1;
		else collision = 0;

		if (unknown & 0x20)
			data ^= wow_videoram[offset];	/* draw in XOR mode */
		else
			data |= ~mask & wow_videoram[offset];	/* draw in copy mode */
		wow_videoram_w(offset,data);
		if (unknown & 0x20)
			data1 ^= wow_videoram[offset+1];	/* draw in XOR mode */
		else
			data1 |= mask & wow_videoram[offset+1];	/* draw in copy mode */
		wow_videoram_w(offset+1,data1);
	}
}



void wow_masked_videoram_w(int offset,int data)
{
	if ((unknown & 0x9c) == 0x08	/* copy 1 bitplane with color */
			|| (unknown & 0x9c) == 0x18)
	{
		int bits,bibits,k;

static int count;

count ^= 1;
if (!count) return;

		bits = data;
		bibits = 0;
		for (k = 0;k < 4;k++)
		{
			bibits <<= 2;
			if (bits & 0x80) bibits |= 0x03;
			bits <<= 1;
		}
		if (mask == 0) bibits = 0;
		else if (mask == 4) bibits &= 0x55;
		else if (mask == 8) bibits &= 0xaa;

		if (unknown & 0x40)	/* copy backwards */
			copywithflip(offset+1,bibits);
		else
			copywithflip(offset,bibits);

		bits = data;
		bibits = 0;
		for (k = 0;k < 4;k++)
		{
			bibits <<= 2;
			if (bits & 0x08) bibits |= 0x03;
			bits <<= 1;
		}
		if (mask == 0) bibits = 0;
		else if (mask == 4) bibits &= 0x55;
		else if (mask == 8) bibits &= 0xaa;

		if (unknown & 0x40)	/* copy backwards */
			copywithflip(offset,bibits);
		else
			copywithflip(offset+1,bibits);
	}
	else
	{
		copywithflip(offset,data);
	}
}



void wow_blitter_w(int offset,int data)
{
	static int src;
	static int mode;	/* ?? */
	static int skip;	/* bytes to skip after row copy */
	static int dest;
	static int length;	/* row length */
	static int loops;	/* rows to copy - 1 */


	switch (offset)
	{
		case 0:
			src = data;
			break;
		case 1:
			src = src + data * 256;
			break;
		case 2:
			mode = data & 0x3f;	/* register is 6 bit wide */
			break;
		case 3:
			skip = data;
			break;
		case 4:
			dest = skip + data * 256;	/* register 3 is shared between skip and dest */
			break;
		case 5:
			length = data;
			break;
		case 6:
			loops = data;
			break;
	}

	if (offset == 6)	/* trigger blit */
	{
		int i,j;


if (errorlog) fprintf(errorlog,"%04x: blit src %04x mode %02x skip %d dest %04x length %d loops %d\n",
		Z80_GetPC(),src,mode,skip,dest,length,loops);

//		if ((mode & 0x09) == 0x00)	/* copy from 1 bitplane */
//		else if ((mode & 0x09) == 0x08)	/* copy from 2 bitplanes */

		for (i = 0; i <= loops;i++)
		{
			for (j = 0;j <= length;j++)
			{
if (!(mode & 0x08) || j < length)
				Z80_WRMEM(dest,RAM[src]);
				if (mode & 0x20) dest++;	/* copy forwards */
				else dest--;				/* backwards */

				if ((j & 1) || !(mode & 0x02))	/* don't increment source on odd loops */
					if (mode & 0x04) src++;
			}

			if ((j & 1) && (mode & 0x02))	/* always increment source at end of line */
				if (mode & 0x04) src++;
if (mode & 0x08) src--;

if (mode & 0x20) dest--;	/* copy forwards */
else dest++;				/* backwards */

			dest += (int)((signed char)skip);	/* extend the sign of the skip register */

		/* Note: actually the hardware doesn't handle the sign of the skip register, */
		/* when incrementing the destination address the carry bit is taken from the */
		/* mode register. To faithfully emulate the hardware I should do: */
#if 0
			{
				int lo,hi;

				lo = dest & 0x00ff;
				hi = dest & 0xff00;
				lo += skip;
				if (mode & 0x10)
				{
					if (lo < 0x100) hi -= 0x100;
				}
				else
				{
					if (lo > 0xff) hi += 0x100;
				}
				dest = hi | (lo & 0xff);
			}
#endif
		}
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void wow_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&visiblearea,TRANSPARENCY_NONE,0);
}
