/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#ifndef WILLIAMS_BLITTERS


#include "driver.h"
#include "vidhrdw/generic.h"


#define VIDEORAM_WIDTH		304
#define VIDEORAM_HEIGHT		256
#define VIDEORAM_SIZE		(VIDEORAM_WIDTH * VIDEORAM_HEIGHT)


/* globals from machine/williams.c */
extern UINT8 williams2_bank;
extern UINT16 sinistar_clip;
extern UINT8 williams_cocktail;

/* RAM globals */
UINT8 *williams_videoram;
static UINT8 *williams_videoram_copy;
UINT8 *williams2_paletteram;

/* blitter variables */
UINT8 *williams_blitterram;
UINT8 williams_blitter_xor;
UINT8 williams_blitter_remap;
UINT8 williams_blitter_clip;

/* Blaster extra variables */
UINT8 *blaster_video_bits;
UINT8 *blaster_color_zero_flags;
UINT8 *blaster_color_zero_table;
static const UINT8 *blaster_remap;
static UINT8 *blaster_remap_lookup;

/* tilemap variables */
UINT8 williams2_tilemap_mask;
const UINT8 *williams2_row_to_palette; /* take care of IC79 and J1/J2 */
UINT8 williams2_M7_flip;
INT8  williams2_videoshift;
UINT8 williams2_special_bg_color;
static UINT8 williams2_fg_color; /* IC90 */
static UINT8 williams2_bg_color; /* IC89 */

/* later-Williams video control variables */
UINT8 *williams2_blit_inhibit;
UINT8 *williams2_xscroll_low;
UINT8 *williams2_xscroll_high;

/* control routines */
void williams2_vh_stop(void);

/* pixel copiers */
static UINT8 *scanline_dirty;

/* blitter functions */
static void williams_blit_opaque(int sstart, int dstart, int w, int h, int data);
static void williams_blit_transparent(int sstart, int dstart, int w, int h, int data);
static void williams_blit_opaque_solid(int sstart, int dstart, int w, int h, int data);
static void williams_blit_transparent_solid(int sstart, int dstart, int w, int h, int data);
static void sinistar_blit_opaque(int sstart, int dstart, int w, int h, int data);
static void sinistar_blit_transparent(int sstart, int dstart, int w, int h, int data);
static void sinistar_blit_opaque_solid(int sstart, int dstart, int w, int h, int data);
static void sinistar_blit_transparent_solid(int sstart, int dstart, int w, int h, int data);
static void blaster_blit_opaque(int sstart, int dstart, int w, int h, int data);
static void blaster_blit_transparent(int sstart, int dstart, int w, int h, int data);
static void blaster_blit_opaque_solid(int sstart, int dstart, int w, int h, int data);
static void blaster_blit_transparent_solid(int sstart, int dstart, int w, int h, int data);
static void williams2_blit_opaque(int sstart, int dstart, int w, int h, int data);
static void williams2_blit_transparent(int sstart, int dstart, int w, int h, int data);
static void williams2_blit_opaque_solid(int sstart, int dstart, int w, int h, int data);
static void williams2_blit_transparent_solid(int sstart, int dstart, int w, int h, int data);

/* blitter tables */
static void (**blitter_table)(int, int, int, int, int);

static void (*williams_blitters[])(int, int, int, int, int) =
{
	williams_blit_opaque,
	williams_blit_transparent,
	williams_blit_opaque_solid,
	williams_blit_transparent_solid
};

static void (*sinistar_blitters[])(int, int, int, int, int) =
{
	sinistar_blit_opaque,
	sinistar_blit_transparent,
	sinistar_blit_opaque_solid,
	sinistar_blit_transparent_solid
};

static void (*blaster_blitters[])(int, int, int, int, int) =
{
	blaster_blit_opaque,
	blaster_blit_transparent,
	blaster_blit_opaque_solid,
	blaster_blit_transparent_solid
};

static void (*williams2_blitters[])(int, int, int, int, int) =
{
	williams2_blit_opaque,
	williams2_blit_transparent,
	williams2_blit_opaque_solid,
	williams2_blit_transparent_solid
};



/*************************************
 *
 *	Copy pixels from videoram to
 *	the screen bitmap
 *
 *************************************/

static void copy_pixels(struct mame_bitmap *bitmap, const struct rectangle *clip, int transparent_pen)
{
	int blaster_back_color = 0;
	int pairs = (clip->max_x - clip->min_x + 1) / 2;
	int xoffset = clip->min_x;
	int x, y;

	/* loop over rows */
	for (y = clip->min_y; y <= clip->max_y; y++)
	{
		const UINT8 *source = &williams_videoram_copy[y + 256 * (xoffset / 2)];
		UINT8 scanline[400];
		UINT8 *dest = scanline;

		/* skip if not dirty (but only for non-transparent drawing) */
		if (transparent_pen == -1 && !williams_blitter_remap)
		{
			if (!scanline_dirty[y])
				continue;
			scanline_dirty[y]--;
		}

		/* mark the pixels dirty */
		mark_dirty(clip->min_x, y, clip->max_x, y);

		/* draw all pairs */
		for (x = 0; x < pairs; x++, source += 256)
		{
			int pix = *source;
			*dest++ = pix >> 4;
			*dest++ = pix & 0x0f;
		}

		/* handle general case */
		if (!williams_blitter_remap)
			draw_scanline8(bitmap, xoffset, y, pairs * 2, scanline, Machine->pens, transparent_pen);

		/* handle Blaster special case */
		else
		{
			UINT8 saved_pen0;

			/* pick the background pen */
			if (*blaster_video_bits & 1)
			{
				if (blaster_color_zero_flags[y] & 1)
					blaster_back_color = 16 + y - Machine->visible_area.min_y;
			}
			else
				blaster_back_color = 0;

			/* draw the scanline, temporarily remapping pen 0 */
			saved_pen0 = Machine->pens[0];
			Machine->pens[0] = Machine->pens[blaster_back_color];
			draw_scanline8(bitmap, xoffset, y, pairs * 2, scanline, Machine->pens, transparent_pen);
			Machine->pens[0] = saved_pen0;
		}
	}
}



/*************************************
 *
 *	Early Williams video startup/shutdown
 *
 *************************************/

int williams_vh_start(void)
{
	/* allocate space for video RAM and dirty scanlines */
	williams_videoram = malloc(2 * VIDEORAM_SIZE + 256);
	if (!williams_videoram)
		return 1;
	williams_videoram_copy = williams_videoram + VIDEORAM_SIZE;
	scanline_dirty = williams_videoram_copy + VIDEORAM_SIZE;

	/* reset everything */
	memset(williams_videoram, 0, VIDEORAM_SIZE);
	memset(williams_videoram_copy, 0, VIDEORAM_SIZE);
	memset(scanline_dirty, 2, 256);

	/* pick the blitters */
	blitter_table = williams_blitters;
	if (williams_blitter_remap) blitter_table = blaster_blitters;
	if (williams_blitter_clip) blitter_table = sinistar_blitters;

	/* reset special-purpose flags */
	blaster_remap_lookup = 0;
	sinistar_clip = 0xffff;

	return 0;
}


void williams_vh_stop(void)
{
	/* free any remap lookup tables */
	if (blaster_remap_lookup)
		free(blaster_remap_lookup);
	blaster_remap_lookup = NULL;

	/* free video RAM */
	if (williams_videoram)
		free(williams_videoram);
	williams_videoram = williams_videoram_copy = NULL;
	scanline_dirty = NULL;
}



/*************************************
 *
 *	Early Williams video update
 *
 *************************************/

void williams_vh_update(int scanline)
{
	int erase_behind = 0;
	UINT32 *srcbase, *dstbase;
	int x;

	/* wrap around at the bottom */
	if (scanline == 0)
		scanline = 256;

	/* should we erase as we draw? */
	if (williams_blitter_remap && scanline >= 32 && (*blaster_video_bits & 0x02))
		erase_behind = 1;

	/* determine the source and destination */
 	srcbase = (UINT32 *)&williams_videoram[scanline - 8];
 	dstbase = (UINT32 *)&williams_videoram_copy[scanline - 8];

 	/* loop over columns and copy a 16-row chunk */
 	for (x = 0; x < VIDEORAM_WIDTH/2; x++)
 	{
 		/* copy 16 rows' worth of data */
 		dstbase[0] = srcbase[0];
 		dstbase[1] = srcbase[1];

		/* handle Blaster autoerase for scanlines 24 and up */
		if (erase_behind)
			srcbase[0] = srcbase[1] = 0;

 		/* advance to the next column */
 		srcbase += 256/4;
 		dstbase += 256/4;
 	}
}


void williams_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	/* full refresh forces us to redraw everything */
	if (full_refresh)
		memset(scanline_dirty, 2, 256);

	/* copy the pixels into the final result */
	copy_pixels(bitmap, &Machine->visible_area, -1);
}



/*************************************
 *
 *	Early Williams video I/O
 *
 *************************************/

WRITE_HANDLER( williams_videoram_w )
{
	/* only update if different */
	if (williams_videoram[offset] != data)
	{
		/* store to videoram and mark the scanline dirty */
		williams_videoram[offset] = data;
		scanline_dirty[offset % 256] = 2;
	}
}


READ_HANDLER( williams_video_counter_r )
{
	return cpu_getscanline() & 0xfc;
}



/*************************************
 *
 *	Later Williams video startup/shutdown
 *
 *************************************/

int williams2_vh_start(void)
{
	/* standard initialization */
	if (williams_vh_start())
		return 1;

	/* override the blitters */
	blitter_table = williams2_blitters;

	/* allocate a buffer for palette RAM */
	williams2_paletteram = malloc(4 * 1024 * 4 / 8);
	if (!williams2_paletteram)
	{
		williams2_vh_stop();
		return 1;
	}

	/* clear it */
	memset(williams2_paletteram, 0, 4 * 1024 * 4 / 8);

	/* reset the FG/BG colors */
	williams2_fg_color = 0;
	williams2_bg_color = 0;

	return 0;
}


void williams2_vh_stop(void)
{
	/* free palette RAM */
	if (williams2_paletteram)
		free(williams2_paletteram);
	williams2_paletteram = NULL;

	/* clean up other stuff */
	williams_vh_stop();
}



/*************************************
 *
 *	Later Williams video update
 *
 *************************************/

void williams2_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	UINT8 *tileram = &memory_region(REGION_CPU1)[0xc000];
	int xpixeloffset, xtileoffset;
	int color, col, y;

	/* full refresh forces us to redraw everything */
	if (full_refresh)
		memset(scanline_dirty, 2, 256);

	/* assemble the bits that describe the X scroll offset */
	xpixeloffset = (*williams2_xscroll_high & 1) * 12 +
	               (*williams2_xscroll_low >> 7) * 6 +
	               (*williams2_xscroll_low & 7) +
	               williams2_videoshift;
	xtileoffset = *williams2_xscroll_high >> 1;

	/* adjust the offset for the row and compute the palette index */
	for (y = 0; y < 256; y += 16, tileram++)
	{
		color = williams2_row_to_palette[y / 16];

		/* 12 columns wide, each block is 24 pixels wide, 288 pixel lines */
		for (col = 0; col <= 12; col++)
		{
			unsigned int map = tileram[((col + xtileoffset) * 16) & 0x07ff];

			drawgfx(bitmap, Machine->gfx[0], map & williams2_tilemap_mask,
					color, map & williams2_M7_flip, 0, col * 24 - xpixeloffset, y,
					&Machine->visible_area, TRANSPARENCY_NONE, 0);
		}
	}

	/* copy the bitmap data on top of that */
	copy_pixels(bitmap, &Machine->visible_area, 0);
}



/*************************************
 *
 *	Later Williams palette I/O
 *
 *************************************/

static void williams2_modify_color(int color, int offset)
{
	static const UINT8 ztable[16] =
	{
		0x0, 0x3, 0x4,  0x5, 0x6, 0x7, 0x8,  0x9,
		0xa, 0xb, 0xc,  0xd, 0xe, 0xf, 0x10, 0x11
	};

	UINT8 entry_lo = williams2_paletteram[offset * 2];
	UINT8 entry_hi = williams2_paletteram[offset * 2 + 1];
	UINT8 i = ztable[(entry_hi >> 4) & 15];
	UINT8 b = ((entry_hi >> 0) & 15) * i;
	UINT8 g = ((entry_lo >> 4) & 15) * i;
	UINT8 r = ((entry_lo >> 0) & 15) * i;

	palette_set_color(color, r, g, b);
}


static void williams2_update_fg_color(unsigned int offset)
{
	unsigned int page_offset = williams2_fg_color * 16;

	/* only modify the palette if we're talking to the current page */
	if (offset >= page_offset && offset < page_offset + 16)
		williams2_modify_color(offset - page_offset, offset);
}


static void williams2_update_bg_color(unsigned int offset)
{
	unsigned int page_offset = williams2_bg_color * 16;

	/* non-Mystic Marathon variant */
	if (!williams2_special_bg_color)
	{
		/* only modify the palette if we're talking to the current page */
		if (offset >= page_offset && offset < page_offset + Machine->drv->total_colors - 16)
			williams2_modify_color(offset - page_offset + 16, offset);
	}

	/* Mystic Marathon variant */
	else
	{
		/* only modify the palette if we're talking to the current page */
		if (offset >= page_offset && offset < page_offset + 16)
			williams2_modify_color(offset - page_offset + 16, offset);

		/* check the secondary palette as well */
		page_offset |= 0x10;
		if (offset >= page_offset && offset < page_offset + 16)
			williams2_modify_color(offset - page_offset + 32, offset);
	}
}


WRITE_HANDLER( williams2_fg_select_w )
{
	unsigned int i, palindex;

	/* if we're already mapped, leave it alone */
	if (williams2_fg_color == data)
		return;
	williams2_fg_color = data & 0x3f;

	/* remap the foreground colors */
	palindex = williams2_fg_color * 16;
	for (i = 0; i < 16; i++)
		williams2_modify_color(i, palindex++);
}


WRITE_HANDLER( williams2_bg_select_w )
{
	unsigned int i, palindex;

	/* if we're already mapped, leave it alone */
	if (williams2_bg_color == data)
		return;
	williams2_bg_color = data & 0x3f;

	/* non-Mystic Marathon variant */
	if (!williams2_special_bg_color)
	{
		/* remap the background colors */
		palindex = williams2_bg_color * 16;
		for (i = 16; i < Machine->drv->total_colors; i++)
			williams2_modify_color(i, palindex++);
	}

	/* Mystic Marathon variant */
	else
	{
		/* remap the background colors */
		palindex = williams2_bg_color * 16;
		for (i = 16; i < 32; i++)
			williams2_modify_color(i, palindex++);

		/* remap the secondary background colors */
		palindex = (williams2_bg_color | 1) * 16;
		for (i = 32; i < 48; i++)
			williams2_modify_color(i, palindex++);
	}
}



/*************************************
 *
 *	Later Williams video I/O
 *
 *************************************/

WRITE_HANDLER( williams2_videoram_w )
{
	/* bank 3 doesn't touch the screen */
	if ((williams2_bank & 0x03) == 0x03)
	{
		/* bank 3 from $8000 - $8800 affects palette RAM */
		if (offset >= 0x8000 && offset < 0x8800)
		{
			offset -= 0x8000;
			williams2_paletteram[offset] = data;

			/* update the palette value if necessary */
			offset >>= 1;
			williams2_update_fg_color(offset);
			williams2_update_bg_color(offset);
		}
		return;
	}

	/* everyone else talks to the screen */
	williams_videoram[offset] = data;
}



/*************************************
 *
 *	Blaster-specific video start
 *
 *************************************/

int blaster_vh_start(void)
{
	int i, j;

	/* standard startup first */
	if (williams_vh_start())
		return 1;

	/* Expand the lookup table so that we do one lookup per byte */
	blaster_remap_lookup = malloc(256 * 256);
	if (blaster_remap_lookup)
		for (i = 0; i < 256; i++)
		{
			const UINT8 *table = memory_region(REGION_PROMS) + (i & 0x7f) * 16;
			for (j = 0; j < 256; j++)
				blaster_remap_lookup[i * 256 + j] = (table[j >> 4] << 4) | table[j & 0x0f];
		}

	return 0;
}



/*************************************
 *
 *	Blaster-specific enhancements
 *
 *************************************/

WRITE_HANDLER( blaster_remap_select_w )
{
	blaster_remap = blaster_remap_lookup + data * 256;
}


WRITE_HANDLER( blaster_palette_0_w )
{
	blaster_color_zero_table[offset] = data;
	data ^= 0xff;
	if (offset >= Machine->visible_area.min_y && offset <= Machine->visible_area.max_y)
	{
		int r = data & 7;
		int g = (data >> 3) & 7;
		int b = (data >> 6) & 3;

		r = (r << 5) | (r << 2) | (r >> 1);
		g = (g << 5) | (g << 2) | (g >> 1);
		b = (b << 6) | (b << 4) | (b << 2) | b;
		palette_set_color(16 + offset - Machine->visible_area.min_y, r, g, b);
	}
}



/*************************************
 *
 *	Blitter core
 *
 *************************************/

/*

	Blitter description from Sean Riddle's page:

	This page contains information about the Williams Special Chips, which
	were 'bit blitters'- block transfer chips that could move data around on
	the screen and in memory faster than the CPU. In fact, I've timed the
	special chips at 16 megs in 18.1 seconds. That's 910K/sec, not bad for
	the early 80s.

	The blitters were not used in Defender and Stargate, but
	were added to the ROM boards of the later games. Splat!, Blaster, Mystic
	Marathon and Joust 2 used Special Chip 2s. The only difference that I've
	seen is that SC1s have a small bug. When you tell the SC1 the size of
	the data to move, you have to exclusive-or the width and height with 2.
	The SC2s eliminate this bug.

	The blitters were accessed at memory location $CA00-CA06.

	CA01 is the mask, usually $FF to move all bits.
	CA02-3 is the source data location.
	CA04-5 is the destination data location.

	Writing to CA00 starts the blit, and the byte written determines how the
	data is blitted.

	Bit 0 indicates that the source data is either laid out linear, one
	pixel after the last, or in screen format, where there are 256 bytes from
	one pair of pixels to the next.

	Bit 1 indicates the same, but for the destination data.

	I'm not sure what bit 2 does. Looking at the image, I can't tell, but
	perhaps it has to do with the mask. My test files only used a mask of $FF.

	Bit 3 tells the blitter only to blit the foreground- that is, everything
	that is not color 0. Also known as transparency mode.

	Bit 4 is 'solid' mode. Only the color indicated by the mask is blitted.
	Note that this just creates a rectangle unless bit 3 is also set, in which
	case it blits the image, but in a solid color.

	Bit 5 shifts the image one pixel to the right. Any data on the far right
	jumps to the far left.

	Bits 6 and 7 only blit every other pixel of the image. Bit 6 says even only,
	while bit 7 says odd only.

*/



/*************************************
 *
 *	Blitter I/O
 *
 *************************************/

WRITE_HANDLER( williams_blitter_w )
{
	int sstart, dstart, w, h, count;

	/* store the data */
	williams_blitterram[offset] = data;

	/* only writes to location 0 trigger the blit */
	if (offset != 0)
		return;

	/* compute the starting locations */
	sstart = (williams_blitterram[2] << 8) + williams_blitterram[3];
	dstart = (williams_blitterram[4] << 8) + williams_blitterram[5];

	/* compute the width and height */
	w = williams_blitterram[6] ^ williams_blitter_xor;
	h = williams_blitterram[7] ^ williams_blitter_xor;

	/* adjust the width and height */
	if (w == 0) w = 1;
	if (h == 0) h = 1;
	if (w == 255) w = 256;
	if (h == 255) h = 256;

	/* call the appropriate blitter */
	(*blitter_table[(data >> 3) & 3])(sstart, dstart, w, h, data);

	/* compute the ending address */
	if (data & 0x02)
		count = h;
	else
		count = w + w * h;
	if (count > 256) count = 256;

	/* mark dirty */
	w = dstart % 256;
	while (count-- > 0)
		scanline_dirty[w++ % 256] = 2;

	/* Log blits */
	logerror("---------- Blit %02X--------------PC: %04X\n",data,cpu_get_pc());
	logerror("Source : %02X %02X\n",williams_blitterram[2],williams_blitterram[3]);
	logerror("Dest   : %02X %02X\n",williams_blitterram[4],williams_blitterram[5]);
	logerror("W H    : %02X %02X (%d,%d)\n",williams_blitterram[6],williams_blitterram[7],williams_blitterram[6]^4,williams_blitterram[7]^4);
	logerror("Mask   : %02X\n",williams_blitterram[1]);
}



/*************************************
 *
 *	Blitter macros
 *
 *************************************/

/* blit with pixel color 0 == transparent */
#define BLIT_TRANSPARENT(offset, data, keepmask)		\
{														\
	data = REMAP(data);									\
	if (data)											\
	{													\
		int pix = BLITTER_DEST_READ(offset);			\
		int tempmask = keepmask;						\
														\
		if (!(data & 0xf0)) tempmask |= 0xf0;			\
		if (!(data & 0x0f)) tempmask |= 0x0f;			\
														\
		pix = (pix & tempmask) | (data & ~tempmask);	\
		BLITTER_DEST_WRITE(offset, pix);				\
	}													\
}

/* blit with pixel color 0 == transparent, other pixels == solid color */
#define BLIT_TRANSPARENT_SOLID(offset, data, keepmask)	\
{														\
	data = REMAP(data);									\
	if (data)											\
	{													\
		int pix = BLITTER_DEST_READ(offset);			\
		int tempmask = keepmask;						\
														\
		if (!(data & 0xf0)) tempmask |= 0xf0;			\
		if (!(data & 0x0f)) tempmask |= 0x0f;			\
														\
		pix = (pix & tempmask) | (solid & ~tempmask);	\
		BLITTER_DEST_WRITE(offset, pix);				\
	}													\
}

/* blit with no transparency */
#define BLIT_OPAQUE(offset, data, keepmask)				\
{														\
	int pix = BLITTER_DEST_READ(offset);				\
	data = REMAP(data);									\
	pix = (pix & keepmask) | (data & ~keepmask);		\
	BLITTER_DEST_WRITE(offset, pix);					\
}

/* blit with no transparency in a solid color */
#define BLIT_OPAQUE_SOLID(offset, data, keepmask)		\
{														\
	int pix = BLITTER_DEST_READ(offset);				\
	pix = (pix & keepmask) | (solid & ~keepmask);		\
	BLITTER_DEST_WRITE(offset, pix);					\
	(void)srcdata;	/* keeps compiler happy */			\
}


/* early Williams blitters */
#define WILLIAMS_DEST_WRITE(d,v)		if (d < 0x9800) williams_videoram[d] = v; else cpu_writemem16(d, v)
#define WILLIAMS_DEST_READ(d)    		((d < 0x9800) ? williams_videoram[d] : cpu_readmem16(d))

/* Sinistar blitter checks clipping circuit */
#define SINISTAR_DEST_WRITE(d,v)		if (d < sinistar_clip) { if (d < 0x9800) williams_videoram[d] = v; else cpu_writemem16(d, v); }
#define SINISTAR_DEST_READ(d)    		((d < 0x9800) ? williams_videoram[d] : cpu_readmem16(d))

/* Blaster blitter remaps through a lookup table */
#define BLASTER_DEST_WRITE(d,v)			if (d < 0x9700) williams_videoram[d] = v; else cpu_writemem16(d, v)
#define BLASTER_DEST_READ(d)    		((d < 0x9700) ? williams_videoram[d] : cpu_readmem16(d))

/* later Williams blitters */
#define WILLIAMS2_DEST_WRITE(d,v)		if (d < 0x9000 && (williams2_bank & 0x03) != 0x03) williams_videoram[d] = v; else if (d < 0x9000 || d >= 0xc000 || *williams2_blit_inhibit == 0) cpu_writemem16(d, v)
#define WILLIAMS2_DEST_READ(d)    		((d < 0x9000 && (williams2_bank & 0x03) != 0x03) ? williams_videoram[d] : cpu_readmem16(d))

/* to remap or not remap */
#define REMAP_FUNC(r)					blaster_remap[(r) & 0xff]
#define NOREMAP_FUNC(r)					(r)

/* define this so that we get the blitter code when we #include ourself */
#define WILLIAMS_BLITTERS				1


/**************** original williams blitters ****************/
#define BLITTER_DEST_WRITE				WILLIAMS_DEST_WRITE
#define BLITTER_DEST_READ				WILLIAMS_DEST_READ
#define REMAP 							NOREMAP_FUNC

#define BLITTER_OP 						BLIT_TRANSPARENT
#define BLITTER_NAME					williams_blit_transparent
#include "williams.c"
#undef BLITTER_NAME
#undef BLITTER_OP

#define BLITTER_OP 						BLIT_TRANSPARENT_SOLID
#define BLITTER_NAME					williams_blit_transparent_solid
#include "williams.c"
#undef BLITTER_NAME
#undef BLITTER_OP

#define BLITTER_OP 						BLIT_OPAQUE
#define BLITTER_NAME					williams_blit_opaque
#include "williams.c"
#undef BLITTER_NAME
#undef BLITTER_OP

#define BLITTER_OP 						BLIT_OPAQUE_SOLID
#define BLITTER_NAME					williams_blit_opaque_solid
#include "williams.c"
#undef BLITTER_NAME
#undef BLITTER_OP

#undef REMAP
#undef BLITTER_DEST_WRITE
#undef BLITTER_DEST_READ


/**************** Sinistar-specific (clipping) blitters ****************/
#define BLITTER_DEST_WRITE				SINISTAR_DEST_WRITE
#define BLITTER_DEST_READ				SINISTAR_DEST_READ
#define REMAP 							NOREMAP_FUNC

#define BLITTER_OP 						BLIT_TRANSPARENT
#define BLITTER_NAME					sinistar_blit_transparent
#include "williams.c"
#undef BLITTER_NAME
#undef BLITTER_OP

#define BLITTER_OP 						BLIT_TRANSPARENT_SOLID
#define BLITTER_NAME					sinistar_blit_transparent_solid
#include "williams.c"
#undef BLITTER_NAME
#undef BLITTER_OP

#define BLITTER_OP 						BLIT_OPAQUE
#define BLITTER_NAME					sinistar_blit_opaque
#include "williams.c"
#undef BLITTER_NAME
#undef BLITTER_OP

#define BLITTER_OP 						BLIT_OPAQUE_SOLID
#define BLITTER_NAME					sinistar_blit_opaque_solid
#include "williams.c"
#undef BLITTER_NAME
#undef BLITTER_OP

#undef REMAP
#undef BLITTER_DEST_WRITE
#undef BLITTER_DEST_READ


/**************** Blaster-specific (remapping) blitters ****************/
#define BLITTER_DEST_WRITE				BLASTER_DEST_WRITE
#define BLITTER_DEST_READ				BLASTER_DEST_READ
#define REMAP 							REMAP_FUNC

#define BLITTER_OP 						BLIT_TRANSPARENT
#define BLITTER_NAME					blaster_blit_transparent
#include "williams.c"
#undef BLITTER_NAME
#undef BLITTER_OP

#define BLITTER_OP 						BLIT_TRANSPARENT_SOLID
#define BLITTER_NAME					blaster_blit_transparent_solid
#include "williams.c"
#undef BLITTER_NAME
#undef BLITTER_OP

#define BLITTER_OP 						BLIT_OPAQUE
#define BLITTER_NAME					blaster_blit_opaque
#include "williams.c"
#undef BLITTER_NAME
#undef BLITTER_OP

#define BLITTER_OP 						BLIT_OPAQUE_SOLID
#define BLITTER_NAME					blaster_blit_opaque_solid
#include "williams.c"
#undef BLITTER_NAME
#undef BLITTER_OP

#undef REMAP
#undef BLITTER_DEST_WRITE
#undef BLITTER_DEST_READ


/**************** Williams2-specific blitters ****************/
#define BLITTER_DEST_WRITE				WILLIAMS2_DEST_WRITE
#define BLITTER_DEST_READ				WILLIAMS2_DEST_READ
#define REMAP 							NOREMAP_FUNC

#define BLITTER_OP 						BLIT_TRANSPARENT
#define BLITTER_NAME					williams2_blit_transparent
#include "williams.c"
#undef BLITTER_NAME
#undef BLITTER_OP

#define BLITTER_OP 						BLIT_TRANSPARENT_SOLID
#define BLITTER_NAME					williams2_blit_transparent_solid
#include "williams.c"
#undef BLITTER_NAME
#undef BLITTER_OP

#define BLITTER_OP 						BLIT_OPAQUE
#define BLITTER_NAME					williams2_blit_opaque
#include "williams.c"
#undef BLITTER_NAME
#undef BLITTER_OP

#define BLITTER_OP 						BLIT_OPAQUE_SOLID
#define BLITTER_NAME					williams2_blit_opaque_solid
#include "williams.c"
#undef BLITTER_NAME
#undef BLITTER_OP

#undef REMAP
#undef BLITTER_DEST_WRITE
#undef BLITTER_DEST_READ


#else


/*************************************
 *
 *	Blitter cores
 *
 *************************************/

static void BLITTER_NAME(int sstart, int dstart, int w, int h, int data)
{
	int source, sxadv, syadv;
	int dest, dxadv, dyadv;
	int i, j, solid;
	int keepmask;

	/* compute how much to advance in the x and y loops */
	sxadv = (data & 0x01) ? 0x100 : 1;
	syadv = (data & 0x01) ? 1 : w;
	dxadv = (data & 0x02) ? 0x100 : 1;
	dyadv = (data & 0x02) ? 1 : w;

	/* determine the common mask */
	keepmask = 0x00;
	if (data & 0x80) keepmask |= 0xf0;
	if (data & 0x40) keepmask |= 0x0f;
	if (keepmask == 0xff)
		return;

	/* set the solid pixel value to the mask value */
	solid = williams_blitterram[1];

	/* first case: no shifting */
	if (!(data & 0x20))
	{
		/* loop over the height */
		for (i = 0; i < h; i++)
		{
			source = sstart & 0xffff;
			dest = dstart & 0xffff;

			/* loop over the width */
			for (j = w; j > 0; j--)
			{
				int srcdata = cpu_readmem16(source);
				BLITTER_OP(dest, srcdata, keepmask);

				source = (source + sxadv) & 0xffff;
				dest   = (dest + dxadv) & 0xffff;
			}

			sstart += syadv;
			dstart += dyadv;
		}
	}
	/* second case: shifted one pixel */
	else
	{
		/* swap halves of the keep mask and the solid color */
		keepmask = ((keepmask & 0xf0) >> 4) | ((keepmask & 0x0f) << 4);
		solid = ((solid & 0xf0) >> 4) | ((solid & 0x0f) << 4);

		/* loop over the height */
		for (i = 0; i < h; i++)
		{
			int pixdata, srcdata, shiftedmask;

			source = sstart & 0xffff;
			dest = dstart & 0xffff;

			/* left edge case */
			pixdata = cpu_readmem16(source);
			srcdata = (pixdata >> 4) & 0x0f;
			shiftedmask = keepmask | 0xf0;
			BLITTER_OP(dest, srcdata, shiftedmask);

			source = (source + sxadv) & 0xffff;
			dest   = (dest + dxadv) & 0xffff;

			/* loop over the width */
			for (j = w - 1; j > 0; j--)
			{
				pixdata = (pixdata << 8) | cpu_readmem16(source);
				srcdata = (pixdata >> 4) & 0xff;
				BLITTER_OP(dest, srcdata, keepmask);

				source = (source + sxadv) & 0xffff;
				dest   = (dest + dxadv) & 0xffff;
			}

			/* right edge case */
			srcdata = (pixdata << 4) & 0xf0;
			shiftedmask = keepmask | 0x0f;
			BLITTER_OP(dest, srcdata, shiftedmask);

			sstart += syadv;
			dstart += dyadv;
		}
	}
}

#endif

