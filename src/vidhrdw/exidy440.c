/***************************************************************************

	Exidy 440 video system

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

#define EXIDY440_DEBUG		0


/* external globals */
extern unsigned char exidy440_bank;


/* globals */
unsigned char *exidy440_scanline;
unsigned char *exidy440_imageram;
unsigned char *exidy440_latched_x;
unsigned char exidy440_firq_vblank;
unsigned char exidy440_firq_beam;
unsigned char topsecex_yscroll;

/* local allocated storage */
static unsigned char *local_videoram;
static unsigned char *local_paletteram;
static unsigned char *scanline_dirty;

/* local variables */
static unsigned char firq_enable;
static unsigned char firq_select;
static unsigned char palettebank_io;
static unsigned char palettebank_vis;
static unsigned char topsecex_last_yscroll;

/* function prototypes */
void exidy440_vh_stop(void);
void exidy440_update_firq(void);
void exidy440_update_callback(int param);


#if EXIDY440_DEBUG
static void exidy440_dump_video(void);
#endif


/************************************
	Initialize the video system
*************************************/

int exidy440_vh_start(void)
{
	/* reset the system */
	firq_enable = 0;
	firq_select = 0;
	palettebank_io = 0;
	palettebank_vis = 0;
	exidy440_firq_vblank = 0;
	exidy440_firq_beam = 0;

	/* reset Top Secret variables */
	topsecex_yscroll = 0;
	topsecex_last_yscroll = 0;

	/* allocate a buffer for VRAM */
	local_videoram = malloc(256 * 256 * 2);
	if (!local_videoram)
	{
		exidy440_vh_stop();
		return 1;
	}

	/* clear it */
	memset(local_videoram, 0, 256 * 256 * 2);

	/* allocate a buffer for palette RAM */
	local_paletteram = malloc(512 * 2);
	if (!local_paletteram)
	{
		exidy440_vh_stop();
		return 1;
	}

	/* clear it */
	memset(local_paletteram, 0, 512 * 2);

	/* allocate a scanline dirty array */
	scanline_dirty = malloc(256);
	if (!scanline_dirty)
	{
		exidy440_vh_stop();
		return 1;
	}

	/* mark everything dirty to start */
	memset(scanline_dirty, 1, 256);

	return 0;
}


/************************************
	Tear down the video system
*************************************/

void exidy440_vh_stop(void)
{
#if EXIDY440_DEBUG
	exidy440_dump_video();
#endif

	/* free VRAM */
	if (local_videoram)
		free(local_videoram);
	local_videoram = NULL;

	/* free palette RAM */
	if (local_paletteram)
		free(local_paletteram);
	local_paletteram = NULL;

	/* free the scanline dirty array */
	if (scanline_dirty)
		free(scanline_dirty);
	scanline_dirty = NULL;
}


/************************************
	Video RAM read/write
*************************************/

int exidy440_videoram_r(int offset)
{
	unsigned char *base = &local_videoram[(*exidy440_scanline * 256 + offset) * 2];

	/* combine the two pixel values into one byte */
	return (base[0] << 4) | base[1];
}

void exidy440_videoram_w(int offset, int data)
{
	unsigned char *base = &local_videoram[(*exidy440_scanline * 256 + offset) * 2];

	/* expand the two pixel values into two bytes */
	base[0] = (data >> 4) & 15;
	base[1] = data & 15;

	/* mark the scanline dirty */
	scanline_dirty[*exidy440_scanline] = 1;
}


/************************************
	Palette RAM read/write
*************************************/

int exidy440_paletteram_r(int offset)
{
	return local_paletteram[palettebank_io * 512 + offset];
}

void exidy440_paletteram_w(int offset, int data)
{
	if (errorlog) fprintf(errorlog, "W palette[%03X]=%02X\n", offset, data);

	/* update palette ram in the I/O bank */
	local_paletteram[palettebank_io * 512 + offset] = data;

	/* if we're modifying the active palette, change the color immediately */
	if (palettebank_io == palettebank_vis)
	{
		int word;

		/* combine two bytes into a word */
		offset = palettebank_vis * 512 + (offset & 0x1fe);
		word = (local_paletteram[offset] << 8) + local_paletteram[offset + 1];

		/* extract the 5-5-5 RGB colors */
		palette_change_color(offset / 2, ((word >> 10) & 31) << 3, ((word >> 5) & 31) << 3, (word & 31) << 3);
	}
}


/************************************
	Horizontal/vertical positions
*************************************/

int exidy440_horizontal_pos_r(int offset)
{
	if (errorlog) fprintf(errorlog, "R horizontal (%d)\n", *exidy440_latched_x);

	/* clear the FIRQ on a read here */
	exidy440_firq_beam = 0;
	exidy440_update_firq();

	/* according to the schems, this value is only latched on an FIRQ
	 * caused by collision or beam */
	return *exidy440_latched_x;
}

int exidy440_vertical_pos_r(int offset)
{
	int result;

	if (errorlog) fprintf(errorlog, "R vertical (%d)\n", cpu_getscanline());

	/* according to the schems, this value is latched on any FIRQ
	 * caused by collision or beam, ORed together with CHRCLK,
	 * which probably goes off once per scanline; for now, we just
	 * always return the current scanline */
	result = cpu_getscanline();
	return (result < 255) ? result : 255;
}


/************************************
	Interrupt and I/O control regs
*************************************/

void exidy440_control_w(int offset, int data)
{
	int oldvis = palettebank_vis;

	if (!firq_enable && (data & 0x08))
 		if (errorlog) fprintf(errorlog, "FIRQ enabled (time=%20.0f)\n", timer_get_time() * 2000000.);

	/* extract the various bits */
	exidy440_bank = data >> 4;
	firq_enable = (data >> 3) & 1;
	firq_select = (data >> 2) & 1;
	palettebank_io = (data >> 1) & 1;
	palettebank_vis = data & 1;

	/* set the memory bank for the main CPU from the upper 4 bits */
	cpu_setbank(1, &Machine->memory_region[0][0x10000 + exidy440_bank * 0x4000]);

	/* update the FIRQ in case we enabled something */
	exidy440_update_firq();

	/* if we're swapping palettes, change all the colors */
	if (oldvis != palettebank_vis)
	{
		int i;

		/* pick colors from the visible bank */
		offset = palettebank_vis * 512;
		for (i = 0; i < 256; i++, offset += 2)
		{
			/* extract a word and the 5-5-5 RGB components */
			int word = (local_paletteram[offset] << 8) + local_paletteram[offset + 1];
			palette_change_color(i, ((word >> 10) & 31) << 3, ((word >> 5) & 31) << 3, (word & 31) << 3);
		}
	}
}

void exidy440_interrupt_clear_w(int offset, int data)
{
	/* clear the VBLANK FIRQ on a write here */
	exidy440_firq_vblank = 0;
	exidy440_update_firq();
}


/************************************
	Interrupt generators
*************************************/

int exidy440_vblank_interrupt(void)
{
	/* set the FIRQ line on a VBLANK */
	exidy440_firq_vblank = 1;
	exidy440_update_firq();

	/* allocate a timer to go off just before the refresh */
	timer_set(TIME_IN_USEC(Machine->drv->vblank_duration - 50), 0, exidy440_update_callback);

	return 0;
}

int topsecex_vblank_interrupt(void)
{
	/* set the FIRQ line on a VBLANK */
	exidy440_firq_vblank = 1;
	exidy440_update_firq();

	return 0;
}

void exidy440_update_firq(void)
{
	if (exidy440_firq_vblank || (firq_enable && exidy440_firq_beam))
		cpu_set_irq_line(0, 1, ASSERT_LINE);
	else
		cpu_set_irq_line(0, 1, CLEAR_LINE);
}

void beam_firq_callback(int param)
{
/*	if (errorlog) fprintf(errorlog, "Got beam FIRQ at %d,%d (time=%20.0f)\n", cpu_gethorzbeampos(), cpu_getscanline(), timer_get_time() * 2000000.);*/

	/* generate the interrupt, if we're selected */
	if (firq_select && firq_enable)
	{
		exidy440_firq_beam = 1;
		exidy440_update_firq();
	}

	/* round the x value to the nearest byte */
	param = (param + 1) / 2;

	/* latch the x value; this convolution comes from the read routine */
	*exidy440_latched_x = (param + 3) ^ 2;
}

void collide_firq_callback(int param)
{
/*	if (errorlog) fprintf(errorlog, "Got collision FIRQ at %d,%d\n", cpu_gethorzbeampos(), cpu_getscanline());*/

	/* generate the interrupt, if we're selected */
	if (!firq_select && firq_enable)
	{
		exidy440_firq_beam = 1;
		exidy440_update_firq();
	}

	/* round the x value to the nearest byte */
	param = (param + 1) / 2;

	/* latch the x value; this convolution comes from the read routine */
	*exidy440_latched_x = (param + 3) ^ 2;
}


/************************************
	Determine the time when the beam
	will intersect a given pixel
*************************************/

double compute_pixel_time(int x, int y)
{
	/* assuming this is called at refresh time, compute how long until we
	 * hit the given x,y position */
	return cpu_getscanlinetime(y) + (cpu_getscanlineperiod() * (double)x * (1.0 / 320.0));
}


/************************************
	Refresh the screen
*************************************/

void exidy440_update_callback(int param)
{
	/* note: we do most of the work here, because collision detection and beam detection need
		to happen at 60Hz, whether or not we're frameskipping; in order to do those, we pretty
		much need to do all the update handling */

	struct osd_bitmap *bitmap = Machine->scrbitmap;

	unsigned char *palette;
	unsigned char *sprite;
	int x, y, i, count;
	int xoffs, yoffs;
	double time, increment;
	int beamx, beamy;

	/* make sure color 256 is white for our crosshair */
	palette_change_color(256, 0xff, 0xff, 0xff);

	/* recompute the palette, and mark all scanlines dirty if we need to redraw */
	if (palette_recalc())
		memset(scanline_dirty, 1, 256);

	/* draw any dirty scanlines from the VRAM directly */
	for (y = 0; y < 240; y++)
	{
		if (scanline_dirty[y])
		{
			unsigned char *src = &local_videoram[y * 512];
			unsigned char *dst = &bitmap->line[y][0];

			for (x = 0; x < 320; x++)
				*dst++ = Machine->pens[*src++];
			scanline_dirty[y] = 0;
		}
	}

	/* get a pointer to the palette to look for collision flags */
	palette = &local_paletteram[palettebank_vis * 512];
	count = 0;

	/* draw the sprite images, checking for collisions along the way */
	sprite = spriteram + 39 * 4;
	for (i = 0; i < 40; i++, sprite -= 4)
	{
		unsigned char *src;
		int image = (~sprite[3] & 0x3f);
		xoffs = (~((sprite[1] << 8) | sprite[2]) & 0x1ff);
		yoffs = (~sprite[0] & 0xff) + 1;

		/* get a pointer to the source image */
		src = &exidy440_imageram[image * 128];

		/* account for large positive offsets meaning small negative values */
		if (xoffs >= 0x1ff - 16)
			xoffs -= 0x1ff;

		/* loop over y */
		for (y = 0; y < 16; y++, yoffs--)
		{
			if (yoffs >= 0 && yoffs < 240)
			{
				unsigned char *old = &local_videoram[yoffs * 512 + xoffs];
				unsigned char *dst = &bitmap->line[yoffs][xoffs];
				int currx = xoffs;

				/* mark this scanline dirty */
				scanline_dirty[yoffs] = 1;

				/* loop over x */
				for (x = 0; x < 8; x++, dst += 2, old += 2)
				{
					int ipixel = *src++;
					int left = ipixel & 0xf0;
					int right = (ipixel << 4) & 0xf0;
					int pen;

					/* left pixel */
					if (left && currx >= 0 && currx < 320)
					{
						/* combine with the background */
						pen = left | old[0];
						dst[0] = Machine->pens[pen];

						/* check the collisions bit */
						if ((palette[2 * pen] & 0x80) && count++ < 128)
							timer_set(compute_pixel_time(currx, yoffs), currx, collide_firq_callback);
					}
					currx++;

					/* right pixel */
					if (right && currx >= 0 && currx < 320)
					{
						/* combine with the background */
						pen = right | old[1];
						dst[1] = Machine->pens[pen];

						/* check the collisions bit */
						if ((palette[2 * pen] & 0x80) && count++ < 128)
							timer_set(compute_pixel_time(currx, yoffs), currx, collide_firq_callback);
					}
					currx++;
				}
			}
			else
				src += 8;
		}
	}

	/* update the analog x,y values */
	beamx = ((input_port_4_r(0) & 0xff) * 320) >> 8;
	beamy = ((input_port_5_r(0) & 0xff) * 240) >> 8;

	/* The timing of this FIRQ is very important. The games look for an FIRQ
		and then wait about 650 cycles, clear the old FIRQ, and wait a
		very short period of time (~130 cycles) for another one to come in.
		From this, it appears that they are expecting to get beams over
		a 12 scanline period, and trying to pick roughly the middle one.
		This is how it is implemented. */
	increment = cpu_getscanlineperiod();
	time = compute_pixel_time(beamx, beamy) - increment * 6;
	for (i = 0; i <= 12; i++, time += increment)
		timer_set(time, beamx, beam_firq_callback);

/*	if (errorlog) fprintf(errorlog, "Attempt beam FIRQ at %d,%d\n", beamx, beamy);*/

	/* draw a crosshair */
	xoffs = beamx - 3;
	yoffs = beamy - 3;
	for (y = -3; y <= 3; y++, yoffs++, xoffs++)
	{
		if (yoffs >= 0 && yoffs < 240 && beamx >= 0 && beamx < 320)
		{
			scanline_dirty[yoffs] = 1;
			bitmap->line[yoffs][beamx] = Machine->pens[256];
		}
		if (xoffs >= 0 && xoffs < 320 && beamy >= 0 && beamy < 240)
			bitmap->line[beamy][xoffs] = Machine->pens[256];
	}
}

void exidy440_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	/* if we need a full refresh, mark all scanlines dirty */
	if (full_refresh)
		memset(scanline_dirty, 1, 256);
}

void topsecex_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	unsigned char *sprite;
	int x, y, i, sy;
	int xoffs, yoffs;

	/* recompute the palette, and mark all scanlines dirty if we need to redraw */
	if (palette_recalc())
		memset(scanline_dirty, 1, 256);

	/* if the scroll changed, mark everything dirty */
	if (topsecex_yscroll != topsecex_last_yscroll)
	{
		topsecex_last_yscroll = topsecex_yscroll;
		memset(scanline_dirty, 1, 256);
	}

	/* draw any dirty scanlines from the VRAM directly */
	sy = topsecex_yscroll;
	for (y = 0; y < 240; y++, sy++)
	{
		if (sy >= 240)
			sy -= 240;
		if (scanline_dirty[sy])
		{
			unsigned char *src = &local_videoram[sy * 512];
			unsigned char *dst = &bitmap->line[y][0];

			for (x = 0; x < 320; x++)
				*dst++ = Machine->pens[*src++];
			scanline_dirty[sy] = 0;
		}
	}

	/* draw the sprite images */
	sprite = spriteram + 39 * 4;
	for (i = 0; i < 40; i++, sprite -= 4)
	{
		unsigned char *src;
		int image = (~sprite[3] & 0x3f);
		xoffs = (~((sprite[1] << 8) | sprite[2]) & 0x1ff);
		yoffs = (~sprite[0] & 0xff) + 1;

		/* get a pointer to the source image */
		src = &exidy440_imageram[image * 128];

		/* account for large positive offsets meaning small negative values */
		if (xoffs >= 0x1ff - 16)
			xoffs -= 0x1ff;

		/* loop over y */
		sy = yoffs + topsecex_yscroll;
		for (y = 0; y < 16; y++, yoffs--, sy--)
		{
			if (sy >= 240)
				sy -= 240;
			else if (sy < 0)
				sy += 240;

			if (yoffs >= 0 && yoffs < 240)
			{
				unsigned char *old = &local_videoram[sy * 512 + xoffs];
				unsigned char *dst = &bitmap->line[yoffs][xoffs];
				int currx = xoffs;

				/* mark this scanline dirty */
				scanline_dirty[sy] = 1;

				/* loop over x */
				for (x = 0; x < 8; x++, dst += 2, old += 2)
				{
					int ipixel = *src++;
					int left = ipixel & 0xf0;
					int right = (ipixel << 4) & 0xf0;

					/* left pixel */
					if (left && currx >= 0 && currx < 320)
						dst[0] = Machine->pens[left | old[0]];
					currx++;

					/* right pixel */
					if (right && currx >= 0 && currx < 320)
						dst[1] = Machine->pens[right | old[1]];
					currx++;
				}
			}
			else
				src += 8;
		}
	}
}


/************************************
	Debugging
*************************************/

#if EXIDY440_DEBUG

void exidy440_dump_video(void)
{
	unsigned char *sprite = spriteram;
	unsigned char *pal = local_paletteram;
	FILE *f = fopen ("local_videoram.log", "w");

	int x, y;
	for (y = 0; y < 256; y++)
	{
		for (x = 0; x < 256; x++)
		{
			char temp[5];
			sprintf(temp, "%02X", local_videoram[y * 256 + x]);
			if (temp[0] == '0') temp[0] = '.';
			if (temp[1] == '0') temp[1] = '.';
			fprintf (f, "%s", temp);
			if (x % 16 == 15)
				fprintf (f, " ");
		}
		fprintf (f, "\n");
	}

	fprintf(f, "\n\nImages:\n");
	for (y = 0; y < 40; y++, sprite += 4)
	{
		int yoffs = (~sprite[0] & 0xff) + 1;
		int xoffs = (~((sprite[1] << 8) | sprite[2]) & 0x1ff);
		int image = (~sprite[3] & 0x3f);

		fprintf(f, "X=%03X Y=%02X IMG=%02X (%02X %02X %02X %02X)\n",
					xoffs, yoffs, image,
					~sprite[0] & 0xff, ~sprite[1] & 0xff, ~sprite[2] & 0xff, ~sprite[3] & 0xff);
	}

	fprintf(f, "\n\nPalette Bank 1:\n");
	for (y = 0; y < 256; y++, pal += 2)
	{
		int word = (pal[0] << 8) + pal[1];
		fprintf(f, " %04X", word);
		if (y % 16 == 15)
			fprintf(f, "\n");
	}

	fprintf(f, "\n\nPalette Bank 2:\n");
	for (y = 0; y < 256; y++, pal += 2)
	{
		int word = (pal[0] << 8) + pal[1];
		fprintf(f, " %04X", word);
		if (y % 16 == 15)
			fprintf(f, "\n");
	}

	fclose (f);
}

#endif
