/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

/*#define TOOBIN_DEBUG 1*/


#define XCHARS 64
#define YCHARS 48

#define XDIM (XCHARS * 8)
#define YDIM (YCHARS * 8)

#define MAX_MOSLIP (384 / 8)


extern int toobin_scanline (void);


/*************************************
 *
 *		Globals we own
 *
 *************************************/

unsigned char *toobin_intensity;
unsigned char *toobin_playfieldram;
unsigned char *toobin_spriteram;
unsigned char *toobin_alpharam;
unsigned char *toobin_paletteram;
unsigned char *toobin_vscroll;
unsigned char *toobin_hscroll;
unsigned char *toobin_moslip;

int toobin_playfieldram_size;
int toobin_spriteram_size;
int toobin_alpharam_size;
int toobin_paletteram_size;



/*************************************
 *
 *		Statics
 *
 *************************************/

static unsigned char *playfielddirty;
static unsigned char *colordirty;
static unsigned char *spritevisit;
static unsigned char *highpriority;
static unsigned short *displaylist[2];

static unsigned short *displaylist_end[2];
static int current_frame;

static struct osd_bitmap *playfieldbitmap;

static int playfield_count[16];
static int last_map[16], this_map[16];

static int intensity_dirty;




/*************************************
 *
 *		Prototypes from other modules
 *
 *************************************/

void toobin_vh_stop (void);
void toobin_sound_reset (void);

#ifdef TOOBIN_DEBUG
static void toobin_dump_video_ram (void);
#endif


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Toobin' doesn't have a color PROM. It uses 1024 words of RAM to
  dynamically create the palette. Each word defines one color (5 bits
  per pixel). However, more than 256 colors is rarely used, so the
  video system has a dynamic palette mapping function

***************************************************************************/



/*************************************
 *
 *		Video system start
 *
 *************************************/

int toobin_vh_start(void)
{
	/* allocate dirty buffers */
	if (!playfielddirty)
		playfielddirty = calloc (toobin_playfieldram_size / 4 +
		                         toobin_paletteram_size / 2 +
		                         toobin_spriteram_size / 8 +
		                         2 * (XCHARS + 1) * (YCHARS + 1) +
		                         2 * 8 * toobin_spriteram_size, 1);
	if (!playfielddirty)
	{
		toobin_vh_stop ();
		return 1;
	}

	/* dole out the memory */
	colordirty = playfielddirty + toobin_playfieldram_size / 4;
	spritevisit = colordirty + toobin_paletteram_size / 2;
	highpriority = spritevisit + toobin_spriteram_size / 8;
	displaylist[0] = (unsigned short *)(highpriority + 2 * (XCHARS + 1) * (YCHARS + 1));
	displaylist[1] = (unsigned short *)((char *)displaylist[0] + 8 * toobin_spriteram_size);

	/* allocate bitmaps */
	if (!playfieldbitmap)
		playfieldbitmap = osd_new_bitmap (128*8, 64*8, Machine->scrbitmap->depth);
	if (!playfieldbitmap)
	{
		toobin_vh_stop ();
		return 1;
	}

	/* reset the color counter */
	memset (playfield_count, 0, sizeof (playfield_count));
	playfield_count[0] = 128 * 64;

	/* the current intensity is dirty */
	intensity_dirty = 1;

	/* no motion object lists */
	current_frame = 0;
	displaylist_end[0] = displaylist[0];
	displaylist_end[1] = displaylist[1];

	return 0;
}



/*************************************
 *
 *		Video system shutdown
 *
 *************************************/

void toobin_vh_stop(void)
{
	/* free bitmaps */
	if (playfieldbitmap)
		osd_free_bitmap (playfieldbitmap);
	playfieldbitmap = 0;

	/* free dirty buffers */
	if (playfielddirty)
		free (playfielddirty);
	playfielddirty = colordirty = spritevisit = highpriority = 0;
}



/*************************************
 *
 *		Playfield RAM read/write handlers
 *
 *************************************/

int toobin_playfieldram_r (int offset)
{
	return READ_WORD (&toobin_playfieldram[offset]);
}


void toobin_playfieldram_w (int offset, int data)
{
	int oldword = READ_WORD (&toobin_playfieldram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&toobin_playfieldram[offset], newword);

		/* mark this tile dirty */
		playfielddirty[offset / 4] = 1;

		/* if we're changing the color word, update the global counts */
		if (!(offset & 2))
		{
			playfield_count[oldword & 15]--;
			playfield_count[newword & 15]++;
		}
	}
}



/*************************************
 *
 *		Palette RAM read/write handlers
 *
 *************************************/

int toobin_paletteram_r (int offset)
{
	return READ_WORD (&toobin_paletteram[offset]);
}


void toobin_paletteram_w (int offset, int data)
{
	int oldword = READ_WORD (&toobin_paletteram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&toobin_paletteram[offset], newword);
		colordirty[offset / 2] = 1;
	}
}


void toobin_intensity_w (int offset, int data)
{
	int oldword = READ_WORD (&toobin_intensity[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if ((oldword & 0x1f) != (newword & 0x1f))
	{
		WRITE_WORD (&toobin_intensity[offset], newword);
		if (!offset)
			intensity_dirty = 1;
	}
}



/*************************************
 *
 *		Motion object list handlers
 *
 *************************************/

void update_display_list (int scanline)
{
	unsigned short *d = displaylist_end[current_frame];
	int link = READ_WORD (&toobin_moslip[0]) & 0xff;

	/* ignore updates after the end of the frame */
	if (scanline > 384)
		return;

	/* back up over any previous entries for the same scanline */
	while (d != displaylist[current_frame] && d[-4] == scanline)
		d -= 4;

	/* visit all the sprites and copy their data into the display list */
	memset (spritevisit, 0, toobin_spriteram_size / 8);
	while (!spritevisit[link])
	{
		*d++ = scanline;
		*d++ = READ_WORD (&toobin_spriteram[link * 8 + 0]);
		*d++ = READ_WORD (&toobin_spriteram[link * 8 + 2]);
		*d++ = READ_WORD (&toobin_spriteram[link * 8 + 6]);

		spritevisit[link] = 1;
		link = READ_WORD (&toobin_spriteram[link * 8 + 4]) & 0xff;
	}

	/* update the end of list */
	displaylist_end[current_frame] = d;
}


void toobin_moslip_w (int offset, int data)
{
	int scanline = toobin_scanline ();

	COMBINE_WORD_MEM (&toobin_moslip[offset], data);

	/* update the display list according to the current scanline */
	update_display_list (scanline);
}


void toobin_begin_frame (int param)
{
	/* update the counts in preparation for the new frame */
	current_frame ^= 1;
	displaylist_end[current_frame] = displaylist[current_frame];

	/* update the display list for the beginning of the frame */
	update_display_list (0);
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

/*
 *   dynamic palette remapper
 */

static inline void remap_color (int color)
{
	int i, j;

	/* first see if there is a match from last frame */
	for (i = 0; i < 16; i++)
		if (last_map[i] == color && this_map[i] < 0)
		{
			this_map[i] = color;
			return;
		}

	/* if not, pick a blank entry from last frame and use it */
	for (i = 0; i < 16; i++)
		if (last_map[i] < 0 && this_map[i] < 0)
		{
			this_map[i] = color;

			/* and update the colortable appropriately */
			for (j = 0; j < 16; j++)
				Machine->colortable[color * 16 + j] = Machine->pens[i * 16 + j];
			return;
		}

	/* if all else fails, pick a blank entry from this frame and use it */
	for (i = 0; i < 16; i++)
		if (this_map[i] < 0)
		{
			this_map[i] = color;

			/* and update the colortable appropriately */
			for (j = 0; j < 16; j++)
				Machine->colortable[color * 16 + j] = Machine->pens[i * 16 + j];
			return;
		}
}


/*
 *		the real deal
 */

void toobin_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int x, y, sx, sy, xoffs, yoffs, xscroll, yscroll, offs, i, intensity;
	unsigned char *hipri = highpriority, *p;
	int sprite_map[16], alpha_map[16];


	/* debugging */
#ifdef TOOBIN_DEBUG
	if (osd_key_pressed (OSD_KEY_9))
		toobin_dump_video_ram ();
#endif


	/* compute scrolling so we know what to update */
	xscroll = (READ_WORD (&toobin_hscroll[0]) >> 6);
	yscroll = (READ_WORD (&toobin_vscroll[0]) >> 6);
	xscroll = -(xscroll & 0x3ff);
	yscroll = -(yscroll & 0x1ff);


	/* playfield gets first color pick */
	for (i = 0; i < 16; i++)
	{
		last_map[i] = this_map[i];
		this_map[i] = playfield_count[i] ? i : -1;
		sprite_map[i] = alpha_map[i] = 0;
	}


	/*
	 * 	Playfield encoding
	 *
	 *		2 16-bit words are used
	 *
	 *		Word 1:
	 *
	 *			Bits 0-3   = color of the image
	 *			Bits 4-5   = priority of the image
	 *                      (could be just bit 5, indicating over sprites, but System 2 has two bits)
	 *
	 *		Word 2:
	 *
	 *			Bits 0-13  = index of the image
	 *			Bit  14    = horizontal flip
	 *			Bit  15    = vertical flip
	 *
	 */

	/* update only the portion of the playfield that's visible. */
	xoffs = -xscroll / 8;
	yoffs = -yscroll / 8;

	/* loop over the visible Y portion */
	for (y = yoffs + 48 + 1; y >= yoffs; y--)
	{
		sy = y & 63;

		/* loop over the visible X portion */
		for (x = xoffs + 64 + 1; x >= xoffs; x--)
		{
			int data1, priority;

			/* compute the offset */
			sx = x & 127;
			offs = sy * 128 + sx;

			data1 = READ_WORD (&toobin_playfieldram[offs * 4]);

			/* add this entry to the priority list if it is higher than 0 */
			priority = (data1 >> 4) & 3;
			if (priority)
				*hipri++ = sx, *hipri++ = sy;

			/* redraw if dirty */
			if (playfielddirty[offs])
			{
				int data2 = READ_WORD (&toobin_playfieldram[offs * 4 + 2]);
				int vflip = data2 & 0x8000;
				int hflip = data2 & 0x4000;
				int pict = data2 & 0x3fff;
				int color = data1 & 0x0f;

				drawgfx (playfieldbitmap, Machine->gfx[1],
						pict, color,
						hflip, vflip,
						8*sx, 8*sy,
						0,
						TRANSPARENCY_NONE, 0);

				playfielddirty[offs] = 0;
			}
		}
	}

	/* copy to the destination with scroll */
	copyscrollbitmap (bitmap, playfieldbitmap,
			1, &xscroll,
			1, &yscroll,
			&Machine->drv->visible_area,
			TRANSPARENCY_NONE, 0);


	/*
	 * 	Motion Object encoding
	 *
	 *		4 16-bit words are used
	 *
	 *		Word 1:
	 *
	 *			Bits 0-2   = width of the sprite / 16 (ranges from 1-8)
	 *			Bits 3-5   = height of the sprite / 16 (ranges from 1-8)
	 *			Bits 6-15  = Y position of the sprite
	 *
	 *		Word 2:
	 *
	 *			Bits 0-13  = index of the image (0-16383)
	 *			Bit  14    = horizontal flip
	 *			Bit  15    = vertical flip
	 *
	 *		Word 3:
	 *
	 *			Bits 0-7   = link to the next image to display
	 *
	 *		Word 4:
	 *
	 *			Bits 0-3   = sprite color
	 *			Bits 6-15  = X position of the sprite
	 *
	 */

	/* at this point, a display list has been created with all sprites that were linked to
	   during the frame; now we just need to render that list */
	{
		unsigned short *end = displaylist_end[current_frame ^ 1];
		unsigned short *d = displaylist[current_frame ^ 1];
		int last_start_scan = -1;
		struct rectangle clip;

		/* create a clipping rectangle so that only partial sections are updated at a time */
		clip.min_x = 0;
		clip.max_x = Machine->drv->screen_width - 1;

		/* loop over the list until the end */
		while (d < end)
		{
			unsigned short *df;
			int start_scan = *d++;
			int data1 = *d++;
			int data2 = *d++;
			int data4 = *d++;

			/* extract data from the various words */
			int xpos = xscroll + (data4 >> 6);
			int ypos = yscroll - (data1 >> 6);
			int hsize = (data1 & 7) + 1;
			int vsize = ((data1 >> 3) & 7) + 1;
			int hflip = data2 & 0x4000;
			int vflip = data2 & 0x8000;
			int pict = data2 & 0x3fff;
			int color = data4 & 0x0f;
			int xadv, yadv;

			/* adjust for h flip */
			if (hflip)
				xpos += (hsize - 1) * 16, xadv = -16;
			else
				xadv = 16;

			/* adjust for vflip */
			if (vflip)
				ypos -= 16, yadv = -16;
			else
				ypos -= vsize * 16, yadv = 16;

			/* keep them in range to start */
			xpos &= 0x3ff;
			ypos &= 0x1ff;

			/* if this is a new region of the screen, find the end and adjust our clipping */
			if (start_scan != last_start_scan)
			{
				last_start_scan = start_scan;
				clip.min_y = start_scan;

				/* look for an entry whose scanline start is different from ours; that's our bottom */
				for (df = d; df < end; df += 4)
					if (*df != start_scan)
					{
						clip.max_y = *df - 1;
						break;
					}

				/* if we didn't find any additional regions, go until the bottom of the screen */
				if (df == end)
					clip.max_y = Machine->drv->screen_height - 1;
			}

			/* loop over the width */
			for (x = 0, sx = xpos; x < hsize; x++, sx = (sx + xadv) & 0x3ff)
			{
				/* wrap and clip the X coordinate */
				if (sx > 0x3f0) sx -= 0x400;
				if (sx <= -16 || sx >= XCHARS*8)
				{
					pict += vsize;
					continue;
				}

				/* loop over the height */
				for (y = 0, sy = ypos; y < vsize; y++, sy = (sy + yadv) & 0x1ff, pict++)
				{
					/* wrap and clip the Y coordinate */
					if (sy > 0x1f0) sy -= 0x200;
					if (sy <= -16 || sy >= YCHARS*8)
						continue;

					/* if this sprite's color has not been used yet this frame, grab it */
					/* note that by doing it here, we don't map sprites that aren't visible */
					if (!sprite_map[color])
					{
						remap_color (color + 0x10);
						sprite_map[color] = 1;
					}

					/* draw the sprite */
					drawgfx (bitmap, Machine->gfx[2],
							pict, color, hflip, vflip, sx, sy, &clip, TRANSPARENCY_PEN, 0);
				}
			}
		}
	}


	/* redraw playfield tiles with higher priority */
	for (p = highpriority; p < hipri; p += 2)
	{
		int sx = p[0];
		int sy = p[1];
		int offs = sy * 128 + sx;

		int data1 = READ_WORD (&toobin_playfieldram[offs * 4]);
		int data2 = READ_WORD (&toobin_playfieldram[offs * 4 + 2]);
		int vflip = data2 & 0x8000;
		int hflip = data2 & 0x4000;
		int pict = data2 & 0x3fff;
		int color = data1 & 0x0f;

		/* compute the screen location with scrolling included */
		sx = (8*sx + xscroll) & 0x3ff;
		sy = (8*sy + yscroll) & 0x1ff;

		/* wrap and clip the X coordinate */
		if (sx > 0x3f8)
			sx -= 0x400;
		if (sx <= -8 || sx >= XCHARS*8)
			continue;

		/* wrap and clip the Y coordinate */
		if (sy > 0x1f8)
			sy -= 0x200;
		if (sy <= -8 || sy >= YCHARS*8)
			continue;

		/* redraw with pens 0-7 transparent */
		drawgfx (bitmap, Machine->gfx[1],
				pict, color,
				hflip, vflip,
				sx, sy,
				0,
				TRANSPARENCY_PENS, 0x000000ff);
	}


	/*
	 * 	Alpha layer encoding
	 *
	 *		1 16-bit word is used
	 *
	 *			Bits 0-9   = index of the character
	 *			Bits 12-14 = color??
	 *			Bit  15    = transparency??
	 *
	 */

	/* loop over the Y coordinate */
	for (sy = 0; sy < YCHARS; sy++)
	{
		/* loop over the X coordinate */
		for (sx = 0, offs = sy*64; sx < XCHARS; sx++, offs++)
		{
			int data = READ_WORD (&toobin_alpharam[offs*2]);
			int pict = data & 0x3ff;

			/* if there's a non-zero picture or if we're fully opaque, draw the tile */
			if (pict || (data & 0x8000))
			{
				int color = ((data >> 12) & 7);

				/* if this character's color has not been used yet this frame, grab it */
				/* note that by doing it here, we don't map characters that aren't visible */
				if (!alpha_map[color / 4])
				{
					remap_color (color / 4 + 0x20);
					alpha_map[color / 4] = 1;
				}

				/* draw the character */
				drawgfx (bitmap, Machine->gfx[0],
						pict, color,
						0, 0,
						8*sx, 8*sy,
						0,
						(data & 0x8000) ? TRANSPARENCY_NONE : TRANSPARENCY_PEN, 0);
			}
		}
	}


	/* compute the intensity */
	intensity = 0x20 - (READ_WORD (&toobin_intensity[0]) & 0x1f);
	if (intensity == 1)
		intensity = 0;

	/* update the active colors */
	for (i = 0; i < 16; i++)
		if (this_map[i] >= 0)
		{
			int index = this_map[i] * 16, pal = i * 16, j;

			/* modify the pens if this set is dirty or different than last frame */
			for (j = 0; j < 16; j++)
				if (colordirty[index + j] || this_map[i] != last_map[i] || intensity_dirty)
				{
					int palette = READ_WORD (&toobin_paletteram[(index + j) * 2]);

					int red = (((palette >> 10) & 0x1f) * intensity) >> 2;
					int green = (((palette >> 5) & 0x1f) * intensity) >> 2;
					int blue = ((palette & 0x1f) * intensity) >> 2;

					osd_modify_pen (Machine->pens[pal + j], red, green, blue);

					colordirty[index + j] = 0;
				}
		}

	/* intensity is no longer dirty */
	intensity_dirty = 0;
}



/*************************************
 *
 *		Debugging
 *
 *************************************/

#ifdef TOOBIN_DEBUG
static void toobin_dump_video_ram (void)
{
	static int count;
	char name[50];
	FILE *f;
	int i;

	while (osd_key_pressed (OSD_KEY_9)) { }

	sprintf (name, "Dump %d", ++count);
	f = fopen (name, "wt");

	fprintf (f, "\n\nPlayfield Palette:\n");
	for (i = 0x000; i < 0x100; i++)
	{
		fprintf (f, "%04X ", READ_WORD (&toobin_paletteram[i*2]));
		if ((i & 15) == 15) fprintf (f, "\n");
	}

	fprintf (f, "\n\nMotion Object Palette:\n");
	for (i = 0x100; i < 0x200; i++)
	{
		fprintf (f, "%04X ", READ_WORD (&toobin_paletteram[i*2]));
		if ((i & 15) == 15) fprintf (f, "\n");
	}

	fprintf (f, "\n\nAlpha Palette\n");
	for (i = 0x200; i < 0x300; i++)
	{
		fprintf (f, "%04X ", READ_WORD (&toobin_paletteram[i*2]));
		if ((i & 15) == 15) fprintf (f, "\n");
	}

	fprintf (f, "\n\nX Scroll = %03X\nY Scroll = %03X\n",
		READ_WORD (&toobin_hscroll[0]) >> 6, READ_WORD (&toobin_vscroll[0]) >> 6);

	fprintf (f, "\n\nMotion Objects\n");
	for (i = 0; i < toobin_spriteram_size; i += 8)
	{
		int data1 = READ_WORD (&toobin_spriteram[i+0]);
		int data2 = READ_WORD (&toobin_spriteram[i+2]);
		int data3 = READ_WORD (&toobin_spriteram[i+4]);
		int data4 = READ_WORD (&toobin_spriteram[i+6]);

		int ypos = (data1 >> 6) & 0x1ff;
		int vsize = ((data1 >> 3) & 7) + 1;
		int hsize = (data1 & 7) + 1;
		int pict = (data2 & 0x3fff);
		int vflip = ((data2 >> 15) & 1);
		int hflip = ((data2 >> 14) & 1);
		int link = (data3 & 0xff);
		int color = (data4 & 0x0f);
		int xpos = (data4 >> 6) & 0x3ff;

		fprintf (f, "  Object %02X: P=%04X C=%01X X=%03X Y=%03X HSIZ=%d VSIZ=%d HFLP=%d VFLP=%d L=%02X Leftovers: %04X %04X\n",
				i/8, pict, color, xpos, ypos, hsize, vsize, hflip, vflip, link, data3 & 0xff00, data4 & 0x30);
	}

	fprintf (f, "\n\nPlayfield palette usage\n");
	for (i = 0; i < 16; i++)
		fprintf (f, "   Pal %01X: %4d tiles\n", i, playfield_count[i]);

	fprintf (f, "\n\nPlayfield dump\n");
	for (i = 0; i < toobin_playfieldram_size / 4; i++)
	{
		fprintf (f, "%01X%04X ", READ_WORD (&toobin_playfieldram[i*4]), READ_WORD (&toobin_playfieldram[i*4+2]));
		if ((i & 127) == 127) fprintf (f, "\n");
		else if ((i & 127) == 63) fprintf (f, "\n      ");
	}

	fprintf (f, "\n\nAlpha dump\n");
	for (i = 0; i < toobin_alpharam_size / 2; i++)
	{
		fprintf (f, "%04X ", READ_WORD (&toobin_alpharam[i*2]));
		if ((i & 63) == 63) fprintf (f, "\n");
	}

	fclose (f);
}
#endif
