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

int vindctr2_screen_refresh;



/*************************************
 *
 *		Statics
 *
 *************************************/

static struct tilemap *playfield;
static struct tilemap *alpha;

static int playfield_bank;
static int playfield_color_base;

static int xscroll;
static int yscroll;


#if 0
static void vindictr_debug(void);
#endif



/*************************************
 *
 *		TileMap callbacks
 *
 *************************************/

/*-----------------------------------------------------
 * 	Playfield encoding
 *
 *		64x64 tile scrolling grid, with X/Y swapped
 *
 *		1 16-bit word is used per tile
 *
 *			Bits 0-11  = image
 *			Bits 12-14 = color
 *			Bit  15    = horizontal flip
 *---------------------------------------------------*/

static void playfield_get_info(int col, int row)
{
	int tile_index = 64 * col + row;
	int data = READ_WORD(&atarigen_playfieldram[tile_index * 2]);
	int code = playfield_bank + ((data & 0xfff) ^ 0x800);
	int color = playfield_color_base + ((data >> 12) & 7);

	SET_TILE_INFO(1, code, color);
	tile_info.flags = (data & 0x8000) ? TILE_FLIPX : 0;
}


/*-----------------------------------------------------
 * 	Alpha layer encoding
 *
 *		64x64 tile fixed grid, only 42x30 is visible
 *
 *		1 16-bit word is used per tile
 *
 *			Bits 0-9   = index of the character
 *			Bit  10-13 = color
 *			Bit  15    = transparent/opaque
 *---------------------------------------------------*/

static void alpha_get_info(int col, int row)
{
	int tile_index = 64 * row + col;
	int data = READ_WORD(&atarigen_alpharam[tile_index * 2]);
	int code = data & 0x3ff;
	int color = ((data >> 10) & 0xf) | ((data >> 9) & 0x20);

	SET_TILE_INFO(0, code, color);
	tile_info.flags = (data & 0x8000) ? TILE_IGNORE_TRANSPARENCY : 0;
}



/*************************************
 *
 *		Video system start
 *
 *************************************/

int gauntlet_vh_start(void)
{
	static struct atarigen_modesc gauntlet_modesc =
	{
		1024,                /* maximum number of MO's */
		2,                   /* number of bytes per MO entry */
		0x800,               /* number of bytes between MO words */
		3,                   /* ignore an entry if this word == 0xffff */
		3, 0, 0x3ff,         /* link = (data[linkword] >> linkshift) & linkmask */
		0                    /* render in reverse link order */
	};

	/* allocate playfield tilemap */
	playfield = tilemap_create(playfield_get_info, TILEMAP_OPAQUE, 8,8, 64,64);
	if (!playfield)
		return 1;
	tilemap_set_scroll_cols(playfield, 1);
	tilemap_set_scroll_rows(playfield, 1);

	/* allocate alpha tilemap */
	alpha = tilemap_create(alpha_get_info, TILEMAP_TRANSPARENT, 8,8, 64,64);
	if (!alpha)
		return 1;
	alpha->transparent_pen = 0;

	/* set up the appropriate variables for Gauntlet vs. Vindicators 2 */
	if (vindctr2_screen_refresh)
		playfield_color_base = 0x10;
	else
		playfield_color_base = 0x18;

	/* initialize the displaylist system */
	return atarigen_init_display_list(&gauntlet_modesc);
}



/*************************************
 *
 *		Video system shutdown
 *
 *************************************/

void gauntlet_vh_stop(void)
{
}



/*************************************
 *
 *		Scroll/bank registers
 *
 *************************************/

void gauntlet_hscroll_w(int offset, int data)
{
	/* update memory */
	int oldword = READ_WORD(&atarigen_hscroll[offset]);
	int newword = COMBINE_WORD(oldword, data);
	WRITE_WORD(&atarigen_hscroll[offset], newword);

	/* set the new scroll */
	xscroll = newword & 0x1ff;
	tilemap_set_scrollx(playfield, 0, -xscroll);
}


void gauntlet_vscroll_w(int offset, int data)
{
	int temp;

	/* update memory */
	int oldword = READ_WORD(&atarigen_vscroll[offset]);
	int newword = COMBINE_WORD(oldword, data);
	WRITE_WORD(&atarigen_vscroll[offset], newword);

	/* set the new scroll */
	yscroll = (newword >> 7) & 0x1ff;
	tilemap_set_scrolly(playfield, 0, -yscroll);

	/* set the new ROM bank */
	temp = (newword & 3) << 12;
	if (temp != playfield_bank)
	{
		playfield_bank = temp;
		tilemap_mark_all_tiles_dirty(playfield);
	}
}



/*************************************
 *
 *		Playfield/alpha RAM
 *
 *************************************/

void gauntlet_playfieldram_w(int offset, int data)
{
	int oldword = READ_WORD(&atarigen_playfieldram[offset]);
	int newword = COMBINE_WORD(oldword, data);
	if (oldword != newword)
	{
		WRITE_WORD(&atarigen_playfieldram[offset], newword);
		tilemap_mark_tile_dirty(playfield, (offset / 2) / 64, (offset / 2) % 64);
	}
}


void gauntlet_alpharam_w(int offset, int data)
{
	int oldword = READ_WORD(&atarigen_alpharam[offset]);
	int newword = COMBINE_WORD(oldword, data);
	if (oldword != newword)
	{
		WRITE_WORD(&atarigen_alpharam[offset], newword);
		tilemap_mark_tile_dirty(alpha, (offset / 2) % 64, (offset / 2) / 64);
	}
}



/*************************************
 *
 *		Motion object list handlers
 *
 *************************************/

/*-----------------------------------------------------
 * 	Motion Object encoding
 *
 *		4 16-bit words are used
 *
 *		Word 1:
 *			Bits 0-14  = index of the image (0-32767)
 *
 *		Word 2:
 *			Bits 0-3   = color
 *			Bits 7-15  = X position
 *
 *		Word 3:
 *			Bits 0-2   = vertical tiles - 1
 *			Bits 3-5   = horizontal tiles - 1
 *			Bit  6     = horizontal flip
 *			Bits 7-14  = Y position
 *
 *		Word 4:
 *			Bits 0-9   = link to the next object
 *---------------------------------------------------*/

static void render_mo(struct osd_bitmap *bitmap, struct rectangle *clip, unsigned short *data, void *param)
{
	int sx, sy, x, y, xadv;

	/* extract data from the various words */
	int pict = data[0] & 0x7fff;
	int color = data[1] & 0x0f;
	int xpos = (data[1] >> 7) - xscroll;
	int vsize = (data[2] & 7) + 1;
	int hsize = ((data[2] >> 3) & 7) + 1;
	int hflip = data[2] & 0x40;
	int ypos = -(data[2] >> 7) - vsize * 8 - yscroll;

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

			/* draw the mo */
			drawgfx(bitmap, Machine->gfx[1],
					pict ^ 0x800, color, hflip, 0, sx, sy, clip, TRANSPARENCY_PEN, 0);
		}
	}
}


int gauntlet_update_display_list(int scanline)
{
	/* look up the SLIP link */
	int link = READ_WORD(&atarigen_alpharam[0xf80 + 2 * (((scanline + yscroll) / 8) & 0x3f)]) & 0x3ff;
	atarigen_update_display_list(atarigen_spriteram, link, scanline);
	return yscroll;
}


/*************************************
 *
 *		Motion object palette
 *
 *************************************/

static void calc_mo_colors(struct osd_bitmap *bitmap, struct rectangle *clip, unsigned short *data, void *param)
{
	unsigned char *colors = param;
	int color = data[1] & 0x0f;
	colors[color] = 1;
}


static void mark_mo_colors(void)
{
	unsigned char mo_map[16];
	int i;

	/* update color usage for the mo's */
	memset(mo_map, 0, sizeof(mo_map));
	atarigen_render_display_list(NULL, calc_mo_colors, mo_map);

	/* rebuild the palette */
	for (i = 0; i < 16; i++)
		if (mo_map[i])
		{
			palette_used_colors[256 + i * 16] = PALETTE_COLOR_TRANSPARENT;
			memset(&palette_used_colors[256 + i * 16 + 1], PALETTE_COLOR_USED, 15);
		}
}


/*************************************
 *
 *		Main screen refresh
 *
 *************************************/

void gauntlet_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
#if 0
	vindictr_debug();
#endif

	/* update the tilemaps */
	tilemap_update(ALL_TILEMAPS);

	/* handle the palette */
	palette_init_used_colors();
	mark_mo_colors();
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	/* render the tilemaps */
	tilemap_render(ALL_TILEMAPS);

	/* draw it all */
	tilemap_draw(bitmap, playfield, 0);
	atarigen_render_display_list(bitmap, render_mo, NULL);
	tilemap_draw(bitmap, alpha, 0);
}


/*************************************
 *
 *		Debugging
 *
 *************************************/

#if 0

void vindictr_dump_mo(struct osd_bitmap *bitmap, struct rectangle *clip, unsigned short *data, void *param)
{
	FILE *f = param;

	/* extract data from the various words */
	int pict = data[0] & 0x7fff;
	int color = data[1] & 0x0f;
	int xpos = xscroll + (data[1] >> 7);
	int vsize = (data[2] & 7) + 1;
	int hsize = ((data[2] >> 3) & 7) + 1;
	int hflip = data[2] & 0x40;
	int ypos = yscroll - (data[2] >> 7) - vsize * 8;

	fprintf(f, "PICT=%04X color=%1X X=%03X Y=%03X SIZE=%dx%d FLIP=%d\n",
		pict, color, xpos, ypos, hsize, vsize, hflip);
}


static void vindictr_debug(void)
{
	if (osd_key_pressed(OSD_KEY_9))
	{
		static int count;
		char name[50];
		FILE *f;
		int i;

		while (osd_key_pressed(OSD_KEY_9)) { }

		sprintf(name, "Dump %d", ++count);
		f = fopen(name, "wt");

		fprintf(f, "\n\nAlpha Palette:\n");
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

		fprintf(f, "\n\nPlayfield Palette:\n");
		for (i = 0x200; i < 0x400; i++)
		{
			fprintf(f, "%04X ", READ_WORD(&paletteram[i*2]));
			if ((i & 15) == 15) fprintf(f, "\n");
		}

		fprintf(f, "\n\nPlayfield dump\n");
		for (i = 0; i < atarigen_playfieldram_size / 2; i++)
		{
			fprintf(f, "%04X ", READ_WORD(&atarigen_playfieldram[i*2]));
			if ((i & 63) == 63) fprintf(f, "\n");
		}

		fprintf(f, "\n\nAlpha dump\n");
		for (i = 0; i < atarigen_alpharam_size / 2; i++)
		{
			fprintf(f, "%04X ", READ_WORD(&atarigen_alpharam[i*2]));
			if ((i & 63) == 63) fprintf(f, "\n");
		}

		fprintf(f,"\n\nMotion objects\n");
		atarigen_render_display_list(NULL, vindictr_dump_mo, f);

		fclose(f);
	}
}
#endif
