/***************************************************************************

  vidhrdw/rampart.c

  Functions to emulate the video hardware of the machine.

****************************************************************************

	Motion Object encoding
	----------------------
		4 16-bit words are used

		Word 1:
			Bits  0-7  = link to the next motion object

		Word 2:
			Bit   15   = horizontal flip
			Bits  0-11 = image index

		Word 3:
			Bits  7-15 = horizontal position
			Bits  0-3  = motion object palette

		Word 4:
			Bits  7-15 = vertical position
			Bits  4-6  = horizontal size of the object, in tiles
			Bits  0-2  = vertical size of the object, in tiles

***************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/generic.h"

#define XCHARS 43
#define YCHARS 30

#define XDIM (XCHARS*8)
#define YDIM (YCHARS*8)


#define DEBUG_VIDEO 0



/*************************************
 *
 *		Statics
 *
 *************************************/

static int *color_usage;



/*************************************
 *
 *		Prototypes
 *
 *************************************/

static const unsigned char *update_palette(void);

static void mo_color_callback(const unsigned short *data, const struct rectangle *clip, void *param);
static void mo_render_callback(const unsigned short *data, const struct rectangle *clip, void *param);

#if DEBUG_VIDEO
static void debug(void);
#endif



/*************************************
 *
 *		Video system start
 *
 *************************************/

int rampart_vh_start(void)
{
	static struct atarigen_mo_desc mo_desc =
	{
		256,                 /* maximum number of MO's */
		8,                   /* number of bytes per MO entry */
		2,                   /* number of bytes between MO words */
		0,                   /* ignore an entry if this word == 0xffff */
		0, 0, 0xff,          /* link = (data[linkword] >> linkshift) & linkmask */
		0                    /* render in reverse link order */
	};

	static struct atarigen_pf_desc pf_desc =
	{
		8, 8,				/* width/height of each tile */
		64, 64				/* number of tiles in each direction */
	};

	/* allocate color usage */
	color_usage = malloc(sizeof(int) * 256);
	if (!color_usage)
		return 1;
	color_usage[0] = XDIM * YDIM;

	/* initialize the playfield */
	if (atarigen_pf_init(&pf_desc))
	{
		free(color_usage);
		return 1;
	}

	/* initialize the motion objects */
	if (atarigen_mo_init(&mo_desc))
	{
		atarigen_pf_free();
		free(color_usage);
		return 1;
	}

	return 0;
}



/*************************************
 *
 *		Video system shutdown
 *
 *************************************/

void rampart_vh_stop(void)
{
	/* free data */
	if (color_usage)
		free(color_usage);
	color_usage = 0;

	atarigen_pf_free();
	atarigen_mo_free();
}



/*************************************
 *
 *		Playfield RAM write handler
 *
 *************************************/

void rampart_playfieldram_w(int offset, int data)
{
	int oldword = READ_WORD(&atarigen_playfieldram[offset]);
	int newword = COMBINE_WORD(oldword, data);
	int x, y;

	if (oldword != newword)
	{
		WRITE_WORD(&atarigen_playfieldram[offset], newword);

		/* track color usage */
		x = offset % 512;
		y = offset / 512;
		if (x < XDIM && y < YDIM)
		{
			color_usage[(oldword >> 8) & 0xff]--;
			color_usage[oldword & 0xff]--;
			color_usage[(newword >> 8) & 0xff]++;
			color_usage[newword & 0xff]++;
		}

		/* mark scanlines dirty */
		atarigen_pf_dirty[y] = 1;
	}
}



/*************************************
 *
 *		Periodic scanline updater
 *
 *************************************/

void rampart_scanline_update(int scanline)
{
	/* look up the SLIP link */
	if (scanline < YDIM)
	{
		int link = READ_WORD(&atarigen_spriteram[0x3f40 + 2 * (scanline / 8)]) & 0xff;
		atarigen_mo_update(atarigen_spriteram, link, scanline);
	}
}



/*************************************
 *
 *		Main refresh
 *
 *************************************/

void rampart_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
#if DEBUG_VIDEO
	debug();
#endif

	/* remap if necessary */
	if (update_palette())
		memset(atarigen_pf_dirty, 1, YDIM);

	/* update the cached bitmap */
	{
		int x, y;

		for (y = 0; y < YDIM; y++)
			if (atarigen_pf_dirty[y])
			{
				const unsigned char *src = &atarigen_playfieldram[512 * y];
				unsigned char *dst = atarigen_pf_bitmap->line[y];

				/* regenerate the line */
				for (x = 0; x < XDIM/2; x++)
				{
					int bits = READ_WORD(src);
					src += 2;
					*dst++ = Machine->pens[bits >> 8];
					*dst++ = Machine->pens[bits & 0xff];
				}
				atarigen_pf_dirty[y] = 0;

				/* make sure we update */
				osd_mark_dirty(0, y, XDIM - 1, y, 0);
			}
	}

	/* copy the cached bitmap */
	copybitmap(bitmap, atarigen_pf_bitmap, 0, 0, 0, 0, NULL, TRANSPARENCY_NONE, 0);

	/* render the motion objects */
	atarigen_mo_process(mo_render_callback, bitmap);

	/* update onscreen messages */
	atarigen_update_messages();
}



/*************************************
 *
 *		Palette management
 *
 *************************************/

static const unsigned char *update_palette(void)
{
	unsigned short mo_map[16];
	int i, j;

	/* reset color tracking */
	memset(mo_map, 0, sizeof(mo_map));
	palette_init_used_colors();

	/* update color usage for the mo's */
	atarigen_mo_process(mo_color_callback, mo_map);

	/* rebuild the playfield palette */
	for (i = 0; i < 256; i++)
		if (color_usage[i])
			palette_used_colors[0x000 + i] = PALETTE_COLOR_USED;

	/* rebuild the motion object palette */
	for (i = 0; i < 16; i++)
	{
		unsigned short used = mo_map[i];
		if (used)
		{
			palette_used_colors[0x100 + i * 16 + 0] = PALETTE_COLOR_TRANSPARENT;
			for (j = 1; j < 16; j++)
				if (used & (1 << j))
					palette_used_colors[0x100 + i * 16 + j] = PALETTE_COLOR_USED;
		}
	}

	return palette_recalc();
}



/*************************************
 *
 *		Motion object palette
 *
 *************************************/

static void mo_color_callback(const unsigned short *data, const struct rectangle *clip, void *param)
{
	const unsigned int *usage = Machine->gfx[0]->pen_usage;
	unsigned short *colormap = param;
	int code = data[1] & 0x7fff;
	int color = data[2] & 0x000f;
	int hsize = ((data[3] >> 4) & 7) + 1;
	int vsize = (data[3] & 7) + 1;
	int tiles = hsize * vsize;
	unsigned short temp = 0;
	int i;

	for (i = 0; i < tiles; i++)
		temp |= usage[code++];
	colormap[color] |= temp;
}



/*************************************
 *
 *		Motion object rendering
 *
 *************************************/

static void mo_render_callback(const unsigned short *data, const struct rectangle *clip, void *param)
{
	const struct GfxElement *gfx = Machine->gfx[0];
	struct osd_bitmap *bitmap = param;
	struct rectangle pf_clip;

	/* extract data from the various words */
	int hflip = data[1] & 0x8000;
	int code = data[1] & 0x7fff;
	int xpos = (data[2] >> 7) + 4;
	int color = data[2] & 0x000f;
	int ypos = 512 - (data[3] >> 7);
	int hsize = ((data[3] >> 4) & 7) + 1;
	int vsize = (data[3] & 7) + 1;

	/* adjust for height */
	ypos -= vsize * 8;

	/* adjust the final coordinates */
	xpos &= 0x1ff;
	ypos &= 0x1ff;
	if (xpos >= XDIM) xpos -= 0x200;
	if (ypos >= YDIM) ypos -= 0x200;

	/* determine the bounding box */
	atarigen_mo_compute_clip_8x8(pf_clip, xpos, ypos, hsize, vsize, clip);

	/* draw the motion object */
	atarigen_mo_draw_8x8(bitmap, gfx, code, color, hflip, 0, xpos, ypos, hsize, vsize, clip, TRANSPARENCY_PEN, 0);
}



/*************************************
 *
 *		Debugging
 *
 *************************************/

#if DEBUG_VIDEO

static void mo_print_callback(const unsigned short *data, const struct rectangle *clip, void *param)
{
	FILE *file = param;

	/* extract data from the various words */
	int hflip = data[1] & 0x8000;
	int code = data[1] & 0x7fff;
	int xpos = (data[2] >> 7) + 4;
	int color = data[2] & 0x000f;
	int ypos = 512 - (data[3] >> 7);
	int hsize = ((data[3] >> 4) & 7) + 1;
	int vsize = (data[3] & 7) + 1;

	fprintf(file, "P=%04X C=%X F=%X  X=%03X Y=%03X S=%dx%d\n", code, color, hflip >> 15, xpos & 0xfff, ypos & 0xfff, hsize, vsize);
}

static void debug(void)
{
	if (keyboard_key_pressed(KEYCODE_9))
	{
		static int count;
		char name[50];
		FILE *f;
		int i;

		while (keyboard_key_pressed(KEYCODE_9)) { }

		sprintf(name, "Dump %d", ++count);
		f = fopen(name, "wt");

		fprintf(f, "\n\nPlayfield Palette:\n");
		for (i = 0x000; i < 0x100; i++)
		{
			fprintf(f, "%04X ", READ_WORD(&paletteram[i*2]));
			if ((i & 15) == 15) fprintf(f, "\n");
		}

		fprintf(f, "\n\nMotion Object Palette:\n");
		for (i = 0x100; i < 0x200; i++)
		{
			fprintf(f, "%04X ", READ_WORD(&paletteram[i*2]));
			if ((i & 15) == 15) fprintf(f, "\n");
		}

		fprintf(f, "\n\nMotion Objects Drawn:\n");
		atarigen_mo_process(mo_print_callback, f);

		fprintf(f, "\n\nMotion Objects\n");
		for (i = 0; i < 256; i++)
		{
			unsigned short *data = (unsigned short *)&atarigen_spriteram[i*8];
			int hflip = data[1] & 0x8000;
			int code = data[1] & 0x7fff;
			int xpos = (data[2] >> 7) + 4;
			int ypos = 512 - (data[3] >> 7);
			int color = data[2] & 0x000f;
			int hsize = ((data[3] >> 4) & 7) + 1;
			int vsize = (data[3] & 7) + 1;
			fprintf(f, "   Object %03X: L=%03X P=%04X C=%X X=%03X Y=%03X W=%d H=%d F=%d LEFT=(%04X %04X %04X %04X)\n",
					i, data[0] & 0x3ff, code, color, xpos & 0x1ff, ypos & 0x1ff, hsize, vsize, hflip,
					data[0] & 0xfc00, data[1] & 0x0000, data[2] & 0x0070, data[3] & 0x0008);
		}

		fprintf(f, "\n\nSprite RAM dump\n");
		for (i = 0; i < 0x4000 / 2; i++)
		{
			fprintf(f, "%04X ", READ_WORD(&atarigen_spriteram[i*2]));
			if ((i & 31) == 31) fprintf(f, "\n");
		}

		fclose(f);
	}
}

#endif
