/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#define XDIM 42
#define YDIM 30


/*
 *		Globals we own
 */

unsigned char *gauntlet_playfieldram;
unsigned char *gauntlet_spriteram;
unsigned char *gauntlet_alpharam;
unsigned char *gauntlet_paletteram;
unsigned char *gauntlet_vscroll;
unsigned char *gauntlet_hscroll;
unsigned char *gauntlet_bank3;

int gauntlet_playfieldram_size;
int gauntlet_spriteram_size;
int gauntlet_alpharam_size;
int gauntlet_paletteram_size;


/*
 *		Statics
 */

static unsigned char *playfielddirty;
static unsigned char *colordirty;
static unsigned char *spritevisit;

static struct osd_bitmap *playfieldbitmap;

static unsigned char trans_count_col[XDIM];
static unsigned char trans_count_row[YDIM];

static int current_head;
static int current_head_link;


/*
 *		Prototypes from other modules
 */

int gauntlet_system_start (void);
int gauntlet_system_stop (void);

void gauntlet_vh_stop (void);


/*
 *   video system start; we also initialize the system memory as well here
 */

int gauntlet_vh_start(void)
{
	/* allocate dirty buffers */
	if (!playfielddirty)
		playfielddirty = calloc (gauntlet_playfieldram_size/2 + gauntlet_paletteram_size/2 +
		                         gauntlet_spriteram_size/8, 1);
	if (!playfielddirty)
	{
		gauntlet_vh_stop ();
		return 1;
	}
	colordirty = playfielddirty + gauntlet_playfieldram_size/2;
	spritevisit = colordirty + gauntlet_paletteram_size/2;

	/* allocate bitmaps */
	if (!playfieldbitmap)
		playfieldbitmap = osd_new_bitmap (64*8, 64*8, Machine->scrbitmap->depth);
	if (!playfieldbitmap)
	{
		gauntlet_vh_stop ();
		return 1;
	}

	/* initialize the transparency trackers */
	memset (trans_count_col, YDIM, sizeof (trans_count_col));
	memset (trans_count_row, XDIM, sizeof (trans_count_row));

	current_head = current_head_link = 0;

	return 0;
}


/*
 *   video system shutdown; we also bring down the system memory as well here
 */

void gauntlet_vh_stop(void)
{
	/* free bitmaps */
	if (playfieldbitmap)
		osd_free_bitmap (playfieldbitmap);
	playfieldbitmap = 0;

	/* free dirty buffers */
	if (playfielddirty)
		free (playfielddirty);
	playfielddirty = colordirty = spritevisit = 0;
}


/*
 *   vertical scroll read/write handlers
 */

int gauntlet_vscroll_r (int offset)
{
	return READ_WORD (&gauntlet_vscroll[offset]);
}

void gauntlet_vscroll_w (int offset, int data)
{
	int oldword = READ_WORD (&gauntlet_vscroll[offset]);
	int newword = COMBINE_WORD (oldword, data);
	WRITE_WORD (&gauntlet_vscroll[offset], newword);

	/* invalidate the entire playfield if we're switching ROM banks */
	if (offset == 2 && (oldword & 3) != (newword & 3))
		memset (playfielddirty, 1, gauntlet_playfieldram_size / 2);
}


/*
 *   playfield RAM read/write handlers
 */

int gauntlet_playfieldram_r (int offset)
{
	return READ_WORD (&gauntlet_playfieldram[offset]);
}

void gauntlet_playfieldram_w (int offset, int data)
{
	int oldword = READ_WORD (&gauntlet_playfieldram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&gauntlet_playfieldram[offset], newword);
		playfielddirty[offset / 2] = 1;
	}
}


/*
 *   alpha RAM read/write handlers
 */

int gauntlet_alpharam_r (int offset)
{
	return READ_WORD (&gauntlet_alpharam[offset]);
}

void gauntlet_alpharam_w (int offset, int data)
{
	int oldword = READ_WORD (&gauntlet_alpharam[offset]);
	int newword = COMBINE_WORD (oldword, data);
	WRITE_WORD (&gauntlet_alpharam[offset], newword);

	/* track opacity of rows & columns */
	if ((oldword ^ newword) & 0x8000)
	{
		int sx,sy;

		sx = (offset/2) % 64;
		sy = (offset/2) / 64;

		if (sx < XDIM && sy < YDIM)
		{
			if (newword & 0x8000)
				trans_count_col[sx]--, trans_count_row[sy]--;
			else
				trans_count_col[sx]++, trans_count_row[sy]++;
		}
	}
}


/*
 *   palette RAM read/write handlers
 */

int gauntlet_paletteram_r (int offset)
{
	return READ_WORD (&gauntlet_paletteram[offset]);
}

void gauntlet_paletteram_w (int offset, int data)
{
	int oldword = READ_WORD (&gauntlet_paletteram[offset]);
	int newword = COMBINE_WORD (oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD (&gauntlet_paletteram[offset], newword);
		colordirty[offset / 2] = 1;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void gauntlet_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	unsigned char smalldirty[64];
	struct rectangle clip;
	int xscroll, yscroll;
	int offs, bank, link;
	int x, y, sx, sy, xoffs, yoffs;


	/* clip out any rows and columns that are completely covered by characters */
	for (x = 0; x < XDIM; x++)
		if (trans_count_col[x]) break;
	clip.min_x = x*8;

	for (x = XDIM-1; x > 0; x--)
		if (trans_count_col[x]) break;
	clip.max_x = x*8+7;

	for (y = 0; y < YDIM; y++)
		if (trans_count_row[y]) break;
	clip.min_y = y*8;

	for (y = YDIM-1; y > 0; y--)
		if (trans_count_row[y]) break;
	clip.max_y = y*8+7;


	/* update the colors */
	memset (smalldirty, 0, sizeof (smalldirty));
	for (offs = gauntlet_paletteram_size - 2; offs >= 0; offs -= 2)
	{
		if (colordirty[offs / 2])
		{
			int palette = READ_WORD (&gauntlet_paletteram[offs]);
			int inten = (palette >> 12) & 15;
			int red = ((palette >> 8) & 15) * inten + inten;
			int green = ((palette >> 4) & 15) * inten + inten;
			int blue = (palette & 15) * inten + inten;

			int index = offs / 2;
			smalldirty[index >> 4] = 1;

			setgfxcolorentry (Machine, index, red, green, blue);

			colordirty[offs / 2] = 0;
		}
	}


	/* mark any tiles containing dirty colors dirty now */
	for (offs = gauntlet_playfieldram_size - 2; offs >= 0; offs -= 2)
	{
		int color = ((READ_WORD (&gauntlet_playfieldram[offs]) >> 12) & 7) + 0x18;
		if (smalldirty[color+16])
			playfielddirty[offs/2] = 1;
	}


	/* compute scrolling so we know what to update */
	xscroll = READ_WORD (&gauntlet_hscroll[0]);
	yscroll = READ_WORD (&gauntlet_vscroll[2]);
	xscroll = -(xscroll & 0x1ff);
	yscroll = -((yscroll >> 7) & 0x1ff);


	/* update only the portion of the playfield that's visible. */
	bank = (READ_WORD (&gauntlet_vscroll[2]) & 3) << 12;
	xoffs = (-xscroll/8) + (clip.min_x/8);
	yoffs = (-yscroll/8) + (clip.min_y/8);
	for (y = yoffs + (clip.max_y-clip.min_y)/8 + 1; y >= yoffs; y--)
	{
		sy = y & 63;
		for (x = xoffs + clip.max_x/8 + 1; x >= xoffs; x--)
		{
			int data, color;

			sx = x & 63;
			offs = sx * 64 + sy;

			data = READ_WORD (&gauntlet_playfieldram[offs*2]);
			color = ((data >> 12) & 7) + 0x18;

			if (playfielddirty[offs])
			{
				int hflip = (data >> 15) & 1;
				int pict = bank + (data & 0xfff);

				playfielddirty[offs] = 0;

				drawgfx (playfieldbitmap, Machine->gfx[1],
						pict ^ 0x800, color,
						hflip, 0,
						8*sx, 8*sy,
						0,
						TRANSPARENCY_NONE, 0);
			}
		}
	}


	/* copy the playfield to the destination */
	copyscrollbitmap (bitmap, playfieldbitmap,
			1, &xscroll,
			1, &yscroll,
			&clip,
			TRANSPARENCY_NONE, 0);


	/* motion objects -- attempt to simulate the real hardware behavior as closely as possible */
	yoffs = (-yscroll/8) % 64;
	memset (spritevisit, 0, gauntlet_spriteram_size/8);
	for (y = 0; y <= YDIM; y++, yoffs = (yoffs + 1) % 64)
	{
		/* compute the address of the link list start */
		offs = 0xf80 + yoffs*2;
		link = (READ_WORD (&gauntlet_alpharam[offs]) & 0x3ff) * 2;

		/* loop over motion objects until one is clipped */
		while (!spritevisit[link/2])
		{
			int data1 = READ_WORD (&gauntlet_spriteram[link + 0x0000]);
			int data2 = READ_WORD (&gauntlet_spriteram[link + 0x0800]);
			int data3 = READ_WORD (&gauntlet_spriteram[link + 0x1000]);
			int data4 = READ_WORD (&gauntlet_spriteram[link + 0x1800]);

			/* extract data from the various words */
			int pict = data1 & 0x7fff;
			int color = (data2 & 0x0f);
			int xpos = xscroll + (data2 >> 7);
			int vsize = (data3 & 7) + 1;
			int hsize = ((data3 >> 3) & 7) + 1;
			int hflip = ((data3 >> 6) & 1);
			int ypos = yscroll - (data3 >> 7) - vsize * 8;
			int x, y;

			/* h-flip case is handled differently */
			if (hflip)
			{
				for (y = 0; y < vsize; y++)
				{
					/* wrap and clip the Y coordinate */
					int ty = (ypos + y*8) & 0x1ff;
					if (ty > 0x1f8) ty -= 0x200;
					if (ty <= -8 || ty >= YDIM*8)
					{
						pict += hsize;
						continue;
					}

					for (x = 0; x < hsize; x++, pict++)
					{
						/* wrap and clip the X coordinate */
						int tx = (xpos + (hsize-x-1)*8) & 0x1ff;
						if (tx > 0x1f8) tx -= 0x200;
						if (tx <= -8 || tx >= XDIM*8) continue;

						drawgfx (bitmap, Machine->gfx[1],
								pict ^ 0x800, color, 1, 0, tx, ty, &clip, TRANSPARENCY_PEN, 0);
					}
				}
			}
			else
			{
				for (y = 0; y < vsize; y++)
				{
					/* wrap and clip the Y coordinate */
					int ty = (ypos + y*8) & 0x1ff;
					if (ty > 0x1f8) ty -= 0x200;
					if (ty <= -8 || ty >= YDIM*8)
					{
						pict += hsize;
						continue;
					}

					for (x = 0; x < hsize; x++, pict++)
					{
						/* wrap and clip the X coordinate */
						int tx = (xpos + x*8) & 0x1ff;
						if (tx > 0x1f8) tx -= 0x200;
						if (tx <= -8 || tx >= XDIM*8) continue;

						drawgfx (bitmap, Machine->gfx[1],
								pict ^ 0x800, color, 0, 0, tx, ty, &clip, TRANSPARENCY_PEN, 0);
					}
				}
			}

			/* get a link to the next sprite */
			spritevisit[link/2] = 1;
			link = (data4 & 0x3ff) * 2;
		}
	}


	/* redraw the alpha layer completely */
	for (sy = 0; sy < YDIM; sy++)
	{
		for (sx = 0, offs = sy*64; sx < XDIM; sx++, offs++)
		{
			int data = READ_WORD (&gauntlet_alpharam[offs*2]);
			int pict = (data & 0x3ff);

			if (pict || (data & 0x8000))
			{
				int color = ((data >> 10) & 0xf) | ((data >> 9) & 0x20);

				drawgfx (bitmap, Machine->gfx[0],
						pict, color,
						0, 0,
						8*sx, 8*sy,
						0,
						(data & 0x8000) ? TRANSPARENCY_NONE : TRANSPARENCY_PEN, 0);
			}
		}
	}
}
