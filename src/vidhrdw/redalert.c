/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *redalert_backram;
unsigned char *redalert_spriteram1;
unsigned char *redalert_spriteram2;
unsigned char *redalert_characterram;

static unsigned char redalert_dirtyback[0x400];
static unsigned char redalert_dirtycharacter[0x100];

/***************************************************************************
redalert_backram_w
***************************************************************************/

void redalert_backram_w(int offset,int data)
{
	if (redalert_backram[offset] != data)
	{
		redalert_dirtyback[offset / 8 % 0x400] = 1;
		dirtybuffer[offset / 8 % 0x400] = 1;

		redalert_backram[offset] = data;
	}
}

/***************************************************************************
redalert_spriteram1_w
***************************************************************************/

void redalert_spriteram1_w(int offset,int data)
{
	if (redalert_spriteram1[offset] != data)
	{
		redalert_dirtycharacter[((offset / 8) % 0x80) + 0x80] = 1;

		redalert_spriteram1[offset] = data;
	}
}

/***************************************************************************
redalert_spriteram2_w
***************************************************************************/

void redalert_spriteram2_w(int offset,int data)
{
	if (redalert_spriteram2[offset] != data)
	{
		redalert_dirtycharacter[((offset / 8) % 0x80) + 0x80] = 1;

		redalert_spriteram2[offset] = data;
	}
}

/***************************************************************************
redalert_characterram_w
***************************************************************************/

void redalert_characterram_w(int offset,int data)
{
	if (redalert_characterram[offset] != data)
	{
		redalert_dirtycharacter[((offset / 8) % 0x80)] = 1;

		redalert_characterram[offset] = data;
	}
}

/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void redalert_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs,i;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		int charcode;


		charcode = videoram[offs];

		if (dirtybuffer[offs] || redalert_dirtycharacter[charcode])
		{
			int sx,sy,color;


			/* decode modified background */
			if (redalert_dirtyback[offs] == 1)
			{
				decodechar(Machine->gfx[0],offs,redalert_backram,
							Machine->drv->gfxdecodeinfo[0].gfxlayout);
				redalert_dirtyback[offs] = 2;
			}

			/* decode modified characters */
			if (redalert_dirtycharacter[charcode] == 1)
			{
				if (charcode < 0x80)
					decodechar(Machine->gfx[1],charcode,redalert_characterram,
								Machine->drv->gfxdecodeinfo[1].gfxlayout);
				else
					decodechar(Machine->gfx[2],charcode-0x80,redalert_spriteram1,
								Machine->drv->gfxdecodeinfo[2].gfxlayout);
				redalert_dirtycharacter[charcode] = 2;
			}


			dirtybuffer[offs] = 0;

			sx = 8 * (offs % 32);
			sy = 8 * (offs / 32);

			/* First layer of color */
			if (charcode >= 0xC0)
			{
				color = 0;
				drawgfx(tmpbitmap,Machine->gfx[2],
						charcode-0x80,color,
						0,0,sx,sy,
						&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
			}
			else
			{
				/* Second layer - background */
				color = 3;
				drawgfx(tmpbitmap,Machine->gfx[0],
						offs,color,
						0,0,sx,sy,
						&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
			}

			/* Second layer - background */
			color = 3;
			drawgfx(tmpbitmap,Machine->gfx[0],
					offs,color,
					0,0,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);

			/* Third layer - alphanumerics & sprites */
			if (charcode < 0x80)
			{
				color = 6;
				drawgfx(tmpbitmap,Machine->gfx[1],
						charcode,color,
						0,0,sx,sy,
						&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
			}
			else if (charcode < 0xC0)
			{
				color = 0;
				drawgfx(tmpbitmap,Machine->gfx[2],
						charcode-0x80,color,
						0,0,sx,sy,
						&Machine->drv->visible_area,TRANSPARENCY_COLOR,0);
			}

		}
	}

	for (i = 0;i < 256;i++)
	{
		if (redalert_dirtycharacter[i] == 2)
			redalert_dirtycharacter[i] = 0;
	}

	for (i = 0;i < 0x400;i++)
	{
		if (redalert_dirtyback[i] == 2)
			redalert_dirtyback[i] = 0;
	}

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

}

