/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


unsigned char *sonson_scrollx;

#define GFX_SPRITE 4

/***************************************************************************

  Convert the color PROMs into a more useable format.

  This function comes from 1942. It is yet unknown how Son Son works.

***************************************************************************/
void sonson_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;


	for (i = 0;i < 256;i++)
	{
		int bit0,bit1,bit2,bit3;


		bit0 = (color_prom[i] >> 0) & 0x01;
		bit1 = (color_prom[i] >> 1) & 0x01;
		bit2 = (color_prom[i] >> 2) & 0x01;
		bit3 = (color_prom[i] >> 3) & 0x01;
		palette[3*i] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[i+256] >> 0) & 0x01;
		bit1 = (color_prom[i+256] >> 1) & 0x01;
		bit2 = (color_prom[i+256] >> 2) & 0x01;
		bit3 = (color_prom[i+256] >> 3) & 0x01;
		palette[3*i + 1] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[i+256*2] >> 0) & 0x01;
		bit1 = (color_prom[i+256*2] >> 1) & 0x01;
		bit2 = (color_prom[i+256*2] >> 2) & 0x01;
		bit3 = (color_prom[i+256*2] >> 3) & 0x01;
		palette[3*i + 2] = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
	}

	/* characters use colors 128-143 */
	for (i = 0;i < 64*4;i++)
		colortable[i] = color_prom[i + 256*3] + 128;

	/* sprites use colors 64-79 */
	for (i = 64*4;i < 64*4+16*16;i++)
		colortable[i] = color_prom[i + 256*3] + 64;

	/* background tiles use colors 0-63 in four banks */
	for (i = 64*4+16*16;i < 64*4+16*16+32*8;i++)
	{
		colortable[i] = color_prom[i + 256*3];
		colortable[i+32*8] = color_prom[i + 256*3] + 16;
		colortable[i+2*32*8] = color_prom[i + 256*3] + 32;
		colortable[i+3*32*8] = color_prom[i + 256*3] + 48;
	}
}





/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void sonson_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = 8 * (offs % 32);
			sy = 8 * (offs / 32);

			drawgfx(tmpbitmap,Machine->gfx[colorram[offs] & 3],
					videoram[offs],
					colorram[offs] >> 4,
					0,0,
					sx,sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the background graphics */
	{
		int i,scroll[32];

		for (i = 0;i < 5;i++)
			scroll[i] = 0;
		for (i = 5;i < 32;i++)
			scroll[i] = -(*sonson_scrollx);

		copyscrollbitmap(bitmap,tmpbitmap,32,scroll,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}

        for (offs = spriteram_size-4; offs >= 0; offs-=4)
        {
                if (spriteram[offs+2] != 0xf8) {
		   int bank = (spriteram[offs+1] >> 5) & 1;
		   drawgfx(bitmap,Machine->gfx[GFX_SPRITE + bank],
					   spriteram[offs + 2],
                                           spriteram[offs + 1] & 0x1f,
					   !(spriteram[offs + 1] & 0x40),!(spriteram[offs+1] & 0x80),
					   spriteram[offs + 3],spriteram[offs + 0],
					   &Machine->drv->visible_area,TRANSPARENCY_PEN,0);
                }
        }
}
