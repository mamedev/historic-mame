/***************************************************************************

  vidhrdw/blstroid.c

  Functions to emulate the video hardware of the machine.

****************************************************************************

	Playfield encoding
	------------------
		1 16-bit word is used

		Word 1:
			Bits 13-15 = palette
			Bits  0-12 = image number


	Motion Object encoding
	----------------------
		4 16-bit words are used

		Word 1:
			Bits  7-15 = vertical position
			Bits  0-3  = vertical size of the object, in tiles

		Word 2:
			Bit  15    = horizontal flip
			Bit  14    = vertical flip
			Bits  0-13 = image index

		Word 3:
			Bits  3-11 = link to the next motion object

		Word 4:
			Bits  6-15 = horizontal position
			Bits  0-3  = motion object palette

***************************************************************************/

#include "driver.h"
#include "machine/atarigen.h"
#include "vidhrdw/generic.h"

#define XCHARS 40
#define YCHARS 30

#define XDIM (XCHARS*16)
#define YDIM (YCHARS*8)


#define DEBUG_VIDEO 0



/*************************************
 *
 *		Statics
 *
 *************************************/

static unsigned long priority[8];



/*************************************
 *
 *		Prototypes
 *
 *************************************/

static const unsigned char *update_palette(void);

static void pf_color_callback(const struct rectangle *clip, const struct rectangle *tiles, const struct atarigen_pf_state *state, void *data);
static void pf_render_callback(const struct rectangle *clip, const struct rectangle *tiles, const struct atarigen_pf_state *state, void *data);
static void pf_overrender_callback(const struct rectangle *clip, const struct rectangle *tiles, const struct atarigen_pf_state *state, void *data);

static void mo_color_callback(const unsigned short *data, const struct rectangle *clip, void *param);
static void mo_render_callback(const unsigned short *data, const struct rectangle *clip, void *param);

static void irq_off(int param);

#if DEBUG_VIDEO
static int debug(void);
#endif



/*************************************
 *
 *		Generic video system start
 *
 *************************************/

int blstroid_vh_start(void)
{
	static struct atarigen_mo_desc mo_desc =
	{
		512,                 /* maximum number of MO's */
		8,                   /* number of bytes per MO entry */
		2,                   /* number of bytes between MO words */
		0,                   /* ignore an entry if this word == 0xffff */
		2, 3, 0x1ff,         /* link = (data[linkword] >> linkshift) & linkmask */
		0                    /* render in reverse link order */
	};

	static struct atarigen_pf_desc pf_desc =
	{
		16, 8,				/* width/height of each tile */
		64, 64				/* number of tiles in each direction */
	};

	/* reset statics */
	memset(priority, 0, sizeof(priority));

	/* initialize the playfield */
	if (atarigen_pf_init(&pf_desc))
		return 1;

	/* initialize the motion objects */
	if (atarigen_mo_init(&mo_desc))
	{
		atarigen_pf_free();
		return 1;
	}

	return 0;
}



/*************************************
 *
 *		Video system shutdown
 *
 *************************************/

void blstroid_vh_stop(void)
{
	atarigen_pf_free();
	atarigen_mo_free();
}



/*************************************
 *
 *		Periodic scanline updater
 *
 *************************************/

void blstroid_scanline_update(int scanline)
{
	int offset = (scanline / 8) * 0x80 + 0x50;

	/* update motion objects */
	if (scanline == 0)
		atarigen_mo_update(atarigen_spriteram, 0, scanline);

	/* check for interrupts */
	if (offset < atarigen_playfieldram_size)
		if (READ_WORD(&atarigen_playfieldram[offset]) & 0x8000)
		{
			/* generate the interrupt */
			atarigen_scanline_int_gen();
			atarigen_update_interrupts();

			/* also set a timer to turn ourself off */
			timer_set(cpu_getscanlineperiod(), 0, irq_off);
		}
}



/*************************************
 *
 *		Playfield RAM write handler
 *
 *************************************/

void blstroid_playfieldram_w(int offset, int data)
{
	int oldword = READ_WORD(&atarigen_playfieldram[offset]);
	int newword = COMBINE_WORD(oldword, data);

	if (oldword != newword)
	{
		WRITE_WORD(&atarigen_playfieldram[offset], newword);
		atarigen_pf_dirty[offset / 2] = 1;
	}
}



/*************************************
 *
 *		Priority RAM write handler
 *
 *************************************/

void blstroid_priorityram_w(int offset, int data)
{
	int shift, which;

	/* pick which playfield palette to look at */
	which = (offset >> 5) & 7;

	/* upper 16 bits are for H == 1, lower 16 for H == 0 */
	shift = (offset >> 4) & 0x10;
	shift += (offset >> 1) & 0x0f;

	/* set or clear the appropriate bit */
	priority[which] = (priority[which] & ~(1 << shift)) | ((data & 1) << shift);
}



/*************************************
 *
 *		Main refresh
 *
 *************************************/

void blstroid_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	/* remap if necessary */
	if (update_palette())
		memset(atarigen_pf_dirty, 1, atarigen_playfieldram_size / 2);

	/* draw the playfield */
	atarigen_pf_process(pf_render_callback, bitmap, &Machine->drv->visible_area);

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
	unsigned short pf_map[8], mo_map[16];
	int i, j;

	/* reset color tracking */
	memset(mo_map, 0, sizeof(mo_map));
	memset(pf_map, 0, sizeof(pf_map));
	palette_init_used_colors();

	/* update color usage for the playfield */
	atarigen_pf_process(pf_color_callback, pf_map, &Machine->drv->visible_area);

	/* update color usage for the mo's */
	atarigen_mo_process(mo_color_callback, mo_map);

	/* rebuild the playfield palette */
	for (i = 0; i < 8; i++)
	{
		unsigned short used = pf_map[i];
		if (used)
			for (j = 0; j < 16; j++)
				if (used & (1 << j))
					palette_used_colors[0x100 + i * 16 + j] = PALETTE_COLOR_USED;
	}

	/* rebuild the motion object palette */
	for (i = 0; i < 16; i++)
	{
		unsigned short used = mo_map[i];
		if (used)
		{
			palette_used_colors[0x000 + i * 16 + 0] = PALETTE_COLOR_TRANSPARENT;
			for (j = 1; j < 16; j++)
				if (used & (1 << j))
					palette_used_colors[0x000 + i * 16 + j] = PALETTE_COLOR_USED;
		}
	}

	return palette_recalc();
}



/*************************************
 *
 *		Playfield palette
 *
 *************************************/

static void pf_color_callback(const struct rectangle *clip, const struct rectangle *tiles, const struct atarigen_pf_state *state, void *param)
{
	const unsigned int *usage = Machine->gfx[0]->pen_usage;
	unsigned short *colormap = param;
	int x, y;

	/* standard loop over tiles */
	for (y = tiles->min_y; y != tiles->max_y; y = (y + 1) & 63)
		for (x = tiles->min_x; x != tiles->max_x; x = (x + 1) & 63)
		{
			int offs = y * 64 + x;
			int data = READ_WORD(&atarigen_playfieldram[offs * 2]);
			int code = data & 0x1fff;
			int color = data >> 13;

			/* mark the colors used by this tile */
			colormap[color] |= usage[code];
		}
}



/*************************************
 *
 *		Playfield rendering
 *
 *************************************/

static void pf_render_callback(const struct rectangle *clip, const struct rectangle *tiles, const struct atarigen_pf_state *state, void *param)
{
	const struct GfxElement *gfx = Machine->gfx[0];
	struct osd_bitmap *bitmap = param;
	int x, y;

	/* standard loop over tiles */
	for (y = tiles->min_y; y != tiles->max_y; y = (y + 1) & 63)
		for (x = tiles->min_x; x != tiles->max_x; x = (x + 1) & 63)
		{
			int offs = y * 64 + x;

			/* update only if dirty */
			if (atarigen_pf_dirty[offs])
			{
				int data = READ_WORD(&atarigen_playfieldram[offs * 2]);
				int code = data & 0x1fff;
				int color = data >> 13;

				drawgfx(atarigen_pf_bitmap, gfx, code, color, 0, 0, 16 * x, 8 * y, 0, TRANSPARENCY_NONE, 0);
				atarigen_pf_dirty[offs] = 0;
			}
		}

	/* then blast the result */
	copybitmap(bitmap, atarigen_pf_bitmap, 0, 0, 0, 0, clip, TRANSPARENCY_NONE, 0);
}



/*************************************
 *
 *		Playfield overrendering
 *
 *************************************/

static void pf_overrender_callback(const struct rectangle *clip, const struct rectangle *tiles, const struct atarigen_pf_state *state, void *param)
{
	const struct GfxElement *gfx = Machine->gfx[0];
	struct osd_bitmap *bitmap = param;
	int x, y;

	/* standard loop over tiles */
	for (y = tiles->min_y; y != tiles->max_y; y = (y + 1) & 63)
		for (x = tiles->min_x; x != tiles->max_x; x = (x + 1) & 63)
		{
			int offs = y * 64 + x;
			int data = READ_WORD(&atarigen_playfieldram[offs * 2]);
			int color = data >> 13;

			/* overrender if there is a non-zero priority for this color */
			/* not perfect, but works for the most obvious cases */
			if (!priority[color])
			{
				int code = data & 0x1fff;
				drawgfx(bitmap, gfx, code, color, 0, 0, 16 * x, 8 * y, clip, TRANSPARENCY_NONE, 0);
			}
		}
}



/*************************************
 *
 *		Motion object palette
 *
 *************************************/

static void mo_color_callback(const unsigned short *data, const struct rectangle *clip, void *param)
{
	const unsigned int *usage = Machine->gfx[1]->pen_usage;
	unsigned short *colormap = param;
	int vsize = (data[0] & 0x000f) + 1;
	int code = data[1] & 0x3fff;
	int color = data[3] & 0x000f;
	unsigned short temp = 0;
	int i;

	for (i = 0; i < vsize; i++)
		temp |= usage[code++];
	colormap[color] |= temp;;
}



/*************************************
 *
 *		Motion object rendering
 *
 *************************************/

static void mo_render_callback(const unsigned short *data, const struct rectangle *clip, void *param)
{
	const struct GfxElement *gfx = Machine->gfx[1];
	struct osd_bitmap *bitmap = param;
	struct rectangle pf_clip;

	/* extract data from the various words */
	int ypos = -(data[0] >> 7);
	int vsize = (data[0] & 0x000f) + 1;
	int hflip = data[1] & 0x8000;
	int vflip = data[1] & 0x4000;
	int code = data[1] & 0x3fff;
	int xpos = (data[3] >> 7) << 1;
	int color = data[3] & 0x000f;

	/* adjust for height */
	ypos -= vsize * 8;

	/* adjust the final coordinates */
	xpos &= 0x3ff;
	ypos &= 0x1ff;
	if (xpos >= XDIM) xpos -= 0x400;
	if (ypos >= YDIM) ypos -= 0x200;

	/* clip the X coordinate */
	if (xpos <= -16 || xpos >= XDIM)
		return;

	/* determine the bounding box */
	atarigen_mo_compute_clip_16x8(pf_clip, xpos, ypos, 1, vsize, clip);

	/* draw the motion object */
	atarigen_mo_draw_16x8_strip(bitmap, gfx, code, color, hflip, vflip, xpos, ypos, vsize, clip, TRANSPARENCY_PEN, 0);

	/* overrender the playfield */
	atarigen_pf_process(pf_overrender_callback, bitmap, &pf_clip);
}



/*************************************
 *
 *		Automatic IRQ shutoff
 *
 *************************************/

static void irq_off(int param)
{
	atarigen_scanline_int_ack_w(0, 0);
}



/*************************************
 *
 *		Debugging
 *
 *************************************/

#if DEBUG_VIDEO

static int debug(void)
{
	static unsigned long oldpri[8];
	int hidebank = -1;

	if (memcmp(oldpri, priority, sizeof(oldpri)))
	{
		static FILE *f;
		int i;
		if (!f) f = fopen("priority.log", "w");
		for (i = 0; i < 8; i++)
			fprintf(f, "%08lX ", priority[i]);
		fprintf(f, "\n");
		memcpy(oldpri, priority, sizeof(oldpri));
	}

	if (keyboard_key_pressed(KEYCODE_Q)) hidebank = 0;
	if (keyboard_key_pressed(KEYCODE_W)) hidebank = 1;
	if (keyboard_key_pressed(KEYCODE_E)) hidebank = 2;
	if (keyboard_key_pressed(KEYCODE_R)) hidebank = 3;
	if (keyboard_key_pressed(KEYCODE_T)) hidebank = 4;
	if (keyboard_key_pressed(KEYCODE_Y)) hidebank = 5;
	if (keyboard_key_pressed(KEYCODE_U)) hidebank = 6;
	if (keyboard_key_pressed(KEYCODE_I)) hidebank = 7;

	if (keyboard_key_pressed(KEYCODE_A)) hidebank = 8;
	if (keyboard_key_pressed(KEYCODE_S)) hidebank = 9;
	if (keyboard_key_pressed(KEYCODE_D)) hidebank = 10;
	if (keyboard_key_pressed(KEYCODE_F)) hidebank = 11;
	if (keyboard_key_pressed(KEYCODE_G)) hidebank = 12;
	if (keyboard_key_pressed(KEYCODE_H)) hidebank = 13;
	if (keyboard_key_pressed(KEYCODE_J)) hidebank = 14;
	if (keyboard_key_pressed(KEYCODE_K)) hidebank = 15;

	if (keyboard_key_pressed(KEYCODE_9))
	{
		static int count;
		char name[50];
		FILE *f;
		int i;

		while (keyboard_key_pressed(KEYCODE_9)) { }

		sprintf(name, "Dump %d", ++count);
		f = fopen(name, "wt");

		fprintf(f, "\n\nMotion Object Palette:\n");
		for (i = 0x000; i < 0x100; i++)
		{
			fprintf(f, "%04X ", READ_WORD(&paletteram[i*2]));
			if ((i & 15) == 15) fprintf(f, "\n");
		}

		fprintf(f, "\n\nPlayfield Palette:\n");
		for (i = 0x100; i < 0x200; i++)
		{
			fprintf(f, "%04X ", READ_WORD(&paletteram[i*2]));
			if ((i & 15) == 15) fprintf(f, "\n");
		}

		fprintf(f, "\n\nMotion Objects\n");
		for (i = 0; i < 0x40; i++)
		{
			fprintf(f, "   Object %02X:  Y=%04X  P=%04X  L=%04X  X=%04X\n",
					i,
					READ_WORD(&atarigen_spriteram[i*8+0]),
					READ_WORD(&atarigen_spriteram[i*8+2]),
					READ_WORD(&atarigen_spriteram[i*8+4]),
					READ_WORD(&atarigen_spriteram[i*8+6])
			);
		}

		fprintf(f, "\n\nPlayfield dump\n");
		for (i = 0; i < atarigen_playfieldram_size / 2; i++)
		{
			fprintf(f, "%04X ", READ_WORD(&atarigen_playfieldram[i*2]));
			if ((i & 63) == 63) fprintf(f, "\n");
		}

		fclose(f);
	}

	return hidebank;
}

#endif
