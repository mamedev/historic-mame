/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/generic.h"

#define klax_DEBUG 0

#define XCHARS 42
#define YCHARS 30

#define XDIM (XCHARS*8)
#define YDIM (YCHARS*8)



/*************************************
 *
 *		Globals we own
 *
 *************************************/



/*************************************
 *
 *		Statics
 *
 *************************************/

static unsigned char *playfielddirty;
static unsigned char *spritevisit;
static unsigned short *displaylist;

static unsigned short *displaylist_end;
static unsigned short *displaylist_last;

static struct osd_bitmap *playfieldbitmap;



/*************************************
 *
 *		Prototypes from other modules
 *
 *************************************/

void klax_vh_stop (void);
void klax_sound_reset (void);
void klax_update_display_list (int scanline);

#if klax_DEBUG
static int klax_debug (void);
#endif



/*************************************
 *
 *		Generic video system start
 *
 *************************************/

int klax_vh_start(void)
{
	/* allocate dirty buffers */
	if (!playfielddirty)
		playfielddirty = calloc (atarigen_playfieldram_size / 2 +
		                         atarigen_spriteram_size / 8 +
		                         atarigen_spriteram_size / 4 * (YCHARS + 1), 1);
	if (!playfielddirty)
	{
		klax_vh_stop ();
		return 1;
	}
	spritevisit = (unsigned char *)(playfielddirty + atarigen_playfieldram_size / 2);
	displaylist = (unsigned short *)(spritevisit + atarigen_spriteram_size / 8);

	/* allocate bitmaps */
	if (!playfieldbitmap)
		playfieldbitmap = osd_new_bitmap (XDIM, YDIM, Machine->scrbitmap->depth);
	if (!playfieldbitmap)
	{
		klax_vh_stop ();
		return 1;
	}

	/* no motion object lists */
	displaylist_end = displaylist;
	displaylist_last = NULL;

	/* initialize the palette remapping */
	atarigen_init_remap (COLOR_PALETTE_555);

	return 0;
}



/*************************************
 *
 *		Video system shutdown
 *
 *************************************/

void klax_vh_stop (void)
{
	/* free bitmaps */
	if (playfieldbitmap)
		osd_free_bitmap (playfieldbitmap);
	playfieldbitmap = 0;

	/* free dirty buffers */
	if (playfielddirty)
		free (playfielddirty);
	spritevisit = 0;
	displaylist = 0;
}



/*************************************
 *
 *		Latch write handler
 *
 *************************************/

void klax_latch_w (int offset, int data)
{
}



/*************************************
 *
 *		Playfield RAM read/write handlers
 *
 *************************************/

int klax_playfieldram_r (int offset)
{
	return READ_WORD (&atarigen_playfieldram[offset]);
}


void klax_playfieldram_w (int offset, int data)
{
	int oldword = READ_WORD (&atarigen_playfieldram[offset]);
	int newword = COMBINE_WORD (oldword, data);

//	if (oldword != newword)
	{
		WRITE_WORD (&atarigen_playfieldram[offset], newword);
		playfielddirty[(offset & 0xfff) / 2] = 1;
	}
}



/*************************************
 *
 *		Palette RAM read/write handlers
 *
 *************************************/

#ifdef LSB_FIRST
	#define BYTE_XOR 1
#else
	#define BYTE_XOR 0
#endif

int klax_paletteram_r (int offset)
{
	return (atarigen_paletteram[(offset / 2) ^ BYTE_XOR] << 8) | 0xff;
}


void klax_paletteram_w (int offset, int data)
{
	if (!(data & 0xff000000))
		atarigen_paletteram[(offset / 2) ^ BYTE_XOR] = data >> 8;
}



/*************************************
 *
 *		Motion object list handlers
 *
 *************************************/

void klax_update_display_list (int scanline)
{
	unsigned short *d, *startd, *lastd;
	int link, match = 0;

	/* scanline -1 means first update */
	if (!scanline)
	{
		displaylist_end = displaylist;
		displaylist_last = NULL;
	}

	/* set up local pointers */
	startd = d = displaylist_end;
	lastd = displaylist_last;

	/* look up the SLIP link */
	link = READ_WORD (&atarigen_playfieldram[0xf80 + 2 * (scanline / 8)]) & 0xff;

	/* if the last list entries were on the same scanline, overwrite them */
	if (lastd)
	{
		if (*lastd == scanline)
			d = startd = lastd;
		else
			match = 1;
	}

	/* visit all the sprites and copy their data into the display list */
	memset (spritevisit, 0, atarigen_spriteram_size / 8);
	while (!spritevisit[link])
	{
		int data1 = READ_WORD (&atarigen_spriteram[link * 8 + 0]);
		int data2 = READ_WORD (&atarigen_spriteram[link * 8 + 2]);
		int data3 = READ_WORD (&atarigen_spriteram[link * 8 + 4]);
		int data4 = READ_WORD (&atarigen_spriteram[link * 8 + 6]);

		/* ignore updates after the end of the frame */
		if (scanline < YDIM)
		{
			*d++ = scanline;
			*d++ = data2;
			*d++ = data3;
			*d++ = data4;

			/* update our match status */
			if (match)
			{
				lastd++;
				if (*lastd++ != data2 || *lastd++ != data3 || *lastd++ != data4)
					match = 0;
			}
		}

		/* link to the next object */
		spritevisit[link] = 1;
		link = data1 & 0xff;
	}

	/* if we didn't match the last set of entries, update the counters */
	if (!match)
	{
		displaylist_end = d;
		displaylist_last = startd;
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void klax_vh_screenrefresh (struct osd_bitmap *bitmap)
{
	int sprite_map[16], playfield_count[16];
	int x, y, sx, sy, offs, i;


	/*
	 *---------------------------------------------------------------------------------
	 *
	 *   Step 0: debugging
	 *
	 *---------------------------------------------------------------------------------
	 */
	#if klax_DEBUG
		int hidebank = klax_debug ();
	#endif


	/*
	 *---------------------------------------------------------------------------------
	 *
	 *   Step 1: update the offscreen version of the playfield
	 *
	 *---------------------------------------------------------------------------------
	 *
	 * 	Playfield encoding
	 *
	 *		1 16-bit word is used
	 *
	 *			Bits 0-12  = image number
	 *			Bits 13-15 = palette
	 *
	 *---------------------------------------------------------------------------------
	 */
	for (i = 0; i < 16; i++)
		sprite_map[i] = playfield_count[i] = 0;

	for (x = 0; x < XCHARS; x++)
	{
		offs = x * 32;
		for (y = 0; y < YCHARS; y++, offs++)
		{
			int data2 = READ_WORD (&atarigen_playfieldram[offs * 2 + 0x1000]);
			int color = (data2 >> 8) & 15;

			playfield_count[color] = 1;

			if (playfielddirty[offs])
			{
				int data1 = READ_WORD (&atarigen_playfieldram[offs * 2]);
				int hflip = data1 & 0x8000;
				drawgfx (playfieldbitmap, Machine->gfx[0], data1 & 0x1fff, color, hflip, 0,
						8 * x, 8 * y, 0, TRANSPARENCY_NONE, 0);
				playfielddirty[offs] = 0;
			}
		}
	}


	/*
	 *---------------------------------------------------------------------------------
	 *
	 *   Step 2: give the playfield first pick of colors
	 *
	 *---------------------------------------------------------------------------------
	 */
	atarigen_alloc_fixed_colors (playfield_count, 0x100, 16, 16);


	/*
	 *---------------------------------------------------------------------------------
	 *
	 *   Step 3: copy the playfield bitmap to the destination
	 *
	 *---------------------------------------------------------------------------------
	 */
	copybitmap (bitmap, playfieldbitmap, 0, 0, 0, 0, &Machine->drv->visible_area, TRANSPARENCY_NONE, 0);


	/*
	 *---------------------------------------------------------------------------------
	 *
	 *   Step 4: render the pregenerated list of motion objects
	 *
	 *---------------------------------------------------------------------------------
	 *
	 * 	Motion Object encoding
	 *
	 *		4 16-bit words are used total
	 *
	 *		Word 1: Link
	 *
	 *			Bits 0-7   = link to the next motion object
	 *
	 *		Word 2: Image
	 *
	 *			Bits 0-10  = image index
	 *
	 *		Word 3: Horizontal position
	 *
	 *			Bits 0-3   = motion object palette
	 *			Bits 7-15  = horizontal position
	 *
	 *		Word 4: Vertical position
	 *
	 *			Bits 0-2   = vertical size of the object, in tiles
	 *			Bit  3     = horizontal flip
	 *			Bits 4-6   = horizontal size of the object, in tiles
	 *			Bits 7-15  = vertical position
	 *
	 *---------------------------------------------------------------------------------
	 */
	{
		int last_start_scan = -1;
		struct rectangle clip;
		unsigned short *d;

		/* create a clipping rectangle so that only partial sections are updated at a time */
		clip.min_x = 0;
		clip.max_x = Machine->drv->screen_width - 1;

		/* loop over the list until the end */
		for (d = displaylist; d < displaylist_end; d += 4)
		{
			int start_scan = d[0], data2 = d[1], data3 = d[2], data4 = d[3];

			/* extract data from the various words */
			int pict = data2 & 0x0fff;
			int hsize = ((data4 >> 4) & 7) + 1;
			int vsize = (data4 & 7) + 1;
			int xpos = data3 >> 7;
			int ypos = 256 - ((data4 >> 7) - 256) - vsize * 8;
			int color = data3 & 15;
			int hflip = data4 & 0x0008;
			int xadv, yadv = 8;


			/* if this is a new region of the screen, find the end and adjust our clipping */
			if (start_scan != last_start_scan)
			{
				unsigned short *df;

				last_start_scan = start_scan;
				clip.min_y = start_scan;

				/* look for an entry whose scanline start is different from ours; that's our bottom */
				for (df = d; df < displaylist_end; df += 4)
					if (*df != start_scan)
					{
						clip.max_y = *df - 1;
						break;
					}

				/* if we didn't find any additional regions, go until the bottom of the screen */
				if (df == displaylist_end)
					clip.max_y = Machine->drv->screen_height - 1;
			}

			/* adjust for h flip */
			if (hflip)
				xpos += (hsize - 1) * 8, xadv = -8;
			else
				xadv = 8;

			/* adjust the final coordinates */
			xpos &= 0x1ff;
			ypos &= 0x1ff;
			if (xpos >= XDIM) xpos -= 0x200;
			if (ypos >= YDIM) ypos -= 0x200;

			/* loop over the height */
			for (y = 0, sy = ypos; y < vsize; y++, sy += yadv)
			{
				/* clip the Y coordinate */
				if (sy <= -8)
				{
					pict += hsize;
					continue;
				}
				else if (sy >= YDIM)
					break;

				/* loop over the width */
				for (x = 0, sx = xpos; x < hsize; x++, sx += xadv, pict++)
				{
					/* clip the X coordinate */
					if (sx <= -8)
						continue;
					else if (sx >= XDIM)
						break;

					/* if this sprite's color has not been used yet this frame, grab it */
					/* note that by doing it here, we don't map sprites that aren't visible */
					if (!sprite_map[color])
					{
						atarigen_alloc_dynamic_colors (0x000 + 16 * color + 1, 15);
						sprite_map[color] = 1;
					}

					/* draw the sprite */
					drawgfx (bitmap, Machine->gfx[1], pict, color, hflip, 0,
								sx, sy, &clip, TRANSPARENCY_PEN, 0);
				}
			}
		}
	}


	/*
	 *---------------------------------------------------------------------------------
	 *
	 *   Step 5: update the palette
	 *
	 *---------------------------------------------------------------------------------
	 */
	atarigen_update_colors (-1);
}


/*************************************
 *
 *		Debugging
 *
 *************************************/

#if klax_DEBUG
static int klax_debug (void)
{
	int hidebank = -1;

	if (osd_key_pressed (OSD_KEY_Q)) hidebank = 0;
	if (osd_key_pressed (OSD_KEY_W)) hidebank = 1;
	if (osd_key_pressed (OSD_KEY_E)) hidebank = 2;
	if (osd_key_pressed (OSD_KEY_R)) hidebank = 3;
	if (osd_key_pressed (OSD_KEY_T)) hidebank = 4;
	if (osd_key_pressed (OSD_KEY_Y)) hidebank = 5;
	if (osd_key_pressed (OSD_KEY_U)) hidebank = 6;
	if (osd_key_pressed (OSD_KEY_I)) hidebank = 7;

	if (osd_key_pressed (OSD_KEY_A)) hidebank = 8;
	if (osd_key_pressed (OSD_KEY_S)) hidebank = 9;
	if (osd_key_pressed (OSD_KEY_D)) hidebank = 10;
	if (osd_key_pressed (OSD_KEY_F)) hidebank = 11;
	if (osd_key_pressed (OSD_KEY_G)) hidebank = 12;
	if (osd_key_pressed (OSD_KEY_H)) hidebank = 13;
	if (osd_key_pressed (OSD_KEY_J)) hidebank = 14;
	if (osd_key_pressed (OSD_KEY_K)) hidebank = 15;

	if (osd_key_pressed (OSD_KEY_9))
	{
		static int count;
		char name[50];
		FILE *f;
		int i;

		while (osd_key_pressed (OSD_KEY_9)) { }

		sprintf (name, "Dump %d", ++count);
		f = fopen (name, "wt");

		fprintf (f, "\n\nMotion Object Palette:\n");
		for (i = 0x000; i < 0x100; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&atarigen_paletteram[i*2]));
			if ((i & 15) == 15) fprintf (f, "\n");
		}

		fprintf (f, "\n\nPlayfield Palette:\n");
		for (i = 0x100; i < 0x200; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&atarigen_paletteram[i*2]));
			if ((i & 15) == 15) fprintf (f, "\n");
		}

		fprintf (f, "\n\nMotion Object Config:\n");
		for (i = 0x00; i < 0x40; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&atarigen_playfieldram[0xf00 + i*2]));
			if ((i & 15) == 15) fprintf (f, "\n");
		}

		fprintf (f, "\n\nMotion Object SLIPs:\n");
		for (i = 0x00; i < 0x40; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&atarigen_playfieldram[0xf80 + i*2]));
			if ((i & 15) == 15) fprintf (f, "\n");
		}

		fprintf (f, "\n\nMotion Objects\n");
		for (i = 0; i < 0x100; i++)
		{
			fprintf (f, "   Object %02X:  0=%04X  1=%04X  2=%04X  3=%04X\n",
					i,
					READ_WORD (&atarigen_spriteram[i*8+0]),
					READ_WORD (&atarigen_spriteram[i*8+2]),
					READ_WORD (&atarigen_spriteram[i*8+4]),
					READ_WORD (&atarigen_spriteram[i*8+6])
			);
		}

		fprintf (f, "\n\nPlayfield dump\n");
		for (i = 0; i < atarigen_playfieldram_size / 2; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&atarigen_playfieldram[i*2]));
			if ((i & 63) == 63) fprintf (f, "\n");
		}

		fclose (f);
	}

	return hidebank;
}
#endif
