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



void wow_masked_videoram_w(int offset,int data)
{
	if (offset < 0 || offset >= 0x4000) return;

	if ((unknown & 0xf) == 8)
	{
		if (mask == 0) data = 0;
		else if (mask == 4) data &= 0x55;
		else if (mask == 8) data &= 0xaa;
	}

	if (unknown & 0x20) data ^= wow_videoram[offset];	/* draw in XOR mode */

	if (wow_videoram[offset]) collision = 1;
	else collision = 0;

	wow_videoram_w(offset,data);
}



void wow_blitter_mask_w(int offset,int data)
{
if (errorlog) fprintf(errorlog,"%04x: mask = %02x\n",Z80_GetPC(),data);
	mask = data;
}



void wow_blitter_unknown_w(int offset,int data)
{
if (errorlog) fprintf(errorlog,"%04x: unknown = %02x\n",Z80_GetPC(),data);
	unknown = data;
}



void wow_blitter_w(int offset,int data)
{
	static int src;
	static int mode;	/* ?? */
	static int skip;	/* bytes to skip after row copy */
	static int dest;
	static int length;	/* row length */
	static int loops;	/* rows to copy - 1 */


//if (errorlog) fprintf(errorlog,"%04x: write %02x to I/O port %02x\n",Z80_GetPC(),data,offset+0x78);

	switch (offset)
	{
		case 0:
			src = data;
			break;
		case 1:
			src = src + data * 256;
			break;
		case 2:
			mode = data;
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


		skip = (int)((signed char)skip);	/* extend the sign */

if (errorlog) fprintf(errorlog,"%04x: blit src %04x mode %02x skip %d dest %04x length %d loops %d\n",
		Z80_GetPC(),src,mode,skip,dest,length,loops);

		if (mode == 0x22)	/* fill with mask */
		{
			for (i = 0; i <= loops;i++)
			{
				for (j = 0;j <= length;j++)
				{
					int bits,bibits,k;


					bits = RAM[src];
					if (j & 1)
					{
						bibits = 0;
						for (k = 0;k < 4;k++)
						{
							bibits <<= 2;
							if (bits & 0x08) bibits |= 0x03;
							bits <<= 1;
						}
					}
					else
					{
						bibits = 0;
						for (k = 0;k < 4;k++)
						{
							bibits <<= 2;
							if (bits & 0x80) bibits |= 0x03;
							bits <<= 1;
						}
					}

					wow_masked_videoram_w(dest + j,bibits);
				}
				dest += skip + length;
			}
		}
		else if (mode == 0x26)	/* copy from 1 bitplane with mask */
		{
			for (i = 0; i <= loops;i++)
			{
				for (j = 0;j < 2*length;j++)
				{
					int bits,bibits,k;


					bits = RAM[src+j/2];
					if (j & 1)
					{
						bibits = 0;
						for (k = 0;k < 4;k++)
						{
							bibits <<= 2;
							if (bits & 0x08) bibits |= 0x03;
							bits <<= 1;
						}
					}
					else
					{
						bibits = 0;
						for (k = 0;k < 4;k++)
						{
							bibits <<= 2;
							if (bits & 0x80) bibits |= 0x03;
							bits <<= 1;
						}
					}

					wow_masked_videoram_w(dest + j,bibits);
				}
				dest += skip + length;
				src += length;
			}
		}
		else if (mode == 0x2c || mode == 0x3c)	/* copy from 2 bitplanes in XOR mode */
		{
			for (i = 0; i <= loops;i++)
			{
				for (j = 0;j < length;j++)
				{
					wow_masked_videoram_w(dest + j,RAM[src+j]);
				}
				dest += skip + length;
				src += length;
			}
		}
//		else if (mode == 0x16)	/* copy 1 bitplane backwards */
		else if (mode == 0x0c || mode == 0x1c)	/* copy 2 bitplanes backwards in XOR mode */
		{
			for (i = 0; i <= loops;i++)
			{
				for (j = 0;j < length;j++)
				{
					int bits,stib,k;

					bits = RAM[src+j];
					stib = 0;
					for (k = 0;k < 4;k++)
					{
						stib >>= 2;
						stib |= (bits & 0xc0);
						bits <<= 2;
					}

					wow_masked_videoram_w(dest - j,stib);
				}
				dest += skip - length;
				src += length;
			}
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
