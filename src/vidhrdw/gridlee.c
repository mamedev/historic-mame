/***************************************************************************

	Videa Gridlee hardware

    driver by Aaron Giles

	Based on the Bally/Sente SAC system

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


/*************************************
 *
 *	Globals
 *
 *************************************/

UINT8 gridlee_cocktail_flip;



/*************************************
 *
 *	Statics
 *
 *************************************/

static UINT8 *local_videoram;
static UINT8 *scanline_dirty;
static UINT8 *scanline_palette;

static UINT8 last_scanline_palette;
static UINT8 screen_refresh_counter;
static UINT8 palettebank_vis;



/*************************************
 *
 *	Prototypes
 *
 *************************************/

void gridlee_vh_stop(void);



/*************************************
 *
 *	Color PROM conversion
 *
 *************************************/

void gridlee_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable, const unsigned char *color_prom)
{
	int i;

	for (i = 0; i < Machine->drv->total_colors; i++)
	{
		*palette++ = color_prom[0x0000] | (color_prom[0x0000] << 4);
		*palette++ = color_prom[0x0800] | (color_prom[0x0800] << 4);
		*palette++ = color_prom[0x1000] | (color_prom[0x1000] << 4);
		*colortable++ = i;
		color_prom++;
	}
}



/*************************************
 *
 *	Video system start
 *
 *************************************/

int gridlee_vh_start(void)
{
	/* allocate a local copy of video RAM */
	local_videoram = malloc(256 * 256);
	if (!local_videoram)
	{
		gridlee_vh_stop();
		return 1;
	}

	/* allocate a scanline dirty array */
	scanline_dirty = malloc(256);
	if (!scanline_dirty)
	{
		gridlee_vh_stop();
		return 1;
	}

	/* allocate a scanline palette array */
	scanline_palette = malloc(256);
	if (!scanline_palette)
	{
		gridlee_vh_stop();
		return 1;
	}

	/* mark everything dirty to start */
	memset(scanline_dirty, 1, 256);

	/* reset the scanline palette */
	memset(scanline_palette, 0, 256);
	last_scanline_palette = 0;
	palettebank_vis = -1;

	return 0;
}



/*************************************
 *
 *	Video system shutdown
 *
 *************************************/

void gridlee_vh_stop(void)
{
	/* free the local video RAM array */
	if (local_videoram)
		free(local_videoram);
	local_videoram = NULL;

	/* free the scanline dirty array */
	if (scanline_dirty)
		free(scanline_dirty);
	scanline_dirty = NULL;

	/* free the scanline palette array */
	if (scanline_palette)
		free(scanline_palette);
	scanline_palette = NULL;
}



/*************************************
 *
 *	Cocktail flip
 *
 *************************************/

WRITE_HANDLER( gridlee_cocktail_flip_w )
{
	gridlee_cocktail_flip = data & 1;
	memset(scanline_dirty, 1, 256);
}



/*************************************
 *
 *	Video RAM write
 *
 *************************************/

WRITE_HANDLER( gridlee_videoram_w )
{
	videoram[offset] = data;

	/* expand the two pixel values into two bytes */
	local_videoram[offset * 2 + 0] = data >> 4;
	local_videoram[offset * 2 + 1] = data & 15;

	/* mark the scanline dirty */
	scanline_dirty[offset / 128] = 1;
}



/*************************************
 *
 *	Palette banking
 *
 *************************************/

static void update_palette(void)
{
	int scanline = cpu_getscanline(), i;
	if (scanline > 255) scanline = 0;

logerror("update_palette: %d-%d (%02x)\n", last_scanline_palette, scanline, palettebank_vis);

	/* special case: the scanline is the same as last time, but a screen refresh has occurred */
	if (scanline == last_scanline_palette && screen_refresh_counter)
	{
		for (i = 0; i < 256; i++)
		{
			/* mark the scanline dirty if it was a different palette */
			if (scanline_palette[i] != palettebank_vis)
				scanline_dirty[i] = 1;
			scanline_palette[i] = palettebank_vis;
		}
	}

	/* fill in the scanlines up till now */
	else
	{
		for (i = last_scanline_palette; i != scanline; i = (i + 1) & 255)
		{
			/* mark the scanline dirty if it was a different palette */
			if (scanline_palette[i] != palettebank_vis)
				scanline_dirty[i] = 1;
			scanline_palette[i] = palettebank_vis;
		}

		/* remember where we left off */
		last_scanline_palette = scanline;
	}

	/* reset the screen refresh counter */
	screen_refresh_counter = 0;
}


WRITE_HANDLER( gridlee_palette_select_w )
{
	/* update the scanline palette */
	update_palette();
	palettebank_vis = data & 0x3f;
}



/*************************************
 *
 *	Main screen refresh
 *
 *************************************/

void gridlee_vh_screenrefresh(struct mame_bitmap *bitmap, int full_refresh)
{
	int x, y, i;

logerror("-------------------------------\n");

	/* update the remaining scanlines */
	screen_refresh_counter++;
	update_palette();

	/* full refresh dirties all */
	if (full_refresh)
		memset(scanline_dirty, 1, 240);

	/* draw any dirty scanlines from the VRAM directly */
	for (y = 0; y < 240; y++)
		if (scanline_dirty[y])
		{
			pen_t *pens = &Machine->pens[scanline_palette[y] * 32 + 16];

			/* non-flipped: draw directly from the bitmap */
			if (!gridlee_cocktail_flip)
				draw_scanline8(bitmap, 0, y, 256, &local_videoram[y * 256], pens, -1);

			/* flipped: x-flip the scanline into a temp buffer and draw that */
			else
			{
				UINT8 temp[256];
				int xx;

				for (xx = 0; xx < 256; xx++)
					temp[xx] = local_videoram[y * 256 + 255 - xx];
				draw_scanline8(bitmap, 0, 239 - y, 256, temp, pens, -1);
			}

			scanline_dirty[y] = 0;
		}

	/* draw the sprite images */
	for (i = 0; i < 32; i++)
	{
		UINT8 *sprite = spriteram + i * 4;
		UINT8 *src;
		int image = sprite[0];
		int ypos = sprite[2] + 17;
		int xpos = sprite[3];

		/* get a pointer to the source image */
		src = &memory_region(REGION_GFX1)[64 * image];

		/* loop over y */
		for (y = 0; y < 16; y++, ypos = (ypos + 1) & 255)
		{
			if (ypos >= 16 && ypos < 240)
			{
				pen_t *pens = &Machine->pens[scanline_palette[ypos] * 32];
				int currx = xpos, currxor = 0;

				/* mark this scanline dirty */
				scanline_dirty[ypos] = 1;

				/* adjust for flip */
				if (gridlee_cocktail_flip)
				{
					ypos = 239 - ypos;
					currxor = 0xff;
				}

				/* loop over x */
				for (x = 0; x < 4; x++)
				{
					int ipixel = *src++;
					int left = ipixel >> 4;
					int right = ipixel & 0x0f;

					/* left pixel */
					if (left && currx >= 0 && currx < 256)
						plot_pixel(bitmap, currx ^ currxor, ypos, pens[left]);
					currx++;

					/* right pixel */
					if (right && currx >= 0 && currx < 256)
						plot_pixel(bitmap, currx ^ currxor, ypos, pens[right]);
					currx++;
				}

				/* adjust for flip */
				if (gridlee_cocktail_flip)
					ypos = 239 - ypos;
			}
			else
				src += 4;
		}
	}
}
