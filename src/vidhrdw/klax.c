/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/generic.h"

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

static struct osd_bitmap *playfieldbitmap;



/*************************************
 *
 *		Prototypes from other modules
 *
 *************************************/

void klax_vh_stop (void);

static void klax_debug (void);



/*************************************
 *
 *		Video system start
 *
 *************************************/

int klax_vh_start(void)
{
	static struct atarigen_modesc klax_modesc =
	{
		256,                 /* maximum number of MO's */
		8,                   /* number of bytes per MO entry */
		2,                   /* number of bytes between MO words */
		0,                   /* ignore an entry if this word == 0xffff */
		0, 0, 0xff,          /* link = (data[linkword] >> linkshift) & linkmask */
		0                    /* render in reverse link order */
	};

	/* allocate dirty buffers */
	if (!playfielddirty)
		playfielddirty = malloc (atarigen_playfieldram_size / 2);
	if (!playfielddirty)
	{
		klax_vh_stop ();
		return 1;
	}
	memset (playfielddirty, 1, atarigen_playfieldram_size / 2);

	/* allocate bitmaps */
	if (!playfieldbitmap)
		playfieldbitmap = osd_new_bitmap (XDIM, YDIM, Machine->scrbitmap->depth);
	if (!playfieldbitmap)
	{
		klax_vh_stop ();
		return 1;
	}

	/* initialize the palette remapping */
	atarigen_init_remap (COLOR_PALETTE_555, 1);
	
	/* initialize the displaylist system */
	return atarigen_init_display_list (&klax_modesc);
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
	playfielddirty = 0;
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
	
	if (oldword != newword)
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
	/* look up the SLIP link */
	int link = READ_WORD (&atarigen_playfieldram[0xf80 + 2 * (scanline / 8)]) & 0xff;
	atarigen_update_display_list (atarigen_spriteram, link, scanline);
}


/*---------------------------------------------------------------------------------
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
 *			Bits 0-11  = image index
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
 
void klax_render_mo (struct osd_bitmap *bitmap, struct rectangle *clip, unsigned short *data, void *param)
{
	int *sprite_map = param;
	int xadv, x, y, sx, sy;
	
	/* extract data from the various words */
	int pict = data[1] & 0x0fff;
	int hsize = ((data[3] >> 4) & 7) + 1;
	int vsize = (data[3] & 7) + 1;
	int xpos = data[2] >> 7;
	int ypos = 256 - ((data[3] >> 7) - 256) - vsize * 8;
	int color = data[2] & 15;
	int hflip = data[3] & 0x0008;

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
	for (y = 0, sy = ypos; y < vsize; y++, sy += 8)
	{
		/* clip the Y coordinate */
		if (sy <= clip->min_y - 8)
		{
			pict += hsize;
			continue;
		}
		else if (sy > clip->max_y)
			break;

		/* loop over the width */
		for (x = 0, sx = xpos; x < hsize; x++, sx += xadv, pict++)
		{
			/* clip the X coordinate */
			if (sx <= -8 || sx >= XDIM)
				continue;

			/* if this sprite's color has not been used yet this frame, grab it */
			/* note that by doing it here, we don't map sprites that aren't visible */
			if (!sprite_map[color])
			{
				atarigen_alloc_dynamic_colors (0x000 + 16 * color + 1, 15);
				sprite_map[color] = 1;
			}

			/* draw the sprite */
			drawgfx (bitmap, Machine->gfx[1], pict, color, hflip, 0,
						sx, sy, clip, TRANSPARENCY_PEN, 0);
		}
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/

void klax_vh_screenrefresh (struct osd_bitmap *bitmap)
{
	int playfield_count[16], sprite_map[16];
	int x, y, offs, i;

	klax_debug ();

	/* reset color tracking */
	for (i = 0; i < 16; i++)
		sprite_map[i] = playfield_count[i] = 0;

	/*
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

	/* loop over the visible X region */
	for (x = 0; x < XCHARS; x++)
	{
		offs = x * 32;

		/* loop over the visible Y region */
		for (y = 0; y < YCHARS; y++, offs++)
		{
			/* read the data word */
			int data2 = READ_WORD (&atarigen_playfieldram[offs * 2 + 0x1000]);
			int color = (data2 >> 8) & 15;

			/* update color statistics */
			playfield_count[color] = 1;

			/* rerender if dirty */
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

	/* allocate playfield colors */
	atarigen_alloc_fixed_colors (playfield_count, 0x100, 16, 16);

	/* copy to the destination */
	copybitmap (bitmap, playfieldbitmap, 0, 0, 0, 0, &Machine->drv->visible_area, TRANSPARENCY_NONE, 0);

	/* render the motion objects */
	atarigen_render_display_list (bitmap, klax_render_mo, sprite_map);

	/* final color update */
	atarigen_update_colors (-1);
}


/*************************************
 *
 *		Debugging
 *
 *************************************/

static void klax_debug (void)
{
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
		for (i = 0; i < 0x400; i++)
		{
			unsigned short *data = (unsigned short *)&atarigen_spriteram[i*8];
			int pict = data[1] & 0x7fff;
			int hsize = ((data[3] >> 4) & 7) + 1;
			int vsize = (data[3] & 7) + 1;
			int xpos = (data[2] >> 7);
			int ypos = (data[3] >> 7) - vsize * 8;
			int color = data[2] & 15;
			int hflip = data[3] & 0x0008;
			fprintf (f, "   Object %03X: L=%03X P=%04X C=%X X=%03X Y=%03X W=%d H=%d F=%d LEFT=(%04X %04X %04X %04X)\n",
					i, data[0] & 0x3ff, pict, color, xpos & 0x1ff, ypos & 0x1ff, hsize, vsize, hflip, 
					data[0] & 0xfc00, data[1] & 0x0000, data[2] & 0x0070, data[3] & 0x0000);
		}

		fprintf (f, "\n\nPlayfield dump\n");
		for (i = 0; i < atarigen_playfieldram_size / 2; i++)
		{
			fprintf (f, "%04X ", READ_WORD (&atarigen_playfieldram[i*2]));
			if ((i & 63) == 63) fprintf (f, "\n");
		}

		fclose (f);
	}
}
