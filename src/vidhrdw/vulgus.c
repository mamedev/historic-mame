/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *vulgus_bgvideoram,*vulgus_bgcolorram;
int vulgus_bgvideoram_size;
unsigned char *vulgus_scrolllow,*vulgus_scrollhigh;
static unsigned char *dirtybuffer2;
static struct osd_bitmap *tmpbitmap2;



/***************************************************************************

  Convert the color PROMs into a more useable format.

  This function comes from 1942. It is yet unknown how Vulgus works.

***************************************************************************/
void vulgus_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
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

  Start the video hardware emulation.

***************************************************************************/
int vulgus_vh_start(void)
{
	if (generic_vh_start() != 0)
		return 1;

	if ((dirtybuffer2 = malloc(vulgus_bgvideoram_size)) == 0)
	{
		generic_vh_stop();
		return 1;
	}
	memset(dirtybuffer2,0,vulgus_bgvideoram_size);

	/* the background area is twice as tall and twice as large as the screen */
	if ((tmpbitmap2 = osd_create_bitmap(2*Machine->drv->screen_width,2*Machine->drv->screen_height)) == 0)
	{
		free(dirtybuffer2);
		generic_vh_stop();
		return 1;
	}

	return 0;
}



/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void vulgus_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap2);
	free(dirtybuffer2);
	generic_vh_stop();
}



void vulgus_bgvideoram_w(int offset,int data)
{
	if (vulgus_bgvideoram[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		vulgus_bgvideoram[offset] = data;
	}
}



void vulgus_bgcolorram_w(int offset,int data)
{
	if (vulgus_bgcolorram[offset] != data)
	{
		dirtybuffer2[offset] = 1;

		vulgus_bgcolorram[offset] = data;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void vulgus_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	for (offs = vulgus_bgvideoram_size - 1;offs >= 0;offs--)
	{
		int sx,sy;


		if (dirtybuffer2[offs])
		{
			dirtybuffer2[offs] = 0;

			sx = offs % 32;
			sy = 31 - offs / 32;

			drawgfx(tmpbitmap2,Machine->gfx[(vulgus_bgcolorram[offs] & 0x80) ? 2 : 1],
					vulgus_bgvideoram[offs],
					vulgus_bgcolorram[offs] & 0x1f,
					vulgus_bgcolorram[offs] & 0x40,vulgus_bgcolorram[offs] & 0x20,
					16 * sx,16 * sy,
					0,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the background graphics */
	{
		int scrollx,scrolly;


		scrollx = -(vulgus_scrolllow[0] + 256 * vulgus_scrollhigh[0]);
		scrolly = vulgus_scrolllow[1] + 256 * vulgus_scrollhigh[1] - 256;

		copyscrollbitmap(bitmap,tmpbitmap2,1,&scrollx,1,&scrolly,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
	}


	/* Draw the sprites. */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int bank,i,code,col,sx,sy;


		bank = 3;
		if (spriteram[offs] & 0x80) bank++;

		code = spriteram[offs] & 0x7f;
		col = spriteram[offs + 1] & 0x0f;
		sx = spriteram[offs + 2];
		sy = 240 - spriteram[offs + 3] + 0x10 * (spriteram[offs + 1] & 0x10);

		i = (spriteram[offs + 1] & 0xc0) >> 6;
		if (i == 2) i = 3;

		do
		{
			drawgfx(bitmap,Machine->gfx[bank],
					code + i,col,
					0, 0,
					sx + 16 * i,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,15);

			i--;
		} while (i >= 0);
	}


	/* draw the frontmost playfield. They are characters, but draw them as sprites */
	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (videoram[offs] != 0x20)	/* don't draw spaces */
		{
			int sx,sy;


			sx = 8 * (offs / 32);
			sy = 8 * (31 - offs % 32);

			drawgfx(bitmap,Machine->gfx[0],
					videoram[offs] + 2 * (colorram[offs] & 0x80),
					colorram[offs] & 0x0f,
					0,0,sx,sy,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
		}
	}
}
