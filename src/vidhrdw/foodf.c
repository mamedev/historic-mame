/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#define PALETTE_SIZE 256


/*
 *		Globals we own
 */

int foodf_playfieldram_size;
int foodf_spriteram_size;

unsigned char *foodf_playfieldram;
unsigned char *foodf_paletteram;
unsigned char *foodf_spriteram;


/*
 *		Statics
 */

static unsigned char *playfielddirty;

static struct osd_bitmap *playfieldbitmap;


/*
 *		Prototypes from other modules
 */

void foodf_vh_stop (void);


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Food Fight doesn't have a color PROM. It uses 256 bytes of RAM to
  dynamically create the palette. Each byte defines one
  color (3-3-2 bits per R-G-B).
  Graphics use 2 bitplanes.

***************************************************************************/
void foodf_vh_convert_color_prom (unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;
	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		*(palette++) = ((i & 1) >> 0) * 0xff;
		*(palette++) = ((i & 2) >> 1) * 0xff;
		*(palette++) = ((i & 4) >> 2) * 0xff;
	}

	/* characters and sprites use the same palette */
	for (i = 0;i < TOTAL_COLORS(0);i++)
		COLOR(0,i) = i;
}


/*
 *   video system start; we also initialize the system memory as well here
 */

int foodf_vh_start(void)
{
	/* allocate dirty buffers */
	if (!playfielddirty) playfielddirty = malloc (foodf_playfieldram_size / 2);
	if (!playfielddirty)
	{
		foodf_vh_stop ();
		return 1;
	}
	memset (playfielddirty, 1, foodf_playfieldram_size / 2);

	/* allocate bitmaps */
	if (!playfieldbitmap) playfieldbitmap = osd_create_bitmap (32*8, 32*8);
	if (!playfieldbitmap)
	{
		foodf_vh_stop ();
		return 1;
	}

	return 0;
}


/*
 *   video system shutdown; we also bring down the system memory as well here
 */

void foodf_vh_stop(void)
{
	/* free bitmaps */
	if (playfieldbitmap) osd_free_bitmap (playfieldbitmap); playfieldbitmap = 0;

	/* free dirty buffers */
	if (playfielddirty) free (playfielddirty); playfielddirty = 0;
}


/*
 *   playfield RAM read/write handlers
 */

int foodf_playfieldram_r (int offset)
{
	return READ_WORD (&foodf_playfieldram[offset]);
}

void foodf_playfieldram_w (int offset, int data)
{
	int oldword = READ_WORD (&foodf_playfieldram[offset]);
	int newword = COMBINE_WORD (oldword, data);
	
	if (oldword != newword)
	{
		WRITE_WORD (&foodf_playfieldram[offset], newword);
		playfielddirty[offset / 2] = 1;
	}
}


/*
 *   palette RAM read/write handlers
 */

int foodf_paletteram_r (int offset)
{
	offset = (offset / 2) % PALETTE_SIZE;
	return foodf_paletteram[offset];
}

void foodf_paletteram_w (int offset, int data)
{
	int red, green, blue;

	offset = (offset / 2) % PALETTE_SIZE;

	foodf_paletteram[offset] = data;

	/* extract RGB */
	red = (data >> 0) & 7;
	green = (data >> 3) & 7;
	blue = (data >> 6) & 3;

	/* up to 8 bits */
	red = (red << 5) | (red << 2) | (red >> 1);
	green = (green << 5) | (green << 2) | (green >> 1);
	blue = (blue << 6) | (blue << 4) | (blue << 2) | blue;

	osd_modify_pen (Machine->gfx[0]->colortable[offset], red, green, blue);
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void foodf_vh_screenrefresh (struct osd_bitmap *bitmap)
{
	int offs;

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (offs = foodf_playfieldram_size - 2; offs >= 0; offs -= 2)
	{
		int data = READ_WORD (&foodf_playfieldram[offs]);
		int color = (data >> 8) & 0x3f;

		if (playfielddirty[offs / 2])
		{
			int pict = (data & 0xff) | ((data >> 7) & 0x100);
			int sx,sy;

			playfielddirty[offs / 2] = 0;

			sx = ((offs/2) / 32 + 1) % 32;
			sy = (offs/2) % 32;

			drawgfx (playfieldbitmap, Machine->gfx[0],
					pict, color,
					0, 0,
					8*sx, 8*sy,
					0,
					TRANSPARENCY_NONE, 0);
		}
	}
	copybitmap (bitmap, playfieldbitmap, 0, 0, 0, 0, &Machine->drv->visible_area, TRANSPARENCY_NONE, 0);

	/* walk the motion object list. */
	for (offs = 0; offs < foodf_spriteram_size; offs += 4)
	{
		int data1 = READ_WORD (&foodf_spriteram[offs + 0x0000]);
		int data2 = READ_WORD (&foodf_spriteram[offs + 0x0002]);

		int pict = data1 & 0xff;
		int color = (data1 >> 8) & 0x1f;
		int xpos = (data2 >> 8) & 0xff;
		int ypos = (0xff - data2 - 16) & 0xff;
		int hflip = (data1 >> 15) & 1;
		int vflip = (data1 >> 14) & 1;

		drawgfx (bitmap, Machine->gfx[1],
				pict, color,
				hflip, vflip,
				xpos, ypos,
				&Machine->drv->visible_area,
				TRANSPARENCY_PEN, 0);
	}
}
