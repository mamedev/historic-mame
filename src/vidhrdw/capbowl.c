/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/tms34061.h"
#include "cpu/m6809/m6809.h"

unsigned char *capbowl_rowaddress;

static UINT8 *color_usage;


/*************************************
 *
 *	TMS34061 interfacing
 *
 *************************************/

static void generate_interrupt(int state)
{
	cpu_set_irq_line(0, M6809_FIRQ_LINE, state);
}

static struct tms34061_interface tms34061intf =
{
	8,						/* VRAM address is (row << rowshift) | col */
	0x10000,				/* size of video RAM */
	0x100,					/* size of dirty chunks (must be power of 2) */
	generate_interrupt		/* interrupt gen callback */
};



/*************************************
 *
 *	Video start
 *
 *************************************/

int capbowl_vh_start(void)
{
	/* initialize TMS34061 emulation */
    if (tms34061_start(&tms34061intf))
		return 1;

	/* allocate memory for color tracking */
	color_usage = malloc(256 * 16);
	if (!color_usage)
	{
		tms34061_stop();
		return 1;
	}
	memset(color_usage, 0, sizeof(color_usage));
	return 0;
}



/*************************************
 *
 *	Video stop
 *
 *************************************/

void capbowl_vh_stop(void)
{
	free(color_usage);
	tms34061_stop();
}


/*************************************
 *
 *	TMS34061 I/O
 *
 *************************************/

WRITE_HANDLER( capbowl_tms34061_w )
{
	int func = (offset >> 8) & 3;
	int col = offset & 0xff;

	/* Column address (CA0-CA8) is hooked up the A0-A7, with A1 being inverted
	   during register access. CA8 is ignored */
	if (func == 0 || func == 2)
		col ^= 2;

	/* Row address (RA0-RA8) is not dependent on the offset */
	tms34061_w(col, *capbowl_rowaddress, func, data);
}


READ_HANDLER( capbowl_tms34061_r )
{
	int func = (offset >> 8) & 3;
	int col = offset & 0xff;

	/* Column address (CA0-CA8) is hooked up the A0-A7, with A1 being inverted
	   during register access. CA8 is ignored */
	if (func == 0 || func == 2)
		col ^= 2;

	/* Row address (RA0-RA8) is not dependent on the offset */
	return tms34061_r(col, *capbowl_rowaddress, func);
}


/*************************************
 *
 *	Main refresh
 *
 *************************************/

void capbowl_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	int halfwidth = (Machine->visible_area.max_x - Machine->visible_area.min_x + 1) / 2;
	struct tms34061_display state;
	int x, y, palindex;

	/* first get the current display state */
	tms34061_get_display_state(&state);

	/* if we're blanked, just fill with black */
	if (state.blanked)
	{
		fillbitmap(bitmap, palette_transparent_pen, &Machine->visible_area);
		return;
	}

	/* update the palette and color usage */
	for (y = Machine->visible_area.min_y; y <= Machine->visible_area.max_y; y++)
		if (state.dirty[y])
		{
			UINT8 *src = &state.vram[256 * y];
			UINT8 *usage = &color_usage[16 * y];

			/* update the palette */
			for (x = 0; x < 16; x++)
			{
				int r = *src++ & 0x0f;
				int g = *src >> 4;
				int b = *src++ & 0x0f;

				palette_change_color(y * 16 + x, (r << 4) | r, (g << 4) | g, (b << 4) | b);
			}

			/* recount the colors */
			memset(usage, 0, 16);
			for (x = 0; x < halfwidth; x++)
			{
				int pix = *src++;
				usage[pix >> 4] = 1;
				usage[pix & 0x0f] = 1;
			}
		}

	/* reset the usage */
	palette_init_used_colors();
	palindex = Machine->visible_area.min_y * 16;

	/* mark used colors */
	for (y = Machine->visible_area.min_y; y <= Machine->visible_area.max_y; y++)
		for (x = 0; x < 16; x++, palindex++)
			if (color_usage[palindex])
				palette_used_colors[palindex] = PALETTE_COLOR_USED;

	/* recalc */
	if (palette_recalc())
		full_refresh = 1;

	/* now regenerate the bitmap */
	for (y = Machine->visible_area.min_y; y <= Machine->visible_area.max_y; y++)
		if (full_refresh || state.dirty[y])
		{
			UINT8 *src = &state.vram[256 * y + 32];
			UINT8 scanline[400];
			UINT8 *dst = scanline;

			/* expand row to 8bpp */
			for (x = 0; x < halfwidth; x++)
			{
				int pix = *src++;
				*dst++ = pix >> 4;
				*dst++ = pix & 0x0f;
			}

			/* redraw the scanline and mark it no longer dirty */
			draw_scanline8(bitmap, Machine->visible_area.min_x, y, halfwidth * 2, scanline, &Machine->pens[16 * y], -1);
			state.dirty[y] = 0;
		}
}
