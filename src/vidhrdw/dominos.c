/***************************************************************************

	Atari Dominos hardware

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "dominos.h"

unsigned char *dominos_sound_ram;


VIDEO_UPDATE( dominos )
{
	int offs;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int charcode;
			int sx,sy;

			dirtybuffer[offs]=0;

			charcode = videoram[offs] & 0x3F;

			sx = 8 * (offs % 32);
			sy = 8 * (offs / 32);
			drawgfx(tmpbitmap,Machine->gfx[0],
					charcode, (videoram[offs] & 0x80)>>7,
					0,0,sx,sy,
					&Machine->visible_area,TRANSPARENCY_NONE,0);
		}
	}

	/* copy the character mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);

	/* The video circuitry updates our sound registers. */
	discrete_sound_w(0, dominos_sound_ram[0] % 16);	// Freq
	discrete_sound_w(1, dominos_sound_ram[2] % 16);	// Amp
}
