/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *gauntlet_bank3;

/*#define GAUNTLET_DEBUG 1*/

#define XDIM 42
#define YDIM 30


/*
 *		Globals we own
 */

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

static unsigned char *playfieldram;
static unsigned char *spriteram;
static unsigned char *alpharam;
static unsigned char *paletteram;

static unsigned char vscroll[4];
static unsigned char hscroll[4];

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


/***************************************************************************

  Convert the color PROMs into a more useable format.

  Gauntlet doesn't have a color PROM. It uses 64KB bytes of RAM to
  dynamically create the palette. Each couple of bytes defines one
  color (5 bits per R,G,B; the highest 1 bit of the first byte is unused).
  Graphics use 4 bitplanes.

***************************************************************************/
void gauntlet_vh_convert_color_prom (unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom)
{
	int i;

	for (i = 0;i < 256;i++)
	{
		int bits;

		bits = (i >> 0) & 0x07;
		palette[3*i] = (bits >> 1) | (bits << 2) | (bits << 5);
		bits = (i >> 3) & 0x07;
		palette[3*i + 1] = (bits >> 1) | (bits << 2) | (bits << 5);
		bits = (i >> 6) & 0x03;
		palette[3*i + 2] = bits | (bits << 2) | (bits << 4) | (bits << 6);
	}
}


/*
 *   video system start; we also initialize the system memory as well here
 */

int gauntlet_vh_start(void)
{
	int res;

	/* start up the machine */
	res = gauntlet_system_start ();
	if (res != 0)
		return res;

	/* allocate RAM space */
	if (!playfieldram)
		playfieldram = calloc (gauntlet_playfieldram_size + gauntlet_spriteram_size +
		                       gauntlet_paletteram_size + gauntlet_alpharam_size, 1);
	if (!playfieldram)
	{
		gauntlet_vh_stop ();
		return 1;
	}
	spriteram = playfieldram + gauntlet_playfieldram_size;
	paletteram = spriteram + gauntlet_spriteram_size;
	alpharam = paletteram + gauntlet_paletteram_size;

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
		playfieldbitmap = osd_create_bitmap (64*8, 64*8);
	if (!playfieldbitmap)
	{
		gauntlet_vh_stop ();
		return 1;
	}

	/* initialize the transparency trackers */
	memset (trans_count_col, YDIM, sizeof (trans_count_col));
	memset (trans_count_row, XDIM, sizeof (trans_count_row));

	/* spriteram is bank 4 */
	cpu_setbank (4, spriteram);

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

	/* free RAM space */
	if (playfieldram)
		free (playfieldram);
	playfieldram = spriteram = paletteram = alpharam = 0;

	/* close down the machine */
	gauntlet_system_stop();
}


/*
 *   vertical scroll read/write handlers
 */

int gauntlet_vscroll_r (int offset)
{
	return vscroll[offset];
}

void gauntlet_vscroll_w (int offset, int data)
{
	/* invalidate the entire playfield if we're switching ROM banks */
	if (offset == 3 && (vscroll[3] & 3) != (data & 3))
		memset (playfielddirty, 1, gauntlet_playfieldram_size / 2);
	vscroll[offset] = data;
}


/*
 *   horizontal scroll read/write handlers
 */

int gauntlet_hscroll_r (int offset)
{
	return hscroll[offset];
}

void gauntlet_hscroll_w (int offset, int data)
{
	hscroll[offset] = data;
}


/*
 *   playfield RAM read/write handlers
 */

int gauntlet_playfieldram_r (int offset)
{
	return playfieldram[offset];
}

void gauntlet_playfieldram_w (int offset, int data)
{
	if (playfieldram[offset] != data)
	{
		playfieldram[offset] = data;
		playfielddirty[offset / 2] = 1;
	}
}


/*
 *   alpha RAM read/write handlers
 */

int gauntlet_alpharam_r (int offset)
{
	return alpharam[offset];
}

void gauntlet_alpharam_w (int offset, int data)
{
	/* track opacity of rows & columns */
	if (!(offset & 1) && ((data ^ alpharam[offset]) & 0x80))
	{
		int sx,sy;

		sx = (offset/2) % 64;
		sy = (offset/2) / 64;

		if (sx < XDIM && sy < YDIM)
		{
			if (data & 0x80)
				trans_count_col[sx]--, trans_count_row[sy]--;
			else
				trans_count_col[sx]++, trans_count_row[sy]++;
		}
	}

	alpharam[offset] = data;
}


/*
 *   palette RAM read/write handlers
 */

int gauntlet_paletteram_r (int offset)
{
	return paletteram[offset];
}

void gauntlet_paletteram_w (int offset, int data)
{
	if (paletteram[offset] != data)
	{
		paletteram[offset] = data;
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
	int offs, bank, link, count = 64*64;
	int x, y, sx, sy, xoffs, yoffs;


/* Fun debugging stuff -- leave here until I can figure out the motion object system */
#if GAUNTLET_DEBUG
FILE *f = NULL;
int record = 0;
if (osd_key_pressed (OSD_KEY_BACKSPACE))
{
	while (osd_key_pressed (OSD_KEY_BACKSPACE));
	dumpit();
}

if (osd_key_pressed (OSD_KEY_L))
{
	record = 1;
	f = fopen ("sprite.log", "a");
	if (f) fprintf (f, "\n\n=======================\n\n");
}
#endif


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
			int inten = ((paletteram[offs] >> 4) + 2) & 0x1c;
			int red = (paletteram[offs] & 0xf) * inten;
			int green = (paletteram[offs+1] >> 4) * inten;
			int blue = (paletteram[offs+1] & 0xf) * inten;
			int color = ((red >> 5) & 0x07) + ((green >> 2) & 0x38) + (blue & 0xc0);

			int index = offs / 2;
			smalldirty[index >> 4] = 1;

			if (offs < 0x200)
				Machine->gfx[0]->colortable[index] = Machine->pens[color];
			else
				Machine->gfx[1]->colortable[(index-16*16) ^ 0xf] = Machine->pens[color];

			colordirty[offs / 2] = 0;
		}
	}

	/* mark any tiles containing dirty colors dirty now */
	for (offs = gauntlet_playfieldram_size - 2; offs >= 0; offs -= 2)
	{
		int color = ((playfieldram[offs] >> 4) & 7) + 0x18;
		if (smalldirty[color+16])
			playfielddirty[offs/2] = 1;
	}


	/* compute scrolling so we know what to update */
	xscroll = (hscroll[0] << 8) + hscroll[1];
	yscroll = (vscroll[2] << 8) + vscroll[3];
	xscroll = -(xscroll & 0x1ff);
	yscroll = -((yscroll >> 7) & 0x1ff);

	/* update only the portion of the playfield that's visible. */
	bank = (vscroll[3] & 3) << 12;
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

			data = (playfieldram[offs*2] << 8) + playfieldram[offs*2+1];
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

	copyscrollbitmap (bitmap, playfieldbitmap,
			1, &xscroll,
			1, &yscroll,
			&clip,
			TRANSPARENCY_NONE, 0);


	/* motion objects -- yuck! */

	/* kludge alert -- this address in bank3 is seemingly where the software keeps track of the
	   head of the linked list; I don't know how the hardware does it yet */
	memset (spritevisit, 0, gauntlet_spriteram_size / 8);
	link = (((gauntlet_bank3[0x80] << 8) + gauntlet_bank3[0x81]) & 0x3ff) * 2;

	/* kludge of a kludge alert -- the above doesn't work for the title screen, so if we're pointing
	   to any empty list, go back to my old brute-force method */
	if (!link && !spriteram[0x1801] && !(spriteram[0x1800] & 3))
	{
		for (offs = 0; offs < gauntlet_spriteram_size / 4; offs += 2)
		{
			int data4 = (spriteram[offs + 0x1800] << 8) + spriteram[offs + 0x1801];
			if ((data4 & 0x3ff) && (data4 & 0x3ff) != offs/2)
				break;
		}

		if (offs == gauntlet_spriteram_size / 4)
			goto nomo;

		link = offs/2;
		do
		{
			for (offs = 0; offs < gauntlet_spriteram_size / 4; offs += 2)
			{
				int data4 = (spriteram[offs + 0x1800] << 8) + spriteram[offs + 0x1801];
				if ((data4 & 0x3ff) == link && !spritevisit[link])
				{
					spritevisit[link] = 1;
					link = offs/2;
					break;
				}
			}
		} while (offs != gauntlet_spriteram_size / 4);
		link *= 2;
	}

	/* max out at 64*64 tiles, just to be safe */
	while (count > 0)
	{
		int data1 = (spriteram[link + 0x0000] << 8) + spriteram[link + 0x0001];
		int data2 = (spriteram[link + 0x0800] << 8) + spriteram[link + 0x0801];
		int data3 = (spriteram[link + 0x1000] << 8) + spriteram[link + 0x1001];
		int data4 = (spriteram[link + 0x1800] << 8) + spriteram[link + 0x1801];

		/* extract data from the various words */
		int pict = data1 & 0x7fff;
		int color = (data2 & 0x0f);
		int xpos = xscroll + (data2 >> 7);
		int vsize = (data3 & 7) + 1;
		int hsize = ((data3 >> 3) & 7) + 1;
		int hflip = ((data3 >> 6) & 1);
		int ypos = yscroll - (data3 >> 7) - vsize * 8;

		int x, y;

#ifdef GAUNTLET_DEBUG
		if (record && f)
			fprintf (f, "%04X: %04X %04X %04X %04X -> LNK=%04X PIC=%04X COL=%01X XPOS=%3d YPOS=%3d HSIZ=%d VSIZ=%d HFLIP=%d\n", link/2, data1, data2, data3, data4, data4 & 0x3ff, pict, color & 0xf, xpos, ypos, hsize, vsize, hflip);
#endif

		/* h-flip case is handled differently */
		if (hflip)
		{
			for (y = 0; y < vsize; y++)
				for (x = 0; x < hsize; x++, pict++)
					drawgfx (bitmap, Machine->gfx[1],
							pict ^ 0x800, color,
							1, 0,
							(xpos + (hsize-x-1)*8) & 0x1ff,
							(ypos + y*8) & 0x1ff,
							&clip,
							TRANSPARENCY_PEN, 15);
		}
		else
		{
			for (y = 0; y < vsize; y++)
				for (x = 0; x < hsize; x++, pict++)
					drawgfx (bitmap, Machine->gfx[1],
							pict ^ 0x800, color,
							0, 0,
							(xpos + x*8) & 0x1ff,
							(ypos + y*8) & 0x1ff,
							&clip,
							TRANSPARENCY_PEN, 15);
		}

		/* count the tiles and remember that we visited this sprite already */
		count -= hsize * vsize;
		spritevisit[link/2] = 2;

		/* get a link to the next sprite; bail if we've done it already */
		link = (data4 & 0x3ff);
		if (spritevisit[link] == 2)
			break;
		link *= 2;
	}

nomo:

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */
	for (sy = 0; sy < YDIM; sy++)
	{
		for (sx = 0, offs = sy*64; sx < XDIM; sx++, offs++)
		{
			int data = (alpharam[offs*2] << 8) + alpharam[offs*2+1];
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

#ifdef GAUNTLET_DEBUG
if (f) fclose(f);
#endif
}



#ifdef GAUNTLET_DEBUG
static dumpit()
{
	FILE *f;

	f = fopen ("gauntlet-vid.bin", "wb");
	fwrite (playfieldram, 1, gauntlet_playfieldram_size, f);
	fclose (f);

	f = fopen ("gauntlet-alpha.bin", "wb");
	fwrite (alpharam, 1, gauntlet_alpharam_size, f);
	fclose (f);

	f = fopen ("gauntlet-spr.bin", "wb");
	fwrite (spriteram, 1, gauntlet_spriteram_size, f);
	fclose (f);

	f = fopen ("gauntlet-pal.bin", "wb");
	fwrite (paletteram, 1, gauntlet_paletteram_size, f);
	fclose (f);
}
#endif
